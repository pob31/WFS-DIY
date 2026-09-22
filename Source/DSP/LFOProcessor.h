#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSValueTreeState.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../../spatcore/dsp/LFOWaveforms.h"
#include <functional>

/**
    Which family an LFOProcessor animates: the <LFO> node of a slot and the
    identifiers on it. The waveform engine is the same for every family; only
    the tree it reads differs - the twin of AutomOtionFamily.

    Gyrophone is optional. An input has a brightness cone to rotate; an effect
    return is an omnidirectional render source and has no such property, so
    its family leaves the identifier null and the processor publishes 0 rad.
*/
struct LFOFamily
{
    std::function<juce::ValueTree (int slot)> section;
    int numSlots = 0;

    juce::Identifier active, period, phase;
    juce::Identifier shapeX, shapeY, shapeZ;
    juce::Identifier rateX, rateY, rateZ;
    juce::Identifier amplitudeX, amplitudeY, amplitudeZ;
    juce::Identifier phaseX, phaseY, phaseZ;
    juce::Identifier gyrophone;      // null for a family without one

    static LFOFamily inputs (WFSValueTreeState& state, int numInputs)
    {
        namespace P = WFSParameterIDs;
        LFOFamily f;
        f.section  = [&state] (int slot) { return state.getInputLFOSection (slot); };
        f.numSlots = numInputs;
        f.active = P::inputLFOactive;  f.period = P::inputLFOperiod;  f.phase = P::inputLFOphase;
        f.shapeX = P::inputLFOshapeX;  f.shapeY = P::inputLFOshapeY;  f.shapeZ = P::inputLFOshapeZ;
        f.rateX = P::inputLFOrateX;    f.rateY = P::inputLFOrateY;    f.rateZ = P::inputLFOrateZ;
        f.amplitudeX = P::inputLFOamplitudeX; f.amplitudeY = P::inputLFOamplitudeY; f.amplitudeZ = P::inputLFOamplitudeZ;
        f.phaseX = P::inputLFOphaseX;  f.phaseY = P::inputLFOphaseY;  f.phaseZ = P::inputLFOphaseZ;
        f.gyrophone = P::inputLFOgyrophone;
        return f;
    }

    /** The effect returns: the same waveforms over effectLFO*, no gyrophone,
        one slot per possible effect (the budget, as AutomOtionFamily::effects). */
    static LFOFamily effects (WFSValueTreeState& state)
    {
        namespace P = WFSParameterIDs;
        LFOFamily f;
        f.section  = [&state] (int slot) { return state.getEffectLFOSection (slot); };
        f.numSlots = WFSParameterDefaults::maxEffectChannels;
        f.active = P::effectLFOactive;  f.period = P::effectLFOperiod;  f.phase = P::effectLFOphase;
        f.shapeX = P::effectLFOshapeX;  f.shapeY = P::effectLFOshapeY;  f.shapeZ = P::effectLFOshapeZ;
        f.rateX = P::effectLFOrateX;    f.rateY = P::effectLFOrateY;    f.rateZ = P::effectLFOrateZ;
        f.amplitudeX = P::effectLFOamplitudeX; f.amplitudeY = P::effectLFOamplitudeY; f.amplitudeZ = P::effectLFOamplitudeZ;
        f.phaseX = P::effectLFOphaseX;  f.phaseY = P::effectLFOphaseY;  f.phaseZ = P::effectLFOphaseZ;
        return f;
    }
};

/**
 * LFO Processor for WFS Input Position Modulation
 *
 * Generates periodic position offsets for each input channel based on LFO parameters.
 * Called at 50Hz from the MainComponent timer callback.
 *
 * Each input has independent LFO state with:
 * - Main ramp (0→1) that cycles at the period rate
 * - Per-axis waveform shape, rate multiplier, amplitude, and phase
 * - 500ms global fade in/out when activating/deactivating LFO
 * - 1-second per-axis fade when shape changes from OFF to any other (prevents brisk changes)
 * - Random shape picks new target at period boundary
 */
class LFOProcessor
{
public:
    // Waveform shapes — delegated to LFOWaveforms shared utility
    static constexpr int Off      = LFOWaveforms::Off;
    static constexpr int Random   = LFOWaveforms::Random;

    //==========================================================================
    // Per-Input LFO State
    //==========================================================================
    struct LFOState
    {
        float ramp = 0.0f;              // Main ramp 0→1 (kept for gyrophone + progress UI)
        float rampX = 0.0f;             // Independent X-axis ramp 0→1
        float rampY = 0.0f;             // Independent Y-axis ramp 0→1
        float rampZ = 0.0f;             // Independent Z-axis ramp 0→1
        float fadeLevel = 0.0f;         // 0→1 for 500ms fade in/out
        bool wasActive = false;         // Previous active state for fade detection

        // Per-axis amplitude fade (0→1, 1-second ramp when shape changes from OFF)
        float fadeX = 0.0f;
        float fadeY = 0.0f;
        float fadeZ = 0.0f;

        // Previous shape values for detecting OFF→ON transitions
        int prevShapeX = 0;
        int prevShapeY = 0;
        int prevShapeZ = 0;

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
        : LFOProcessor (LFOFamily::inputs (state, numInputs))
    {
    }

    /** Any family: the effects twin is LFOProcessor (LFOFamily::effects (vts)). */
    explicit LFOProcessor (LFOFamily familyToUse)
        : family (std::move (familyToUse)), numInputChannels (family.numSlots)
    {
        states.resize (static_cast<size_t> (numInputChannels));
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
        constexpr float fadeTimeSeconds = 0.5f;      // 500ms global fade
        constexpr float axisFadeTimeSeconds = 1.0f;  // 1-second per-axis fade
        const float fadeIncrement = deltaTimeSeconds / fadeTimeSeconds;
        const float axisFadeIncrement = deltaTimeSeconds / axisFadeTimeSeconds;

        for (int i = 0; i < numInputChannels; ++i)
        {
            processInput (i, deltaTimeSeconds, fadeIncrement, axisFadeIncrement);
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
        auto lfoSection = family.section (inputIndex);
        return static_cast<int> (lfoSection.getProperty (family.active, 0)) != 0;
    }

private:
    //==========================================================================
    // Per-Input Processing
    //==========================================================================
    void processInput (int inputIndex, float deltaTime, float fadeIncrement, float axisFadeIncrement)
    {
        auto& state = states[static_cast<size_t> (inputIndex)];
        auto lfoSection = family.section (inputIndex);

        // Read parameters
        bool isActive = static_cast<int> (lfoSection.getProperty (family.active, 0)) != 0;
        float period = static_cast<float> (lfoSection.getProperty (family.period, 5.0f));
        int globalPhase = static_cast<int> (lfoSection.getProperty (family.phase, 0));

        int shapeX = static_cast<int> (lfoSection.getProperty (family.shapeX, 0));
        int shapeY = static_cast<int> (lfoSection.getProperty (family.shapeY, 0));
        int shapeZ = static_cast<int> (lfoSection.getProperty (family.shapeZ, 0));

        float rateX = static_cast<float> (lfoSection.getProperty (family.rateX, 1.0f));
        float rateY = static_cast<float> (lfoSection.getProperty (family.rateY, 1.0f));
        float rateZ = static_cast<float> (lfoSection.getProperty (family.rateZ, 1.0f));

        float amplitudeX = static_cast<float> (lfoSection.getProperty (family.amplitudeX, 1.0f));
        float amplitudeY = static_cast<float> (lfoSection.getProperty (family.amplitudeY, 1.0f));
        float amplitudeZ = static_cast<float> (lfoSection.getProperty (family.amplitudeZ, 1.0f));

        int phaseX = static_cast<int> (lfoSection.getProperty (family.phaseX, 0));
        int phaseY = static_cast<int> (lfoSection.getProperty (family.phaseY, 0));
        int phaseZ = static_cast<int> (lfoSection.getProperty (family.phaseZ, 0));

        // Gyrophone: -1 = Anti-Clockwise, 0 = OFF, 1 = Clockwise; a family
        // without one (the effect returns) reads OFF
        int gyrophone = family.gyrophone.isNull()
                          ? 0
                          : static_cast<int> (lfoSection.getProperty (family.gyrophone, 0));

        // Update fade level (500ms fade in/out)
        if (isActive && state.fadeLevel < 1.0f)
        {
            state.fadeLevel = juce::jmin (1.0f, state.fadeLevel + fadeIncrement);
        }
        else if (!isActive && state.fadeLevel > 0.0f)
        {
            state.fadeLevel = juce::jmax (0.0f, state.fadeLevel - fadeIncrement);
        }

        // Per-axis amplitude fade (1-second ramp when shape changes from OFF to any other)
        // Reset fade when shape transitions from OFF (0) to non-OFF, or when LFO just became active
        bool justActivated = isActive && !state.wasActive;

        // Reset all ramps to phase 0 on enable so clocks start synchronized
        if (justActivated)
        {
            state.ramp  = 0.0f;
            state.rampX = 0.0f;
            state.rampY = 0.0f;
            state.rampZ = 0.0f;
        }

        if ((state.prevShapeX == Off && shapeX != Off) || (justActivated && shapeX != Off && state.fadeX >= 1.0f))
            state.fadeX = 0.0f;
        if ((state.prevShapeY == Off && shapeY != Off) || (justActivated && shapeY != Off && state.fadeY >= 1.0f))
            state.fadeY = 0.0f;
        if ((state.prevShapeZ == Off && shapeZ != Off) || (justActivated && shapeZ != Off && state.fadeZ >= 1.0f))
            state.fadeZ = 0.0f;

        // Ramp up per-axis fade when shape is active, ramp down when OFF
        if (shapeX != Off && state.fadeX < 1.0f)
            state.fadeX = juce::jmin (1.0f, state.fadeX + axisFadeIncrement);
        else if (shapeX == Off && state.fadeX > 0.0f)
            state.fadeX = juce::jmax (0.0f, state.fadeX - axisFadeIncrement);

        if (shapeY != Off && state.fadeY < 1.0f)
            state.fadeY = juce::jmin (1.0f, state.fadeY + axisFadeIncrement);
        else if (shapeY == Off && state.fadeY > 0.0f)
            state.fadeY = juce::jmax (0.0f, state.fadeY - axisFadeIncrement);

        if (shapeZ != Off && state.fadeZ < 1.0f)
            state.fadeZ = juce::jmin (1.0f, state.fadeZ + axisFadeIncrement);
        else if (shapeZ == Off && state.fadeZ > 0.0f)
            state.fadeZ = juce::jmax (0.0f, state.fadeZ - axisFadeIncrement);

        // Store current shapes for next frame's transition detection
        state.prevShapeX = shapeX;
        state.prevShapeY = shapeY;
        state.prevShapeZ = shapeZ;

        // Update ramp (continues even during fade out for smooth transition)
        if (state.fadeLevel > 0.0f || isActive)
        {
            float rampIncrement = deltaTime / juce::jmax (0.01f, period);

            // Main ramp kept for gyrophone + progress UI
            state.ramp += rampIncrement;
            if (state.ramp >= 1.0f)
                state.ramp = std::fmod (state.ramp, 1.0f);

            // Independent per-axis ramps — each advances at its own rate
            state.rampX += rampIncrement * rateX;
            if (state.rampX >= 1.0f) state.rampX = std::fmod (state.rampX, 1.0f);

            state.rampY += rampIncrement * rateY;
            if (state.rampY >= 1.0f) state.rampY = std::fmod (state.rampY, 1.0f);

            state.rampZ += rampIncrement * rateZ;
            if (state.rampZ >= 1.0f) state.rampZ = std::fmod (state.rampZ, 1.0f);

            // Phase-adjusted ramps for waveform generation
            float totalPhaseX = (globalPhase + phaseX) / 360.0f;
            float totalPhaseY = (globalPhase + phaseY) / 360.0f;
            float totalPhaseZ = (globalPhase + phaseZ) / 360.0f;

            float rampX = std::fmod (state.rampX + totalPhaseX + 10.0f, 1.0f);
            float rampY = std::fmod (state.rampY + totalPhaseY + 10.0f, 1.0f);
            float rampZ = std::fmod (state.rampZ + totalPhaseZ + 10.0f, 1.0f);

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

            // Apply amplitude, global fade, and per-axis fade to get final offsets
            state.offsetX = state.normalizedX * amplitudeX * state.fadeLevel * state.fadeX;
            state.offsetY = state.normalizedY * amplitudeY * state.fadeLevel * state.fadeY;
            state.offsetZ = state.normalizedZ * amplitudeZ * state.fadeLevel * state.fadeZ;

            // Gyrophone: rotate brightness cone based on main ramp
            // Uses main ramp (not per-axis) so rotation completes one full cycle per period
            // gyrophone: -1 = anti-clockwise, 0 = off, 1 = clockwise
            if (gyrophone != 0)
            {
                // Full rotation (2π) over one period, direction based on gyrophone sign
                // Negated so positive (clockwise) rotates in positive angular direction
                state.gyrophoneOffsetRad = static_cast<float> (-gyrophone) * state.ramp
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
    // Waveform Generation (delegated to shared LFOWaveforms utility)
    //==========================================================================
    float applyWaveform (int shape, float ramp, float lastRandom, float targetRandom) const
    {
        return LFOWaveforms::applyWaveform (shape, ramp, lastRandom, targetRandom);
    }

    //==========================================================================
    // Member Variables
    //==========================================================================
    LFOFamily family;
    int numInputChannels;
    std::vector<LFOState> states;
    juce::Random random;
};
