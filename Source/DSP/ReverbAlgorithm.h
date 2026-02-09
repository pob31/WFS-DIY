#pragma once

#include <JuceHeader.h>
#include <vector>

class AudioParallelFor;

//==============================================================================
/**
    Parameters shared by all reverb algorithms.
    Set from the 50Hz timer via ReverbEngine::setAlgorithmParameters().
*/
struct AlgorithmParameters
{
    float rt60         = 1.5f;    // Mid-frequency decay time (seconds)
    float rt60LowMult  = 1.3f;   // LF decay multiplier
    float rt60HighMult = 0.5f;   // HF decay multiplier
    float crossoverLow  = 200.0f;  // Low/mid crossover (Hz)
    float crossoverHigh = 4000.0f; // Mid/high crossover (Hz)
    float diffusion    = 0.5f;    // Allpass diffusion amount (0-1)
    float sdnScale     = 1.0f;   // SDN inter-node delay multiplier
    float fdnSize      = 1.0f;   // FDN delay line size multiplier
    float wetLevel     = 1.0f;   // Output level (linear, converted from dB)
};

//==============================================================================
/**
    3D position for a reverb node.
*/
struct NodePosition
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

//==============================================================================
/**
    Abstract base class for reverb algorithm implementations (FDN, SDN, IR).

    Each algorithm receives pre-processed per-node audio and produces
    per-node wet reverb output. The engine handles pre/post processing
    and ring buffer I/O around this interface.
*/
class ReverbAlgorithm
{
public:
    virtual ~ReverbAlgorithm() = default;

    /** Prepare for playback. Allocate all buffers. */
    virtual void prepare (double sampleRate, int maxBlockSize, int numNodes) = 0;

    /** Reset all internal state (delay lines, filters) to silence. */
    virtual void reset() = 0;

    /**
        Process one block of audio.

        @param nodeInputs   Buffer with numNodes channels of pre-processed input.
        @param nodeOutputs  Buffer with numNodes channels for wet output (cleared before call).
        @param numSamples   Number of samples to process in this block.
    */
    virtual void processBlock (const juce::AudioBuffer<float>& nodeInputs,
                               juce::AudioBuffer<float>& nodeOutputs,
                               int numSamples) = 0;

    /** Update algorithm parameters. Called from the engine at control rate. */
    virtual void setParameters (const AlgorithmParameters& params) = 0;

    /** Update node geometry. Called when node count or positions change.
        SDN uses this for inter-node delay calculation. FDN/IR may ignore it. */
    virtual void updateGeometry (const std::vector<NodePosition>& nodes) = 0;

    /** Set a shared thread pool for parallel per-node processing.
        FDN and IR override this; SDN ignores it (inter-node coupling). */
    virtual void setParallelFor (AudioParallelFor*) {}
};
