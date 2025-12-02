#pragma once

#include <JuceHeader.h>
#include <atomic>

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

        auto* writePtr = buffer.getWritePointer(0);

        for (int i = 0; i < toWrite; ++i)
        {
            writePtr[writePos] = data[i];
            writePos = (writePos + 1) % bufferSize;
        }

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

        auto* readPtr = buffer.getReadPointer(0);

        for (int i = 0; i < toRead; ++i)
        {
            data[i] = readPtr[readPos];
            readPos = (readPos + 1) % bufferSize;
        }

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
    }

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
