#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSValueTreeState.h"
#include "../Parameters/WFSParameterIDs.h"

/**
 * LFO Processor for WFS Input Position Modulation
 *
 * Generates periodic position offsets for each input channel based on LFO parameters.
 * Called at 50Hz from the MainComponent timer callback.
 *
 * Each input has independent LFO state with:
 * - Main ramp (0→1) that cycles at the period rate
 * - Per-axis waveform shape, rate multiplier, amplitude, and phase
 * - 500ms fade in/out when activating/deactivating
 * - Random shape picks new target at period boundary
 */
class LFOProcessor
{
public:
    //==========================================================================
    // Waveform Shape Enumeration
    //==========================================================================
    enum Shape
    {
        Off = 0,
        Sine = 1,
        Square = 2,
        Sawtooth = 3,
        Triangle = 4,
        Keystone = 5,
        Log = 6,
        Exp = 7,
        Random = 8
    };

    //==========================================================================
    // Per-Input LFO State
    //==========================================================================
    struct LFOState
    {
        float ramp = 0.0f;              // Main ramp 0→1
        float fadeLevel = 0.0f;         // 0→1 for 500ms fade in/out
        bool wasActive = false;         // Previous active state for fade detection

        // Random shape state - per axis
        float randomTargetX = 0.0f;
        float randomTargetY = 0.0f;
        float randomTargetZ = 0.0f;
        float lastRandomX = 0.0f;
        float lastRandomY = 0.0f;
        float lastRandomZ = 0.0f;

        // Per-axis ramp tracking for independent random generation
        float prevRampX = 0.0f;
        float prevRampY = 0.0f;
        float prevRampZ = 0.0f;

        // Cached output values for UI display
        float normalizedX = 0.0f;       // -1 to +1 for UI slider
        float normalizedY = 0.0f;
        float normalizedZ = 0.0f;

        // Final offset values in meters
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        float offsetZ = 0.0f;

        // Gyrophone rotation offset in radians (added to input rotation for HF directivity)
        float gyrophoneOffsetRad = 0.0f;
    };

    //==========================================================================
    // Construction
    //==========================================================================
    explicit LFOProcessor (WFSValueTreeState& state, int numInputs = 64)
        : valueTreeState (state), numInputChannels (numInputs)
    {
        states.resize (static_cast<size_t> (numInputs));
    }

    //==========================================================================
    // Processing
    //==========================================================================

    /**
     * Process all LFOs - called at 50Hz (every 20ms)
     * @param deltaTimeSeconds Time since last call (typically 0.02f)
     */
    void process (float deltaTimeSeconds)
    {
        constexpr float fadeTimeSeconds = 0.5f;  // 500ms fade
        const float fadeIncrement = deltaTimeSeconds / fadeTimeSeconds;

        for (int i = 0; i < numInputChannels; ++i)
        {
            processInput (i, deltaTimeSeconds, fadeIncrement);
        }
    }

    //==========================================================================
    // Output Accessors
    //==========================================================================

    /** Get current LFO offset in meters for an input */
    float getOffsetX (int inputIndex) const
    {
        if (inputIndex < 0 || inputIndex >= numInputChannels)
            return 0.0f;
        return states[static_cast<size_t> (inputIndex)].offsetX;
    }

    float getOffsetY (int inputIndex) const
    {
        if (inputIndex < 0 || inputIndex >= numInputChannels)
            return 0.0f;
        return states[static_cast<size_t> (inputIndex)].offsetY;
    }

    float getOffsetZ (int inputIndex) const
    {
        if (inputIndex < 0 || inputIndex >= numInputChannels)
            return 0.0f;
        return states[static_cast<size_t> (inputIndex)].offsetZ;
    }

    /** Get normalized output (-1 to +1) for UI display */
    float getNormalizedX (int inputIndex) const
    {
        if (inputIndex < 0 || inputIndex >= numInputChannels)
            return 0.0f;
        return states[static_cast<size_t> (inputIndex)].normalizedX;
    }

    float getNormalizedY (int inputIndex) const
    {
        if (inputIndex < 0 || inputIndex >= numInputChannels)
            return 0.0f;
        return states[static_cast<size_t> (inputIndex)].normalizedY;
    }

    float getNormalizedZ (int inputIndex) const
    {
        if (inputIndex < 0 || inputIndex >= numInputChannels)
            return 0.0f;
        return states[static_cast<size_t> (inputIndex)].normalizedZ;
    }

    /** Get gyrophone rotation offset in radians (for HF directivity modulation) */
    float getGyrophoneOffsetRad (int inputIndex) const
    {
        if (inputIndex < 0 || inputIndex >= numInputChannels)
            return 0.0f;
        return states[static_cast<size_t> (inputIndex)].gyrophoneOffsetRad;
    }

    /** Get ramp progress (0→1) for progress indicator */
    float getRampProgress (int inputIndex) const
    {
        if (inputIndex < 0 || inputIndex >= numInputChannels)
            return 0.0f;
        return states[static_cast<size_t> (inputIndex)].ramp;
    }

    /** Check if LFO is active for an input */
    bool isActive (int inputIndex) const
    {
        if (inputIndex < 0 || inputIndex >= numInputChannels)
            return false;
        auto lfoSection = valueTreeState.getInputLFOSection (inputIndex);
        return static_cast<int> (lfoSection.getProperty (WFSParameterIDs::inputLFOactive, 0)) != 0;
    }

private:
    //==========================================================================
    // Per-Input Processing
    //==========================================================================
    void processInput (int inputIndex, float deltaTime, float fadeIncrement)
    {
        auto& state = states[static_cast<size_t> (inputIndex)];
        auto lfoSection = valueTreeState.getInputLFOSection (inputIndex);

        // Read parameters
        bool isActive = static_cast<int> (lfoSection.getProperty (WFSParameterIDs::inputLFOactive, 0)) != 0;
        float period = static_cast<float> (lfoSection.getProperty (WFSParameterIDs::inputLFOperiod, 5.0f));
        int globalPhase = static_cast<int> (lfoSection.getProperty (WFSParameterIDs::inputLFOphase, 0));

        int shapeX = static_cast<int> (lfoSection.getProperty (WFSParameterIDs::inputLFOshapeX, 0));
        int shapeY = static_cast<int> (lfoSection.getProperty (WFSParameterIDs::inputLFOshapeY, 0));
        int shapeZ = static_cast<int> (lfoSection.getProperty (WFSParameterIDs::inputLFOshapeZ, 0));

        float rateX = static_cast<float> (lfoSection.getProperty (WFSParameterIDs::inputLFOrateX, 1.0f));
        float rateY = static_cast<float> (lfoSection.getProperty (WFSParameterIDs::inputLFOrateY, 1.0f));
        float rateZ = static_cast<float> (lfoSection.getProperty (WFSParameterIDs::inputLFOrateZ, 1.0f));

        float amplitudeX = static_cast<float> (lfoSection.getProperty (WFSParameterIDs::inputLFOamplitudeX, 1.0f));
        float amplitudeY = static_cast<float> (lfoSection.getProperty (WFSParameterIDs::inputLFOamplitudeY, 1.0f));
        float amplitudeZ = static_cast<float> (lfoSection.getProperty (WFSParameterIDs::inputLFOamplitudeZ, 1.0f));

        int phaseX = static_cast<int> (lfoSection.getProperty (WFSParameterIDs::inputLFOphaseX, 0));
        int phaseY = static_cast<int> (lfoSection.getProperty (WFSParameterIDs::inputLFOphaseY, 0));
        int phaseZ = static_cast<int> (lfoSection.getProperty (WFSParameterIDs::inputLFOphaseZ, 0));

        // Gyrophone: -1 = Anti-Clockwise, 0 = OFF, 1 = Clockwise
        int gyrophone = static_cast<int> (lfoSection.getProperty (WFSParameterIDs::inputLFOgyrophone, 0));

        // Update fade level (500ms fade in/out)
        if (isActive && state.fadeLevel < 1.0f)
        {
            state.fadeLevel = juce::jmin (1.0f, state.fadeLevel + fadeIncrement);
        }
        else if (!isActive && state.fadeLevel > 0.0f)
        {
            state.fadeLevel = juce::jmax (0.0f, state.fadeLevel - fadeIncrement);
        }

        // Update ramp (continues even during fade out for smooth transition)
        if (state.fadeLevel > 0.0f || isActive)
        {
            float rampIncrement = deltaTime / juce::jmax (0.01f, period);
            state.ramp += rampIncrement;

            // Handle main ramp wrap
            if (state.ramp >= 1.0f)
                state.ramp = std::fmod (state.ramp, 1.0f);

            // Calculate phase-adjusted ramps for each axis
            float totalPhaseX = (globalPhase + phaseX) / 360.0f;
            float totalPhaseY = (globalPhase + phaseY) / 360.0f;
            float totalPhaseZ = (globalPhase + phaseZ) / 360.0f;

            float rampX = std::fmod (state.ramp * rateX + totalPhaseX + 10.0f, 1.0f);
            float rampY = std::fmod (state.ramp * rateY + totalPhaseY + 10.0f, 1.0f);
            float rampZ = std::fmod (state.ramp * rateZ + totalPhaseZ + 10.0f, 1.0f);

            // Generate new random targets when each axis's ramp wraps independently
            // Wrap detected when current ramp < previous ramp (went from ~1.0 back to ~0.0)
            if (shapeX == Random && rampX < state.prevRampX)
            {
                state.lastRandomX = state.randomTargetX;
                state.randomTargetX = random.nextFloat() * 2.0f - 1.0f;
            }
            if (shapeY == Random && rampY < state.prevRampY)
            {
                state.lastRandomY = state.randomTargetY;
                state.randomTargetY = random.nextFloat() * 2.0f - 1.0f;
            }
            if (shapeZ == Random && rampZ < state.prevRampZ)
            {
                state.lastRandomZ = state.randomTargetZ;
                state.randomTargetZ = random.nextFloat() * 2.0f - 1.0f;
            }

            // Store current ramps for next frame's wrap detection
            state.prevRampX = rampX;
            state.prevRampY = rampY;
            state.prevRampZ = rampZ;

            // Calculate waveform outputs (-1 to +1)
            state.normalizedX = applyWaveform (shapeX, rampX, state.lastRandomX, state.randomTargetX);
            state.normalizedY = applyWaveform (shapeY, rampY, state.lastRandomY, state.randomTargetY);
            state.normalizedZ = applyWaveform (shapeZ, rampZ, state.lastRandomZ, state.randomTargetZ);

            // Apply amplitude and fade to get final offsets
            state.offsetX = state.normalizedX * amplitudeX * state.fadeLevel;
            state.offsetY = state.normalizedY * amplitudeY * state.fadeLevel;
            state.offsetZ = state.normalizedZ * amplitudeZ * state.fadeLevel;

            // Gyrophone: rotate brightness cone based on main ramp
            // Uses main ramp (not per-axis) so rotation completes one full cycle per period
            // gyrophone: -1 = anti-clockwise, 0 = off, 1 = clockwise
            if (gyrophone != 0)
            {
                // Full rotation (2π) over one period, direction based on gyrophone sign
                state.gyrophoneOffsetRad = static_cast<float> (gyrophone) * state.ramp
                                           * juce::MathConstants<float>::twoPi * state.fadeLevel;
            }
            else
            {
                state.gyrophoneOffsetRad = 0.0f;
            }
        }
        else
        {
            // Fully faded out - reset offsets
            state.offsetX = 0.0f;
            state.offsetY = 0.0f;
            state.offsetZ = 0.0f;
            state.gyrophoneOffsetRad = 0.0f;
            state.normalizedX = 0.0f;
            state.normalizedY = 0.0f;
            state.normalizedZ = 0.0f;
        }

        state.wasActive = isActive;
    }

    //==========================================================================
    // Waveform Generation
    //==========================================================================

    /**
     * Apply waveform shape to ramp value
     * @param shape Waveform shape (0-8)
     * @param ramp Normalized ramp value (0→1)
     * @param lastRandom Previous random target (for ramping)
     * @param targetRandom Current random target
     * @return Output value (-1 to +1)
     */
    float applyWaveform (int shape, float ramp, float lastRandom, float targetRandom) const
    {
        switch (shape)
        {
            case Off:
                return 0.0f;

            case Sine:
                // -cos(2π * r) gives sine starting at -1, going to +1 at 0.5, back to -1
                return -std::cos (juce::MathConstants<float>::twoPi * ramp);

            case Square:
                // Jump between -1 and +1 at midpoint
                return ramp < 0.5f ? -1.0f : 1.0f;

            case Sawtooth:
                // Ramp from -1 to +1
                return 2.0f * ramp - 1.0f;

            case Triangle:
                // Ramp up to +1 then down to -1
                return ramp < 0.5f
                           ? 4.0f * ramp - 1.0f
                           : 3.0f - 4.0f * ramp;

            case Keystone:
                // Plateau at ends, ramp in middle (0.25 threshold)
                // 0.00-0.25: hold at -1
                // 0.25-0.50: ramp from -1 to +1
                // 0.50-0.75: hold at +1
                // 0.75-1.00: ramp from +1 to -1
                if (ramp < 0.25f)
                    return -1.0f;
                else if (ramp < 0.5f)
                    return (ramp - 0.25f) * 8.0f - 1.0f;
                else if (ramp < 0.75f)
                    return 1.0f;
                else
                    return 1.0f - (ramp - 0.75f) * 8.0f;

            case Log:
                // Logarithmic curve: 2*log10(20*r + 1) - 1, normalized
                // At r=0: 2*log10(1) - 1 = -1
                // At r=1: 2*log10(21) - 1 ≈ 1.64
                // Normalize to -1 to +1 range
                {
                    float logVal = 2.0f * std::log10 (20.0f * ramp + 1.0f) - 1.0f;
                    // Normalize: at r=1, logVal ≈ 1.644
                    return juce::jmap (logVal, -1.0f, 1.644f, -1.0f, 1.0f);
                }

            case Exp:
                // Exponential curve using pow(3, r*2)
                // Normalized to -1 to +1 range
                {
                    float expVal = std::pow (3.0f, ramp * 2.0f);
                    // At r=0: pow(3,0) = 1
                    // At r=1: pow(3,2) = 9
                    // Map [1, 9] to [-1, +1]
                    return juce::jmap (expVal, 1.0f, 9.0f, -1.0f, 1.0f);
                }

            case Random:
                // Smoothly ramp from last random target to current random target
                return lastRandom + (targetRandom - lastRandom) * ramp;

            default:
                return 0.0f;
        }
    }

    //==========================================================================
    // Member Variables
    //==========================================================================
    WFSValueTreeState& valueTreeState;
    int numInputChannels;
    std::vector<LFOState> states;
    juce::Random random;
};
