#pragma once

#include <JuceHeader.h>
#include <atomic>
#include "DSP/ReverbDiagnostics.h"

//==============================================================================
/**
    Lock-free ring buffer for single producer, single consumer audio data transfer.
    Thread-safe without locks - suitable for real-time audio processing.
*/
class LockFreeRingBuffer
{
public:
    LockFreeRingBuffer() = default;

    void setSize(int numSamples)
    {
        bufferSize = numSamples;
        buffer.setSize(1, bufferSize);
        buffer.clear();
        writePosition.store(0);
        readPosition.store(0);
    }

    // Write samples to the ring buffer (called by producer thread)
    int write(const float* data, int numSamples)
    {
        int writePos = writePosition.load(std::memory_order_relaxed);
        int readPos = readPosition.load(std::memory_order_acquire);

        int available = getAvailableSpace(writePos, readPos);
        int toWrite = juce::jmin(numSamples, available);

#if REVERB_DIAGNOSTICS
        int dropped = numSamples - toWrite;
        if (dropped > 0)
        {
            overflowSamples.fetch_add (static_cast<uint64_t> (dropped), std::memory_order_relaxed);
            overflowEvents.fetch_add (1, std::memory_order_relaxed);
        }
#endif

        auto* writePtr = buffer.getWritePointer(0);

        // Use memcpy with wrap-around handling (1-2 copies instead of per-sample loop)
        int firstChunk = juce::jmin(toWrite, bufferSize - writePos);
        std::memcpy(writePtr + writePos, data, static_cast<size_t>(firstChunk) * sizeof(float));

        int secondChunk = toWrite - firstChunk;
        if (secondChunk > 0)
            std::memcpy(writePtr, data + firstChunk, static_cast<size_t>(secondChunk) * sizeof(float));

        writePos = (writePos + toWrite) % bufferSize;

        writePosition.store(writePos, std::memory_order_release);
        return toWrite;
    }

    // Read samples from the ring buffer (called by consumer thread)
    int read(float* data, int numSamples)
    {
        int readPos = readPosition.load(std::memory_order_relaxed);
        int writePos = writePosition.load(std::memory_order_acquire);

        int available = getAvailableData(writePos, readPos);
        int toRead = juce::jmin(numSamples, available);

#if REVERB_DIAGNOSTICS
        int deficit = numSamples - toRead;
        if (deficit > 0)
        {
            underrunSamples.fetch_add (static_cast<uint64_t> (deficit), std::memory_order_relaxed);
            underrunEvents.fetch_add (1, std::memory_order_relaxed);
        }
#endif

        auto* readPtr = buffer.getReadPointer(0);

        // Use memcpy with wrap-around handling (1-2 copies instead of per-sample loop)
        int firstChunk = juce::jmin(toRead, bufferSize - readPos);
        std::memcpy(data, readPtr + readPos, static_cast<size_t>(firstChunk) * sizeof(float));

        int secondChunk = toRead - firstChunk;
        if (secondChunk > 0)
            std::memcpy(data + firstChunk, readPtr, static_cast<size_t>(secondChunk) * sizeof(float));

        readPos = (readPos + toRead) % bufferSize;

        readPosition.store(readPos, std::memory_order_release);
        return toRead;
    }

    // Get available samples for reading
    int getAvailableData() const
    {
        int writePos = writePosition.load(std::memory_order_acquire);
        int readPos = readPosition.load(std::memory_order_acquire);
        return getAvailableData(writePos, readPos);
    }

    void reset()
    {
        writePosition.store(0);
        readPosition.store(0);
        buffer.clear();
#if REVERB_DIAGNOSTICS
        overflowSamples.store (0);
        overflowEvents.store (0);
        underrunSamples.store (0);
        underrunEvents.store (0);
#endif
    }

#if REVERB_DIAGNOSTICS
    std::atomic<uint64_t> overflowSamples { 0 };
    std::atomic<uint64_t> overflowEvents  { 0 };
    std::atomic<uint64_t> underrunSamples { 0 };
    std::atomic<uint64_t> underrunEvents  { 0 };
#endif

private:
    juce::AudioBuffer<float> buffer;
    int bufferSize = 0;
    std::atomic<int> writePosition {0};
    std::atomic<int> readPosition {0};

    int getAvailableData(int writePos, int readPos) const
    {
        if (writePos >= readPos)
            return writePos - readPos;
        else
            return bufferSize - readPos + writePos;
    }

    int getAvailableSpace(int writePos, int readPos) const
    {
        // Keep one sample empty to distinguish full from empty
        return bufferSize - getAvailableData(writePos, readPos) - 1;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LockFreeRingBuffer)
};
