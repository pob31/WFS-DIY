/*
    MetalSdnBackend implementation (Objective-C++).

    The MSL kernel source lives in MetalSdnKernels.h as a string literal,
    compiled at prepare() into one pipeline state (sdn_process). The shared host
    math (geometry->delays, decay gains, crossfade state) lives in SdnHostConfig;
    this file owns the Metal buffers and the dispatch.

    All buffers use shared storage (Apple Silicon unified memory). Persistent
    state (per-path delay lines, decay-filter states, diffuser rings + write
    positions, tone/DC state) lives in device buffers across launches; reset
    zeroes them and the shared ring write head. The per-path delays/gains and the
    crossfade mix are uploaded whenever a param/geometry change or an in-flight
    crossfade makes them stale.

    Dispatch: ONE threadgroup x numNodes threads (one per node), per-sample
    lockstep with a device-memory barrier — the network couples nodes within a
    block, so the whole network must live in one threadgroup.
*/

#include "MetalSdnBackend.h"

#if WFS_GPU_NATIVE

#include "MetalSdnKernels.h"
#include "SdnHostConfig.h"

#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <mutex>
#include <vector>

namespace spatcore::gpu {

namespace
{
// Host mirror of the kernel-side SdnParams - layout must match exactly.
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

struct MetalSdnBackend::Impl
{
    id<MTLDevice> device = nil;
    id<MTLCommandQueue> queue = nil;
    id<MTLComputePipelineState> pso = nil;

    id<MTLBuffer> bParams = nil;
    id<MTLBuffer> bInputs = nil, bOutputs = nil;
    id<MTLBuffer> bDelayLines = nil;
    id<MTLBuffer> bDelayLength = nil, bTargetDelayLength = nil, bCrossfadeMix = nil;
    id<MTLBuffer> bGainLow = nil, bGainMid = nil, bGainHigh = nil;
    id<MTLBuffer> bDecayLowState = nil, bDecayHighState = nil;
    id<MTLBuffer> bDiffuserDelays = nil, bDiffRings = nil, bDiffWritePos = nil;
    id<MTLBuffer> bToneState = nil, bDcState = nil;

    int numNodes = 0, numPaths = 0, blockSize = 0;
    double sampleRate = 0.0;
    uint32_t ringWritePos = 0;
    bool needUpload = true;   // re-upload the dynamic per-path buffers next launch

    SdnHostConfig cfg;        // pump-thread owned after prepare

    // Parameter staging (engine thread -> pump thread). The members always hold
    // the latest set values; paramsDirty just flags a change to consume.
    std::mutex paramMutex;
    std::atomic<bool> paramsDirty { false };
    float pRt60 = 1.5f, pLowMult = 1.3f, pHighMult = 0.5f;
    float pXLow = 200.0f, pXHigh = 4000.0f, pDiffusion = 0.5f, pSdnScale = 1.0f;

    // Geometry staging (engine thread -> pump thread).
    std::mutex geomMutex;
    std::atomic<bool> geometryDirty { false };
    std::vector<float> stagedXyz;     // count * 3
    int stagedCount = 0;
    std::vector<SdnHostConfig::NodePos> posScratch;

    std::atomic<bool> resetRequested { false };
};

// deviceIndex accepted for API uniformity but ignored on macOS (system default device).
MetalSdnBackend::MetalSdnBackend (int deviceIndex) : impl (std::make_unique<Impl>()) { (void) deviceIndex; }
MetalSdnBackend::~MetalSdnBackend() { release(); }

bool MetalSdnBackend::prepare (int numNodes, int blockSize, double sampleRate)
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

    @autoreleasepool
    {
        m.device = MTLCreateSystemDefaultDevice();
        if (m.device == nil)
        {
            lastError = "No Metal device available";
            return false;
        }
        deviceName = std::string (m.device.name.UTF8String) + " (Metal)";

        NSError* err = nil;
        id<MTLLibrary> lib = [m.device newLibraryWithSource:
                                  [NSString stringWithUTF8String: kSdnProcessKernelSource]
                                                    options: nil
                                                      error: &err];
        if (lib == nil)
        {
            lastError = std::string ("SDN kernel compile failed: ")
                        + (err ? err.localizedDescription.UTF8String : "?");
            return false;
        }

        m.pso = [m.device newComputePipelineStateWithFunction:
                     [lib newFunctionWithName: @"sdn_process"]
                                                        error: &err];
        if (m.pso == nil)
        {
            lastError = std::string ("Pipeline state failed (sdn_process): ")
                        + (err ? err.localizedDescription.UTF8String : "?");
            return false;
        }

        if (m.pso.maxTotalThreadsPerThreadgroup < (NSUInteger) m.numNodes)
        {
            lastError = "Threadgroup too small for the SDN node count";
            release();
            return false;
        }

        m.queue = [m.device newCommandQueue];

        const int N = m.numNodes;
        const int P = std::max (1, m.numPaths);
        const int D = SdnHostConfig::NUM_DIFFUSERS;
        const size_t maxDelay = (size_t) SdnHostConfig::MAX_DELAY_SAMPLES;
        const size_t maxDiff  = (size_t) m.cfg.maxDiffLen;

        auto shared = MTLResourceStorageModeShared;
        auto mkF = [&](size_t floats) { return [m.device newBufferWithLength: floats * sizeof (float) options: shared]; };
        auto mkI = [&](size_t ints)   { return [m.device newBufferWithLength: ints  * sizeof (int)   options: shared]; };

        m.bParams  = [m.device newBufferWithLength: sizeof (SdnParamsGpu) options: shared];
        m.bInputs  = mkF ((size_t) N * m.blockSize);
        m.bOutputs = mkF ((size_t) N * m.blockSize);

        m.bDelayLines        = mkF ((size_t) P * maxDelay);
        m.bDelayLength       = mkI ((size_t) P);
        m.bTargetDelayLength = mkI ((size_t) P);
        m.bCrossfadeMix      = mkF ((size_t) P);
        m.bGainLow  = mkF ((size_t) P);
        m.bGainMid  = mkF ((size_t) P);
        m.bGainHigh = mkF ((size_t) P);
        m.bDecayLowState  = mkF ((size_t) P);
        m.bDecayHighState = mkF ((size_t) P);

        m.bDiffuserDelays = mkI ((size_t) N * D);
        m.bDiffRings      = mkF ((size_t) N * D * maxDiff);
        m.bDiffWritePos   = mkI ((size_t) N * D);
        m.bToneState = mkF ((size_t) N);
        m.bDcState   = mkF ((size_t) N * 2);

        if (! (m.bParams && m.bInputs && m.bOutputs && m.bDelayLines && m.bDelayLength
               && m.bTargetDelayLength && m.bCrossfadeMix && m.bGainLow && m.bGainMid
               && m.bGainHigh && m.bDecayLowState && m.bDecayHighState && m.bDiffuserDelays
               && m.bDiffRings && m.bDiffWritePos && m.bToneState && m.bDcState))
        {
            lastError = "Metal buffer allocation failed";
            release();
            return false;
        }

        // Upload the static config + initial dynamic config.
        memcpy (m.bDiffuserDelays.contents, m.cfg.diffuserDelays.data(), (size_t) N * D * sizeof (int));
        memcpy (m.bDelayLength.contents,       m.cfg.delayLength.data(),       (size_t) P * sizeof (int));
        memcpy (m.bTargetDelayLength.contents, m.cfg.targetDelayLength.data(), (size_t) P * sizeof (int));
        memcpy (m.bCrossfadeMix.contents,      m.cfg.crossfadeMix.data(),      (size_t) P * sizeof (float));
        memcpy (m.bGainLow.contents,  m.cfg.gainLow.data(),  (size_t) P * sizeof (float));
        memcpy (m.bGainMid.contents,  m.cfg.gainMid.data(),  (size_t) P * sizeof (float));
        memcpy (m.bGainHigh.contents, m.cfg.gainHigh.data(), (size_t) P * sizeof (float));

        // Zero the persistent state.
        memset (m.bDelayLines.contents,    0, m.bDelayLines.length);
        memset (m.bDecayLowState.contents,  0, m.bDecayLowState.length);
        memset (m.bDecayHighState.contents, 0, m.bDecayHighState.length);
        memset (m.bDiffRings.contents,    0, m.bDiffRings.length);
        memset (m.bDiffWritePos.contents, 0, m.bDiffWritePos.length);
        memset (m.bToneState.contents, 0, m.bToneState.length);
        memset (m.bDcState.contents,   0, m.bDcState.length);

        SdnParamsGpu p { (uint32_t) N, (uint32_t) m.numPaths, (uint32_t) m.blockSize,
                         (uint32_t) SdnHostConfig::MAX_DELAY_SAMPLES, 0u,
                         (uint32_t) m.cfg.maxDiffLen,
                         m.cfg.diffusionCoeff, m.cfg.toneCoeff, m.cfg.lowCoeff,
                         m.cfg.highCoeff, SdnHostConfig::DC_POLE, m.cfg.sdnOutputGain,
                         m.cfg.inputDistribution, m.cfg.crossfadeRate };
        memcpy (m.bParams.contents, &p, sizeof (p));
    }

    ready = true;
    lastError.clear();
    return true;
}

void MetalSdnBackend::setGeometry (const float* xyz, int count) noexcept
{
    auto& m = *impl;
    std::lock_guard<std::mutex> lock (m.geomMutex);
    m.stagedXyz.assign (xyz, xyz + (size_t) std::max (0, count) * 3);
    m.stagedCount = std::max (0, count);
    m.geometryDirty.store (true, std::memory_order_release);
}

void MetalSdnBackend::setParameters (float rt60, float rt60LowMult, float rt60HighMult,
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

void MetalSdnBackend::requestReset() noexcept
{
    impl->resetRequested.store (true, std::memory_order_release);
}

bool MetalSdnBackend::processBlock (const float* const* inputs, float* const* outputs)
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
                    memcpy (outputs[n], inputs[n], (size_t) m.blockSize * sizeof (float));
                else
                    memset (outputs[n], 0, (size_t) m.blockSize * sizeof (float));
            }
        return true;
    }

    const int P = std::max (1, m.numPaths);

    @autoreleasepool
    {
        const auto t0 = std::chrono::steady_clock::now();

        if (m.resetRequested.exchange (false, std::memory_order_acq_rel))
        {
            memset (m.bDelayLines.contents,    0, m.bDelayLines.length);
            memset (m.bDecayLowState.contents,  0, m.bDecayLowState.length);
            memset (m.bDecayHighState.contents, 0, m.bDecayHighState.length);
            memset (m.bDiffRings.contents,    0, m.bDiffRings.length);
            memset (m.bDiffWritePos.contents, 0, m.bDiffWritePos.length);
            memset (m.bToneState.contents, 0, m.bToneState.length);
            memset (m.bDcState.contents,   0, m.bDcState.length);
            m.ringWritePos = 0;
        }

        // Consume staged geometry/params: recompute the config (CPU order —
        // delays first, then decay gains, then diffusion) and flag an upload.
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

        // Upload the dynamic per-path buffers if stale (recompute or an in-flight
        // crossfade), plus the scalar coefficients in the param block.
        if (m.needUpload)
        {
            memcpy (m.bDelayLength.contents,       m.cfg.delayLength.data(),       (size_t) P * sizeof (int));
            memcpy (m.bTargetDelayLength.contents, m.cfg.targetDelayLength.data(), (size_t) P * sizeof (int));
            memcpy (m.bCrossfadeMix.contents,      m.cfg.crossfadeMix.data(),      (size_t) P * sizeof (float));
            memcpy (m.bGainLow.contents,  m.cfg.gainLow.data(),  (size_t) P * sizeof (float));
            memcpy (m.bGainMid.contents,  m.cfg.gainMid.data(),  (size_t) P * sizeof (float));
            memcpy (m.bGainHigh.contents, m.cfg.gainHigh.data(), (size_t) P * sizeof (float));

            auto* pp = (SdnParamsGpu*) m.bParams.contents;
            pp->diffusionCoeff = m.cfg.diffusionCoeff;
            pp->lowCoeff = m.cfg.lowCoeff;
            pp->highCoeff = m.cfg.highCoeff;
            pp->sdnOutputGain = m.cfg.sdnOutputGain;
            pp->crossfadeRate = m.cfg.crossfadeRate;
            m.needUpload = false;
        }

        // Inputs -> flat shared buffer (silence for missing channels).
        float* inFlat = (float*) m.bInputs.contents;
        for (int n = 0; n < N; ++n)
        {
            if (inputs[n] != nullptr)
                memcpy (inFlat + (size_t) n * m.blockSize, inputs[n], (size_t) m.blockSize * sizeof (float));
            else
                memset (inFlat + (size_t) n * m.blockSize, 0, (size_t) m.blockSize * sizeof (float));
        }

        // Block-start write head.
        ((SdnParamsGpu*) m.bParams.contents)->ringWritePos = m.ringWritePos;

        id<MTLCommandBuffer> cb = [m.queue commandBuffer];
        id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];

        [enc setComputePipelineState: m.pso];
        [enc setBuffer: m.bParams            offset: 0 atIndex: 0];
        [enc setBuffer: m.bInputs            offset: 0 atIndex: 1];
        [enc setBuffer: m.bOutputs           offset: 0 atIndex: 2];
        [enc setBuffer: m.bDelayLines        offset: 0 atIndex: 3];
        [enc setBuffer: m.bDelayLength       offset: 0 atIndex: 4];
        [enc setBuffer: m.bTargetDelayLength offset: 0 atIndex: 5];
        [enc setBuffer: m.bCrossfadeMix      offset: 0 atIndex: 6];
        [enc setBuffer: m.bGainLow           offset: 0 atIndex: 7];
        [enc setBuffer: m.bGainMid           offset: 0 atIndex: 8];
        [enc setBuffer: m.bGainHigh          offset: 0 atIndex: 9];
        [enc setBuffer: m.bDecayLowState     offset: 0 atIndex: 10];
        [enc setBuffer: m.bDecayHighState    offset: 0 atIndex: 11];
        [enc setBuffer: m.bDiffuserDelays    offset: 0 atIndex: 12];
        [enc setBuffer: m.bDiffRings         offset: 0 atIndex: 13];
        [enc setBuffer: m.bDiffWritePos      offset: 0 atIndex: 14];
        [enc setBuffer: m.bToneState         offset: 0 atIndex: 15];
        [enc setBuffer: m.bDcState           offset: 0 atIndex: 16];

        [enc dispatchThreadgroups: MTLSizeMake (1, 1, 1)
            threadsPerThreadgroup: MTLSizeMake ((NSUInteger) N, 1, 1)];

        [enc endEncoding];
        [cb commit];
        [cb waitUntilCompleted];

        if (cb.status == MTLCommandBufferStatusError)
        {
            lastError = std::string ("GPU SDN launch failed: ")
                        + (cb.error ? cb.error.localizedDescription.UTF8String : "?");
            ready = false;
            return false;
        }

        const float* outFlat = (const float*) m.bOutputs.contents;
        for (int n = 0; n < N; ++n)
            if (outputs[n] != nullptr)
                memcpy (outputs[n], outFlat + (size_t) n * m.blockSize, (size_t) m.blockSize * sizeof (float));

        // Advance the crossfade (host-side, like the CPU post-block loop) and the
        // shared ring write head for the next launch.
        if (m.cfg.advanceCrossfades (m.blockSize))
            m.needUpload = true;
        m.ringWritePos = (m.ringWritePos + (uint32_t) m.blockSize)
                         % (uint32_t) SdnHostConfig::MAX_DELAY_SAMPLES;

        lastLaunchMs = std::chrono::duration<double, std::milli> (
                           std::chrono::steady_clock::now() - t0).count();
    }
    return true;
}

void MetalSdnBackend::release() noexcept
{
    auto& m = *impl;
    @autoreleasepool
    {
        m.bParams = m.bInputs = m.bOutputs = nil;
        m.bDelayLines = m.bDelayLength = m.bTargetDelayLength = m.bCrossfadeMix = nil;
        m.bGainLow = m.bGainMid = m.bGainHigh = nil;
        m.bDecayLowState = m.bDecayHighState = nil;
        m.bDiffuserDelays = m.bDiffRings = m.bDiffWritePos = nil;
        m.bToneState = m.bDcState = nil;
        m.pso = nil;
        m.queue = nil;
        m.device = nil;
    }
    ready = false;
}

} // namespace spatcore::gpu

#endif // WFS_GPU_NATIVE
