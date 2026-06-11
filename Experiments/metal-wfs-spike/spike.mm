//
// Headless harness for the native-Metal WFS delay-and-sum kernel.
//
// Goals (the SDK-independence benchmark):
//   1. CORRECTNESS: bit-near agreement with a CPU reference of the same math
//      across multiple streaming iterations (rings, ramps, history).
//   2. LATENCY: per-dispatch round-trip (commit + waitUntilCompleted) -
//      directly comparable to the GPU Audio SDK's in-callback sync Execute
//      (measured floor 0.8-1.2 ms on this machine).
//   3. PIPELINED THROUGHPUT: commit-without-wait at depth D - comparable to
//      the GpuAsyncPipeline pump round-trip (measured 0.02-0.2 ms).
//   4. SCALING: 8x16 -> 64x128 matrix sizes at block 128.
//
// Build & run:  ./build.sh && ./spike
//

#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

struct WfsParams
{
    uint32_t numInputs;
    uint32_t numOutputs;
    uint32_t bufferLength;
    uint32_t ringCapacity;
    uint32_t ringWritePos;
    uint32_t ringValidSamples;
};

// CPU reference: identical math to the kernel.
static void cpuReference(const WfsParams& p,
                         const std::vector<float>& input,
                         std::vector<float>& output,
                         std::vector<float>& ring,
                         const std::vector<float>& dPrev, const std::vector<float>& dCurr,
                         const std::vector<float>& gPrev, const std::vector<float>& gCurr)
{
    auto fetch = [&](uint32_t inIdx, int off) -> float {
        if (off >= 0)
            return input[inIdx * p.bufferLength + (uint32_t) off];
        if ((uint32_t) (-off) > p.ringValidSamples)
            return 0.0f;
        int idx = (int) p.ringWritePos + off;
        if (idx < 0)
            idx += (int) p.ringCapacity;
        return ring[inIdx * p.ringCapacity + (uint32_t) idx];
    };

    const float invLen = 1.0f / (float) p.bufferLength;
    for (uint32_t out = 0; out < p.numOutputs; ++out)
    {
        for (uint32_t s = 0; s < p.bufferLength; ++s)
        {
            const float t = (float) (s + 1) * invLen;
            float acc = 0.0f;
            for (uint32_t in = 0; in < p.numInputs; ++in)
            {
                const uint32_t m = in * p.numOutputs + out;
                const float gp = gPrev[m], gc = gCurr[m];
                if (gp == 0.0f && gc == 0.0f)
                    continue;
                const float gain = gp + (gc - gp) * t;
                float d = dPrev[m] + (dCurr[m] - dPrev[m]) * t;
                d = std::max(d, 0.0f);
                const uint32_t di = (uint32_t) d;
                const float frac = d - (float) di;
                const int off0 = (int) s - (int) di;
                acc += gain * (fetch(in, off0) * (1.0f - frac) + fetch(in, off0 - 1) * frac);
            }
            output[out * p.bufferLength + s] = acc;
        }
    }
    // append
    for (uint32_t in = 0; in < p.numInputs; ++in)
        for (uint32_t s = 0; s < p.bufferLength; ++s)
            ring[in * p.ringCapacity + (p.ringWritePos + s) % p.ringCapacity] = input[in * p.bufferLength + s];
}

struct Stats
{
    double minMs = 1e9, maxMs = 0, sumMs = 0;
    std::vector<double> all;
    void add(double ms) { minMs = std::min(minMs, ms); maxMs = std::max(maxMs, ms); sumMs += ms; all.push_back(ms); }
    double mean() const { return all.empty() ? 0 : sumMs / all.size(); }
    double pct(double p) const
    {
        if (all.empty()) return 0;
        auto v = all;
        std::sort(v.begin(), v.end());
        return v[std::min(v.size() - 1, (size_t) (p * v.size()))];
    }
};

int main()
{
    @autoreleasepool
    {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) { fprintf(stderr, "No Metal device\n"); return 1; }
        printf("Metal device: %s\n", device.name.UTF8String);

        NSError* err = nil;
        NSString* src = [NSString stringWithContentsOfFile:@"wfs_delay_sum.metal"
                                                  encoding:NSUTF8StringEncoding error:&err];
        if (!src) { fprintf(stderr, "Cannot read kernel source\n"); return 1; }

        id<MTLLibrary> lib = [device newLibraryWithSource:src options:nil error:&err];
        if (!lib) { fprintf(stderr, "Compile failed: %s\n", err.localizedDescription.UTF8String); return 1; }
        id<MTLComputePipelineState> pso =
            [device newComputePipelineStateWithFunction:[lib newFunctionWithName:@"wfs_delay_sum"] error:&err];
        if (!pso) { fprintf(stderr, "PSO failed: %s\n", err.localizedDescription.UTF8String); return 1; }

        id<MTLCommandQueue> queue = [device newCommandQueue];

        const uint32_t blockLen = 128;
        const uint32_t sampleRate = 48000;

        struct Size { uint32_t in, out; };
        const Size sizes[] = { {8, 16}, {16, 32}, {32, 64}, {64, 64}, {64, 128} };

        printf("\nblock=%u @ %u Hz (budget %.2f ms). %s\n", blockLen, sampleRate,
               1000.0 * blockLen / sampleRate,
               "sync = commit+wait per block; pipelined = wait only every 4th");
        printf("%-9s | %-37s | %-29s | %-25s | %s\n", "matrix", "sync dispatch ms (mean/p99/max)",
               "parked-event ms (mean/p99/max)", "pipelined ms (mean/p99)", "correctness");
        printf("---------+---------------------------------------+-------------------------------+---------------------------+------------\n");

        for (auto sz : sizes)
        {
            WfsParams p {};
            p.numInputs = sz.in;
            p.numOutputs = sz.out;
            p.bufferLength = blockLen;
            p.ringCapacity = sampleRate + blockLen; // 1 s max delay
            p.ringWritePos = 0;
            p.ringValidSamples = 0;

            const uint32_t matrix = sz.in * sz.out;

            auto mkBuf = [&](size_t n) {
                return [device newBufferWithLength:n * sizeof(float) options:MTLResourceStorageModeShared];
            };
            id<MTLBuffer> bIn = mkBuf((size_t) sz.in * blockLen);
            id<MTLBuffer> bOut = mkBuf((size_t) sz.out * blockLen);
            id<MTLBuffer> bRing = mkBuf((size_t) sz.in * p.ringCapacity);
            id<MTLBuffer> bDp = mkBuf(matrix), bDc = mkBuf(matrix), bGp = mkBuf(matrix), bGc = mkBuf(matrix);
            id<MTLBuffer> bParams = [device newBufferWithLength:sizeof(WfsParams)
                                                        options:MTLResourceStorageModeShared];
            memset(bRing.contents, 0, bRing.length);

            // Deterministic test matrices: spread of delays (0..200ms) and gains.
            std::mt19937 rng(42);
            std::uniform_real_distribution<float> dDelay(0.0f, 0.2f * sampleRate);
            std::uniform_real_distribution<float> dGain(0.0f, 1.0f);
            std::vector<float> dPrev(matrix), dCurr(matrix), gPrev(matrix), gCurr(matrix);
            for (uint32_t i = 0; i < matrix; ++i)
            {
                dPrev[i] = dDelay(rng);
                dCurr[i] = dPrev[i] + dDelay(rng) * 0.01f; // slow motion
                gPrev[i] = dGain(rng);
                gCurr[i] = dGain(rng);
            }
            memcpy(bDp.contents, dPrev.data(), matrix * 4);
            memcpy(bDc.contents, dCurr.data(), matrix * 4);
            memcpy(bGp.contents, gPrev.data(), matrix * 4);
            memcpy(bGc.contents, gCurr.data(), matrix * 4);

            // CPU mirror state for correctness
            std::vector<float> cpuRing((size_t) sz.in * p.ringCapacity, 0.0f);
            std::vector<float> cpuOut((size_t) sz.out * blockLen, 0.0f);
            std::vector<float> inBlock((size_t) sz.in * blockLen);

            const uint32_t tgCount = sz.out + sz.in;
            const NSUInteger threads = std::min<NSUInteger>(256, pso.maxTotalThreadsPerThreadgroup);

            auto encode = [&](id<MTLCommandBuffer> cb) {
                id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];
                [enc setComputePipelineState:pso];
                [enc setBuffer:bParams offset:0 atIndex:0];
                [enc setBuffer:bIn offset:0 atIndex:1];
                [enc setBuffer:bOut offset:0 atIndex:2];
                [enc setBuffer:bRing offset:0 atIndex:3];
                [enc setBuffer:bDp offset:0 atIndex:4];
                [enc setBuffer:bDc offset:0 atIndex:5];
                [enc setBuffer:bGp offset:0 atIndex:6];
                [enc setBuffer:bGc offset:0 atIndex:7];
                [enc dispatchThreadgroups:MTLSizeMake(tgCount, 1, 1)
                    threadsPerThreadgroup:MTLSizeMake(threads, 1, 1)];
                [enc endEncoding];
            };

            std::uniform_real_distribution<float> dSig(-0.25f, 0.25f);
            auto fillInput = [&](uint32_t iter) {
                for (uint32_t in = 0; in < sz.in; ++in)
                    for (uint32_t s = 0; s < blockLen; ++s)
                        inBlock[in * blockLen + s] =
                            0.2f * std::sin(0.01f * (float) (iter * blockLen + s) * (1.0f + in)) + dSig(rng) * 0.01f;
                memcpy(bIn.contents, inBlock.data(), inBlock.size() * 4);
            };

            auto advanceState = [&]() {
                p.ringWritePos = (p.ringWritePos + blockLen) % p.ringCapacity;
                p.ringValidSamples = std::min<uint32_t>(sampleRate, p.ringValidSamples + blockLen);
            };

            // ---- correctness + warmup: 50 streaming iterations, sync, compare each
            float maxDiff = 0.0f;
            for (uint32_t iter = 0; iter < 50; ++iter)
            {
                fillInput(iter);
                memcpy(bParams.contents, &p, sizeof(p));

                id<MTLCommandBuffer> cb = [queue commandBuffer];
                encode(cb);
                [cb commit];
                [cb waitUntilCompleted];

                cpuReference(p, inBlock, cpuOut, cpuRing, dPrev, dCurr, gPrev, gCurr);

                const float* gpuOut = (const float*) bOut.contents;
                for (size_t i = 0; i < cpuOut.size(); ++i)
                    maxDiff = std::max(maxDiff, std::fabs(gpuOut[i] - cpuOut[i]));

                advanceState();
            }

            // ---- sync latency: 500 iterations, wait each
            Stats sync;
            for (uint32_t iter = 0; iter < 500; ++iter)
            {
                fillInput(50 + iter);
                memcpy(bParams.contents, &p, sizeof(p));
                auto t0 = std::chrono::steady_clock::now();
                id<MTLCommandBuffer> cb = [queue commandBuffer];
                encode(cb);
                [cb commit];
                [cb waitUntilCompleted];
                auto t1 = std::chrono::steady_clock::now();
                sync.add(std::chrono::duration<double, std::milli>(t1 - t0).count());
                advanceState();
            }

            // ---- parked: pre-committed command buffers gated on MTLSharedEvent.
            // The "supported persistent kernel": work is already scheduled in
            // the queue; per block the CPU only writes input + signals the
            // event. Measures signal -> completed (no encode/commit hot path).
            Stats parked;
            {
                id<MTLSharedEvent> evt = [device newSharedEvent];
                const uint32_t kIters = 500;
                const uint32_t kPark = 8; // buffers parked ahead
                std::vector<id<MTLCommandBuffer>> bufs(kIters, nil);

                auto parkOne = [&](uint64_t i) {
                    if (i >= kIters) return;
                    id<MTLCommandBuffer> cb = [queue commandBuffer];
                    [cb encodeWaitForEvent:evt value:(i + 1)];
                    encode(cb);
                    [cb commit];
                    bufs[i] = cb;
                };
                uint64_t nextToPark = 0;
                for (; nextToPark < kPark; ++nextToPark)
                    parkOne(nextToPark);

                for (uint32_t iter = 0; iter < kIters; ++iter)
                {
                    // Safe to reuse the shared param/input buffers: only one
                    // parked buffer is released and executed at a time.
                    fillInput(1050 + iter);
                    memcpy(bParams.contents, &p, sizeof(p));

                    auto t0 = std::chrono::steady_clock::now();
                    evt.signaledValue = iter + 1;   // release parked buffer #iter
                    [bufs[iter] waitUntilCompleted];
                    auto t1 = std::chrono::steady_clock::now();
                    parked.add(std::chrono::duration<double, std::milli>(t1 - t0).count());

                    bufs[iter] = nil;
                    parkOne(nextToPark++);
                    advanceState();
                }
            }

            // ---- pipelined: depth 4 (wait only on the cb from 4 iterations ago)
            Stats pip;
            {
                id<MTLCommandBuffer> inflight[4] = { nil, nil, nil, nil };
                for (uint32_t iter = 0; iter < 500; ++iter)
                {
                    fillInput(550 + iter);
                    memcpy(bParams.contents, &p, sizeof(p));
                    auto t0 = std::chrono::steady_clock::now();
                    id<MTLCommandBuffer> cb = [queue commandBuffer];
                    encode(cb);
                    [cb commit];
                    const uint32_t slot = iter % 4;
                    if (inflight[slot] != nil)
                        [inflight[slot] waitUntilCompleted];
                    inflight[slot] = cb;
                    auto t1 = std::chrono::steady_clock::now();
                    pip.add(std::chrono::duration<double, std::milli>(t1 - t0).count());
                    advanceState();
                }
                for (auto& cb : inflight)
                    if (cb) [cb waitUntilCompleted];
            }

            printf("%3ux%-5u | %6.3f / %6.3f / %6.3f             | %6.3f / %6.3f / %6.3f      | %6.3f / %6.3f           | maxDiff %.2e %s\n",
                   sz.in, sz.out,
                   sync.mean(), sync.pct(0.99), sync.maxMs,
                   parked.mean(), parked.pct(0.99), parked.maxMs,
                   pip.mean(), pip.pct(0.99),
                   maxDiff, maxDiff < 1e-3f ? "PASS" : "FAIL");
        }
    }
    return 0;
}
