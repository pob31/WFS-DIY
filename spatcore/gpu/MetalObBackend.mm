/*
    MetalObBackend implementation (Objective-C++).

    The MSL kernel sources live in MetalObKernels.h as a string literal so the
    app needs no resource plumbing; they are compiled at prepare() time and
    cached in two pipeline state objects (ob_pairs + ob_reduce).

    v2 architecture: one thread per (in,out) pair scattering into a PRIVATE
    persistent per-pair accumulator, then a deterministic per-output reduce -
    the proven wfs_pairs/wfs_reduce occupancy shape (v1's single writer per
    output was ~32x under-parallel and missed the buffer-64 deadline in the
    field). Faithful to the CPU shared-buffer semantics by linearity. Delay
    contract: all scatter delays >= 1 sample (host-clamped, kernel re-clamped) -
    a write-time scatter cannot represent d < 1 (see MetalObKernels.h).

    Host-side processBlock mirrors CudaObBackend.cpp (the validated reference -
    Experiments/cuda-output-buffer-test checks the CUDA twin of the kernel string
    against a CPU scatter model on real hardware): matrix snapshot with -L
    compensation (min 1 sample), prev->curr ramp continuity, persistent device
    accumulators + shelf states, host-tracked accumulator-head advance.
    Floor-Reflection host work (per-input pre-filter, per-sample sub-stepped
    diffusion delay staging gated on FR-active pairs) lives in the SHARED
    WfsFrHostState.

    Device memory: the per-pair accumulators dominate at
    numIn*numOut*accLen*4 B (32x27 @ 1 s/48 kHz ~ 166 MB; 64x64 ~ 787 MB).
    Fine on Apple unified memory; revisit a delay cap only if huge configs need
    this algorithm on small-VRAM cards.

    Dispatch ordering: one command buffer, ONE serial compute encoder, two
    dispatches (ob_pairs then ob_reduce). A serial-dispatch-type encoder orders
    consecutive dispatches with full memory visibility, so no barrier is needed.
*/

#include "MetalObBackend.h"
#include "MetalObKernels.h"
#include "WfsFrHostState.h"
#include "GpuHostWorkPool.h"

#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
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

struct MetalObBackend::Impl
{
    id<MTLDevice> device = nil;
    id<MTLCommandQueue> queue = nil;
    id<MTLComputePipelineState> psoPairs = nil;
    id<MTLComputePipelineState> psoReduce = nil;

    id<MTLBuffer> bParams = nil;
    id<MTLBuffer> bIn = nil;
    id<MTLBuffer> bFrIn = nil;
    id<MTLBuffer> bOut = nil;
    id<MTLBuffer> bPairAcc = nil;         // [pairs][accLen] persistent (thread-ordered)
    id<MTLBuffer> bPairOut = nil;         // [(s*numOut+out)*numIn+in] transient
    id<MTLBuffer> bShelfState = nil;      // [pairs][4] persistent
    id<MTLBuffer> bFrShelfState = nil;    // [pairs][4] persistent
    // Host staging (curr snapshot only — prev never leaves the device). The GPU
    // reads the ping-pong slots / the *Dev buffers, never these staging buffers.
    id<MTLBuffer> bDelaysCurr = nil;
    id<MTLBuffer> bGainsCurr = nil;
    id<MTLBuffer> bFrDelaySamples = nil;     // [pairs][blockSize] per-sample FR delay staging (jitter sub-stepped)
    id<MTLBuffer> bFrDelaySamplesDev = nil;  // [pairs][blockSize] device buffer the kernel reads (tiered upload)
    id<MTLBuffer> bFrGainsCurr = nil;
    id<MTLBuffer> bHfAttenDb = nil, bFrHfAttenDb = nil;         // single-buffer staging
    id<MTLBuffer> bHfAttenDbDev = nil, bFrHfAttenDbDev = nil;   // single device buffers (GPU-read)

    // Upload diet (M2): device ping-pong per prev/curr matrix pair, mirrored
    // from CudaObBackend.cpp (d9893cc). The kernel takes the matrix pointers as
    // bound BUFFERS and only READS them (const, MetalObKernels.h), so "prev"
    // never needs a host round-trip: it is simply the slot uploaded one launch
    // earlier. Only a CHANGED curr is uploaded (memcpy'd into the alternate
    // slot, then swap); unchanged blocks bind prev == curr == the live slot —
    // the kernel ramps x->x = x, bit-exact. First launch: upload once, bind
    // prev == curr (the old havePrev bootstrap). The pump is fully synchronous
    // ([cb waitUntilCompleted] before the next fill), so memcpy'ing a slot the
    // previous launch read is hazard-free.
    struct PingPong
    {
        id<MTLBuffer> slot[2] = { nil, nil };
        int  curr = 0;               // slot holding the last-consumed curr
        bool everUploaded = false;   // first launch: upload once, prev == curr
    };
    PingPong ppDelays, ppGains, ppFrGains;
    bool hfUploaded = false;         // single-buffer change-detect state
    bool frHfUploaded = false;

    // Change-detect baselines: the last STAGED (== last uploaded) copy of each
    // matrix, memcmp'd against the freshly staged buffer. lastFrGains doubles as
    // the FR-activity gate input for computeFrDelaysPerSample and the active-row
    // upload scan (it IS last launch's staged FR gains — the same values the
    // kernel's doFr gate reads as frGainsPrev).
    std::vector<float> lastDelays, lastGains, lastFrGains;
    std::vector<float> lastHfAttenDb, lastFrHfAttenDb;

    // Scatter FR tiers: coalesced [firstPair, count) runs of FR-active pairs
    // (prev|curr gain != 0). Rows are pair-indexed in*numOut+out, so activity
    // yields contiguous runs; one memcpy per run copies exactly the rows the
    // kernel's doFr gate will read (all other rows are neither filled by
    // WfsFrHostState nor read). Empty => the whole [pairs][blockSize] upload
    // disappears.
    std::vector<std::pair<uint32_t, uint32_t>> frActiveRuns;

    int numIn = 0, numOut = 0, blockSize = 0;
    uint32_t accLen = 0;          // per-pair accumulator length (samples)
    uint32_t writePos = 0;        // accumulator head
    float maxDelayClamp = 0.0f;   // accLen - 1 (CPU: delayBufferLength - 1)
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

    std::vector<float> frBasePrev;        // [pairs] base FR delay ramp continuity (no jitter)

    WfsFrHostState frHost;            // per-input FR pre-filters + jitter (shared)

    // M3: host worker pool for the fused per-input prep. Joined in release()
    // before any Metal teardown; workers touch only host memory (plan section 3).
    GpuHostWorkPool pool;

    NSUInteger threadsPerGroup = 256;
};

// deviceIndex accepted for API uniformity but ignored on macOS (system default device).
MetalObBackend::MetalObBackend (int deviceIndex) : impl (std::make_unique<Impl>()) { (void) deviceIndex; }
MetalObBackend::~MetalObBackend() { release(); }

bool MetalObBackend::prepare (int numInputs, int numOutputs, int blockSize,
                              double sampleRate, double pipelineLatencyMs,
                              double maxDelaySeconds)
{
    release();
    auto& m = *impl;

    @autoreleasepool
    {
        m.device = MTLCreateSystemDefaultDevice();
        if (m.device == nil)
        {
            lastError = "No Metal device available";
            return false;
        }

        NSError* err = nil;
        id<MTLLibrary> lib = [m.device newLibraryWithSource:
                                  [NSString stringWithUTF8String: kObScatterKernelSource]
                                                    options: nil
                                                      error: &err];
        if (lib == nil)
        {
            lastError = std::string ("Kernel compile failed: ")
                        + (err ? err.localizedDescription.UTF8String : "?");
            return false;
        }

        m.psoPairs = [m.device newComputePipelineStateWithFunction:
                          [lib newFunctionWithName: @"ob_pairs"]
                                                             error: &err];
        if (m.psoPairs == nil)
        {
            lastError = std::string ("Pipeline state failed (pairs): ")
                        + (err ? err.localizedDescription.UTF8String : "?");
            return false;
        }
        m.psoReduce = [m.device newComputePipelineStateWithFunction:
                           [lib newFunctionWithName: @"ob_reduce"]
                                                              error: &err];
        if (m.psoReduce == nil)
        {
            lastError = std::string ("Pipeline state failed (reduce): ")
                        + (err ? err.localizedDescription.UTF8String : "?");
            return false;
        }

        m.queue = [m.device newCommandQueue];

        m.numIn = std::max (1, numInputs);
        m.numOut = std::max (1, numOutputs);
        m.blockSize = std::max (1, blockSize);
        m.sampleRate = sampleRate;
        m.latencyMs = (float) pipelineLatencyMs;
        m.accLen = (uint32_t) (maxDelaySeconds * sampleRate);
        if (m.accLen < (uint32_t) m.blockSize + 2)
            m.accLen = (uint32_t) m.blockSize + 2;           // sanity floor
        m.maxDelayClamp = (float) (m.accLen - 1);            // CPU: delayBufferLength - 1
        m.writePos = 0;

        m.threadsPerGroup = std::min<NSUInteger> (256,
                                std::min (m.psoPairs.maxTotalThreadsPerThreadgroup,
                                          m.psoReduce.maxTotalThreadsPerThreadgroup));

        // Fixed 800 Hz shelf frequency (WFSHighShelfFilter parity).
        const double w0 = 2.0 * 3.14159265358979 * 800.0 / sampleRate;
        m.shelfCosW0 = (float) std::cos (w0);
        m.shelfSinW0 = (float) std::sin (w0);

        const uint32_t matrix = (uint32_t) (m.numIn * m.numOut);
        auto shared = MTLResourceStorageModeShared;
        auto mkBuf = [&](size_t floats) {
            return [m.device newBufferWithLength: floats * sizeof (float) options: shared];
        };

        m.bParams = [m.device newBufferWithLength: sizeof (ObParamsGpu) options: shared];
        m.bIn = mkBuf ((size_t) m.numIn * m.blockSize);
        m.bFrIn = mkBuf ((size_t) m.numIn * m.blockSize);
        m.bOut = mkBuf ((size_t) m.numOut * m.blockSize);
        m.bPairAcc = mkBuf ((size_t) matrix * m.accLen);     // the big one
        m.bPairOut = mkBuf ((size_t) matrix * m.blockSize);
        m.bShelfState = mkBuf ((size_t) matrix * 4);
        m.bFrShelfState = mkBuf ((size_t) matrix * 4);
        m.bDelaysCurr = mkBuf (matrix);
        m.bGainsCurr = mkBuf (matrix);
        m.bFrGainsCurr = mkBuf (matrix);
        for (auto* pp : { &m.ppDelays, &m.ppGains, &m.ppFrGains })
        {
            pp->slot[0] = mkBuf (matrix);
            pp->slot[1] = mkBuf (matrix);
            pp->curr = 0;
            pp->everUploaded = false;
        }
        m.bFrDelaySamples = mkBuf ((size_t) matrix * m.blockSize);
        m.bFrDelaySamplesDev = mkBuf ((size_t) matrix * m.blockSize);
        m.bHfAttenDb = mkBuf (matrix);
        m.bFrHfAttenDb = mkBuf (matrix);
        m.bHfAttenDbDev = mkBuf (matrix);
        m.bFrHfAttenDbDev = mkBuf (matrix);
        m.hfUploaded = false;
        m.frHfUploaded = false;

        if (! (m.bParams && m.bIn && m.bFrIn && m.bOut && m.bPairAcc && m.bPairOut
               && m.bShelfState && m.bFrShelfState
               && m.bDelaysCurr && m.bGainsCurr && m.bFrGainsCurr
               && m.ppDelays.slot[0] && m.ppDelays.slot[1]
               && m.ppGains.slot[0] && m.ppGains.slot[1]
               && m.ppFrGains.slot[0] && m.ppFrGains.slot[1]
               && m.bFrDelaySamples && m.bFrDelaySamplesDev
               && m.bHfAttenDb && m.bFrHfAttenDb
               && m.bHfAttenDbDev && m.bFrHfAttenDbDev))
        {
            lastError = "Metal buffer allocation failed (per-pair accumulators need "
                        + std::to_string ((size_t) matrix * m.accLen * sizeof (float) / (1024 * 1024))
                        + " MB)";
            release();
            return false;
        }

        memset (m.bPairAcc.contents, 0, m.bPairAcc.length);
        memset (m.bShelfState.contents, 0, m.bShelfState.length);
        memset (m.bFrShelfState.contents, 0, m.bFrShelfState.length);
        // Hygiene: define the FR delay device rows once — with the tiered
        // upload, rows for never-active pairs would otherwise stay unwritten
        // (the kernel never reads them, but a defined buffer is one less trap).
        memset (m.bFrDelaySamplesDev.contents, 0, m.bFrDelaySamplesDev.length);

        m.lastDelays.assign (matrix, 0.0f);
        m.lastGains.assign (matrix, 0.0f);
        m.lastFrGains.assign (matrix, 0.0f);   // zeros: first-launch FR gate input (CPU parity)
        m.lastHfAttenDb.assign (matrix, 0.0f);
        m.lastFrHfAttenDb.assign (matrix, 0.0f);
        m.frBasePrev.assign (matrix, 1.0f);
        m.frActiveRuns.clear();
        m.frActiveRuns.reserve (64);

        m.frHost.prepare (m.numIn, m.numOut, sampleRate);
    }

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

void MetalObBackend::setMatrixPointers (const float* delaysMsPtr, const float* gainsPtr,
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

void MetalObBackend::setFRFilterParams (int inputIndex,
                                        bool lowCutActive, float lowCutFreq,
                                        bool highShelfActive, float highShelfFreq,
                                        float highShelfGain, float highShelfSlope) noexcept
{
    impl->frHost.setFRFilterParams (inputIndex, lowCutActive, lowCutFreq,
                                    highShelfActive, highShelfFreq,
                                    highShelfGain, highShelfSlope);
}

void MetalObBackend::setFRDiffusion (int inputIndex, float diffusionPercent) noexcept
{
    impl->frHost.setFRDiffusion (inputIndex, diffusionPercent);
}

bool MetalObBackend::processBlock (const float* const* inputs, float* const* outputs)
{
    if (! ready)
        return false;

    auto& m = *impl;
    const uint32_t matrix = (uint32_t) (m.numIn * m.numOut);
    const float srScale = (float) (m.sampleRate / 1000.0);
    const float maxDelay = m.maxDelayClamp;

    @autoreleasepool
    {
        const auto t0 = std::chrono::steady_clock::now();

        // Shared-storage staging pointers, captured before the parallelFor; the
        // run scan + staging below read them AFTER the join.
        float* dCurr   = (float*) m.bDelaysCurr.contents;
        float* gCurr   = (float*) m.bGainsCurr.contents;
        float* hfDb    = (float*) m.bHfAttenDb.contents;
        float* frHfDb  = (float*) m.bFrHfAttenDb.contents;
        float* fgCurr  = (float*) m.bFrGainsCurr.contents;
        float* inFlat  = (float*) m.bIn.contents;
        float* frInFlat = (float*) m.bFrIn.contents;
        float* frDelaySamples = (float*) m.bFrDelaySamples.contents;

        // M3: pump-thread setup for the fused per-input prep, BEFORE the
        // parallelFor (basePrev sizing + base-delay bootstrap + sub-step base).
        const int pairs = m.numIn * m.numOut;
        if ((int) m.frBasePrev.size() != pairs)
            m.frBasePrev.assign ((size_t) pairs, 1.0f);
        m.frHost.beginFrDelaysPerSample (pairs);
        const bool firstFrBlock  = m.frHost.consumeFirstFrDelayBlock();
        const uint32_t subStepBase = m.frHost.currentSubStep();
        const int subLen = std::max (1, kObSubBlock);
        const int numSubBlocks = (m.blockSize + subLen - 1) / subLen;

        // ONE fused parallelFor(numIn): direct + shelf + FR-gain snapshot rows,
        // the PER-SAMPLE FR-delay lane (global step index reconstructed as
        // subStepBase + k + 1), input pack, and FR pre-filter — all for one
        // input. Item-indexed state only => bit-identical for any worker count
        // (section-4 OB row). subStepCounter committed once after the join.
        m.pool.parallelFor (m.numIn, [&] (int in)
        {
            const int nOut = m.numOut;
            const size_t rowBase = (size_t) in * (size_t) nOut;

            for (int out = 0; out < nOut; ++out)
            {
                const size_t i = rowBase + (size_t) out;
                const float d = m.delaysMs != nullptr ? (m.delaysMs[i] - m.latencyMs) * srScale : 0.0f;
                dCurr[i]  = std::clamp (d, 1.0f, maxDelay);   // scatter d >= 1
                gCurr[i]  = m.gains      != nullptr ? m.gains[i]      : 0.0f;
                hfDb[i]   = m.hfAttenDb   != nullptr ? m.hfAttenDb[i]   : 0.0f;
                frHfDb[i] = m.frHfAttenDb != nullptr ? m.frHfAttenDb[i] : 0.0f;
                fgCurr[i] = m.frLevels    != nullptr ? m.frLevels[i]    : 0.0f;
            }

            m.frHost.computeFrDelaysPerSampleForInput (in, firstFrBlock, subStepBase,
                                                       m.delaysMs, m.frDelaysMs,
                                                       m.lastFrGains.data(), fgCurr,
                                                       m.latencyMs, srScale, maxDelay,
                                                       m.blockSize, kObSubBlock, m.frBasePrev,
                                                       frDelaySamples);

            if (inputs[in] != nullptr)
                memcpy (inFlat + (size_t) in * m.blockSize, inputs[in], (size_t) m.blockSize * sizeof (float));
            else
                memset (inFlat + (size_t) in * m.blockSize, 0, (size_t) m.blockSize * sizeof (float));
            m.frHost.filterBlockForInput (in, inputs, frInFlat, m.blockSize);
        });
        m.frHost.commitSubSteps (numSubBlocks);   // hoisted subStepCounter += numSubBlocks

        // Scatter FR tiers: scan pair activity (prev|curr FR gain != 0) and
        // coalesce consecutive active pairs into upload runs. Runs on the pump
        // thread AFTER the join (needs the full staged fgCurr). lastFrGains still
        // holds LAST launch's staged gains here (refresh happens at upload time
        // below) — numerically the same frGainsPrev the kernel's doFr gate reads
        // and the same predicate the per-sample fill used, so filled == uploaded
        // == kernel-read rows, exactly. Toggle parity: a pair turning off stays
        // active for one ramp-out block (prev != 0), then drops out.
        m.frActiveRuns.clear();
        {
            uint32_t runStart = 0;
            bool inRun = false;
            for (uint32_t i = 0; i < matrix; ++i)
            {
                const bool active = m.lastFrGains[i] != 0.0f || fgCurr[i] != 0.0f;
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

        // Upload diet: memcmp each freshly staged matrix against its lastStaged
        // baseline. Unchanged => skip the memcpy AND the slot swap (bind prev ==
        // curr == the live slot; the kernel ramps x->x = x). Changed => memcpy
        // into the alternate slot, prev = the previous slot (last launch's curr,
        // already on-device), swap. First launch: single memcpy, prev == curr
        // (the old havePrev bootstrap, bit-exact). Safe against in-flight reads:
        // the previous block ended with [cb waitUntilCompleted], so no launch is
        // reading either slot while we write it. (Shared storage: a skipped
        // memcpy leaves the slot's previous contents intact — CUDA skip
        // semantics exactly.)
        auto stagePair = [matrix] (Impl::PingPong& pp, const float* staged,
                                   std::vector<float>& last,
                                   id<MTLBuffer>& prevArg, id<MTLBuffer>& currArg)
        {
            const size_t bytes = (size_t) matrix * sizeof (float);
            if (pp.everUploaded && std::memcmp (staged, last.data(), bytes) == 0)
            {
                prevArg = currArg = pp.slot[pp.curr];      // unchanged: no upload, no swap
                return;
            }
            const int next = pp.everUploaded ? (pp.curr ^ 1) : pp.curr;
            memcpy (pp.slot[next].contents, staged, bytes);   // "upload" into alternate slot
            std::memcpy (last.data(), staged, bytes);
            prevArg = pp.slot[pp.curr];                    // first launch: prev == curr
            currArg = pp.slot[next];
            pp.curr = next;
            pp.everUploaded = true;
        };
        auto stageSingle = [matrix] (id<MTLBuffer> dBuf, const float* staged,
                                     std::vector<float>& last, bool& uploaded)
        {
            const size_t bytes = (size_t) matrix * sizeof (float);
            if (uploaded && std::memcmp (staged, last.data(), bytes) == 0)
                return;
            memcpy (dBuf.contents, staged, bytes);
            std::memcpy (last.data(), staged, bytes);
            uploaded = true;
        };

        id<MTLBuffer> dDelaysPrevArg = nil, dDelaysCurrArg = nil;
        id<MTLBuffer> dGainsPrevArg = nil, dGainsCurrArg = nil;
        id<MTLBuffer> dFrGainsPrevArg = nil, dFrGainsCurrArg = nil;
        stagePair (m.ppDelays,  dCurr,  m.lastDelays,  dDelaysPrevArg,  dDelaysCurrArg);
        stagePair (m.ppGains,   gCurr,  m.lastGains,   dGainsPrevArg,   dGainsCurrArg);
        stagePair (m.ppFrGains, fgCurr, m.lastFrGains, dFrGainsPrevArg, dFrGainsCurrArg);
        stageSingle (m.bHfAttenDbDev,   hfDb,   m.lastHfAttenDb,   m.hfUploaded);
        stageSingle (m.bFrHfAttenDbDev, frHfDb, m.lastFrHfAttenDb, m.frHfUploaded);

        // Per-sample FR delays: upload only the coalesced active-pair runs; no
        // runs (FR fully idle) => the whole [pairs][blockSize] copy disappears.
        // Every row the kernel will read this block was freshly filled this
        // block; never-active device rows keep their prepare-time zeros.
        {
            const float* frStaged = (const float*) m.bFrDelaySamples.contents;
            float* frDev = (float*) m.bFrDelaySamplesDev.contents;
            for (const auto& r : m.frActiveRuns)
                memcpy (frDev + (size_t) r.first * m.blockSize,
                        frStaged + (size_t) r.first * m.blockSize,
                        (size_t) r.second * m.blockSize * sizeof (float));
        }

        ObParamsGpu p { (uint32_t) m.numIn, (uint32_t) m.numOut, (uint32_t) m.blockSize,
                        m.accLen, m.writePos, m.shelfCosW0, m.shelfSinW0 };
        memcpy (m.bParams.contents, &p, sizeof (p));

        id<MTLCommandBuffer> cb = [m.queue commandBuffer];
        id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder]; // serial dispatch type

        // K1: per-pair filter + emit + scatter (one thread per pair).
        [enc setComputePipelineState: m.psoPairs];
        [enc setBuffer: m.bParams offset: 0 atIndex: 0];
        [enc setBuffer: m.bIn offset: 0 atIndex: 1];
        [enc setBuffer: m.bFrIn offset: 0 atIndex: 2];
        [enc setBuffer: m.bHfAttenDbDev offset: 0 atIndex: 3];
        [enc setBuffer: m.bFrHfAttenDbDev offset: 0 atIndex: 4];
        [enc setBuffer: dDelaysPrevArg offset: 0 atIndex: 5];
        [enc setBuffer: dDelaysCurrArg offset: 0 atIndex: 6];
        [enc setBuffer: dGainsPrevArg offset: 0 atIndex: 7];
        [enc setBuffer: dGainsCurrArg offset: 0 atIndex: 8];
        [enc setBuffer: m.bFrDelaySamplesDev offset: 0 atIndex: 9];
        [enc setBuffer: dFrGainsPrevArg offset: 0 atIndex: 10];
        [enc setBuffer: dFrGainsCurrArg offset: 0 atIndex: 11];
        [enc setBuffer: m.bShelfState offset: 0 atIndex: 12];
        [enc setBuffer: m.bFrShelfState offset: 0 atIndex: 13];
        [enc setBuffer: m.bPairAcc offset: 0 atIndex: 14];
        [enc setBuffer: m.bPairOut offset: 0 atIndex: 15];
        {
            const NSUInteger pairs = (NSUInteger) matrix;
            const NSUInteger groups = (pairs + m.threadsPerGroup - 1) / m.threadsPerGroup;
            [enc dispatchThreadgroups: MTLSizeMake (groups, 1, 1)
                threadsPerThreadgroup: MTLSizeMake (m.threadsPerGroup, 1, 1)];
        }

        // K2: deterministic per-output reduction (ordered after K1 by the
        // serial encoder). Rebind only the slots the reduce kernel uses.
        [enc setComputePipelineState: m.psoReduce];
        [enc setBuffer: m.bParams offset: 0 atIndex: 0];
        [enc setBuffer: m.bPairOut offset: 0 atIndex: 1];
        [enc setBuffer: m.bOut offset: 0 atIndex: 2];
        [enc dispatchThreadgroups: MTLSizeMake ((NSUInteger) m.numOut, 1, 1)
            threadsPerThreadgroup: MTLSizeMake (m.threadsPerGroup, 1, 1)];

        [enc endEncoding];
        [cb commit];
        [cb waitUntilCompleted];

        if (cb.status == MTLCommandBufferStatusError)
        {
            lastError = std::string ("GPU launch failed: ")
                        + (cb.error ? cb.error.localizedDescription.UTF8String : "?");
            ready = false;
            return false;
        }

        // Output flat buffer -> channels
        const float* outFlat = (const float*) m.bOut.contents;
        for (int ch = 0; ch < m.numOut; ++ch)
            if (outputs[ch] != nullptr)
                memcpy (outputs[ch], outFlat + (size_t) ch * m.blockSize, (size_t) m.blockSize * sizeof (float));

        // Advance host-tracked state (FR delay continuity lives in frBasePrev,
        // updated inside computeFrDelaysPerSample; matrix prev continuity now
        // lives on the device: the ping-pong slots + lastStaged baselines).
        m.writePos = (m.writePos + (uint32_t) m.blockSize) % m.accLen;

        lastLaunchMs = std::chrono::duration<double, std::milli> (
                           std::chrono::steady_clock::now() - t0).count();
    }
    return true;
}

void MetalObBackend::reset() noexcept
{
    auto& m = *impl;
    if (m.bPairAcc != nil)
        memset (m.bPairAcc.contents, 0, m.bPairAcc.length);
    if (m.bShelfState != nil)
        memset (m.bShelfState.contents, 0, m.bShelfState.length);
    if (m.bFrShelfState != nil)
        memset (m.bFrShelfState.contents, 0, m.bFrShelfState.length);
    m.frHost.reset();
    m.writePos = 0;

    // Upload-diet state back to first-launch semantics: the next block
    // force-uploads every matrix and binds prev == curr (old havePrev
    // bootstrap). lastFrGains back to zeros = the first-launch FR gate input.
    for (auto* pp : { &m.ppDelays, &m.ppGains, &m.ppFrGains })
        pp->everUploaded = false;
    m.hfUploaded = false;
    m.frHfUploaded = false;
    for (auto* v : { &m.lastDelays, &m.lastGains, &m.lastFrGains,
                     &m.lastHfAttenDb, &m.lastFrHfAttenDb })
        std::fill (v->begin(), v->end(), 0.0f);
}

void MetalObBackend::release() noexcept
{
    auto& m = *impl;

    // M3: join the host worker pool BEFORE any Metal teardown (host memory only).
    m.pool.shutdown();

    @autoreleasepool
    {
        m.bParams = m.bIn = m.bFrIn = m.bOut = nil;
        m.bPairAcc = m.bPairOut = nil;
        m.bShelfState = m.bFrShelfState = nil;
        m.bDelaysCurr = m.bGainsCurr = m.bFrGainsCurr = nil;
        m.bFrDelaySamples = m.bFrDelaySamplesDev = nil;
        m.bHfAttenDb = m.bFrHfAttenDb = nil;
        m.bHfAttenDbDev = m.bFrHfAttenDbDev = nil;
        for (auto* pp : { &m.ppDelays, &m.ppGains, &m.ppFrGains })
        {
            pp->slot[0] = nil;
            pp->slot[1] = nil;
            pp->curr = 0;
            pp->everUploaded = false;
        }
        m.hfUploaded = false;
        m.frHfUploaded = false;
        m.psoPairs = nil;
        m.psoReduce = nil;
        m.queue = nil;
        m.device = nil;
    }
    m.delaysMs = nullptr;
    m.gains = nullptr;
    m.hfAttenDb = nullptr;
    m.frDelaysMs = nullptr;
    m.frLevels = nullptr;
    m.frHfAttenDb = nullptr;
    ready = false;
}

} // namespace spatcore::gpu
