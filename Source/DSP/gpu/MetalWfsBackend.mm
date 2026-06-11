/*
    MetalWfsBackend implementation (Objective-C++).

    The MSL kernel source lives in MetalWfsKernels.h as a string literal so
    the app needs no resource plumbing; it is compiled at prepare() time
    (~10 ms once) and cached in the pipeline state object.
*/

#include "MetalWfsBackend.h"
#include "MetalWfsKernels.h"

#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

#include <algorithm>
#include <chrono>
#include <cstring>
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

struct MetalWfsBackend::Impl
{
    id<MTLDevice> device = nil;
    id<MTLCommandQueue> queue = nil;
    id<MTLComputePipelineState> pso = nil;

    id<MTLBuffer> bParams = nil;
    id<MTLBuffer> bIn = nil;
    id<MTLBuffer> bOut = nil;
    id<MTLBuffer> bRing = nil;
    id<MTLBuffer> bDelaysPrev = nil, bDelaysCurr = nil;
    id<MTLBuffer> bGainsPrev = nil, bGainsCurr = nil;

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

    NSUInteger threadsPerGroup = 256;
};

MetalWfsBackend::MetalWfsBackend() : impl (std::make_unique<Impl>()) {}
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

        m.pso = [m.device newComputePipelineStateWithFunction:
                     [lib newFunctionWithName: @"wfs_delay_sum"]
                                                        error: &err];
        if (m.pso == nil)
        {
            lastError = std::string ("Pipeline state failed: ")
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

        const uint32_t matrix = (uint32_t) (m.numIn * m.numOut);
        auto shared = MTLResourceStorageModeShared;
        auto mkBuf = [&](size_t floats) {
            return [m.device newBufferWithLength: floats * sizeof (float) options: shared];
        };

        m.bParams = [m.device newBufferWithLength: sizeof (WfsParamsGpu) options: shared];
        m.bIn = mkBuf ((size_t) m.numIn * m.blockSize);
        m.bOut = mkBuf ((size_t) m.numOut * m.blockSize);
        m.bRing = mkBuf ((size_t) m.numIn * m.ringCapacity);
        m.bDelaysPrev = mkBuf (matrix);
        m.bDelaysCurr = mkBuf (matrix);
        m.bGainsPrev = mkBuf (matrix);
        m.bGainsCurr = mkBuf (matrix);

        if (! (m.bParams && m.bIn && m.bOut && m.bRing && m.bDelaysPrev
               && m.bDelaysCurr && m.bGainsPrev && m.bGainsCurr))
        {
            lastError = "Metal buffer allocation failed";
            release();
            return false;
        }

        memset (m.bRing.contents, 0, m.bRing.length);
        m.delaysPrevSamples.assign (matrix, 0.0f);
        m.gainsPrev.assign (matrix, 0.0f);

        m.threadsPerGroup = std::min<NSUInteger> (256, m.pso.maxTotalThreadsPerThreadgroup);
    }

    ready = true;
    lastError.clear();
    return true;
}

void MetalWfsBackend::setMatrixPointers (const float* delaysMsPtr, const float* gainsPtr) noexcept
{
    impl->delaysMs = delaysMsPtr;
    impl->gains = gainsPtr;
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
        if (! m.havePrev)
        {
            memcpy (m.delaysPrevSamples.data(), dCurr, matrix * sizeof (float));
            memcpy (m.gainsPrev.data(), gCurr, matrix * sizeof (float));
            m.havePrev = true;
        }
        memcpy (m.bDelaysPrev.contents, m.delaysPrevSamples.data(), matrix * sizeof (float));
        memcpy (m.bGainsPrev.contents, m.gainsPrev.data(), matrix * sizeof (float));

        // Input channels -> flat shared buffer
        float* inFlat = (float*) m.bIn.contents;
        for (int ch = 0; ch < m.numIn; ++ch)
        {
            if (inputs[ch] != nullptr)
                memcpy (inFlat + (size_t) ch * m.blockSize, inputs[ch], (size_t) m.blockSize * sizeof (float));
            else
                memset (inFlat + (size_t) ch * m.blockSize, 0, (size_t) m.blockSize * sizeof (float));
        }

        WfsParamsGpu p { (uint32_t) m.numIn, (uint32_t) m.numOut, (uint32_t) m.blockSize,
                         m.ringCapacity, m.ringWritePos, m.ringValid };
        memcpy (m.bParams.contents, &p, sizeof (p));

        id<MTLCommandBuffer> cb = [m.queue commandBuffer];
        id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];
        [enc setComputePipelineState: m.pso];
        [enc setBuffer: m.bParams offset: 0 atIndex: 0];
        [enc setBuffer: m.bIn offset: 0 atIndex: 1];
        [enc setBuffer: m.bOut offset: 0 atIndex: 2];
        [enc setBuffer: m.bRing offset: 0 atIndex: 3];
        [enc setBuffer: m.bDelaysPrev offset: 0 atIndex: 4];
        [enc setBuffer: m.bDelaysCurr offset: 0 atIndex: 5];
        [enc setBuffer: m.bGainsPrev offset: 0 atIndex: 6];
        [enc setBuffer: m.bGainsCurr offset: 0 atIndex: 7];
        [enc dispatchThreadgroups: MTLSizeMake ((NSUInteger) (m.numOut + m.numIn), 1, 1)
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
    m.ringWritePos = 0;
    m.ringValid = 0;
    m.havePrev = false;
}

void MetalWfsBackend::release() noexcept
{
    auto& m = *impl;
    @autoreleasepool
    {
        m.bParams = m.bIn = m.bOut = m.bRing = nil;
        m.bDelaysPrev = m.bDelaysCurr = m.bGainsPrev = m.bGainsCurr = nil;
        m.pso = nil;
        m.queue = nil;
        m.device = nil;
    }
    m.delaysMs = nullptr;
    m.gains = nullptr;
    m.havePrev = false;
    ready = false;
}
