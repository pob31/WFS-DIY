#pragma once

#include <JuceHeader.h>
#include "LockFreeRingBuffer.h"
#include <atomic>

//==============================================================================
/**
    Processes a single output channel with contributions from multiple input channels.
    Uses write-time delays: when input arrives, calculates where to write in output buffer.
    Runs on its own thread for parallel processing.
*/
class OutputBufferProcessor : public juce::Thread
{
public:
    OutputBufferProcessor(int outputIndex, int numInputs, int numOutputs, const float* delayTimesPtr, const float* levelsPtr)
        : juce::Thread("OutputBufferProcessor_" + juce::String(outputIndex)),
          outputChannelIndex(outputIndex),
          numInputChannels(numInputs),
          numOutputChannels(numOutputs),
          sharedDelayTimes(delayTimesPtr),
          sharedLevels(levelsPtr)
    {
        // Pre-allocate input buffers (one per input channel)
        for (int i = 0; i < numInputs; ++i)
            inputBuffers.emplace_back(std::make_unique<LockFreeRingBuffer>());
    }

    ~OutputBufferProcessor() override
    {
        stopThread(1000);
    }

    void prepare(double sampleRate, int maxBlockSize)
    {
        currentSampleRate = sampleRate;

        // Allocate 1 second delay buffer for this output
        delayBufferLength = (int)(sampleRate * 1.0);
        delayBuffer.setSize(1, delayBufferLength);
        delayBuffer.clear();
        writePosition = 0;

        // Setup input ring buffers - one for each input channel
        for (auto& inputBuffer : inputBuffers)
            inputBuffer->setSize(maxBlockSize * 4);

        // Setup output ring buffer
        outputRingBuffer.setSize(maxBlockSize * 4);
    }

    // Called by audio thread to push input data from a specific input channel
    void pushInput(int inputChannel, const float* data, int numSamples)
    {
        if (inputChannel >= 0 && inputChannel < inputBuffers.size())
        {
            inputBuffers[inputChannel]->write(data, numSamples);

            // Update minimum available samples across all inputs
            int minAvailable = std::numeric_limits<int>::max();
            for (auto& buf : inputBuffers)
            {
                minAvailable = juce::jmin(minAvailable, buf->getAvailableData());
            }
            samplesAvailable.store(minAvailable, std::memory_order_release);
        }
    }

    // Called by audio thread to pull output data
    int pullOutput(float* destination, int numSamples)
    {
        return outputRingBuffer.read(destination, numSamples);
    }

    void reset()
    {
        for (auto& inputBuffer : inputBuffers)
            inputBuffer->reset();
        outputRingBuffer.reset();
        delayBuffer.clear();
        writePosition = 0;
    }

    void setProcessingEnabled(bool enabled)
    {
        processingEnabled.store(enabled, std::memory_order_release);
    }

    int getOutputChannelIndex() const { return outputChannelIndex; }

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
        std::vector<juce::AudioBuffer<float>> inputBlocks;

        // Pre-allocate input blocks (one per input channel)
        for (int i = 0; i < numInputChannels; ++i)
            inputBlocks.emplace_back(1, processingBlockSize);

        juce::AudioBuffer<float> outputBlock(1, processingBlockSize);

        double processingTimeMs = 0.0;
        double processingTimeMsForAvg = 0.0;
        int processedBlockCount = 0;
        auto measurementStartTime = juce::Time::getMillisecondCounterHiRes();

        while (!threadShouldExit())
        {
            // Wait for input data from all channels
            if (samplesAvailable.load(std::memory_order_acquire) < processingBlockSize)
            {
                wait(1); // Wait 1ms
                continue;
            }

            // Read input samples from all input channels
            int samplesRead = processingBlockSize;
            for (int inChannel = 0; inChannel < numInputChannels; ++inChannel)
            {
                int read = inputBuffers[inChannel]->read(inputBlocks[inChannel].getWritePointer(0), processingBlockSize);
                samplesRead = juce::jmin(samplesRead, read);
            }

            // Update available count
            int minAvailable = std::numeric_limits<int>::max();
            for (auto& buf : inputBuffers)
            {
                minAvailable = juce::jmin(minAvailable, buf->getAvailableData());
            }
            samplesAvailable.store(minAvailable, std::memory_order_release);

            if (samplesRead == 0)
                continue;

            // Process if enabled
            if (processingEnabled.load(std::memory_order_acquire))
            {
                auto processStartTime = juce::Time::getMillisecondCounterHiRes();

                processBlock(inputBlocks, outputBlock, samplesRead);

                auto processEndTime = juce::Time::getMillisecondCounterHiRes();
                double blockProcessTime = processEndTime - processStartTime;

                processingTimeMs += blockProcessTime;
                processingTimeMsForAvg += blockProcessTime;
                processedBlockCount++;

                // Write processed output to output ring buffer
                outputRingBuffer.write(outputBlock.getReadPointer(0), samplesRead);
            }
            else
            {
                // If processing disabled, write silence
                float silence[processingBlockSize] = {0};
                outputRingBuffer.write(silence, samplesRead);
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

    void processBlock(std::vector<juce::AudioBuffer<float>>& inputs, juce::AudioBuffer<float>& output, int numSamples)
    {
        auto* delayData = delayBuffer.getWritePointer(0);
        auto* outputData = output.getWritePointer(0);

        // Safety check
        if (delayBufferLength == 0 || delayData == nullptr)
            return;

        // Process each sample
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Read output from current position (no delay calculation needed on read)
            outputData[sample] = delayData[writePosition];

            // Clear this position after reading
            delayData[writePosition] = 0.0f;

            // Now accumulate contributions from all inputs with their respective delays
            for (int inChannel = 0; inChannel < numInputChannels; ++inChannel)
            {
                // Get input sample
                float inputSample = inputs[inChannel].getReadPointer(0)[sample];

                // Calculate index into shared arrays: [inputChannel * numOutputChannels + outputChannel]
                int routingIndex = inChannel * numOutputChannels + outputChannelIndex;

                // Get level for this routing
                float level = sharedLevels[routingIndex];

                // Optimization: skip if level is zero
                if (level == 0.0f)
                    continue;

                // Get delay time in milliseconds and convert to samples
                float delayMs = sharedDelayTimes[routingIndex];
                float delaySamples = (delayMs / 1000.0f) * (float)currentSampleRate;

                // Ensure delay doesn't exceed buffer length
                if (delaySamples >= (float)delayBufferLength)
                    delaySamples = (float)(delayBufferLength - 1);

                // Calculate write position with delay offset
                // We want this input to appear in output after 'delaySamples' samples
                float exactWritePos = (float)writePosition + delaySamples;

                // Handle wraparound
                while (exactWritePos >= (float)delayBufferLength)
                    exactWritePos -= (float)delayBufferLength;

                // Split into integer and fractional parts for interpolation
                int writePos1 = (int)exactWritePos;
                int writePos2 = (writePos1 + 1) % delayBufferLength;
                float fraction = exactWritePos - (int)exactWritePos;

                // Apply level
                float contribution = inputSample * level;

                // Write with linear interpolation (distribute across two adjacent samples)
                // This maintains smooth interpolation when delay times change
                delayData[writePos1] += contribution * (1.0f - fraction);
                delayData[writePos2] += contribution * fraction;
            }

            // Advance write/read position
            writePosition = (writePosition + 1) % delayBufferLength;
        }
    }

    int outputChannelIndex;
    int numInputChannels;
    int numOutputChannels;
    double currentSampleRate = 44100.0;

    // Delay line for this output
    juce::AudioBuffer<float> delayBuffer;
    int delayBufferLength = 0;
    int writePosition = 0;

    // Lock-free communication
    std::vector<std::unique_ptr<LockFreeRingBuffer>> inputBuffers;  // One per input channel
    LockFreeRingBuffer outputRingBuffer;
    std::atomic<int> samplesAvailable {0};
    std::atomic<bool> processingEnabled {false};

    // CPU monitoring
    std::atomic<float> cpuUsagePercent {0.0f};
    std::atomic<float> processingTimeMicroseconds {0.0f};

    // Pointers to shared routing matrices (owned by MainComponent)
    const float* sharedDelayTimes;  // delays[inputChannel * numOutputs + outputChannel]
    const float* sharedLevels;      // levels[inputChannel * numOutputs + outputChannel]

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputBufferProcessor)
};
