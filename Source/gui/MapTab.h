#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"

/**
 * Map Tab Component
 * Spatial mapping and visualization
 */
class MapTab : public juce::Component
{
public:
    MapTab(WfsParameters& params)
        : parameters(params)
    {
        addAndMakeVisible(placeholderLabel);
        placeholderLabel.setText("Map View", juce::dontSendNotification);
        placeholderLabel.setFont(juce::FontOptions().withHeight(24.0f).withStyle("Bold"));
        placeholderLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        placeholderLabel.setJustificationType(juce::Justification::centred);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xFF1E1E1E));
    }

    void resized() override
    {
        placeholderLabel.setBounds(getLocalBounds());
    }

private:
    WfsParameters& parameters;
    juce::Label placeholderLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MapTab)
};
