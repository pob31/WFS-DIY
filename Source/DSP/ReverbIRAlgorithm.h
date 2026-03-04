#pragma once

#include "ReverbAlgorithm.h"
#include "AudioParallelFor.h"

//==============================================================================
/**
    IR (Impulse Response Convolution) reverb algorithm.

    Each node convolves its input with a loaded impulse response using
    juce::dsp::Convolution (partitioned convolution). Maximum realism
    from captured spaces.

    The full IR file is cached in memory. Trim (offset from start) and
    length (max duration) are applied from the cached buffer, so parameter
    changes don't require re-reading from disk.
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

        // Reload from cached buffer if available
        if (cachedIRBuffer.getNumSamples() > 0)
            applyIRToConvolvers();
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
        auto processNode = [&] (int n)
        {
            auto& conv = *convolvers[static_cast<size_t> (n)];
            auto& buf = processBuffers[static_cast<size_t> (n)];

            buf.copyFrom (0, 0, nodeInputs, n, 0, numSamples);

            auto block = juce::dsp::AudioBlock<float> (buf).getSubBlock (0, static_cast<size_t> (numSamples));
            auto context = juce::dsp::ProcessContextReplacing<float> (block);
            conv.process (context);

            nodeOutputs.copyFrom (n, 0, buf, 0, 0, numSamples);
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

    /** Load an IR from a pre-read buffer. The caller reads the file
        outside any lock so that file I/O never happens under the SpinLock.
        The buffer is cached so that trim/length changes don't require re-reading. */
    void loadIRFromBuffer (const juce::File& file,
                           juce::AudioBuffer<float>&& buf,
                           double fileSampleRate)
    {
        currentIRFile = file;
        cachedIRBuffer = std::move (buf);
        cachedIRSampleRate = fileSampleRate;
        irFileDurationSec = static_cast<float> (cachedIRBuffer.getNumSamples())
                          / static_cast<float> (cachedIRSampleRate);
        applyIRToConvolvers();
    }

    /** Set IR trim time (ms from start) and max length (seconds).
        Uses the cached buffer — no file I/O. */
    void setIRParameters (float trimMs, float lengthSec)
    {
        bool changed = (trimMs != irTrimMs || lengthSec != irLengthSec);
        irTrimMs = trimMs;
        irLengthSec = lengthSec;

        if (changed && cachedIRBuffer.getNumSamples() > 0)
            applyIRToConvolvers();
    }

    /** Get the currently loaded IR file. */
    const juce::File& getCurrentIRFile() const { return currentIRFile; }

    /** Get the duration of the currently loaded IR file in seconds. */
    float getIRFileDuration() const { return irFileDurationSec; }

    /** Destroy all convolvers, create fresh ones, and load a new IR.
        Called from the engine thread during a fade-to-silence transition
        so that no residual state from the old IR leaks through. */
    void recreateConvolversAndLoad (const juce::File& file,
                                     juce::AudioBuffer<float>&& buf,
                                     double fileSampleRate)
    {
        // Cache the new IR
        currentIRFile = file;
        cachedIRBuffer = std::move (buf);
        cachedIRSampleRate = fileSampleRate;
        irFileDurationSec = static_cast<float> (cachedIRBuffer.getNumSamples())
                          / static_cast<float> (cachedIRSampleRate);

        // Destroy old convolvers and create fresh ones — prepare FIRST
        // so that processSpec is correct when loadImpulseResponse is queued.
        // (JUCE default spec is {44100, 128, 2} — without prepare() first,
        //  the background thread resamples the IR to 44100 Hz instead of 48000.)
        convolvers.clear();
        processBuffers.clear();

        for (int i = 0; i < numActiveNodes; ++i)
        {
            convolvers.push_back (std::make_unique<juce::dsp::Convolution>());
            convolvers.back()->prepare (spec);
            processBuffers.push_back (juce::AudioBuffer<float> (1, blockSize));
        }

        // Load IR after prepare — background thread builds engine with correct spec.
        // Our fade-in from silence masks the async transition.
        applyIRToConvolvers();
    }

private:
    //==========================================================================
    // Apply trim and length to cached buffer, load into all convolvers
    //==========================================================================
    void applyIRToConvolvers()
    {
        if (cachedIRBuffer.getNumSamples() == 0)
            return;

        // Trim: skip N ms from start of IR
        int trimSamples = static_cast<int> (irTrimMs * 0.001f * cachedIRSampleRate);
        int startSample = juce::jmin (trimSamples, cachedIRBuffer.getNumSamples());
        int remaining = cachedIRBuffer.getNumSamples() - startSample;

        // Length: limit to N seconds
        int maxLenSamples = (irLengthSec > 0.0f)
            ? static_cast<int> (irLengthSec * cachedIRSampleRate)
            : remaining;
        int numToUse = juce::jmin (remaining, maxLenSamples);

        if (numToUse <= 0)
            return;

        for (auto& conv : convolvers)
        {
            juce::AudioBuffer<float> trimmed (1, numToUse);
            trimmed.copyFrom (0, 0, cachedIRBuffer, 0, startSample, numToUse);

            conv->loadImpulseResponse (std::move (trimmed), cachedIRSampleRate,
                juce::dsp::Convolution::Stereo::no,
                juce::dsp::Convolution::Trim::no,
                juce::dsp::Convolution::Normalise::yes);
        }
    }

    //==========================================================================
    double sr = 48000.0;
    int numActiveNodes = 0;
    int blockSize = 256;
    AudioParallelFor* parallel = nullptr;

    juce::dsp::ProcessSpec spec {};

    std::vector<std::unique_ptr<juce::dsp::Convolution>> convolvers;
    std::vector<juce::AudioBuffer<float>> processBuffers;

    juce::File currentIRFile;
    float irTrimMs = 0.0f;
    float irLengthSec = 6.0f;

    // Cached full IR buffer (read once, reused for trim/length changes)
    juce::AudioBuffer<float> cachedIRBuffer;
    double cachedIRSampleRate = 0.0;
    float irFileDurationSec = 0.0f;
};
