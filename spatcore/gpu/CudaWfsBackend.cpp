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
#include "GpuHostWorkPool.h"

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
    cudaEvent_t  syncEvent = nullptr;  // blocking-sync end-of-block wait (no spin)

    // Pinned host staging (curr only — prev never leaves the device, see below).
    float* hIn = nullptr;
    float* hFrIn = nullptr;
    float* hOut = nullptr;
    float* hDelaysCurr = nullptr;
    float* hGainsCurr = nullptr;
    float* hFrDelaysCurr = nullptr;
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
    void* dHfAttenDb = nullptr;      // single buffer (stepwise, no prev/curr ramp)
    void* dFrHfAttenDb = nullptr;

    // Upload diet (M2): device ping-pong per prev/curr matrix pair. The kernel
    // takes the matrix pointers as launch ARGUMENTS and only READS them
    // (const __restrict__, CudaWfsKernels.h), so "prev" never needs a host
    // round-trip: it is simply the slot uploaded one launch earlier. Only a
    // CHANGED curr is uploaded (into the alternate slot, then swap); unchanged
    // blocks pass prev == curr == the live slot — the kernel ramps x->x = x,
    // bit-exact, and aliasing the two pointers is legal because neither is
    // written through. First launch: upload once, pass prev == curr (exactly
    // the old havePrev bootstrap semantics).
    struct PingPong
    {
        void* slot[2] = { nullptr, nullptr };
        int   curr = 0;               // slot holding the last-consumed curr
        bool  everUploaded = false;   // first launch: upload once, prev == curr
    };
    PingPong ppDelays, ppGains, ppFrDelays, ppFrGains;
    bool hfUploaded = false;          // single-buffer change-detect state
    bool frHfUploaded = false;

    // Change-detect baselines: the last STAGED (== last uploaded) copy of each
    // matrix, memcmp'd against the freshly staged pinned buffer. Comparing
    // staged copies (never the live app matrices) is torn-read-safe: it
    // compares exactly the values the kernel would consume.
    std::vector<float> lastDelays, lastGains, lastFrDelays, lastFrGains;
    std::vector<float> lastHfAttenDb, lastFrHfAttenDb;

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

    WfsFrHostState frHost;            // per-input FR pre-filters + jitter

    // M3: host worker pool for the fused per-input prep (input pack + FR filter
    // + jitter advance + matrix/FR snapshot rows). Owned here, created in
    // prepare(), joined in release() BEFORE any CUDA teardown; workers only
    // touch host memory, never CUDA APIs (plan section 3).
    GpuHostWorkPool pool;

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

    // End-of-block sync event: BlockingSync makes cudaEventSynchronize yield the
    // pump thread on an OS primitive instead of the spin-wait of
    // cudaStreamSynchronize; DisableTiming skips timestamp bookkeeping.
    CK_RT (cudaEventCreateWithFlags (&m.syncEvent, cudaEventBlockingSync | cudaEventDisableTiming));

    auto pin = [] (float** p, size_t n) {
        return cudaHostAlloc ((void**) p, n * sizeof (float), cudaHostAllocDefault);
    };
    CK_RT (pin (&m.hIn,   (size_t) m.numIn  * m.blockSize));
    CK_RT (pin (&m.hFrIn, (size_t) m.numIn  * m.blockSize));
    CK_RT (pin (&m.hOut,  (size_t) m.numOut * m.blockSize));
    CK_RT (pin (&m.hDelaysCurr, matrix));
    CK_RT (pin (&m.hGainsCurr,  matrix));
    CK_RT (pin (&m.hFrDelaysCurr, matrix));
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
    for (auto* pp : { &m.ppDelays, &m.ppGains, &m.ppFrDelays, &m.ppFrGains })
    {
        CK_RT (cudaMalloc (&pp->slot[0], matrix * sizeof (float)));
        CK_RT (cudaMalloc (&pp->slot[1], matrix * sizeof (float)));
        pp->curr = 0;
        pp->everUploaded = false;
    }
    CK_RT (cudaMalloc (&m.dHfAttenDb,    matrix * sizeof (float)));
    CK_RT (cudaMalloc (&m.dFrHfAttenDb,  matrix * sizeof (float)));
    m.hfUploaded = false;
    m.frHfUploaded = false;

    CK_RT (cudaMemset (m.dRing,   0, (size_t) m.numIn * m.ringCapacity * sizeof (float)));
    CK_RT (cudaMemset (m.dFrRing, 0, (size_t) m.numIn * m.ringCapacity * sizeof (float)));
    CK_RT (cudaMemset (m.dShelfState,   0, (size_t) matrix * 4 * sizeof (float)));
    CK_RT (cudaMemset (m.dFrShelfState, 0, (size_t) matrix * 4 * sizeof (float)));

    m.lastDelays.assign (matrix, 0.0f);
    m.lastGains.assign (matrix, 0.0f);
    m.lastFrDelays.assign (matrix, 0.0f);
    m.lastFrGains.assign (matrix, 0.0f);
    m.lastHfAttenDb.assign (matrix, 0.0f);
    m.lastFrHfAttenDb.assign (matrix, 0.0f);

    m.frHost.prepare (m.numIn, m.numOut, sampleRate);

    // M3 host worker pool: auto = clamp(physicalCores/8, 1, 3) lanes for
    // Wfs/Ob; WFS_GPU_HOST_WORKERS overrides (0 = sequential kill switch +
    // determinism cross-check). periodMs/computationMs feed macOS P-core
    // placement only. Workers self-elevate to the audio scheduling class.
    {
        const int autoWorkers = std::clamp (spatcore::rt::physicalCoreCount() / 8, 1, 3);
        const int workers = hostWorkerCountFromEnv (autoWorkers);
        const double periodMs = (sampleRate > 0.0) ? (1000.0 * m.blockSize / sampleRate) : 0.0;
        m.pool.prepare (workers, periodMs, periodMs);
    }

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
    // runtime copies (cudaMemcpyAsync / event sync on m.stream) also
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

    // M3: ONE fused parallelFor(numIn) does ALL per-input host prep — input
    // pack, per-input FR pre-filter, per-input jitter advance, and the direct +
    // shelf + FR-curr matrix snapshot rows for that input ([in*numOut,
    // (in+1)*numOut)). Every item writes only item-indexed state and each FP
    // sequence is a pure function of (input, block inputs, per-input persistent
    // state) with no cross-item host accumulation, so the result is
    // bit-identical for ANY worker count (section-4 determinism table). The
    // launchCounter is HOISTED: currentLaunchIndex() is read here and passed to
    // every item; commitJitterLaunch() runs ONCE after the join. The M2 memcmp
    // change-detect + upload stay on the pump thread AFTER the join (they need
    // the full staged matrix).
    const uint32_t launchIdx = m.frHost.currentLaunchIndex();
    m.pool.parallelFor (m.numIn, [&] (int in)
    {
        const int nOut = m.numOut;
        const size_t rowBase = (size_t) in * (size_t) nOut;

        // Direct matrix snapshot (-L, clamped) + raw-dB shelf staging, this
        // input's rows. prev = the previous launch's curr (device ping-pong).
        for (int out = 0; out < nOut; ++out)
        {
            const size_t i = rowBase + (size_t) out;
            const float d = m.delaysMs != nullptr ? (m.delaysMs[i] - m.latencyMs) * srScale : 0.0f;
            m.hDelaysCurr[i]  = std::clamp (d, 0.0f, maxDelay);
            m.hGainsCurr[i]   = m.gains      != nullptr ? m.gains[i]      : 0.0f;
            m.hHfAttenDb[i]   = m.hfAttenDb   != nullptr ? m.hfAttenDb[i]   : 0.0f;
            m.hFrHfAttenDb[i] = m.frHfAttenDb != nullptr ? m.frHfAttenDb[i] : 0.0f;
        }

        // FR jitter advance (hoisted launch index) then FR curr snapshot rows —
        // computeFrCurrForInput reads this item's jitterSamples, written just
        // above in the SAME item (lane-local read-after-write).
        m.frHost.advanceJitterForInput (in, launchIdx, m.blockSize);
        m.frHost.computeFrCurrForInput (in, m.delaysMs, m.frDelaysMs, m.frLevels,
                                        m.latencyMs, srScale, maxDelay,
                                        m.hFrDelaysCurr, m.hFrGainsCurr);

        // Input pack row (silence for a missing channel) + FR pre-filter row.
        if (inputs[in] != nullptr)
            std::memcpy (m.hIn + (size_t) in * m.blockSize, inputs[in], (size_t) m.blockSize * sizeof (float));
        else
            std::memset (m.hIn + (size_t) in * m.blockSize, 0, (size_t) m.blockSize * sizeof (float));
        m.frHost.filterBlockForInput (in, inputs, m.hFrIn, m.blockSize);
    });
    m.frHost.commitJitterLaunch();   // hoisted ++launchCounter, once after the join

    WFS_STAGE_MARK (stA);   // host prep (fused parallelFor)
    WFS_STAGE_MARK (stB);
    WFS_STAGE_MARK (stD);
    WFS_STAGE_MARK (stE);

    // Host -> device (the persistent rings + shelf states stay on the device).
#define PB_RT(call) do { cudaError_t _e = (call); if (_e != cudaSuccess) { \
    lastError = std::string ("CUDA runtime: ") + cudaGetErrorString (_e); ready = false; return false; } } while (0)

    auto up = [&m] (void* dst, const float* src, size_t floats) {
        return cudaMemcpyAsync (dst, src, floats * sizeof (float), cudaMemcpyHostToDevice, m.stream);
    };
    PB_RT (up (m.dIn,   m.hIn,   (size_t) m.numIn * m.blockSize));
    PB_RT (up (m.dFrIn, m.hFrIn, (size_t) m.numIn * m.blockSize));

    // Upload diet: memcmp each freshly staged matrix against its lastStaged
    // baseline. Unchanged => skip the upload AND the slot swap (prev == curr ==
    // live slot; the kernel ramps x->x = x). Changed => upload into the
    // alternate slot, prev = the previous slot (last launch's curr, already
    // on-device), swap. First launch: single upload, prev == curr (the old
    // havePrev bootstrap, bit-exact). Safe against in-flight reads: the
    // previous block ended with cudaEventSynchronize, so no launch is reading
    // either slot while we upload.
    auto stagePair = [&m, matrix] (Impl::PingPong& pp, const float* staged,
                                   std::vector<float>& last,
                                   void*& prevArg, void*& currArg) -> cudaError_t
    {
        const size_t bytes = (size_t) matrix * sizeof (float);
        if (pp.everUploaded && std::memcmp (staged, last.data(), bytes) == 0)
        {
            prevArg = currArg = pp.slot[pp.curr];      // unchanged: no upload, no swap
            return cudaSuccess;
        }
        const int next = pp.everUploaded ? (pp.curr ^ 1) : pp.curr;
        const cudaError_t e = cudaMemcpyAsync (pp.slot[next], staged, bytes,
                                               cudaMemcpyHostToDevice, m.stream);
        if (e != cudaSuccess)
            return e;
        std::memcpy (last.data(), staged, bytes);
        prevArg = pp.slot[pp.curr];                    // first launch: prev == curr
        currArg = pp.slot[next];
        pp.curr = next;
        pp.everUploaded = true;
        return e;
    };
    auto stageSingle = [&m, matrix] (void* dBuf, const float* staged,
                                     std::vector<float>& last, bool& uploaded) -> cudaError_t
    {
        const size_t bytes = (size_t) matrix * sizeof (float);
        if (uploaded && std::memcmp (staged, last.data(), bytes) == 0)
            return cudaSuccess;
        const cudaError_t e = cudaMemcpyAsync (dBuf, staged, bytes,
                                               cudaMemcpyHostToDevice, m.stream);
        if (e != cudaSuccess)
            return e;
        std::memcpy (last.data(), staged, bytes);
        uploaded = true;
        return e;
    };

    void* dDelaysPrevArg = nullptr;   void* dDelaysCurrArg = nullptr;
    void* dGainsPrevArg = nullptr;    void* dGainsCurrArg = nullptr;
    void* dFrDelaysPrevArg = nullptr; void* dFrDelaysCurrArg = nullptr;
    void* dFrGainsPrevArg = nullptr;  void* dFrGainsCurrArg = nullptr;
    PB_RT (stagePair (m.ppDelays,   m.hDelaysCurr,   m.lastDelays,   dDelaysPrevArg,   dDelaysCurrArg));
    PB_RT (stagePair (m.ppGains,    m.hGainsCurr,    m.lastGains,    dGainsPrevArg,    dGainsCurrArg));
    PB_RT (stagePair (m.ppFrDelays, m.hFrDelaysCurr, m.lastFrDelays, dFrDelaysPrevArg, dFrDelaysCurrArg));
    PB_RT (stagePair (m.ppFrGains,  m.hFrGainsCurr,  m.lastFrGains,  dFrGainsPrevArg,  dFrGainsCurrArg));
    PB_RT (stageSingle (m.dHfAttenDb,   m.hHfAttenDb,   m.lastHfAttenDb,   m.hfUploaded));
    PB_RT (stageSingle (m.dFrHfAttenDb, m.hFrHfAttenDb, m.lastFrHfAttenDb, m.frHfUploaded));

    WfsParamsGpu p { (uint32_t) m.numIn, (uint32_t) m.numOut, (uint32_t) m.blockSize,
                     m.ringCapacity, m.ringWritePos, m.ringValid,
                     m.pairGroups, m.shelfCosW0, m.shelfSinW0 };

    void* pairsArgs[] = { &p, &m.dIn, &m.dFrIn, &m.dRing, &m.dFrRing,
                          &dDelaysPrevArg, &dDelaysCurrArg, &dGainsPrevArg, &dGainsCurrArg,
                          &dFrDelaysPrevArg, &dFrDelaysCurrArg, &dFrGainsPrevArg, &dFrGainsCurrArg,
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
    PB_RT (cudaEventRecord (m.syncEvent, m.stream));
    PB_RT (cudaEventSynchronize (m.syncEvent));   // blocking-sync event: yields, no spin
    WFS_STAGE_MARK (stG);   // wait: event sync

#undef PB_RT

    // Output pinned buffer -> channels.
    for (int ch = 0; ch < m.numOut; ++ch)
        if (outputs[ch] != nullptr)
            std::memcpy (outputs[ch], m.hOut + (size_t) ch * m.blockSize, (size_t) m.blockSize * sizeof (float));

    // Advance host-tracked state (matrix prev continuity now lives on the
    // device: the ping-pong slots + lastStaged baselines, updated at upload).
    m.ringWritePos = (m.ringWritePos + (uint32_t) m.blockSize) % m.ringCapacity;
    m.ringValid = std::min (m.maxDelaySamples, m.ringValid + (uint32_t) m.blockSize);

#if WFS_GPU_STAGE_TIMERS
    {
        const auto stH = std::chrono::steady_clock::now();   // unpack end
        auto ms = [] (auto a, auto b) { return std::chrono::duration<double, std::milli> (b - a).count(); };
        m.stSnapshotMs    += ms (t0, stA);
        m.stFrPrepMs      += ms (stA, stB) + ms (stD, stE);
        m.stUploadIssueMs += ms (stB, stD) + ms (stE, stF);
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

    // Upload-diet state back to first-launch semantics: the next block
    // force-uploads every matrix and passes prev == curr (old havePrev
    // bootstrap). lastStaged baselines re-zeroed to match prepare().
    for (auto* pp : { &m.ppDelays, &m.ppGains, &m.ppFrDelays, &m.ppFrGains })
        pp->everUploaded = false;
    m.hfUploaded = false;
    m.frHfUploaded = false;
    for (auto* v : { &m.lastDelays, &m.lastGains, &m.lastFrDelays, &m.lastFrGains,
                     &m.lastHfAttenDb, &m.lastFrHfAttenDb })
        std::fill (v->begin(), v->end(), 0.0f);
}

void CudaWfsBackend::release() noexcept
{
    auto& m = *impl;

    // M3: join the host worker pool BEFORE any CUDA teardown (and before the
    // plugin DLL could unload). Workers touch only host memory, so this is a
    // clean fork-join drain with no GPU-context concerns.
    m.pool.shutdown();

    if (m.context != nullptr)
        cuCtxSetCurrent (m.context);

    auto freeHost = [] (float*& p) { if (p != nullptr) { cudaFreeHost (p); p = nullptr; } };
    auto freeDev  = [] (void*&  p) { if (p != nullptr) { cudaFree (p);     p = nullptr; } };

    freeHost (m.hIn);  freeHost (m.hFrIn);  freeHost (m.hOut);
    freeHost (m.hDelaysCurr);   freeHost (m.hGainsCurr);
    freeHost (m.hFrDelaysCurr); freeHost (m.hFrGainsCurr);
    freeHost (m.hHfAttenDb);    freeHost (m.hFrHfAttenDb);

    freeDev (m.dIn);  freeDev (m.dFrIn);  freeDev (m.dOut);
    freeDev (m.dRing);  freeDev (m.dFrRing);
    freeDev (m.dScratch);
    freeDev (m.dShelfState); freeDev (m.dFrShelfState);
    for (auto* pp : { &m.ppDelays, &m.ppGains, &m.ppFrDelays, &m.ppFrGains })
    {
        freeDev (pp->slot[0]);
        freeDev (pp->slot[1]);
        pp->curr = 0;
        pp->everUploaded = false;
    }
    freeDev (m.dHfAttenDb);    freeDev (m.dFrHfAttenDb);
    m.hfUploaded = false;
    m.frHfUploaded = false;

    if (m.syncEvent != nullptr) { cudaEventDestroy (m.syncEvent); m.syncEvent = nullptr; }
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
    ready = false;
}

#undef CK_RT
#undef CK_DRV

} // namespace spatcore::gpu

#endif // WFS_GPU_NATIVE && !defined(__APPLE__)
