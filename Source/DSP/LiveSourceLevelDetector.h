#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <vector>
#include <cmath>

/**
 * LiveSourceLevelDetector
 *
 * Per-input audio level detection for the Live Source Tamer feature.
 * Runs on audio thread, provides gain reduction values to timer thread via atomics.
 *
 * Two detection paths:
 * 1. Peak: abs -> envelope (1 sample attack, 100ms release) -> dB -> gain calc -> smooth (2ms/2ms)
 * 2. Slow: RMS (200ms window) -> dB -> gain calc -> smooth (2ms/20ms)
 *
 * Gain calculation uses soft knee compression with 20dB knee width.
 */
class LiveSourceLevelDetector
{
public:
    LiveSourceLevelDetector() = default;

    /**
     * Prepare the detector for a given sample rate.
     * Must be called before processSample().
     */
    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;

        // Peak envelope: 1 sample attack (instant), 100ms release (4800 samples at 48kHz)
        // Release coefficient: value decays to 1/e in releaseTime seconds
        // coeff = exp(-1 / (sampleRate * releaseTime))
        peakEnvelopeReleaseCoeff = static_cast<float>(std::exp(-1.0 / (sampleRate * 0.1)));
        peakEnvelope = 0.0f;

        // Short peak envelope: 1 sample attack (instant), 5ms release for AutomOtion triggering
        shortPeakReleaseCoeff = static_cast<float>(std::exp(-1.0 / (sampleRate * 0.005)));
        shortPeakEnvelope = 0.0f;

        // RMS buffer: window = sampleRate / 5 (~200ms at 48kHz)
        rmsWindowSize = static_cast<int>(sampleRate / 5.0);
        rmsBuffer.resize(rmsWindowSize, 0.0f);
        rmsWritePos = 0;
        rmsSumSquared = 0.0f;

        // Gain smoothing coefficients
        // Attack: 96 samples (~2ms at 48kHz)
        // Peak release: 96 samples (~2ms)
        // Slow release: 960 samples (~20ms)
        float attackSamples = 96.0f;
        float peakReleaseSamples = 96.0f;
        float slowReleaseSamples = 960.0f;

        peakGainAttackCoeff = static_cast<float>(std::exp(-1.0 / attackSamples));
        peakGainReleaseCoeff = static_cast<float>(std::exp(-1.0 / peakReleaseSamples));
        slowGainAttackCoeff = static_cast<float>(std::exp(-1.0 / attackSamples));
        slowGainReleaseCoeff = static_cast<float>(std::exp(-1.0 / slowReleaseSamples));

        // Initialize smoothed gains to 1.0 (no reduction)
        peakGainSmoothed = 1.0f;
        slowGainSmoothed = 1.0f;

        // Reset atomic outputs
        peakGR.store(1.0f, std::memory_order_relaxed);
        slowGR.store(1.0f, std::memory_order_relaxed);
    }

    /**
     * Process a single audio sample.
     * Call this for every sample on the audio thread.
     */
    void processSample(float sample)
    {
        // === PEAK DETECTION PATH ===
        // abs -> envelope (instant attack, 100ms release) -> dB -> gain -> smooth

        float absSample = std::abs(sample);

        // Peak envelope follower: instant attack, exponential release
        if (absSample > peakEnvelope)
            peakEnvelope = absSample;  // Instant attack
        else
            peakEnvelope *= peakEnvelopeReleaseCoeff;  // Exponential release

        // Short peak envelope follower: instant attack, 5ms release for AutomOtion triggering
        if (absSample > shortPeakEnvelope)
            shortPeakEnvelope = absSample;  // Instant attack
        else
            shortPeakEnvelope *= shortPeakReleaseCoeff;  // 5ms exponential release

        // Convert to dB (with floor to avoid -inf)
        float peakDb = (peakEnvelope > 1e-10f)
            ? 20.0f * std::log10(peakEnvelope)
            : -200.0f;

        // Calculate gain reduction
        float peakThresh = peakThreshold.load(std::memory_order_relaxed);
        float peakRat = peakRatio.load(std::memory_order_relaxed);
        float peakGainTarget = calculateGainReduction(peakDb, peakThresh, peakRat);

        // Smooth the gain (fast attack when reducing, fast release when recovering)
        if (peakGainTarget < peakGainSmoothed)
            peakGainSmoothed = peakGainTarget + peakGainAttackCoeff * (peakGainSmoothed - peakGainTarget);
        else
            peakGainSmoothed = peakGainTarget + peakGainReleaseCoeff * (peakGainSmoothed - peakGainTarget);

        // Update atomic output
        peakGR.store(peakGainSmoothed, std::memory_order_relaxed);

        // === SLOW DETECTION PATH ===
        // RMS (200ms window) -> dB -> gain -> smooth

        // Update RMS circular buffer
        float sampleSquared = sample * sample;
        rmsSumSquared -= rmsBuffer[rmsWritePos];  // Remove old sample
        rmsSumSquared += sampleSquared;           // Add new sample
        rmsBuffer[rmsWritePos] = sampleSquared;
        rmsWritePos = (rmsWritePos + 1) % rmsWindowSize;

        // Calculate RMS level
        float rmsLevel = std::sqrt(rmsSumSquared / static_cast<float>(rmsWindowSize));

        // Convert to dB
        float rmsDb = (rmsLevel > 1e-10f)
            ? 20.0f * std::log10(rmsLevel)
            : -200.0f;

        // Calculate gain reduction
        float slowThresh = slowThreshold.load(std::memory_order_relaxed);
        float slowRat = slowRatio.load(std::memory_order_relaxed);
        float slowGainTarget = calculateGainReduction(rmsDb, slowThresh, slowRat);

        // Smooth the gain (fast attack, slow release)
        if (slowGainTarget < slowGainSmoothed)
            slowGainSmoothed = slowGainTarget + slowGainAttackCoeff * (slowGainSmoothed - slowGainTarget);
        else
            slowGainSmoothed = slowGainTarget + slowGainReleaseCoeff * (slowGainSmoothed - slowGainTarget);

        // Update atomic output
        slowGR.store(slowGainSmoothed, std::memory_order_relaxed);
    }

    /**
     * Get the current peak gain reduction (linear, 0-1).
     * Safe to call from any thread.
     */
    float getPeakGainReduction() const
    {
        return peakGR.load(std::memory_order_relaxed);
    }

    /**
     * Get the current slow gain reduction (linear, 0-1).
     * Safe to call from any thread.
     */
    float getSlowGainReduction() const
    {
        return slowGR.load(std::memory_order_relaxed);
    }

    /**
     * Set compressor parameters.
     * Safe to call from any thread (typically timer thread).
     */
    void setParameters(float peakThreshDb, float peakRat,
                       float slowThreshDb, float slowRat)
    {
        peakThreshold.store(peakThreshDb, std::memory_order_relaxed);
        peakRatio.store(peakRat, std::memory_order_relaxed);
        slowThreshold.store(slowThreshDb, std::memory_order_relaxed);
        slowRatio.store(slowRat, std::memory_order_relaxed);
    }

    /**
     * Get peak level in dB (for metering).
     */
    float getPeakLevelDb() const
    {
        float env = peakEnvelope;
        return (env > 1e-10f) ? 20.0f * std::log10(env) : -200.0f;
    }

    /**
     * Get RMS level in dB (for metering).
     */
    float getRmsLevelDb() const
    {
        float rms = std::sqrt(rmsSumSquared / static_cast<float>(rmsWindowSize));
        return (rms > 1e-10f) ? 20.0f * std::log10(rms) : -200.0f;
    }

    /**
     * Get short peak level in dB (5ms hold for AutomOtion triggering).
     */
    float getShortPeakLevelDb() const
    {
        float env = shortPeakEnvelope;
        return (env > 1e-10f) ? 20.0f * std::log10(env) : -200.0f;
    }

private:
    /**
     * Calculate gain reduction using soft knee compression.
     * From Max patch CodeBox formula.
     *
     * @param levelDb Input level in dB
     * @param threshold Threshold in dB
     * @param ratio Compression ratio (1.0 = no compression)
     * @return Gain reduction as linear multiplier (0-1)
     */
    float calculateGainReduction(float levelDb, float threshold, float ratio)
    {
        // No compression if ratio <= 1
        if (ratio <= 1.0f)
            return 1.0f;

        // Above threshold + 10dB: full compression (hard knee region)
        if (levelDb > threshold + 10.0f)
        {
            // gainReduction = pow(10, (threshold - levelDb) * (ratio - 1) / ratio / 20)
            float gainDb = (threshold - levelDb) * (ratio - 1.0f) / ratio;
            return std::pow(10.0f, gainDb / 20.0f);
        }
        // Soft knee region: threshold - 10dB to threshold + 10dB
        else if (levelDb > threshold - 10.0f)
        {
            // Soft knee formula from Max patch:
            // gainReduction = pow(10, ((levelDb - threshold + 10) * (threshold + 10 - levelDb * 20) / (ratio * 20)) / 20)
            // Note: This appears to be a quadratic interpolation through the knee
            float kneePosition = levelDb - threshold + 10.0f;  // 0 at knee start, 20 at knee end
            float kneeGainDb = (kneePosition * (threshold + 10.0f - levelDb * 20.0f)) / (ratio * 20.0f);
            return std::pow(10.0f, kneeGainDb / 20.0f);
        }

        // Below knee: no gain reduction
        return 1.0f;
    }

    double sampleRate = 48000.0;

    // Peak envelope follower
    float peakEnvelope = 0.0f;
    float peakEnvelopeReleaseCoeff = 0.0f;

    // Short peak envelope follower (5ms release for AutomOtion triggering)
    float shortPeakEnvelope = 0.0f;
    float shortPeakReleaseCoeff = 0.0f;

    // RMS calculation (circular buffer)
    std::vector<float> rmsBuffer;
    int rmsWindowSize = 9600;
    int rmsWritePos = 0;
    float rmsSumSquared = 0.0f;

    // Gain smoothing
    float peakGainSmoothed = 1.0f;
    float slowGainSmoothed = 1.0f;
    float peakGainAttackCoeff = 0.0f;
    float peakGainReleaseCoeff = 0.0f;
    float slowGainAttackCoeff = 0.0f;
    float slowGainReleaseCoeff = 0.0f;

    // Thread-safe outputs (written by audio thread, read by timer thread)
    std::atomic<float> peakGR{1.0f};
    std::atomic<float> slowGR{1.0f};

    // Parameters (written by timer thread, read by audio thread)
    std::atomic<float> peakThreshold{-20.0f};
    std::atomic<float> peakRatio{2.0f};
    std::atomic<float> slowThreshold{-20.0f};
    std::atomic<float> slowRatio{2.0f};
};
