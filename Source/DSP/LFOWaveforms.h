#pragma once

#include <cmath>
#include <JuceHeader.h>

/**
 * Shared LFO waveform shapes and generation utility.
 * Used by both LFOProcessor (per-input) and ClusterLFOProcessor (per-cluster).
 */
namespace LFOWaveforms
{
    //==========================================================================
    // Waveform Shape Enumeration
    //==========================================================================
    enum Shape
    {
        Off = 0,
        Sine = 1,
        Square = 2,
        Sawtooth = 3,
        Triangle = 4,
        Keystone = 5,
        Log = 6,
        Exp = 7,
        Random = 8
    };

    //==========================================================================
    // Waveform Generation
    //==========================================================================

    /**
     * Apply waveform shape to ramp value
     * @param shape Waveform shape (0-8)
     * @param ramp Normalized ramp value (0->1)
     * @param lastRandom Previous random target (for ramping)
     * @param targetRandom Current random target
     * @return Output value (-1 to +1)
     */
    inline float applyWaveform (int shape, float ramp, float lastRandom, float targetRandom)
    {
        switch (shape)
        {
            case Off:
                return 0.0f;

            case Sine:
                // -cos(2pi * r) gives sine starting at -1, going to +1 at 0.5, back to -1
                return -std::cos (juce::MathConstants<float>::twoPi * ramp);

            case Square:
                // Jump between -1 and +1 at midpoint
                return ramp < 0.5f ? -1.0f : 1.0f;

            case Sawtooth:
                // Ramp from -1 to +1
                return 2.0f * ramp - 1.0f;

            case Triangle:
                // Ramp up to +1 then down to -1
                return ramp < 0.5f
                           ? 4.0f * ramp - 1.0f
                           : 3.0f - 4.0f * ramp;

            case Keystone:
                // Plateau at ends, ramp in middle (0.25 threshold)
                // 0.00-0.25: hold at -1
                // 0.25-0.50: ramp from -1 to +1
                // 0.50-0.75: hold at +1
                // 0.75-1.00: ramp from +1 to -1
                if (ramp < 0.25f)
                    return -1.0f;
                else if (ramp < 0.5f)
                    return (ramp - 0.25f) * 8.0f - 1.0f;
                else if (ramp < 0.75f)
                    return 1.0f;
                else
                    return 1.0f - (ramp - 0.75f) * 8.0f;

            case Log:
                // Logarithmic curve: 2*log10(20*r + 1) - 1, normalized
                {
                    float logVal = 2.0f * std::log10 (20.0f * ramp + 1.0f) - 1.0f;
                    return juce::jmap (logVal, -1.0f, 1.644f, -1.0f, 1.0f);
                }

            case Exp:
                // Exponential curve using pow(3, r*2)
                {
                    float expVal = std::pow (3.0f, ramp * 2.0f);
                    return juce::jmap (expVal, 1.0f, 9.0f, -1.0f, 1.0f);
                }

            case Random:
                // Smoothly ramp from last random target to current random target
                return lastRandom + (targetRandom - lastRandom) * ramp;

            default:
                return 0.0f;
        }
    }

} // namespace LFOWaveforms
