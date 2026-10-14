#pragma once

#include <JuceHeader.h>
#include "SamplerData.h"
#include "../../spatcore/dsp/WFSHighShelfFilter.h"

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
        enum Type { NoteOn, NoteOff, Pressure, XYMove, Reset };
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
        clearPositionOverride();
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
    /** Push a touch event to the FIFO. Called from the Lightpad's thread and
        from the message thread (preview, remote pad, XY moves): the producers
        share a spin lock; the audio thread pops without one. */
    bool pushEvent (const TouchEvent& event)
    {
        const juce::SpinLock::ScopedLockType lock (pushLock);
        return eventFifo.push (event);
    }

    /** Ends a note still held, without a fade, and what was queued before
        this call with it; what is queued after plays as usual. For a channel
        coming back on: while it was off nothing processed it, so a note it
        was playing stood frozen and played on, moving the input, when the
        channel came back. Queued as an event so a pad pressed right after
        the channel comes on still sounds; with the queue full, every queued
        event goes. Message thread; done on the audio thread. */
    void requestReset() noexcept
    {
        TouchEvent reset;
        reset.type = TouchEvent::Reset;
        if (! pushEvent (reset))
            resetRequested.store (true, std::memory_order_release);
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
        TouchEvent evt;
        if (resetRequested.exchange (false, std::memory_order_acquire))
        {
            while (eventFifo.pop (evt)) {}
            reset();
        }

        // Drain pending events
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
            stopPlayback();
            juce::FloatVectorOperations::clear (output, numSamples);
            return;
        }

        // Compute gains
        float setLevelLin;
        float pressLevelMod;
        bool  pressLevelEnabled;
        float pressHFGain;
        bool  pressZEnabled;
        float pressZMod;
        {
            juce::SpinLock::ScopedLockType lock (setLock);
            setLevelLin       = gainOf (currentSet.level);
            pressLevelMod     = currentSet.pressLevel.apply (currentPressure);
            pressLevelEnabled = currentSet.pressLevel.enabled;
            pressHFGain = currentSet.pressHF.enabled
                              ? -12.0f * currentSet.pressHF.apply (currentPressure)  // Up to -12dB HF cut
                              : 0.0f;
            pressZEnabled = currentSet.pressZ.enabled;
            pressZMod     = currentSet.pressZ.apply (currentPressure);
        }

        // Height: pressure scales the cell's Z offset (re-audit F9; it was
        // shown and saved, never applied). Only while the note holds the
        // position override.
        if (pressZEnabled && positionOverrideActive.load (std::memory_order_relaxed))
            posZ.store (currentCellOffsetZ * juce::jlimit (0.0f, 1.0f, pressZMod), std::memory_order_relaxed);

        float cellAttenLin = gainOf (cellAtten);
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
                stopPlayback();
                juce::FloatVectorOperations::clear (output + i, numSamples - i);
                return;
            }

            // Apply envelope. When the fade-out ends partway through the
            // block the rest is silence: the loop used to carry on into the
            // sustain branch at full level (re-audit F5).
            const float envelope = computeEnvelope();
            if (! playing)
            {
                juce::FloatVectorOperations::clear (output + i, numSamples - i);
                return;
            }
            sample *= envelope;

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
    bool  isPlaying()    const noexcept { return playing.load (std::memory_order_relaxed); }
    int   getCurrentCellIndex() const noexcept { return currentCellIndex.load (std::memory_order_relaxed); }
    bool  hasPositionOverride() const noexcept { return positionOverrideActive.load (std::memory_order_relaxed); }

    /** Number of triggers rejected (empty/invalid cell) since the last call.
        Written on the audio thread, drained by a message-thread timer for logging. */
    int fetchAndClearRejectedTriggers() noexcept
    {
        return rejectedTriggerCount.exchange (0, std::memory_order_relaxed);
    }

private:
    //==========================================================================
    // The position override lives exactly as long as the note: set when a
    // cell triggers, cleared on the audio thread when playback ends - its
    // fade-out over, the sample run out, or the engine reset. It used to be
    // cleared only by a Lightpad or tablet finger lift on the message thread,
    // so a preview, a sample reaching its end or a tap shorter than one block
    // (cleared before the NoteOn was even drained) left the input moved by the
    // cell's offset for good (re-audit F4).

    void stopPlayback() noexcept
    {
        playing = false;
        fadeInRemaining = 0;
        fadeOutRemaining = 0;
        envelopeGain = 0.0f;
        clearPositionOverride();
    }

    void clearPositionOverride() noexcept
    {
        positionOverrideActive.store (false, std::memory_order_release);
        posX.store (0.0f, std::memory_order_relaxed);
        posY.store (0.0f, std::memory_order_relaxed);
        posZ.store (0.0f, std::memory_order_relaxed);
        xyAccumX = 0.0f;
        xyAccumY = 0.0f;
    }

    /** XY delta from controller movement (Lightpad/remote), pre-scaled by the
        caller's global sensitivity, and scaled again by the set's XY scale
        relative to its default (re-audit F9: shown and saved, never used).
        Ignored once the note is releasing: a move arriving after the finger
        lifted must not bring the override back. Audio thread. */
    void updatePosition (float scaledDeltaX, float scaledDeltaY)
    {
        if (! playing || fadeOutRemaining > 0)
            return;

        bool enabled;
        float scale;
        {
            juce::SpinLock::ScopedLockType lock (setLock);
            enabled = currentSet.pressXYEnabled;
            scale = currentSet.pressXYScale / WFSParameterDefaults::samplerSetPressXYScaleDefault;
        }
        if (! enabled || ! std::isfinite (scaledDeltaX) || ! std::isfinite (scaledDeltaY))
            return;

        xyAccumX += scaledDeltaX * scale;
        xyAccumY += scaledDeltaY * scale;
        posX.store (xyAccumX, std::memory_order_relaxed);
        posY.store (xyAccumY, std::memory_order_relaxed);
        positionOverrideActive.store (true, std::memory_order_relaxed);
    }

    /** dB to gain, held at or below unity: NaN or a level above 0 dB never
        reaches the output (the loaders bound it; this is the last stop). */
    static float gainOf (float db) noexcept
    {
        if (! std::isfinite (db))
            return 0.0f;
        return juce::Decibels::decibelsToGain (juce::jmin (0.0f, db));
    }
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
                currentPressure = std::isfinite (evt.pressure) ? juce::jlimit (0.0f, 1.0f, evt.pressure) : 0.0f;
                break;

            case TouchEvent::XYMove:
                updatePosition (evt.deltaX, evt.deltaY);
                break;

            case TouchEvent::Reset:
                reset();
                break;
        }
    }

    void triggerCell (int cellIndex, float pressure)
    {
        juce::SpinLock::ScopedLockType lock (cellLock);

        if (cellIndex < 0 || cellIndex >= static_cast<int> (cells.size()))
        {
            rejectedTriggerCount.fetch_add (1, std::memory_order_relaxed);
            return;
        }

        auto& cell = cells[static_cast<size_t> (cellIndex)];
        if (! cell.hasAudio())
        {
            rejectedTriggerCount.fetch_add (1, std::memory_order_relaxed);
            return;
        }

        currentBuffer = cell.audioBuffer;
        currentCellIndex = cellIndex;
        currentPressure = std::isfinite (pressure) ? juce::jlimit (0.0f, 1.0f, pressure) : 0.0f;
        currentCellOffsetZ = cell.offsetZ;

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

    /** The fade-out starts from the level the envelope has reached: from 1.0
        it was a full-level burst on a quick tap of a cell with a long fade-in
        (NoteOn and NoteOff in one block: the whole fade-out at full level). */
    void startFadeOut()
    {
        if (playing && fadeOutRemaining <= 0)
        {
            fadeOutStartGain = envelopeGain;
            fadeInRemaining = 0;
            fadeOutRemaining = fadeOutTotal;
            if (fadeOutRemaining <= 0 || fadeOutStartGain <= 0.0f)
                stopPlayback();
        }
    }

    float computeEnvelope()
    {
        // Fade-out takes priority
        if (fadeOutRemaining > 0)
        {
            envelopeGain = fadeOutStartGain * static_cast<float> (fadeOutRemaining)
                                            / static_cast<float> (juce::jmax (1, fadeOutTotal));
            fadeOutRemaining--;
            if (fadeOutRemaining <= 0)
            {
                stopPlayback();
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
    std::atomic<bool> playing { false };        // read by the message thread (isPlaying)
    int playbackPos = 0;
    std::atomic<int> currentCellIndex { -1 };   // read by the message thread
    float currentPressure = 0.0f;
    float envelopeGain = 0.0f;
    float fadeOutStartGain = 1.0f;              // envelope level when the release began
    float currentCellOffsetZ = 0.0f;            // the playing cell's Z offset, for the Height mapping

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

    // Triggers rejected on the audio thread (no audio in cell); drained for logging
    std::atomic<int> rejectedTriggerCount { 0 };

    // Position override (atomics read by 50Hz message-thread timer)
    std::atomic<float> posX { 0.0f };
    std::atomic<float> posY { 0.0f };
    std::atomic<float> posZ { 0.0f };
    std::atomic<bool> positionOverrideActive { false };
    std::atomic<bool> resetRequested { false };

    // XY joystick accumulator (audio thread only)
    float xyAccumX = 0.0f;
    float xyAccumY = 0.0f;

    // Event FIFO
    EventFifo eventFifo;
    juce::SpinLock pushLock;   // producers only

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerEngine)
};
