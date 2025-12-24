#pragma once

#include <JuceHeader.h>
#include "LockFreeRingBuffer.h"
#include "WFSHighShelfFilter.h"
#include "WFSBiquadFilter.h"
#include <atomic>
#include <random>

//==============================================================================
/**
    Processes a single output channel with contributions from multiple input channels.
    Uses write-time delays: when input arrives, calculates where to write in output buffer.
    Runs on its own thread for parallel processing.

    Includes HF shelf filters (800Hz, Q=0.3) for air absorption simulation.
    One filter per input channel.
*/
class OutputBufferProcessor : public juce::Thread
{
public:
    OutputBufferProcessor(int outputIndex, int numInputs, int numOutputs,
                          const float* delayTimesPtr, const float* levelsPtr,
                          const float* hfAttenuationPtr = nullptr,
                          const float* frDelayTimesPtr = nullptr,
                          const float* frLevelsPtr = nullptr,
                          const float* frHFAttenuationPtr = nullptr)
        : juce::Thread("OutputBufferProcessor_" + juce::String(outputIndex)),
          outputChannelIndex(outputIndex),
          numInputChannels(numInputs),
          numOutputChannels(numOutputs),
          sharedDelayTimes(delayTimesPtr),
          sharedLevels(levelsPtr),
          sharedHFAttenuation(hfAttenuationPtr),
          sharedFRDelayTimes(frDelayTimesPtr),
          sharedFRLevels(frLevelsPtr),
          sharedFRHFAttenuation(frHFAttenuationPtr)
    {
        // Pre-allocate input buffers and HF filters (one per input channel)
        for (int i = 0; i < numInputs; ++i)
        {
            inputBuffers.emplace_back(std::make_unique<LockFreeRingBuffer>());
            hfFilters.emplace_back();  // One filter per input for direct signal
            frLowCutFilters.emplace_back();  // FR low-cut filter per input
            frHighShelfFilters.emplace_back();  // FR high-shelf filter per input
            frHFFilters.emplace_back();  // FR HF air absorption per input
        }

        // Initialize diffusion state (one per input)
        frDiffusionState.resize(static_cast<size_t>(numInputs), 0.0f);
        frDiffusionTarget.resize(static_cast<size_t>(numInputs), 0.0f);

        // Allocate FR filter enable flags and max jitter (per input) using placement new
        storedNumInputs = numInputs;
        frLowCutActiveFlags = new std::atomic<bool>[static_cast<size_t>(numInputs)];
        frHighShelfActiveFlags = new std::atomic<bool>[static_cast<size_t>(numInputs)];
        frMaxJitterMs = new std::atomic<float>[static_cast<size_t>(numInputs)];
        for (int i = 0; i < numInputs; ++i)
        {
            frLowCutActiveFlags[i].store(false, std::memory_order_relaxed);
            frHighShelfActiveFlags[i].store(false, std::memory_order_relaxed);
            frMaxJitterMs[i].store(0.0f, std::memory_order_relaxed);
        }
    }

    ~OutputBufferProcessor() override
    {
        stopThread(1000);
        delete[] frLowCutActiveFlags;
        delete[] frHighShelfActiveFlags;
        delete[] frMaxJitterMs;
    }

    void prepare(double sampleRate, int maxBlockSize)
    {
        currentSampleRate = sampleRate;

        // Allocate 1 second delay buffer for this output
        delayBufferLength = (int)(sampleRate * 1.0);
        delayBuffer.setSize(1, delayBufferLength);
        delayBuffer.clear();
        writePosition = 0;

        // Allocate FR delay buffer (same size)
        frDelayBuffer.setSize(1, delayBufferLength);
        frDelayBuffer.clear();

        // Setup input ring buffers - one for each input channel
        for (auto& inputBuffer : inputBuffers)
            inputBuffer->setSize(maxBlockSize * 4);

        // Setup output ring buffer
        outputRingBuffer.setSize(maxBlockSize * 4);

        // Initialize HF filters
        for (auto& filter : hfFilters)
        {
            filter.prepare(sampleRate);
            filter.setGainDb(0.0f);  // Start with no attenuation
        }

        // Initialize FR filters (one set per input)
        for (size_t i = 0; i < frLowCutFilters.size(); ++i)
        {
            frLowCutFilters[i].prepare(sampleRate);
            frLowCutFilters[i].setType(WFSBiquadFilter::FilterType::LowCut);
            frLowCutFilters[i].setFrequency(100.0f);

            frHighShelfFilters[i].prepare(sampleRate);
            frHighShelfFilters[i].setType(WFSBiquadFilter::FilterType::HighShelf);
            frHighShelfFilters[i].setFrequency(3000.0f);
            frHighShelfFilters[i].setGainDb(-2.0f);
            frHighShelfFilters[i].setSlope(0.4f);

            frHFFilters[i].prepare(sampleRate);
            frHFFilters[i].setGainDb(0.0f);
        }

        // Initialize diffusion random generator with output-specific seed
        frRandom.seed(static_cast<unsigned int>(outputChannelIndex * 54321 + 98765));
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
        frDelayBuffer.clear();
        writePosition = 0;

        // Reset HF filters
        for (auto& filter : hfFilters)
            filter.reset();

        // Reset FR filters
        for (size_t i = 0; i < frLowCutFilters.size(); ++i)
        {
            frLowCutFilters[i].reset();
            frHighShelfFilters[i].reset();
            frHFFilters[i].reset();
        }
        std::fill(frDiffusionState.begin(), frDiffusionState.end(), 0.0f);
        std::fill(frDiffusionTarget.begin(), frDiffusionTarget.end(), 0.0f);
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

    // === Floor Reflection parameter setters (called from timer thread at 50Hz) ===

    /** Set FR filter parameters for a specific input */
    void setFRFilterParams(int inputIndex,
                           bool lowCutActive, float lowCutFreq,
                           bool highShelfActive, float highShelfFreq,
                           float highShelfGain, float highShelfSlope)
    {
        if (inputIndex < 0 || inputIndex >= storedNumInputs)
            return;

        frLowCutActiveFlags[static_cast<size_t>(inputIndex)].store(lowCutActive, std::memory_order_release);
        if (lowCutActive)
            frLowCutFilters[static_cast<size_t>(inputIndex)].setFrequency(lowCutFreq);

        frHighShelfActiveFlags[static_cast<size_t>(inputIndex)].store(highShelfActive, std::memory_order_release);
        if (highShelfActive)
        {
            frHighShelfFilters[static_cast<size_t>(inputIndex)].setFrequency(highShelfFreq);
            frHighShelfFilters[static_cast<size_t>(inputIndex)].setGainDb(highShelfGain);
            frHighShelfFilters[static_cast<size_t>(inputIndex)].setSlope(highShelfSlope);
        }
    }

    /** Set FR diffusion amount for a specific input (0-100%) */
    void setFRDiffusion(int inputIndex, float diffusionPercent)
    {
        if (inputIndex < 0 || inputIndex >= storedNumInputs)
            return;

        // Max jitter is 5ms at 100% diffusion
        frMaxJitterMs[static_cast<size_t>(inputIndex)].store(diffusionPercent * 0.05f, std::memory_order_release);
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
        auto* frDelayData = frDelayBuffer.getWritePointer(0);
        auto* outputData = output.getWritePointer(0);

        // Safety check
        if (delayBufferLength == 0 || delayData == nullptr || frDelayData == nullptr)
            return;

        // Update diffusion jitter once per block
        updateDiffusionJitter();

        // Process each sample
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Read output from current position (direct + FR buffers)
            outputData[sample] = delayData[writePosition] + frDelayData[writePosition];

            // Clear this position after reading
            delayData[writePosition] = 0.0f;
            frDelayData[writePosition] = 0.0f;

            // Now accumulate contributions from all inputs with their respective delays
            for (int inChannel = 0; inChannel < numInputChannels; ++inChannel)
            {
                // Get input sample
                float inputSample = inputs[inChannel].getReadPointer(0)[sample];

                // Calculate index into shared arrays: [inputChannel * numOutputChannels + outputChannel]
                int routingIndex = inChannel * numOutputChannels + outputChannelIndex;

                // Get levels for direct and FR
                float directLevel = sharedLevels[routingIndex];
                float frLevel = (sharedFRLevels != nullptr) ? sharedFRLevels[routingIndex] : 0.0f;

                // Optimization: skip if both levels are zero
                if (directLevel == 0.0f && frLevel == 0.0f)
                    continue;

                // ==========================================
                // Direct signal processing
                // ==========================================
                if (directLevel > 0.0f)
                {
                    // Update HF filter gain if pointer is provided
                    if (sharedHFAttenuation != nullptr)
                    {
                        float hfGainDb = sharedHFAttenuation[routingIndex];
                        hfFilters[inChannel].setGainDb(hfGainDb);
                    }

                    // Apply HF filter (air absorption) to input sample
                    float filteredSample = hfFilters[inChannel].processSample(inputSample);

                    // Get delay time in milliseconds and convert to samples
                    float directDelayMs = sharedDelayTimes[routingIndex];
                    float directDelaySamples = (directDelayMs / 1000.0f) * (float)currentSampleRate;

                    // Ensure delay doesn't exceed buffer length
                    if (directDelaySamples >= (float)delayBufferLength)
                        directDelaySamples = (float)(delayBufferLength - 1);

                    // Calculate write position with delay offset
                    float exactWritePos = (float)writePosition + directDelaySamples;
                    while (exactWritePos >= (float)delayBufferLength)
                        exactWritePos -= (float)delayBufferLength;

                    // Split into integer and fractional parts for interpolation
                    int writePos1 = (int)exactWritePos;
                    int writePos2 = (writePos1 + 1) % delayBufferLength;
                    float fraction = exactWritePos - (int)exactWritePos;

                    // Apply level to filtered sample
                    float contribution = filteredSample * directLevel;

                    // Write with linear interpolation
                    delayData[writePos1] += contribution * (1.0f - fraction);
                    delayData[writePos2] += contribution * fraction;
                }

                // ==========================================
                // Floor Reflection signal processing
                // ==========================================
                if (frLevel > 0.0f)
                {
                    // Apply FR filters to input sample
                    float frFilteredSample = inputSample;
                    if (frLowCutActiveFlags[static_cast<size_t>(inChannel)].load(std::memory_order_acquire))
                        frFilteredSample = frLowCutFilters[static_cast<size_t>(inChannel)].processSample(frFilteredSample);
                    if (frHighShelfActiveFlags[static_cast<size_t>(inChannel)].load(std::memory_order_acquire))
                        frFilteredSample = frHighShelfFilters[static_cast<size_t>(inChannel)].processSample(frFilteredSample);

                    // Update FR HF filter gain
                    if (sharedFRHFAttenuation != nullptr)
                    {
                        float frHFGainDb = sharedFRHFAttenuation[routingIndex];
                        frHFFilters[static_cast<size_t>(inChannel)].setGainDb(frHFGainDb);
                    }

                    // Apply FR HF filter (air absorption for longer path)
                    frFilteredSample = frHFFilters[static_cast<size_t>(inChannel)].processSample(frFilteredSample);

                    // Get FR delay: direct delay + extra delay + diffusion jitter
                    float directDelayMs = sharedDelayTimes[routingIndex];
                    float frExtraDelayMs = (sharedFRDelayTimes != nullptr) ? sharedFRDelayTimes[routingIndex] : 0.0f;
                    float frJitterMs = frDiffusionState[static_cast<size_t>(inChannel)];
                    float totalFRDelayMs = directDelayMs + frExtraDelayMs + frJitterMs;
                    if (totalFRDelayMs < 0.0f) totalFRDelayMs = 0.0f;

                    float frDelaySamples = (totalFRDelayMs / 1000.0f) * (float)currentSampleRate;
                    if (frDelaySamples >= (float)delayBufferLength)
                        frDelaySamples = (float)(delayBufferLength - 1);

                    // Calculate write position with FR delay offset
                    float exactWritePos = (float)writePosition + frDelaySamples;
                    while (exactWritePos >= (float)delayBufferLength)
                        exactWritePos -= (float)delayBufferLength;

                    int writePos1 = (int)exactWritePos;
                    int writePos2 = (writePos1 + 1) % delayBufferLength;
                    float fraction = exactWritePos - (int)exactWritePos;

                    // Apply FR level
                    float frContribution = frFilteredSample * frLevel;

                    // Write to FR delay buffer with linear interpolation
                    frDelayData[writePos1] += frContribution * (1.0f - fraction);
                    frDelayData[writePos2] += frContribution * fraction;
                }
            }

            // Advance write/read position
            writePosition = (writePosition + 1) % delayBufferLength;
        }
    }

    /** Update time-varying diffusion jitter (called once per block) */
    void updateDiffusionJitter()
    {
        const float smoothingFactor = 0.05f;

        for (size_t inIdx = 0; inIdx < frDiffusionState.size(); ++inIdx)
        {
            float maxJitter = frMaxJitterMs[inIdx].load(std::memory_order_acquire);

            // Occasionally update the target jitter
            if (frDiffusionUpdateCounter >= 3)
            {
                std::uniform_real_distribution<float> dist(-maxJitter, maxJitter);
                frDiffusionTarget[inIdx] = dist(frRandom);
            }

            // Smooth towards target
            frDiffusionState[inIdx] += (frDiffusionTarget[inIdx] - frDiffusionState[inIdx]) * smoothingFactor;
        }

        frDiffusionUpdateCounter++;
        if (frDiffusionUpdateCounter >= 3)
            frDiffusionUpdateCounter = 0;
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
    const float* sharedDelayTimes;      // delays[inputChannel * numOutputs + outputChannel]
    const float* sharedLevels;          // levels[inputChannel * numOutputs + outputChannel]
    const float* sharedHFAttenuation;   // HF attenuation dB[inputChannel * numOutputs + outputChannel]

    // Pointers to shared FR routing matrices (owned by MainComponent)
    const float* sharedFRDelayTimes;    // FR extra delays[inputChannel * numOutputs + outputChannel]
    const float* sharedFRLevels;        // FR levels[inputChannel * numOutputs + outputChannel]
    const float* sharedFRHFAttenuation; // FR HF attenuation dB[inputChannel * numOutputs + outputChannel]

    // HF shelf filters for air absorption (one per input channel)
    std::vector<WFSHighShelfFilter> hfFilters;

    // Floor Reflection components
    juce::AudioBuffer<float> frDelayBuffer;  // Separate delay buffer for FR signals

    // FR filters (one per input channel)
    std::vector<WFSBiquadFilter> frLowCutFilters;
    std::vector<WFSBiquadFilter> frHighShelfFilters;
    std::vector<WFSHighShelfFilter> frHFFilters;  // FR air absorption per input

    // FR filter enable flags (one per input) - relaxed atomics for thread safety
    std::atomic<bool>* frLowCutActiveFlags = nullptr;
    std::atomic<bool>* frHighShelfActiveFlags = nullptr;

    // FR diffusion (time-varying jitter per input)
    std::vector<float> frDiffusionState;   // Current jitter value per input
    std::vector<float> frDiffusionTarget;  // Target jitter value per input
    std::atomic<float>* frMaxJitterMs = nullptr;  // Max jitter per input
    int storedNumInputs = 0;  // Store for cleanup
    std::mt19937 frRandom;                 // Random generator for diffusion
    int frDiffusionUpdateCounter = 0;      // Counter for updating targets

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputBufferProcessor)
};
