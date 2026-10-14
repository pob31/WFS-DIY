#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSValueTreeState.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "../../spatcore/dsp/LFOWaveforms.h"
#include "LFOAxis.h"

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
 *
 * No change makes an axis jump (LFOAxis, re-audit F6): Off fades out the shape
 * it replaces, and a shape, amplitude or phase change is bridged over a second.
 * Rotation is bridged in degrees, scale in its logarithm so it stays positive.
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

        float fadeLevel = 0.0f;         // 0->1 for 500ms fade in/out
        bool wasActive = false;

        // Per-axis ramp, fade, Random targets and continuity (LFOAxis).
        // Scale's output is the logarithm of the factor.
        LFOAxis x, y, z, rot, scale;

        bool bridging() const noexcept
        {
            return x.bridging() || y.bridging() || z.bridging() || rot.bridging() || scale.bridging();
        }

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
        const float fadeIncrement = deltaTimeSeconds / fadeTimeSeconds;

        for (int c = 0; c < maxClusters; ++c)
        {
            processCluster (c, deltaTimeSeconds, fadeIncrement);
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

    /** Get absolute position offset (meters) — for offset-based routing */
    float getOffsetX (int clusterIndex) const { return getState (clusterIndex).offsetX; }
    float getOffsetY (int clusterIndex) const { return getState (clusterIndex).offsetY; }
    float getOffsetZ (int clusterIndex) const { return getState (clusterIndex).offsetZ; }

    /** Get absolute rotation offset (degrees) */
    float getOffsetRotDeg (int clusterIndex) const { return getState (clusterIndex).offsetRotDeg; }

    /** Get absolute scale factor (1.0 = no change) */
    float getOffsetScale (int clusterIndex) const { return getState (clusterIndex).offsetScale; }

    /** Get normalized output (-1 to +1) for UI indicators */
    float getNormalizedX (int clusterIndex) const { return getState (clusterIndex).normalizedX; }
    float getNormalizedY (int clusterIndex) const { return getState (clusterIndex).normalizedY; }
    float getNormalizedZ (int clusterIndex) const { return getState (clusterIndex).normalizedZ; }
    float getNormalizedRot (int clusterIndex) const { return getState (clusterIndex).normalizedRot; }
    float getNormalizedScale (int clusterIndex) const { return getState (clusterIndex).normalizedScale; }

    /** Get ramp progress (0->1) for progress indicator */
    float getRampProgress (int clusterIndex) const { return getState (clusterIndex).ramp; }

    /** Check if LFO is active (or fading, or still releasing a step) for a cluster */
    bool isActive (int clusterIndex) const
    {
        auto& s = getState (clusterIndex);
        return s.wasActive || s.fadeLevel > 0.0f || s.bridging();
    }

private:
    //==========================================================================
    // State Access
    //==========================================================================
    const ClusterLFOState& getState (int clusterIndex) const
    {
        static const ClusterLFOState emptyState {};
        int idx = clusterIndex - 1;
        if (idx >= 0 && idx < maxClusters)
            return states[static_cast<size_t> (idx)];
        return emptyState;
    }

    //==========================================================================
    // Per-Cluster Processing
    //==========================================================================
    void processCluster (int idx, float deltaTime, float fadeIncrement)
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
        const bool wasSilent = state.fadeLevel <= 0.0f;
        if (active && state.fadeLevel < 1.0f)
            state.fadeLevel = juce::jmin (1.0f, state.fadeLevel + fadeIncrement);
        else if (!active && state.fadeLevel > 0.0f)
            state.fadeLevel = juce::jmax (0.0f, state.fadeLevel - fadeIncrement);

        // Reset ramps on activation - from silence only: switched back on
        // during its fade-out, the LFO carries on where it is (re-audit F6)
        if (active && !state.wasActive && wasSilent)
        {
            state.ramp = 0.0f;
            state.x.restart (shapeX);
            state.y.restart (shapeY);
            state.z.restart (shapeZ);
            state.rot.restart (shapeRot);
            state.scale.restart (shapeScale);
        }

        // Per-axis fade: 1 s in while a shape is chosen, 1 s out at OFF. A
        // change between two shapes is bridged; it used to dip the fade to 0,
        // dropping the axis to about 0 in one tick.
        state.x.updateFade (shapeX, deltaTime);
        state.y.updateFade (shapeY, deltaTime);
        state.z.updateFade (shapeZ, deltaTime);
        state.rot.updateFade (shapeRot, deltaTime);
        state.scale.updateFade (shapeScale, deltaTime);

        const float phaseCyclesX     = (globalPhase + phaseX) / 360.0f;
        const float phaseCyclesY     = (globalPhase + phaseY) / 360.0f;
        const float phaseCyclesZ     = (globalPhase + phaseZ) / 360.0f;
        const float phaseCyclesRot   = (globalPhase + phaseRot) / 360.0f;
        const float phaseCyclesScale = (globalPhase + phaseScale) / 360.0f;

        // Scale is bridged in its logarithm: pow (ampScale, p) = exp (p * ln ampScale)
        const float logAmpScale = std::log (juce::jmax (1.0e-6f, ampScale));

        if (state.fadeLevel > 0.0f || active || state.bridging())
        {
            float rampInc = deltaTime / juce::jmax (0.01f, period);

            // Main ramp
            state.ramp += rampInc;
            if (state.ramp >= 1.0f) state.ramp = std::fmod (state.ramp, 1.0f);

            // Per-axis ramps with rate multipliers
            state.x.advance (rampInc * rateX);
            state.y.advance (rampInc * rateY);
            state.z.advance (rampInc * rateZ);
            state.rot.advance (rampInc * rateRot);
            state.scale.advance (rampInc * rateScale);

            // X, Y, Z: direct amplitude * waveform * fades
            state.offsetX = state.x.evaluate (shapeX, ampX, phaseCyclesX, state.fadeLevel, deltaTime, random);
            state.offsetY = state.y.evaluate (shapeY, ampY, phaseCyclesY, state.fadeLevel, deltaTime, random);
            state.offsetZ = state.z.evaluate (shapeZ, ampZ, phaseCyclesZ, state.fadeLevel, deltaTime, random);

            // Rotation: unipolar 0° to ampRot (waveform -1..+1 mapped to 0..1)
            state.offsetRotDeg = state.rot.evaluate (shapeRot, ampRot, phaseCyclesRot, state.fadeLevel,
                                                     deltaTime, random, true);

            // Scale: unipolar 1.0x to ampScale, pow (ampScale, uni * fades).
            // When ampScale=1.0 (default), result is always 1.0 (no modulation)
            state.offsetScale = std::exp (state.scale.evaluate (shapeScale, logAmpScale, phaseCyclesScale,
                                                                state.fadeLevel, deltaTime, random, true));

            state.normalizedX     = state.x.normalized;
            state.normalizedY     = state.y.normalized;
            state.normalizedZ     = state.z.normalized;
            state.normalizedRot   = state.rot.normalized;
            state.normalizedScale = state.scale.normalized;
        }
        else
        {
            // Fully faded out — reset offsets
            state.normalizedX = state.normalizedY = state.normalizedZ = 0.0f;
            state.normalizedRot = state.normalizedScale = 0.0f;
            state.offsetX = state.offsetY = state.offsetZ = 0.0f;
            state.offsetRotDeg = 0.0f;
            state.offsetScale = 1.0f;
            state.x.rest (shapeX, ampX, phaseCyclesX);
            state.y.rest (shapeY, ampY, phaseCyclesY);
            state.z.rest (shapeZ, ampZ, phaseCyclesZ);
            state.rot.rest (shapeRot, ampRot, phaseCyclesRot);
            state.scale.rest (shapeScale, logAmpScale, phaseCyclesScale);
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
