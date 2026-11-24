#pragma once

#include <JuceHeader.h>
#include "../MCPCompat.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"
#include "ToolArgs.h"

/*
    The follow-up actions System Config offers after the stage changes, and the
    Effects Settings' Reset Return Positions - each one runs the same
    WFSValueTreeState function as the GUI's long-press.

    System Config remembers the stage it had before an edit and offers Scale /
    Fit / Shift against it. MCP has no "before", so stage_scale_inputs takes the
    old size explicitly and channels_shift takes the shift itself.
*/
namespace WFSNetwork::Tools::Stage
{

namespace detail
{
    inline void file (ChangeRecord* record, const juce::String& what)
    {
        if (record == nullptr)
            return;
        record->operatorDescription = what + " (not undoable over MCP)";
        record->affectedParameters.add ("positions");
        record->affectedGroups.push_back ({ 0, juce::String ("Stage") });
        record->undoable = false;
    }
}

inline ToolDescriptor describeScaleInputs (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "stage_scale_inputs";
    d.description = juce::String (
        "After a stage size change, move every input proportionally (System Config's Scale): give "
        "the stage's PREVIOUS width, depth and height in metres (and its previous origin offsets if "
        "they changed too). mode=fit instead pulls only the inputs that now lie outside the stage "
        "back onto its edge (System Config's Fit), and needs no previous size. Not undoable over "
        "MCP.") + juce::String (kTier2DescriptionSuffix);
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("mode", Args::enumArg ("scale (default) or fit.", { "scale", "fit" }));
    for (const char* n : { "from_width", "from_depth", "from_height" })
        props->setProperty (n, Args::numberArg ("Previous stage size, metres (scale mode).", 0.1, 1000.0));
    for (const char* n : { "from_origin_width", "from_origin_depth", "from_origin_height" })
        props->setProperty (n, Args::numberArg ("Previous origin offset, metres (scale mode; default: the current one)."));
    props->setProperty ("confirm", Args::confirmArg());
    d.inputSchema   = Args::schemaOf (std::move (props), {});
    d.modifiesState = true;
    d.tier          = 2;
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        juce::String code, message;
        const auto mode = obj != nullptr && obj->hasProperty ("mode") ? obj->getProperty ("mode").toString() : juce::String ("scale");
        if (mode == "fit")
        {
            state.fitAllInputPositionsToStage();
            detail::file (record, "Fitted every input inside the stage");
        }
        else if (mode == "scale")
        {
            if (obj == nullptr)
                return ToolResult::error ("invalid_args", "scale needs from_width, from_depth and from_height");
            float v[6] {};
            const char* names[] = { "from_width", "from_depth", "from_height",
                                    "from_origin_width", "from_origin_depth", "from_origin_height" };
            const juce::Identifier current[] = { WFSParameterIDs::stageWidth, WFSParameterIDs::stageDepth,
                                                 WFSParameterIDs::stageHeight, WFSParameterIDs::originWidth,
                                                 WFSParameterIDs::originDepth, WFSParameterIDs::originHeight };
            for (int i = 0; i < 6; ++i)
            {
                if (Args::missing (*obj, names[i]))
                {
                    if (i < 3)
                        return ToolResult::error ("invalid_args", juce::String ("Missing required arg for scale: ") + names[i]);
                    v[i] = static_cast<float> (state.getParameter (current[i], -1));
                    continue;
                }
                const auto n = Args::number (obj->getProperty (names[i]), names[i], code, message);
                if (! n.has_value()) return ToolResult::error (code, message);
                if (i < 3 && *n <= 0.0)
                    return ToolResult::error ("out_of_range", juce::String (names[i]) + " must be above 0");
                v[i] = static_cast<float> (*n);
            }
            state.scaleAllInputPositions (v[0], v[1], v[2], v[3], v[4], v[5]);
            detail::file (record, "Scaled every input to the new stage size");
        }
        else
            return ToolResult::error ("invalid_args", "mode must be scale or fit");

        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("mode", mode);
        result->setProperty ("inputs", state.getNumInputChannels());
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

inline ToolDescriptor describeShift (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "channels_shift";
    d.description = juce::String (
        "Move every input, output (speaker) and/or reverb node by the same offset, as System "
        "Config's Shift buttons do after an origin change (shifting by minus the origin's move keeps "
        "everything where it was on stage). kinds: any of inputs, outputs, reverbs. Not undoable over "
        "MCP.") + juce::String (kTier2DescriptionSuffix);
    auto items = std::make_unique<juce::DynamicObject>();
    items->setProperty ("type", "string");
    items->setProperty ("enum", juce::var (juce::Array<juce::var> { "inputs", "outputs", "reverbs" }));
    auto kinds = std::make_unique<juce::DynamicObject>();
    kinds->setProperty ("type", "array");
    kinds->setProperty ("items", juce::var (items.release()));
    kinds->setProperty ("description", "Which channel families move.");
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("kinds", juce::var (kinds.release()));
    props->setProperty ("dx", Args::numberArg ("Metres along X.", -100.0, 100.0));
    props->setProperty ("dy", Args::numberArg ("Metres along Y.", -100.0, 100.0));
    props->setProperty ("dz", Args::numberArg ("Metres along Z.", -100.0, 100.0));
    props->setProperty ("confirm", Args::confirmArg());
    d.inputSchema   = Args::schemaOf (std::move (props), { "kinds" });
    d.modifiesState = true;
    d.tier          = 2;
    d.handler = [&state] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        juce::String code, message;
        const auto* kinds = obj != nullptr ? obj->getProperty ("kinds").getArray() : nullptr;
        if (kinds == nullptr || kinds->isEmpty())
            return ToolResult::error ("invalid_args", "kinds must list inputs, outputs and/or reverbs");
        float delta[3] {};
        const char* names[] = { "dx", "dy", "dz" };
        for (int i = 0; i < 3; ++i)
            if (! Args::missing (*obj, names[i]))
            {
                const auto n = Args::number (obj->getProperty (names[i]), names[i], code, message);
                if (! n.has_value()) return ToolResult::error (code, message);
                delta[i] = static_cast<float> (*n);
            }
        if (delta[0] == 0 && delta[1] == 0 && delta[2] == 0)
            return ToolResult::error ("invalid_args", "Nothing to do: give dx, dy and/or dz");

        juce::StringArray moved;
        for (const auto& k : *kinds)
        {
            const auto kind = k.toString();
            if      (kind == "inputs")  state.shiftAllInputPositions  (delta[0], delta[1], delta[2]);
            else if (kind == "outputs") state.shiftAllOutputPositions (delta[0], delta[1], delta[2]);
            else if (kind == "reverbs") state.shiftAllReverbPositions (delta[0], delta[1], delta[2]);
            else return ToolResult::error ("invalid_args", "unknown kind " + kind.quoted());
            moved.add (kind);
        }
        detail::file (record, "Shifted " + moved.joinIntoString (", ") + " by (" + juce::String (delta[0]) + ", "
                                + juce::String (delta[1]) + ", " + juce::String (delta[2]) + ") m");
        auto result = std::make_unique<juce::DynamicObject>();
        juce::Array<juce::var> shifted;
        for (const auto& m : moved)
            shifted.add (m);
        result->setProperty ("shifted", juce::var (shifted));
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

inline ToolDescriptor describeEffectRelayout (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "effect_reset_return_positions";
    d.description = juce::String (
        "Lay every effect return out on the default ring again and hand the positions back to the "
        "application, so later effect-count changes re-lay the ring (Effects Settings' Reset Return "
        "Positions). Not undoable over MCP.") + juce::String (kTier2DescriptionSuffix);
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("confirm", Args::confirmArg());
    d.inputSchema   = Args::schemaOf (std::move (props), {});
    d.modifiesState = true;
    d.tier          = 2;
    d.handler = [&state] (const juce::var&, ChangeRecord* record) -> ToolResult
    {
        if (state.getNumEffectChannels() <= 0)
            return ToolResult::error ("empty", "The session has no effect channels");
        state.redistributeAllEffectPositions();
        state.getEffectsState().setProperty (WFSParameterIDs::effectPositionsUserOwned, 0, nullptr);
        detail::file (record, "Reset every effect return position");
        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("effects", state.getNumEffectChannels());
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

} // namespace WFSNetwork::Tools::Stage
