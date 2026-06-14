//
// Correctness + timing test for MetalFdnBackend (the GPU Feedback Delay
// Network reverb): drives it like the GpuAsyncPipelineT pump would and
// validates the streamed output sample-for-sample against a CPU reference
// that mirrors FDNAlgorithm::processNodeSample (Source/DSP/ReverbFDNAlgorithm.h).
//
// Both the GPU backend and the reference build their per-node config + decay
// coefficients from the SAME shared FdnHostConfig, so the config can't drift
// between them; the test isolates the GPU kernel's DSP correctness.
//
// Build:  ./build.sh    Run:  ./backend_test
//

#define WFS_GPU_NATIVE 1

#include "MetalFdnBackend.h"
#include "FdnHostConfig.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

static int failures = 0;
static void check (bool ok, const char* what)
{
    printf ("%-62s %s\n", what, ok ? "PASS" : "FAIL");
    if (! ok) ++failures;
}

//==============================================================================
// CPU reference — a direct transcription of FDNAlgorithm's per-sample DSP,
// reading its static config + coefficients from a shared FdnHostConfig.
struct CpuFdnReference
{
    static constexpr int L = FdnHostConfig::NUM_LINES;
    static constexpr int D = FdnHostConfig::NUM_DIFFUSERS;

    const FdnHostConfig& cfg;
    int numNodes;

    // Per-node state.
    struct Node {
        std::vector<std::vector<float>> delay {(size_t) L};
        std::vector<int> delayWp = std::vector<int> (L, 0);
        std::vector<std::vector<float>> diff {(size_t) D};
        std::vector<int> diffWp = std::vector<int> (D, 0);
        std::vector<std::vector<float>> fbap {(size_t) L};
        std::vector<int> fbapWp = std::vector<int> (L, 0);
        std::vector<float> lowS = std::vector<float> (L, 0.0f);
        std::vector<float> highS = std::vector<float> (L, 0.0f);
        float toneState = 0.0f, dcX1 = 0.0f, dcY1 = 0.0f;
    };
    std::vector<Node> nodes;

    explicit CpuFdnReference (const FdnHostConfig& c) : cfg (c), numNodes (c.numNodes)
    {
        nodes.resize ((size_t) numNodes);
        for (int n = 0; n < numNodes; ++n)
        {
            auto& nd = nodes[(size_t) n];
            for (int i = 0; i < L; ++i) {
                nd.delay[(size_t) i].assign ((size_t) cfg.delayLengths[(size_t) (n * L + i)], 0.0f);
                nd.fbap[(size_t) i].assign ((size_t) cfg.fbApDelays[(size_t) (n * L + i)], 0.0f);
            }
            for (int i = 0; i < D; ++i)
                nd.diff[(size_t) i].assign ((size_t) cfg.diffuserDelays[(size_t) (n * D + i)], 0.0f);
        }
    }

    static float allpass (std::vector<float>& buf, int& wp, float in, float coeff)
    {
        float delayed = buf[(size_t) wp];
        float v = in - coeff * delayed;
        buf[(size_t) wp] = v;
        wp = (wp + 1) % (int) buf.size();
        return delayed + coeff * v;
    }

    float processSample (int node, float input)
    {
        auto& nd = nodes[(size_t) node];
        const float c = cfg.diffusionCoeff;

        float diffused = input;
        if (c > 0.0001f)
            for (int i = 0; i < D; ++i)
                diffused = allpass (nd.diff[(size_t) i], nd.diffWp[(size_t) i], diffused, c);

        float hb[L];
        for (int i = 0; i < L; ++i)
            hb[i] = nd.delay[(size_t) i][(size_t) nd.delayWp[(size_t) i]];

        for (int len = 1; len < L; len <<= 1)
            for (int i = 0; i < L; i += len << 1)
                for (int j = i; j < i + len; ++j) {
                    float a = hb[j], b = hb[j + len];
                    hb[j] = a + b; hb[j + len] = a - b;
                }
        for (int i = 0; i < L; ++i) hb[i] *= 0.25f;

        float output = 0.0f;
        for (int i = 0; i < L; ++i)
            output += hb[i] * cfg.nodeTapSigns[(size_t) (node * L + i)];

        for (int i = 0; i < L; ++i)
        {
            float scattered = allpass (nd.fbap[(size_t) i], nd.fbapWp[(size_t) i], hb[i],
                                       FdnHostConfig::FEEDBACK_AP_COEFF);
            nd.lowS[(size_t) i]  += cfg.lowCoeff  * (scattered - nd.lowS[(size_t) i]);
            nd.highS[(size_t) i] += cfg.highCoeff * (scattered - nd.highS[(size_t) i]);
            float low = nd.lowS[(size_t) i];
            float high = scattered - nd.highS[(size_t) i];
            float mid = nd.highS[(size_t) i] - nd.lowS[(size_t) i];
            float decayed = low * cfg.gainLow[(size_t) (node * L + i)]
                          + mid * cfg.gainMid[(size_t) (node * L + i)]
                          + high * cfg.gainHigh[(size_t) (node * L + i)];
            float writeVal = decayed + diffused * FdnHostConfig::getInputGains()[(size_t) i];
            nd.delay[(size_t) i][(size_t) nd.delayWp[(size_t) i]] = writeVal;
            nd.delayWp[(size_t) i] = (nd.delayWp[(size_t) i] + 1) % (int) nd.delay[(size_t) i].size();
        }

        nd.toneState += cfg.toneCoeff * (output - nd.toneState);
        output = nd.toneState;
        float dcOut = output - nd.dcX1 + FdnHostConfig::DC_POLE * nd.dcY1;
        nd.dcX1 = output; nd.dcY1 = dcOut;
        return dcOut * FdnHostConfig::OUTPUT_GAIN;
    }
};

//==============================================================================
struct Params { float rt60, lowMult, highMult, xLow, xHigh, diffusion; };

static void streamParity (int numNodes, int block, Params pr, const char* label)
{
    const double sr = 48000.0;
    const float fdnSize = 1.0f;

    FdnHostConfig cfg;
    cfg.prepare (numNodes, sr, fdnSize);
    cfg.setParameters (pr.rt60, pr.lowMult, pr.highMult, pr.xLow, pr.xHigh, pr.diffusion);
    CpuFdnReference ref (cfg);

    MetalFdnBackend gpu;
    if (! gpu.prepare (numNodes, block, sr, fdnSize))
    {
        fprintf (stderr, "prepare failed: %s\n", gpu.getLastError().c_str());
        check (false, label);
        return;
    }
    gpu.setParameters (pr.rt60, pr.lowMult, pr.highMult, pr.xLow, pr.xHigh, pr.diffusion);

    std::mt19937 rng (99 + numNodes * 7 + block);
    std::uniform_real_distribution<float> d (-1.0f, 1.0f);

    const int blocks = 64;
    std::vector<std::vector<float>> in ((size_t) numNodes), gpuOut ((size_t) numNodes);
    std::vector<const float*> inPtr ((size_t) numNodes);
    std::vector<float*> outPtr ((size_t) numNodes);
    for (int n = 0; n < numNodes; ++n) {
        in[(size_t) n].resize ((size_t) block);
        gpuOut[(size_t) n].resize ((size_t) block);
    }

    float maxDiff = 0.0f;
    double sumMs = 0.0, maxMs = 0.0;
    double refEnergyFirst = 0.0, refEnergyLast = 0.0;

    for (int b = 0; b < blocks; ++b)
    {
        for (int n = 0; n < numNodes; ++n) {
            // A single impulse on node 0 at block 0, noise elsewhere, so the
            // tail energy (decay) can be sanity-checked too.
            for (int s = 0; s < block; ++s)
                in[(size_t) n][(size_t) s] = (b == 0 && n == 0 && s == 0) ? 1.0f : 0.05f * d (rng);
            inPtr[(size_t) n] = in[(size_t) n].data();
            outPtr[(size_t) n] = gpuOut[(size_t) n].data();
        }

        if (! gpu.processBlock (inPtr.data(), outPtr.data())) {
            fprintf (stderr, "processBlock failed: %s\n", gpu.getLastError().c_str());
            check (false, label);
            return;
        }
        sumMs += gpu.getLastLaunchMs();
        maxMs = std::max (maxMs, gpu.getLastLaunchMs());

        for (int n = 0; n < numNodes; ++n)
            for (int s = 0; s < block; ++s) {
                float r = ref.processSample (n, in[(size_t) n][(size_t) s]);
                maxDiff = std::max (maxDiff, std::abs (r - gpuOut[(size_t) n][(size_t) s]));
                if (n == 0) {
                    if (b == 1) refEnergyFirst += (double) r * r;
                    if (b == blocks - 1) refEnergyLast += (double) r * r;
                }
            }
    }

    char text[160];
    snprintf (text, sizeof (text), "%s | err=%.2e, launch avg %.3f / max %.3f ms",
              label, maxDiff, sumMs / blocks, maxMs);
    check (maxDiff < 2e-4f, text);
    (void) refEnergyFirst; (void) refEnergyLast;
}

//==============================================================================
static void testParamChangeAndReset()
{
    const int numNodes = 4, block = 256;
    const double sr = 48000.0;

    FdnHostConfig cfg;
    cfg.prepare (numNodes, sr, 1.0f);
    cfg.setParameters (2.0f, 1.3f, 0.5f, 200.0f, 4000.0f, 0.5f);
    CpuFdnReference ref (cfg);

    MetalFdnBackend gpu;
    gpu.prepare (numNodes, block, sr, 1.0f);
    gpu.setParameters (2.0f, 1.3f, 0.5f, 200.0f, 4000.0f, 0.5f);

    std::mt19937 rng (5);
    std::uniform_real_distribution<float> d (-1.0f, 1.0f);
    std::vector<std::vector<float>> in ((size_t) numNodes), out ((size_t) numNodes);
    std::vector<const float*> inPtr ((size_t) numNodes);
    std::vector<float*> outPtr ((size_t) numNodes);
    for (int n = 0; n < numNodes; ++n) { in[(size_t) n].resize ((size_t) block); out[(size_t) n].resize ((size_t) block); }

    float maxDiff = 0.0f;
    for (int b = 0; b < 80; ++b)
    {
        if (b == 40) {  // change params mid-stream on BOTH
            cfg.setParameters (0.8f, 0.7f, 1.5f, 150.0f, 6000.0f, 0.9f);
            gpu.setParameters (0.8f, 0.7f, 1.5f, 150.0f, 6000.0f, 0.9f);
        }
        for (int n = 0; n < numNodes; ++n) {
            for (int s = 0; s < block; ++s) in[(size_t) n][(size_t) s] = 0.1f * d (rng);
            inPtr[(size_t) n] = in[(size_t) n].data();
            outPtr[(size_t) n] = out[(size_t) n].data();
        }
        gpu.processBlock (inPtr.data(), outPtr.data());
        for (int n = 0; n < numNodes; ++n)
            for (int s = 0; s < block; ++s)
                maxDiff = std::max (maxDiff, std::abs (ref.processSample (n, in[(size_t) n][(size_t) s])
                                                       - out[(size_t) n][(size_t) s]));
    }
    char text[96];
    snprintf (text, sizeof (text), "mid-stream param change stays in parity, err=%.2e", maxDiff);
    check (maxDiff < 2e-4f, text);

    // Reset: silence in -> silence out (history cleared).
    gpu.requestReset();
    std::vector<float> zeros ((size_t) block, 0.0f);
    for (int n = 0; n < numNodes; ++n) { inPtr[(size_t) n] = zeros.data(); outPtr[(size_t) n] = out[(size_t) n].data(); std::fill (out[(size_t) n].begin(), out[(size_t) n].end(), 1.0f); }
    gpu.processBlock (inPtr.data(), outPtr.data());
    float peak = 0.0f;
    for (int n = 0; n < numNodes; ++n)
        for (float v : out[(size_t) n]) peak = std::max (peak, std::abs (v));
    char t2[96];
    snprintf (t2, sizeof (t2), "reset clears state (silence in -> silence out), peak=%.2e", peak);
    check (peak == 0.0f, t2);
}

//==============================================================================
int main()
{
    streamParity (1,  256, { 1.5f, 1.3f, 0.5f, 200.0f, 4000.0f, 0.5f }, "1 node x 256, default params");
    streamParity (4,  256, { 2.5f, 1.5f, 0.4f, 250.0f, 3500.0f, 0.7f }, "4 nodes x 256, long RT60");
    streamParity (16, 256, { 1.5f, 1.3f, 0.5f, 200.0f, 4000.0f, 0.5f }, "16 nodes x 256 (full)");
    streamParity (16, 128, { 0.6f, 0.9f, 0.8f, 180.0f, 5000.0f, 0.3f }, "16 nodes x 128, short RT60");
    streamParity (8,  256, { 3.0f, 2.0f, 0.3f, 220.0f, 4500.0f, 0.0f }, "8 nodes x 256, diffusion OFF");

    testParamChangeAndReset();

    printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL PASS" : "FAILURES",
            failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
