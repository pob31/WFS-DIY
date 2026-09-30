#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <optional>

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

    /** The number a string spells, when it spells a finite one: an optional
        sign, digits with at most one point, and an optional exponent ("1.5",
        "-3", ".5", "1.2e-16"). The exponent matters because a project load
        hands every property back as text, and JUCE writes |x| >= 1e6 and
        0 < |x| <= 1e-5 in scientific notation. "inf", "nan", "0x10", "12abc",
        "e" and "" are not numbers here, although String::getDoubleValue reads
        the first two as the IEEE values and the rest as 12 or 0.

        Unlike the readers above this is a gate, not a coercion: it is for
        text that must be a finite number before anything uses it (the
        store's write rule, the MCP tools' number arguments). */
    inline std::optional<double> parseFiniteNumber (const juce::String& text)
    {
        const juce::String trimmed = text.trim();
        auto p = trimmed.getCharPointer();

        if (*p == '+' || *p == '-')
            ++p;

        int mantissaDigits = 0;
        while (juce::CharacterFunctions::isDigit (*p)) { ++p; ++mantissaDigits; }
        if (*p == '.')
        {
            ++p;
            while (juce::CharacterFunctions::isDigit (*p)) { ++p; ++mantissaDigits; }
        }
        if (mantissaDigits == 0)
            return std::nullopt;

        if (*p == 'e' || *p == 'E')
        {
            ++p;
            if (*p == '+' || *p == '-')
                ++p;
            int exponentDigits = 0;
            while (juce::CharacterFunctions::isDigit (*p)) { ++p; ++exponentDigits; }
            if (exponentDigits == 0)
                return std::nullopt;
        }

        if (! p.isEmpty())
            return std::nullopt;

        const double value = trimmed.getDoubleValue();   // "1e999" is infinite
        return std::isfinite (value) ? std::optional<double> (value) : std::nullopt;
    }
}
