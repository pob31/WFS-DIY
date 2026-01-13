#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSValueTreeState.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "WFSCalculationEngine.h"
#include <cmath>

/**
 * BinauralCalculationEngine
 *
 * Calculates delay, level, and HF attenuation for the binaural virtual speaker pair.
 * Uses the composite input positions from WFSCalculationEngine and renders through
 * a pair of virtual speakers positioned relative to the listener.
 *
 * Virtual Speaker Configuration:
 * - Two speakers at ±10cm from listener center (20cm apart total)
 * - Angled 45° left/right from listener's front-facing direction
 * - On Angle: 135° (full coverage behind speaker)
 * - Off Angle: 30° (mute zone in front of speaker)
 * - HF Shelf: -0.3 dB/m
 */
class BinauralCalculationEngine
{
public:
    struct BinauralOutput
    {
        float delayMs = 0.0f;
        float level = 0.0f;           // linear 0-1
        float hfAttenuationDb = 0.0f;
    };

    struct BinauralPair
    {
        BinauralOutput left;
        BinauralOutput right;
    };

    BinauralCalculationEngine (WFSValueTreeState& params, WFSCalculationEngine& wfsCalc)
        : valueTreeState (params)
        , wfsCalcEngine (wfsCalc)
    {
        recalculatePositions();
    }

    /**
     * Get binaural output parameters for an input channel.
     * Uses composite position from WFSCalculationEngine.
     */
    BinauralPair calculate (int inputIndex) const
    {
        using namespace WFSParameterDefaults;

        BinauralPair result;

        // Get input position from WFS engine
        auto inputPos = wfsCalcEngine.getCompositeInputPosition (inputIndex);

        // Get binaural parameters
        float attenOffset = getBinauralAttenuation();
        float delayOffset = getBinauralDelay();

        // Calculate for left speaker
        result.left = calculateForSpeaker (inputPos, leftSpeakerPos, leftSpeakerOrientation);
        result.left.delayMs += delayOffset;
        result.left.level *= std::pow (10.0f, attenOffset / 20.0f);

        // Calculate for right speaker
        result.right = calculateForSpeaker (inputPos, rightSpeakerPos, rightSpeakerOrientation);
        result.right.delayMs += delayOffset;
        result.right.level *= std::pow (10.0f, attenOffset / 20.0f);

        return result;
    }

    /**
     * Check if an input is currently soloed.
     */
    bool isInputSoloed (int inputIndex) const
    {
        return valueTreeState.isInputSoloed (inputIndex);
    }

    /**
     * Check if multi-solo mode is active.
     */
    bool isMultiSoloMode() const
    {
        return valueTreeState.getBinauralSoloMode() == 1;
    }

    /**
     * Get the binaural output channel (-1 = disabled).
     */
    int getBinauralOutputChannel() const
    {
        return valueTreeState.getBinauralOutputChannel();
    }

    /**
     * Get binaural attenuation in dB.
     */
    float getBinauralAttenuation() const
    {
        auto binaural = valueTreeState.getBinauralState();
        if (binaural.isValid())
            return (float) binaural.getProperty (WFSParameterIDs::binauralAttenuation,
                                                  WFSParameterDefaults::binauralAttenuationDefault);
        return WFSParameterDefaults::binauralAttenuationDefault;
    }

    /**
     * Get binaural delay in ms.
     */
    float getBinauralDelay() const
    {
        auto binaural = valueTreeState.getBinauralState();
        if (binaural.isValid())
            return (float) binaural.getProperty (WFSParameterIDs::binauralDelay,
                                                  WFSParameterDefaults::binauralDelayDefault);
        return WFSParameterDefaults::binauralDelayDefault;
    }

    /**
     * Get number of currently soloed inputs.
     */
    int getNumSoloedInputs() const
    {
        return valueTreeState.getNumSoloedInputs();
    }

    /**
     * Recalculate listener and speaker positions when parameters change.
     */
    void recalculatePositions()
    {
        using namespace WFSParameterDefaults;

        auto binaural = valueTreeState.getBinauralState();
        if (!binaural.isValid())
            return;

        // Get listener parameters
        float distance = (float) binaural.getProperty (WFSParameterIDs::binauralListenerDistance,
                                                        binauralListenerDistanceDefault);
        int angleDeg = (int) binaural.getProperty (WFSParameterIDs::binauralListenerAngle,
                                                    binauralListenerAngleDefault);

        // Convert angle to radians (0° = facing origin/stage, positive = clockwise)
        float angleRad = juce::degreesToRadians ((float) angleDeg);

        // Calculate listener position (at distance from origin, facing origin)
        // Listener looks toward origin, so position is on the negative Y axis (audience side)
        listenerPosition.x = distance * std::sin (angleRad);
        listenerPosition.y = -distance * std::cos (angleRad);  // Negative = audience side
        listenerPosition.z = 1.5f;  // Ear height

        // Listener's forward direction (toward origin)
        float forwardAngle = angleRad + juce::MathConstants<float>::pi;  // 180° flip to face origin

        // Calculate virtual speaker positions (±10cm from listener center, angled 45°)
        float halfSpacing = binauralSpeakerSpacing / 2.0f;  // 0.10m
        float speakerAngleRad = juce::degreesToRadians (binauralSpeakerAngle);  // 45°

        // Left speaker: 45° left of forward direction
        float leftAngle = forwardAngle - speakerAngleRad;
        leftSpeakerPos.x = listenerPosition.x - halfSpacing * std::cos (forwardAngle);
        leftSpeakerPos.y = listenerPosition.y - halfSpacing * std::sin (forwardAngle);
        leftSpeakerPos.z = listenerPosition.z;
        leftSpeakerOrientation = leftAngle;

        // Right speaker: 45° right of forward direction
        float rightAngle = forwardAngle + speakerAngleRad;
        rightSpeakerPos.x = listenerPosition.x + halfSpacing * std::cos (forwardAngle);
        rightSpeakerPos.y = listenerPosition.y + halfSpacing * std::sin (forwardAngle);
        rightSpeakerPos.z = listenerPosition.z;
        rightSpeakerOrientation = rightAngle;
    }

private:
    using Position = WFSCalculationEngine::Position;

    /**
     * Calculate binaural output for one virtual speaker.
     */
    BinauralOutput calculateForSpeaker (const Position& inputPos,
                                         const Position& speakerPos,
                                         float speakerOrientation) const
    {
        using namespace WFSParameterDefaults;

        BinauralOutput output;

        // Calculate distance from input to speaker
        float dx = inputPos.x - speakerPos.x;
        float dy = inputPos.y - speakerPos.y;
        float dz = inputPos.z - speakerPos.z;
        float distance = std::sqrt (dx * dx + dy * dy + dz * dz);

        if (distance < 0.01f)
            distance = 0.01f;  // Prevent division by zero

        // Calculate delay (distance / speed of sound)
        // We use a simplified model where delay is proportional to distance
        // (not WFS delay which subtracts listener-to-speaker distance)
        float speedOfSound = 343.0f;  // m/s
        output.delayMs = (distance / speedOfSound) * 1000.0f;

        // Calculate angular attenuation using keystone pattern
        output.level = calculateAngularAttenuation (inputPos, speakerPos, speakerOrientation);

        // Apply distance attenuation (100% nominal = linear law)
        // Use -6dB per doubling of distance (inverse square approximation)
        float referenceDistance = 1.0f;
        if (distance > referenceDistance)
        {
            float distanceAttenDb = -20.0f * std::log10 (distance / referenceDistance);
            output.level *= std::pow (10.0f, distanceAttenDb / 20.0f);
        }

        // Calculate HF attenuation
        output.hfAttenuationDb = distance * binauralHFShelfPerMeter;  // -0.3 dB/m

        return output;
    }

    /**
     * Calculate angular attenuation based on keystone pattern.
     * Returns 1.0 when input is in the speaker's coverage zone,
     * 0.0 when in the mute zone, with smooth transition between.
     */
    float calculateAngularAttenuation (const Position& inputPos,
                                        const Position& speakerPos,
                                        float speakerOrientation) const
    {
        using namespace WFSParameterDefaults;

        // Vector from speaker to input
        float dx = inputPos.x - speakerPos.x;
        float dy = inputPos.y - speakerPos.y;
        float dz = inputPos.z - speakerPos.z;
        float dist = std::sqrt (dx * dx + dy * dy + dz * dz);

        if (dist < 0.001f)
            return 1.0f;

        // Speaker's rear-pointing axis (opposite of speaker direction)
        // Speaker faces toward speakerOrientation, rear is +180°
        float rearAngle = speakerOrientation + juce::MathConstants<float>::pi;
        float rearDirX = std::cos (rearAngle);
        float rearDirY = std::sin (rearAngle);
        float rearDirZ = 0.0f;

        // Normalize input direction
        float inputDirX = dx / dist;
        float inputDirY = dy / dist;
        float inputDirZ = dz / dist;

        // Dot product to get cosine of angle between rear axis and input direction
        float dotProduct = rearDirX * inputDirX + rearDirY * inputDirY + rearDirZ * inputDirZ;
        dotProduct = juce::jlimit (-1.0f, 1.0f, dotProduct);

        // Angle from rear axis (0 = directly behind speaker, π = directly in front)
        float angle = std::acos (dotProduct);

        // Convert angle limits to radians
        float onAngleRad = juce::degreesToRadians ((float) binauralOnAngle);   // 135°
        float offAngleRad = juce::degreesToRadians ((float) binauralOffAngle); // 30°
        float muteAngle = juce::MathConstants<float>::pi - offAngleRad;        // Where full mute begins

        // Calculate attenuation based on angle zones
        if (angle <= onAngleRad)
        {
            // Full coverage zone (behind speaker within on-angle)
            return 1.0f;
        }
        else if (angle >= muteAngle)
        {
            // Mute zone (in front of speaker within off-angle)
            return 0.0f;
        }
        else
        {
            // Transition zone - linear interpolation
            float progress = (angle - onAngleRad) / (muteAngle - onAngleRad);
            return 1.0f - progress;
        }
    }

    WFSValueTreeState& valueTreeState;
    WFSCalculationEngine& wfsCalcEngine;

    // Listener and virtual speaker positions
    Position listenerPosition;
    Position leftSpeakerPos;
    Position rightSpeakerPos;
    float leftSpeakerOrientation = 0.0f;   // radians
    float rightSpeakerOrientation = 0.0f;  // radians
};
