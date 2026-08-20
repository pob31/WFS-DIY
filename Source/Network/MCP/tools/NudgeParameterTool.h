#pragma once

/*
    wfs_nudge_parameter — the generic relative-adjustment tool.

    Absolute writes had a generic escape hatch (wfs_set_parameter) from the
    start, but relative moves did not: the only way to nudge was one of the
    63 auto-generated per-parameter nudge tools. Now that the generated
    catalog is unlisted, that would leave "make it a bit louder" with no
    discoverable tool at all — hence this one.

    It does no arithmetic of its own. It resolves the parameter through the
    registry, builds the same NudgeBinding the generated tools use, and
    hands off to Detail::dispatchGenericNudge, so clamping, before/after
    capture and change-record population behave identically either way.
*/

#include <JuceHeader.h>
#include "../MCPCompat.h"
#include "../MCPParameterRegistry.h"
#include "../MCPGenericDispatch.h"
#include "../../../Parameters/WFSValueTreeState.h"

namespace WFSNetwork::Tools::NudgeParameter
{

inline juce::var buildSchema()
{
    auto variable = std::make_unique<juce::DynamicObject>();
    variable->setProperty ("type", "string");
    variable->setProperty ("description",
        "Parameter to adjust, e.g. 'inputPositionX'. Use "
        "mcp_describe_parameters to find names.");

    auto direction = std::make_unique<juce::DynamicObject>();
    direction->setProperty ("type", "string");
    juce::Array<juce::var> directions;
    directions.add ("inc");
    directions.add ("dec");
    direction->setProperty ("enum", juce::var (directions));
    direction->setProperty ("description", "Adjust up ('inc') or down ('dec').");

    auto amount = std::make_unique<juce::DynamicObject>();
    amount->setProperty ("type", "number");
    amount->setProperty ("description",
        "Step size in the parameter's own unit. Defaults to 1.0. The result "
        "is clamped to the parameter's range.");

    auto channelId = std::make_unique<juce::DynamicObject>();
    channelId->setProperty ("type", "integer");
    channelId->setProperty ("minimum", 1);
    channelId->setProperty ("description",
        "Channel number (1-based) for per-channel parameters. For input "
        "parameters this is the permanent input number shown in the UI, which "
        "can have gaps and need not match the channel's position in the list. "
        "Omit for globals.");

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("variable",   juce::var (variable.release()));
    props->setProperty ("direction",  juce::var (direction.release()));
    props->setProperty ("amount",     juce::var (amount.release()));
    props->setProperty ("channel_id", juce::var (channelId.release()));

    juce::Array<juce::var> required;
    required.add ("variable");
    required.add ("direction");

    auto schema = std::make_unique<juce::DynamicObject>();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (props.release()));
    schema->setProperty ("required", juce::var (required));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema.release());
}

inline ToolResult nudge (WFSValueTreeState& state,
                         const juce::var& args,
                         ChangeRecord* record)
{
    auto* obj = args.getDynamicObject();
    if (obj == nullptr)
        return ToolResult::error ("invalid_args", "Arguments must be a JSON object");

    const auto requested = obj->getProperty ("variable").toString().trim();
    if (requested.isEmpty())
        return ToolResult::error ("invalid_args", "Missing required arg: variable");

    const auto& reg = MCPParameterRegistry::getInstance();
    if (! reg.isKnown (requested))
    {
        juce::String message = "Unknown parameter: " + requested.quoted() + ".";
        const auto suggestions = reg.suggestSimilar (requested);
        if (! suggestions.isEmpty())
            message += " Did you mean: " + suggestions.joinIntoString (", ") + "?";
        else
            message += " Use mcp_describe_parameters to browse the registry.";
        return ToolResult::error ("unknown_parameter", message);
    }

    const auto variable = reg.canonicalize (requested);
    const auto* rec = reg.findByVariable (variable);
    if (rec == nullptr)
        return ToolResult::error ("unknown_parameter",
                                  "No registry record for " + variable.quoted());

    // A nudge is a tier-1 gesture and must stay one. Silently stepping a
    // confirm-gated parameter would let a caller walk it to any value one
    // increment at a time, which is exactly what the gate exists to prevent.
    if (rec->tier >= 2)
    {
        juce::String msg = "Parameter " + variable.quoted() + " is tier-"
                         + juce::String (rec->tier)
                         + " and cannot be nudged. Use wfs_set_parameter";
        if (rec->toolName.isNotEmpty())
            msg += " or `" + rec->toolName + "`";
        msg += ", which carry the required confirmation.";
        return ToolResult::error ("tier_escalation", msg);
    }

    // Relative arithmetic needs a number to start from.
    if (! rec->enumValues.isEmpty())
        return ToolResult::error ("invalid_parameter_type",
                                  "Parameter " + variable.quoted() + " is an enum; "
                                  "use wfs_set_parameter with one of: "
                                  + rec->enumValues.joinIntoString (", "));

    if (rec->type != "number" && rec->type != "integer")
        return ToolResult::error ("invalid_parameter_type",
                                  "Parameter " + variable.quoted() + " is "
                                  + (rec->type.isEmpty() ? juce::String ("non-numeric") : rec->type)
                                  + " and cannot be nudged. Use wfs_set_parameter.");

    // The shared nudge dispatcher addresses parameters by (id, channelIndex)
    // and has no EQ-band path, so band-scoped parameters must go the
    // absolute route rather than silently writing to the wrong place.
    if (rec->scope == "eq_band")
        return ToolResult::error ("unsupported_scope",
                                  "Parameter " + variable.quoted() + " is EQ-band scoped. "
                                  "Nudging bands is not supported - use wfs_set_parameter "
                                  "with channel_id and band.");

    Generated::Detail::NudgeBinding binding;
    binding.name             = "wfs_nudge_parameter";
    binding.internalVariable = variable;
    binding.csvSection       = rec->csvSection;
    binding.isNumericType    = true;
    binding.isIntegerType    = (rec->type == "integer");
    if (rec->scope.isNotEmpty() && rec->scope != "global")
        binding.channelArgName = "channel_id";
    if (rec->minValue.has_value() && rec->maxValue.has_value())
    {
        binding.hasRange = true;
        binding.minValue = *rec->minValue;
        binding.maxValue = *rec->maxValue;
    }

    // The shared resolver keys the permanent-number lookup off the arg *name*
    // "input_id"; under the public name "channel_id" it would resolve an input
    // as channel_id - 1. Input numbers may have gaps and are not in slot order
    // after a reorder, so that arithmetic addresses a different channel than
    // the caller named, silently. Renaming the arg on a private copy puts the
    // dispatch back on the lookup path without adding a second name to the
    // schema. Output / reverb / cluster ids really are dense slot positions,
    // so they keep channel_id and its - 1.
    juce::var dispatchArgs = args;
    if (rec->scope == "input")
    {
        if (! obj->hasProperty ("channel_id"))
            return ToolResult::error ("invalid_args", "Missing required arg: channel_id");

        const int displayId = static_cast<int> (obj->getProperty ("channel_id"));

        // Naming an input by number makes that number an external reference the
        // client quotes back, and it must stop reflowing before the
        // read-modify-write lands.
        state.markChannelNumbersUserOwned();

        // Resolved here as well as in the dispatcher so the failure names the
        // arg the caller actually sent rather than the renamed one.
        if (state.getSlotForChannelNumber (displayId) < 0)
            return ToolResult::error ("invalid_args",
                                      "channel_id not a live channel: " + juce::String (displayId));

        auto renamedArgs = obj->clone();
        renamedArgs->removeProperty ("channel_id");
        renamedArgs->setProperty ("input_id", displayId);
        dispatchArgs = juce::var (renamedArgs.release());
        binding.channelArgName = "input_id";
    }

    return Generated::Detail::dispatchGenericNudge (state, binding, dispatchArgs, record);
}

inline ToolDescriptor describe (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "wfs_nudge_parameter";
    d.description =
        "Adjust any numeric parameter relative to its current value. "
        "Give a variable and a direction ('inc' or 'dec'); optional amount "
        "defaults to 1.0. The result is clamped to the parameter's range, "
        "so repeated nudges saturate at the limit rather than failing. "
        "Per-channel parameters need channel_id (1-based). For enums, "
        "strings, EQ bands or tier-2+ parameters use wfs_set_parameter.";
    d.inputSchema   = buildSchema();
    d.modifiesState = true;
    d.tier          = 1;
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        return nudge (state, args, record);
    };
    return d;
}

} // namespace WFSNetwork::Tools::NudgeParameter
