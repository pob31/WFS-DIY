#pragma once

#include <JuceHeader.h>
#include <cmath>

/**
 * Input Speed Limiter
 *
 * Provides smooth speed-limited movement for input positions with tanh-based
 * acceleration/deceleration. Target positions are interpolated to create natural
 * movement that accelerates smoothly from rest and decelerates when approaching
 * the target.
 *
 * Processing is performed at 50Hz (called from MainComponent timer).
 * The speed limiter sits BEFORE flip/offset/LFO in the position chain.
 */
class InputSpeedLimiter
{
public:
    InputSpeedLimiter() = default;

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

            // Calculate vector from current to target
            float dx = state.targetX - state.currentX;
            float dy = state.targetY - state.currentY;
            float dz = state.targetZ - state.currentZ;
            float distance = std::sqrt (dx * dx + dy * dy + dz * dz);

            // Snap threshold - close enough to target
            constexpr float snapThreshold = 0.001f;
            if (distance < snapThreshold)
            {
                state.currentX = state.targetX;
                state.currentY = state.targetY;
                state.currentZ = state.targetZ;
                continue;
            }

            // Mark that we're still moving
            anyMoving = true;

            // Maximum distance we can travel this frame
            float maxStep = state.maxSpeed * deltaTime;

            // Tanh smoothing for natural acceleration/deceleration
            // When far from target: normalizedDist is large, tanh(x)/x → 1 (full speed)
            // When near target: normalizedDist is small, tanh(x)/x → 1 but step is limited by distance
            // The factor 5.0 controls the smoothing range (larger = softer acceleration)
            constexpr float smoothingFactor = 5.0f;
            float normalizedDist = distance / (maxStep * smoothingFactor);

            float speedScale;
            if (normalizedDist > 0.001f)
            {
                // tanh(x)/x provides smooth S-curve:
                // - Far from target: approaches 1.0 (full speed)
                // - Near target: tanh(x) ≈ x, so tanh(x)/x ≈ 1.0
                // The limiting happens naturally via min(distance, maxStep)
                speedScale = std::tanh (normalizedDist) / normalizedDist;
            }
            else
            {
                speedScale = 1.0f;
            }

            // Calculate actual step (limited by both speed and remaining distance)
            float step = std::min (distance, maxStep * speedScale);

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

private:
    //==========================================================================
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
    };

    std::vector<InputState> states;
    bool anyMoving = false;
};
