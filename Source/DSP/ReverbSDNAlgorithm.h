#pragma once

#include "ReverbAlgorithm.h"
#include "AudioParallelFor.h"
#include <array>
#include <cmath>

//==============================================================================
/**
    SDN (Scattering Delay Network) reverb algorithm.

    Nodes are interconnected via delay lines whose lengths are derived from
    physical distances between node positions. Each node scatters incoming
    energy to all other nodes using a Householder matrix. The result is a
    coherent reverberant field where spatial relationships are physically meaningful.

    N active nodes -> N*(N-1) inter-node delay lines.
*/
class SDNAlgorithm : public ReverbAlgorithm
{
public:
    static constexpr int MAX_NODES = 16;
    static constexpr int MAX_DELAY_SAMPLES = 8192;
    static constexpr float SPEED_OF_SOUND = 343.0f;
    static constexpr int NUM_DIFFUSERS_PER_NODE = 2;
    static constexpr float REFERENCE_SAMPLE_RATE = 48000.0f;

    //==========================================================================
    void prepare (double newSampleRate, int /*maxBlockSize*/, int numNodes) override
    {
        sr = newSampleRate;
        numActiveNodes = juce::jmin (numNodes, MAX_NODES);
        rateScale = static_cast<float> (sr / REFERENCE_SAMPLE_RATE);

        // Allocate inter-node paths: N*(N-1) paths (a -> b where a != b)
        int numPaths = numActiveNodes * (numActiveNodes - 1);
        paths.resize (static_cast<size_t> (numPaths));

        for (auto& path : paths)
            preparePath (path);

        // Allocate per-node diffusers
        nodeDiffusers.resize (static_cast<size_t> (numActiveNodes));
        for (int n = 0; n < numActiveNodes; ++n)
            prepareNodeDiffusers (nodeDiffusers[static_cast<size_t> (n)]);

        // Allocate per-node working buffers
        int bufSize = juce::jmax (1, numActiveNodes - 1);
        for (int n = 0; n < numActiveNodes; ++n)
        {
            incomingSignals[static_cast<size_t> (n)].resize (static_cast<size_t> (bufSize));
            scatteredSignals[static_cast<size_t> (n)].resize (static_cast<size_t> (bufSize));
        }

        // Apply current parameters to existing geometry
        if (! nodePositions.empty())
            recalculateDelaysFromGeometry();

        recalculateDecayGains();
        recalculateDiffusionCoeffs();
    }

    void reset() override
    {
        for (auto& path : paths)
            resetPath (path);

        for (auto& nd : nodeDiffusers)
            for (auto& d : nd)
                d.reset();
    }

    void processBlock (const juce::AudioBuffer<float>& nodeInputs,
                       juce::AudioBuffer<float>& nodeOutputs,
                       int numSamples) override
    {
        if (numActiveNodes < 2)
        {
            // With 0-1 nodes, SDN cannot scatter — just pass through
            if (numActiveNodes == 1)
            {
                const float* in = nodeInputs.getReadPointer (0);
                float* out = nodeOutputs.getWritePointer (0);
                for (int s = 0; s < numSamples; ++s)
                    out[s] = in[s];
            }
            return;
        }

        int N = numActiveNodes;

        // Snapshot all write positions for parallel reads
        for (auto& path : paths)
            path.readBasePos = path.writePos;

        // Process all nodes (parallel when pool available, sequential otherwise)
        auto processNode = [&] (int n)
        {
            auto sn = static_cast<size_t> (n);
            auto& incoming  = incomingSignals[sn];
            auto& scattered = scatteredSignals[sn];

            const float* inputData = nodeInputs.getReadPointer (n);
            float* outputData = nodeOutputs.getWritePointer (n);

            for (int s = 0; s < numSamples; ++s)
            {
                // 1. Read incoming from all paths leading to node n
                int inCount = 0;
                for (int i = 0; i < N; ++i)
                {
                    if (i == n) continue;
                    auto& path = paths[static_cast<size_t> (getPathIndex (i, n))];
                    incoming[static_cast<size_t> (inCount)] = readFromDelayAt (path, s);
                    inCount++;
                }

                // 2. Householder scattering: X = (2/(N-1)) * sum(incoming)
                float sum = 0.0f;
                for (int i = 0; i < inCount; ++i)
                    sum += incoming[static_cast<size_t> (i)];

                float X = (2.0f / static_cast<float> (N - 1)) * sum;

                for (int i = 0; i < inCount; ++i)
                    scattered[static_cast<size_t> (i)] = X - incoming[static_cast<size_t> (i)];

                // 3. Apply diffusion to node input
                float diffused = inputData[s];
                if (diffusionCoeff > 0.0001f)
                {
                    auto& nd = nodeDiffusers[sn];
                    for (auto& stage : nd)
                        diffused = stage.process (diffused, diffusionCoeff);
                }

                // 4. Write to outgoing delay lines (only this node writes to paths {n→*})
                float inputDistribution = 1.0f / static_cast<float> (N);
                int outIdx = 0;
                for (int i = 0; i < N; ++i)
                {
                    if (i == n) continue;
                    auto& path = paths[static_cast<size_t> (getPathIndex (n, i))];
                    float signal = scattered[static_cast<size_t> (outIdx)] + diffused * inputDistribution;
                    signal = path.decayFilter.process (signal);
                    int writeIdx = (path.readBasePos + s) % MAX_DELAY_SAMPLES;
                    path.delayLine[static_cast<size_t> (writeIdx)] = signal;
                    outIdx++;
                }

                // 5. Output = sum of all scattered signals
                float output = 0.0f;
                for (int i = 0; i < inCount; ++i)
                    output += scattered[static_cast<size_t> (i)];

                outputData[s] = output;
            }
        };

        if (parallel)
            parallel->parallelFor (N, processNode);
        else
            for (int n = 0; n < N; ++n)
                processNode (n);

        // Advance all writePos by numSamples (done once after parallel section)
        for (auto& path : paths)
            path.writePos = (path.readBasePos + numSamples) % MAX_DELAY_SAMPLES;

        // Finalize crossfade state for paths that were crossfading
        for (auto& path : paths)
        {
            if (path.crossfadeMix < 1.0f)
            {
                path.crossfadeMix += path.crossfadeRate * static_cast<float> (numSamples);
                if (path.crossfadeMix >= 1.0f)
                {
                    path.crossfadeMix = 1.0f;
                    path.delayLength = path.targetDelayLength;
                }
            }
        }
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
                             params.sdnScale != currentParams.sdnScale);

        bool scaleChanged = (params.sdnScale != currentParams.sdnScale);
        bool diffusionChanged = (params.diffusion != currentParams.diffusion);

        currentParams = params;

        if (scaleChanged && ! nodePositions.empty())
            recalculateDelaysFromGeometry();

        if (decayChanged)
            recalculateDecayGains();

        if (diffusionChanged)
            recalculateDiffusionCoeffs();
    }

    void updateGeometry (const std::vector<NodePosition>& positions) override
    {
        nodePositions = positions;
        recalculateDelaysFromGeometry();
        recalculateDecayGains();
    }

private:
    //==========================================================================
    // 3-band crossover decay filter (same as FDN)
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
    // Inter-node delay path
    //==========================================================================
    struct InterNodePath
    {
        std::vector<float> delayLine;
        int delayLength = 1;
        int writePos = 0;

        // Crossfade state for smooth delay changes
        int targetDelayLength = 1;
        float crossfadeMix = 1.0f;  // 0 = reading from old, 1 = reading from new
        float crossfadeRate = 0.0f;

        // Decay filter for this path
        DecayFilter decayFilter;

        // Snapshotted at block start for parallel reads
        int readBasePos = 0;
    };

    //==========================================================================
    // Path management
    //==========================================================================

    int getPathIndex (int fromNode, int toNode) const
    {
        // Map (from, to) pair to linear index in paths array
        // Skip the diagonal (from == to)
        int idx = fromNode * (numActiveNodes - 1);
        if (toNode > fromNode)
            idx += toNode - 1;
        else
            idx += toNode;
        return idx;
    }

    void preparePath (InterNodePath& path)
    {
        path.delayLine.assign (MAX_DELAY_SAMPLES, 0.0f);
        path.delayLength = 1;
        path.targetDelayLength = 1;
        path.writePos = 0;
        path.crossfadeMix = 1.0f;
        path.decayFilter.reset();
    }

    void resetPath (InterNodePath& path)
    {
        std::fill (path.delayLine.begin(), path.delayLine.end(), 0.0f);
        path.writePos = 0;
        path.decayFilter.reset();
    }

    void prepareNodeDiffusers (std::array<AllpassStage, NUM_DIFFUSERS_PER_NODE>& diffusers)
    {
        // SDN: 2 allpass diffusers per node at 142, 277 samples (48kHz reference)
        static constexpr std::array<int, NUM_DIFFUSERS_PER_NODE> baseDiffDelays = { 142, 277 };

        for (int i = 0; i < NUM_DIFFUSERS_PER_NODE; ++i)
        {
            int delay = juce::jmax (1, static_cast<int> (baseDiffDelays[static_cast<size_t> (i)] * rateScale));
            diffusers[static_cast<size_t> (i)].prepare (delay);
        }
    }

    //==========================================================================
    // Snapshot-based delay read (uses readBasePos, not live writePos)
    //==========================================================================

    float readFromDelayAt (const InterNodePath& path, int sampleOffset) const
    {
        if (path.crossfadeMix >= 1.0f)
        {
            int readPos = (path.readBasePos + sampleOffset - path.delayLength
                           + MAX_DELAY_SAMPLES) % MAX_DELAY_SAMPLES;
            return path.delayLine[static_cast<size_t> (readPos)];
        }
        else
        {
            int oldReadPos = (path.readBasePos + sampleOffset - path.delayLength
                              + MAX_DELAY_SAMPLES) % MAX_DELAY_SAMPLES;
            int newReadPos = (path.readBasePos + sampleOffset - path.targetDelayLength
                              + MAX_DELAY_SAMPLES) % MAX_DELAY_SAMPLES;

            float oldSample = path.delayLine[static_cast<size_t> (oldReadPos)];
            float newSample = path.delayLine[static_cast<size_t> (newReadPos)];

            // Compute crossfade mix for this sample without mutating state
            float mix = std::min (1.0f, path.crossfadeMix
                                        + path.crossfadeRate * static_cast<float> (sampleOffset));
            return oldSample * (1.0f - mix) + newSample * mix;
        }
    }

    //==========================================================================
    // Recalculate delay lengths from node geometry
    //==========================================================================

    void recalculateDelaysFromGeometry()
    {
        if (nodePositions.size() < static_cast<size_t> (numActiveNodes) || numActiveNodes < 2)
            return;

        // ~10ms crossfade at current sample rate
        float crossfadeRate = sr > 0 ? 1.0f / static_cast<float> (sr * 0.01) : 1.0f;

        for (int a = 0; a < numActiveNodes; ++a)
        {
            for (int b = 0; b < numActiveNodes; ++b)
            {
                if (a == b) continue;

                auto& posA = nodePositions[static_cast<size_t> (a)];
                auto& posB = nodePositions[static_cast<size_t> (b)];

                float dist = std::sqrt (
                    (posA.x - posB.x) * (posA.x - posB.x) +
                    (posA.y - posB.y) * (posA.y - posB.y) +
                    (posA.z - posB.z) * (posA.z - posB.z));

                // Minimum distance of 0.5m to avoid zero-length delays
                dist = juce::jmax (0.5f, dist);

                int delaySamples = juce::jlimit (1, MAX_DELAY_SAMPLES - 1,
                    static_cast<int> (dist / SPEED_OF_SOUND * static_cast<float> (sr) * currentParams.sdnScale));

                auto& path = paths[static_cast<size_t> (getPathIndex (a, b))];

                if (delaySamples != path.targetDelayLength)
                {
                    path.targetDelayLength = delaySamples;
                    path.crossfadeMix = 0.0f;
                    path.crossfadeRate = crossfadeRate;
                }
            }
        }
    }

    //==========================================================================
    // Recalculate decay filter gains
    //==========================================================================

    void recalculateDecayGains()
    {
        if (sr <= 0.0 || numActiveNodes < 2)
            return;

        float rt60Low  = juce::jmax (0.01f, currentParams.rt60 * currentParams.rt60LowMult);
        float rt60Mid  = juce::jmax (0.01f, currentParams.rt60);
        float rt60High = juce::jmax (0.01f, currentParams.rt60 * currentParams.rt60HighMult);

        float lowCoeff  = 1.0f - std::exp (-juce::MathConstants<float>::twoPi
                            * currentParams.crossoverLow / static_cast<float> (sr));
        float highCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi
                            * currentParams.crossoverHigh / static_cast<float> (sr));

        for (int a = 0; a < numActiveNodes; ++a)
        {
            for (int b = 0; b < numActiveNodes; ++b)
            {
                if (a == b) continue;

                auto& path = paths[static_cast<size_t> (getPathIndex (a, b))];

                // Use the target delay for decay calculation
                float delaySec = static_cast<float> (path.targetDelayLength) / static_cast<float> (sr);

                auto& f = path.decayFilter;
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
        // SDN uses more conservative diffusion than FDN (0.5 vs 0.7)
        diffusionCoeff = currentParams.diffusion * 0.5f;
    }

    //==========================================================================
    // State
    //==========================================================================

    double sr = 48000.0;
    float rateScale = 1.0f;
    int numActiveNodes = 0;
    float diffusionCoeff = 0.25f;
    AlgorithmParameters currentParams;

    std::vector<InterNodePath> paths;
    std::vector<std::array<AllpassStage, NUM_DIFFUSERS_PER_NODE>> nodeDiffusers;
    std::vector<NodePosition> nodePositions;

    // Per-node working buffers (thread-safe for parallel processing)
    std::array<std::vector<float>, MAX_NODES> incomingSignals;
    std::array<std::vector<float>, MAX_NODES> scatteredSignals;

    AudioParallelFor* parallel = nullptr;
};
