/*
    HipIrBackend implementation.

    The kernel source lives in CudaIrKernels.h as a string literal (valid HIP),
    compiled at prepare() time via hipRTC into code, loaded with the HIP driver
    API and launched with hipModuleLaunchKernel; buffers and copies use the Runtime
    API on a private stream — the exact pattern of CudaWfsBackend.cpp.

    Host-side behaviour mirrors MetalIrBackend.mm via the shared
    IrConvHostState; the only HIP-specific work is the pinned staging for
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

// Only the Linux/AMD build (WFS_GPU_HIP) pulls in the ROCm toolkit; on macOS the
// Metal backend is used and otherwise this compiles to an empty TU (same as
// CudaWfsBackend).
#if WFS_GPU_NATIVE && !defined(__APPLE__) && defined(WFS_GPU_HIP)

#include "HipIrBackend.h"
#include "CudaIrKernels.h"
#include "IrConvHostState.h"
#include "GpuHostWorkPool.h"

#include <hip/hip_runtime.h>   // HIP runtime + driver API (hipMalloc, hipModule*, hipModuleLaunchKernel, props)
#include <hip/hiprtc.h>        // hipRTC: runtime kernel compilation

// Linux links the HIP libs via the .jucer externalLibraries (-lamdhip64 -lhiprtc),
// and the Windows wfs_hip.dll links them via hipcc; no MSVC-style #pragma
// comment(lib, ...) is needed or honoured here. (A CUDA pragma here would wrongly
// pull cudart/nvrtc into the AMD plugin.)

#include <algorithm>
#include <chrono>
#include <cstring>
#include <string>
#include <vector>

namespace spatcore::gpu {

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
    hipModule_t     module = nullptr;
    hipFunction_t   kernelMac = nullptr;
    hipStream_t stream = nullptr;
    hipEvent_t  syncEvent = nullptr;  // blocking-sync end-of-block wait (no spin)

    // Pinned host staging.
    float* hIrSpectra = nullptr;   // [segCapacity][fftLen]
    float* hInSpectra = nullptr;   // [numNodes][fftLen]
    float* hOutSpectra = nullptr;  // [numNodes][fftLen]

    // Device buffers.
    void* dIrSpectra = nullptr;    // [segCapacity][fftLen]
    void* dInSpectra = nullptr;    // [numNodes][segCapacity][fftLen]
    void* dOutSpectra = nullptr;   // [numNodes][fftLen]

    int deviceIndex = 0;             // which HIP device to bind (ctor-injected)
    int numNodes = 0, blockSize = 0, fftLen = 0;
    double sampleRate = 0.0;

    IrConvHostState host;

    // M3: host worker pool for the per-node input/output FFTs + segment loader.
    // Joined in release() before any HIP teardown; host memory only.
    GpuHostWorkPool pool;
};

HipIrBackend::HipIrBackend (int deviceIndex) : impl (std::make_unique<Impl>()) { impl->deviceIndex = deviceIndex; }
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
    const std::string archName = prop.gcnArchName;

    if (blockSize > prop.maxThreadsPerBlock)
    {
        lastError = "Block size exceeds CUDA thread-block limit";
        return false;
    }

    // 3) hipRTC-compile the kernel string to a code object for this GPU's arch.
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

    // End-of-block sync event: BlockingSync makes hipEventSynchronize yield the
    // pump thread on an OS primitive instead of the spin-wait of
    // hipStreamSynchronize; DisableTiming skips timestamp bookkeeping.
    CK_RT (hipEventCreateWithFlags (&m.syncEvent, hipEventBlockingSync | hipEventDisableTiming));

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

    // M3 host worker pool: auto = clamp(physicalCores/8, 1, 2) lanes for the IR
    // pump; WFS_GPU_HOST_WORKERS overrides (0 = sequential kill switch).
    {
        const int autoWorkers = std::clamp (spatcore::rt::physicalCoreCount() / 8, 1, 2);
        const int workers = hostWorkerCountFromEnv (autoWorkers);
        const double periodMs = (sampleRate > 0.0) ? (1000.0 * m.blockSize / sampleRate) : 0.0;
        m.pool.prepare (workers, periodMs, periodMs);
    }

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

    // processBlock runs on the pump thread; bind the selected device here.
    if (hipSetDevice (m.deviceIndex) != hipSuccess)
    {
        lastError = "HIP: hipSetDevice failed on pump thread";
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
        // M3: budgeted IR-segment FFTs run across the pool (per-segment scratch).
        const int written = m.host.loadMoreSegments (m.hIrSpectra, kIrSegmentsPerLaunch, m.pool);
        if (written > 0)
            PB_RT (hipMemcpyAsync ((char*) m.dIrSpectra + (size_t) before * specBytes,
                                    m.hIrSpectra + (size_t) before * (size_t) m.fftLen,
                                    (size_t) written * specBytes,
                                    hipMemcpyHostToDevice, m.stream));
    }

    // Newest input spectra -> pinned staging -> one strided 2D copy into each
    // node's ring slot. M3: the per-node forward FFTs run across the pool
    // (per-node scratch; node-safe), bit-identical to the sequential loop.
    m.host.advanceRing();
    const size_t head = (size_t) m.host.getRingHead();
    m.host.transformInputs (inputs, m.hInSpectra, m.pool);
    PB_RT (hipMemcpy2DAsync ((char*) m.dInSpectra + head * specBytes,
                              segCap * specBytes,
                              m.hInSpectra, specBytes,
                              specBytes, (size_t) m.numNodes,
                              hipMemcpyHostToDevice, m.stream));

    // No IR yet: wet output is silence, but the ring copy above still has to
    // land before the pinned staging is reused next launch.
    if (m.host.getSegmentsLoaded() == 0)
    {
        PB_RT (hipEventRecord (m.syncEvent, m.stream));
        PB_RT (hipEventSynchronize (m.syncEvent));   // blocking-sync event: yields, no spin
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
    PB_RT (hipEventRecord (m.syncEvent, m.stream));
    PB_RT (hipEventSynchronize (m.syncEvent));   // blocking-sync event: yields, no spin

#undef PB_RT

    // Accumulated spectra -> inverse FFT + overlap-add per node. M3: runs across
    // the pool (per-node scratch + per-node tail), bit-identical to the loop.
    m.host.produceOutputs (outputs, m.hOutSpectra, m.pool);

    lastLaunchMs = std::chrono::duration<double, std::milli> (
                       std::chrono::steady_clock::now() - t0).count();
    return true;
}

void HipIrBackend::release() noexcept
{
    auto& m = *impl;

    // M3: join the host worker pool BEFORE any HIP teardown (host memory only).
    m.pool.shutdown();

    hipSetDevice (m.deviceIndex);

    auto freeHost = [] (float*& p) { if (p != nullptr) { hipHostFree (p); p = nullptr; } };
    auto freeDev  = [] (void*&  p) { if (p != nullptr) { hipFree (p);     p = nullptr; } };

    freeHost (m.hIrSpectra); freeHost (m.hInSpectra); freeHost (m.hOutSpectra);
    freeDev (m.dIrSpectra); freeDev (m.dInSpectra); freeDev (m.dOutSpectra);

    if (m.syncEvent != nullptr) { hipEventDestroy (m.syncEvent); m.syncEvent = nullptr; }
    if (m.stream != nullptr) { hipStreamDestroy (m.stream); m.stream = nullptr; }
    if (m.module != nullptr) { hipModuleUnload (m.module); m.module = nullptr; }
    m.kernelMac = nullptr;


    ready = false;
}

#undef CK_RT
#undef CK_DRV

} // namespace spatcore::gpu

#endif // WFS_GPU_NATIVE && !defined(__APPLE__) && defined(WFS_GPU_HIP)
