/*
    Standalone validation harness for the CUDA OutputBuffer (scatter) kernels
    (ob_pairs + ob_reduce in Source/DSP/gpu/CudaObKernels.h). The Windows/NVIDIA
    twin of Experiments/metal-output-buffer-test/backend_test.mm.

    NVRTC-compiles the exact kernel string the app ships and runs targeted
    scenarios against a CPU float scatter reference (a faithful reimplementation
    of the host snapshot + the kernels; FR pre-filter passthrough so frIn == raw
    input; FR diffusion sub-stepped per 64 samples like the CPU
    OutputBufferProcessor):

      A. Impulse anatomy: direct spike at D, FR spike at D+E.
      B. Streamed configs vs the CPU twin (direct / HF shelf / FR / diffusion /
         wide / narrow), multi-launch with persistent per-pair accumulators.
      C. Determinism: identical run twice -> bit-identical output.
      D. Latency-clamp regression: raw delay < pipeline L clamps to the 1-sample
         floor and emits PROMPTLY (the v1 bug scattered into the just-cleared
         cell -> ~1 s echo).

    v2 kernel architecture (see CudaObKernels.h): one thread per (in,out) pair,
    private persistent per-pair accumulator (emit+clear+filter+scatter serial per
    pair), then a deterministic per-output reduce. Delay contract: d >= 1 sample.

    Note on precision: write-time fractional positioning against a ~1 s
    accumulator (exactWritePos = writePos + delay, writePos up to ~accLen) loses
    mantissa bits (~accLen * 2^-23 ~ 5e-4); the CPU OutputBufferProcessor does
    the identical computation, so this is faithful behaviour, not error. The
    float twin shares that precision, so GPU-vs-twin agrees to ~1e-5.

    Build (from this directory): nvcc -o test_ob_kernels.exe test_ob_kernels.cpp -lnvrtc -lcuda
    Run:                        test_ob_kernels.exe
*/

#include "../../spatcore/gpu/CudaObKernels.h"
#include "../../spatcore/dsp/FrDiffusionModel.h"

#include <cuda.h>
#include <cuda_runtime.h>
#include <nvrtc.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#define CHECK_CU(call) do { CUresult e = (call); if (e != CUDA_SUCCESS) { \
    const char* s = nullptr; cuGetErrorString (e, &s); \
    std::fprintf (stderr, "CUDA driver error %s at %s:%d\n", s ? s : "?", __FILE__, __LINE__); std::exit (1); } } while (0)
#define CHECK_RT(call) do { cudaError_t e = (call); if (e != cudaSuccess) { \
    std::fprintf (stderr, "CUDA runtime error %s at %s:%d\n", cudaGetErrorString (e), __FILE__, __LINE__); std::exit (1); } } while (0)

static int failures = 0;
static void check (bool ok, const char* what)
{
    std::printf ("%-70s %s\n", what, ok ? "PASS" : "FAIL");
    if (! ok) ++failures;
}

struct ObParamsGpu
{
    uint32_t numInputs, numOutputs, bufferLength, accLength, writePos;
    float shelfCosW0, shelfSinW0;
};


//==============================================================================
struct Shelf { float b0, b1, b2, a1, a2; };
static Shelf shelfCoeffs (float gainDb, float cosw0, float sinw0)
{
    const float A = std::pow (10.0f, gainDb / 40.0f), rootA = std::sqrt (A);
    const float alpha = (sinw0 * 0.5f) * std::sqrt ((A + 1.0f / A) * (1.0f / 0.3f - 1.0f) + 2.0f);
    const float a0inv = 1.0f / ((A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * rootA * alpha);
    Shelf s;
    s.b0 =  A * ((A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * rootA * alpha) * a0inv;
    s.b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw0) * a0inv;
    s.b2 =  A * ((A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * rootA * alpha) * a0inv;
    s.a1 =  2.0f * ((A - 1.0f) - (A + 1.0f) * cosw0) * a0inv;
    s.a2 =  ((A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * rootA * alpha) * a0inv;
    return s;
}

//==============================================================================
// CPU float twin of the host snapshot + the kernels (FR prefilter passthrough).
// Shared-accumulator model (the CPU shape) - matches the GPU's per-pair+reduce
// by linearity within float tolerance. Matrices input-major [in*numOut+out];
// FR delays absolute. Delays clamp to [1, accLen-1] (the d >= 1 contract).
struct CpuObTwin
{
    int numIn, numOut, block; double sr;
    float L = 0.0f;
    uint32_t accLen, writePos = 0; float maxClamp, cosw0, sinw0, srScale;
    bool havePrev = false, haveBase = false;
    std::vector<float> accDir, accFr, shelf, frShelf, dPrev, gPrev, fgPrev;
    std::vector<float> diffusion, baseFrPrev;
    std::vector<FrDiffusion::State> jitterStates;

    void setDiffusion (int in, float pct) { if (in >= 0 && in < (int) diffusion.size()) diffusion[(size_t) in] = std::min (1.0f, std::max (0.0f, pct * 0.01f)); }

    void prepare (int ni, int no, int bs, double sampleRate, float latencyMs, double maxSec)
    {
        numIn = ni; numOut = no; block = bs; sr = sampleRate; L = latencyMs;
        accLen = (uint32_t) (maxSec * sampleRate); if (accLen < (uint32_t) bs + 2) accLen = bs + 2;
        maxClamp = (float) (accLen - 1); srScale = (float) (sampleRate / 1000.0);
        const double w0 = 2.0 * 3.14159265358979 * 800.0 / sampleRate; cosw0 = (float) std::cos (w0); sinw0 = (float) std::sin (w0);
        writePos = 0; havePrev = false; haveBase = false;
        accDir.assign ((size_t) numOut * accLen, 0.0f); accFr.assign ((size_t) numOut * accLen, 0.0f);
        const size_t p = (size_t) numIn * numOut; shelf.assign (p * 4, 0.0f); frShelf.assign (p * 4, 0.0f);
        dPrev.assign (p, 1.0f); gPrev.assign (p, 0.0f); fgPrev.assign (p, 0.0f);
        diffusion.assign ((size_t) numIn, 0.0f);
        jitterStates.assign (p, FrDiffusion::State {});
        for (int in = 0; in < numIn; ++in)
            for (int out = 0; out < numOut; ++out)
                FrDiffusion::resetState (jitterStates[(size_t) in * numOut + out], FrDiffusion::makeKey (in, out));
        baseFrPrev.assign (p, 1.0f);
    }

    void launch (const float* delaysMs, const float* gains, const float* hfDb,
                 const float* frDelaysMs, const float* frLevels, const float* frHfDb,
                 const float* input, float* output)  // input [numIn][block], output [numOut][block]
    {
        const size_t P = (size_t) numIn * numOut;
        std::vector<float> dC (P), gC (P), fgC (P), baseC (P);
        for (size_t m = 0; m < P; ++m) {
            float d = (delaysMs ? (delaysMs[m] - L) : 0.0f) * srScale;
            dC[m] = std::min (std::max (d, 1.0f), maxClamp);                 // d >= 1 contract
            gC[m] = gains ? gains[m] : 0.0f; fgC[m] = frLevels ? frLevels[m] : 0.0f;
            float fb = (delaysMs && frDelaysMs) ? (delaysMs[m] + frDelaysMs[m] - L) * srScale : 0.0f;
            baseC[m] = std::min (std::max (fb, 1.0f), maxClamp);
        }
        if (! havePrev) { dPrev = dC; gPrev = gC; fgPrev = fgC; havePrev = true; }
        if (! haveBase) { baseFrPrev = baseC; haveBase = true; }

        // Per-sample absolute FR delay: jitter stepped per sample for FR-active
        // pairs (exact WfsFrHostState / CPU OutputBufferProcessor model),
        // clamped to >= 1; inactive pairs advance their stream in one span.
        std::vector<float> frPS (P * block, 0.0f);
        {
            const float invLen = 1.0f / (float) block;
            for (int in = 0; in < numIn; ++in)
            {
                const auto coeffs = FrDiffusion::computeCoeffs (diffusion[(size_t) in], (float) sr);
                for (int out = 0; out < numOut; ++out)
                {
                    const size_t m = (size_t) in * numOut + out;
                    const uint32_t key = FrDiffusion::makeKey (in, out);
                    if (fgPrev[m] == 0.0f && fgC[m] == 0.0f)
                    {
                        FrDiffusion::advanceSpan (jitterStates[m], key, coeffs, block);
                        continue;
                    }
                    const float bp = baseFrPrev[m], bc = baseC[m];
                    for (int s = 0; s < block; ++s)
                    {
                        const float jit  = FrDiffusion::processSample (jitterStates[m], key, coeffs);
                        const float base = bp + (bc - bp) * ((float) (s + 1) * invLen);
                        frPS[m * block + s] = std::min (std::max (base + jit, 1.0f), maxClamp);
                    }
                }
            }
            baseFrPrev = baseC;
        }

        // Per-pair shelves into filtered staging (frIn == raw input).
        std::vector<float> fD (P * block, 0.0f), fF (P * block, 0.0f);
        for (size_t m = 0; m < P; ++m) {
            const int in = (int) (m / (size_t) numOut);
            if (gPrev[m] != 0.0f || gC[m] != 0.0f) {
                Shelf c = shelfCoeffs (hfDb ? hfDb[m] : 0.0f, cosw0, sinw0);
                float x1 = shelf[m*4], x2 = shelf[m*4+1], y1 = shelf[m*4+2], y2 = shelf[m*4+3];
                for (int s = 0; s < block; ++s) { float v = input[(size_t) in * block + s]; float w = c.b0*v+c.b1*x1+c.b2*x2-c.a1*y1-c.a2*y2; x2=x1;x1=v;y2=y1;y1=w; fD[m*block+s]=w; }
                shelf[m*4]=x1; shelf[m*4+1]=x2; shelf[m*4+2]=y1; shelf[m*4+3]=y2;
            }
            if (fgPrev[m] != 0.0f || fgC[m] != 0.0f) {
                Shelf c = shelfCoeffs (frHfDb ? frHfDb[m] : 0.0f, cosw0, sinw0);
                float x1 = frShelf[m*4], x2 = frShelf[m*4+1], y1 = frShelf[m*4+2], y2 = frShelf[m*4+3];
                for (int s = 0; s < block; ++s) { float v = input[(size_t) in * block + s]; float w = c.b0*v+c.b1*x1+c.b2*x2-c.a1*y1-c.a2*y2; x2=x1;x1=v;y2=y1;y1=w; fF[m*block+s]=w; }
                frShelf[m*4]=x1; frShelf[m*4+1]=x2; frShelf[m*4+2]=y1; frShelf[m*4+3]=y2;
            }
        }

        // Shared-accumulator scatter (CPU shape).
        const float invLen = 1.0f / (float) block;
        for (int out = 0; out < numOut; ++out) {
            float* aD = accDir.data() + (size_t) out * accLen; float* aF = accFr.data() + (size_t) out * accLen;
            for (int s = 0; s < block; ++s) {
                uint32_t cell = writePos + (uint32_t) s; if (cell >= accLen) cell -= accLen;
                output[(size_t) out * block + s] = aD[cell] + aF[cell]; aD[cell] = 0.0f; aF[cell] = 0.0f;
                const float tt = (float) (s + 1) * invLen;
                for (int in = 0; in < numIn; ++in) {
                    const size_t m = (size_t) in * numOut + out;
                    const float gp = gPrev[m], gc = gC[m], fgp = fgPrev[m], fgc = fgC[m];
                    if (gp != 0.0f || gc != 0.0f) {
                        float gain = gp + (gc - gp) * tt; float d = dPrev[m] + (dC[m] - dPrev[m]) * tt; d = std::max (d, 1.0f);
                        float ewp = (float) cell + d; if (ewp >= (float) accLen) ewp -= (float) accLen;
                        uint32_t p1 = (uint32_t) ewp, p2 = p1 + 1; if (p2 >= accLen) p2 -= accLen; float frac = ewp - (float) p1;
                        float c = fD[m*block+s] * gain; aD[p1] += c * (1.0f - frac); aD[p2] += c * frac;
                    }
                    if (fgp != 0.0f || fgc != 0.0f) {
                        float gain = fgp + (fgc - fgp) * tt; float d = frPS[m*block+s]; d = std::max (d, 1.0f);
                        float ewp = (float) cell + d; if (ewp >= (float) accLen) ewp -= (float) accLen;
                        uint32_t p1 = (uint32_t) ewp, p2 = p1 + 1; if (p2 >= accLen) p2 -= accLen; float frac = ewp - (float) p1;
                        float c = fF[m*block+s] * gain; aF[p1] += c * (1.0f - frac); aF[p2] += c * frac;
                    }
                }
            }
        }
        writePos = (writePos + (uint32_t) block) % accLen;
        dPrev = dC; gPrev = gC; fgPrev = fgC;
    }
};

//==============================================================================
// GPU runner mirroring CudaObBackend's host logic (v2: per-pair acc + reduce).
struct Runner
{
    int numIn, numOut, len; double sr;
    float L = 0.0f;
    uint32_t accLen, writePos = 0; float maxClamp, cosW0, sinW0, srScale;
    bool havePrev = false, haveBase = false;
    CUfunction kPairs, kReduce; cudaStream_t stream;
    float *dIn, *dFrIn, *dOut, *dPairAcc, *dPairOut, *dShelf, *dFrShelf;
    float *dDp, *dDc, *dGp, *dGc, *dFrPS, *dFgp, *dFgc, *dHf, *dFrHf;
    std::vector<float> dPrev, gPrev, fgPrev;
    std::vector<float> diffusion, baseFrPrev;
    std::vector<FrDiffusion::State> jitterStates;
    unsigned int tpb = 256;

    void setDiffusion (int in, float pct) { if (in >= 0 && in < (int) diffusion.size()) diffusion[(size_t) in] = std::min (1.0f, std::max (0.0f, pct * 0.01f)); }

    void init (int nIn, int nOut, int blockLen, double sampleRate, float latencyMs, double maxSec,
               CUfunction pairs, CUfunction reduce)
    {
        numIn = nIn; numOut = nOut; len = blockLen; sr = sampleRate; L = latencyMs;
        accLen = (uint32_t) (maxSec * sampleRate); if (accLen < (uint32_t) len + 2) accLen = len + 2;
        maxClamp = (float) (accLen - 1); srScale = (float) (sampleRate / 1000.0);
        const double w0 = 2.0 * 3.14159265358979 * 800.0 / sampleRate; cosW0 = (float) std::cos (w0); sinW0 = (float) std::sin (w0);
        kPairs = pairs; kReduce = reduce;
        CHECK_RT (cudaStreamCreate (&stream));
        const size_t mat = (size_t) nIn * nOut;
        auto dev = [] (float** p, size_t n) { CHECK_RT (cudaMalloc ((void**) p, n * sizeof (float))); };
        dev (&dIn, (size_t) nIn * len); dev (&dFrIn, (size_t) nIn * len); dev (&dOut, (size_t) nOut * len);
        dev (&dPairAcc, mat * accLen); dev (&dPairOut, mat * len);
        dev (&dShelf, mat * 4); dev (&dFrShelf, mat * 4);
        dev (&dDp, mat); dev (&dDc, mat); dev (&dGp, mat); dev (&dGc, mat);
        dev (&dFrPS, mat * len); dev (&dFgp, mat); dev (&dFgc, mat);
        dev (&dHf, mat); dev (&dFrHf, mat);
        dPrev.assign (mat, 1.0f); gPrev.assign (mat, 0.0f); fgPrev.assign (mat, 0.0f);
        diffusion.assign ((size_t) nIn, 0.0f);
        jitterStates.assign (mat, FrDiffusion::State {});
        baseFrPrev.assign (mat, 1.0f);
        resetState();
    }
    void resetState ()
    {
        const size_t mat = (size_t) numIn * numOut;
        CHECK_RT (cudaMemset (dPairAcc, 0, mat * accLen * sizeof (float)));
        CHECK_RT (cudaMemset (dShelf,   0, mat * 4 * sizeof (float)));
        CHECK_RT (cudaMemset (dFrShelf, 0, mat * 4 * sizeof (float)));
        writePos = 0; havePrev = false; haveBase = false;
        for (int in = 0; in < numIn; ++in)
            for (int o = 0; o < numOut; ++o)
                FrDiffusion::resetState (jitterStates[(size_t) in * numOut + o], FrDiffusion::makeKey (in, o));
    }

    // Full backend-equivalent launch: raw ms matrices in, audio out.
    void launch (const float* delaysMs, const float* gains, const float* hfDb,
                 const float* frDelaysMs, const float* frLevels, const float* frHfDb,
                 const std::vector<float>& in, std::vector<float>& out)
    {
        const size_t P = (size_t) numIn * numOut;
        std::vector<float> dC (P), gC (P), fgC (P), baseC (P), frPS (P * (size_t) len);
        for (size_t m = 0; m < P; ++m) {
            float d = (delaysMs ? (delaysMs[m] - L) : 0.0f) * srScale;
            dC[m] = std::min (std::max (d, 1.0f), maxClamp);
            gC[m] = gains ? gains[m] : 0.0f; fgC[m] = frLevels ? frLevels[m] : 0.0f;
            float fb = (delaysMs && frDelaysMs) ? (delaysMs[m] + frDelaysMs[m] - L) * srScale : 0.0f;
            baseC[m] = std::min (std::max (fb, 1.0f), maxClamp);
        }
        if (! havePrev) { dPrev = dC; gPrev = gC; fgPrev = fgC; havePrev = true; }
        if (! haveBase) { baseFrPrev = baseC; haveBase = true; }

        // Per-sample FR delay (per-sample jitter), identical to WfsFrHostState.
        {
            const float invLen = 1.0f / (float) len;
            for (int inCh = 0; inCh < numIn; ++inCh)
            {
                const auto coeffs = FrDiffusion::computeCoeffs (diffusion[(size_t) inCh], (float) sr);
                for (int o = 0; o < numOut; ++o)
                {
                    const size_t m = (size_t) inCh * numOut + o;
                    const uint32_t key = FrDiffusion::makeKey (inCh, o);
                    if (fgPrev[m] == 0.0f && fgC[m] == 0.0f)
                    {
                        FrDiffusion::advanceSpan (jitterStates[m], key, coeffs, len);
                        continue;
                    }
                    const float bp = baseFrPrev[m], bc = baseC[m];
                    for (int s = 0; s < len; ++s)
                    {
                        const float jit  = FrDiffusion::processSample (jitterStates[m], key, coeffs);
                        const float base = bp + (bc - bp) * ((float) (s + 1) * invLen);
                        frPS[m * (size_t) len + s] = std::min (std::max (base + jit, 1.0f), maxClamp);
                    }
                }
            }
            baseFrPrev = baseC;
        }

        std::vector<float> hf (P, 0.0f), frHf (P, 0.0f);
        for (size_t m = 0; m < P; ++m) { hf[m] = hfDb ? hfDb[m] : 0.0f; frHf[m] = frHfDb ? frHfDb[m] : 0.0f; }

        auto up = [&] (float* d, const float* h, size_t n) { CHECK_RT (cudaMemcpyAsync (d, h, n * sizeof (float), cudaMemcpyHostToDevice, stream)); };
        up (dIn, in.data(), (size_t) numIn * len); up (dFrIn, in.data(), (size_t) numIn * len);  // frIn == raw (prefilter off)
        up (dDp, dPrev.data(), P); up (dDc, dC.data(), P);
        up (dGp, gPrev.data(), P); up (dGc, gC.data(), P);
        up (dFrPS, frPS.data(), P * (size_t) len);
        up (dFgp, fgPrev.data(), P); up (dFgc, fgC.data(), P);
        up (dHf, hf.data(), P); up (dFrHf, frHf.data(), P);

        ObParamsGpu p { (uint32_t) numIn, (uint32_t) numOut, (uint32_t) len, accLen, writePos, cosW0, sinW0 };
        void* pairsArgs[] = { &p, &dIn, &dFrIn, &dHf, &dFrHf, &dDp, &dDc, &dGp, &dGc,
                              &dFrPS, &dFgp, &dFgc, &dShelf, &dFrShelf, &dPairAcc, &dPairOut };
        const unsigned int grid = (unsigned int) ((P + tpb - 1) / tpb);
        CHECK_CU (cuLaunchKernel (kPairs, grid, 1, 1, tpb, 1, 1, 0, (CUstream) stream, pairsArgs, nullptr));

        void* reduceArgs[] = { &p, &dPairOut, &dOut };
        CHECK_CU (cuLaunchKernel (kReduce, (unsigned int) numOut, 1, 1, tpb, 1, 1, 0, (CUstream) stream, reduceArgs, nullptr));

        out.resize ((size_t) numOut * len);
        CHECK_RT (cudaMemcpyAsync (out.data(), dOut, out.size() * sizeof (float), cudaMemcpyDeviceToHost, stream));
        CHECK_RT (cudaStreamSynchronize (stream));

        writePos = (writePos + (uint32_t) len) % accLen;
        dPrev = dC; gPrev = gC; fgPrev = fgC;
    }
};

//==============================================================================
static void streamCompare (const char* label, int numIn, int numOut, int block, int blocks,
                           bool withHf, bool withFr, float diffusionPct, float tol,
                           CUfunction kPairs, CUfunction kReduce)
{
    const double sr = 48000.0;
    const size_t P = (size_t) numIn * numOut;

    std::vector<float> delaysMs (P), gains (P), hf, frDelaysMs, frLevels, frHf;
    for (size_t m = 0; m < P; ++m) {
        delaysMs[m] = (3.0f + 30.0f * (float) ((m * 3) % 13) / 13.0f) / 48.0f;
        gains[m]    = 0.15f + 0.7f * (float) ((m * 5) % 7) / 7.0f;
    }
    if (withHf) { hf.assign (P, 0.0f); for (size_t m = 0; m < P; ++m) hf[m] = -1.0f - 5.0f * (float) (m % 4) / 4.0f; }
    if (withFr) {
        frDelaysMs.assign (P, 0.0f); frLevels.assign (P, 0.0f); frHf.assign (P, 0.0f);
        for (size_t m = 0; m < P; ++m) {
            frDelaysMs[m] = 1.0f + 4.0f * (float) (m % 5) / 5.0f;
            frLevels[m]   = 0.3f * (float) ((m + 1) % 3) / 3.0f;
            frHf[m]       = -2.0f * (float) (m % 3);
        }
    }

    Runner r; r.init (numIn, numOut, block, sr, 0.0f, 1.0, kPairs, kReduce);
    CpuObTwin twin; twin.prepare (numIn, numOut, block, sr, 0.0f, 1.0);
    if (diffusionPct > 0.0f)
        for (int i = 0; i < numIn; ++i) { r.setDiffusion (i, diffusionPct); twin.setDiffusion (i, diffusionPct); }

    std::mt19937 rng (31 + (unsigned) numIn);
    std::uniform_real_distribution<float> dist (-1.0f, 1.0f);
    std::vector<float> in ((size_t) numIn * block), gOut, tOut ((size_t) numOut * block);

    float maxDiff = 0.0f;
    const int checkBlocks = 6;
    for (int b = 0; b < blocks; ++b) {
        for (auto& v : in) v = dist (rng);
        r.launch (delaysMs.data(), gains.data(), withHf ? hf.data() : nullptr,
                  withFr ? frDelaysMs.data() : nullptr, withFr ? frLevels.data() : nullptr, withFr ? frHf.data() : nullptr,
                  in, gOut);
        twin.launch (delaysMs.data(), gains.data(), withHf ? hf.data() : nullptr,
                     withFr ? frDelaysMs.data() : nullptr, withFr ? frLevels.data() : nullptr, withFr ? frHf.data() : nullptr,
                     in.data(), tOut.data());
        if (b >= blocks - checkBlocks)
            for (size_t i = 0; i < gOut.size(); ++i) maxDiff = std::max (maxDiff, std::abs (gOut[i] - tOut[i]));
    }
    char text[160];
    std::snprintf (text, sizeof (text), "%s, GPU-vs-twin err=%.2e", label, maxDiff);
    check (maxDiff < tol, text);
}

//==============================================================================
static void testImpulseAnatomy (CUfunction kPairs, CUfunction kReduce)
{
    const double sr = 48000.0; const float srScale = (float) (sr / 1000.0);
    const int block = 64;
    const float Dsamp = 5.0f, Esamp = 7.0f, fg = 0.5f;
    std::vector<float> delaysMs = { Dsamp / srScale }, gains = { 1.0f };
    std::vector<float> frDelaysMs = { Esamp / srScale }, frLevels = { fg };

    Runner r; r.init (1, 1, block, sr, 0.0f, 1.0, kPairs, kReduce);
    std::vector<float> in (block, 0.0f), out;
    in[0] = 1.0f;
    r.launch (delaysMs.data(), gains.data(), nullptr, frDelaysMs.data(), frLevels.data(), nullptr, in, out);
    const bool direct = std::abs (out[(int) Dsamp] - 1.0f) < 1e-4f;
    const bool fr     = std::abs (out[(int) (Dsamp + Esamp)] - fg) < 1e-4f;
    char text[96];
    std::snprintf (text, sizeof (text), "impulse: direct spike@%d=1, FR spike@%d=%.2f", (int) Dsamp, (int) (Dsamp + Esamp), fg);
    check (direct && fr, text);
}

//==============================================================================
// Latency-clamp regression: raw delay < L clamps to the 1-sample floor and
// emits PROMPTLY (the v1 bug scattered into the just-cleared cell -> ~1 s echo).
static void testLatencyClampPrompt (CUfunction kPairs, CUfunction kReduce)
{
    const double sr = 48000.0;
    const int block = 64;
    const float latencyMs = 256.0f / 48.0f;   // 256 samples (buffer 64 x depth 4)
    std::vector<float> delaysMs = { 2.0f };   // 96 samples raw < L -> clamps to 1
    std::vector<float> gains = { 1.0f };

    Runner r; r.init (1, 1, block, sr, latencyMs, 1.0, kPairs, kReduce);
    std::vector<float> in (block, 0.0f), out;
    in[0] = 1.0f;
    r.launch (delaysMs.data(), gains.data(), nullptr, nullptr, nullptr, nullptr, in, out);
    float blockEnergy = 0.0f;
    for (float v : out) blockEnergy += std::abs (v);
    const bool prompt = std::abs (out[1] - 1.0f) < 1e-4f && std::abs (blockEnergy - 1.0f) < 1e-3f;

    in[0] = 0.0f;
    float latePeak = 0.0f;
    const int wrapBlocks = (int) (sr / block) + 2;
    for (int b = 1; b < wrapBlocks; ++b)
    {
        r.launch (delaysMs.data(), gains.data(), nullptr, nullptr, nullptr, nullptr, in, out);
        for (float v : out) latePeak = std::max (latePeak, std::abs (v));
    }
    char text[128];
    std::snprintf (text, sizeof (text), "raw delay < L emits promptly, no ~1s echo (late peak=%.2e)", latePeak);
    check (prompt && latePeak == 0.0f, text);
}

//==============================================================================
static void testDeterminism (CUfunction kPairs, CUfunction kReduce)
{
    const double sr = 48000.0; const float srScale = (float) (sr / 1000.0);
    const int numIn = 4, numOut = 4, block = 256, blocks = 12;
    const size_t P = (size_t) numIn * numOut;
    std::vector<float> dM (P), gM (P, 0.0f), frD (P, 3.0f), frL (P, 0.5f);
    for (size_t m = 0; m < P; ++m) dM[m] = (5.0f + (float) (m % 7)) / srScale;

    auto run = [&] (std::vector<float>& all) {
        Runner r; r.init (numIn, numOut, block, sr, 0.0f, 1.0, kPairs, kReduce);
        for (int i = 0; i < numIn; ++i) r.setDiffusion (i, 80.0f);
        std::mt19937 rng (123); std::uniform_real_distribution<float> d (-1.0f, 1.0f);
        std::vector<float> in ((size_t) numIn * block), out;
        for (int b = 0; b < blocks; ++b) { for (auto& v : in) v = d (rng);
            r.launch (dM.data(), gM.data(), nullptr, frD.data(), frL.data(), nullptr, in, out);
            all.insert (all.end(), out.begin(), out.end()); }
    };
    std::vector<float> a, b; run (a); run (b);
    float maxDiff = 0.0f, peak = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) { maxDiff = std::max (maxDiff, std::abs (a[i] - b[i])); peak = std::max (peak, std::abs (a[i])); }
    char text[96];
    std::snprintf (text, sizeof (text), "deterministic run-to-run (FR diffusion on), diff=%.2e peak=%.2e", maxDiff, peak);
    check (maxDiff == 0.0f && peak > 1e-3f, text);
}

//==============================================================================
int main()
{
    CHECK_CU (cuInit (0));
    CUdevice dev; CHECK_CU (cuDeviceGet (&dev, 0));
    CUcontext ctx; CHECK_CU (cuDevicePrimaryCtxRetain (&ctx, dev)); CHECK_CU (cuCtxSetCurrent (ctx));
    int major = 0, minor = 0;
    cuDeviceGetAttribute (&major, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, dev);
    cuDeviceGetAttribute (&minor, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR, dev);
    char name[256] = {0}; cuDeviceGetName (name, sizeof (name), dev);
    std::printf ("device: %s (sm_%d%d)\n\n", name, major, minor);

    nvrtcProgram prog;
    if (nvrtcCreateProgram (&prog, kObScatterKernelSource, "ob_scatter.cu", 0, nullptr, nullptr) != NVRTC_SUCCESS)
    { std::fprintf (stderr, "nvrtcCreateProgram failed\n"); return 1; }
    const std::string arch = "--gpu-architecture=compute_" + std::to_string (major * 10 + minor);
    const char* opts[] = { arch.c_str() };
    if (nvrtcCompileProgram (prog, 1, opts) != NVRTC_SUCCESS)
    {
        size_t n = 0; nvrtcGetProgramLogSize (prog, &n); std::string log (n, '\0');
        if (n) nvrtcGetProgramLog (prog, &log[0]); std::fprintf (stderr, "NVRTC compile failed:\n%s\n", log.c_str()); return 1;
    }
    size_t ptxN = 0; nvrtcGetPTXSize (prog, &ptxN); std::vector<char> ptx (ptxN); nvrtcGetPTX (prog, ptx.data());
    CUmodule mod; CHECK_CU (cuModuleLoadDataEx (&mod, ptx.data(), 0, nullptr, nullptr));
    CUfunction kPairs, kReduce;
    CHECK_CU (cuModuleGetFunction (&kPairs, mod, "ob_pairs"));
    CHECK_CU (cuModuleGetFunction (&kReduce, mod, "ob_reduce"));

    testImpulseAnatomy (kPairs, kReduce);
    streamCompare ("direct only, 0 dB shelf (8x8x256)",      8, 8, 256, 24, false, false, 0.0f,   1e-4f, kPairs, kReduce);
    streamCompare ("direct + HF shelf (8x8x256)",            8, 8, 256, 24, true,  false, 0.0f,   2e-4f, kPairs, kReduce);
    streamCompare ("direct + FR path (6x6x256)",             6, 6, 256, 24, true,  true,  0.0f,   2e-4f, kPairs, kReduce);
    streamCompare ("direct + FR + 60% diffusion (6x6x256)",  6, 6, 256, 24, true,  true,  60.0f,  2e-4f, kPairs, kReduce);
    streamCompare ("direct + FR + 100% diffusion (8x8x128)", 8, 8, 128, 24, true,  true,  100.0f, 2e-4f, kPairs, kReduce);
    streamCompare ("wide 16x32x128",                        16, 32, 128, 24, true,  true,  0.0f,   2e-4f, kPairs, kReduce);
    streamCompare ("narrow output 32x4x256 (occupancy)",    32, 4, 256, 24, true,  true,  0.0f,   2e-4f, kPairs, kReduce);
    testLatencyClampPrompt (kPairs, kReduce);
    testDeterminism (kPairs, kReduce);

    std::printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL PASS" : "FAILURES", failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
