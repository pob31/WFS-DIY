#pragma once

#include "OutputBufferProcessor.h"
#include "LiveSourceLevelDetector.h"
#include <vector>
#include <memory>

//==============================================================================
/**
    Output-based WFS algorithm using write-time delays.

    Strategy:
    - One processing thread per output channel
    - Each thread receives all inputs and accumulates delayed contributions
    - Delay calculation happens at write time (when input arrives)

    This class manages a collection of OutputBufferProcessor instances.
*/
class OutputBufferAlgorithm
{
public:
    OutputBufferAlgorithm() = default;
    ~OutputBufferAlgorithm()
    {
        clear();
    }

    void prepare(int numInputs,
                int numOutputs,
                double sampleRate,
                int blockSize,
                const float* delayTimesPtr,
                const float* levelsPtr,
                bool processingEnabled,
                const float* hfAttenuationPtr = nullptr,
                const float* frDelayTimesPtr = nullptr,
                const float* frLevelsPtr = nullptr,
                const float* frHFAttenuationPtr = nullptr)
    {
        storedNumInputs = numInputs;

        // Create Live Source level detectors (one per input channel)
        lsDetectors.clear();
        for (int i = 0; i < numInputs; ++i)
        {
            auto detector = std::make_unique<LiveSourceLevelDetector>();
            detector->prepare(sampleRate);
            lsDetectors.push_back(std::move(detector));
        }

        // Create output-based processors (one thread per output channel)
        for (int i = 0; i < numOutputs; ++i)
        {
            auto processor = std::make_unique<OutputBufferProcessor>(i, numInputs, numOutputs,
                                                                      delayTimesPtr,
                                                                      levelsPtr,
                                                                      hfAttenuationPtr,
                                                                      frDelayTimesPtr,
                                                                      frLevelsPtr,
                                                                      frHFAttenuationPtr);
            processor->prepare(sampleRate, blockSize);
            outputProcessors.push_back(std::move(processor));
        }

        // Start threads AFTER all processors are created and prepared
        for (auto& processor : outputProcessors)
        {
            processor->setProcessingEnabled(processingEnabled);
            processor->startThread(juce::Thread::Priority::high);
        }
    }

    void reprepare(double sampleRate, int blockSize, bool processingEnabled)
    {
        // Stop threads first
        for (auto& processor : outputProcessors)
            processor->stopThread(1000);

        // Re-prepare and restart
        for (auto& processor : outputProcessors)
        {
            processor->prepare(sampleRate, blockSize);
            processor->setProcessingEnabled(processingEnabled);
            processor->startThread(juce::Thread::Priority::high);
        }
    }

    void processBlock(const juce::AudioSourceChannelInfo& bufferToFill,
                     int numInputChannels,
                     int numOutputChannels)
    {
        auto totalChannels = bufferToFill.buffer->getNumChannels();
        auto numSamples = bufferToFill.numSamples;

        if (outputProcessors.empty())
        {
            bufferToFill.clearActiveBufferRegion();
            return;
        }

        // Determine actual available channels
        auto numChannels = juce::jmin(numInputChannels, totalChannels);

        // Step 1: Run level detection on input data BEFORE distributing to processors
        for (int inChannel = 0; inChannel < numChannels && inChannel < (int)lsDetectors.size(); ++inChannel)
        {
            auto* inputData = bufferToFill.buffer->getReadPointer(inChannel, bufferToFill.startSample);

            // Run Live Source level detection for this input
            if (lsDetectors[inChannel])
            {
                for (int sample = 0; sample < numSamples; ++sample)
                {
                    lsDetectors[inChannel]->processSample(inputData[sample]);
                }
            }
        }

        // Step 2: Distribute input data to all output processors
        for (int inChannel = 0; inChannel < numChannels; ++inChannel)
        {
            auto* inputData = bufferToFill.buffer->getReadPointer(inChannel, bufferToFill.startSample);

            // Send this input to all output processors
            for (auto& processor : outputProcessors)
            {
                processor->pushInput(inChannel, inputData, numSamples);
            }
        }

        // Step 3: Clear output buffer
        bufferToFill.clearActiveBufferRegion();

        // Step 4: Pull processed outputs from each output processor
        int numOutputs = juce::jmin(numOutputChannels, totalChannels, (int)outputProcessors.size());
        for (int outChannel = 0; outChannel < numOutputs; ++outChannel)
        {
            if (outputProcessors[outChannel] == nullptr)
                continue;

            auto* outputData = bufferToFill.buffer->getWritePointer(outChannel, bufferToFill.startSample);

            // Pull processed data from this output processor
            outputProcessors[outChannel]->pullOutput(outputData, numSamples);
        }
    }

    void setProcessingEnabled(bool enabled)
    {
        for (auto& processor : outputProcessors)
            processor->setProcessingEnabled(enabled);
    }

    void releaseResources()
    {
        for (auto& processor : outputProcessors)
        {
            processor->stopThread(1000);
            processor->reset();
        }
    }

    void clear()
    {
        outputProcessors.clear();
        lsDetectors.clear();
    }

    bool isEmpty() const
    {
        return outputProcessors.empty();
    }

    size_t getNumProcessors() const
    {
        return outputProcessors.size();
    }

    float getCpuUsagePercent(size_t index) const
    {
        if (index < outputProcessors.size())
            return outputProcessors[index]->getCpuUsagePercent();
        return 0.0f;
    }

    float getProcessingTimeMicroseconds(size_t index) const
    {
        if (index < outputProcessors.size())
            return outputProcessors[index]->getProcessingTimeMicroseconds();
        return 0.0f;
    }

    // === Live Source Tamer accessors ===

    float getPeakGainReduction(size_t inputIndex) const
    {
        if (inputIndex < lsDetectors.size())
            return lsDetectors[inputIndex]->getPeakGainReduction();
        return 1.0f;
    }

    float getSlowGainReduction(size_t inputIndex) const
    {
        if (inputIndex < lsDetectors.size())
            return lsDetectors[inputIndex]->getSlowGainReduction();
        return 1.0f;
    }

    // Get short peak level in dB (5ms hold for AutomOtion triggering)
    float getShortPeakLevelDb(size_t inputIndex) const
    {
        if (inputIndex < lsDetectors.size())
            return lsDetectors[inputIndex]->getShortPeakLevelDb();
        return -200.0f;
    }

    // Get RMS level in dB (200ms window)
    float getRmsLevelDb(size_t inputIndex) const
    {
        if (inputIndex < lsDetectors.size())
            return lsDetectors[inputIndex]->getRmsLevelDb();
        return -200.0f;
    }

    void setLSParameters(size_t inputIndex, float peakThreshDb, float peakRatio,
                         float slowThreshDb, float slowRatio)
    {
        if (inputIndex < lsDetectors.size())
            lsDetectors[inputIndex]->setParameters(peakThreshDb, peakRatio,
                                                    slowThreshDb, slowRatio);
    }

    // === Floor Reflection parameter setters ===
    // Note: Each output processor handles all inputs, so FR params need to be
    // forwarded to all processors for the specific input index

    void setFRFilterParams(size_t inputIndex,
                           bool lowCutActive, float lowCutFreq,
                           bool highShelfActive, float highShelfFreq,
                           float highShelfGain, float highShelfSlope)
    {
        // Forward to all output processors (each handles all inputs)
        for (auto& processor : outputProcessors)
        {
            processor->setFRFilterParams(static_cast<int>(inputIndex),
                                          lowCutActive, lowCutFreq,
                                          highShelfActive, highShelfFreq,
                                          highShelfGain, highShelfSlope);
        }
    }

    void setFRDiffusion(size_t inputIndex, float diffusionPercent)
    {
        // Forward to all output processors (each handles all inputs)
        for (auto& processor : outputProcessors)
        {
            processor->setFRDiffusion(static_cast<int>(inputIndex), diffusionPercent);
        }
    }

private:
    std::vector<std::unique_ptr<OutputBufferProcessor>> outputProcessors;

    // Live Source level detectors (one per input channel)
    std::vector<std::unique_ptr<LiveSourceLevelDetector>> lsDetectors;
    int storedNumInputs = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputBufferAlgorithm)
};
