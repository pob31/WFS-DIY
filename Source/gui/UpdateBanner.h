#pragma once

#include <JuceHeader.h>
#include "ColorScheme.h"
#include "../AppSettings.h"
#include "../Localization/LocalizationManager.h"

class UpdateBanner : public juce::Component
{
public:
    std::function<void()> onDismiss;

    UpdateBanner()
    {
        addAndMakeVisible (messageLabel);
        messageLabel.setJustificationType (juce::Justification::centredLeft);

        addAndMakeVisible (downloadButton);
        downloadButton.onClick = [this]()
        {
            juce::URL (releaseUrl).launchInDefaultBrowser();
        };

        addAndMakeVisible (dismissButton);
        dismissButton.onClick = [this]() { dismiss(); };

        setVisible (false);
    }

    void showUpdate (const juce::String& version, const juce::String& url)
    {
        newVersion = version;
        releaseUrl = url;

        messageLabel.setText (LocalizationManager::getInstance().get (
            "updateBanner.available", {{"version", version}}),
            juce::dontSendNotification);
        downloadButton.setButtonText (LOC ("updateBanner.download"));

        setVisible (true);

        if (onDismiss)
            onDismiss();
    }

    void dismiss()
    {
        AppSettings::setUpdateDismissedVersion (newVersion);
        setVisible (false);

        if (onDismiss)
            onDismiss();
    }

    void paint (juce::Graphics& g) override
    {
        auto& cs = ColorScheme::get();
        g.fillAll (cs.accentBlue.withAlpha (0.15f));

        g.setColour (cs.accentBlue.withAlpha (0.4f));
        g.drawLine (0.0f, (float) getHeight(), (float) getWidth(), (float) getHeight(), 1.0f);
    }

    void resized() override
    {
        auto& cs = ColorScheme::get();
        float s = WfsLookAndFeel::uiScale;

        // Update colors dynamically on each layout pass
        downloadButton.setColour (juce::TextButton::buttonColourId, cs.accentBlue);
        downloadButton.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        dismissButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        dismissButton.setColour (juce::TextButton::textColourOnId, cs.textSecondary);
        messageLabel.setColour (juce::Label::textColourId, cs.textPrimary);

        auto bounds = getLocalBounds().reduced (juce::roundToInt (8 * s), 0);

        dismissButton.setBounds (bounds.removeFromRight (juce::roundToInt (24 * s)));
        bounds.removeFromRight (juce::roundToInt (6 * s));
        downloadButton.setBounds (bounds.removeFromRight (juce::roundToInt (90 * s)));
        bounds.removeFromRight (juce::roundToInt (10 * s));
        messageLabel.setBounds (bounds);
    }

private:
    juce::Label      messageLabel;
    juce::TextButton downloadButton;
    juce::TextButton dismissButton { "x" };
    juce::String     newVersion;
    juce::String     releaseUrl;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UpdateBanner)
};
