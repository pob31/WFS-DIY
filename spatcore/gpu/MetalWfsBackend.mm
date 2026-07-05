/*
    MetalWfsBackend implementation (Objective-C++).

    The MSL kernel sources live in MetalWfsKernels.h as a string literal so
    the app needs no resource plumbing; they are compiled at prepare() time
    (~10 ms once) and cached in two pipeline state objects (wfs_pairs +
    wfs_reduce).

    Host-side processBlock mirrors CudaWfsBackend.cpp (the validated
    reference - Experiments/cuda-wfs-test checks the CUDA twin of the kernel
    string against a CPU model on real hardware): matrix snapshot with -L
    compensation, prev->curr ramp continuity, persistent device rings + shelf
    states, host-tracked ring advance. Floor-Reflection host work (per-input
    pre-filter, diffusion jitter, FR matrix snapshot) lives in the shared
    WfsFrHostState.

    Dispatch ordering: one command buffer, ONE serial compute encoder, two
    dispatches (wfs_pairs then wfs_reduce). A serial-dispatch-type encoder
    orders consecutive dispatches with full memory visibility, so no barrier
    is needed. (Switching to MTLDispatchTypeConcurrent would require a
    memoryBarrierWithScope between them.)
*/

#include "MetalWfsBackend.h"
#include "MetalWfsKernels.h"
#include "WfsFrHostState.h"
#include "GpuHostWorkPool.h"

#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
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

struct MetalWfsBackend::Impl
{
    id<MTLDevice> device = nil;
    id<MTLCommandQueue> queue = nil;
    id<MTLComputePipelineState> psoPairs = nil;
    id<MTLComputePipelineState> psoReduce = nil;

    id<MTLBuffer> bParams = nil;
    id<MTLBuffer> bIn = nil;
    id<MTLBuffer> bFrIn = nil;
    id<MTLBuffer> bOut = nil;
    id<MTLBuffer> bRing = nil;
    id<MTLBuffer> bFrRing = nil;
    id<MTLBuffer> bScratch = nil;        // [(s*numOut+out)*numIn+in], rewritten each launch
    id<MTLBuffer> bShelfState = nil;     // [pairs][4] persistent
    id<MTLBuffer> bFrShelfState = nil;   // [pairs][4] persistent
    // Host staging (curr snapshot only — prev never leaves the device, see the
    // PingPong note below). The GPU reads the ping-pong slots / the *Dev single
    // buffers, never these staging buffers directly.
    id<MTLBuffer> bDelaysCurr = nil;
    id<MTLBuffer> bGainsCurr = nil;
    id<MTLBuffer> bFrDelaysCurr = nil;
    id<MTLBuffer> bFrGainsCurr = nil;
    id<MTLBuffer> bHfAttenDb = nil, bFrHfAttenDb = nil;         // single-buffer staging
    id<MTLBuffer> bHfAttenDbDev = nil, bFrHfAttenDbDev = nil;   // single device buffers (GPU-read)

    // Upload diet (M2): device ping-pong per prev/curr matrix pair, mirrored
    // from CudaWfsBackend.cpp (d9893cc). The kernel takes the matrix pointers
    // as bound BUFFERS and only READS them (const, MetalWfsKernels.h), so
    // "prev" never needs a host round-trip: it is simply the slot uploaded one
    // launch earlier. Only a CHANGED curr is uploaded (memcpy'd into the
    // alternate slot, then swap); unchanged blocks bind prev == curr == the
    // live slot — the kernel ramps x->x = x, bit-exact, and aliasing the two
    // bindings is legal because neither is written through. First launch:
    // upload once, bind prev == curr (the old havePrev bootstrap semantics).
    // The pump is fully synchronous ([cb waitUntilCompleted] before the next
    // fill), so memcpy'ing a slot the previous launch read is hazard-free.
    struct PingPong
    {
        id<MTLBuffer> slot[2] = { nil, nil };
        int  curr = 0;               // slot holding the last-consumed curr
        bool everUploaded = false;   // first launch: upload once, prev == curr
    };
    PingPong ppDelays, ppGains, ppFrDelays, ppFrGains;
    bool hfUploaded = false;         // single-buffer change-detect state
    bool frHfUploaded = false;

    // Change-detect baselines: the last STAGED (== last uploaded) copy of each
    // matrix, memcmp'd against the freshly staged buffer. Comparing staged
    // copies (never the live app matrices) is torn-read-safe: it compares
    // exactly the values the kernel would consume.
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
    // before any Metal teardown; workers touch only host memory (plan section 3).
    GpuHostWorkPool pool;

    NSUInteger threadsPerGroup = 256;
};

// deviceIndex is accepted for backend-API uniformity but ignored on macOS:
// device selection binds the system default device (multi-device enumeration
// for Metal is a separate follow-up).
MetalWfsBackend::MetalWfsBackend (int deviceIndex) : impl (std::make_unique<Impl>()) { (void) deviceIndex; }
MetalWfsBackend::~MetalWfsBackend() { release(); }

bool MetalWfsBackend::prepare (int numInputs, int numOutputs, int blockSize,
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
                                  [NSString stringWithUTF8String: kWfsDelaySumKernelSource]
                                                    options: nil
                                                      error: &err];
        if (lib == nil)
        {
            lastError = std::string ("Kernel compile failed: ")
                        + (err ? err.localizedDescription.UTF8String : "?");
            return false;
        }

        m.psoPairs = [m.device newComputePipelineStateWithFunction:
                          [lib newFunctionWithName: @"wfs_pairs"]
                                                             error: &err];
        if (m.psoPairs == nil)
        {
            lastError = std::string ("Pipeline state failed (pairs): ")
                        + (err ? err.localizedDescription.UTF8String : "?");
            return false;
        }
        m.psoReduce = [m.device newComputePipelineStateWithFunction:
                           [lib newFunctionWithName: @"wfs_reduce"]
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
        m.maxDelaySamples = (uint32_t) (maxDelaySeconds * sampleRate);
        m.ringCapacity = m.maxDelaySamples + (uint32_t) m.blockSize;
        m.ringWritePos = 0;
        m.ringValid = 0;

        // Threadgroup size must satisfy BOTH PSOs; pairGroups derives from it.
        m.threadsPerGroup = std::min<NSUInteger> (256,
                                std::min (m.psoPairs.maxTotalThreadsPerThreadgroup,
                                          m.psoReduce.maxTotalThreadsPerThreadgroup));
        m.pairGroups = (uint32_t) (((NSUInteger) (m.numIn * m.numOut) + m.threadsPerGroup - 1)
                                   / m.threadsPerGroup);

        // Fixed 800 Hz shelf frequency (WFSHighShelfFilter parity).
        const double w0 = 2.0 * 3.14159265358979 * 800.0 / sampleRate;
        m.shelfCosW0 = (float) std::cos (w0);
        m.shelfSinW0 = (float) std::sin (w0);

        const uint32_t matrix = (uint32_t) (m.numIn * m.numOut);
        auto shared = MTLResourceStorageModeShared;
        auto mkBuf = [&](size_t floats) {
            return [m.device newBufferWithLength: floats * sizeof (float) options: shared];
        };

        m.bParams = [m.device newBufferWithLength: sizeof (WfsParamsGpu) options: shared];
        m.bIn = mkBuf ((size_t) m.numIn * m.blockSize);
        m.bFrIn = mkBuf ((size_t) m.numIn * m.blockSize);
        m.bOut = mkBuf ((size_t) m.numOut * m.blockSize);
        m.bRing = mkBuf ((size_t) m.numIn * m.ringCapacity);
        m.bFrRing = mkBuf ((size_t) m.numIn * m.ringCapacity);
        m.bScratch = mkBuf ((size_t) m.numIn * m.numOut * m.blockSize);
        m.bShelfState = mkBuf ((size_t) matrix * 4);
        m.bFrShelfState = mkBuf ((size_t) matrix * 4);
        m.bDelaysCurr = mkBuf (matrix);
        m.bGainsCurr = mkBuf (matrix);
        m.bFrDelaysCurr = mkBuf (matrix);
        m.bFrGainsCurr = mkBuf (matrix);
        m.bHfAttenDb = mkBuf (matrix);
        m.bFrHfAttenDb = mkBuf (matrix);
        for (auto* pp : { &m.ppDelays, &m.ppGains, &m.ppFrDelays, &m.ppFrGains })
        {
            pp->slot[0] = mkBuf (matrix);
            pp->slot[1] = mkBuf (matrix);
            pp->curr = 0;
            pp->everUploaded = false;
        }
        m.bHfAttenDbDev = mkBuf (matrix);
        m.bFrHfAttenDbDev = mkBuf (matrix);
        m.hfUploaded = false;
        m.frHfUploaded = false;

        if (! (m.bParams && m.bIn && m.bFrIn && m.bOut && m.bRing && m.bFrRing
               && m.bScratch && m.bShelfState && m.bFrShelfState
               && m.bDelaysCurr && m.bGainsCurr && m.bFrDelaysCurr && m.bFrGainsCurr
               && m.ppDelays.slot[0] && m.ppDelays.slot[1]
               && m.ppGains.slot[0] && m.ppGains.slot[1]
               && m.ppFrDelays.slot[0] && m.ppFrDelays.slot[1]
               && m.ppFrGains.slot[0] && m.ppFrGains.slot[1]
               && m.bHfAttenDb && m.bFrHfAttenDb
               && m.bHfAttenDbDev && m.bFrHfAttenDbDev))
        {
            lastError = "Metal buffer allocation failed";
            release();
            return false;
        }

        memset (m.bRing.contents, 0, m.bRing.length);
        memset (m.bFrRing.contents, 0, m.bFrRing.length);
        memset (m.bShelfState.contents, 0, m.bShelfState.length);
        memset (m.bFrShelfState.contents, 0, m.bFrShelfState.length);

        m.lastDelays.assign (matrix, 0.0f);
        m.lastGains.assign (matrix, 0.0f);
        m.lastFrDelays.assign (matrix, 0.0f);
        m.lastFrGains.assign (matrix, 0.0f);
        m.lastHfAttenDb.assign (matrix, 0.0f);
        m.lastFrHfAttenDb.assign (matrix, 0.0f);

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

void MetalWfsBackend::setMatrixPointers (const float* delaysMsPtr, const float* gainsPtr,
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

void MetalWfsBackend::setFRFilterParams (int inputIndex,
                                         bool lowCutActive, float lowCutFreq,
                                         bool highShelfActive, float highShelfFreq,
                                         float highShelfGain, float highShelfSlope) noexcept
{
    impl->frHost.setFRFilterParams (inputIndex, lowCutActive, lowCutFreq,
                                    highShelfActive, highShelfFreq,
                                    highShelfGain, highShelfSlope);
}

void MetalWfsBackend::setFRDiffusion (int inputIndex, float diffusionPercent) noexcept
{
    impl->frHost.setFRDiffusion (inputIndex, diffusionPercent);
}

bool MetalWfsBackend::processBlock (const float* const* inputs, float* const* outputs)
{
    if (! ready)
        return false;

    auto& m = *impl;
    const uint32_t matrix = (uint32_t) (m.numIn * m.numOut);
    const float srScale = (float) (m.sampleRate / 1000.0);
    const float maxDelay = (float) m.maxDelaySamples;

    @autoreleasepool
    {
        const auto t0 = std::chrono::steady_clock::now();

        // Shared-storage staging pointers (the GPU reads the ping-pong slots /
        // *Dev buffers, never these directly). Captured before the parallelFor;
        // the staging (memcmp/upload) below reads them AFTER the join.
        float* dCurr  = (float*) m.bDelaysCurr.contents;
        float* gCurr  = (float*) m.bGainsCurr.contents;
        float* hfDb   = (float*) m.bHfAttenDb.contents;
        float* frHfDb = (float*) m.bFrHfAttenDb.contents;
        float* fdCurr = (float*) m.bFrDelaysCurr.contents;
        float* fgCurr = (float*) m.bFrGainsCurr.contents;
        float* inFlat = (float*) m.bIn.contents;
        float* frInFlat = (float*) m.bFrIn.contents;

        // M3: ONE fused parallelFor(numIn) — input pack, per-input FR pre-filter,
        // per-input jitter advance (launch index HOISTED), and the direct +
        // shelf + FR-curr snapshot rows for that input. Item-indexed state only
        // => bit-identical for any worker count (section-4 determinism table).
        const uint32_t launchIdx = m.frHost.currentLaunchIndex();
        m.pool.parallelFor (m.numIn, [&] (int in)
        {
            const int nOut = m.numOut;
            const size_t rowBase = (size_t) in * (size_t) nOut;

            for (int out = 0; out < nOut; ++out)
            {
                const size_t i = rowBase + (size_t) out;
                const float d = m.delaysMs != nullptr ? (m.delaysMs[i] - m.latencyMs) * srScale : 0.0f;
                dCurr[i]  = std::clamp (d, 0.0f, maxDelay);
                gCurr[i]  = m.gains      != nullptr ? m.gains[i]      : 0.0f;
                hfDb[i]   = m.hfAttenDb   != nullptr ? m.hfAttenDb[i]   : 0.0f;
                frHfDb[i] = m.frHfAttenDb != nullptr ? m.frHfAttenDb[i] : 0.0f;
            }

            m.frHost.advanceJitterForInput (in, launchIdx, m.blockSize);
            m.frHost.computeFrCurrForInput (in, m.delaysMs, m.frDelaysMs, m.frLevels,
                                            m.latencyMs, srScale, maxDelay,
                                            fdCurr, fgCurr);

            if (inputs[in] != nullptr)
                memcpy (inFlat + (size_t) in * m.blockSize, inputs[in], (size_t) m.blockSize * sizeof (float));
            else
                memset (inFlat + (size_t) in * m.blockSize, 0, (size_t) m.blockSize * sizeof (float));
            m.frHost.filterBlockForInput (in, inputs, frInFlat, m.blockSize);
        });
        m.frHost.commitJitterLaunch();   // hoisted ++launchCounter, once after the join

        // Upload diet: memcmp each freshly staged matrix against its lastStaged
        // baseline. Unchanged => skip the memcpy AND the slot swap (bind prev ==
        // curr == the live slot; the kernel ramps x->x = x). Changed => memcpy
        // into the alternate slot, prev = the previous slot (last launch's curr,
        // already on-device), swap. First launch: single memcpy, prev == curr
        // (the old havePrev bootstrap, bit-exact). Safe against in-flight reads:
        // the previous block ended with [cb waitUntilCompleted], so no launch is
        // reading either slot while we write it. (Shared storage: a skipped
        // memcpy leaves the slot's previous contents intact — the CUDA skip
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
        id<MTLBuffer> dFrDelaysPrevArg = nil, dFrDelaysCurrArg = nil;
        id<MTLBuffer> dFrGainsPrevArg = nil, dFrGainsCurrArg = nil;
        stagePair (m.ppDelays,   dCurr,  m.lastDelays,   dDelaysPrevArg,   dDelaysCurrArg);
        stagePair (m.ppGains,    gCurr,  m.lastGains,    dGainsPrevArg,    dGainsCurrArg);
        stagePair (m.ppFrDelays, fdCurr, m.lastFrDelays, dFrDelaysPrevArg, dFrDelaysCurrArg);
        stagePair (m.ppFrGains,  fgCurr, m.lastFrGains,  dFrGainsPrevArg,  dFrGainsCurrArg);
        stageSingle (m.bHfAttenDbDev,   hfDb,   m.lastHfAttenDb,   m.hfUploaded);
        stageSingle (m.bFrHfAttenDbDev, frHfDb, m.lastFrHfAttenDb, m.frHfUploaded);

        WfsParamsGpu p { (uint32_t) m.numIn, (uint32_t) m.numOut, (uint32_t) m.blockSize,
                         m.ringCapacity, m.ringWritePos, m.ringValid,
                         m.pairGroups, m.shelfCosW0, m.shelfSinW0 };
        memcpy (m.bParams.contents, &p, sizeof (p));

        id<MTLCommandBuffer> cb = [m.queue commandBuffer];
        id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder]; // serial dispatch type

        // K1: pair role + both ring appends.
        [enc setComputePipelineState: m.psoPairs];
        [enc setBuffer: m.bParams offset: 0 atIndex: 0];
        [enc setBuffer: m.bIn offset: 0 atIndex: 1];
        [enc setBuffer: m.bFrIn offset: 0 atIndex: 2];
        [enc setBuffer: m.bRing offset: 0 atIndex: 3];
        [enc setBuffer: m.bFrRing offset: 0 atIndex: 4];
        [enc setBuffer: dDelaysPrevArg offset: 0 atIndex: 5];
        [enc setBuffer: dDelaysCurrArg offset: 0 atIndex: 6];
        [enc setBuffer: dGainsPrevArg offset: 0 atIndex: 7];
        [enc setBuffer: dGainsCurrArg offset: 0 atIndex: 8];
        [enc setBuffer: dFrDelaysPrevArg offset: 0 atIndex: 9];
        [enc setBuffer: dFrDelaysCurrArg offset: 0 atIndex: 10];
        [enc setBuffer: dFrGainsPrevArg offset: 0 atIndex: 11];
        [enc setBuffer: dFrGainsCurrArg offset: 0 atIndex: 12];
        [enc setBuffer: m.bHfAttenDbDev offset: 0 atIndex: 13];
        [enc setBuffer: m.bFrHfAttenDbDev offset: 0 atIndex: 14];
        [enc setBuffer: m.bShelfState offset: 0 atIndex: 15];
        [enc setBuffer: m.bFrShelfState offset: 0 atIndex: 16];
        [enc setBuffer: m.bScratch offset: 0 atIndex: 17];
        [enc dispatchThreadgroups: MTLSizeMake ((NSUInteger) m.pairGroups + 2 * (NSUInteger) m.numIn, 1, 1)
            threadsPerThreadgroup: MTLSizeMake (m.threadsPerGroup, 1, 1)];

        // K2: deterministic per-output reduction (ordered after K1 by the
        // serial encoder). Rebind only the slots the reduce kernel uses.
        [enc setComputePipelineState: m.psoReduce];
        [enc setBuffer: m.bScratch offset: 0 atIndex: 1];
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

        // Advance host-tracked state (matrix prev continuity now lives on the
        // device: the ping-pong slots + lastStaged baselines, updated at upload).
        m.ringWritePos = (m.ringWritePos + (uint32_t) m.blockSize) % m.ringCapacity;
        m.ringValid = std::min (m.maxDelaySamples, m.ringValid + (uint32_t) m.blockSize);

        lastLaunchMs = std::chrono::duration<double, std::milli> (
                           std::chrono::steady_clock::now() - t0).count();
    }
    return true;
}

void MetalWfsBackend::reset() noexcept
{
    auto& m = *impl;
    if (m.bRing != nil)
        memset (m.bRing.contents, 0, m.bRing.length);
    if (m.bFrRing != nil)
        memset (m.bFrRing.contents, 0, m.bFrRing.length);
    if (m.bShelfState != nil)
        memset (m.bShelfState.contents, 0, m.bShelfState.length);
    if (m.bFrShelfState != nil)
        memset (m.bFrShelfState.contents, 0, m.bFrShelfState.length);
    m.frHost.reset();
    m.ringWritePos = 0;
    m.ringValid = 0;

    // Upload-diet state back to first-launch semantics: the next block
    // force-uploads every matrix and binds prev == curr (old havePrev
    // bootstrap). lastStaged baselines re-zeroed to match prepare().
    for (auto* pp : { &m.ppDelays, &m.ppGains, &m.ppFrDelays, &m.ppFrGains })
        pp->everUploaded = false;
    m.hfUploaded = false;
    m.frHfUploaded = false;
    for (auto* v : { &m.lastDelays, &m.lastGains, &m.lastFrDelays, &m.lastFrGains,
                     &m.lastHfAttenDb, &m.lastFrHfAttenDb })
        std::fill (v->begin(), v->end(), 0.0f);
}

void MetalWfsBackend::release() noexcept
{
    auto& m = *impl;

    // M3: join the host worker pool BEFORE any Metal teardown (host memory only).
    m.pool.shutdown();

    @autoreleasepool
    {
        m.bParams = m.bIn = m.bFrIn = m.bOut = nil;
        m.bRing = m.bFrRing = m.bScratch = nil;
        m.bShelfState = m.bFrShelfState = nil;
        m.bDelaysCurr = m.bGainsCurr = nil;
        m.bFrDelaysCurr = m.bFrGainsCurr = nil;
        m.bHfAttenDb = m.bFrHfAttenDb = nil;
        m.bHfAttenDbDev = m.bFrHfAttenDbDev = nil;
        for (auto* pp : { &m.ppDelays, &m.ppGains, &m.ppFrDelays, &m.ppFrGains })
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
