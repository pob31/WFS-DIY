#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSValueTreeState.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "LFOWaveforms.h"

/**
 * Cluster LFO Processor
 *
 * Generates periodic position, rotation, and scale offsets for each cluster.
 * Called at 50Hz from the ClustersTab timer callback.
 *
 * Each cluster has independent LFO state with 5 axes:
 * - X, Y, Z: position offsets in meters
 * - Rotation: angle offset in degrees (XY plane only)
 * - Scale: uniform scale factor (centered at 1.0)
 *
 * Outputs are delta-based: each tick reports the change from the previous tick,
 * so the caller can apply incremental transforms. This enables transient offsets
 * that smoothly fade back to zero when the LFO is deactivated.
 */
class ClusterLFOProcessor
{
public:
    static constexpr int maxClusters = 10;

    //==========================================================================
    // Per-Cluster LFO State
    //==========================================================================
    struct ClusterLFOState
    {
        float ramp = 0.0f;              // Main ramp 0->1 (for progress UI)

        // Per-axis ramps
        float rampX = 0.0f;
        float rampY = 0.0f;
        float rampZ = 0.0f;
        float rampRot = 0.0f;
        float rampScale = 0.0f;

        float fadeLevel = 0.0f;         // 0->1 for 500ms fade in/out
        bool wasActive = false;

        // Per-axis amplitude fades (1-second ramp when shape changes from OFF)
        float fadeX = 0.0f;
        float fadeY = 0.0f;
        float fadeZ = 0.0f;
        float fadeRot = 0.0f;
        float fadeScale = 0.0f;

        // Previous shape values for detecting OFF->ON transitions
        int prevShapeX = 0;
        int prevShapeY = 0;
        int prevShapeZ = 0;
        int prevShapeRot = 0;
        int prevShapeScale = 0;

        // Random shape state — per axis
        float randomTargetX = 0.0f, lastRandomX = 0.0f;
        float randomTargetY = 0.0f, lastRandomY = 0.0f;
        float randomTargetZ = 0.0f, lastRandomZ = 0.0f;
        float randomTargetRot = 0.0f, lastRandomRot = 0.0f;
        float randomTargetScale = 0.0f, lastRandomScale = 0.0f;

        // Per-axis ramp tracking for random wrap detection
        float prevRampX = 0.0f;
        float prevRampY = 0.0f;
        float prevRampZ = 0.0f;
        float prevRampRot = 0.0f;
        float prevRampScale = 0.0f;

        // Cached normalized outputs (-1 to +1) for UI display
        float normalizedX = 0.0f;
        float normalizedY = 0.0f;
        float normalizedZ = 0.0f;
        float normalizedRot = 0.0f;
        float normalizedScale = 0.0f;

        // Current absolute offsets (for delta computation)
        float offsetX = 0.0f;           // meters
        float offsetY = 0.0f;
        float offsetZ = 0.0f;
        float offsetRotDeg = 0.0f;      // degrees
        float offsetScale = 1.0f;       // multiplier (1.0 = no change)

        // Previous offsets (for computing deltas)
        float prevOffsetX = 0.0f;
        float prevOffsetY = 0.0f;
        float prevOffsetZ = 0.0f;
        float prevOffsetRotDeg = 0.0f;
        float prevOffsetScale = 1.0f;

        // Deltas computed this tick
        float deltaX = 0.0f;
        float deltaY = 0.0f;
        float deltaZ = 0.0f;
        float deltaRotDeg = 0.0f;
        float deltaScale = 1.0f;        // multiplier (1.0 = no change)
    };

    //==========================================================================
    // Construction
    //==========================================================================
    explicit ClusterLFOProcessor (WFSValueTreeState& state)
        : valueTreeState (state)
    {
        states.resize (maxClusters);
    }

    //==========================================================================
    // Processing
    //==========================================================================

    /**
     * Process all cluster LFOs — called at 50Hz (every 20ms)
     * @param deltaTimeSeconds Time since last call (typically 0.02f)
     */
    void process (float deltaTimeSeconds)
    {
        constexpr float fadeTimeSeconds = 0.5f;
        constexpr float axisFadeTimeSeconds = 1.0f;
        const float fadeIncrement = deltaTimeSeconds / fadeTimeSeconds;
        const float axisFadeIncrement = deltaTimeSeconds / axisFadeTimeSeconds;

        for (int c = 0; c < maxClusters; ++c)
        {
            processCluster (c, deltaTimeSeconds, fadeIncrement, axisFadeIncrement);
        }
    }

    //==========================================================================
    // Output Accessors (1-based cluster index)
    //==========================================================================

    /** Get position delta this tick (meters) */
    float getDeltaX (int clusterIndex) const { return getState (clusterIndex).deltaX; }
    float getDeltaY (int clusterIndex) const { return getState (clusterIndex).deltaY; }
    float getDeltaZ (int clusterIndex) const { return getState (clusterIndex).deltaZ; }

    /** Get rotation delta this tick (degrees, XY plane) */
    float getDeltaRotDeg (int clusterIndex) const { return getState (clusterIndex).deltaRotDeg; }

    /** Get scale delta this tick (multiplier, 1.0 = no change) */
    float getDeltaScale (int clusterIndex) const { return getState (clusterIndex).deltaScale; }

    /** Get normalized output (-1 to +1) for UI indicators */
    float getNormalizedX (int clusterIndex) const { return getState (clusterIndex).normalizedX; }
    float getNormalizedY (int clusterIndex) const { return getState (clusterIndex).normalizedY; }
    float getNormalizedZ (int clusterIndex) const { return getState (clusterIndex).normalizedZ; }
    float getNormalizedRot (int clusterIndex) const { return getState (clusterIndex).normalizedRot; }
    float getNormalizedScale (int clusterIndex) const { return getState (clusterIndex).normalizedScale; }

    /** Get ramp progress (0->1) for progress indicator */
    float getRampProgress (int clusterIndex) const { return getState (clusterIndex).ramp; }

    /** Check if LFO is active (or fading) for a cluster */
    bool isActive (int clusterIndex) const
    {
        auto& s = getState (clusterIndex);
        return s.wasActive || s.fadeLevel > 0.0f;
    }

private:
    //==========================================================================
    // State Access
    //==========================================================================
    static constexpr ClusterLFOState emptyState {};

    const ClusterLFOState& getState (int clusterIndex) const
    {
        int idx = clusterIndex - 1;
        if (idx >= 0 && idx < maxClusters)
            return states[static_cast<size_t> (idx)];
        return emptyState;
    }

    //==========================================================================
    // Per-Cluster Processing
    //==========================================================================
    void processCluster (int idx, float deltaTime, float fadeIncrement, float axisFadeIncrement)
    {
        using namespace WFSParameterIDs;
        using namespace WFSParameterDefaults;

        auto& state = states[static_cast<size_t> (idx)];
        auto lfoSection = valueTreeState.getClusterLFOSection (idx + 1);  // 1-based

        if (! lfoSection.isValid())
        {
            state.deltaX = state.deltaY = state.deltaZ = state.deltaRotDeg = 0.0f;
            state.deltaScale = 1.0f;
            return;
        }

        // Read parameters
        bool active = static_cast<int> (lfoSection.getProperty (clusterLFOactive, 0)) != 0;
        float period = static_cast<float> (lfoSection.getProperty (clusterLFOperiod, clusterLFOperiodDefault));
        int globalPhase = static_cast<int> (lfoSection.getProperty (clusterLFOphase, 0));

        int shapeX     = static_cast<int> (lfoSection.getProperty (clusterLFOshapeX, 0));
        int shapeY     = static_cast<int> (lfoSection.getProperty (clusterLFOshapeY, 0));
        int shapeZ     = static_cast<int> (lfoSection.getProperty (clusterLFOshapeZ, 0));
        int shapeRot   = static_cast<int> (lfoSection.getProperty (clusterLFOshapeRot, 0));
        int shapeScale = static_cast<int> (lfoSection.getProperty (clusterLFOshapeScale, 0));

        float rateX     = static_cast<float> (lfoSection.getProperty (clusterLFOrateX, clusterLFOrateDefault));
        float rateY     = static_cast<float> (lfoSection.getProperty (clusterLFOrateY, clusterLFOrateDefault));
        float rateZ     = static_cast<float> (lfoSection.getProperty (clusterLFOrateZ, clusterLFOrateDefault));
        float rateRot   = static_cast<float> (lfoSection.getProperty (clusterLFOrateRot, clusterLFOrateDefault));
        float rateScale = static_cast<float> (lfoSection.getProperty (clusterLFOrateScale, clusterLFOrateDefault));

        float ampX     = static_cast<float> (lfoSection.getProperty (clusterLFOamplitudeX, clusterLFOamplitudeXYZDefault));
        float ampY     = static_cast<float> (lfoSection.getProperty (clusterLFOamplitudeY, clusterLFOamplitudeXYZDefault));
        float ampZ     = static_cast<float> (lfoSection.getProperty (clusterLFOamplitudeZ, clusterLFOamplitudeXYZDefault));
        float ampRot   = static_cast<float> (static_cast<int> (lfoSection.getProperty (clusterLFOamplitudeRot, clusterLFOamplitudeRotDefault)));
        float ampScale = static_cast<float> (lfoSection.getProperty (clusterLFOamplitudeScale, clusterLFOamplitudeScaleDefault));

        int phaseX     = static_cast<int> (lfoSection.getProperty (clusterLFOphaseX, 0));
        int phaseY     = static_cast<int> (lfoSection.getProperty (clusterLFOphaseY, 0));
        int phaseZ     = static_cast<int> (lfoSection.getProperty (clusterLFOphaseZ, 0));
        int phaseRot   = static_cast<int> (lfoSection.getProperty (clusterLFOphaseRot, 0));
        int phaseScale = static_cast<int> (lfoSection.getProperty (clusterLFOphaseScale, 0));

        // Store previous offsets for delta computation
        state.prevOffsetX = state.offsetX;
        state.prevOffsetY = state.offsetY;
        state.prevOffsetZ = state.offsetZ;
        state.prevOffsetRotDeg = state.offsetRotDeg;
        state.prevOffsetScale = state.offsetScale;

        // Update global fade level (500ms fade in/out)
        if (active && state.fadeLevel < 1.0f)
            state.fadeLevel = juce::jmin (1.0f, state.fadeLevel + fadeIncrement);
        else if (!active && state.fadeLevel > 0.0f)
            state.fadeLevel = juce::jmax (0.0f, state.fadeLevel - fadeIncrement);

        // Detect activation
        bool justActivated = active && !state.wasActive;

        // Reset ramps on activation
        if (justActivated)
        {
            state.ramp = 0.0f;
            state.rampX = state.rampY = state.rampZ = state.rampRot = state.rampScale = 0.0f;
        }

        // Per-axis fade: reset when shape transitions OFF->ON or on activation
        auto updateAxisFade = [&](int prevShape, int curShape, float& fade, bool justAct) {
            if ((prevShape == LFOWaveforms::Off && curShape != LFOWaveforms::Off) ||
                (justAct && curShape != LFOWaveforms::Off && fade >= 1.0f))
                fade = 0.0f;

            if (curShape != LFOWaveforms::Off && fade < 1.0f)
                fade = juce::jmin (1.0f, fade + axisFadeIncrement);
            else if (curShape == LFOWaveforms::Off && fade > 0.0f)
                fade = juce::jmax (0.0f, fade - axisFadeIncrement);
        };

        updateAxisFade (state.prevShapeX,     shapeX,     state.fadeX,     justActivated);
        updateAxisFade (state.prevShapeY,     shapeY,     state.fadeY,     justActivated);
        updateAxisFade (state.prevShapeZ,     shapeZ,     state.fadeZ,     justActivated);
        updateAxisFade (state.prevShapeRot,   shapeRot,   state.fadeRot,   justActivated);
        updateAxisFade (state.prevShapeScale, shapeScale, state.fadeScale, justActivated);

        state.prevShapeX     = shapeX;
        state.prevShapeY     = shapeY;
        state.prevShapeZ     = shapeZ;
        state.prevShapeRot   = shapeRot;
        state.prevShapeScale = shapeScale;

        if (state.fadeLevel > 0.0f || active)
        {
            float rampInc = deltaTime / juce::jmax (0.01f, period);

            // Main ramp
            state.ramp += rampInc;
            if (state.ramp >= 1.0f) state.ramp = std::fmod (state.ramp, 1.0f);

            // Per-axis ramps with rate multipliers
            state.rampX     += rampInc * rateX;     if (state.rampX >= 1.0f)     state.rampX = std::fmod (state.rampX, 1.0f);
            state.rampY     += rampInc * rateY;     if (state.rampY >= 1.0f)     state.rampY = std::fmod (state.rampY, 1.0f);
            state.rampZ     += rampInc * rateZ;     if (state.rampZ >= 1.0f)     state.rampZ = std::fmod (state.rampZ, 1.0f);
            state.rampRot   += rampInc * rateRot;   if (state.rampRot >= 1.0f)   state.rampRot = std::fmod (state.rampRot, 1.0f);
            state.rampScale += rampInc * rateScale;  if (state.rampScale >= 1.0f) state.rampScale = std::fmod (state.rampScale, 1.0f);

            // Phase-adjusted ramps
            float adjRampX     = std::fmod (state.rampX     + (globalPhase + phaseX) / 360.0f     + 10.0f, 1.0f);
            float adjRampY     = std::fmod (state.rampY     + (globalPhase + phaseY) / 360.0f     + 10.0f, 1.0f);
            float adjRampZ     = std::fmod (state.rampZ     + (globalPhase + phaseZ) / 360.0f     + 10.0f, 1.0f);
            float adjRampRot   = std::fmod (state.rampRot   + (globalPhase + phaseRot) / 360.0f   + 10.0f, 1.0f);
            float adjRampScale = std::fmod (state.rampScale + (globalPhase + phaseScale) / 360.0f + 10.0f, 1.0f);

            // Random target generation on ramp wrap
            auto updateRandom = [&](int shape, float adjRamp, float& prevRamp, float& lastRand, float& targetRand) {
                if (shape == LFOWaveforms::Random && adjRamp < prevRamp)
                {
                    lastRand = targetRand;
                    targetRand = random.nextFloat() * 2.0f - 1.0f;
                }
                prevRamp = adjRamp;
            };

            updateRandom (shapeX,     adjRampX,     state.prevRampX,     state.lastRandomX,     state.randomTargetX);
            updateRandom (shapeY,     adjRampY,     state.prevRampY,     state.lastRandomY,     state.randomTargetY);
            updateRandom (shapeZ,     adjRampZ,     state.prevRampZ,     state.lastRandomZ,     state.randomTargetZ);
            updateRandom (shapeRot,   adjRampRot,   state.prevRampRot,   state.lastRandomRot,   state.randomTargetRot);
            updateRandom (shapeScale, adjRampScale, state.prevRampScale, state.lastRandomScale, state.randomTargetScale);

            // Waveform generation
            state.normalizedX     = LFOWaveforms::applyWaveform (shapeX,     adjRampX,     state.lastRandomX,     state.randomTargetX);
            state.normalizedY     = LFOWaveforms::applyWaveform (shapeY,     adjRampY,     state.lastRandomY,     state.randomTargetY);
            state.normalizedZ     = LFOWaveforms::applyWaveform (shapeZ,     adjRampZ,     state.lastRandomZ,     state.randomTargetZ);
            state.normalizedRot   = LFOWaveforms::applyWaveform (shapeRot,   adjRampRot,   state.lastRandomRot,   state.randomTargetRot);
            state.normalizedScale = LFOWaveforms::applyWaveform (shapeScale, adjRampScale, state.lastRandomScale, state.randomTargetScale);

            // Compute absolute offsets
            // X, Y, Z: direct amplitude * waveform * fades
            state.offsetX = state.normalizedX * ampX * state.fadeLevel * state.fadeX;
            state.offsetY = state.normalizedY * ampY * state.fadeLevel * state.fadeY;
            state.offsetZ = state.normalizedZ * ampZ * state.fadeLevel * state.fadeZ;

            // Rotation: amplitude in degrees * waveform * fades
            state.offsetRotDeg = state.normalizedRot * ampRot * state.fadeLevel * state.fadeRot;

            // Scale: log mapping — amplitude is the max scale factor (0.1 to 10.0)
            // pow(ampScale, normalizedScale * fadeLevel * fadeScale)
            // When ampScale=1.0 (default), result is always 1.0 (no modulation)
            // When ampScale=10.0 and normalized=+1, result=10.0x
            // When ampScale=10.0 and normalized=-1, result=0.1x (=1/10)
            float scalePower = state.normalizedScale * state.fadeLevel * state.fadeScale;
            state.offsetScale = std::pow (ampScale, scalePower);
        }
        else
        {
            // Fully faded out — reset offsets
            state.normalizedX = state.normalizedY = state.normalizedZ = 0.0f;
            state.normalizedRot = state.normalizedScale = 0.0f;
            state.offsetX = state.offsetY = state.offsetZ = 0.0f;
            state.offsetRotDeg = 0.0f;
            state.offsetScale = 1.0f;
        }

        state.wasActive = active;

        // Compute deltas
        state.deltaX = state.offsetX - state.prevOffsetX;
        state.deltaY = state.offsetY - state.prevOffsetY;
        state.deltaZ = state.offsetZ - state.prevOffsetZ;
        state.deltaRotDeg = state.offsetRotDeg - state.prevOffsetRotDeg;

        // Scale delta is a ratio: newScale / prevScale
        if (state.prevOffsetScale > 0.0001f)
            state.deltaScale = state.offsetScale / state.prevOffsetScale;
        else
            state.deltaScale = 1.0f;
    }

    //==========================================================================
    // Member Variables
    //==========================================================================
    WFSValueTreeState& valueTreeState;
    std::vector<ClusterLFOState> states;
    juce::Random random;
};
