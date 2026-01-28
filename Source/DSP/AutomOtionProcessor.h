#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSValueTreeState.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "../Helpers/CoordinateConverter.h"

/**
 * AutomOtion Processor for WFS Input Position Animation
 *
 * Provides programmed point-to-point movement for input channel positions.
 * Called at 50Hz from the MainComponent timer callback.
 *
 * Features:
 * - Absolute or relative destination coordinates
 * - Configurable duration (0.1s to 3600s)
 * - Speed profile (0% = constant speed, 100% = bell curve acceleration)
 * - Path curve (-100% to +100% bends perpendicular to direction in XY plane)
 * - Stay at destination or return to origin
 * - Global stop/pause controls
 * - Only active when tracking is disabled for the input
 */
class AutomOtionProcessor
{
public:
    //==========================================================================
    // State Enumeration
    //==========================================================================
    enum class State
    {
        Stopped,
        Playing,
        Paused,
        Returning
    };

    //==========================================================================
    // Per-Input AutomOtion State
    //==========================================================================
    struct AutomOtionState
    {
        State state = State::Stopped;

        // Starting position (captured when movement begins)
        float startX = 0.0f;
        float startY = 0.0f;
        float startZ = 0.0f;

        // Target position (absolute coordinates)
        float targetX = 0.0f;
        float targetY = 0.0f;
        float targetZ = 0.0f;

        // Parameters (captured at movement start)
        float duration = 5.0f;
        int speedProfile = 0;     // 0-100%
        int curve = 0;            // -100 to +100
        bool isAbsolute = true;
        bool shouldReturn = false;

        // Progress tracking
        float elapsedTime = 0.0f;
        bool inReturnPhase = false;

        // Original position (for return functionality)
        float originalX = 0.0f;
        float originalY = 0.0f;
        float originalZ = 0.0f;

        // Output offsets (applied to base position)
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        float offsetZ = 0.0f;

        // Current animated position (for UI display)
        float currentX = 0.0f;
        float currentY = 0.0f;
        float currentZ = 0.0f;

        // Audio trigger state
        float currentShortPeakDb = -200.0f;  // Latest short peak level from audio
        float currentRmsDb = -200.0f;        // Latest RMS level from audio
        bool triggerArmed = true;            // Ready to trigger on audio peak
        bool waitingForRearm = false;        // Movement complete, waiting for RMS to drop

        // Coordinate mode for this movement (captured at start)
        int coordinateMode = 0;  // 0=Cartesian, 1=Cylindrical, 2=Spherical

        // Polar start position (captured at movement start)
        float startR = 0.0f;       // Cylindrical radius
        float startTheta = 0.0f;   // Azimuth (shared cyl/sph)
        float startRsph = 0.0f;    // Spherical radius
        float startPhi = 0.0f;     // Elevation

        // Polar target position
        float targetR = 0.0f;
        float targetTheta = 0.0f;
        float targetRsph = 0.0f;
        float targetPhi = 0.0f;
    };

    //==========================================================================
    // Construction
    //==========================================================================
    explicit AutomOtionProcessor (WFSValueTreeState& state, int numInputs = 64)
        : valueTreeState (state), numInputChannels (numInputs)
    {
        states.resize (static_cast<size_t> (numInputs));
    }

    //==========================================================================
    // Processing - Called at 50Hz
    //==========================================================================
    void process (float deltaTimeSeconds)
    {
        for (int i = 0; i < numInputChannels; ++i)
        {
            processInput (i, deltaTimeSeconds);
        }
    }

    //==========================================================================
    // Per-Input Control Methods
    //==========================================================================

    /** Start motion for a specific input channel */
    void startMotion (int inputIndex)
    {
        if (inputIndex < 0 || inputIndex >= numInputChannels)
            return;

        auto& state = states[static_cast<size_t> (inputIndex)];

        // If movement is already in progress, ignore (must complete before restart)
        if (state.state == State::Playing || state.state == State::Paused || state.state == State::Returning)
        {
            DBG ("AutomOtion: Cannot start motion on input " << (inputIndex + 1) << " - movement in progress");
            return;
        }

        // Check if tracking is enabled - reject if so
        if (isTrackingActive (inputIndex))
        {
            DBG ("AutomOtion: Cannot start motion on input " << (inputIndex + 1) << " - tracking is active");
            return;
        }

        // Reset trigger state when manually or programmatically started
        state.triggerArmed = false;
        state.waitingForRearm = false;

        // Get current base position from ValueTree
        auto posSection = valueTreeState.getInputPositionSection (inputIndex);
        float baseX = static_cast<float> (posSection.getProperty (WFSParameterIDs::inputPositionX, 0.0f));
        float baseY = static_cast<float> (posSection.getProperty (WFSParameterIDs::inputPositionY, 0.0f));
        float baseZ = static_cast<float> (posSection.getProperty (WFSParameterIDs::inputPositionZ, 0.0f));

        // Get AutomOtion parameters
        auto otomoSection = valueTreeState.getInputAutoMotionSection (inputIndex);
        bool isAbsolute = static_cast<int> (otomoSection.getProperty (WFSParameterIDs::inputOtomoAbsoluteRelative, 0)) == 0;
        bool shouldReturn = static_cast<int> (otomoSection.getProperty (WFSParameterIDs::inputOtomoStayReturn, 0)) != 0;
        int speedProfile = static_cast<int> (otomoSection.getProperty (WFSParameterIDs::inputOtomoSpeedProfile, 0));
        float duration = static_cast<float> (otomoSection.getProperty (WFSParameterIDs::inputOtomoDuration, 5.0f));
        int curve = static_cast<int> (otomoSection.getProperty (WFSParameterIDs::inputOtomoCurve, 0));
        int coordMode = static_cast<int> (otomoSection.getProperty (WFSParameterIDs::inputOtomoCoordinateMode, 0));

        // Clamp duration to valid range
        duration = juce::jlimit (WFSParameterDefaults::inputOtomoDurationMin,
                                 WFSParameterDefaults::inputOtomoDurationMax,
                                 duration);

        // Store starting position (always Cartesian)
        state.startX = baseX;
        state.startY = baseY;
        state.startZ = baseZ;
        state.originalX = baseX;
        state.originalY = baseY;
        state.originalZ = baseZ;

        // Store coordinate mode
        state.coordinateMode = coordMode;

        if (coordMode == 1)  // Cylindrical
        {
            // Convert start position to cylindrical
            auto startCyl = WFSCoordinates::cartesianToCylindrical ({ baseX, baseY, baseZ });
            state.startR = startCyl.r;
            state.startTheta = startCyl.theta;
            // startZ already set

            // Get target in cylindrical
            float targetR = static_cast<float> (otomoSection.getProperty (WFSParameterIDs::inputOtomoR, 0.0f));
            float targetTheta = static_cast<float> (otomoSection.getProperty (WFSParameterIDs::inputOtomoTheta, 0.0f));
            float targetZ = static_cast<float> (otomoSection.getProperty (WFSParameterIDs::inputOtomoZ, 0.0f));

            if (isAbsolute)
            {
                state.targetR = targetR;
                state.targetTheta = targetTheta;
                state.targetZ = targetZ;
            }
            else
            {
                // Relative: add to start (radius clamped to >= 0)
                state.targetR = std::max (0.0f, state.startR + targetR);
                state.targetTheta = state.startTheta + targetTheta;  // Additive angle for spirals!
                state.targetZ = state.startZ + targetZ;
            }

            // Convert final target to Cartesian (for UI display and offset calculation)
            auto targetCart = WFSCoordinates::cylindricalToCartesian ({
                state.targetR, WFSCoordinates::normalizeAngle (state.targetTheta), state.targetZ });
            state.targetX = targetCart.x;
            state.targetY = targetCart.y;

            // Force curve to 0 for polar modes
            curve = 0;
        }
        else if (coordMode == 2)  // Spherical
        {
            // Convert start position to spherical
            auto startSph = WFSCoordinates::cartesianToSpherical ({ baseX, baseY, baseZ });
            state.startRsph = startSph.r;
            state.startTheta = startSph.theta;
            state.startPhi = startSph.phi;

            // Get target in spherical
            float targetR = static_cast<float> (otomoSection.getProperty (WFSParameterIDs::inputOtomoRsph, 0.0f));
            float targetTheta = static_cast<float> (otomoSection.getProperty (WFSParameterIDs::inputOtomoTheta, 0.0f));
            float targetPhi = static_cast<float> (otomoSection.getProperty (WFSParameterIDs::inputOtomoPhi, 0.0f));

            if (isAbsolute)
            {
                state.targetRsph = targetR;
                state.targetTheta = targetTheta;
                state.targetPhi = targetPhi;
            }
            else
            {
                // Relative: add to start (radius clamped to >= 0)
                state.targetRsph = std::max (0.0f, state.startRsph + targetR);
                state.targetTheta = state.startTheta + targetTheta;  // Additive angle for spirals!
                state.targetPhi = state.startPhi + targetPhi;        // Additive elevation for spirals!
            }

            // Convert final target to Cartesian (for UI display and offset calculation)
            auto targetCart = WFSCoordinates::sphericalToCartesian ({
                state.targetRsph,
                WFSCoordinates::normalizeAngle (state.targetTheta),
                WFSCoordinates::clampElevation (state.targetPhi) });
            state.targetX = targetCart.x;
            state.targetY = targetCart.y;
            state.targetZ = targetCart.z;

            // Force curve to 0 for polar modes
            curve = 0;
        }
        else  // Cartesian (mode 0)
        {
            float destX = static_cast<float> (otomoSection.getProperty (WFSParameterIDs::inputOtomoX, 0.0f));
            float destY = static_cast<float> (otomoSection.getProperty (WFSParameterIDs::inputOtomoY, 0.0f));
            float destZ = static_cast<float> (otomoSection.getProperty (WFSParameterIDs::inputOtomoZ, 0.0f));

            if (isAbsolute)
            {
                state.targetX = destX;
                state.targetY = destY;
                state.targetZ = destZ;
            }
            else
            {
                // Relative: destination is offset from current position
                state.targetX = baseX + destX;
                state.targetY = baseY + destY;
                state.targetZ = baseZ + destZ;
            }
        }

        // Store parameters
        state.duration = duration;
        state.speedProfile = speedProfile;
        state.curve = curve;
        state.isAbsolute = isAbsolute;
        state.shouldReturn = shouldReturn;

        // Initialize motion
        state.elapsedTime = 0.0f;
        state.inReturnPhase = false;
        state.state = State::Playing;
        state.currentX = baseX;
        state.currentY = baseY;
        state.currentZ = baseZ;
    }

    /** Stop motion for a specific input channel */
    void stopMotion (int inputIndex)
    {
        if (inputIndex < 0 || inputIndex >= numInputChannels)
            return;

        auto& state = states[static_cast<size_t> (inputIndex)];
        state.state = State::Stopped;
        state.offsetX = 0.0f;
        state.offsetY = 0.0f;
        state.offsetZ = 0.0f;
        state.elapsedTime = 0.0f;
        state.inReturnPhase = false;
    }

    /** Pause motion for a specific input channel */
    void pauseMotion (int inputIndex)
    {
        if (inputIndex < 0 || inputIndex >= numInputChannels)
            return;

        auto& state = states[static_cast<size_t> (inputIndex)];
        if (state.state == State::Playing || state.state == State::Returning)
        {
            state.state = State::Paused;
        }
    }

    /** Resume motion for a specific input channel */
    void resumeMotion (int inputIndex)
    {
        if (inputIndex < 0 || inputIndex >= numInputChannels)
            return;

        auto& state = states[static_cast<size_t> (inputIndex)];
        if (state.state == State::Paused)
        {
            state.state = state.inReturnPhase ? State::Returning : State::Playing;
        }
    }

    //==========================================================================
    // Global Control Methods
    //==========================================================================

    /** Stop all active motions */
    void stopAllMotion()
    {
        for (int i = 0; i < numInputChannels; ++i)
        {
            stopMotion (i);
        }
    }

    /** Pause all active motions */
    void pauseAllMotion()
    {
        for (int i = 0; i < numInputChannels; ++i)
        {
            pauseMotion (i);
        }
    }

    /** Resume all paused motions */
    void resumeAllMotion()
    {
        for (int i = 0; i < numInputChannels; ++i)
        {
            resumeMotion (i);
        }
    }

    /** Check if any motion is currently paused */
    bool isAnyPaused() const
    {
        for (int i = 0; i < numInputChannels; ++i)
        {
            if (states[static_cast<size_t> (i)].state == State::Paused)
                return true;
        }
        return false;
    }

    /** Check if any motion is currently active (playing or paused) */
    bool isAnyActive() const
    {
        for (int i = 0; i < numInputChannels; ++i)
        {
            auto s = states[static_cast<size_t> (i)].state;
            if (s == State::Playing || s == State::Paused || s == State::Returning)
                return true;
        }
        return false;
    }

    //==========================================================================
    // Audio Level Input (for audio triggering)
    //==========================================================================

    /** Set current audio levels for an input (called from timer thread at 50Hz) */
    void setInputLevels (int inputIndex, float shortPeakDb, float rmsDb)
    {
        if (inputIndex < 0 || inputIndex >= numInputChannels)
            return;
        auto& state = states[static_cast<size_t> (inputIndex)];
        state.currentShortPeakDb = shortPeakDb;
        state.currentRmsDb = rmsDb;
    }

    //==========================================================================
    // Output Accessors
    //==========================================================================

    /** Get current offset X for an input */
    float getOffsetX (int inputIndex) const
    {
        if (inputIndex < 0 || inputIndex >= numInputChannels)
            return 0.0f;
        return states[static_cast<size_t> (inputIndex)].offsetX;
    }

    /** Get current offset Y for an input */
    float getOffsetY (int inputIndex) const
    {
        if (inputIndex < 0 || inputIndex >= numInputChannels)
            return 0.0f;
        return states[static_cast<size_t> (inputIndex)].offsetY;
    }

    /** Get current offset Z for an input */
    float getOffsetZ (int inputIndex) const
    {
        if (inputIndex < 0 || inputIndex >= numInputChannels)
            return 0.0f;
        return states[static_cast<size_t> (inputIndex)].offsetZ;
    }

    /** Check if motion is active for an input */
    bool isActive (int inputIndex) const
    {
        if (inputIndex < 0 || inputIndex >= numInputChannels)
            return false;
        auto s = states[static_cast<size_t> (inputIndex)].state;
        return s == State::Playing || s == State::Paused || s == State::Returning;
    }

    /** Check if motion is paused for an input */
    bool isPaused (int inputIndex) const
    {
        if (inputIndex < 0 || inputIndex >= numInputChannels)
            return false;
        return states[static_cast<size_t> (inputIndex)].state == State::Paused;
    }

    /** Get motion progress (0.0 to 1.0) for an input */
    float getProgress (int inputIndex) const
    {
        if (inputIndex < 0 || inputIndex >= numInputChannels)
            return 0.0f;
        const auto& state = states[static_cast<size_t> (inputIndex)];
        if (state.duration <= 0.0f)
            return 0.0f;
        return juce::jmin (1.0f, state.elapsedTime / state.duration);
    }

    /** Get current animated position X */
    float getCurrentX (int inputIndex) const
    {
        if (inputIndex < 0 || inputIndex >= numInputChannels)
            return 0.0f;
        return states[static_cast<size_t> (inputIndex)].currentX;
    }

    /** Get current animated position Y */
    float getCurrentY (int inputIndex) const
    {
        if (inputIndex < 0 || inputIndex >= numInputChannels)
            return 0.0f;
        return states[static_cast<size_t> (inputIndex)].currentY;
    }

    /** Get current animated position Z */
    float getCurrentZ (int inputIndex) const
    {
        if (inputIndex < 0 || inputIndex >= numInputChannels)
            return 0.0f;
        return states[static_cast<size_t> (inputIndex)].currentZ;
    }

private:
    //==========================================================================
    // Per-Input Processing
    //==========================================================================
    void processInput (int inputIndex, float deltaTime)
    {
        auto& state = states[static_cast<size_t> (inputIndex)];

        // Get trigger mode and thresholds
        auto otomoSection = valueTreeState.getInputAutoMotionSection (inputIndex);
        bool audioTriggerEnabled = static_cast<int> (otomoSection.getProperty (WFSParameterIDs::inputOtomoTrigger, 0)) == 1;

        // Handle audio triggering when stopped
        if (audioTriggerEnabled && state.state == State::Stopped)
        {
            float triggerThresholdDb = static_cast<float> (otomoSection.getProperty (WFSParameterIDs::inputOtomoThreshold, -20.0f));
            float resetThresholdDb = static_cast<float> (otomoSection.getProperty (WFSParameterIDs::inputOtomoReset, -60.0f));

            // Check rearm condition: RMS dropped below reset threshold
            if (state.waitingForRearm)
            {
                if (state.currentRmsDb < resetThresholdDb)
                {
                    state.triggerArmed = true;
                    state.waitingForRearm = false;
                }
            }

            // Check trigger condition: armed + short peak above threshold
            if (state.triggerArmed && state.currentShortPeakDb > triggerThresholdDb)
            {
                // Trigger the motion!
                startMotion (inputIndex);
                // Note: startMotion will set triggerArmed = false
            }
        }

        // Skip movement processing if not playing
        if (state.state != State::Playing && state.state != State::Returning)
            return;

        // Check if tracking became active - stop if so
        if (isTrackingActive (inputIndex))
        {
            stopMotion (inputIndex);
            return;
        }

        // Update elapsed time
        state.elapsedTime += deltaTime;

        // Calculate linear progress (0 to 1)
        float linearProgress = juce::jmin (1.0f, state.elapsedTime / state.duration);

        // Apply speed profile
        float adjustedProgress = applySpeedProfile (linearProgress, state.speedProfile);

        // Calculate position based on coordinate mode
        if (state.coordinateMode == 0)  // Cartesian
            calculateCurvedPosition (state, adjustedProgress, inputIndex);
        else  // Polar (Cylindrical or Spherical)
            calculatePolarPosition (state, adjustedProgress, inputIndex);

        // Check if movement complete
        if (linearProgress >= 1.0f)
        {
            // For audio trigger mode with Return: instant snap back (no animated return)
            if (audioTriggerEnabled && state.shouldReturn && !state.inReturnPhase)
            {
                // Snap back instantly to origin
                state.state = State::Stopped;
                state.offsetX = 0.0f;
                state.offsetY = 0.0f;
                state.offsetZ = 0.0f;
                state.currentX = state.originalX;
                state.currentY = state.originalY;
                state.currentZ = state.originalZ;
                state.elapsedTime = 0.0f;
                state.inReturnPhase = false;

                // Set up rearm for audio trigger
                state.waitingForRearm = true;
                state.triggerArmed = false;
            }
            else if (!state.inReturnPhase && state.shouldReturn)
            {
                // Manual mode: Start animated return phase
                state.inReturnPhase = true;
                state.state = State::Returning;

                // Swap start and target for return journey (Cartesian)
                std::swap (state.startX, state.targetX);
                std::swap (state.startY, state.targetY);
                std::swap (state.startZ, state.targetZ);

                // Swap polar values for return journey
                std::swap (state.startR, state.targetR);
                std::swap (state.startTheta, state.targetTheta);
                std::swap (state.startRsph, state.targetRsph);
                std::swap (state.startPhi, state.targetPhi);

                // Invert curve for return path (Cartesian mode only)
                if (state.coordinateMode == 0)
                    state.curve = -state.curve;

                // Reset elapsed time for return
                state.elapsedTime = 0.0f;
            }
            else
            {
                // Movement complete (either stayed at destination, or finished return phase)
                state.state = State::Stopped;
                state.elapsedTime = 0.0f;

                if (state.inReturnPhase)
                {
                    // Returned to origin - clear offsets
                    state.offsetX = 0.0f;
                    state.offsetY = 0.0f;
                    state.offsetZ = 0.0f;
                    state.currentX = state.originalX;
                    state.currentY = state.originalY;
                    state.currentZ = state.originalZ;
                }
                else
                {
                    // Stayed at destination - keep final offsets
                    state.currentX = state.targetX;
                    state.currentY = state.targetY;
                    state.currentZ = state.targetZ;
                }

                state.inReturnPhase = false;

                // Set up rearm for audio trigger mode
                if (audioTriggerEnabled)
                {
                    state.waitingForRearm = true;
                    state.triggerArmed = false;
                }
            }
        }
    }

    //==========================================================================
    // Speed Profile Algorithm
    //==========================================================================

    /**
     * Apply speed profile to transform linear progress into eased progress
     * @param linearProgress Linear progress 0→1
     * @param speedProfilePercent 0 = constant speed, 100 = full bell curve
     * @return Adjusted progress 0→1 with speed profile applied
     */
    float applySpeedProfile (float linearProgress, int speedProfilePercent) const
    {
        if (speedProfilePercent <= 0)
            return linearProgress;

        // Bell curve using cosine: (1 - cos(π * t)) / 2
        // This gives slow start, fast middle, slow end
        float bellProgress = (1.0f - std::cos (juce::MathConstants<float>::pi * linearProgress)) / 2.0f;

        // Blend between linear and bell based on speedProfile percentage
        float blend = static_cast<float> (speedProfilePercent) / 100.0f;
        return linearProgress * (1.0f - blend) + bellProgress * blend;
    }

    //==========================================================================
    // Curved Position Algorithm
    //==========================================================================

    /**
     * Calculate position along curved path
     * Curve bends perpendicular to direction of travel in XY plane
     */
    void calculateCurvedPosition (AutomOtionState& state, float progress, int inputIndex)
    {
        // Get base position for offset calculation
        auto posSection = valueTreeState.getInputPositionSection (inputIndex);
        float baseX = static_cast<float> (posSection.getProperty (WFSParameterIDs::inputPositionX, 0.0f));
        float baseY = static_cast<float> (posSection.getProperty (WFSParameterIDs::inputPositionY, 0.0f));
        float baseZ = static_cast<float> (posSection.getProperty (WFSParameterIDs::inputPositionZ, 0.0f));

        // Direction vector from start to target
        float dx = state.targetX - state.startX;
        float dy = state.targetY - state.startY;
        float dz = state.targetZ - state.startZ;

        // Linear interpolation position
        float linearX = state.startX + dx * progress;
        float linearY = state.startY + dy * progress;
        float linearZ = state.startZ + dz * progress;

        if (state.curve == 0)
        {
            // No curve - straight path
            state.currentX = linearX;
            state.currentY = linearY;
            state.currentZ = linearZ;
            state.offsetX = linearX - baseX;
            state.offsetY = linearY - baseY;
            state.offsetZ = linearZ - baseZ;
            return;
        }

        // Calculate perpendicular vector in XY plane
        // Perpendicular to (dx, dy) is (-dy, dx) (rotated 90 degrees counter-clockwise)
        float pathLength2D = std::sqrt (dx * dx + dy * dy);

        if (pathLength2D < 0.001f)
        {
            // Very short horizontal path - no meaningful curve possible
            state.currentX = linearX;
            state.currentY = linearY;
            state.currentZ = linearZ;
            state.offsetX = linearX - baseX;
            state.offsetY = linearY - baseY;
            state.offsetZ = linearZ - baseZ;
            return;
        }

        // Normalized perpendicular vector (points to the "left" of direction)
        float perpX = -dy / pathLength2D;
        float perpY = dx / pathLength2D;

        // Curve displacement calculation
        // curvePercent: negative = left bend, positive = right bend
        float curveAmount = static_cast<float> (state.curve) / 100.0f;

        // Maximum displacement at midpoint (scaled by path length for proportional curves)
        float maxCurveDisplacement = pathLength2D * 0.5f * std::abs (curveAmount);

        // Sine arc: sin(π * progress) is 0 at start, 1 at midpoint, 0 at end
        float arcFactor = std::sin (juce::MathConstants<float>::pi * progress);
        float curveDisplacement = maxCurveDisplacement * arcFactor * (curveAmount > 0 ? 1.0f : -1.0f);

        // Apply curve displacement perpendicular to direction
        float curvedX = linearX + perpX * curveDisplacement;
        float curvedY = linearY + perpY * curveDisplacement;
        float curvedZ = linearZ;  // Z follows linear interpolation (no horizontal curve in Z)

        // Store results
        state.currentX = curvedX;
        state.currentY = curvedY;
        state.currentZ = curvedZ;
        state.offsetX = curvedX - baseX;
        state.offsetY = curvedY - baseY;
        state.offsetZ = curvedZ - baseZ;
    }

    //==========================================================================
    // Polar Position Algorithm
    //==========================================================================

    /**
     * Calculate position along polar path (cylindrical or spherical)
     * Interpolates directly in polar space for natural spiral movements
     */
    void calculatePolarPosition (AutomOtionState& state, float progress, int inputIndex)
    {
        // Get base position for offset calculation
        auto posSection = valueTreeState.getInputPositionSection (inputIndex);
        float baseX = static_cast<float> (posSection.getProperty (WFSParameterIDs::inputPositionX, 0.0f));
        float baseY = static_cast<float> (posSection.getProperty (WFSParameterIDs::inputPositionY, 0.0f));
        float baseZ = static_cast<float> (posSection.getProperty (WFSParameterIDs::inputPositionZ, 0.0f));

        if (state.coordinateMode == 1)  // Cylindrical
        {
            // Linear interpolation in cylindrical space
            float r = state.startR + (state.targetR - state.startR) * progress;
            float theta = state.startTheta + (state.targetTheta - state.startTheta) * progress;
            float z = state.startZ + (state.targetZ - state.startZ) * progress;

            // Ensure radius is non-negative
            r = std::max (0.0f, r);

            // Convert to Cartesian (normalize angle for conversion function)
            auto cart = WFSCoordinates::cylindricalToCartesian ({ r, WFSCoordinates::normalizeAngle (theta), z });

            state.currentX = cart.x;
            state.currentY = cart.y;
            state.currentZ = z;
            state.offsetX = cart.x - baseX;
            state.offsetY = cart.y - baseY;
            state.offsetZ = z - baseZ;
        }
        else if (state.coordinateMode == 2)  // Spherical
        {
            // Linear interpolation in spherical space
            float r = state.startRsph + (state.targetRsph - state.startRsph) * progress;
            float theta = state.startTheta + (state.targetTheta - state.startTheta) * progress;
            float phi = state.startPhi + (state.targetPhi - state.startPhi) * progress;

            // Ensure radius is non-negative
            r = std::max (0.0f, r);

            // Convert to Cartesian (normalize angles for conversion function)
            // Note: For phi, we use a modulo approach to handle multi-rotation elevation
            float normalizedPhi = std::fmod (phi, 360.0f);
            if (normalizedPhi > 180.0f) normalizedPhi -= 360.0f;
            if (normalizedPhi < -180.0f) normalizedPhi += 360.0f;
            normalizedPhi = WFSCoordinates::clampElevation (normalizedPhi);

            auto cart = WFSCoordinates::sphericalToCartesian ({
                r, WFSCoordinates::normalizeAngle (theta), normalizedPhi });

            state.currentX = cart.x;
            state.currentY = cart.y;
            state.currentZ = cart.z;
            state.offsetX = cart.x - baseX;
            state.offsetY = cart.y - baseY;
            state.offsetZ = cart.z - baseZ;
        }
    }

    //==========================================================================
    // Tracking Check
    //==========================================================================

    /** Check if tracking is active for an input */
    bool isTrackingActive (int inputIndex) const
    {
        auto posSection = valueTreeState.getInputPositionSection (inputIndex);
        return static_cast<int> (posSection.getProperty (WFSParameterIDs::inputTrackingActive, 0)) != 0;
    }

    //==========================================================================
    // Member Variables
    //==========================================================================
    WFSValueTreeState& valueTreeState;
    int numInputChannels;
    std::vector<AutomOtionState> states;
};
