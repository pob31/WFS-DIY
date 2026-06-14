//
// Correctness + timing test for MetalSdnBackend (the GPU Scattering Delay
// Network reverb): drives it like the GpuAsyncPipelineT pump would and
// validates the streamed output sample-for-sample against a CPU reference that
// mirrors the synchronous SDNAlgorithm::processBlock (Source/DSP/ReverbSDNAlgorithm.h).
//
// Both the GPU backend and the reference build their per-path delays + decay
// coefficients + crossfade state from the SAME shared SdnHostConfig, driven with
// the same geometry/param sequence, so the config can't drift between them; the
// test isolates the GPU kernel's DSP correctness.
//
// Build:  ./build.sh    Run:  ./backend_test
//

#define WFS_GPU_NATIVE 1

#include "MetalSdnBackend.h"
#include "SdnHostConfig.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

static int failures = 0;
static void check (bool ok, const char* what)
{
    printf ("%-66s %s\n", what, ok ? "PASS" : "FAIL");
    if (! ok) ++failures;
}

//==============================================================================
// CPU reference — a direct transcription of the synchronous SDN per-sample DSP,
// reading its per-path delays/gains/crossfade from a shared SdnHostConfig. The
// harness advances the shared config's crossfade once per block (mirroring the
// backend's host-side advance), so the reference reads the same block-start state.
struct CpuSdnReference
{
    const SdnHostConfig& cfg;
    int N, P;
    uint32_t ringWritePos = 0;

    std::vector<std::vector<float>> delayLines;   // [P][MAXD]
    std::vector<float> decayLow, decayHigh;       // [P]
    std::vector<std::array<std::vector<float>, 2>> diff;  // [N][2]
    std::vector<std::array<int, 2>> diffWp;       // [N][2]
    std::vector<float> toneState, dcX1, dcY1;     // [N]

    explicit CpuSdnReference (const SdnHostConfig& c)
        : cfg (c), N (c.numNodes), P (std::max (1, c.numPaths))
    {
        const int MAXD = SdnHostConfig::MAX_DELAY_SAMPLES;
        delayLines.assign ((size_t) P, std::vector<float> ((size_t) MAXD, 0.0f));
        decayLow.assign ((size_t) P, 0.0f);
        decayHigh.assign ((size_t) P, 0.0f);
        diff.resize ((size_t) N);
        diffWp.assign ((size_t) N, { 0, 0 });
        for (int n = 0; n < N; ++n)
            for (int j = 0; j < 2; ++j)
                diff[(size_t) n][(size_t) j].assign ((size_t) cfg.diffuserDelays[(size_t) (n * 2 + j)], 0.0f);
        toneState.assign ((size_t) N, 0.0f);
        dcX1.assign ((size_t) N, 0.0f);
        dcY1.assign ((size_t) N, 0.0f);
    }

    void processBlock (const float* const* in, float* const* out, int numSamples)
    {
        const int MAXD = SdnHostConfig::MAX_DELAY_SAMPLES;
        for (int s = 0; s < numSamples; ++s)
        {
            const int base = (int) ringWritePos + s;
            for (int n = 0; n < N; ++n)
            {
                float incoming[16];
                float sumIn = 0.0f;
                int k = 0;
                for (int i = 0; i < N; ++i)
                {
                    if (i == n) continue;
                    const int pidx = SdnHostConfig::pathIndex (i, n, N);
                    const float mix = cfg.crossfadeMix[(size_t) pidx];
                    float val;
                    if (mix >= 1.0f)
                    {
                        int rp = (base - cfg.delayLength[(size_t) pidx]) % MAXD;
                        if (rp < 0) rp += MAXD;
                        val = delayLines[(size_t) pidx][(size_t) rp];
                    }
                    else
                    {
                        int orp = (base - cfg.delayLength[(size_t) pidx]) % MAXD;
                        if (orp < 0) orp += MAXD;
                        int nrp = (base - cfg.targetDelayLength[(size_t) pidx]) % MAXD;
                        if (nrp < 0) nrp += MAXD;
                        const float oldS = delayLines[(size_t) pidx][(size_t) orp];
                        const float newS = delayLines[(size_t) pidx][(size_t) nrp];
                        const float m = std::min (1.0f, mix + cfg.crossfadeRate * (float) s);
                        val = oldS * (1.0f - m) + newS * m;
                    }
                    incoming[k] = val;
                    sumIn += val;
                    ++k;
                }
                const int inCount = k;
                const float X = 2.0f / (float) (N - 1) * sumIn;

                float output = 0.0f;
                for (int i = 0; i < inCount; ++i)
                    output += (X - incoming[i]);

                float diffused = in[n][s];
                if (cfg.diffusionCoeff > 0.0001f)
                {
                    const float c = cfg.diffusionCoeff;
                    for (int j = 0; j < 2; ++j)
                    {
                        auto& buf = diff[(size_t) n][(size_t) j];
                        int& wp = diffWp[(size_t) n][(size_t) j];
                        const float delayed = buf[(size_t) wp];
                        const float v = diffused - c * delayed;
                        buf[(size_t) wp] = v;
                        if (++wp >= (int) buf.size()) wp = 0;
                        diffused = delayed + c * v;
                    }
                }

                const int writePos = base % MAXD;
                int outIdx = 0;
                for (int i = 0; i < N; ++i)
                {
                    if (i == n) continue;
                    const int pidx = SdnHostConfig::pathIndex (n, i, N);
                    const float signal = (X - incoming[outIdx]) + diffused * cfg.inputDistribution;
                    float lowS = decayLow[(size_t) pidx];
                    float highS = decayHigh[(size_t) pidx];
                    lowS  += cfg.lowCoeff  * (signal - lowS);
                    highS += cfg.highCoeff * (signal - highS);
                    const float low = lowS;
                    const float high = signal - highS;
                    const float mid = highS - lowS;
                    const float outSig = low * cfg.gainLow[(size_t) pidx]
                                       + mid * cfg.gainMid[(size_t) pidx]
                                       + high * cfg.gainHigh[(size_t) pidx];
                    decayLow[(size_t) pidx] = lowS;
                    decayHigh[(size_t) pidx] = highS;
                    delayLines[(size_t) pidx][(size_t) writePos] = outSig;
                    ++outIdx;
                }

                toneState[(size_t) n] += cfg.toneCoeff * (output - toneState[(size_t) n]);
                const float toned = toneState[(size_t) n];
                const float dcOut = toned - dcX1[(size_t) n] + SdnHostConfig::DC_POLE * dcY1[(size_t) n];
                dcX1[(size_t) n] = toned;
                dcY1[(size_t) n] = dcOut;
                out[n][s] = dcOut * cfg.sdnOutputGain;
            }
        }
        ringWritePos = (ringWritePos + (uint32_t) numSamples) % (uint32_t) MAXD;
    }
};

//==============================================================================
struct Params { float rt60, lowMult, highMult, xLow, xHigh, diffusion, scale; };

// A simple deterministic ring of node positions (meters).
static std::vector<SdnHostConfig::NodePos> ringGeometry (int n, float radius)
{
    std::vector<SdnHostConfig::NodePos> pos ((size_t) n);
    for (int i = 0; i < n; ++i)
    {
        const float a = 6.2831853f * (float) i / (float) n;
        pos[(size_t) i] = { radius * std::cos (a), radius * std::sin (a), 0.0f };
    }
    return pos;
}

static std::vector<float> flat (const std::vector<SdnHostConfig::NodePos>& p)
{
    std::vector<float> xyz (p.size() * 3);
    for (size_t i = 0; i < p.size(); ++i)
    { xyz[i*3+0] = p[i].x; xyz[i*3+1] = p[i].y; xyz[i*3+2] = p[i].z; }
    return xyz;
}

// Apply a param/geometry change to the reference config exactly like the backend
// consume path (recalcDelays -> recalcDecay -> setDiffusion).
static void applyToRefCfg (SdnHostConfig& cfg, const Params& pr,
                           const std::vector<SdnHostConfig::NodePos>* geo)
{
    if (geo != nullptr)
        cfg.recalcDelays (geo->data(), (int) geo->size(), pr.scale);
    else
        cfg.recalcDelaysFromStored (pr.scale);
    cfg.recalcDecay (pr.rt60, pr.lowMult, pr.highMult, pr.xLow, pr.xHigh);
    cfg.setDiffusion (pr.diffusion);
}

static void streamParity (int numNodes, int block, Params pr, float radius, const char* label)
{
    const double sr = 48000.0;

    SdnHostConfig cfg;
    cfg.prepare (numNodes, sr);
    auto geo = ringGeometry (numNodes, radius);
    applyToRefCfg (cfg, pr, &geo);
    CpuSdnReference ref (cfg);

    MetalSdnBackend gpu;
    if (! gpu.prepare (numNodes, block, sr))
    {
        fprintf (stderr, "prepare failed: %s\n", gpu.getLastError().c_str());
        check (false, label);
        return;
    }
    gpu.setGeometry (flat (geo).data(), numNodes);
    gpu.setParameters (pr.rt60, pr.lowMult, pr.highMult, pr.xLow, pr.xHigh, pr.diffusion, pr.scale);

    std::mt19937 rng (99 + numNodes * 7 + block);
    std::uniform_real_distribution<float> d (-1.0f, 1.0f);

    const int blocks = 64;
    std::vector<std::vector<float>> in ((size_t) numNodes), gpuOut ((size_t) numNodes), refOut ((size_t) numNodes);
    std::vector<const float*> inPtr ((size_t) numNodes);
    std::vector<float*> outPtr ((size_t) numNodes), refPtr ((size_t) numNodes);
    for (int n = 0; n < numNodes; ++n)
    {
        in[(size_t) n].resize ((size_t) block);
        gpuOut[(size_t) n].resize ((size_t) block);
        refOut[(size_t) n].resize ((size_t) block);
    }

    float maxDiff = 0.0f;
    double sumMs = 0.0, maxMs = 0.0;

    for (int b = 0; b < blocks; ++b)
    {
        for (int n = 0; n < numNodes; ++n)
        {
            for (int s = 0; s < block; ++s)
                in[(size_t) n][(size_t) s] = (b == 0 && n == 0 && s == 0) ? 1.0f : 0.05f * d (rng);
            inPtr[(size_t) n] = in[(size_t) n].data();
            outPtr[(size_t) n] = gpuOut[(size_t) n].data();
            refPtr[(size_t) n] = refOut[(size_t) n].data();
        }

        ref.processBlock (inPtr.data(), refPtr.data(), block);
        cfg.advanceCrossfades (block);

        if (! gpu.processBlock (inPtr.data(), outPtr.data()))
        {
            fprintf (stderr, "processBlock failed: %s\n", gpu.getLastError().c_str());
            check (false, label);
            return;
        }
        sumMs += gpu.getLastLaunchMs();
        maxMs = std::max (maxMs, gpu.getLastLaunchMs());

        for (int n = 0; n < numNodes; ++n)
            for (int s = 0; s < block; ++s)
                maxDiff = std::max (maxDiff, std::abs (refOut[(size_t) n][(size_t) s] - gpuOut[(size_t) n][(size_t) s]));
    }

    char text[180];
    snprintf (text, sizeof (text), "%s | err=%.2e, launch avg %.3f / max %.3f ms",
              label, maxDiff, sumMs / blocks, maxMs);
    check (maxDiff < 2e-4f, text);
}

//==============================================================================
static void testParamAndGeometryChange()
{
    const int numNodes = 8, block = 256;
    const double sr = 48000.0;
    Params pr { 2.0f, 1.3f, 0.5f, 200.0f, 4000.0f, 0.5f, 1.0f };

    SdnHostConfig cfg;
    cfg.prepare (numNodes, sr);
    auto geo = ringGeometry (numNodes, 3.0f);
    applyToRefCfg (cfg, pr, &geo);
    CpuSdnReference ref (cfg);

    MetalSdnBackend gpu;
    gpu.prepare (numNodes, block, sr);
    gpu.setGeometry (flat (geo).data(), numNodes);
    gpu.setParameters (pr.rt60, pr.lowMult, pr.highMult, pr.xLow, pr.xHigh, pr.diffusion, pr.scale);

    std::mt19937 rng (5);
    std::uniform_real_distribution<float> d (-1.0f, 1.0f);
    std::vector<std::vector<float>> in ((size_t) numNodes), gOut ((size_t) numNodes), rOut ((size_t) numNodes);
    std::vector<const float*> inPtr ((size_t) numNodes);
    std::vector<float*> gPtr ((size_t) numNodes), rPtr ((size_t) numNodes);
    for (int n = 0; n < numNodes; ++n)
    { in[(size_t) n].resize ((size_t) block); gOut[(size_t) n].resize ((size_t) block); rOut[(size_t) n].resize ((size_t) block); }

    float maxDiff = 0.0f;
    for (int b = 0; b < 120; ++b)
    {
        if (b == 40)  // params change on BOTH
        {
            pr = { 0.8f, 0.7f, 1.5f, 150.0f, 6000.0f, 0.9f, 1.0f };
            applyToRefCfg (cfg, pr, nullptr);
            gpu.setParameters (pr.rt60, pr.lowMult, pr.highMult, pr.xLow, pr.xHigh, pr.diffusion, pr.scale);
        }
        if (b == 70)  // geometry change (closer nodes -> shorter delays -> crossfade)
        {
            geo = ringGeometry (numNodes, 1.2f);
            applyToRefCfg (cfg, pr, &geo);
            gpu.setGeometry (flat (geo).data(), numNodes);
            gpu.setParameters (pr.rt60, pr.lowMult, pr.highMult, pr.xLow, pr.xHigh, pr.diffusion, pr.scale);
        }
        for (int n = 0; n < numNodes; ++n)
        {
            for (int s = 0; s < block; ++s) in[(size_t) n][(size_t) s] = 0.1f * d (rng);
            inPtr[(size_t) n] = in[(size_t) n].data();
            gPtr[(size_t) n] = gOut[(size_t) n].data();
            rPtr[(size_t) n] = rOut[(size_t) n].data();
        }

        ref.processBlock (inPtr.data(), rPtr.data(), block);
        cfg.advanceCrossfades (block);
        gpu.processBlock (inPtr.data(), gPtr.data());

        for (int n = 0; n < numNodes; ++n)
            for (int s = 0; s < block; ++s)
                maxDiff = std::max (maxDiff, std::abs (rOut[(size_t) n][(size_t) s] - gOut[(size_t) n][(size_t) s]));
    }
    char text[120];
    snprintf (text, sizeof (text), "mid-stream param + geometry/crossfade stays in parity, err=%.2e", maxDiff);
    check (maxDiff < 2e-4f, text);

    // Reset: silence in -> silence out (history cleared).
    gpu.requestReset();
    std::vector<float> zeros ((size_t) block, 0.0f);
    for (int n = 0; n < numNodes; ++n) { inPtr[(size_t) n] = zeros.data(); gPtr[(size_t) n] = gOut[(size_t) n].data(); std::fill (gOut[(size_t) n].begin(), gOut[(size_t) n].end(), 1.0f); }
    gpu.processBlock (inPtr.data(), gPtr.data());
    float peak = 0.0f;
    for (int n = 0; n < numNodes; ++n)
        for (float v : gOut[(size_t) n]) peak = std::max (peak, std::abs (v));
    char t2[120];
    snprintf (t2, sizeof (t2), "reset clears state (silence in -> silence out), peak=%.2e", peak);
    check (peak == 0.0f, t2);
}

//==============================================================================
static void testPassthrough()
{
    // N==1 must pass channel 0 straight through (the SDN cannot scatter).
    const int block = 128;
    MetalSdnBackend gpu;
    gpu.prepare (1, block, 48000.0);
    std::vector<float> in ((size_t) block), out ((size_t) block, 0.0f);
    std::mt19937 rng (3);
    std::uniform_real_distribution<float> d (-1.0f, 1.0f);
    for (int s = 0; s < block; ++s) in[(size_t) s] = d (rng);
    const float* inPtr[1] = { in.data() };
    float* outPtr[1] = { out.data() };
    gpu.processBlock (inPtr, outPtr);
    float maxDiff = 0.0f;
    for (int s = 0; s < block; ++s) maxDiff = std::max (maxDiff, std::abs (in[(size_t) s] - out[(size_t) s]));
    char t[96];
    snprintf (t, sizeof (t), "1 node passthrough (in == out), err=%.2e", maxDiff);
    check (maxDiff == 0.0f, t);
}

//==============================================================================
int main()
{
    streamParity (2,  256, { 1.5f, 1.3f, 0.5f, 200.0f, 4000.0f, 0.5f, 1.0f }, 3.0f, "2 nodes x 256, default params, far");
    streamParity (4,  256, { 2.5f, 1.5f, 0.4f, 250.0f, 3500.0f, 0.7f, 1.0f }, 3.0f, "4 nodes x 256, long RT60");
    streamParity (8,  256, { 1.5f, 1.3f, 0.5f, 200.0f, 4000.0f, 0.5f, 1.0f }, 0.8f, "8 nodes x 256, CLOSE nodes (delay < block)");
    streamParity (16, 256, { 1.5f, 1.3f, 0.5f, 200.0f, 4000.0f, 0.5f, 1.0f }, 4.0f, "16 nodes x 256 (full)");
    streamParity (16, 128, { 0.6f, 0.9f, 0.8f, 180.0f, 5000.0f, 0.3f, 2.0f }, 5.0f, "16 nodes x 128, short RT60, scale 2x");
    streamParity (8,  256, { 3.0f, 2.0f, 0.3f, 220.0f, 4500.0f, 0.0f, 1.0f }, 3.0f, "8 nodes x 256, diffusion OFF");

    testParamAndGeometryChange();
    testPassthrough();

    printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL PASS" : "FAILURES",
            failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
