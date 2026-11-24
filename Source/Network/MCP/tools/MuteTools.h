#pragma once

#include <JuceHeader.h>
#include "../MCPCompat.h"
#include "../MCPEffectWrites.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"
#include "../../../Parameters/VarCoercion.h"
#include "InputTools.h"
#include "ToolArgs.h"

/*
    Per-output mutes over MCP, for the three families that have them.

    Each channel stores its mutes as one packed row, one "0"/"1" per output.
    The generated tools could not edit one: reverb_set_mutes took a bare
    "MUTE"/"unmute" with no output index, which the row guard refuses (and
    should), and the three *_mute_macro tools wrote a number that nothing reads -
    the macros are applied by the tabs' panels, to their buttons. These tools do
    the two things the panels do: switch one output, and apply a macro to the
    whole row (mute all, unmute all, invert, mute odd / even, mute / unmute one
    output array). input_set_output_mute already covers one input output.

    A write is read-modify-write of the WHOLE row through the family's setter,
    and the record carries the whole row before and after, so undo puts it back
    exactly. The row is never cut back to the live output count
    (WFSValueTreeState::perOutputRowWidth).
*/
namespace WFSNetwork::Tools::Mutes
{

namespace detail
{
    struct Family
    {
        juce::String kind;           // "input" / "reverb" / "effect"
        juce::String idArg;          // input_id / reverb_id / effect_id
        juce::Identifier rowId;
    };

    inline const Family& family (const juce::String& kind)
    {
        static const Family input  { "input",  "input_id",  WFSParameterIDs::inputMutes };
        static const Family reverb { "reverb", "reverb_id", WFSParameterIDs::reverbMutes };
        static const Family effect { "effect", "effect_id", WFSParameterIDs::effectMutes };
        if (kind == "input")  return input;
        if (kind == "effect") return effect;
        return reverb;
    }

    /** The channel's slot, or -1 with an explanation. Input ids are permanent
        numbers; reverbs and effects are dense. */
    inline int resolve (WFSValueTreeState& state, const Family& f, int id, juce::String& message)
    {
        int slot = -1;
        int live = 0;
        if (f.kind == "input")
        {
            slot = Input::detail::resolveChannelIndex (state, id);
        }
        else
        {
            live = f.kind == "reverb" ? state.getNumReverbChannels() : state.getNumEffectChannels();
            slot = (id >= 1 && id <= live) ? id - 1 : -1;
        }
        if (slot < 0)
            message = f.idArg + " " + juce::String (id) + " is not a live " + f.kind + " channel"
                    + (f.kind == "input" ? juce::String() : " (they are 1-" + juce::String (live) + ")");
        return slot;
    }

    inline juce::String readRow (WFSValueTreeState& state, const Family& f, int slot)
    {
        juce::var raw;
        if (f.kind == "input")       raw = state.getInputParameter  (slot, f.rowId);
        else if (f.kind == "reverb") raw = state.getReverbParameter (slot, f.rowId);
        else                         raw = state.getEffectParameter (slot, f.rowId);
        const int width = WFSValueTreeState::perOutputRowWidth (raw, state.getNumOutputChannels());
        return WFSValueTreeState::normaliseMuteList (raw, width);
    }

    inline void writeRow (WFSValueTreeState& state, const Family& f, int slot, const juce::String& row)
    {
        if (f.kind == "input")
            state.setInputParameter (slot, f.rowId, row);
        else if (f.kind == "reverb")
            state.setReverbParameter (slot, f.rowId, row);
        else
            MCPEffect::writeChannelValue (state, slot, f.rowId, row);   // Effects undo domain
    }

    inline void fillRecord (ChangeRecord* record, const Family& f, int id, const juce::String& description,
                            const juce::String& before, const juce::String& after)
    {
        if (record == nullptr)
            return;
        record->operatorDescription = description;
        record->affectedParameters.add (f.rowId.toString());
        record->affectedGroups.push_back ({ id, "Mutes" });
        auto b = std::make_unique<juce::DynamicObject>();
        b->setProperty (f.rowId, before);
        record->beforeState = juce::var (b.release());
        auto a = std::make_unique<juce::DynamicObject>();
        a->setProperty (f.rowId, after);
        record->afterState = juce::var (a.release());
    }

    inline juce::var mutedOutputs (const juce::String& row, int numOutputs)
    {
        juce::Array<juce::var> muted;
        auto tokens = juce::StringArray::fromTokens (row, ",", "");
        for (int i = 0; i < juce::jmin (numOutputs, tokens.size()); ++i)
            if (tokens[i].trim() == "1")
                muted.add (i + 1);
        return juce::var (muted);
    }
}

//==============================================================================
// <family>_set_output_mute
//==============================================================================

inline ToolDescriptor describeSetOutputMute (WFSValueTreeState& state, const juce::String& kind)
{
    const auto& f = detail::family (kind);
    ToolDescriptor d;
    d.name        = kind + "_set_output_mute";
    d.description = "Mute or unmute one " + kind + (kind == "input" ? juce::String() : " return")
                  + " on one output (speaker), leaving its other outputs as they are. For example 'mute "
                  + kind + " 2 on output 12'. Undoable.";
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty (f.idArg, Args::integerArg ("The " + kind + " channel number (1-based).", 1));
    props->setProperty ("output_id", Args::integerArg ("Output channel number (1-based).", 1));
    props->setProperty ("muted", Args::booleanArg ("true mutes the output for this channel, false unmutes it."));
    d.inputSchema   = Args::schemaOf (std::move (props), { f.idArg.toRawUTF8(), "output_id", "muted" });
    d.modifiesState = true;
    d.tier          = 1;
    d.handler = [&state, f] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        if (obj == nullptr)
            return ToolResult::error ("invalid_args", "Arguments must be a JSON object");
        for (const auto& arg : { f.idArg, juce::String ("output_id"), juce::String ("muted") })
            if (Args::missing (*obj, arg.toRawUTF8()))
                return ToolResult::error ("invalid_args", "Missing required arg: " + arg);

        juce::String code, message;
        const int numOutputs = state.getNumOutputChannels();
        const auto id = Args::wholeNumber (obj->getProperty (f.idArg), f.idArg.toRawUTF8(), 1, 128, code, message);
        if (! id.has_value()) return ToolResult::error (code, message);
        const auto out = Args::wholeNumber (obj->getProperty ("output_id"), "output_id", 1, juce::jmax (1, numOutputs), code, message);
        if (! out.has_value()) return ToolResult::error (code, message);
        const auto muted = Args::boolean (obj->getProperty ("muted"), "muted", code, message);
        if (! muted.has_value()) return ToolResult::error (code, message);

        const int slot = detail::resolve (state, f, *id, message);
        if (slot < 0) return ToolResult::error ("invalid_args", message);

        const auto before = detail::readRow (state, f, slot);
        auto tokens = juce::StringArray::fromTokens (before, ",", "");
        if (*out > tokens.size())
            return ToolResult::error ("internal_error", "The mute row has no column for output " + juce::String (*out));
        tokens.set (*out - 1, *muted ? "1" : "0");
        detail::writeRow (state, f, slot, tokens.joinIntoString (","));
        const auto after = detail::readRow (state, f, slot);

        detail::fillRecord (record, f, *id, juce::String (*muted ? "Muted " : "Unmuted ") + f.kind + " "
                                              + juce::String (*id) + " on output " + juce::String (*out),
                            before, after);

        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty (f.idArg, *id);
        result->setProperty ("output_id", *out);
        result->setProperty ("muted", *muted);
        result->setProperty ("muted_outputs", detail::mutedOutputs (after, numOutputs));
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

//==============================================================================
// <family>_apply_mute_macro
//==============================================================================

inline ToolDescriptor describeApplyMuteMacro (WFSValueTreeState& state, const juce::String& kind)
{
    const auto& f = detail::family (kind);
    static const juce::StringArray macros { "mute_all", "unmute_all", "invert", "mute_odd", "mute_even",
                                            "mute_array", "unmute_array" };
    ToolDescriptor d;
    d.name        = kind + "_apply_mute_macro";
    d.description = "Apply a mute macro to one " + kind + "'s per-output mutes, as the Mute Macro "
                    "selector of its tab does: mute_all, unmute_all, invert, mute_odd (outputs 1, 3, 5...), "
                    "mute_even (2, 4, 6...), or mute_array / unmute_array for the outputs of one output "
                    "array (give array 1-10). Returns the muted outputs. Undoable.";
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty (f.idArg, Args::integerArg ("The " + kind + " channel number (1-based).", 1));
    props->setProperty ("macro", Args::enumArg ("The macro to apply.", macros));
    props->setProperty ("array", Args::integerArg ("Output array number (1-10), for mute_array / unmute_array.", 1, 10));
    d.inputSchema   = Args::schemaOf (std::move (props), { f.idArg.toRawUTF8(), "macro" });
    d.modifiesState = true;
    d.tier          = 1;
    d.handler = [&state, f] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        if (obj == nullptr)
            return ToolResult::error ("invalid_args", "Arguments must be a JSON object");
        for (const auto& arg : { f.idArg, juce::String ("macro") })
            if (Args::missing (*obj, arg.toRawUTF8()))
                return ToolResult::error ("invalid_args", "Missing required arg: " + arg);

        juce::String code, message;
        const auto id = Args::wholeNumber (obj->getProperty (f.idArg), f.idArg.toRawUTF8(), 1, 128, code, message);
        if (! id.has_value()) return ToolResult::error (code, message);
        const auto macro = obj->getProperty ("macro").toString().trim().toLowerCase();
        if (! macros.contains (macro))
            return ToolResult::error ("invalid_args", "macro must be one of " + macros.joinIntoString (", "));

        int array = 0;
        if (macro.endsWith ("_array"))
        {
            if (Args::missing (*obj, "array"))
                return ToolResult::error ("invalid_args", "Missing required arg for " + macro + ": array");
            const auto a = Args::wholeNumber (obj->getProperty ("array"), "array", 1, 10, code, message);
            if (! a.has_value()) return ToolResult::error (code, message);
            array = *a;
        }

        const int slot = detail::resolve (state, f, *id, message);
        if (slot < 0) return ToolResult::error ("invalid_args", message);

        const int numOutputs = state.getNumOutputChannels();
        const auto before = detail::readRow (state, f, slot);
        auto tokens = juce::StringArray::fromTokens (before, ",", "");
        int inArray = 0;
        for (int i = 0; i < juce::jmin (numOutputs, tokens.size()); ++i)
        {
            const bool was = tokens[i].trim() == "1";
            bool now = was;
            if      (macro == "mute_all")   now = true;
            else if (macro == "unmute_all") now = false;
            else if (macro == "invert")     now = ! was;
            else if (macro == "mute_odd")   now = (i % 2) == 0;
            else if (macro == "mute_even")  now = (i % 2) == 1;
            else if (WFSVar::toInt (state.getOutputParameter (i, WFSParameterIDs::outputArray)) == array)
            {
                now = macro == "mute_array";
                ++inArray;
            }
            tokens.set (i, now ? "1" : "0");
        }
        if (array > 0 && inArray == 0)
            return ToolResult::error ("invalid_args", "No output belongs to array " + juce::String (array));

        detail::writeRow (state, f, slot, tokens.joinIntoString (","));
        const auto after = detail::readRow (state, f, slot);

        detail::fillRecord (record, f, *id, "Applied " + macro + (array > 0 ? " " + juce::String (array) : juce::String())
                                              + " to the mutes of " + f.kind + " " + juce::String (*id),
                            before, after);

        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty (f.idArg, *id);
        result->setProperty ("macro", macro);
        result->setProperty ("muted_outputs", detail::mutedOutputs (after, numOutputs));
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

} // namespace WFSNetwork::Tools::Mutes
