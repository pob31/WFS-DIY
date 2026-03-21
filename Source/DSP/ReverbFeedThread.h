#pragma once

#include <JuceHeader.h>
#include "SharedInputRingBuffer.h"
#include "ReverbEngine.h"
#include <atomic>
#include <vector>

/**
    Dedicated thread for computing reverb feed sums from input audio.
    Reads from shared input ring buffers, computes weighted input->reverb sums,
    handles downsampling, and pushes to ReverbEngine node inputs.

    Runs one block behind the audio callback (2.67ms at 256/96kHz — imperceptible for reverb).
*/
class ReverbFeedThread : public juce::Thread
{
public:
    ReverbFeedThread() : juce::Thread("ReverbFeed") {}
    ~ReverbFeedThread() override { stopThread(2000); }

    void prepare(const std::vector<std::unique_ptr<SharedInputRingBuffer>>& sharedInputs,
                 ReverbEngine* engine,
                 const float* reverbLevelsPtr,
                 int calcReverbStride,
                 int numInputCh,
                 int numReverbNodes,
                 int blockSize,
                 int srRatio)
    {
        inputBuffers.clear();
        for (auto& buf : sharedInputs)
            inputBuffers.push_back(buf.get());

        reverbEngine = engine;
        reverbLevels = reverbLevelsPtr;
        reverbStride = calcReverbStride;
        numInputs = numInputCh;
        numRevs = numReverbNodes;
        processingBlockSize = blockSize;
        reverbSRRatio = srRatio;

        readPositions.assign(inputBuffers.size(), 0);

        // Temp buffer: one row per input channel for batch reading
        inputBlocks.setSize(numInputCh, blockSize);

        feedBuffer.setSize(numReverbNodes, blockSize);

        int dsBlockSize = (srRatio > 1) ? (blockSize / srRatio) : blockSize;
        downsampleBuffer.setSize(numReverbNodes, dsBlockSize);
    }

    void notifyInputAvailable()
    {
        dataReady.store(true, std::memory_order_release);
        notify();
    }

    void setMuted(bool muted)
    {
        isMuted.store(muted, std::memory_order_relaxed);
    }

    void updateReverbLevels(const float* newLevelsPtr, int newStride, int newNumReverbs)
    {
        reverbLevels = newLevelsPtr;
        reverbStride = newStride;
        numRevs = newNumReverbs;
    }

private:
    void run() override
    {
        while (!threadShouldExit())
        {
            if (!dataReady.load(std::memory_order_acquire))
            {
                wait(1);
                continue;
            }
            dataReady.store(false, std::memory_order_relaxed);

            if (reverbEngine == nullptr || numRevs <= 0 || numInputs <= 0)
                continue;

            int numSamples = processingBlockSize;

            // Check all channels have enough data
            int minAvail = std::numeric_limits<int>::max();
            for (int ch = 0; ch < numInputs && ch < (int)inputBuffers.size(); ++ch)
                minAvail = juce::jmin(minAvail, inputBuffers[ch]->getAvailableAt(readPositions[ch]));

            if (minAvail < numSamples)
                continue;

            // Read all input channels into temp buffers (one read per channel)
            for (int ch = 0; ch < numInputs && ch < (int)inputBuffers.size(); ++ch)
                inputBuffers[ch]->readWithPosition(readPositions[ch], inputBlocks.getWritePointer(ch), numSamples);

            int pushSamples = numSamples / reverbSRRatio;

            if (isMuted.load(std::memory_order_relaxed))
            {
                // Push silence — reverb tail decays naturally
                for (int revIdx = 0; revIdx < numRevs; ++revIdx)
                {
                    downsampleBuffer.clear(revIdx, 0, pushSamples);
                    reverbEngine->pushNodeInput(revIdx, downsampleBuffer.getReadPointer(revIdx), pushSamples);
                }
                continue;
            }

            // Compute reverb feeds: for each node, sum weighted input contributions
            feedBuffer.clear();

            for (int revIdx = 0; revIdx < numRevs; ++revIdx)
            {
                float* feedData = feedBuffer.getWritePointer(revIdx);

                for (int inIdx = 0; inIdx < numInputs; ++inIdx)
                {
                    float feedLevel = reverbLevels[inIdx * reverbStride + revIdx];

                    if (feedLevel > 0.0001f)
                    {
                        const float* inputData = inputBlocks.getReadPointer(inIdx);
                        juce::FloatVectorOperations::addWithMultiply(feedData, inputData, feedLevel, numSamples);
                    }
                }

                // Downsample and push
                if (reverbSRRatio > 1)
                {
                    float* dsData = downsampleBuffer.getWritePointer(revIdx);
                    float invRatio = 1.0f / static_cast<float>(reverbSRRatio);
                    for (int i = 0; i < pushSamples; ++i)
                    {
                        float sum = 0.0f;
                        for (int j = 0; j < reverbSRRatio; ++j)
                            sum += feedData[i * reverbSRRatio + j];
                        dsData[i] = sum * invRatio;
                    }
                    reverbEngine->pushNodeInput(revIdx, dsData, pushSamples);
                }
                else
                {
                    reverbEngine->pushNodeInput(revIdx, feedData, numSamples);
                }
            }
        }
    }

    std::vector<SharedInputRingBuffer*> inputBuffers;
    std::vector<int> readPositions;
    ReverbEngine* reverbEngine = nullptr;
    const float* reverbLevels = nullptr;
    int reverbStride = 0;
    int numInputs = 0;
    int numRevs = 0;
    int processingBlockSize = 256;
    int reverbSRRatio = 1;
    std::atomic<bool> dataReady{false};
    std::atomic<bool> isMuted{false};

    juce::AudioBuffer<float> inputBlocks;      // numInputs channels, one block each
    juce::AudioBuffer<float> feedBuffer;       // numReverbs channels, feed sums
    juce::AudioBuffer<float> downsampleBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbFeedThread)
};
