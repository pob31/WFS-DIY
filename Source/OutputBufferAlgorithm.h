#pragma once

#include "OutputBufferProcessor.h"
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
                bool processingEnabled)
    {
        // Create output-based processors (one thread per output channel)
        for (int i = 0; i < numOutputs; ++i)
        {
            auto processor = std::make_unique<OutputBufferProcessor>(i, numInputs, numOutputs,
                                                                      delayTimesPtr,
                                                                      levelsPtr);
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

        // Step 1: Distribute input data to all output processors
        for (int inChannel = 0; inChannel < numChannels; ++inChannel)
        {
            auto* inputData = bufferToFill.buffer->getReadPointer(inChannel, bufferToFill.startSample);

            // Send this input to all output processors
            for (auto& processor : outputProcessors)
            {
                processor->pushInput(inChannel, inputData, numSamples);
            }
        }

        // Step 2: Clear output buffer
        bufferToFill.clearActiveBufferRegion();

        // Step 3: Pull processed outputs from each output processor
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

private:
    std::vector<std::unique_ptr<OutputBufferProcessor>> outputProcessors;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputBufferAlgorithm)
};
