#pragma once

#include <JuceHeader.h>
#include "ColorUtilities.h"
#include "../Parameters/WFSValueTreeState.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "../Parameters/VarCoercion.h"

/**
 * The one place that answers "what colour is this input?".
 *
 * An input's colour is derived from its channel number unless the operator has picked one,
 * so every surface that paints an input — the selector tiles and the collapsed selector
 * button, the map marker and its live-source gradient, the patch matrix — has to make the
 * same decision. Putting it here rather than at each call site is what stops them drifting.
 *
 * This lives in its own header, NOT in ColorUtilities.h: that one is a pure JUCE leaf and is
 * included by ChannelSelector.h and ColouredIndexComboBox.h, two widgets that are deliberately
 * parameter-tree-free. Teaching it about the tree would drag WFSValueTreeState (and with it
 * spatcore's TreeParameterStore) into both of them.
 */
namespace WfsInputColour
{
    /** Resolves the stored 24-bit RGB, or the derived hue when it is -1 (auto).

        Reads through WFSVar::toInt rather than a var::isInt() guard: ValueTree::fromXml types
        every property as a string, so a type-guarded read works on a fresh session and
        silently returns the default on a loaded one.
    */
    inline juce::Colour resolve (const WFSValueTreeState& state, int channelNumber)
    {
        const int slot = state.getSlotForChannelNumber (channelNumber);

        if (slot >= 0)
        {
            const int stored = WFSVar::toInt (state.getInputParameter (slot, WFSParameterIDs::inputColour),
                                              WFSParameterDefaults::inputColourDefault);
            if (stored >= 0)
                return juce::Colour (static_cast<juce::uint32> (stored)).withAlpha (1.0f);
        }

        // -1, or a number that names no live channel: the colour the app has always used.
        return WfsColorUtilities::getInputColor (channelNumber);
    }

    /** Same, addressed by SLOT rather than by permanent channel number — for the map and the
        patch matrix, which iterate slots and would otherwise round-trip number->slot->number. */
    inline juce::Colour resolveForSlot (const WFSValueTreeState& state, int slot)
    {
        const int stored = WFSVar::toInt (state.getInputParameter (slot, WFSParameterIDs::inputColour),
                                          WFSParameterDefaults::inputColourDefault);
        if (stored >= 0)
            return juce::Colour (static_cast<juce::uint32> (stored)).withAlpha (1.0f);

        return WfsColorUtilities::getInputColor (state.getInputChannelNumber (slot));
    }

    /** The value to STORE for a picked colour: 24-bit RGB, alpha dropped. */
    inline int toStoredValue (juce::Colour c) noexcept
    {
        return static_cast<int> (c.getARGB() & 0x00FFFFFFu);
    }

    /** The sentinel meaning "derive it from the channel number". */
    inline constexpr int autoValue() noexcept { return WFSParameterDefaults::inputColourDefault; }
}
