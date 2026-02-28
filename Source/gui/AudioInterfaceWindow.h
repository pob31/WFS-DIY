#pragma once

#include <JuceHeader.h>
#include "AudioPatchTab.h"
#include "../WfsParameters.h"
#include "../DSP/TestSignalGenerator.h"
#include "../Localization/LocalizationManager.h"

/**
 * DeviceInfoBar
 *
 * Shows current audio device information at the top of the window.
 * Displays: device type, device name, sample rate, buffer size.
 * Read-only display - device selection must be done externally before opening this window.
 */
class DeviceInfoBar : public juce::Component,
                      private juce::Timer
{
public:
    DeviceInfoBar(juce::AudioDeviceManager& deviceManager);
    ~DeviceInfoBar() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateDeviceInfo();

    juce::AudioDeviceManager& deviceManager;

    juce::String deviceType;
    juce::String deviceName;
    double sampleRate = 0.0;
    int bufferSize = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeviceInfoBar)
};

/**
 * DeviceSettingsPanel
 *
 * Custom device settings panel that shows only device type, device, sample rate,
 * and buffer size. Does NOT show channel selection - all available channels are
 * automatically enabled when a device is selected.
 */
class DeviceSettingsPanel : public juce::Component,
                            private juce::ChangeListener
{
public:
    DeviceSettingsPanel(juce::AudioDeviceManager& deviceManager);
    ~DeviceSettingsPanel() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

    /** Enable/disable the panel (disabled during processing) */
    void setEnabled(bool shouldBeEnabled);

private:
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void updateDeviceTypes();
    void updateDevices();
    void updateSampleRates();
    void updateBufferSizes();
    void updateAllControls();

    void deviceTypeChanged();
    void deviceChanged();
    void sampleRateChanged();
    void bufferSizeChanged();

    void enableAllChannels();

    juce::AudioDeviceManager& deviceManager;

    // UI Components
    juce::Label deviceTypeLabel;
    juce::ComboBox deviceTypeCombo;

    juce::Label deviceLabel;
    juce::ComboBox deviceCombo;

    juce::Label sampleRateLabel;
    juce::ComboBox sampleRateCombo;

    juce::Label bufferSizeLabel;
    juce::ComboBox bufferSizeCombo;

    juce::TextButton controlPanelButton;
    juce::TextButton resetDeviceButton;

    // Track current state to avoid recursive updates
    bool isUpdating = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeviceSettingsPanel)
};

/**
 * PatchTabbedComponent
 *
 * Custom TabbedComponent that gives focus to patch matrices when their tabs are selected.
 */
class PatchTabbedComponent : public juce::TabbedComponent
{
public:
    PatchTabbedComponent()
        : juce::TabbedComponent(juce::TabbedButtonBar::TabsAtTop) {}

    std::function<void(int)> onTabChanged;

    void currentTabChanged(int newCurrentTabIndex, const juce::String&) override
    {
        if (onTabChanged)
            onTabChanged(newCurrentTabIndex);
    }
};

/**
 * AudioInterfaceContent
 *
 * Main content component for the Audio Interface window.
 * Contains device info bar and tabbed interface for input/output patching.
 */
class AudioInterfaceContent : public juce::Component
{
public:
    AudioInterfaceContent(juce::AudioDeviceManager& deviceManager,
                          WFSValueTreeState& valueTreeState,
                          TestSignalGenerator* testSignalGen);
    ~AudioInterfaceContent() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

    /** Notify that processing state changed */
    void setProcessingStateChanged(bool isProcessing);

    /** Reset all tab modes to scrolling */
    void resetAllModes();

    /** Get the input patch tab (for Stream Deck integration). */
    InputPatchTab* getInputPatchTab() { return inputPatchTab; }

    /** Get the output patch tab (for Stream Deck integration). */
    OutputPatchTab* getOutputPatchTab() { return outputPatchTab; }

    /** Get the tabbed component (for Stream Deck tab switching). */
    PatchTabbedComponent& getTabbedComponent() { return tabbedComponent; }

private:
    juce::AudioDeviceManager& deviceManager;
    WFSValueTreeState& parameters;
    TestSignalGenerator* testSignalGenerator;

    // Components
    std::unique_ptr<DeviceInfoBar> deviceInfoBar;
    PatchTabbedComponent tabbedComponent;

    // Custom device settings panel (replaces AudioDeviceSelectorComponent)
    std::unique_ptr<DeviceSettingsPanel> deviceSettingsPanel;

    // Tabs (owned by TabbedComponent)
    InputPatchTab* inputPatchTab = nullptr;
    OutputPatchTab* outputPatchTab = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioInterfaceContent)
};

/**
 * AudioInterfaceWindow
 *
 * Floating window for audio interface patching configuration.
 * Provides visual patching interface for routing WFS channels to hardware I/O.
 * Includes test signal generation for output testing.
 *
 * Note: Audio device selection (driver type, device, sample rate) must be done
 * before opening this window, as those settings require stopping audio processing.
 */
class AudioInterfaceWindow : public juce::DocumentWindow
{
public:
    AudioInterfaceWindow(juce::AudioDeviceManager& deviceManager,
                         WFSValueTreeState& valueTreeState,
                         TestSignalGenerator* testSignalGen);

    ~AudioInterfaceWindow() override = default;

    void closeButtonPressed() override;

    /** Notify that processing state changed */
    void setProcessingStateChanged(bool isProcessing);

    /** Get the content component (for Stream Deck integration). */
    AudioInterfaceContent* getContent() { return content; }

    /** Called by JUCE when window becomes active/inactive. */
    void activeWindowStatusChanged() override
    {
        if (isActiveWindow())
        {
            if (onWindowFocused)
                onWindowFocused();
        }
        else
        {
            if (onWindowUnfocused)
                onWindowUnfocused();
        }
    }

    /** Callback when this window gains focus. */
    std::function<void()> onWindowFocused;

    /** Callback when this window loses focus. */
    std::function<void()> onWindowUnfocused;

private:
    TestSignalGenerator* testSignalGenerator;
    AudioInterfaceContent* content = nullptr;  // Owned by DocumentWindow

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioInterfaceWindow)
};
