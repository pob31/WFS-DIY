#pragma once

#include <JuceHeader.h>
#include "BinauralCalculationEngine.h"
#include "WFSHighShelfFilter.h"
#include <vector>

/**
 * BinauralProcessor
 *
 * Processes soloed inputs to a stereo binaural output pair.
 * Runs synchronously in the audio callback (no threading).
 *
 * For each soloed input:
 * - Applies per-input delay using circular buffer
 * - Applies HF shelf filter for air absorption
 * - Applies level attenuation
 * - Sums to left/right outputs
 */
class BinauralProcessor
{
public:
    explicit BinauralProcessor (BinauralCalculationEngine& calcEngine)
        : binauralCalc (calcEngine)
    {
    }

    /**
     * Prepare the processor for playback.
     */
    void prepare (double newSampleRate, int maxBlockSize, int numInputs)
    {
        sampleRate = newSampleRate;
        numInputChannels = numInputs;

        // Maximum delay = 1 second
        int maxDelaySamples = (int) (sampleRate * 1.0);

        // Create per-input delay buffers for left and right
        delayBuffersL.clear();
        delayBuffersR.clear();
        writePositions.clear();
        hfFiltersL.clear();
        hfFiltersR.clear();

        for (int i = 0; i < numInputs; ++i)
        {
            delayBuffersL.push_back (juce::AudioBuffer<float> (1, maxDelaySamples));
            delayBuffersL.back().clear();
            delayBuffersR.push_back (juce::AudioBuffer<float> (1, maxDelaySamples));
            delayBuffersR.back().clear();
            writePositions.push_back (0);

            WFSHighShelfFilter filterL, filterR;
            filterL.prepare (sampleRate);
            filterR.prepare (sampleRate);
            hfFiltersL.push_back (filterL);
            hfFiltersR.push_back (filterR);
        }

        // Temporary buffer for processing
        tempBuffer.setSize (1, maxBlockSize);
    }

    /**
     * Process a block of audio.
     * Takes input buffer and writes binaural output to leftOutput and rightOutput.
     *
     * @param inputBuffer The input audio buffer (all channels)
     * @param leftOutput Destination for left binaural output
     * @param rightOutput Destination for right binaural output
     * @param numSamples Number of samples to process
     */
    void processBlock (const juce::AudioBuffer<float>& inputBuffer,
                       float* leftOutput,
                       float* rightOutput,
                       int numSamples)
    {
        // Clear outputs
        juce::FloatVectorOperations::clear (leftOutput, numSamples);
        juce::FloatVectorOperations::clear (rightOutput, numSamples);

        int numInputs = juce::jmin (numInputChannels, inputBuffer.getNumChannels(), (int) delayBuffersL.size());

        // Check if any inputs are soloed
        bool anySoloed = false;
        for (int i = 0; i < numInputs; ++i)
        {
            if (binauralCalc.isInputSoloed (i))
            {
                anySoloed = true;
                break;
            }
        }

        // Process each input
        // If solos exist, only process soloed inputs
        // If no solos, process all inputs (full spatial mix for binaural preview)
        for (int inputIdx = 0; inputIdx < numInputs; ++inputIdx)
        {
            if (anySoloed && !binauralCalc.isInputSoloed (inputIdx))
                continue;

            // Get binaural parameters for this input
            auto binauralPair = binauralCalc.calculate (inputIdx);

            // Get input data
            const float* inputData = inputBuffer.getReadPointer (inputIdx);

            // Process left channel
            processInputToOutput (inputIdx, inputData, numSamples,
                                  binauralPair.left,
                                  delayBuffersL[inputIdx],
                                  hfFiltersL[inputIdx],
                                  leftOutput,
                                  true);

            // Process right channel
            processInputToOutput (inputIdx, inputData, numSamples,
                                  binauralPair.right,
                                  delayBuffersR[inputIdx],
                                  hfFiltersR[inputIdx],
                                  rightOutput,
                                  false);
        }
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
        for (auto& pos : writePositions)
            pos = 0;
        for (auto& filter : hfFiltersL)
            filter.reset();
        for (auto& filter : hfFiltersR)
            filter.reset();
    }

    /**
     * Update for changed input channel count.
     */
    void setNumInputChannels (int numInputs)
    {
        if (numInputs != numInputChannels && sampleRate > 0)
            prepare (sampleRate, tempBuffer.getNumSamples(), numInputs);
    }

private:
    /**
     * Process one input to one output (left or right).
     */
    void processInputToOutput (int inputIdx,
                               const float* inputData,
                               int numSamples,
                               const BinauralCalculationEngine::BinauralOutput& params,
                               juce::AudioBuffer<float>& delayBuffer,
                               WFSHighShelfFilter& hfFilter,
                               float* output,
                               bool updateWritePos)
    {
        int bufferSize = delayBuffer.getNumSamples();
        float* delayData = delayBuffer.getWritePointer (0);
        int writePos = writePositions[inputIdx];

        // Calculate delay in samples
        int delaySamples = (int) (params.delayMs * sampleRate / 1000.0);
        delaySamples = juce::jlimit (0, bufferSize - 1, delaySamples);

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
                readPos += bufferSize;

            float delayedSample = delayData[readPos];

            // Apply HF filter
            float filteredSample = hfFilter.processSample (delayedSample);

            // Apply level and sum to output
            output[i] += filteredSample * params.level;

            // Advance write position
            writePos = (writePos + 1) % bufferSize;
        }

        // Save write position (only once per input, not per L/R)
        if (updateWritePos)
            writePositions[inputIdx] = writePos;
    }

    BinauralCalculationEngine& binauralCalc;

    double sampleRate = 48000.0;
    int numInputChannels = 0;

    // Per-input delay buffers (separate for left and right)
    std::vector<juce::AudioBuffer<float>> delayBuffersL;
    std::vector<juce::AudioBuffer<float>> delayBuffersR;
    std::vector<int> writePositions;

    // Per-input HF shelf filters
    std::vector<WFSHighShelfFilter> hfFiltersL;
    std::vector<WFSHighShelfFilter> hfFiltersR;

    // Temporary buffer
    juce::AudioBuffer<float> tempBuffer;
};
