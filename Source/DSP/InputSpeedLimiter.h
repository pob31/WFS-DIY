#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <array>
#include <memory>

/**
 * Input Speed Limiter
 *
 * Provides smooth speed-limited movement for input positions with tanh-based
 * acceleration/deceleration. Target positions are interpolated to create natural
 * movement that accelerates smoothly from rest and decelerates when approaching
 * the target.
 *
 * Path Mode: When enabled, waypoints are captured during drag operations and
 * the marker follows the drawn path instead of moving in a straight line.
 *
 * Processing is performed at 50Hz (called from MainComponent timer).
 * The speed limiter sits BEFORE flip/offset/LFO in the position chain.
 */
class InputSpeedLimiter
{
public:
    InputSpeedLimiter() = default;

    //==========================================================================
    // Waypoint structure for path mode
    //==========================================================================
    struct Waypoint
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    //==========================================================================
    // Configuration
    //==========================================================================

    /** Resize for the given number of inputs */
    void resize (int numInputs)
    {
        states.resize (static_cast<size_t> (numInputs));
    }

    /** Get number of inputs */
    int getNumInputs() const { return static_cast<int> (states.size()); }

    //==========================================================================
    // Target and Speed Settings
    //==========================================================================

    /** Set target position for an input (typically from ValueTree) */
    void setTargetPosition (int inputIndex, float x, float y, float z)
    {
        if (inputIndex < 0 || inputIndex >= static_cast<int> (states.size()))
            return;

        auto& state = states[static_cast<size_t> (inputIndex)];

        // Initialize current position on first call
        if (!state.initialized)
        {
            state.currentX = x;
            state.currentY = y;
            state.currentZ = z;
            state.initialized = true;
        }

        state.targetX = x;
        state.targetY = y;
        state.targetZ = z;
    }

    /** Set speed limit parameters for an input */
    void setSpeedLimit (int inputIndex, bool active, float maxSpeed)
    {
        if (inputIndex < 0 || inputIndex >= static_cast<int> (states.size()))
            return;

        auto& state = states[static_cast<size_t> (inputIndex)];
        state.active = active;
        state.maxSpeed = juce::jlimit (0.01f, 20.0f, maxSpeed);
    }

    //==========================================================================
    // Processing
    //==========================================================================

    /** Process all inputs, interpolating towards targets.
        Call this at 50Hz (deltaTime = 0.02f) */
    void process (float deltaTime)
    {
        anyMoving = false;

        for (auto& state : states)
        {
            if (!state.initialized)
            {
                // Not yet initialized, wait for first target
                continue;
            }

            if (!state.active)
            {
                // Speed limit disabled - pass through target directly
                state.currentX = state.targetX;
                state.currentY = state.targetY;
                state.currentZ = state.targetZ;
                continue;
            }

            // Determine movement target: waypoint (if path mode) or final target
            float moveTargetX = state.targetX;
            float moveTargetY = state.targetY;
            float moveTargetZ = state.targetZ;
            bool usingWaypoint = false;

            if (state.pathModeEnabled)
            {
                // Path mode: check if we have waypoints to follow
                // Follow waypoints even during recording so movement starts immediately
                juce::SpinLock::ScopedLockType lock (*state.waypointLock);
                if (state.waypointCount > 0)
                {
                    // Use next waypoint as intermediate target
                    const auto& wp = state.waypointBuffer[state.waypointTail];
                    moveTargetX = wp.x;
                    moveTargetY = wp.y;
                    moveTargetZ = wp.z;
                    usingWaypoint = true;
                }
            }

            // Calculate vector from current to movement target
            float dx = moveTargetX - state.currentX;
            float dy = moveTargetY - state.currentY;
            float dz = moveTargetZ - state.currentZ;
            float distance = std::sqrt (dx * dx + dy * dy + dz * dz);

            // Snap threshold - close enough to target
            constexpr float snapThreshold = 0.001f;
            if (distance < snapThreshold)
            {
                state.currentX = moveTargetX;
                state.currentY = moveTargetY;
                state.currentZ = moveTargetZ;

                // If we reached a waypoint, advance to next
                if (usingWaypoint)
                {
                    juce::SpinLock::ScopedLockType lock (*state.waypointLock);
                    if (state.waypointCount > 0)
                    {
                        state.waypointTail = (state.waypointTail + 1) % maxWaypoints;
                        state.waypointCount--;
                        anyMoving = true;  // More waypoints to follow
                    }
                }
                continue;
            }

            // Mark that we're still moving
            anyMoving = true;

            // Maximum distance we can travel this frame
            float maxStep = state.maxSpeed * deltaTime;

            float step;
            if (usingWaypoint)
            {
                // Following waypoints: use constant speed for smooth path following
                // No deceleration between waypoints - just move at max speed
                step = std::min (maxStep, distance);
            }
            else
            {
                // Approaching final target: use tanh smoothing for natural deceleration
                // tanh(x) approaches 1.0 when x is large, approaches x when x is small
                // This gives: full speed when far, gradual slowdown when near
                float normalizedDist = distance / maxStep;
                step = maxStep * std::tanh (normalizedDist);
                step = std::min (step, distance);
            }

            // Apply step in direction of target
            float invDist = 1.0f / distance;
            state.currentX += dx * invDist * step;
            state.currentY += dy * invDist * step;
            state.currentZ += dz * invDist * step;
        }
    }

    //==========================================================================
    // Position Access
    //==========================================================================

    /** Get the current (interpolated) position for an input.
        If speed limiting is disabled, returns the target position. */
    void getPosition (int inputIndex, float& x, float& y, float& z) const
    {
        if (inputIndex < 0 || inputIndex >= static_cast<int> (states.size()))
        {
            x = y = z = 0.0f;
            return;
        }

        const auto& state = states[static_cast<size_t> (inputIndex)];
        x = state.currentX;
        y = state.currentY;
        z = state.currentZ;
    }

    /** Get target position for an input (the position we're moving towards) */
    void getTargetPosition (int inputIndex, float& x, float& y, float& z) const
    {
        if (inputIndex < 0 || inputIndex >= static_cast<int> (states.size()))
        {
            x = y = z = 0.0f;
            return;
        }

        const auto& state = states[static_cast<size_t> (inputIndex)];
        x = state.targetX;
        y = state.targetY;
        z = state.targetZ;
    }

    /** Check if any input is currently moving towards its target */
    bool isAnyInputMoving() const { return anyMoving; }

    /** Check if a specific input is moving towards its target */
    bool isInputMoving (int inputIndex) const
    {
        if (inputIndex < 0 || inputIndex >= static_cast<int> (states.size()))
            return false;

        const auto& state = states[static_cast<size_t> (inputIndex)];
        if (!state.active || !state.initialized)
            return false;

        float dx = state.targetX - state.currentX;
        float dy = state.targetY - state.currentY;
        float dz = state.targetZ - state.currentZ;
        float distance = std::sqrt (dx * dx + dy * dy + dz * dz);

        return distance >= 0.001f;
    }

    //==========================================================================
    // Path Mode Methods
    //==========================================================================

    /** Enable or disable path mode for an input */
    void setPathModeEnabled (int inputIndex, bool enabled)
    {
        if (inputIndex < 0 || inputIndex >= static_cast<int> (states.size()))
            return;

        auto& state = states[static_cast<size_t> (inputIndex)];
        state.pathModeEnabled = enabled;
    }

    /** Check if path mode is enabled for an input */
    bool isPathModeEnabled (int inputIndex) const
    {
        if (inputIndex < 0 || inputIndex >= static_cast<int> (states.size()))
            return false;

        return states[static_cast<size_t> (inputIndex)].pathModeEnabled;
    }

    /** Start recording waypoints (call on mouseDown/drag start) */
    void startRecording (int inputIndex)
    {
        if (inputIndex < 0 || inputIndex >= static_cast<int> (states.size()))
            return;

        auto& state = states[static_cast<size_t> (inputIndex)];

        // Clear waypoint queue for fresh recording
        {
            juce::SpinLock::ScopedLockType lock (*state.waypointLock);
            state.waypointHead = 0;
            state.waypointTail = 0;
            state.waypointCount = 0;
        }

        state.isRecording = true;
        state.lastWaypointTime = 0;
    }

    /** Stop recording waypoints (call on mouseUp/drag end) */
    void stopRecording (int inputIndex)
    {
        if (inputIndex < 0 || inputIndex >= static_cast<int> (states.size()))
            return;

        auto& state = states[static_cast<size_t> (inputIndex)];
        state.isRecording = false;
    }

    /** Add a waypoint during recording (rate-limited internally) */
    bool addWaypoint (int inputIndex, float x, float y, float z)
    {
        if (inputIndex < 0 || inputIndex >= static_cast<int> (states.size()))
            return false;

        auto& state = states[static_cast<size_t> (inputIndex)];

        if (!state.isRecording)
            return false;

        // Rate-limit waypoint capture
        juce::int64 now = juce::Time::currentTimeMillis();
        if (now - state.lastWaypointTime < waypointIntervalMs)
            return false;

        state.lastWaypointTime = now;

        juce::SpinLock::ScopedLockType lock (*state.waypointLock);

        // Add waypoint to circular buffer
        state.waypointBuffer[state.waypointHead] = { x, y, z };
        state.waypointHead = (state.waypointHead + 1) % maxWaypoints;

        if (state.waypointCount < maxWaypoints)
        {
            state.waypointCount++;
        }
        else
        {
            // Buffer full, advance tail (drop oldest)
            state.waypointTail = (state.waypointTail + 1) % maxWaypoints;
        }

        return true;
    }

    /** Clear all waypoints for an input */
    void clearWaypoints (int inputIndex)
    {
        if (inputIndex < 0 || inputIndex >= static_cast<int> (states.size()))
            return;

        auto& state = states[static_cast<size_t> (inputIndex)];
        juce::SpinLock::ScopedLockType lock (*state.waypointLock);

        state.waypointHead = 0;
        state.waypointTail = 0;
        state.waypointCount = 0;
    }

    /** Get the number of waypoints queued for an input */
    size_t getWaypointCount (int inputIndex) const
    {
        if (inputIndex < 0 || inputIndex >= static_cast<int> (states.size()))
            return 0;

        const auto& state = states[static_cast<size_t> (inputIndex)];
        juce::SpinLock::ScopedLockType lock (*state.waypointLock);
        return state.waypointCount;
    }

    /** Check if an input is currently recording waypoints */
    bool isRecording (int inputIndex) const
    {
        if (inputIndex < 0 || inputIndex >= static_cast<int> (states.size()))
            return false;

        return states[static_cast<size_t> (inputIndex)].isRecording;
    }

private:
    //==========================================================================
    static constexpr size_t maxWaypoints = 100;

    struct InputState
    {
        // Target position (from ValueTree/OSC)
        float targetX = 0.0f;
        float targetY = 0.0f;
        float targetZ = 0.0f;

        // Current interpolated position
        float currentX = 0.0f;
        float currentY = 0.0f;
        float currentZ = 0.0f;

        // Speed limit parameters
        bool active = false;
        float maxSpeed = 1.0f;  // m/s

        // State tracking
        bool initialized = false;

        // Path mode waypoint queue (circular buffer)
        std::array<Waypoint, maxWaypoints> waypointBuffer;
        size_t waypointHead = 0;  // Next write position
        size_t waypointTail = 0;  // Next read position
        size_t waypointCount = 0;

        // Path mode state
        bool pathModeEnabled = false;
        bool isRecording = false;
        juce::int64 lastWaypointTime = 0;

        // Thread safety for waypoint queue
        std::unique_ptr<juce::SpinLock> waypointLock = std::make_unique<juce::SpinLock>();
    };

    std::vector<InputState> states;
    bool anyMoving = false;

    static constexpr int waypointIntervalMs = 20;  // ~50Hz capture rate
};
