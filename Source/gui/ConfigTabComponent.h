#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"

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
class ConfigTabComponent : public juce::Component,
                           private juce::ValueTree::Listener,
                           private juce::TextEditor::Listener
{
public:
    ConfigTabComponent(WfsParameters& params)
        : parameters(params)
    {
        // Show Section
        addAndMakeVisible(showNameLabel);
        showNameLabel.setText("Name:", juce::dontSendNotification);
        addAndMakeVisible(showNameEditor);

        addAndMakeVisible(showLocationLabel);
        showLocationLabel.setText("Location:", juce::dontSendNotification);
        addAndMakeVisible(showLocationEditor);

        // I/O Section
        addAndMakeVisible(inputChannelsLabel);
        inputChannelsLabel.setText("Input Channels:", juce::dontSendNotification);
        addAndMakeVisible(inputChannelsEditor);

        addAndMakeVisible(outputChannelsLabel);
        outputChannelsLabel.setText("Output Channels:", juce::dontSendNotification);
        addAndMakeVisible(outputChannelsEditor);

        addAndMakeVisible(reverbChannelsLabel);
        reverbChannelsLabel.setText("Reverb Channels:", juce::dontSendNotification);
        addAndMakeVisible(reverbChannelsEditor);

        addAndMakeVisible(audioPatchingButton);
        audioPatchingButton.setButtonText("Audio Interface and Patching Window");

        addAndMakeVisible(processingToggle);
        processingToggle.setButtonText("Processing");
        processingToggle.setClickingTogglesState(true);

        // Stage Section
        addAndMakeVisible(stageWidthLabel);
        stageWidthLabel.setText("Stage Width:", juce::dontSendNotification);
        addAndMakeVisible(stageWidthEditor);
        addAndMakeVisible(stageWidthUnitLabel);
        stageWidthUnitLabel.setText("m", juce::dontSendNotification);

        addAndMakeVisible(stageDepthLabel);
        stageDepthLabel.setText("Stage Depth:", juce::dontSendNotification);
        addAndMakeVisible(stageDepthEditor);
        addAndMakeVisible(stageDepthUnitLabel);
        stageDepthUnitLabel.setText("m", juce::dontSendNotification);

        addAndMakeVisible(stageHeightLabel);
        stageHeightLabel.setText("Stage Height:", juce::dontSendNotification);
        addAndMakeVisible(stageHeightEditor);
        addAndMakeVisible(stageHeightUnitLabel);
        stageHeightUnitLabel.setText("m", juce::dontSendNotification);

        addAndMakeVisible(speedOfSoundLabel);
        speedOfSoundLabel.setText("Speed of Sound:", juce::dontSendNotification);
        addAndMakeVisible(speedOfSoundEditor);
        addAndMakeVisible(speedOfSoundUnitLabel);
        speedOfSoundUnitLabel.setText("m/s", juce::dontSendNotification);

        addAndMakeVisible(temperatureLabel);
        temperatureLabel.setText("Temperature:", juce::dontSendNotification);
        addAndMakeVisible(temperatureEditor);
        addAndMakeVisible(temperatureUnitLabel);
        temperatureUnitLabel.setText("C", juce::dontSendNotification);

        // Master Section
        addAndMakeVisible(masterLevelLabel);
        masterLevelLabel.setText("Master Level:", juce::dontSendNotification);
        addAndMakeVisible(masterLevelEditor);
        addAndMakeVisible(masterLevelUnitLabel);
        masterLevelUnitLabel.setText("dB", juce::dontSendNotification);

        addAndMakeVisible(systemLatencyLabel);
        systemLatencyLabel.setText("System Latency:", juce::dontSendNotification);
        addAndMakeVisible(systemLatencyEditor);
        addAndMakeVisible(systemLatencyUnitLabel);
        systemLatencyUnitLabel.setText("ms", juce::dontSendNotification);

        addAndMakeVisible(haasEffectLabel);
        haasEffectLabel.setText("Haas Effect:", juce::dontSendNotification);
        addAndMakeVisible(haasEffectEditor);
        addAndMakeVisible(haasEffectUnitLabel);
        haasEffectUnitLabel.setText("ms", juce::dontSendNotification);

        // Network Section
        addAndMakeVisible(currentIPLabel);
        currentIPLabel.setText("Current IPv4:", juce::dontSendNotification);
        addAndMakeVisible(currentIPEditor);
        currentIPEditor.setReadOnly(true);

        addAndMakeVisible(udpPortLabel);
        udpPortLabel.setText("UDP Port:", juce::dontSendNotification);
        addAndMakeVisible(udpPortEditor);

        addAndMakeVisible(tcpPortLabel);
        tcpPortLabel.setText("TCP Port:", juce::dontSendNotification);
        addAndMakeVisible(tcpPortEditor);

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

        // Set up text editor listeners
        showNameEditor.addListener(this);
        showLocationEditor.addListener(this);
        inputChannelsEditor.addListener(this);
        outputChannelsEditor.addListener(this);
        reverbChannelsEditor.addListener(this);
        stageWidthEditor.addListener(this);
        stageDepthEditor.addListener(this);
        stageHeightEditor.addListener(this);
        speedOfSoundEditor.addListener(this);
        temperatureEditor.addListener(this);
        masterLevelEditor.addListener(this);
        systemLatencyEditor.addListener(this);
        haasEffectEditor.addListener(this);
        udpPortEditor.addListener(this);
        tcpPortEditor.addListener(this);

        // Set up button callbacks
        storeCompleteConfigButton.onClick = [this]() { saveCompleteConfig(); };
        reloadCompleteConfigButton.onClick = [this]() { loadCompleteConfig(); };
        storeSystemConfigButton.onClick = [this]() { saveSystemConfig(); };
        reloadSystemConfigButton.onClick = [this]() { loadSystemConfig(); };

        // Listen to parameter changes
        parameters.getConfigTree().addListener(this);

        // Load initial values from parameters
        loadParametersToUI();

        setSize(1400, 700);
    }

    ~ConfigTabComponent() override
    {
        parameters.getConfigTree().removeListener(this);
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

    // Parameter system
    WfsParameters& parameters;
    juce::File projectFolder;

    //==============================================================================
    // ValueTree::Listener implementation
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override
    {
        // Update UI when parameters change from elsewhere
        juce::MessageManager::callAsync([this]() { loadParametersToUI(); });
    }

    void valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&) override {}
    void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged(juce::ValueTree&) override {}

    //==============================================================================
    // TextEditor::Listener implementation
    void textEditorTextChanged(juce::TextEditor& editor) override
    {
        updateParameterFromEditor(editor);
    }

    void textEditorReturnKeyPressed(juce::TextEditor&) override {}
    void textEditorEscapeKeyPressed(juce::TextEditor&) override {}
    void textEditorFocusLost(juce::TextEditor&) override {}

    //==============================================================================
    // Helper methods
    void loadParametersToUI()
    {
        showNameEditor.setText(parameters.getConfigParam("ShowName").toString(), false);
        showLocationEditor.setText(parameters.getConfigParam("ShowLocation").toString(), false);
        inputChannelsEditor.setText(parameters.getConfigParam("InputChannels").toString(), false);
        outputChannelsEditor.setText(parameters.getConfigParam("OutputChannels").toString(), false);
        reverbChannelsEditor.setText(parameters.getConfigParam("ReverbChannels").toString(), false);
        stageWidthEditor.setText(parameters.getConfigParam("StageWidth").toString(), false);
        stageDepthEditor.setText(parameters.getConfigParam("StageDepth").toString(), false);
        stageHeightEditor.setText(parameters.getConfigParam("StageHeight").toString(), false);
        speedOfSoundEditor.setText(parameters.getConfigParam("SpeedOfSound").toString(), false);
        temperatureEditor.setText(parameters.getConfigParam("Temperature").toString(), false);
        masterLevelEditor.setText(parameters.getConfigParam("MasterLevel").toString(), false);
        systemLatencyEditor.setText(parameters.getConfigParam("SystemLatency").toString(), false);
        haasEffectEditor.setText(parameters.getConfigParam("HaasEffect").toString(), false);
        currentIPEditor.setText(parameters.getConfigParam("CurrentIPv4").toString(), false);
        udpPortEditor.setText(parameters.getConfigParam("UdpPort").toString(), false);
        tcpPortEditor.setText(parameters.getConfigParam("TcpPort").toString(), false);
    }

    void updateParameterFromEditor(juce::TextEditor& editor)
    {
        auto text = editor.getText();

        if (&editor == &showNameEditor)
            parameters.setConfigParam("ShowName", text);
        else if (&editor == &showLocationEditor)
            parameters.setConfigParam("ShowLocation", text);
        else if (&editor == &inputChannelsEditor)
            parameters.setConfigParam("InputChannels", text.getIntValue());
        else if (&editor == &outputChannelsEditor)
            parameters.setConfigParam("OutputChannels", text.getIntValue());
        else if (&editor == &reverbChannelsEditor)
            parameters.setConfigParam("ReverbChannels", text.getIntValue());
        else if (&editor == &stageWidthEditor)
            parameters.setConfigParam("StageWidth", text.getFloatValue());
        else if (&editor == &stageDepthEditor)
            parameters.setConfigParam("StageDepth", text.getFloatValue());
        else if (&editor == &stageHeightEditor)
            parameters.setConfigParam("StageHeight", text.getFloatValue());
        else if (&editor == &speedOfSoundEditor)
            parameters.setConfigParam("SpeedOfSound", text.getFloatValue());
        else if (&editor == &temperatureEditor)
            parameters.setConfigParam("Temperature", text.getFloatValue());
        else if (&editor == &masterLevelEditor)
            parameters.setConfigParam("MasterLevel", text.getFloatValue());
        else if (&editor == &systemLatencyEditor)
            parameters.setConfigParam("SystemLatency", text.getFloatValue());
        else if (&editor == &haasEffectEditor)
            parameters.setConfigParam("HaasEffect", text.getFloatValue());
        else if (&editor == &udpPortEditor)
            parameters.setConfigParam("UdpPort", text.getIntValue());
        else if (&editor == &tcpPortEditor)
            parameters.setConfigParam("TcpPort", text.getIntValue());
    }

    //==============================================================================
    // Save/Load methods
    void saveCompleteConfig()
    {
        juce::FileChooser chooser("Save Complete Configuration",
                                   projectFolder.getChildFile("complete_config.xml"),
                                   "*.xml");

        if (chooser.browseForFileToSave(true))
        {
            auto file = chooser.getResult();
            if (parameters.saveCompleteConfig(file))
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                    "Success", "Configuration saved successfully");
            else
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                    "Error", "Failed to save configuration");
        }
    }

    void loadCompleteConfig()
    {
        juce::FileChooser chooser("Load Complete Configuration",
                                   projectFolder,
                                   "*.xml");

        if (chooser.browseForFileToOpen())
        {
            auto file = chooser.getResult();
            if (parameters.loadCompleteConfig(file))
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                    "Success", "Configuration loaded successfully");
            else
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                    "Error", "Failed to load configuration");
        }
    }

    void saveSystemConfig()
    {
        juce::FileChooser chooser("Save System Configuration",
                                   projectFolder.getChildFile("system_config.xml"),
                                   "*.xml");

        if (chooser.browseForFileToSave(true))
        {
            auto file = chooser.getResult();
            if (parameters.saveSystemConfig(file))
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                    "Success", "System configuration saved successfully");
            else
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                    "Error", "Failed to save system configuration");
        }
    }

    void loadSystemConfig()
    {
        juce::FileChooser chooser("Load System Configuration",
                                   projectFolder,
                                   "*.xml");

        if (chooser.browseForFileToOpen())
        {
            auto file = chooser.getResult();
            if (parameters.loadSystemConfig(file))
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                    "Success", "System configuration loaded successfully");
            else
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                    "Error", "Failed to load system configuration");
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConfigTabComponent)
};
