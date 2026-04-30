#pragma once

#include <JuceHeader.h>
#include <optional>

namespace WFSNetwork
{

/**
 * Numeric bounds for an OSC-settable parameter, mirrored from the
 * `Min`/`Max` constants in Source/Parameters/WFSParameterDefaults.h.
 *
 * Used as a gate at the OSC entry path: incoming float/int values are
 * checked against the parameter's documented range before they reach
 * the ValueTree. Out-of-range values are rejected via
 * `OSCLogger::logRejected` and surface in the Network log Rejected
 * filter alongside the existing NaN gate.
 *
 * `min` / `max` are stored as `double` so a single range check covers
 * both `FLOAT` and `INT` parameter types. `isInt` tells callers to
 * truncate the incoming value before reporting (purely cosmetic — the
 * comparison is the same).
 */
struct ParamBounds
{
    double min;
    double max;
    bool   isInt;
};

/** Look up the documented bounds for a parameter by its
 *  `WFSParameterIDs::*` identifier. Returns nullopt for parameters
 *  with no numeric bounds (strings, free-form ints, names) or
 *  parameters that are not yet covered by the bounds table. */
std::optional<ParamBounds> getBounds (const juce::Identifier& paramId);

/** Convenience: returns true if `value` lies within `[min, max]` (or
 *  no bounds are known for `paramId`). NaN/Inf must have been
 *  filtered earlier — this function does not re-check finiteness. */
bool isInRange (const juce::Identifier& paramId, double value);

/** Format a short rejection reason such as
 *  `"out of range: 99.0 not in [0.0, 50.0]"`. */
juce::String formatOutOfRangeReason (const juce::Identifier& paramId,
                                     double value);

} // namespace WFSNetwork
