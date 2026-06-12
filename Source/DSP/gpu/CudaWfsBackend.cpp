/*
    CudaWfsBackend implementation.

    The kernel source lives in CudaWfsKernels.h as a string literal so the app
    needs no .cu file and no nvcc build step; it is compiled at prepare() time
    via NVRTC (~tens of ms once) into PTX, loaded with the CUDA Driver API, and
    launched with cuLaunchKernel. Buffers and copies use the CUDA Runtime API.
    Runtime + driver API share the device's primary context.

    Host-side processBlock mirrors MetalWfsBackend.mm exactly (matrix snapshot
    with -L compensation, prev->curr ramp continuity, persistent device ring,
    host-tracked ring advance), so the audible behaviour matches the Metal twin
    and the CPU reference.

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
#if WFS_GPU_NATIVE && !defined(__APPLE__)

#include "CudaWfsBackend.h"
#include "CudaWfsKernels.h"

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
#endif

#include <algorithm>
#include <chrono>
#include <cstring>
#include <string>
#include <vector>

namespace
{
struct WfsParamsGpu
{
    uint32_t numInputs;
    uint32_t numOutputs;
    uint32_t bufferLength;
    uint32_t ringCapacity;
    uint32_t ringWritePos;
    uint32_t ringValidSamples;
};
} // namespace

struct CudaWfsBackend::Impl
{
    CUcontext    context = nullptr;   // device primary context (retained)
    CUdevice     cuDevice = 0;
    CUmodule     module = nullptr;
    CUfunction   kernel = nullptr;
    cudaStream_t stream = nullptr;

    // Pinned host staging.
    float* hIn = nullptr;
    float* hOut = nullptr;
    float* hDelaysPrev = nullptr;
    float* hDelaysCurr = nullptr;
    float* hGainsPrev = nullptr;
    float* hGainsCurr = nullptr;

    // Device buffers (ring is persistent across launches).
    void* dIn = nullptr;
    void* dOut = nullptr;
    void* dRing = nullptr;
    void* dDelaysPrev = nullptr;
    void* dDelaysCurr = nullptr;
    void* dGainsPrev = nullptr;
    void* dGainsCurr = nullptr;

    int numIn = 0, numOut = 0, blockSize = 0;
    uint32_t ringCapacity = 0;
    uint32_t ringWritePos = 0;
    uint32_t ringValid = 0;
    uint32_t maxDelaySamples = 0;
    double sampleRate = 0.0;
    float latencyMs = 0.0f;

    const float* delaysMs = nullptr; // app's live matrices (input-major, ms / linear)
    const float* gains = nullptr;

    std::vector<float> delaysPrevSamples; // last launch's end matrices (samples / linear)
    std::vector<float> gainsPrev;
    bool havePrev = false;

    unsigned int threadsPerBlock = 256;
};

CudaWfsBackend::CudaWfsBackend() : impl (std::make_unique<Impl>()) {}
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

    // 1) Pick device 0, report its name, derive the SM architecture.
    int devCount = 0;
    CK_RT (cudaGetDeviceCount (&devCount));
    if (devCount == 0)
    {
        lastError = "No CUDA device available";
        return false;
    }
    CK_RT (cudaSetDevice (0));

    cudaDeviceProp prop;
    CK_RT (cudaGetDeviceProperties (&prop, 0));
    deviceName = std::string (prop.name) + " (CUDA)";
    const int arch = prop.major * 10 + prop.minor; // e.g. Turing GTX 1650 -> 75

    // 2) Init the driver API and retain the same primary context the runtime uses.
    CK_DRV (cuInit (0));
    CK_DRV (cuDeviceGet (&m.cuDevice, 0));
    CK_DRV (cuDevicePrimaryCtxRetain (&m.context, m.cuDevice));
    CK_DRV (cuCtxSetCurrent (m.context));

    // 3) NVRTC-compile the kernel string to PTX for this GPU's arch.
    {
        nvrtcProgram prog = nullptr;
        if (nvrtcCreateProgram (&prog, kWfsDelaySumKernelSource, "wfs_delay_sum.cu", 0, nullptr, nullptr) != NVRTC_SUCCESS)
        {
            lastError = "NVRTC: program creation failed";
            release();
            return false;
        }

        const std::string archOpt = "--gpu-architecture=compute_" + std::to_string (arch);
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

        size_t ptxSize = 0;
        nvrtcGetPTXSize (prog, &ptxSize);
        std::vector<char> ptx (ptxSize);
        nvrtcGetPTX (prog, ptx.data());
        nvrtcDestroyProgram (&prog);

        CK_DRV (cuModuleLoadDataEx (&m.module, ptx.data(), 0, nullptr, nullptr));
        CK_DRV (cuModuleGetFunction (&m.kernel, m.module, "wfs_delay_sum"));
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

    const uint32_t matrix = (uint32_t) (m.numIn * m.numOut);

    // 5) Stream + pinned host staging + device buffers (+ zero the ring once).
    CK_RT (cudaStreamCreate (&m.stream));

    CK_RT (cudaHostAlloc ((void**) &m.hIn,  (size_t) m.numIn  * m.blockSize * sizeof (float), cudaHostAllocDefault));
    CK_RT (cudaHostAlloc ((void**) &m.hOut, (size_t) m.numOut * m.blockSize * sizeof (float), cudaHostAllocDefault));
    CK_RT (cudaHostAlloc ((void**) &m.hDelaysPrev, matrix * sizeof (float), cudaHostAllocDefault));
    CK_RT (cudaHostAlloc ((void**) &m.hDelaysCurr, matrix * sizeof (float), cudaHostAllocDefault));
    CK_RT (cudaHostAlloc ((void**) &m.hGainsPrev,  matrix * sizeof (float), cudaHostAllocDefault));
    CK_RT (cudaHostAlloc ((void**) &m.hGainsCurr,  matrix * sizeof (float), cudaHostAllocDefault));

    CK_RT (cudaMalloc (&m.dIn,  (size_t) m.numIn  * m.blockSize * sizeof (float)));
    CK_RT (cudaMalloc (&m.dOut, (size_t) m.numOut * m.blockSize * sizeof (float)));
    CK_RT (cudaMalloc (&m.dRing, (size_t) m.numIn * m.ringCapacity * sizeof (float)));
    CK_RT (cudaMalloc (&m.dDelaysPrev, matrix * sizeof (float)));
    CK_RT (cudaMalloc (&m.dDelaysCurr, matrix * sizeof (float)));
    CK_RT (cudaMalloc (&m.dGainsPrev,  matrix * sizeof (float)));
    CK_RT (cudaMalloc (&m.dGainsCurr,  matrix * sizeof (float)));

    CK_RT (cudaMemset (m.dRing, 0, (size_t) m.numIn * m.ringCapacity * sizeof (float)));

    m.delaysPrevSamples.assign (matrix, 0.0f);
    m.gainsPrev.assign (matrix, 0.0f);
    m.threadsPerBlock = 256;

    ready = true;
    lastError.clear();
    return true;
}

void CudaWfsBackend::setMatrixPointers (const float* delaysMsPtr, const float* gainsPtr) noexcept
{
    impl->delaysMs = delaysMsPtr;
    impl->gains = gainsPtr;
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
    // both the driver launch and the runtime copies use it.
    if (cuCtxSetCurrent (m.context) != CUDA_SUCCESS)
    {
        lastError = "CUDA driver: cuCtxSetCurrent failed on pump thread";
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
    if (! m.havePrev)
    {
        std::memcpy (m.delaysPrevSamples.data(), dCurr, matrix * sizeof (float));
        std::memcpy (m.gainsPrev.data(), gCurr, matrix * sizeof (float));
        m.havePrev = true;
    }
    std::memcpy (m.hDelaysPrev, m.delaysPrevSamples.data(), matrix * sizeof (float));
    std::memcpy (m.hGainsPrev, m.gainsPrev.data(), matrix * sizeof (float));

    // Input channels -> flat pinned buffer (silence for missing channels).
    for (int ch = 0; ch < m.numIn; ++ch)
    {
        if (inputs[ch] != nullptr)
            std::memcpy (m.hIn + (size_t) ch * m.blockSize, inputs[ch], (size_t) m.blockSize * sizeof (float));
        else
            std::memset (m.hIn + (size_t) ch * m.blockSize, 0, (size_t) m.blockSize * sizeof (float));
    }

    // Host -> device (the persistent ring stays on the device).
#define PB_RT(call) do { cudaError_t _e = (call); if (_e != cudaSuccess) { \
    lastError = std::string ("CUDA runtime: ") + cudaGetErrorString (_e); ready = false; return false; } } while (0)

    PB_RT (cudaMemcpyAsync (m.dIn, m.hIn, (size_t) m.numIn * m.blockSize * sizeof (float), cudaMemcpyHostToDevice, m.stream));
    PB_RT (cudaMemcpyAsync (m.dDelaysPrev, m.hDelaysPrev, matrix * sizeof (float), cudaMemcpyHostToDevice, m.stream));
    PB_RT (cudaMemcpyAsync (m.dDelaysCurr, m.hDelaysCurr, matrix * sizeof (float), cudaMemcpyHostToDevice, m.stream));
    PB_RT (cudaMemcpyAsync (m.dGainsPrev, m.hGainsPrev, matrix * sizeof (float), cudaMemcpyHostToDevice, m.stream));
    PB_RT (cudaMemcpyAsync (m.dGainsCurr, m.hGainsCurr, matrix * sizeof (float), cudaMemcpyHostToDevice, m.stream));

    WfsParamsGpu p { (uint32_t) m.numIn, (uint32_t) m.numOut, (uint32_t) m.blockSize,
                     m.ringCapacity, m.ringWritePos, m.ringValid };

    void* args[] = { &p, &m.dIn, &m.dOut, &m.dRing,
                     &m.dDelaysPrev, &m.dDelaysCurr, &m.dGainsPrev, &m.dGainsCurr };

    const unsigned int gridX = (unsigned int) (m.numOut + m.numIn);
    const CUresult lr = cuLaunchKernel (m.kernel,
                                        gridX, 1, 1,
                                        m.threadsPerBlock, 1, 1,
                                        0, (CUstream) m.stream,
                                        args, nullptr);
    if (lr != CUDA_SUCCESS)
    {
        const char* s = nullptr;
        cuGetErrorString (lr, &s);
        lastError = std::string ("CUDA launch failed: ") + (s ? s : "unknown");
        ready = false;
        return false;
    }

    PB_RT (cudaMemcpyAsync (m.hOut, m.dOut, (size_t) m.numOut * m.blockSize * sizeof (float), cudaMemcpyDeviceToHost, m.stream));
    PB_RT (cudaStreamSynchronize (m.stream));

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

    lastLaunchMs = std::chrono::duration<double, std::milli> (
                       std::chrono::steady_clock::now() - t0).count();
    return true;
}

void CudaWfsBackend::reset() noexcept
{
    auto& m = *impl;
    if (m.context != nullptr)
        cuCtxSetCurrent (m.context);
    if (m.dRing != nullptr && m.numIn > 0 && m.ringCapacity > 0)
        cudaMemset (m.dRing, 0, (size_t) m.numIn * m.ringCapacity * sizeof (float));
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

    freeHost (m.hIn);  freeHost (m.hOut);
    freeHost (m.hDelaysPrev); freeHost (m.hDelaysCurr);
    freeHost (m.hGainsPrev);  freeHost (m.hGainsCurr);

    freeDev (m.dIn);  freeDev (m.dOut);  freeDev (m.dRing);
    freeDev (m.dDelaysPrev); freeDev (m.dDelaysCurr);
    freeDev (m.dGainsPrev);  freeDev (m.dGainsCurr);

    if (m.stream != nullptr) { cudaStreamDestroy (m.stream); m.stream = nullptr; }
    if (m.module != nullptr) { cuModuleUnload (m.module); m.module = nullptr; }
    m.kernel = nullptr;

    if (m.context != nullptr) { cuDevicePrimaryCtxRelease (m.cuDevice); m.context = nullptr; }

    m.delaysMs = nullptr;
    m.gains = nullptr;
    m.havePrev = false;
    ready = false;
}

#undef CK_RT
#undef CK_DRV

#endif // WFS_GPU_NATIVE && !defined(__APPLE__)
