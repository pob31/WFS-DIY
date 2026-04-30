#pragma once

#include <JuceHeader.h>
#include "../MCPToolRegistry.h"
#include "../MCPChangeRecords.h"
#include "../MCPParameterRegistry.h"
#include "../../OSCParameterBounds.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"

namespace WFSNetwork::Tools::SetParameter
{

inline juce::var buildSchema()
{
    auto variable = std::make_unique<juce::DynamicObject>();
    variable->setProperty ("type", "string");
    variable->setProperty ("description",
        "Parameter identifier name as defined in WFSParameterIDs.h "
        "(e.g. 'inputPositionX', 'stageWidth', 'eqFrequency'). "
        "Case-sensitive - use the exact name. Run mcp_get_ai_change_history "
        "to see canonical names from prior writes if unsure.");

    auto value = std::make_unique<juce::DynamicObject>();
    value->setProperty ("description", "Value to write. Type matches the underlying parameter (number/integer/string/bool).");
    // No `type` declared here — the JSON schema accepts any. Strict-typed
    // enum mapping (string -> index) only happens for the auto-generated
    // tools where the schema declares the enum list.

    auto channelId = std::make_unique<juce::DynamicObject>();
    channelId->setProperty ("type", "integer");
    channelId->setProperty ("minimum", 1);
    channelId->setProperty ("description",
        "Channel number (1-based) for per-channel parameters. Omit for "
        "globals (config / network / stage / master / binaural).");

    auto band = std::make_unique<juce::DynamicObject>();
    band->setProperty ("type", "integer");
    band->setProperty ("minimum", 1);
    band->setProperty ("maximum", 6);
    band->setProperty ("description",
        "EQ band number (1-6) for output-EQ parameters. Omit for non-EQ.");

    // Phase 8: tier-2 tools require a two-step confirmation handshake.
    auto confirm = std::make_unique<juce::DynamicObject>();
    confirm->setProperty ("type", "string");
    confirm->setProperty ("description",
        "Confirmation token returned by the previous call to this tool. "
        "Tier 2 tools require a two-step handshake: the first call returns a "
        "confirmation_token in tier_enforcement; re-call with confirm set to "
        "that token (within 30 seconds) to actually execute. Omit on the first call.");

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("variable",   juce::var (variable.release()));
    props->setProperty ("value",      juce::var (value.release()));
    props->setProperty ("channel_id", juce::var (channelId.release()));
    props->setProperty ("band",       juce::var (band.release()));
    props->setProperty ("confirm",    juce::var (confirm.release()));

    auto required = juce::Array<juce::var>();
    required.add ("variable");
    required.add ("value");

    auto schema = std::make_unique<juce::DynamicObject>();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (props.release()));
    schema->setProperty ("required", juce::var (required));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema.release());
}

inline ToolResult set (WFSValueTreeState& state, const juce::var& args, ChangeRecord* record)
{
    if (! args.isObject())
        return ToolResult::error ("invalid_args", "Arguments must be a JSON object");

    auto* obj = args.getDynamicObject();

    const juce::String requestedVariable = obj->getProperty ("variable").toString();
    if (requestedVariable.isEmpty())
        return ToolResult::error ("invalid_args", "Missing required arg: variable");

    // Whitelist against the parameter registry. Without this, unknown
    // names previously silently no-op'd (the underlying ValueTree just
    // wrote a property nobody reads, and the response showed
    // before:null/after:null — visually identical to "the param exists
    // but was unset"). Reject up front and offer a did-you-mean.
    juce::String variable = requestedVariable;
    {
        const auto& reg = MCPParameterRegistry::getInstance();
        if (reg.size() > 0 && ! reg.isKnown (requestedVariable))
        {
            juce::String message = "Unknown parameter '" + requestedVariable + "'.";
            const auto suggestions = reg.suggestSimilar (requestedVariable);
            if (! suggestions.isEmpty())
            {
                message += " Did you mean: ";
                message += suggestions.joinIntoString (", ");
                message += "? Use mcp_describe_parameters to browse the full registry.";
            }
            else
            {
                message += " Use mcp_describe_parameters to browse the registry.";
            }
            return ToolResult::error ("unknown_parameter", message);
        }
        // Resolve synonyms (e.g. stageOriginX → originWidth) so the rest
        // of the handler sees the canonical name. The result envelope
        // surfaces both names so the caller can confirm what landed.
        variable = reg.canonicalize (requestedVariable);
    }

    if (! obj->hasProperty ("value"))
        return ToolResult::error ("invalid_args", "Missing required arg: value");
    juce::var value = obj->getProperty ("value");

    // Enum string -> int coercion. The registry surfaces enum_values for
    // the auto-gen path; mirror that here so wfs_set_parameter("stageShape",
    // "Dome") works the same way system_stage_set_shape(value="Dome") does.
    // Run BEFORE the numeric coercion so an enum label doesn't get rejected
    // as "not numeric".
    if (value.isString())
    {
        if (auto resolved = MCPParameterRegistry::getInstance()
                              .resolveEnumLabel (variable, value.toString()))
        {
            value = juce::var (*resolved);
        }
    }

    // Loose-typed clients sometimes wire JSON numbers as strings ("999"
    // rather than 999). For params with a known numeric bounds entry,
    // coerce a numeric-looking string to a double here so the range
    // check below applies AND the ValueTree receives a numeric var
    // (writing "999" into a float slot corrupts the type). Params with
    // no numeric bounds entry pass through unchanged so string-typed
    // params (inputName, inputMutes, etc.) still work.
    {
        const juce::Identifier coerceParamId (variable);
        const auto bounds = WFSNetwork::getBounds (coerceParamId);
        if (bounds.has_value() && value.isString())
        {
            const auto s = value.toString().trim();
            if (s.isEmpty() || ! s.containsOnly ("0123456789.+-eE"))
                return ToolResult::error ("invalid_args",
                                          "value not numeric for " + variable + ": " + s.quoted());
            value = juce::var (s.getDoubleValue());
        }
    }

    int channelIndex = -1;
    int displayId = 0;
    if (obj->hasProperty ("channel_id"))
    {
        displayId = static_cast<int> (obj->getProperty ("channel_id"));
        channelIndex = displayId - 1;
        if (channelIndex < 0)
            return ToolResult::error ("invalid_args",
                                      "channel_id out of range: " + juce::String (displayId));
    }

    int bandIndex = -1;
    bool isEqBand = false;
    if (obj->hasProperty ("band"))
    {
        isEqBand = true;
        bandIndex = static_cast<int> (obj->getProperty ("band")) - 1;
        if (bandIndex < 0)
            return ToolResult::error ("invalid_args", "band out of range");
    }

    const juce::Identifier paramId (variable);

    // Range gate: reuse the OSC ingress bounds table so the escape-hatch
    // can't bypass the safety policy applied to the OSC path. Params with
    // no entry in the bounds table return true from isInRange (permissive)
    // — keeping the escape-hatch useful for params that haven't yet been
    // added to OSCParameterBounds. String-typed values short-circuit the
    // numeric check.
    if (value.isDouble() || value.isInt() || value.isInt64())
    {
        const double d = static_cast<double> (value);
        if (! WFSNetwork::isInRange (paramId, d))
            return ToolResult::error ("out_of_range",
                                      WFSNetwork::formatOutOfRangeReason (paramId, d));

        // Type-coerce the var to match the param's declared int-vs-float
        // shape, so an MCP-sent int doesn't land as an int-typed var in a
        // float ValueTree slot (or vice-versa). Mirrors the schema-type
        // coercion in the auto-generated tool path.
        if (const auto bounds = WFSNetwork::getBounds (paramId))
        {
            if (bounds->isInt)
                value = juce::var (juce::roundToInt (d));
            else
                value = juce::var (d);
        }
    }

    // Resolve EQ-band family from the variable name. The schema has no
    // hint, so we infer from the param prefix the same way the auto-
    // generated dispatcher does. Output EQ uses array-propagation;
    // reverb pre-EQ is per-channel; reverb post-EQ is global.
    enum class EqFamily { Output, ReverbPre, ReverbPost };
    auto eqFamily = [&]() -> EqFamily
    {
        if (variable.startsWith ("reverbPostEQ")) return EqFamily::ReverbPost;
        if (variable.startsWith ("reverbPreEQ"))  return EqFamily::ReverbPre;
        return EqFamily::Output;
    }();

    auto getBandTree = [&]() -> juce::ValueTree
    {
        switch (eqFamily)
        {
            case EqFamily::ReverbPost: return state.getReverbPostEQBand (bandIndex);
            case EqFamily::ReverbPre:  return state.getReverbEQBand (channelIndex, bandIndex);
            case EqFamily::Output:
            default:                   return state.getOutputEQBand (channelIndex, bandIndex);
        }
    };

    juce::var beforeValue;
    if (isEqBand)
    {
        auto bandTree = getBandTree();
        if (bandTree.isValid())
            beforeValue = bandTree.getProperty (paramId);
    }
    else
    {
        beforeValue = state.getParameter (paramId, channelIndex);
    }

    if (isEqBand)
    {
        if (eqFamily == EqFamily::Output)
        {
            state.setOutputEQBandParameterWithArrayPropagation (channelIndex, bandIndex, paramId, value);
        }
        else
        {
            auto bandTree = getBandTree();
            if (bandTree.isValid())
                bandTree.setProperty (paramId, value, state.getActiveUndoManager());
        }
    }
    else
    {
        state.setParameter (paramId, value, channelIndex);
    }

    juce::var afterValue;
    if (isEqBand)
    {
        auto bandTree = getBandTree();
        if (bandTree.isValid())
            afterValue = bandTree.getProperty (paramId);
    }
    else
    {
        afterValue = state.getParameter (paramId, channelIndex);
    }

    if (record != nullptr)
    {
        record->affectedParameters.add (variable);
        record->affectedGroups.push_back ({ displayId, juce::String ("escape_hatch") });

        auto before = std::make_unique<juce::DynamicObject>();
        before->setProperty (paramId, beforeValue);
        record->beforeState = juce::var (before.release());

        auto after = std::make_unique<juce::DynamicObject>();
        after->setProperty (paramId, afterValue);
        record->afterState = juce::var (after.release());

        juce::String desc = "Set " + variable;
        if (channelIndex >= 0)
            desc += " for channel " + juce::String (displayId);
        if (isEqBand)
            desc += " band " + juce::String (bandIndex + 1);
        desc += " to " + value.toString() + " (via wfs.set_parameter escape-hatch)";
        record->operatorDescription = desc;
    }

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("variable", variable);
    if (variable != requestedVariable)
        result->setProperty ("synonym_of", requestedVariable);
    if (channelIndex >= 0)
        result->setProperty ("channel_id", displayId);
    if (isEqBand)
        result->setProperty ("band", bandIndex + 1);
    result->setProperty ("before", beforeValue);
    result->setProperty ("after", afterValue);
    return ToolResult::ok (juce::var (result.release()));
}

inline ToolDescriptor describe (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "wfs_set_parameter";
    d.description = "Generic escape-hatch parameter writer. Use only when the "
                    "specific auto-generated tool (e.g. input.position.set_x) "
                    "doesn't fit the flow. Caller is responsible for the exact "
                    "variable name (case-sensitive) and value type. Numeric values "
                    "are range-checked against the same bounds table used by OSC "
                    "ingress (out-of-range writes are rejected). Per-channel "
                    "parameters need channel_id (1-based); EQ parameters need both "
                    "output channel_id and band (1-6). Globals "
                    "(stage/master/network/binaural) take no channel_id.";
    d.inputSchema   = buildSchema();
    d.modifiesState = true;
    d.tier        = 2;  // bypasses range clamping — confirm before applying
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        return set (state, args, record);
    };
    return d;
}

} // namespace WFSNetwork::Tools::SetParameter
