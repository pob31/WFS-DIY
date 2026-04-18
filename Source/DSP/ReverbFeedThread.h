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
        // Publish the triplet under a brief SpinLock so the worker thread
        // can never observe a torn (pointer, stride, count) update. Matches
        // the pattern used for pendingPreParams / pendingPostParams in
        // ReverbEngine. The worker snapshots once per batch in run(); the
        // hot inner loop never touches this lock.
        juce::SpinLock::ScopedLockType lock (matrixLock);
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

            // Snapshot the (levels, stride, numRevs) triplet once per batch under
            // the matrix lock. The rest of this batch uses only the locals, so
            // the inner per-sample / per-input / per-node loop runs with zero
            // synchronisation. A matrix update via updateReverbLevels() becomes
            // visible on the *next* batch — one block of stale levels is
            // inaudible for reverb send routing (matrix updates happen at
            // user-interaction cadence, not per block).
            const float* levelsSnap;
            int strideSnap;
            int numRevsSnap;
            {
                juce::SpinLock::ScopedLockType lock (matrixLock);
                levelsSnap  = reverbLevels;
                strideSnap  = reverbStride;
                numRevsSnap = numRevs;
            }

            if (reverbEngine == nullptr || numRevsSnap <= 0 || numInputs <= 0)
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
                for (int revIdx = 0; revIdx < numRevsSnap; ++revIdx)
                {
                    downsampleBuffer.clear(revIdx, 0, pushSamples);
                    reverbEngine->pushNodeInput(revIdx, downsampleBuffer.getReadPointer(revIdx), pushSamples);
                }
                continue;
            }

            // Compute reverb feeds: for each node, sum weighted input contributions
            feedBuffer.clear();

            for (int revIdx = 0; revIdx < numRevsSnap; ++revIdx)
            {
                float* feedData = feedBuffer.getWritePointer(revIdx);

                for (int inIdx = 0; inIdx < numInputs; ++inIdx)
                {
                    float feedLevel = levelsSnap[inIdx * strideSnap + revIdx];

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

    // (reverbLevels, reverbStride, numRevs) form a triplet published by
    // updateReverbLevels() from the message thread and consumed by run() on
    // the worker thread. matrixLock serialises publication so the worker
    // never observes a torn triplet. See updateReverbLevels() and run() for
    // the read-snapshot pattern.
    const float* reverbLevels = nullptr;
    int reverbStride = 0;
    int numRevs = 0;
    juce::SpinLock matrixLock;

    int numInputs = 0;
    int processingBlockSize = 256;
    int reverbSRRatio = 1;
    std::atomic<bool> dataReady{false};
    std::atomic<bool> isMuted{false};

    juce::AudioBuffer<float> inputBlocks;      // numInputs channels, one block each
    juce::AudioBuffer<float> feedBuffer;       // numReverbs channels, feed sums
    juce::AudioBuffer<float> downsampleBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbFeedThread)
};
