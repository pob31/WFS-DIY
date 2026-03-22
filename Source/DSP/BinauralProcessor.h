#pragma once

#include <JuceHeader.h>
#include "BinauralCalculationEngine.h"
#include "SharedInputRingBuffer.h"
#include "WFSHighShelfFilter.h"
#include "../LockFreeRingBuffer.h"
#include <vector>
#include <atomic>

/**
 * BinauralProcessor
 *
 * Thread-based processor for binaural rendering.
 * Processes inputs to a stereo binaural output pair.
 *
 * Behavior:
 * - When no inputs are soloed: ALL inputs are processed
 * - When any input is soloed: only soloed inputs are processed
 *
 * For each processed input:
 * - Applies per-input delay using circular buffer (separate L/R)
 * - Applies HF shelf filter for air absorption (separate L/R)
 * - Applies level attenuation
 * - Sums to left/right outputs
 */
class BinauralProcessor : public juce::Thread
{
public:
    explicit BinauralProcessor (BinauralCalculationEngine& calcEngine)
        : juce::Thread ("BinauralProcessor"),
          binauralCalc (calcEngine)
    {
    }

    ~BinauralProcessor() override
    {
        stopThread (1000);
    }

    /**
     * Prepare the processor for playback.
     */
    void prepareToPlay (double newSampleRate, int maxBlockSize, int numInputs)
    {
        sampleRate = newSampleRate;
        numInputChannels = numInputs;
        currentBlockSize = maxBlockSize;

        // Maximum delay = 1 second
        delayBufferLength = (int) (sampleRate * 1.0);

        // Create per-input delay buffers for left and right
        delayBuffersL.clear();
        delayBuffersR.clear();
        writePositionsL.clear();
        writePositionsR.clear();
        hfFiltersL.clear();
        hfFiltersR.clear();
        prevParamsL.clear();
        prevParamsR.clear();
        inputBuffers.clear();

        for (int i = 0; i < numInputs; ++i)
        {
            // Delay buffers
            delayBuffersL.push_back (juce::AudioBuffer<float> (1, delayBufferLength));
            delayBuffersL.back().clear();
            delayBuffersR.push_back (juce::AudioBuffer<float> (1, delayBufferLength));
            delayBuffersR.back().clear();
            writePositionsL.push_back (0);
            writePositionsR.push_back (0);

            // HF filters
            WFSHighShelfFilter filterL, filterR;
            filterL.prepare (sampleRate);
            filterR.prepare (sampleRate);
            hfFiltersL.push_back (filterL);
            hfFiltersR.push_back (filterR);

            // Smoothed parameter state (snap on first block)
            prevParamsL.push_back (SmoothedParams());
            prevParamsR.push_back (SmoothedParams());

            // Input ring buffers (4x block size for safety margin)
            inputBuffers.push_back (std::make_unique<LockFreeRingBuffer>());
            inputBuffers.back()->setSize (maxBlockSize * 4);
        }

        // Output ring buffers
        outputBufferL = std::make_unique<LockFreeRingBuffer>();
        outputBufferR = std::make_unique<LockFreeRingBuffer>();
        outputBufferL->setSize (maxBlockSize * 4);
        outputBufferR->setSize (maxBlockSize * 4);

        // Working buffers
        inputBlock.setSize (1, maxBlockSize);
        outputBlockL.setSize (1, maxBlockSize);
        outputBlockR.setSize (1, maxBlockSize);
    }

    /**
     * Release resources when stopping.
     */
    void releaseResources()
    {
        stopThread (1000);
        delayBuffersL.clear();
        delayBuffersR.clear();
        hfFiltersL.clear();
        hfFiltersR.clear();
        inputBuffers.clear();
        outputBufferL.reset();
        outputBufferR.reset();
    }

    /**
     * Push input samples from audio callback (producer).
     * Call this for each input channel.
     */
    void pushInput (int inputIndex, const float* data, int numSamples)
    {
        if (inputIndex >= 0 && inputIndex < (int) inputBuffers.size())
            inputBuffers[inputIndex]->write (data, numSamples);
        notify();  // Wake binaural worker thread immediately (immune to timer coalescing)
    }

    /**
     * Pull output samples from audio callback (consumer).
     * Retrieves processed binaural stereo output.
     */
    void pullOutput (float* leftOutput, float* rightOutput, int numSamples)
    {
        int samplesReadL = outputBufferL->read (leftOutput, numSamples);
        int samplesReadR = outputBufferR->read (rightOutput, numSamples);

        // If not enough samples available, zero-pad the rest
        if (samplesReadL < numSamples)
            juce::FloatVectorOperations::clear (leftOutput + samplesReadL, numSamples - samplesReadL);
        if (samplesReadR < numSamples)
            juce::FloatVectorOperations::clear (rightOutput + samplesReadR, numSamples - samplesReadR);
    }

    /**
     * Enable or disable processing.
     */
    void setEnabled (bool enabled)
    {
        processingEnabled.store (enabled, std::memory_order_release);
    }

    /**
     * Check if processing is enabled.
     */
    bool isEnabled() const
    {
        return processingEnabled.load (std::memory_order_acquire);
    }

    /**
     * Start the processing thread.
     */
    void startProcessing()
    {
        if (!isThreadRunning())
            startThread (juce::Thread::Priority::high);
    }

    /**
     * Stop the processing thread.
     */
    void stopProcessing()
    {
        stopThread (1000);
    }

    /** Set shared input buffers (reads from these instead of private ring buffers). */
    void setSharedInputBuffers(const std::vector<std::unique_ptr<SharedInputRingBuffer>>& buffers)
    {
        sharedInputs.clear();
        for (auto& buf : buffers)
            sharedInputs.push_back(buf.get());
        sharedReadPositions.assign(sharedInputs.size(), 0);
        useSharedInputs = true;
    }

    /** Clear shared input buffer references (fall back to pushInput mode). */
    void clearSharedInputBuffers()
    {
        sharedInputs.clear();
        sharedReadPositions.clear();
        useSharedInputs = false;
    }

    /** Notify that new input data is available in shared buffers. */
    void notifyInputAvailable()
    {
        notify();
    }

    /**
     * Reset all delay buffers and filters.
     */
    void reset()
    {
        for (auto& buf : delayBuffersL)
            buf.clear();
        for (auto& buf : delayBuffersR)
            buf.clear();
        for (auto& pos : writePositionsL)
            pos = 0;
        for (auto& pos : writePositionsR)
            pos = 0;
        for (auto& filter : hfFiltersL)
            filter.reset();
        for (auto& filter : hfFiltersR)
            filter.reset();
        for (auto& p : prevParamsL)
            p.initialized = false;
        for (auto& p : prevParamsR)
            p.initialized = false;
        for (auto& buf : inputBuffers)
            buf->reset();
        if (outputBufferL) outputBufferL->reset();
        if (outputBufferR) outputBufferR->reset();
    }

    /**
     * Update for changed input channel count.
     */
    void setNumInputChannels (int numInputs)
    {
        if (numInputs != numInputChannels && sampleRate > 0)
        {
            bool wasRunning = isThreadRunning();
            if (wasRunning) stopThread (1000);
            prepareToPlay (sampleRate, currentBlockSize, numInputs);
            if (wasRunning) startThread (juce::Thread::Priority::high);
        }
    }

private:
    // Per-input smoothed parameter state for interpolation between blocks
    struct SmoothedParams
    {
        float delayMs = 0.0f;
        float level = 0.0f;
        float hfDb = 0.0f;
        bool initialized = false;
    };

    /**
     * Worker thread main loop.
     */
    void run() override
    {
        while (!threadShouldExit())
        {
            if (processingEnabled.load (std::memory_order_acquire))
            {
                // Check if we have enough input data to process a block
                bool hasData = true;

                if (useSharedInputs)
                {
                    for (int i = 0; i < numInputChannels && i < (int)sharedInputs.size() && hasData; ++i)
                    {
                        if (sharedInputs[i]->getAvailableAt(sharedReadPositions[i]) < currentBlockSize)
                            hasData = false;
                    }
                }
                else
                {
                    for (int i = 0; i < numInputChannels && hasData; ++i)
                    {
                        if (inputBuffers[i]->getAvailableData() < currentBlockSize)
                            hasData = false;
                    }
                }

                if (hasData)
                {
                    processBlock();
                }
                else
                {
                    // Wait a short time for more data
                    wait (1);
                }
            }
            else
            {
                // Not enabled, wait longer
                wait (10);
            }
        }
    }

    /**
     * Process one block of audio.
     */
    void processBlock()
    {
        int numSamples = currentBlockSize;

        // Clear output accumulators
        outputBlockL.clear();
        outputBlockR.clear();

        float* outL = outputBlockL.getWritePointer (0);
        float* outR = outputBlockR.getWritePointer (0);

        // Check if any inputs are soloed
        bool anySoloed = binauralCalc.getNumSoloedInputs() > 0;

        // Process each input
        for (int inputIdx = 0; inputIdx < numInputChannels; ++inputIdx)
        {
            // Skip if soloed mode and this input isn't soloed
            if (anySoloed && !binauralCalc.isInputSoloed (inputIdx))
            {
                // Still need to consume input data to keep buffers in sync
                if (useSharedInputs && inputIdx < (int)sharedInputs.size())
                    sharedInputs[inputIdx]->readWithPosition(sharedReadPositions[inputIdx], inputBlock.getWritePointer(0), numSamples);
                else
                    inputBuffers[inputIdx]->read (inputBlock.getWritePointer (0), numSamples);
                continue;
            }

            // Read input from shared buffers or private ring buffers
            int samplesRead;
            if (useSharedInputs && inputIdx < (int)sharedInputs.size())
                samplesRead = sharedInputs[inputIdx]->readWithPosition(sharedReadPositions[inputIdx], inputBlock.getWritePointer(0), numSamples);
            else
                samplesRead = inputBuffers[inputIdx]->read (inputBlock.getWritePointer (0), numSamples);
            if (samplesRead == 0)
                continue;

            // Get binaural parameters for this input
            auto binauralPair = binauralCalc.calculate (inputIdx);

            const float* inputData = inputBlock.getReadPointer (0);

            // Process left channel
            processInputToChannel (inputIdx, inputData, samplesRead,
                                   binauralPair.left,
                                   delayBuffersL[inputIdx],
                                   writePositionsL[inputIdx],
                                   hfFiltersL[inputIdx],
                                   outL,
                                   prevParamsL[inputIdx]);

            // Process right channel
            processInputToChannel (inputIdx, inputData, samplesRead,
                                   binauralPair.right,
                                   delayBuffersR[inputIdx],
                                   writePositionsR[inputIdx],
                                   hfFiltersR[inputIdx],
                                   outR,
                                   prevParamsR[inputIdx]);
        }

        // Write to output ring buffers
        outputBufferL->write (outL, numSamples);
        outputBufferR->write (outR, numSamples);
    }

    /**
     * Process one input to one output channel (left or right).
     * Uses fractional delay with linear interpolation and per-sample
     * parameter interpolation to avoid graininess on fast position changes.
     */
    void processInputToChannel (int inputIdx,
                                const float* inputData,
                                int numSamples,
                                const BinauralCalculationEngine::BinauralOutput& params,
                                juce::AudioBuffer<float>& delayBuffer,
                                int& writePos,
                                WFSHighShelfFilter& hfFilter,
                                float* output,
                                SmoothedParams& prevParams)
    {
        juce::ignoreUnused (inputIdx);
        float* delayData = delayBuffer.getWritePointer (0);

        // First block after init: snap to current values (no interpolation from zero)
        if (!prevParams.initialized)
        {
            prevParams.delayMs = params.delayMs;
            prevParams.level = params.level;
            prevParams.hfDb = params.hfAttenuationDb;
            prevParams.initialized = true;
        }

        // Per-sample interpolation endpoints
        float startDelayMs = prevParams.delayMs;
        float startLevel = prevParams.level;
        float endDelayMs = params.delayMs;
        float endLevel = params.level;
        float invNumSamples = 1.0f / (float) numSamples;

        // Update prev for next block
        prevParams.delayMs = params.delayMs;
        prevParams.level = params.level;
        prevParams.hfDb = params.hfAttenuationDb;

        // Set HF filter gain (filter state provides inherent smoothing)
        hfFilter.setGainDb (params.hfAttenuationDb);

        float msToSamples = (float) (sampleRate / 1000.0);
        float maxDelay = (float) (delayBufferLength - 2);

        // Process each sample with interpolated parameters
        for (int i = 0; i < numSamples; ++i)
        {
            // Write input to delay buffer
            delayData[writePos] = inputData[i];

            // Interpolate delay and level across the block
            float t = (float) i * invNumSamples;
            float currentDelayMs = startDelayMs + (endDelayMs - startDelayMs) * t;
            float currentLevel = startLevel + (endLevel - startLevel) * t;

            // Fractional delay in samples
            float delaySamples = currentDelayMs * msToSamples;
            if (delaySamples < 0.0f) delaySamples = 0.0f;
            if (delaySamples > maxDelay) delaySamples = maxDelay;

            // Fractional read position with linear interpolation
            float exactReadPos = (float) writePos - delaySamples;
            if (exactReadPos < 0.0f)
                exactReadPos += (float) delayBufferLength;

            int readPos1 = (int) exactReadPos;
            if (readPos1 >= delayBufferLength) readPos1 -= delayBufferLength;
            int readPos2 = (readPos1 + 1) % delayBufferLength;
            float fraction = exactReadPos - std::floor (exactReadPos);

            float delayedSample = delayData[readPos1] + fraction * (delayData[readPos2] - delayData[readPos1]);

            // Apply HF filter
            float filteredSample = hfFilter.processSample (delayedSample);

            // Apply interpolated level and sum to output
            output[i] += filteredSample * currentLevel;

            // Advance write position
            writePos = (writePos + 1) % delayBufferLength;
        }
    }

    BinauralCalculationEngine& binauralCalc;

    double sampleRate = 48000.0;
    int numInputChannels = 0;
    int currentBlockSize = 512;
    int delayBufferLength = 0;

    std::atomic<bool> processingEnabled {false};

    // Shared input buffers (read from these when available, bypasses pushInput)
    std::vector<SharedInputRingBuffer*> sharedInputs;
    std::vector<int> sharedReadPositions;
    bool useSharedInputs = false;

    // Lock-free ring buffers for input (fallback when shared buffers aren't set)
    std::vector<std::unique_ptr<LockFreeRingBuffer>> inputBuffers;

    // Lock-free ring buffers for output (L/R stereo)
    std::unique_ptr<LockFreeRingBuffer> outputBufferL;
    std::unique_ptr<LockFreeRingBuffer> outputBufferR;

    // Per-input delay buffers (separate for left and right)
    std::vector<juce::AudioBuffer<float>> delayBuffersL;
    std::vector<juce::AudioBuffer<float>> delayBuffersR;
    std::vector<int> writePositionsL;
    std::vector<int> writePositionsR;

    // Per-input HF shelf filters
    std::vector<WFSHighShelfFilter> hfFiltersL;
    std::vector<WFSHighShelfFilter> hfFiltersR;

    std::vector<SmoothedParams> prevParamsL;
    std::vector<SmoothedParams> prevParamsR;

    // Working buffers
    juce::AudioBuffer<float> inputBlock;
    juce::AudioBuffer<float> outputBlockL;
    juce::AudioBuffer<float> outputBlockR;
};
