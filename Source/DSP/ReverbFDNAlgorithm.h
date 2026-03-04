#pragma once

#include "ReverbAlgorithm.h"
#include "AudioParallelFor.h"
#include <array>
#include <cmath>

//==============================================================================
/**
    FDN (Feedback Delay Network) reverb algorithm.

    Each node runs an independent 16-line FDN. No inter-node coupling.
    Spatial distribution comes from the existing WFS feed/return infrastructure.

    Per node:
    - 4-stage cascade allpass diffusion on input
    - 16 prime-length delay lines
    - Walsh-Hadamard 16-point mixing matrix
    - 3-band frequency-dependent decay per line
    - DC blocker on output
*/
class FDNAlgorithm : public ReverbAlgorithm
{
public:
    static constexpr int NUM_DELAY_LINES = 16;
    static constexpr int MAX_DELAY_SAMPLES = 16384;
    static constexpr int NUM_DIFFUSER_STAGES = 4;
    static constexpr float REFERENCE_SAMPLE_RATE = 48000.0f;

    //==========================================================================
    void prepare (double newSampleRate, int /*maxBlockSize*/, int numNodes) override
    {
        sr = newSampleRate;
        numActiveNodes = numNodes;
        rateScale = static_cast<float> (sr / REFERENCE_SAMPLE_RATE);

        nodes.resize (static_cast<size_t> (numNodes));
        for (int n = 0; n < numNodes; ++n)
            prepareNode (nodes[static_cast<size_t> (n)], n);

        // Output tone filter: one-pole LPF at ~8kHz
        toneCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi
                        * 8000.0f / static_cast<float> (sr));

        // Apply current parameters
        recalculateDecayGains();
        recalculateDiffusionCoeffs();
    }

    void reset() override
    {
        for (auto& node : nodes)
            resetNode (node);
    }

    void processBlock (const juce::AudioBuffer<float>& nodeInputs,
                       juce::AudioBuffer<float>& nodeOutputs,
                       int numSamples) override
    {
        auto processNode = [&] (int n)
        {
            const float* input = nodeInputs.getReadPointer (n);
            float* output = nodeOutputs.getWritePointer (n);
            auto& node = nodes[static_cast<size_t> (n)];

            for (int s = 0; s < numSamples; ++s)
                output[s] = processNodeSample (node, input[s]);
        };

        if (parallel)
            parallel->parallelFor (numActiveNodes, processNode);
        else
            for (int n = 0; n < numActiveNodes; ++n)
                processNode (n);
    }

    void setParallelFor (AudioParallelFor* pool) override
    {
        parallel = pool;
    }

    void setParameters (const AlgorithmParameters& params) override
    {
        bool decayChanged = (params.rt60 != currentParams.rt60 ||
                             params.rt60LowMult != currentParams.rt60LowMult ||
                             params.rt60HighMult != currentParams.rt60HighMult ||
                             params.crossoverLow != currentParams.crossoverLow ||
                             params.crossoverHigh != currentParams.crossoverHigh ||
                             params.fdnSize != currentParams.fdnSize);

        bool diffusionChanged = (params.diffusion != currentParams.diffusion);

        currentParams = params;

        if (decayChanged)
            recalculateDecayGains();

        if (diffusionChanged)
            recalculateDiffusionCoeffs();
    }

    void updateGeometry (const std::vector<NodePosition>&) override
    {
        // FDN does not use geometry (no inter-node coupling)
    }

private:
    //==========================================================================
    // Base delay lengths (primes at 48kHz)
    //==========================================================================
    static constexpr std::array<int, NUM_DELAY_LINES> baseDelays = {
        337, 389, 449, 521,       // Short  (logarithmic spacing, ratio ~1.155)
        601, 701, 811, 941,       // Medium
        1087, 1259, 1453, 1667,   // Long
        1931, 2221, 2557, 2953    // Very long
    };

    // Diffuser base delays at 48kHz
    static constexpr std::array<int, NUM_DIFFUSER_STAGES> baseDiffuserDelays = {
        142, 107, 379, 277
    };

    // Feedback allpass base delays (small primes at 48kHz, one per delay line)
    static constexpr std::array<int, NUM_DELAY_LINES> baseFeedbackAPDelays = {
        23, 29, 31, 37, 41, 43, 47, 53,
        59, 61, 67, 71, 73, 79, 83, 89
    };
    static constexpr float feedbackAPCoeff = 0.55f;

    // Input gain distribution (±12% variation around 1/16 to break modal symmetry)
    static constexpr std::array<float, NUM_DELAY_LINES> inputGains = {
        0.0710f, 0.0555f, 0.0680f, 0.0570f,
        0.0640f, 0.0590f, 0.0660f, 0.0545f,
        0.0620f, 0.0700f, 0.0560f, 0.0650f,
        0.0580f, 0.0690f, 0.0550f, 0.0600f
    };

    // Output tap signs: varied magnitudes and irregular sign pattern
    // to break comb-filter regularity (sum-of-squares ≈ 1.0)
    static constexpr std::array<float, NUM_DELAY_LINES> outputTapSigns = {
         0.30f, -0.22f,  0.27f, -0.19f,
        -0.26f,  0.23f, -0.28f,  0.21f,
         0.24f, -0.29f,  0.20f, -0.25f,
        -0.22f,  0.26f, -0.21f,  0.27f
    };

    //==========================================================================
    // 3-band crossover decay filter
    //==========================================================================
    struct DecayFilter
    {
        float lowState = 0.0f;
        float highState = 0.0f;
        float gainLow = 1.0f;
        float gainMid = 1.0f;
        float gainHigh = 1.0f;
        float lowCoeff = 0.0f;
        float highCoeff = 0.0f;

        void reset()
        {
            lowState = 0.0f;
            highState = 0.0f;
        }

        float process (float input)
        {
            lowState += lowCoeff * (input - lowState);
            highState += highCoeff * (input - highState);
            float low = lowState;                // LPF at crossoverLow = bass
            float high = input - highState;      // HPF at crossoverHigh = treble
            float mid = highState - lowState;    // bandpass = midrange
            return low * gainLow + mid * gainMid + high * gainHigh;
        }
    };

    //==========================================================================
    // Allpass diffuser stage
    //==========================================================================
    struct AllpassStage
    {
        std::vector<float> buffer;
        int writePos = 0;
        int delaySamples = 0;

        void prepare (int delay)
        {
            delaySamples = delay;
            buffer.assign (static_cast<size_t> (delay), 0.0f);
            writePos = 0;
        }

        void reset()
        {
            std::fill (buffer.begin(), buffer.end(), 0.0f);
            writePos = 0;
        }

        float process (float input, float coeff)
        {
            if (delaySamples <= 0)
                return input;

            float delayed = buffer[static_cast<size_t> (writePos)];
            float v = input - coeff * delayed;
            buffer[static_cast<size_t> (writePos)] = v;
            writePos = (writePos + 1) % delaySamples;
            return delayed + coeff * v;
        }
    };

    //==========================================================================
    // Deterministic hash for per-node variation
    //==========================================================================
    static uint32_t nodeHash (int nodeIndex, int lineIndex)
    {
        uint32_t h = static_cast<uint32_t> (nodeIndex * 16 + lineIndex + 1);
        h ^= h >> 16;
        h *= 0x45d9f3bu;
        h ^= h >> 16;
        h *= 0x45d9f3bu;
        h ^= h >> 16;
        return h;
    }

    //==========================================================================
    // Per-node FDN state
    //==========================================================================
    struct FDNNode
    {
        // 16 circular-buffer delay lines
        std::array<std::vector<float>, NUM_DELAY_LINES> delayLines;
        std::array<int, NUM_DELAY_LINES> delayLengths {};
        std::array<int, NUM_DELAY_LINES> writePositions {};

        // 3-band decay filter per delay line
        std::array<DecayFilter, NUM_DELAY_LINES> decayFilters;

        // 4-stage allpass diffuser (input)
        std::array<AllpassStage, NUM_DIFFUSER_STAGES> diffusers;

        // Per-delay-line allpass in feedback path (mode scattering)
        std::array<AllpassStage, NUM_DELAY_LINES> feedbackAllpass;

        // Per-node output tap signs (rotated/shuffled per node)
        std::array<float, NUM_DELAY_LINES> nodeTapSigns {};

        // Output tone filter state (one-pole LPF to soften harshness)
        float toneState = 0.0f;

        // DC blocker state
        float dcX1 = 0.0f;
        float dcY1 = 0.0f;

        // Working buffer for Hadamard
        std::array<float, NUM_DELAY_LINES> hadamardBuf {};
    };

    //==========================================================================
    // Node preparation and reset
    //==========================================================================

    void prepareNode (FDNNode& node, int nodeIndex)
    {
        float sizeScale = currentParams.fdnSize * rateScale;

        // Per-node delay line variation: ±6% of base delay using deterministic hash
        for (int i = 0; i < NUM_DELAY_LINES; ++i)
        {
            auto si = static_cast<size_t> (i);
            int baseDelay = static_cast<int> (baseDelays[si] * sizeScale);
            int range = juce::jmax (1, baseDelay / 16);  // ±6.25% variation
            int offset = static_cast<int> (nodeHash (nodeIndex, i) % static_cast<uint32_t> (range * 2 + 1)) - range;
            int delay = juce::jlimit (1, MAX_DELAY_SAMPLES, baseDelay + offset);

            node.delayLengths[si] = delay;
            node.delayLines[si].assign (static_cast<size_t> (delay), 0.0f);
            node.writePositions[si] = 0;
            node.decayFilters[si].reset();
        }

        // Per-node diffuser variation: ±10% of base diffuser delay
        for (int i = 0; i < NUM_DIFFUSER_STAGES; ++i)
        {
            auto si = static_cast<size_t> (i);
            int baseDelay = juce::jmax (1, static_cast<int> (baseDiffuserDelays[si] * rateScale));
            int range = juce::jmax (1, baseDelay / 10);  // ±10% variation
            int offset = static_cast<int> (nodeHash (nodeIndex, i + NUM_DELAY_LINES) % static_cast<uint32_t> (range * 2 + 1)) - range;
            int delay = juce::jmax (1, baseDelay + offset);
            node.diffusers[si].prepare (delay);
        }

        // Per-node feedback allpass: small primes with per-node variation
        for (int i = 0; i < NUM_DELAY_LINES; ++i)
        {
            auto si = static_cast<size_t> (i);
            int baseDelay = juce::jmax (1, static_cast<int> (baseFeedbackAPDelays[si] * rateScale));
            int range = juce::jmax (1, baseDelay / 5);  // ±20% variation
            int offset = static_cast<int> (nodeHash (nodeIndex, i + 48) % static_cast<uint32_t> (range * 2 + 1)) - range;
            int delay = juce::jmax (1, baseDelay + offset);
            node.feedbackAllpass[si].prepare (delay);
        }

        // Per-node output tap signs: rotate the base array by nodeIndex positions
        // and flip signs based on hash to decorrelate node outputs
        for (int i = 0; i < NUM_DELAY_LINES; ++i)
        {
            auto si = static_cast<size_t> (i);
            int rotated = (i + nodeIndex) % NUM_DELAY_LINES;
            float sign = (nodeHash (nodeIndex, i + 32) & 1u) ? 1.0f : -1.0f;
            node.nodeTapSigns[si] = outputTapSigns[static_cast<size_t> (rotated)] * sign;
        }

        node.toneState = 0.0f;
        node.dcX1 = 0.0f;
        node.dcY1 = 0.0f;
    }

    void resetNode (FDNNode& node)
    {
        for (int i = 0; i < NUM_DELAY_LINES; ++i)
        {
            std::fill (node.delayLines[static_cast<size_t> (i)].begin(),
                       node.delayLines[static_cast<size_t> (i)].end(), 0.0f);
            node.writePositions[static_cast<size_t> (i)] = 0;
            node.decayFilters[static_cast<size_t> (i)].reset();
        }

        for (auto& d : node.diffusers)
            d.reset();

        for (auto& ap : node.feedbackAllpass)
            ap.reset();

        node.toneState = 0.0f;
        node.dcX1 = 0.0f;
        node.dcY1 = 0.0f;
    }

    //==========================================================================
    // Walsh-Hadamard 16-point in-place transform
    //==========================================================================

    static void hadamard16 (std::array<float, NUM_DELAY_LINES>& out)
    {
        for (int len = 1; len < NUM_DELAY_LINES; len <<= 1)
        {
            for (int i = 0; i < NUM_DELAY_LINES; i += len << 1)
            {
                for (int j = i; j < i + len; ++j)
                {
                    float a = out[static_cast<size_t> (j)];
                    float b = out[static_cast<size_t> (j + len)];
                    out[static_cast<size_t> (j)]       = a + b;
                    out[static_cast<size_t> (j + len)] = a - b;
                }
            }
        }

        // Scale by 1/sqrt(16) = 0.25
        for (auto& v : out)
            v *= 0.25f;
    }

    //==========================================================================
    // Process one sample through a node
    //==========================================================================

    float processNodeSample (FDNNode& node, float input)
    {
        // 1. Allpass diffusion cascade
        float diffused = input;
        if (diffusionCoeff > 0.0001f)
        {
            for (auto& stage : node.diffusers)
                diffused = stage.process (diffused, diffusionCoeff);
        }

        // 2. Read from 16 delay lines
        for (int i = 0; i < NUM_DELAY_LINES; ++i)
        {
            auto si = static_cast<size_t> (i);
            int readPos = node.writePositions[si] - node.delayLengths[si];
            if (readPos < 0)
                readPos += node.delayLengths[si];
            // Use modulo for safety
            readPos = ((readPos % node.delayLengths[si]) + node.delayLengths[si]) % node.delayLengths[si];

            node.hadamardBuf[si] = node.delayLines[si][static_cast<size_t> (readPos)];
        }

        // 3. Hadamard mixing (before output tapping for better diffusion)
        hadamard16 (node.hadamardBuf);

        // 4. Compute output from post-Hadamard signal (more diffuse)
        float output = 0.0f;
        for (int i = 0; i < NUM_DELAY_LINES; ++i)
        {
            auto si = static_cast<size_t> (i);
            output += node.hadamardBuf[si] * node.nodeTapSigns[si];
        }

        // 5. Feedback allpass + decay and write back to delay lines
        for (int i = 0; i < NUM_DELAY_LINES; ++i)
        {
            auto si = static_cast<size_t> (i);
            float scattered = node.feedbackAllpass[si].process (node.hadamardBuf[si], feedbackAPCoeff);
            float decayed = node.decayFilters[si].process (scattered);
            float writeVal = decayed + diffused * inputGains[si];

            node.delayLines[si][static_cast<size_t> (node.writePositions[si])] = writeVal;
            node.writePositions[si] = (node.writePositions[si] + 1) % node.delayLengths[si];
        }

        // 6. Output tone filter (one-pole LPF ~8kHz to soften harshness)
        node.toneState += toneCoeff * (output - node.toneState);
        output = node.toneState;

        // 7. DC blocker: y = x - x_prev + 0.9995 * y_prev
        float dcOut = output - node.dcX1 + 0.9995f * node.dcY1;
        node.dcX1 = output;
        node.dcY1 = dcOut;

        return dcOut * 4.0f;  // +12dB output level correction
    }

    //==========================================================================
    // Recalculate decay filter gains from parameters
    //==========================================================================

    void recalculateDecayGains()
    {
        if (sr <= 0.0)
            return;

        float rt60Low  = currentParams.rt60 * currentParams.rt60LowMult;
        float rt60Mid  = currentParams.rt60;
        float rt60High = currentParams.rt60 * currentParams.rt60HighMult;

        // Prevent division by zero
        rt60Low  = juce::jmax (0.01f, rt60Low);
        rt60Mid  = juce::jmax (0.01f, rt60Mid);
        rt60High = juce::jmax (0.01f, rt60High);

        float lowCoeff  = 1.0f - std::exp (-juce::MathConstants<float>::twoPi
                            * currentParams.crossoverLow / static_cast<float> (sr));
        float highCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi
                            * currentParams.crossoverHigh / static_cast<float> (sr));

        for (auto& node : nodes)
        {
            for (int i = 0; i < NUM_DELAY_LINES; ++i)
            {
                auto si = static_cast<size_t> (i);
                float delaySec = static_cast<float> (node.delayLengths[si]) / static_cast<float> (sr);

                auto& f = node.decayFilters[si];
                f.lowCoeff  = lowCoeff;
                f.highCoeff = highCoeff;
                f.gainLow  = std::pow (0.001f, delaySec / rt60Low);
                f.gainMid  = std::pow (0.001f, delaySec / rt60Mid);
                f.gainHigh = std::pow (0.001f, delaySec / rt60High);
            }
        }
    }

    //==========================================================================
    // Recalculate diffusion coefficients
    //==========================================================================

    void recalculateDiffusionCoeffs()
    {
        diffusionCoeff = currentParams.diffusion * 0.85f;
    }

    //==========================================================================
    // State
    //==========================================================================

    double sr = 48000.0;
    float rateScale = 1.0f;
    int numActiveNodes = 0;
    float diffusionCoeff = 0.35f;
    float toneCoeff = 0.65f;  // one-pole LPF coefficient for output tone filter
    AlgorithmParameters currentParams;
    std::vector<FDNNode> nodes;
    AudioParallelFor* parallel = nullptr;
};
