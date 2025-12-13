#pragma once

#include <JuceHeader.h>

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
        modeLabel.setText("Display:", juce::dontSendNotification);
        modeLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(modeSelector);
        modeSelector.addItem("Help", 1);
        modeSelector.addItem("OSC", 2);
        modeSelector.setSelectedId(1, juce::dontSendNotification);
        modeSelector.onChange = [this]() {
            currentMode = (modeSelector.getSelectedId() == 1) ? DisplayMode::Help : DisplayMode::OSC;
            updateDisplay();
        };

        // Status text
        addAndMakeVisible(statusLabel);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        statusLabel.setColour(juce::Label::backgroundColourId, juce::Colours::darkgrey);
        statusLabel.setJustificationType(juce::Justification::centredLeft);

        setSize(800, 30);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::darkgrey);

        // Draw separator line at top
        g.setColour(juce::Colours::black);
        g.drawLine(0.0f, 0.0f, (float)getWidth(), 0.0f, 2.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(5, 2);

        // Mode selector on the right
        auto selectorArea = area.removeFromRight(200);
        modeLabel.setBounds(selectorArea.removeFromLeft(60));
        selectorArea.removeFromLeft(5);
        modeSelector.setBounds(selectorArea.removeFromLeft(100));

        // Status text on the left
        area.removeFromRight(10); // spacing
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
