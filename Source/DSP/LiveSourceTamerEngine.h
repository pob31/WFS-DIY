#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSValueTreeState.h"
#include "../Parameters/WFSParameterIDs.h"
#include "WFSCalculationEngine.h"
#include <vector>
#include <cmath>

/**
 * LiveSourceTamerEngine
 *
 * Control-rate engine for Live Source Tamer feature.
 * Calculates per-speaker gain reduction based on:
 * 1. Distance from input to output (within radius)
 * 2. Shape curve (linear, log, square, sine)
 * 3. Fixed attenuation setting
 * 4. Dynamic gain reduction from peak and slow detectors
 *
 * Call process() at 50Hz (every 4 timer ticks) to update LS gains.
 * The gains are then applied in WFSCalculationEngine during level calculation.
 *
 * Activation conditions:
 * - inputLSactive must be true (master enable per input)
 * - Output must be within inputLSradius of input
 * - outputLSattenEnable must be non-zero (per-output bypass)
 */
class LiveSourceTamerEngine
{
public:
    LiveSourceTamerEngine(WFSValueTreeState& state,
                          WFSCalculationEngine& calcEngine,
                          int numInputChannels,
                          int numOutputChannels)
        : valueTreeState(state),
          calculationEngine(calcEngine),
          numInputs(numInputChannels),
          numOutputs(numOutputChannels)
    {
        // Allocate LS gains array (one per input-output routing)
        lsGains.resize(static_cast<size_t>(numInputs * numOutputs), 1.0f);

        // Initialize ramp state per input (start at 0 = inactive)
        rampProgress.resize(static_cast<size_t>(numInputs), 0.0f);
    }

    /**
     * Process LS gains at control rate.
     * Call this at ~50Hz (every 4 timer ticks).
     *
     * @param peakGRs Peak gain reductions per input (linear 0-1), size = numInputs
     * @param slowGRs Slow gain reductions per input (linear 0-1), size = numInputs
     */
    void process(const std::vector<float>& peakGRs,
                 const std::vector<float>& slowGRs)
    {
        using namespace WFSParameterIDs;

        // Ramp increment per process() call: 500ms at 50Hz = 25 ticks
        constexpr float rampIncrement = 1.0f / 25.0f;  // 0.04 per tick

        for (int inIdx = 0; inIdx < numInputs; ++inIdx)
        {
            // Get LS section for this input
            auto lsSection = valueTreeState.getInputLiveSourceSection(inIdx);

            // Check master enable
            bool lsActive = static_cast<int>(lsSection.getProperty(inputLSactive, 0)) != 0;

            // Update ramp progress based on active state
            // Ramp towards 1.0 when active, towards 0.0 when inactive
            float& ramp = rampProgress[static_cast<size_t>(inIdx)];
            if (lsActive)
            {
                // Ramping in
                if (ramp < 1.0f)
                {
                    ramp += rampIncrement;
                    if (ramp > 1.0f)
                        ramp = 1.0f;
                }
            }
            else
            {
                // Ramping out
                if (ramp > 0.0f)
                {
                    ramp -= rampIncrement;
                    if (ramp < 0.0f)
                        ramp = 0.0f;
                }
            }

            // If ramp is 0, no LS effect at all - skip calculations
            if (ramp <= 0.0f)
            {
                for (int outIdx = 0; outIdx < numOutputs; ++outIdx)
                {
                    size_t matrixIdx = static_cast<size_t>(inIdx * numOutputs + outIdx);
                    lsGains[matrixIdx] = 1.0f;
                }
                continue;
            }

            // Get LS parameters
            float radius = lsSection.getProperty(inputLSradius, 2.0f);
            int shape = lsSection.getProperty(inputLSshape, 0);
            float fixedAttenDb = lsSection.getProperty(inputLSattenuation, -6.0f);

            // Convert fixed attenuation from dB to linear
            float fixedAttenLinear = std::pow(10.0f, fixedAttenDb / 20.0f);

            // Get composite input position (includes speed-limiting, flip, offset, LFO)
            auto inputPos = calculationEngine.getCompositeInputPosition(inIdx);

            // Get dynamic gain reductions
            float peakGR = (inIdx < static_cast<int>(peakGRs.size())) ? peakGRs[inIdx] : 1.0f;
            float slowGR = (inIdx < static_cast<int>(slowGRs.size())) ? slowGRs[inIdx] : 1.0f;

            // Process each output
            for (int outIdx = 0; outIdx < numOutputs; ++outIdx)
            {
                size_t matrixIdx = static_cast<size_t>(inIdx * numOutputs + outIdx);

                // Check per-output LS enable
                auto outputOptions = valueTreeState.getOutputOptionsSection(outIdx);
                int outputLSEnable = outputOptions.getProperty(outputLSattenEnable, 1);

                if (outputLSEnable == 0)
                {
                    // LS bypassed for this output
                    lsGains[matrixIdx] = 1.0f;
                    continue;
                }

                // Get speaker position
                auto speakerPos = calculationEngine.getSpeakerPosition(outIdx);

                // Calculate distance from input to speaker
                float dx = speakerPos.x - inputPos.x;
                float dy = speakerPos.y - inputPos.y;
                float dz = speakerPos.z - inputPos.z;
                float distance = std::sqrt(dx * dx + dy * dy + dz * dz);

                // Normalize distance by radius
                float normalizedDist = distance / radius;

                // Check if outside radius - no LS effect
                if (normalizedDist >= 1.0f)
                {
                    lsGains[matrixIdx] = 1.0f;
                    continue;
                }

                // Calculate shape factor (attenuation amount at this distance)
                // shapeFactor = 1.0 at center, 0.0 at edge
                float shapeFactor = calculateShapeFactor(normalizedDist, shape);

                // Calculate final LS gain
                // At center (shapeFactor=1): full attenuation (fixedAtten * peakGR * slowGR)
                // At edge (shapeFactor=0): no attenuation (1.0)
                // Formula: gain = 1.0 - shapeFactor * (1.0 - combinedAtten)
                //        = shapeFactor * combinedAtten + (1.0 - shapeFactor) * 1.0
                //        = lerp(1.0, combinedAtten, shapeFactor)

                float combinedAtten = fixedAttenLinear * peakGR * slowGR;
                float targetGain = 1.0f - shapeFactor * (1.0f - combinedAtten);

                // Apply enable ramp: lerp from 1.0 to targetGain over 500ms
                float lsGain = 1.0f + ramp * (targetGain - 1.0f);

                lsGains[matrixIdx] = lsGain;
            }
        }
    }

    /**
     * Get pointer to LS gains array.
     * Index: [inputIndex * numOutputs + outputIndex]
     * Values are linear multipliers (0-1).
     */
    const float* getLSGains() const
    {
        return lsGains.data();
    }

    /**
     * Get LS gain for specific routing.
     */
    float getLSGain(int inputIndex, int outputIndex) const
    {
        if (inputIndex >= 0 && inputIndex < numInputs &&
            outputIndex >= 0 && outputIndex < numOutputs)
        {
            return lsGains[static_cast<size_t>(inputIndex * numOutputs + outputIndex)];
        }
        return 1.0f;
    }

    /**
     * Check if any input is currently ramping (either in or out).
     * Used to determine if matrix recalculation is needed during transitions.
     */
    bool isAnyInputRamping() const
    {
        for (int i = 0; i < numInputs; ++i)
        {
            float ramp = rampProgress[static_cast<size_t>(i)];
            // Ramping if not at 0 or 1
            if (ramp > 0.0f && ramp < 1.0f)
                return true;
        }
        return false;
    }

    /**
     * Check if any input has non-zero ramp (active or ramping out).
     * Used to determine if matrix recalculation is needed.
     */
    bool isAnyInputActive() const
    {
        for (int i = 0; i < numInputs; ++i)
        {
            if (rampProgress[static_cast<size_t>(i)] > 0.0f)
                return true;
        }
        return false;
    }

    /**
     * Mark positions as dirty (call when input/output positions change).
     * This doesn't affect LS gains directly since we recalculate every frame,
     * but can be used to trigger immediate recalculation if needed.
     */
    void markPositionsDirty()
    {
        // Currently we recalculate every process() call, so no caching needed
    }

private:
    /**
     * Calculate shape factor based on normalized distance and shape type.
     *
     * @param t Normalized distance (0 = center, 1 = edge)
     * @param shape Shape type: 0=linear, 1=log, 2=square, 3=sine
     * @return Shape factor (1 = full attenuation, 0 = no attenuation)
     */
    float calculateShapeFactor(float t, int shape)
    {
        // Clamp t to [0, 1]
        t = juce::jlimit(0.0f, 1.0f, t);

        switch (shape)
        {
            case 1:  // Log: 1 - log10(1 + 9*t)
                return 1.0f - std::log10(1.0f + 9.0f * t);

            case 2:  // Square (d^2): 1 - t^2
                return 1.0f - t * t;

            case 3:  // Sine: 0.5 + 0.5*cos(t*pi)
                return 0.5f + 0.5f * std::cos(t * juce::MathConstants<float>::pi);

            default: // Linear (case 0): 1 - t
                return 1.0f - t;
        }
    }

    WFSValueTreeState& valueTreeState;
    WFSCalculationEngine& calculationEngine;

    int numInputs;
    int numOutputs;

    // LS gains per routing [inputIndex * numOutputs + outputIndex]
    std::vector<float> lsGains;

    // Ramp state for smooth enable/disable transition (500ms)
    // 0.0 = fully inactive, 1.0 = fully active
    std::vector<float> rampProgress;
};
