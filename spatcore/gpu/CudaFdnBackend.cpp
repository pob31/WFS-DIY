/*
    CudaFdnBackend implementation.

    The kernel source lives in CudaFdnKernels.h as a string literal, compiled
    at prepare() via NVRTC into PTX, loaded with the CUDA Driver API and
    launched with cuLaunchKernel; buffers and copies use the Runtime API on a
    private stream — the same pattern as CudaWfsBackend / CudaIrBackend.

    Host-side behaviour mirrors MetalFdnBackend.mm via the shared FdnHostConfig.
    The persistent FDN state (delay/diffuser/feedback rings, write positions,
    filter states) lives in device memory across launches; only the static
    config and the per-block coefficients are uploaded host->device, and only
    the outputs are read back.

    processBlock() runs on the GpuAsyncPipelineT pump thread; it binds the
    device primary context at the top, like the WFS/IR twins.
*/

#if WFS_GPU_NATIVE && !defined(__APPLE__) && !defined(WFS_GPU_HIP) && !defined(WFS_GPU_PLUGINS)

#include "CudaFdnBackend.h"
#include "CudaFdnKernels.h"
#include "FdnHostConfig.h"

#include <cuda.h>
#include <cuda_runtime.h>
#include <nvrtc.h>

#if defined(_MSC_VER)
 #pragma comment(lib, "cudart.lib")
 #pragma comment(lib, "nvrtc.lib")
 #pragma comment(lib, "cuda.lib")
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

namespace spatcore::gpu {

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

struct CudaFdnBackend::Impl
{
    CUcontext    context = nullptr;
    CUdevice     cuDevice = 0;
    CUmodule     module = nullptr;
    CUfunction   kernel = nullptr;
    cudaStream_t stream = nullptr;
    cudaEvent_t  syncEvent = nullptr;  // blocking-sync end-of-block wait (no spin)

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

    int deviceIndex = 0;             // which CUDA device to bind (ctor-injected)
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

CudaFdnBackend::CudaFdnBackend (int deviceIndex) : impl (std::make_unique<Impl>()) { impl->deviceIndex = deviceIndex; }
CudaFdnBackend::~CudaFdnBackend() { release(); }

#define CK_RT(call)  do { cudaError_t _e = (call); if (_e != cudaSuccess) { \
    lastError = std::string ("CUDA runtime: ") + cudaGetErrorString (_e); release(); return false; } } while (0)
#define CK_DRV(call) do { CUresult _e = (call); if (_e != CUDA_SUCCESS) { const char* _s = nullptr; \
    cuGetErrorString (_e, &_s); lastError = std::string ("CUDA driver: ") + (_s ? _s : "unknown"); \
    release(); return false; } } while (0)

bool CudaFdnBackend::prepare (int numNodes, int blockSize, double sampleRate, float fdnSize)
{
    release();
    auto& m = *impl;

    m.numNodes = std::max (1, numNodes);
    m.blockSize = std::max (1, blockSize);
    m.sampleRate = sampleRate;
    m.cfg.prepare (m.numNodes, sampleRate, fdnSize);

    int devCount = 0;
    CK_RT (cudaGetDeviceCount (&devCount));
    if (devCount == 0) { lastError = "No CUDA device available"; return false; }
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
    const int arch = prop.major * 10 + prop.minor;

    CK_DRV (cuInit (0));
    CK_DRV (cuDeviceGet (&m.cuDevice, m.deviceIndex));
    CK_DRV (cuDevicePrimaryCtxRetain (&m.context, m.cuDevice));
    CK_DRV (cuCtxSetCurrent (m.context));

    {
        nvrtcProgram prog = nullptr;
        if (nvrtcCreateProgram (&prog, kFdnProcessKernelSource, "fdn_process.cu", 0, nullptr, nullptr) != NVRTC_SUCCESS)
        {
            lastError = "NVRTC: program creation failed";
            release();
            return false;
        }
        // sm_ + cubin, not compute_ + driver PTX JIT — see CudaWfsBackend.cpp for why.
        const std::string archOpt = "--gpu-architecture=sm_" + std::to_string (arch);
        const char* opts[] = { archOpt.c_str() };
        if (nvrtcCompileProgram (prog, 1, opts) != NVRTC_SUCCESS)
        {
            size_t logSize = 0;
            nvrtcGetProgramLogSize (prog, &logSize);
            std::string log (logSize, '\0');
            if (logSize > 0) nvrtcGetProgramLog (prog, &log[0]);
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
        CK_DRV (cuModuleGetFunction (&m.kernel, m.module, "fdn_process"));
    }

    const int N = m.numNodes;
    const int L = FdnHostConfig::NUM_LINES;
    const int D = FdnHostConfig::NUM_DIFFUSERS;
    const size_t maxDelayLen = (size_t) m.cfg.maxDelayLen;
    const size_t maxDiffLen  = (size_t) m.cfg.maxDiffLen;
    const size_t maxFbApLen  = (size_t) m.cfg.maxFbApLen;

    CK_RT (cudaStreamCreate (&m.stream));

    // End-of-block sync event: BlockingSync makes cudaEventSynchronize yield the
    // pump thread on an OS primitive instead of the spin-wait of
    // cudaStreamSynchronize; DisableTiming skips timestamp bookkeeping.
    CK_RT (cudaEventCreateWithFlags (&m.syncEvent, cudaEventBlockingSync | cudaEventDisableTiming));

    auto pin = [] (float** p, size_t n) {
        return cudaHostAlloc ((void**) p, n * sizeof (float), cudaHostAllocDefault);
    };
    CK_RT (pin (&m.hInputs,  (size_t) N * m.blockSize));
    CK_RT (pin (&m.hOutputs, (size_t) N * m.blockSize));
    CK_RT (pin (&m.hGainLow,  (size_t) N * L));
    CK_RT (pin (&m.hGainMid,  (size_t) N * L));
    CK_RT (pin (&m.hGainHigh, (size_t) N * L));

    auto devF = [] (void** p, size_t n) { return cudaMalloc (p, n * sizeof (float)); };
    auto devI = [] (void** p, size_t n) { return cudaMalloc (p, n * sizeof (int));   };
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
    CK_RT (cudaMemcpy (m.dDelayLengths,   m.cfg.delayLengths.data(),   (size_t) N * L * sizeof (int), cudaMemcpyHostToDevice));
    CK_RT (cudaMemcpy (m.dDiffuserDelays, m.cfg.diffuserDelays.data(), (size_t) N * D * sizeof (int), cudaMemcpyHostToDevice));
    CK_RT (cudaMemcpy (m.dFbApDelays,     m.cfg.fbApDelays.data(),     (size_t) N * L * sizeof (int), cudaMemcpyHostToDevice));
    CK_RT (cudaMemcpy (m.dNodeTapSigns,   m.cfg.nodeTapSigns.data(),   (size_t) N * L * sizeof (float), cudaMemcpyHostToDevice));
    CK_RT (cudaMemcpy (m.dInputGains,     FdnHostConfig::getInputGains().data(), (size_t) L * sizeof (float), cudaMemcpyHostToDevice));
    CK_RT (cudaMemcpy (m.dGainLow,  m.cfg.gainLow.data(),  (size_t) N * L * sizeof (float), cudaMemcpyHostToDevice));
    CK_RT (cudaMemcpy (m.dGainMid,  m.cfg.gainMid.data(),  (size_t) N * L * sizeof (float), cudaMemcpyHostToDevice));
    CK_RT (cudaMemcpy (m.dGainHigh, m.cfg.gainHigh.data(), (size_t) N * L * sizeof (float), cudaMemcpyHostToDevice));

    // Zero the persistent state.
    CK_RT (cudaMemset (m.dDelayRings,    0, (size_t) N * L * maxDelayLen * sizeof (float)));
    CK_RT (cudaMemset (m.dDelayWritePos, 0, (size_t) N * L * sizeof (int)));
    CK_RT (cudaMemset (m.dDiffRings,     0, (size_t) N * D * maxDiffLen * sizeof (float)));
    CK_RT (cudaMemset (m.dDiffWritePos,  0, (size_t) N * D * sizeof (int)));
    CK_RT (cudaMemset (m.dFbApRings,     0, (size_t) N * L * maxFbApLen * sizeof (float)));
    CK_RT (cudaMemset (m.dFbApWritePos,  0, (size_t) N * L * sizeof (int)));
    CK_RT (cudaMemset (m.dDecayLowState,  0, (size_t) N * L * sizeof (float)));
    CK_RT (cudaMemset (m.dDecayHighState, 0, (size_t) N * L * sizeof (float)));
    CK_RT (cudaMemset (m.dToneState, 0, (size_t) N * sizeof (float)));
    CK_RT (cudaMemset (m.dDcState,   0, (size_t) N * 2 * sizeof (float)));

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

void CudaFdnBackend::setParameters (float rt60, float rt60LowMult, float rt60HighMult,
                                    float crossoverLow, float crossoverHigh,
                                    float diffusion) noexcept
{
    auto& m = *impl;
    std::lock_guard<std::mutex> lock (m.paramMutex);
    m.pRt60 = rt60; m.pLowMult = rt60LowMult; m.pHighMult = rt60HighMult;
    m.pXLow = crossoverLow; m.pXHigh = crossoverHigh; m.pDiffusion = diffusion;
    m.paramsDirty.store (true, std::memory_order_release);
}

void CudaFdnBackend::requestReset() noexcept
{
    impl->resetRequested.store (true, std::memory_order_release);
}

bool CudaFdnBackend::processBlock (const float* const* inputs, float* const* outputs)
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

    if (cuCtxSetCurrent (m.context) != CUDA_SUCCESS)
    {
        lastError = "CUDA driver: cuCtxSetCurrent failed on pump thread";
        ready = false;
        return false;
    }
    cudaSetDevice (m.deviceIndex);   // runtime copies/memsets on m.stream target the selected device

    const auto t0 = std::chrono::steady_clock::now();

#define PB_RT(call) do { cudaError_t _e = (call); if (_e != cudaSuccess) { \
    lastError = std::string ("CUDA runtime: ") + cudaGetErrorString (_e); ready = false; return false; } } while (0)

    if (m.resetRequested.exchange (false, std::memory_order_acq_rel))
    {
        PB_RT (cudaMemsetAsync (m.dDelayRings,    0, (size_t) N * L * maxDelayLen * sizeof (float), m.stream));
        PB_RT (cudaMemsetAsync (m.dDelayWritePos, 0, (size_t) N * L * sizeof (int), m.stream));
        PB_RT (cudaMemsetAsync (m.dDiffRings,     0, (size_t) N * D * maxDiffLen * sizeof (float), m.stream));
        PB_RT (cudaMemsetAsync (m.dDiffWritePos,  0, (size_t) N * D * sizeof (int), m.stream));
        PB_RT (cudaMemsetAsync (m.dFbApRings,     0, (size_t) N * L * maxFbApLen * sizeof (float), m.stream));
        PB_RT (cudaMemsetAsync (m.dFbApWritePos,  0, (size_t) N * L * sizeof (int), m.stream));
        PB_RT (cudaMemsetAsync (m.dDecayLowState,  0, (size_t) N * L * sizeof (float), m.stream));
        PB_RT (cudaMemsetAsync (m.dDecayHighState, 0, (size_t) N * L * sizeof (float), m.stream));
        PB_RT (cudaMemsetAsync (m.dToneState, 0, (size_t) N * sizeof (float), m.stream));
        PB_RT (cudaMemsetAsync (m.dDcState,   0, (size_t) N * 2 * sizeof (float), m.stream));
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
        PB_RT (cudaMemcpyAsync (m.dGainLow,  m.hGainLow,  (size_t) N * L * sizeof (float), cudaMemcpyHostToDevice, m.stream));
        PB_RT (cudaMemcpyAsync (m.dGainMid,  m.hGainMid,  (size_t) N * L * sizeof (float), cudaMemcpyHostToDevice, m.stream));
        PB_RT (cudaMemcpyAsync (m.dGainHigh, m.hGainHigh, (size_t) N * L * sizeof (float), cudaMemcpyHostToDevice, m.stream));

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
    PB_RT (cudaMemcpyAsync (m.dInputs, m.hInputs, (size_t) N * m.blockSize * sizeof (float), cudaMemcpyHostToDevice, m.stream));

    void* args[] = {
        &m.params, &m.dInputs, &m.dOutputs,
        &m.dDelayLengths, &m.dDiffuserDelays, &m.dFbApDelays, &m.dNodeTapSigns, &m.dInputGains,
        &m.dGainLow, &m.dGainMid, &m.dGainHigh,
        &m.dDelayRings, &m.dDelayWritePos, &m.dDiffRings, &m.dDiffWritePos,
        &m.dFbApRings, &m.dFbApWritePos, &m.dDecayLowState, &m.dDecayHighState,
        &m.dToneState, &m.dDcState
    };

    const CUresult lr = cuLaunchKernel (m.kernel,
                                        (unsigned int) N, 1, 1,
                                        (unsigned int) L, 1, 1,
                                        0, (CUstream) m.stream,
                                        args, nullptr);
    if (lr != CUDA_SUCCESS)
    {
        const char* s = nullptr;
        cuGetErrorString (lr, &s);
        lastError = std::string ("CUDA launch failed (fdn_process): ") + (s ? s : "unknown");
        ready = false;
        return false;
    }

    PB_RT (cudaMemcpyAsync (m.hOutputs, m.dOutputs, (size_t) N * m.blockSize * sizeof (float), cudaMemcpyDeviceToHost, m.stream));
    PB_RT (cudaEventRecord (m.syncEvent, m.stream));
    PB_RT (cudaEventSynchronize (m.syncEvent));   // blocking-sync event: yields, no spin

#undef PB_RT

    for (int n = 0; n < N; ++n)
        if (outputs[n] != nullptr)
            std::memcpy (outputs[n], m.hOutputs + (size_t) n * m.blockSize, (size_t) m.blockSize * sizeof (float));

    lastLaunchMs = std::chrono::duration<double, std::milli> (
                       std::chrono::steady_clock::now() - t0).count();
    return true;
}

void CudaFdnBackend::release() noexcept
{
    auto& m = *impl;
    if (m.context != nullptr)
        cuCtxSetCurrent (m.context);

    auto freeHost = [] (float*& p) { if (p) { cudaFreeHost (p); p = nullptr; } };
    auto freeDev  = [] (void*&  p) { if (p) { cudaFree (p);     p = nullptr; } };

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

    if (m.syncEvent != nullptr) { cudaEventDestroy (m.syncEvent); m.syncEvent = nullptr; }
    if (m.stream != nullptr) { cudaStreamDestroy (m.stream); m.stream = nullptr; }
    if (m.module != nullptr) { cuModuleUnload (m.module); m.module = nullptr; }
    m.kernel = nullptr;

    if (m.context != nullptr) { cuDevicePrimaryCtxRelease (m.cuDevice); m.context = nullptr; }
    ready = false;
}

#undef CK_RT
#undef CK_DRV

} // namespace spatcore::gpu

#endif // WFS_GPU_NATIVE && !defined(__APPLE__)
