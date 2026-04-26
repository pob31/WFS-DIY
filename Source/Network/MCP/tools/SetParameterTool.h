#pragma once

#include <JuceHeader.h>
#include "../MCPToolRegistry.h"
#include "../MCPChangeRecords.h"
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

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("variable",   juce::var (variable.release()));
    props->setProperty ("value",      juce::var (value.release()));
    props->setProperty ("channel_id", juce::var (channelId.release()));
    props->setProperty ("band",       juce::var (band.release()));

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

    const juce::String variable = obj->getProperty ("variable").toString();
    if (variable.isEmpty())
        return ToolResult::error ("invalid_args", "Missing required arg: variable");

    if (! obj->hasProperty ("value"))
        return ToolResult::error ("invalid_args", "Missing required arg: value");
    const juce::var value = obj->getProperty ("value");

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

    juce::var beforeValue;
    if (isEqBand)
    {
        auto bandTree = state.getOutputEQBand (channelIndex, bandIndex);
        if (bandTree.isValid())
            beforeValue = bandTree.getProperty (paramId);
    }
    else
    {
        beforeValue = state.getParameter (paramId, channelIndex);
    }

    if (isEqBand)
        state.setOutputEQBandParameterWithArrayPropagation (channelIndex, bandIndex, paramId, value);
    else
        state.setParameter (paramId, value, channelIndex);

    juce::var afterValue;
    if (isEqBand)
    {
        auto bandTree = state.getOutputEQBand (channelIndex, bandIndex);
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
                    "variable name (case-sensitive) and value type. No range "
                    "clamping. Per-channel parameters need channel_id (1-based); "
                    "EQ parameters need both output channel_id and band (1-6). "
                    "Globals (stage/master/network/binaural) take no channel_id.";
    d.inputSchema   = buildSchema();
    d.modifiesState = true;
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        return set (state, args, record);
    };
    return d;
}

} // namespace WFSNetwork::Tools::SetParameter
