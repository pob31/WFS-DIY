#pragma once

#include <JuceHeader.h>
#include <atomic>

//==============================================================================
/**
    Single-producer, multi-consumer ring buffer for sharing input audio data
    across multiple output processor threads.

    The producer (audio callback) writes via write().
    Each consumer tracks its own read position via readWithPosition().
    No contention between consumers since each has an independent cursor.
*/
class SharedInputRingBuffer
{
public:
    SharedInputRingBuffer() = default;

    void setSize(int numSamples)
    {
        bufferSize = numSamples;
        buffer.setSize(1, bufferSize);
        buffer.clear();
        writePos.store(0, std::memory_order_relaxed);
    }

    /** Write samples (called by single producer — audio callback thread). */
    int write(const float* data, int numSamples)
    {
        int wp = writePos.load(std::memory_order_relaxed);
        int toWrite = juce::jmin(numSamples, bufferSize - 1); // leave 1 empty slot
        auto* dst = buffer.getWritePointer(0);

        for (int i = 0; i < toWrite; ++i)
        {
            dst[wp] = data[i];
            wp = (wp + 1) % bufferSize;
        }

        writePos.store(wp, std::memory_order_release);
        return toWrite;
    }

    /** Read samples using a per-consumer read position (thread-safe, no shared mutation). */
    int readWithPosition(int& readPosition, float* data, int numSamples) const
    {
        int wp = writePos.load(std::memory_order_acquire);
        int available = getAvailableAt(wp, readPosition);
        int toRead = juce::jmin(numSamples, available);
        auto* src = buffer.getReadPointer(0);

        for (int i = 0; i < toRead; ++i)
        {
            data[i] = src[readPosition];
            readPosition = (readPosition + 1) % bufferSize;
        }

        return toRead;
    }

    /** Get available samples for a given read position. */
    int getAvailableAt(int readPosition) const
    {
        int wp = writePos.load(std::memory_order_acquire);
        return getAvailableAt(wp, readPosition);
    }

    void reset()
    {
        writePos.store(0, std::memory_order_relaxed);
        buffer.clear();
    }

    int getBufferSize() const { return bufferSize; }

private:
    juce::AudioBuffer<float> buffer;
    int bufferSize = 0;
    std::atomic<int> writePos{0};

    int getAvailableAt(int wp, int rp) const
    {
        if (wp >= rp)
            return wp - rp;
        else
            return bufferSize - rp + wp;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedInputRingBuffer)
};
