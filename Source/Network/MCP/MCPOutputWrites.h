#pragma once

#include <JuceHeader.h>
#include "MCPCompat.h"
#include "../../Parameters/WFSValueTreeState.h"

/*
    Output writes over MCP follow the GUI's Apply to Array.

    In the Outputs tab an edit to an array-linked parameter (attenuation,
    delay, orientation, angles, pitch, HF damping, the enables, parallax...)
    on an array member reaches the other members by their Apply to Array mode:
    ABSOLUTE copies the value, RELATIVE adds the same change, OFF is skipped;
    Ctrl/Cmd edits the one output. MCP wrote with setParameter, so an AI
    changed one speaker while the tool description promised the array.

    Writes now go through setOutputParameterWithArrayPropagation, with an
    optional this_output_only argument for the Ctrl/Cmd case. A propagated
    write changes several outputs, so the change record carries one sub-write
    per output that moved: undo puts every one of them back.
*/
namespace WFSNetwork::MCPOutput
{

inline constexpr const char* bypassArgName = "this_output_only";

inline bool isArrayLinked (const juce::Identifier& id)
{
    return WFSValueTreeState::isArrayLinkedParameter (id);
}

inline juce::var bypassArgSchema()
{
    auto v = std::make_unique<juce::DynamicObject>();
    v->setProperty ("type", "boolean");
    v->setProperty ("description",
        "Output parameters that belong to an array follow its Apply to Array mode (Absolute copies "
        "the value to the other members, Relative adds the same change). true changes this output "
        "only, as holding Ctrl/Cmd does in the Outputs tab. Default false.");
    return juce::var (v.release());
}

inline bool readBypass (const juce::var& args)
{
    if (auto* obj = args.getDynamicObject())
        if (obj->hasProperty (bypassArgName))
            return static_cast<bool> (obj->getProperty (bypassArgName));
    return false;
}

/** Write one output value the way the Outputs tab does and, when `record` is
    given, add a sub-write for every output whose value changed. */
inline void write (WFSValueTreeState& state, int channelIndex, const juce::Identifier& id,
                   const juce::var& value, bool thisOutputOnly, ChangeRecord* record)
{
    const int n = state.getNumOutputChannels();
    juce::Array<juce::var> before;
    for (int i = 0; i < n; ++i)
        before.add (state.getOutputParameter (i, id));

    state.setOutputParameterWithArrayPropagation (channelIndex, id, value, ! thisOutputOnly);

    if (record == nullptr)
        return;

    for (int i = 0; i < n; ++i)
    {
        const auto after = state.getOutputParameter (i, id);
        if (after == before[i] && i != channelIndex)
            continue;

        auto b = std::make_unique<juce::DynamicObject>();
        b->setProperty (id, before[i]);
        auto a = std::make_unique<juce::DynamicObject>();
        a->setProperty (id, after);

        ChangeSubWrite sw;
        sw.channelIndex  = i;
        sw.channelNumber = 0;   // outputs are addressed by dense slot
        sw.beforeState   = juce::var (b.release());
        sw.afterState    = juce::var (a.release());
        record->subWrites.push_back (std::move (sw));
    }
}

/** How many outputs besides `channelIndex` a record's sub-writes moved. */
inline int othersMoved (const ChangeRecord* record, int channelIndex)
{
    if (record == nullptr)
        return 0;
    int n = 0;
    for (const auto& sw : record->subWrites)
        if (sw.channelIndex != channelIndex)
            ++n;
    return n;
}

} // namespace WFSNetwork::MCPOutput
