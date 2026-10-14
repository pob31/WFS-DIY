#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSValueTreeState.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../../spatcore/dsp/LFOWaveforms.h"
#include "LFOAxis.h"
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
 * - 1-second per-axis fade in when a shape is chosen, and out at OFF
 * - no step in the output for any change (LFOAxis: re-audit F6)
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
        float fadeLevel = 0.0f;         // 0→1 for 500ms fade in/out
        bool wasActive = false;         // Previous active state for fade detection

        // Per-axis ramp, fade, Random targets and continuity (LFOAxis)
        LFOAxis x, y, z;

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
        auto lfoSection = family.section (inputIndex);
        return static_cast<int> (lfoSection.getProperty (family.active, 0)) != 0;
    }

private:
    //==========================================================================
    // Per-Input Processing
    //==========================================================================
    void processInput (int inputIndex, float deltaTime, float fadeIncrement)
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
        const bool wasSilent = state.fadeLevel <= 0.0f;
        if (isActive && state.fadeLevel < 1.0f)
        {
            state.fadeLevel = juce::jmin (1.0f, state.fadeLevel + fadeIncrement);
        }
        else if (!isActive && state.fadeLevel > 0.0f)
        {
            state.fadeLevel = juce::jmax (0.0f, state.fadeLevel - fadeIncrement);
        }

        // Reset all ramps to phase 0 on enable so clocks start synchronized -
        // from silence only: switched back on during its fade-out, the LFO
        // carries on where it is (the reset jumped the phase, re-audit F6)
        if (isActive && !state.wasActive && wasSilent)
        {
            state.ramp = 0.0f;
            state.x.restart (shapeX);
            state.y.restart (shapeY);
            state.z.restart (shapeZ);
        }

        // Per-axis fade: 1 s in while a shape is chosen, 1 s out at OFF
        state.x.updateFade (shapeX, deltaTime);
        state.y.updateFade (shapeY, deltaTime);
        state.z.updateFade (shapeZ, deltaTime);

        const float phaseCyclesX = (globalPhase + phaseX) / 360.0f;
        const float phaseCyclesY = (globalPhase + phaseY) / 360.0f;
        const float phaseCyclesZ = (globalPhase + phaseZ) / 360.0f;

        // Update ramp (continues even during fade out for smooth transition,
        // and while a step is still being released)
        if (state.fadeLevel > 0.0f || isActive
            || state.x.bridging() || state.y.bridging() || state.z.bridging())
        {
            float rampIncrement = deltaTime / juce::jmax (0.01f, period);

            // Main ramp kept for gyrophone + progress UI
            state.ramp += rampIncrement;
            if (state.ramp >= 1.0f)
                state.ramp = std::fmod (state.ramp, 1.0f);

            // Independent per-axis ramps — each advances at its own rate
            state.x.advance (rampIncrement * rateX);
            state.y.advance (rampIncrement * rateY);
            state.z.advance (rampIncrement * rateZ);

            // Waveform, amplitude, global fade and per-axis fade, continuous
            state.offsetX = state.x.evaluate (shapeX, amplitudeX, phaseCyclesX, state.fadeLevel, deltaTime, random);
            state.offsetY = state.y.evaluate (shapeY, amplitudeY, phaseCyclesY, state.fadeLevel, deltaTime, random);
            state.offsetZ = state.z.evaluate (shapeZ, amplitudeZ, phaseCyclesZ, state.fadeLevel, deltaTime, random);
            state.normalizedX = state.x.normalized;
            state.normalizedY = state.y.normalized;
            state.normalizedZ = state.z.normalized;

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
            state.x.rest (shapeX, amplitudeX, phaseCyclesX);
            state.y.rest (shapeY, amplitudeY, phaseCyclesY);
            state.z.rest (shapeZ, amplitudeZ, phaseCyclesZ);
        }

        state.wasActive = isActive;
    }

    //==========================================================================
    // Member Variables
    //==========================================================================
    LFOFamily family;
    int numInputChannels;
    std::vector<LFOState> states;
    juce::Random random;
};
