#pragma once

#include <JuceHeader.h>
#include "BinauralCalculationEngine.h"
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
                int minAvailable = currentBlockSize;
                bool hasData = true;

                for (int i = 0; i < numInputChannels && hasData; ++i)
                {
                    if (inputBuffers[i]->getAvailableData() < minAvailable)
                        hasData = false;
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
                inputBuffers[inputIdx]->read (inputBlock.getWritePointer (0), numSamples);
                continue;
            }

            // Read input from ring buffer
            int samplesRead = inputBuffers[inputIdx]->read (inputBlock.getWritePointer (0), numSamples);
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
                                   outL);

            // Process right channel
            processInputToChannel (inputIdx, inputData, samplesRead,
                                   binauralPair.right,
                                   delayBuffersR[inputIdx],
                                   writePositionsR[inputIdx],
                                   hfFiltersR[inputIdx],
                                   outR);
        }

        // Write to output ring buffers
        outputBufferL->write (outL, numSamples);
        outputBufferR->write (outR, numSamples);
    }

    /**
     * Process one input to one output channel (left or right).
     */
    void processInputToChannel (int inputIdx,
                                const float* inputData,
                                int numSamples,
                                const BinauralCalculationEngine::BinauralOutput& params,
                                juce::AudioBuffer<float>& delayBuffer,
                                int& writePos,
                                WFSHighShelfFilter& hfFilter,
                                float* output)
    {
        juce::ignoreUnused (inputIdx);
        float* delayData = delayBuffer.getWritePointer (0);

        // Calculate delay in samples
        int delaySamples = (int) (params.delayMs * sampleRate / 1000.0);
        delaySamples = juce::jlimit (0, delayBufferLength - 1, delaySamples);

        // Set HF filter gain
        hfFilter.setGainDb (params.hfAttenuationDb);

        // Process each sample
        for (int i = 0; i < numSamples; ++i)
        {
            // Write input to delay buffer
            delayData[writePos] = inputData[i];

            // Read from delay buffer
            int readPos = writePos - delaySamples;
            if (readPos < 0)
                readPos += delayBufferLength;

            float delayedSample = delayData[readPos];

            // Apply HF filter
            float filteredSample = hfFilter.processSample (delayedSample);

            // Apply level and sum to output
            output[i] += filteredSample * params.level;

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

    // Lock-free ring buffers for input (one per input channel)
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

    // Working buffers
    juce::AudioBuffer<float> inputBlock;
    juce::AudioBuffer<float> outputBlockL;
    juce::AudioBuffer<float> outputBlockR;
};
