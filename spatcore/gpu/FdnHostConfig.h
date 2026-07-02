#pragma once
#if WFS_GPU_NATIVE

/*
    FdnHostConfig — backend-shared host math for the GPU FDN reverb (the FDN
    analogue of WfsFrHostState / IrConvHostState). JUCE-free so both the Metal
    and CUDA backends include it and only do the device upload + dispatch.

    Computes, byte-for-byte like FDNAlgorithm (Source/DSP/ReverbFDNAlgorithm.h):
      - STATIC per-node config at prepare() (fixed for the backend's lifetime,
        from fdnSize + the deterministic nodeHash): delay-line lengths, diffuser
        delays, feedback-allpass delays, output tap signs, and the ring strides.
      - DYNAMIC per-block coefficients on setParameters() (stepwise, no ramp):
        3-band decay gains per node/line, the two crossover coeffs, the
        diffusion coeff. toneCoeff is fixed at prepare (depends only on sr).

    fdnSize is captured at prepare() ONLY — matching the CPU, whose delay
    lengths are fixed in prepareNode() and never resized for a runtime fdnSize
    change (so a runtime fdnSize change is inert there too).
*/

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

class FdnHostConfig
{
public:
    static constexpr int   NUM_LINES = 16;
    static constexpr int   NUM_DIFFUSERS = 4;
    static constexpr int   MAX_DELAY_SAMPLES = 16384;
    static constexpr float REFERENCE_SAMPLE_RATE = 48000.0f;
    static constexpr float FEEDBACK_AP_COEFF = 0.55f;
    static constexpr float DC_POLE = 0.9995f;
    static constexpr float OUTPUT_GAIN = 4.0f;
    static constexpr float TWO_PI = 6.283185307179586f;

    // Static per-node config (sized [numNodes * NUM_LINES] etc.).
    std::vector<int>   delayLengths;
    std::vector<int>   diffuserDelays;   // [numNodes * NUM_DIFFUSERS]
    std::vector<int>   fbApDelays;
    std::vector<float> nodeTapSigns;

    // Dynamic per-block coefficients.
    std::vector<float> gainLow, gainMid, gainHigh;   // [numNodes * NUM_LINES]
    float lowCoeff = 0.0f;
    float highCoeff = 0.0f;
    float diffusionCoeff = 0.0f;
    float toneCoeff = 0.0f;

    int numNodes = 0;
    int maxDelayLen = 1;
    int maxDiffLen = 1;
    int maxFbApLen = 1;
    double sampleRate = 0.0;
    float rateScale = 1.0f;

    /** Computes the static config. Call once at prepare(). */
    void prepare (int numNodesIn, double sr, float fdnSize)
    {
        numNodes = std::max (1, numNodesIn);
        sampleRate = sr;
        rateScale = (float) (sr / (double) REFERENCE_SAMPLE_RATE);

        const size_t nl = (size_t) numNodes * NUM_LINES;
        const size_t nd = (size_t) numNodes * NUM_DIFFUSERS;
        delayLengths.assign (nl, 0);
        fbApDelays.assign (nl, 0);
        nodeTapSigns.assign (nl, 0.0f);
        diffuserDelays.assign (nd, 0);

        const float sizeScale = fdnSize * rateScale;
        maxDelayLen = 1; maxDiffLen = 1; maxFbApLen = 1;

        for (int node = 0; node < numNodes; ++node)
        {
            for (int i = 0; i < NUM_LINES; ++i)
            {
                // Delay lines: ±6.25% of base via hash (CPU prepareNode).
                int base = (int) (baseDelays[(size_t) i] * sizeScale);
                int range = std::max (1, base / 16);
                int off = (int) (nodeHash (node, i) % (uint32_t) (range * 2 + 1)) - range;
                int delay = clampi (base + off, 1, MAX_DELAY_SAMPLES);
                delayLengths[(size_t) (node * NUM_LINES + i)] = delay;
                maxDelayLen = std::max (maxDelayLen, delay);

                // Feedback allpass: small primes, ±20% (hash idx i+48).
                int fbBase = std::max (1, (int) (baseFeedbackAPDelays[(size_t) i] * rateScale));
                int fbRange = std::max (1, fbBase / 5);
                int fbOff = (int) (nodeHash (node, i + 48) % (uint32_t) (fbRange * 2 + 1)) - fbRange;
                int fbDelay = std::max (1, fbBase + fbOff);
                fbApDelays[(size_t) (node * NUM_LINES + i)] = fbDelay;
                maxFbApLen = std::max (maxFbApLen, fbDelay);

                // Output tap signs: rotate base by node, flip by hash (idx i+32).
                int rotated = (i + node) % NUM_LINES;
                float sign = (nodeHash (node, i + 32) & 1u) ? 1.0f : -1.0f;
                nodeTapSigns[(size_t) (node * NUM_LINES + i)] = outputTapSigns[(size_t) rotated] * sign;
            }

            for (int i = 0; i < NUM_DIFFUSERS; ++i)
            {
                // Diffusers: ±10% (hash idx i+16).
                int base = std::max (1, (int) (baseDiffuserDelays[(size_t) i] * rateScale));
                int range = std::max (1, base / 10);
                int off = (int) (nodeHash (node, i + NUM_LINES) % (uint32_t) (range * 2 + 1)) - range;
                int delay = std::max (1, base + off);
                diffuserDelays[(size_t) (node * NUM_DIFFUSERS + i)] = delay;
                maxDiffLen = std::max (maxDiffLen, delay);
            }
        }

        gainLow.assign (nl, 1.0f);
        gainMid.assign (nl, 1.0f);
        gainHigh.assign (nl, 1.0f);

        toneCoeff = 1.0f - std::exp (-TWO_PI * 8000.0f / (float) sr);

        // Initial coefficients from the CPU AlgorithmParameters defaults, so the
        // gains are sane from the first launch (mirrors FDNAlgorithm::prepare
        // calling recalculateDecayGains before any setParameters).
        setParameters (1.5f, 1.3f, 0.5f, 200.0f, 4000.0f, 0.5f);
    }

    /** Recomputes the per-block decay/diffusion coefficients (stepwise, like
        FDNAlgorithm::recalculateDecayGains / recalculateDiffusionCoeffs).
        fdnSize is ignored here on purpose — see the class note. */
    void setParameters (float rt60, float rt60LowMult, float rt60HighMult,
                        float crossoverLow, float crossoverHigh, float diffusion)
    {
        if (sampleRate <= 0.0)
            return;

        float rt60Low  = std::max (0.01f, rt60 * rt60LowMult);
        float rt60Mid  = std::max (0.01f, rt60);
        float rt60High = std::max (0.01f, rt60 * rt60HighMult);

        lowCoeff  = 1.0f - std::exp (-TWO_PI * crossoverLow  / (float) sampleRate);
        highCoeff = 1.0f - std::exp (-TWO_PI * crossoverHigh / (float) sampleRate);

        for (int node = 0; node < numNodes; ++node)
            for (int i = 0; i < NUM_LINES; ++i)
            {
                const size_t idx = (size_t) (node * NUM_LINES + i);
                float delaySec = (float) delayLengths[idx] / (float) sampleRate;
                gainLow[idx]  = std::pow (0.001f, delaySec / rt60Low);
                gainMid[idx]  = std::pow (0.001f, delaySec / rt60Mid);
                gainHigh[idx] = std::pow (0.001f, delaySec / rt60High);
            }

        diffusionCoeff = diffusion * 0.85f;
    }

    /** Global input-gain distribution (same for every node, CPU constant). */
    static const std::array<float, NUM_LINES>& getInputGains() { return inputGains; }

private:
    // Deterministic per-node hash (verbatim from FDNAlgorithm::nodeHash).
    static uint32_t nodeHash (int nodeIndex, int lineIndex)
    {
        uint32_t h = (uint32_t) (nodeIndex * 16 + lineIndex + 1);
        h ^= h >> 16;
        h *= 0x45d9f3bu;
        h ^= h >> 16;
        h *= 0x45d9f3bu;
        h ^= h >> 16;
        return h;
    }

    static int clampi (int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

    static constexpr std::array<int, NUM_LINES> baseDelays = {
        337, 389, 449, 521,  601, 701, 811, 941,
        1087, 1259, 1453, 1667,  1931, 2221, 2557, 2953
    };
    static constexpr std::array<int, NUM_DIFFUSERS> baseDiffuserDelays = { 142, 107, 379, 277 };
    static constexpr std::array<int, NUM_LINES> baseFeedbackAPDelays = {
        23, 29, 31, 37, 41, 43, 47, 53,  59, 61, 67, 71, 73, 79, 83, 89
    };
    static constexpr std::array<float, NUM_LINES> inputGains = {
        0.0710f, 0.0555f, 0.0680f, 0.0570f,  0.0640f, 0.0590f, 0.0660f, 0.0545f,
        0.0620f, 0.0700f, 0.0560f, 0.0650f,  0.0580f, 0.0690f, 0.0550f, 0.0600f
    };
    static constexpr std::array<float, NUM_LINES> outputTapSigns = {
         0.30f, -0.22f,  0.27f, -0.19f,  -0.26f,  0.23f, -0.28f,  0.21f,
         0.24f, -0.29f,  0.20f, -0.25f,  -0.22f,  0.26f, -0.21f,  0.27f
    };
};
#endif // WFS_GPU_NATIVE
