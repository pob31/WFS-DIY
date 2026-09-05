#pragma once

/*
    MCPParameterValidation — registry-backed value validation for the
    generic parameter tools.

    The auto-generated per-parameter tools each ship a JSON Schema carrying
    that parameter's `enum` / `minimum` / `maximum`, and dispatchGenericSet
    enforces it. The generic tools (wfs_set_parameter, its batch form) have
    no such per-call schema: their own schema is just {variable, value}, and
    historically they range-checked against OSCParameterBounds — a smaller,
    hand-maintained table that returns "in range" for anything it doesn't
    list, and which never checked enum membership at all.

    That gap didn't matter much while every parameter also had its own
    listed tool. Now that the generated catalog is hidden and the generic
    tools are the primary write path, it does: without this, an out-of-range
    or out-of-enum write that the named tool would have rejected sails
    through the escape hatch.

    So validate against the parameter registry, which is derived from the
    same manifest the generated schemas come from. OSCParameterBounds stays
    in place downstream as a second gate for anything the registry doesn't
    describe.
*/

#include <juce_core/juce_core.h>
#include <cmath>
#include <limits>
#include "MCPCompat.h"
#include "MCPParameterRegistry.h"

namespace WFSNetwork::MCPValidation
{

/** True when `d` names an exact `int`: finite, whole, and inside int's range.

    The range half is the part that isn't pedantry. A `static_cast<int>` of a
    double that doesn't fit is undefined behaviour, not a clamp and not even a
    wrong-but-bounded answer, and every `d` tested here arrives off the wire
    from an MCP client. `1e300` and `1e999` both clear a bare
    `d == std::floor (d)` gate — the first because it is already whole, the
    second because the floor of an infinity is that infinity — and either one
    then reaches the cast.

    Shared with the mirrored enum gate in MCPGeneratedToolLoader.cpp, where
    the same two branches had the same hole, so they cannot drift apart on it.
    int's bounds are exactly representable as doubles, so the comparisons
    below do not round. */
inline bool isExactInt (double d) noexcept
{
    return std::isfinite (d)
        && d == std::floor (d)
        && d >= static_cast<double> (std::numeric_limits<int>::min())
        && d <= static_cast<double> (std::numeric_limits<int>::max());
}

/** Validate (and where needed coerce) `value` for `variable`.

    `rec` may be null — an unknown parameter is not this function's problem
    to report, and callers already handle that case with a did-you-mean
    error. Returns an error ToolResult on rejection; on success returns ok()
    and `value` may have been coerced in place (enum label already resolved
    upstream, numeric strings parsed, ints rounded to the declared type).

    Deliberately mirrors dispatchGenericSet's checks and error codes
    (`invalid_enum_value`, `out_of_range`, `invalid_args`) so a caller sees
    the same failure whether it used the named tool or the generic one. */
inline ToolResult validateAgainstRegistry (const ParameterRegistryRecord* rec,
                                           const juce::String& variable,
                                           juce::var& value)
{
    if (rec == nullptr)
        return ToolResult::ok ({});

    // ---- Enum parameters -------------------------------------------------
    // By this point a label like "Dome" has already been mapped to its
    // storage integer by the caller's resolveEnumLabel pass, so anything
    // still non-integer is a value we cannot honour.
    if (! rec->enumValues.isEmpty())
    {
        auto reject = [&] (const juce::String& shown)
        {
            juce::String expected;
            if (! rec->enumIntValues.isEmpty())
            {
                juce::StringArray ints;
                for (int i : rec->enumIntValues)
                    ints.add (juce::String (i));
                expected = "one of [" + ints.joinIntoString (", ") + "]";
            }
            else
            {
                expected = "an integer index 0.."
                         + juce::String (rec->enumValues.size() - 1);
            }

            return ToolResult::error ("invalid_enum_value",
                                      "value " + shown.quoted() + " is not a valid "
                                      + variable + " value (expected " + expected
                                      + ", or one of: "
                                      + rec->enumValues.joinIntoString (", ") + ")");
        };

        if (value.isString())
        {
            const auto s = value.toString().trim();
            if (s.isEmpty() || ! s.containsOnly ("0123456789.+-eE"))
                return reject (s);
            const double d = s.getDoubleValue();
            if (! isExactInt (d))
                return reject (s);
            value = juce::var (static_cast<int> (d));
        }
        else if (value.isDouble())
        {
            const double d = static_cast<double> (value);
            if (! isExactInt (d))
                return reject (juce::String (d));
            value = juce::var (static_cast<int> (d));
        }

        if (! (value.isInt() || value.isInt64()))
            return reject (value.toString());

        const int candidate = static_cast<int> (value);
        const bool valid = rec->enumIntValues.isEmpty()
                               ? (candidate >= 0 && candidate < rec->enumValues.size())
                               : rec->enumIntValues.contains (candidate);
        if (! valid)
            return reject (juce::String (candidate));

        return ToolResult::ok ({});
    }

    // ---- Numeric range ---------------------------------------------------
    const bool hasRange = rec->minValue.has_value() && rec->maxValue.has_value();
    if (! hasRange)
        return ToolResult::ok ({});

    if (value.isString())
    {
        const auto s = value.toString().trim();
        if (s.isEmpty() || ! s.containsOnly ("0123456789.+-eE"))
            return ToolResult::error ("invalid_args",
                                      "value not numeric for " + variable + ": " + s.quoted());
        value = juce::var (s.getDoubleValue());
    }

    if (value.isDouble() || value.isInt() || value.isInt64())
    {
        const double d = static_cast<double> (value);
        if (d < *rec->minValue || d > *rec->maxValue)
            return ToolResult::error ("out_of_range",
                                      "value " + juce::String (d, 6) + " not in ["
                                      + juce::String (*rec->minValue, 6) + ", "
                                      + juce::String (*rec->maxValue, 6) + "] for " + variable);

        // Match the parameter's declared storage type so an int-typed var
        // doesn't land in a float ValueTree slot (or vice-versa).
        if (rec->type == "integer")
            value = juce::var (juce::roundToInt (d));
        else if (rec->type == "number")
            value = juce::var (d);
    }

    return ToolResult::ok ({});
}

} // namespace WFSNetwork::MCPValidation
