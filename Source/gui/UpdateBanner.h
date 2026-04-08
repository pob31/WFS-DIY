#pragma once

#include <JuceHeader.h>
#include "ColorScheme.h"
#include "../AppSettings.h"

class UpdateBanner : public juce::Component
{
public:
    std::function<void()> onDismiss;

    UpdateBanner()
    {
        addAndMakeVisible (messageLabel);
        messageLabel.setJustificationType (juce::Justification::centredLeft);

        addAndMakeVisible (downloadButton);
        downloadButton.setColour (juce::TextButton::buttonColourId, ColorScheme::get().accentBlue);
        downloadButton.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        downloadButton.onClick = [this]()
        {
            juce::URL (releaseUrl).launchInDefaultBrowser();
        };

        addAndMakeVisible (dismissButton);
        dismissButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        dismissButton.setColour (juce::TextButton::textColourOnId, ColorScheme::get().textSecondary);
        dismissButton.onClick = [this]() { dismiss(); };

        setVisible (false);
    }

    void showUpdate (const juce::String& version, const juce::String& url)
    {
        newVersion = version;
        releaseUrl = url;

        messageLabel.setText ("WFS-DIY v" + version + " is available!",
                              juce::dontSendNotification);
        downloadButton.setButtonText ("Download");

        setVisible (true);

        if (onDismiss)
            onDismiss();  // triggers re-layout in parent
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
        g.fillAll (ColorScheme::get().accentBlue.withAlpha (0.15f));

        g.setColour (ColorScheme::get().accentBlue.withAlpha (0.4f));
        g.drawLine (0.0f, (float) getHeight(), (float) getWidth(), (float) getHeight(), 1.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (8, 0);

        dismissButton.setBounds (bounds.removeFromRight (24));
        bounds.removeFromRight (6);
        downloadButton.setBounds (bounds.removeFromRight (90));
        bounds.removeFromRight (10);
        messageLabel.setBounds (bounds);
    }

private:
    juce::Label      messageLabel;
    juce::TextButton downloadButton;
    juce::TextButton dismissButton { juce::CharPointer_UTF8 ("\xc3\x97") }; // multiplication sign as X
    juce::String     newVersion;
    juce::String     releaseUrl;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UpdateBanner)
};
