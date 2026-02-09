#pragma once

#include "ReverbAlgorithm.h"
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
    static constexpr int MAX_DELAY_SAMPLES = 8192;
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
        for (int n = 0; n < numActiveNodes; ++n)
        {
            const float* input = nodeInputs.getReadPointer (n);
            float* output = nodeOutputs.getWritePointer (n);
            auto& node = nodes[static_cast<size_t> (n)];

            for (int s = 0; s < numSamples; ++s)
            {
                output[s] = processNodeSample (node, input[s]);
            }
        }
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
        509, 571, 631, 701,       // Short (early density)
        797, 887, 967, 1061,      // Medium
        1151, 1259, 1373, 1481,   // Long (modal density)
        1601, 1733, 1867, 1997    // Very long (LF support)
    };

    // Diffuser base delays at 48kHz
    static constexpr std::array<int, NUM_DIFFUSER_STAGES> baseDiffuserDelays = {
        142, 107, 379, 277
    };

    // Input gain distribution (slight ±1% variation around 1/16)
    static constexpr std::array<float, NUM_DELAY_LINES> inputGains = {
        0.0638f, 0.0613f, 0.0631f, 0.0619f,
        0.0625f, 0.0632f, 0.0618f, 0.0637f,
        0.0612f, 0.0638f, 0.0625f, 0.0619f,
        0.0631f, 0.0613f, 0.0637f, 0.0612f
    };

    // Output tap signs: alternating +/- at magnitude 1/4
    static constexpr std::array<float, NUM_DELAY_LINES> outputTapSigns = {
        0.25f, -0.25f,  0.25f, -0.25f,
        0.25f, -0.25f,  0.25f, -0.25f,
        0.25f, -0.25f,  0.25f, -0.25f,
        0.25f, -0.25f,  0.25f, -0.25f
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
            float low = lowState;
            float high = highState;
            float mid = input - low - high;
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

        // 4-stage allpass diffuser
        std::array<AllpassStage, NUM_DIFFUSER_STAGES> diffusers;

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

        for (int i = 0; i < NUM_DELAY_LINES; ++i)
        {
            int delay = juce::jlimit (1, MAX_DELAY_SAMPLES,
                static_cast<int> (baseDelays[static_cast<size_t> (i)] * sizeScale)
                + nodeIndex * 6);

            node.delayLengths[static_cast<size_t> (i)] = delay;
            node.delayLines[static_cast<size_t> (i)].assign (static_cast<size_t> (delay), 0.0f);
            node.writePositions[static_cast<size_t> (i)] = 0;
            node.decayFilters[static_cast<size_t> (i)].reset();
        }

        for (int i = 0; i < NUM_DIFFUSER_STAGES; ++i)
        {
            int delay = juce::jmax (1, static_cast<int> (
                baseDiffuserDelays[static_cast<size_t> (i)] * rateScale));
            node.diffusers[static_cast<size_t> (i)].prepare (delay);
        }

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

        // 3. Compute output from delay line outputs (before Hadamard)
        float output = 0.0f;
        for (int i = 0; i < NUM_DELAY_LINES; ++i)
        {
            auto si = static_cast<size_t> (i);
            output += node.hadamardBuf[si] * outputTapSigns[si];
        }

        // 4. Hadamard mixing
        hadamard16 (node.hadamardBuf);

        // 5. Apply decay and write back to delay lines
        for (int i = 0; i < NUM_DELAY_LINES; ++i)
        {
            auto si = static_cast<size_t> (i);
            float decayed = node.decayFilters[si].process (node.hadamardBuf[si]);
            float writeVal = decayed + diffused * inputGains[si];

            node.delayLines[si][static_cast<size_t> (node.writePositions[si])] = writeVal;
            node.writePositions[si] = (node.writePositions[si] + 1) % node.delayLengths[si];
        }

        // 6. DC blocker: y = x - x_prev + 0.9995 * y_prev
        float dcOut = output - node.dcX1 + 0.9995f * node.dcY1;
        node.dcX1 = output;
        node.dcY1 = dcOut;

        return dcOut;
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

        float sizeScale = currentParams.fdnSize * rateScale;

        for (auto& node : nodes)
        {
            for (int i = 0; i < NUM_DELAY_LINES; ++i)
            {
                auto si = static_cast<size_t> (i);
                float delaySec = (baseDelays[si] * sizeScale) / static_cast<float> (sr);

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
        diffusionCoeff = currentParams.diffusion * 0.7f;
    }

    //==========================================================================
    // State
    //==========================================================================

    double sr = 48000.0;
    float rateScale = 1.0f;
    int numActiveNodes = 0;
    float diffusionCoeff = 0.35f;
    AlgorithmParameters currentParams;
    std::vector<FDNNode> nodes;
};
