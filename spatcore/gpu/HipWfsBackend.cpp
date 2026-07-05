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
#include "GpuHostWorkPool.h"
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
    hipEvent_t  syncEvent = nullptr;  // blocking-sync end-of-block wait (no spin)

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

    // M3: host worker pool for the fused per-input prep. Joined in release()
    // before any HIP teardown; workers touch only host memory (plan section 3).
    GpuHostWorkPool pool;

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

    // End-of-block sync event: BlockingSync makes hipEventSynchronize yield the
    // pump thread on an OS primitive instead of the spin-wait of
    // hipStreamSynchronize; DisableTiming skips timestamp bookkeeping.
    CK_RT (hipEventCreateWithFlags (&m.syncEvent, hipEventBlockingSync | hipEventDisableTiming));

    auto pin = [] (float** p, size_t n) {
        return hipHostMalloc ((void**) p, n * sizeof (float), hipHostMallocDefault);
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

    CK_RT (hipMalloc (&m.dIn,   (size_t) m.numIn  * m.blockSize * sizeof (float)));
    CK_RT (hipMalloc (&m.dFrIn, (size_t) m.numIn  * m.blockSize * sizeof (float)));
    CK_RT (hipMalloc (&m.dOut,  (size_t) m.numOut * m.blockSize * sizeof (float)));
    CK_RT (hipMalloc (&m.dRing,   (size_t) m.numIn * m.ringCapacity * sizeof (float)));
    CK_RT (hipMalloc (&m.dFrRing, (size_t) m.numIn * m.ringCapacity * sizeof (float)));
    CK_RT (hipMalloc (&m.dScratch, (size_t) m.numIn * m.numOut * m.blockSize * sizeof (float)));
    CK_RT (hipMalloc (&m.dShelfState,   (size_t) matrix * 4 * sizeof (float)));
    CK_RT (hipMalloc (&m.dFrShelfState, (size_t) matrix * 4 * sizeof (float)));
    for (auto* pp : { &m.ppDelays, &m.ppGains, &m.ppFrDelays, &m.ppFrGains })
    {
        CK_RT (hipMalloc (&pp->slot[0], matrix * sizeof (float)));
        CK_RT (hipMalloc (&pp->slot[1], matrix * sizeof (float)));
        pp->curr = 0;
        pp->everUploaded = false;
    }
    CK_RT (hipMalloc (&m.dHfAttenDb,    matrix * sizeof (float)));
    CK_RT (hipMalloc (&m.dFrHfAttenDb,  matrix * sizeof (float)));
    m.hfUploaded = false;
    m.frHfUploaded = false;

    CK_RT (hipMemset (m.dRing,   0, (size_t) m.numIn * m.ringCapacity * sizeof (float)));
    CK_RT (hipMemset (m.dFrRing, 0, (size_t) m.numIn * m.ringCapacity * sizeof (float)));
    CK_RT (hipMemset (m.dShelfState,   0, (size_t) matrix * 4 * sizeof (float)));
    CK_RT (hipMemset (m.dFrShelfState, 0, (size_t) matrix * 4 * sizeof (float)));

    m.lastDelays.assign (matrix, 0.0f);
    m.lastGains.assign (matrix, 0.0f);
    m.lastFrDelays.assign (matrix, 0.0f);
    m.lastFrGains.assign (matrix, 0.0f);
    m.lastHfAttenDb.assign (matrix, 0.0f);
    m.lastFrHfAttenDb.assign (matrix, 0.0f);

    m.frHost.prepare (m.numIn, m.numOut, sampleRate);

    // M3 host worker pool: auto = clamp(physicalCores/8, 1, 3) lanes for
    // Wfs/Ob; WFS_GPU_HOST_WORKERS overrides (0 = sequential kill switch).
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

    // M3: ONE fused parallelFor(numIn) does ALL per-input host prep — input
    // pack, per-input FR pre-filter, per-input jitter advance, and the direct +
    // shelf + FR-curr matrix snapshot rows for that input. Item-indexed state
    // only, no cross-item host accumulation => bit-identical for any worker
    // count (section-4 determinism table). launchCounter is HOISTED
    // (currentLaunchIndex() read here, commitJitterLaunch() once after the join).
    // The M2 memcmp change-detect + upload stay on the pump thread after the join.
    const uint32_t launchIdx = m.frHost.currentLaunchIndex();
    m.pool.parallelFor (m.numIn, [&] (int in)
    {
        const int nOut = m.numOut;
        const size_t rowBase = (size_t) in * (size_t) nOut;

        for (int out = 0; out < nOut; ++out)
        {
            const size_t i = rowBase + (size_t) out;
            const float d = m.delaysMs != nullptr ? (m.delaysMs[i] - m.latencyMs) * srScale : 0.0f;
            m.hDelaysCurr[i]  = std::clamp (d, 0.0f, maxDelay);
            m.hGainsCurr[i]   = m.gains      != nullptr ? m.gains[i]      : 0.0f;
            m.hHfAttenDb[i]   = m.hfAttenDb   != nullptr ? m.hfAttenDb[i]   : 0.0f;
            m.hFrHfAttenDb[i] = m.frHfAttenDb != nullptr ? m.frHfAttenDb[i] : 0.0f;
        }

        m.frHost.advanceJitterForInput (in, launchIdx, m.blockSize);
        m.frHost.computeFrCurrForInput (in, m.delaysMs, m.frDelaysMs, m.frLevels,
                                        m.latencyMs, srScale, maxDelay,
                                        m.hFrDelaysCurr, m.hFrGainsCurr);

        if (inputs[in] != nullptr)
            std::memcpy (m.hIn + (size_t) in * m.blockSize, inputs[in], (size_t) m.blockSize * sizeof (float));
        else
            std::memset (m.hIn + (size_t) in * m.blockSize, 0, (size_t) m.blockSize * sizeof (float));
        m.frHost.filterBlockForInput (in, inputs, m.hFrIn, m.blockSize);
    });
    m.frHost.commitJitterLaunch();   // hoisted ++launchCounter, once after the join

    // Host -> device (the persistent rings + shelf states stay on the device).
#define PB_RT(call) do { hipError_t _e = (call); if (_e != hipSuccess) { \
    lastError = std::string ("HIP runtime: ") + hipGetErrorString (_e); ready = false; return false; } } while (0)

    auto up = [&m] (void* dst, const float* src, size_t floats) {
        return hipMemcpyAsync (dst, src, floats * sizeof (float), hipMemcpyHostToDevice, m.stream);
    };
    PB_RT (up (m.dIn,   m.hIn,   (size_t) m.numIn * m.blockSize));
    PB_RT (up (m.dFrIn, m.hFrIn, (size_t) m.numIn * m.blockSize));

    // Upload diet: memcmp each freshly staged matrix against its lastStaged
    // baseline. Unchanged => skip the upload AND the slot swap (prev == curr ==
    // live slot; the kernel ramps x->x = x). Changed => upload into the
    // alternate slot, prev = the previous slot (last launch's curr, already
    // on-device), swap. First launch: single upload, prev == curr (the old
    // havePrev bootstrap, bit-exact). Safe against in-flight reads: the
    // previous block ended with hipEventSynchronize, so no launch is reading
    // either slot while we upload.
    auto stagePair = [&m, matrix] (Impl::PingPong& pp, const float* staged,
                                   std::vector<float>& last,
                                   void*& prevArg, void*& currArg) -> hipError_t
    {
        const size_t bytes = (size_t) matrix * sizeof (float);
        if (pp.everUploaded && std::memcmp (staged, last.data(), bytes) == 0)
        {
            prevArg = currArg = pp.slot[pp.curr];      // unchanged: no upload, no swap
            return hipSuccess;
        }
        const int next = pp.everUploaded ? (pp.curr ^ 1) : pp.curr;
        const hipError_t e = hipMemcpyAsync (pp.slot[next], staged, bytes,
                                             hipMemcpyHostToDevice, m.stream);
        if (e != hipSuccess)
            return e;
        std::memcpy (last.data(), staged, bytes);
        prevArg = pp.slot[pp.curr];                    // first launch: prev == curr
        currArg = pp.slot[next];
        pp.curr = next;
        pp.everUploaded = true;
        return e;
    };
    auto stageSingle = [&m, matrix] (void* dBuf, const float* staged,
                                     std::vector<float>& last, bool& uploaded) -> hipError_t
    {
        const size_t bytes = (size_t) matrix * sizeof (float);
        if (uploaded && std::memcmp (staged, last.data(), bytes) == 0)
            return hipSuccess;
        const hipError_t e = hipMemcpyAsync (dBuf, staged, bytes,
                                             hipMemcpyHostToDevice, m.stream);
        if (e != hipSuccess)
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
    PB_RT (hipEventRecord (m.syncEvent, m.stream));
    PB_RT (hipEventSynchronize (m.syncEvent));   // blocking-sync event: yields, no spin

#undef PB_RT

    // Output pinned buffer -> channels.
    for (int ch = 0; ch < m.numOut; ++ch)
        if (outputs[ch] != nullptr)
            std::memcpy (outputs[ch], m.hOut + (size_t) ch * m.blockSize, (size_t) m.blockSize * sizeof (float));

    // Advance host-tracked state (matrix prev continuity now lives on the
    // device: the ping-pong slots + lastStaged baselines, updated at upload).
    m.ringWritePos = (m.ringWritePos + (uint32_t) m.blockSize) % m.ringCapacity;
    m.ringValid = std::min (m.maxDelaySamples, m.ringValid + (uint32_t) m.blockSize);

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

void HipWfsBackend::release() noexcept
{
    auto& m = *impl;

    // M3: join the host worker pool BEFORE any HIP teardown (workers touch only
    // host memory).
    m.pool.shutdown();

    hipSetDevice (m.deviceIndex);

    auto freeHost = [] (float*& p) { if (p != nullptr) { hipHostFree (p); p = nullptr; } };
    auto freeDev  = [] (void*&  p) { if (p != nullptr) { hipFree (p);     p = nullptr; } };

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

    if (m.syncEvent != nullptr) { hipEventDestroy (m.syncEvent); m.syncEvent = nullptr; }
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
    ready = false;
}

#undef CK_RT
#undef CK_DRV

} // namespace spatcore::gpu

#endif // WFS_GPU_NATIVE && !defined(__APPLE__) && defined(WFS_GPU_HIP)
