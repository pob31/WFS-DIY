#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <vector>
#include <cmath>

/**
 * OutputLevelDetector
 *
 * Per-output audio level detection for metering.
 * Runs on audio thread, provides peak/RMS levels to UI thread via atomics.
 *
 * Simplified version of LiveSourceLevelDetector without compression/gain reduction.
 * Two detection paths:
 * 1. Peak: abs -> envelope (1 sample attack, 100ms release) -> dB
 * 2. RMS: circular buffer (200ms window) -> dB
 */
class OutputLevelDetector
{
public:
    OutputLevelDetector() = default;

    /**
     * Prepare the detector for a given sample rate.
     * Must be called before processSample().
     */
    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;

        // Peak envelope: 1 sample attack (instant), 100ms release
        // Release coefficient: value decays to 1/e in releaseTime seconds
        peakEnvelopeReleaseCoeff = static_cast<float>(std::exp(-1.0 / (sampleRate * 0.1)));
        peakEnvelope = 0.0f;

        // RMS buffer: window = sampleRate / 5 (~200ms at 48kHz)
        rmsWindowSize = static_cast<int>(sampleRate / 5.0);
        rmsBuffer.resize(rmsWindowSize, 0.0f);
        rmsWritePos = 0;
        rmsSumSquared = 0.0f;

        // Reset atomic outputs
        peakDb.store(-200.0f, std::memory_order_relaxed);
        rmsDb.store(-200.0f, std::memory_order_relaxed);
    }

    /**
     * Process a single audio sample.
     * Call this for every sample on the audio thread.
     */
    void processSample(float sample)
    {
        // === PEAK DETECTION PATH ===
        float absSample = std::abs(sample);

        // Peak envelope follower: instant attack, exponential release
        if (absSample > peakEnvelope)
            peakEnvelope = absSample;  // Instant attack
        else
            peakEnvelope *= peakEnvelopeReleaseCoeff;  // Exponential release

        // Convert to dB (with floor to avoid -inf)
        float currentPeakDb = (peakEnvelope > 1e-10f)
            ? 20.0f * std::log10(peakEnvelope)
            : -200.0f;

        // Update atomic output
        peakDb.store(currentPeakDb, std::memory_order_relaxed);

        // === RMS DETECTION PATH ===
        // Update RMS circular buffer
        float sampleSquared = sample * sample;
        rmsSumSquared -= rmsBuffer[rmsWritePos];  // Remove old sample
        rmsSumSquared += sampleSquared;           // Add new sample
        rmsBuffer[rmsWritePos] = sampleSquared;
        rmsWritePos = (rmsWritePos + 1) % rmsWindowSize;

        // Calculate RMS level
        float rmsLevel = std::sqrt(rmsSumSquared / static_cast<float>(rmsWindowSize));

        // Convert to dB
        float currentRmsDb = (rmsLevel > 1e-10f)
            ? 20.0f * std::log10(rmsLevel)
            : -200.0f;

        // Update atomic output
        rmsDb.store(currentRmsDb, std::memory_order_relaxed);
    }

    /**
     * Get peak level in dB (for metering).
     * Safe to call from any thread.
     */
    float getPeakLevelDb() const
    {
        return peakDb.load(std::memory_order_relaxed);
    }

    /**
     * Get RMS level in dB (for metering).
     * Safe to call from any thread.
     */
    float getRmsLevelDb() const
    {
        return rmsDb.load(std::memory_order_relaxed);
    }

    /**
     * Reset the detector state (call when audio stops).
     */
    void reset()
    {
        peakEnvelope = 0.0f;
        std::fill(rmsBuffer.begin(), rmsBuffer.end(), 0.0f);
        rmsSumSquared = 0.0f;
        rmsWritePos = 0;
        peakDb.store(-200.0f, std::memory_order_relaxed);
        rmsDb.store(-200.0f, std::memory_order_relaxed);
    }

private:
    double sampleRate = 48000.0;

    // Peak envelope follower
    float peakEnvelope = 0.0f;
    float peakEnvelopeReleaseCoeff = 0.0f;

    // RMS calculation (circular buffer)
    std::vector<float> rmsBuffer;
    int rmsWindowSize = 9600;
    int rmsWritePos = 0;
    float rmsSumSquared = 0.0f;

    // Thread-safe outputs (written by audio thread, read by UI thread)
    std::atomic<float> peakDb{-200.0f};
    std::atomic<float> rmsDb{-200.0f};
};
