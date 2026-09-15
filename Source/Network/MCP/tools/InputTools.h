#pragma once

#include <JuceHeader.h>
#include "../MCPCompat.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"

namespace WFSNetwork::Tools::Input
{

namespace detail
{
    /** Resolve a permanent input channel number to a 0-based slot, returning
        -1 when it is not a live channel so the handler can return a
        structured error. The list may have gaps after deletions, so this
        must go through the state lookup, never inputId - 1. */
    inline int resolveChannelIndex (WFSValueTreeState& state, int inputId)
    {
        // An MCP client is an external agent: it caches the numbers it was
        // handed and quotes them back on later calls, so naming one here turns
        // it into a live external reference. Freeze the numbering before the
        // lookup — otherwise a drag-reorder in the GUI between two tool calls
        // renumbers the channels the client is still holding.
        state.markChannelNumbersUserOwned ("MCP input tool");
        return state.getSlotForChannelNumber (inputId);
    }

    inline juce::var positionRangeSchema()
    {
        // Stage bounds in WFS-DIY are at most ±50 m on any axis (per
        // Documentation/WFS-UI_input.csv). Clamp here AND on the way in.
        auto v = std::make_unique<juce::DynamicObject>();
        v->setProperty ("type", "number");
        v->setProperty ("minimum", -50.0);
        v->setProperty ("maximum",  50.0);
        return juce::var (v.release());
    }
}

//==============================================================================
// input.set_name
//==============================================================================

inline juce::var buildSetNameSchema()
{
    auto inputId = std::make_unique<juce::DynamicObject>();
    inputId->setProperty ("type", "integer");
    inputId->setProperty ("minimum", 1);
    inputId->setProperty ("description", "Input channel number (1-based).");

    auto name = std::make_unique<juce::DynamicObject>();
    name->setProperty ("type", "string");
    name->setProperty ("maxLength", 64);
    name->setProperty ("description", "New display name for the input.");

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("input_id", juce::var (inputId.release()));
    props->setProperty ("name",     juce::var (name.release()));

    auto required = juce::Array<juce::var>();
    required.add ("input_id");
    required.add ("name");

    auto schema = std::make_unique<juce::DynamicObject>();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (props.release()));
    schema->setProperty ("required", juce::var (required));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema.release());
}

inline ToolResult setName (WFSValueTreeState& state, const juce::var& args, ChangeRecord* record)
{
    if (! args.isObject())
        return ToolResult::error ("invalid_args", "Arguments must be a JSON object");

    auto* obj = args.getDynamicObject();
    int inputId    = static_cast<int> (obj->getProperty ("input_id"));
    juce::String newName = obj->getProperty ("name").toString();

    int channelIndex = detail::resolveChannelIndex (state, inputId);
    if (channelIndex < 0)
        return ToolResult::error ("invalid_args", "input_id out of range: " + juce::String (inputId));

    juce::String oldName = state.getInputParameter (channelIndex, WFSParameterIDs::inputName).toString();
    state.setInputParameter (channelIndex, WFSParameterIDs::inputName, newName);

    if (record != nullptr)
    {
        record->operatorDescription = "Renamed input " + juce::String (inputId)
                                       + " from '" + oldName + "' to '" + newName + "'";
        record->affectedParameters.add ("inputName");
        record->affectedGroups.push_back ({ inputId, "Channel" });

        auto before = std::make_unique<juce::DynamicObject>();
        before->setProperty ("inputName", oldName);
        record->beforeState = juce::var (before.release());

        auto after = std::make_unique<juce::DynamicObject>();
        after->setProperty ("inputName", newName);
        record->afterState = juce::var (after.release());
    }

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("input_id",  inputId);
    result->setProperty ("old_name",  oldName);
    result->setProperty ("new_name",  newName);
    return ToolResult::ok (juce::var (result.release()));
}

inline ToolDescriptor describeSetName (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "input_set_name";
    d.description = "Rename an input channel. Useful in voice flows like "
                    "'Rename input 3 to Marie'.";
    d.inputSchema   = buildSetNameSchema();
    d.modifiesState = true;
    d.tier        = 1;  // cosmetic, easily reversible
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        return setName (state, args, record);
    };
    return d;
}

//==============================================================================
// input.set_output_mute
//==============================================================================

inline juce::var buildSetOutputMuteSchema()
{
    auto inputId = std::make_unique<juce::DynamicObject>();
    inputId->setProperty ("type", "integer");
    inputId->setProperty ("minimum", 1);
    inputId->setProperty ("description", "Input channel number (1-based, permanent).");

    auto outputId = std::make_unique<juce::DynamicObject>();
    outputId->setProperty ("type", "integer");
    outputId->setProperty ("minimum", 1);
    outputId->setProperty ("description", "Output channel number (1-based).");

    auto muted = std::make_unique<juce::DynamicObject>();
    muted->setProperty ("type", "boolean");
    muted->setProperty ("description", "true mutes this input on that output, false unmutes it.");

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("input_id",  juce::var (inputId.release()));
    props->setProperty ("output_id", juce::var (outputId.release()));
    props->setProperty ("muted",     juce::var (muted.release()));

    auto required = juce::Array<juce::var>();
    required.add ("input_id");
    required.add ("output_id");
    required.add ("muted");

    auto schema = std::make_unique<juce::DynamicObject>();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (props.release()));
    schema->setProperty ("required", juce::var (required));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema.release());
}

inline ToolResult setOutputMute (WFSValueTreeState& state, const juce::var& args, ChangeRecord* record)
{
    if (! args.isObject())
        return ToolResult::error ("invalid_args", "Arguments must be a JSON object");

    auto* obj = args.getDynamicObject();

    // The dispatcher does not enforce `required`, and a missing flag would cast
    // to false: a call that forgot it would UNMUTE, the opposite of the tool's
    // name, and report success.
    for (const char* arg : { "input_id", "output_id", "muted" })
        if (! obj->hasProperty (arg) || obj->getProperty (arg).isVoid())
            return ToolResult::error ("invalid_args", juce::String ("Missing required arg: ") + arg);

    const auto mutedVar = obj->getProperty ("muted");
    if (! (mutedVar.isBool() || mutedVar.isInt() || mutedVar.isInt64() || mutedVar.isDouble()))
        return ToolResult::error ("invalid_args", "muted must be true or false");

    const int inputId  = static_cast<int> (obj->getProperty ("input_id"));
    const int outputId = static_cast<int> (obj->getProperty ("output_id"));
    const bool muted   = static_cast<bool> (mutedVar);

    const int channelIndex = detail::resolveChannelIndex (state, inputId);
    if (channelIndex < 0)
        return ToolResult::error ("invalid_args", "input_id is not a live input channel: " + juce::String (inputId));

    const int numOutputs = state.getNumOutputChannels();
    if (outputId < 1 || outputId > numOutputs)
        return ToolResult::error ("invalid_args", "output_id out of range: " + juce::String (outputId)
                                                     + " (outputs are 1-" + juce::String (numOutputs) + ")");

    const juce::String before = WFSValueTreeState::normaliseMuteList (
        state.getInputParameter (channelIndex, WFSParameterIDs::inputMutes), numOutputs);

    if (! state.setInputOutputMute (channelIndex, outputId - 1, muted))
        return ToolResult::error ("internal_error", "Input " + juce::String (inputId) + " has no mute list");

    const juce::String after = state.getInputParameter (channelIndex, WFSParameterIDs::inputMutes).toString();

    if (record != nullptr)
    {
        record->operatorDescription = juce::String (muted ? "Muted" : "Unmuted") + " input "
                                      + juce::String (inputId) + " on output " + juce::String (outputId);
        record->affectedParameters.add ("inputMutes");
        record->affectedGroups.push_back ({ inputId, "Mutes" });

        // The whole list, so undo restores it exactly.
        auto beforeObj = std::make_unique<juce::DynamicObject>();
        beforeObj->setProperty ("inputMutes", before);
        record->beforeState = juce::var (beforeObj.release());

        auto afterObj = std::make_unique<juce::DynamicObject>();
        afterObj->setProperty ("inputMutes", after);
        record->afterState = juce::var (afterObj.release());
    }

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("input_id",  inputId);
    result->setProperty ("output_id", outputId);
    result->setProperty ("muted",     muted);
    result->setProperty ("mutes",     after);
    return ToolResult::ok (juce::var (result.release()));
}

inline ToolDescriptor describeSetOutputMute (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "input_set_output_mute";
    d.description = "Mute or unmute one input on one output (speaker), leaving its other "
                    "outputs as they are. For example 'mute input 3 on output 12'.";
    d.inputSchema   = buildSetOutputMuteSchema();
    d.modifiesState = true;
    d.tier        = 1;  // one routing point, undoable; input_set_mutes is tier 1 too
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        return setOutputMute (state, args, record);
    };
    return d;
}

//==============================================================================
// input.position.set_cartesian
//==============================================================================

inline juce::var buildSetCartesianSchema()
{
    auto inputId = std::make_unique<juce::DynamicObject>();
    inputId->setProperty ("type", "integer");
    inputId->setProperty ("minimum", 1);
    inputId->setProperty ("description", "Input channel number (1-based).");

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("input_id", juce::var (inputId.release()));
    props->setProperty ("x", detail::positionRangeSchema());
    props->setProperty ("y", detail::positionRangeSchema());
    props->setProperty ("z", detail::positionRangeSchema());

    auto required = juce::Array<juce::var>();
    required.add ("input_id");
    required.add ("x");
    required.add ("y");
    required.add ("z");

    auto schema = std::make_unique<juce::DynamicObject>();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (props.release()));
    schema->setProperty ("required", juce::var (required));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema.release());
}

inline ToolResult setCartesian (WFSValueTreeState& state, const juce::var& args, ChangeRecord* record)
{
    if (! args.isObject())
        return ToolResult::error ("invalid_args", "Arguments must be a JSON object");

    auto* obj = args.getDynamicObject();
    int inputId = static_cast<int> (obj->getProperty ("input_id"));
    int channelIndex = detail::resolveChannelIndex (state, inputId);
    if (channelIndex < 0)
        return ToolResult::error ("invalid_args", "input_id out of range: " + juce::String (inputId));

    // Clamp at the server boundary — LLMs sometimes emit out-of-range values.
    constexpr float kStageMax = 50.0f;
    float x = juce::jlimit (-kStageMax, kStageMax, static_cast<float> (obj->getProperty ("x")));
    float y = juce::jlimit (-kStageMax, kStageMax, static_cast<float> (obj->getProperty ("y")));
    float z = juce::jlimit (-kStageMax, kStageMax, static_cast<float> (obj->getProperty ("z")));

    float oldX = static_cast<float> (state.getInputParameter (channelIndex, WFSParameterIDs::inputPositionX));
    float oldY = static_cast<float> (state.getInputParameter (channelIndex, WFSParameterIDs::inputPositionY));
    float oldZ = static_cast<float> (state.getInputParameter (channelIndex, WFSParameterIDs::inputPositionZ));

    state.setInputParameter (channelIndex, WFSParameterIDs::inputPositionX, x);
    state.setInputParameter (channelIndex, WFSParameterIDs::inputPositionY, y);
    state.setInputParameter (channelIndex, WFSParameterIDs::inputPositionZ, z);

    juce::String displayName =
        state.getInputParameter (channelIndex, WFSParameterIDs::inputName).toString();
    juce::String pretty = displayName.isNotEmpty()
                            ? " (" + displayName + ")"
                            : juce::String();

    if (record != nullptr)
    {
        record->operatorDescription = "Moved input " + juce::String (inputId) + pretty
                                       + " to x=" + juce::String (x, 2)
                                       + ", y="   + juce::String (y, 2)
                                       + ", z="   + juce::String (z, 2);
        record->affectedParameters.add ("inputPositionX");
        record->affectedParameters.add ("inputPositionY");
        record->affectedParameters.add ("inputPositionZ");
        record->affectedGroups.push_back ({ inputId, "Position" });

        auto before = std::make_unique<juce::DynamicObject>();
        before->setProperty ("inputPositionX", oldX);
        before->setProperty ("inputPositionY", oldY);
        before->setProperty ("inputPositionZ", oldZ);
        record->beforeState = juce::var (before.release());

        auto after = std::make_unique<juce::DynamicObject>();
        after->setProperty ("inputPositionX", x);
        after->setProperty ("inputPositionY", y);
        after->setProperty ("inputPositionZ", z);
        record->afterState = juce::var (after.release());
    }

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("input_id", inputId);
    result->setProperty ("x", x);
    result->setProperty ("y", y);
    result->setProperty ("z", z);
    return ToolResult::ok (juce::var (result.release()));
}

inline ToolDescriptor describeSetCartesian (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "input_position_set_cartesian";
    d.description = "Move an input source to absolute Cartesian coordinates "
                    "(meters). Coordinate convention: +X = stage right, "
                    "+Y = upstage (away from audience), +Z = up. Origin is "
                    "the user-configured stage origin. Out-of-range values "
                    "are clamped to the +/-50 m stage envelope.";
    d.inputSchema   = buildSetCartesianSchema();
    d.modifiesState = true;
    d.tier        = 1;  // moves are core spatialisation; AI undo handles reversal
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        return setCartesian (state, args, record);
    };
    return d;
}

//==============================================================================
// input.set_attenuation
//==============================================================================

inline juce::var buildSetAttenuationSchema()
{
    auto inputId = std::make_unique<juce::DynamicObject>();
    inputId->setProperty ("type", "integer");
    inputId->setProperty ("minimum", 1);
    inputId->setProperty ("description", "Input channel number (1-based).");

    auto db = std::make_unique<juce::DynamicObject>();
    db->setProperty ("type", "number");
    db->setProperty ("minimum", -92.0);
    db->setProperty ("maximum",   0.0);
    db->setProperty ("description", "Attenuation in dB. 0 = unity, -92 = effectively muted.");

    // Phase 8: tier-2 tools require a two-step confirmation handshake.
    // The first call returns a confirmation_token in tier_enforcement; the
    // AI re-calls with confirm set to that token to actually execute.
    // additionalProperties: false forces us to declare it explicitly.
    auto confirm = std::make_unique<juce::DynamicObject>();
    confirm->setProperty ("type", "string");
    confirm->setProperty ("description",
        "Confirmation token returned by the previous call to this tool. "
        "Tier 2 tools require a two-step handshake: the first call returns a "
        "confirmation_token in tier_enforcement; re-call with confirm set to "
        "that token (within 30 seconds) to actually execute. Omit on the first call.");

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("input_id", juce::var (inputId.release()));
    props->setProperty ("db",       juce::var (db.release()));
    props->setProperty ("confirm",  juce::var (confirm.release()));

    auto required = juce::Array<juce::var>();
    required.add ("input_id");
    required.add ("db");

    auto schema = std::make_unique<juce::DynamicObject>();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (props.release()));
    schema->setProperty ("required", juce::var (required));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema.release());
}

inline ToolResult setAttenuation (WFSValueTreeState& state, const juce::var& args, ChangeRecord* record)
{
    if (! args.isObject())
        return ToolResult::error ("invalid_args", "Arguments must be a JSON object");

    auto* obj = args.getDynamicObject();
    int inputId = static_cast<int> (obj->getProperty ("input_id"));
    int channelIndex = detail::resolveChannelIndex (state, inputId);
    if (channelIndex < 0)
        return ToolResult::error ("invalid_args", "input_id out of range: " + juce::String (inputId));

    float db = juce::jlimit (-92.0f, 0.0f, static_cast<float> (obj->getProperty ("db")));
    float oldDb = static_cast<float> (state.getInputParameter (channelIndex, WFSParameterIDs::inputAttenuation));

    state.setInputParameter (channelIndex, WFSParameterIDs::inputAttenuation, db);

    juce::String displayName =
        state.getInputParameter (channelIndex, WFSParameterIDs::inputName).toString();
    juce::String pretty = displayName.isNotEmpty()
                            ? " (" + displayName + ")"
                            : juce::String();

    if (record != nullptr)
    {
        record->operatorDescription = "Set input " + juce::String (inputId) + pretty
                                       + " attenuation to " + juce::String (db, 1) + " dB";
        record->affectedParameters.add ("inputAttenuation");
        record->affectedGroups.push_back ({ inputId, "Attenuation" });

        auto before = std::make_unique<juce::DynamicObject>();
        before->setProperty ("inputAttenuation", oldDb);
        record->beforeState = juce::var (before.release());

        auto after = std::make_unique<juce::DynamicObject>();
        after->setProperty ("inputAttenuation", db);
        record->afterState = juce::var (after.release());
    }

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("input_id", inputId);
    result->setProperty ("db", db);
    return ToolResult::ok (juce::var (result.release()));
}

inline ToolDescriptor describeSetAttenuation (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "input_set_attenuation";
    d.description = juce::String ("Set an input channel's attenuation in dB. Range "
                    "[-92, 0] where 0 = unity gain.")
                  + kTier2DescriptionSuffix;
    d.inputSchema   = buildSetAttenuationSchema();
    d.modifiesState = true;
    d.tier        = 2;  // wide dB range can produce sudden loud output — confirm
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        return setAttenuation (state, args, record);
    };
    return d;
}

} // namespace WFSNetwork::Tools::Input
