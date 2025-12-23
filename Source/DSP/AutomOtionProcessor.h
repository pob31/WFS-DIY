#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSValueTreeState.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"

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
        float destX = static_cast<float> (otomoSection.getProperty (WFSParameterIDs::inputOtomoX, 0.0f));
        float destY = static_cast<float> (otomoSection.getProperty (WFSParameterIDs::inputOtomoY, 0.0f));
        float destZ = static_cast<float> (otomoSection.getProperty (WFSParameterIDs::inputOtomoZ, 0.0f));
        bool isAbsolute = static_cast<int> (otomoSection.getProperty (WFSParameterIDs::inputOtomoAbsoluteRelative, 0)) == 0;
        bool shouldReturn = static_cast<int> (otomoSection.getProperty (WFSParameterIDs::inputOtomoStayReturn, 0)) != 0;
        int speedProfile = static_cast<int> (otomoSection.getProperty (WFSParameterIDs::inputOtomoSpeedProfile, 0));
        float duration = static_cast<float> (otomoSection.getProperty (WFSParameterIDs::inputOtomoDuration, 5.0f));
        int curve = static_cast<int> (otomoSection.getProperty (WFSParameterIDs::inputOtomoCurve, 0));

        // Clamp duration to valid range
        duration = juce::jlimit (WFSParameterDefaults::inputOtomoDurationMin,
                                 WFSParameterDefaults::inputOtomoDurationMax,
                                 duration);

        // Store starting position
        state.startX = baseX;
        state.startY = baseY;
        state.startZ = baseZ;
        state.originalX = baseX;
        state.originalY = baseY;
        state.originalZ = baseZ;

        // Calculate target position
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

        // Calculate curved position
        calculateCurvedPosition (state, adjustedProgress, inputIndex);

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

                // Swap start and target for return journey
                std::swap (state.startX, state.targetX);
                std::swap (state.startY, state.targetY);
                std::swap (state.startZ, state.targetZ);

                // Invert curve for return path (maintains same arc direction relative to observer)
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
