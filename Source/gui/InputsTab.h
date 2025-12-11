#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"
#include "ChannelSelector.h"
#include "SliderUIComponents.h"
#include "DialUIComponents.h"
#include "StatusBar.h"

/**
 * Inputs Tab Component
 * Configuration for input channels (audio objects) with sub-tabs for different parameter groups.
 *
 * Structure:
 * - Header: Channel selector + Name editor (always visible)
 * - Sub-tabs: Input Properties, Position, Sound, Live Source, Effects (more to be added)
 * - Footer: Store/Reload buttons (always visible)
 */
class InputsTab : public juce::Component,
                  private juce::TextEditor::Listener,
                  private juce::ChangeListener,
                  private juce::Label::Listener
{
public:
    InputsTab(WfsParameters& params)
        : parameters(params)
    {
        // ==================== HEADER SECTION ====================
        // Channel Selector
        channelSelector.setNumChannels(64);
        channelSelector.onChannelChanged = [this](int channel) {
            loadChannelParameters(channel);
        };
        addAndMakeVisible(channelSelector);

        // Input Name
        addAndMakeVisible(nameLabel);
        nameLabel.setText("Name:", juce::dontSendNotification);
        nameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(nameEditor);
        nameEditor.addListener(this);

        // Cluster selector
        addAndMakeVisible(clusterLabel);
        clusterLabel.setText("Cluster:", juce::dontSendNotification);
        clusterLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(clusterSelector);
        clusterSelector.addItem("Single", 1);
        for (int i = 1; i <= 10; ++i)
            clusterSelector.addItem("Cluster " + juce::String(i), i + 1);
        clusterSelector.setSelectedId(1, juce::dontSendNotification);

        // ==================== SUB-TABS ====================
        addAndMakeVisible(subTabBar);
        subTabBar.addTab("Input", juce::Colour(0xFF2A2A2A), -1);
        subTabBar.addTab("Position", juce::Colour(0xFF2A2A2A), -1);
        subTabBar.addTab("Sound", juce::Colour(0xFF2A2A2A), -1);
        subTabBar.addTab("Live Source", juce::Colour(0xFF2A2A2A), -1);
        subTabBar.addTab("Hackoustics", juce::Colour(0xFF2A2A2A), -1);
        subTabBar.addTab("L.F.O", juce::Colour(0xFF2A2A2A), -1);
        subTabBar.addTab("AutomOtion", juce::Colour(0xFF2A2A2A), -1);
        subTabBar.addTab("Mutes", juce::Colour(0xFF2A2A2A), -1);
        subTabBar.setCurrentTabIndex(0);
        subTabBar.addChangeListener(static_cast<juce::ChangeListener*>(this));

        // ==================== SETUP SUB-TABS ====================
        setupInputPropertiesTab();
        setupPositionTab();
        setupSoundTab();
        setupLiveSourceTab();
        setupEffectsTab();
        setupLfoTab();
        setupAutomotionTab();
        setupMutesTab();

        // ==================== FOOTER - STORE/RELOAD BUTTONS ====================
        addAndMakeVisible(storeButton);
        storeButton.setButtonText("Store Input Config");
        storeButton.onClick = [this]() { storeInputConfiguration(); };

        addAndMakeVisible(reloadButton);
        reloadButton.setButtonText("Reload Input Config");
        reloadButton.onClick = [this]() { reloadInputConfiguration(); };

        addAndMakeVisible(reloadBackupButton);
        reloadBackupButton.setButtonText("Reload Backup");
        reloadBackupButton.onClick = [this]() { reloadInputConfigBackup(); };

        addAndMakeVisible(importButton);
        importButton.setButtonText("Import");
        importButton.onClick = [this]() { importInputConfiguration(); };

        addAndMakeVisible(exportButton);
        exportButton.setButtonText("Export");
        exportButton.onClick = [this]() { exportInputConfiguration(); };

        // Snapshot management
        addAndMakeVisible(storeSnapshotButton);
        storeSnapshotButton.setButtonText("Store Snapshot");
        storeSnapshotButton.onClick = [this]() { storeNewSnapshot(); };

        addAndMakeVisible(snapshotSelector);
        snapshotSelector.addItem("Select Snapshot...", 1);
        // Snapshots would be populated dynamically

        addAndMakeVisible(reloadSnapshotButton);
        reloadSnapshotButton.setButtonText("Reload Snapshot");
        reloadSnapshotButton.onClick = [this]() { reloadSnapshot(); };

        addAndMakeVisible(updateSnapshotButton);
        updateSnapshotButton.setButtonText("Update Snapshot");
        updateSnapshotButton.onClick = [this]() { updateSnapshot(); };

        addAndMakeVisible(editScopeButton);
        editScopeButton.setButtonText("Edit Scope");
        editScopeButton.onClick = [this]() { editSnapshotScope(); };

        addAndMakeVisible(deleteSnapshotButton);
        deleteSnapshotButton.setButtonText("Delete Snapshot");
        deleteSnapshotButton.onClick = [this]() { deleteSnapshot(); };

        // Load initial channel parameters
        loadChannelParameters(1);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xFF1E1E1E));

        // Header background
        g.setColour(juce::Colour(0xFF252525));
        g.fillRect(0, 0, getWidth(), headerHeight);

        // Footer background
        g.setColour(juce::Colour(0xFF252525));
        g.fillRect(0, getHeight() - footerHeight, getWidth(), footerHeight);

        // Section dividers
        g.setColour(juce::Colour(0xFF404040));
        g.drawLine(0.0f, (float)headerHeight, (float)getWidth(), (float)headerHeight, 1.0f);
        g.drawLine(0.0f, (float)(getHeight() - footerHeight), (float)getWidth(), (float)(getHeight() - footerHeight), 1.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        const int padding = 10;
        const int rowHeight = 30;
        const int spacing = 5;

        // ==================== HEADER ====================
        auto headerArea = bounds.removeFromTop(headerHeight).reduced(padding, padding);

        auto row1 = headerArea.removeFromTop(rowHeight);
        channelSelector.setBounds(row1.removeFromLeft(150));
        row1.removeFromLeft(spacing * 2);
        nameLabel.setBounds(row1.removeFromLeft(50));
        nameEditor.setBounds(row1.removeFromLeft(200));
        row1.removeFromLeft(spacing * 4);
        clusterLabel.setBounds(row1.removeFromLeft(60));
        clusterSelector.setBounds(row1.removeFromLeft(100));

        // ==================== FOOTER ==================== (matching Output tab style)
        auto footerArea = bounds.removeFromBottom(footerHeight).reduced(padding, padding);
        const int buttonRowHeight = 30;  // Same as Output tab buttons

        // First row - Snapshot buttons (on top) - 6 items with selector being 1.5x width
        auto footerRow1 = footerArea.removeFromTop(buttonRowHeight);
        const int snapButtonWidth = (footerRow1.getWidth() - spacing * 5) / 7;  // 6.5 units total
        const int selectorWidth = snapButtonWidth * 3 / 2;  // 1.5x width for selector

        storeSnapshotButton.setBounds(footerRow1.removeFromLeft(snapButtonWidth));
        footerRow1.removeFromLeft(spacing);
        snapshotSelector.setBounds(footerRow1.removeFromLeft(selectorWidth));
        footerRow1.removeFromLeft(spacing);
        reloadSnapshotButton.setBounds(footerRow1.removeFromLeft(snapButtonWidth));
        footerRow1.removeFromLeft(spacing);
        updateSnapshotButton.setBounds(footerRow1.removeFromLeft(snapButtonWidth));
        footerRow1.removeFromLeft(spacing);
        editScopeButton.setBounds(footerRow1.removeFromLeft(snapButtonWidth));
        footerRow1.removeFromLeft(spacing);
        deleteSnapshotButton.setBounds(footerRow1);  // Take remaining width

        footerArea.removeFromTop(padding);  // Same spacing as padding for consistency

        // Second row - Config buttons (below) - 5 equal-width buttons
        auto footerRow2 = footerArea.removeFromTop(buttonRowHeight);
        const int configButtonWidth = (footerRow2.getWidth() - spacing * 4) / 5;

        storeButton.setBounds(footerRow2.removeFromLeft(configButtonWidth));
        footerRow2.removeFromLeft(spacing);
        reloadButton.setBounds(footerRow2.removeFromLeft(configButtonWidth));
        footerRow2.removeFromLeft(spacing);
        reloadBackupButton.setBounds(footerRow2.removeFromLeft(configButtonWidth));
        footerRow2.removeFromLeft(spacing);
        importButton.setBounds(footerRow2.removeFromLeft(configButtonWidth));
        footerRow2.removeFromLeft(spacing);
        exportButton.setBounds(footerRow2);

        // ==================== SUB-TABS AREA ====================
        auto contentArea = bounds.reduced(padding, 0);
        auto tabBarArea = contentArea.removeFromTop(32);
        subTabBar.setBounds(tabBarArea);

        subTabContentArea = contentArea.reduced(0, padding);
        layoutCurrentSubTab();
    }

    void setStatusBar(StatusBar* bar)
    {
        statusBar = bar;
        setupHelpText();
        setupOscMethods();
        setupMouseListeners();
    }

private:
    // ==================== CHANGE LISTENER ====================

    void changeListenerCallback(juce::ChangeBroadcaster*) override
    {
        layoutCurrentSubTab();
        repaint();
    }

    // ==================== SETUP METHODS ====================

    void setupInputPropertiesTab()
    {
        // Attenuation slider (-92 to 0 dB)
        addAndMakeVisible(attenuationLabel);
        attenuationLabel.setText("Attenuation:", juce::dontSendNotification);
        attenuationLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        attenuationSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFFFF5722));
        attenuationSlider.onValueChanged = [this](float v) {
            float dB = 20.0f * std::log10(std::pow(10.0f, -92.0f / 20.0f) +
                       ((1.0f - std::pow(10.0f, -92.0f / 20.0f)) * v * v));
            attenuationValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
        };
        addAndMakeVisible(attenuationSlider);

        addAndMakeVisible(attenuationValueLabel);
        attenuationValueLabel.setText("0.0 dB", juce::dontSendNotification);
        attenuationValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel(attenuationValueLabel);

        // Delay/Latency slider (-100 to 100 ms)
        addAndMakeVisible(delayLatencyLabel);
        delayLatencyLabel.setText("Delay/Latency:", juce::dontSendNotification);
        delayLatencyLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        delayLatencySlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFF4CAF50));
        delayLatencySlider.onValueChanged = [this](float v) {
            // Slider range is -1 to 1, map to -100ms to 100ms
            float ms = v * 100.0f;
            juce::String label = (ms < 0) ? "Latency: " : "Delay: ";
            delayLatencyValueLabel.setText(label + juce::String(std::abs(ms), 1) + " ms", juce::dontSendNotification);
        };
        addAndMakeVisible(delayLatencySlider);

        addAndMakeVisible(delayLatencyValueLabel);
        delayLatencyValueLabel.setText("Delay: 0.0 ms", juce::dontSendNotification);
        delayLatencyValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel(delayLatencyValueLabel);

        // Minimal Latency button
        addAndMakeVisible(minimalLatencyButton);
        minimalLatencyButton.setButtonText("Acoustic Precedence");
        minimalLatencyButton.setClickingTogglesState(true);
        minimalLatencyButton.onClick = [this]() {
            bool minLat = minimalLatencyButton.getToggleState();
            minimalLatencyButton.setButtonText(minLat ? "Minimal Latency" : "Acoustic Precedence");
        };
    }

    void setupPositionTab()
    {
        // Position X
        addAndMakeVisible(posXLabel);
        posXLabel.setText("Position X:", juce::dontSendNotification);
        posXLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(posXEditor);
        posXEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(posXEditor, true, true);
        addAndMakeVisible(posXUnitLabel);
        posXUnitLabel.setText("m", juce::dontSendNotification);
        posXUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        // Position Y
        addAndMakeVisible(posYLabel);
        posYLabel.setText("Position Y:", juce::dontSendNotification);
        posYLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(posYEditor);
        posYEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(posYEditor, true, true);
        addAndMakeVisible(posYUnitLabel);
        posYUnitLabel.setText("m", juce::dontSendNotification);
        posYUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        // Position Z
        addAndMakeVisible(posZLabel);
        posZLabel.setText("Position Z:", juce::dontSendNotification);
        posZLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(posZEditor);
        posZEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(posZEditor, true, true);
        addAndMakeVisible(posZUnitLabel);
        posZUnitLabel.setText("m", juce::dontSendNotification);
        posZUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        // Offset X
        addAndMakeVisible(offsetXLabel);
        offsetXLabel.setText("Offset X:", juce::dontSendNotification);
        offsetXLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(offsetXEditor);
        offsetXEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(offsetXEditor, true, true);
        addAndMakeVisible(offsetXUnitLabel);
        offsetXUnitLabel.setText("m", juce::dontSendNotification);
        offsetXUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        // Offset Y
        addAndMakeVisible(offsetYLabel);
        offsetYLabel.setText("Offset Y:", juce::dontSendNotification);
        offsetYLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(offsetYEditor);
        offsetYEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(offsetYEditor, true, true);
        addAndMakeVisible(offsetYUnitLabel);
        offsetYUnitLabel.setText("m", juce::dontSendNotification);
        offsetYUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        // Offset Z
        addAndMakeVisible(offsetZLabel);
        offsetZLabel.setText("Offset Z:", juce::dontSendNotification);
        offsetZLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(offsetZEditor);
        offsetZEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(offsetZEditor, true, true);
        addAndMakeVisible(offsetZUnitLabel);
        offsetZUnitLabel.setText("m", juce::dontSendNotification);
        offsetZUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        // Constraint buttons
        addAndMakeVisible(constraintXButton);
        constraintXButton.setButtonText("Constraint X: ON");
        constraintXButton.setClickingTogglesState(true);
        constraintXButton.setToggleState(true, juce::dontSendNotification);
        constraintXButton.onClick = [this]() {
            constraintXButton.setButtonText(constraintXButton.getToggleState() ? "Constraint X: ON" : "Constraint X: OFF");
        };

        addAndMakeVisible(constraintYButton);
        constraintYButton.setButtonText("Constraint Y: ON");
        constraintYButton.setClickingTogglesState(true);
        constraintYButton.setToggleState(true, juce::dontSendNotification);
        constraintYButton.onClick = [this]() {
            constraintYButton.setButtonText(constraintYButton.getToggleState() ? "Constraint Y: ON" : "Constraint Y: OFF");
        };

        addAndMakeVisible(constraintZButton);
        constraintZButton.setButtonText("Constraint Z: ON");
        constraintZButton.setClickingTogglesState(true);
        constraintZButton.setToggleState(true, juce::dontSendNotification);
        constraintZButton.onClick = [this]() {
            constraintZButton.setButtonText(constraintZButton.getToggleState() ? "Constraint Z: ON" : "Constraint Z: OFF");
        };

        // Flip buttons
        addAndMakeVisible(flipXButton);
        flipXButton.setButtonText("Flip X: OFF");
        flipXButton.setClickingTogglesState(true);
        flipXButton.onClick = [this]() {
            flipXButton.setButtonText(flipXButton.getToggleState() ? "Flip X: ON" : "Flip X: OFF");
        };

        addAndMakeVisible(flipYButton);
        flipYButton.setButtonText("Flip Y: OFF");
        flipYButton.setClickingTogglesState(true);
        flipYButton.onClick = [this]() {
            flipYButton.setButtonText(flipYButton.getToggleState() ? "Flip Y: ON" : "Flip Y: OFF");
        };

        addAndMakeVisible(flipZButton);
        flipZButton.setButtonText("Flip Z: OFF");
        flipZButton.setClickingTogglesState(true);
        flipZButton.onClick = [this]() {
            flipZButton.setButtonText(flipZButton.getToggleState() ? "Flip Z: ON" : "Flip Z: OFF");
        };

        // Tracking
        addAndMakeVisible(trackingActiveButton);
        trackingActiveButton.setButtonText("Tracking: OFF");
        trackingActiveButton.setClickingTogglesState(true);
        trackingActiveButton.onClick = [this]() {
            trackingActiveButton.setButtonText(trackingActiveButton.getToggleState() ? "Tracking: ON" : "Tracking: OFF");
        };

        // Tracking ID selector (1-32)
        addAndMakeVisible(trackingIdLabel);
        trackingIdLabel.setText("Tracking ID:", juce::dontSendNotification);
        trackingIdLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(trackingIdSelector);
        for (int i = 1; i <= 32; ++i)
            trackingIdSelector.addItem(juce::String(i), i);
        trackingIdSelector.setSelectedId(1, juce::dontSendNotification);

        // Tracking Smoothing dial (0-100%)
        addAndMakeVisible(trackingSmoothLabel);
        trackingSmoothLabel.setText("Tracking Smooth:", juce::dontSendNotification);
        trackingSmoothLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        trackingSmoothDial.setColours(juce::Colours::black, juce::Colour(0xFF00BCD4), juce::Colours::grey);
        trackingSmoothDial.setValue(1.0f);  // Default 100%
        trackingSmoothDial.onValueChanged = [this](float v) {
            trackingSmoothValueLabel.setText(juce::String(static_cast<int>(v * 100.0f)) + " %", juce::dontSendNotification);
        };
        addAndMakeVisible(trackingSmoothDial);
        addAndMakeVisible(trackingSmoothValueLabel);
        trackingSmoothValueLabel.setText("100 %", juce::dontSendNotification);
        trackingSmoothValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        trackingSmoothValueLabel.setJustificationType(juce::Justification::centred);
        setupEditableValueLabel(trackingSmoothValueLabel);

        // Max Speed
        addAndMakeVisible(maxSpeedActiveButton);
        maxSpeedActiveButton.setButtonText("Max Speed: OFF");
        maxSpeedActiveButton.setClickingTogglesState(true);
        maxSpeedActiveButton.onClick = [this]() {
            maxSpeedActiveButton.setButtonText(maxSpeedActiveButton.getToggleState() ? "Max Speed: ON" : "Max Speed: OFF");
        };

        // Max Speed dial (0.01-20.0 m/s)
        addAndMakeVisible(maxSpeedLabel);
        maxSpeedLabel.setText("Max Speed:", juce::dontSendNotification);
        maxSpeedLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        maxSpeedDial.setColours(juce::Colours::black, juce::Colour(0xFFFF9800), juce::Colours::grey);
        maxSpeedDial.onValueChanged = [this](float v) {
            float speed = v * 19.99f + 0.01f;
            maxSpeedValueLabel.setText(juce::String(speed, 2) + " m/s", juce::dontSendNotification);
        };
        addAndMakeVisible(maxSpeedDial);
        addAndMakeVisible(maxSpeedValueLabel);
        maxSpeedValueLabel.setText("1.00 m/s", juce::dontSendNotification);
        maxSpeedValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        maxSpeedValueLabel.setJustificationType(juce::Justification::centred);
        setupEditableValueLabel(maxSpeedValueLabel);

        // Height Factor dial
        addAndMakeVisible(heightFactorLabel);
        heightFactorLabel.setText("Height Factor:", juce::dontSendNotification);
        heightFactorLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        heightFactorDial.setColours(juce::Colours::black, juce::Colour(0xFF4CAF50), juce::Colours::grey);
        heightFactorDial.onValueChanged = [this](float v) {
            heightFactorValueLabel.setText(juce::String(static_cast<int>(v * 100.0f)) + " %", juce::dontSendNotification);
        };
        addAndMakeVisible(heightFactorDial);
        addAndMakeVisible(heightFactorValueLabel);
        heightFactorValueLabel.setText("0 %", juce::dontSendNotification);
        heightFactorValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        heightFactorValueLabel.setJustificationType(juce::Justification::centred);
        setupEditableValueLabel(heightFactorValueLabel);
    }

    void setupSoundTab()
    {
        // Attenuation Law button
        addAndMakeVisible(attenuationLawButton);
        attenuationLawButton.setButtonText("Log");
        attenuationLawButton.setClickingTogglesState(true);
        attenuationLawButton.onClick = [this]() {
            bool is1OverD = attenuationLawButton.getToggleState();
            attenuationLawButton.setButtonText(is1OverD ? "1/d" : "Log");
            // Show/hide Distance Atten vs Distance Ratio based on law
            distanceAttenLabel.setVisible(!is1OverD && subTabBar.getCurrentTabIndex() == 2);
            distanceAttenDial.setVisible(!is1OverD && subTabBar.getCurrentTabIndex() == 2);
            distanceAttenValueLabel.setVisible(!is1OverD && subTabBar.getCurrentTabIndex() == 2);
            distanceRatioLabel.setVisible(is1OverD && subTabBar.getCurrentTabIndex() == 2);
            distanceRatioDial.setVisible(is1OverD && subTabBar.getCurrentTabIndex() == 2);
            distanceRatioValueLabel.setVisible(is1OverD && subTabBar.getCurrentTabIndex() == 2);
        };

        // Distance Attenuation dial (visible when attenuationLaw == Log)
        addAndMakeVisible(distanceAttenLabel);
        distanceAttenLabel.setText("Distance Atten:", juce::dontSendNotification);
        distanceAttenLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        distanceAttenDial.setColours(juce::Colours::black, juce::Colour(0xFF9C27B0), juce::Colours::grey);
        distanceAttenDial.onValueChanged = [this](float v) {
            float dBm = (v * 6.0f) - 6.0f;
            distanceAttenValueLabel.setText(juce::String(dBm, 1) + " dB/m", juce::dontSendNotification);
        };
        addAndMakeVisible(distanceAttenDial);
        addAndMakeVisible(distanceAttenValueLabel);
        distanceAttenValueLabel.setText("-0.7 dB/m", juce::dontSendNotification);
        distanceAttenValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        distanceAttenValueLabel.setJustificationType(juce::Justification::centred);
        setupEditableValueLabel(distanceAttenValueLabel);

        // Distance Ratio dial (visible when attenuationLaw == 1/d)
        addAndMakeVisible(distanceRatioLabel);
        distanceRatioLabel.setText("Distance Ratio:", juce::dontSendNotification);
        distanceRatioLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        distanceRatioDial.setColours(juce::Colours::black, juce::Colour(0xFF9C27B0), juce::Colours::grey);
        distanceRatioDial.onValueChanged = [this](float v) {
            // Formula: pow(10.0,(x*2.0)-1.0) maps 0-1 to 0.1-10.0
            float ratio = std::pow(10.0f, (v * 2.0f) - 1.0f);
            distanceRatioValueLabel.setText(juce::String(ratio, 2) + "x", juce::dontSendNotification);
        };
        distanceRatioDial.setValue(0.5f);  // Default 1.0x
        addAndMakeVisible(distanceRatioDial);
        addAndMakeVisible(distanceRatioValueLabel);
        distanceRatioValueLabel.setText("1.00x", juce::dontSendNotification);
        distanceRatioValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        distanceRatioValueLabel.setJustificationType(juce::Justification::centred);
        setupEditableValueLabel(distanceRatioValueLabel);
        // Initially hidden (Log is default)
        distanceRatioLabel.setVisible(false);
        distanceRatioDial.setVisible(false);
        distanceRatioValueLabel.setVisible(false);

        // Common Attenuation dial
        addAndMakeVisible(commonAttenLabel);
        commonAttenLabel.setText("Common Atten:", juce::dontSendNotification);
        commonAttenLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        commonAttenDial.setColours(juce::Colours::black, juce::Colour(0xFF2196F3), juce::Colours::grey);
        commonAttenDial.setValue(1.0f);
        commonAttenDial.onValueChanged = [this](float v) {
            commonAttenValueLabel.setText(juce::String(static_cast<int>(v * 100.0f)) + " %", juce::dontSendNotification);
        };
        addAndMakeVisible(commonAttenDial);
        addAndMakeVisible(commonAttenValueLabel);
        commonAttenValueLabel.setText("100 %", juce::dontSendNotification);
        commonAttenValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        commonAttenValueLabel.setJustificationType(juce::Justification::centred);
        setupEditableValueLabel(commonAttenValueLabel);

        // Directivity slider
        addAndMakeVisible(directivityLabel);
        directivityLabel.setText("Directivity:", juce::dontSendNotification);
        directivityLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        directivitySlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFF00BCD4));
        directivitySlider.setValue(1.0f);
        directivitySlider.onValueChanged = [this](float v) {
            int degrees = static_cast<int>((v * 358.0f) + 2.0f);
            directivityValueLabel.setText(juce::String(degrees) + juce::String::fromUTF8("°"), juce::dontSendNotification);
        };
        addAndMakeVisible(directivitySlider);
        addAndMakeVisible(directivityValueLabel);
        directivityValueLabel.setText(juce::String::fromUTF8("360°"), juce::dontSendNotification);
        directivityValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel(directivityValueLabel);

        // Rotation dial
        addAndMakeVisible(rotationLabel);
        rotationLabel.setText("Rotation:", juce::dontSendNotification);
        rotationLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        rotationDial.setColours(juce::Colours::black, juce::Colours::white, juce::Colours::grey);
        rotationDial.onAngleChanged = [this](float angle) {
            rotationValueLabel.setText(juce::String(static_cast<int>(angle)) + juce::String::fromUTF8("°"), juce::dontSendNotification);
        };
        addAndMakeVisible(rotationDial);
        addAndMakeVisible(rotationValueLabel);
        rotationValueLabel.setText(juce::String::fromUTF8("0°"), juce::dontSendNotification);
        rotationValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        rotationValueLabel.setJustificationType(juce::Justification::centred);
        setupEditableValueLabel(rotationValueLabel);

        // Tilt slider
        addAndMakeVisible(tiltLabel);
        tiltLabel.setText("Tilt:", juce::dontSendNotification);
        tiltLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        tiltSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFF2196F3));
        tiltSlider.onValueChanged = [this](float v) {
            // Slider range is -1 to 1, map to -90° to 90°
            int degrees = static_cast<int>(v * 90.0f);
            tiltValueLabel.setText(juce::String(degrees) + juce::String::fromUTF8("°"), juce::dontSendNotification);
        };
        addAndMakeVisible(tiltSlider);
        addAndMakeVisible(tiltValueLabel);
        tiltValueLabel.setText(juce::String::fromUTF8("0°"), juce::dontSendNotification);
        tiltValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel(tiltValueLabel);

        // HF Shelf slider
        addAndMakeVisible(hfShelfLabel);
        hfShelfLabel.setText("HF Shelf:", juce::dontSendNotification);
        hfShelfLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        hfShelfSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFFFF9800));
        hfShelfSlider.onValueChanged = [this](float v) {
            float dB = 20.0f * std::log10(std::pow(10.0f, -24.0f / 20.0f) +
                       ((1.0f - std::pow(10.0f, -24.0f / 20.0f)) * v * v));
            hfShelfValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
        };
        addAndMakeVisible(hfShelfSlider);
        addAndMakeVisible(hfShelfValueLabel);
        hfShelfValueLabel.setText("-6.0 dB", juce::dontSendNotification);
        hfShelfValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel(hfShelfValueLabel);
    }

    void setupLiveSourceTab()
    {
        // Live Source Active button
        addAndMakeVisible(lsActiveButton);
        lsActiveButton.setButtonText("Live Source Tamer: OFF");
        lsActiveButton.setClickingTogglesState(true);
        lsActiveButton.onClick = [this]() {
            lsActiveButton.setButtonText(lsActiveButton.getToggleState() ? "Live Source Tamer: ON" : "Live Source Tamer: OFF");
        };

        // Radius slider
        addAndMakeVisible(lsRadiusLabel);
        lsRadiusLabel.setText("Radius:", juce::dontSendNotification);
        lsRadiusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lsRadiusSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFF00BCD4));
        lsRadiusSlider.setValue(0.06f);  // 3m
        lsRadiusSlider.onValueChanged = [this](float v) {
            float meters = v * 50.0f;
            lsRadiusValueLabel.setText(juce::String(meters, 1) + " m", juce::dontSendNotification);
        };
        addAndMakeVisible(lsRadiusSlider);
        addAndMakeVisible(lsRadiusValueLabel);
        lsRadiusValueLabel.setText("3.0 m", juce::dontSendNotification);
        lsRadiusValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel(lsRadiusValueLabel);

        // Shape selector
        addAndMakeVisible(lsShapeLabel);
        lsShapeLabel.setText("Shape:", juce::dontSendNotification);
        lsShapeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(lsShapeSelector);
        lsShapeSelector.addItem("linear", 1);
        lsShapeSelector.addItem("log", 2);
        lsShapeSelector.addItem("square d", 3);
        lsShapeSelector.addItem("sine", 4);
        lsShapeSelector.setSelectedId(1, juce::dontSendNotification);

        // Attenuation slider
        addAndMakeVisible(lsAttenuationLabel);
        lsAttenuationLabel.setText("Attenuation:", juce::dontSendNotification);
        lsAttenuationLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lsAttenuationSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFFFF5722));
        lsAttenuationSlider.onValueChanged = [this](float v) {
            float dB = 20.0f * std::log10(std::pow(10.0f, -24.0f / 20.0f) +
                       ((1.0f - std::pow(10.0f, -24.0f / 20.0f)) * v * v));
            lsAttenuationValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
        };
        addAndMakeVisible(lsAttenuationSlider);
        addAndMakeVisible(lsAttenuationValueLabel);
        lsAttenuationValueLabel.setText("0.0 dB", juce::dontSendNotification);
        lsAttenuationValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel(lsAttenuationValueLabel);

        // Peak Threshold slider
        addAndMakeVisible(lsPeakThresholdLabel);
        lsPeakThresholdLabel.setText("Peak Threshold:", juce::dontSendNotification);
        lsPeakThresholdLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lsPeakThresholdSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFFE91E63));
        lsPeakThresholdSlider.onValueChanged = [this](float v) {
            float dB = 20.0f * std::log10(std::pow(10.0f, -48.0f / 20.0f) +
                       ((1.0f - std::pow(10.0f, -48.0f / 20.0f)) * v * v));
            lsPeakThresholdValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
        };
        addAndMakeVisible(lsPeakThresholdSlider);
        addAndMakeVisible(lsPeakThresholdValueLabel);
        lsPeakThresholdValueLabel.setText("-20.0 dB", juce::dontSendNotification);
        lsPeakThresholdValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel(lsPeakThresholdValueLabel);

        // Peak Ratio dial
        addAndMakeVisible(lsPeakRatioLabel);
        lsPeakRatioLabel.setText("Peak Ratio:", juce::dontSendNotification);
        lsPeakRatioLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lsPeakRatioDial.setColours(juce::Colours::black, juce::Colour(0xFFE91E63), juce::Colours::grey);
        lsPeakRatioDial.onValueChanged = [this](float v) {
            float ratio = (v * 9.0f) + 1.0f;
            lsPeakRatioValueLabel.setText(juce::String(ratio, 1) + ":1", juce::dontSendNotification);
        };
        addAndMakeVisible(lsPeakRatioDial);
        addAndMakeVisible(lsPeakRatioValueLabel);
        lsPeakRatioValueLabel.setText("2.0:1", juce::dontSendNotification);
        lsPeakRatioValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lsPeakRatioValueLabel.setJustificationType(juce::Justification::centred);
        setupEditableValueLabel(lsPeakRatioValueLabel);

        // Slow Threshold slider
        addAndMakeVisible(lsSlowThresholdLabel);
        lsSlowThresholdLabel.setText("Slow Threshold:", juce::dontSendNotification);
        lsSlowThresholdLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lsSlowThresholdSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFF9C27B0));
        lsSlowThresholdSlider.onValueChanged = [this](float v) {
            float dB = 20.0f * std::log10(std::pow(10.0f, -48.0f / 20.0f) +
                       ((1.0f - std::pow(10.0f, -48.0f / 20.0f)) * v * v));
            lsSlowThresholdValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
        };
        addAndMakeVisible(lsSlowThresholdSlider);
        addAndMakeVisible(lsSlowThresholdValueLabel);
        lsSlowThresholdValueLabel.setText("-20.0 dB", juce::dontSendNotification);
        lsSlowThresholdValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel(lsSlowThresholdValueLabel);

        // Slow Ratio dial
        addAndMakeVisible(lsSlowRatioLabel);
        lsSlowRatioLabel.setText("Slow Ratio:", juce::dontSendNotification);
        lsSlowRatioLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lsSlowRatioDial.setColours(juce::Colours::black, juce::Colour(0xFF9C27B0), juce::Colours::grey);
        lsSlowRatioDial.onValueChanged = [this](float v) {
            float ratio = (v * 9.0f) + 1.0f;
            lsSlowRatioValueLabel.setText(juce::String(ratio, 1) + ":1", juce::dontSendNotification);
        };
        addAndMakeVisible(lsSlowRatioDial);
        addAndMakeVisible(lsSlowRatioValueLabel);
        lsSlowRatioValueLabel.setText("2.0:1", juce::dontSendNotification);
        lsSlowRatioValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lsSlowRatioValueLabel.setJustificationType(juce::Justification::centred);
        setupEditableValueLabel(lsSlowRatioValueLabel);
    }

    void setupEffectsTab()
    {
        // Floor Reflections Active
        addAndMakeVisible(frActiveButton);
        frActiveButton.setButtonText("Floor Reflections: OFF");
        frActiveButton.setClickingTogglesState(true);
        frActiveButton.onClick = [this]() {
            frActiveButton.setButtonText(frActiveButton.getToggleState() ? "Floor Reflections: ON" : "Floor Reflections: OFF");
        };

        // FR Attenuation slider
        addAndMakeVisible(frAttenuationLabel);
        frAttenuationLabel.setText("FR Attenuation:", juce::dontSendNotification);
        frAttenuationLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        frAttenuationSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFF795548));
        frAttenuationSlider.onValueChanged = [this](float v) {
            float dB = 20.0f * std::log10(std::pow(10.0f, -60.0f / 20.0f) +
                       ((1.0f - std::pow(10.0f, -60.0f / 20.0f)) * v * v));
            frAttenuationValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
        };
        addAndMakeVisible(frAttenuationSlider);
        addAndMakeVisible(frAttenuationValueLabel);
        frAttenuationValueLabel.setText("-3.0 dB", juce::dontSendNotification);
        frAttenuationValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel(frAttenuationValueLabel);

        // FR Diffusion dial
        addAndMakeVisible(frDiffusionLabel);
        frDiffusionLabel.setText("FR Diffusion:", juce::dontSendNotification);
        frDiffusionLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        frDiffusionDial.setColours(juce::Colours::black, juce::Colour(0xFF795548), juce::Colours::grey);
        frDiffusionDial.setValue(0.2f);
        frDiffusionDial.onValueChanged = [this](float v) {
            frDiffusionValueLabel.setText(juce::String(static_cast<int>(v * 100.0f)) + " %", juce::dontSendNotification);
        };
        addAndMakeVisible(frDiffusionDial);
        addAndMakeVisible(frDiffusionValueLabel);
        frDiffusionValueLabel.setText("20 %", juce::dontSendNotification);
        frDiffusionValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        frDiffusionValueLabel.setJustificationType(juce::Justification::centred);
        setupEditableValueLabel(frDiffusionValueLabel);

        // FR Low Cut Active
        addAndMakeVisible(frLowCutActiveButton);
        frLowCutActiveButton.setButtonText("Low Cut: ON");
        frLowCutActiveButton.setClickingTogglesState(true);
        frLowCutActiveButton.setToggleState(true, juce::dontSendNotification);
        frLowCutActiveButton.onClick = [this]() {
            frLowCutActiveButton.setButtonText(frLowCutActiveButton.getToggleState() ? "Low Cut: ON" : "Low Cut: OFF");
        };

        // FR Low Cut Frequency slider (20-20000 Hz)
        addAndMakeVisible(frLowCutFreqLabel);
        frLowCutFreqLabel.setText("Low Cut Freq:", juce::dontSendNotification);
        frLowCutFreqLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        frLowCutFreqSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFF607D8B));
        frLowCutFreqSlider.onValueChanged = [this](float v) {
            // Formula: 20*pow(10,4*x) maps 0-1 to 20-20000 Hz
            int freq = static_cast<int>(20.0f * std::pow(10.0f, 3.0f * v));
            frLowCutFreqValueLabel.setText(juce::String(freq) + " Hz", juce::dontSendNotification);
        };
        addAndMakeVisible(frLowCutFreqSlider);
        addAndMakeVisible(frLowCutFreqValueLabel);
        frLowCutFreqValueLabel.setText("100 Hz", juce::dontSendNotification);
        frLowCutFreqValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel(frLowCutFreqValueLabel);

        // FR High Shelf Active
        addAndMakeVisible(frHighShelfActiveButton);
        frHighShelfActiveButton.setButtonText("High Shelf: ON");
        frHighShelfActiveButton.setClickingTogglesState(true);
        frHighShelfActiveButton.setToggleState(true, juce::dontSendNotification);
        frHighShelfActiveButton.onClick = [this]() {
            frHighShelfActiveButton.setButtonText(frHighShelfActiveButton.getToggleState() ? "High Shelf: ON" : "High Shelf: OFF");
        };

        // FR High Shelf Frequency slider (20-20000 Hz)
        addAndMakeVisible(frHighShelfFreqLabel);
        frHighShelfFreqLabel.setText("HS Freq:", juce::dontSendNotification);
        frHighShelfFreqLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        frHighShelfFreqSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFF607D8B));
        frHighShelfFreqSlider.onValueChanged = [this](float v) {
            int freq = static_cast<int>(20.0f * std::pow(10.0f, 3.0f * v));
            frHighShelfFreqValueLabel.setText(juce::String(freq) + " Hz", juce::dontSendNotification);
        };
        addAndMakeVisible(frHighShelfFreqSlider);
        addAndMakeVisible(frHighShelfFreqValueLabel);
        frHighShelfFreqValueLabel.setText("3000 Hz", juce::dontSendNotification);
        frHighShelfFreqValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel(frHighShelfFreqValueLabel);

        // FR High Shelf Gain slider (-24 to 0 dB)
        addAndMakeVisible(frHighShelfGainLabel);
        frHighShelfGainLabel.setText("HS Gain:", juce::dontSendNotification);
        frHighShelfGainLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        frHighShelfGainSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFF607D8B));
        frHighShelfGainSlider.onValueChanged = [this](float v) {
            float dB = 20.0f * std::log10(std::pow(10.0f, -24.0f / 20.0f) +
                       ((1.0f - std::pow(10.0f, -24.0f / 20.0f)) * v * v));
            frHighShelfGainValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
        };
        addAndMakeVisible(frHighShelfGainSlider);
        addAndMakeVisible(frHighShelfGainValueLabel);
        frHighShelfGainValueLabel.setText("-2.0 dB", juce::dontSendNotification);
        frHighShelfGainValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel(frHighShelfGainValueLabel);

        // FR High Shelf Slope slider (0.1-0.9)
        addAndMakeVisible(frHighShelfSlopeLabel);
        frHighShelfSlopeLabel.setText("HS Slope:", juce::dontSendNotification);
        frHighShelfSlopeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        frHighShelfSlopeSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFF607D8B));
        frHighShelfSlopeSlider.onValueChanged = [this](float v) {
            // Formula: (x*0.8)+0.1 maps 0-1 to 0.1-0.9
            float slope = (v * 0.8f) + 0.1f;
            frHighShelfSlopeValueLabel.setText(juce::String(slope, 2), juce::dontSendNotification);
        };
        addAndMakeVisible(frHighShelfSlopeSlider);
        addAndMakeVisible(frHighShelfSlopeValueLabel);
        frHighShelfSlopeValueLabel.setText("0.40", juce::dontSendNotification);
        frHighShelfSlopeValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel(frHighShelfSlopeValueLabel);

        // Jitter slider
        addAndMakeVisible(jitterLabel);
        jitterLabel.setText("Jitter:", juce::dontSendNotification);
        jitterLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        jitterSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFFCDDC39));
        jitterSlider.onValueChanged = [this](float v) {
            float meters = 10.0f * v * v;
            jitterValueLabel.setText(juce::String(meters, 2) + " m", juce::dontSendNotification);
        };
        addAndMakeVisible(jitterSlider);
        addAndMakeVisible(jitterValueLabel);
        jitterValueLabel.setText("0.00 m", juce::dontSendNotification);
        jitterValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel(jitterValueLabel);
    }

    void setupLfoTab()
    {
        // LFO Active button
        addAndMakeVisible(lfoActiveButton);
        lfoActiveButton.setButtonText("L.F.O: OFF");
        lfoActiveButton.setClickingTogglesState(true);
        lfoActiveButton.onClick = [this]() {
            lfoActiveButton.setButtonText(lfoActiveButton.getToggleState() ? "L.F.O: ON" : "L.F.O: OFF");
        };

        // Period dial (0.01-100.0 s) - Formula: pow(10.0,sqrt(x)*4.0-2.0)
        addAndMakeVisible(lfoPeriodLabel);
        lfoPeriodLabel.setText("Period:", juce::dontSendNotification);
        lfoPeriodLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lfoPeriodDial.setColours(juce::Colours::black, juce::Colour(0xFF00BCD4), juce::Colours::grey);
        lfoPeriodDial.onValueChanged = [this](float v) {
            float period = std::pow(10.0f, std::sqrt(v) * 4.0f - 2.0f);
            lfoPeriodValueLabel.setText(juce::String(period, 2) + " s", juce::dontSendNotification);
        };
        addAndMakeVisible(lfoPeriodDial);
        addAndMakeVisible(lfoPeriodValueLabel);
        lfoPeriodValueLabel.setText("5.00 s", juce::dontSendNotification);
        lfoPeriodValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lfoPeriodValueLabel.setJustificationType(juce::Justification::centred);
        setupEditableValueLabel(lfoPeriodValueLabel);

        // Main Phase dial (0-360°) - uses WfsRotationDial
        addAndMakeVisible(lfoPhaseLabel);
        lfoPhaseLabel.setText("Phase:", juce::dontSendNotification);
        lfoPhaseLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lfoPhaseDial.setColours(juce::Colours::black, juce::Colour(0xFF4CAF50), juce::Colours::grey);
        lfoPhaseDial.onAngleChanged = [this](float angle) {
            int degrees = static_cast<int>(angle);
            if (degrees < 0) degrees += 360;
            lfoPhaseValueLabel.setText(juce::String(degrees) + juce::String::fromUTF8("°"), juce::dontSendNotification);
        };
        addAndMakeVisible(lfoPhaseDial);
        addAndMakeVisible(lfoPhaseValueLabel);
        lfoPhaseValueLabel.setText(juce::String::fromUTF8("0°"), juce::dontSendNotification);
        lfoPhaseValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lfoPhaseValueLabel.setJustificationType(juce::Justification::centred);
        setupEditableValueLabel(lfoPhaseValueLabel);

        // Shape X/Y/Z dropdowns
        juce::StringArray lfoShapes = {"OFF", "sine", "square", "sawtooth", "triangle", "keystone", "log", "exp", "random"};

        addAndMakeVisible(lfoShapeXLabel);
        lfoShapeXLabel.setText("Shape X:", juce::dontSendNotification);
        lfoShapeXLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(lfoShapeXSelector);
        for (int i = 0; i < lfoShapes.size(); ++i)
            lfoShapeXSelector.addItem(lfoShapes[i], i + 1);
        lfoShapeXSelector.setSelectedId(1, juce::dontSendNotification);

        addAndMakeVisible(lfoShapeYLabel);
        lfoShapeYLabel.setText("Shape Y:", juce::dontSendNotification);
        lfoShapeYLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(lfoShapeYSelector);
        for (int i = 0; i < lfoShapes.size(); ++i)
            lfoShapeYSelector.addItem(lfoShapes[i], i + 1);
        lfoShapeYSelector.setSelectedId(1, juce::dontSendNotification);

        addAndMakeVisible(lfoShapeZLabel);
        lfoShapeZLabel.setText("Shape Z:", juce::dontSendNotification);
        lfoShapeZLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(lfoShapeZSelector);
        for (int i = 0; i < lfoShapes.size(); ++i)
            lfoShapeZSelector.addItem(lfoShapes[i], i + 1);
        lfoShapeZSelector.setSelectedId(1, juce::dontSendNotification);

        // Rate X/Y/Z sliders (0.01-100, formula: pow(10.0,(x*4.0)-2.0))
        addAndMakeVisible(lfoRateXLabel);
        lfoRateXLabel.setText("Rate X:", juce::dontSendNotification);
        lfoRateXLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lfoRateXSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFFE91E63));
        lfoRateXSlider.onValueChanged = [this](float v) {
            float rate = std::pow(10.0f, (v * 4.0f) - 2.0f);
            lfoRateXValueLabel.setText(juce::String(rate, 2) + "x", juce::dontSendNotification);
        };
        addAndMakeVisible(lfoRateXSlider);
        addAndMakeVisible(lfoRateXValueLabel);
        lfoRateXValueLabel.setText("1.00x", juce::dontSendNotification);
        lfoRateXValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel(lfoRateXValueLabel);

        addAndMakeVisible(lfoRateYLabel);
        lfoRateYLabel.setText("Rate Y:", juce::dontSendNotification);
        lfoRateYLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lfoRateYSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFFE91E63));
        lfoRateYSlider.onValueChanged = [this](float v) {
            float rate = std::pow(10.0f, (v * 4.0f) - 2.0f);
            lfoRateYValueLabel.setText(juce::String(rate, 2) + "x", juce::dontSendNotification);
        };
        addAndMakeVisible(lfoRateYSlider);
        addAndMakeVisible(lfoRateYValueLabel);
        lfoRateYValueLabel.setText("1.00x", juce::dontSendNotification);
        lfoRateYValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel(lfoRateYValueLabel);

        addAndMakeVisible(lfoRateZLabel);
        lfoRateZLabel.setText("Rate Z:", juce::dontSendNotification);
        lfoRateZLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lfoRateZSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFFE91E63));
        lfoRateZSlider.onValueChanged = [this](float v) {
            float rate = std::pow(10.0f, (v * 4.0f) - 2.0f);
            lfoRateZValueLabel.setText(juce::String(rate, 2) + "x", juce::dontSendNotification);
        };
        addAndMakeVisible(lfoRateZSlider);
        addAndMakeVisible(lfoRateZValueLabel);
        lfoRateZValueLabel.setText("1.00x", juce::dontSendNotification);
        lfoRateZValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel(lfoRateZValueLabel);

        // Amplitude X/Y/Z sliders (0-50 m)
        addAndMakeVisible(lfoAmplitudeXLabel);
        lfoAmplitudeXLabel.setText("Ampl. X:", juce::dontSendNotification);
        lfoAmplitudeXLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lfoAmplitudeXSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFF9C27B0));
        lfoAmplitudeXSlider.onValueChanged = [this](float v) {
            float amp = v * 50.0f;
            lfoAmplitudeXValueLabel.setText(juce::String(amp, 1) + " m", juce::dontSendNotification);
        };
        addAndMakeVisible(lfoAmplitudeXSlider);
        addAndMakeVisible(lfoAmplitudeXValueLabel);
        lfoAmplitudeXValueLabel.setText("1.0 m", juce::dontSendNotification);
        lfoAmplitudeXValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel(lfoAmplitudeXValueLabel);

        addAndMakeVisible(lfoAmplitudeYLabel);
        lfoAmplitudeYLabel.setText("Ampl. Y:", juce::dontSendNotification);
        lfoAmplitudeYLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lfoAmplitudeYSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFF9C27B0));
        lfoAmplitudeYSlider.onValueChanged = [this](float v) {
            float amp = v * 50.0f;
            lfoAmplitudeYValueLabel.setText(juce::String(amp, 1) + " m", juce::dontSendNotification);
        };
        addAndMakeVisible(lfoAmplitudeYSlider);
        addAndMakeVisible(lfoAmplitudeYValueLabel);
        lfoAmplitudeYValueLabel.setText("1.0 m", juce::dontSendNotification);
        lfoAmplitudeYValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel(lfoAmplitudeYValueLabel);

        addAndMakeVisible(lfoAmplitudeZLabel);
        lfoAmplitudeZLabel.setText("Ampl. Z:", juce::dontSendNotification);
        lfoAmplitudeZLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lfoAmplitudeZSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFF9C27B0));
        lfoAmplitudeZSlider.onValueChanged = [this](float v) {
            float amp = v * 50.0f;
            lfoAmplitudeZValueLabel.setText(juce::String(amp, 1) + " m", juce::dontSendNotification);
        };
        addAndMakeVisible(lfoAmplitudeZSlider);
        addAndMakeVisible(lfoAmplitudeZValueLabel);
        lfoAmplitudeZValueLabel.setText("1.0 m", juce::dontSendNotification);
        lfoAmplitudeZValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel(lfoAmplitudeZValueLabel);

        // Phase X/Y/Z dials (0-360°)
        addAndMakeVisible(lfoPhaseXLabel);
        lfoPhaseXLabel.setText("Phase X:", juce::dontSendNotification);
        lfoPhaseXLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lfoPhaseXDial.setColours(juce::Colours::black, juce::Colour(0xFFFF9800), juce::Colours::grey);
        lfoPhaseXDial.onAngleChanged = [this](float angle) {
            int degrees = static_cast<int>(angle);
            if (degrees < 0) degrees += 360;
            lfoPhaseXValueLabel.setText(juce::String(degrees) + juce::String::fromUTF8("°"), juce::dontSendNotification);
        };
        addAndMakeVisible(lfoPhaseXDial);
        addAndMakeVisible(lfoPhaseXValueLabel);
        lfoPhaseXValueLabel.setText(juce::String::fromUTF8("0°"), juce::dontSendNotification);
        lfoPhaseXValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lfoPhaseXValueLabel.setJustificationType(juce::Justification::centred);
        setupEditableValueLabel(lfoPhaseXValueLabel);

        addAndMakeVisible(lfoPhaseYLabel);
        lfoPhaseYLabel.setText("Phase Y:", juce::dontSendNotification);
        lfoPhaseYLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lfoPhaseYDial.setColours(juce::Colours::black, juce::Colour(0xFFFF9800), juce::Colours::grey);
        lfoPhaseYDial.onAngleChanged = [this](float angle) {
            int degrees = static_cast<int>(angle);
            if (degrees < 0) degrees += 360;
            lfoPhaseYValueLabel.setText(juce::String(degrees) + juce::String::fromUTF8("°"), juce::dontSendNotification);
        };
        addAndMakeVisible(lfoPhaseYDial);
        addAndMakeVisible(lfoPhaseYValueLabel);
        lfoPhaseYValueLabel.setText(juce::String::fromUTF8("0°"), juce::dontSendNotification);
        lfoPhaseYValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lfoPhaseYValueLabel.setJustificationType(juce::Justification::centred);
        setupEditableValueLabel(lfoPhaseYValueLabel);

        addAndMakeVisible(lfoPhaseZLabel);
        lfoPhaseZLabel.setText("Phase Z:", juce::dontSendNotification);
        lfoPhaseZLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lfoPhaseZDial.setColours(juce::Colours::black, juce::Colour(0xFFFF9800), juce::Colours::grey);
        lfoPhaseZDial.onAngleChanged = [this](float angle) {
            int degrees = static_cast<int>(angle);
            if (degrees < 0) degrees += 360;
            lfoPhaseZValueLabel.setText(juce::String(degrees) + juce::String::fromUTF8("°"), juce::dontSendNotification);
        };
        addAndMakeVisible(lfoPhaseZDial);
        addAndMakeVisible(lfoPhaseZValueLabel);
        lfoPhaseZValueLabel.setText(juce::String::fromUTF8("0°"), juce::dontSendNotification);
        lfoPhaseZValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lfoPhaseZValueLabel.setJustificationType(juce::Justification::centred);
        setupEditableValueLabel(lfoPhaseZValueLabel);

        // Gyrophone dropdown
        addAndMakeVisible(lfoGyrophoneLabel);
        lfoGyrophoneLabel.setText("Gyrophone:", juce::dontSendNotification);
        lfoGyrophoneLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(lfoGyrophoneSelector);
        lfoGyrophoneSelector.addItem("Anti-Clockwise", 1);
        lfoGyrophoneSelector.addItem("OFF", 2);
        lfoGyrophoneSelector.addItem("Clockwise", 3);
        lfoGyrophoneSelector.setSelectedId(2, juce::dontSendNotification);
    }

    void setupAutomotionTab()
    {
        // Destination X/Y/Z number boxes
        addAndMakeVisible(otomoDestXLabel);
        otomoDestXLabel.setText("Dest. X:", juce::dontSendNotification);
        otomoDestXLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(otomoDestXEditor);
        otomoDestXEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(otomoDestXEditor, true, true);
        addAndMakeVisible(otomoDestXUnitLabel);
        otomoDestXUnitLabel.setText("m", juce::dontSendNotification);
        otomoDestXUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(otomoDestYLabel);
        otomoDestYLabel.setText("Dest. Y:", juce::dontSendNotification);
        otomoDestYLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(otomoDestYEditor);
        otomoDestYEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(otomoDestYEditor, true, true);
        addAndMakeVisible(otomoDestYUnitLabel);
        otomoDestYUnitLabel.setText("m", juce::dontSendNotification);
        otomoDestYUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(otomoDestZLabel);
        otomoDestZLabel.setText("Dest. Z:", juce::dontSendNotification);
        otomoDestZLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(otomoDestZEditor);
        otomoDestZEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(otomoDestZEditor, true, true);
        addAndMakeVisible(otomoDestZUnitLabel);
        otomoDestZUnitLabel.setText("m", juce::dontSendNotification);
        otomoDestZUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        // Absolute/Relative button
        addAndMakeVisible(otomoAbsRelButton);
        otomoAbsRelButton.setButtonText("Absolute");
        otomoAbsRelButton.setClickingTogglesState(true);
        otomoAbsRelButton.onClick = [this]() {
            otomoAbsRelButton.setButtonText(otomoAbsRelButton.getToggleState() ? "Relative" : "Absolute");
        };

        // Stay/Return button
        addAndMakeVisible(otomoStayReturnButton);
        otomoStayReturnButton.setButtonText("Stay");
        otomoStayReturnButton.setClickingTogglesState(true);
        otomoStayReturnButton.onClick = [this]() {
            otomoStayReturnButton.setButtonText(otomoStayReturnButton.getToggleState() ? "Return" : "Stay");
        };

        // Speed Profile dial (0-100%)
        addAndMakeVisible(otomoSpeedProfileLabel);
        otomoSpeedProfileLabel.setText("Speed Profile:", juce::dontSendNotification);
        otomoSpeedProfileLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        otomoSpeedProfileDial.setColours(juce::Colours::black, juce::Colour(0xFF2196F3), juce::Colours::grey);
        otomoSpeedProfileDial.onValueChanged = [this](float v) {
            otomoSpeedProfileValueLabel.setText(juce::String(static_cast<int>(v * 100.0f)) + " %", juce::dontSendNotification);
        };
        addAndMakeVisible(otomoSpeedProfileDial);
        addAndMakeVisible(otomoSpeedProfileValueLabel);
        otomoSpeedProfileValueLabel.setText("0 %", juce::dontSendNotification);
        otomoSpeedProfileValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        otomoSpeedProfileValueLabel.setJustificationType(juce::Justification::centred);
        setupEditableValueLabel(otomoSpeedProfileValueLabel);

        // Trigger button (Manual/Trigger)
        addAndMakeVisible(otomoTriggerButton);
        otomoTriggerButton.setButtonText("Manual");
        otomoTriggerButton.setClickingTogglesState(true);
        otomoTriggerButton.onClick = [this]() {
            otomoTriggerButton.setButtonText(otomoTriggerButton.getToggleState() ? "Trigger" : "Manual");
        };

        // Trigger Threshold dial (-92 to 0 dB)
        addAndMakeVisible(otomoThresholdLabel);
        otomoThresholdLabel.setText("Threshold:", juce::dontSendNotification);
        otomoThresholdLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        otomoThresholdDial.setColours(juce::Colours::black, juce::Colour(0xFFE91E63), juce::Colours::grey);
        otomoThresholdDial.onValueChanged = [this](float v) {
            float dB = 20.0f * std::log10(std::pow(10.0f, -92.0f / 20.0f) +
                       ((1.0f - std::pow(10.0f, -92.0f / 20.0f)) * v * v));
            otomoThresholdValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
        };
        addAndMakeVisible(otomoThresholdDial);
        addAndMakeVisible(otomoThresholdValueLabel);
        otomoThresholdValueLabel.setText("-20.0 dB", juce::dontSendNotification);
        otomoThresholdValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        otomoThresholdValueLabel.setJustificationType(juce::Justification::centred);
        setupEditableValueLabel(otomoThresholdValueLabel);

        // Trigger Reset dial (-92 to 0 dB)
        addAndMakeVisible(otomoResetLabel);
        otomoResetLabel.setText("Reset:", juce::dontSendNotification);
        otomoResetLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        otomoResetDial.setColours(juce::Colours::black, juce::Colour(0xFF9C27B0), juce::Colours::grey);
        otomoResetDial.onValueChanged = [this](float v) {
            float dB = 20.0f * std::log10(std::pow(10.0f, -92.0f / 20.0f) +
                       ((1.0f - std::pow(10.0f, -92.0f / 20.0f)) * v * v));
            otomoResetValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
        };
        addAndMakeVisible(otomoResetDial);
        addAndMakeVisible(otomoResetValueLabel);
        otomoResetValueLabel.setText("-60.0 dB", juce::dontSendNotification);
        otomoResetValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        otomoResetValueLabel.setJustificationType(juce::Justification::centred);
        setupEditableValueLabel(otomoResetValueLabel);

        // Transport buttons
        addAndMakeVisible(otomoStartButton);
        otomoStartButton.setButtonText(juce::String::fromUTF8("\u25B6"));  // Play symbol
        otomoStartButton.onClick = [this]() { /* Start movement */ };

        addAndMakeVisible(otomoStopButton);
        otomoStopButton.setButtonText(juce::String::fromUTF8("\u25A0"));  // Stop symbol
        otomoStopButton.onClick = [this]() { /* Stop movement */ };

        addAndMakeVisible(otomoPauseButton);
        otomoPauseButton.setButtonText(juce::String::fromUTF8("\u23F8"));  // Pause symbol
        otomoPauseButton.setClickingTogglesState(true);
        otomoPauseButton.onClick = [this]() {
            // Toggle pause/resume
        };
    }

    void setupMutesTab()
    {
        // Create 64 mute toggle buttons (8x8 grid)
        for (int i = 0; i < 64; ++i)
        {
            muteButtons[i].setButtonText(juce::String(i + 1));
            muteButtons[i].setClickingTogglesState(true);
            muteButtons[i].setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF3A3A3A));
            muteButtons[i].setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFFFF5722));
            muteButtons[i].onClick = [this, i]() {
                // Toggle mute for output i+1
            };
            addAndMakeVisible(muteButtons[i]);
        }

        // Mute Macros selector
        addAndMakeVisible(muteMacrosLabel);
        muteMacrosLabel.setText("Mute Macros:", juce::dontSendNotification);
        muteMacrosLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(muteMacrosSelector);
        muteMacrosSelector.addItem("Select Macro...", 1);
        muteMacrosSelector.addItem("MUTE ALL", 2);
        muteMacrosSelector.addItem("UNMUTE ALL", 3);
        muteMacrosSelector.addItem("INVERT MUTES", 4);
        muteMacrosSelector.addItem("MUTE ODD", 5);
        muteMacrosSelector.addItem("MUTE EVEN", 6);
        for (int i = 1; i <= 10; ++i)
        {
            muteMacrosSelector.addItem("MUTE ARRAY " + juce::String(i), 6 + (i * 2) - 1);
            muteMacrosSelector.addItem("UNMUTE ARRAY " + juce::String(i), 6 + (i * 2));
        }
        muteMacrosSelector.setSelectedId(1, juce::dontSendNotification);
        muteMacrosSelector.onChange = [this]() {
            int macroId = muteMacrosSelector.getSelectedId();
            if (macroId > 1) applyMuteMacro(macroId);
            muteMacrosSelector.setSelectedId(1, juce::dontSendNotification);
        };
    }

    void applyMuteMacro(int macroId)
    {
        switch (macroId)
        {
            case 2: // MUTE ALL
                for (int i = 0; i < 64; ++i)
                    muteButtons[i].setToggleState(true, juce::sendNotification);
                break;
            case 3: // UNMUTE ALL
                for (int i = 0; i < 64; ++i)
                    muteButtons[i].setToggleState(false, juce::sendNotification);
                break;
            case 4: // INVERT MUTES
                for (int i = 0; i < 64; ++i)
                    muteButtons[i].setToggleState(!muteButtons[i].getToggleState(), juce::sendNotification);
                break;
            case 5: // MUTE ODD
                for (int i = 0; i < 64; ++i)
                    muteButtons[i].setToggleState((i % 2) == 0, juce::sendNotification);
                break;
            case 6: // MUTE EVEN
                for (int i = 0; i < 64; ++i)
                    muteButtons[i].setToggleState((i % 2) == 1, juce::sendNotification);
                break;
            default:
                // Array mute/unmute macros would need array information
                break;
        }
    }

    void setupNumericEditor(juce::TextEditor& editor, bool allowNegative, bool allowDecimal)
    {
        juce::String allowedChars = "0123456789";
        if (allowNegative) allowedChars += "-";
        if (allowDecimal) allowedChars += ".";
        editor.setInputFilter(new juce::TextEditor::LengthAndCharacterRestriction(10, allowedChars), true);
        editor.setSelectAllWhenFocused(true);
        editor.addListener(this);
    }

    void setupEditableValueLabel(juce::Label& label)
    {
        label.setEditable(true, false);  // Single click to edit
        label.addListener(this);
    }

    // ==================== LAYOUT METHODS ====================

    void layoutCurrentSubTab()
    {
        int tabIndex = subTabBar.getCurrentTabIndex();

        // Hide all
        setInputPropertiesVisible(false);
        setPositionVisible(false);
        setSoundVisible(false);
        setLiveSourceVisible(false);
        setEffectsVisible(false);
        setLfoVisible(false);
        setAutomotionVisible(false);
        setMutesVisible(false);

        // Show current
        if (tabIndex == 0) { setInputPropertiesVisible(true); layoutInputPropertiesTab(); }
        else if (tabIndex == 1) { setPositionVisible(true); layoutPositionTab(); }
        else if (tabIndex == 2) { setSoundVisible(true); layoutSoundTab(); }
        else if (tabIndex == 3) { setLiveSourceVisible(true); layoutLiveSourceTab(); }
        else if (tabIndex == 4) { setEffectsVisible(true); layoutEffectsTab(); }
        else if (tabIndex == 5) { setLfoVisible(true); layoutLfoTab(); }
        else if (tabIndex == 6) { setAutomotionVisible(true); layoutAutomotionTab(); }
        else if (tabIndex == 7) { setMutesVisible(true); layoutMutesTab(); }
    }

    void setInputPropertiesVisible(bool v)
    {
        attenuationLabel.setVisible(v);
        attenuationSlider.setVisible(v);
        attenuationValueLabel.setVisible(v);
        delayLatencyLabel.setVisible(v);
        delayLatencySlider.setVisible(v);
        delayLatencyValueLabel.setVisible(v);
        minimalLatencyButton.setVisible(v);
    }

    void setPositionVisible(bool v)
    {
        posXLabel.setVisible(v); posXEditor.setVisible(v); posXUnitLabel.setVisible(v);
        posYLabel.setVisible(v); posYEditor.setVisible(v); posYUnitLabel.setVisible(v);
        posZLabel.setVisible(v); posZEditor.setVisible(v); posZUnitLabel.setVisible(v);
        offsetXLabel.setVisible(v); offsetXEditor.setVisible(v); offsetXUnitLabel.setVisible(v);
        offsetYLabel.setVisible(v); offsetYEditor.setVisible(v); offsetYUnitLabel.setVisible(v);
        offsetZLabel.setVisible(v); offsetZEditor.setVisible(v); offsetZUnitLabel.setVisible(v);
        constraintXButton.setVisible(v); constraintYButton.setVisible(v); constraintZButton.setVisible(v);
        flipXButton.setVisible(v); flipYButton.setVisible(v); flipZButton.setVisible(v);
        trackingActiveButton.setVisible(v);
        trackingIdLabel.setVisible(v); trackingIdSelector.setVisible(v);
        trackingSmoothLabel.setVisible(v); trackingSmoothDial.setVisible(v); trackingSmoothValueLabel.setVisible(v);
        maxSpeedActiveButton.setVisible(v);
        maxSpeedLabel.setVisible(v); maxSpeedDial.setVisible(v); maxSpeedValueLabel.setVisible(v);
        heightFactorLabel.setVisible(v); heightFactorDial.setVisible(v); heightFactorValueLabel.setVisible(v);
    }

    void setSoundVisible(bool v)
    {
        attenuationLawButton.setVisible(v);
        // Show Distance Atten or Distance Ratio based on attenuation law
        bool is1OverD = attenuationLawButton.getToggleState();
        distanceAttenLabel.setVisible(v && !is1OverD);
        distanceAttenDial.setVisible(v && !is1OverD);
        distanceAttenValueLabel.setVisible(v && !is1OverD);
        distanceRatioLabel.setVisible(v && is1OverD);
        distanceRatioDial.setVisible(v && is1OverD);
        distanceRatioValueLabel.setVisible(v && is1OverD);
        commonAttenLabel.setVisible(v); commonAttenDial.setVisible(v); commonAttenValueLabel.setVisible(v);
        directivityLabel.setVisible(v); directivitySlider.setVisible(v); directivityValueLabel.setVisible(v);
        rotationLabel.setVisible(v); rotationDial.setVisible(v); rotationValueLabel.setVisible(v);
        tiltLabel.setVisible(v); tiltSlider.setVisible(v); tiltValueLabel.setVisible(v);
        hfShelfLabel.setVisible(v); hfShelfSlider.setVisible(v); hfShelfValueLabel.setVisible(v);
    }

    void setLiveSourceVisible(bool v)
    {
        lsActiveButton.setVisible(v);
        lsRadiusLabel.setVisible(v); lsRadiusSlider.setVisible(v); lsRadiusValueLabel.setVisible(v);
        lsShapeLabel.setVisible(v); lsShapeSelector.setVisible(v);
        lsAttenuationLabel.setVisible(v); lsAttenuationSlider.setVisible(v); lsAttenuationValueLabel.setVisible(v);
        lsPeakThresholdLabel.setVisible(v); lsPeakThresholdSlider.setVisible(v); lsPeakThresholdValueLabel.setVisible(v);
        lsPeakRatioLabel.setVisible(v); lsPeakRatioDial.setVisible(v); lsPeakRatioValueLabel.setVisible(v);
        lsSlowThresholdLabel.setVisible(v); lsSlowThresholdSlider.setVisible(v); lsSlowThresholdValueLabel.setVisible(v);
        lsSlowRatioLabel.setVisible(v); lsSlowRatioDial.setVisible(v); lsSlowRatioValueLabel.setVisible(v);
    }

    void setEffectsVisible(bool v)
    {
        frActiveButton.setVisible(v);
        frAttenuationLabel.setVisible(v); frAttenuationSlider.setVisible(v); frAttenuationValueLabel.setVisible(v);
        frDiffusionLabel.setVisible(v); frDiffusionDial.setVisible(v); frDiffusionValueLabel.setVisible(v);
        frLowCutActiveButton.setVisible(v);
        frLowCutFreqLabel.setVisible(v); frLowCutFreqSlider.setVisible(v); frLowCutFreqValueLabel.setVisible(v);
        frHighShelfActiveButton.setVisible(v);
        frHighShelfFreqLabel.setVisible(v); frHighShelfFreqSlider.setVisible(v); frHighShelfFreqValueLabel.setVisible(v);
        frHighShelfGainLabel.setVisible(v); frHighShelfGainSlider.setVisible(v); frHighShelfGainValueLabel.setVisible(v);
        frHighShelfSlopeLabel.setVisible(v); frHighShelfSlopeSlider.setVisible(v); frHighShelfSlopeValueLabel.setVisible(v);
        jitterLabel.setVisible(v); jitterSlider.setVisible(v); jitterValueLabel.setVisible(v);
    }

    void layoutInputPropertiesTab()
    {
        auto area = subTabContentArea;
        const int rowHeight = 30;
        const int sliderHeight = 40;
        const int spacing = 10;
        const int labelWidth = 120;
        const int valueWidth = 100;

        auto leftCol = area.removeFromLeft(area.getWidth() / 2).reduced(5, 0);

        // Attenuation
        auto row = leftCol.removeFromTop(rowHeight);
        attenuationLabel.setBounds(row.removeFromLeft(labelWidth));
        attenuationValueLabel.setBounds(row.removeFromRight(valueWidth));
        leftCol.removeFromTop(spacing / 2);
        attenuationSlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing);

        // Delay/Latency
        row = leftCol.removeFromTop(rowHeight);
        delayLatencyLabel.setBounds(row.removeFromLeft(labelWidth));
        delayLatencyValueLabel.setBounds(row.removeFromRight(valueWidth));
        leftCol.removeFromTop(spacing / 2);
        delayLatencySlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing);

        // Minimal Latency
        minimalLatencyButton.setBounds(leftCol.removeFromTop(rowHeight).withWidth(200));
    }

    void layoutPositionTab()
    {
        auto area = subTabContentArea;
        const int rowHeight = 30;
        const int spacing = 8;
        const int labelWidth = 80;
        const int editorWidth = 80;
        const int unitWidth = 25;
        const int buttonWidth = 130;

        auto leftCol = area.removeFromLeft(area.getWidth() / 2).reduced(5, 0);
        auto rightCol = area.reduced(5, 0);

        // Position row
        auto row = leftCol.removeFromTop(rowHeight);
        posXLabel.setBounds(row.removeFromLeft(labelWidth));
        posXEditor.setBounds(row.removeFromLeft(editorWidth));
        posXUnitLabel.setBounds(row.removeFromLeft(unitWidth));
        row.removeFromLeft(spacing);
        posYLabel.setBounds(row.removeFromLeft(labelWidth));
        posYEditor.setBounds(row.removeFromLeft(editorWidth));
        posYUnitLabel.setBounds(row.removeFromLeft(unitWidth));
        leftCol.removeFromTop(spacing);

        row = leftCol.removeFromTop(rowHeight);
        posZLabel.setBounds(row.removeFromLeft(labelWidth));
        posZEditor.setBounds(row.removeFromLeft(editorWidth));
        posZUnitLabel.setBounds(row.removeFromLeft(unitWidth));
        leftCol.removeFromTop(spacing * 2);

        // Offset row
        row = leftCol.removeFromTop(rowHeight);
        offsetXLabel.setBounds(row.removeFromLeft(labelWidth));
        offsetXEditor.setBounds(row.removeFromLeft(editorWidth));
        offsetXUnitLabel.setBounds(row.removeFromLeft(unitWidth));
        row.removeFromLeft(spacing);
        offsetYLabel.setBounds(row.removeFromLeft(labelWidth));
        offsetYEditor.setBounds(row.removeFromLeft(editorWidth));
        offsetYUnitLabel.setBounds(row.removeFromLeft(unitWidth));
        leftCol.removeFromTop(spacing);

        row = leftCol.removeFromTop(rowHeight);
        offsetZLabel.setBounds(row.removeFromLeft(labelWidth));
        offsetZEditor.setBounds(row.removeFromLeft(editorWidth));
        offsetZUnitLabel.setBounds(row.removeFromLeft(unitWidth));
        leftCol.removeFromTop(spacing * 2);

        // Constraint buttons
        row = leftCol.removeFromTop(rowHeight);
        constraintXButton.setBounds(row.removeFromLeft(buttonWidth));
        row.removeFromLeft(spacing);
        constraintYButton.setBounds(row.removeFromLeft(buttonWidth));
        row.removeFromLeft(spacing);
        constraintZButton.setBounds(row.removeFromLeft(buttonWidth));
        leftCol.removeFromTop(spacing);

        // Flip buttons
        row = leftCol.removeFromTop(rowHeight);
        flipXButton.setBounds(row.removeFromLeft(buttonWidth));
        row.removeFromLeft(spacing);
        flipYButton.setBounds(row.removeFromLeft(buttonWidth));
        row.removeFromLeft(spacing);
        flipZButton.setBounds(row.removeFromLeft(buttonWidth));

        // Right column - Tracking section
        row = rightCol.removeFromTop(rowHeight);
        trackingActiveButton.setBounds(row.removeFromLeft(150));
        rightCol.removeFromTop(spacing);

        row = rightCol.removeFromTop(rowHeight);
        trackingIdLabel.setBounds(row.removeFromLeft(90));
        trackingIdSelector.setBounds(row.removeFromLeft(70));
        rightCol.removeFromTop(spacing);

        // Tracking Smooth dial
        const int dialSize = 70;
        trackingSmoothLabel.setBounds(rightCol.removeFromTop(rowHeight));
        auto dialArea = rightCol.removeFromTop(dialSize);
        trackingSmoothDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
        trackingSmoothValueLabel.setBounds(rightCol.removeFromTop(rowHeight - 5));
        rightCol.removeFromTop(spacing);

        // Max Speed section
        row = rightCol.removeFromTop(rowHeight);
        maxSpeedActiveButton.setBounds(row.removeFromLeft(150));
        rightCol.removeFromTop(spacing);

        maxSpeedLabel.setBounds(rightCol.removeFromTop(rowHeight));
        dialArea = rightCol.removeFromTop(dialSize);
        maxSpeedDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
        maxSpeedValueLabel.setBounds(rightCol.removeFromTop(rowHeight - 5));
        rightCol.removeFromTop(spacing);

        // Height Factor dial
        heightFactorLabel.setBounds(rightCol.removeFromTop(rowHeight));
        dialArea = rightCol.removeFromTop(dialSize);
        heightFactorDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
        heightFactorValueLabel.setBounds(rightCol.removeFromTop(rowHeight - 5));
    }

    void layoutSoundTab()
    {
        auto area = subTabContentArea;
        const int rowHeight = 30;
        const int sliderHeight = 40;
        const int spacing = 10;
        const int labelWidth = 120;
        const int valueWidth = 80;
        const int dialSize = 100;

        auto leftCol = area.removeFromLeft(area.getWidth() * 2 / 3).reduced(5, 0);
        auto rightCol = area.reduced(5, 0);

        // Attenuation Law button
        attenuationLawButton.setBounds(leftCol.removeFromTop(rowHeight).withWidth(100));
        leftCol.removeFromTop(spacing);

        // Directivity
        auto row = leftCol.removeFromTop(rowHeight);
        directivityLabel.setBounds(row.removeFromLeft(labelWidth));
        directivityValueLabel.setBounds(row.removeFromRight(valueWidth));
        leftCol.removeFromTop(spacing / 2);
        directivitySlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing);

        // Tilt
        row = leftCol.removeFromTop(rowHeight);
        tiltLabel.setBounds(row.removeFromLeft(labelWidth));
        tiltValueLabel.setBounds(row.removeFromRight(valueWidth));
        leftCol.removeFromTop(spacing / 2);
        tiltSlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing);

        // HF Shelf
        row = leftCol.removeFromTop(rowHeight);
        hfShelfLabel.setBounds(row.removeFromLeft(labelWidth));
        hfShelfValueLabel.setBounds(row.removeFromRight(valueWidth));
        leftCol.removeFromTop(spacing / 2);
        hfShelfSlider.setBounds(leftCol.removeFromTop(sliderHeight));

        // Right column - Dials
        // Distance Attenuation and Distance Ratio share the same position
        auto distanceDialLabelBounds = rightCol.removeFromTop(rowHeight);
        distanceAttenLabel.setBounds(distanceDialLabelBounds);
        distanceRatioLabel.setBounds(distanceDialLabelBounds);
        auto dialArea = rightCol.removeFromTop(dialSize);
        auto dialBounds = dialArea.withSizeKeepingCentre(dialSize, dialSize);
        distanceAttenDial.setBounds(dialBounds);
        distanceRatioDial.setBounds(dialBounds);
        auto distanceDialValueBounds = rightCol.removeFromTop(rowHeight);
        distanceAttenValueLabel.setBounds(distanceDialValueBounds);
        distanceRatioValueLabel.setBounds(distanceDialValueBounds);
        rightCol.removeFromTop(spacing);

        commonAttenLabel.setBounds(rightCol.removeFromTop(rowHeight));
        dialArea = rightCol.removeFromTop(dialSize);
        commonAttenDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
        commonAttenValueLabel.setBounds(rightCol.removeFromTop(rowHeight));
        rightCol.removeFromTop(spacing);

        rotationLabel.setBounds(rightCol.removeFromTop(rowHeight));
        dialArea = rightCol.removeFromTop(dialSize);
        rotationDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
        rotationValueLabel.setBounds(rightCol.removeFromTop(rowHeight));
    }

    void layoutLiveSourceTab()
    {
        auto area = subTabContentArea;
        const int rowHeight = 30;
        const int sliderHeight = 40;
        const int spacing = 10;
        const int labelWidth = 120;
        const int valueWidth = 80;
        const int dialSize = 80;

        auto leftCol = area.removeFromLeft(area.getWidth() * 2 / 3).reduced(5, 0);
        auto rightCol = area.reduced(5, 0);

        // Active button
        lsActiveButton.setBounds(leftCol.removeFromTop(rowHeight).withWidth(200));
        leftCol.removeFromTop(spacing);

        // Shape selector
        auto row = leftCol.removeFromTop(rowHeight);
        lsShapeLabel.setBounds(row.removeFromLeft(labelWidth));
        lsShapeSelector.setBounds(row.removeFromLeft(100));
        leftCol.removeFromTop(spacing);

        // Radius
        row = leftCol.removeFromTop(rowHeight);
        lsRadiusLabel.setBounds(row.removeFromLeft(labelWidth));
        lsRadiusValueLabel.setBounds(row.removeFromRight(valueWidth));
        leftCol.removeFromTop(spacing / 2);
        lsRadiusSlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing);

        // Attenuation
        row = leftCol.removeFromTop(rowHeight);
        lsAttenuationLabel.setBounds(row.removeFromLeft(labelWidth));
        lsAttenuationValueLabel.setBounds(row.removeFromRight(valueWidth));
        leftCol.removeFromTop(spacing / 2);
        lsAttenuationSlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing);

        // Peak Threshold
        row = leftCol.removeFromTop(rowHeight);
        lsPeakThresholdLabel.setBounds(row.removeFromLeft(labelWidth));
        lsPeakThresholdValueLabel.setBounds(row.removeFromRight(valueWidth));
        leftCol.removeFromTop(spacing / 2);
        lsPeakThresholdSlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing);

        // Slow Threshold
        row = leftCol.removeFromTop(rowHeight);
        lsSlowThresholdLabel.setBounds(row.removeFromLeft(labelWidth));
        lsSlowThresholdValueLabel.setBounds(row.removeFromRight(valueWidth));
        leftCol.removeFromTop(spacing / 2);
        lsSlowThresholdSlider.setBounds(leftCol.removeFromTop(sliderHeight));

        // Right column - Ratio dials
        lsPeakRatioLabel.setBounds(rightCol.removeFromTop(rowHeight));
        auto dialArea = rightCol.removeFromTop(dialSize);
        lsPeakRatioDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
        lsPeakRatioValueLabel.setBounds(rightCol.removeFromTop(rowHeight));
        rightCol.removeFromTop(spacing * 2);

        lsSlowRatioLabel.setBounds(rightCol.removeFromTop(rowHeight));
        dialArea = rightCol.removeFromTop(dialSize);
        lsSlowRatioDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
        lsSlowRatioValueLabel.setBounds(rightCol.removeFromTop(rowHeight));
    }

    void layoutEffectsTab()
    {
        auto area = subTabContentArea;
        const int rowHeight = 26;
        const int sliderHeight = 32;
        const int spacing = 6;
        const int labelWidth = 100;
        const int valueWidth = 70;
        const int dialSize = 70;
        const int buttonWidth = 120;

        auto leftCol = area.removeFromLeft(area.getWidth() / 2).reduced(5, 0);
        auto rightCol = area.reduced(5, 0);

        // Floor Reflections Active
        frActiveButton.setBounds(leftCol.removeFromTop(rowHeight).withWidth(200));
        leftCol.removeFromTop(spacing);

        // FR Attenuation
        auto row = leftCol.removeFromTop(rowHeight);
        frAttenuationLabel.setBounds(row.removeFromLeft(labelWidth));
        frAttenuationValueLabel.setBounds(row.removeFromRight(valueWidth));
        frAttenuationSlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing);

        // Low Cut Active + Frequency
        row = leftCol.removeFromTop(rowHeight);
        frLowCutActiveButton.setBounds(row.removeFromLeft(buttonWidth));
        row.removeFromLeft(spacing);
        frLowCutFreqLabel.setBounds(row.removeFromLeft(labelWidth));
        frLowCutFreqValueLabel.setBounds(row.removeFromRight(valueWidth));
        frLowCutFreqSlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing);

        // High Shelf Active + Frequency
        row = leftCol.removeFromTop(rowHeight);
        frHighShelfActiveButton.setBounds(row.removeFromLeft(buttonWidth));
        row.removeFromLeft(spacing);
        frHighShelfFreqLabel.setBounds(row.removeFromLeft(labelWidth - 20));
        frHighShelfFreqValueLabel.setBounds(row.removeFromRight(valueWidth));
        frHighShelfFreqSlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing);

        // High Shelf Gain
        row = leftCol.removeFromTop(rowHeight);
        frHighShelfGainLabel.setBounds(row.removeFromLeft(labelWidth));
        frHighShelfGainValueLabel.setBounds(row.removeFromRight(valueWidth));
        frHighShelfGainSlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing);

        // High Shelf Slope
        row = leftCol.removeFromTop(rowHeight);
        frHighShelfSlopeLabel.setBounds(row.removeFromLeft(labelWidth));
        frHighShelfSlopeValueLabel.setBounds(row.removeFromRight(valueWidth));
        frHighShelfSlopeSlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing);

        // Jitter
        row = leftCol.removeFromTop(rowHeight);
        jitterLabel.setBounds(row.removeFromLeft(labelWidth));
        jitterValueLabel.setBounds(row.removeFromRight(valueWidth));
        jitterSlider.setBounds(leftCol.removeFromTop(sliderHeight));

        // Right column - FR Diffusion dial
        frDiffusionLabel.setBounds(rightCol.removeFromTop(rowHeight));
        auto dialArea = rightCol.removeFromTop(dialSize);
        frDiffusionDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
        frDiffusionValueLabel.setBounds(rightCol.removeFromTop(rowHeight));
    }

    void setLfoVisible(bool v)
    {
        lfoActiveButton.setVisible(v);
        lfoPeriodLabel.setVisible(v); lfoPeriodDial.setVisible(v); lfoPeriodValueLabel.setVisible(v);
        lfoPhaseLabel.setVisible(v); lfoPhaseDial.setVisible(v); lfoPhaseValueLabel.setVisible(v);
        lfoShapeXLabel.setVisible(v); lfoShapeXSelector.setVisible(v);
        lfoShapeYLabel.setVisible(v); lfoShapeYSelector.setVisible(v);
        lfoShapeZLabel.setVisible(v); lfoShapeZSelector.setVisible(v);
        lfoRateXLabel.setVisible(v); lfoRateXSlider.setVisible(v); lfoRateXValueLabel.setVisible(v);
        lfoRateYLabel.setVisible(v); lfoRateYSlider.setVisible(v); lfoRateYValueLabel.setVisible(v);
        lfoRateZLabel.setVisible(v); lfoRateZSlider.setVisible(v); lfoRateZValueLabel.setVisible(v);
        lfoAmplitudeXLabel.setVisible(v); lfoAmplitudeXSlider.setVisible(v); lfoAmplitudeXValueLabel.setVisible(v);
        lfoAmplitudeYLabel.setVisible(v); lfoAmplitudeYSlider.setVisible(v); lfoAmplitudeYValueLabel.setVisible(v);
        lfoAmplitudeZLabel.setVisible(v); lfoAmplitudeZSlider.setVisible(v); lfoAmplitudeZValueLabel.setVisible(v);
        lfoPhaseXLabel.setVisible(v); lfoPhaseXDial.setVisible(v); lfoPhaseXValueLabel.setVisible(v);
        lfoPhaseYLabel.setVisible(v); lfoPhaseYDial.setVisible(v); lfoPhaseYValueLabel.setVisible(v);
        lfoPhaseZLabel.setVisible(v); lfoPhaseZDial.setVisible(v); lfoPhaseZValueLabel.setVisible(v);
        lfoGyrophoneLabel.setVisible(v); lfoGyrophoneSelector.setVisible(v);
    }

    void layoutLfoTab()
    {
        auto area = subTabContentArea;
        const int rowHeight = 24;
        const int sliderHeight = 28;
        const int spacing = 4;
        const int labelWidth = 70;
        const int valueWidth = 60;
        const int selectorWidth = 100;
        const int dialSize = 55;

        // Split into three columns
        auto leftCol = area.removeFromLeft(area.getWidth() / 3).reduced(5, 0);
        auto middleCol = area.removeFromLeft(area.getWidth() / 2).reduced(5, 0);
        auto rightCol = area.reduced(5, 0);

        // ========== LEFT COLUMN ==========
        // Active button
        lfoActiveButton.setBounds(leftCol.removeFromTop(rowHeight).withWidth(120));
        leftCol.removeFromTop(spacing * 2);

        // Period dial
        lfoPeriodLabel.setBounds(leftCol.removeFromTop(rowHeight));
        auto dialArea = leftCol.removeFromTop(dialSize);
        lfoPeriodDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
        lfoPeriodValueLabel.setBounds(leftCol.removeFromTop(rowHeight));
        leftCol.removeFromTop(spacing);

        // Main Phase dial
        lfoPhaseLabel.setBounds(leftCol.removeFromTop(rowHeight));
        dialArea = leftCol.removeFromTop(dialSize);
        lfoPhaseDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
        lfoPhaseValueLabel.setBounds(leftCol.removeFromTop(rowHeight));
        leftCol.removeFromTop(spacing);

        // Gyrophone
        auto row = leftCol.removeFromTop(rowHeight);
        lfoGyrophoneLabel.setBounds(row.removeFromLeft(labelWidth));
        lfoGyrophoneSelector.setBounds(row.removeFromLeft(selectorWidth));

        // ========== MIDDLE COLUMN - X/Y/Z Parameters ==========
        // Shape selectors
        row = middleCol.removeFromTop(rowHeight);
        lfoShapeXLabel.setBounds(row.removeFromLeft(labelWidth));
        lfoShapeXSelector.setBounds(row.removeFromLeft(selectorWidth));
        middleCol.removeFromTop(spacing);

        row = middleCol.removeFromTop(rowHeight);
        lfoShapeYLabel.setBounds(row.removeFromLeft(labelWidth));
        lfoShapeYSelector.setBounds(row.removeFromLeft(selectorWidth));
        middleCol.removeFromTop(spacing);

        row = middleCol.removeFromTop(rowHeight);
        lfoShapeZLabel.setBounds(row.removeFromLeft(labelWidth));
        lfoShapeZSelector.setBounds(row.removeFromLeft(selectorWidth));
        middleCol.removeFromTop(spacing * 2);

        // Rate sliders
        row = middleCol.removeFromTop(rowHeight);
        lfoRateXLabel.setBounds(row.removeFromLeft(labelWidth));
        lfoRateXValueLabel.setBounds(row.removeFromRight(valueWidth));
        lfoRateXSlider.setBounds(middleCol.removeFromTop(sliderHeight));
        middleCol.removeFromTop(spacing);

        row = middleCol.removeFromTop(rowHeight);
        lfoRateYLabel.setBounds(row.removeFromLeft(labelWidth));
        lfoRateYValueLabel.setBounds(row.removeFromRight(valueWidth));
        lfoRateYSlider.setBounds(middleCol.removeFromTop(sliderHeight));
        middleCol.removeFromTop(spacing);

        row = middleCol.removeFromTop(rowHeight);
        lfoRateZLabel.setBounds(row.removeFromLeft(labelWidth));
        lfoRateZValueLabel.setBounds(row.removeFromRight(valueWidth));
        lfoRateZSlider.setBounds(middleCol.removeFromTop(sliderHeight));
        middleCol.removeFromTop(spacing * 2);

        // Amplitude sliders
        row = middleCol.removeFromTop(rowHeight);
        lfoAmplitudeXLabel.setBounds(row.removeFromLeft(labelWidth));
        lfoAmplitudeXValueLabel.setBounds(row.removeFromRight(valueWidth));
        lfoAmplitudeXSlider.setBounds(middleCol.removeFromTop(sliderHeight));
        middleCol.removeFromTop(spacing);

        row = middleCol.removeFromTop(rowHeight);
        lfoAmplitudeYLabel.setBounds(row.removeFromLeft(labelWidth));
        lfoAmplitudeYValueLabel.setBounds(row.removeFromRight(valueWidth));
        lfoAmplitudeYSlider.setBounds(middleCol.removeFromTop(sliderHeight));
        middleCol.removeFromTop(spacing);

        row = middleCol.removeFromTop(rowHeight);
        lfoAmplitudeZLabel.setBounds(row.removeFromLeft(labelWidth));
        lfoAmplitudeZValueLabel.setBounds(row.removeFromRight(valueWidth));
        lfoAmplitudeZSlider.setBounds(middleCol.removeFromTop(sliderHeight));

        // ========== RIGHT COLUMN - Phase dials ==========
        // Phase X dial
        lfoPhaseXLabel.setBounds(rightCol.removeFromTop(rowHeight));
        dialArea = rightCol.removeFromTop(dialSize);
        lfoPhaseXDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
        lfoPhaseXValueLabel.setBounds(rightCol.removeFromTop(rowHeight));
        rightCol.removeFromTop(spacing);

        // Phase Y dial
        lfoPhaseYLabel.setBounds(rightCol.removeFromTop(rowHeight));
        dialArea = rightCol.removeFromTop(dialSize);
        lfoPhaseYDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
        lfoPhaseYValueLabel.setBounds(rightCol.removeFromTop(rowHeight));
        rightCol.removeFromTop(spacing);

        // Phase Z dial
        lfoPhaseZLabel.setBounds(rightCol.removeFromTop(rowHeight));
        dialArea = rightCol.removeFromTop(dialSize);
        lfoPhaseZDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
        lfoPhaseZValueLabel.setBounds(rightCol.removeFromTop(rowHeight));
    }

    void setAutomotionVisible(bool v)
    {
        otomoDestXLabel.setVisible(v); otomoDestXEditor.setVisible(v); otomoDestXUnitLabel.setVisible(v);
        otomoDestYLabel.setVisible(v); otomoDestYEditor.setVisible(v); otomoDestYUnitLabel.setVisible(v);
        otomoDestZLabel.setVisible(v); otomoDestZEditor.setVisible(v); otomoDestZUnitLabel.setVisible(v);
        otomoAbsRelButton.setVisible(v);
        otomoStayReturnButton.setVisible(v);
        otomoSpeedProfileLabel.setVisible(v); otomoSpeedProfileDial.setVisible(v); otomoSpeedProfileValueLabel.setVisible(v);
        otomoTriggerButton.setVisible(v);
        otomoThresholdLabel.setVisible(v); otomoThresholdDial.setVisible(v); otomoThresholdValueLabel.setVisible(v);
        otomoResetLabel.setVisible(v); otomoResetDial.setVisible(v); otomoResetValueLabel.setVisible(v);
        otomoStartButton.setVisible(v);
        otomoStopButton.setVisible(v);
        otomoPauseButton.setVisible(v);
    }

    void layoutAutomotionTab()
    {
        auto area = subTabContentArea;
        const int rowHeight = 30;
        const int spacing = 8;
        const int labelWidth = 70;
        const int editorWidth = 80;
        const int unitWidth = 25;
        const int buttonWidth = 100;
        const int dialSize = 70;
        const int transportButtonSize = 40;

        auto leftCol = area.removeFromLeft(area.getWidth() / 2).reduced(5, 0);
        auto rightCol = area.reduced(5, 0);

        // Destination X/Y/Z
        auto row = leftCol.removeFromTop(rowHeight);
        otomoDestXLabel.setBounds(row.removeFromLeft(labelWidth));
        otomoDestXEditor.setBounds(row.removeFromLeft(editorWidth));
        otomoDestXUnitLabel.setBounds(row.removeFromLeft(unitWidth));
        leftCol.removeFromTop(spacing);

        row = leftCol.removeFromTop(rowHeight);
        otomoDestYLabel.setBounds(row.removeFromLeft(labelWidth));
        otomoDestYEditor.setBounds(row.removeFromLeft(editorWidth));
        otomoDestYUnitLabel.setBounds(row.removeFromLeft(unitWidth));
        leftCol.removeFromTop(spacing);

        row = leftCol.removeFromTop(rowHeight);
        otomoDestZLabel.setBounds(row.removeFromLeft(labelWidth));
        otomoDestZEditor.setBounds(row.removeFromLeft(editorWidth));
        otomoDestZUnitLabel.setBounds(row.removeFromLeft(unitWidth));
        leftCol.removeFromTop(spacing * 2);

        // Buttons row
        row = leftCol.removeFromTop(rowHeight);
        otomoAbsRelButton.setBounds(row.removeFromLeft(buttonWidth));
        row.removeFromLeft(spacing);
        otomoStayReturnButton.setBounds(row.removeFromLeft(buttonWidth));
        leftCol.removeFromTop(spacing);

        row = leftCol.removeFromTop(rowHeight);
        otomoTriggerButton.setBounds(row.removeFromLeft(buttonWidth));
        leftCol.removeFromTop(spacing * 2);

        // Transport buttons
        row = leftCol.removeFromTop(transportButtonSize);
        otomoStartButton.setBounds(row.removeFromLeft(transportButtonSize));
        row.removeFromLeft(spacing);
        otomoPauseButton.setBounds(row.removeFromLeft(transportButtonSize));
        row.removeFromLeft(spacing);
        otomoStopButton.setBounds(row.removeFromLeft(transportButtonSize));

        // Right column - Dials
        otomoSpeedProfileLabel.setBounds(rightCol.removeFromTop(rowHeight));
        auto dialArea = rightCol.removeFromTop(dialSize);
        otomoSpeedProfileDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
        otomoSpeedProfileValueLabel.setBounds(rightCol.removeFromTop(rowHeight));
        rightCol.removeFromTop(spacing);

        otomoThresholdLabel.setBounds(rightCol.removeFromTop(rowHeight));
        dialArea = rightCol.removeFromTop(dialSize);
        otomoThresholdDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
        otomoThresholdValueLabel.setBounds(rightCol.removeFromTop(rowHeight));
        rightCol.removeFromTop(spacing);

        otomoResetLabel.setBounds(rightCol.removeFromTop(rowHeight));
        dialArea = rightCol.removeFromTop(dialSize);
        otomoResetDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
        otomoResetValueLabel.setBounds(rightCol.removeFromTop(rowHeight));
    }

    void setMutesVisible(bool v)
    {
        for (int i = 0; i < 64; ++i)
            muteButtons[i].setVisible(v);
        muteMacrosLabel.setVisible(v);
        muteMacrosSelector.setVisible(v);
    }

    void layoutMutesTab()
    {
        auto area = subTabContentArea;
        const int buttonSize = 35;
        const int gridSpacing = 3;
        const int rowHeight = 30;
        const int selectorWidth = 200;

        // Mute grid - 8x8
        auto gridArea = area.removeFromTop(8 * (buttonSize + gridSpacing));

        for (int row = 0; row < 8; ++row)
        {
            auto rowArea = gridArea.removeFromTop(buttonSize + gridSpacing);
            for (int col = 0; col < 8; ++col)
            {
                int index = row * 8 + col;
                muteButtons[index].setBounds(rowArea.removeFromLeft(buttonSize));
                rowArea.removeFromLeft(gridSpacing);
            }
        }

        area.removeFromTop(20);

        // Mute Macros selector
        auto row = area.removeFromTop(rowHeight);
        muteMacrosLabel.setBounds(row.removeFromLeft(100));
        muteMacrosSelector.setBounds(row.removeFromLeft(selectorWidth));
    }

    // ==================== PARAMETER MANAGEMENT ====================

    void loadChannelParameters(int channel)
    {
        currentChannel = channel;
        nameEditor.setText("Input " + juce::String(channel), juce::dontSendNotification);
        clusterSelector.setSelectedId(1, juce::dontSendNotification);
    }

    // ==================== TEXT EDITOR LISTENER ====================

    void textEditorReturnKeyPressed(juce::TextEditor& editor) override
    {
        editor.giveAwayKeyboardFocus();
    }

    void textEditorFocusLost(juce::TextEditor&) override {}

    // ==================== LABEL LISTENER ====================

    void labelTextChanged(juce::Label* label) override
    {
        juce::String text = label->getText();
        float value = text.retainCharacters("-0123456789.").getFloatValue();

        // Input Properties tab
        if (label == &attenuationValueLabel)
        {
            float dB = juce::jlimit(-92.0f, 0.0f, value);
            float minLinear = std::pow(10.0f, -92.0f / 20.0f);
            float targetLinear = std::pow(10.0f, dB / 20.0f);
            float v = std::sqrt((targetLinear - minLinear) / (1.0f - minLinear));
            attenuationSlider.setValue(juce::jlimit(0.0f, 1.0f, v));
        }
        else if (label == &delayLatencyValueLabel)
        {
            float ms = juce::jlimit(-100.0f, 100.0f, value);
            delayLatencySlider.setValue(ms / 100.0f);
        }
        // Position tab
        else if (label == &trackingSmoothValueLabel)
        {
            int percent = juce::jlimit(0, 100, static_cast<int>(value));
            trackingSmoothDial.setValue(percent / 100.0f);
        }
        else if (label == &maxSpeedValueLabel)
        {
            float speed = juce::jlimit(0.01f, 10.0f, value);
            // Inverse of: speed = 0.01 + v * 9.99
            maxSpeedDial.setValue((speed - 0.01f) / 9.99f);
        }
        else if (label == &heightFactorValueLabel)
        {
            int percent = juce::jlimit(0, 100, static_cast<int>(value));
            heightFactorDial.setValue(percent / 100.0f);
        }
        // Sound tab
        else if (label == &distanceAttenValueLabel)
        {
            float dBm = juce::jlimit(-12.0f, 0.0f, value);
            // Inverse of: dBm = v * 12.0 - 12.0
            distanceAttenDial.setValue((dBm + 12.0f) / 12.0f);
        }
        else if (label == &distanceRatioValueLabel)
        {
            float ratio = juce::jlimit(0.0f, 2.0f, value);
            // Inverse of: ratio = v * 2.0
            distanceRatioDial.setValue(ratio / 2.0f);
        }
        else if (label == &commonAttenValueLabel)
        {
            int percent = juce::jlimit(0, 100, static_cast<int>(value));
            commonAttenDial.setValue(percent / 100.0f);
        }
        else if (label == &directivityValueLabel)
        {
            int degrees = juce::jlimit(1, 360, static_cast<int>(value));
            // Inverse of: degrees = v * 359 + 1
            directivitySlider.setValue((degrees - 1.0f) / 359.0f);
        }
        else if (label == &rotationValueLabel)
        {
            rotationDial.setAngle(value);
        }
        else if (label == &tiltValueLabel)
        {
            int degrees = juce::jlimit(-90, 90, static_cast<int>(value));
            // Inverse of: degrees = v * 180 - 90
            tiltSlider.setValue((degrees + 90.0f) / 180.0f);
        }
        else if (label == &hfShelfValueLabel)
        {
            float dB = juce::jlimit(-12.0f, 0.0f, value);
            // Inverse of: dB = v * 12.0 - 12.0
            hfShelfSlider.setValue((dB + 12.0f) / 12.0f);
        }
        // Live Source tab
        else if (label == &lsRadiusValueLabel)
        {
            float meters = juce::jlimit(0.0f, 20.0f, value);
            // Inverse of: meters = v * 20.0
            lsRadiusSlider.setValue(meters / 20.0f);
        }
        else if (label == &lsAttenuationValueLabel)
        {
            float dB = juce::jlimit(-92.0f, 0.0f, value);
            float minLinear = std::pow(10.0f, -92.0f / 20.0f);
            float targetLinear = std::pow(10.0f, dB / 20.0f);
            float v = std::sqrt((targetLinear - minLinear) / (1.0f - minLinear));
            lsAttenuationSlider.setValue(juce::jlimit(0.0f, 1.0f, v));
        }
        else if (label == &lsPeakThresholdValueLabel)
        {
            float dB = juce::jlimit(-60.0f, 0.0f, value);
            // Inverse of: dB = v * 60.0 - 60.0
            lsPeakThresholdSlider.setValue((dB + 60.0f) / 60.0f);
        }
        else if (label == &lsPeakRatioValueLabel)
        {
            float ratio = juce::jlimit(1.0f, 10.0f, value);
            // Inverse of: ratio = v * 9.0 + 1.0
            lsPeakRatioDial.setValue((ratio - 1.0f) / 9.0f);
        }
        else if (label == &lsSlowThresholdValueLabel)
        {
            float dB = juce::jlimit(-60.0f, 0.0f, value);
            lsSlowThresholdSlider.setValue((dB + 60.0f) / 60.0f);
        }
        else if (label == &lsSlowRatioValueLabel)
        {
            float ratio = juce::jlimit(1.0f, 10.0f, value);
            lsSlowRatioDial.setValue((ratio - 1.0f) / 9.0f);
        }
        // Effects/Hackoustics tab
        else if (label == &frAttenuationValueLabel)
        {
            float dB = juce::jlimit(-60.0f, 0.0f, value);
            // Inverse of: dB = v * 60.0 - 60.0
            frAttenuationSlider.setValue((dB + 60.0f) / 60.0f);
        }
        else if (label == &frDiffusionValueLabel)
        {
            int percent = juce::jlimit(0, 100, static_cast<int>(value));
            frDiffusionDial.setValue(percent / 100.0f);
        }
        else if (label == &frLowCutFreqValueLabel)
        {
            int freq = juce::jlimit(20, 1000, static_cast<int>(value));
            // Inverse of: freq = 20 + (v^2) * 980
            float v = std::sqrt((freq - 20.0f) / 980.0f);
            frLowCutFreqSlider.setValue(v);
        }
        else if (label == &frHighShelfFreqValueLabel)
        {
            int freq = juce::jlimit(1000, 10000, static_cast<int>(value));
            // Inverse of: freq = 1000 + v^2 * 9000
            float v = std::sqrt((freq - 1000.0f) / 9000.0f);
            frHighShelfFreqSlider.setValue(v);
        }
        else if (label == &frHighShelfGainValueLabel)
        {
            float dB = juce::jlimit(-12.0f, 0.0f, value);
            // Inverse of: dB = v * 12.0 - 12.0
            frHighShelfGainSlider.setValue((dB + 12.0f) / 12.0f);
        }
        else if (label == &frHighShelfSlopeValueLabel)
        {
            float slope = juce::jlimit(0.1f, 1.0f, value);
            // Inverse of: slope = v * 0.9 + 0.1
            frHighShelfSlopeSlider.setValue((slope - 0.1f) / 0.9f);
        }
        else if (label == &jitterValueLabel)
        {
            float meters = juce::jlimit(0.0f, 1.0f, value);
            jitterSlider.setValue(meters);
        }
        // LFO tab
        else if (label == &lfoPeriodValueLabel)
        {
            float period = juce::jlimit(0.1f, 60.0f, value);
            // Inverse of: period = 0.1 + v^2 * 59.9
            float v = std::sqrt((period - 0.1f) / 59.9f);
            lfoPeriodDial.setValue(v);
        }
        else if (label == &lfoPhaseValueLabel)
        {
            int degrees = juce::jlimit(0, 359, static_cast<int>(value));
            // WfsRotationDial uses -180 to 180 range, convert from 0-360
            float angle = (degrees <= 180) ? static_cast<float>(degrees) : static_cast<float>(degrees - 360);
            lfoPhaseDial.setAngle(angle);
        }
        else if (label == &lfoRateXValueLabel)
        {
            float rate = juce::jlimit(0.0f, 10.0f, value);
            lfoRateXSlider.setValue(rate / 10.0f);
        }
        else if (label == &lfoRateYValueLabel)
        {
            float rate = juce::jlimit(0.0f, 10.0f, value);
            lfoRateYSlider.setValue(rate / 10.0f);
        }
        else if (label == &lfoRateZValueLabel)
        {
            float rate = juce::jlimit(0.0f, 10.0f, value);
            lfoRateZSlider.setValue(rate / 10.0f);
        }
        else if (label == &lfoAmplitudeXValueLabel)
        {
            float amp = juce::jlimit(0.0f, 10.0f, value);
            lfoAmplitudeXSlider.setValue(amp / 10.0f);
        }
        else if (label == &lfoAmplitudeYValueLabel)
        {
            float amp = juce::jlimit(0.0f, 10.0f, value);
            lfoAmplitudeYSlider.setValue(amp / 10.0f);
        }
        else if (label == &lfoAmplitudeZValueLabel)
        {
            float amp = juce::jlimit(0.0f, 10.0f, value);
            lfoAmplitudeZSlider.setValue(amp / 10.0f);
        }
        else if (label == &lfoPhaseXValueLabel)
        {
            int degrees = juce::jlimit(0, 359, static_cast<int>(value));
            float angle = (degrees <= 180) ? static_cast<float>(degrees) : static_cast<float>(degrees - 360);
            lfoPhaseXDial.setAngle(angle);
        }
        else if (label == &lfoPhaseYValueLabel)
        {
            int degrees = juce::jlimit(0, 359, static_cast<int>(value));
            float angle = (degrees <= 180) ? static_cast<float>(degrees) : static_cast<float>(degrees - 360);
            lfoPhaseYDial.setAngle(angle);
        }
        else if (label == &lfoPhaseZValueLabel)
        {
            int degrees = juce::jlimit(0, 359, static_cast<int>(value));
            float angle = (degrees <= 180) ? static_cast<float>(degrees) : static_cast<float>(degrees - 360);
            lfoPhaseZDial.setAngle(angle);
        }
        // AutomOtion tab
        else if (label == &otomoSpeedProfileValueLabel)
        {
            int percent = juce::jlimit(0, 100, static_cast<int>(value));
            otomoSpeedProfileDial.setValue(percent / 100.0f);
        }
        else if (label == &otomoThresholdValueLabel)
        {
            float dB = juce::jlimit(-60.0f, 0.0f, value);
            otomoThresholdDial.setValue((dB + 60.0f) / 60.0f);
        }
        else if (label == &otomoResetValueLabel)
        {
            float dB = juce::jlimit(-80.0f, -20.0f, value);
            // Inverse of: dB = v * 60.0 - 80.0
            otomoResetDial.setValue((dB + 80.0f) / 60.0f);
        }
    }

    // ==================== STORE/RELOAD METHODS ====================

    void storeInputConfiguration() {}
    void reloadInputConfiguration() {}
    void reloadInputConfigBackup() {}
    void importInputConfiguration() {}
    void exportInputConfiguration() {}
    void storeNewSnapshot() {}
    void reloadSnapshot() {}
    void updateSnapshot() {}
    void editSnapshotScope() {}
    void deleteSnapshot() {}

    //==============================================================================
    // Status bar helper methods

    void setupHelpText()
    {
        helpTextMap[&channelSelector] = "Input Channel Number and Selection.";
        helpTextMap[&nameEditor] = "Displayed Input Channel Name (editable).";
        helpTextMap[&clusterSelector] = "Object is Part of a Cluster.";
        helpTextMap[&attenuationSlider] = "Input Channel Attenuation.";
        helpTextMap[&delayLatencySlider] = "Input Channel Delay (positive values) or Latency Compensation (negative values).";
        helpTextMap[&minimalLatencyButton] = "Select between Acoustic Precedence and Minimal Latency for Amplification Precedence.";
        helpTextMap[&posXEditor] = "Object Position in Width. Nudge with Left and Right Arrow Keys.";
        helpTextMap[&posYEditor] = "Object Position in Depth. Nudge with Up and Down Arrow Keys.";
        helpTextMap[&posZEditor] = "Object Position in Height. Nudge with Page Up and Page Down Keys.";
        helpTextMap[&offsetXEditor] = "Object Position Offset in Width. Adjusted when Tracking is Enabled.";
        helpTextMap[&offsetYEditor] = "Object Position Offset in Depth. Adjusted when Tracking is Enabled.";
        helpTextMap[&offsetZEditor] = "Object Position Offset in Height. Adjusted when Tracking is Enabled.";
        helpTextMap[&constraintXButton] = "Limit Position to the Bounds of the Stage in Width.";
        helpTextMap[&constraintYButton] = "Limit Position to the Bounds of the Stage in Depth.";
        helpTextMap[&constraintZButton] = "Limit Position to the Bounds of the Stage in Height.";
        helpTextMap[&flipXButton] = "X will be Symetrical to the Origin. Keyboard Nudging will be Inverted.";
        helpTextMap[&flipYButton] = "Y will be Symetrical to the Origin. Keyboard Nudging will be Inverted.";
        helpTextMap[&flipZButton] = "Z will be Symetrical to the Origin. Keyboard Nudging will be Inverted.";
        helpTextMap[&trackingActiveButton] = "Enable or Disable Tracking for Object.";
        helpTextMap[&trackingIdSelector] = "Tracker ID for Object.";
        helpTextMap[&trackingSmoothDial] = "Smoothing of Tracking Data for Object.";
        helpTextMap[&maxSpeedActiveButton] = "Enable or Disable Speed Limiting for Object.";
        helpTextMap[&maxSpeedDial] = "Maximum Speed Limit for Object.";
        helpTextMap[&heightFactorDial] = "Take Elevation of Object into Account Fully, Partially or Not.";
        helpTextMap[&attenuationLawButton] = "Attenuation Law Model (Linear Decrease of Volume with Distance Between Object and Speaker or Squared).";
        helpTextMap[&distanceAttenDial] = "Attenuation per Meter Between Object and Speaker.";
        helpTextMap[&distanceRatioDial] = "Attenuation Ratio for Squared Model.";
        helpTextMap[&commonAttenDial] = "Percentage of the Common Part of the Attenuation for selected Object Relative to All Outputs.";
        helpTextMap[&directivitySlider] = "How Wide is the Brightness of The Object.";
        helpTextMap[&rotationDial] = "Where is the Object pointing to in the Horizontal Plane.";
        helpTextMap[&tiltSlider] = "Where is the Object pointing to in the Vertical Plane.";
        helpTextMap[&hfShelfSlider] = "How Much Brightness is lost in the Back of the Object, Out of its Brightness Cone.";
        helpTextMap[&lsActiveButton] = "If You Need to Reduce the Level in Speakers Close to the Object. (eg. Loud Source Present on Stage)";
        helpTextMap[&lsRadiusSlider] = "How Far does the Attenuation Affect The Speakers.";
        helpTextMap[&lsShapeSelector] = "Profile of the Attenuation Around the Object.";
        helpTextMap[&lsAttenuationSlider] = "Constant Attenuation of Speakers Around the Object.";
        helpTextMap[&lsPeakThresholdSlider] = "Fast Compression Threshold for Speakers Around the Object to Control Transients.";
        helpTextMap[&lsPeakRatioDial] = "Ratio to Apply the Fast Compression for Speakers Around the Object.";
        helpTextMap[&lsSlowThresholdSlider] = "Slow Compression Threshold for Speakers Around the Object to Control Sustained Level.";
        helpTextMap[&lsSlowRatioDial] = "Ratio to Apply the Slow Compression for Speakers Around the Object.";
        helpTextMap[&frActiveButton] = "Enable Simulated Floor Reflections for the Object.";
        helpTextMap[&frAttenuationSlider] = "Attenuation of the Simulated Floor Reflections for the Object.";
        helpTextMap[&frDiffusionDial] = "Diffusion Effect of the Simulated Floor Reflections for the Object.";
        helpTextMap[&frLowCutActiveButton] = "Enable Low Cut Filter for Floor Reflections.";
        helpTextMap[&frLowCutFreqSlider] = "Low Cut Frequency for Floor Reflections.";
        helpTextMap[&frHighShelfActiveButton] = "Enable High Shelf Filter for Floor Reflections.";
        helpTextMap[&frHighShelfFreqSlider] = "High Shelf Frequency for Floor Reflections.";
        helpTextMap[&frHighShelfGainSlider] = "High Shelf Gain for Floor Reflections.";
        helpTextMap[&frHighShelfSlopeSlider] = "High Shelf Slope for Floor Reflections.";
        helpTextMap[&jitterSlider] = "Sphere of Rapid Movements of the Object.";
        // LFO tab
        helpTextMap[&lfoActiveButton] = "Enable or Disable the Periodic Movement of the Object (LFO).";
        helpTextMap[&lfoPeriodDial] = "Base Period of the Movement of the Object.";
        helpTextMap[&lfoPhaseDial] = "Phase Offset of the Movement of the Object.";
        helpTextMap[&lfoShapeXSelector] = "Movement Behaviour of the Object in Width.";
        helpTextMap[&lfoShapeYSelector] = "Movement Behaviour of the Object in Depth.";
        helpTextMap[&lfoShapeZSelector] = "Movement Behaviour of the Object in Height.";
        helpTextMap[&lfoRateXSlider] = "Faster or Slower Movement in Relation to Base Period in Width.";
        helpTextMap[&lfoRateYSlider] = "Faster or Slower Movement in Relation to Base Period in Depth.";
        helpTextMap[&lfoRateZSlider] = "Faster or Slower Movement in Relation to Base Period in Height.";
        helpTextMap[&lfoAmplitudeXSlider] = "Width of Movement in Relation to Base Position of the Object.";
        helpTextMap[&lfoAmplitudeYSlider] = "Depth of Movement in Relation to Base Position of the Object.";
        helpTextMap[&lfoAmplitudeZSlider] = "Height of Movement in Relation to Base Position of the Object.";
        helpTextMap[&lfoPhaseXDial] = "Phase Offset of the Movement of the Object in Width.";
        helpTextMap[&lfoPhaseYDial] = "Phase Offset of the Movement of the Object in Depth.";
        helpTextMap[&lfoPhaseZDial] = "Phase Offset of the Movement of the Object in Height.";
        helpTextMap[&lfoGyrophoneSelector] = "Rotation of the Brightness Cone of the Object.";
        // AutomOtion tab
        helpTextMap[&otomoDestXEditor] = "Relative or Absolute Destination X.";
        helpTextMap[&otomoDestYEditor] = "Relative or Absolute Destination Y.";
        helpTextMap[&otomoDestZEditor] = "Relative or Absolute Destination Z.";
        helpTextMap[&otomoAbsRelButton] = "Select Relative or Absolute Coordinates of Displacement.";
        helpTextMap[&otomoStayReturnButton] = "At the End of the Movement, should the Source Stay or Return to the Original Position.";
        helpTextMap[&otomoSpeedProfileDial] = "Constant Speed or Gradual Acceleration and Slow Down at the Start and the End of the Movement.";
        helpTextMap[&otomoTriggerButton] = "Manual Start of Displacement or Automatic Trigger on the Audio Level.";
        helpTextMap[&otomoThresholdDial] = "Set the Threshold for the Automatic Trigger of the Movement.";
        helpTextMap[&otomoResetDial] = "Set the Reset Level for the Automatic Trigger.";
        helpTextMap[&otomoStartButton] = "Start the Movement Manually.";
        helpTextMap[&otomoStopButton] = "Stop the Movement.";
        helpTextMap[&otomoPauseButton] = "Pause and Resume the Movement.";
        // Mutes tab
        for (int i = 0; i < 64; ++i)
            helpTextMap[&muteButtons[i]] = "Mute Output " + juce::String(i + 1) + " for this Object.";
        helpTextMap[&muteMacrosSelector] = "Mute Macros for Fast Muting and Unmuting of Arrays.";
        helpTextMap[&storeButton] = "Store Input Configuration to file (overwrite with confirmation).";
        helpTextMap[&reloadButton] = "Reload Input Configuration from file (with confirmation).";
        helpTextMap[&reloadBackupButton] = "Reload Input Configuration from backup file (with confirmation).";
        helpTextMap[&importButton] = "Import Input Configuration from file (with file explorer window).";
        helpTextMap[&exportButton] = "Export Input Configuration to file (with file explorer window).";
        helpTextMap[&storeSnapshotButton] = "Store new Input Snapshot for All Objects.";
        helpTextMap[&snapshotSelector] = "Select Input Snapshot Without Loading.";
        helpTextMap[&reloadSnapshotButton] = "Reload Selected Input Snapshot for All Objects Taking the Scope into Account.";
        helpTextMap[&updateSnapshotButton] = "Update Selected Input Snapshot (with confirmation).";
        helpTextMap[&editScopeButton] = "Open Selected Input Snapshot Scope Window.";
        helpTextMap[&deleteSnapshotButton] = "Delete Selected Input Snapshot With Confirmation.";
    }

    void setupOscMethods()
    {
        oscMethodMap[&channelSelector] = "/wfs/input/selected <ID>";
        oscMethodMap[&nameEditor] = "/wfs/input/name <ID> <value>";
        oscMethodMap[&clusterSelector] = "/wfs/input/cluster <ID> <value>";
        oscMethodMap[&attenuationSlider] = "/wfs/input/attenuation <ID> <value>";
        oscMethodMap[&delayLatencySlider] = "/wfs/input/delayLatency <ID> <value>";
        oscMethodMap[&minimalLatencyButton] = "/wfs/input/minimalLatency <ID> <value>";
        oscMethodMap[&posXEditor] = "/wfs/input/positionX <ID> <value>";
        oscMethodMap[&posYEditor] = "/wfs/input/positionY <ID> <value>";
        oscMethodMap[&posZEditor] = "/wfs/input/positionZ <ID> <value>";
        oscMethodMap[&offsetXEditor] = "/wfs/input/offsetX <ID> <value>";
        oscMethodMap[&offsetYEditor] = "/wfs/input/offsetY <ID> <value>";
        oscMethodMap[&offsetZEditor] = "/wfs/input/offsetZ <ID> <value>";
        oscMethodMap[&constraintXButton] = "/wfs/input/constraintX <ID> <value>";
        oscMethodMap[&constraintYButton] = "/wfs/input/constraintY <ID> <value>";
        oscMethodMap[&constraintZButton] = "/wfs/input/constraintZ <ID> <value>";
        oscMethodMap[&flipXButton] = "/wfs/input/flipX <ID> <value>";
        oscMethodMap[&flipYButton] = "/wfs/input/flipY <ID> <value>";
        oscMethodMap[&flipZButton] = "/wfs/input/flipZ <ID> <value>";
        oscMethodMap[&trackingActiveButton] = "/wfs/input/trackingActive <ID> <value>";
        oscMethodMap[&trackingIdSelector] = "/wfs/input/trackingID <ID> <value>";
        oscMethodMap[&trackingSmoothDial] = "/wfs/input/trackingSmooth <ID> <value>";
        oscMethodMap[&maxSpeedActiveButton] = "/wfs/input/maxSpeedActive <ID> <value>";
        oscMethodMap[&maxSpeedDial] = "/wfs/input/maxSpeed <ID> <value>";
        oscMethodMap[&heightFactorDial] = "/wfs/input/heightFactor <ID> <value>";
        oscMethodMap[&attenuationLawButton] = "/wfs/input/attenuationLaw <ID> <value>";
        oscMethodMap[&distanceAttenDial] = "/wfs/input/distanceAttenuation <ID> <value>";
        oscMethodMap[&distanceRatioDial] = "/wfs/input/distanceRatio <ID> <value>";
        oscMethodMap[&commonAttenDial] = "/wfs/input/commonAtten <ID> <value>";
        oscMethodMap[&directivitySlider] = "/wfs/input/directivity <ID> <value>";
        oscMethodMap[&rotationDial] = "/wfs/input/rotation <ID> <value>";
        oscMethodMap[&tiltSlider] = "/wfs/input/tilt <ID> <value>";
        oscMethodMap[&hfShelfSlider] = "/wfs/input/HFshelf <ID> <value>";
        oscMethodMap[&lsActiveButton] = "/wfs/input/LSactive <ID> <value>";
        oscMethodMap[&lsRadiusSlider] = "/wfs/input/LSradius <ID> <value>";
        oscMethodMap[&lsShapeSelector] = "/wfs/input/LSshape <ID> <value>";
        oscMethodMap[&lsAttenuationSlider] = "/wfs/input/LSattenuation <ID> <value>";
        oscMethodMap[&lsPeakThresholdSlider] = "/wfs/input/LSpeakThreshold <ID> <value>";
        oscMethodMap[&lsPeakRatioDial] = "/wfs/input/LSpeakRatio <ID> <value>";
        oscMethodMap[&lsSlowThresholdSlider] = "/wfs/input/LSslowThreshold <ID> <value>";
        oscMethodMap[&lsSlowRatioDial] = "/wfs/input/LSslowRatio <ID> <value>";
        oscMethodMap[&frActiveButton] = "/wfs/input/FRactive <ID> <value>";
        oscMethodMap[&frAttenuationSlider] = "/wfs/input/FRattenuation <ID> <value>";
        oscMethodMap[&frDiffusionDial] = "/wfs/input/FRdiffusion <ID> <value>";
        oscMethodMap[&frLowCutActiveButton] = "/wfs/input/FRlowCutActive <ID> <value>";
        oscMethodMap[&frLowCutFreqSlider] = "/wfs/input/FRlowCutFreq <ID> <value>";
        oscMethodMap[&frHighShelfActiveButton] = "/wfs/input/FRhighShelfActive <ID> <value>";
        oscMethodMap[&frHighShelfFreqSlider] = "/wfs/input/FRhighShelfFreq <ID> <value>";
        oscMethodMap[&frHighShelfGainSlider] = "/wfs/input/FRhighShelfGain <ID> <value>";
        oscMethodMap[&frHighShelfSlopeSlider] = "/wfs/input/FRhighShelfSlope <ID> <value>";
        oscMethodMap[&jitterSlider] = "/wfs/input/jitter <ID> <value>";
        // LFO tab
        oscMethodMap[&lfoActiveButton] = "/wfs/input/LFOactive <ID> <value>";
        oscMethodMap[&lfoPeriodDial] = "/wfs/input/LFOperiod <ID> <value>";
        oscMethodMap[&lfoPhaseDial] = "/wfs/input/LFOphase <ID> <value>";
        oscMethodMap[&lfoShapeXSelector] = "/wfs/input/LFOshapeX <ID> <value>";
        oscMethodMap[&lfoShapeYSelector] = "/wfs/input/LFOshapeY <ID> <value>";
        oscMethodMap[&lfoShapeZSelector] = "/wfs/input/LFOshapeZ <ID> <value>";
        oscMethodMap[&lfoRateXSlider] = "/wfs/input/LFOrateX <ID> <value>";
        oscMethodMap[&lfoRateYSlider] = "/wfs/input/LFOrateY <ID> <value>";
        oscMethodMap[&lfoRateZSlider] = "/wfs/input/LFOrateZ <ID> <value>";
        oscMethodMap[&lfoAmplitudeXSlider] = "/wfs/input/LFOamplitudeX <ID> <value>";
        oscMethodMap[&lfoAmplitudeYSlider] = "/wfs/input/LFOamplitudeY <ID> <value>";
        oscMethodMap[&lfoAmplitudeZSlider] = "/wfs/input/LFOamplitudeZ <ID> <value>";
        oscMethodMap[&lfoPhaseXDial] = "/wfs/input/LFOphaseX <ID> <value>";
        oscMethodMap[&lfoPhaseYDial] = "/wfs/input/LFOphaseY <ID> <value>";
        oscMethodMap[&lfoPhaseZDial] = "/wfs/input/LFOphaseZ <ID> <value>";
        oscMethodMap[&lfoGyrophoneSelector] = "/wfs/input/LFOgyrophone <ID> <value>";
        // AutomOtion tab
        oscMethodMap[&otomoDestXEditor] = "/wfs/input/otomoX <ID> <value>";
        oscMethodMap[&otomoDestYEditor] = "/wfs/input/otomoY <ID> <value>";
        oscMethodMap[&otomoDestZEditor] = "/wfs/input/otomoZ <ID> <value>";
        oscMethodMap[&otomoAbsRelButton] = "/wfs/input/otomoAbsoluteRelative <ID> <value>";
        oscMethodMap[&otomoStayReturnButton] = "/wfs/input/otomoStayReturn <ID> <value>";
        oscMethodMap[&otomoSpeedProfileDial] = "/wfs/input/otomoSpeed <ID> <value>";
        oscMethodMap[&otomoTriggerButton] = "/wfs/input/otomoTrigger <ID> <value>";
        oscMethodMap[&otomoThresholdDial] = "/wfs/input/otomoTriggerThreshold <ID> <value>";
        oscMethodMap[&otomoResetDial] = "/wfs/input/otomoTriggerReset <ID> <value>";
        oscMethodMap[&otomoStartButton] = "/wfs/input/otomoStart <ID>";
        oscMethodMap[&otomoStopButton] = "/wfs/input/otomoResume <ID>";
        oscMethodMap[&otomoPauseButton] = "/wfs/input/otomoPause <ID>";
        // Mutes tab
        for (int i = 0; i < 64; ++i)
            oscMethodMap[&muteButtons[i]] = "/wfs/input/mutes <ID> " + juce::String(i + 1) + " <value>";
        oscMethodMap[&muteMacrosSelector] = "/wfs/input/muteMacro <ID> <value>";
    }

    void setupMouseListeners()
    {
        for (auto& pair : helpTextMap)
            pair.first->addMouseListener(this, false);
    }

    void mouseEnter(const juce::MouseEvent& event) override
    {
        if (statusBar == nullptr) return;
        auto* component = event.eventComponent;
        if (helpTextMap.find(component) != helpTextMap.end())
            statusBar->setHelpText(helpTextMap[component]);
        if (oscMethodMap.find(component) != oscMethodMap.end())
            statusBar->setOscMethod(oscMethodMap[component]);
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (statusBar != nullptr)
            statusBar->clearText();
    }

    // ==================== MEMBER VARIABLES ====================

    WfsParameters& parameters;
    StatusBar* statusBar = nullptr;
    std::map<juce::Component*, juce::String> helpTextMap;
    std::map<juce::Component*, juce::String> oscMethodMap;
    int currentChannel = 1;

    static constexpr int headerHeight = 60;
    static constexpr int footerHeight = 90;  // Two 30px button rows + 10px spacing + 20px padding
    juce::Rectangle<int> subTabContentArea;

    // Header components
    ChannelSelectorButton channelSelector { "Input" };
    juce::Label nameLabel;
    juce::TextEditor nameEditor;
    juce::Label clusterLabel;
    juce::ComboBox clusterSelector;

    // Sub-tab bar
    juce::TabbedButtonBar subTabBar { juce::TabbedButtonBar::TabsAtTop };

    // Input Properties tab
    juce::Label attenuationLabel;
    WfsStandardSlider attenuationSlider;
    juce::Label attenuationValueLabel;
    juce::Label delayLatencyLabel;
    WfsBidirectionalSlider delayLatencySlider;
    juce::Label delayLatencyValueLabel;
    juce::TextButton minimalLatencyButton;

    // Position tab
    juce::Label posXLabel, posYLabel, posZLabel;
    juce::TextEditor posXEditor, posYEditor, posZEditor;
    juce::Label posXUnitLabel, posYUnitLabel, posZUnitLabel;
    juce::Label offsetXLabel, offsetYLabel, offsetZLabel;
    juce::TextEditor offsetXEditor, offsetYEditor, offsetZEditor;
    juce::Label offsetXUnitLabel, offsetYUnitLabel, offsetZUnitLabel;
    juce::TextButton constraintXButton, constraintYButton, constraintZButton;
    juce::TextButton flipXButton, flipYButton, flipZButton;
    juce::TextButton trackingActiveButton;
    juce::Label trackingIdLabel;
    juce::ComboBox trackingIdSelector;
    juce::Label trackingSmoothLabel;
    WfsBasicDial trackingSmoothDial;
    juce::Label trackingSmoothValueLabel;
    juce::TextButton maxSpeedActiveButton;
    juce::Label maxSpeedLabel;
    WfsBasicDial maxSpeedDial;
    juce::Label maxSpeedValueLabel;
    juce::Label heightFactorLabel;
    WfsBasicDial heightFactorDial;
    juce::Label heightFactorValueLabel;

    // Sound tab
    juce::TextButton attenuationLawButton;
    juce::Label distanceAttenLabel;
    WfsBasicDial distanceAttenDial;
    juce::Label distanceAttenValueLabel;
    juce::Label distanceRatioLabel;
    WfsBasicDial distanceRatioDial;
    juce::Label distanceRatioValueLabel;
    juce::Label commonAttenLabel;
    WfsBasicDial commonAttenDial;
    juce::Label commonAttenValueLabel;
    juce::Label directivityLabel;
    WfsWidthExpansionSlider directivitySlider;
    juce::Label directivityValueLabel;
    juce::Label rotationLabel;
    WfsEndlessDial rotationDial;
    juce::Label rotationValueLabel;
    juce::Label tiltLabel;
    WfsBidirectionalSlider tiltSlider;
    juce::Label tiltValueLabel;
    juce::Label hfShelfLabel;
    WfsStandardSlider hfShelfSlider;
    juce::Label hfShelfValueLabel;

    // Live Source tab
    juce::TextButton lsActiveButton;
    juce::Label lsRadiusLabel;
    WfsWidthExpansionSlider lsRadiusSlider;
    juce::Label lsRadiusValueLabel;
    juce::Label lsShapeLabel;
    juce::ComboBox lsShapeSelector;
    juce::Label lsAttenuationLabel;
    WfsStandardSlider lsAttenuationSlider;
    juce::Label lsAttenuationValueLabel;
    juce::Label lsPeakThresholdLabel;
    WfsStandardSlider lsPeakThresholdSlider;
    juce::Label lsPeakThresholdValueLabel;
    juce::Label lsPeakRatioLabel;
    WfsBasicDial lsPeakRatioDial;
    juce::Label lsPeakRatioValueLabel;
    juce::Label lsSlowThresholdLabel;
    WfsStandardSlider lsSlowThresholdSlider;
    juce::Label lsSlowThresholdValueLabel;
    juce::Label lsSlowRatioLabel;
    WfsBasicDial lsSlowRatioDial;
    juce::Label lsSlowRatioValueLabel;

    // Effects tab
    juce::TextButton frActiveButton;
    juce::Label frAttenuationLabel;
    WfsStandardSlider frAttenuationSlider;
    juce::Label frAttenuationValueLabel;
    juce::Label frDiffusionLabel;
    WfsBasicDial frDiffusionDial;
    juce::Label frDiffusionValueLabel;
    juce::TextButton frLowCutActiveButton;
    juce::Label frLowCutFreqLabel;
    WfsStandardSlider frLowCutFreqSlider;
    juce::Label frLowCutFreqValueLabel;
    juce::TextButton frHighShelfActiveButton;
    juce::Label frHighShelfFreqLabel;
    WfsStandardSlider frHighShelfFreqSlider;
    juce::Label frHighShelfFreqValueLabel;
    juce::Label frHighShelfGainLabel;
    WfsStandardSlider frHighShelfGainSlider;
    juce::Label frHighShelfGainValueLabel;
    juce::Label frHighShelfSlopeLabel;
    WfsStandardSlider frHighShelfSlopeSlider;
    juce::Label frHighShelfSlopeValueLabel;
    juce::Label jitterLabel;
    WfsWidthExpansionSlider jitterSlider;
    juce::Label jitterValueLabel;

    // L.F.O tab
    juce::TextButton lfoActiveButton;
    juce::Label lfoPeriodLabel;
    WfsBasicDial lfoPeriodDial;
    juce::Label lfoPeriodValueLabel;
    juce::Label lfoPhaseLabel;
    WfsRotationDial lfoPhaseDial;
    juce::Label lfoPhaseValueLabel;
    juce::Label lfoShapeXLabel, lfoShapeYLabel, lfoShapeZLabel;
    juce::ComboBox lfoShapeXSelector, lfoShapeYSelector, lfoShapeZSelector;
    juce::Label lfoRateXLabel, lfoRateYLabel, lfoRateZLabel;
    WfsStandardSlider lfoRateXSlider, lfoRateYSlider, lfoRateZSlider;
    juce::Label lfoRateXValueLabel, lfoRateYValueLabel, lfoRateZValueLabel;
    juce::Label lfoAmplitudeXLabel, lfoAmplitudeYLabel, lfoAmplitudeZLabel;
    WfsStandardSlider lfoAmplitudeXSlider, lfoAmplitudeYSlider, lfoAmplitudeZSlider;
    juce::Label lfoAmplitudeXValueLabel, lfoAmplitudeYValueLabel, lfoAmplitudeZValueLabel;
    juce::Label lfoPhaseXLabel, lfoPhaseYLabel, lfoPhaseZLabel;
    WfsRotationDial lfoPhaseXDial, lfoPhaseYDial, lfoPhaseZDial;
    juce::Label lfoPhaseXValueLabel, lfoPhaseYValueLabel, lfoPhaseZValueLabel;
    juce::Label lfoGyrophoneLabel;
    juce::ComboBox lfoGyrophoneSelector;

    // AutomOtion tab
    juce::Label otomoDestXLabel, otomoDestYLabel, otomoDestZLabel;
    juce::TextEditor otomoDestXEditor, otomoDestYEditor, otomoDestZEditor;
    juce::Label otomoDestXUnitLabel, otomoDestYUnitLabel, otomoDestZUnitLabel;
    juce::TextButton otomoAbsRelButton;
    juce::TextButton otomoStayReturnButton;
    juce::Label otomoSpeedProfileLabel;
    WfsBasicDial otomoSpeedProfileDial;
    juce::Label otomoSpeedProfileValueLabel;
    juce::TextButton otomoTriggerButton;
    juce::Label otomoThresholdLabel;
    WfsBasicDial otomoThresholdDial;
    juce::Label otomoThresholdValueLabel;
    juce::Label otomoResetLabel;
    WfsBasicDial otomoResetDial;
    juce::Label otomoResetValueLabel;
    juce::TextButton otomoStartButton;
    juce::TextButton otomoStopButton;
    juce::TextButton otomoPauseButton;

    // Mutes tab
    juce::TextButton muteButtons[64];
    juce::Label muteMacrosLabel;
    juce::ComboBox muteMacrosSelector;

    // Footer buttons - Config
    juce::TextButton storeButton;
    juce::TextButton reloadButton;
    juce::TextButton reloadBackupButton;
    juce::TextButton importButton;
    juce::TextButton exportButton;

    // Footer buttons - Snapshot
    juce::TextButton storeSnapshotButton;
    juce::ComboBox snapshotSelector;
    juce::TextButton reloadSnapshotButton;
    juce::TextButton updateSnapshotButton;
    juce::TextButton editScopeButton;
    juce::TextButton deleteSnapshotButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InputsTab)
};
