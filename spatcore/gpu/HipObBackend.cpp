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
#include "GpuHostWorkPool.h"
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
#include <utility>
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
    hipEvent_t  syncEvent = nullptr;  // blocking-sync end-of-block wait (no spin)

    // Pinned host staging (curr only — prev never leaves the device, see below).
    float* hIn = nullptr;
    float* hFrIn = nullptr;
    float* hOut = nullptr;
    float* hDelaysCurr = nullptr;
    float* hGainsCurr = nullptr;
    float* hFrDelaySamples = nullptr;     // [pairs][blockSize] per-sample FR delay (jitter sub-stepped)
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
    void* dFrDelaySamples = nullptr;  // [pairs][blockSize] per-sample FR delay
    void* dHfAttenDb = nullptr;       // single buffer (stepwise, no prev/curr ramp)
    void* dFrHfAttenDb = nullptr;

    // Upload diet (M2): device ping-pong per prev/curr matrix pair. The kernel
    // takes the matrix pointers as launch ARGUMENTS and only READS them
    // (const __restrict__, CudaObKernels.h), so "prev" never needs a host
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
    PingPong ppDelays, ppGains, ppFrGains;
    bool hfUploaded = false;          // single-buffer change-detect state
    bool frHfUploaded = false;

    // Change-detect baselines: the last STAGED (== last uploaded) copy of each
    // matrix, memcmp'd against the freshly staged pinned buffer. Comparing
    // staged copies (never the live app matrices) is torn-read-safe. lastFrGains
    // doubles as the FR-activity gate input for computeFrDelaysPerSample and the
    // active-row upload scan (it IS last launch's staged FR gains — the same
    // values the kernel's doFr gate reads as frGainsPrev).
    std::vector<float> lastDelays, lastGains, lastFrGains;
    std::vector<float> lastHfAttenDb, lastFrHfAttenDb;

    // Scatter FR tiers: coalesced [firstPair, count) runs of FR-active pairs
    // (prev|curr gain != 0). Rows are pair-indexed in*numOut+out, so activity
    // yields contiguous runs; one hipMemcpyAsync per run uploads exactly the
    // rows the kernel's doFr gate will read (all other rows are neither filled
    // by WfsFrHostState nor read — F6). Empty => tier-1 global skip: the whole
    // [pairs][blockSize] upload disappears.
    std::vector<std::pair<uint32_t, uint32_t>> frActiveRuns;

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

    std::vector<float> frBasePrev;        // [pairs] base FR delay ramp continuity (no jitter)

    WfsFrHostState frHost;            // per-input FR pre-filters + jitter (shared)

    // M3: host worker pool for the fused per-input prep. Joined in release()
    // before any HIP teardown; workers touch only host memory (plan section 3).
    GpuHostWorkPool pool;

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
    m.threadsPerBlock = 256;

    const double w0 = 2.0 * 3.14159265358979 * 800.0 / sampleRate;
    m.shelfCosW0 = (float) std::cos (w0);
    m.shelfSinW0 = (float) std::sin (w0);

    const uint32_t matrix = (uint32_t) (m.numIn * m.numOut);

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
    CK_RT (pin (&m.hFrDelaySamples, (size_t) matrix * m.blockSize));
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
    for (auto* pp : { &m.ppDelays, &m.ppGains, &m.ppFrGains })
    {
        CK_RT (hipMalloc (&pp->slot[0], matrix * sizeof (float)));
        CK_RT (hipMalloc (&pp->slot[1], matrix * sizeof (float)));
        pp->curr = 0;
        pp->everUploaded = false;
    }
    CK_RT (hipMalloc (&m.dFrDelaySamples, (size_t) matrix * m.blockSize * sizeof (float)));
    CK_RT (hipMalloc (&m.dHfAttenDb,    matrix * sizeof (float)));
    CK_RT (hipMalloc (&m.dFrHfAttenDb,  matrix * sizeof (float)));
    m.hfUploaded = false;
    m.frHfUploaded = false;

    CK_RT (hipMemset (m.dPairAcc, 0, (size_t) matrix * m.accLen * sizeof (float)));
    CK_RT (hipMemset (m.dShelfState,   0, (size_t) matrix * 4 * sizeof (float)));
    CK_RT (hipMemset (m.dFrShelfState, 0, (size_t) matrix * 4 * sizeof (float)));
    // Hygiene: define the FR delay rows once — with the tiered upload, rows for
    // never-active pairs would otherwise stay unwritten VRAM forever (the
    // kernel never reads them, but a defined buffer is one less trap).
    CK_RT (hipMemset (m.dFrDelaySamples, 0, (size_t) matrix * m.blockSize * sizeof (float)));

    m.lastDelays.assign (matrix, 0.0f);
    m.lastGains.assign (matrix, 0.0f);
    m.lastFrGains.assign (matrix, 0.0f);   // zeros: first-launch FR gate input (CPU parity)
    m.lastHfAttenDb.assign (matrix, 0.0f);
    m.lastFrHfAttenDb.assign (matrix, 0.0f);
    m.frBasePrev.assign (matrix, 1.0f);
    m.frActiveRuns.clear();
    m.frActiveRuns.reserve (64);

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

    // M3: pump-thread setup for the fused per-input prep (basePrev sizing +
    // base-delay bootstrap + sub-step base), BEFORE the parallelFor.
    const int pairs = m.numIn * m.numOut;
    if ((int) m.frBasePrev.size() != pairs)
        m.frBasePrev.assign ((size_t) pairs, 1.0f);
    m.frHost.beginFrDelaysPerSample (pairs);
    const bool firstFrBlock  = m.frHost.consumeFirstFrDelayBlock();
    const uint32_t subStepBase = m.frHost.currentSubStep();
    const int subLen = std::max (1, kObSubBlock);
    const int numSubBlocks = (m.blockSize + subLen - 1) / subLen;

    // ONE fused parallelFor(numIn): direct + shelf + FR-gain snapshot rows, the
    // PER-SAMPLE FR-delay lane (global step index reconstructed as
    // subStepBase + k + 1), input pack, and FR pre-filter — all for one input.
    // Item-indexed state only => bit-identical for any worker count (section-4
    // OB row). subStepCounter committed once after the join; the active-run scan
    // + change-detect + uploads stay on the pump thread.
    m.pool.parallelFor (m.numIn, [&] (int in)
    {
        const int nOut = m.numOut;
        const size_t rowBase = (size_t) in * (size_t) nOut;

        for (int out = 0; out < nOut; ++out)
        {
            const size_t i = rowBase + (size_t) out;
            const float d = m.delaysMs != nullptr ? (m.delaysMs[i] - m.latencyMs) * srScale : 0.0f;
            m.hDelaysCurr[i]   = std::clamp (d, 1.0f, maxDelay);   // scatter d >= 1
            m.hGainsCurr[i]    = m.gains      != nullptr ? m.gains[i]      : 0.0f;
            m.hHfAttenDb[i]    = m.hfAttenDb   != nullptr ? m.hfAttenDb[i]   : 0.0f;
            m.hFrHfAttenDb[i]  = m.frHfAttenDb != nullptr ? m.frHfAttenDb[i] : 0.0f;
            m.hFrGainsCurr[i]  = m.frLevels    != nullptr ? m.frLevels[i]    : 0.0f;
        }

        m.frHost.computeFrDelaysPerSampleForInput (in, firstFrBlock, subStepBase,
                                                   m.delaysMs, m.frDelaysMs,
                                                   m.lastFrGains.data(), m.hFrGainsCurr,
                                                   m.latencyMs, srScale, maxDelay,
                                                   m.blockSize, kObSubBlock, m.frBasePrev,
                                                   m.hFrDelaySamples);

        if (inputs[in] != nullptr)
            std::memcpy (m.hIn + (size_t) in * m.blockSize, inputs[in], (size_t) m.blockSize * sizeof (float));
        else
            std::memset (m.hIn + (size_t) in * m.blockSize, 0, (size_t) m.blockSize * sizeof (float));
        m.frHost.filterBlockForInput (in, inputs, m.hFrIn, m.blockSize);
    });
    m.frHost.commitSubSteps (numSubBlocks);   // hoisted subStepCounter += numSubBlocks

    // Scatter FR tiers: scan pair activity (prev|curr FR gain != 0) and coalesce
    // consecutive active pairs into upload runs. Runs on the pump thread AFTER
    // the join (needs the full staged hFrGainsCurr). lastFrGains still holds
    // LAST launch's staged gains here (refresh happens at upload time below) —
    // numerically the same frGainsPrev the kernel's doFr gate reads and the same
    // predicate the per-sample fill used, so filled == uploaded == kernel-read
    // rows, exactly. Toggle parity: a pair turning off stays active for one
    // ramp-out block (prev != 0), then drops out.
    m.frActiveRuns.clear();
    {
        uint32_t runStart = 0;
        bool inRun = false;
        for (uint32_t i = 0; i < matrix; ++i)
        {
            const bool active = m.lastFrGains[i] != 0.0f || m.hFrGainsCurr[i] != 0.0f;
            if (active && ! inRun)
            {
                runStart = i;
                inRun = true;
            }
            else if (! active && inRun)
            {
                m.frActiveRuns.push_back ({ runStart, i - runStart });
                inRun = false;
            }
        }
        if (inRun)
            m.frActiveRuns.push_back ({ runStart, matrix - runStart });
    }

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

    void* dDelaysPrevArg = nullptr;  void* dDelaysCurrArg = nullptr;
    void* dGainsPrevArg = nullptr;   void* dGainsCurrArg = nullptr;
    void* dFrGainsPrevArg = nullptr; void* dFrGainsCurrArg = nullptr;
    PB_RT (stagePair (m.ppDelays,  m.hDelaysCurr,  m.lastDelays,  dDelaysPrevArg,  dDelaysCurrArg));
    PB_RT (stagePair (m.ppGains,   m.hGainsCurr,   m.lastGains,   dGainsPrevArg,   dGainsCurrArg));
    PB_RT (stagePair (m.ppFrGains, m.hFrGainsCurr, m.lastFrGains, dFrGainsPrevArg, dFrGainsCurrArg));
    PB_RT (stageSingle (m.dHfAttenDb,   m.hHfAttenDb,   m.lastHfAttenDb,   m.hfUploaded));
    PB_RT (stageSingle (m.dFrHfAttenDb, m.hFrHfAttenDb, m.lastFrHfAttenDb, m.frHfUploaded));

    // Per-sample FR delays: upload only the coalesced active-pair runs (tier 2);
    // no runs (tier 1, FR fully idle) => the 4 MiB-class upload disappears.
    // Every row the kernel will read this block was freshly filled this block.
    for (const auto& r : m.frActiveRuns)
        PB_RT (up ((float*) m.dFrDelaySamples + (size_t) r.first * m.blockSize,
                   m.hFrDelaySamples + (size_t) r.first * m.blockSize,
                   (size_t) r.second * m.blockSize));

    ObParamsGpu p { (uint32_t) m.numIn, (uint32_t) m.numOut, (uint32_t) m.blockSize,
                    m.accLen, m.writePos, m.shelfCosW0, m.shelfSinW0 };

    // K1: per-pair filter + emit + scatter (one thread per pair).
    void* pairsArgs[] = { &p, &m.dIn, &m.dFrIn, &m.dHfAttenDb, &m.dFrHfAttenDb,
                          &dDelaysPrevArg, &dDelaysCurrArg, &dGainsPrevArg, &dGainsCurrArg,
                          &m.dFrDelaySamples, &dFrGainsPrevArg, &dFrGainsCurrArg,
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
    PB_RT (hipEventRecord (m.syncEvent, m.stream));
    PB_RT (hipEventSynchronize (m.syncEvent));   // blocking-sync event: yields, no spin

#undef PB_RT

    for (int ch = 0; ch < m.numOut; ++ch)
        if (outputs[ch] != nullptr)
            std::memcpy (outputs[ch], m.hOut + (size_t) ch * m.blockSize, (size_t) m.blockSize * sizeof (float));

    // Advance host-tracked state (FR delay continuity lives in frBasePrev,
    // updated inside computeFrDelaysPerSample; matrix prev continuity now lives
    // on the device: the ping-pong slots + lastStaged baselines).
    m.writePos = (m.writePos + (uint32_t) m.blockSize) % m.accLen;

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

    // Upload-diet state back to first-launch semantics: the next block
    // force-uploads every matrix and passes prev == curr (old havePrev
    // bootstrap). lastFrGains back to zeros = the first-launch FR gate input.
    for (auto* pp : { &m.ppDelays, &m.ppGains, &m.ppFrGains })
        pp->everUploaded = false;
    m.hfUploaded = false;
    m.frHfUploaded = false;
    for (auto* v : { &m.lastDelays, &m.lastGains, &m.lastFrGains,
                     &m.lastHfAttenDb, &m.lastFrHfAttenDb })
        std::fill (v->begin(), v->end(), 0.0f);
}

void HipObBackend::release() noexcept
{
    auto& m = *impl;

    // M3: join the host worker pool BEFORE any HIP teardown (workers touch only
    // host memory).
    m.pool.shutdown();

    hipSetDevice (m.deviceIndex);

    auto freeHost = [] (float*& p) { if (p != nullptr) { hipHostFree (p); p = nullptr; } };
    auto freeDev  = [] (void*&  p) { if (p != nullptr) { hipFree (p);     p = nullptr; } };

    freeHost (m.hIn);  freeHost (m.hFrIn);  freeHost (m.hOut);
    freeHost (m.hDelaysCurr); freeHost (m.hGainsCurr);
    freeHost (m.hFrDelaySamples);
    freeHost (m.hFrGainsCurr);
    freeHost (m.hHfAttenDb);    freeHost (m.hFrHfAttenDb);

    freeDev (m.dIn);  freeDev (m.dFrIn);  freeDev (m.dOut);
    freeDev (m.dPairAcc);  freeDev (m.dPairOut);
    freeDev (m.dShelfState); freeDev (m.dFrShelfState);
    for (auto* pp : { &m.ppDelays, &m.ppGains, &m.ppFrGains })
    {
        freeDev (pp->slot[0]);
        freeDev (pp->slot[1]);
        pp->curr = 0;
        pp->everUploaded = false;
    }
    freeDev (m.dFrDelaySamples);
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
