#pragma once

#include <JuceHeader.h>

/**
 * Audio Interface and Patching Window
 * Contains the JUCE AudioDeviceSelectorComponent for configuring:
 * - Audio device type (ASIO, Windows Audio, etc.)
 * - Device selection
 * - Active input/output channels
 * - Sample rate
 * - Buffer size
 * - Control panel access
 */
class AudioInterfaceWindow : public juce::DocumentWindow
{
public:
    AudioInterfaceWindow(juce::AudioDeviceManager& deviceManager)
        : DocumentWindow("Audio Interface and Patching",
                         juce::Colours::darkgrey,
                         DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);

        // Create audio device selector component
        // This is the standard JUCE component for audio setup
        audioSetupComp = std::make_unique<juce::AudioDeviceSelectorComponent>(
            deviceManager,
            0,      // minimum input channels
            256,    // maximum input channels
            0,      // minimum output channels
            256,    // maximum output channels
            false,  // show MIDI input options
            false,  // show MIDI output options
            false,  // show channels as stereo pairs
            false   // hide advanced options
        );

        setContentOwned(audioSetupComp.get(), true);

        // Window size
        const int preferredWidth = 700;
        const int preferredHeight = 600;

        // Get display bounds to ensure window fits on screen
        auto& displays = juce::Desktop::getInstance().getDisplays();
        const auto* displayPtr = displays.getPrimaryDisplay();
        juce::Rectangle<int> userArea = (displayPtr != nullptr && !displayPtr->userArea.isEmpty())
            ? displayPtr->userArea
            : displays.getTotalBounds(true);

        const int margin = 40;
        const int windowWidth = juce::jmin(preferredWidth, userArea.getWidth() - margin);
        const int windowHeight = juce::jmin(preferredHeight, userArea.getHeight() - margin);

        setResizeLimits(400, 300, userArea.getWidth(), userArea.getHeight());

        centreWithSize(windowWidth, windowHeight);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }

private:
    std::unique_ptr<juce::AudioDeviceSelectorComponent> audioSetupComp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioInterfaceWindow)
};
