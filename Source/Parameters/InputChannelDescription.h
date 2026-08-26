#pragma once

#include <JuceHeader.h>
#include "../Localization/LocalizationManager.h"
#include "InputChannelIdentity.h"
#include "WFSValueTreeState.h"

/**
    The one way to name an input channel to the operator.

        #12 "Kick" (mono)        #12 (mono)

    Number first, with a hash, because the number is the ADDRESS - it is what
    OSC, QLab cues, snapshots and the DAW plug-in use, and the thing an operator
    has to type. The name is quoted because default names carry a PER-TYPE
    ordinal ("Mono 7" is the seventh mono channel, whatever its number), so an
    unquoted "Mono 7 #12" reads as a typo. The type is localised.

    Four ad-hoc conventions existed before this ("N\nname", "Input N",
    "Input N - name", "input N (name)"); new operator-facing text uses this one.
    Migrating the older sites is a follow-up.
*/
inline juce::String describeInputChannel (int number, const juce::String& name, bool stereo)
{
    auto& loc = LocalizationManager::getInstance();
    const juce::String type = LOC (stereo ? "systemConfig.channelList.stereo"
                                          : "systemConfig.channelList.mono");
    if (name.trim().isEmpty())
        return loc.get ("common.channelDescriptionUnnamed",
                        {{ "number", juce::String (number) }, { "type", type }});
    return loc.get ("common.channelDescription",
                    {{ "number", juce::String (number) }, { "name", name }, { "type", type }});
}

inline juce::String describeInputChannel (const InputChannelRef& ref)
{
    return describeInputChannel (ref.number, ref.name, ref.stereo);
}

/** By slot on the live tree. An out-of-range slot yields an empty string
    (getInputChannelNumber returns 0 there, and "#0" is never a channel). */
inline juce::String describeInputChannel (const WFSValueTreeState& state, int slot)
{
    const int number = state.getInputChannelNumber (slot);
    if (number <= 0)
        return {};
    auto channel = state.getInputsState().getChild (slot).getChildWithName (WFSParameterIDs::Channel);
    const juce::String name = channel.isValid()
                                  ? channel.getProperty (WFSParameterIDs::inputName).toString()
                                  : juce::String();
    return describeInputChannel (number, name, state.isInputChannelStereo (slot));
}
