#pragma once

#include <JuceHeader.h>
#include "ColorScheme.h"
#include "../Localization/LocalizationManager.h"

/**
 * Status Bar Component
 * Displays contextual information at the bottom of the window
 * - Help mode: Shows help text for UI elements
 * - OSC mode: Shows OSC methods for UI elements
 */
class StatusBar : public juce::Component,
                  private juce::Timer
{
public:
    enum class DisplayMode
    {
        Help,
        OSC
    };

    StatusBar()
    {
        // Mode selector
        addAndMakeVisible(modeLabel);
        modeLabel.setText(LOC("statusBar.displayLabel"), juce::dontSendNotification);

        addAndMakeVisible(modeSelector);
        modeSelector.addItem(LOC("statusBar.helpMode"), 1);
        modeSelector.addItem(LOC("statusBar.oscMode"), 2);
        modeSelector.setSelectedId(1, juce::dontSendNotification);
        modeSelector.onChange = [this]() {
            currentMode = (modeSelector.getSelectedId() == 1) ? DisplayMode::Help : DisplayMode::OSC;
            updateDisplay();
        };

        // Status text
        addAndMakeVisible(statusLabel);
        statusLabel.setJustificationType(juce::Justification::centredLeft);

        setSize(800, 30);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(ColorScheme::get().chromeBackground);

        // Draw separator line at top
        g.setColour(ColorScheme::get().chromeDivider);
        g.drawLine(0.0f, 0.0f, (float)getWidth(), 0.0f, 2.0f);
    }

    void resized() override
    {
        float s = static_cast<float>(getHeight()) / 30.0f;
        auto sc = [s](int ref) { return juce::jmax(static_cast<int>(ref * 0.65f), static_cast<int>(ref * s)); };

        auto area = getLocalBounds().reduced(sc(5), sc(2));

        // Mode selector on the right
        auto selectorArea = area.removeFromRight(sc(200));
        modeLabel.setBounds(selectorArea.removeFromLeft(sc(60)));
        selectorArea.removeFromLeft(sc(5));
        modeSelector.setBounds(selectorArea.removeFromLeft(sc(100)));

        // Status text on the left
        area.removeFromRight(sc(10)); // spacing
        statusLabel.setBounds(area);
    }

    void setHelpText(const juce::String& helpText)
    {
        currentHelpText = helpText;
        updateDisplay();
    }

    void setOscMethod(const juce::String& oscMethod)
    {
        currentOscMethod = oscMethod;
        updateDisplay();
    }

    void clearText()
    {
        currentHelpText = "";
        currentOscMethod = "";
        temporaryMessage.clear();
        stopTimer();
        updateDisplay();
    }

    /** Show a temporary message that auto-clears after specified milliseconds */
    void showTemporaryMessage(const juce::String& message, int durationMs = 3000)
    {
        temporaryMessage = message;
        statusLabel.setText(message, juce::dontSendNotification);
        startTimer(durationMs);
    }

    DisplayMode getCurrentMode() const { return currentMode; }

private:
    void timerCallback() override
    {
        stopTimer();
        temporaryMessage.clear();
        updateDisplay();
    }
    void updateDisplay()
    {
        // Temporary messages take priority
        if (!temporaryMessage.isEmpty())
        {
            statusLabel.setText(temporaryMessage, juce::dontSendNotification);
            return;
        }

        if (currentMode == DisplayMode::Help && !currentHelpText.isEmpty())
            statusLabel.setText(currentHelpText, juce::dontSendNotification);
        else if (currentMode == DisplayMode::OSC && !currentOscMethod.isEmpty())
            statusLabel.setText(currentOscMethod, juce::dontSendNotification);
        else
            statusLabel.setText("", juce::dontSendNotification);
    }

    juce::Label modeLabel;
    juce::ComboBox modeSelector;
    juce::Label statusLabel;

    DisplayMode currentMode = DisplayMode::Help;
    juce::String currentHelpText;
    juce::String currentOscMethod;
    juce::String temporaryMessage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatusBar)
};
