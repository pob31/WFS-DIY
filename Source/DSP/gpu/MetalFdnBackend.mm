/*
    MetalFdnBackend implementation (Objective-C++).

    The MSL kernel source lives in MetalFdnKernels.h as a string literal,
    compiled at prepare() into one pipeline state (fdn_process). The shared
    host math (static per-node config + per-block coefficients) lives in
    FdnHostConfig; this file owns the Metal buffers and the dispatch.

    All buffers use shared storage (Apple Silicon unified memory) so the host
    can upload config/coefficients and the kernel reads them with no copies.
    Persistent state (delay rings, filter states, write positions) lives in
    device buffers across launches; reset zeroes them.

    Dispatch: numNodes threadgroups x 16 threads (one per delay line).
*/

#include "MetalFdnBackend.h"

#if WFS_GPU_NATIVE

#include "MetalFdnKernels.h"
#include "FdnHostConfig.h"

#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <mutex>

namespace
{
// Host mirror of the kernel-side FdnParams - layout must match exactly.
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

struct MetalFdnBackend::Impl
{
    id<MTLDevice> device = nil;
    id<MTLCommandQueue> queue = nil;
    id<MTLComputePipelineState> pso = nil;

    id<MTLBuffer> bParams = nil;
    id<MTLBuffer> bInputs = nil, bOutputs = nil;
    id<MTLBuffer> bDelayLengths = nil, bDiffuserDelays = nil, bFbApDelays = nil;
    id<MTLBuffer> bNodeTapSigns = nil, bInputGains = nil;
    id<MTLBuffer> bGainLow = nil, bGainMid = nil, bGainHigh = nil;
    id<MTLBuffer> bDelayRings = nil, bDelayWritePos = nil;
    id<MTLBuffer> bDiffRings = nil, bDiffWritePos = nil;
    id<MTLBuffer> bFbApRings = nil, bFbApWritePos = nil;
    id<MTLBuffer> bDecayLowState = nil, bDecayHighState = nil;
    id<MTLBuffer> bToneState = nil, bDcState = nil;

    int numNodes = 0, blockSize = 0;
    double sampleRate = 0.0;

    FdnHostConfig cfg;                  // pump-thread owned after prepare

    // Parameter staging (engine thread -> pump thread).
    std::mutex paramMutex;
    std::atomic<bool> paramsDirty { false };
    float pRt60 = 1.5f, pLowMult = 1.3f, pHighMult = 0.5f;
    float pXLow = 200.0f, pXHigh = 4000.0f, pDiffusion = 0.5f;

    std::atomic<bool> resetRequested { false };
};

// deviceIndex accepted for API uniformity but ignored on macOS (system default device).
MetalFdnBackend::MetalFdnBackend (int deviceIndex) : impl (std::make_unique<Impl>()) { (void) deviceIndex; }
MetalFdnBackend::~MetalFdnBackend() { release(); }

bool MetalFdnBackend::prepare (int numNodes, int blockSize,
                               double sampleRate, float fdnSize)
{
    release();
    auto& m = *impl;

    m.numNodes = std::max (1, numNodes);
    m.blockSize = std::max (1, blockSize);
    m.sampleRate = sampleRate;
    m.cfg.prepare (m.numNodes, sampleRate, fdnSize);

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
                                  [NSString stringWithUTF8String: kFdnProcessKernelSource]
                                                    options: nil
                                                      error: &err];
        if (lib == nil)
        {
            lastError = std::string ("FDN kernel compile failed: ")
                        + (err ? err.localizedDescription.UTF8String : "?");
            return false;
        }

        m.pso = [m.device newComputePipelineStateWithFunction:
                     [lib newFunctionWithName: @"fdn_process"]
                                                        error: &err];
        if (m.pso == nil)
        {
            lastError = std::string ("Pipeline state failed (fdn_process): ")
                        + (err ? err.localizedDescription.UTF8String : "?");
            return false;
        }

        if (m.pso.maxTotalThreadsPerThreadgroup < (NSUInteger) FdnHostConfig::NUM_LINES)
        {
            lastError = "Threadgroup too small for 16 FDN lines";
            release();
            return false;
        }

        m.queue = [m.device newCommandQueue];

        const int N = m.numNodes;
        const int L = FdnHostConfig::NUM_LINES;
        const int D = FdnHostConfig::NUM_DIFFUSERS;
        const size_t maxDelayLen = (size_t) m.cfg.maxDelayLen;
        const size_t maxDiffLen  = (size_t) m.cfg.maxDiffLen;
        const size_t maxFbApLen  = (size_t) m.cfg.maxFbApLen;

        auto shared = MTLResourceStorageModeShared;
        auto mkF = [&](size_t floats) { return [m.device newBufferWithLength: floats * sizeof (float) options: shared]; };
        auto mkI = [&](size_t ints)   { return [m.device newBufferWithLength: ints  * sizeof (int)   options: shared]; };

        m.bParams = [m.device newBufferWithLength: sizeof (FdnParamsGpu) options: shared];
        m.bInputs  = mkF ((size_t) N * m.blockSize);
        m.bOutputs = mkF ((size_t) N * m.blockSize);

        m.bDelayLengths   = mkI ((size_t) N * L);
        m.bDiffuserDelays = mkI ((size_t) N * D);
        m.bFbApDelays     = mkI ((size_t) N * L);
        m.bNodeTapSigns   = mkF ((size_t) N * L);
        m.bInputGains     = mkF ((size_t) L);
        m.bGainLow  = mkF ((size_t) N * L);
        m.bGainMid  = mkF ((size_t) N * L);
        m.bGainHigh = mkF ((size_t) N * L);

        m.bDelayRings    = mkF ((size_t) N * L * maxDelayLen);
        m.bDelayWritePos = mkI ((size_t) N * L);
        m.bDiffRings     = mkF ((size_t) N * D * maxDiffLen);
        m.bDiffWritePos  = mkI ((size_t) N * D);
        m.bFbApRings     = mkF ((size_t) N * L * maxFbApLen);
        m.bFbApWritePos  = mkI ((size_t) N * L);
        m.bDecayLowState  = mkF ((size_t) N * L);
        m.bDecayHighState = mkF ((size_t) N * L);
        m.bToneState = mkF ((size_t) N);
        m.bDcState   = mkF ((size_t) N * 2);

        if (! (m.bParams && m.bInputs && m.bOutputs && m.bDelayLengths && m.bDiffuserDelays
               && m.bFbApDelays && m.bNodeTapSigns && m.bInputGains && m.bGainLow && m.bGainMid
               && m.bGainHigh && m.bDelayRings && m.bDelayWritePos && m.bDiffRings && m.bDiffWritePos
               && m.bFbApRings && m.bFbApWritePos && m.bDecayLowState && m.bDecayHighState
               && m.bToneState && m.bDcState))
        {
            lastError = "Metal buffer allocation failed";
            release();
            return false;
        }

        // Upload the static config (immutable for the backend's lifetime).
        memcpy (m.bDelayLengths.contents,   m.cfg.delayLengths.data(),   (size_t) N * L * sizeof (int));
        memcpy (m.bDiffuserDelays.contents, m.cfg.diffuserDelays.data(), (size_t) N * D * sizeof (int));
        memcpy (m.bFbApDelays.contents,     m.cfg.fbApDelays.data(),     (size_t) N * L * sizeof (int));
        memcpy (m.bNodeTapSigns.contents,   m.cfg.nodeTapSigns.data(),   (size_t) N * L * sizeof (float));
        memcpy (m.bInputGains.contents,     FdnHostConfig::getInputGains().data(), (size_t) L * sizeof (float));

        // Initial coefficients (CPU defaults, set by cfg.prepare).
        memcpy (m.bGainLow.contents,  m.cfg.gainLow.data(),  (size_t) N * L * sizeof (float));
        memcpy (m.bGainMid.contents,  m.cfg.gainMid.data(),  (size_t) N * L * sizeof (float));
        memcpy (m.bGainHigh.contents, m.cfg.gainHigh.data(), (size_t) N * L * sizeof (float));

        // Zero the persistent state.
        memset (m.bDelayRings.contents,    0, m.bDelayRings.length);
        memset (m.bDelayWritePos.contents, 0, m.bDelayWritePos.length);
        memset (m.bDiffRings.contents,     0, m.bDiffRings.length);
        memset (m.bDiffWritePos.contents,  0, m.bDiffWritePos.length);
        memset (m.bFbApRings.contents,     0, m.bFbApRings.length);
        memset (m.bFbApWritePos.contents,  0, m.bFbApWritePos.length);
        memset (m.bDecayLowState.contents,  0, m.bDecayLowState.length);
        memset (m.bDecayHighState.contents, 0, m.bDecayHighState.length);
        memset (m.bToneState.contents, 0, m.bToneState.length);
        memset (m.bDcState.contents,   0, m.bDcState.length);

        FdnParamsGpu p { (uint32_t) N, (uint32_t) m.blockSize,
                         (uint32_t) m.cfg.maxDelayLen, (uint32_t) m.cfg.maxDiffLen,
                         (uint32_t) m.cfg.maxFbApLen,
                         m.cfg.toneCoeff, m.cfg.lowCoeff, m.cfg.highCoeff,
                         m.cfg.diffusionCoeff, FdnHostConfig::FEEDBACK_AP_COEFF,
                         FdnHostConfig::DC_POLE, FdnHostConfig::OUTPUT_GAIN };
        memcpy (m.bParams.contents, &p, sizeof (p));
    }

    ready = true;
    lastError.clear();
    return true;
}

void MetalFdnBackend::setParameters (float rt60, float rt60LowMult, float rt60HighMult,
                                     float crossoverLow, float crossoverHigh,
                                     float diffusion) noexcept
{
    auto& m = *impl;
    std::lock_guard<std::mutex> lock (m.paramMutex);
    m.pRt60 = rt60; m.pLowMult = rt60LowMult; m.pHighMult = rt60HighMult;
    m.pXLow = crossoverLow; m.pXHigh = crossoverHigh; m.pDiffusion = diffusion;
    m.paramsDirty.store (true, std::memory_order_release);
}

void MetalFdnBackend::requestReset() noexcept
{
    impl->resetRequested.store (true, std::memory_order_release);
}

bool MetalFdnBackend::processBlock (const float* const* inputs, float* const* outputs)
{
    if (! ready)
        return false;

    auto& m = *impl;
    const int N = m.numNodes;
    const int L = FdnHostConfig::NUM_LINES;

    @autoreleasepool
    {
        const auto t0 = std::chrono::steady_clock::now();

        if (m.resetRequested.exchange (false, std::memory_order_acq_rel))
        {
            memset (m.bDelayRings.contents,    0, m.bDelayRings.length);
            memset (m.bDelayWritePos.contents, 0, m.bDelayWritePos.length);
            memset (m.bDiffRings.contents,     0, m.bDiffRings.length);
            memset (m.bDiffWritePos.contents,  0, m.bDiffWritePos.length);
            memset (m.bFbApRings.contents,     0, m.bFbApRings.length);
            memset (m.bFbApWritePos.contents,  0, m.bFbApWritePos.length);
            memset (m.bDecayLowState.contents,  0, m.bDecayLowState.length);
            memset (m.bDecayHighState.contents, 0, m.bDecayHighState.length);
            memset (m.bToneState.contents, 0, m.bToneState.length);
            memset (m.bDcState.contents,   0, m.bDcState.length);
        }

        // Consume staged parameters: recompute the coefficients and upload.
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

            memcpy (m.bGainLow.contents,  m.cfg.gainLow.data(),  (size_t) N * L * sizeof (float));
            memcpy (m.bGainMid.contents,  m.cfg.gainMid.data(),  (size_t) N * L * sizeof (float));
            memcpy (m.bGainHigh.contents, m.cfg.gainHigh.data(), (size_t) N * L * sizeof (float));

            auto* p = (FdnParamsGpu*) m.bParams.contents;
            p->lowCoeff = m.cfg.lowCoeff;
            p->highCoeff = m.cfg.highCoeff;
            p->diffusionCoeff = m.cfg.diffusionCoeff;
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

        id<MTLCommandBuffer> cb = [m.queue commandBuffer];
        id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];

        [enc setComputePipelineState: m.pso];
        [enc setBuffer: m.bParams         offset: 0 atIndex: 0];
        [enc setBuffer: m.bInputs         offset: 0 atIndex: 1];
        [enc setBuffer: m.bOutputs        offset: 0 atIndex: 2];
        [enc setBuffer: m.bDelayLengths   offset: 0 atIndex: 3];
        [enc setBuffer: m.bDiffuserDelays offset: 0 atIndex: 4];
        [enc setBuffer: m.bFbApDelays     offset: 0 atIndex: 5];
        [enc setBuffer: m.bNodeTapSigns   offset: 0 atIndex: 6];
        [enc setBuffer: m.bInputGains     offset: 0 atIndex: 7];
        [enc setBuffer: m.bGainLow        offset: 0 atIndex: 8];
        [enc setBuffer: m.bGainMid        offset: 0 atIndex: 9];
        [enc setBuffer: m.bGainHigh       offset: 0 atIndex: 10];
        [enc setBuffer: m.bDelayRings     offset: 0 atIndex: 11];
        [enc setBuffer: m.bDelayWritePos  offset: 0 atIndex: 12];
        [enc setBuffer: m.bDiffRings      offset: 0 atIndex: 13];
        [enc setBuffer: m.bDiffWritePos   offset: 0 atIndex: 14];
        [enc setBuffer: m.bFbApRings      offset: 0 atIndex: 15];
        [enc setBuffer: m.bFbApWritePos   offset: 0 atIndex: 16];
        [enc setBuffer: m.bDecayLowState  offset: 0 atIndex: 17];
        [enc setBuffer: m.bDecayHighState offset: 0 atIndex: 18];
        [enc setBuffer: m.bToneState      offset: 0 atIndex: 19];
        [enc setBuffer: m.bDcState        offset: 0 atIndex: 20];

        [enc dispatchThreadgroups: MTLSizeMake ((NSUInteger) N, 1, 1)
            threadsPerThreadgroup: MTLSizeMake ((NSUInteger) L, 1, 1)];

        [enc endEncoding];
        [cb commit];
        [cb waitUntilCompleted];

        if (cb.status == MTLCommandBufferStatusError)
        {
            lastError = std::string ("GPU FDN launch failed: ")
                        + (cb.error ? cb.error.localizedDescription.UTF8String : "?");
            ready = false;
            return false;
        }

        const float* outFlat = (const float*) m.bOutputs.contents;
        for (int n = 0; n < N; ++n)
            if (outputs[n] != nullptr)
                memcpy (outputs[n], outFlat + (size_t) n * m.blockSize, (size_t) m.blockSize * sizeof (float));

        lastLaunchMs = std::chrono::duration<double, std::milli> (
                           std::chrono::steady_clock::now() - t0).count();
    }
    return true;
}

void MetalFdnBackend::release() noexcept
{
    auto& m = *impl;
    @autoreleasepool
    {
        m.bParams = m.bInputs = m.bOutputs = nil;
        m.bDelayLengths = m.bDiffuserDelays = m.bFbApDelays = nil;
        m.bNodeTapSigns = m.bInputGains = nil;
        m.bGainLow = m.bGainMid = m.bGainHigh = nil;
        m.bDelayRings = m.bDelayWritePos = nil;
        m.bDiffRings = m.bDiffWritePos = nil;
        m.bFbApRings = m.bFbApWritePos = nil;
        m.bDecayLowState = m.bDecayHighState = nil;
        m.bToneState = m.bDcState = nil;
        m.pso = nil;
        m.queue = nil;
        m.device = nil;
    }
    ready = false;
}

#endif // WFS_GPU_NATIVE
