#pragma once

#include <JuceHeader.h>
#include "../LockFreeRingBuffer.h"
#include "SharedInputRingBuffer.h"
#include "OutputLevelDetector.h"
#include "WFSHighShelfFilter.h"
#include "WFSBiquadFilter.h"
#include "DelayTargetSmoother.h"
#include <atomic>
#include <cstdint>
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
        // Pre-allocate HF filters (one per input channel)
        for (int i = 0; i < numInputs; ++i)
        {
            hfFilters.emplace_back();  // One filter per input for direct signal
            frLowCutFilters.emplace_back();  // FR low-cut filter per input
            frHighShelfFilters.emplace_back();  // FR high-shelf filter per input
            frHFFilters.emplace_back();  // FR HF air absorption per input
        }

        // Per-input read positions for shared input buffers
        inputReadPositions.resize(static_cast<size_t>(numInputs), 0);

        // Per-input delay smoothers (direct + FR). Window is set in prepare() once
        // we know the sample rate.
        smootherDirect.resize(static_cast<size_t>(numInputs));
        smootherFR.resize(static_cast<size_t>(numInputs));

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

        // Setup output ring buffer + metering buffer
        outputRingBuffer.setSize(maxBlockSize * 4);
        meteringRingBuffer.setSize(maxBlockSize * 4);

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

        // Configure the per-input delay smoothers for a ~10 ms box window at
        // the current sample rate.
        const int windowSamples = juce::jmax(2, static_cast<int>(sampleRate * 0.010));
        for (auto& s : smootherDirect) s.prepare(windowSamples);
        for (auto& s : smootherFR)     s.prepare(windowSamples);
        sampleCounter = 0;

        // Initialize diffusion random generator with output-specific seed
        frRandom.seed(static_cast<unsigned int>(outputChannelIndex * 54321 + 98765));
    }

    /** Set shared input buffer pointers (called once at prepare time). */
    void setSharedInputBuffers(const std::vector<std::unique_ptr<SharedInputRingBuffer>>& buffers)
    {
        sharedInputBuffers.clear();
        for (auto& buf : buffers)
            sharedInputBuffers.push_back(buf.get());

        inputReadPositions.assign(sharedInputBuffers.size(), 0);
    }

    /** Notify this processor that new input data is available. */
    void notifyInputAvailable(int available)
    {
        samplesAvailable.store(available, std::memory_order_release);
        notify();
    }

    /** Read metering output data (called by OutputMeteringThread). */
    int readMeteringOutput(float* destination, int numSamples)
    {
        return meteringRingBuffer.read(destination, numSamples);
    }

    /** Get available metering samples. */
    int getMeteringAvailable() const { return meteringRingBuffer.getAvailableData(); }

    // Called by audio thread to pull output data
    int pullOutput(float* destination, int numSamples)
    {
        return outputRingBuffer.read(destination, numSamples);
    }

    void reset()
    {
        std::fill(inputReadPositions.begin(), inputReadPositions.end(), 0);
        outputRingBuffer.reset();
        meteringRingBuffer.reset();
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
        if (enabled)
            notify(); // Wake thread from infinite sleep
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
            // Sleep when processing is disabled (no work to do)
            if (!processingEnabled.load(std::memory_order_acquire))
            {
                wait(-1); // Infinite sleep until notify() from setProcessingEnabled
                continue;
            }

            // Wait for input data from all channels
            if (samplesAvailable.load(std::memory_order_acquire) < processingBlockSize)
            {
                wait(1); // Wait 1ms
                continue;
            }

            // Read input samples from shared input buffers (each processor has own read position)
            int samplesRead = processingBlockSize;
            for (int inChannel = 0; inChannel < numInputChannels && inChannel < (int)sharedInputBuffers.size(); ++inChannel)
            {
                int read = sharedInputBuffers[inChannel]->readWithPosition(
                    inputReadPositions[inChannel],
                    inputBlocks[inChannel].getWritePointer(0),
                    processingBlockSize);
                samplesRead = juce::jmin(samplesRead, read);
            }

            // Update available count from this processor's read positions
            int minAvailable = std::numeric_limits<int>::max();
            for (int i = 0; i < numInputChannels && i < (int)sharedInputBuffers.size(); ++i)
                minAvailable = juce::jmin(minAvailable, sharedInputBuffers[i]->getAvailableAt(inputReadPositions[i]));
            samplesAvailable.store(minAvailable, std::memory_order_release);

            if (samplesRead == 0)
                continue;

            // Process block (processingEnabled already checked at top of loop)
            {
                auto processStartTime = juce::Time::getMillisecondCounterHiRes();

                processBlock(inputBlocks, outputBlock, samplesRead);

                auto processEndTime = juce::Time::getMillisecondCounterHiRes();
                double blockProcessTime = processEndTime - processStartTime;

                processingTimeMs += blockProcessTime;
                processingTimeMsForAvg += blockProcessTime;
                processedBlockCount++;

                // Write processed output to output ring buffer + metering buffer
                auto* outData = outputBlock.getReadPointer(0);
                outputRingBuffer.write(outData, samplesRead);
                meteringRingBuffer.write(outData, samplesRead);
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

        // Precompute current delay values per input for interpolation
        float msToSamples = (float)currentSampleRate / 1000.0f;
        float maxDelay = (float)(delayBufferLength - 1);

        // Feed raw delay targets to the per-input smoothers once per block.
        // The smoothers build a box-filtered piecewise-linear approximation of
        // the 50 Hz target trajectory; the per-sample loop below queries them.
        for (int inChannel = 0; inChannel < numInputChannels; ++inChannel)
        {
            int routingIndex = inChannel * numOutputChannels + outputChannelIndex;
            size_t inIdx = static_cast<size_t>(inChannel);

            // Direct delay (clamped to buffer length)
            float directDelayMs = sharedDelayTimes[routingIndex];
            float directDelay = directDelayMs * msToSamples;
            if (directDelay >= maxDelay) directDelay = maxDelay;

            // FR delay includes the diffusion jitter. Jitter is slowly-varying
            // (updated every 3 blocks, smoothingFactor=0.05) so feeding it in
            // pre-smoother preserves its intended diffusion character while
            // still removing block-to-block slope jumps.
            float frExtraDelayMs = (sharedFRDelayTimes != nullptr) ? sharedFRDelayTimes[routingIndex] : 0.0f;
            float frJitterMs = frDiffusionState[inIdx];
            float totalFRDelayMs = directDelayMs + frExtraDelayMs + frJitterMs;
            if (totalFRDelayMs < 0.0f) totalFRDelayMs = 0.0f;
            float frDelay = totalFRDelayMs * msToSamples;
            if (frDelay >= maxDelay) frDelay = maxDelay;

            smootherDirect[inIdx].observe (directDelay, sampleCounter);
            smootherFR[inIdx].observe     (frDelay,     sampleCounter);
        }

        // Pre-cache per-input filter state and gains outside the sample loop
        // to avoid repeated atomic loads and filter coefficient updates per sample
        struct PerInputCache {
            float directLevel = 0.0f;
            float frLevel = 0.0f;
            bool frLowCutActive = false;
            bool frHighShelfActive = false;
        };
        std::vector<PerInputCache> inputCache(static_cast<size_t>(numInputChannels));

        for (int inChannel = 0; inChannel < numInputChannels; ++inChannel)
        {
            int routingIndex = inChannel * numOutputChannels + outputChannelIndex;
            size_t inIdx = static_cast<size_t>(inChannel);
            auto& ic = inputCache[inIdx];

            ic.directLevel = sharedLevels[routingIndex];
            ic.frLevel = (sharedFRLevels != nullptr) ? sharedFRLevels[routingIndex] : 0.0f;

            // Update HF filter gains once per block (not per sample)
            if (ic.directLevel > 0.0f && sharedHFAttenuation != nullptr)
                hfFilters[inChannel].setGainDb(sharedHFAttenuation[routingIndex]);

            if (ic.frLevel > 0.0f)
            {
                ic.frLowCutActive = frLowCutActiveFlags[inIdx].load(std::memory_order_relaxed);
                ic.frHighShelfActive = frHighShelfActiveFlags[inIdx].load(std::memory_order_relaxed);

                if (sharedFRHFAttenuation != nullptr)
                    frHFFilters[inIdx].setGainDb(sharedFRHFAttenuation[routingIndex]);
            }
        }

        // Process each sample
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Read output from current position (direct + FR buffers)
            outputData[sample] = delayData[writePosition] + frDelayData[writePosition];

            // Clear this position after reading
            delayData[writePosition] = 0.0f;
            frDelayData[writePosition] = 0.0f;

            const std::int64_t currentSample = sampleCounter + sample;

            // Now accumulate contributions from all inputs with their respective delays
            for (int inChannel = 0; inChannel < numInputChannels; ++inChannel)
            {
                size_t inIdx = static_cast<size_t>(inChannel);
                auto& ic = inputCache[inIdx];

                // Optimization: skip if both levels are zero
                if (ic.directLevel == 0.0f && ic.frLevel == 0.0f)
                    continue;

                // Get input sample
                float inputSample = inputs[inChannel].getReadPointer(0)[sample];

                // ==========================================
                // Direct signal processing
                // ==========================================
                if (ic.directLevel > 0.0f)
                {
                    // Apply HF filter (air absorption) to input sample
                    float filteredSample = hfFilters[inChannel].processSample(inputSample);

                    // Smoothed delay + teleport gain envelope
                    auto smoothed = smootherDirect[inIdx].smoothedAt (currentSample);
                    float directDelaySamples = smoothed.delay;

                    // Calculate write position with delay offset
                    float exactWritePos = (float)writePosition + directDelaySamples;
                    while (exactWritePos >= (float)delayBufferLength)
                        exactWritePos -= (float)delayBufferLength;

                    // Split into integer and fractional parts for interpolation
                    int writePos1 = (int)exactWritePos;
                    int writePos2 = (writePos1 + 1) % delayBufferLength;
                    float fraction = exactWritePos - (int)exactWritePos;

                    // Apply level (multiplied by the teleport gain, = 1.0 outside a teleport)
                    float contribution = filteredSample * ic.directLevel * smoothed.gain;

                    // Write with linear interpolation
                    delayData[writePos1] += contribution * (1.0f - fraction);
                    delayData[writePos2] += contribution * fraction;
                }

                // ==========================================
                // Floor Reflection signal processing
                // ==========================================
                if (ic.frLevel > 0.0f)
                {
                    // Apply FR filters to input sample (flags cached outside loop)
                    float frFilteredSample = inputSample;
                    if (ic.frLowCutActive)
                        frFilteredSample = frLowCutFilters[inIdx].processSample(frFilteredSample);
                    if (ic.frHighShelfActive)
                        frFilteredSample = frHighShelfFilters[inIdx].processSample(frFilteredSample);

                    // Apply FR HF filter (air absorption for longer path)
                    frFilteredSample = frHFFilters[inIdx].processSample(frFilteredSample);

                    // Smoothed FR delay + teleport gain envelope (independent smoother)
                    auto smoothedFr = smootherFR[inIdx].smoothedAt (currentSample);
                    float frDelaySamples = smoothedFr.delay;

                    // Calculate write position with FR delay offset
                    float exactWritePos = (float)writePosition + frDelaySamples;
                    while (exactWritePos >= (float)delayBufferLength)
                        exactWritePos -= (float)delayBufferLength;

                    int writePos1 = (int)exactWritePos;
                    int writePos2 = (writePos1 + 1) % delayBufferLength;
                    float fraction = exactWritePos - (int)exactWritePos;

                    // Apply FR level (multiplied by the teleport gain)
                    float frContribution = frFilteredSample * ic.frLevel * smoothedFr.gain;

                    // Write to FR delay buffer with linear interpolation
                    frDelayData[writePos1] += frContribution * (1.0f - fraction);
                    frDelayData[writePos2] += frContribution * fraction;
                }
            }

            // Advance write/read position
            writePosition = (writePosition + 1) % delayBufferLength;
        }

        // Advance the monotonic sample counter for the next block's smoother queries.
        sampleCounter += numSamples;
    }

    /** Update time-varying diffusion jitter (called once per block) */
    void updateDiffusionJitter()
    {
        const float smoothingFactor = 0.05f;

        for (size_t inIdx = 0; inIdx < frDiffusionState.size(); ++inIdx)
        {
            float maxJitter = frMaxJitterMs[inIdx].load(std::memory_order_relaxed);

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
    std::vector<SharedInputRingBuffer*> sharedInputBuffers;  // Pointers to algorithm-owned shared buffers
    std::vector<int> inputReadPositions;                      // Per-consumer read positions
    LockFreeRingBuffer outputRingBuffer;
    std::atomic<int> samplesAvailable {0};
    std::atomic<bool> processingEnabled {false};

    // Metering ring buffer (read by OutputMeteringThread)
    LockFreeRingBuffer meteringRingBuffer;

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

    // Per-input delay smoothers (one direct + one FR per input channel).
    // Replaces the previous inert one-pole on delay targets.
    std::vector<DelayTargetSmoother> smootherDirect;
    std::vector<DelayTargetSmoother> smootherFR;

    // Monotonic sample counter, advanced once per block. Used as the time
    // axis for DelayTargetSmoother::observe() / smoothedAt().
    std::int64_t sampleCounter = 0;

    // FR diffusion (time-varying jitter per input)
    std::vector<float> frDiffusionState;   // Current jitter value per input
    std::vector<float> frDiffusionTarget;  // Target jitter value per input
    std::atomic<float>* frMaxJitterMs = nullptr;  // Max jitter per input
    int storedNumInputs = 0;  // Store for cleanup
    std::mt19937 frRandom;                 // Random generator for diffusion
    int frDiffusionUpdateCounter = 0;      // Counter for updating targets

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputBufferProcessor)
};
