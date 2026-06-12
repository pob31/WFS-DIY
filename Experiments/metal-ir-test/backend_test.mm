//
// Correctness + timing test for MetalIrBackend (the hybrid host-FFT /
// GPU-MAC convolution the reverb uses): drives it like the GpuAsyncPipelineT
// pump would and validates the streamed output against a direct time-domain
// convolution of the same input/IR.
//
// Covers: host-FFT roundtrip + forward vs naive DFT, IR shorter than one
// partition, IR spanning many partitions, the progressive segment loader
// (output converges to the full convolution as segments come online),
// mid-stream IR restage, reset, and the non-power-of-two block rejection.
//
// Build:  ./build.sh   (or see the clang++ line inside)
// Run:    ./backend_test
//

#define WFS_GPU_NATIVE 1

#include "MetalIrBackend.h"
#include "IrHostFft.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

static int failures = 0;

static void check (bool ok, const char* what)
{
    printf ("%-58s %s\n", what, ok ? "PASS" : "FAIL");
    if (! ok)
        ++failures;
}

//==============================================================================
static void testHostFft()
{
    // Roundtrip + forward parity with a naive DFT at the block sizes the
    // engine can produce.
    for (int P : { 64, 128, 256 })
    {
        const int N = 2 * P;
        IrHostFft fft;
        if (! fft.prepare (N)) { check (false, "fft prepare"); return; }

        std::mt19937 rng (42 + P);
        std::uniform_real_distribution<float> d (-1.0f, 1.0f);
        std::vector<float> x ((size_t) N), spec ((size_t) N), back ((size_t) N);
        for (auto& v : x) v = d (rng);

        fft.forward (x.data(), spec.data());

        // Naive DFT reference
        float maxErr = 0.0f;
        for (int k = 0; k <= P; ++k)
        {
            double re = 0.0, im = 0.0;
            for (int n = 0; n < N; ++n)
            {
                const double a = -2.0 * M_PI * k * n / N;
                re += x[(size_t) n] * std::cos (a);
                im += x[(size_t) n] * std::sin (a);
            }
            float gre, gim;
            if (k == 0)       { gre = spec[0]; gim = 0.0f; }
            else if (k == P)  { gre = spec[1]; gim = 0.0f; }
            else              { gre = spec[(size_t) (2 * k)]; gim = spec[(size_t) (2 * k + 1)]; }
            maxErr = std::max (maxErr, (float) std::abs (re - gre));
            maxErr = std::max (maxErr, (float) std::abs (im - gim));
        }
        char label[64];
        snprintf (label, sizeof (label), "host FFT vs naive DFT (N=%d), err=%.2e", N, maxErr);
        check (maxErr < 1e-2f * std::sqrt ((float) N), label);   // naive DFT accumulates error too

        fft.inverse (spec.data(), back.data());
        float rtErr = 0.0f;
        for (int n = 0; n < N; ++n)
            rtErr = std::max (rtErr, std::abs (back[(size_t) n] - x[(size_t) n]));
        snprintf (label, sizeof (label), "host FFT roundtrip (N=%d), err=%.2e", N, rtErr);
        check (rtErr < 1e-5f, label);
    }
}

//==============================================================================
// Streams `blocks` blocks of input through the backend and compares the tail
// of the stream (after the IR is fully loaded) against direct convolution.
static void testConvolution (int numNodes, int block, int irLen, const char* label)
{
    const double sr = 48000.0;
    const int maxIr = (int) (10.0 * sr);

    MetalIrBackend backend;
    if (! backend.prepare (numNodes, block, sr, maxIr))
    {
        fprintf (stderr, "prepare failed: %s\n", backend.getLastError().c_str());
        check (false, label);
        return;
    }

    std::mt19937 rng (1234 + irLen);
    std::uniform_real_distribution<float> d (-1.0f, 1.0f);

    std::vector<float> ir ((size_t) irLen);
    for (auto& v : ir) v = d (rng) * std::exp (-3.0f * (float) (&v - ir.data()) / (float) irLen);
    backend.stageIr (ir.data(), irLen);

    const int loaderLaunches = (irLen + block - 1) / block / 64 + 2; // progressive-load window
    const int checkBlocks = 8;
    const int blocks = loaderLaunches + (irLen / block) + checkBlocks + 4;

    std::vector<float> input ((size_t) numNodes * (size_t) blocks * block);
    for (auto& v : input) v = d (rng);

    std::vector<float> gpuOut ((size_t) numNodes * (size_t) blocks * block, 0.0f);
    std::vector<const float*> inPtrs ((size_t) numNodes);
    std::vector<float*> outPtrs ((size_t) numNodes);

    double sumMs = 0.0, maxMs = 0.0;
    for (int b = 0; b < blocks; ++b)
    {
        for (int n = 0; n < numNodes; ++n)
        {
            inPtrs[(size_t) n] = input.data() + ((size_t) n * blocks + b) * block;
            outPtrs[(size_t) n] = gpuOut.data() + ((size_t) n * blocks + b) * block;
        }
        if (! backend.processBlock (inPtrs.data(), outPtrs.data()))
        {
            fprintf (stderr, "processBlock failed: %s\n", backend.getLastError().c_str());
            check (false, label);
            return;
        }
        sumMs += backend.getLastLaunchMs();
        maxMs = std::max (maxMs, backend.getLastLaunchMs());
    }

    // Direct convolution reference over the final checkBlocks blocks (the IR
    // is fully loaded and the input ring holds full history there).
    float maxDiff = 0.0f;
    const int t0 = (blocks - checkBlocks) * block;
    for (int n = 0; n < numNodes; ++n)
    {
        const float* x = input.data() + (size_t) n * blocks * block;
        const float* y = gpuOut.data() + (size_t) n * blocks * block;
        for (int t = t0; t < blocks * block; ++t)
        {
            double acc = 0.0;
            const int kMax = std::min (irLen - 1, t);
            for (int k = 0; k <= kMax; ++k)
                acc += (double) ir[(size_t) k] * x[(size_t) (t - k)];
            maxDiff = std::max (maxDiff, (float) std::abs (acc - y[(size_t) t]));
        }
    }

    char text[128];
    snprintf (text, sizeof (text), "%s, err=%.2e, launch avg %.3f / max %.3f ms",
              label, maxDiff, sumMs / blocks, maxMs);
    check (maxDiff < 2e-4f * std::sqrt ((float) irLen), text);
}

//==============================================================================
// Restage a different IR mid-stream, then verify the tail converges to the
// new IR's convolution; then reset and verify history is cleared.
static void testRestageAndReset()
{
    const int numNodes = 4, block = 256, irLen = 8192;
    const double sr = 48000.0;

    MetalIrBackend backend;
    if (! backend.prepare (numNodes, block, sr, (int) (10.0 * sr)))
    {
        check (false, "restage prepare");
        return;
    }

    std::mt19937 rng (77);
    std::uniform_real_distribution<float> d (-1.0f, 1.0f);
    std::vector<float> irA ((size_t) irLen), irB ((size_t) irLen);
    for (auto& v : irA) v = d (rng);
    for (auto& v : irB) v = d (rng);

    backend.stageIr (irA.data(), irLen);

    const int blocks = 200;   // plenty for both load windows
    std::vector<float> input ((size_t) numNodes * (size_t) blocks * block);
    for (auto& v : input) v = d (rng);
    std::vector<float> gpuOut ((size_t) numNodes * (size_t) blocks * block, 0.0f);
    std::vector<const float*> inPtrs ((size_t) numNodes);
    std::vector<float*> outPtrs ((size_t) numNodes);

    for (int b = 0; b < blocks; ++b)
    {
        if (b == blocks / 2)
            backend.stageIr (irB.data(), irLen);   // mid-stream restage

        for (int n = 0; n < numNodes; ++n)
        {
            inPtrs[(size_t) n] = input.data() + ((size_t) n * blocks + b) * block;
            outPtrs[(size_t) n] = gpuOut.data() + ((size_t) n * blocks + b) * block;
        }
        if (! backend.processBlock (inPtrs.data(), outPtrs.data()))
        {
            check (false, "restage processBlock");
            return;
        }
    }

    float maxDiff = 0.0f;
    const int t0 = (blocks - 4) * block;
    for (int n = 0; n < numNodes; ++n)
    {
        const float* x = input.data() + (size_t) n * blocks * block;
        const float* y = gpuOut.data() + (size_t) n * blocks * block;
        for (int t = t0; t < blocks * block; ++t)
        {
            double acc = 0.0;
            for (int k = 0; k < irLen; ++k)
                acc += (double) irB[(size_t) k] * x[(size_t) (t - k)];
            maxDiff = std::max (maxDiff, (float) std::abs (acc - y[(size_t) t]));
        }
    }
    char text[96];
    snprintf (text, sizeof (text), "mid-stream IR restage converges to new IR, err=%.2e", maxDiff);
    check (maxDiff < 2e-4f * std::sqrt ((float) irLen), text);

    // Reset: silence in -> output must be silence (history cleared), even
    // though the IR spectra survive.
    backend.requestReset();
    std::vector<float> zeros ((size_t) block, 0.0f);
    std::vector<float> out ((size_t) numNodes * block, 1.0f);
    for (int n = 0; n < numNodes; ++n)
    {
        inPtrs[(size_t) n] = zeros.data();
        outPtrs[(size_t) n] = out.data() + (size_t) n * block;
    }
    backend.processBlock (inPtrs.data(), outPtrs.data());
    float peak = 0.0f;
    for (float v : out) peak = std::max (peak, std::abs (v));
    char text2[96];
    snprintf (text2, sizeof (text2), "reset clears history (silence in -> silence out), peak=%.2e", peak);
    check (peak == 0.0f, text2);
}

//==============================================================================
int main()
{
    testHostFft();

    MetalIrBackend reject;
    check (! reject.prepare (4, 240, 48000.0, 480000)
           && ! reject.getLastError().empty(),
           "non-power-of-two block size is rejected");

    testConvolution (4, 256, 100,    "IR shorter than one partition (4n x 256, ir 100)");
    testConvolution (4, 256, 8192,   "IR spanning 32 partitions (4n x 256, ir 8192)");
    testConvolution (16, 256, 48000, "1 s IR, 16 nodes (16n x 256, ir 48000)");
    testConvolution (4, 128, 12000,  "block 128 (4n x 128, ir 12000)");
    testConvolution (16, 256, 480000, "10 s IR, 16 nodes - worst case");

    testRestageAndReset();

    printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL PASS" : "FAILURES",
            failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
