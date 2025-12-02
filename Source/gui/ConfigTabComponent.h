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
        // TextEditor defaults to empty, no need to set explicitly

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
        stageWidthLabel.setText("Stage Width:", juce::dontSendNotification);
        addAndMakeVisible(stageWidthEditor);
        stageWidthEditor.setText("20.0");
        addAndMakeVisible(stageWidthUnitLabel);
        stageWidthUnitLabel.setText("m", juce::dontSendNotification);

        addAndMakeVisible(stageDepthLabel);
        stageDepthLabel.setText("Stage Depth:", juce::dontSendNotification);
        addAndMakeVisible(stageDepthEditor);
        stageDepthEditor.setText("10.0");
        addAndMakeVisible(stageDepthUnitLabel);
        stageDepthUnitLabel.setText("m", juce::dontSendNotification);

        addAndMakeVisible(stageHeightLabel);
        stageHeightLabel.setText("Stage Height:", juce::dontSendNotification);
        addAndMakeVisible(stageHeightEditor);
        stageHeightEditor.setText("8.0");
        addAndMakeVisible(stageHeightUnitLabel);
        stageHeightUnitLabel.setText("m", juce::dontSendNotification);

        addAndMakeVisible(speedOfSoundLabel);
        speedOfSoundLabel.setText("Speed of Sound:", juce::dontSendNotification);
        addAndMakeVisible(speedOfSoundEditor);
        speedOfSoundEditor.setText("343.0");
        addAndMakeVisible(speedOfSoundUnitLabel);
        speedOfSoundUnitLabel.setText("m/s", juce::dontSendNotification);

        addAndMakeVisible(temperatureLabel);
        temperatureLabel.setText("Temperature:", juce::dontSendNotification);
        addAndMakeVisible(temperatureEditor);
        temperatureEditor.setText("20.0");
        addAndMakeVisible(temperatureUnitLabel);
        temperatureUnitLabel.setText("C", juce::dontSendNotification);

        // Master Section
        addAndMakeVisible(masterLevelLabel);
        masterLevelLabel.setText("Master Level:", juce::dontSendNotification);
        addAndMakeVisible(masterLevelEditor);
        masterLevelEditor.setText("0.0");
        addAndMakeVisible(masterLevelUnitLabel);
        masterLevelUnitLabel.setText("dB", juce::dontSendNotification);

        addAndMakeVisible(systemLatencyLabel);
        systemLatencyLabel.setText("System Latency:", juce::dontSendNotification);
        addAndMakeVisible(systemLatencyEditor);
        systemLatencyEditor.setText("0.0");
        addAndMakeVisible(systemLatencyUnitLabel);
        systemLatencyUnitLabel.setText("ms", juce::dontSendNotification);

        addAndMakeVisible(haasEffectLabel);
        haasEffectLabel.setText("Haas Effect:", juce::dontSendNotification);
        addAndMakeVisible(haasEffectEditor);
        haasEffectEditor.setText("0.1");
        addAndMakeVisible(haasEffectUnitLabel);
        haasEffectUnitLabel.setText("ms", juce::dontSendNotification);

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

        setSize(1400, 700);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);

        // Draw section headers
        g.setColour(juce::Colours::white);
        g.setFont(16.0f);

        // Left column headers (x=20, y positions calculated from layout)
        g.drawText("Show", 20, 20, 200, 30, juce::Justification::left);
        g.drawText("I/O", 20, 130, 200, 30, juce::Justification::left);
        g.drawText("Store/Reload", 20, 365, 200, 30, juce::Justification::left);

        // Middle column headers (x=480)
        g.drawText("Stage", 480, 20, 200, 30, juce::Justification::left);
        g.drawText("Master Section", 480, 255, 200, 30, juce::Justification::left);

        // Right column headers (x=940)
        g.drawText("Network", 940, 20, 200, 30, juce::Justification::left);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(20);

        // Create three columns
        auto leftColumn = area.removeFromLeft(440);
        area.removeFromLeft(20); // spacing
        auto middleColumn = area.removeFromLeft(440);
        area.removeFromLeft(20); // spacing
        auto rightColumn = area;

        // LEFT COLUMN
        // Show Section
        leftColumn.removeFromTop(40); // Skip header
        auto row = leftColumn.removeFromTop(30);
        showNameLabel.setBounds(row.removeFromLeft(120));
        showNameEditor.setBounds(row.removeFromLeft(300));

        leftColumn.removeFromTop(5);
        row = leftColumn.removeFromTop(30);
        showLocationLabel.setBounds(row.removeFromLeft(120));
        showLocationEditor.setBounds(row.removeFromLeft(300));

        // I/O Section
        leftColumn.removeFromTop(25);
        row = leftColumn.removeFromTop(30);
        inputChannelsLabel.setBounds(row.removeFromLeft(140));
        inputChannelsEditor.setBounds(row.removeFromLeft(100));

        leftColumn.removeFromTop(5);
        row = leftColumn.removeFromTop(30);
        outputChannelsLabel.setBounds(row.removeFromLeft(140));
        outputChannelsEditor.setBounds(row.removeFromLeft(100));

        leftColumn.removeFromTop(5);
        row = leftColumn.removeFromTop(30);
        reverbChannelsLabel.setBounds(row.removeFromLeft(140));
        reverbChannelsEditor.setBounds(row.removeFromLeft(100));

        leftColumn.removeFromTop(10);
        audioPatchingButton.setBounds(leftColumn.removeFromTop(30).removeFromLeft(350));

        leftColumn.removeFromTop(10);
        processingToggle.setBounds(leftColumn.removeFromTop(30).removeFromLeft(200));

        // Store/Reload Section
        leftColumn.removeFromTop(45);
        selectProjectFolderButton.setBounds(leftColumn.removeFromTop(30).removeFromLeft(250));

        leftColumn.removeFromTop(10);
        storeCompleteConfigButton.setBounds(leftColumn.removeFromTop(30).removeFromLeft(300));

        leftColumn.removeFromTop(5);
        reloadCompleteConfigButton.setBounds(leftColumn.removeFromTop(30).removeFromLeft(300));

        leftColumn.removeFromTop(10);
        storeSystemConfigButton.setBounds(leftColumn.removeFromTop(30).removeFromLeft(300));

        leftColumn.removeFromTop(5);
        reloadSystemConfigButton.setBounds(leftColumn.removeFromTop(30).removeFromLeft(300));

        // MIDDLE COLUMN
        // Stage Section
        middleColumn.removeFromTop(40);
        const int editorStartX = 140; // Aligned position for all editors
        const int editorWidth = 80;
        const int unitOffset = 5;
        const int unitWidth = 40;

        row = middleColumn.removeFromTop(30);
        stageWidthLabel.setBounds(row.getX(), row.getY(), editorStartX - row.getX(), 30);
        stageWidthEditor.setBounds(row.getX() + editorStartX, row.getY(), editorWidth, 30);
        stageWidthUnitLabel.setBounds(row.getX() + editorStartX + editorWidth + unitOffset, row.getY(), unitWidth, 30);

        middleColumn.removeFromTop(5);
        row = middleColumn.removeFromTop(30);
        stageDepthLabel.setBounds(row.getX(), row.getY(), editorStartX - row.getX(), 30);
        stageDepthEditor.setBounds(row.getX() + editorStartX, row.getY(), editorWidth, 30);
        stageDepthUnitLabel.setBounds(row.getX() + editorStartX + editorWidth + unitOffset, row.getY(), unitWidth, 30);

        middleColumn.removeFromTop(5);
        row = middleColumn.removeFromTop(30);
        stageHeightLabel.setBounds(row.getX(), row.getY(), editorStartX - row.getX(), 30);
        stageHeightEditor.setBounds(row.getX() + editorStartX, row.getY(), editorWidth, 30);
        stageHeightUnitLabel.setBounds(row.getX() + editorStartX + editorWidth + unitOffset, row.getY(), unitWidth, 30);

        middleColumn.removeFromTop(5);
        row = middleColumn.removeFromTop(30);
        speedOfSoundLabel.setBounds(row.getX(), row.getY(), editorStartX - row.getX(), 30);
        speedOfSoundEditor.setBounds(row.getX() + editorStartX, row.getY(), editorWidth, 30);
        speedOfSoundUnitLabel.setBounds(row.getX() + editorStartX + editorWidth + unitOffset, row.getY(), unitWidth, 30);

        middleColumn.removeFromTop(5);
        row = middleColumn.removeFromTop(30);
        temperatureLabel.setBounds(row.getX(), row.getY(), editorStartX - row.getX(), 30);
        temperatureEditor.setBounds(row.getX() + editorStartX, row.getY(), editorWidth, 30);
        temperatureUnitLabel.setBounds(row.getX() + editorStartX + editorWidth + unitOffset, row.getY(), unitWidth, 30);

        // Master Section
        middleColumn.removeFromTop(35);
        row = middleColumn.removeFromTop(30);
        masterLevelLabel.setBounds(row.getX(), row.getY(), editorStartX - row.getX(), 30);
        masterLevelEditor.setBounds(row.getX() + editorStartX, row.getY(), editorWidth, 30);
        masterLevelUnitLabel.setBounds(row.getX() + editorStartX + editorWidth + unitOffset, row.getY(), unitWidth, 30);

        middleColumn.removeFromTop(5);
        row = middleColumn.removeFromTop(30);
        systemLatencyLabel.setBounds(row.getX(), row.getY(), editorStartX - row.getX(), 30);
        systemLatencyEditor.setBounds(row.getX() + editorStartX, row.getY(), editorWidth, 30);
        systemLatencyUnitLabel.setBounds(row.getX() + editorStartX + editorWidth + unitOffset, row.getY(), unitWidth, 30);

        middleColumn.removeFromTop(5);
        row = middleColumn.removeFromTop(30);
        haasEffectLabel.setBounds(row.getX(), row.getY(), editorStartX - row.getX(), 30);
        haasEffectEditor.setBounds(row.getX() + editorStartX, row.getY(), editorWidth, 30);
        haasEffectUnitLabel.setBounds(row.getX() + editorStartX + editorWidth + unitOffset, row.getY(), unitWidth, 30);

        // RIGHT COLUMN
        // Network Section
        rightColumn.removeFromTop(40);
        row = rightColumn.removeFromTop(30);
        currentIPLabel.setBounds(row.removeFromLeft(120));
        currentIPEditor.setBounds(row.removeFromLeft(150));

        rightColumn.removeFromTop(5);
        row = rightColumn.removeFromTop(30);
        udpPortLabel.setBounds(row.removeFromLeft(120));
        udpPortEditor.setBounds(row.removeFromLeft(100));

        rightColumn.removeFromTop(5);
        row = rightColumn.removeFromTop(30);
        tcpPortLabel.setBounds(row.removeFromLeft(120));
        tcpPortEditor.setBounds(row.removeFromLeft(100));

        rightColumn.removeFromTop(10);
        networkLogButton.setBounds(rightColumn.removeFromTop(30).removeFromLeft(200));
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
    juce::Label stageWidthUnitLabel, stageDepthUnitLabel, stageHeightUnitLabel;
    juce::Label speedOfSoundUnitLabel, temperatureUnitLabel;

    // Master Section
    juce::Label masterLevelLabel, systemLatencyLabel, haasEffectLabel;
    juce::TextEditor masterLevelEditor, systemLatencyEditor, haasEffectEditor;
    juce::Label masterLevelUnitLabel, systemLatencyUnitLabel, haasEffectUnitLabel;

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
