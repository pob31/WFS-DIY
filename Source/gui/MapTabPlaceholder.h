#pragma once

#include <JuceHeader.h>
#include "ColorScheme.h"
#include "WfsLookAndFeel.h"
#include "../Localization/LocalizationManager.h"

/**
 * MapTabPlaceholder
 *
 * Lightweight stand-in component shown in tab 6 when the MapTab is detached
 * into its own window. Provides a "Re-attach Map" button to bring it back.
 */
class MapTabPlaceholder : public juce::Component
{
public:
    MapTabPlaceholder()
    {
        addAndMakeVisible(reattachButton);
        reattachButton.setButtonText(LOC("map.reattach"));
        reattachButton.onClick = [this]()
        {
            if (onReattachRequested)
                onReattachRequested();
        };
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(ColorScheme::get().background);

        g.setColour(ColorScheme::get().textSecondary);
        g.setFont(juce::FontOptions(16.0f * WfsLookAndFeel::uiScale));
        g.drawText(LOC("map.detachedMessage"),
                   getLocalBounds().reduced(40).withTrimmedBottom(getHeight() / 4),
                   juce::Justification::centred, true);
    }

    void resized() override
    {
        const float us = WfsLookAndFeel::uiScale;
        const int btnW = juce::jmax(160, static_cast<int>(200.0f * us));
        const int btnH = juce::jmax(30, static_cast<int>(40.0f * us));
        reattachButton.setBounds((getWidth() - btnW) / 2,
                                 getHeight() / 2 + static_cast<int>(20.0f * us),
                                 btnW, btnH);
    }

    std::function<void()> onReattachRequested;

private:
    juce::TextButton reattachButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MapTabPlaceholder)
};
