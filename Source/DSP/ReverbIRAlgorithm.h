#pragma once

#include "ReverbAlgorithm.h"

//==============================================================================
/**
    IR (Impulse Response Convolution) reverb algorithm.

    Each node convolves its input with a loaded impulse response using
    juce::dsp::Convolution (partitioned convolution). Maximum realism
    from captured spaces.

    Supports global IR (all nodes share one file) or per-node IR.
*/
class IRAlgorithm : public ReverbAlgorithm
{
public:
    //==========================================================================
    void prepare (double newSampleRate, int maxBlockSize, int numNodes) override
    {
        sr = newSampleRate;
        numActiveNodes = numNodes;
        blockSize = maxBlockSize;

        spec.sampleRate = sr;
        spec.maximumBlockSize = static_cast<juce::uint32> (maxBlockSize);
        spec.numChannels = 1;

        convolvers.clear();
        processBuffers.clear();

        for (int i = 0; i < numNodes; ++i)
        {
            convolvers.push_back (std::make_unique<juce::dsp::Convolution>());
            convolvers.back()->prepare (spec);

            processBuffers.push_back (juce::AudioBuffer<float> (1, maxBlockSize));
        }

        // Reload IR if we have a file path
        if (currentIRFile.existsAsFile())
            loadIRFile (currentIRFile);
    }

    void reset() override
    {
        for (auto& conv : convolvers)
            conv->reset();
    }

    void processBlock (const juce::AudioBuffer<float>& nodeInputs,
                       juce::AudioBuffer<float>& nodeOutputs,
                       int numSamples) override
    {
        for (int n = 0; n < numActiveNodes; ++n)
        {
            auto& conv = *convolvers[static_cast<size_t> (n)];
            auto& buf = processBuffers[static_cast<size_t> (n)];

            // Copy input to process buffer
            buf.copyFrom (0, 0, nodeInputs, n, 0, numSamples);

            // Process through convolver
            auto block = juce::dsp::AudioBlock<float> (buf).getSubBlock (0, static_cast<size_t> (numSamples));
            auto context = juce::dsp::ProcessContextReplacing<float> (block);
            conv.process (context);

            // Copy to output
            nodeOutputs.copyFrom (n, 0, buf, 0, 0, numSamples);
        }
    }

    void setParameters (const AlgorithmParameters&) override
    {
        // IR algorithm parameters (trim, length) are handled via setIRParameters()
    }

    void updateGeometry (const std::vector<NodePosition>&) override
    {
        // IR does not use geometry
    }

    //==========================================================================
    // IR-specific parameter setters
    //==========================================================================

    /** Load an IR file. Called when the user selects a new file. */
    void loadIRFile (const juce::File& file)
    {
        if (! file.existsAsFile())
            return;

        currentIRFile = file;

        // Calculate trim in samples
        int trimSamples = static_cast<int> (irTrimMs * 0.001f * sr);

        for (auto& conv : convolvers)
        {
            conv->loadImpulseResponse (file,
                juce::dsp::Convolution::Stereo::no,
                juce::dsp::Convolution::Trim::yes,
                trimSamples);
        }
    }

    /** Set IR trim time (ms) and max length (seconds). */
    void setIRParameters (float trimMs, float lengthSec)
    {
        bool changed = (trimMs != irTrimMs || lengthSec != irLengthSec);
        irTrimMs = trimMs;
        irLengthSec = lengthSec;

        // Reload if parameters changed and we have a file
        if (changed && currentIRFile.existsAsFile())
            loadIRFile (currentIRFile);
    }

    /** Get the currently loaded IR file. */
    const juce::File& getCurrentIRFile() const { return currentIRFile; }

private:
    double sr = 48000.0;
    int numActiveNodes = 0;
    int blockSize = 256;

    juce::dsp::ProcessSpec spec {};

    std::vector<std::unique_ptr<juce::dsp::Convolution>> convolvers;
    std::vector<juce::AudioBuffer<float>> processBuffers;

    juce::File currentIRFile;
    float irTrimMs = 0.0f;
    float irLengthSec = 6.0f;
};
