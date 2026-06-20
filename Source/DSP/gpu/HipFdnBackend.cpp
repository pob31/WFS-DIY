/*
    HipFdnBackend implementation.

    The kernel source lives in CudaFdnKernels.h as a string literal (valid HIP),
    compiled at prepare() via hipRTC into code, loaded with the HIP driver API and
    launched with hipModuleLaunchKernel; buffers and copies use the Runtime API on a
    private stream — the same pattern as CudaWfsBackend / CudaIrBackend.

    Host-side behaviour mirrors MetalFdnBackend.mm via the shared FdnHostConfig.
    The persistent FDN state (delay/diffuser/feedback rings, write positions,
    filter states) lives in device memory across launches; only the static
    config and the per-block coefficients are uploaded host->device, and only
    the outputs are read back.

    processBlock() runs on the GpuAsyncPipelineT pump thread; it binds the
    device primary context at the top, like the WFS/IR twins.
*/

#if WFS_GPU_NATIVE && !defined(__APPLE__) && defined(WFS_GPU_HIP)

#include "HipFdnBackend.h"
#include "CudaFdnKernels.h"
#include "FdnHostConfig.h"

#include <hip/hip_runtime.h>   // HIP runtime + driver API (hipMalloc, hipModule*, hipModuleLaunchKernel, props)
#include <hip/hiprtc.h>        // hipRTC: runtime kernel compilation

// Linux links the HIP libs via the .jucer externalLibraries (-lamdhip64 -lhiprtc),
// and the Windows wfs_hip.dll links them via hipcc; no MSVC-style #pragma
// comment(lib, ...) is needed or honoured here. (A CUDA pragma here would wrongly
// pull cudart/nvrtc into the AMD plugin.)

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

namespace
{
struct FdnParamsGpu
{
    uint32_t numNodes;
    uint32_t blockSize;
    uint32_t maxDelayLen;
    uint32_t maxDiffLen;
    uint32_t maxFbApLen;
    float    toneCoeff;
    float    lowCoeff;
    float    highCoeff;
    float    diffusionCoeff;
    float    feedbackAPCoeff;
    float    dcPole;
    float    outputGain;
};
} // namespace

struct HipFdnBackend::Impl
{
    hipModule_t     module = nullptr;
    hipFunction_t   kernel = nullptr;
    hipStream_t stream = nullptr;

    // Pinned host staging.
    float* hInputs = nullptr;
    float* hOutputs = nullptr;
    float* hGainLow = nullptr;
    float* hGainMid = nullptr;
    float* hGainHigh = nullptr;

    // Device buffers.
    void* dInputs = nullptr;
    void* dOutputs = nullptr;
    void* dDelayLengths = nullptr;
    void* dDiffuserDelays = nullptr;
    void* dFbApDelays = nullptr;
    void* dNodeTapSigns = nullptr;
    void* dInputGains = nullptr;
    void* dGainLow = nullptr;
    void* dGainMid = nullptr;
    void* dGainHigh = nullptr;
    void* dDelayRings = nullptr;
    void* dDelayWritePos = nullptr;
    void* dDiffRings = nullptr;
    void* dDiffWritePos = nullptr;
    void* dFbApRings = nullptr;
    void* dFbApWritePos = nullptr;
    void* dDecayLowState = nullptr;
    void* dDecayHighState = nullptr;
    void* dToneState = nullptr;
    void* dDcState = nullptr;

    int deviceIndex = 0;             // which HIP device to bind (ctor-injected)
    int numNodes = 0, blockSize = 0;
    double sampleRate = 0.0;
    FdnParamsGpu params {};

    FdnHostConfig cfg;

    std::mutex paramMutex;
    std::atomic<bool> paramsDirty { false };
    float pRt60 = 1.5f, pLowMult = 1.3f, pHighMult = 0.5f;
    float pXLow = 200.0f, pXHigh = 4000.0f, pDiffusion = 0.5f;

    std::atomic<bool> resetRequested { false };
};

HipFdnBackend::HipFdnBackend (int deviceIndex) : impl (std::make_unique<Impl>()) { impl->deviceIndex = deviceIndex; }
HipFdnBackend::~HipFdnBackend() { release(); }

#define CK_RT(call)  do { hipError_t _e = (call); if (_e != hipSuccess) { \
    lastError = std::string ("HIP runtime: ") + hipGetErrorString (_e); release(); return false; } } while (0)
#define CK_DRV(call) do { hipError_t _e = (call); if (_e != hipSuccess) { const char* _s = nullptr; \
    hipDrvGetErrorString (_e, &_s); lastError = std::string ("HIP driver: ") + (_s ? _s : "unknown"); \
    release(); return false; } } while (0)

bool HipFdnBackend::prepare (int numNodes, int blockSize, double sampleRate, float fdnSize)
{
    release();
    auto& m = *impl;

    m.numNodes = std::max (1, numNodes);
    m.blockSize = std::max (1, blockSize);
    m.sampleRate = sampleRate;
    m.cfg.prepare (m.numNodes, sampleRate, fdnSize);

    int devCount = 0;
    CK_RT (hipGetDeviceCount (&devCount));
    if (devCount == 0) { lastError = "No HIP device available"; return false; }
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
        if (hiprtcCreateProgram (&prog, kFdnProcessKernelSource, "fdn_process.cu", 0, nullptr, nullptr) != HIPRTC_SUCCESS)
        {
            lastError = "hipRTC: program creation failed";
            release();
            return false;
        }
        const std::string archOpt = "--offload-arch=" + archName;
        const char* opts[] = { archOpt.c_str() };
        if (hiprtcCompileProgram (prog, 1, opts) != HIPRTC_SUCCESS)
        {
            size_t logSize = 0;
            hiprtcGetProgramLogSize (prog, &logSize);
            std::string log (logSize, '\0');
            if (logSize > 0) hiprtcGetProgramLog (prog, &log[0]);
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
        CK_DRV (hipModuleGetFunction (&m.kernel, m.module, "fdn_process"));
    }

    const int N = m.numNodes;
    const int L = FdnHostConfig::NUM_LINES;
    const int D = FdnHostConfig::NUM_DIFFUSERS;
    const size_t maxDelayLen = (size_t) m.cfg.maxDelayLen;
    const size_t maxDiffLen  = (size_t) m.cfg.maxDiffLen;
    const size_t maxFbApLen  = (size_t) m.cfg.maxFbApLen;

    CK_RT (hipStreamCreate (&m.stream));

    auto pin = [] (float** p, size_t n) {
        return hipHostMalloc ((void**) p, n * sizeof (float), hipHostMallocDefault);
    };
    CK_RT (pin (&m.hInputs,  (size_t) N * m.blockSize));
    CK_RT (pin (&m.hOutputs, (size_t) N * m.blockSize));
    CK_RT (pin (&m.hGainLow,  (size_t) N * L));
    CK_RT (pin (&m.hGainMid,  (size_t) N * L));
    CK_RT (pin (&m.hGainHigh, (size_t) N * L));

    auto devF = [] (void** p, size_t n) { return hipMalloc (p, n * sizeof (float)); };
    auto devI = [] (void** p, size_t n) { return hipMalloc (p, n * sizeof (int));   };
    CK_RT (devF (&m.dInputs,  (size_t) N * m.blockSize));
    CK_RT (devF (&m.dOutputs, (size_t) N * m.blockSize));
    CK_RT (devI (&m.dDelayLengths,   (size_t) N * L));
    CK_RT (devI (&m.dDiffuserDelays, (size_t) N * D));
    CK_RT (devI (&m.dFbApDelays,     (size_t) N * L));
    CK_RT (devF (&m.dNodeTapSigns,   (size_t) N * L));
    CK_RT (devF (&m.dInputGains,     (size_t) L));
    CK_RT (devF (&m.dGainLow,  (size_t) N * L));
    CK_RT (devF (&m.dGainMid,  (size_t) N * L));
    CK_RT (devF (&m.dGainHigh, (size_t) N * L));
    CK_RT (devF (&m.dDelayRings,    (size_t) N * L * maxDelayLen));
    CK_RT (devI (&m.dDelayWritePos, (size_t) N * L));
    CK_RT (devF (&m.dDiffRings,     (size_t) N * D * maxDiffLen));
    CK_RT (devI (&m.dDiffWritePos,  (size_t) N * D));
    CK_RT (devF (&m.dFbApRings,     (size_t) N * L * maxFbApLen));
    CK_RT (devI (&m.dFbApWritePos,  (size_t) N * L));
    CK_RT (devF (&m.dDecayLowState,  (size_t) N * L));
    CK_RT (devF (&m.dDecayHighState, (size_t) N * L));
    CK_RT (devF (&m.dToneState, (size_t) N));
    CK_RT (devF (&m.dDcState,   (size_t) N * 2));

    // Upload the static config + initial coefficients (one-time).
    CK_RT (hipMemcpy (m.dDelayLengths,   m.cfg.delayLengths.data(),   (size_t) N * L * sizeof (int), hipMemcpyHostToDevice));
    CK_RT (hipMemcpy (m.dDiffuserDelays, m.cfg.diffuserDelays.data(), (size_t) N * D * sizeof (int), hipMemcpyHostToDevice));
    CK_RT (hipMemcpy (m.dFbApDelays,     m.cfg.fbApDelays.data(),     (size_t) N * L * sizeof (int), hipMemcpyHostToDevice));
    CK_RT (hipMemcpy (m.dNodeTapSigns,   m.cfg.nodeTapSigns.data(),   (size_t) N * L * sizeof (float), hipMemcpyHostToDevice));
    CK_RT (hipMemcpy (m.dInputGains,     FdnHostConfig::getInputGains().data(), (size_t) L * sizeof (float), hipMemcpyHostToDevice));
    CK_RT (hipMemcpy (m.dGainLow,  m.cfg.gainLow.data(),  (size_t) N * L * sizeof (float), hipMemcpyHostToDevice));
    CK_RT (hipMemcpy (m.dGainMid,  m.cfg.gainMid.data(),  (size_t) N * L * sizeof (float), hipMemcpyHostToDevice));
    CK_RT (hipMemcpy (m.dGainHigh, m.cfg.gainHigh.data(), (size_t) N * L * sizeof (float), hipMemcpyHostToDevice));

    // Zero the persistent state.
    CK_RT (hipMemset (m.dDelayRings,    0, (size_t) N * L * maxDelayLen * sizeof (float)));
    CK_RT (hipMemset (m.dDelayWritePos, 0, (size_t) N * L * sizeof (int)));
    CK_RT (hipMemset (m.dDiffRings,     0, (size_t) N * D * maxDiffLen * sizeof (float)));
    CK_RT (hipMemset (m.dDiffWritePos,  0, (size_t) N * D * sizeof (int)));
    CK_RT (hipMemset (m.dFbApRings,     0, (size_t) N * L * maxFbApLen * sizeof (float)));
    CK_RT (hipMemset (m.dFbApWritePos,  0, (size_t) N * L * sizeof (int)));
    CK_RT (hipMemset (m.dDecayLowState,  0, (size_t) N * L * sizeof (float)));
    CK_RT (hipMemset (m.dDecayHighState, 0, (size_t) N * L * sizeof (float)));
    CK_RT (hipMemset (m.dToneState, 0, (size_t) N * sizeof (float)));
    CK_RT (hipMemset (m.dDcState,   0, (size_t) N * 2 * sizeof (float)));

    m.params = FdnParamsGpu { (uint32_t) N, (uint32_t) m.blockSize,
                              (uint32_t) m.cfg.maxDelayLen, (uint32_t) m.cfg.maxDiffLen,
                              (uint32_t) m.cfg.maxFbApLen,
                              m.cfg.toneCoeff, m.cfg.lowCoeff, m.cfg.highCoeff,
                              m.cfg.diffusionCoeff, FdnHostConfig::FEEDBACK_AP_COEFF,
                              FdnHostConfig::DC_POLE, FdnHostConfig::OUTPUT_GAIN };

    ready = true;
    lastError.clear();
    return true;
}

void HipFdnBackend::setParameters (float rt60, float rt60LowMult, float rt60HighMult,
                                    float crossoverLow, float crossoverHigh,
                                    float diffusion) noexcept
{
    auto& m = *impl;
    std::lock_guard<std::mutex> lock (m.paramMutex);
    m.pRt60 = rt60; m.pLowMult = rt60LowMult; m.pHighMult = rt60HighMult;
    m.pXLow = crossoverLow; m.pXHigh = crossoverHigh; m.pDiffusion = diffusion;
    m.paramsDirty.store (true, std::memory_order_release);
}

void HipFdnBackend::requestReset() noexcept
{
    impl->resetRequested.store (true, std::memory_order_release);
}

bool HipFdnBackend::processBlock (const float* const* inputs, float* const* outputs)
{
    if (! ready)
        return false;

    auto& m = *impl;
    const int N = m.numNodes;
    const int L = FdnHostConfig::NUM_LINES;
    const int D = FdnHostConfig::NUM_DIFFUSERS;
    const size_t maxDelayLen = (size_t) m.cfg.maxDelayLen;
    const size_t maxDiffLen  = (size_t) m.cfg.maxDiffLen;
    const size_t maxFbApLen  = (size_t) m.cfg.maxFbApLen;

    if (hipSetDevice (m.deviceIndex) != hipSuccess)
    {
        lastError = "HIP: hipSetDevice failed on pump thread";
        ready = false;
        return false;
    }

    const auto t0 = std::chrono::steady_clock::now();

#define PB_RT(call) do { hipError_t _e = (call); if (_e != hipSuccess) { \
    lastError = std::string ("HIP runtime: ") + hipGetErrorString (_e); ready = false; return false; } } while (0)

    if (m.resetRequested.exchange (false, std::memory_order_acq_rel))
    {
        PB_RT (hipMemsetAsync (m.dDelayRings,    0, (size_t) N * L * maxDelayLen * sizeof (float), m.stream));
        PB_RT (hipMemsetAsync (m.dDelayWritePos, 0, (size_t) N * L * sizeof (int), m.stream));
        PB_RT (hipMemsetAsync (m.dDiffRings,     0, (size_t) N * D * maxDiffLen * sizeof (float), m.stream));
        PB_RT (hipMemsetAsync (m.dDiffWritePos,  0, (size_t) N * D * sizeof (int), m.stream));
        PB_RT (hipMemsetAsync (m.dFbApRings,     0, (size_t) N * L * maxFbApLen * sizeof (float), m.stream));
        PB_RT (hipMemsetAsync (m.dFbApWritePos,  0, (size_t) N * L * sizeof (int), m.stream));
        PB_RT (hipMemsetAsync (m.dDecayLowState,  0, (size_t) N * L * sizeof (float), m.stream));
        PB_RT (hipMemsetAsync (m.dDecayHighState, 0, (size_t) N * L * sizeof (float), m.stream));
        PB_RT (hipMemsetAsync (m.dToneState, 0, (size_t) N * sizeof (float), m.stream));
        PB_RT (hipMemsetAsync (m.dDcState,   0, (size_t) N * 2 * sizeof (float), m.stream));
    }

    if (m.paramsDirty.load (std::memory_order_acquire))
    {
        float rt60, lowMult, highMult, xLow, xHigh, diffusion;
        {
            std::lock_guard<std::mutex> lock (m.paramMutex);
            m.paramsDirty.store (false, std::memory_order_release);
            rt60 = m.pRt60; lowMult = m.pLowMult; highMult = m.pHighMult;
            xLow = m.pXLow; xHigh = m.pXHigh; diffusion = m.pDiffusion;
        }
        m.cfg.setParameters (rt60, lowMult, highMult, xLow, xHigh, diffusion);

        std::memcpy (m.hGainLow,  m.cfg.gainLow.data(),  (size_t) N * L * sizeof (float));
        std::memcpy (m.hGainMid,  m.cfg.gainMid.data(),  (size_t) N * L * sizeof (float));
        std::memcpy (m.hGainHigh, m.cfg.gainHigh.data(), (size_t) N * L * sizeof (float));
        PB_RT (hipMemcpyAsync (m.dGainLow,  m.hGainLow,  (size_t) N * L * sizeof (float), hipMemcpyHostToDevice, m.stream));
        PB_RT (hipMemcpyAsync (m.dGainMid,  m.hGainMid,  (size_t) N * L * sizeof (float), hipMemcpyHostToDevice, m.stream));
        PB_RT (hipMemcpyAsync (m.dGainHigh, m.hGainHigh, (size_t) N * L * sizeof (float), hipMemcpyHostToDevice, m.stream));

        m.params.lowCoeff = m.cfg.lowCoeff;
        m.params.highCoeff = m.cfg.highCoeff;
        m.params.diffusionCoeff = m.cfg.diffusionCoeff;
    }

    for (int n = 0; n < N; ++n)
    {
        if (inputs[n] != nullptr)
            std::memcpy (m.hInputs + (size_t) n * m.blockSize, inputs[n], (size_t) m.blockSize * sizeof (float));
        else
            std::memset (m.hInputs + (size_t) n * m.blockSize, 0, (size_t) m.blockSize * sizeof (float));
    }
    PB_RT (hipMemcpyAsync (m.dInputs, m.hInputs, (size_t) N * m.blockSize * sizeof (float), hipMemcpyHostToDevice, m.stream));

    void* args[] = {
        &m.params, &m.dInputs, &m.dOutputs,
        &m.dDelayLengths, &m.dDiffuserDelays, &m.dFbApDelays, &m.dNodeTapSigns, &m.dInputGains,
        &m.dGainLow, &m.dGainMid, &m.dGainHigh,
        &m.dDelayRings, &m.dDelayWritePos, &m.dDiffRings, &m.dDiffWritePos,
        &m.dFbApRings, &m.dFbApWritePos, &m.dDecayLowState, &m.dDecayHighState,
        &m.dToneState, &m.dDcState
    };

    const hipError_t lr = hipModuleLaunchKernel (m.kernel,
                                        (unsigned int) N, 1, 1,
                                        (unsigned int) L, 1, 1,
                                        0, (hipStream_t) m.stream,
                                        args, nullptr);
    if (lr != hipSuccess)
    {
        const char* s = nullptr;
        hipDrvGetErrorString (lr, &s);
        lastError = std::string ("HIP launch failed (fdn_process): ") + (s ? s : "unknown");
        ready = false;
        return false;
    }

    PB_RT (hipMemcpyAsync (m.hOutputs, m.dOutputs, (size_t) N * m.blockSize * sizeof (float), hipMemcpyDeviceToHost, m.stream));
    PB_RT (hipStreamSynchronize (m.stream));

#undef PB_RT

    for (int n = 0; n < N; ++n)
        if (outputs[n] != nullptr)
            std::memcpy (outputs[n], m.hOutputs + (size_t) n * m.blockSize, (size_t) m.blockSize * sizeof (float));

    lastLaunchMs = std::chrono::duration<double, std::milli> (
                       std::chrono::steady_clock::now() - t0).count();
    return true;
}

void HipFdnBackend::release() noexcept
{
    auto& m = *impl;
    hipSetDevice (m.deviceIndex);

    auto freeHost = [] (float*& p) { if (p) { hipHostFree (p); p = nullptr; } };
    auto freeDev  = [] (void*&  p) { if (p) { hipFree (p);     p = nullptr; } };

    freeHost (m.hInputs); freeHost (m.hOutputs);
    freeHost (m.hGainLow); freeHost (m.hGainMid); freeHost (m.hGainHigh);

    freeDev (m.dInputs); freeDev (m.dOutputs);
    freeDev (m.dDelayLengths); freeDev (m.dDiffuserDelays); freeDev (m.dFbApDelays);
    freeDev (m.dNodeTapSigns); freeDev (m.dInputGains);
    freeDev (m.dGainLow); freeDev (m.dGainMid); freeDev (m.dGainHigh);
    freeDev (m.dDelayRings); freeDev (m.dDelayWritePos);
    freeDev (m.dDiffRings); freeDev (m.dDiffWritePos);
    freeDev (m.dFbApRings); freeDev (m.dFbApWritePos);
    freeDev (m.dDecayLowState); freeDev (m.dDecayHighState);
    freeDev (m.dToneState); freeDev (m.dDcState);

    if (m.stream != nullptr) { hipStreamDestroy (m.stream); m.stream = nullptr; }
    if (m.module != nullptr) { hipModuleUnload (m.module); m.module = nullptr; }
    m.kernel = nullptr;

    ready = false;
}

#undef CK_RT
#undef CK_DRV

#endif // WFS_GPU_NATIVE && !defined(__APPLE__) && defined(WFS_GPU_HIP)
