#pragma once

#include <JuceHeader.h>
#include "../../spatcore/rt/LockFreeRingBuffer.h"
#include "../../spatcore/dsp/WFSHighShelfFilter.h"
#include "../../spatcore/dsp/WFSBiquadFilter.h"
#include "LiveSourceLevelDetector.h"
#include "../../spatcore/dsp/DelayTargetSmoother.h"
#include "../../spatcore/rt/AudioWorkgroupCoordinator.h"
#include "../../spatcore/dsp/FrDiffusionModel.h"
#include <atomic>
#include <cstdint>
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

        // Per-output delay smoothers (direct + FR). Window is set in prepare()
        // once we know the sample rate.
        smootherDirect.resize(static_cast<size_t>(numOutputs));
        smootherFR.resize(static_cast<size_t>(numOutputs));

        // Diffusion jitter state (one per output): a one-pole low-passed white
        // noise added to the FR delay POST-smoother. frJitterKey is the per-output
        // noise stream key (set in prepare()); frDiffusionState holds the LP value.
        frDiffusionState.resize(static_cast<size_t>(numOutputs), 0.0f);
        frJitterKey.resize(static_cast<size_t>(numOutputs), 0u);
        frJitterPrev.resize(static_cast<size_t>(numOutputs), 0.0f);
        frJitterCurr.resize(static_cast<size_t>(numOutputs), 0.0f);
        frLastLevel.resize(static_cast<size_t>(numOutputs), 0.0f);
    }

    ~InputBufferProcessor() override
    {
        stopThread(1000);
    }

    /** Optional: realtime workgroup to (re)join from the worker thread (macOS). */
    void setWorkgroupCoordinator (AudioWorkgroupCoordinator* c) { workgroupCoordinator = c; }
    AudioWorkgroupCoordinator* workgroupCoordinator = nullptr;

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

        // Configure the per-output delay smoothers for a ~10 ms box window at
        // the current sample rate.
        const int windowSamples = juce::jmax(2, static_cast<int>(sampleRate * 0.010));
        for (auto& s : smootherDirect) s.prepare(windowSamples);
        for (auto& s : smootherFR)     s.prepare(windowSamples);
        sampleCounter = 0;

        // Per-output noise stream keys: distinct per (input, output) so every
        // speaker's grain is an independent stream (see FrDiffusionModel.h).
        for (size_t o = 0; o < frJitterKey.size(); ++o)
            frJitterKey[o] = FrDiffusion::makeKey (inputChannelIndex, static_cast<int> (o));

        // Initialize Live Source level detector
        lsDetector = std::make_unique<LiveSourceLevelDetector>();
        lsDetector->prepare(sampleRate);
    }

    // Called by audio thread to push input data
    void pushInput(const float* data, int numSamples)
    {
        inputRingBuffer.write(data, numSamples);
        samplesAvailable.store(inputRingBuffer.getAvailableData(), std::memory_order_release);
        notify();  // Wake worker thread immediately (immune to timer coalescing)
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
        std::fill(frJitterPrev.begin(), frJitterPrev.end(), 0.0f);
        std::fill(frJitterCurr.begin(), frJitterCurr.end(), 0.0f);
        std::fill(frLastLevel.begin(), frLastLevel.end(), 0.0f);
    }

    void setProcessingEnabled(bool enabled)
    {
        processingEnabled.store(enabled, std::memory_order_release);
        if (enabled)
            notify(); // Wake thread from infinite sleep
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

    // Get peak level in dB (100ms release for metering)
    float getPeakLevelDb() const
    {
        return lsDetector ? lsDetector->getPeakLevelDb() : -200.0f;
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

    /** Set FR diffusion amount (0-100%).
        Floor-roughness scattering, three perceptual zones - see
        FrDiffusionModel.h for the shared mapping; here we just publish
        the fraction. */
    void setFRDiffusion(float diffusionPercent)
    {
        frDiffusionAmount.store(juce::jlimit(0.0f, 1.0f, diffusionPercent * 0.01f),
                                std::memory_order_release);
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

        // Audio workgroup membership: token lives on (and is destroyed on) this thread.
        juce::WorkgroupToken wgToken;
        uint32_t wgSeenGeneration = 0;

        while (!threadShouldExit())
        {
            // Sleep when processing is disabled (no work to do)
            if (!processingEnabled.load(std::memory_order_acquire))
            {
                wait(-1); // Infinite sleep until notify() from setProcessingEnabled
                continue;
            }

            // (Re)join the audio workgroup if it changed (no-op off macOS / when unset).
            if (workgroupCoordinator != nullptr)
                workgroupCoordinator->joinIfChanged(wgToken, wgSeenGeneration);

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

            // Process block (processingEnabled already checked at top of loop)
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

        // Floor-Reflection diffusion: shared zone-mapped grain model (see
        // FrDiffusionModel.h). The noise is stepped ONCE PER BLOCK and ramped
        // linearly across the block (same prev->curr semantics as the GPU
        // launches) - per-sample stepping put a white micro-walk on the delay,
        // which the OutputBuffer's scatter-write turned into audible hiss.
        // Applied POST-smoother in the FR tap below.
        frJitterCoeffs = FrDiffusion::computeCoeffs(
            frDiffusionAmount.load(std::memory_order_acquire),
            static_cast<float>(currentSampleRate), static_cast<float>(numSamples));
        ++frJitterBlockIndex;
        for (size_t o = 0; o < frDiffusionState.size(); ++o)
        {
            frJitterPrev[o] = frJitterCurr[o];
            frJitterCurr[o] = FrDiffusion::step(frDiffusionState[o], frJitterBlockIndex,
                                                frJitterKey[o], frJitterCoeffs);
        }
        const float frJitterInvLen = 1.0f / static_cast<float>(numSamples);

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

            // Get FR delay parameters. The diffusion jitter is NOT added here - it
            // is applied per-sample AFTER the box smoother (see the FR tap below),
            // so the smoother can't average the grain away.
            float frExtraDelayMs = (sharedFRDelayTimes != nullptr) ? sharedFRDelayTimes[routingIndex] : 0.0f;
            float totalFRDelayMs = directDelayMs + frExtraDelayMs;
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

            // Per-output delay smoothing. Feed the raw target into the smoother
            // once per block; the per-sample loop below queries smoothedAt() to
            // get the box-filtered value and teleport-envelope gain.
            auto outIdx = static_cast<size_t>(outChannel);

            // FR engage: when the tap rises from silence, snap the FR delay
            // smoother to the current target instead of letting it slide from
            // its stale value (= the direct delay while FR was off). The slide
            // made the tap start coherent with the direct sound - an abrupt
            // +6 dB onset that then decayed as the delay ramped away. With the
            // level fading in (MainComponent) and the delay snapping here, the
            // FR now ramps ON from silence at the correct delay.
            if (frLevel > FrDiffusion::kEngageEps && frLastLevel[outIdx] <= FrDiffusion::kEngageEps)
                smootherFR[outIdx].reset();
            frLastLevel[outIdx] = frLevel;

            smootherDirect[outIdx].observe (directDelaySamples, sampleCounter);
            smootherFR[outIdx].observe     (frDelaySamples,     sampleCounter);

            // Process each sample with per-sample delay evaluation
            for (int sample = 0; sample < numSamples; ++sample)
            {
                float outputSample = 0.0f;
                const std::int64_t currentSample = sampleCounter + sample;

                // ==========================================
                // Direct signal
                // ==========================================
                if (directLevel > 0.0f)
                {
                    // Smoothed delay + teleport gain envelope
                    auto smoothed = smootherDirect[outIdx].smoothedAt (currentSample);

                    // Calculate fractional read position for direct signal
                    float exactReadPos = (float)writePosition + (float)sample - smoothed.delay;
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

                    outputSample += filteredSample * directLevel * smoothed.gain;
                }

                // ==========================================
                // Floor Reflection signal
                // ==========================================
                if (frLevel > 0.0f)
                {
                    // Smoothed FR delay + teleport gain envelope (independent smoother)
                    auto smoothedFr = smootherFR[outIdx].smoothedAt (currentSample);

                    // Diffusion grain (shared model): block-stepped, ramped across
                    // the block (GPU prev->curr semantics), added POST-smoother.
                    const float frJitterT = static_cast<float> (sample + 1) * frJitterInvLen;
                    const float jitterSamples = frJitterPrev[outIdx]
                        + (frJitterCurr[outIdx] - frJitterPrev[outIdx]) * frJitterT;

                    float frReadDelay = smoothedFr.delay + jitterSamples;
                    if (frReadDelay < 0.0f)
                        frReadDelay = 0.0f;

                    // Calculate fractional read position for FR signal (from FR-filtered buffer)
                    float exactReadPos = (float)frWritePosition + (float)sample - frReadDelay;
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

                    outputSample += filteredSample * frLevel * smoothedFr.gain;
                }

                outputData[sample] = outputSample;
            }
        }

        // Advance the monotonic sample counter for the next block's smoother queries.
        sampleCounter += numSamples;

        // Advance write positions
        writePosition = (writePosition + numSamples) % delayBufferLength;
        frWritePosition = (frWritePosition + numSamples) % delayBufferLength;
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

    // Per-output delay smoothers (one direct + one FR per output channel).
    // Replaces the previous inert one-pole on delay targets.
    std::vector<DelayTargetSmoother> smootherDirect;
    std::vector<DelayTargetSmoother> smootherFR;

    // Monotonic sample counter, advanced once per block. Used as the time
    // axis for DelayTargetSmoother::observe() / smoothedAt().
    std::int64_t sampleCounter = 0;

    // FR diffusion grain: low-passed white noise per output, added POST-smoother.
    std::vector<float> frDiffusionState;      // one-pole LP value per output (the grain)
    std::vector<uint32_t> frJitterKey;        // per-output independent noise stream key
    std::vector<float> frLastLevel;           // previous block's FR level per output (engage detect)
    std::atomic<float> frDiffusionAmount {0.0f};  // Diffusion fraction 0..1 (set from timer thread)
    FrDiffusion::Coeffs frJitterCoeffs;       // per-block coefficients (shared zone-map model)
    std::vector<float> frJitterPrev, frJitterCurr; // block-boundary jitter values (ramped per sample)
    uint32_t frJitterBlockIndex = 0;          // noise step index (one per block)

    // Live Source level detector (for peak/slow compression)
    std::unique_ptr<LiveSourceLevelDetector> lsDetector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InputBufferProcessor)
};
