#pragma once

#include <JuceHeader.h>
#include <cmath>
#include "../MCPCompat.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"
#include "../../../Parameters/WFSParameterDefaults.h"
#include "InputTools.h"

/*
    An input's sends into the effect channels, over MCP.

    THE CELL, NOT THE ROW. An effect stores its input sends as two packed
    rows on its <Sends> node (effectSendLevels / effectSendOns), one column
    per input PERMANENT number, and no generated tool reaches them: the
    codegen reads no effects CSV, and a row is a string the generic setter
    would take whole. These three tools address ONE cell the way the Inputs
    tab's Effect Sends strips and /wfs/effect/sendLevel do, through the typed
    accessors that canonicalise the row and key it by permanent number.

    THE LEVEL AND THE SWITCH ARE TWO TOOLS, at the CSV's two tiers: a dB write
    confirms (tier 2, as every other level), a switch executes (tier 1), and
    an agent can mute a send and bring it back at the level it had.

    UNDO: the record carries a sub-write against the EFFECT's dense index,
    with the whole row before and after, so the undo engine puts the row back
    on the effect - not on an input, which the legacy single-channel path
    would resolve the input_id to.
*/
namespace WFSNetwork::Tools::EffectSend
{

namespace detail
{
    inline juce::var integerArg (const juce::String& description, int minimum)
    {
        auto v = std::make_unique<juce::DynamicObject>();
        v->setProperty ("type", "integer");
        v->setProperty ("minimum", minimum);
        v->setProperty ("description", description);
        return juce::var (v.release());
    }

    inline juce::var schemaOf (std::unique_ptr<juce::DynamicObject> props, std::initializer_list<const char*> required)
    {
        juce::Array<juce::var> req;
        for (auto* r : required)
            req.add (r);

        auto schema = std::make_unique<juce::DynamicObject>();
        schema->setProperty ("type", "object");
        schema->setProperty ("properties", juce::var (props.release()));
        schema->setProperty ("required", juce::var (req));
        schema->setProperty ("additionalProperties", false);
        return juce::var (schema.release());
    }

    inline bool isNumber (const juce::var& v) { return v.isInt() || v.isInt64() || v.isDouble(); }

    /** The dispatcher does not enforce `required`: a missing argument casts
        to 0 or false and the write goes through with the wrong meaning. */
    inline bool missing (const juce::DynamicObject& obj, const char* name)
    {
        return ! obj.hasProperty (name) || obj.getProperty (name).isVoid();
    }

    /** A whole number within [minimum, maximum], or an explanation. The loose
        forms a client emits ("3", 3.0) are accepted as the generated tools
        accept them. */
    inline std::optional<int> wholeNumber (const juce::var& raw, const char* name, int minimum, int maximum,
                                           juce::String& errorCode, juce::String& errorMessage)
    {
        const auto text = raw.toString().trim();
        double d = 0.0;
        if (isNumber (raw))
            d = static_cast<double> (raw);
        else if (raw.isString() && text.isNotEmpty() && text.containsOnly ("0123456789.+-eE"))
            d = text.getDoubleValue();
        else
        {
            errorCode = "invalid_args";
            errorMessage = juce::String (name) + " must be an integer, got " + text.quoted();
            return std::nullopt;
        }

        if (! std::isfinite (d) || std::floor (d) != d)
        {
            errorCode = "invalid_args";
            errorMessage = juce::String (name) + " must be an integer, got " + text.quoted();
            return std::nullopt;
        }
        if (d < minimum || d > maximum)
        {
            errorCode = "out_of_range";
            errorMessage = juce::String (name) + " " + text + " not in [" + juce::String (minimum)
                         + ", " + juce::String (maximum) + "]";
            return std::nullopt;
        }
        return static_cast<int> (d);
    }

    struct Target
    {
        int inputId = 0;        // permanent number
        int inputSlot = -1;     // dense slot
        int effectId = 0;       // 1-based
        int effectIndex = -1;   // dense
    };

    /** Resolve input_id and effect_id, or answer with the error to return. */
    inline std::optional<Target> resolveTarget (WFSValueTreeState& state, const juce::DynamicObject& obj,
                                                juce::String& errorCode, juce::String& errorMessage)
    {
        Target t;
        juce::String code, message;

        const auto inputId = wholeNumber (obj.getProperty ("input_id"), "input_id", 1,
                                          WFSParameterDefaults::maxInputChannels, code, message);
        if (! inputId.has_value())
        {
            errorCode = code; errorMessage = message;
            return std::nullopt;
        }
        t.inputId = *inputId;
        t.inputSlot = Input::detail::resolveChannelIndex (state, t.inputId);
        if (t.inputSlot < 0)
        {
            errorCode = "invalid_args";
            errorMessage = "input_id is not a live input channel: " + juce::String (t.inputId);
            return std::nullopt;
        }

        const int numEffects = state.getNumEffectChannels();
        if (numEffects <= 0)
        {
            errorCode = "invalid_args";
            errorMessage = "The session has no effect channels (set an Effects Channels count first)";
            return std::nullopt;
        }
        const auto effectId = wholeNumber (obj.getProperty ("effect_id"), "effect_id", 1, numEffects, code, message);
        if (! effectId.has_value())
        {
            errorCode = code;
            errorMessage = message + " (effects are 1-" + juce::String (numEffects) + ")";
            return std::nullopt;
        }
        t.effectId = *effectId;
        t.effectIndex = t.effectId - 1;
        return t;
    }

    inline juce::String inputDisplay (WFSValueTreeState& state, const Target& t)
    {
        const auto name = state.getInputParameter (t.inputSlot, WFSParameterIDs::inputName).toString();
        return "input " + juce::String (t.inputId) + (name.isNotEmpty() ? " (" + name + ")" : juce::String());
    }

    inline juce::String effectDisplay (WFSValueTreeState& state, int effectIndex)
    {
        const auto name = state.getEffectParameter (effectIndex, WFSParameterIDs::effectName).toString();
        return "effect " + juce::String (effectIndex + 1) + (name.isNotEmpty() ? " (" + name + ")" : juce::String());
    }

    inline juce::var rowPayload (const juce::Identifier& rowId, const juce::String& row)
    {
        auto obj = std::make_unique<juce::DynamicObject>();
        obj->setProperty (rowId, row);
        return juce::var (obj.release());
    }

    /** One sub-write on the effect's dense index, whole row before and after,
        plus the top-level states the staleness check reads. */
    inline void fillRecord (ChangeRecord& record, const juce::String& description, const Target& t,
                            const juce::Identifier& rowId, const juce::String& before, const juce::String& after)
    {
        record.operatorDescription = description;
        record.affectedParameters.add (rowId.toString());
        record.affectedGroups.push_back ({ t.inputId, "Effect Sends" });
        record.beforeState = rowPayload (rowId, before);
        record.afterState  = rowPayload (rowId, after);

        ChangeSubWrite sw;
        sw.channelIndex  = t.effectIndex;
        sw.channelNumber = 0;              // effects are addressed by dense slot
        sw.beforeState   = rowPayload (rowId, before);
        sw.afterState    = rowPayload (rowId, after);
        record.subWrites.push_back (std::move (sw));
    }

    inline juce::var cellResult (WFSValueTreeState& state, const Target& t)
    {
        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("input_id",  t.inputId);
        result->setProperty ("effect_id", t.effectId);
        result->setProperty ("on",        state.getEffectSendOnFromInput (t.effectIndex, t.inputId));
        result->setProperty ("level_db",  state.getEffectSendLevelFromInput (t.effectIndex, t.inputId));
        return juce::var (result.release());
    }
}

//==============================================================================
// input.set_effect_send_level
//==============================================================================

inline juce::var buildSetLevelSchema()
{
    auto level = std::make_unique<juce::DynamicObject>();
    level->setProperty ("type", "number");
    level->setProperty ("minimum", WFSParameterDefaults::effectSendLevelMin);
    level->setProperty ("maximum", WFSParameterDefaults::effectSendLevelMax);
    level->setProperty ("description", "Send level in dB, -92 (silent) to 0 (unity). The send's on/off switch is left as it is.");

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("input_id",  detail::integerArg ("Input channel number (1-based, permanent).", 1));
    props->setProperty ("effect_id", detail::integerArg ("Effect channel number (1-based).", 1));
    props->setProperty ("level_db",  juce::var (level.release()));
    return detail::schemaOf (std::move (props), { "input_id", "effect_id", "level_db" });
}

inline ToolResult setLevel (WFSValueTreeState& state, const juce::var& args, ChangeRecord* record)
{
    if (! args.isObject())
        return ToolResult::error ("invalid_args", "Arguments must be a JSON object");
    auto* obj = args.getDynamicObject();

    for (const char* arg : { "input_id", "effect_id", "level_db" })
        if (detail::missing (*obj, arg))
            return ToolResult::error ("invalid_args", juce::String ("Missing required arg: ") + arg);

    const auto raw = obj->getProperty ("level_db");
    double level = 0.0;
    if (detail::isNumber (raw))
        level = static_cast<double> (raw);
    else if (raw.isString() && raw.toString().trim().containsOnly ("0123456789.+-eE") && raw.toString().trim().isNotEmpty())
        level = raw.toString().getDoubleValue();
    else
        return ToolResult::error ("invalid_args", "level_db must be a number in dB, got " + raw.toString().quoted());

    if (! std::isfinite (level)
        || level < WFSParameterDefaults::effectSendLevelMin || level > WFSParameterDefaults::effectSendLevelMax)
        return ToolResult::error ("out_of_range", "level_db " + juce::String (level, 2) + " not in ["
                                                     + juce::String (WFSParameterDefaults::effectSendLevelMin, 1) + ", "
                                                     + juce::String (WFSParameterDefaults::effectSendLevelMax, 1) + "] dB");

    juce::String code, message;
    const auto target = detail::resolveTarget (state, *obj, code, message);
    if (! target.has_value())
        return ToolResult::error (code, message);
    const auto& t = *target;

    const juce::String before = state.getEffectParameter (t.effectIndex, WFSParameterIDs::effectSendLevels).toString();
    if (! state.setEffectSendLevelFromInput (t.effectIndex, t.inputId, static_cast<float> (level)))
        return ToolResult::error ("internal_error", "Effect " + juce::String (t.effectId) + " has no send rows");
    const juce::String after = state.getEffectParameter (t.effectIndex, WFSParameterIDs::effectSendLevels).toString();

    if (record != nullptr)
        detail::fillRecord (*record,
                            "Set the send from " + detail::inputDisplay (state, t) + " into "
                                + detail::effectDisplay (state, t.effectIndex) + " to " + juce::String (level, 1) + " dB",
                            t, WFSParameterIDs::effectSendLevels, before, after);

    return ToolResult::ok (detail::cellResult (state, t));
}

inline ToolDescriptor describeSetLevel (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "input_set_effect_send_level";
    d.description = "Set the level (dB) of one input's send into one effect channel, leaving the "
                    "send's on/off switch and every other send as they are. For example 'send input 3 "
                    "into effect 2 at -6 dB'. Use input_set_effect_send_on to switch the send.";
    d.inputSchema   = buildSetLevelSchema();
    d.modifiesState = true;
    d.tier        = 2;  // a level write, as input_set_attenuation and the array attenuations
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        return setLevel (state, args, record);
    };
    return d;
}

//==============================================================================
// input.set_effect_send_on
//==============================================================================

inline juce::var buildSetOnSchema()
{
    auto on = std::make_unique<juce::DynamicObject>();
    on->setProperty ("type", "boolean");
    on->setProperty ("description", "true switches the send on, false off. The level is kept.");

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("input_id",  detail::integerArg ("Input channel number (1-based, permanent).", 1));
    props->setProperty ("effect_id", detail::integerArg ("Effect channel number (1-based).", 1));
    props->setProperty ("on",        juce::var (on.release()));
    return detail::schemaOf (std::move (props), { "input_id", "effect_id", "on" });
}

inline ToolResult setOn (WFSValueTreeState& state, const juce::var& args, ChangeRecord* record)
{
    if (! args.isObject())
        return ToolResult::error ("invalid_args", "Arguments must be a JSON object");
    auto* obj = args.getDynamicObject();

    for (const char* arg : { "input_id", "effect_id", "on" })
        if (detail::missing (*obj, arg))
            return ToolResult::error ("invalid_args", juce::String ("Missing required arg: ") + arg);

    const auto onVar = obj->getProperty ("on");
    if (! (onVar.isBool() || detail::isNumber (onVar)))
        return ToolResult::error ("invalid_args", "on must be true or false");
    const bool on = static_cast<bool> (onVar);

    juce::String code, message;
    const auto target = detail::resolveTarget (state, *obj, code, message);
    if (! target.has_value())
        return ToolResult::error (code, message);
    const auto& t = *target;

    const juce::String before = state.getEffectParameter (t.effectIndex, WFSParameterIDs::effectSendOns).toString();
    if (! state.setEffectSendOnFromInput (t.effectIndex, t.inputId, on))
        return ToolResult::error ("internal_error", "Effect " + juce::String (t.effectId) + " has no send rows");
    const juce::String after = state.getEffectParameter (t.effectIndex, WFSParameterIDs::effectSendOns).toString();

    if (record != nullptr)
        detail::fillRecord (*record,
                            juce::String (on ? "Switched on" : "Switched off") + " the send from "
                                + detail::inputDisplay (state, t) + " into " + detail::effectDisplay (state, t.effectIndex),
                            t, WFSParameterIDs::effectSendOns, before, after);

    return ToolResult::ok (detail::cellResult (state, t));
}

inline ToolDescriptor describeSetOn (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "input_set_effect_send_on";
    d.description = "Switch one input's send into one effect channel on or off without changing its "
                    "level, leaving every other send as it is. For example 'mute the send from input 3 "
                    "into effect 2'. Use input_set_effect_send_level for the level.";
    d.inputSchema   = buildSetOnSchema();
    d.modifiesState = true;
    d.tier        = 1;  // one routing point, undoable; input_set_output_mute is tier 1 too
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        return setOn (state, args, record);
    };
    return d;
}

//==============================================================================
// input.get_effect_sends
//==============================================================================

inline juce::var buildGetSchema()
{
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("input_id", detail::integerArg ("Input channel number (1-based, permanent).", 1));
    return detail::schemaOf (std::move (props), { "input_id" });
}

inline ToolResult getSends (WFSValueTreeState& state, const juce::var& args)
{
    if (! args.isObject())
        return ToolResult::error ("invalid_args", "Arguments must be a JSON object");
    auto* obj = args.getDynamicObject();

    if (detail::missing (*obj, "input_id"))
        return ToolResult::error ("invalid_args", "Missing required arg: input_id");

    juce::String code, message;
    const auto inputId = detail::wholeNumber (obj->getProperty ("input_id"), "input_id", 1,
                                              WFSParameterDefaults::maxInputChannels, code, message);
    if (! inputId.has_value())
        return ToolResult::error (code, message);

    const int slot = Input::detail::resolveChannelIndex (state, *inputId);
    if (slot < 0)
        return ToolResult::error ("invalid_args", "input_id is not a live input channel: " + juce::String (*inputId));

    juce::Array<juce::var> sends;
    const int numEffects = state.getNumEffectChannels();
    for (int fx = 0; fx < numEffects; ++fx)
    {
        auto cell = std::make_unique<juce::DynamicObject>();
        cell->setProperty ("effect_id",   fx + 1);
        cell->setProperty ("effect_name", state.getEffectParameter (fx, WFSParameterIDs::effectName).toString());
        cell->setProperty ("on",          state.getEffectSendOnFromInput (fx, *inputId));
        cell->setProperty ("level_db",    state.getEffectSendLevelFromInput (fx, *inputId));
        sends.add (juce::var (cell.release()));
    }

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("input_id",   *inputId);
    result->setProperty ("input_name", state.getInputParameter (slot, WFSParameterIDs::inputName).toString());
    result->setProperty ("sends",      juce::var (sends));
    return ToolResult::ok (juce::var (result.release()));
}

inline ToolDescriptor describeGet (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "input_get_effect_sends";
    d.description = "Read one input's sends into every effect channel: for each effect, whether the "
                    "send is on and its level in dB. Read-only.";
    d.inputSchema   = buildGetSchema();
    d.modifiesState = false;
    d.tier        = 1;
    d.handler = [&state] (const juce::var& args, ChangeRecord*) -> ToolResult
    {
        return getSends (state, args);
    };
    return d;
}

} // namespace WFSNetwork::Tools::EffectSend
