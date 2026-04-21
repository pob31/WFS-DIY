#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSValueTreeState.h"

#include <map>
#include <utility>
#include <vector>

namespace WFSNetwork
{

/**
 * OSCParameterRamper
 *
 * Drives smooth transitions on input parameters when an OSC message
 * carries a third "transition time in seconds" argument. Each ramp
 * linearly interpolates from the ValueTree's current value at the
 * time of message receipt to the new target over the given duration,
 * stepped at 50 Hz.
 */
class OSCParameterRamper
{
public:
    explicit OSCParameterRamper (WFSValueTreeState& s) : state (s) {}

    /** Start (or replace) a ramp for one input parameter on one channel.
        @param channelIndex 0-based channel index.
        @param paramId      Input parameter identifier.
        @param targetValue  Target value to reach at the end of the ramp.
        @param rampTimeSec  Ramp duration in seconds (minimum 1 ms). */
    void startRamp (int channelIndex,
                    const juce::Identifier& paramId,
                    double targetValue,
                    float rampTimeSec)
    {
        if (channelIndex < 0)
            return;

        rampTimeSec = juce::jmax (0.001f, rampTimeSec);

        // Snapshot the current value so interpolation starts from there.
        const juce::var currentVar = state.getInputParameter (channelIndex, paramId);
        const double startValue = currentVar.isVoid()
            ? targetValue
            : static_cast<double> (currentVar);

        RampState r;
        r.startValue  = startValue;
        r.targetValue = targetValue;
        r.elapsed     = 0.0f;
        r.total       = rampTimeSec;

        const Key key { channelIndex, paramId.toString() };
        const juce::ScopedLock sl (lock);
        activeRamps[key] = r;
    }

    /** Step all active ramps. Called from the 50 Hz timer on the message thread. */
    void process (float deltaTimeSeconds)
    {
        std::vector<std::tuple<int, juce::String, double>> writes;

        {
            const juce::ScopedLock sl (lock);
            for (auto it = activeRamps.begin(); it != activeRamps.end(); )
            {
                auto& r = it->second;
                r.elapsed += deltaTimeSeconds;

                if (r.elapsed >= r.total)
                {
                    writes.emplace_back (it->first.first, it->first.second, r.targetValue);
                    it = activeRamps.erase (it);
                }
                else
                {
                    const float progress = r.elapsed / r.total;
                    const double v = r.startValue + (r.targetValue - r.startValue) * progress;
                    writes.emplace_back (it->first.first, it->first.second, v);
                    ++it;
                }
            }
        }

        // Apply writes outside the lock to avoid holding it during ValueTree
        // listener notification chains.
        for (auto& [ch, name, v] : writes)
            state.setInputParameter (ch, juce::Identifier (name), juce::var (v));
    }

    /** Cancel a specific ramp without writing its final target. */
    void cancel (int channelIndex, const juce::Identifier& paramId)
    {
        const Key key { channelIndex, paramId.toString() };
        const juce::ScopedLock sl (lock);
        activeRamps.erase (key);
    }

    void cancelAll()
    {
        const juce::ScopedLock sl (lock);
        activeRamps.clear();
    }

    /** True when the given channel+param has a ramp in progress. */
    bool isRamping (int channelIndex, const juce::Identifier& paramId) const
    {
        const Key key { channelIndex, paramId.toString() };
        const juce::ScopedLock sl (lock);
        return activeRamps.find (key) != activeRamps.end();
    }

private:
    using Key = std::pair<int, juce::String>;

    struct RampState
    {
        double startValue  = 0.0;
        double targetValue = 0.0;
        float  elapsed     = 0.0f;
        float  total       = 1.0f;
    };

    WFSValueTreeState& state;
    std::map<Key, RampState> activeRamps;
    juce::CriticalSection lock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSCParameterRamper)
};

} // namespace WFSNetwork
