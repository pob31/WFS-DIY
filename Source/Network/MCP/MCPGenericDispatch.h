#pragma once

/*
    MCPGenericDispatch — shared write/nudge dispatch for parameter tools.

    These types and functions were private to MCPGeneratedToolLoader.cpp
    while the auto-generated tools were their only caller. They are declared
    here so hand-written tools can reuse the same code path instead of
    growing a third copy of the ValueTree-write logic: wfs_nudge_parameter
    builds a NudgeBinding from the parameter registry and delegates straight
    to dispatchGenericNudge, inheriting its clamping and change-record
    population for free.

    The definitions still live in MCPGeneratedToolLoader.cpp — this header
    only publishes them.
*/

#include <juce_core/juce_core.h>
#include "MCPCompat.h"

class WFSValueTreeState;

namespace WFSNetwork::Tools::Generated::Detail
{

/** Per-tool data the generic dispatch handlers need to do their job.

    Range fields mirror the schema's `minimum`/`maximum`. Setters treat them
    as a fail-closed gate (out-of-range is an error); nudges treat them as a
    clamp, since a nudge is a relative gesture where saturating at the limit
    is the expected behaviour rather than a mistake. */
struct ToolBinding
{
    juce::String name;              // "input.position.set_x"
    juce::String internalVariable;  // canonical ValueTree property name (case-corrected)
    juce::String csvSection;        // "Position" — used in affected_groups + operatorDescription
    juce::StringArray enumValues;   // empty if not an enum tool
    juce::String channelArgName;    // input_id / output_id / reverb_id / cluster_id, or empty for global
    juce::String valueArgName { "value" };  // self-documenting renames: "name" / "mode" / "shape" / "protocol"
    bool isEqBand = false;

    /** Where a parameter lives below its channel, when one index is not enough.

        EQ bands were the first of these and were hardcoded; the same shape then
        turned up in the sampler (36 cells and N sets per input), the gradient map
        (3 layers, each with a list of shapes) and ADM-OSC (4 mappings, 3 axes
        each). Those families were generated with no index argument at all, so
        every tool named a parameter it had no way to point at. */
    enum class SubTree
    {
        None,
        GradientLayer,   // input_id + layer
        GradientShape,   // input_id + layer + shape
        GradientAlias,   // input_id; the layer is baked into the variable name
        AdmCartAxis,     // mapping + axis, global (no channel)
        AdmPolarMapping, // mapping, global (no channel)
        SamplerCell,     // input_id + cell_id
        SamplerSet,      // input_id + set_id
        NetworkTarget,   // target_id, global (no channel)
    };

    SubTree subTree = SubTree::None;
    juce::String subIndexArgA;      // first extra index arg ("layer"), empty if none
    juce::String subIndexArgB;      // second ("shape"), empty if none
    int aliasIndex = -1;            // GradientAlias: the layer the name encodes
    juce::Identifier aliasProperty; // GradientAlias: the property actually stored

    bool hasRange = false;
    double minValue = 0.0;
    double maxValue = 0.0;
    bool isIntegerType = false;     // schema declared type: "integer" → write int, not double
    bool isNumericType = false;     // schema declared type: "number" / "integer"
};

/** Extension of ToolBinding for nudge variants. The range fields on
    ToolBinding feed the clamp; this subtype exists for naming clarity
    and future divergence (e.g. nudges may grow per-press defaults). */
struct NudgeBinding : ToolBinding {};

/** Map an enum label to its 0-based index; pass through non-enum or
    unrecognised values unchanged so the caller can raise a clear error. */
juce::var coerceValue (const juce::var& incoming, const ToolBinding& binding);

/** Convert "<scope>_id" → "<scope>" for human-readable descriptions. */
juce::String channelArgToScopeLabel (const juce::String& argName);

/** Absolute write: validates enum membership and range, then writes. */
ToolResult dispatchGenericSet (WFSValueTreeState& state,
                               const ToolBinding& binding,
                               const juce::var& args,
                               ChangeRecord* record);

/** Relative write: read-modify-write with clamping.

    Note this path has no EQ-band support — it addresses parameters via
    `state.getParameter/setParameter(id, channelIndex)` only. Callers must
    reject eq_band-scoped parameters before delegating here. */
ToolResult dispatchGenericNudge (WFSValueTreeState& state,
                                 const NudgeBinding& binding,
                                 const juce::var& args,
                                 ChangeRecord* record);

} // namespace WFSNetwork::Tools::Generated::Detail
