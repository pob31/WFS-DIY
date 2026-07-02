/*
    HipSdnBackend implementation.

    The kernel source lives in CudaSdnKernels.h as a string literal (valid HIP),
    compiled at prepare() via hipRTC into code, loaded with the HIP driver API and
    launched with hipModuleLaunchKernel; buffers and copies use the Runtime API on a
    private stream — the same pattern as CudaFdnBackend / CudaWfsBackend.

    Host-side behaviour mirrors MetalSdnBackend.mm via the shared SdnHostConfig.
    The persistent SDN state (per-path delay lines, decay-filter states, diffuser
    rings + write positions, tone/DC state) lives in device memory across
    launches; only the static config, the per-block coefficients, the geometry-
    derived delays/crossfade and the inputs are uploaded host->device, and only
    the outputs are read back.

    Dispatch: ONE block x numNodes threads (one per node), per-sample lockstep
    with __syncthreads() — the network couples nodes within a block.
*/

#if WFS_GPU_NATIVE && !defined(__APPLE__) && defined(WFS_GPU_HIP)

#include "HipSdnBackend.h"
#include "CudaSdnKernels.h"
#include "SdnHostConfig.h"

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
struct SdnParamsGpu
{
    uint32_t numNodes;
    uint32_t numPaths;
    uint32_t blockSize;
    uint32_t maxDelaySamples;
    uint32_t ringWritePos;
    uint32_t maxDiffLen;
    float    diffusionCoeff;
    float    toneCoeff;
    float    lowCoeff;
    float    highCoeff;
    float    dcPole;
    float    sdnOutputGain;
    float    inputDistribution;
    float    crossfadeRate;
};
} // namespace

struct HipSdnBackend::Impl
{
    hipModule_t     module = nullptr;
    hipFunction_t   kernel = nullptr;
    hipStream_t stream = nullptr;

    // Pinned host staging.
    float* hInputs = nullptr;
    float* hOutputs = nullptr;
    int*   hDelayLength = nullptr;
    int*   hTargetDelayLength = nullptr;
    float* hCrossfadeMix = nullptr;
    float* hGainLow = nullptr;
    float* hGainMid = nullptr;
    float* hGainHigh = nullptr;

    // Device buffers.
    void* dInputs = nullptr;
    void* dOutputs = nullptr;
    void* dDelayLines = nullptr;
    void* dDelayLength = nullptr;
    void* dTargetDelayLength = nullptr;
    void* dCrossfadeMix = nullptr;
    void* dGainLow = nullptr;
    void* dGainMid = nullptr;
    void* dGainHigh = nullptr;
    void* dDecayLowState = nullptr;
    void* dDecayHighState = nullptr;
    void* dDiffuserDelays = nullptr;
    void* dDiffRings = nullptr;
    void* dDiffWritePos = nullptr;
    void* dToneState = nullptr;
    void* dDcState = nullptr;

    int numNodes = 0, numPaths = 0, blockSize = 0;
    double sampleRate = 0.0;
    uint32_t ringWritePos = 0;
    bool needUpload = true;
    SdnParamsGpu params {};

    SdnHostConfig cfg;

    std::mutex paramMutex;
    std::atomic<bool> paramsDirty { false };
    float pRt60 = 1.5f, pLowMult = 1.3f, pHighMult = 0.5f;
    float pXLow = 200.0f, pXHigh = 4000.0f, pDiffusion = 0.5f, pSdnScale = 1.0f;

    std::mutex geomMutex;
    std::atomic<bool> geometryDirty { false };
    std::vector<float> stagedXyz;
    int stagedCount = 0;
    std::vector<SdnHostConfig::NodePos> posScratch;

    std::atomic<bool> resetRequested { false };

    int deviceIndex = 0;             // which HIP device to bind (ctor-injected)
};

HipSdnBackend::HipSdnBackend (int deviceIndex) : impl (std::make_unique<Impl>()) { impl->deviceIndex = deviceIndex; }
HipSdnBackend::~HipSdnBackend() { release(); }

#define CK_RT(call)  do { hipError_t _e = (call); if (_e != hipSuccess) { \
    lastError = std::string ("HIP runtime: ") + hipGetErrorString (_e); release(); return false; } } while (0)
#define CK_DRV(call) do { hipError_t _e = (call); if (_e != hipSuccess) { const char* _s = nullptr; \
    hipDrvGetErrorString (_e, &_s); lastError = std::string ("HIP driver: ") + (_s ? _s : "unknown"); \
    release(); return false; } } while (0)

bool HipSdnBackend::prepare (int numNodes, int blockSize, double sampleRate)
{
    release();
    auto& m = *impl;

    m.numNodes = std::min (std::max (1, numNodes), SdnHostConfig::MAX_NODES);
    m.blockSize = std::max (1, blockSize);
    m.sampleRate = sampleRate;
    m.cfg.prepare (m.numNodes, sampleRate);
    m.numPaths = m.cfg.numPaths;
    m.ringWritePos = 0;
    m.needUpload = true;

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
        if (hiprtcCreateProgram (&prog, kSdnProcessKernelSource, "sdn_process.cu", 0, nullptr, nullptr) != HIPRTC_SUCCESS)
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
        CK_DRV (hipModuleGetFunction (&m.kernel, m.module, "sdn_process"));
    }

    const int N = m.numNodes;
    const int P = std::max (1, m.numPaths);
    const int D = SdnHostConfig::NUM_DIFFUSERS;
    const size_t maxDelay = (size_t) SdnHostConfig::MAX_DELAY_SAMPLES;
    const size_t maxDiff  = (size_t) m.cfg.maxDiffLen;

    CK_RT (hipStreamCreate (&m.stream));

    auto pinF = [] (float** p, size_t n) { return hipHostMalloc ((void**) p, n * sizeof (float), hipHostMallocDefault); };
    auto pinI = [] (int**   p, size_t n) { return hipHostMalloc ((void**) p, n * sizeof (int),   hipHostMallocDefault); };
    CK_RT (pinF (&m.hInputs,  (size_t) N * m.blockSize));
    CK_RT (pinF (&m.hOutputs, (size_t) N * m.blockSize));
    CK_RT (pinI (&m.hDelayLength,       (size_t) P));
    CK_RT (pinI (&m.hTargetDelayLength, (size_t) P));
    CK_RT (pinF (&m.hCrossfadeMix, (size_t) P));
    CK_RT (pinF (&m.hGainLow,  (size_t) P));
    CK_RT (pinF (&m.hGainMid,  (size_t) P));
    CK_RT (pinF (&m.hGainHigh, (size_t) P));

    auto devF = [] (void** p, size_t n) { return hipMalloc (p, n * sizeof (float)); };
    auto devI = [] (void** p, size_t n) { return hipMalloc (p, n * sizeof (int));   };
    CK_RT (devF (&m.dInputs,  (size_t) N * m.blockSize));
    CK_RT (devF (&m.dOutputs, (size_t) N * m.blockSize));
    CK_RT (devF (&m.dDelayLines,        (size_t) P * maxDelay));
    CK_RT (devI (&m.dDelayLength,       (size_t) P));
    CK_RT (devI (&m.dTargetDelayLength, (size_t) P));
    CK_RT (devF (&m.dCrossfadeMix, (size_t) P));
    CK_RT (devF (&m.dGainLow,  (size_t) P));
    CK_RT (devF (&m.dGainMid,  (size_t) P));
    CK_RT (devF (&m.dGainHigh, (size_t) P));
    CK_RT (devF (&m.dDecayLowState,  (size_t) P));
    CK_RT (devF (&m.dDecayHighState, (size_t) P));
    CK_RT (devI (&m.dDiffuserDelays, (size_t) N * D));
    CK_RT (devF (&m.dDiffRings,      (size_t) N * D * maxDiff));
    CK_RT (devI (&m.dDiffWritePos,   (size_t) N * D));
    CK_RT (devF (&m.dToneState, (size_t) N));
    CK_RT (devF (&m.dDcState,   (size_t) N * 2));

    // Upload the static config + initial dynamic config (one-time).
    CK_RT (hipMemcpy (m.dDiffuserDelays, m.cfg.diffuserDelays.data(), (size_t) N * D * sizeof (int), hipMemcpyHostToDevice));
    CK_RT (hipMemcpy (m.dDelayLength,       m.cfg.delayLength.data(),       (size_t) P * sizeof (int), hipMemcpyHostToDevice));
    CK_RT (hipMemcpy (m.dTargetDelayLength, m.cfg.targetDelayLength.data(), (size_t) P * sizeof (int), hipMemcpyHostToDevice));
    CK_RT (hipMemcpy (m.dCrossfadeMix,      m.cfg.crossfadeMix.data(),      (size_t) P * sizeof (float), hipMemcpyHostToDevice));
    CK_RT (hipMemcpy (m.dGainLow,  m.cfg.gainLow.data(),  (size_t) P * sizeof (float), hipMemcpyHostToDevice));
    CK_RT (hipMemcpy (m.dGainMid,  m.cfg.gainMid.data(),  (size_t) P * sizeof (float), hipMemcpyHostToDevice));
    CK_RT (hipMemcpy (m.dGainHigh, m.cfg.gainHigh.data(), (size_t) P * sizeof (float), hipMemcpyHostToDevice));

    // Zero the persistent state.
    CK_RT (hipMemset (m.dDelayLines,    0, (size_t) P * maxDelay * sizeof (float)));
    CK_RT (hipMemset (m.dDecayLowState,  0, (size_t) P * sizeof (float)));
    CK_RT (hipMemset (m.dDecayHighState, 0, (size_t) P * sizeof (float)));
    CK_RT (hipMemset (m.dDiffRings,    0, (size_t) N * D * maxDiff * sizeof (float)));
    CK_RT (hipMemset (m.dDiffWritePos, 0, (size_t) N * D * sizeof (int)));
    CK_RT (hipMemset (m.dToneState, 0, (size_t) N * sizeof (float)));
    CK_RT (hipMemset (m.dDcState,   0, (size_t) N * 2 * sizeof (float)));

    m.params = SdnParamsGpu { (uint32_t) N, (uint32_t) m.numPaths, (uint32_t) m.blockSize,
                              (uint32_t) SdnHostConfig::MAX_DELAY_SAMPLES, 0u,
                              (uint32_t) m.cfg.maxDiffLen,
                              m.cfg.diffusionCoeff, m.cfg.toneCoeff, m.cfg.lowCoeff,
                              m.cfg.highCoeff, SdnHostConfig::DC_POLE, m.cfg.sdnOutputGain,
                              m.cfg.inputDistribution, m.cfg.crossfadeRate };

    ready = true;
    lastError.clear();
    return true;
}

void HipSdnBackend::setGeometry (const float* xyz, int count) noexcept
{
    auto& m = *impl;
    std::lock_guard<std::mutex> lock (m.geomMutex);
    m.stagedXyz.assign (xyz, xyz + (size_t) std::max (0, count) * 3);
    m.stagedCount = std::max (0, count);
    m.geometryDirty.store (true, std::memory_order_release);
}

void HipSdnBackend::setParameters (float rt60, float rt60LowMult, float rt60HighMult,
                                    float crossoverLow, float crossoverHigh,
                                    float diffusion, float sdnScale) noexcept
{
    auto& m = *impl;
    std::lock_guard<std::mutex> lock (m.paramMutex);
    m.pRt60 = rt60; m.pLowMult = rt60LowMult; m.pHighMult = rt60HighMult;
    m.pXLow = crossoverLow; m.pXHigh = crossoverHigh; m.pDiffusion = diffusion;
    m.pSdnScale = sdnScale;
    m.paramsDirty.store (true, std::memory_order_release);
}

void HipSdnBackend::requestReset() noexcept
{
    impl->resetRequested.store (true, std::memory_order_release);
}

bool HipSdnBackend::processBlock (const float* const* inputs, float* const* outputs)
{
    if (! ready)
        return false;

    auto& m = *impl;
    const int N = m.numNodes;

    // With 0-1 nodes the SDN cannot scatter — pass through on the host, exactly
    // like SDNAlgorithm::processBlock (N==1 copies in->out, N==0 is silence).
    if (N < 2)
    {
        for (int n = 0; n < N; ++n)
            if (outputs[n] != nullptr)
            {
                if (inputs[n] != nullptr)
                    std::memcpy (outputs[n], inputs[n], (size_t) m.blockSize * sizeof (float));
                else
                    std::memset (outputs[n], 0, (size_t) m.blockSize * sizeof (float));
            }
        return true;
    }

    const int P = std::max (1, m.numPaths);
    const int D = SdnHostConfig::NUM_DIFFUSERS;
    const size_t maxDelay = (size_t) SdnHostConfig::MAX_DELAY_SAMPLES;
    const size_t maxDiff  = (size_t) m.cfg.maxDiffLen;

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
        PB_RT (hipMemsetAsync (m.dDelayLines,    0, (size_t) P * maxDelay * sizeof (float), m.stream));
        PB_RT (hipMemsetAsync (m.dDecayLowState,  0, (size_t) P * sizeof (float), m.stream));
        PB_RT (hipMemsetAsync (m.dDecayHighState, 0, (size_t) P * sizeof (float), m.stream));
        PB_RT (hipMemsetAsync (m.dDiffRings,    0, (size_t) N * D * maxDiff * sizeof (float), m.stream));
        PB_RT (hipMemsetAsync (m.dDiffWritePos, 0, (size_t) N * D * sizeof (int), m.stream));
        PB_RT (hipMemsetAsync (m.dToneState, 0, (size_t) N * sizeof (float), m.stream));
        PB_RT (hipMemsetAsync (m.dDcState,   0, (size_t) N * 2 * sizeof (float), m.stream));
        m.ringWritePos = 0;
    }

    const bool geomDirty = m.geometryDirty.exchange (false, std::memory_order_acquire);
    const bool parDirty  = m.paramsDirty.exchange (false, std::memory_order_acquire);
    if (geomDirty || parDirty)
    {
        float rt60, lowMult, highMult, xLow, xHigh, diffusion, scale;
        {
            std::lock_guard<std::mutex> lock (m.paramMutex);
            rt60 = m.pRt60; lowMult = m.pLowMult; highMult = m.pHighMult;
            xLow = m.pXLow; xHigh = m.pXHigh; diffusion = m.pDiffusion;
            scale = m.pSdnScale;
        }

        if (geomDirty)
        {
            std::lock_guard<std::mutex> lock (m.geomMutex);
            m.posScratch.resize ((size_t) m.stagedCount);
            for (int i = 0; i < m.stagedCount; ++i)
                m.posScratch[(size_t) i] = { m.stagedXyz[(size_t) (i * 3 + 0)],
                                             m.stagedXyz[(size_t) (i * 3 + 1)],
                                             m.stagedXyz[(size_t) (i * 3 + 2)] };
            m.cfg.recalcDelays (m.posScratch.data(), m.stagedCount, scale);
        }
        else
        {
            m.cfg.recalcDelaysFromStored (scale);
        }

        m.cfg.recalcDecay (rt60, lowMult, highMult, xLow, xHigh);
        m.cfg.setDiffusion (diffusion);
        m.needUpload = true;
    }

    if (m.needUpload)
    {
        std::memcpy (m.hDelayLength,       m.cfg.delayLength.data(),       (size_t) P * sizeof (int));
        std::memcpy (m.hTargetDelayLength, m.cfg.targetDelayLength.data(), (size_t) P * sizeof (int));
        std::memcpy (m.hCrossfadeMix,      m.cfg.crossfadeMix.data(),      (size_t) P * sizeof (float));
        std::memcpy (m.hGainLow,  m.cfg.gainLow.data(),  (size_t) P * sizeof (float));
        std::memcpy (m.hGainMid,  m.cfg.gainMid.data(),  (size_t) P * sizeof (float));
        std::memcpy (m.hGainHigh, m.cfg.gainHigh.data(), (size_t) P * sizeof (float));
        PB_RT (hipMemcpyAsync (m.dDelayLength,       m.hDelayLength,       (size_t) P * sizeof (int), hipMemcpyHostToDevice, m.stream));
        PB_RT (hipMemcpyAsync (m.dTargetDelayLength, m.hTargetDelayLength, (size_t) P * sizeof (int), hipMemcpyHostToDevice, m.stream));
        PB_RT (hipMemcpyAsync (m.dCrossfadeMix,      m.hCrossfadeMix,      (size_t) P * sizeof (float), hipMemcpyHostToDevice, m.stream));
        PB_RT (hipMemcpyAsync (m.dGainLow,  m.hGainLow,  (size_t) P * sizeof (float), hipMemcpyHostToDevice, m.stream));
        PB_RT (hipMemcpyAsync (m.dGainMid,  m.hGainMid,  (size_t) P * sizeof (float), hipMemcpyHostToDevice, m.stream));
        PB_RT (hipMemcpyAsync (m.dGainHigh, m.hGainHigh, (size_t) P * sizeof (float), hipMemcpyHostToDevice, m.stream));

        m.params.diffusionCoeff = m.cfg.diffusionCoeff;
        m.params.lowCoeff = m.cfg.lowCoeff;
        m.params.highCoeff = m.cfg.highCoeff;
        m.params.sdnOutputGain = m.cfg.sdnOutputGain;
        m.params.crossfadeRate = m.cfg.crossfadeRate;
        m.needUpload = false;
    }

    for (int n = 0; n < N; ++n)
    {
        if (inputs[n] != nullptr)
            std::memcpy (m.hInputs + (size_t) n * m.blockSize, inputs[n], (size_t) m.blockSize * sizeof (float));
        else
            std::memset (m.hInputs + (size_t) n * m.blockSize, 0, (size_t) m.blockSize * sizeof (float));
    }
    PB_RT (hipMemcpyAsync (m.dInputs, m.hInputs, (size_t) N * m.blockSize * sizeof (float), hipMemcpyHostToDevice, m.stream));

    m.params.ringWritePos = m.ringWritePos;

    void* args[] = {
        &m.params, &m.dInputs, &m.dOutputs, &m.dDelayLines,
        &m.dDelayLength, &m.dTargetDelayLength, &m.dCrossfadeMix,
        &m.dGainLow, &m.dGainMid, &m.dGainHigh,
        &m.dDecayLowState, &m.dDecayHighState,
        &m.dDiffuserDelays, &m.dDiffRings, &m.dDiffWritePos,
        &m.dToneState, &m.dDcState
    };

    const hipError_t lr = hipModuleLaunchKernel (m.kernel,
                                        1, 1, 1,
                                        (unsigned int) N, 1, 1,
                                        0, (hipStream_t) m.stream,
                                        args, nullptr);
    if (lr != hipSuccess)
    {
        const char* s = nullptr;
        hipDrvGetErrorString (lr, &s);
        lastError = std::string ("HIP launch failed (sdn_process): ") + (s ? s : "unknown");
        ready = false;
        return false;
    }

    PB_RT (hipMemcpyAsync (m.hOutputs, m.dOutputs, (size_t) N * m.blockSize * sizeof (float), hipMemcpyDeviceToHost, m.stream));
    PB_RT (hipStreamSynchronize (m.stream));

#undef PB_RT

    for (int n = 0; n < N; ++n)
        if (outputs[n] != nullptr)
            std::memcpy (outputs[n], m.hOutputs + (size_t) n * m.blockSize, (size_t) m.blockSize * sizeof (float));

    if (m.cfg.advanceCrossfades (m.blockSize))
        m.needUpload = true;
    m.ringWritePos = (m.ringWritePos + (uint32_t) m.blockSize)
                     % (uint32_t) SdnHostConfig::MAX_DELAY_SAMPLES;

    lastLaunchMs = std::chrono::duration<double, std::milli> (
                       std::chrono::steady_clock::now() - t0).count();
    return true;
}

void HipSdnBackend::release() noexcept
{
    auto& m = *impl;
    hipSetDevice (m.deviceIndex);

    auto freeHostF = [] (float*& p) { if (p) { hipHostFree (p); p = nullptr; } };
    auto freeHostI = [] (int*&   p) { if (p) { hipHostFree (p); p = nullptr; } };
    auto freeDev   = [] (void*&  p) { if (p) { hipFree (p);     p = nullptr; } };

    freeHostF (m.hInputs); freeHostF (m.hOutputs);
    freeHostI (m.hDelayLength); freeHostI (m.hTargetDelayLength);
    freeHostF (m.hCrossfadeMix);
    freeHostF (m.hGainLow); freeHostF (m.hGainMid); freeHostF (m.hGainHigh);

    freeDev (m.dInputs); freeDev (m.dOutputs); freeDev (m.dDelayLines);
    freeDev (m.dDelayLength); freeDev (m.dTargetDelayLength); freeDev (m.dCrossfadeMix);
    freeDev (m.dGainLow); freeDev (m.dGainMid); freeDev (m.dGainHigh);
    freeDev (m.dDecayLowState); freeDev (m.dDecayHighState);
    freeDev (m.dDiffuserDelays); freeDev (m.dDiffRings); freeDev (m.dDiffWritePos);
    freeDev (m.dToneState); freeDev (m.dDcState);

    if (m.stream != nullptr) { hipStreamDestroy (m.stream); m.stream = nullptr; }
    if (m.module != nullptr) { hipModuleUnload (m.module); m.module = nullptr; }
    m.kernel = nullptr;

    ready = false;
}

#undef CK_RT
#undef CK_DRV

#endif // WFS_GPU_NATIVE && !defined(__APPLE__) && defined(WFS_GPU_HIP)
