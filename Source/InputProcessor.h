#pragma once

#include <JuceHeader.h>
#include "LockFreeRingBuffer.h"
#include <atomic>

//==============================================================================
/**
    Processes a single input channel with delay lines outputting to multiple channels.
    Runs on its own thread for parallel processing.
*/
class InputProcessor : public juce::Thread
{
public:
    InputProcessor(int inputIndex, int numOutputs)
        : juce::Thread("InputProcessor_" + juce::String(inputIndex)),
          inputChannelIndex(inputIndex),
          numOutputChannels(numOutputs)
    {
        // Pre-allocate output buffers without copying
        for (int i = 0; i < numOutputs; ++i)
            outputBuffers.emplace_back(std::make_unique<LockFreeRingBuffer>());
    }

    ~InputProcessor() override
    {
        stopThread(1000);
    }

    void prepare(double sampleRate, int maxBlockSize)
    {
        currentSampleRate = sampleRate;

        // Allocate 1 second delay buffer
        delayBufferLength = (int)(sampleRate * 1.0);
        delayBuffer.setSize(1, delayBufferLength);
        delayBuffer.clear();
        writePosition = 0;

        // Setup input ring buffer - make it 4x block size for safety
        inputRingBuffer.setSize(maxBlockSize * 4);

        // Setup output ring buffers for each output channel
        for (auto& outputBuffer : outputBuffers)
            outputBuffer->setSize(maxBlockSize * 4);
    }

    // Called by audio thread to push input data
    void pushInput(const float* data, int numSamples)
    {
        inputRingBuffer.write(data, numSamples);
        samplesAvailable.store(inputRingBuffer.getAvailableData(), std::memory_order_release);
    }

    // Called by audio thread to pull output data for a specific output channel
    int pullOutput(int outputChannel, float* destination, int numSamples)
    {
        if (outputChannel >= 0 && outputChannel < outputBuffers.size())
            return outputBuffers[outputChannel]->read(destination, numSamples);
        return 0;
    }

    void reset()
    {
        inputRingBuffer.reset();
        for (auto& outputBuffer : outputBuffers)
            outputBuffer->reset();
        delayBuffer.clear();
        writePosition = 0;
    }

    void setProcessingEnabled(bool enabled)
    {
        processingEnabled.store(enabled, std::memory_order_release);
    }

private:
    void run() override
    {
        const int processingBlockSize = 64; // Internal processing block size
        juce::AudioBuffer<float> inputBlock(1, processingBlockSize);
        juce::AudioBuffer<float> outputBlock(numOutputChannels, processingBlockSize);

        while (!threadShouldExit())
        {
            // Wait for input data
            if (samplesAvailable.load(std::memory_order_acquire) < processingBlockSize)
            {
                wait(1); // Wait 1ms
                continue;
            }

            // Read input samples
            int samplesRead = inputRingBuffer.read(inputBlock.getWritePointer(0), processingBlockSize);
            samplesAvailable.store(inputRingBuffer.getAvailableData(), std::memory_order_release);

            if (samplesRead == 0)
                continue;

            // Process if enabled
            if (processingEnabled.load(std::memory_order_acquire))
            {
                processBlock(inputBlock.getReadPointer(0), outputBlock, samplesRead);

                // Write processed outputs to each output ring buffer
                for (int outChannel = 0; outChannel < numOutputChannels; ++outChannel)
                {
                    outputBuffers[outChannel]->write(outputBlock.getReadPointer(outChannel), samplesRead);
                }
            }
            else
            {
                // If processing disabled, write silence
                for (int outChannel = 0; outChannel < numOutputChannels; ++outChannel)
                {
                    float silence[processingBlockSize] = {0};
                    outputBuffers[outChannel]->write(silence, samplesRead);
                }
            }
        }
    }

    void processBlock(const float* input, juce::AudioBuffer<float>& outputs, int numSamples)
    {
        // Base delay increment (200ms)
        int delayIncrement = (int)(currentSampleRate * 0.2);

        auto* delayData = delayBuffer.getWritePointer(0);

        // Safety check
        if (delayBufferLength == 0 || delayData == nullptr)
            return;

        // Write input to delay buffer
        for (int sample = 0; sample < numSamples; ++sample)
        {
            delayData[writePosition] = input[sample];
            writePosition = (writePosition + 1) % delayBufferLength;
        }

        // Reset write position to process outputs
        writePosition = (writePosition - numSamples + delayBufferLength) % delayBufferLength;

        // Generate delayed outputs for each output channel
        for (int outChannel = 0; outChannel < numOutputChannels; ++outChannel)
        {
            auto* outputData = outputs.getWritePointer(outChannel);

            // Calculate delay: shortest for matching channel, increasing by 200ms for each step
            int delaySteps = (outChannel - inputChannelIndex + numOutputChannels) % numOutputChannels;
            int delaySamples = delayIncrement * (delaySteps + 1);

            // Ensure delay doesn't exceed buffer length
            if (delaySamples >= delayBufferLength)
                delaySamples = delayBufferLength - 1;

            for (int sample = 0; sample < numSamples; ++sample)
            {
                // Calculate read position with proper wraparound
                int readPos = writePosition + sample - delaySamples;
                while (readPos < 0)
                    readPos += delayBufferLength;
                readPos = readPos % delayBufferLength;

                outputData[sample] = delayData[readPos];
            }
        }

        // Advance write position
        writePosition = (writePosition + numSamples) % delayBufferLength;
    }

    int inputChannelIndex;
    int numOutputChannels;
    double currentSampleRate = 44100.0;

    // Delay line
    juce::AudioBuffer<float> delayBuffer;
    int delayBufferLength = 0;
    int writePosition = 0;

    // Lock-free communication
    LockFreeRingBuffer inputRingBuffer;
    std::vector<std::unique_ptr<LockFreeRingBuffer>> outputBuffers;
    std::atomic<int> samplesAvailable {0};
    std::atomic<bool> processingEnabled {false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InputProcessor)
};
