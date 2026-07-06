#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSValueTreeState.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "../../spatcore/rt/RtSnapshot.h"
#include "WFSCalculationEngine.h"
#include <atomic>
#include <cmath>
#include <cstdint>

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
 *
 * Threading model:
 * - The message thread publishes an RtParams snapshot via refreshRtSnapshot()
 *   (called from MainComponent's timer). It is the only place the ValueTree is read.
 * - The realtime BinauralProcessor thread copies the snapshot once per block via
 *   getRtParams() and never touches the ValueTree or allocates.
 * - The audio callback reads only the lock-free rtOutputChannel atomic.
 */
class BinauralCalculationEngine
{
public:
    using Position = WFSCalculationEngine::Position;

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

    /**
     * POD snapshot of everything the realtime thread needs for one block.
     * Published by the message thread (refreshRtSnapshot), copied by the RT
     * worker via spatcore::rt::RtSnapshot (getRtParams). Trivially copyable —
     * no ValueTree access, no heap allocation on the RT side.
     */
    struct RtParams
    {
        Position leftSpeakerPos;
        Position rightSpeakerPos;
        float leftSpeakerOrientation  = 0.0f;   // radians
        float rightSpeakerOrientation = 0.0f;   // radians
        float attenLinear   = 1.0f;             // 10^(binauralAttenuation/20), precomputed on publish
        float delayOffsetMs = 0.0f;
        std::uint64_t soloMask = 0;             // bit i set = input i soloed
        int numSoloed = 0;

        bool isSoloed (int inputIndex) const noexcept
        {
            return inputIndex >= 0 && inputIndex < 64
                && ((soloMask >> inputIndex) & 1ull) != 0;
        }
    };

    static_assert (WFSParameterDefaults::maxInputChannels <= 64,
                   "RtParams::soloMask is a 64-bit bitmask");

    BinauralCalculationEngine (WFSValueTreeState& params, WFSCalculationEngine& wfsCalc)
        : valueTreeState (params)
        , wfsCalcEngine (wfsCalc)
    {
        refreshRtSnapshot();
    }

    /**
     * Get binaural output parameters for an input channel.
     * Uses composite position from WFSCalculationEngine and the caller's RtParams
     * snapshot. RT-safe: no ValueTree access, no allocation.
     */
    BinauralPair calculate (int inputIndex, const RtParams& rt) const
    {
        BinauralPair result;

        // Get input position from WFS engine (positionLock-guarded cache, tree-free)
        auto inputPos = wfsCalcEngine.getCompositeInputPosition (inputIndex);

        // Calculate for left speaker
        result.left = calculateForSpeaker (inputPos, rt.leftSpeakerPos, rt.leftSpeakerOrientation);
        result.left.delayMs += rt.delayOffsetMs;
        result.left.level *= rt.attenLinear;

        // Calculate for right speaker
        result.right = calculateForSpeaker (inputPos, rt.rightSpeakerPos, rt.rightSpeakerOrientation);
        result.right.delayMs += rt.delayOffsetMs;
        result.right.level *= rt.attenLinear;

        return result;
    }

    /**
     * Copy the current RT snapshot. Called once per block by the realtime
     * BinauralProcessor thread. The lock window is a single struct copy —
     * same cost class as BinauralProcessor's sharedInputsLock.
     */
    RtParams getRtParams() const
    {
        return rtSnapshot.acquire();
    }

    /**
     * Get the binaural output channel (-1 = disabled).
     * Lock-free relaxed atomic — safe to call from the audio device callback.
     */
    int getBinauralOutputChannel() const
    {
        return rtOutputChannel.load (std::memory_order_relaxed);
    }

    /**
     * Recompute listener/speaker geometry from the ValueTree and publish a fresh
     * RtParams snapshot for the realtime thread. MESSAGE THREAD ONLY — this is
     * the single place the binaural parameters are read from the ValueTree.
     * Called every timer tick from MainComponent and before the RT worker is
     * (re)enabled, so the worker never observes an unpublished snapshot.
     */
    void refreshRtSnapshot()
    {
        JUCE_ASSERT_MESSAGE_THREAD

        // Update the message-thread geometry scratch (listener/speaker positions)
        recalculatePositions();

        // Build the snapshot locally, outside the lock
        RtParams fresh;
        fresh.leftSpeakerPos          = leftSpeakerPos;
        fresh.rightSpeakerPos         = rightSpeakerPos;
        fresh.leftSpeakerOrientation  = leftSpeakerOrientation;
        fresh.rightSpeakerOrientation = rightSpeakerOrientation;
        fresh.attenLinear             = std::pow (10.0f, getBinauralAttenuation() / 20.0f);
        fresh.delayOffsetMs           = getBinauralDelay();

        int outputChannel = WFSParameterDefaults::binauralOutputChannelDefault;

        auto binaural = valueTreeState.getBinauralState();
        if (binaural.isValid())
        {
            outputChannel = (int) binaural.getProperty (WFSParameterIDs::binauralOutputChannel,
                                                        WFSParameterDefaults::binauralOutputChannelDefault);

            // Parse the solo CSV here, on the message thread, so the RT thread
            // never touches Strings. Matches WFSValueTreeState::isInputSoloed semantics.
            juce::String soloStates = binaural.getProperty (WFSParameterIDs::inputSoloStates,
                                                            juce::String()).toString();
            juce::StringArray states;
            states.addTokens (soloStates, ",", "");

            const int numStates = juce::jmin (states.size(), 64);
            for (int i = 0; i < numStates; ++i)
            {
                if (states[i] == "1")
                {
                    fresh.soloMask |= (1ull << i);
                    ++fresh.numSoloed;
                }
            }
        }

        // Publish: the critical section is exactly one struct copy
        rtSnapshot.publish (fresh);
        rtOutputChannel.store (outputChannel, std::memory_order_relaxed);
    }

private:
    /**
     * Get binaural attenuation in dB.
     * Message-thread publish-side scratch — reads the ValueTree; do not call from RT threads.
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
     * Message-thread publish-side scratch — reads the ValueTree; do not call from RT threads.
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
     * Recalculate listener and speaker positions when parameters change.
     * Message-thread publish-side scratch (members below are snapshot sources);
     * the RT thread consumes them only via the published RtParams copy.
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

        // Calculate virtual speaker positions (±10cm from listener center, angled 45°)
        float halfSpacing = binauralSpeakerSpacing / 2.0f;  // 0.10m
        float speakerAngleRad = juce::degreesToRadians (binauralSpeakerAngle);  // 45°

        // Speaker orientations: -listenerAngle ± 45° (stereo pair always covers the stage)
        leftSpeakerOrientation = -angleRad + speakerAngleRad;   // -listenerAngle + 45°
        rightSpeakerOrientation = -angleRad - speakerAngleRad;  // -listenerAngle - 45°

        // Speaker positions: perpendicular to facing direction
        // Listener's right direction in WFS convention at angle α is (cos(α), sin(α))
        leftSpeakerPos.x = listenerPosition.x - halfSpacing * std::cos (angleRad);
        leftSpeakerPos.y = listenerPosition.y - halfSpacing * std::sin (angleRad);
        leftSpeakerPos.z = listenerPosition.z;

        rightSpeakerPos.x = listenerPosition.x + halfSpacing * std::cos (angleRad);
        rightSpeakerPos.y = listenerPosition.y + halfSpacing * std::sin (angleRad);
        rightSpeakerPos.z = listenerPosition.z;
    }

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
        // WFS convention: angle θ → direction (sin(θ), -cos(θ))
        float rearDirX = std::sin (rearAngle);
        float rearDirY = -std::cos (rearAngle);
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

    // Listener and virtual speaker positions — message-thread scratch, snapshot
    // sources for refreshRtSnapshot(). The RT thread reads only the RtParams copy.
    Position listenerPosition;
    Position leftSpeakerPos;
    Position rightSpeakerPos;
    float leftSpeakerOrientation = 0.0f;   // radians
    float rightSpeakerOrientation = 0.0f;  // radians

    // RT snapshot: published by the message thread, copied per block by the RT worker.
    // spatcore::rt::RtSnapshot was modeled on this engine's original hand-rolled
    // RtParams+SpinLock publish/acquire; this now uses the shared primitive.
    spatcore::rt::RtSnapshot<RtParams> rtSnapshot;
    std::atomic<int> rtOutputChannel { WFSParameterDefaults::binauralOutputChannelDefault };
};
