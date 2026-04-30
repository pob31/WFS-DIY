#pragma once

#include <JuceHeader.h>
#include <cmath>

namespace WFSHelpers
{

/**
 * Defensive clamp: like juce::jlimit, but tolerant of corrupt bounds.
 *
 * juce::jlimit asserts (lowerLimit <= upperLimit) — which fails for any
 * NaN/Inf operand. When stage geometry, constraint settings, or any
 * other ValueTree-derived bound has been poisoned (e.g. by a corrupted
 * config file loaded from disk before our OSC sanitisation gate was
 * deployed), we'd rather pass the value through unclamped than crash
 * the renderer/audio thread.
 *
 * Returns value unchanged when either limit is non-finite or the
 * interval is inverted; otherwise behaves like juce::jlimit.
 */
inline float safeClamp (float lowerLimit, float upperLimit, float value) noexcept
{
    if (! std::isfinite (lowerLimit) || ! std::isfinite (upperLimit) || lowerLimit > upperLimit)
        return value;
    return juce::jlimit (lowerLimit, upperLimit, value);
}

} // namespace WFSHelpers
