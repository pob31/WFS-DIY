#pragma once
#if WFS_GPU_NATIVE

/*
    SdnHostConfig — backend-shared host math for the GPU SDN reverb (the SDN
    analogue of FdnHostConfig / WfsFrHostState). JUCE-free so both the Metal and
    CUDA backends include it and only do the device upload + dispatch.

    Computes, byte-for-byte like SDNAlgorithm (Source/DSP/ReverbSDNAlgorithm.h):
      - STATIC per-node config at prepare(): the two allpass diffuser delays
        ({142,277}*rateScale, identical for every node), the tone coeff, the
        output-gain compensation and the 1/N input distribution.
      - GEOMETRY-DRIVEN per-path delays via recalcDelays(): one inter-node delay
        line per ordered (a,b) pair, length = dist/c * sr * sdnScale, with the
        crossfade trigger when a length changes (SDNAlgorithm::
        recalculateDelaysFromGeometry).
      - DYNAMIC per-block coefficients via recalcDecay()/setDiffusion()
        (stepwise, no ramp): the 3-band decay gains per path (from the path's
        TARGET delay), the two crossover coeffs, and diffusion*0.5.
      - advanceCrossfades() ramps the per-path crossfade once per block, exactly
        like SDNAlgorithm's post-block loop; the kernel reads the block-start mix.

    Path layout matches SDNAlgorithm::getPathIndex: a node reads pathIndex(i,n)
    and writes pathIndex(n,i). The single ring write head (ringWritePos) and the
    modulo-MAX_DELAY_SAMPLES addressing are owned by the backend, not here.
*/

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

class SdnHostConfig
{
public:
    // spatcore capability bound: the SDN kernels' per-node scratch is float[32]
    // (Cuda/Metal SdnKernels.h, byte-frozen), so consumers must keep their node
    // count <= 32; backend prepare() clamps to this. Cost is O(N^2) — prefer
    // FDN/IR beyond it (docs/architecture/open-questions-audio.md Q6).
    static constexpr int   MAX_NODES = 32;
    static constexpr int   MAX_DELAY_SAMPLES = 8192;
    static constexpr int   NUM_DIFFUSERS = 2;
    static constexpr float SPEED_OF_SOUND = 343.0f;
    static constexpr float REFERENCE_SAMPLE_RATE = 48000.0f;
    static constexpr float DC_POLE = 0.9995f;
    static constexpr float TWO_PI = 6.283185307179586f;

    struct NodePos { float x = 0.0f, y = 0.0f, z = 0.0f; };

    // Geometry/dimensions.
    int numNodes = 0;
    int numPaths = 0;        // numNodes * (numNodes - 1)
    double sampleRate = 0.0;
    float rateScale = 1.0f;

    // Static per-node diffuser config [numNodes * NUM_DIFFUSERS].
    std::vector<int> diffuserDelays;
    int maxDiffLen = 1;

    // Per-path delay state [numPaths].
    std::vector<int>   delayLength;        // current (post-crossfade) delay
    std::vector<int>   targetDelayLength;  // crossfade destination
    std::vector<float> crossfadeMix;       // 0..1 (>=1 => settled)
    float crossfadeRate = 0.0f;            // global; 1/(sr*0.01) while crossfading

    // Per-path 3-band decay gains [numPaths] + global crossover coeffs.
    std::vector<float> gainLow, gainMid, gainHigh;
    float lowCoeff = 0.0f;
    float highCoeff = 0.0f;

    // Global scalars.
    float diffusionCoeff = 0.0f;     // diffusion * 0.5
    float toneCoeff = 0.0f;          // one-pole 8 kHz LPF
    float sdnOutputGain = 1.0f;      // (1 + 18/N) * 0.25 for N>=2
    float inputDistribution = 1.0f;  // 1/N

    // Last-seen geometry inputs (so an sdnScale-only param change can recompute
    // the delays, like SDNAlgorithm::setParameters).
    std::vector<NodePos> positions;
    float sdnScale = 1.0f;
    bool  haveGeometry = false;

    /** Path index identical to SDNAlgorithm::getPathIndex. */
    static int pathIndex (int from, int to, int n)
    {
        int idx = from * (n - 1);
        idx += (to > from) ? (to - 1) : to;
        return idx;
    }

    /** Static config. Call once at prepare(); delays stay at the default 1
        until recalcDelays() receives geometry (mirrors SDNAlgorithm::prepare,
        whose delays are 1 until updateGeometry runs). */
    void prepare (int numNodesIn, double sr)
    {
        numNodes = std::max (1, numNodesIn);
        sampleRate = sr;
        rateScale = (float) (sr / (double) REFERENCE_SAMPLE_RATE);
        numPaths = numNodes * (numNodes - 1);

        // 2 allpass diffusers per node at {142,277}*rateScale, same every node.
        static const int baseDiffDelays[NUM_DIFFUSERS] = { 142, 277 };
        diffuserDelays.assign ((size_t) numNodes * NUM_DIFFUSERS, 1);
        maxDiffLen = 1;
        for (int node = 0; node < numNodes; ++node)
            for (int i = 0; i < NUM_DIFFUSERS; ++i)
            {
                int d = std::max (1, (int) (baseDiffDelays[i] * rateScale));
                diffuserDelays[(size_t) (node * NUM_DIFFUSERS + i)] = d;
                maxDiffLen = std::max (maxDiffLen, d);
            }

        const size_t np = (size_t) std::max (1, numPaths);
        delayLength.assign (np, 1);
        targetDelayLength.assign (np, 1);
        crossfadeMix.assign (np, 1.0f);
        gainLow.assign (np, 1.0f);
        gainMid.assign (np, 1.0f);
        gainHigh.assign (np, 1.0f);
        crossfadeRate = 0.0f;

        toneCoeff = 1.0f - std::exp (-TWO_PI * 8000.0f / (float) sr);

        sdnOutputGain = numNodes >= 2 ? (1.0f + 18.0f / (float) numNodes) * 0.25f : 1.0f;
        inputDistribution = 1.0f / (float) numNodes;

        positions.clear();
        haveGeometry = false;
        sdnScale = 1.0f;

        // Initial coefficients from the CPU AlgorithmParameters defaults so the
        // gains are sane from the first launch (mirrors SDNAlgorithm::prepare
        // calling recalculateDecayGains/recalculateDiffusionCoeffs).
        recalcDecay (1.5f, 1.3f, 0.5f, 200.0f, 4000.0f);
        setDiffusion (0.5f);
    }

    /** Recomputes the per-path inter-node delays from geometry + sdnScale, and
        triggers a ~10 ms crossfade on any path whose length changed. Mirrors
        SDNAlgorithm::recalculateDelaysFromGeometry. */
    void recalcDelays (const NodePos* pos, int count, float scale)
    {
        positions.assign (pos, pos + count);
        sdnScale = scale;
        haveGeometry = (count >= numNodes);

        if (count < numNodes || numNodes < 2)
            return;

        const float cfRate = sampleRate > 0.0 ? 1.0f / (float) (sampleRate * 0.01) : 1.0f;

        for (int a = 0; a < numNodes; ++a)
            for (int b = 0; b < numNodes; ++b)
            {
                if (a == b) continue;

                const float dx = pos[a].x - pos[b].x;
                const float dy = pos[a].y - pos[b].y;
                const float dz = pos[a].z - pos[b].z;
                float dist = std::sqrt (dx * dx + dy * dy + dz * dz);
                dist = std::max (0.5f, dist);

                int d = (int) (dist / SPEED_OF_SOUND * (float) sampleRate * scale);
                d = std::min (std::max (d, 1), MAX_DELAY_SAMPLES - 1);

                const size_t idx = (size_t) pathIndex (a, b, numNodes);
                if (d != targetDelayLength[idx])
                {
                    targetDelayLength[idx] = d;
                    crossfadeMix[idx] = 0.0f;
                    crossfadeRate = cfRate;
                }
            }
    }

    /** Convenience: recompute the delays from the stored geometry (used when an
        sdnScale change must re-derive lengths but no new positions arrived). */
    void recalcDelaysFromStored (float scale)
    {
        if (haveGeometry)
            recalcDelays (positions.data(), (int) positions.size(), scale);
        else
            sdnScale = scale;
    }

    /** Recomputes the 3-band decay gains (from each path's TARGET delay) and the
        crossover coeffs. Mirrors SDNAlgorithm::recalculateDecayGains. */
    void recalcDecay (float rt60, float rt60LowMult, float rt60HighMult,
                      float crossoverLow, float crossoverHigh)
    {
        if (sampleRate <= 0.0 || numNodes < 2)
            return;

        const float rt60Low  = std::max (0.01f, rt60 * rt60LowMult);
        const float rt60Mid  = std::max (0.01f, rt60);
        const float rt60High = std::max (0.01f, rt60 * rt60HighMult);

        lowCoeff  = 1.0f - std::exp (-TWO_PI * crossoverLow  / (float) sampleRate);
        highCoeff = 1.0f - std::exp (-TWO_PI * crossoverHigh / (float) sampleRate);

        for (int a = 0; a < numNodes; ++a)
            for (int b = 0; b < numNodes; ++b)
            {
                if (a == b) continue;
                const size_t idx = (size_t) pathIndex (a, b, numNodes);
                const float delaySec = (float) targetDelayLength[idx] / (float) sampleRate;
                gainLow[idx]  = std::pow (0.001f, delaySec / rt60Low);
                gainMid[idx]  = std::pow (0.001f, delaySec / rt60Mid);
                gainHigh[idx] = std::pow (0.001f, delaySec / rt60High);
            }
    }

    /** diffusionCoeff = diffusion * 0.5 (SDNAlgorithm::recalculateDiffusionCoeffs). */
    void setDiffusion (float diffusion)
    {
        diffusionCoeff = diffusion * 0.5f;
    }

    /** Advances the per-path crossfade once per block, latching the new delay on
        completion (SDNAlgorithm::processBlock post-block loop). Returns true if
        any path was crossfading at entry (its uploaded state changed). */
    bool advanceCrossfades (int numSamples)
    {
        bool changed = false;
        for (int p = 0; p < numPaths; ++p)
        {
            if (crossfadeMix[(size_t) p] < 1.0f)
            {
                changed = true;
                crossfadeMix[(size_t) p] += crossfadeRate * (float) numSamples;
                if (crossfadeMix[(size_t) p] >= 1.0f)
                {
                    crossfadeMix[(size_t) p] = 1.0f;
                    delayLength[(size_t) p] = targetDelayLength[(size_t) p];
                }
            }
        }
        return changed;
    }
};
#endif // WFS_GPU_NATIVE
