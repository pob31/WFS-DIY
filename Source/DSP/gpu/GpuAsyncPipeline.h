#pragma once
#if WFS_GPU_NATIVE

/*
    GpuAsyncPipeline — deadline isolation for GPU audio processing.
    Native edition: backend-agnostic port of the design proven on the
    GPU Audio SDK branch (same architecture, simpler internals — the pump
    drives a synchronous backend call; no executor machinery).

        audio thread:  pushInput(block) ──► in-rings        out-rings ──► popOutput(block)
                                               │                ▲
        pump thread:                 read block┘ backend->     │ write block
                                          └────  processBlock ─┘  (sync GPU launch)

    Pipeline depth D blocks = constant added latency L = D x blockSize/sr,
    which the backend PRE-SUBTRACTS from the WFS delay matrix (clamped >= 0),
    so arrival times are unchanged for all delays >= L. A GPU stall shorter
    than the cushion is absorbed invisibly; a longer one silence-fills the
    GPU outputs (counted, never blocking the audio thread).

    Field data behind the design (measured on this machine, 2026-06-11):
    desktop-compositor transients stall GPU dispatch by 3-5 ms regardless of
    API; the native Metal launch floor is 0.13-0.17 ms, so the same cushion
    is ~7x more robust here than it was on the SDK.
*/

#include <JuceHeader.h>
#include <atomic>
#include <memory>
#include <vector>

#include "../../LockFreeRingBuffer.h"
#include "MetalWfsBackend.h"

class GpuAsyncPipeline : private juce::Thread
{
public:
    static constexpr int kMinDepthBlocks = 1;
    static constexpr int kMaxDepthBlocks = 8;

    GpuAsyncPipeline() : juce::Thread ("Native GPU Pipeline Pump") {}
    ~GpuAsyncPipeline() override { release(); }

    /** Takes (non-owning) the prepared backend and starts the pump.
        The backend must already be prepare()d for the same geometry. */
    bool prepare (MetalWfsBackend* backendToUse,
                  int numInputChannels,
                  int numOutputChannels,
                  int blockSizeToUse,
                  double sampleRateToUse,
                  int depthBlocksRequested)
    {
        release();

        backend = backendToUse;
        if (backend == nullptr || ! backend->isReady())
        {
            lastError = "Backend not ready";
            return false;
        }

        numIns = juce::jmax (1, numInputChannels);
        numOuts = juce::jmax (1, numOutputChannels);
        blockSize = juce::jmax (1, blockSizeToUse);
        sampleRate = sampleRateToUse;
        depthBlocks = juce::jlimit (kMinDepthBlocks, kMaxDepthBlocks, depthBlocksRequested);

        const int ringCapacity = blockSize * (depthBlocks + 8);
        ringsIn.clear();
        ringsOut.clear();
        for (int ch = 0; ch < numIns; ++ch)
        {
            ringsIn.push_back (std::make_unique<LockFreeRingBuffer>());
            ringsIn.back()->setSize (ringCapacity);
        }
        for (int ch = 0; ch < numOuts; ++ch)
        {
            ringsOut.push_back (std::make_unique<LockFreeRingBuffer>());
            ringsOut.back()->setSize (ringCapacity);
        }

        pumpIn.setSize (numIns, blockSize);
        pumpIn.clear();
        pumpOut.setSize (numOuts, blockSize);
        pumpOut.clear();
        pumpInPtrs.resize ((size_t) numIns);
        pumpOutPtrs.resize ((size_t) numOuts);
        for (int ch = 0; ch < numIns; ++ch)
            pumpInPtrs[(size_t) ch] = pumpIn.getReadPointer (ch);
        for (int ch = 0; ch < numOuts; ++ch)
            pumpOutPtrs[(size_t) ch] = pumpOut.getWritePointer (ch);

        zeroBlock.assign ((size_t) blockSize, 0.0f);
        discardScratch.assign ((size_t) blockSize * 4, 0.0f);

        // Prime the cushion: D blocks of silence ahead of the consumer.
        for (int d = 0; d < depthBlocks; ++d)
            for (auto& ring : ringsOut)
                ring->write (zeroBlock.data(), blockSize);

        underrunCount.store (0, std::memory_order_relaxed);
        peakPumpMs.store (0.0f, std::memory_order_relaxed);
        lastPumpMs.store (0.0f, std::memory_order_relaxed);
        pumpFailed.store (false, std::memory_order_relaxed);
        lastError.clear();

        startRealtimeThread (juce::Thread::RealtimeOptions {}
                                 .withApproximateAudioProcessingTime (blockSize, sampleRate));

        readyFlag.store (true, std::memory_order_release);
        return true;
    }

    void release()
    {
        readyFlag.store (false, std::memory_order_release);
        if (isThreadRunning())
        {
            signalThreadShouldExit();
            notify();
            stopThread (4000);
        }
        ringsIn.clear();
        ringsOut.clear();
        backend = nullptr;
    }

    /** Audio thread: queue one block of input (silence for missing channels).
        Never blocks. */
    void pushInput (const juce::AudioBuffer<float>& source, int numAvailable, int startSample, int numSamples)
    {
        if (! readyFlag.load (std::memory_order_acquire))
            return;

        for (int ch = 0; ch < numIns; ++ch)
        {
            if (ch < numAvailable && ch < source.getNumChannels())
                ringsIn[(size_t) ch]->write (source.getReadPointer (ch, startSample), numSamples);
            else
                writeSilence (*ringsIn[(size_t) ch], numSamples);
        }
        notify();
    }

    /** Audio thread: pop one processed block (silence-fill + count on
        underrun). Never blocks. */
    bool popOutput (juce::AudioBuffer<float>& destination, int numAvailable, int startSample, int numSamples)
    {
        const int writable = juce::jmin (numAvailable, numOuts, destination.getNumChannels());

        if (! readyFlag.load (std::memory_order_acquire)
            || ringsOut.empty()
            || ringsOut.front()->getAvailableData() < numSamples)
        {
            for (int ch = 0; ch < writable; ++ch)
                destination.clear (ch, startSample, numSamples);
            if (readyFlag.load (std::memory_order_acquire))
                underrunCount.fetch_add (1, std::memory_order_relaxed);
            return false;
        }

        for (int ch = 0; ch < numOuts; ++ch)
        {
            if (ch < writable)
            {
                ringsOut[(size_t) ch]->read (destination.getWritePointer (ch, startSample), numSamples);
            }
            else
            {
                int remaining = numSamples;
                while (remaining > 0)
                {
                    const int chunk = juce::jmin (remaining, (int) discardScratch.size());
                    ringsOut[(size_t) ch]->read (discardScratch.data(), chunk);
                    remaining -= chunk;
                }
            }
        }
        return true;
    }

    bool isReady() const noexcept              { return readyFlag.load (std::memory_order_acquire); }
    bool hasPumpFailed() const noexcept        { return pumpFailed.load (std::memory_order_relaxed); }
    juce::String getLastError() const          { return lastError; }
    int getDepthBlocks() const noexcept        { return depthBlocks; }
    double getLatencyMs() const noexcept       { return sampleRate > 0.0 ? depthBlocks * 1000.0 * blockSize / sampleRate : 0.0; }
    uint32_t getUnderrunCount() const noexcept { return underrunCount.load (std::memory_order_relaxed); }
    float getLastPumpMs() const noexcept       { return lastPumpMs.load (std::memory_order_relaxed); }
    float getAndResetPeakPumpMs() noexcept     { return peakPumpMs.exchange (0.0f, std::memory_order_relaxed); }

private:
    void run() override
    {
        while (! threadShouldExit())
        {
            if (ringsIn.empty() || ringsIn.front()->getAvailableData() < blockSize)
            {
                wait (50);
                continue;
            }

            for (int ch = 0; ch < numIns; ++ch)
                ringsIn[(size_t) ch]->read (pumpIn.getWritePointer (ch), blockSize);

            const bool ok = backend != nullptr
                            && backend->processBlock (pumpInPtrs.data(), pumpOutPtrs.data());

            if (! ok)
            {
                DBG ("Native GPU pipeline: backend launch failed - pump idling. "
                     + juce::String (backend != nullptr ? backend->getLastError() : "no backend"));
                pumpFailed.store (true, std::memory_order_relaxed);
                readyFlag.store (false, std::memory_order_release);
                return;
            }

            const float ms = (float) backend->getLastLaunchMs();
            lastPumpMs.store (ms, std::memory_order_relaxed);
            if (ms > peakPumpMs.load (std::memory_order_relaxed))
                peakPumpMs.store (ms, std::memory_order_relaxed);

            for (int ch = 0; ch < numOuts; ++ch)
                ringsOut[(size_t) ch]->write (pumpOut.getReadPointer (ch), blockSize);
        }
    }

    void writeSilence (LockFreeRingBuffer& ring, int numSamples)
    {
        int remaining = numSamples;
        while (remaining > 0)
        {
            const int chunk = juce::jmin (remaining, (int) zeroBlock.size());
            ring.write (zeroBlock.data(), chunk);
            remaining -= chunk;
        }
    }

    MetalWfsBackend* backend { nullptr }; // non-owning
    std::vector<std::unique_ptr<LockFreeRingBuffer>> ringsIn, ringsOut;
    juce::AudioBuffer<float> pumpIn, pumpOut;
    std::vector<const float*> pumpInPtrs;
    std::vector<float*> pumpOutPtrs;
    std::vector<float> zeroBlock, discardScratch;

    int numIns { 0 }, numOuts { 0 }, blockSize { 0 }, depthBlocks { 4 };
    double sampleRate { 0.0 };
    juce::String lastError;

    std::atomic<bool> readyFlag { false };
    std::atomic<bool> pumpFailed { false };
    std::atomic<uint32_t> underrunCount { 0 };
    std::atomic<float> lastPumpMs { 0.0f };
    std::atomic<float> peakPumpMs { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GpuAsyncPipeline)
};

#endif // WFS_GPU_NATIVE
