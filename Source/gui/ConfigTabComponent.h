#pragma once

#include <JuceHeader.h>

/**
 * Config Tab UI Component
 * Based on WFS-UI_config.csv specification (65 parameters)
 *
 * Sections:
 * - Show (name, location)
 * - I/O (input/output/reverb channels, audio interface, processing toggle)
 * - Stage (dimensions, origin, speed of sound, temperature)
 * - Master Section (level, latency, Haas effect)
 * - Network (IP, ports, targets/servers)
 * - ADM-OSC (offset, scale, flip)
 * - Tracking (protocol, port, offset, scale, flip)
 * - Store/Reload (save/load buttons)
 */
class ConfigTabComponent : public juce::Component
{
public:
    ConfigTabComponent()
    {
        // Show Section
        addAndMakeVisible(showNameLabel);
        showNameLabel.setText("Name:", juce::dontSendNotification);
        addAndMakeVisible(showNameEditor);
        showNameEditor.setText("My Show");

        addAndMakeVisible(showLocationLabel);
        showLocationLabel.setText("Location:", juce::dontSendNotification);
        addAndMakeVisible(showLocationEditor);
        showLocationEditor.setText("");

        // I/O Section
        addAndMakeVisible(inputChannelsLabel);
        inputChannelsLabel.setText("Input Channels:", juce::dontSendNotification);
        addAndMakeVisible(inputChannelsEditor);
        inputChannelsEditor.setText("8");

        addAndMakeVisible(outputChannelsLabel);
        outputChannelsLabel.setText("Output Channels:", juce::dontSendNotification);
        addAndMakeVisible(outputChannelsEditor);
        outputChannelsEditor.setText("16");

        addAndMakeVisible(reverbChannelsLabel);
        reverbChannelsLabel.setText("Reverb Channels:", juce::dontSendNotification);
        addAndMakeVisible(reverbChannelsEditor);
        reverbChannelsEditor.setText("0");

        addAndMakeVisible(audioPatchingButton);
        audioPatchingButton.setButtonText("Audio Interface and Patching Window");

        addAndMakeVisible(processingToggle);
        processingToggle.setButtonText("Processing");
        processingToggle.setClickingTogglesState(true);

        // Stage Section
        addAndMakeVisible(stageWidthLabel);
        stageWidthLabel.setText("Stage Width (m):", juce::dontSendNotification);
        addAndMakeVisible(stageWidthEditor);
        stageWidthEditor.setText("20.0");

        addAndMakeVisible(stageDepthLabel);
        stageDepthLabel.setText("Stage Depth (m):", juce::dontSendNotification);
        addAndMakeVisible(stageDepthEditor);
        stageDepthEditor.setText("10.0");

        addAndMakeVisible(stageHeightLabel);
        stageHeightLabel.setText("Stage Height (m):", juce::dontSendNotification);
        addAndMakeVisible(stageHeightEditor);
        stageHeightEditor.setText("8.0");

        addAndMakeVisible(speedOfSoundLabel);
        speedOfSoundLabel.setText("Speed of Sound (m/s):", juce::dontSendNotification);
        addAndMakeVisible(speedOfSoundEditor);
        speedOfSoundEditor.setText("343.0");

        addAndMakeVisible(temperatureLabel);
        temperatureLabel.setText("Temperature (°C):", juce::dontSendNotification);
        addAndMakeVisible(temperatureEditor);
        temperatureEditor.setText("20.0");

        // Master Section
        addAndMakeVisible(masterLevelLabel);
        masterLevelLabel.setText("Master Level (dB):", juce::dontSendNotification);
        addAndMakeVisible(masterLevelEditor);
        masterLevelEditor.setText("0.0");

        addAndMakeVisible(systemLatencyLabel);
        systemLatencyLabel.setText("System Latency (ms):", juce::dontSendNotification);
        addAndMakeVisible(systemLatencyEditor);
        systemLatencyEditor.setText("0.0");

        addAndMakeVisible(haasEffectLabel);
        haasEffectLabel.setText("Haas Effect (ms):", juce::dontSendNotification);
        addAndMakeVisible(haasEffectEditor);
        haasEffectEditor.setText("0.1");

        // Network Section
        addAndMakeVisible(currentIPLabel);
        currentIPLabel.setText("Current IPv4:", juce::dontSendNotification);
        addAndMakeVisible(currentIPEditor);
        currentIPEditor.setText("127.0.0.1");
        currentIPEditor.setReadOnly(true);

        addAndMakeVisible(udpPortLabel);
        udpPortLabel.setText("UDP Port:", juce::dontSendNotification);
        addAndMakeVisible(udpPortEditor);
        udpPortEditor.setText("9000");

        addAndMakeVisible(tcpPortLabel);
        tcpPortLabel.setText("TCP Port:", juce::dontSendNotification);
        addAndMakeVisible(tcpPortEditor);
        tcpPortEditor.setText("9001");

        addAndMakeVisible(networkLogButton);
        networkLogButton.setButtonText("Open Log Window");

        // Store/Reload Section
        addAndMakeVisible(selectProjectFolderButton);
        selectProjectFolderButton.setButtonText("Select Project Folder");

        addAndMakeVisible(storeCompleteConfigButton);
        storeCompleteConfigButton.setButtonText("Store Complete Configuration");

        addAndMakeVisible(reloadCompleteConfigButton);
        reloadCompleteConfigButton.setButtonText("Reload Complete Configuration");

        addAndMakeVisible(storeSystemConfigButton);
        storeSystemConfigButton.setButtonText("Store System Configuration");

        addAndMakeVisible(reloadSystemConfigButton);
        reloadSystemConfigButton.setButtonText("Reload System Configuration");

        setSize(800, 1200);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::darkgrey);

        // Draw section headers
        g.setColour(juce::Colours::white);
        g.setFont(16.0f);
        g.drawText("Show", 20, 10, 200, 30, juce::Justification::left);
        g.drawText("I/O", 20, 120, 200, 30, juce::Justification::left);
        g.drawText("Stage", 20, 300, 200, 30, juce::Justification::left);
        g.drawText("Master Section", 20, 520, 200, 30, juce::Justification::left);
        g.drawText("Network", 20, 660, 200, 30, juce::Justification::left);
        g.drawText("Store/Reload", 20, 880, 200, 30, juce::Justification::left);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(20);

        // Show Section
        area.removeFromTop(40); // Skip header
        auto row = area.removeFromTop(30);
        showNameLabel.setBounds(row.removeFromLeft(150));
        showNameEditor.setBounds(row.removeFromLeft(300));

        area.removeFromTop(5);
        row = area.removeFromTop(30);
        showLocationLabel.setBounds(row.removeFromLeft(150));
        showLocationEditor.setBounds(row.removeFromLeft(300));

        // I/O Section
        area.removeFromTop(45); // Skip to next section
        row = area.removeFromTop(30);
        inputChannelsLabel.setBounds(row.removeFromLeft(150));
        inputChannelsEditor.setBounds(row.removeFromLeft(100));

        area.removeFromTop(5);
        row = area.removeFromTop(30);
        outputChannelsLabel.setBounds(row.removeFromLeft(150));
        outputChannelsEditor.setBounds(row.removeFromLeft(100));

        area.removeFromTop(5);
        row = area.removeFromTop(30);
        reverbChannelsLabel.setBounds(row.removeFromLeft(150));
        reverbChannelsEditor.setBounds(row.removeFromLeft(100));

        area.removeFromTop(10);
        audioPatchingButton.setBounds(area.removeFromTop(30).removeFromLeft(350));

        area.removeFromTop(10);
        processingToggle.setBounds(area.removeFromTop(30).removeFromLeft(200));

        // Stage Section
        area.removeFromTop(45);
        row = area.removeFromTop(30);
        stageWidthLabel.setBounds(row.removeFromLeft(180));
        stageWidthEditor.setBounds(row.removeFromLeft(100));

        area.removeFromTop(5);
        row = area.removeFromTop(30);
        stageDepthLabel.setBounds(row.removeFromLeft(180));
        stageDepthEditor.setBounds(row.removeFromLeft(100));

        area.removeFromTop(5);
        row = area.removeFromTop(30);
        stageHeightLabel.setBounds(row.removeFromLeft(180));
        stageHeightEditor.setBounds(row.removeFromLeft(100));

        area.removeFromTop(5);
        row = area.removeFromTop(30);
        speedOfSoundLabel.setBounds(row.removeFromLeft(180));
        speedOfSoundEditor.setBounds(row.removeFromLeft(100));

        area.removeFromTop(5);
        row = area.removeFromTop(30);
        temperatureLabel.setBounds(row.removeFromLeft(180));
        temperatureEditor.setBounds(row.removeFromLeft(100));

        // Master Section
        area.removeFromTop(45);
        row = area.removeFromTop(30);
        masterLevelLabel.setBounds(row.removeFromLeft(180));
        masterLevelEditor.setBounds(row.removeFromLeft(100));

        area.removeFromTop(5);
        row = area.removeFromTop(30);
        systemLatencyLabel.setBounds(row.removeFromLeft(180));
        systemLatencyEditor.setBounds(row.removeFromLeft(100));

        area.removeFromTop(5);
        row = area.removeFromTop(30);
        haasEffectLabel.setBounds(row.removeFromLeft(180));
        haasEffectEditor.setBounds(row.removeFromLeft(100));

        // Network Section
        area.removeFromTop(45);
        row = area.removeFromTop(30);
        currentIPLabel.setBounds(row.removeFromLeft(150));
        currentIPEditor.setBounds(row.removeFromLeft(150));

        area.removeFromTop(5);
        row = area.removeFromTop(30);
        udpPortLabel.setBounds(row.removeFromLeft(150));
        udpPortEditor.setBounds(row.removeFromLeft(100));

        area.removeFromTop(5);
        row = area.removeFromTop(30);
        tcpPortLabel.setBounds(row.removeFromLeft(150));
        tcpPortEditor.setBounds(row.removeFromLeft(100));

        area.removeFromTop(10);
        networkLogButton.setBounds(area.removeFromTop(30).removeFromLeft(200));

        // Store/Reload Section
        area.removeFromTop(45);
        selectProjectFolderButton.setBounds(area.removeFromTop(30).removeFromLeft(250));

        area.removeFromTop(10);
        storeCompleteConfigButton.setBounds(area.removeFromTop(30).removeFromLeft(300));

        area.removeFromTop(5);
        reloadCompleteConfigButton.setBounds(area.removeFromTop(30).removeFromLeft(300));

        area.removeFromTop(10);
        storeSystemConfigButton.setBounds(area.removeFromTop(30).removeFromLeft(300));

        area.removeFromTop(5);
        reloadSystemConfigButton.setBounds(area.removeFromTop(30).removeFromLeft(300));
    }

private:
    // Show Section
    juce::Label showNameLabel, showLocationLabel;
    juce::TextEditor showNameEditor, showLocationEditor;

    // I/O Section
    juce::Label inputChannelsLabel, outputChannelsLabel, reverbChannelsLabel;
    juce::TextEditor inputChannelsEditor, outputChannelsEditor, reverbChannelsEditor;
    juce::TextButton audioPatchingButton;
    juce::ToggleButton processingToggle;

    // Stage Section
    juce::Label stageWidthLabel, stageDepthLabel, stageHeightLabel;
    juce::Label speedOfSoundLabel, temperatureLabel;
    juce::TextEditor stageWidthEditor, stageDepthEditor, stageHeightEditor;
    juce::TextEditor speedOfSoundEditor, temperatureEditor;

    // Master Section
    juce::Label masterLevelLabel, systemLatencyLabel, haasEffectLabel;
    juce::TextEditor masterLevelEditor, systemLatencyEditor, haasEffectEditor;

    // Network Section
    juce::Label currentIPLabel, udpPortLabel, tcpPortLabel;
    juce::TextEditor currentIPEditor, udpPortEditor, tcpPortEditor;
    juce::TextButton networkLogButton;

    // Store/Reload Section
    juce::TextButton selectProjectFolderButton;
    juce::TextButton storeCompleteConfigButton, reloadCompleteConfigButton;
    juce::TextButton storeSystemConfigButton, reloadSystemConfigButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConfigTabComponent)
};
