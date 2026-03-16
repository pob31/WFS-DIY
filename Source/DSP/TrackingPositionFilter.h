#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <vector>

/**
 * Tracking Position Filter
 *
 * Adaptive smoothing for tracking system position data using the 1-Euro Filter
 * algorithm (Casiez et al., CHI 2012). Designed for noisy tracking systems
 * (UWB, etc.) where tags may lose line-of-sight to anchors.
 *
 * Features:
 * - Adaptive low-pass: responsive to real movement, smooth when stationary
 * - Jump detection: rejects single-frame spikes from multipath reflections
 * - Quality factor support: lower quality = more smoothing
 * - Per-input filter state with per-axis independence
 * - Thread-safe (SpinLock per input)
 *
 * Controlled by a single smoothing percentage (0-100%):
 *   0% = bypass (raw data), 100% = maximum jitter removal
 */
class TrackingPositionFilter
{
public:
    TrackingPositionFilter() = default;

    //==========================================================================
    // Configuration
    //==========================================================================

    /** Resize for the given number of inputs */
    void resize (int numInputs)
    {
        auto newSize = static_cast<size_t> (numInputs);
        while (states.size() < newSize)
            states.push_back (std::make_unique<PerInputState>());
        if (states.size() > newSize)
            states.resize (newSize);
    }

    /** Get number of inputs */
    int getNumInputs() const { return static_cast<int> (states.size()); }

    //==========================================================================
    // Main filtering entry point
    //==========================================================================

    /**
     * Filter a tracking position for a given input.
     *
     * @param inputIndex    The input channel index
     * @param trackingId    The tracking ID (auto-resets filter on ID change)
     * @param x, y, z       Position values (modified in-place with filtered result)
     * @param hasX/Y/Z      Whether each axis has new data (false = skip that axis)
     * @param smoothPercent  Smoothing amount 0-100 (from inputTrackingSmooth)
     * @param qualityFactor  Optional quality/confidence 0-1 (1.0 = full confidence)
     * @return true if the sample was accepted, false if rejected (jump detected)
     */
    bool filterPosition (int inputIndex, int trackingId,
                         float& x, float& y, float& z,
                         bool hasX, bool hasY, bool hasZ,
                         float smoothPercent, float qualityFactor = 1.0f)
    {
        if (inputIndex < 0 || inputIndex >= static_cast<int> (states.size()))
            return true; // out of range, pass through

        auto& state = *states[static_cast<size_t> (inputIndex)];
        juce::SpinLock::ScopedLockType lock (state.spinLock);

        // Auto-reset on tracking ID change
        if (state.lastTrackingId != trackingId)
        {
            state.reset();
            state.lastTrackingId = trackingId;
        }

        double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;

        // Apply quality factor: lower quality = more smoothing
        // quality 1.0 → effectiveSmooth unchanged
        // quality 0.5 → effectiveSmooth * 1.5
        // quality 0.0 → effectiveSmooth * 2.0 (clamped to 100)
        float effectiveSmooth = juce::jlimit (0.0f, 100.0f,
                                              smoothPercent * (2.0f - qualityFactor));

        // Compute 1-Euro parameters from smoothing percentage
        // Exponential mapping for perceptually even control
        float t = effectiveSmooth / 100.0f;
        float minCutoff = 10.0f * std::pow (0.005f, t);  // 10 Hz at 0%, ~0.05 Hz at 100%
        float beta = t * 0.5f;                             // 0 at 0%, 0.5 at 100%

        // --- Jump detection ---
        if (state.initialized)
        {
            float dx = hasX ? (x - state.lastRawX) : 0.0f;
            float dy = hasY ? (y - state.lastRawY) : 0.0f;
            float dz = hasZ ? (z - state.lastRawZ) : 0.0f;
            float dist = std::sqrt (dx * dx + dy * dy + dz * dz);

            double dt = now - state.lastTimestamp;
            if (dt < 0.001) dt = 0.001;

            float velocity = dist / static_cast<float> (dt);

            if (velocity > jumpVelocityThreshold)
            {
                state.jumpHoldCount++;

                if (state.jumpHoldCount < jumpPersistenceFrames)
                {
                    // Reject this sample — likely a spike
                    return false;
                }
                else
                {
                    // Persistent relocation — snap to new position
                    state.jumpHoldCount = 0;
                    state.filterX.reset();
                    state.filterY.reset();
                    state.filterZ.reset();
                    // Fall through to initialize at new position
                }
            }
            else
            {
                state.jumpHoldCount = 0;
            }
        }

        // Update raw position tracking
        if (hasX) state.lastRawX = x;
        if (hasY) state.lastRawY = y;
        if (hasZ) state.lastRawZ = z;
        state.lastTimestamp = now;
        state.initialized = true;

        // --- Apply 1-Euro filter per axis ---
        if (hasX) x = state.filterX.filter (x, now, minCutoff, beta, dCutoff);
        if (hasY) y = state.filterY.filter (y, now, minCutoff, beta, dCutoff);
        if (hasZ) z = state.filterZ.filter (z, now, minCutoff, beta, dCutoff);

        return true;
    }

    /** Reset filter state for a single input */
    void reset (int inputIndex)
    {
        if (inputIndex >= 0 && inputIndex < static_cast<int> (states.size()))
        {
            auto& state = *states[static_cast<size_t> (inputIndex)];
            juce::SpinLock::ScopedLockType lock (state.spinLock);
            state.reset();
        }
    }

    /** Reset all filter states */
    void resetAll()
    {
        for (auto& statePtr : states)
        {
            juce::SpinLock::ScopedLockType lock (statePtr->spinLock);
            statePtr->reset();
        }
    }

private:
    //==========================================================================
    // 1-Euro Filter (single axis)
    //==========================================================================

    struct OneEuroFilter
    {
        float prevFiltered = 0.0f;
        float prevDerivative = 0.0f;
        double prevTimestamp = 0.0;
        bool initialized = false;

        void reset()
        {
            prevFiltered = 0.0f;
            prevDerivative = 0.0f;
            prevTimestamp = 0.0;
            initialized = false;
        }

        float filter (float rawValue, double timestamp,
                      float minCutoff, float beta, float derivativeCutoff)
        {
            if (! initialized)
            {
                prevFiltered = rawValue;
                prevDerivative = 0.0f;
                prevTimestamp = timestamp;
                initialized = true;
                return rawValue;
            }

            double dt = timestamp - prevTimestamp;
            if (dt < 0.001) dt = 0.001;
            if (dt > 1.0) dt = 1.0;
            prevTimestamp = timestamp;

            // Estimate derivative (speed)
            float rawDerivative = (rawValue - prevFiltered) / static_cast<float> (dt);

            // Smooth the derivative with fixed cutoff
            float dAlpha = computeAlpha (derivativeCutoff, dt);
            float filteredDerivative = lowPass (rawDerivative, prevDerivative, dAlpha);
            prevDerivative = filteredDerivative;

            // Adaptive cutoff based on speed
            float speed = std::abs (filteredDerivative);
            float cutoff = minCutoff + beta * speed;

            // Filter the value
            float alpha = computeAlpha (cutoff, dt);
            float filteredValue = lowPass (rawValue, prevFiltered, alpha);
            prevFiltered = filteredValue;

            return filteredValue;
        }

    private:
        static float computeAlpha (float cutoff, double dt)
        {
            float tau = 1.0f / (2.0f * juce::MathConstants<float>::pi * cutoff);
            return 1.0f / (1.0f + tau / static_cast<float> (dt));
        }

        static float lowPass (float raw, float prev, float alpha)
        {
            return prev + alpha * (raw - prev);
        }
    };

    //==========================================================================
    // Per-input filter state
    //==========================================================================

    struct PerInputState
    {
        OneEuroFilter filterX, filterY, filterZ;

        float lastRawX = 0.0f, lastRawY = 0.0f, lastRawZ = 0.0f;
        double lastTimestamp = 0.0;
        int lastTrackingId = -1;
        int jumpHoldCount = 0;
        bool initialized = false;

        juce::SpinLock spinLock;

        void reset()
        {
            filterX.reset();
            filterY.reset();
            filterZ.reset();
            lastRawX = lastRawY = lastRawZ = 0.0f;
            lastTimestamp = 0.0;
            lastTrackingId = -1;
            jumpHoldCount = 0;
            initialized = false;
        }
    };

    //==========================================================================
    // Constants
    //==========================================================================

    static constexpr float jumpVelocityThreshold = 20.0f;  // m/s
    static constexpr int   jumpPersistenceFrames = 3;       // frames before snap
    static constexpr float dCutoff = 1.0f;                  // derivative filter cutoff (Hz)

    //==========================================================================
    // State
    //==========================================================================

    std::vector<std::unique_ptr<PerInputState>> states;
};
