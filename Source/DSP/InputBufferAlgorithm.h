#pragma once

#include "InputBufferProcessor.h"
#include <vector>
#include <memory>

//==============================================================================
/**
    Input-based WFS algorithm using read-time delays.

    Strategy:
    - One processing thread per input channel
    - Each thread reads from its input, applies delays, and writes to all outputs
    - Delay calculation happens at read time (when generating output)

    This class manages a collection of InputBufferProcessor instances.
*/
class InputBufferAlgorithm
{
public:
    InputBufferAlgorithm() = default;
    ~InputBufferAlgorithm()
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
        // Create input-based processors (one thread per input channel)
        for (int i = 0; i < numInputs; ++i)
        {
            auto processor = std::make_unique<InputBufferProcessor>(i, numOutputs,
                                                                     delayTimesPtr,
                                                                     levelsPtr,
                                                                     hfAttenuationPtr,
                                                                     frDelayTimesPtr,
                                                                     frLevelsPtr,
                                                                     frHFAttenuationPtr);
            processor->prepare(sampleRate, blockSize);
            inputProcessors.push_back(std::move(processor));
        }

        // Start threads AFTER all processors are created and prepared
        for (auto& processor : inputProcessors)
        {
            processor->setProcessingEnabled(processingEnabled);
            processor->startThread(juce::Thread::Priority::high);
        }
    }

    void reprepare(double sampleRate, int blockSize, bool processingEnabled)
    {
        // Stop threads first
        for (auto& processor : inputProcessors)
            processor->stopThread(1000);

        // Re-prepare and restart
        for (auto& processor : inputProcessors)
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

        if (inputProcessors.empty())
        {
            bufferToFill.clearActiveBufferRegion();
            return;
        }

        // Determine actual available channels
        auto numChannels = juce::jmin(numInputChannels, totalChannels, (int)inputProcessors.size());

        // Step 1: Distribute input data to each input processor thread
        for (int inChannel = 0; inChannel < numChannels; ++inChannel)
        {
            if (inputProcessors[inChannel] != nullptr && inChannel < totalChannels)
            {
                auto* inputData = bufferToFill.buffer->getReadPointer(inChannel, bufferToFill.startSample);
                inputProcessors[inChannel]->pushInput(inputData, numSamples);
            }
        }

        // Step 2: Clear output buffer
        bufferToFill.clearActiveBufferRegion();

        // Step 3: Sum outputs from all input processors to output channels
        juce::AudioBuffer<float> tempBuffer(1, numSamples);

        for (int inChannel = 0; inChannel < numChannels; ++inChannel)
        {
            if (inputProcessors[inChannel] == nullptr)
                continue;

            // Loop over all output channels
            int numOutputs = juce::jmin(numOutputChannels, totalChannels);
            for (int outChannel = 0; outChannel < numOutputs; ++outChannel)
            {
                auto* outputData = bufferToFill.buffer->getWritePointer(outChannel, bufferToFill.startSample);
                auto* tempData = tempBuffer.getWritePointer(0);

                // Pull processed data from this input processor for this output channel
                int samplesRead = inputProcessors[inChannel]->pullOutput(outChannel, tempData, numSamples);

                // Sum into output channel
                for (int i = 0; i < samplesRead; ++i)
                {
                    outputData[i] += tempData[i];
                }
            }
        }
    }

    void setProcessingEnabled(bool enabled)
    {
        for (auto& processor : inputProcessors)
            processor->setProcessingEnabled(enabled);
    }

    void releaseResources()
    {
        for (auto& processor : inputProcessors)
        {
            processor->stopThread(1000);
            processor->reset();
        }
    }

    void clear()
    {
        inputProcessors.clear();
    }

    bool isEmpty() const
    {
        return inputProcessors.empty();
    }

    size_t getNumProcessors() const
    {
        return inputProcessors.size();
    }

    float getCpuUsagePercent(size_t index) const
    {
        if (index < inputProcessors.size())
            return inputProcessors[index]->getCpuUsagePercent();
        return 0.0f;
    }

    float getProcessingTimeMicroseconds(size_t index) const
    {
        if (index < inputProcessors.size())
            return inputProcessors[index]->getProcessingTimeMicroseconds();
        return 0.0f;
    }

    // === Live Source Tamer accessors ===

    float getPeakGainReduction(size_t inputIndex) const
    {
        if (inputIndex < inputProcessors.size())
            return inputProcessors[inputIndex]->getLSPeakGainReduction();
        return 1.0f;
    }

    float getSlowGainReduction(size_t inputIndex) const
    {
        if (inputIndex < inputProcessors.size())
            return inputProcessors[inputIndex]->getLSSlowGainReduction();
        return 1.0f;
    }

    // Get short peak level in dB (5ms hold for AutomOtion triggering)
    float getShortPeakLevelDb(size_t inputIndex) const
    {
        if (inputIndex < inputProcessors.size())
            return inputProcessors[inputIndex]->getShortPeakLevelDb();
        return -200.0f;
    }

    // Get RMS level in dB (200ms window)
    float getRmsLevelDb(size_t inputIndex) const
    {
        if (inputIndex < inputProcessors.size())
            return inputProcessors[inputIndex]->getRmsLevelDb();
        return -200.0f;
    }

    void setLSParameters(size_t inputIndex, float peakThreshDb, float peakRatio,
                         float slowThreshDb, float slowRatio)
    {
        if (inputIndex < inputProcessors.size())
            inputProcessors[inputIndex]->setLSParameters(peakThreshDb, peakRatio,
                                                          slowThreshDb, slowRatio);
    }

    // === Floor Reflection parameter setters ===

    void setFRFilterParams(size_t inputIndex,
                           bool lowCutActive, float lowCutFreq,
                           bool highShelfActive, float highShelfFreq,
                           float highShelfGain, float highShelfSlope)
    {
        if (inputIndex < inputProcessors.size())
            inputProcessors[inputIndex]->setFRFilterParams(lowCutActive, lowCutFreq,
                                                            highShelfActive, highShelfFreq,
                                                            highShelfGain, highShelfSlope);
    }

    void setFRDiffusion(size_t inputIndex, float diffusionPercent)
    {
        if (inputIndex < inputProcessors.size())
            inputProcessors[inputIndex]->setFRDiffusion(diffusionPercent);
    }

private:
    std::vector<std::unique_ptr<InputBufferProcessor>> inputProcessors;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InputBufferAlgorithm)
};
