#pragma once

#include <JuceHeader.h>
#include "LockFreeRingBuffer.h"
#include <atomic>

//==============================================================================
/**
    Processes a single input channel with delay lines outputting to multiple channels.
    Runs on its own thread for parallel processing.
*/
class InputBufferProcessor : public juce::Thread
{
public:
    InputBufferProcessor(int inputIndex, int numOutputs, const float* delayTimesPtr, const float* levelsPtr)
        : juce::Thread("InputBufferProcessor_" + juce::String(inputIndex)),
          inputChannelIndex(inputIndex),
          numOutputChannels(numOutputs),
          sharedDelayTimes(delayTimesPtr),
          sharedLevels(levelsPtr)
    {
        // Pre-allocate output buffers without copying
        for (int i = 0; i < numOutputs; ++i)
            outputBuffers.emplace_back(std::make_unique<LockFreeRingBuffer>());
    }

    ~InputBufferProcessor() override
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

    int getInputChannelIndex() const { return inputChannelIndex; }

    // Get CPU usage percentage for this thread (0-100)
    float getCpuUsagePercent() const
    {
        return cpuUsagePercent.load(std::memory_order_acquire);
    }

    // Get average processing time per block in microseconds (for algorithm comparison)
    float getProcessingTimeMicroseconds() const
    {
        return processingTimeMicroseconds.load(std::memory_order_acquire);
    }

private:
    void run() override
    {
        const int processingBlockSize = 64; // Internal processing block size
        juce::AudioBuffer<float> inputBlock(1, processingBlockSize);
        juce::AudioBuffer<float> outputBlock(numOutputChannels, processingBlockSize);

        double processingTimeMs = 0.0;
        double processingTimeMsForAvg = 0.0;
        int processedBlockCount = 0;
        auto measurementStartTime = juce::Time::getMillisecondCounterHiRes();

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
                auto processStartTime = juce::Time::getMillisecondCounterHiRes();

                processBlock(inputBlock.getReadPointer(0), outputBlock, samplesRead);

                auto processEndTime = juce::Time::getMillisecondCounterHiRes();
                double blockProcessTime = processEndTime - processStartTime;

                processingTimeMs += blockProcessTime;
                processingTimeMsForAvg += blockProcessTime;
                processedBlockCount++;

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

            // Update CPU usage every ~200ms of wall-clock time
            auto now = juce::Time::getMillisecondCounterHiRes();
            double elapsedWallClockTime = now - measurementStartTime;

            if (elapsedWallClockTime >= 200.0)
            {
                // Wall-clock CPU usage percentage
                float usage = (float)((processingTimeMs / elapsedWallClockTime) * 100.0);
                cpuUsagePercent.store(usage, std::memory_order_release);

                // Average processing time per block in microseconds
                if (processedBlockCount > 0)
                {
                    float avgTimeMicroseconds = (float)((processingTimeMsForAvg / processedBlockCount) * 1000.0);
                    processingTimeMicroseconds.store(avgTimeMicroseconds, std::memory_order_release);
                }

                // Reset counters
                processingTimeMs = 0.0;
                processingTimeMsForAvg = 0.0;
                processedBlockCount = 0;
                measurementStartTime = now;
            }
        }
    }

    void processBlock(const float* input, juce::AudioBuffer<float>& outputs, int numSamples)
    {
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

            // Calculate index into shared arrays: [inputChannel * numOutputs + outputChannel]
            int routingIndex = inputChannelIndex * numOutputChannels + outChannel;

            // Get level for this output from shared array
            float level = sharedLevels[routingIndex];

            // Optimization: skip processing if level is zero
            if (level == 0.0f)
            {
                // Write silence
                for (int sample = 0; sample < numSamples; ++sample)
                    outputData[sample] = 0.0f;
                continue;
            }

            // Get delay time in milliseconds from shared array and convert to samples (fractional)
            float delayMs = sharedDelayTimes[routingIndex];
            float delaySamples = (delayMs / 1000.0f) * (float)currentSampleRate;

            // Ensure delay doesn't exceed buffer length
            if (delaySamples >= (float)delayBufferLength)
                delaySamples = (float)(delayBufferLength - 1);

            // Process each sample with linear interpolation
            for (int sample = 0; sample < numSamples; ++sample)
            {
                // Calculate fractional read position
                float exactReadPos = (float)writePosition + (float)sample - delaySamples;

                // Handle wraparound for negative positions
                while (exactReadPos < 0.0f)
                    exactReadPos += (float)delayBufferLength;

                // Split into integer and fractional parts
                int readPos1 = (int)exactReadPos % delayBufferLength;
                int readPos2 = (readPos1 + 1) % delayBufferLength;
                float fraction = exactReadPos - (int)exactReadPos;

                // Linear interpolation between two samples
                float sample1 = delayData[readPos1];
                float sample2 = delayData[readPos2];
                float interpolatedSample = sample1 + fraction * (sample2 - sample1);

                // Apply level and write output
                outputData[sample] = interpolatedSample * level;
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

    // CPU monitoring
    std::atomic<float> cpuUsagePercent {0.0f};
    std::atomic<float> processingTimeMicroseconds {0.0f};

    // Pointers to shared routing matrices (owned by MainComponent)
    const float* sharedDelayTimes;  // delays[inputChannel * numOutputs + outputChannel]
    const float* sharedLevels;      // levels[inputChannel * numOutputs + outputChannel]

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InputBufferProcessor)
};
