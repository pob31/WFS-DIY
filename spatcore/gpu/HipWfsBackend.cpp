/*
    HipWfsBackend implementation.

    The kernel sources live in CudaWfsKernels.h as a string literal (shared
    verbatim with the CUDA backend - the same CUDA-C source is valid HIP) so the
    app needs no .hip file and no hipcc build step; they are compiled at
    prepare() time via hipRTC (~tens of ms once) into a HIP code object, loaded
    with the HIP module API, and launched with hipModuleLaunchKernel. Buffers and
    copies use the HIP runtime API. Runtime + driver API share the device's
    primary context.

    Host-side processBlock mirrors CudaWfsBackend / MetalWfsBackend exactly
    (matrix snapshot with -L compensation, prev->curr ramp continuity, persistent
    device rings, host-tracked ring advance), so the audible behaviour matches the
    twins and the CPU reference. Floor-Reflection host work (per-input pre-filter,
    diffusion jitter, FR matrix snapshot) lives in the shared WfsFrHostState.

    Threading note: prepare() runs on the engine-setup thread, but processBlock()
    runs on the GpuAsyncPipeline pump thread. HIP driver-API context currency is
    per-thread, so processBlock() binds the device (hipSetDevice) at
    the top - cheap (a TLS write) and makes both the driver launch and the runtime
    copies use the same context on the pump thread.
*/

// This translation unit is in the shared file list for every exporter, but only
// the Linux/AMD build (WFS_GPU_NATIVE, non-Apple, WFS_GPU_HIP) pulls in the ROCm
// toolkit. On macOS the Metal backend is used; on Windows/NVIDIA the CUDA twin is;
// otherwise this compiles to an empty TU so the hip headers are never required.
#if WFS_GPU_NATIVE && !defined(__APPLE__) && defined(WFS_GPU_HIP)

#include "HipWfsBackend.h"
#include "CudaWfsKernels.h"   // kernel source shared with the CUDA backend (valid HIP)
#include "WfsFrHostState.h"

#include <hip/hip_runtime.h>   // HIP runtime + driver API (hipMalloc, hipModule*, hipModuleLaunchKernel, props)
#include <hip/hiprtc.h>        // hipRTC: runtime kernel compilation

// Linux links the HIP libs via the .jucer externalLibraries (-lamdhip64 -lhiprtc);
// no MSVC-style #pragma comment(lib, ...) is needed or honoured here.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

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

struct HipWfsBackend::Impl
{
    hipModule_t     module = nullptr;
    hipFunction_t   kernelPairs = nullptr;
    hipFunction_t   kernelReduce = nullptr;
    hipStream_t stream = nullptr;

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

    int deviceIndex = 0;             // which HIP device to bind (ctor-injected)
    unsigned int threadsPerBlock = 256;
};

HipWfsBackend::HipWfsBackend (int deviceIndex) : impl (std::make_unique<Impl>()) { impl->deviceIndex = deviceIndex; }
HipWfsBackend::~HipWfsBackend() { release(); }

// prepare()-only error helpers: set lastError, tear down, return false.
#define CK_RT(call)  do { hipError_t _e = (call); if (_e != hipSuccess) { \
    lastError = std::string ("HIP runtime: ") + hipGetErrorString (_e); release(); return false; } } while (0)
#define CK_DRV(call) do { hipError_t _e = (call); if (_e != hipSuccess) { const char* _s = nullptr; \
    hipDrvGetErrorString (_e, &_s); lastError = std::string ("HIP driver: ") + (_s ? _s : "unknown"); \
    release(); return false; } } while (0)

bool HipWfsBackend::prepare (int numInputs, int numOutputs, int blockSize,
                              double sampleRate, double pipelineLatencyMs,
                              double maxDelaySeconds)
{
    release();
    auto& m = *impl;

    // 1) Pick the selected device; read its name + GFX arch (gcnArchName).
    int devCount = 0;
    CK_RT (hipGetDeviceCount (&devCount));
    if (devCount == 0)
    {
        lastError = "No HIP device available";
        return false;
    }
    if (m.deviceIndex < 0 || m.deviceIndex >= devCount)
    {
        lastError = "HIP device index " + std::to_string (m.deviceIndex)
                    + " out of range (" + std::to_string (devCount) + " present)";
        return false;
    }
    CK_RT (hipSetDevice (m.deviceIndex));

    hipDeviceProp_t prop;
    CK_RT (hipGetDeviceProperties (&prop, m.deviceIndex));
    deviceName = std::string (prop.name) + " (HIP)";
    // hipRTC targets an AMD GFX arch by name (e.g. "gfx1103"), not an SM number.
    const std::string archName = prop.gcnArchName;

    // 3) hipRTC-compile the kernel string to a code object for this GPU's arch.
    {
        hiprtcProgram prog = nullptr;
        if (hiprtcCreateProgram (&prog, kWfsDelaySumKernelSource, "wfs_delay_sum.cu", 0, nullptr, nullptr) != HIPRTC_SUCCESS)
        {
            lastError = "hipRTC: program creation failed";
            release();
            return false;
        }

        const std::string archOpt = "--offload-arch=" + archName;
        const char* opts[] = { archOpt.c_str() };
        const hiprtcResult comp = hiprtcCompileProgram (prog, 1, opts);
        if (comp != HIPRTC_SUCCESS)
        {
            size_t logSize = 0;
            hiprtcGetProgramLogSize (prog, &logSize);
            std::string log (logSize, '\0');
            if (logSize > 0)
                hiprtcGetProgramLog (prog, &log[0]);
            lastError = std::string ("hipRTC compile failed: ") + log;
            hiprtcDestroyProgram (&prog);
            release();
            return false;
        }

        size_t ptxSize = 0;
        hiprtcGetCodeSize (prog, &ptxSize);
        std::vector<char> ptx (ptxSize);
        hiprtcGetCode (prog, ptx.data());
        hiprtcDestroyProgram (&prog);

        CK_DRV (hipModuleLoadDataEx (&m.module, ptx.data(), 0, nullptr, nullptr));
        CK_DRV (hipModuleGetFunction (&m.kernelPairs, m.module, "wfs_pairs"));
        CK_DRV (hipModuleGetFunction (&m.kernelReduce, m.module, "wfs_reduce"));
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
    CK_RT (hipStreamCreate (&m.stream));

    auto pin = [] (float** p, size_t n) {
        return hipHostMalloc ((void**) p, n * sizeof (float), hipHostMallocDefault);
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

    CK_RT (hipMalloc (&m.dIn,   (size_t) m.numIn  * m.blockSize * sizeof (float)));
    CK_RT (hipMalloc (&m.dFrIn, (size_t) m.numIn  * m.blockSize * sizeof (float)));
    CK_RT (hipMalloc (&m.dOut,  (size_t) m.numOut * m.blockSize * sizeof (float)));
    CK_RT (hipMalloc (&m.dRing,   (size_t) m.numIn * m.ringCapacity * sizeof (float)));
    CK_RT (hipMalloc (&m.dFrRing, (size_t) m.numIn * m.ringCapacity * sizeof (float)));
    CK_RT (hipMalloc (&m.dScratch, (size_t) m.numIn * m.numOut * m.blockSize * sizeof (float)));
    CK_RT (hipMalloc (&m.dShelfState,   (size_t) matrix * 4 * sizeof (float)));
    CK_RT (hipMalloc (&m.dFrShelfState, (size_t) matrix * 4 * sizeof (float)));
    CK_RT (hipMalloc (&m.dDelaysPrev, matrix * sizeof (float)));
    CK_RT (hipMalloc (&m.dDelaysCurr, matrix * sizeof (float)));
    CK_RT (hipMalloc (&m.dGainsPrev,  matrix * sizeof (float)));
    CK_RT (hipMalloc (&m.dGainsCurr,  matrix * sizeof (float)));
    CK_RT (hipMalloc (&m.dFrDelaysPrev, matrix * sizeof (float)));
    CK_RT (hipMalloc (&m.dFrDelaysCurr, matrix * sizeof (float)));
    CK_RT (hipMalloc (&m.dFrGainsPrev,  matrix * sizeof (float)));
    CK_RT (hipMalloc (&m.dFrGainsCurr,  matrix * sizeof (float)));
    CK_RT (hipMalloc (&m.dHfAttenDb,    matrix * sizeof (float)));
    CK_RT (hipMalloc (&m.dFrHfAttenDb,  matrix * sizeof (float)));

    CK_RT (hipMemset (m.dRing,   0, (size_t) m.numIn * m.ringCapacity * sizeof (float)));
    CK_RT (hipMemset (m.dFrRing, 0, (size_t) m.numIn * m.ringCapacity * sizeof (float)));
    CK_RT (hipMemset (m.dShelfState,   0, (size_t) matrix * 4 * sizeof (float)));
    CK_RT (hipMemset (m.dFrShelfState, 0, (size_t) matrix * 4 * sizeof (float)));

    m.delaysPrevSamples.assign (matrix, 0.0f);
    m.gainsPrev.assign (matrix, 0.0f);
    m.frDelaysPrevSamples.assign (matrix, 0.0f);
    m.frGainsPrev.assign (matrix, 0.0f);

    m.frHost.prepare (m.numIn, m.numOut, sampleRate);

    ready = true;
    lastError.clear();
    return true;
}

void HipWfsBackend::setMatrixPointers (const float* delaysMsPtr, const float* gainsPtr,
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

void HipWfsBackend::setFRFilterParams (int inputIndex,
                                        bool lowCutActive, float lowCutFreq,
                                        bool highShelfActive, float highShelfFreq,
                                        float highShelfGain, float highShelfSlope) noexcept
{
    impl->frHost.setFRFilterParams (inputIndex, lowCutActive, lowCutFreq,
                                    highShelfActive, highShelfFreq,
                                    highShelfGain, highShelfSlope);
}

void HipWfsBackend::setFRDiffusion (int inputIndex, float diffusionPercent) noexcept
{
    impl->frHost.setFRDiffusion (inputIndex, diffusionPercent);
}

bool HipWfsBackend::processBlock (const float* const* inputs, float* const* outputs)
{
    if (! ready)
        return false;

    auto& m = *impl;
    const uint32_t matrix = (uint32_t) (m.numIn * m.numOut);
    const float srScale = (float) (m.sampleRate / 1000.0);
    const float maxDelay = (float) m.maxDelaySamples;

    // processBlock runs on the pump thread; bind the selected device here so
    // both the driver launch and the runtime copies use it.
    if (hipSetDevice (m.deviceIndex) != hipSuccess)
    {
        lastError = "HIP: hipSetDevice failed on pump thread";
        ready = false;
        return false;
    }

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

    // FR: advance diffusion jitter (64-sample sub-step cadence), then snapshot
    // the FR curr matrices. The pipeline latency is subtracted from the
    // ABSOLUTE FR delay (direct + extra + jitter - L), preserving the
    // FR-vs-direct offset exactly.
    m.frHost.advanceJitter (m.blockSize);
    m.frHost.computeFrCurr (m.delaysMs, m.frDelaysMs, m.frLevels,
                            m.latencyMs, srScale, maxDelay,
                            m.hFrDelaysCurr, m.hFrGainsCurr);

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

    // Input channels -> flat pinned buffer (silence for missing channels),
    // and the host-side FR pre-filter chain -> frIn staging.
    for (int ch = 0; ch < m.numIn; ++ch)
    {
        if (inputs[ch] != nullptr)
            std::memcpy (m.hIn + (size_t) ch * m.blockSize, inputs[ch], (size_t) m.blockSize * sizeof (float));
        else
            std::memset (m.hIn + (size_t) ch * m.blockSize, 0, (size_t) m.blockSize * sizeof (float));
    }
    m.frHost.filterBlock (inputs, m.hFrIn, m.blockSize);

    // Host -> device (the persistent rings + shelf states stay on the device).
#define PB_RT(call) do { hipError_t _e = (call); if (_e != hipSuccess) { \
    lastError = std::string ("HIP runtime: ") + hipGetErrorString (_e); ready = false; return false; } } while (0)

    auto up = [&m] (void* dst, const float* src, size_t floats) {
        return hipMemcpyAsync (dst, src, floats * sizeof (float), hipMemcpyHostToDevice, m.stream);
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
    hipError_t lr = hipModuleLaunchKernel (m.kernelPairs,
                                  gridPairs, 1, 1,
                                  m.threadsPerBlock, 1, 1,
                                  0, (hipStream_t) m.stream,
                                  pairsArgs, nullptr);
    if (lr != hipSuccess)
    {
        const char* s = nullptr;
        hipDrvGetErrorString (lr, &s);
        lastError = std::string ("HIP launch failed (pairs): ") + (s ? s : "unknown");
        ready = false;
        return false;
    }

    // K2: deterministic per-output reduction over the scratch (stream-ordered
    // after K1).
    void* reduceArgs[] = { &p, &m.dScratch, &m.dOut };
    lr = hipModuleLaunchKernel (m.kernelReduce,
                         (unsigned int) m.numOut, 1, 1,
                         m.threadsPerBlock, 1, 1,
                         0, (hipStream_t) m.stream,
                         reduceArgs, nullptr);
    if (lr != hipSuccess)
    {
        const char* s = nullptr;
        hipDrvGetErrorString (lr, &s);
        lastError = std::string ("HIP launch failed (reduce): ") + (s ? s : "unknown");
        ready = false;
        return false;
    }

    PB_RT (hipMemcpyAsync (m.hOut, m.dOut, (size_t) m.numOut * m.blockSize * sizeof (float), hipMemcpyDeviceToHost, m.stream));
    PB_RT (hipStreamSynchronize (m.stream));

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

    lastLaunchMs = std::chrono::duration<double, std::milli> (
                       std::chrono::steady_clock::now() - t0).count();
    return true;
}

void HipWfsBackend::reset() noexcept
{
    auto& m = *impl;
    hipSetDevice (m.deviceIndex);
    const size_t ringBytes = (size_t) m.numIn * m.ringCapacity * sizeof (float);
    const size_t stateBytes = (size_t) m.numIn * m.numOut * 4 * sizeof (float);
    if (m.dRing != nullptr && ringBytes > 0)
        hipMemset (m.dRing, 0, ringBytes);
    if (m.dFrRing != nullptr && ringBytes > 0)
        hipMemset (m.dFrRing, 0, ringBytes);
    if (m.dShelfState != nullptr && stateBytes > 0)
        hipMemset (m.dShelfState, 0, stateBytes);
    if (m.dFrShelfState != nullptr && stateBytes > 0)
        hipMemset (m.dFrShelfState, 0, stateBytes);
    m.frHost.reset();
    m.ringWritePos = 0;
    m.ringValid = 0;
    m.havePrev = false;
}

void HipWfsBackend::release() noexcept
{
    auto& m = *impl;

    hipSetDevice (m.deviceIndex);

    auto freeHost = [] (float*& p) { if (p != nullptr) { hipHostFree (p); p = nullptr; } };
    auto freeDev  = [] (void*&  p) { if (p != nullptr) { hipFree (p);     p = nullptr; } };

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

    if (m.stream != nullptr) { hipStreamDestroy (m.stream); m.stream = nullptr; }
    if (m.module != nullptr) { hipModuleUnload (m.module); m.module = nullptr; }
    m.kernelPairs = nullptr;
    m.kernelReduce = nullptr;


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

#endif // WFS_GPU_NATIVE && !defined(__APPLE__) && defined(WFS_GPU_HIP)
