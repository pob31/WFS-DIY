#pragma once

#include <JuceHeader.h>
#include "../LockFreeRingBuffer.h"
#include "WFSHighShelfFilter.h"
#include "WFSBiquadFilter.h"
#include "LiveSourceLevelDetector.h"
#include <atomic>
#include <memory>
#include <random>

//==============================================================================
/**
    Processes a single input channel with delay lines outputting to multiple channels.
    Runs on its own thread for parallel processing.

    Includes HF shelf filters (800Hz, Q=0.3) for air absorption simulation.
    One filter per output channel.
*/
class InputBufferProcessor : public juce::Thread
{
public:
    InputBufferProcessor(int inputIndex, int numOutputs,
                         const float* delayTimesPtr, const float* levelsPtr,
                         const float* hfAttenuationPtr = nullptr,
                         const float* frDelayTimesPtr = nullptr,
                         const float* frLevelsPtr = nullptr,
                         const float* frHFAttenuationPtr = nullptr)
        : juce::Thread("InputBufferProcessor_" + juce::String(inputIndex)),
          inputChannelIndex(inputIndex),
          numOutputChannels(numOutputs),
          sharedDelayTimes(delayTimesPtr),
          sharedLevels(levelsPtr),
          sharedHFAttenuation(hfAttenuationPtr),
          sharedFRDelayTimes(frDelayTimesPtr),
          sharedFRLevels(frLevelsPtr),
          sharedFRHFAttenuation(frHFAttenuationPtr)
    {
        // Pre-allocate output buffers and HF filters
        for (int i = 0; i < numOutputs; ++i)
        {
            outputBuffers.emplace_back(std::make_unique<LockFreeRingBuffer>());
            hfFilters.emplace_back();  // One filter per output
            frHFFilters.emplace_back();  // FR HF filter per output
        }

        // Initialize diffusion state for time-varying jitter (one per output)
        frDiffusionState.resize(static_cast<size_t>(numOutputs), 0.0f);
        frDiffusionTarget.resize(static_cast<size_t>(numOutputs), 0.0f);
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

        // Allocate FR delay buffer (same size as main buffer)
        frDelayBuffer.setSize(1, delayBufferLength);
        frDelayBuffer.clear();
        frWritePosition = 0;

        // Setup input ring buffer - make it 4x block size for safety
        inputRingBuffer.setSize(maxBlockSize * 4);

        // Setup output ring buffers for each output channel
        for (auto& outputBuffer : outputBuffers)
            outputBuffer->setSize(maxBlockSize * 4);

        // Initialize HF filters
        for (auto& filter : hfFilters)
        {
            filter.prepare(sampleRate);
            filter.setGainDb(0.0f);  // Start with no attenuation
        }

        // Initialize FR input filters (per-input, shared across outputs)
        frLowCutFilter.prepare(sampleRate);
        frLowCutFilter.setType(WFSBiquadFilter::FilterType::LowCut);
        frLowCutFilter.setFrequency(100.0f);  // Default 100 Hz

        frHighShelfFilter.prepare(sampleRate);
        frHighShelfFilter.setType(WFSBiquadFilter::FilterType::HighShelf);
        frHighShelfFilter.setFrequency(3000.0f);  // Default 3000 Hz
        frHighShelfFilter.setGainDb(-2.0f);       // Default -2 dB
        frHighShelfFilter.setSlope(0.4f);         // Default 0.4 slope

        // Initialize FR HF filters (per-output, for air absorption)
        for (auto& filter : frHFFilters)
        {
            filter.prepare(sampleRate);
            filter.setGainDb(0.0f);
        }

        // Initialize diffusion random generator with input-specific seed
        frRandom.seed(static_cast<unsigned int>(inputChannelIndex * 12345 + 67890));

        // Initialize Live Source level detector
        lsDetector = std::make_unique<LiveSourceLevelDetector>();
        lsDetector->prepare(sampleRate);
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

        // Reset HF filters
        for (auto& filter : hfFilters)
            filter.reset();

        // Reset FR components
        frDelayBuffer.clear();
        frWritePosition = 0;
        frLowCutFilter.reset();
        frHighShelfFilter.reset();
        for (auto& filter : frHFFilters)
            filter.reset();
        std::fill(frDiffusionState.begin(), frDiffusionState.end(), 0.0f);
        std::fill(frDiffusionTarget.begin(), frDiffusionTarget.end(), 0.0f);
    }

    void setProcessingEnabled(bool enabled)
    {
        processingEnabled.store(enabled, std::memory_order_release);
    }

    int getInputChannelIndex() const { return inputChannelIndex; }

    // === Live Source Tamer accessors ===

    // Get peak gain reduction (linear, 0-1) from level detector
    float getLSPeakGainReduction() const
    {
        return lsDetector ? lsDetector->getPeakGainReduction() : 1.0f;
    }

    // Get slow gain reduction (linear, 0-1) from level detector
    float getLSSlowGainReduction() const
    {
        return lsDetector ? lsDetector->getSlowGainReduction() : 1.0f;
    }

    // Get short peak level in dB (5ms hold for AutomOtion triggering)
    float getShortPeakLevelDb() const
    {
        return lsDetector ? lsDetector->getShortPeakLevelDb() : -200.0f;
    }

    // Get RMS level in dB (200ms window)
    float getRmsLevelDb() const
    {
        return lsDetector ? lsDetector->getRmsLevelDb() : -200.0f;
    }

    // Set Live Source compressor parameters (called from timer thread)
    void setLSParameters(float peakThreshDb, float peakRatio,
                         float slowThreshDb, float slowRatio)
    {
        if (lsDetector)
            lsDetector->setParameters(peakThreshDb, peakRatio, slowThreshDb, slowRatio);
    }

    // === Floor Reflection parameter setters (called from timer thread at 50Hz) ===

    /** Set FR filter parameters for this input */
    void setFRFilterParams(bool lowCutActive, float lowCutFreq,
                           bool highShelfActive, float highShelfFreq,
                           float highShelfGain, float highShelfSlope)
    {
        frLowCutActive.store(lowCutActive, std::memory_order_release);
        if (lowCutActive)
            frLowCutFilter.setFrequency(lowCutFreq);

        frHighShelfActive.store(highShelfActive, std::memory_order_release);
        if (highShelfActive)
        {
            frHighShelfFilter.setFrequency(highShelfFreq);
            frHighShelfFilter.setGainDb(highShelfGain);
            frHighShelfFilter.setSlope(highShelfSlope);
        }
    }

    /** Set FR diffusion amount (0-100%) */
    void setFRDiffusion(float diffusionPercent)
    {
        // Max jitter is 5ms at 100% diffusion
        frMaxJitterMs.store(diffusionPercent * 0.05f, std::memory_order_release);  // 5ms / 100 = 0.05
    }

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
        auto* frDelayData = frDelayBuffer.getWritePointer(0);

        // Safety check
        if (delayBufferLength == 0 || delayData == nullptr || frDelayData == nullptr)
            return;

        // Get FR filter enable states
        bool lowCutActive = frLowCutActive.load(std::memory_order_acquire);
        bool highShelfActive = frHighShelfActive.load(std::memory_order_acquire);

        // Write input to both delay buffers
        // Direct buffer gets unfiltered input, FR buffer gets filtered input
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float inputSample = input[sample];

            // Direct path: write unfiltered to delay buffer
            delayData[writePosition] = inputSample;

            // FR path: apply filters then write to FR delay buffer
            float frSample = inputSample;
            if (lowCutActive)
                frSample = frLowCutFilter.processSample(frSample);
            if (highShelfActive)
                frSample = frHighShelfFilter.processSample(frSample);
            frDelayData[frWritePosition] = frSample;

            // Advance write positions together
            writePosition = (writePosition + 1) % delayBufferLength;
            frWritePosition = (frWritePosition + 1) % delayBufferLength;

            // Live Source level detection (runs on every sample)
            if (lsDetector)
                lsDetector->processSample(inputSample);
        }

        // Reset write positions to process outputs
        writePosition = (writePosition - numSamples + delayBufferLength) % delayBufferLength;
        frWritePosition = (frWritePosition - numSamples + delayBufferLength) % delayBufferLength;

        // Update diffusion jitter for time-varying effect (once per block, ~every 64 samples)
        float maxJitter = frMaxJitterMs.load(std::memory_order_acquire);
        updateDiffusionJitter(maxJitter);

        // Generate delayed outputs for each output channel
        for (int outChannel = 0; outChannel < numOutputChannels; ++outChannel)
        {
            auto* outputData = outputs.getWritePointer(outChannel);

            // Calculate index into shared arrays: [inputChannel * numOutputs + outputChannel]
            int routingIndex = inputChannelIndex * numOutputChannels + outChannel;

            // Get direct and FR levels for this output
            float directLevel = sharedLevels[routingIndex];
            float frLevel = (sharedFRLevels != nullptr) ? sharedFRLevels[routingIndex] : 0.0f;

            // Optimization: skip processing if both levels are zero
            if (directLevel == 0.0f && frLevel == 0.0f)
            {
                // Write silence
                for (int sample = 0; sample < numSamples; ++sample)
                    outputData[sample] = 0.0f;
                continue;
            }

            // Get direct delay parameters
            float directDelayMs = sharedDelayTimes[routingIndex];
            float directDelaySamples = (directDelayMs / 1000.0f) * (float)currentSampleRate;
            if (directDelaySamples >= (float)delayBufferLength)
                directDelaySamples = (float)(delayBufferLength - 1);

            // Update direct HF filter gain
            if (sharedHFAttenuation != nullptr)
            {
                float hfGainDb = sharedHFAttenuation[routingIndex];
                hfFilters[outChannel].setGainDb(hfGainDb);
            }

            // Get FR delay parameters (extra delay + diffusion jitter)
            float frExtraDelayMs = (sharedFRDelayTimes != nullptr) ? sharedFRDelayTimes[routingIndex] : 0.0f;
            float frJitterMs = frDiffusionState[static_cast<size_t>(outChannel)];
            float totalFRDelayMs = directDelayMs + frExtraDelayMs + frJitterMs;
            float frDelaySamples = (totalFRDelayMs / 1000.0f) * (float)currentSampleRate;
            if (frDelaySamples < 0.0f)
                frDelaySamples = 0.0f;
            if (frDelaySamples >= (float)delayBufferLength)
                frDelaySamples = (float)(delayBufferLength - 1);

            // Update FR HF filter gain (FR uses additional HF attenuation on top of direct)
            if (sharedFRHFAttenuation != nullptr)
            {
                float frHFGainDb = sharedFRHFAttenuation[routingIndex];
                frHFFilters[outChannel].setGainDb(frHFGainDb);
            }

            // Process each sample
            for (int sample = 0; sample < numSamples; ++sample)
            {
                float outputSample = 0.0f;

                // ==========================================
                // Direct signal
                // ==========================================
                if (directLevel > 0.0f)
                {
                    // Calculate fractional read position for direct signal
                    float exactReadPos = (float)writePosition + (float)sample - directDelaySamples;
                    while (exactReadPos < 0.0f)
                        exactReadPos += (float)delayBufferLength;

                    int readPos1 = (int)exactReadPos % delayBufferLength;
                    int readPos2 = (readPos1 + 1) % delayBufferLength;
                    float fraction = exactReadPos - (int)exactReadPos;

                    // Linear interpolation
                    float sample1 = delayData[readPos1];
                    float sample2 = delayData[readPos2];
                    float interpolatedSample = sample1 + fraction * (sample2 - sample1);

                    // Apply HF filter (air absorption)
                    float filteredSample = hfFilters[outChannel].processSample(interpolatedSample);

                    outputSample += filteredSample * directLevel;
                }

                // ==========================================
                // Floor Reflection signal
                // ==========================================
                if (frLevel > 0.0f)
                {
                    // Calculate fractional read position for FR signal (from FR-filtered buffer)
                    float exactReadPos = (float)frWritePosition + (float)sample - frDelaySamples;
                    while (exactReadPos < 0.0f)
                        exactReadPos += (float)delayBufferLength;

                    int readPos1 = (int)exactReadPos % delayBufferLength;
                    int readPos2 = (readPos1 + 1) % delayBufferLength;
                    float fraction = exactReadPos - (int)exactReadPos;

                    // Linear interpolation from FR-filtered buffer
                    float sample1 = frDelayData[readPos1];
                    float sample2 = frDelayData[readPos2];
                    float interpolatedSample = sample1 + fraction * (sample2 - sample1);

                    // Apply FR HF filter (additional air absorption for longer path)
                    float filteredSample = frHFFilters[outChannel].processSample(interpolatedSample);

                    outputSample += filteredSample * frLevel;
                }

                outputData[sample] = outputSample;
            }
        }

        // Advance write positions
        writePosition = (writePosition + numSamples) % delayBufferLength;
        frWritePosition = (frWritePosition + numSamples) % delayBufferLength;
    }

    /** Update time-varying diffusion jitter (called once per block) */
    void updateDiffusionJitter(float maxJitterMs)
    {
        // Smoothing factor for jitter transitions (~50Hz update at 64-sample blocks @ 48kHz)
        const float smoothingFactor = 0.05f;

        // Update each output's jitter
        for (size_t outIdx = 0; outIdx < frDiffusionState.size(); ++outIdx)
        {
            // Occasionally update the target jitter
            if (frDiffusionUpdateCounter >= 3)  // Update target every ~3 blocks
            {
                // Generate random value in range [-maxJitter, +maxJitter]
                std::uniform_real_distribution<float> dist(-maxJitterMs, maxJitterMs);
                frDiffusionTarget[outIdx] = dist(frRandom);
            }

            // Smooth towards target
            frDiffusionState[outIdx] += (frDiffusionTarget[outIdx] - frDiffusionState[outIdx]) * smoothingFactor;
        }

        frDiffusionUpdateCounter++;
        if (frDiffusionUpdateCounter >= 3)
            frDiffusionUpdateCounter = 0;
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
    const float* sharedDelayTimes;      // delays[inputChannel * numOutputs + outputChannel]
    const float* sharedLevels;          // levels[inputChannel * numOutputs + outputChannel]
    const float* sharedHFAttenuation;   // HF attenuation dB[inputChannel * numOutputs + outputChannel]

    // Pointers to shared FR routing matrices (owned by MainComponent)
    const float* sharedFRDelayTimes;    // FR extra delays[inputChannel * numOutputs + outputChannel]
    const float* sharedFRLevels;        // FR levels[inputChannel * numOutputs + outputChannel]
    const float* sharedFRHFAttenuation; // FR HF attenuation dB[inputChannel * numOutputs + outputChannel]

    // HF shelf filters for air absorption (one per output channel)
    std::vector<WFSHighShelfFilter> hfFilters;

    // Floor Reflection components
    juce::AudioBuffer<float> frDelayBuffer;  // Separate delay buffer for FR-filtered audio
    int frWritePosition = 0;

    // FR input filters (per-input, shared across all outputs)
    WFSBiquadFilter frLowCutFilter;
    WFSBiquadFilter frHighShelfFilter;
    std::atomic<bool> frLowCutActive {false};
    std::atomic<bool> frHighShelfActive {false};

    // FR HF filters for air absorption (one per output channel)
    std::vector<WFSHighShelfFilter> frHFFilters;

    // FR diffusion (time-varying jitter per output)
    std::vector<float> frDiffusionState;   // Current jitter value per output
    std::vector<float> frDiffusionTarget;  // Target jitter value per output
    std::atomic<float> frMaxJitterMs {0.0f};  // Max jitter in ms (set from timer thread)
    std::mt19937 frRandom;                 // Random generator for diffusion
    int frDiffusionUpdateCounter = 0;      // Counter for updating targets

    // Live Source level detector (for peak/slow compression)
    std::unique_ptr<LiveSourceLevelDetector> lsDetector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InputBufferProcessor)
};
