//
// Correctness + timing test for MetalObBackend (the WFS OutputBuffer scatter /
// write-time algorithm on Metal): drives it like the GpuAsyncPipelineT pump
// would and validates the streamed output against (a) an independent
// closed-form fractional delay-and-sum for the steady constant-delay case, and
// (b) a faithful CPU reimplementation of the backend's host+kernel pipeline (the
// "twin") for ramped delays, HF shelves and the Floor-Reflection second path.
//
// The twin and the backend keep Floor-Reflection PRE-filtering (LowCut/HighShelf)
// and diffusion disabled (defaults), so frIn == raw input and jitter == 0; the
// FR path is then a second delay+shelf+level scatter the twin reproduces exactly.
// Diffusion is covered by a run-to-run determinism check instead.
//
// Build:  ./build.sh     Run: ./backend_test
//

#define WFS_GPU_NATIVE 1

#include "MetalObBackend.h"
#include "../../spatcore/dsp/FrDiffusionModel.h"   // shared grain model (CPU parity check)

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

static int failures = 0;

static void check (bool ok, const char* what)
{
    printf ("%-66s %s\n", what, ok ? "PASS" : "FAIL");
    if (! ok)
        ++failures;
}

//==============================================================================
// Float high-shelf coefficients: byte-for-byte the in-kernel shelfCoeffs (and
// WFSHighShelfFilter), 800 Hz / Q 0.3.
struct Shelf { float b0, b1, b2, a1, a2; };

static Shelf shelfCoeffs (float gainDb, float cosw0, float sinw0)
{
    const float A     = std::pow (10.0f, gainDb / 40.0f);
    const float rootA = std::sqrt (A);
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
// Faithful CPU reimplementation of the backend host snapshot + the two kernels
// (FR pre-filter passthrough, diffusion off). Matrices are input-major
// [in*numOut+out]; FR delays are ABSOLUTE (direct + extra).
struct CpuObTwin
{
    int numIn = 0, numOut = 0, blockSize = 0;
    double sr = 0.0;
    float L = 0.0f;
    uint32_t accLen = 0, writePos = 0;
    float maxClamp = 0.0f, cosw0 = 1.0f, sinw0 = 0.0f, srScale = 0.0f;
    bool havePrev = false;

    std::vector<float> accDir, accFr;          // [numOut][accLen]
    std::vector<float> shelf, frShelf;         // [pairs][4] = x1,x2,y1,y2
    std::vector<float> dPrev, gPrev, fgPrev;   // [pairs]
    // FR diffusion (WfsFrHostState / CPU OutputBufferProcessor parity): per-sample.
    std::vector<float> diffusion;              // [numIn] fraction 0..1
    std::vector<FrDiffusion::State> jitterStates;  // [pairs]
    std::vector<float> baseFrPrev;             // [pairs]
    bool haveBase = false;

    void setDiffusion (int in, float percent) { if (in >= 0 && in < (int) diffusion.size()) diffusion[(size_t) in] = std::min (1.0f, std::max (0.0f, percent * 0.01f)); }

    void prepare (int ni, int no, int bs, double sampleRate, float latencyMs, double maxDelaySeconds)
    {
        numIn = ni; numOut = no; blockSize = bs; sr = sampleRate; L = latencyMs;
        accLen = (uint32_t) (maxDelaySeconds * sampleRate);
        if (accLen < (uint32_t) bs + 1) accLen = (uint32_t) bs + 1;
        maxClamp = (float) (accLen - 1);
        srScale = (float) (sampleRate / 1000.0);
        const double w0 = 2.0 * 3.14159265358979 * 800.0 / sampleRate;
        cosw0 = (float) std::cos (w0); sinw0 = (float) std::sin (w0);
        writePos = 0; havePrev = false; haveBase = false;
        accDir.assign ((size_t) numOut * accLen, 0.0f);
        accFr.assign ((size_t) numOut * accLen, 0.0f);
        const size_t pairs = (size_t) numIn * numOut;
        shelf.assign (pairs * 4, 0.0f);
        frShelf.assign (pairs * 4, 0.0f);
        dPrev.assign (pairs, 0.0f); gPrev.assign (pairs, 0.0f); fgPrev.assign (pairs, 0.0f);
        diffusion.assign ((size_t) numIn, 0.0f);
        jitterStates.assign (pairs, FrDiffusion::State {});
        for (int in = 0; in < numIn; ++in)
            for (int out = 0; out < numOut; ++out)
                FrDiffusion::resetState (jitterStates[(size_t) in * numOut + out], FrDiffusion::makeKey (in, out));
        baseFrPrev.assign (pairs, 0.0f);
    }

    void processBlock (const float* delaysMs, const float* gains, const float* hfDb,
                       const float* frDelaysMs, const float* frLevels, const float* frHfDb,
                       const float* const* inputs, float* const* outputs)
    {
        const size_t pairs = (size_t) numIn * numOut;
        std::vector<float> dCurr (pairs), gCurr (pairs), fgCurr (pairs), baseCurr (pairs);
        for (size_t m = 0; m < pairs; ++m)
        {
            // Min-1 clamp: the scatter's d >= 1 contract (writes strictly future).
            float d = (delaysMs ? (delaysMs[m] - L) : 0.0f) * srScale;
            dCurr[m] = std::clamp (d, 1.0f, maxClamp);
            gCurr[m] = gains ? gains[m] : 0.0f;
            fgCurr[m] = frLevels ? frLevels[m] : 0.0f;
            float fb = 0.0f;
            if (delaysMs && frDelaysMs) fb = (delaysMs[m] + frDelaysMs[m] - L) * srScale; // base, no jitter
            baseCurr[m] = std::clamp (fb, 1.0f, maxClamp);
        }
        if (! havePrev) { dPrev = dCurr; gPrev = gCurr; fgPrev = fgCurr; havePrev = true; }
        if (! haveBase) { baseFrPrev = baseCurr; haveBase = true; }

        // Per-sample absolute FR delay: jitter stepped per sample for FR-active
        // pairs (exact WfsFrHostState / CPU OutputBufferProcessor model);
        // inactive pairs advance their stream in one span.
        std::vector<float> frDelayPS (pairs * blockSize, 0.0f);
        {
            const float invLen = 1.0f / (float) blockSize;
            for (int in = 0; in < numIn; ++in)
            {
                const auto coeffs = FrDiffusion::computeCoeffs (diffusion[(size_t) in], (float) sr);
                for (int out = 0; out < numOut; ++out)
                {
                    const size_t m = (size_t) in * numOut + out;
                    const uint32_t key = FrDiffusion::makeKey (in, out);
                    if (fgPrev[m] == 0.0f && fgCurr[m] == 0.0f)
                    {
                        FrDiffusion::advanceSpan (jitterStates[m], key, coeffs, blockSize);
                        continue;
                    }
                    const float bp = baseFrPrev[m], bc = baseCurr[m];
                    for (int s = 0; s < blockSize; ++s)
                    {
                        const float jit  = FrDiffusion::processSample (jitterStates[m], key, coeffs);
                        const float base = bp + (bc - bp) * ((float) (s + 1) * invLen);
                        frDelayPS[m * blockSize + s] = std::clamp (base + jit, 1.0f, maxClamp);
                    }
                }
            }
            baseFrPrev = baseCurr;
        }

        // K1: per-pair shelf -> filtered staging (frIn == raw input, prefilter off)
        std::vector<float> fDir (pairs * blockSize, 0.0f), fFr (pairs * blockSize, 0.0f);
        for (size_t m = 0; m < pairs; ++m)
        {
            const int in = (int) (m / (size_t) numOut);
            const bool doDir = (gPrev[m] != 0.0f || gCurr[m] != 0.0f);
            const bool doFr  = (fgPrev[m] != 0.0f || fgCurr[m] != 0.0f);
            if (doDir)
            {
                const Shelf c = shelfCoeffs (hfDb ? hfDb[m] : 0.0f, cosw0, sinw0);
                float x1 = shelf[m*4+0], x2 = shelf[m*4+1], y1 = shelf[m*4+2], y2 = shelf[m*4+3];
                for (int s = 0; s < blockSize; ++s)
                {
                    const float v = inputs[in] ? inputs[in][s] : 0.0f;
                    const float w = c.b0*v + c.b1*x1 + c.b2*x2 - c.a1*y1 - c.a2*y2;
                    x2=x1; x1=v; y2=y1; y1=w; fDir[m*blockSize+s] = w;
                }
                shelf[m*4+0]=x1; shelf[m*4+1]=x2; shelf[m*4+2]=y1; shelf[m*4+3]=y2;
            }
            if (doFr)
            {
                const Shelf c = shelfCoeffs (frHfDb ? frHfDb[m] : 0.0f, cosw0, sinw0);
                float x1 = frShelf[m*4+0], x2 = frShelf[m*4+1], y1 = frShelf[m*4+2], y2 = frShelf[m*4+3];
                for (int s = 0; s < blockSize; ++s)
                {
                    const float v = inputs[in] ? inputs[in][s] : 0.0f;   // frIn == raw input
                    const float w = c.b0*v + c.b1*x1 + c.b2*x2 - c.a1*y1 - c.a2*y2;
                    x2=x1; x1=v; y2=y1; y1=w; fFr[m*blockSize+s] = w;
                }
                frShelf[m*4+0]=x1; frShelf[m*4+1]=x2; frShelf[m*4+2]=y1; frShelf[m*4+3]=y2;
            }
        }

        // K2: per-output scatter
        const float invLen = 1.0f / (float) blockSize;
        for (int out = 0; out < numOut; ++out)
        {
            float* aDir = accDir.data() + (size_t) out * accLen;
            float* aFr  = accFr.data()  + (size_t) out * accLen;
            for (int s = 0; s < blockSize; ++s)
            {
                uint32_t cell = writePos + (uint32_t) s;
                if (cell >= accLen) cell -= accLen;
                if (outputs[out]) outputs[out][s] = aDir[cell] + aFr[cell];
                aDir[cell] = 0.0f; aFr[cell] = 0.0f;
                const float tt = (float) (s + 1) * invLen;
                for (int in = 0; in < numIn; ++in)
                {
                    const size_t m = (size_t) in * numOut + out;
                    const float gp = gPrev[m], gc = gCurr[m], fgp = fgPrev[m], fgc = fgCurr[m];
                    const bool doDir = (gp != 0.0f || gc != 0.0f);
                    const bool doFr  = (fgp != 0.0f || fgc != 0.0f);
                    if (! doDir && ! doFr) continue;
                    if (doDir)
                    {
                        const float gain = gp + (gc - gp) * tt;
                        float d = dPrev[m] + (dCurr[m] - dPrev[m]) * tt; d = std::max (d, 1.0f);
                        float ewp = (float) cell + d; if (ewp >= (float) accLen) ewp -= (float) accLen;
                        const uint32_t p1 = (uint32_t) ewp; uint32_t p2 = p1 + 1; if (p2 >= accLen) p2 -= accLen;
                        const float frac = ewp - (float) p1;
                        const float c = fDir[m*blockSize+s] * gain;
                        aDir[p1] += c * (1.0f - frac); aDir[p2] += c * frac;
                    }
                    if (doFr)
                    {
                        const float gain = fgp + (fgc - fgp) * tt;
                        float d = frDelayPS[m * blockSize + s]; d = std::max (d, 1.0f);
                        float ewp = (float) cell + d; if (ewp >= (float) accLen) ewp -= (float) accLen;
                        const uint32_t p1 = (uint32_t) ewp; uint32_t p2 = p1 + 1; if (p2 >= accLen) p2 -= accLen;
                        const float frac = ewp - (float) p1;
                        const float c = fFr[m*blockSize+s] * gain;
                        aFr[p1] += c * (1.0f - frac); aFr[p2] += c * frac;
                    }
                }
            }
        }
        writePos = (writePos + (uint32_t) blockSize) % accLen;
        dPrev = dCurr; gPrev = gCurr; fgPrev = fgCurr;
    }
};

//==============================================================================
// Helper: stream `blocks` blocks through the backend AND the twin with the given
// (constant across the stream) matrices; return max abs diff over the tail.
struct StreamResult { float gpuVsTwin = 0.0f; double avgMs = 0.0, maxMs = 0.0; };

static StreamResult streamCompare (int numIn, int numOut, int block, int blocks,
                                   const std::vector<float>& delaysMs,
                                   const std::vector<float>& gains,
                                   const std::vector<float>& hfDb,
                                   const std::vector<float>& frDelaysMs,
                                   const std::vector<float>& frLevels,
                                   const std::vector<float>& frHfDb,
                                   unsigned seed, bool* prepOk, float diffusionPct = 0.0f)
{
    const double sr = 48000.0;
    StreamResult r;

    MetalObBackend backend;
    if (! backend.prepare (numIn, numOut, block, sr, /*L*/ 0.0, /*maxDelaySeconds*/ 1.0))
    {
        fprintf (stderr, "prepare failed: %s\n", backend.getLastError().c_str());
        if (prepOk) *prepOk = false;
        return r;
    }
    if (prepOk) *prepOk = true;
    backend.setMatrixPointers (delaysMs.data(), gains.data(),
                               hfDb.empty() ? nullptr : hfDb.data(),
                               frDelaysMs.empty() ? nullptr : frDelaysMs.data(),
                               frLevels.empty() ? nullptr : frLevels.data(),
                               frHfDb.empty() ? nullptr : frHfDb.data());

    CpuObTwin twin;
    twin.prepare (numIn, numOut, block, sr, 0.0f, 1.0);

    if (diffusionPct > 0.0f)
        for (int i = 0; i < numIn; ++i) { backend.setFRDiffusion (i, diffusionPct); twin.setDiffusion (i, diffusionPct); }

    std::mt19937 rng (seed);
    std::uniform_real_distribution<float> d (-1.0f, 1.0f);

    std::vector<float> in ((size_t) numIn * blocks * block);
    for (auto& v : in) v = d (rng);
    std::vector<float> gOut ((size_t) numOut * block), tOut ((size_t) numOut * block);
    std::vector<const float*> inPtrs ((size_t) numIn);
    std::vector<float*> gPtrs ((size_t) numOut), tPtrs ((size_t) numOut);

    const int checkBlocks = 6;
    for (int b = 0; b < blocks; ++b)
    {
        for (int n = 0; n < numIn; ++n)
            inPtrs[(size_t) n] = in.data() + ((size_t) n * blocks + b) * block;
        for (int n = 0; n < numOut; ++n) { gPtrs[(size_t) n] = gOut.data() + (size_t) n * block;
                                           tPtrs[(size_t) n] = tOut.data() + (size_t) n * block; }

        backend.processBlock (inPtrs.data(), gPtrs.data());
        twin.processBlock (delaysMs.data(), gains.data(),
                           hfDb.empty() ? nullptr : hfDb.data(),
                           frDelaysMs.empty() ? nullptr : frDelaysMs.data(),
                           frLevels.empty() ? nullptr : frLevels.data(),
                           frHfDb.empty() ? nullptr : frHfDb.data(),
                           inPtrs.data(), tPtrs.data());

        r.avgMs += backend.getLastLaunchMs();
        r.maxMs = std::max (r.maxMs, backend.getLastLaunchMs());

        if (b >= blocks - checkBlocks)
            for (int n = 0; n < numOut; ++n)
                for (int s = 0; s < block; ++s)
                    r.gpuVsTwin = std::max (r.gpuVsTwin,
                        std::abs (gOut[(size_t) n * block + s] - tOut[(size_t) n * block + s]));
    }
    r.avgMs /= blocks;
    return r;
}

//==============================================================================
// Independent closed-form check: constant integer-or-fractional delay, 0 dB
// shelf, constant gain. Forward scatter == fractional gather, so
// out[o][n] = sum_in gain * lerp(x[in], n - d[in,o]).
static void testClosedFormDelaySum()
{
    const int numIn = 6, numOut = 4, block = 128, blocks = 40;
    const double sr = 48000.0;

    std::mt19937 rng (7);
    std::uniform_real_distribution<float> dist (-1.0f, 1.0f);

    // Delays 2..40 samples => ms = samples / (sr/1000)
    std::vector<float> delaysMs ((size_t) numIn * numOut), gains ((size_t) numIn * numOut);
    std::vector<float> dSamples ((size_t) numIn * numOut);
    for (size_t m = 0; m < dSamples.size(); ++m)
    {
        dSamples[m] = 2.0f + 38.0f * (float) ((m * 7) % 11) / 11.0f + 0.37f;
        delaysMs[m] = dSamples[m] / (float) (sr / 1000.0);
        gains[m] = 0.2f + 0.6f * (float) ((m * 5) % 9) / 9.0f;
    }

    MetalObBackend backend;
    if (! backend.prepare (numIn, numOut, block, sr, 0.0, 1.0)) { check (false, "closed-form prepare"); return; }
    backend.setMatrixPointers (delaysMs.data(), gains.data(), nullptr, nullptr, nullptr, nullptr);

    std::mt19937 r2 (99);
    std::vector<float> in ((size_t) numIn * blocks * block);
    for (auto& v : in) v = dist (r2);
    std::vector<float> gOut ((size_t) numOut * blocks * block, 0.0f);
    std::vector<const float*> inPtrs ((size_t) numIn);
    std::vector<float*> outPtrs ((size_t) numOut);

    for (int b = 0; b < blocks; ++b)
    {
        for (int n = 0; n < numIn; ++n)  inPtrs[(size_t) n]  = in.data()   + ((size_t) n * blocks + b) * block;
        for (int n = 0; n < numOut; ++n) outPtrs[(size_t) n] = gOut.data() + ((size_t) n * blocks + b) * block;
        backend.processBlock (inPtrs.data(), outPtrs.data());
    }

    float maxDiff = 0.0f;
    const int t0 = (blocks - 6) * block, T = blocks * block;
    const float srScale = (float) (sr / 1000.0);
    for (int o = 0; o < numOut; ++o)
        for (int t = t0; t < T; ++t)
        {
            double acc = 0.0;
            for (int in0 = 0; in0 < numIn; ++in0)
            {
                const size_t m = (size_t) in0 * numOut + o;
                // Use the SAME delay the backend sees: clamp((delaysMs)*srScale).
                const float d = delaysMs[m] * srScale;
                const int di = (int) d; const float f = d - (float) di;
                const float* x = in.data() + (size_t) in0 * blocks * block;
                const float x0 = (t - di     >= 0) ? x[t - di]     : 0.0f;
                const float x1 = (t - di - 1 >= 0) ? x[t - di - 1] : 0.0f;
                acc += (double) gains[m] * ((1.0f - f) * x0 + f * x1);
            }
            const float* y = gOut.data() + (size_t) o * blocks * block;
            maxDiff = std::max (maxDiff, (float) std::abs (acc - y[t]));
        }
    // Tolerance reflects the INHERENT precision of write-time fractional
    // positioning against a ~1 s delay buffer: the scatter computes
    // exactWritePos = writePosition + delay (writePosition up to ~accLen), so
    // frac = exactWritePos - floor(exactWritePos) loses mantissa bits
    // (~accLen * 2^-23). The CPU OutputBufferProcessor does the IDENTICAL
    // computation (OutputBufferProcessor.h:510), so the GPU faithfully
    // reproduces the CPU's precision; this "ideal" double gather is strictly
    // more accurate than the algorithm under test. The float twin (which
    // matches the scatter precision) agrees with the GPU to ~6e-6 - that is the
    // tight check; this one guards against structural errors (off-by-one delay,
    // wrong gain, missing taps), all of which dwarf the ~5e-4 precision floor.
    char text[128];
    snprintf (text, sizeof (text), "closed-form delay+sum (6x4x128), err=%.2e (precision floor ~5e-4)", maxDiff);
    check (maxDiff < 2e-3f, text);
}

//==============================================================================
static void testTwin (const char* label, int numIn, int numOut, int block, int blocks,
                      bool withHf, bool withFr, float tol, float diffusionPct = 0.0f)
{
    const size_t pairs = (size_t) numIn * numOut;
    std::vector<float> delaysMs (pairs), gains (pairs), hfDb, frDelaysMs, frLevels, frHfDb;
    for (size_t m = 0; m < pairs; ++m)
    {
        delaysMs[m] = (3.0f + 30.0f * (float) ((m * 3) % 13) / 13.0f) / 48.0f; // ms (48 samp/ms)
        gains[m]    = 0.15f + 0.7f * (float) ((m * 5) % 7) / 7.0f;
    }
    if (withHf) { hfDb.assign (pairs, 0.0f); for (size_t m = 0; m < pairs; ++m) hfDb[m] = -1.0f - 5.0f * (float) (m % 4) / 4.0f; }
    if (withFr)
    {
        frDelaysMs.assign (pairs, 0.0f); frLevels.assign (pairs, 0.0f); frHfDb.assign (pairs, 0.0f);
        for (size_t m = 0; m < pairs; ++m)
        {
            frDelaysMs[m] = 1.0f + 4.0f * (float) (m % 5) / 5.0f;   // extra ms
            frLevels[m]   = 0.3f * (float) ((m + 1) % 3) / 3.0f;
            frHfDb[m]     = -2.0f * (float) (m % 3);
        }
    }
    bool ok = false;
    StreamResult r = streamCompare (numIn, numOut, block, blocks,
                                    delaysMs, gains, hfDb, frDelaysMs, frLevels, frHfDb, 31 + (unsigned) numIn, &ok, diffusionPct);
    if (! ok) { check (false, label); return; }
    char text[160];
    snprintf (text, sizeof (text), "%s, GPU-vs-twin err=%.2e, launch avg %.3f / max %.3f ms",
              label, r.gpuVsTwin, r.avgMs, r.maxMs);
    check (r.gpuVsTwin < tol, text);
}

//==============================================================================
// Ramp: change the delay/gain matrices between two streamed phases (prev->curr
// linear ramp). Compared against the twin which ramps identically.
static void testRamp()
{
    const int numIn = 8, numOut = 8, block = 256;
    const double sr = 48000.0;
    const size_t pairs = (size_t) numIn * numOut;

    std::vector<float> dA (pairs), gA (pairs), dB (pairs), gB (pairs);
    for (size_t m = 0; m < pairs; ++m)
    {
        dA[m] = (5.0f + (float) (m % 10)) / 48.0f;  gA[m] = 0.5f;
        dB[m] = (5.0f + (float) (m % 10) + 20.0f) / 48.0f; gB[m] = 0.9f; // +20 samp jump
    }
    std::vector<float> none;

    MetalObBackend backend;
    if (! backend.prepare (numIn, numOut, block, sr, 0.0, 1.0)) { check (false, "ramp prepare"); return; }
    CpuObTwin twin; twin.prepare (numIn, numOut, block, sr, 0.0f, 1.0);

    std::mt19937 rng (5);
    std::uniform_real_distribution<float> d (-1.0f, 1.0f);
    const int blocks = 30;
    std::vector<float> in ((size_t) numIn * blocks * block);
    for (auto& v : in) v = d (rng);
    std::vector<float> gOut ((size_t) numOut * block), tOut ((size_t) numOut * block);
    std::vector<const float*> inPtrs ((size_t) numIn);
    std::vector<float*> gPtrs ((size_t) numOut), tPtrs ((size_t) numOut);

    float maxDiff = 0.0f;
    for (int b = 0; b < blocks; ++b)
    {
        const std::vector<float>& dM = (b < blocks / 2) ? dA : dB;
        const std::vector<float>& gM = (b < blocks / 2) ? gA : gB;
        backend.setMatrixPointers (dM.data(), gM.data(), nullptr, nullptr, nullptr, nullptr);

        for (int n = 0; n < numIn; ++n) inPtrs[(size_t) n] = in.data() + ((size_t) n * blocks + b) * block;
        for (int n = 0; n < numOut; ++n) { gPtrs[(size_t) n] = gOut.data() + (size_t) n * block;
                                           tPtrs[(size_t) n] = tOut.data() + (size_t) n * block; }
        backend.processBlock (inPtrs.data(), gPtrs.data());
        twin.processBlock (dM.data(), gM.data(), nullptr, nullptr, nullptr, nullptr, inPtrs.data(), tPtrs.data());

        if (b > 2)
            for (int n = 0; n < numOut; ++n)
                for (int s = 0; s < block; ++s)
                    maxDiff = std::max (maxDiff, std::abs (gOut[(size_t) n * block + s] - tOut[(size_t) n * block + s]));
    }
    char text[128];
    snprintf (text, sizeof (text), "delay+gain ramp across blocks (8x8x256), err=%.2e", maxDiff);
    check (maxDiff < 1e-4f, text);
}

//==============================================================================
// Diffusion determinism: two backends, same inputs + same FR-diffusion params,
// must produce identical output (the hash-noise grain is deterministic).
static void testDiffusionDeterminism()
{
    const int numIn = 4, numOut = 4, block = 256, blocks = 20;
    const double sr = 48000.0;
    const size_t pairs = (size_t) numIn * numOut;
    std::vector<float> delaysMs (pairs, 5.0f / 48.0f), gains (pairs, 0.0f); // direct off
    std::vector<float> frDelaysMs (pairs, 3.0f), frLevels (pairs, 0.5f), frHfDb (pairs, 0.0f);

    auto run = [&](std::vector<float>& out)
    {
        MetalObBackend be;
        if (! be.prepare (numIn, numOut, block, sr, 0.0, 1.0)) return false;
        be.setMatrixPointers (delaysMs.data(), gains.data(), nullptr,
                              frDelaysMs.data(), frLevels.data(), frHfDb.data());
        for (int in = 0; in < numIn; ++in) be.setFRDiffusion (in, 80.0f); // 80% diffusion
        std::mt19937 rng (123);
        std::uniform_real_distribution<float> d (-1.0f, 1.0f);
        std::vector<float> input ((size_t) numIn * blocks * block);
        for (auto& v : input) v = d (rng);
        out.assign ((size_t) numOut * blocks * block, 0.0f);
        std::vector<const float*> inPtrs ((size_t) numIn);
        std::vector<float*> outPtrs ((size_t) numOut);
        for (int b = 0; b < blocks; ++b)
        {
            for (int n = 0; n < numIn; ++n)  inPtrs[(size_t) n]  = input.data() + ((size_t) n * blocks + b) * block;
            for (int n = 0; n < numOut; ++n) outPtrs[(size_t) n] = out.data()   + ((size_t) n * blocks + b) * block;
            be.processBlock (inPtrs.data(), outPtrs.data());
        }
        return true;
    };

    std::vector<float> a, b;
    if (! run (a) || ! run (b)) { check (false, "diffusion determinism prepare"); return; }
    float maxDiff = 0.0f, peak = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) { maxDiff = std::max (maxDiff, std::abs (a[i] - b[i])); peak = std::max (peak, std::abs (a[i])); }
    char text[128];
    snprintf (text, sizeof (text), "FR diffusion is deterministic (run-to-run), diff=%.2e peak=%.2e", maxDiff, peak);
    check (maxDiff == 0.0f && peak > 1e-3f, text);
}

//==============================================================================
// reset(): after streaming non-silence, reset, then silence in must give
// silence out (accumulators cleared).
static void testReset()
{
    const int numIn = 4, numOut = 4, block = 128;
    const double sr = 48000.0;
    const size_t pairs = (size_t) numIn * numOut;
    std::vector<float> delaysMs (pairs, 10.0f / 48.0f), gains (pairs, 0.7f);

    MetalObBackend be;
    if (! be.prepare (numIn, numOut, block, sr, 0.0, 1.0)) { check (false, "reset prepare"); return; }
    be.setMatrixPointers (delaysMs.data(), gains.data(), nullptr, nullptr, nullptr, nullptr);

    std::mt19937 rng (3);
    std::uniform_real_distribution<float> d (-1.0f, 1.0f);
    std::vector<float> blk ((size_t) numIn * block);
    std::vector<float> out ((size_t) numOut * block);
    std::vector<const float*> inPtrs ((size_t) numIn);
    std::vector<float*> outPtrs ((size_t) numOut);
    for (int b = 0; b < 30; ++b)
    {
        for (auto& v : blk) v = d (rng);
        for (int n = 0; n < numIn; ++n)  inPtrs[(size_t) n]  = blk.data() + (size_t) n * block;
        for (int n = 0; n < numOut; ++n) outPtrs[(size_t) n] = out.data() + (size_t) n * block;
        be.processBlock (inPtrs.data(), outPtrs.data());
    }

    be.reset();
    std::vector<float> zeros ((size_t) numIn * block, 0.0f);
    out.assign ((size_t) numOut * block, 1.0f);
    for (int n = 0; n < numIn; ++n)  inPtrs[(size_t) n]  = zeros.data() + (size_t) n * block;
    for (int n = 0; n < numOut; ++n) outPtrs[(size_t) n] = out.data() + (size_t) n * block;
    // The accumulator may still hold contributions scattered before reset within
    // one block of look-ahead; stream a few silent blocks to flush, then check.
    float peak = 0.0f;
    for (int b = 0; b < 4; ++b)
    {
        be.processBlock (inPtrs.data(), outPtrs.data());
        peak = 0.0f;
        for (float v : out) peak = std::max (peak, std::abs (v));
    }
    char text[96];
    snprintf (text, sizeof (text), "reset clears accumulators (silence in -> silence out), peak=%.2e", peak);
    check (peak == 0.0f, text);
}

//==============================================================================
// Latency-clamp regression: with pipeline latency L, a tap whose raw delay is
// BELOW L clamps to the 1-sample floor. v1 BUG: the write-time scatter at d~0
// wrote into the just-read+cleared head cell, so the energy re-emerged only
// when the head wrapped (~1 s later). After the d >= 1 fix, the impulse must
// emit promptly (sample 1) and the first block must carry ~all of its energy.
static void testLatencyClampPrompt()
{
    const int block = 64;
    const double sr = 48000.0;
    const double latencyMs = 256.0 / 48.0;   // 256 samples (buffer 64 x depth 4)

    MetalObBackend be;
    if (! be.prepare (1, 1, block, sr, latencyMs, 1.0)) { check (false, "latency-clamp prepare"); return; }
    std::vector<float> delaysMs = { 2.0f };  // 96 samples raw < 256-sample L -> clamps to 1
    std::vector<float> gains = { 1.0f };
    be.setMatrixPointers (delaysMs.data(), gains.data(), nullptr, nullptr, nullptr, nullptr);

    std::vector<float> in (block, 0.0f), out (block, 0.0f);
    in[0] = 1.0f;
    const float* ip[1] = { in.data() };
    float* op[1] = { out.data() };
    be.processBlock (ip, op);

    float blockEnergy = 0.0f;
    for (float v : out) blockEnergy += std::abs (v);
    const bool prompt = std::abs (out[1] - 1.0f) < 1e-5f && std::abs (blockEnergy - 1.0f) < 1e-4f;

    // And verify nothing re-emerges ~1 s later (the v1 bug signature): stream
    // silence until the head wraps and confirm it stays silent.
    in[0] = 0.0f;
    float latePeak = 0.0f;
    const int wrapBlocks = (int) (sr / block) + 2;
    for (int b = 1; b < wrapBlocks; ++b)
    {
        be.processBlock (ip, op);
        for (float v : out) latePeak = std::max (latePeak, std::abs (v));
    }
    char text[128];
    snprintf (text, sizeof (text), "raw delay < L emits promptly, no ~1s echo (late peak=%.2e)", latePeak);
    check (prompt && latePeak == 0.0f, text);
}

//==============================================================================
// Timing gate at the field config that failed with v1: 32x27 @ block 64 with
// FR + diffusion enabled. The pump must beat the buffer-64 deadline (1.33 ms);
// v1 measured ~2.7 ms here (= ~350 underruns/s in the venue log).
static void testTimingGate()
{
    const int numIn = 32, numOut = 27, block = 64;
    const double sr = 48000.0;
    const double deadlineMs = 1000.0 * block / sr;   // 1.333 ms
    const size_t pairs = (size_t) numIn * numOut;

    std::vector<float> delaysMs (pairs), gains (pairs), hf (pairs), frDelaysMs (pairs), frLevels (pairs), frHf (pairs);
    for (size_t m = 0; m < pairs; ++m)
    {
        delaysMs[m]   = (10.0f + 30.0f * (float) ((m * 3) % 13) / 13.0f) / 48.0f;
        gains[m]      = 0.15f + 0.7f * (float) ((m * 5) % 7) / 7.0f;
        hf[m]         = -1.0f - 5.0f * (float) (m % 4) / 4.0f;
        frDelaysMs[m] = 1.0f + 4.0f * (float) (m % 5) / 5.0f;
        frLevels[m]   = 0.2f + 0.2f * (float) (m % 3) / 3.0f;   // FR on for ALL pairs (worst case)
        frHf[m]       = -2.0f * (float) (m % 3);
    }

    MetalObBackend be;
    if (! be.prepare (numIn, numOut, block, sr, 0.0, 1.0)) { check (false, "timing-gate prepare"); return; }
    be.setMatrixPointers (delaysMs.data(), gains.data(), hf.data(),
                          frDelaysMs.data(), frLevels.data(), frHf.data());
    for (int i = 0; i < numIn; ++i) be.setFRDiffusion (i, 60.0f);

    std::mt19937 rng (11);
    std::uniform_real_distribution<float> dist (-1.0f, 1.0f);
    std::vector<float> in ((size_t) numIn * block), out ((size_t) numOut * block);
    std::vector<const float*> ip ((size_t) numIn);
    std::vector<float*> op ((size_t) numOut);
    for (int n = 0; n < numIn; ++n)  ip[(size_t) n] = in.data() + (size_t) n * block;
    for (int n = 0; n < numOut; ++n) op[(size_t) n] = out.data() + (size_t) n * block;

    const int warmup = 20, measured = 300;
    double sumMs = 0.0, maxMs = 0.0;
    for (int b = 0; b < warmup + measured; ++b)
    {
        for (auto& v : in) v = dist (rng);
        be.processBlock (ip.data(), op.data());
        if (b >= warmup)
        {
            sumMs += be.getLastLaunchMs();
            maxMs = std::max (maxMs, be.getLastLaunchMs());
        }
    }
    const double avgMs = sumMs / measured;
    char text[160];
    snprintf (text, sizeof (text), "timing gate 32x27x64 FR+diffusion: avg %.3f / max %.3f ms (deadline %.2f)",
              avgMs, maxMs, deadlineMs);
    check (avgMs < deadlineMs, text);
}

//==============================================================================
int main()
{
    testClosedFormDelaySum();
    testTwin ("direct only, 0 dB shelf (8x8x256)",      8, 8, 256, 24, false, false, 1e-4f);
    testTwin ("direct + HF shelf (8x8x256)",            8, 8, 256, 24, true,  false, 2e-4f);
    testTwin ("direct + FR path (6x6x256)",             6, 6, 256, 24, true,  true,  2e-4f);
    testTwin ("direct + FR + 60% diffusion (6x6x256)",  6, 6, 256, 24, true,  true,  2e-4f, 60.0f);
    testTwin ("direct + FR + 100% diffusion (8x8x128)", 8, 8, 128, 24, true,  true,  2e-4f, 100.0f);
    testTwin ("wide 16x32x128",                        16, 32, 128, 24, true,  true,  2e-4f);
    testTwin ("narrow output 32x4x256 (occupancy)",    32, 4, 256, 24, true,  true,  2e-4f);
    testRamp();
    testDiffusionDeterminism();
    testReset();
    testLatencyClampPrompt();
    testTimingGate();

    printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL PASS" : "FAILURES",
            failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
