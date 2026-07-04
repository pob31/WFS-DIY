/*
    CudaWfsBackend implementation.

    The kernel sources live in CudaWfsKernels.h as a string literal so the app
    needs no .cu file and no nvcc build step; they are compiled at prepare()
    time via NVRTC (~tens of ms once) into PTX, loaded with the CUDA Driver
    API, and launched with cuLaunchKernel. Buffers and copies use the CUDA
    Runtime API. Runtime + driver API share the device's primary context.

    Host-side processBlock mirrors MetalWfsBackend.mm exactly (matrix snapshot
    with -L compensation, prev->curr ramp continuity, persistent device rings,
    host-tracked ring advance), so the audible behaviour matches the Metal twin
    and the CPU reference. Floor-Reflection host work (per-input pre-filter,
    diffusion jitter, FR matrix snapshot) lives in the shared WfsFrHostState.

    Threading note: prepare() runs on the engine-setup thread, but processBlock()
    runs on the GpuAsyncPipeline pump thread. CUDA driver-API context currency is
    per-thread, so processBlock() binds the primary context (cuCtxSetCurrent) at
    the top - cheap (a TLS write) and makes both the driver launch and the runtime
    copies use the same context on the pump thread.
*/

// This translation unit is in the shared file list for every exporter, but only
// the Windows/NVIDIA build (WFS_GPU_NATIVE, non-Apple) pulls in the CUDA toolkit.
// On macOS the Metal backend is used; on Linux the GPU path is off - either way
// this compiles to an empty TU so cuda.h is never required there.
#if WFS_GPU_NATIVE && !defined(__APPLE__) && !defined(WFS_GPU_HIP) && !defined(WFS_GPU_PLUGINS)

#include "CudaWfsBackend.h"
#include "CudaWfsKernels.h"
#include "WfsFrHostState.h"

#include <cuda.h>           // driver API: CUcontext, cuModule*, cuLaunchKernel
#include <cuda_runtime.h>   // runtime API: cudaMalloc, cudaHostAlloc, cudaMemcpyAsync, props
#include <nvrtc.h>

// Projucer's externalLibraries field does not reach AdditionalDependencies for
// this project (setupapi.lib is likewise linked via a pragma in hidapi), so link
// the CUDA import libs here. libraryPath ($(CUDA_PATH)\lib\x64) is on the search
// path via the .jucer, so the linker resolves these.
#if defined(_MSC_VER)
 #pragma comment(lib, "cudart.lib")
 #pragma comment(lib, "nvrtc.lib")
 #pragma comment(lib, "cuda.lib")

 // The CUDA DLLs are DELAY-LOADED (the /DELAYLOAD flags live in the VS exporter's
 // extraLinkerFlags / the .vcxproj <Link><AdditionalOptions> -- the linker does
 // NOT honour /DELAYLOAD via #pragma comment(linker)). Without delay-load the
 // three DLLs sit in the exe's import table and the loader fails at startup when
 // nvcuda.dll is absent (AMD / Intel / no GPU), stranding CPU-only users.
 // Delay-loaded, they load only on first use: every GPU backend calls
 // cudaGetDeviceCount first -- cudart (shipped next to the exe) loads fine and
 // reports "no driver" gracefully, so the engine falls back to the CPU path;
 // nvcuda is only reached once a device is confirmed present. delayimp.lib
 // provides the delay-load helper (__delayLoadHelper2); the comment(lib) pragma
 // IS honoured, so it stays here next to the other CUDA libs.
 #pragma comment(lib, "delayimp.lib")
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

// Optional per-stage host timers (GPU host-path optimization M0 deep-dive
// tool). Default OFF: rebuild the vendor plugin with /DWFS_GPU_STAGE_TIMERS=1
// to get a stderr line every 512 blocks with per-stage mean ms
// {snapshot, frPrep, uploadIssue, wait, unpack}. When off this preprocesses
// to nothing — lastLaunchMs semantics untouched; NOT exposed via IGpuBackend.
#ifndef WFS_GPU_STAGE_TIMERS
 #define WFS_GPU_STAGE_TIMERS 0
#endif
#if WFS_GPU_STAGE_TIMERS
 #include <cstdio>
 #define WFS_STAGE_MARK(name) const auto name = std::chrono::steady_clock::now()
#else
 #define WFS_STAGE_MARK(name)
#endif

namespace spatcore::gpu {

namespace
{
// Host mirror of the kernel-side WfsParams - layouts must match exactly.
struct WfsParamsGpu
{
    uint32_t numInputs;
    uint32_t numOutputs;
    uint32_t bufferLength;
    uint32_t ringCapacity;
    uint32_t ringWritePos;
    uint32_t ringValidSamples;
    uint32_t pairGroups;
    float    shelfCosW0;
    float    shelfSinW0;
};
} // namespace

struct CudaWfsBackend::Impl
{
    CUcontext    context = nullptr;   // device primary context (retained)
    CUdevice     cuDevice = 0;
    CUmodule     module = nullptr;
    CUfunction   kernelPairs = nullptr;
    CUfunction   kernelReduce = nullptr;
    cudaStream_t stream = nullptr;

    // Pinned host staging.
    float* hIn = nullptr;
    float* hFrIn = nullptr;
    float* hOut = nullptr;
    float* hDelaysPrev = nullptr;
    float* hDelaysCurr = nullptr;
    float* hGainsPrev = nullptr;
    float* hGainsCurr = nullptr;
    float* hFrDelaysPrev = nullptr;
    float* hFrDelaysCurr = nullptr;
    float* hFrGainsPrev = nullptr;
    float* hFrGainsCurr = nullptr;
    float* hHfAttenDb = nullptr;
    float* hFrHfAttenDb = nullptr;

    // Device buffers (rings + shelf states persist across launches).
    void* dIn = nullptr;
    void* dFrIn = nullptr;
    void* dOut = nullptr;
    void* dRing = nullptr;
    void* dFrRing = nullptr;
    void* dScratch = nullptr;        // [(s*numOut+out)*numIn+in], rewritten each launch
    void* dShelfState = nullptr;     // [pairs][4] persistent
    void* dFrShelfState = nullptr;   // [pairs][4] persistent
    void* dDelaysPrev = nullptr;
    void* dDelaysCurr = nullptr;
    void* dGainsPrev = nullptr;
    void* dGainsCurr = nullptr;
    void* dFrDelaysPrev = nullptr;
    void* dFrDelaysCurr = nullptr;
    void* dFrGainsPrev = nullptr;
    void* dFrGainsCurr = nullptr;
    void* dHfAttenDb = nullptr;
    void* dFrHfAttenDb = nullptr;

    int deviceIndex = 0;             // which CUDA device to bind (ctor-injected)
    int numIn = 0, numOut = 0, blockSize = 0;
    uint32_t ringCapacity = 0;
    uint32_t ringWritePos = 0;
    uint32_t ringValid = 0;
    uint32_t maxDelaySamples = 0;
    uint32_t pairGroups = 0;
    double sampleRate = 0.0;
    float latencyMs = 0.0f;
    float shelfCosW0 = 1.0f;
    float shelfSinW0 = 0.0f;

    // App's live matrices (input-major). FR/HF pointers may be null.
    const float* delaysMs = nullptr;
    const float* gains = nullptr;
    const float* hfAttenDb = nullptr;
    const float* frDelaysMs = nullptr;
    const float* frLevels = nullptr;
    const float* frHfAttenDb = nullptr;

    // Last launch's end matrices (ramp continuity).
    std::vector<float> delaysPrevSamples;
    std::vector<float> gainsPrev;
    std::vector<float> frDelaysPrevSamples;
    std::vector<float> frGainsPrev;
    bool havePrev = false;

    WfsFrHostState frHost;            // per-input FR pre-filters + jitter

#if WFS_GPU_STAGE_TIMERS
    // Per-stage accumulators (pump thread only; printed/reset every 512 blocks)
    double stSnapshotMs = 0.0, stFrPrepMs = 0.0, stUploadIssueMs = 0.0,
           stWaitMs = 0.0, stUnpackMs = 0.0;
    uint32_t stBlocks = 0;
#endif

    unsigned int threadsPerBlock = 256;
};

CudaWfsBackend::CudaWfsBackend (int deviceIndex) : impl (std::make_unique<Impl>()) { impl->deviceIndex = deviceIndex; }
CudaWfsBackend::~CudaWfsBackend() { release(); }

// prepare()-only error helpers: set lastError, tear down, return false.
#define CK_RT(call)  do { cudaError_t _e = (call); if (_e != cudaSuccess) { \
    lastError = std::string ("CUDA runtime: ") + cudaGetErrorString (_e); release(); return false; } } while (0)
#define CK_DRV(call) do { CUresult _e = (call); if (_e != CUDA_SUCCESS) { const char* _s = nullptr; \
    cuGetErrorString (_e, &_s); lastError = std::string ("CUDA driver: ") + (_s ? _s : "unknown"); \
    release(); return false; } } while (0)

bool CudaWfsBackend::prepare (int numInputs, int numOutputs, int blockSize,
                              double sampleRate, double pipelineLatencyMs,
                              double maxDelaySeconds)
{
    release();
    auto& m = *impl;

    // 1) Pick the selected device, report its name, derive the SM architecture.
    //    The runtime ordinal (cudaSetDevice) and driver ordinal (cuDeviceGet)
    //    agree under the default CUDA_DEVICE_ORDER; for two identical GPUs the
    //    fastest-first tie resolves to PCI-bus order, so they always match.
    int devCount = 0;
    CK_RT (cudaGetDeviceCount (&devCount));
    if (devCount == 0)
    {
        lastError = "No CUDA device available";
        return false;
    }
    if (m.deviceIndex < 0 || m.deviceIndex >= devCount)
    {
        lastError = "CUDA device index " + std::to_string (m.deviceIndex)
                    + " out of range (" + std::to_string (devCount) + " present)";
        return false;
    }
    CK_RT (cudaSetDevice (m.deviceIndex));

    cudaDeviceProp prop;
    CK_RT (cudaGetDeviceProperties (&prop, m.deviceIndex));
    deviceName = std::string (prop.name) + " (CUDA)";
    const int arch = prop.major * 10 + prop.minor; // e.g. Turing GTX 1650 -> 75

    // 2) Init the driver API and retain the same primary context the runtime uses.
    CK_DRV (cuInit (0));
    CK_DRV (cuDeviceGet (&m.cuDevice, m.deviceIndex));
    CK_DRV (cuDevicePrimaryCtxRetain (&m.context, m.cuDevice));
    CK_DRV (cuCtxSetCurrent (m.context));

    // 3) NVRTC-compile the kernel string to cubin for this GPU's arch.
    {
        nvrtcProgram prog = nullptr;
        if (nvrtcCreateProgram (&prog, kWfsDelaySumKernelSource, "wfs_delay_sum.cu", 0, nullptr, nullptr) != NVRTC_SUCCESS)
        {
            lastError = "NVRTC: program creation failed";
            release();
            return false;
        }

        // sm_ (not compute_): emit arch-exact SASS and skip the driver's PTX JIT,
        // which rejects PTX from an NVRTC newer than the driver
        // (CUDA_ERROR_UNSUPPORTED_PTX_VERSION). With cubin the min-driver bar is
        // "supports this GPU", not "at least as new as the bundled NVRTC".
        const std::string archOpt = "--gpu-architecture=sm_" + std::to_string (arch);
        const char* opts[] = { archOpt.c_str() };
        const nvrtcResult comp = nvrtcCompileProgram (prog, 1, opts);
        if (comp != NVRTC_SUCCESS)
        {
            size_t logSize = 0;
            nvrtcGetProgramLogSize (prog, &logSize);
            std::string log (logSize, '\0');
            if (logSize > 0)
                nvrtcGetProgramLog (prog, &log[0]);
            lastError = std::string ("NVRTC compile failed: ") + log;
            nvrtcDestroyProgram (&prog);
            release();
            return false;
        }

        size_t cubinSize = 0;
        nvrtcGetCUBINSize (prog, &cubinSize);
        std::vector<char> cubin (cubinSize);
        nvrtcGetCUBIN (prog, cubin.data());
        nvrtcDestroyProgram (&prog);

        CK_DRV (cuModuleLoadDataEx (&m.module, cubin.data(), 0, nullptr, nullptr));
        CK_DRV (cuModuleGetFunction (&m.kernelPairs, m.module, "wfs_pairs"));
        CK_DRV (cuModuleGetFunction (&m.kernelReduce, m.module, "wfs_reduce"));
    }

    // 4) Geometry + sizing (identical to the Metal backend).
    m.numIn = std::max (1, numInputs);
    m.numOut = std::max (1, numOutputs);
    m.blockSize = std::max (1, blockSize);
    m.sampleRate = sampleRate;
    m.latencyMs = (float) pipelineLatencyMs;
    m.maxDelaySamples = (uint32_t) (maxDelaySeconds * sampleRate);
    m.ringCapacity = m.maxDelaySamples + (uint32_t) m.blockSize;
    m.ringWritePos = 0;
    m.ringValid = 0;
    m.havePrev = false;
    m.threadsPerBlock = 256;
    m.pairGroups = (uint32_t) ((m.numIn * m.numOut + (int) m.threadsPerBlock - 1)
                               / (int) m.threadsPerBlock);

    // Fixed 800 Hz shelf frequency (WFSHighShelfFilter parity).
    const double w0 = 2.0 * 3.14159265358979 * 800.0 / sampleRate;
    m.shelfCosW0 = (float) std::cos (w0);
    m.shelfSinW0 = (float) std::sin (w0);

    const uint32_t matrix = (uint32_t) (m.numIn * m.numOut);

    // 5) Stream + pinned host staging + device buffers (+ zero persistent state).
    CK_RT (cudaStreamCreate (&m.stream));

    auto pin = [] (float** p, size_t n) {
        return cudaHostAlloc ((void**) p, n * sizeof (float), cudaHostAllocDefault);
    };
    CK_RT (pin (&m.hIn,   (size_t) m.numIn  * m.blockSize));
    CK_RT (pin (&m.hFrIn, (size_t) m.numIn  * m.blockSize));
    CK_RT (pin (&m.hOut,  (size_t) m.numOut * m.blockSize));
    CK_RT (pin (&m.hDelaysPrev, matrix));
    CK_RT (pin (&m.hDelaysCurr, matrix));
    CK_RT (pin (&m.hGainsPrev,  matrix));
    CK_RT (pin (&m.hGainsCurr,  matrix));
    CK_RT (pin (&m.hFrDelaysPrev, matrix));
    CK_RT (pin (&m.hFrDelaysCurr, matrix));
    CK_RT (pin (&m.hFrGainsPrev,  matrix));
    CK_RT (pin (&m.hFrGainsCurr,  matrix));
    CK_RT (pin (&m.hHfAttenDb,    matrix));
    CK_RT (pin (&m.hFrHfAttenDb,  matrix));

    CK_RT (cudaMalloc (&m.dIn,   (size_t) m.numIn  * m.blockSize * sizeof (float)));
    CK_RT (cudaMalloc (&m.dFrIn, (size_t) m.numIn  * m.blockSize * sizeof (float)));
    CK_RT (cudaMalloc (&m.dOut,  (size_t) m.numOut * m.blockSize * sizeof (float)));
    CK_RT (cudaMalloc (&m.dRing,   (size_t) m.numIn * m.ringCapacity * sizeof (float)));
    CK_RT (cudaMalloc (&m.dFrRing, (size_t) m.numIn * m.ringCapacity * sizeof (float)));
    CK_RT (cudaMalloc (&m.dScratch, (size_t) m.numIn * m.numOut * m.blockSize * sizeof (float)));
    CK_RT (cudaMalloc (&m.dShelfState,   (size_t) matrix * 4 * sizeof (float)));
    CK_RT (cudaMalloc (&m.dFrShelfState, (size_t) matrix * 4 * sizeof (float)));
    CK_RT (cudaMalloc (&m.dDelaysPrev, matrix * sizeof (float)));
    CK_RT (cudaMalloc (&m.dDelaysCurr, matrix * sizeof (float)));
    CK_RT (cudaMalloc (&m.dGainsPrev,  matrix * sizeof (float)));
    CK_RT (cudaMalloc (&m.dGainsCurr,  matrix * sizeof (float)));
    CK_RT (cudaMalloc (&m.dFrDelaysPrev, matrix * sizeof (float)));
    CK_RT (cudaMalloc (&m.dFrDelaysCurr, matrix * sizeof (float)));
    CK_RT (cudaMalloc (&m.dFrGainsPrev,  matrix * sizeof (float)));
    CK_RT (cudaMalloc (&m.dFrGainsCurr,  matrix * sizeof (float)));
    CK_RT (cudaMalloc (&m.dHfAttenDb,    matrix * sizeof (float)));
    CK_RT (cudaMalloc (&m.dFrHfAttenDb,  matrix * sizeof (float)));

    CK_RT (cudaMemset (m.dRing,   0, (size_t) m.numIn * m.ringCapacity * sizeof (float)));
    CK_RT (cudaMemset (m.dFrRing, 0, (size_t) m.numIn * m.ringCapacity * sizeof (float)));
    CK_RT (cudaMemset (m.dShelfState,   0, (size_t) matrix * 4 * sizeof (float)));
    CK_RT (cudaMemset (m.dFrShelfState, 0, (size_t) matrix * 4 * sizeof (float)));

    m.delaysPrevSamples.assign (matrix, 0.0f);
    m.gainsPrev.assign (matrix, 0.0f);
    m.frDelaysPrevSamples.assign (matrix, 0.0f);
    m.frGainsPrev.assign (matrix, 0.0f);

    m.frHost.prepare (m.numIn, m.numOut, sampleRate);

    ready = true;
    lastError.clear();
    return true;
}

void CudaWfsBackend::setMatrixPointers (const float* delaysMsPtr, const float* gainsPtr,
                                        const float* hfAttenDbPtr,
                                        const float* frDelaysMsPtr,
                                        const float* frLevelsPtr,
                                        const float* frHfAttenDbPtr) noexcept
{
    impl->delaysMs = delaysMsPtr;
    impl->gains = gainsPtr;
    impl->hfAttenDb = hfAttenDbPtr;
    impl->frDelaysMs = frDelaysMsPtr;
    impl->frLevels = frLevelsPtr;
    impl->frHfAttenDb = frHfAttenDbPtr;
}

void CudaWfsBackend::setFRFilterParams (int inputIndex,
                                        bool lowCutActive, float lowCutFreq,
                                        bool highShelfActive, float highShelfFreq,
                                        float highShelfGain, float highShelfSlope) noexcept
{
    impl->frHost.setFRFilterParams (inputIndex, lowCutActive, lowCutFreq,
                                    highShelfActive, highShelfFreq,
                                    highShelfGain, highShelfSlope);
}

void CudaWfsBackend::setFRDiffusion (int inputIndex, float diffusionPercent) noexcept
{
    impl->frHost.setFRDiffusion (inputIndex, diffusionPercent);
}

bool CudaWfsBackend::processBlock (const float* const* inputs, float* const* outputs)
{
    if (! ready)
        return false;

    auto& m = *impl;
    const uint32_t matrix = (uint32_t) (m.numIn * m.numOut);
    const float srScale = (float) (m.sampleRate / 1000.0);
    const float maxDelay = (float) m.maxDelaySamples;

    // processBlock runs on the pump thread; bind the primary context here so
    // the driver launch uses it, and bind the runtime current-device so the
    // runtime copies (cudaMemcpyAsync / cudaStreamSynchronize on m.stream) also
    // target the selected device (the stream is bound to the device it was
    // created on). Both are cheap per-thread writes.
    if (cuCtxSetCurrent (m.context) != CUDA_SUCCESS)
    {
        lastError = "CUDA driver: cuCtxSetCurrent failed on pump thread";
        ready = false;
        return false;
    }
    cudaSetDevice (m.deviceIndex);

    const auto t0 = std::chrono::steady_clock::now();

    // Snapshot the live matrices -> curr (with -L compensation, clamped),
    // prev = the previous launch's curr (ramp continuity).
    float* dCurr = m.hDelaysCurr;
    float* gCurr = m.hGainsCurr;
    for (uint32_t i = 0; i < matrix; ++i)
    {
        float d = m.delaysMs != nullptr ? (m.delaysMs[i] - m.latencyMs) * srScale : 0.0f;
        dCurr[i] = std::clamp (d, 0.0f, maxDelay);
        gCurr[i] = m.gains != nullptr ? m.gains[i] : 0.0f;
    }

    // Shelf gains: raw dB, stepwise per launch (CPU parity: per-block setGainDb).
    for (uint32_t i = 0; i < matrix; ++i)
    {
        m.hHfAttenDb[i] = m.hfAttenDb != nullptr ? m.hfAttenDb[i] : 0.0f;
        m.hFrHfAttenDb[i] = m.frHfAttenDb != nullptr ? m.frHfAttenDb[i] : 0.0f;
    }
    WFS_STAGE_MARK (stA);   // snapshot: matrix + shelf staging

    // FR: advance diffusion jitter (64-sample sub-step cadence), then snapshot
    // the FR curr matrices. The pipeline latency is subtracted from the
    // ABSOLUTE FR delay (direct + extra + jitter - L), preserving the
    // FR-vs-direct offset exactly.
    m.frHost.advanceJitter (m.blockSize);
    m.frHost.computeFrCurr (m.delaysMs, m.frDelaysMs, m.frLevels,
                            m.latencyMs, srScale, maxDelay,
                            m.hFrDelaysCurr, m.hFrGainsCurr);
    WFS_STAGE_MARK (stB);   // frPrep: jitter advance + FR curr snapshot

    if (! m.havePrev)
    {
        std::memcpy (m.delaysPrevSamples.data(), dCurr, matrix * sizeof (float));
        std::memcpy (m.gainsPrev.data(), gCurr, matrix * sizeof (float));
        std::memcpy (m.frDelaysPrevSamples.data(), m.hFrDelaysCurr, matrix * sizeof (float));
        std::memcpy (m.frGainsPrev.data(), m.hFrGainsCurr, matrix * sizeof (float));
        m.havePrev = true;
    }
    std::memcpy (m.hDelaysPrev, m.delaysPrevSamples.data(), matrix * sizeof (float));
    std::memcpy (m.hGainsPrev, m.gainsPrev.data(), matrix * sizeof (float));
    std::memcpy (m.hFrDelaysPrev, m.frDelaysPrevSamples.data(), matrix * sizeof (float));
    std::memcpy (m.hFrGainsPrev, m.frGainsPrev.data(), matrix * sizeof (float));
    WFS_STAGE_MARK (stC);   // snapshot: prev staging memcpys

    // Input channels -> flat pinned buffer (silence for missing channels),
    // and the host-side FR pre-filter chain -> frIn staging.
    for (int ch = 0; ch < m.numIn; ++ch)
    {
        if (inputs[ch] != nullptr)
            std::memcpy (m.hIn + (size_t) ch * m.blockSize, inputs[ch], (size_t) m.blockSize * sizeof (float));
        else
            std::memset (m.hIn + (size_t) ch * m.blockSize, 0, (size_t) m.blockSize * sizeof (float));
    }
    WFS_STAGE_MARK (stD);   // uploadIssue: input pack
    m.frHost.filterBlock (inputs, m.hFrIn, m.blockSize);
    WFS_STAGE_MARK (stE);   // frPrep: FR pre-filter chain

    // Host -> device (the persistent rings + shelf states stay on the device).
#define PB_RT(call) do { cudaError_t _e = (call); if (_e != cudaSuccess) { \
    lastError = std::string ("CUDA runtime: ") + cudaGetErrorString (_e); ready = false; return false; } } while (0)

    auto up = [&m] (void* dst, const float* src, size_t floats) {
        return cudaMemcpyAsync (dst, src, floats * sizeof (float), cudaMemcpyHostToDevice, m.stream);
    };
    PB_RT (up (m.dIn,   m.hIn,   (size_t) m.numIn * m.blockSize));
    PB_RT (up (m.dFrIn, m.hFrIn, (size_t) m.numIn * m.blockSize));
    PB_RT (up (m.dDelaysPrev, m.hDelaysPrev, matrix));
    PB_RT (up (m.dDelaysCurr, m.hDelaysCurr, matrix));
    PB_RT (up (m.dGainsPrev, m.hGainsPrev, matrix));
    PB_RT (up (m.dGainsCurr, m.hGainsCurr, matrix));
    PB_RT (up (m.dFrDelaysPrev, m.hFrDelaysPrev, matrix));
    PB_RT (up (m.dFrDelaysCurr, m.hFrDelaysCurr, matrix));
    PB_RT (up (m.dFrGainsPrev, m.hFrGainsPrev, matrix));
    PB_RT (up (m.dFrGainsCurr, m.hFrGainsCurr, matrix));
    PB_RT (up (m.dHfAttenDb, m.hHfAttenDb, matrix));
    PB_RT (up (m.dFrHfAttenDb, m.hFrHfAttenDb, matrix));

    WfsParamsGpu p { (uint32_t) m.numIn, (uint32_t) m.numOut, (uint32_t) m.blockSize,
                     m.ringCapacity, m.ringWritePos, m.ringValid,
                     m.pairGroups, m.shelfCosW0, m.shelfSinW0 };

    void* pairsArgs[] = { &p, &m.dIn, &m.dFrIn, &m.dRing, &m.dFrRing,
                          &m.dDelaysPrev, &m.dDelaysCurr, &m.dGainsPrev, &m.dGainsCurr,
                          &m.dFrDelaysPrev, &m.dFrDelaysCurr, &m.dFrGainsPrev, &m.dFrGainsCurr,
                          &m.dHfAttenDb, &m.dFrHfAttenDb,
                          &m.dShelfState, &m.dFrShelfState, &m.dScratch };

    // K1: pair role + both ring appends.
    const unsigned int gridPairs = m.pairGroups + 2u * (unsigned int) m.numIn;
    CUresult lr = cuLaunchKernel (m.kernelPairs,
                                  gridPairs, 1, 1,
                                  m.threadsPerBlock, 1, 1,
                                  0, (CUstream) m.stream,
                                  pairsArgs, nullptr);
    if (lr != CUDA_SUCCESS)
    {
        const char* s = nullptr;
        cuGetErrorString (lr, &s);
        lastError = std::string ("CUDA launch failed (pairs): ") + (s ? s : "unknown");
        ready = false;
        return false;
    }

    // K2: deterministic per-output reduction over the scratch (stream-ordered
    // after K1).
    void* reduceArgs[] = { &p, &m.dScratch, &m.dOut };
    lr = cuLaunchKernel (m.kernelReduce,
                         (unsigned int) m.numOut, 1, 1,
                         m.threadsPerBlock, 1, 1,
                         0, (CUstream) m.stream,
                         reduceArgs, nullptr);
    if (lr != CUDA_SUCCESS)
    {
        const char* s = nullptr;
        cuGetErrorString (lr, &s);
        lastError = std::string ("CUDA launch failed (reduce): ") + (s ? s : "unknown");
        ready = false;
        return false;
    }

    PB_RT (cudaMemcpyAsync (m.hOut, m.dOut, (size_t) m.numOut * m.blockSize * sizeof (float), cudaMemcpyDeviceToHost, m.stream));
    WFS_STAGE_MARK (stF);   // uploadIssue: H2D uploads + launches + D2H issue
    PB_RT (cudaStreamSynchronize (m.stream));
    WFS_STAGE_MARK (stG);   // wait: stream sync

#undef PB_RT

    // Output pinned buffer -> channels.
    for (int ch = 0; ch < m.numOut; ++ch)
        if (outputs[ch] != nullptr)
            std::memcpy (outputs[ch], m.hOut + (size_t) ch * m.blockSize, (size_t) m.blockSize * sizeof (float));

    // Advance host-tracked state.
    m.ringWritePos = (m.ringWritePos + (uint32_t) m.blockSize) % m.ringCapacity;
    m.ringValid = std::min (m.maxDelaySamples, m.ringValid + (uint32_t) m.blockSize);
    std::memcpy (m.delaysPrevSamples.data(), dCurr, matrix * sizeof (float));
    std::memcpy (m.gainsPrev.data(), gCurr, matrix * sizeof (float));
    std::memcpy (m.frDelaysPrevSamples.data(), m.hFrDelaysCurr, matrix * sizeof (float));
    std::memcpy (m.frGainsPrev.data(), m.hFrGainsCurr, matrix * sizeof (float));

#if WFS_GPU_STAGE_TIMERS
    {
        const auto stH = std::chrono::steady_clock::now();   // unpack end
        auto ms = [] (auto a, auto b) { return std::chrono::duration<double, std::milli> (b - a).count(); };
        m.stSnapshotMs    += ms (t0, stA) + ms (stB, stC);
        m.stFrPrepMs      += ms (stA, stB) + ms (stD, stE);
        m.stUploadIssueMs += ms (stC, stD) + ms (stE, stF);
        m.stWaitMs        += ms (stF, stG);
        m.stUnpackMs      += ms (stG, stH);
        if (++m.stBlocks == 512)
        {
            const double inv = 1.0 / 512.0;
            std::fprintf (stderr, "[wfs-cuda stages, mean ms over 512 blocks] "
                          "snapshot=%.4f frPrep=%.4f uploadIssue=%.4f wait=%.4f unpack=%.4f\n",
                          m.stSnapshotMs * inv, m.stFrPrepMs * inv, m.stUploadIssueMs * inv,
                          m.stWaitMs * inv, m.stUnpackMs * inv);
            m.stSnapshotMs = m.stFrPrepMs = m.stUploadIssueMs = m.stWaitMs = m.stUnpackMs = 0.0;
            m.stBlocks = 0;
        }
    }
#endif

    lastLaunchMs = std::chrono::duration<double, std::milli> (
                       std::chrono::steady_clock::now() - t0).count();
    return true;
}

void CudaWfsBackend::reset() noexcept
{
    auto& m = *impl;
    if (m.context != nullptr)
    {
        cuCtxSetCurrent (m.context);
        cudaSetDevice (m.deviceIndex);   // runtime cudaMemset below targets the selected device
    }
    const size_t ringBytes = (size_t) m.numIn * m.ringCapacity * sizeof (float);
    const size_t stateBytes = (size_t) m.numIn * m.numOut * 4 * sizeof (float);
    if (m.dRing != nullptr && ringBytes > 0)
        cudaMemset (m.dRing, 0, ringBytes);
    if (m.dFrRing != nullptr && ringBytes > 0)
        cudaMemset (m.dFrRing, 0, ringBytes);
    if (m.dShelfState != nullptr && stateBytes > 0)
        cudaMemset (m.dShelfState, 0, stateBytes);
    if (m.dFrShelfState != nullptr && stateBytes > 0)
        cudaMemset (m.dFrShelfState, 0, stateBytes);
    m.frHost.reset();
    m.ringWritePos = 0;
    m.ringValid = 0;
    m.havePrev = false;
}

void CudaWfsBackend::release() noexcept
{
    auto& m = *impl;

    if (m.context != nullptr)
        cuCtxSetCurrent (m.context);

    auto freeHost = [] (float*& p) { if (p != nullptr) { cudaFreeHost (p); p = nullptr; } };
    auto freeDev  = [] (void*&  p) { if (p != nullptr) { cudaFree (p);     p = nullptr; } };

    freeHost (m.hIn);  freeHost (m.hFrIn);  freeHost (m.hOut);
    freeHost (m.hDelaysPrev); freeHost (m.hDelaysCurr);
    freeHost (m.hGainsPrev);  freeHost (m.hGainsCurr);
    freeHost (m.hFrDelaysPrev); freeHost (m.hFrDelaysCurr);
    freeHost (m.hFrGainsPrev);  freeHost (m.hFrGainsCurr);
    freeHost (m.hHfAttenDb);    freeHost (m.hFrHfAttenDb);

    freeDev (m.dIn);  freeDev (m.dFrIn);  freeDev (m.dOut);
    freeDev (m.dRing);  freeDev (m.dFrRing);
    freeDev (m.dScratch);
    freeDev (m.dShelfState); freeDev (m.dFrShelfState);
    freeDev (m.dDelaysPrev); freeDev (m.dDelaysCurr);
    freeDev (m.dGainsPrev);  freeDev (m.dGainsCurr);
    freeDev (m.dFrDelaysPrev); freeDev (m.dFrDelaysCurr);
    freeDev (m.dFrGainsPrev);  freeDev (m.dFrGainsCurr);
    freeDev (m.dHfAttenDb);    freeDev (m.dFrHfAttenDb);

    if (m.stream != nullptr) { cudaStreamDestroy (m.stream); m.stream = nullptr; }
    if (m.module != nullptr) { cuModuleUnload (m.module); m.module = nullptr; }
    m.kernelPairs = nullptr;
    m.kernelReduce = nullptr;

    if (m.context != nullptr) { cuDevicePrimaryCtxRelease (m.cuDevice); m.context = nullptr; }

    m.delaysMs = nullptr;
    m.gains = nullptr;
    m.hfAttenDb = nullptr;
    m.frDelaysMs = nullptr;
    m.frLevels = nullptr;
    m.frHfAttenDb = nullptr;
    m.havePrev = false;
    ready = false;
}

#undef CK_RT
#undef CK_DRV

} // namespace spatcore::gpu

#endif // WFS_GPU_NATIVE && !defined(__APPLE__)
