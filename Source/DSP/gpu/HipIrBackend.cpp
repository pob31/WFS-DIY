/*
    HipIrBackend implementation.

    The kernel source lives in CudaIrKernels.h as a string literal, compiled
    at prepare() time via NVRTC into PTX, loaded with the CUDA Driver API and
    launched with hipModuleLaunchKernel; buffers and copies use the Runtime API on a
    private stream — the exact pattern of CudaWfsBackend.cpp.

    Host-side behaviour mirrors MetalIrBackend.mm via the shared
    IrConvHostState; the only CUDA-specific work is the pinned staging for
    the per-launch spectra (host FFTs can't write device memory directly,
    unlike Metal's shared storage):
        hIrSpectra  [segCapacity][fftLen]   full-size pinned IR staging;
                                            newly transformed segment ranges
                                            are uploaded incrementally
        hInSpectra  [numNodes][fftLen]      this launch's input spectra,
                                            scattered into the device ring
                                            with one strided 2D copy
        hOutSpectra [numNodes][fftLen]      accumulated products, downloaded

    Threading note: processBlock() runs on the GpuAsyncPipelineT pump thread;
    it binds the device's primary context at the top (cheap TLS write), like
    the WFS twin.
*/

// Only the Windows/NVIDIA build pulls in the CUDA toolkit; on macOS the Metal
// backend is used and this compiles to an empty TU (same as CudaWfsBackend).
#if WFS_GPU_NATIVE && !defined(__APPLE__) && defined(WFS_GPU_HIP)

#include "HipIrBackend.h"
#include "CudaIrKernels.h"
#include "IrConvHostState.h"

#include <hip/hip_runtime.h>           // driver API: hipCtx_t, cuModule*, hipModuleLaunchKernel
#include <hip/hip_runtime.h>   // runtime API: hipMalloc, hipHostMalloc, hipMemcpyAsync
#include <hip/hiprtc.h>

// Same linkage approach as CudaWfsBackend.cpp (Projucer's externalLibraries
// does not reach AdditionalDependencies for this project).
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
// Host mirror of the kernel-side IrParams - layouts must match exactly.
struct IrParamsGpu
{
    uint32_t numNodes;
    uint32_t bins;
    uint32_t segCapacity;
    uint32_t segmentsLoaded;
    uint32_t ringHead;
};

// Progressive IR loader budget (see MetalIrBackend.mm for the rationale).
constexpr int kIrSegmentsPerLaunch = 64;
} // namespace

struct HipIrBackend::Impl
{
    hipCtx_t    context = nullptr;   // device primary context (retained)
    hipDevice_t     cuDevice = 0;
    hipModule_t     module = nullptr;
    hipFunction_t   kernelMac = nullptr;
    hipStream_t stream = nullptr;

    // Pinned host staging.
    float* hIrSpectra = nullptr;   // [segCapacity][fftLen]
    float* hInSpectra = nullptr;   // [numNodes][fftLen]
    float* hOutSpectra = nullptr;  // [numNodes][fftLen]

    // Device buffers.
    void* dIrSpectra = nullptr;    // [segCapacity][fftLen]
    void* dInSpectra = nullptr;    // [numNodes][segCapacity][fftLen]
    void* dOutSpectra = nullptr;   // [numNodes][fftLen]

    int numNodes = 0, blockSize = 0, fftLen = 0;
    double sampleRate = 0.0;

    IrConvHostState host;
};

HipIrBackend::HipIrBackend() : impl (std::make_unique<Impl>()) {}
HipIrBackend::~HipIrBackend() { release(); }

// prepare()-only error helpers: set lastError, tear down, return false.
#define CK_RT(call)  do { hipError_t _e = (call); if (_e != hipSuccess) { \
    lastError = std::string ("HIP runtime: ") + hipGetErrorString (_e); release(); return false; } } while (0)
#define CK_DRV(call) do { hipError_t _e = (call); if (_e != hipSuccess) { const char* _s = nullptr; \
    hipDrvGetErrorString (_e, &_s); lastError = std::string ("HIP driver: ") + (_s ? _s : "unknown"); \
    release(); return false; } } while (0)

bool HipIrBackend::prepare (int numNodes, int blockSize,
                             double sampleRate, int maxIrSamples)
{
    release();
    auto& m = *impl;

    if (! m.host.prepare (numNodes, blockSize, maxIrSamples))
    {
        lastError = "Unsupported reverb block size " + std::to_string (blockSize)
                    + " (need a power of two in [4, 1024])";
        return false;
    }

    // 1) Pick device 0, report its name, derive the SM architecture.
    int devCount = 0;
    CK_RT (hipGetDeviceCount (&devCount));
    if (devCount == 0)
    {
        lastError = "No HIP device available";
        return false;
    }
    CK_RT (hipSetDevice (0));

    hipDeviceProp_t prop;
    CK_RT (hipGetDeviceProperties (&prop, 0));
    deviceName = std::string (prop.name) + " (HIP)";
    const std::string archName = prop.gcnArchName;

    if (blockSize > prop.maxThreadsPerBlock)
    {
        lastError = "Block size exceeds CUDA thread-block limit";
        return false;
    }

    // 2) Init the driver API and retain the primary context the runtime uses.
    CK_DRV (hipInit (0));
    CK_DRV (hipDeviceGet (&m.cuDevice, 0));
    CK_DRV (hipDevicePrimaryCtxRetain (&m.context, m.cuDevice));
    CK_DRV (hipCtxSetCurrent (m.context));

    // 3) NVRTC-compile the kernel string to PTX for this GPU's arch.
    {
        hiprtcProgram prog = nullptr;
        if (hiprtcCreateProgram (&prog, kIrFdlMacKernelSource, "ir_fdl_mac.cu", 0, nullptr, nullptr) != HIPRTC_SUCCESS)
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
        CK_DRV (hipModuleGetFunction (&m.kernelMac, m.module, "ir_fdl_mac"));
    }

    // 4) Geometry + buffers.
    m.numNodes = m.host.getNumNodes();
    m.blockSize = m.host.getBlockSize();
    m.fftLen = m.host.getFftLen();
    m.sampleRate = sampleRate;
    const size_t segCap = (size_t) m.host.getSegCapacity();

    CK_RT (hipStreamCreate (&m.stream));

    auto pin = [] (float** p, size_t n) {
        return hipHostMalloc ((void**) p, n * sizeof (float), hipHostMallocDefault);
    };
    CK_RT (pin (&m.hIrSpectra, segCap * (size_t) m.fftLen));
    CK_RT (pin (&m.hInSpectra, (size_t) m.numNodes * (size_t) m.fftLen));
    CK_RT (pin (&m.hOutSpectra, (size_t) m.numNodes * (size_t) m.fftLen));

    CK_RT (hipMalloc (&m.dIrSpectra, segCap * (size_t) m.fftLen * sizeof (float)));
    CK_RT (hipMalloc (&m.dInSpectra, (size_t) m.numNodes * segCap * (size_t) m.fftLen * sizeof (float)));
    CK_RT (hipMalloc (&m.dOutSpectra, (size_t) m.numNodes * (size_t) m.fftLen * sizeof (float)));

    // Unwritten ring slots must read as silence history.
    CK_RT (hipMemset (m.dInSpectra, 0,
                       (size_t) m.numNodes * segCap * (size_t) m.fftLen * sizeof (float)));

    ready = true;
    lastError.clear();
    return true;
}

void HipIrBackend::stageIr (const float* monoIr, int numSamples)
{
    impl->host.stageIr (monoIr, numSamples);
}

void HipIrBackend::requestReset() noexcept
{
    impl->host.requestReset();
}

int HipIrBackend::getSegmentsLoaded() const noexcept { return impl->host.getSegmentsLoaded(); }
int HipIrBackend::getSegmentsTotal() const noexcept  { return impl->host.getSegmentsTotal(); }

bool HipIrBackend::processBlock (const float* const* inputs, float* const* outputs)
{
    if (! ready)
        return false;

    auto& m = *impl;

    // processBlock runs on the pump thread; bind the primary context here.
    if (hipCtxSetCurrent (m.context) != hipSuccess)
    {
        lastError = "HIP driver: hipCtxSetCurrent failed on pump thread";
        ready = false;
        return false;
    }

    const auto t0 = std::chrono::steady_clock::now();
    const size_t segCap = (size_t) m.host.getSegCapacity();
    const size_t specBytes = (size_t) m.fftLen * sizeof (float);

#define PB_RT(call) do { hipError_t _e = (call); if (_e != hipSuccess) { \
    lastError = std::string ("HIP runtime: ") + hipGetErrorString (_e); ready = false; return false; } } while (0)

    if (m.host.consumeResetRequest())
        PB_RT (hipMemsetAsync (m.dInSpectra, 0,
                                (size_t) m.numNodes * segCap * specBytes, m.stream));

    // Progressive IR load: FFT a budgeted batch into the pinned staging and
    // upload exactly the freshly written contiguous segment range.
    m.host.consumeStagedIr();
    {
        const int before = m.host.getSegmentsLoaded();
        const int written = m.host.loadMoreSegments (m.hIrSpectra, kIrSegmentsPerLaunch);
        if (written > 0)
            PB_RT (hipMemcpyAsync ((char*) m.dIrSpectra + (size_t) before * specBytes,
                                    m.hIrSpectra + (size_t) before * (size_t) m.fftLen,
                                    (size_t) written * specBytes,
                                    hipMemcpyHostToDevice, m.stream));
    }

    // Newest input spectra -> pinned staging -> one strided 2D copy into each
    // node's ring slot.
    m.host.advanceRing();
    const size_t head = (size_t) m.host.getRingHead();
    for (int node = 0; node < m.numNodes; ++node)
        m.host.transformInput (inputs[node],
                               m.hInSpectra + (size_t) node * (size_t) m.fftLen);
    PB_RT (hipMemcpy2DAsync ((char*) m.dInSpectra + head * specBytes,
                              segCap * specBytes,
                              m.hInSpectra, specBytes,
                              specBytes, (size_t) m.numNodes,
                              hipMemcpyHostToDevice, m.stream));

    // No IR yet: wet output is silence, but the ring copy above still has to
    // land before the pinned staging is reused next launch.
    if (m.host.getSegmentsLoaded() == 0)
    {
        PB_RT (hipStreamSynchronize (m.stream));
        for (int node = 0; node < m.numNodes; ++node)
            if (outputs[node] != nullptr)
                std::memset (outputs[node], 0, (size_t) m.blockSize * sizeof (float));
        lastLaunchMs = std::chrono::duration<double, std::milli> (
                           std::chrono::steady_clock::now() - t0).count();
        return true;
    }

    IrParamsGpu p { (uint32_t) m.numNodes, (uint32_t) m.blockSize,
                    (uint32_t) segCap, (uint32_t) m.host.getSegmentsLoaded(),
                    (uint32_t) head };

    void* args[] = { &p, &m.dIrSpectra, &m.dInSpectra, &m.dOutSpectra };
    const hipError_t lr = hipModuleLaunchKernel (m.kernelMac,
                                        (unsigned int) m.numNodes, 1, 1,
                                        (unsigned int) m.blockSize, 1, 1,
                                        0, (hipStream_t) m.stream,
                                        args, nullptr);
    if (lr != hipSuccess)
    {
        const char* s = nullptr;
        hipDrvGetErrorString (lr, &s);
        lastError = std::string ("HIP launch failed (ir_fdl_mac): ") + (s ? s : "unknown");
        ready = false;
        return false;
    }

    PB_RT (hipMemcpyAsync (m.hOutSpectra, m.dOutSpectra,
                            (size_t) m.numNodes * specBytes,
                            hipMemcpyDeviceToHost, m.stream));
    PB_RT (hipStreamSynchronize (m.stream));

#undef PB_RT

    // Accumulated spectra -> inverse FFT + overlap-add per node.
    for (int node = 0; node < m.numNodes; ++node)
        if (outputs[node] != nullptr)
            m.host.produceOutput (node,
                                  m.hOutSpectra + (size_t) node * (size_t) m.fftLen,
                                  outputs[node]);

    lastLaunchMs = std::chrono::duration<double, std::milli> (
                       std::chrono::steady_clock::now() - t0).count();
    return true;
}

void HipIrBackend::release() noexcept
{
    auto& m = *impl;

    if (m.context != nullptr)
        hipCtxSetCurrent (m.context);

    auto freeHost = [] (float*& p) { if (p != nullptr) { hipHostFree (p); p = nullptr; } };
    auto freeDev  = [] (void*&  p) { if (p != nullptr) { hipFree (p);     p = nullptr; } };

    freeHost (m.hIrSpectra); freeHost (m.hInSpectra); freeHost (m.hOutSpectra);
    freeDev (m.dIrSpectra); freeDev (m.dInSpectra); freeDev (m.dOutSpectra);

    if (m.stream != nullptr) { hipStreamDestroy (m.stream); m.stream = nullptr; }
    if (m.module != nullptr) { hipModuleUnload (m.module); m.module = nullptr; }
    m.kernelMac = nullptr;

    if (m.context != nullptr) { hipDevicePrimaryCtxRelease (m.cuDevice); m.context = nullptr; }

    ready = false;
}

#undef CK_RT
#undef CK_DRV

#endif // WFS_GPU_NATIVE && !defined(__APPLE__)
