#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"
#include "StatusBar.h"

#if JUCE_WINDOWS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>
    #pragma comment(lib, "iphlpapi.lib")
    #pragma comment(lib, "ws2_32.lib")
#endif

//==============================================================================
// Custom Origin Preset Button - Front (broken rectangle with dot at bottom)
class OriginFrontButton : public juce::Button
{
public:
    OriginFrontButton() : juce::Button("Front") {}

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);

        // Background
        if (shouldDrawButtonAsDown)
            g.setColour(juce::Colour(0xFF404040));
        else if (shouldDrawButtonAsHighlighted)
            g.setColour(juce::Colour(0xFF353535));
        else
            g.setColour(juce::Colour(0xFF2A2A2A));

        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(juce::Colour(0xFF606060));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        // Draw icon - broken rectangle (open at bottom) with dot at front center
        auto iconBounds = bounds.reduced(6.0f);
        float lineThickness = 2.0f;
        float gapSize = iconBounds.getWidth() * 0.55f;  // Wider gap in the middle of bottom edge

        g.setColour(juce::Colours::white);

        // Left side (full height)
        g.drawLine(iconBounds.getX(), iconBounds.getY(),
                   iconBounds.getX(), iconBounds.getBottom(), lineThickness);

        // Top (full width)
        g.drawLine(iconBounds.getX(), iconBounds.getY(),
                   iconBounds.getRight(), iconBounds.getY(), lineThickness);

        // Right side (full height)
        g.drawLine(iconBounds.getRight(), iconBounds.getY(),
                   iconBounds.getRight(), iconBounds.getBottom(), lineThickness);

        // Bottom left piece (partial - leaving gap in center)
        g.drawLine(iconBounds.getX(), iconBounds.getBottom(),
                   iconBounds.getCentreX() - gapSize * 0.5f, iconBounds.getBottom(), lineThickness);

        // Bottom right piece (partial - leaving gap in center)
        g.drawLine(iconBounds.getCentreX() + gapSize * 0.5f, iconBounds.getBottom(),
                   iconBounds.getRight(), iconBounds.getBottom(), lineThickness);

        // Dot at front center (positioned at the bottom edge, in the gap)
        float dotRadius = 2.5f;
        g.fillEllipse(iconBounds.getCentreX() - dotRadius,
                      iconBounds.getBottom() - dotRadius,
                      dotRadius * 2, dotRadius * 2);
    }
};

//==============================================================================
// Custom Origin Preset Button - Center Ground (complete rectangle with dot in center)
class OriginCenterGroundButton : public juce::Button
{
public:
    OriginCenterGroundButton() : juce::Button("Center Ground") {}

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);

        // Background
        if (shouldDrawButtonAsDown)
            g.setColour(juce::Colour(0xFF404040));
        else if (shouldDrawButtonAsHighlighted)
            g.setColour(juce::Colour(0xFF353535));
        else
            g.setColour(juce::Colour(0xFF2A2A2A));

        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(juce::Colour(0xFF606060));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        // Draw icon - complete rectangle with dot in center
        auto iconBounds = bounds.reduced(6.0f);
        float lineThickness = 2.0f;

        g.setColour(juce::Colours::white);

        // Complete rectangle
        g.drawRect(iconBounds, lineThickness);

        // Dot in center
        float dotRadius = 2.5f;
        g.fillEllipse(iconBounds.getCentreX() - dotRadius,
                      iconBounds.getCentreY() - dotRadius,
                      dotRadius * 2, dotRadius * 2);
    }
};

//==============================================================================
// Custom Origin Preset Button - Center (3D cube with dot in center)
class OriginCenterButton : public juce::Button
{
public:
    OriginCenterButton() : juce::Button("Center") {}

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);

        // Background
        if (shouldDrawButtonAsDown)
            g.setColour(juce::Colour(0xFF404040));
        else if (shouldDrawButtonAsHighlighted)
            g.setColour(juce::Colour(0xFF353535));
        else
            g.setColour(juce::Colour(0xFF2A2A2A));

        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(juce::Colour(0xFF606060));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        // Draw icon - 3D cube with dot in center
        auto iconBounds = bounds.reduced(6.0f);
        float lineThickness = 2.0f;
        float depth = iconBounds.getWidth() * 0.3f;  // Depth offset for 3D effect

        g.setColour(juce::Colours::white);

        // Front face (rectangle) - positioned at bottom-left
        auto frontFace = juce::Rectangle<float>(
            iconBounds.getX(),
            iconBounds.getY() + depth,
            iconBounds.getWidth() - depth,
            iconBounds.getHeight() - depth
        );
        g.drawRect(frontFace, lineThickness);

        // Back top-right corner position
        float backRight = iconBounds.getRight();
        float backTop = iconBounds.getY();

        // Top edge of back face
        g.drawLine(frontFace.getX() + depth, backTop, backRight, backTop, lineThickness);
        // Right edge of back face
        g.drawLine(backRight, backTop, backRight, frontFace.getBottom() - depth, lineThickness);

        // Connecting lines (front to back)
        g.drawLine(frontFace.getX(), frontFace.getY(), frontFace.getX() + depth, backTop, lineThickness);
        g.drawLine(frontFace.getRight(), frontFace.getY(), backRight, backTop, lineThickness);
        g.drawLine(frontFace.getRight(), frontFace.getBottom(), backRight, frontFace.getBottom() - depth, lineThickness);

        // Dot in center of cube (visual center)
        float dotRadius = 2.5f;
        float dotX = frontFace.getCentreX() + depth * 0.5f;
        float dotY = frontFace.getCentreY() - depth * 0.5f;
        g.fillEllipse(dotX - dotRadius, dotY - dotRadius, dotRadius * 2, dotRadius * 2);
    }
};

/**
 * System Configuration Tab Component
 * Contains all system-level configuration:
 * - Show (name, location)
 * - I/O (input/output/reverb channels, audio interface, processing toggle)
 * - Stage (dimensions, origin, speed of sound, temperature)
 * - Master Section (level, latency, Haas effect)
 * - Network (IP, ports, targets/servers)
 * - ADM-OSC (offset, scale, flip)
 * - Tracking (protocol, port, offset, scale, flip)
 * - Store/Reload (save/load buttons)
 */
class SystemConfigTab : public juce::Component,
                        private juce::ValueTree::Listener,
                        private juce::TextEditor::Listener
{
public:
    // Callback types for notifying MainComponent of changes
    using ProcessingCallback = std::function<void(bool enabled)>;
    using ChannelCountCallback = std::function<void(int inputs, int outputs)>;
    using AudioInterfaceCallback = std::function<void()>;

    SystemConfigTab(WfsParameters& params)
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
        audioPatchingButton.onClick = [this]() {
            if (onAudioInterfaceWindowRequested)
                onAudioInterfaceWindowRequested();
        };

        // Algorithm selector
        addAndMakeVisible(algorithmLabel);
        algorithmLabel.setText("Algorithm:", juce::dontSendNotification);
        algorithmLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(algorithmSelector);
        algorithmSelector.addItem("InputBuffer (read-time delays)", 1);
        algorithmSelector.addItem("OutputBuffer (write-time delays)", 2);
        // algorithmSelector.addItem("GPU InputBuffer (GPU Audio)", 3);  // Commented out - GPU Audio SDK not configured
        algorithmSelector.setSelectedId(1, juce::dontSendNotification);
        algorithmSelector.onChange = [this]() {
            int selectedId = algorithmSelector.getSelectedId();
            parameters.setConfigParam("ProcessingAlgorithm", selectedId);
        };

        addAndMakeVisible(processingButton);
        processingButton.setButtonText("Processing: OFF");
        processingButton.onClick = [this]() { toggleProcessing(); };

        // Stage Section
        addAndMakeVisible(stageWidthLabel);
        stageWidthLabel.setText("Stage Width:", juce::dontSendNotification);
        stageWidthLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(stageWidthEditor);
        addAndMakeVisible(stageWidthUnitLabel);
        stageWidthUnitLabel.setText("m", juce::dontSendNotification);
        stageWidthUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(stageDepthLabel);
        stageDepthLabel.setText("Stage Depth:", juce::dontSendNotification);
        stageDepthLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(stageDepthEditor);
        addAndMakeVisible(stageDepthUnitLabel);
        stageDepthUnitLabel.setText("m", juce::dontSendNotification);
        stageDepthUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(stageHeightLabel);
        stageHeightLabel.setText("Stage Height:", juce::dontSendNotification);
        stageHeightLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(stageHeightEditor);
        addAndMakeVisible(stageHeightUnitLabel);
        stageHeightUnitLabel.setText("m", juce::dontSendNotification);
        stageHeightUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(stageOriginWidthLabel);
        stageOriginWidthLabel.setText("Origin Width:", juce::dontSendNotification);
        stageOriginWidthLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(stageOriginWidthEditor);
        addAndMakeVisible(stageOriginWidthUnitLabel);
        stageOriginWidthUnitLabel.setText("m", juce::dontSendNotification);
        stageOriginWidthUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(stageOriginDepthLabel);
        stageOriginDepthLabel.setText("Origin Depth:", juce::dontSendNotification);
        stageOriginDepthLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(stageOriginDepthEditor);
        addAndMakeVisible(stageOriginDepthUnitLabel);
        stageOriginDepthUnitLabel.setText("m", juce::dontSendNotification);
        stageOriginDepthUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(stageOriginHeightLabel);
        stageOriginHeightLabel.setText("Origin Height:", juce::dontSendNotification);
        stageOriginHeightLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(stageOriginHeightEditor);
        addAndMakeVisible(stageOriginHeightUnitLabel);
        stageOriginHeightUnitLabel.setText("m", juce::dontSendNotification);
        stageOriginHeightUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        // Origin preset buttons (custom drawn icons)
        addAndMakeVisible(originFrontButton);
        originFrontButton.onClick = [this]() { setOriginToFront(); };

        addAndMakeVisible(originCenterGroundButton);
        originCenterGroundButton.onClick = [this]() { setOriginToCenterGround(); };

        addAndMakeVisible(originCenterButton);
        originCenterButton.onClick = [this]() { setOriginToCenter(); };

        addAndMakeVisible(speedOfSoundLabel);
        speedOfSoundLabel.setText("Speed of Sound:", juce::dontSendNotification);
        speedOfSoundLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(speedOfSoundEditor);
        addAndMakeVisible(speedOfSoundUnitLabel);
        speedOfSoundUnitLabel.setText("m/s", juce::dontSendNotification);
        speedOfSoundUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(temperatureLabel);
        temperatureLabel.setText("Temperature:", juce::dontSendNotification);
        temperatureLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(temperatureEditor);
        addAndMakeVisible(temperatureUnitLabel);
        temperatureUnitLabel.setText(juce::CharPointer_UTF8("\xc2\xb0""C"), juce::dontSendNotification);
        temperatureUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        // Master Section
        addAndMakeVisible(masterLevelLabel);
        masterLevelLabel.setText("Master Level:", juce::dontSendNotification);
        masterLevelLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(masterLevelEditor);
        addAndMakeVisible(masterLevelUnitLabel);
        masterLevelUnitLabel.setText("dB", juce::dontSendNotification);
        masterLevelUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(systemLatencyLabel);
        systemLatencyLabel.setText("System Latency:", juce::dontSendNotification);
        systemLatencyLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(systemLatencyEditor);
        addAndMakeVisible(systemLatencyUnitLabel);
        systemLatencyUnitLabel.setText("ms", juce::dontSendNotification);
        systemLatencyUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(haasEffectLabel);
        haasEffectLabel.setText("Haas Effect:", juce::dontSendNotification);
        haasEffectLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(haasEffectEditor);
        addAndMakeVisible(haasEffectUnitLabel);
        haasEffectUnitLabel.setText("ms", juce::dontSendNotification);
        haasEffectUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        // Store/Reload Section
        addAndMakeVisible(selectProjectFolderButton);
        selectProjectFolderButton.setButtonText("Select Project Folder");
        selectProjectFolderButton.onClick = [this]() { selectProjectFolder(); };

        addAndMakeVisible(storeCompleteConfigButton);
        storeCompleteConfigButton.setButtonText("Store Complete Configuration");
        storeCompleteConfigButton.onClick = [this]() { storeCompleteConfiguration(); };

        addAndMakeVisible(reloadCompleteConfigButton);
        reloadCompleteConfigButton.setButtonText("Reload Complete Configuration");
        reloadCompleteConfigButton.onClick = [this]() { reloadCompleteConfiguration(); };

        addAndMakeVisible(reloadCompleteConfigBackupButton);
        reloadCompleteConfigBackupButton.setButtonText("Reload Complete Config. Backup");
        reloadCompleteConfigBackupButton.onClick = [this]() { reloadCompleteConfigBackup(); };

        addAndMakeVisible(storeSystemConfigButton);
        storeSystemConfigButton.setButtonText("Store System Configuration");
        storeSystemConfigButton.onClick = [this]() { storeSystemConfiguration(); };

        addAndMakeVisible(reloadSystemConfigButton);
        reloadSystemConfigButton.setButtonText("Reload System Configuration");
        reloadSystemConfigButton.onClick = [this]() { reloadSystemConfiguration(); };

        addAndMakeVisible(reloadSystemConfigBackupButton);
        reloadSystemConfigBackupButton.setButtonText("Reload System Config. Backup");
        reloadSystemConfigBackupButton.onClick = [this]() { reloadSystemConfigBackup(); };

        addAndMakeVisible(importSystemConfigButton);
        importSystemConfigButton.setButtonText("Import System Configuration");
        importSystemConfigButton.onClick = [this]() { importSystemConfiguration(); };

        addAndMakeVisible(exportSystemConfigButton);
        exportSystemConfigButton.setButtonText("Export System Configuration");
        exportSystemConfigButton.onClick = [this]() { exportSystemConfiguration(); };

        // Setup numeric input filtering
        setupNumericEditors();

        // Add text editor listeners
        showNameEditor.addListener(this);
        showLocationEditor.addListener(this);
        inputChannelsEditor.addListener(this);
        outputChannelsEditor.addListener(this);
        reverbChannelsEditor.addListener(this);
        stageWidthEditor.addListener(this);
        stageDepthEditor.addListener(this);
        stageHeightEditor.addListener(this);
        stageOriginWidthEditor.addListener(this);
        stageOriginDepthEditor.addListener(this);
        stageOriginHeightEditor.addListener(this);
        speedOfSoundEditor.addListener(this);
        temperatureEditor.addListener(this);
        masterLevelEditor.addListener(this);
        systemLatencyEditor.addListener(this);
        haasEffectEditor.addListener(this);

        // Listen to parameter changes
        parameters.getConfigTree().addListener(this);

        // Load initial values
        loadParametersToUI();
    }

    ~SystemConfigTab() override
    {
        parameters.getConfigTree().removeListener(this);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xFF1E1E1E));

        // Section headers
        g.setColour(juce::Colours::white);
        g.setFont(14.0f);
        g.drawText("Show", 20, 10, 200, 20, juce::Justification::left);
        g.drawText("I/O", 20, 110, 200, 20, juce::Justification::left);
        g.drawText("Stage", 500, 10, 200, 20, juce::Justification::left);
        g.drawText("Master Section", 500, 390, 200, 20, juce::Justification::left);
    }

    void resized() override
    {
        const int labelWidth = 150;
        const int editorWidth = 200;
        const int unitWidth = 40;
        const int rowHeight = 30;
        const int spacing = 5;

        int x = 20;
        int y = 40;

        // Show Section
        showNameLabel.setBounds(x, y, labelWidth, rowHeight);
        showNameEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        showLocationLabel.setBounds(x, y, labelWidth, rowHeight);
        showLocationEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        // I/O Section
        y = 140; // Start after "I/O" header
        inputChannelsLabel.setBounds(x, y, labelWidth, rowHeight);
        inputChannelsEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        outputChannelsLabel.setBounds(x, y, labelWidth, rowHeight);
        outputChannelsEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        reverbChannelsLabel.setBounds(x, y, labelWidth, rowHeight);
        reverbChannelsEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        audioPatchingButton.setBounds(x, y, editorWidth + labelWidth, rowHeight);
        y += rowHeight + spacing;

        algorithmLabel.setBounds(x, y, labelWidth, rowHeight);
        algorithmSelector.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        processingButton.setBounds(x, y, editorWidth + labelWidth, rowHeight);

        // Stage Section
        x = 500;
        y = 40;

        stageWidthLabel.setBounds(x, y, labelWidth, rowHeight);
        stageWidthEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        stageWidthUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        y += rowHeight + spacing;

        stageDepthLabel.setBounds(x, y, labelWidth, rowHeight);
        stageDepthEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        stageDepthUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        y += rowHeight + spacing;

        stageHeightLabel.setBounds(x, y, labelWidth, rowHeight);
        stageHeightEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        stageHeightUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        y += rowHeight + spacing;

        // Origin coordinates with preset buttons to the right of each row
        const int originButtonSize = 30;  // Square buttons sized to match row height
        int buttonX = x + labelWidth + editorWidth + spacing + unitWidth + spacing;

        stageOriginWidthLabel.setBounds(x, y, labelWidth, rowHeight);
        stageOriginWidthEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        stageOriginWidthUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        originFrontButton.setBounds(buttonX, y, originButtonSize, rowHeight);
        y += rowHeight + spacing;

        stageOriginDepthLabel.setBounds(x, y, labelWidth, rowHeight);
        stageOriginDepthEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        stageOriginDepthUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        originCenterGroundButton.setBounds(buttonX, y, originButtonSize, rowHeight);
        y += rowHeight + spacing;

        stageOriginHeightLabel.setBounds(x, y, labelWidth, rowHeight);
        stageOriginHeightEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        stageOriginHeightUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        originCenterButton.setBounds(buttonX, y, originButtonSize, rowHeight);
        y += rowHeight + spacing;

        speedOfSoundLabel.setBounds(x, y, labelWidth, rowHeight);
        speedOfSoundEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        speedOfSoundUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        y += rowHeight + spacing;

        temperatureLabel.setBounds(x, y, labelWidth, rowHeight);
        temperatureEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        temperatureUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);

        // Master Section
        x = 500;
        y = 420;

        masterLevelLabel.setBounds(x, y, labelWidth, rowHeight);
        masterLevelEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        masterLevelUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        y += rowHeight + spacing;

        systemLatencyLabel.setBounds(x, y, labelWidth, rowHeight);
        systemLatencyEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        systemLatencyUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        y += rowHeight + spacing;

        haasEffectLabel.setBounds(x, y, labelWidth, rowHeight);
        haasEffectEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        haasEffectUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);

        // Footer buttons - full width at bottom (matching Output tab style)
        const int footerHeight = 90;  // Two 30px button rows + 10px spacing + 20px padding
        const int footerPadding = 10;
        const int buttonRowHeight = 30;  // Same as Output tab buttons
        auto footerArea = getLocalBounds().removeFromBottom(footerHeight).reduced(footerPadding, footerPadding);

        // Row 1: Project folder + Complete config buttons - 4 items
        auto row1 = footerArea.removeFromTop(buttonRowHeight);
        const int row1ButtonWidth = (row1.getWidth() - spacing * 3) / 4;

        selectProjectFolderButton.setBounds(row1.removeFromLeft(row1ButtonWidth));
        row1.removeFromLeft(spacing);
        storeCompleteConfigButton.setBounds(row1.removeFromLeft(row1ButtonWidth));
        row1.removeFromLeft(spacing);
        reloadCompleteConfigButton.setBounds(row1.removeFromLeft(row1ButtonWidth));
        row1.removeFromLeft(spacing);
        reloadCompleteConfigBackupButton.setBounds(row1);

        footerArea.removeFromTop(footerPadding);  // Same spacing as padding for consistency

        // Row 2: System config buttons - 5 equal-width buttons
        auto row2 = footerArea.removeFromTop(buttonRowHeight);
        const int sysButtonWidth = (row2.getWidth() - spacing * 4) / 5;

        storeSystemConfigButton.setBounds(row2.removeFromLeft(sysButtonWidth));
        row2.removeFromLeft(spacing);
        reloadSystemConfigButton.setBounds(row2.removeFromLeft(sysButtonWidth));
        row2.removeFromLeft(spacing);
        reloadSystemConfigBackupButton.setBounds(row2.removeFromLeft(sysButtonWidth));
        row2.removeFromLeft(spacing);
        importSystemConfigButton.setBounds(row2.removeFromLeft(sysButtonWidth));
        row2.removeFromLeft(spacing);
        exportSystemConfigButton.setBounds(row2);
    }

    void setStatusBar(StatusBar* bar)
    {
        statusBar = bar;
        setupHelpText();
        setupMouseListeners();
    }

    void setProcessingCallback(ProcessingCallback callback)
    {
        onProcessingChanged = callback;
    }

    void setChannelCountCallback(ChannelCountCallback callback)
    {
        onChannelCountChanged = callback;
    }

    void setAudioInterfaceCallback(AudioInterfaceCallback callback)
    {
        onAudioInterfaceWindowRequested = callback;
    }

private:
    //==============================================================================
    // Helper methods

    void setupNumericEditors()
    {
        auto setupNumericEditor = [](juce::TextEditor& editor, bool allowNegative, bool allowDecimal) {
            juce::String allowedChars = "0123456789";
            if (allowNegative) allowedChars += "-";
            if (allowDecimal) allowedChars += ".";
            editor.setInputFilter(new juce::TextEditor::LengthAndCharacterRestriction(10, allowedChars), true);
            editor.setPopupMenuEnabled(false);
            editor.setSelectAllWhenFocused(true);
        };

        // Stage Section - floats, no negatives (take absolute value)
        setupNumericEditor(stageWidthEditor, false, true);  // 0.0 to 100.0
        setupNumericEditor(stageDepthEditor, false, true);  // 0.0 to 100.0
        setupNumericEditor(stageHeightEditor, false, true);  // 0.0 to 100.0

        // Stage Origin - floats, allows negatives
        setupNumericEditor(stageOriginWidthEditor, true, true);  // -100.0 to 200.0
        setupNumericEditor(stageOriginDepthEditor, true, true);  // -100.0 to 200.0
        setupNumericEditor(stageOriginHeightEditor, true, true);  // -100.0 to 200.0

        // Speed of Sound - float, no negative (take absolute value)
        setupNumericEditor(speedOfSoundEditor, false, true);  // 319.2 to 367.7

        // Temperature - float, allows negative
        setupNumericEditor(temperatureEditor, true, true);  // -20.0 to 60.0

        // Master Section
        setupNumericEditor(masterLevelEditor, true, true);  // -92.0 to 0.0 (allows negative)
        setupNumericEditor(systemLatencyEditor, false, true);  // 0.0 to 10.0 (no negative)
        setupNumericEditor(haasEffectEditor, false, true);  // 0.0 to 10.0 (no negative)

        // I/O Section - integers only
        setupNumericEditor(inputChannelsEditor, false, false);
        setupNumericEditor(outputChannelsEditor, false, false);
        setupNumericEditor(reverbChannelsEditor, false, false);
    }

    //==============================================================================
    // TextEditor::Listener callbacks

    void textEditorTextChanged(juce::TextEditor&) override
    {
        // Don't update parameters during typing - only on Enter or focus lost
    }

    void textEditorReturnKeyPressed(juce::TextEditor& editor) override
    {
        validateAndClampValue(editor);
        editor.giveAwayKeyboardFocus();
    }

    void textEditorEscapeKeyPressed(juce::TextEditor& editor) override
    {
        // Restore original value from parameters without updating
        if (&editor == &showNameEditor)
            editor.setText(parameters.getConfigParam("ShowName").toString(), false);
        else if (&editor == &showLocationEditor)
            editor.setText(parameters.getConfigParam("ShowLocation").toString(), false);
        else if (&editor == &inputChannelsEditor)
            editor.setText(parameters.getConfigParam("InputChannels").toString(), false);
        else if (&editor == &outputChannelsEditor)
            editor.setText(parameters.getConfigParam("OutputChannels").toString(), false);
        else if (&editor == &reverbChannelsEditor)
            editor.setText(parameters.getConfigParam("ReverbChannels").toString(), false);
        else if (&editor == &stageWidthEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("StageWidth"), 2), false);
        else if (&editor == &stageDepthEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("StageDepth"), 2), false);
        else if (&editor == &stageHeightEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("StageHeight"), 2), false);
        else if (&editor == &stageOriginWidthEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("StageOriginWidth"), 2), false);
        else if (&editor == &stageOriginDepthEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("StageOriginDepth"), 2), false);
        else if (&editor == &stageOriginHeightEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("StageOriginHeight"), 2), false);
        else if (&editor == &speedOfSoundEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("SpeedOfSound"), 2), false);
        else if (&editor == &temperatureEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("Temperature"), 2), false);
        else if (&editor == &masterLevelEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("MasterLevel"), 2), false);
        else if (&editor == &systemLatencyEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("SystemLatency"), 2), false);
        else if (&editor == &haasEffectEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("HaasEffect"), 2), false);

        editor.giveAwayKeyboardFocus();
    }

    void textEditorFocusLost(juce::TextEditor& editor) override
    {
        validateAndClampValue(editor);
    }

    //==============================================================================
    // ValueTree::Listener callbacks

    void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override
    {
        loadParametersToUI();
    }

    //==============================================================================
    // Parameter management

    void loadParametersToUI()
    {
        // String values
        showNameEditor.setText(parameters.getConfigParam("ShowName").toString(), false);
        showLocationEditor.setText(parameters.getConfigParam("ShowLocation").toString(), false);

        // Integer values
        inputChannelsEditor.setText(parameters.getConfigParam("InputChannels").toString(), false);
        outputChannelsEditor.setText(parameters.getConfigParam("OutputChannels").toString(), false);
        reverbChannelsEditor.setText(parameters.getConfigParam("ReverbChannels").toString(), false);

        // Float values - format with 2 decimal places to avoid precision issues
        stageWidthEditor.setText(juce::String((float)parameters.getConfigParam("StageWidth"), 2), false);
        stageDepthEditor.setText(juce::String((float)parameters.getConfigParam("StageDepth"), 2), false);
        stageHeightEditor.setText(juce::String((float)parameters.getConfigParam("StageHeight"), 2), false);
        stageOriginWidthEditor.setText(juce::String((float)parameters.getConfigParam("StageOriginWidth"), 2), false);
        stageOriginDepthEditor.setText(juce::String((float)parameters.getConfigParam("StageOriginDepth"), 2), false);
        stageOriginHeightEditor.setText(juce::String((float)parameters.getConfigParam("StageOriginHeight"), 2), false);
        speedOfSoundEditor.setText(juce::String((float)parameters.getConfigParam("SpeedOfSound"), 2), false);
        temperatureEditor.setText(juce::String((float)parameters.getConfigParam("Temperature"), 2), false);
        masterLevelEditor.setText(juce::String((float)parameters.getConfigParam("MasterLevel"), 2), false);
        systemLatencyEditor.setText(juce::String((float)parameters.getConfigParam("SystemLatency"), 2), false);
        haasEffectEditor.setText(juce::String((float)parameters.getConfigParam("HaasEffect"), 2), false);

        // Algorithm selector
        int algorithmId = (int)parameters.getConfigParam("ProcessingAlgorithm");
        if (algorithmId >= 1 && algorithmId <= 2)  // Valid range for current algorithms
            algorithmSelector.setSelectedId(algorithmId, juce::dontSendNotification);

        // Processing button state
        processingEnabled = (bool)parameters.getConfigParam("ProcessingEnabled");
        processingButton.setButtonText(processingEnabled ? "Processing: ON" : "Processing: OFF");

        // Update I/O controls enabled state based on processing state
        updateIOControlsEnabledState();
    }

    void updateParameterFromEditor(juce::TextEditor& editor)
    {
        auto text = editor.getText();

        if (&editor == &showNameEditor)
            parameters.setConfigParam("ShowName", text);
        else if (&editor == &showLocationEditor)
            parameters.setConfigParam("ShowLocation", text);
        else if (&editor == &inputChannelsEditor)
        {
            parameters.setConfigParam("InputChannels", text.getIntValue());
            // Notify MainComponent of channel count change
            if (onChannelCountChanged)
            {
                int inputs = text.getIntValue();
                int outputs = (int)parameters.getConfigParam("OutputChannels");
                onChannelCountChanged(inputs, outputs);
            }
        }
        else if (&editor == &outputChannelsEditor)
        {
            parameters.setConfigParam("OutputChannels", text.getIntValue());
            // Notify MainComponent of channel count change
            if (onChannelCountChanged)
            {
                int inputs = (int)parameters.getConfigParam("InputChannels");
                int outputs = text.getIntValue();
                onChannelCountChanged(inputs, outputs);
            }
        }
        else if (&editor == &reverbChannelsEditor)
            parameters.setConfigParam("ReverbChannels", text.getIntValue());
        else if (&editor == &stageWidthEditor)
            parameters.setConfigParam("StageWidth", text.getFloatValue());
        else if (&editor == &stageDepthEditor)
            parameters.setConfigParam("StageDepth", text.getFloatValue());
        else if (&editor == &stageHeightEditor)
            parameters.setConfigParam("StageHeight", text.getFloatValue());
        else if (&editor == &stageOriginWidthEditor)
            parameters.setConfigParam("StageOriginWidth", text.getFloatValue());
        else if (&editor == &stageOriginDepthEditor)
            parameters.setConfigParam("StageOriginDepth", text.getFloatValue());
        else if (&editor == &stageOriginHeightEditor)
            parameters.setConfigParam("StageOriginHeight", text.getFloatValue());
        else if (&editor == &speedOfSoundEditor)
        {
            float speedOfSound = text.getFloatValue();
            parameters.setConfigParam("SpeedOfSound", speedOfSound);
            // Calculate and update temperature: T = (c - 331.3) / 0.606
            float temperature = (speedOfSound - 331.3f) / 0.606f;
            temperature = juce::jlimit(-20.0f, 60.0f, temperature);
            parameters.setConfigParam("Temperature", temperature);
        }
        else if (&editor == &temperatureEditor)
        {
            float temperature = text.getFloatValue();
            parameters.setConfigParam("Temperature", temperature);
            // Calculate and update speed of sound: c = 331.3 + 0.606 * T
            float speedOfSound = 331.3f + 0.606f * temperature;
            speedOfSound = juce::jlimit(319.2f, 367.7f, speedOfSound);
            parameters.setConfigParam("SpeedOfSound", speedOfSound);
        }
        else if (&editor == &masterLevelEditor)
            parameters.setConfigParam("MasterLevel", text.getFloatValue());
        else if (&editor == &systemLatencyEditor)
            parameters.setConfigParam("SystemLatency", text.getFloatValue());
        else if (&editor == &haasEffectEditor)
            parameters.setConfigParam("HaasEffect", text.getFloatValue());
    }

    void validateAndClampValue(juce::TextEditor& editor)
    {
        auto text = editor.getText();

        // String fields - just update parameter, no validation needed
        if (&editor == &showNameEditor || &editor == &showLocationEditor)
        {
            updateParameterFromEditor(editor);
            return;
        }

        // If empty, restore default
        if (text.isEmpty())
        {
            loadParametersToUI();
            return;
        }

        float value = text.getFloatValue();

        // Validate and clamp based on CSV specifications
        // Take absolute value for fields that don't allow negatives
        if (&editor == &stageWidthEditor)
            value = juce::jlimit(0.0f, 100.0f, std::abs(value));
        else if (&editor == &stageDepthEditor)
            value = juce::jlimit(0.0f, 100.0f, std::abs(value));
        else if (&editor == &stageHeightEditor)
            value = juce::jlimit(0.0f, 100.0f, std::abs(value));
        else if (&editor == &stageOriginWidthEditor)
            value = juce::jlimit(-100.0f, 200.0f, value);  // Allows negative
        else if (&editor == &stageOriginDepthEditor)
            value = juce::jlimit(-100.0f, 200.0f, value);  // Allows negative
        else if (&editor == &stageOriginHeightEditor)
            value = juce::jlimit(-100.0f, 200.0f, value);  // Allows negative
        else if (&editor == &speedOfSoundEditor)
            value = juce::jlimit(319.2f, 367.7f, std::abs(value));
        else if (&editor == &temperatureEditor)
            value = juce::jlimit(-20.0f, 60.0f, value);  // Allows negative
        else if (&editor == &masterLevelEditor)
            value = juce::jlimit(-92.0f, 0.0f, value);  // Allows negative
        else if (&editor == &systemLatencyEditor)
            value = juce::jlimit(0.0f, 10.0f, std::abs(value));
        else if (&editor == &haasEffectEditor)
            value = juce::jlimit(0.0f, 10.0f, std::abs(value));

        // Update display with clamped value (2 decimal places for floats)
        if (&editor == &inputChannelsEditor || &editor == &outputChannelsEditor ||
            &editor == &reverbChannelsEditor)
        {
            editor.setText(juce::String((int)value), false);
        }
        else
        {
            editor.setText(juce::String(value, 2), false);
        }

        // Update parameter
        updateParameterFromEditor(editor);
    }

    //==============================================================================
    // Processing toggle

    void toggleProcessing()
    {
        processingEnabled = !processingEnabled;
        processingButton.setButtonText(processingEnabled ? "Processing: ON" : "Processing: OFF");
        parameters.setConfigParam("ProcessingEnabled", processingEnabled);

        // Lock/unlock I/O controls based on processing state
        updateIOControlsEnabledState();

        // Notify MainComponent of processing state change
        if (onProcessingChanged)
            onProcessingChanged(processingEnabled);
    }

    void updateIOControlsEnabledState()
    {
        // When processing is ON, disable I/O controls to prevent changes
        bool enabled = !processingEnabled;

        inputChannelsEditor.setEnabled(enabled);
        outputChannelsEditor.setEnabled(enabled);
        reverbChannelsEditor.setEnabled(enabled);
        audioPatchingButton.setEnabled(enabled);
        algorithmSelector.setEnabled(enabled);

        // Visual feedback - dim disabled controls
        auto disabledColour = juce::Colour(0xFF808080);
        auto enabledColour = juce::Colours::white;

        inputChannelsEditor.setColour(juce::TextEditor::textColourId, enabled ? enabledColour : disabledColour);
        outputChannelsEditor.setColour(juce::TextEditor::textColourId, enabled ? enabledColour : disabledColour);
        reverbChannelsEditor.setColour(juce::TextEditor::textColourId, enabled ? enabledColour : disabledColour);
    }

    //==============================================================================
    // Origin preset methods

    void setOriginToFront()
    {
        // Front of stage: X = half width, Y = 0, Z = 0
        float stageWidth = (float)parameters.getConfigParam("StageWidth");
        parameters.setConfigParam("StageOriginWidth", stageWidth * 0.5f);
        parameters.setConfigParam("StageOriginDepth", 0.0f);
        parameters.setConfigParam("StageOriginHeight", 0.0f);
    }

    void setOriginToCenterGround()
    {
        // Center at ground level: X = half width, Y = half depth, Z = 0
        float stageWidth = (float)parameters.getConfigParam("StageWidth");
        float stageDepth = (float)parameters.getConfigParam("StageDepth");
        parameters.setConfigParam("StageOriginWidth", stageWidth * 0.5f);
        parameters.setConfigParam("StageOriginDepth", stageDepth * 0.5f);
        parameters.setConfigParam("StageOriginHeight", 0.0f);
    }

    void setOriginToCenter()
    {
        // Center of stage: X = half width, Y = half depth, Z = half height
        float stageWidth = (float)parameters.getConfigParam("StageWidth");
        float stageDepth = (float)parameters.getConfigParam("StageDepth");
        float stageHeight = (float)parameters.getConfigParam("StageHeight");
        parameters.setConfigParam("StageOriginWidth", stageWidth * 0.5f);
        parameters.setConfigParam("StageOriginDepth", stageDepth * 0.5f);
        parameters.setConfigParam("StageOriginHeight", stageHeight * 0.5f);
    }

    //==============================================================================
    // Store/Reload methods

    void selectProjectFolder()
    {
        auto chooser = std::make_shared<juce::FileChooser>("Select Project Folder", projectFolder);
        auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories;

        chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result.exists() && result.isDirectory())
            {
                projectFolder = result;
                parameters.setConfigParam("ProjectFolder", projectFolder.getFullPathName());
            }
        });
    }

    void storeCompleteConfiguration()
    {
        if (!projectFolder.exists())
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "No Project Folder", "Please select a project folder first.");
            return;
        }

        auto configFile = projectFolder.getChildFile("complete_config.xml");
        if (configFile.existsAsFile())
        {
            if (!juce::AlertWindow::showOkCancelBox(juce::AlertWindow::QuestionIcon,
                "Overwrite File?", "The file already exists. Do you want to overwrite it?",
                juce::String(), juce::String(), nullptr, nullptr))
                return;
        }

        // TODO: Implement actual save logic
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
            "Store Complete Configuration", "Configuration will be saved to:\n" + configFile.getFullPathName());
    }

    void reloadCompleteConfiguration()
    {
        if (!projectFolder.exists())
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "No Project Folder", "Please select a project folder first.");
            return;
        }

        auto configFile = projectFolder.getChildFile("complete_config.xml");
        if (!configFile.existsAsFile())
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "File Not Found", "Configuration file not found:\n" + configFile.getFullPathName());
            return;
        }

        if (!juce::AlertWindow::showOkCancelBox(juce::AlertWindow::QuestionIcon,
            "Reload Configuration?", "This will replace the current configuration. Continue?",
            juce::String(), juce::String(), nullptr, nullptr))
            return;

        // TODO: Implement actual load logic
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
            "Reload Complete Configuration", "Configuration will be loaded from:\n" + configFile.getFullPathName());
    }

    void reloadCompleteConfigBackup()
    {
        if (!projectFolder.exists())
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "No Project Folder", "Please select a project folder first.");
            return;
        }

        auto configFile = projectFolder.getChildFile("complete_config.backup.xml");
        if (!configFile.existsAsFile())
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "File Not Found", "Backup file not found:\n" + configFile.getFullPathName());
            return;
        }

        if (!juce::AlertWindow::showOkCancelBox(juce::AlertWindow::QuestionIcon,
            "Reload Backup?", "This will replace the current configuration with the backup. Continue?",
            juce::String(), juce::String(), nullptr, nullptr))
            return;

        // TODO: Implement actual load logic
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
            "Reload Complete Config Backup", "Configuration will be loaded from:\n" + configFile.getFullPathName());
    }

    void storeSystemConfiguration()
    {
        if (!projectFolder.exists())
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "No Project Folder", "Please select a project folder first.");
            return;
        }

        auto configFile = projectFolder.getChildFile("system_config.xml");
        if (configFile.existsAsFile())
        {
            if (!juce::AlertWindow::showOkCancelBox(juce::AlertWindow::QuestionIcon,
                "Overwrite File?", "The file already exists. Do you want to overwrite it?",
                juce::String(), juce::String(), nullptr, nullptr))
                return;
        }

        // TODO: Implement actual save logic
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
            "Store System Configuration", "System configuration will be saved to:\n" + configFile.getFullPathName());
    }

    void reloadSystemConfiguration()
    {
        if (!projectFolder.exists())
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "No Project Folder", "Please select a project folder first.");
            return;
        }

        auto configFile = projectFolder.getChildFile("system_config.xml");
        if (!configFile.existsAsFile())
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "File Not Found", "System configuration file not found:\n" + configFile.getFullPathName());
            return;
        }

        if (!juce::AlertWindow::showOkCancelBox(juce::AlertWindow::QuestionIcon,
            "Reload System Configuration?", "This will replace the current system configuration. Continue?",
            juce::String(), juce::String(), nullptr, nullptr))
            return;

        // TODO: Implement actual load logic
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
            "Reload System Configuration", "System configuration will be loaded from:\n" + configFile.getFullPathName());
    }

    void reloadSystemConfigBackup()
    {
        if (!projectFolder.exists())
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "No Project Folder", "Please select a project folder first.");
            return;
        }

        auto configFile = projectFolder.getChildFile("system_config.backup.xml");
        if (!configFile.existsAsFile())
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "File Not Found", "System backup file not found:\n" + configFile.getFullPathName());
            return;
        }

        if (!juce::AlertWindow::showOkCancelBox(juce::AlertWindow::QuestionIcon,
            "Reload System Backup?", "This will replace the current system configuration with the backup. Continue?",
            juce::String(), juce::String(), nullptr, nullptr))
            return;

        // TODO: Implement actual load logic
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
            "Reload System Config Backup", "System configuration will be loaded from:\n" + configFile.getFullPathName());
    }

    void importSystemConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser>("Import System Configuration",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.xml");
        auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result.existsAsFile())
            {
                if (!juce::AlertWindow::showOkCancelBox(juce::AlertWindow::QuestionIcon,
                    "Import Configuration?", "This will replace the current system configuration. Continue?",
                    juce::String(), juce::String(), nullptr, nullptr))
                    return;

                // TODO: Implement actual load logic
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                    "Import System Configuration", "System configuration will be loaded from:\n" + result.getFullPathName());
            }
        });
    }

    void exportSystemConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser>("Export System Configuration",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.xml");
        auto chooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result != juce::File())
            {
                // Add .xml extension if not present
                if (!result.hasFileExtension(".xml"))
                    result = result.withFileExtension(".xml");

                if (result.existsAsFile())
                {
                    if (!juce::AlertWindow::showOkCancelBox(juce::AlertWindow::QuestionIcon,
                        "Overwrite File?", "The file already exists. Do you want to overwrite it?",
                        juce::String(), juce::String(), nullptr, nullptr))
                        return;
                }

                // TODO: Implement actual save logic
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                    "Export System Configuration", "System configuration will be saved to:\n" + result.getFullPathName());
            }
        });
    }

    //==============================================================================
    // Status bar helper methods

    void setupHelpText()
    {
        // Based on WFS-UI_config.csv help text column
        helpTextMap[&showNameEditor] = "Name of the current show.";
        helpTextMap[&showLocationEditor] = "Location of the current show.";
        helpTextMap[&inputChannelsEditor] = "Number of Input Channels.";
        helpTextMap[&outputChannelsEditor] = "Number of Output Channels.";
        helpTextMap[&reverbChannelsEditor] = "Number of Reverb Channels.";
        helpTextMap[&audioPatchingButton] = "Open patching window to route Input and Output channels to the Audio Interface.";
        helpTextMap[&algorithmSelector] = "Select the rendering algorithm from the menu.";
        helpTextMap[&processingButton] = "Lock all I/O parameters and start the DSP. Long press to stop the DSP.";
        helpTextMap[&stageWidthEditor] = "Width of the stage (used for remote application and ADM-OSC).";
        helpTextMap[&stageDepthEditor] = "Depth of the stage (used for remote application and ADM-OSC).";
        helpTextMap[&stageHeightEditor] = "Height of the stage (used for remote application and ADM-OSC).";
        helpTextMap[&stageOriginWidthEditor] = "Origin of the stage in Width (set by default to half of the stage width).";
        helpTextMap[&stageOriginDepthEditor] = "Origin of the stage in Depth (set by default to 0).";
        helpTextMap[&stageOriginHeightEditor] = "Origin of the stage in Height (set by default to 0).";
        helpTextMap[&originFrontButton] = "Set origin to front center of stage (X=width/2, Y=0, Z=0). Typical for frontal stages.";
        helpTextMap[&originCenterGroundButton] = "Set origin to center of stage at ground level (X=width/2, Y=depth/2, Z=0). Typical for a Surround or Central Cylindrical Setup.";
        helpTextMap[&originCenterButton] = "Set origin to center of stage (X=width/2, Y=depth/2, Z=height/2). Typical for a Spherical Dome Setup.";
        helpTextMap[&speedOfSoundEditor] = "Speed of Sound (related to the temperature).";
        helpTextMap[&temperatureEditor] = "Temperature (gives the Speed of Sound).";
        helpTextMap[&masterLevelEditor] = "Master Level (affects all outputs).";
        helpTextMap[&systemLatencyEditor] = "Total latency of the system (Mixing board & Computer) / Specific Input and Output Latency/Delay can be set in the respective Input and Output settings.";
        helpTextMap[&haasEffectEditor] = "Hass Effect to apply to the system. Will take into account the Latency Compensations (System, Input and Output).";
        helpTextMap[&selectProjectFolderButton] = "Select the Location of the Current Project Folder where to store files.";
        helpTextMap[&storeCompleteConfigButton] = "Store Complete Configuration to files (overwrite with confirmation)";
        helpTextMap[&reloadCompleteConfigButton] = "Reload Complete Configuration from files (with confirmation)";
        helpTextMap[&reloadCompleteConfigBackupButton] = "Reload Complete Configuration from backup files (with confirmation)";
        helpTextMap[&storeSystemConfigButton] = "Store System Configuration to file (overwrite with confirmation)";
        helpTextMap[&reloadSystemConfigButton] = "Reload System Configuration from file (with confirmation)";
        helpTextMap[&reloadSystemConfigBackupButton] = "Reload System Configuration from backup file (with confirmation)";
        helpTextMap[&importSystemConfigButton] = "Store System Configuration to file (with file explorer window)";
        helpTextMap[&exportSystemConfigButton] = "Reload System Configuration from file (with file explorer window)";
    }

    void setupMouseListeners()
    {
        // Enable mouse enter/exit events for all components with help text
        for (auto& pair : helpTextMap)
        {
            pair.first->setMouseCursor(juce::MouseCursor::PointingHandCursor);
            pair.first->addMouseListener(this, false);
        }
    }

    void mouseEnter(const juce::MouseEvent& event) override
    {
        if (statusBar == nullptr) return;

        auto* component = event.eventComponent;
        if (helpTextMap.find(component) != helpTextMap.end())
        {
            statusBar->setHelpText(helpTextMap[component]);
        }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (statusBar != nullptr)
            statusBar->clearText();
    }

    //==============================================================================
    // Member variables

    WfsParameters& parameters;
    StatusBar* statusBar = nullptr;
    std::map<juce::Component*, juce::String> helpTextMap;
    bool processingEnabled = false;

    // Show Section
    juce::Label showNameLabel;
    juce::TextEditor showNameEditor;
    juce::Label showLocationLabel;
    juce::TextEditor showLocationEditor;

    // I/O Section
    juce::Label inputChannelsLabel;
    juce::TextEditor inputChannelsEditor;
    juce::Label outputChannelsLabel;
    juce::TextEditor outputChannelsEditor;
    juce::Label reverbChannelsLabel;
    juce::TextEditor reverbChannelsEditor;
    juce::TextButton audioPatchingButton;
    juce::Label algorithmLabel;
    juce::ComboBox algorithmSelector;
    juce::TextButton processingButton;

    // Stage Section
    juce::Label stageWidthLabel;
    juce::TextEditor stageWidthEditor;
    juce::Label stageWidthUnitLabel;
    juce::Label stageDepthLabel;
    juce::TextEditor stageDepthEditor;
    juce::Label stageDepthUnitLabel;
    juce::Label stageHeightLabel;
    juce::TextEditor stageHeightEditor;
    juce::Label stageHeightUnitLabel;
    juce::Label stageOriginWidthLabel;
    juce::TextEditor stageOriginWidthEditor;
    juce::Label stageOriginWidthUnitLabel;
    juce::Label stageOriginDepthLabel;
    juce::TextEditor stageOriginDepthEditor;
    juce::Label stageOriginDepthUnitLabel;
    juce::Label stageOriginHeightLabel;
    juce::TextEditor stageOriginHeightEditor;
    juce::Label stageOriginHeightUnitLabel;
    OriginFrontButton originFrontButton;
    OriginCenterGroundButton originCenterGroundButton;
    OriginCenterButton originCenterButton;
    juce::Label speedOfSoundLabel;
    juce::TextEditor speedOfSoundEditor;
    juce::Label speedOfSoundUnitLabel;
    juce::Label temperatureLabel;
    juce::TextEditor temperatureEditor;
    juce::Label temperatureUnitLabel;

    // Master Section
    juce::Label masterLevelLabel;
    juce::TextEditor masterLevelEditor;
    juce::Label masterLevelUnitLabel;
    juce::Label systemLatencyLabel;
    juce::TextEditor systemLatencyEditor;
    juce::Label systemLatencyUnitLabel;
    juce::Label haasEffectLabel;
    juce::TextEditor haasEffectEditor;
    juce::Label haasEffectUnitLabel;

    // Store/Reload Section
    juce::TextButton selectProjectFolderButton;
    juce::TextButton storeCompleteConfigButton;
    juce::TextButton reloadCompleteConfigButton;
    juce::TextButton reloadCompleteConfigBackupButton;
    juce::TextButton storeSystemConfigButton;
    juce::TextButton reloadSystemConfigButton;
    juce::TextButton reloadSystemConfigBackupButton;
    juce::TextButton importSystemConfigButton;
    juce::TextButton exportSystemConfigButton;
    juce::File projectFolder;

    // Callbacks for notifying MainComponent
    ProcessingCallback onProcessingChanged;
    ChannelCountCallback onChannelCountChanged;
    AudioInterfaceCallback onAudioInterfaceWindowRequested;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SystemConfigTab)
};
