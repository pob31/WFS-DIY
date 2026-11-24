#pragma once

#include <JuceHeader.h>
#include "../MCPCompat.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"
#include "ToolArgs.h"

/*
    The Audio Interface window's patch over MCP: which hardware inputs feed
    each input channel, which hardware outputs each output channel drives.

    patchData rows: one per channel SLOT (an input's display position, not its
    number), one 0/1 column per hardware channel, 0-based. Writing is
    STOPPED-ONLY, as the window is (patching a running rig can damage speakers),
    and an input patch write latches the channel numbers first: while they are
    fresh the patch is re-flowed from the channel list, which would discard an
    edit made before the latch (CLAUDE.md, "Fresh-session compaction").
    A mono input takes one hardware input, a stereo one two (lower = left); a
    hardware input belongs to one channel. The app then reloads the patch maps
    (repairInputPatchAfterLoad + loadAudioPatches) through onPatchChanged.
*/
namespace WFSNetwork::Tools::Patch
{

namespace detail
{
    inline juce::ValueTree patchTree (WFSValueTreeState& state, bool outputs)
    {
        return state.getAudioPatchState().getChildWithName (outputs ? WFSParameterIDs::OutputPatch
                                                                    : WFSParameterIDs::InputPatch);
    }

    inline juce::StringArray rows (const juce::ValueTree& patch)
    {
        return juce::StringArray::fromTokens (patch.getProperty (WFSParameterIDs::patchData).toString(), ";", "");
    }

    inline std::vector<int> columnsOn (const juce::String& row)
    {
        std::vector<int> out;
        auto cols = juce::StringArray::fromTokens (row, ",", "");
        for (int c = 0; c < cols.size(); ++c)
            if (cols[c].getIntValue() == 1)
                out.push_back (c + 1);   // 1-based hardware channel, as the window shows it
        return out;
    }
}

inline ToolDescriptor describeGet (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "patch_get";
    d.description = "Read the audio patch, as the Audio Interface window shows it: for each input channel "
                    "(by permanent number, in display order) or each output channel, the hardware channels "
                    "(1-based) it is patched to, plus how many hardware channels the opened device has. "
                    "Read-only.";
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("kind", Args::enumArg ("inputs or outputs.", { "inputs", "outputs" }));
    d.inputSchema   = Args::schemaOf (std::move (props), { "kind" });
    d.modifiesState = false;
    d.tier          = 1;
    d.handler = [&state] (const juce::var& args, ChangeRecord*) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        const auto kind = obj != nullptr ? obj->getProperty ("kind").toString() : juce::String();
        if (kind != "inputs" && kind != "outputs")
            return ToolResult::error ("invalid_args", "kind must be inputs or outputs");
        const bool outputs = kind == "outputs";
        auto patch = detail::patchTree (state, outputs);
        const auto r = detail::rows (patch);

        juce::Array<juce::var> channels;
        const int n = outputs ? state.getNumOutputChannels() : state.getNumInputChannels();
        for (int slot = 0; slot < n; ++slot)
        {
            auto c = std::make_unique<juce::DynamicObject>();
            if (outputs)
                c->setProperty ("output_id", slot + 1);
            else
            {
                c->setProperty ("input_id", state.getInputChannelNumber (slot));
                c->setProperty ("type", state.isInputChannelStereo (slot) ? "stereo" : "mono");
            }
            juce::Array<juce::var> hw;
            if (slot < r.size())
                for (int h : detail::columnsOn (r[slot]))
                    hw.add (h);
            c->setProperty ("hardware", juce::var (hw));
            channels.add (juce::var (c.release()));
        }
        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("kind", kind);
        result->setProperty ("hardware_channels_open", patch.getProperty (outputs ? WFSParameterIDs::activeHardwareOutputs
                                                                                  : WFSParameterIDs::activeHardwareInputs));
        result->setProperty ("channels", juce::var (channels));
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

inline ToolDescriptor describeSet (WFSValueTreeState& state, const std::function<void()>* onPatchChanged)
{
    ToolDescriptor d;
    d.name        = "patch_set";
    d.description = juce::String (
        "Patch one channel to hardware, as the Audio Interface window does: kind inputs (channel_id = "
        "the input's permanent number; a mono input takes 1 hardware input, a stereo one 2, lower = "
        "left) or outputs (channel_id = output number; usually 1 hardware output). hardware is the list "
        "of 1-based hardware channels; [] unpatches it. A hardware channel already patched to another "
        "channel is refused, naming it. Refused while the audio engine runs. Not undoable: patch it "
        "again instead.") + juce::String (kTier2DescriptionSuffix);
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("kind", Args::enumArg ("inputs or outputs.", { "inputs", "outputs" }));
    props->setProperty ("channel_id", Args::integerArg ("Input number or output number.", 1));
    props->setProperty ("hardware", Args::integerArrayArg ("1-based hardware channels.", 1, 512));
    props->setProperty ("confirm", Args::confirmArg());
    d.inputSchema   = Args::schemaOf (std::move (props), { "kind", "channel_id", "hardware" });
    d.modifiesState = true;
    d.tier          = 2;
    d.handler = [&state, onPatchChanged] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        if (obj == nullptr)
            return ToolResult::error ("invalid_args", "Arguments must be a JSON object");
        for (const char* a : { "kind", "channel_id", "hardware" })
            if (Args::missing (*obj, a))
                return ToolResult::error ("invalid_args", juce::String ("Missing required arg: ") + a);
        if (state.isProcessingEnabled())
            return ToolResult::error ("engine_running", "The patch is stopped-only (patching a running rig can "
                                                        "damage speakers): stop processing first, then retry.");

        const auto kind = obj->getProperty ("kind").toString();
        if (kind != "inputs" && kind != "outputs")
            return ToolResult::error ("invalid_args", "kind must be inputs or outputs");
        const bool outputs = kind == "outputs";

        juce::String code, message;
        auto hw = Args::wholeNumberArray (obj->getProperty ("hardware"), "hardware", 1, 512, code, message);
        if (! hw) return ToolResult::error (code, message);
        std::sort (hw->begin(), hw->end());
        hw->erase (std::unique (hw->begin(), hw->end()), hw->end());

        const int id = static_cast<int> (obj->getProperty ("channel_id"));
        int slot = -1;
        if (outputs)
            slot = (id >= 1 && id <= state.getNumOutputChannels()) ? id - 1 : -1;
        else
        {
            state.markChannelNumbersUserOwned ("MCP patch_set");   // before the write: a fresh re-flow would eat it
            slot = state.getSlotForChannelNumber (id);
        }
        if (slot < 0)
            return ToolResult::error ("invalid_args", "channel_id " + juce::String (id) + " is not a live " + kind.dropLastCharacters (1));

        if (! outputs)
        {
            const int capacity = state.isInputChannelStereo (slot) ? 2 : 1;
            if (static_cast<int> (hw->size()) > capacity)
                return ToolResult::error ("invalid_args", "Input " + juce::String (id) + " is "
                                                            + (capacity == 2 ? "stereo: at most 2" : "mono: 1")
                                                            + " hardware input(s)");
        }

        auto patch = detail::patchTree (state, outputs);
        if (! patch.isValid())
            return ToolResult::error ("internal_error", "No " + kind + " patch in this session");
        auto r = detail::rows (patch);
        const int n = outputs ? state.getNumOutputChannels() : state.getNumInputChannels();
        while (r.size() < n)
            r.add ({});

        // One owner per hardware channel.
        for (int other = 0; other < n; ++other)
        {
            if (other == slot) continue;
            for (int h : detail::columnsOn (r[other]))
                if (std::find (hw->begin(), hw->end(), h) != hw->end())
                    return ToolResult::error ("conflict", "Hardware channel " + juce::String (h) + " is patched to "
                                                            + kind.dropLastCharacters (1) + " "
                                                            + juce::String (outputs ? other + 1 : state.getInputChannelNumber (other))
                                                            + "; unpatch it there first");
        }

        const auto before = detail::columnsOn (r[slot]);
        const int width = juce::jmax (static_cast<int> (patch.getProperty (WFSParameterIDs::cols, 0)),
                                      hw->empty() ? 0 : hw->back());
        juce::StringArray cols;
        for (int c = 1; c <= width; ++c)
            cols.add (std::find (hw->begin(), hw->end(), c) != hw->end() ? "1" : "0");
        r.set (slot, cols.joinIntoString (","));
        if (width > static_cast<int> (patch.getProperty (WFSParameterIDs::cols, 0)))
            patch.setProperty (WFSParameterIDs::cols, width, nullptr);
        patch.setProperty (WFSParameterIDs::patchData, r.joinIntoString (";"), nullptr);

        if (onPatchChanged != nullptr && *onPatchChanged)
            (*onPatchChanged)();

        if (record != nullptr)
        {
            juce::StringArray a, b;
            for (int h : before) b.add (juce::String (h));
            for (int h : *hw) a.add (juce::String (h));
            record->operatorDescription = "Patched " + kind.dropLastCharacters (1) + " " + juce::String (id)
                                        + " to hardware [" + a.joinIntoString (",") + "] (was ["
                                        + b.joinIntoString (",") + "]; not undoable)";
            record->affectedParameters.add (WFSParameterIDs::patchData.toString());
            record->affectedGroups.push_back ({ id, juce::String ("Patch") });
            record->undoable = false;
        }
        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("kind", kind);
        result->setProperty ("channel_id", id);
        juce::Array<juce::var> out;
        for (int h : *hw) out.add (h);
        result->setProperty ("hardware", juce::var (out));
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

} // namespace WFSNetwork::Tools::Patch
