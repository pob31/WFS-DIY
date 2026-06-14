/*
    MetalIrBackend implementation (Objective-C++).

    The MSL kernel source lives in MetalIrKernels.h as a string literal,
    compiled at prepare() time into one pipeline state (ir_fdl_mac). All the
    backend-shared host work (IR staging, progressive segment FFTs, input
    FFTs, overlap-add tails) lives in IrConvHostState; this file only owns
    the Metal buffers and the dispatch.

    Buffer model (all shared storage — host FFT writes / kernel reads with
    no explicit copies on Apple Silicon):
        bIrSpectra  [segCapacity][fftLen]              IR spectra, host-built
        bInSpectra  [numNodes][segCapacity][fftLen]    input spectrum ring
        bOutSpectra [numNodes][fftLen]                 accumulated products
    All "spectra" are blockSize packed float2 bins = fftLen floats
    (IrHostFft packing, bin 0 = DC + Nyquist).
*/

#include "MetalIrBackend.h"

#if WFS_GPU_NATIVE

#include "MetalIrKernels.h"
#include "IrConvHostState.h"

#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

#include <algorithm>
#include <chrono>
#include <cstring>

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

// Progressive IR loader budget: 64 segment FFTs (512-point) cost well under
// 0.2 ms on the pump thread, so a 10 s IR (1875 segments at 256/48 k) is
// fully loaded within ~30 launches (~160 ms) without risking the deadline.
constexpr int kIrSegmentsPerLaunch = 64;
} // namespace

struct MetalIrBackend::Impl
{
    id<MTLDevice> device = nil;
    id<MTLCommandQueue> queue = nil;
    id<MTLComputePipelineState> psoMac = nil;

    id<MTLBuffer> bParams = nil;
    id<MTLBuffer> bIrSpectra = nil;
    id<MTLBuffer> bInSpectra = nil;
    id<MTLBuffer> bOutSpectra = nil;

    int numNodes = 0, blockSize = 0, fftLen = 0;
    double sampleRate = 0.0;

    IrConvHostState host;
};

MetalIrBackend::MetalIrBackend() : impl (std::make_unique<Impl>()) {}
MetalIrBackend::~MetalIrBackend() { release(); }

bool MetalIrBackend::prepare (int numNodes, int blockSize,
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
                                  [NSString stringWithUTF8String: kIrFdlMacKernelSource]
                                                    options: nil
                                                      error: &err];
        if (lib == nil)
        {
            lastError = std::string ("IR kernel compile failed: ")
                        + (err ? err.localizedDescription.UTF8String : "?");
            return false;
        }

        m.psoMac = [m.device newComputePipelineStateWithFunction:
                        [lib newFunctionWithName: @"ir_fdl_mac"]
                                                           error: &err];
        if (m.psoMac == nil)
        {
            lastError = std::string ("Pipeline state failed (ir_fdl_mac): ")
                        + (err ? err.localizedDescription.UTF8String : "?");
            return false;
        }

        if ((NSUInteger) blockSize > m.psoMac.maxTotalThreadsPerThreadgroup)
        {
            lastError = "Block size exceeds Metal threadgroup limit";
            release();
            return false;
        }

        m.queue = [m.device newCommandQueue];

        m.numNodes = m.host.getNumNodes();
        m.blockSize = m.host.getBlockSize();
        m.fftLen = m.host.getFftLen();
        m.sampleRate = sampleRate;

        const size_t segCap = (size_t) m.host.getSegCapacity();
        auto shared = MTLResourceStorageModeShared;
        auto mkBuf = [&](size_t floats) {
            return [m.device newBufferWithLength: floats * sizeof (float) options: shared];
        };

        m.bParams = [m.device newBufferWithLength: sizeof (IrParamsGpu) options: shared];
        m.bIrSpectra = mkBuf (segCap * (size_t) m.fftLen);
        m.bInSpectra = mkBuf ((size_t) m.numNodes * segCap * (size_t) m.fftLen);
        m.bOutSpectra = mkBuf ((size_t) m.numNodes * (size_t) m.fftLen);

        if (! (m.bParams && m.bIrSpectra && m.bInSpectra && m.bOutSpectra))
        {
            lastError = "Metal buffer allocation failed";
            release();
            return false;
        }

        // Unwritten ring slots must read as silence history.
        memset (m.bInSpectra.contents, 0, m.bInSpectra.length);
    }

    ready = true;
    lastError.clear();
    return true;
}

void MetalIrBackend::stageIr (const float* monoIr, int numSamples)
{
    impl->host.stageIr (monoIr, numSamples);
}

void MetalIrBackend::requestReset() noexcept
{
    impl->host.requestReset();
}

int MetalIrBackend::getSegmentsLoaded() const noexcept { return impl->host.getSegmentsLoaded(); }
int MetalIrBackend::getSegmentsTotal() const noexcept  { return impl->host.getSegmentsTotal(); }

bool MetalIrBackend::processBlock (const float* const* inputs, float* const* outputs)
{
    if (! ready)
        return false;

    auto& m = *impl;

    @autoreleasepool
    {
        const auto t0 = std::chrono::steady_clock::now();

        if (m.host.consumeResetRequest())
            memset (m.bInSpectra.contents, 0, m.bInSpectra.length);

        m.host.consumeStagedIr();
        m.host.loadMoreSegments ((float*) m.bIrSpectra.contents, kIrSegmentsPerLaunch);

        // Newest input spectra into this launch's ring slot (host FFT writes
        // straight into the shared buffer).
        m.host.advanceRing();
        const size_t segCap = (size_t) m.host.getSegCapacity();
        const size_t head = (size_t) m.host.getRingHead();
        float* inBase = (float*) m.bInSpectra.contents;
        for (int node = 0; node < m.numNodes; ++node)
            m.host.transformInput (inputs[node],
                                   inBase + ((size_t) node * segCap + head) * (size_t) m.fftLen);

        // No IR yet (or still loading segment 0): wet output is silence, but
        // the ring keeps accumulating history so the tail is correct the
        // moment segments come online.
        if (m.host.getSegmentsLoaded() == 0)
        {
            for (int node = 0; node < m.numNodes; ++node)
                if (outputs[node] != nullptr)
                    memset (outputs[node], 0, (size_t) m.blockSize * sizeof (float));
            lastLaunchMs = std::chrono::duration<double, std::milli> (
                               std::chrono::steady_clock::now() - t0).count();
            return true;
        }

        IrParamsGpu p { (uint32_t) m.numNodes, (uint32_t) m.blockSize,
                        (uint32_t) segCap, (uint32_t) m.host.getSegmentsLoaded(),
                        (uint32_t) head };
        memcpy (m.bParams.contents, &p, sizeof (p));

        id<MTLCommandBuffer> cb = [m.queue commandBuffer];
        id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];

        [enc setComputePipelineState: m.psoMac];
        [enc setBuffer: m.bParams offset: 0 atIndex: 0];
        [enc setBuffer: m.bIrSpectra offset: 0 atIndex: 1];
        [enc setBuffer: m.bInSpectra offset: 0 atIndex: 2];
        [enc setBuffer: m.bOutSpectra offset: 0 atIndex: 3];
        [enc dispatchThreadgroups: MTLSizeMake ((NSUInteger) m.numNodes, 1, 1)
            threadsPerThreadgroup: MTLSizeMake ((NSUInteger) m.blockSize, 1, 1)];

        [enc endEncoding];
        [cb commit];
        [cb waitUntilCompleted];

        if (cb.status == MTLCommandBufferStatusError)
        {
            lastError = std::string ("GPU IR launch failed: ")
                        + (cb.error ? cb.error.localizedDescription.UTF8String : "?");
            ready = false;
            return false;
        }

        // Accumulated spectra -> inverse FFT + overlap-add per node.
        const float* outBase = (const float*) m.bOutSpectra.contents;
        for (int node = 0; node < m.numNodes; ++node)
            if (outputs[node] != nullptr)
                m.host.produceOutput (node,
                                      outBase + (size_t) node * (size_t) m.fftLen,
                                      outputs[node]);

        lastLaunchMs = std::chrono::duration<double, std::milli> (
                           std::chrono::steady_clock::now() - t0).count();
    }
    return true;
}

void MetalIrBackend::release() noexcept
{
    auto& m = *impl;
    @autoreleasepool
    {
        m.bParams = m.bIrSpectra = m.bInSpectra = m.bOutSpectra = nil;
        m.psoMac = nil;
        m.queue = nil;
        m.device = nil;
    }
    ready = false;
}

#endif // WFS_GPU_NATIVE
