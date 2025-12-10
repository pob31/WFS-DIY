#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"

/**
 * Clusters Tab Component
 * Configuration for speaker clusters
 */
class ClustersTab : public juce::Component
{
public:
    ClustersTab(WfsParameters& params)
        : parameters(params)
    {
        addAndMakeVisible(placeholderLabel);
        placeholderLabel.setText("Clusters Configuration", juce::dontSendNotification);
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClustersTab)
};
