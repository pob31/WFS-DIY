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

#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <vector>

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
    id<MTLBuffer> bDelaysPrev = nil, bDelaysCurr = nil;
    id<MTLBuffer> bGainsPrev = nil, bGainsCurr = nil;
    id<MTLBuffer> bFrDelaysPrev = nil, bFrDelaysCurr = nil;
    id<MTLBuffer> bFrGainsPrev = nil, bFrGainsCurr = nil;
    id<MTLBuffer> bHfAttenDb = nil, bFrHfAttenDb = nil;

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

    // Last launch's end matrices (ramp continuity).
    std::vector<float> delaysPrevSamples;
    std::vector<float> gainsPrev;
    std::vector<float> frDelaysPrevSamples;
    std::vector<float> frGainsPrev;
    bool havePrev = false;

    WfsFrHostState frHost;            // per-input FR pre-filters + jitter

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
        m.havePrev = false;

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
        m.bDelaysPrev = mkBuf (matrix);
        m.bDelaysCurr = mkBuf (matrix);
        m.bGainsPrev = mkBuf (matrix);
        m.bGainsCurr = mkBuf (matrix);
        m.bFrDelaysPrev = mkBuf (matrix);
        m.bFrDelaysCurr = mkBuf (matrix);
        m.bFrGainsPrev = mkBuf (matrix);
        m.bFrGainsCurr = mkBuf (matrix);
        m.bHfAttenDb = mkBuf (matrix);
        m.bFrHfAttenDb = mkBuf (matrix);

        if (! (m.bParams && m.bIn && m.bFrIn && m.bOut && m.bRing && m.bFrRing
               && m.bScratch && m.bShelfState && m.bFrShelfState
               && m.bDelaysPrev && m.bDelaysCurr && m.bGainsPrev && m.bGainsCurr
               && m.bFrDelaysPrev && m.bFrDelaysCurr && m.bFrGainsPrev && m.bFrGainsCurr
               && m.bHfAttenDb && m.bFrHfAttenDb))
        {
            lastError = "Metal buffer allocation failed";
            release();
            return false;
        }

        memset (m.bRing.contents, 0, m.bRing.length);
        memset (m.bFrRing.contents, 0, m.bFrRing.length);
        memset (m.bShelfState.contents, 0, m.bShelfState.length);
        memset (m.bFrShelfState.contents, 0, m.bFrShelfState.length);

        m.delaysPrevSamples.assign (matrix, 0.0f);
        m.gainsPrev.assign (matrix, 0.0f);
        m.frDelaysPrevSamples.assign (matrix, 0.0f);
        m.frGainsPrev.assign (matrix, 0.0f);

        m.frHost.prepare (m.numIn, m.numOut, sampleRate);
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

        // Snapshot the live matrices -> curr (with -L compensation, clamped),
        // prev = the previous launch's curr (ramp continuity).
        float* dCurr = (float*) m.bDelaysCurr.contents;
        float* gCurr = (float*) m.bGainsCurr.contents;
        for (uint32_t i = 0; i < matrix; ++i)
        {
            float d = m.delaysMs != nullptr ? (m.delaysMs[i] - m.latencyMs) * srScale : 0.0f;
            dCurr[i] = std::clamp (d, 0.0f, maxDelay);
            gCurr[i] = m.gains != nullptr ? m.gains[i] : 0.0f;
        }

        // Shelf gains: raw dB, stepwise per launch (CPU parity: per-block setGainDb).
        float* hfDb = (float*) m.bHfAttenDb.contents;
        float* frHfDb = (float*) m.bFrHfAttenDb.contents;
        for (uint32_t i = 0; i < matrix; ++i)
        {
            hfDb[i] = m.hfAttenDb != nullptr ? m.hfAttenDb[i] : 0.0f;
            frHfDb[i] = m.frHfAttenDb != nullptr ? m.frHfAttenDb[i] : 0.0f;
        }

        // FR: advance diffusion jitter (64-sample sub-step cadence), then
        // snapshot the FR curr matrices. The pipeline latency is subtracted
        // from the ABSOLUTE FR delay (direct + extra + jitter - L),
        // preserving the FR-vs-direct offset exactly.
        float* fdCurr = (float*) m.bFrDelaysCurr.contents;
        float* fgCurr = (float*) m.bFrGainsCurr.contents;
        m.frHost.advanceJitter (m.blockSize);
        m.frHost.computeFrCurr (m.delaysMs, m.frDelaysMs, m.frLevels,
                                m.latencyMs, srScale, maxDelay,
                                fdCurr, fgCurr);

        if (! m.havePrev)
        {
            memcpy (m.delaysPrevSamples.data(), dCurr, matrix * sizeof (float));
            memcpy (m.gainsPrev.data(), gCurr, matrix * sizeof (float));
            memcpy (m.frDelaysPrevSamples.data(), fdCurr, matrix * sizeof (float));
            memcpy (m.frGainsPrev.data(), fgCurr, matrix * sizeof (float));
            m.havePrev = true;
        }
        memcpy (m.bDelaysPrev.contents, m.delaysPrevSamples.data(), matrix * sizeof (float));
        memcpy (m.bGainsPrev.contents, m.gainsPrev.data(), matrix * sizeof (float));
        memcpy (m.bFrDelaysPrev.contents, m.frDelaysPrevSamples.data(), matrix * sizeof (float));
        memcpy (m.bFrGainsPrev.contents, m.frGainsPrev.data(), matrix * sizeof (float));

        // Input channels -> flat shared buffer (silence for missing channels),
        // and the host-side FR pre-filter chain -> frIn staging.
        float* inFlat = (float*) m.bIn.contents;
        for (int ch = 0; ch < m.numIn; ++ch)
        {
            if (inputs[ch] != nullptr)
                memcpy (inFlat + (size_t) ch * m.blockSize, inputs[ch], (size_t) m.blockSize * sizeof (float));
            else
                memset (inFlat + (size_t) ch * m.blockSize, 0, (size_t) m.blockSize * sizeof (float));
        }
        m.frHost.filterBlock (inputs, (float*) m.bFrIn.contents, m.blockSize);

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
        [enc setBuffer: m.bDelaysPrev offset: 0 atIndex: 5];
        [enc setBuffer: m.bDelaysCurr offset: 0 atIndex: 6];
        [enc setBuffer: m.bGainsPrev offset: 0 atIndex: 7];
        [enc setBuffer: m.bGainsCurr offset: 0 atIndex: 8];
        [enc setBuffer: m.bFrDelaysPrev offset: 0 atIndex: 9];
        [enc setBuffer: m.bFrDelaysCurr offset: 0 atIndex: 10];
        [enc setBuffer: m.bFrGainsPrev offset: 0 atIndex: 11];
        [enc setBuffer: m.bFrGainsCurr offset: 0 atIndex: 12];
        [enc setBuffer: m.bHfAttenDb offset: 0 atIndex: 13];
        [enc setBuffer: m.bFrHfAttenDb offset: 0 atIndex: 14];
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

        // Advance host-tracked state
        m.ringWritePos = (m.ringWritePos + (uint32_t) m.blockSize) % m.ringCapacity;
        m.ringValid = std::min (m.maxDelaySamples, m.ringValid + (uint32_t) m.blockSize);
        memcpy (m.delaysPrevSamples.data(), dCurr, matrix * sizeof (float));
        memcpy (m.gainsPrev.data(), gCurr, matrix * sizeof (float));
        memcpy (m.frDelaysPrevSamples.data(), fdCurr, matrix * sizeof (float));
        memcpy (m.frGainsPrev.data(), fgCurr, matrix * sizeof (float));

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
    m.havePrev = false;
}

void MetalWfsBackend::release() noexcept
{
    auto& m = *impl;
    @autoreleasepool
    {
        m.bParams = m.bIn = m.bFrIn = m.bOut = nil;
        m.bRing = m.bFrRing = m.bScratch = nil;
        m.bShelfState = m.bFrShelfState = nil;
        m.bDelaysPrev = m.bDelaysCurr = m.bGainsPrev = m.bGainsCurr = nil;
        m.bFrDelaysPrev = m.bFrDelaysCurr = m.bFrGainsPrev = m.bFrGainsCurr = nil;
        m.bHfAttenDb = m.bFrHfAttenDb = nil;
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
    m.havePrev = false;
    ready = false;
}
