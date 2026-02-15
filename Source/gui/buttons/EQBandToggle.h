#pragma once

#include <JuceHeader.h>
#include "../ColorScheme.h"

/**
 * EQBandToggle
 *
 * A tiny colored indicator button for toggling individual EQ bands on/off.
 * When ON, displays a filled rounded rectangle in the band's color.
 * When OFF, displays a dark grey rounded rectangle.
 */
class EQBandToggle : public juce::Button
{
public:
    EQBandToggle() : juce::Button ("") { setClickingTogglesState (true); }

    void setBandColour (juce::Colour c) { bandColour = c; repaint(); }

private:
    juce::Colour bandColour { juce::Colours::white };

    void paintButton (juce::Graphics& g, bool /*highlighted*/, bool /*down*/) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (1.0f);
        bool on = getToggleState();
        g.setColour (on ? bandColour : juce::Colour (0xFF2D2D2D));
        g.fillRoundedRectangle (bounds, 3.0f);
        g.setColour (ColorScheme::get().buttonBorder);
        g.drawRoundedRectangle (bounds, 3.0f, 1.0f);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQBandToggle)
};
