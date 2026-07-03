/*
    HipObBackend implementation — the scatter / write-time twin of
    CudaWfsBackend.cpp.

    The kernel sources live in CudaObKernels.h as a string literal (hipRTC-compiled
    at prepare() time into PTX, loaded with the CUDA Driver API, launched with
    hipModuleLaunchKernel). Buffers and copies use the CUDA Runtime API; runtime + driver
    share the device's primary context.

    v2 architecture: one thread per (in,out) pair scattering into a PRIVATE
    persistent per-pair accumulator, then a deterministic per-output reduce
    (the proven wfs_pairs/wfs_reduce occupancy shape; v1's single writer per
    output was ~32x under-parallel). Delay contract: all scatter delays >= 1
    sample (host-clamped, kernel re-clamped) - see MetalObKernels.h.

    Host-side processBlock mirrors MetalObBackend.mm exactly, so the audible
    behaviour matches the Metal twin and the CPU reference. Floor-Reflection
    host work lives in the shared WfsFrHostState.

    Device memory: per-pair accumulators = numIn*numOut*accLen*4 B
    (32x27 @ 1 s/48 kHz ~ 166 MB; 64x64 ~ 787 MB - watch small-VRAM cards;
    prepare() fails gracefully with the size in the error message).
*/

// This translation unit is in the shared file list for every exporter, but only
// the Linux/AMD build (WFS_GPU_NATIVE, non-Apple, WFS_GPU_HIP) pulls in the ROCm
// toolkit. On macOS the Metal backend is used; on Windows/NVIDIA the CUDA twin is;
// otherwise this compiles to an empty TU so the hip headers are never required.
#if WFS_GPU_NATIVE && !defined(__APPLE__) && defined(WFS_GPU_HIP)

#include "HipObBackend.h"
#include "CudaObKernels.h"
#include "WfsFrHostState.h"

#include <hip/hip_runtime.h>   // HIP runtime + driver API (hipMalloc, hipModule*, hipModuleLaunchKernel, props)
#include <hip/hiprtc.h>        // hipRTC: runtime kernel compilation

// Linux links the HIP libs via the .jucer externalLibraries (-lamdhip64 -lhiprtc),
// and the Windows wfs_hip.dll links them via hipcc; no MSVC-style #pragma
// comment(lib, ...) is needed or honoured here. (A CUDA pragma here would wrongly
// pull cudart/nvrtc into the AMD plugin.)

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

namespace spatcore::gpu {

namespace
{
// Host mirror of the kernel-side ObParams - layouts must match exactly.
struct ObParamsGpu
{
    uint32_t numInputs;
    uint32_t numOutputs;
    uint32_t bufferLength;
    uint32_t accLength;
    uint32_t writePos;
    float    shelfCosW0;
    float    shelfSinW0;
};

// FR diffusion grain is sub-stepped at this cadence to match the CPU
// OutputBufferProcessor (its internal processing block is 64 samples).
constexpr int kObSubBlock = 64;
} // namespace

struct HipObBackend::Impl
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
    float* hFrDelaySamples = nullptr;     // [pairs][blockSize] per-sample FR delay (jitter sub-stepped)
    float* hFrGainsPrev = nullptr;
    float* hFrGainsCurr = nullptr;
    float* hHfAttenDb = nullptr;
    float* hFrHfAttenDb = nullptr;

    // Device buffers (per-pair accumulators + shelf states persist across launches).
    void* dIn = nullptr;
    void* dFrIn = nullptr;
    void* dOut = nullptr;
    void* dPairAcc = nullptr;         // [pairs][accLen] persistent (thread-ordered)
    void* dPairOut = nullptr;         // [(s*numOut+out)*numIn+in] transient
    void* dShelfState = nullptr;      // [pairs][4] persistent
    void* dFrShelfState = nullptr;    // [pairs][4] persistent
    void* dDelaysPrev = nullptr;
    void* dDelaysCurr = nullptr;
    void* dGainsPrev = nullptr;
    void* dGainsCurr = nullptr;
    void* dFrDelaySamples = nullptr;  // [pairs][blockSize] per-sample FR delay
    void* dFrGainsPrev = nullptr;
    void* dFrGainsCurr = nullptr;
    void* dHfAttenDb = nullptr;
    void* dFrHfAttenDb = nullptr;

    int numIn = 0, numOut = 0, blockSize = 0;
    uint32_t accLen = 0;
    uint32_t writePos = 0;
    float maxDelayClamp = 0.0f;       // accLen - 1
    double sampleRate = 0.0;
    float latencyMs = 0.0f;
    float shelfCosW0 = 1.0f;
    float shelfSinW0 = 0.0f;

    const float* delaysMs = nullptr;
    const float* gains = nullptr;
    const float* hfAttenDb = nullptr;
    const float* frDelaysMs = nullptr;
    const float* frLevels = nullptr;
    const float* frHfAttenDb = nullptr;

    std::vector<float> delaysPrevSamples;
    std::vector<float> gainsPrev;
    std::vector<float> frBasePrev;        // [pairs] base FR delay ramp continuity (no jitter)
    std::vector<float> frGainsPrev;
    bool havePrev = false;

    WfsFrHostState frHost;            // per-input FR pre-filters + jitter (shared)

    int deviceIndex = 0;             // which HIP device to bind (ctor-injected)
    unsigned int threadsPerBlock = 256;
};

HipObBackend::HipObBackend (int deviceIndex) : impl (std::make_unique<Impl>()) { impl->deviceIndex = deviceIndex; }
HipObBackend::~HipObBackend() { release(); }

#define CK_RT(call)  do { hipError_t _e = (call); if (_e != hipSuccess) { \
    lastError = std::string ("HIP runtime: ") + hipGetErrorString (_e); release(); return false; } } while (0)
#define CK_DRV(call) do { hipError_t _e = (call); if (_e != hipSuccess) { const char* _s = nullptr; \
    hipDrvGetErrorString (_e, &_s); lastError = std::string ("HIP driver: ") + (_s ? _s : "unknown"); \
    release(); return false; } } while (0)

bool HipObBackend::prepare (int numInputs, int numOutputs, int blockSize,
                             double sampleRate, double pipelineLatencyMs,
                             double maxDelaySeconds)
{
    release();
    auto& m = *impl;

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
    const std::string archName = prop.gcnArchName;

    {
        hiprtcProgram prog = nullptr;
        if (hiprtcCreateProgram (&prog, kObScatterKernelSource, "ob_scatter.cu", 0, nullptr, nullptr) != HIPRTC_SUCCESS)
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
        CK_DRV (hipModuleGetFunction (&m.kernelPairs, m.module, "ob_pairs"));
        CK_DRV (hipModuleGetFunction (&m.kernelReduce, m.module, "ob_reduce"));
    }

    m.numIn = std::max (1, numInputs);
    m.numOut = std::max (1, numOutputs);
    m.blockSize = std::max (1, blockSize);
    m.sampleRate = sampleRate;
    m.latencyMs = (float) pipelineLatencyMs;
    m.accLen = (uint32_t) (maxDelaySeconds * sampleRate);
    if (m.accLen < (uint32_t) m.blockSize + 2)
        m.accLen = (uint32_t) m.blockSize + 2;
    m.maxDelayClamp = (float) (m.accLen - 1);
    m.writePos = 0;
    m.havePrev = false;
    m.threadsPerBlock = 256;

    const double w0 = 2.0 * 3.14159265358979 * 800.0 / sampleRate;
    m.shelfCosW0 = (float) std::cos (w0);
    m.shelfSinW0 = (float) std::sin (w0);

    const uint32_t matrix = (uint32_t) (m.numIn * m.numOut);

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
    CK_RT (pin (&m.hFrDelaySamples, (size_t) matrix * m.blockSize));
    CK_RT (pin (&m.hFrGainsPrev,  matrix));
    CK_RT (pin (&m.hFrGainsCurr,  matrix));
    CK_RT (pin (&m.hHfAttenDb,    matrix));
    CK_RT (pin (&m.hFrHfAttenDb,  matrix));

    CK_RT (hipMalloc (&m.dIn,   (size_t) m.numIn  * m.blockSize * sizeof (float)));
    CK_RT (hipMalloc (&m.dFrIn, (size_t) m.numIn  * m.blockSize * sizeof (float)));
    CK_RT (hipMalloc (&m.dOut,  (size_t) m.numOut * m.blockSize * sizeof (float)));
    {
        const size_t accBytes = (size_t) matrix * m.accLen * sizeof (float);
        if (hipMalloc (&m.dPairAcc, accBytes) != hipSuccess)
        {
            lastError = "HIP: per-pair accumulator allocation failed ("
                        + std::to_string (accBytes / (1024 * 1024)) + " MB)";
            release();
            return false;
        }
    }
    CK_RT (hipMalloc (&m.dPairOut, (size_t) matrix * m.blockSize * sizeof (float)));
    CK_RT (hipMalloc (&m.dShelfState,   (size_t) matrix * 4 * sizeof (float)));
    CK_RT (hipMalloc (&m.dFrShelfState, (size_t) matrix * 4 * sizeof (float)));
    CK_RT (hipMalloc (&m.dDelaysPrev, matrix * sizeof (float)));
    CK_RT (hipMalloc (&m.dDelaysCurr, matrix * sizeof (float)));
    CK_RT (hipMalloc (&m.dGainsPrev,  matrix * sizeof (float)));
    CK_RT (hipMalloc (&m.dGainsCurr,  matrix * sizeof (float)));
    CK_RT (hipMalloc (&m.dFrDelaySamples, (size_t) matrix * m.blockSize * sizeof (float)));
    CK_RT (hipMalloc (&m.dFrGainsPrev,  matrix * sizeof (float)));
    CK_RT (hipMalloc (&m.dFrGainsCurr,  matrix * sizeof (float)));
    CK_RT (hipMalloc (&m.dHfAttenDb,    matrix * sizeof (float)));
    CK_RT (hipMalloc (&m.dFrHfAttenDb,  matrix * sizeof (float)));

    CK_RT (hipMemset (m.dPairAcc, 0, (size_t) matrix * m.accLen * sizeof (float)));
    CK_RT (hipMemset (m.dShelfState,   0, (size_t) matrix * 4 * sizeof (float)));
    CK_RT (hipMemset (m.dFrShelfState, 0, (size_t) matrix * 4 * sizeof (float)));

    m.delaysPrevSamples.assign (matrix, 1.0f);
    m.gainsPrev.assign (matrix, 0.0f);
    m.frBasePrev.assign (matrix, 1.0f);
    m.frGainsPrev.assign (matrix, 0.0f);

    m.frHost.prepare (m.numIn, m.numOut, sampleRate);

    ready = true;
    lastError.clear();
    return true;
}

void HipObBackend::setMatrixPointers (const float* delaysMsPtr, const float* gainsPtr,
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

void HipObBackend::setFRFilterParams (int inputIndex,
                                       bool lowCutActive, float lowCutFreq,
                                       bool highShelfActive, float highShelfFreq,
                                       float highShelfGain, float highShelfSlope) noexcept
{
    impl->frHost.setFRFilterParams (inputIndex, lowCutActive, lowCutFreq,
                                    highShelfActive, highShelfFreq,
                                    highShelfGain, highShelfSlope);
}

void HipObBackend::setFRDiffusion (int inputIndex, float diffusionPercent) noexcept
{
    impl->frHost.setFRDiffusion (inputIndex, diffusionPercent);
}

bool HipObBackend::processBlock (const float* const* inputs, float* const* outputs)
{
    if (! ready)
        return false;

    auto& m = *impl;
    const uint32_t matrix = (uint32_t) (m.numIn * m.numOut);
    const float srScale = (float) (m.sampleRate / 1000.0);
    const float maxDelay = m.maxDelayClamp;

    if (hipSetDevice (m.deviceIndex) != hipSuccess)
    {
        lastError = "HIP: hipSetDevice failed on pump thread";
        ready = false;
        return false;
    }

    const auto t0 = std::chrono::steady_clock::now();

    // Snapshot the live matrices -> curr (with -L compensation, clamped to
    // [1, max] - the scatter's d >= 1 contract), prev = last launch's curr.
    float* dCurr = m.hDelaysCurr;
    float* gCurr = m.hGainsCurr;
    for (uint32_t i = 0; i < matrix; ++i)
    {
        float d = m.delaysMs != nullptr ? (m.delaysMs[i] - m.latencyMs) * srScale : 0.0f;
        dCurr[i] = std::clamp (d, 1.0f, maxDelay);
        gCurr[i] = m.gains != nullptr ? m.gains[i] : 0.0f;
    }

    for (uint32_t i = 0; i < matrix; ++i)
    {
        m.hHfAttenDb[i] = m.hfAttenDb != nullptr ? m.hfAttenDb[i] : 0.0f;
        m.hFrHfAttenDb[i] = m.frHfAttenDb != nullptr ? m.frHfAttenDb[i] : 0.0f;
    }

    // FR gain: absolute frLevels (gate + level), ramped prev->curr like direct.
    for (uint32_t i = 0; i < matrix; ++i)
        m.hFrGainsCurr[i] = m.frLevels != nullptr ? m.frLevels[i] : 0.0f;

    // FR delay: PER-SAMPLE, diffusion grain sub-stepped at 64 samples and ramped
    // (CPU OutputBufferProcessor parity), clamped to d >= 1, gated on FR-active pairs.
    m.frHost.computeFrDelaysPerSample (m.delaysMs, m.frDelaysMs,
                                       m.frGainsPrev.data(), m.hFrGainsCurr,
                                       m.latencyMs, srScale, maxDelay,
                                       m.blockSize, kObSubBlock, m.frBasePrev,
                                       m.hFrDelaySamples);

    if (! m.havePrev)
    {
        std::memcpy (m.delaysPrevSamples.data(), dCurr, matrix * sizeof (float));
        std::memcpy (m.gainsPrev.data(), gCurr, matrix * sizeof (float));
        std::memcpy (m.frGainsPrev.data(), m.hFrGainsCurr, matrix * sizeof (float));
        m.havePrev = true;
    }
    std::memcpy (m.hDelaysPrev, m.delaysPrevSamples.data(), matrix * sizeof (float));
    std::memcpy (m.hGainsPrev, m.gainsPrev.data(), matrix * sizeof (float));
    std::memcpy (m.hFrGainsPrev, m.frGainsPrev.data(), matrix * sizeof (float));

    for (int ch = 0; ch < m.numIn; ++ch)
    {
        if (inputs[ch] != nullptr)
            std::memcpy (m.hIn + (size_t) ch * m.blockSize, inputs[ch], (size_t) m.blockSize * sizeof (float));
        else
            std::memset (m.hIn + (size_t) ch * m.blockSize, 0, (size_t) m.blockSize * sizeof (float));
    }
    m.frHost.filterBlock (inputs, m.hFrIn, m.blockSize);

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
    PB_RT (up (m.dFrDelaySamples, m.hFrDelaySamples, (size_t) matrix * m.blockSize));
    PB_RT (up (m.dFrGainsPrev, m.hFrGainsPrev, matrix));
    PB_RT (up (m.dFrGainsCurr, m.hFrGainsCurr, matrix));
    PB_RT (up (m.dHfAttenDb, m.hHfAttenDb, matrix));
    PB_RT (up (m.dFrHfAttenDb, m.hFrHfAttenDb, matrix));

    ObParamsGpu p { (uint32_t) m.numIn, (uint32_t) m.numOut, (uint32_t) m.blockSize,
                    m.accLen, m.writePos, m.shelfCosW0, m.shelfSinW0 };

    // K1: per-pair filter + emit + scatter (one thread per pair).
    void* pairsArgs[] = { &p, &m.dIn, &m.dFrIn, &m.dHfAttenDb, &m.dFrHfAttenDb,
                          &m.dDelaysPrev, &m.dDelaysCurr, &m.dGainsPrev, &m.dGainsCurr,
                          &m.dFrDelaySamples, &m.dFrGainsPrev, &m.dFrGainsCurr,
                          &m.dShelfState, &m.dFrShelfState, &m.dPairAcc, &m.dPairOut };
    const unsigned int pairGrid = (matrix + m.threadsPerBlock - 1) / m.threadsPerBlock;
    hipError_t lr = hipModuleLaunchKernel (m.kernelPairs,
                                  pairGrid, 1, 1,
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

    // K2: deterministic per-output reduction (stream-ordered after K1).
    void* reduceArgs[] = { &p, &m.dPairOut, &m.dOut };
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

    for (int ch = 0; ch < m.numOut; ++ch)
        if (outputs[ch] != nullptr)
            std::memcpy (outputs[ch], m.hOut + (size_t) ch * m.blockSize, (size_t) m.blockSize * sizeof (float));

    // Advance host-tracked state (FR delay continuity lives in frBasePrev,
    // updated inside computeFrDelaysPerSample).
    m.writePos = (m.writePos + (uint32_t) m.blockSize) % m.accLen;
    std::memcpy (m.delaysPrevSamples.data(), dCurr, matrix * sizeof (float));
    std::memcpy (m.gainsPrev.data(), gCurr, matrix * sizeof (float));
    std::memcpy (m.frGainsPrev.data(), m.hFrGainsCurr, matrix * sizeof (float));

    lastLaunchMs = std::chrono::duration<double, std::milli> (
                       std::chrono::steady_clock::now() - t0).count();
    return true;
}

void HipObBackend::reset() noexcept
{
    auto& m = *impl;
    hipSetDevice (m.deviceIndex);
    const size_t accBytes = (size_t) m.numIn * m.numOut * m.accLen * sizeof (float);
    const size_t stateBytes = (size_t) m.numIn * m.numOut * 4 * sizeof (float);
    if (m.dPairAcc != nullptr && accBytes > 0)
        hipMemset (m.dPairAcc, 0, accBytes);
    if (m.dShelfState != nullptr && stateBytes > 0)
        hipMemset (m.dShelfState, 0, stateBytes);
    if (m.dFrShelfState != nullptr && stateBytes > 0)
        hipMemset (m.dFrShelfState, 0, stateBytes);
    m.frHost.reset();
    m.writePos = 0;
    m.havePrev = false;
}

void HipObBackend::release() noexcept
{
    auto& m = *impl;

    hipSetDevice (m.deviceIndex);

    auto freeHost = [] (float*& p) { if (p != nullptr) { hipHostFree (p); p = nullptr; } };
    auto freeDev  = [] (void*&  p) { if (p != nullptr) { hipFree (p);     p = nullptr; } };

    freeHost (m.hIn);  freeHost (m.hFrIn);  freeHost (m.hOut);
    freeHost (m.hDelaysPrev); freeHost (m.hDelaysCurr);
    freeHost (m.hGainsPrev);  freeHost (m.hGainsCurr);
    freeHost (m.hFrDelaySamples);
    freeHost (m.hFrGainsPrev);  freeHost (m.hFrGainsCurr);
    freeHost (m.hHfAttenDb);    freeHost (m.hFrHfAttenDb);

    freeDev (m.dIn);  freeDev (m.dFrIn);  freeDev (m.dOut);
    freeDev (m.dPairAcc);  freeDev (m.dPairOut);
    freeDev (m.dShelfState); freeDev (m.dFrShelfState);
    freeDev (m.dDelaysPrev); freeDev (m.dDelaysCurr);
    freeDev (m.dGainsPrev);  freeDev (m.dGainsCurr);
    freeDev (m.dFrDelaySamples);
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
