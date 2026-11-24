#pragma once

#include <JuceHeader.h>
#include "../MCPCompat.h"
#include "../MCPEffectWrites.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"
#include "../../../Parameters/WFSParameterDefaults.h"
#include "ToolArgs.h"

/*
    The effect channels over MCP, beyond the one-value-per-tool generated
    surface (effect_* in generated_tools.json, reachable by name or through
    wfs_set_parameter):

      effect_get              everything about one effect in one read
      effect_set_chain_order  the module order, validated as a permutation
      effect_set_module_on    switch a module in or out by its chain token
      effect_set_fx_send_*    one effect feeding another (the Effects Matrix)
      effect_clear            flush an effect's tails and delay lines

    Every write goes the OSC way (MCPEffectWrites.h) into the Effects undo
    domain, with no link-group propagation. effect_create / effect_delete are
    the channel lifecycle tools (ChannelLifecycleTools.h).
*/
namespace WFSNetwork::Tools::Effect
{

namespace detail
{
    /** The eleven chain tokens in DECLARED slot order - the index
        getEffectModuleSection / getEffectModuleType take. */
    inline const juce::StringArray& slotTokens()
    {
        static const juce::StringArray t { "dist", "eq1", "eq2", "dyn1", "dyn2", "mod",
                                           "phaser", "trem", "reverb", "delay", "crush" };
        return t;
    }

    /** What each module is, for a reader who has not seen the Effects tab. */
    inline juce::String moduleLabel (const juce::String& token)
    {
        if (token == "dist")   return "Distortion";
        if (token == "eq1")    return "EQ 1";
        if (token == "eq2")    return "EQ 2";
        if (token == "dyn1")   return "Dynamics 1";
        if (token == "dyn2")   return "Dynamics 2";
        if (token == "mod")    return "Chorus / Flanger";
        if (token == "phaser") return "Phaser";
        if (token == "trem")   return "Tremolo";
        if (token == "reverb") return "Reverb";
        if (token == "delay")  return "Delay";
        if (token == "crush")  return "Bit crusher";
        return token;
    }

    inline const juce::Identifier& bypassIdFor (const juce::String& token)
    {
        using namespace WFSParameterIDs;
        if (token == "dist")                     return effectDistBypass;
        if (token == "eq1" || token == "eq2")    return effectEQBypass;
        if (token == "dyn1" || token == "dyn2")  return effectDynBypass;
        if (token == "mod")                      return effectModBypass;
        if (token == "phaser")                   return effectPhaserBypass;
        if (token == "trem")                     return effectTremBypass;
        if (token == "reverb")                   return effectReverbBypass;
        if (token == "delay")                    return effectDelayBypass;
        return effectCrushBypass;
    }

    /** A module's two-instance kinds share property names, so a write by name
        through the channel cannot tell eq1 from eq2. */
    inline bool isInstanced (const juce::String& token)
    {
        return token == "eq1" || token == "eq2" || token == "dyn1" || token == "dyn2";
    }

    inline juce::var properties (const juce::ValueTree& node)
    {
        auto obj = std::make_unique<juce::DynamicObject>();
        for (int i = 0; i < node.getNumProperties(); ++i)
        {
            const auto name = node.getPropertyName (i);
            if (name == WFSParameterIDs::id)
                continue;
            obj->setProperty (name, node.getProperty (name));
        }
        return juce::var (obj.release());
    }

    struct Resolved { int id = 0; int index = -1; };

    inline std::optional<Resolved> resolveEffect (WFSValueTreeState& state, const juce::DynamicObject& obj,
                                                  const char* argName, juce::String& code, juce::String& message)
    {
        if (Args::missing (obj, argName))
        {
            code = "invalid_args";
            message = juce::String ("Missing required arg: ") + argName;
            return std::nullopt;
        }
        const int numEffects = state.getNumEffectChannels();
        if (numEffects <= 0)
        {
            code = "invalid_args";
            message = "The session has no effect channels (create one with effect_create first)";
            return std::nullopt;
        }
        const auto id = Args::wholeNumber (obj.getProperty (argName), argName, 1, numEffects, code, message);
        if (! id.has_value())
        {
            message += " (effects are 1-" + juce::String (numEffects) + ")";
            return std::nullopt;
        }
        return Resolved { *id, *id - 1 };
    }

    inline juce::String effectDisplay (WFSValueTreeState& state, int index)
    {
        const auto name = state.getEffectParameter (index, WFSParameterIDs::effectName).toString();
        return "effect " + juce::String (index + 1) + (name.isNotEmpty() ? " (" + name + ")" : juce::String());
    }

    inline juce::var payload (const juce::Identifier& id, const juce::var& value)
    {
        auto obj = std::make_unique<juce::DynamicObject>();
        obj->setProperty (id, value);
        return juce::var (obj.release());
    }

    /** A record the undo engine reverses through MCPEffect::writeChannelValue:
        one parameter on the effect's dense index. */
    inline void fillRecord (ChangeRecord& record, const juce::String& description, int effectIndex,
                            const juce::String& group, const juce::Identifier& id,
                            const juce::var& before, const juce::var& after)
    {
        record.operatorDescription = description;
        record.affectedParameters.add (id.toString());
        record.affectedGroups.push_back ({ effectIndex + 1, group });
        record.beforeState = payload (id, before);
        record.afterState  = payload (id, after);

        ChangeSubWrite sw;
        sw.channelIndex  = effectIndex;
        sw.channelNumber = 0;             // effects are addressed by dense slot
        sw.beforeState   = payload (id, before);
        sw.afterState    = payload (id, after);
        record.subWrites.push_back (std::move (sw));
    }
}

//==============================================================================
// effect_get
//==============================================================================

inline ToolResult get (WFSValueTreeState& state, const juce::var& args)
{
    auto* obj = args.getDynamicObject();
    if (obj == nullptr)
        return ToolResult::error ("invalid_args", "Arguments must be a JSON object");

    juce::String code, message;
    const auto fx = detail::resolveEffect (state, *obj, "effect_id", code, message);
    if (! fx.has_value())
        return ToolResult::error (code, message);

    auto effect = state.getEffectState (fx->index);
    if (! effect.isValid())
        return ToolResult::error ("internal_error", "Effect " + juce::String (fx->id) + " has no state");

    // Module node types, to tell them from the channel's other sections.
    juce::Array<juce::Identifier> moduleTypes;
    for (int i = 0; i < detail::slotTokens().size(); ++i)
        moduleTypes.add (WFSValueTreeState::getEffectModuleType (i));

    // The channel's own sections: Channel, Position, Feed, Return, AutomOtion,
    // LFO, Chain. Sends is left to the two lists below; its rows are packed.
    auto sections = std::make_unique<juce::DynamicObject>();
    for (int i = 0; i < effect.getNumChildren(); ++i)
    {
        const auto child = effect.getChild (i);
        if (moduleTypes.contains (child.getType()) || child.hasType (WFSParameterIDs::Sends))
            continue;
        sections->setProperty (child.getType(), detail::properties (child));
    }

    // The chain in PLAYING order, each module with its whole state.
    const auto orderText = state.getEffectParameter (fx->index, WFSParameterIDs::effectChainOrder).toString();
    auto order = juce::StringArray::fromTokens (orderText, ",", "");
    order.trim();
    order.removeEmptyStrings();
    if (order.size() != detail::slotTokens().size())
        order = detail::slotTokens();

    juce::Array<juce::var> chain;
    for (const auto& token : order)
    {
        const int slot = detail::slotTokens().indexOf (token);
        if (slot < 0)
            continue;
        auto node = state.getEffectModuleSection (fx->index, slot);
        if (! node.isValid())
            continue;

        auto module = std::make_unique<juce::DynamicObject>();
        module->setProperty ("module", token);
        module->setProperty ("label",  detail::moduleLabel (token));
        module->setProperty ("on", static_cast<int> (node.getProperty (detail::bypassIdFor (token))) == 0);
        module->setProperty ("parameters", detail::properties (node));

        juce::Array<juce::var> bands, taps;
        for (int c = 0; c < node.getNumChildren(); ++c)
        {
            const auto child = node.getChild (c);
            if (child.hasType (WFSParameterIDs::Band))
                bands.add (detail::properties (child));
            else if (child.hasType (WFSParameterIDs::Tap))
                taps.add (detail::properties (child));
        }
        if (! bands.isEmpty()) module->setProperty ("bands", juce::var (bands));
        if (! taps.isEmpty())  module->setProperty ("taps",  juce::var (taps));
        chain.add (juce::var (module.release()));
    }

    // Who feeds it, and whom it feeds - only the sends that are switched on.
    juce::Array<juce::var> fromInputs, fromEffects, intoEffects;
    for (int slot = 0; slot < state.getNumInputChannels(); ++slot)
    {
        const int number = state.getInputChannelNumber (slot);
        if (! state.getEffectSendOnFromInput (fx->index, number))
            continue;
        auto s = std::make_unique<juce::DynamicObject>();
        s->setProperty ("input_id", number);
        s->setProperty ("name",     state.getInputParameter (slot, WFSParameterIDs::inputName).toString());
        s->setProperty ("level_db", state.getEffectSendLevelFromInput (fx->index, number));
        fromInputs.add (juce::var (s.release()));
    }
    for (int other = 0; other < state.getNumEffectChannels(); ++other)
    {
        if (other == fx->index)
            continue;
        if (state.getEffectFxSendOnFromEffect (fx->index, other))
        {
            auto s = std::make_unique<juce::DynamicObject>();
            s->setProperty ("effect_id", other + 1);
            s->setProperty ("level_db",  state.getEffectFxSendLevelFromEffect (fx->index, other));
            fromEffects.add (juce::var (s.release()));
        }
        if (state.getEffectFxSendOnFromEffect (other, fx->index))
        {
            auto s = std::make_unique<juce::DynamicObject>();
            s->setProperty ("effect_id", other + 1);
            s->setProperty ("level_db",  state.getEffectFxSendLevelFromEffect (other, fx->index));
            intoEffects.add (juce::var (s.release()));
        }
    }

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("effect_id", fx->id);
    result->setProperty ("name",      state.getEffectParameter (fx->index, WFSParameterIDs::effectName).toString());
    result->setProperty ("sections",  juce::var (sections.release()));
    result->setProperty ("chain_order", order.joinIntoString (","));
    result->setProperty ("chain",     juce::var (chain));
    result->setProperty ("fed_by_inputs",  juce::var (fromInputs));
    result->setProperty ("fed_by_effects", juce::var (fromEffects));
    result->setProperty ("feeds_effects",  juce::var (intoEffects));
    return ToolResult::ok (juce::var (result.release()));
}

inline ToolDescriptor describeGet (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "effect_get";
    d.description = "Read everything about one effect channel: its sections (Channel: name, "
                    "attenuation, mute, link group; Position; Feed; Return; AutomOtion; LFO; "
                    "Chain), its modules in PLAYING order with on/off and every parameter (EQ "
                    "bands and delay taps included), and the switched-on sends feeding it from "
                    "inputs and other effects and from it into other effects. Parameter names are "
                    "the variables wfs_set_parameter and the effect_* tools take. Read-only. See "
                    "resource wfs://knowledge/effects for what effects are for.";
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("effect_id", Args::integerArg ("Effect channel number (1-based).", 1,
                                                       WFSParameterDefaults::maxEffectChannels));
    d.inputSchema   = Args::schemaOf (std::move (props), { "effect_id" });
    d.modifiesState = false;
    d.tier          = 1;
    d.handler = [&state] (const juce::var& args, ChangeRecord*) -> ToolResult { return get (state, args); };
    return d;
}

//==============================================================================
// effect_set_chain_order
//==============================================================================

inline ToolResult setChainOrder (WFSValueTreeState& state, const juce::var& args, ChangeRecord* record)
{
    auto* obj = args.getDynamicObject();
    if (obj == nullptr || Args::missing (*obj, "order"))
        return ToolResult::error ("invalid_args", "Missing required arg: order");

    juce::String code, message;
    const auto fx = detail::resolveEffect (state, *obj, "effect_id", code, message);
    if (! fx.has_value())
        return ToolResult::error (code, message);

    juce::StringArray order;
    if (const auto* arr = obj->getProperty ("order").getArray())
        for (const auto& t : *arr)
            order.add (t.toString().trim().toLowerCase());
    else
        order = juce::StringArray::fromTokens (obj->getProperty ("order").toString().toLowerCase(), ",", "");
    order.trim();
    order.removeEmptyStrings();

    const auto& tokens = detail::slotTokens();
    juce::StringArray missingTokens;
    for (const auto& t : tokens)
        if (! order.contains (t))
            missingTokens.add (t);
    juce::StringArray unknown;
    for (const auto& t : order)
        if (! tokens.contains (t))
            unknown.add (t);
    if (order.size() != tokens.size() || ! missingTokens.isEmpty() || ! unknown.isEmpty())
        return ToolResult::error ("invalid_args",
            "order must list each of the 11 modules exactly once ("
            + tokens.joinIntoString (", ") + ")"
            + (unknown.isEmpty() ? juce::String() : "; unknown: " + unknown.joinIntoString (", "))
            + (missingTokens.isEmpty() ? juce::String() : "; missing: " + missingTokens.joinIntoString (", ")));

    const auto before = state.getEffectParameter (fx->index, WFSParameterIDs::effectChainOrder);
    const auto newOrder = order.joinIntoString (",");
    MCPEffect::writeChannelValue (state, fx->index, WFSParameterIDs::effectChainOrder, newOrder);
    const auto after = state.getEffectParameter (fx->index, WFSParameterIDs::effectChainOrder);

    if (after.toString() != newOrder)
        return ToolResult::error ("internal_error", "The chain order was refused by the state; nothing changed");

    if (record != nullptr)
        detail::fillRecord (*record, "Set the chain order of " + detail::effectDisplay (state, fx->index)
                                         + " to " + newOrder,
                            fx->index, "Chain", WFSParameterIDs::effectChainOrder, before, after);

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("effect_id",   fx->id);
    result->setProperty ("chain_order", after);
    result->setProperty ("before",      before);
    return ToolResult::ok (juce::var (result.release()));
}

inline ToolDescriptor describeSetChainOrder (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "effect_set_chain_order";
    d.description = "Set the order the 11 modules of one effect play in, first to last: a "
                    "permutation of dist, eq1, eq2, dyn1, dyn2, mod, phaser, trem, reverb, delay, "
                    "crush. E.g. put delay before reverb so the echoes are reverberated. Modules "
                    "switched off still hold their place. Only this effect changes (link groups "
                    "are not followed). Undoable.";
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("effect_id", Args::integerArg ("Effect channel number (1-based).", 1,
                                                       WFSParameterDefaults::maxEffectChannels));
    auto items = std::make_unique<juce::DynamicObject>();
    items->setProperty ("type", "string");
    juce::Array<juce::var> tokenVals;
    for (const auto& t : detail::slotTokens())
        tokenVals.add (t);
    items->setProperty ("enum", juce::var (tokenVals));
    auto order = std::make_unique<juce::DynamicObject>();
    order->setProperty ("type", "array");
    order->setProperty ("items", juce::var (items.release()));
    order->setProperty ("minItems", 11);
    order->setProperty ("maxItems", 11);
    order->setProperty ("description", "All 11 module tokens, first-played first.");
    props->setProperty ("order", juce::var (order.release()));
    d.inputSchema   = Args::schemaOf (std::move (props), { "effect_id", "order" });
    d.modifiesState = true;
    d.tier          = 1;
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        return setChainOrder (state, args, record);
    };
    return d;
}

//==============================================================================
// effect_set_module_on
//==============================================================================

inline ToolResult setModuleOn (WFSValueTreeState& state, const juce::var& args, ChangeRecord* record)
{
    auto* obj = args.getDynamicObject();
    if (obj == nullptr)
        return ToolResult::error ("invalid_args", "Arguments must be a JSON object");
    for (const char* arg : { "module", "on" })
        if (Args::missing (*obj, arg))
            return ToolResult::error ("invalid_args", juce::String ("Missing required arg: ") + arg);

    juce::String code, message;
    const auto fx = detail::resolveEffect (state, *obj, "effect_id", code, message);
    if (! fx.has_value())
        return ToolResult::error (code, message);

    const auto token = obj->getProperty ("module").toString().trim().toLowerCase();
    const int slot = detail::slotTokens().indexOf (token);
    if (slot < 0)
        return ToolResult::error ("invalid_args", "module must be one of "
                                                  + detail::slotTokens().joinIntoString (", "));

    const auto on = Args::boolean (obj->getProperty ("on"), "on", code, message);
    if (! on.has_value())
        return ToolResult::error (code, message);

    auto node = state.getEffectModuleSection (fx->index, slot);
    if (! node.isValid())
        return ToolResult::error ("internal_error", "Effect " + juce::String (fx->id) + " has no " + token + " module");

    const auto& bypassId = detail::bypassIdFor (token);
    const auto before = node.getProperty (bypassId);
    MCPEffect::writeSectionValue (state, node, bypassId, *on ? 0 : 1);
    const auto after = node.getProperty (bypassId);

    if (record != nullptr)
    {
        const auto description = juce::String (*on ? "Switched on " : "Switched off ") + detail::moduleLabel (token)
                                + " on " + detail::effectDisplay (state, fx->index);
        detail::fillRecord (*record, description, fx->index, "Chain", bypassId, before, after);

        // eq1/eq2 and dyn1/dyn2 share their names: a reversal by name would
        // land on whichever instance comes first. Call this tool again instead.
        if (detail::isInstanced (token))
        {
            record->undoable = false;
            record->operatorDescription += " (not undoable: switch it back with effect_set_module_on)";
        }
    }

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("effect_id", fx->id);
    result->setProperty ("module",    token);
    result->setProperty ("on",        static_cast<int> (after) == 0);
    return ToolResult::ok (juce::var (result.release()));
}

inline ToolDescriptor describeSetModuleOn (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "effect_set_module_on";
    d.description = "Switch one module of an effect's chain on or off (every module starts off). "
                    "module is its chain token: dist, eq1, eq2, dyn1, dyn2, mod (chorus/flanger), "
                    "phaser, trem, reverb, delay, crush. Its settings are kept. Only this effect "
                    "changes (link groups are not followed). Undoable, except eq1/eq2/dyn1/dyn2: "
                    "switch those back with this tool.";
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("effect_id", Args::integerArg ("Effect channel number (1-based).", 1,
                                                       WFSParameterDefaults::maxEffectChannels));
    props->setProperty ("module", Args::enumArg ("The module's chain token.", detail::slotTokens()));
    props->setProperty ("on", Args::booleanArg ("true switches the module in, false bypasses it."));
    d.inputSchema   = Args::schemaOf (std::move (props), { "effect_id", "module", "on" });
    d.modifiesState = true;
    d.tier          = 1;
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        return setModuleOn (state, args, record);
    };
    return d;
}

//==============================================================================
// effect_set_fx_send_level / effect_set_fx_send_on
//==============================================================================

inline ToolResult setFxSend (WFSValueTreeState& state, const juce::var& args, ChangeRecord* record, bool level)
{
    auto* obj = args.getDynamicObject();
    if (obj == nullptr)
        return ToolResult::error ("invalid_args", "Arguments must be a JSON object");
    const char* valueArg = level ? "level_db" : "on";
    if (Args::missing (*obj, valueArg))
        return ToolResult::error ("invalid_args", juce::String ("Missing required arg: ") + valueArg);

    juce::String code, message;
    const auto dest = detail::resolveEffect (state, *obj, "effect_id", code, message);
    if (! dest.has_value())
        return ToolResult::error (code, message);
    const auto src = detail::resolveEffect (state, *obj, "source_effect_id", code, message);
    if (! src.has_value())
        return ToolResult::error (code, message);
    if (src->index == dest->index)
        return ToolResult::error ("invalid_args", "An effect cannot feed itself (source_effect_id = effect_id)");

    const auto& rowId = level ? WFSParameterIDs::effectFxSendLevels : WFSParameterIDs::effectFxSendOns;
    const auto before = state.getEffectParameter (dest->index, rowId);
    bool ok = false;
    juce::String what;
    {
        WFSValueTreeState::ScopedUndoDomain scope (state, UndoDomain::Effects);
        if (level)
        {
            const auto db = Args::number (obj->getProperty ("level_db"), "level_db", code, message);
            if (! db.has_value())
                return ToolResult::error (code, message);
            if (*db < WFSParameterDefaults::effectSendLevelMin || *db > WFSParameterDefaults::effectSendLevelMax)
                return ToolResult::error ("out_of_range", "level_db " + juce::String (*db, 2) + " not in ["
                                            + juce::String (WFSParameterDefaults::effectSendLevelMin, 1) + ", "
                                            + juce::String (WFSParameterDefaults::effectSendLevelMax, 1) + "] dB");
            ok = state.setEffectFxSendLevelFromEffect (dest->index, src->index, static_cast<float> (*db));
            what = "Set the send from " + detail::effectDisplay (state, src->index) + " into "
                 + detail::effectDisplay (state, dest->index) + " to " + juce::String (*db, 1) + " dB";
        }
        else
        {
            const auto on = Args::boolean (obj->getProperty ("on"), "on", code, message);
            if (! on.has_value())
                return ToolResult::error (code, message);
            ok = state.setEffectFxSendOnFromEffect (dest->index, src->index, *on);
            what = juce::String (*on ? "Switched on" : "Switched off") + " the send from "
                 + detail::effectDisplay (state, src->index) + " into " + detail::effectDisplay (state, dest->index);
        }
    }
    if (! ok)
        return ToolResult::error ("internal_error", "The send was refused by the state; nothing changed");
    const auto after = state.getEffectParameter (dest->index, rowId);

    if (record != nullptr)
        detail::fillRecord (*record, what, dest->index, "Effect Sends", rowId, before, after);

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("effect_id",        dest->id);
    result->setProperty ("source_effect_id", src->id);
    result->setProperty ("on",       state.getEffectFxSendOnFromEffect    (dest->index, src->index));
    result->setProperty ("level_db", state.getEffectFxSendLevelFromEffect (dest->index, src->index));
    return ToolResult::ok (juce::var (result.release()));
}

inline juce::var fxSendSchema (bool level)
{
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("effect_id", Args::integerArg ("The effect RECEIVING the send (1-based).", 1,
                                                       WFSParameterDefaults::maxEffectChannels));
    props->setProperty ("source_effect_id", Args::integerArg ("The effect whose return is SENT (1-based); "
                                                              "not the same as effect_id.", 1,
                                                              WFSParameterDefaults::maxEffectChannels));
    if (level)
    {
        props->setProperty ("level_db", Args::numberArg ("Send level in dB, -92 (silent) to 0 (unity). The "
                                                         "send's on/off switch is left as it is.",
                                                         WFSParameterDefaults::effectSendLevelMin,
                                                         WFSParameterDefaults::effectSendLevelMax));
        props->setProperty ("confirm", Args::confirmArg());
        return Args::schemaOf (std::move (props), { "effect_id", "source_effect_id", "level_db" });
    }
    props->setProperty ("on", Args::booleanArg ("true switches the send on, false off. The level is kept."));
    return Args::schemaOf (std::move (props), { "effect_id", "source_effect_id", "on" });
}

inline ToolDescriptor describeSetFxSendLevel (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "effect_set_fx_send_level";
    d.description = juce::String (
        "Set the level (dB) at which one effect's return feeds another effect (the Effects "
        "Matrix), e.g. a delay feeding a reverb so the echoes are reverberated. The switch "
        "is left as it is; effect_set_fx_send_on switches it. An effect cannot feed itself; "
        "a loop through several effects is allowed but held in check by the loop guard "
        "(Effects Settings). Input sends are input_set_effect_send_level. Undoable.")
        + juce::String (kTier2DescriptionSuffix);
    d.inputSchema   = fxSendSchema (true);
    d.modifiesState = true;
    d.tier          = 2;   // a level write, as input_set_effect_send_level
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        return setFxSend (state, args, record, true);
    };
    return d;
}

inline ToolDescriptor describeSetFxSendOn (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "effect_set_fx_send_on";
    d.description = "Switch the send from one effect's return into another effect on or off (the "
                    "Effects Matrix) without changing its level. An effect cannot feed itself. "
                    "Undoable.";
    d.inputSchema   = fxSendSchema (false);
    d.modifiesState = true;
    d.tier          = 1;
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        return setFxSend (state, args, record, false);
    };
    return d;
}

//==============================================================================
// effect_clear
//==============================================================================

inline ToolDescriptor describeClear (WFSValueTreeState& state, const std::function<void (int)>* onEffectClear)
{
    ToolDescriptor d;
    d.name        = "effect_clear";
    d.description = "Flush one effect's audio state - reverb tail, delay lines, modulation and "
                    "dynamics memory - or every effect's when effect_id is omitted: the Effects "
                    "tab's Clear, for a runaway feedback loop or a tail that should stop now. No "
                    "parameter changes; the effect keeps playing what comes in next.";
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("effect_id", Args::integerArg ("Effect channel number (1-based). Omit to clear "
                                                       "every effect.", 1, WFSParameterDefaults::maxEffectChannels));
    d.inputSchema   = Args::schemaOf (std::move (props), {});
    d.modifiesState = false;   // audio state only: nothing to record or undo
    d.tier          = 1;
    d.handler = [&state, onEffectClear] (const juce::var& args, ChangeRecord*) -> ToolResult
    {
        int index = -1;
        if (auto* obj = args.getDynamicObject(); obj != nullptr && ! Args::missing (*obj, "effect_id"))
        {
            juce::String code, message;
            const auto fx = detail::resolveEffect (state, *obj, "effect_id", code, message);
            if (! fx.has_value())
                return ToolResult::error (code, message);
            index = fx->index;
        }
        if (onEffectClear == nullptr || ! *onEffectClear)
            return ToolResult::error ("unavailable", "The effects engine is not running in this session");

        (*onEffectClear) (index);

        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("cleared", index < 0 ? juce::var ("all") : juce::var (index + 1));
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

} // namespace WFSNetwork::Tools::Effect
