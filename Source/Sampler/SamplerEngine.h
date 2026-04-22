#pragma once

#include <JuceHeader.h>
#include "SamplerData.h"
#include "../DSP/WFSHighShelfFilter.h"

/**
 * Per-channel monophonic sampler engine.
 * Plays one sample at a time with fade-in/out envelope and HF shelf filter.
 * Called from the audio thread — all methods must be real-time safe.
 */
class SamplerEngine
{
public:
    /** Touch event passed from BLOCKS/MIDI to audio thread via lock-free FIFO */
    struct TouchEvent
    {
        enum Type { NoteOn, NoteOff, Pressure, XYMove };
        Type type = NoteOn;
        int cellIndex = -1;          // Which cell to trigger (NoteOn only)
        float pressure = 0.0f;       // 0..1 normalized
        float deltaX = 0.0f;         // Normalized XY delta from strike origin
        float deltaY = 0.0f;
    };

    SamplerEngine() = default;

    //==========================================================================
    /** Prepare engine for playback */
    void prepare (double newSampleRate, int /*maxBlockSize*/)
    {
        sampleRate = newSampleRate;
        hfFilter.prepare (newSampleRate);
        hfFilter.setGainDb (0.0f);
        reset();
    }

    /** Reset engine state */
    void reset()
    {
        playing = false;
        playbackPos = 0;
        envelopeGain = 0.0f;
        currentCellIndex = -1;
        currentBuffer = nullptr;
        fadeOutRemaining = 0;
        fadeInRemaining = 0;
        hfFilter.reset();
    }

    //==========================================================================
    /** Load cells and current set from data (called from message thread, guarded) */
    void loadCells (const std::vector<SamplerData::SampleCell>& newCells)
    {
        // Copy cell data atomically (audio thread reads currentBuffer pointer)
        juce::SpinLock::ScopedLockType lock (cellLock);
        cells = newCells;
    }

    void loadSet (const SamplerData::SamplerSet& newSet)
    {
        juce::SpinLock::ScopedLockType lock (setLock);
        currentSet = newSet;
    }

    /** Get the next cell index from the current set (message thread only) */
    int getNextCellFromSet()
    {
        juce::SpinLock::ScopedLockType lock (setLock);
        return currentSet.getNextCellIndex();
    }

    //==========================================================================
    /** Push a touch event to the lock-free FIFO (called from MIDI/BLOCKS thread) */
    bool pushEvent (const TouchEvent& event)
    {
        return eventFifo.push (event);
    }

    //==========================================================================
    /**
     * Process audio for this channel. Overwrites the output buffer.
     * Called from audio thread.
     *
     * @param output      Pointer to mono output samples
     * @param numSamples  Number of samples to generate
     */
    void processBlock (float* output, int numSamples)
    {
        // Drain pending events
        TouchEvent evt;
        while (eventFifo.pop (evt))
            handleEvent (evt);

        if (! playing)
        {
            juce::FloatVectorOperations::clear (output, numSamples);
            return;
        }

        // Get current cell's audio buffer
        std::shared_ptr<juce::AudioBuffer<float>> buf;
        float cellAtten = 0.0f;
        {
            juce::SpinLock::ScopedLockType lock (cellLock);
            buf = currentBuffer;
            if (currentCellIndex >= 0 && currentCellIndex < static_cast<int> (cells.size()))
                cellAtten = cells[static_cast<size_t> (currentCellIndex)].attenuation;
        }

        if (buf == nullptr || buf->getNumSamples() == 0)
        {
            playing = false;
            juce::FloatVectorOperations::clear (output, numSamples);
            return;
        }

        // Compute gains
        float setLevelLin;
        float pressLevelMod;
        bool  pressLevelEnabled;
        float pressHFGain;
        {
            juce::SpinLock::ScopedLockType lock (setLock);
            setLevelLin       = juce::Decibels::decibelsToGain (currentSet.level);
            pressLevelMod     = currentSet.pressLevel.apply (currentPressure);
            pressLevelEnabled = currentSet.pressLevel.enabled;
            pressHFGain = currentSet.pressHF.enabled
                              ? -12.0f * currentSet.pressHF.apply (currentPressure)  // Up to -12dB HF cut
                              : 0.0f;
        }

        float cellAttenLin = juce::Decibels::decibelsToGain (cellAtten);
        float baseLevelLin = setLevelLin * cellAttenLin;

        // When pressLevel is enabled, pressLevelMod ranges 0..1 over finger
        // pressure (direction 0 = more pressure → more level, direction 1 =
        // inverted). Branch on .enabled rather than on the value so the
        // legitimate zero-pressure case stays at silence instead of snapping
        // back to full level.
        float levelGain = baseLevelLin;
        if (pressLevelEnabled)
            levelGain *= juce::jlimit (0.0f, 1.0f, pressLevelMod);

        // Update HF shelf with pressure
        hfFilter.setGainDb (pressHFGain);

        const float* src = buf->getReadPointer (0);
        int totalSamples = buf->getNumSamples();

        for (int i = 0; i < numSamples; ++i)
        {
            float sample = 0.0f;

            if (playbackPos < totalSamples)
            {
                sample = src[playbackPos];
                playbackPos++;
            }
            else
            {
                // Sample finished
                playing = false;
                juce::FloatVectorOperations::clear (output + i, numSamples - i);
                return;
            }

            // Apply envelope
            sample *= computeEnvelope();

            // Apply level
            sample *= levelGain;

            // Apply HF shelf
            sample = hfFilter.processSample (sample);

            output[i] = sample;
        }
    }

    //==========================================================================
    /** Get current position override values (read by 50Hz timer on message thread) */
    float getPositionX() const noexcept { return posX.load (std::memory_order_relaxed); }
    float getPositionY() const noexcept { return posY.load (std::memory_order_relaxed); }
    float getPositionZ() const noexcept { return posZ.load (std::memory_order_relaxed); }
    bool  isPlaying()    const noexcept { return playing; }
    int   getCurrentCellIndex() const noexcept { return currentCellIndex; }
    bool  hasPositionOverride() const noexcept { return positionOverrideActive.load (std::memory_order_relaxed); }

    /** Release position override (called on touch end / finger lift) */
    void releasePositionOverride()
    {
        positionOverrideActive.store (false, std::memory_order_release);
        posX.store (0.0f, std::memory_order_relaxed);
        posY.store (0.0f, std::memory_order_relaxed);
        posZ.store (0.0f, std::memory_order_relaxed);
        xyAccumX = 0.0f;
        xyAccumY = 0.0f;
    }

    /** Update XY delta from controller movement (Lightpad/remote).
        Values are pre-scaled by the caller (e.g. LightpadManager applies sensitivity). */
    void updatePosition (float scaledDeltaX, float scaledDeltaY)
    {
        if (! currentSet.pressXYEnabled)
            return;

        xyAccumX += scaledDeltaX;
        xyAccumY += scaledDeltaY;
        posX.store (xyAccumX, std::memory_order_relaxed);
        posY.store (xyAccumY, std::memory_order_relaxed);
        positionOverrideActive.store (true, std::memory_order_relaxed);
    }

private:
    //==========================================================================
    void handleEvent (const TouchEvent& evt)
    {
        switch (evt.type)
        {
            case TouchEvent::NoteOn:
                triggerCell (evt.cellIndex, evt.pressure);
                break;

            case TouchEvent::NoteOff:
                startFadeOut();
                break;

            case TouchEvent::Pressure:
                currentPressure = evt.pressure;
                break;

            case TouchEvent::XYMove:
                updatePosition (evt.deltaX, evt.deltaY);
                break;
        }
    }

    void triggerCell (int cellIndex, float pressure)
    {
        juce::SpinLock::ScopedLockType lock (cellLock);

        if (cellIndex < 0 || cellIndex >= static_cast<int> (cells.size()))
            return;

        auto& cell = cells[static_cast<size_t> (cellIndex)];
        if (! cell.hasAudio())
            return;

        currentBuffer = cell.audioBuffer;
        currentCellIndex = cellIndex;
        currentPressure = pressure;

        // Calculate fade-in samples from inTime (ms)
        fadeInRemaining = static_cast<int> (cell.inTime * 0.001f * static_cast<float> (sampleRate));
        fadeInTotal = fadeInRemaining;
        fadeOutRemaining = 0;

        // Calculate fade-out length from outTime for later use
        fadeOutTotal = static_cast<int> (cell.outTime * 0.001f * static_cast<float> (sampleRate));

        playbackPos = 0;
        envelopeGain = (fadeInRemaining > 0) ? 0.0f : 1.0f;
        playing = true;

        // Store cell offset as transient delta from current input position
        // Initialize XY accumulators so updatePosition() accumulates from here
        xyAccumX = cell.offsetX;
        xyAccumY = cell.offsetY;
        posX.store (cell.offsetX, std::memory_order_relaxed);
        posY.store (cell.offsetY, std::memory_order_relaxed);
        posZ.store (cell.offsetZ, std::memory_order_relaxed);
        positionOverrideActive.store (true, std::memory_order_release);
    }

    void startFadeOut()
    {
        if (playing && fadeOutRemaining <= 0)
        {
            fadeOutRemaining = fadeOutTotal;
            if (fadeOutRemaining <= 0)
                playing = false;
        }
    }

    float computeEnvelope()
    {
        // Fade-out takes priority
        if (fadeOutRemaining > 0)
        {
            envelopeGain = static_cast<float> (fadeOutRemaining) / static_cast<float> (juce::jmax (1, fadeOutTotal));
            fadeOutRemaining--;
            if (fadeOutRemaining <= 0)
            {
                playing = false;
                return 0.0f;
            }
            return envelopeGain;
        }

        // Fade-in
        if (fadeInRemaining > 0)
        {
            envelopeGain = 1.0f - static_cast<float> (fadeInRemaining) / static_cast<float> (juce::jmax (1, fadeInTotal));
            fadeInRemaining--;
            return envelopeGain;
        }

        // Sustain
        envelopeGain = 1.0f;
        return 1.0f;
    }

    //==========================================================================
    // Lock-free event FIFO (BLOCKS/MIDI thread → audio thread)
    struct EventFifo
    {
        static constexpr int capacity = 64;
        TouchEvent buffer[capacity];
        std::atomic<int> readPos { 0 };
        std::atomic<int> writePos { 0 };

        bool push (const TouchEvent& e)
        {
            int wp = writePos.load (std::memory_order_relaxed);
            int next = (wp + 1) % capacity;
            if (next == readPos.load (std::memory_order_acquire))
                return false;  // Full
            buffer[wp] = e;
            writePos.store (next, std::memory_order_release);
            return true;
        }

        bool pop (TouchEvent& e)
        {
            int rp = readPos.load (std::memory_order_relaxed);
            if (rp == writePos.load (std::memory_order_acquire))
                return false;  // Empty
            e = buffer[rp];
            readPos.store ((rp + 1) % capacity, std::memory_order_release);
            return true;
        }
    };

    //==========================================================================
    double sampleRate = 44100.0;
    bool playing = false;
    int playbackPos = 0;
    int currentCellIndex = -1;
    float currentPressure = 0.0f;
    float envelopeGain = 0.0f;

    // Fade envelope state
    int fadeInRemaining = 0;
    int fadeInTotal = 0;
    int fadeOutRemaining = 0;
    int fadeOutTotal = 0;

    // Cell data (protected by SpinLock for message→audio thread sync)
    juce::SpinLock cellLock;
    std::vector<SamplerData::SampleCell> cells;
    std::shared_ptr<juce::AudioBuffer<float>> currentBuffer;

    // Set data
    juce::SpinLock setLock;
    SamplerData::SamplerSet currentSet;

    // HF shelf filter for pressure modulation
    WFSHighShelfFilter hfFilter;

    // Position override (atomics read by 50Hz message-thread timer)
    std::atomic<float> posX { 0.0f };
    std::atomic<float> posY { 0.0f };
    std::atomic<float> posZ { 0.0f };
    std::atomic<bool> positionOverrideActive { false };

    // XY joystick accumulator (audio thread only)
    float xyAccumX = 0.0f;
    float xyAccumY = 0.0f;

    // Event FIFO
    EventFifo eventFifo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerEngine)
};
