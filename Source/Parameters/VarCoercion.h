#pragma once

#include <JuceHeader.h>

/**
    Coercing reads for values that came out of a ValueTree.

    `juce::ValueTree::fromXml` types EVERY property as a STRING var. So a read
    guarded by `var::isInt()` / `isDouble()` / `isBool()` works perfectly on a
    freshly-built tree and silently yields its fallback on a loaded one — the
    "works when new, broken when saved" signature. Snapshot recall re-injects
    those string vars into the live tree too, so the hazard is not confined to
    project load.

    These are for reading STORED state. They are deliberately not for inbound
    wire values: an OSC argument, an MQTT payload field or an MCP JSON-RPC
    argument arrives with a real type, and type-CHECKING those is correct. Use
    these where the alternative is a guard that can silently default.

    Void is the one case that genuinely means "absent", so it keeps the caller's
    default. Everything else converts — juce::var handles string→number.
*/
namespace WFSVar
{
    inline float toFloat (const juce::var& v, float defaultVal = 0.0f)
    {
        if (v.isVoid()) return defaultVal;
        return static_cast<float> (static_cast<double> (v));
    }

    inline int toInt (const juce::var& v, int defaultVal = 0)
    {
        if (v.isVoid()) return defaultVal;
        return static_cast<int> (v);
    }

    inline bool toBool (const juce::var& v, bool defaultVal = false)
    {
        if (v.isVoid()) return defaultVal;
        return static_cast<int> (v) != 0;
    }

    /** True when the var holds something numeric, INCLUDING a numeric string
        left behind by a load. Use in place of `isDouble() || isInt()` when the
        question is "can this be sent as a number", not "what type is it". */
    inline bool isNumeric (const juce::var& v)
    {
        if (v.isDouble() || v.isInt() || v.isInt64() || v.isBool())
            return true;
        if (! v.isString())
            return false;

        const juce::String s = v.toString().trim();
        return s.isNotEmpty() && s.containsOnly ("0123456789+-.eE")
               && s.containsAnyOf ("0123456789");
    }
}
