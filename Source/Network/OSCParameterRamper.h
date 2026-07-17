#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSValueTreeState.h"

#include <map>
#include <set>
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

    /** Step all active ramps. Called from the ~50 Hz timer on the message
        thread. Progress is computed from wall-clock elapsed time rather than
        an assumed tick length, so ramp durations stay accurate even when the
        timer runs late (message-thread congestion under heavy GUI load was
        observed stretching the nominal 20 ms tick to 100 ms and more, which
        made ramps take several times the requested duration). */
    void process()
    {
        const double nowMs = juce::Time::getMillisecondCounterHiRes();
        float deltaTimeSeconds = 0.02f;
        if (lastProcessMs > 0.0)
            deltaTimeSeconds = juce::jlimit (0.0f, 0.5f,
                static_cast<float> ((nowMs - lastProcessMs) * 0.001));
        lastProcessMs = nowMs;

        std::vector<std::tuple<int, juce::String, double>> writes;

        {
            const juce::ScopedLock sl (lock);
            for (auto it = activeRamps.begin(); it != activeRamps.end(); )
            {
                auto& r = it->second;

                // Takeover detection: if someone else (an instant OSC set, a
                // GUI dial, a snapshot recall) wrote this parameter since our
                // previous step (or since the ramp was started), the newer
                // write wins — drop the ramp instead of overwriting it.
                {
                    const juce::var current = state.getInputParameter (
                        it->first.first, juce::Identifier (it->first.second));
                    const double reference = r.hasWritten ? r.lastWritten : r.startValue;
                    if (! current.isVoid()
                        && std::abs (static_cast<double> (current) - reference) > 1.0e-6)
                    {
                        it = activeRamps.erase (it);
                        continue;
                    }
                }

                r.elapsed += deltaTimeSeconds;

                double v;
                if (r.elapsed >= r.total)
                {
                    v = r.targetValue;
                }
                else
                {
                    const float progress = r.elapsed / r.total;
                    v = r.startValue + (r.targetValue - r.startValue) * progress;
                }

                if (isIntParam (juce::Identifier (it->first.second)))
                    v = static_cast<double> (juce::roundToInt (v));

                writes.emplace_back (it->first.first, it->first.second, v);

                if (r.elapsed >= r.total)
                {
                    it = activeRamps.erase (it);
                }
                else
                {
                    r.lastWritten = v;
                    r.hasWritten  = true;
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

    /** True for ramp-capable params typed INT in Documentation/WFS-UI_input.csv.
        Interpolated writes for these are rounded so the ValueTree never holds
        fractional values (dials, calc engine and remotes all expect ints). */
    static bool isIntParam (const juce::Identifier& paramId)
    {
        using namespace WFSParameterIDs;
        static const std::set<juce::Identifier> intParams = {
            inputHeightFactor, inputDirectivity, inputRotation, inputTilt,
            inputFRlowCutFreq, inputFRhighShelfFreq, inputFRdiffusion,
            inputLFOphase, inputLFOphaseX, inputLFOphaseY, inputLFOphaseZ
        };
        return intParams.find (paramId) != intParams.end();
    }

private:
    using Key = std::pair<int, juce::String>;

    struct RampState
    {
        double startValue  = 0.0;
        double targetValue = 0.0;
        double lastWritten = 0.0;   // Last value this ramp wrote (takeover detection)
        bool   hasWritten  = false;
        float  elapsed     = 0.0f;
        float  total       = 1.0f;
    };

    WFSValueTreeState& state;
    std::map<Key, RampState> activeRamps;
    juce::CriticalSection lock;
    double lastProcessMs = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSCParameterRamper)
};

} // namespace WFSNetwork
