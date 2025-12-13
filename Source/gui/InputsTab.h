#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"
#include "../Parameters/WFSParameterIDs.h"
#include "ChannelSelector.h"
#include "SliderUIComponents.h"
#include "DialUIComponents.h"
#include "StatusBar.h"

//==============================================================================
// Custom Transport Button - Play (right-pointing triangle)
class PlayButton : public juce::Button
{
public:
    PlayButton() : juce::Button("Play") {}

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

        // Draw play triangle
        auto iconBounds = bounds.reduced(10.0f);
        juce::Path triangle;
        triangle.addTriangle(
            iconBounds.getX(), iconBounds.getY(),
            iconBounds.getX(), iconBounds.getBottom(),
            iconBounds.getRight(), iconBounds.getCentreY());

        g.setColour(juce::Colours::white);
        g.fillPath(triangle);
    }
};

//==============================================================================
// Custom Transport Button - Stop (square)
class StopButton : public juce::Button
{
public:
    StopButton() : juce::Button("Stop") {}

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

        // Draw stop square
        auto iconBounds = bounds.reduced(10.0f);
        g.setColour(juce::Colours::white);
        g.fillRect(iconBounds);
    }
};

//==============================================================================
// Custom Transport Button - Pause (two vertical bars)
class PauseButton : public juce::Button
{
public:
    PauseButton() : juce::Button("Pause") {}

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);

        // Background - toggle state affects color
        if (shouldDrawButtonAsDown || getToggleState())
            g.setColour(juce::Colour(0xFF505050));
        else if (shouldDrawButtonAsHighlighted)
            g.setColour(juce::Colour(0xFF353535));
        else
            g.setColour(juce::Colour(0xFF2A2A2A));

        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(juce::Colour(0xFF606060));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        // Draw pause bars (two vertical rectangles)
        auto iconBounds = bounds.reduced(10.0f);
        float barWidth = iconBounds.getWidth() * 0.3f;
        float gap = iconBounds.getWidth() * 0.4f;

        g.setColour(juce::Colours::white);
        g.fillRect(iconBounds.getX(), iconBounds.getY(), barWidth, iconBounds.getHeight());
        g.fillRect(iconBounds.getX() + barWidth + gap, iconBounds.getY(), barWidth, iconBounds.getHeight());
    }
};

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
                  private juce::Label::Listener,
                  private juce::ValueTree::Listener
{
public:
    InputsTab(WfsParameters& params)
        : parameters(params),
          inputsTree(params.getInputTree()),
          configTree(params.getConfigTree())
    {
        // Add listener to inputs tree and config tree
        inputsTree.addListener(this);
        configTree.addListener(this);

        // ==================== HEADER SECTION ====================
        // Channel Selector - use configured input count
        int numInputs = parameters.getNumInputChannels();
        channelSelector.setNumChannels(numInputs > 0 ? numInputs : 8);  // Default to 8 if not set
        channelSelector.onChannelChanged = [this](int channel) {
            loadChannelParameters(channel);
            // Notify external listeners (e.g., OSCManager for REMOTE protocol)
            if (onChannelSelected)
                onChannelSelected(channel);
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
        clusterSelector.onChange = [this]() {
            saveInputParam(WFSParameterIDs::inputCluster, clusterSelector.getSelectedId() - 1);
        };

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

    ~InputsTab() override
    {
        inputsTree.removeListener(this);
        configTree.removeListener(this);
    }

    /**
     * Callback when channel selection changes.
     * MainComponent can use this to notify OSCManager for REMOTE protocol.
     */
    std::function<void(int channelId)> onChannelSelected;

    /** Get the currently selected channel (1-based) */
    int getCurrentChannel() const { return currentChannel; }

    /** Select a specific channel (1-based). Triggers onChannelSelected callback.
     *  Uses programmatic selection to prevent keyboard Enter from triggering overlay.
     */
    void selectChannel(int channel)
    {
        channelSelector.setSelectedChannelProgrammatically(channel);
    }

    /** Get the total number of input channels */
    int getNumChannels() const { return channelSelector.getSelectedChannel() > 0 ?
                                         parameters.getNumInputChannels() : 1; }

    /** Cycle to next/previous channel. delta=1 for next, delta=-1 for previous. Wraps around. */
    void cycleChannel(int delta)
    {
        int numChannels = parameters.getNumInputChannels();
        if (numChannels <= 0) return;

        int newChannel = currentChannel + delta;
        if (newChannel > numChannels) newChannel = 1;
        else if (newChannel < 1) newChannel = numChannels;

        selectChannel(newChannel);
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
            saveInputParam(WFSParameterIDs::inputAttenuation, dB);  // Save real dB value
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
            saveInputParam(WFSParameterIDs::inputDelayLatency, ms);  // Save real ms value
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
            saveInputParam(WFSParameterIDs::inputMinimalLatency, minLat ? 1 : 0);
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
            bool enabled = constraintXButton.getToggleState();
            constraintXButton.setButtonText(enabled ? "Constraint X: ON" : "Constraint X: OFF");
            saveInputParam(WFSParameterIDs::inputConstraintX, enabled ? 1 : 0);
        };

        addAndMakeVisible(constraintYButton);
        constraintYButton.setButtonText("Constraint Y: ON");
        constraintYButton.setClickingTogglesState(true);
        constraintYButton.setToggleState(true, juce::dontSendNotification);
        constraintYButton.onClick = [this]() {
            bool enabled = constraintYButton.getToggleState();
            constraintYButton.setButtonText(enabled ? "Constraint Y: ON" : "Constraint Y: OFF");
            saveInputParam(WFSParameterIDs::inputConstraintY, enabled ? 1 : 0);
        };

        addAndMakeVisible(constraintZButton);
        constraintZButton.setButtonText("Constraint Z: ON");
        constraintZButton.setClickingTogglesState(true);
        constraintZButton.setToggleState(true, juce::dontSendNotification);
        constraintZButton.onClick = [this]() {
            bool enabled = constraintZButton.getToggleState();
            constraintZButton.setButtonText(enabled ? "Constraint Z: ON" : "Constraint Z: OFF");
            saveInputParam(WFSParameterIDs::inputConstraintZ, enabled ? 1 : 0);
        };

        // Flip buttons
        addAndMakeVisible(flipXButton);
        flipXButton.setButtonText("Flip X: OFF");
        flipXButton.setClickingTogglesState(true);
        flipXButton.onClick = [this]() {
            bool enabled = flipXButton.getToggleState();
            flipXButton.setButtonText(enabled ? "Flip X: ON" : "Flip X: OFF");
            saveInputParam(WFSParameterIDs::inputFlipX, enabled ? 1 : 0);
        };

        addAndMakeVisible(flipYButton);
        flipYButton.setButtonText("Flip Y: OFF");
        flipYButton.setClickingTogglesState(true);
        flipYButton.onClick = [this]() {
            bool enabled = flipYButton.getToggleState();
            flipYButton.setButtonText(enabled ? "Flip Y: ON" : "Flip Y: OFF");
            saveInputParam(WFSParameterIDs::inputFlipY, enabled ? 1 : 0);
        };

        addAndMakeVisible(flipZButton);
        flipZButton.setButtonText("Flip Z: OFF");
        flipZButton.setClickingTogglesState(true);
        flipZButton.onClick = [this]() {
            bool enabled = flipZButton.getToggleState();
            flipZButton.setButtonText(enabled ? "Flip Z: ON" : "Flip Z: OFF");
            saveInputParam(WFSParameterIDs::inputFlipZ, enabled ? 1 : 0);
        };

        // Tracking
        addAndMakeVisible(trackingActiveButton);
        trackingActiveButton.setButtonText("Tracking: OFF");
        trackingActiveButton.setClickingTogglesState(true);
        trackingActiveButton.onClick = [this]() {
            bool enabled = trackingActiveButton.getToggleState();
            trackingActiveButton.setButtonText(enabled ? "Tracking: ON" : "Tracking: OFF");
            saveInputParam(WFSParameterIDs::inputTrackingActive, enabled ? 1 : 0);
        };

        // Tracking ID selector (1-32)
        addAndMakeVisible(trackingIdLabel);
        trackingIdLabel.setText("Tracking ID:", juce::dontSendNotification);
        trackingIdLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(trackingIdSelector);
        for (int i = 1; i <= 32; ++i)
            trackingIdSelector.addItem(juce::String(i), i);
        trackingIdSelector.setSelectedId(1, juce::dontSendNotification);
        trackingIdSelector.onChange = [this]() {
            saveInputParam(WFSParameterIDs::inputTrackingID, trackingIdSelector.getSelectedId());
        };

        // Tracking Smoothing dial (0-100%)
        addAndMakeVisible(trackingSmoothLabel);
        trackingSmoothLabel.setText("Tracking Smooth:", juce::dontSendNotification);
        trackingSmoothLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        trackingSmoothDial.setColours(juce::Colours::black, juce::Colour(0xFF00BCD4), juce::Colours::grey);
        trackingSmoothDial.setValue(1.0f);  // Default 100%
        trackingSmoothDial.onValueChanged = [this](float v) {
            int percent = static_cast<int>(v * 100.0f);
            trackingSmoothValueLabel.setText(juce::String(percent) + " %", juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputTrackingSmooth, percent);
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
            bool enabled = maxSpeedActiveButton.getToggleState();
            maxSpeedActiveButton.setButtonText(enabled ? "Max Speed: ON" : "Max Speed: OFF");
            saveInputParam(WFSParameterIDs::inputMaxSpeedActive, enabled ? 1 : 0);
        };

        // Max Speed dial (0.01-20.0 m/s)
        addAndMakeVisible(maxSpeedLabel);
        maxSpeedLabel.setText("Max Speed:", juce::dontSendNotification);
        maxSpeedLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        maxSpeedDial.setColours(juce::Colours::black, juce::Colour(0xFFFF9800), juce::Colours::grey);
        maxSpeedDial.onValueChanged = [this](float v) {
            float speed = v * 19.99f + 0.01f;
            maxSpeedValueLabel.setText(juce::String(speed, 2) + " m/s", juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputMaxSpeed, speed);
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
            int percent = static_cast<int>(v * 100.0f);
            heightFactorValueLabel.setText(juce::String(percent) + " %", juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputHeightFactor, percent);
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
            saveInputParam(WFSParameterIDs::inputAttenuationLaw, is1OverD ? 1 : 0);
        };

        // Distance Attenuation dial (visible when attenuationLaw == Log)
        addAndMakeVisible(distanceAttenLabel);
        distanceAttenLabel.setText("Distance Atten:", juce::dontSendNotification);
        distanceAttenLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        distanceAttenDial.setColours(juce::Colours::black, juce::Colour(0xFF9C27B0), juce::Colours::grey);
        distanceAttenDial.onValueChanged = [this](float v) {
            float dBm = (v * 6.0f) - 6.0f;
            distanceAttenValueLabel.setText(juce::String(dBm, 1) + " dB/m", juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputDistanceAttenuation, dBm);
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
            saveInputParam(WFSParameterIDs::inputDistanceRatio, ratio);
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
            int percent = static_cast<int>(v * 100.0f);
            commonAttenValueLabel.setText(juce::String(percent) + " %", juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputCommonAtten, percent);
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
            saveInputParam(WFSParameterIDs::inputDirectivity, degrees);
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
            saveInputParam(WFSParameterIDs::inputRotation, static_cast<int>(angle));
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
            saveInputParam(WFSParameterIDs::inputTilt, degrees);
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
            saveInputParam(WFSParameterIDs::inputHFshelf, dB);
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
            bool enabled = lsActiveButton.getToggleState();
            lsActiveButton.setButtonText(enabled ? "Live Source Tamer: ON" : "Live Source Tamer: OFF");
            saveInputParam(WFSParameterIDs::inputLSactive, enabled ? 1 : 0);
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
            saveInputParam(WFSParameterIDs::inputLSradius, meters);
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
        lsShapeSelector.onChange = [this]() {
            saveInputParam(WFSParameterIDs::inputLSshape, lsShapeSelector.getSelectedId() - 1);
        };

        // Attenuation slider
        addAndMakeVisible(lsAttenuationLabel);
        lsAttenuationLabel.setText("Attenuation:", juce::dontSendNotification);
        lsAttenuationLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lsAttenuationSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFFFF5722));
        lsAttenuationSlider.onValueChanged = [this](float v) {
            float dB = 20.0f * std::log10(std::pow(10.0f, -24.0f / 20.0f) +
                       ((1.0f - std::pow(10.0f, -24.0f / 20.0f)) * v * v));
            lsAttenuationValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputLSattenuation, dB);
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
            saveInputParam(WFSParameterIDs::inputLSpeakThreshold, dB);
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
            saveInputParam(WFSParameterIDs::inputLSpeakRatio, ratio);
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
            saveInputParam(WFSParameterIDs::inputLSslowThreshold, dB);
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
            saveInputParam(WFSParameterIDs::inputLSslowRatio, ratio);
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
            bool enabled = frActiveButton.getToggleState();
            frActiveButton.setButtonText(enabled ? "Floor Reflections: ON" : "Floor Reflections: OFF");
            saveInputParam(WFSParameterIDs::inputFRactive, enabled ? 1 : 0);
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
            saveInputParam(WFSParameterIDs::inputFRattenuation, dB);
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
            int percent = static_cast<int>(v * 100.0f);
            frDiffusionValueLabel.setText(juce::String(percent) + " %", juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputFRdiffusion, percent);
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
            bool enabled = frLowCutActiveButton.getToggleState();
            frLowCutActiveButton.setButtonText(enabled ? "Low Cut: ON" : "Low Cut: OFF");
            saveInputParam(WFSParameterIDs::inputFRlowCutActive, enabled ? 1 : 0);
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
            saveInputParam(WFSParameterIDs::inputFRlowCutFreq, freq);
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
            bool enabled = frHighShelfActiveButton.getToggleState();
            frHighShelfActiveButton.setButtonText(enabled ? "High Shelf: ON" : "High Shelf: OFF");
            saveInputParam(WFSParameterIDs::inputFRhighShelfActive, enabled ? 1 : 0);
        };

        // FR High Shelf Frequency slider (20-20000 Hz)
        addAndMakeVisible(frHighShelfFreqLabel);
        frHighShelfFreqLabel.setText("HS Freq:", juce::dontSendNotification);
        frHighShelfFreqLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        frHighShelfFreqSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFF607D8B));
        frHighShelfFreqSlider.onValueChanged = [this](float v) {
            int freq = static_cast<int>(20.0f * std::pow(10.0f, 3.0f * v));
            frHighShelfFreqValueLabel.setText(juce::String(freq) + " Hz", juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputFRhighShelfFreq, freq);
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
            saveInputParam(WFSParameterIDs::inputFRhighShelfGain, dB);
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
            saveInputParam(WFSParameterIDs::inputFRhighShelfSlope, slope);
        };
        addAndMakeVisible(frHighShelfSlopeSlider);
        addAndMakeVisible(frHighShelfSlopeValueLabel);
        frHighShelfSlopeValueLabel.setText("0.40", juce::dontSendNotification);
        frHighShelfSlopeValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel(frHighShelfSlopeValueLabel);
    }

    void setupLfoTab()
    {
        // LFO Active button
        addAndMakeVisible(lfoActiveButton);
        lfoActiveButton.setButtonText("L.F.O: OFF");
        lfoActiveButton.setClickingTogglesState(true);
        lfoActiveButton.onClick = [this]() {
            bool enabled = lfoActiveButton.getToggleState();
            lfoActiveButton.setButtonText(enabled ? "L.F.O: ON" : "L.F.O: OFF");
            saveInputParam(WFSParameterIDs::inputLFOactive, enabled ? 1 : 0);
        };

        // Period dial (0.01-100.0 s) - Formula: pow(10.0,sqrt(x)*4.0-2.0)
        addAndMakeVisible(lfoPeriodLabel);
        lfoPeriodLabel.setText("Period:", juce::dontSendNotification);
        lfoPeriodLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lfoPeriodDial.setColours(juce::Colours::black, juce::Colour(0xFF00BCD4), juce::Colours::grey);
        lfoPeriodDial.onValueChanged = [this](float v) {
            float period = std::pow(10.0f, std::sqrt(v) * 4.0f - 2.0f);
            lfoPeriodValueLabel.setText(juce::String(period, 2) + " s", juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputLFOperiod, period);  // Save real period in seconds
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
            saveInputParam(WFSParameterIDs::inputLFOphase, degrees);  // Save real degrees (0-360)
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
        lfoShapeXSelector.onChange = [this]() {
            saveInputParam(WFSParameterIDs::inputLFOshapeX, lfoShapeXSelector.getSelectedId() - 1);
        };

        addAndMakeVisible(lfoShapeYLabel);
        lfoShapeYLabel.setText("Shape Y:", juce::dontSendNotification);
        lfoShapeYLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(lfoShapeYSelector);
        for (int i = 0; i < lfoShapes.size(); ++i)
            lfoShapeYSelector.addItem(lfoShapes[i], i + 1);
        lfoShapeYSelector.setSelectedId(1, juce::dontSendNotification);
        lfoShapeYSelector.onChange = [this]() {
            saveInputParam(WFSParameterIDs::inputLFOshapeY, lfoShapeYSelector.getSelectedId() - 1);
        };

        addAndMakeVisible(lfoShapeZLabel);
        lfoShapeZLabel.setText("Shape Z:", juce::dontSendNotification);
        lfoShapeZLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(lfoShapeZSelector);
        for (int i = 0; i < lfoShapes.size(); ++i)
            lfoShapeZSelector.addItem(lfoShapes[i], i + 1);
        lfoShapeZSelector.setSelectedId(1, juce::dontSendNotification);
        lfoShapeZSelector.onChange = [this]() {
            saveInputParam(WFSParameterIDs::inputLFOshapeZ, lfoShapeZSelector.getSelectedId() - 1);
        };

        // Rate X/Y/Z sliders (0.01-100, formula: pow(10.0,(x*4.0)-2.0))
        addAndMakeVisible(lfoRateXLabel);
        lfoRateXLabel.setText("Rate X:", juce::dontSendNotification);
        lfoRateXLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        lfoRateXSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFFE91E63));
        lfoRateXSlider.onValueChanged = [this](float v) {
            float rate = std::pow(10.0f, (v * 4.0f) - 2.0f);
            lfoRateXValueLabel.setText(juce::String(rate, 2) + "x", juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputLFOrateX, rate);  // Save real rate multiplier
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
            saveInputParam(WFSParameterIDs::inputLFOrateY, rate);  // Save real rate multiplier
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
            saveInputParam(WFSParameterIDs::inputLFOrateZ, rate);  // Save real rate multiplier
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
            saveInputParam(WFSParameterIDs::inputLFOamplitudeX, amp);  // Save real meters
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
            saveInputParam(WFSParameterIDs::inputLFOamplitudeY, amp);  // Save real meters
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
            saveInputParam(WFSParameterIDs::inputLFOamplitudeZ, amp);  // Save real meters
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
            saveInputParam(WFSParameterIDs::inputLFOphaseX, degrees);  // Save real degrees (0-360)
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
            saveInputParam(WFSParameterIDs::inputLFOphaseY, degrees);  // Save real degrees (0-360)
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
            saveInputParam(WFSParameterIDs::inputLFOphaseZ, degrees);  // Save real degrees (0-360)
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
        lfoGyrophoneSelector.onChange = [this]() {
            saveInputParam(WFSParameterIDs::inputLFOgyrophone, lfoGyrophoneSelector.getSelectedId() - 1);
        };

        // Jitter slider
        addAndMakeVisible(jitterLabel);
        jitterLabel.setText("Jitter:", juce::dontSendNotification);
        jitterLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        jitterSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFFCDDC39));
        jitterSlider.onValueChanged = [this](float v) {
            float meters = 10.0f * v * v;
            jitterValueLabel.setText(juce::String(meters, 2) + " m", juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputJitter, meters);  // Save real meters
        };
        addAndMakeVisible(jitterSlider);
        addAndMakeVisible(jitterValueLabel);
        jitterValueLabel.setText("0.00 m", juce::dontSendNotification);
        jitterValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel(jitterValueLabel);
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
            bool isRelative = otomoAbsRelButton.getToggleState();
            otomoAbsRelButton.setButtonText(isRelative ? "Relative" : "Absolute");
            saveInputParam(WFSParameterIDs::inputOtomoAbsoluteRelative, isRelative ? 1 : 0);
        };

        // Stay/Return button
        addAndMakeVisible(otomoStayReturnButton);
        otomoStayReturnButton.setButtonText("Stay");
        otomoStayReturnButton.setClickingTogglesState(true);
        otomoStayReturnButton.onClick = [this]() {
            bool isReturn = otomoStayReturnButton.getToggleState();
            otomoStayReturnButton.setButtonText(isReturn ? "Return" : "Stay");
            saveInputParam(WFSParameterIDs::inputOtomoStayReturn, isReturn ? 1 : 0);
        };

        // Speed Profile dial (0-100%)
        addAndMakeVisible(otomoSpeedProfileLabel);
        otomoSpeedProfileLabel.setText("Speed Profile:", juce::dontSendNotification);
        otomoSpeedProfileLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        otomoSpeedProfileDial.setColours(juce::Colours::black, juce::Colour(0xFF2196F3), juce::Colours::grey);
        otomoSpeedProfileDial.onValueChanged = [this](float v) {
            int percent = static_cast<int>(v * 100.0f);
            otomoSpeedProfileValueLabel.setText(juce::String(percent) + " %", juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputOtomoSpeedProfile, percent);  // Save real percent
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
            bool isTrigger = otomoTriggerButton.getToggleState();
            otomoTriggerButton.setButtonText(isTrigger ? "Trigger" : "Manual");
            saveInputParam(WFSParameterIDs::inputOtomoTrigger, isTrigger ? 1 : 0);
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
            saveInputParam(WFSParameterIDs::inputOtomoThreshold, dB);  // Save real dB value
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
            saveInputParam(WFSParameterIDs::inputOtomoReset, dB);  // Save real dB value
        };
        addAndMakeVisible(otomoResetDial);
        addAndMakeVisible(otomoResetValueLabel);
        otomoResetValueLabel.setText("-60.0 dB", juce::dontSendNotification);
        otomoResetValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        otomoResetValueLabel.setJustificationType(juce::Justification::centred);
        setupEditableValueLabel(otomoResetValueLabel);

        // Transport buttons (custom drawn icons)
        addAndMakeVisible(otomoStartButton);
        otomoStartButton.onClick = [this]() { /* Start movement */ };

        addAndMakeVisible(otomoStopButton);
        otomoStopButton.onClick = [this]() { /* Stop movement */ };

        addAndMakeVisible(otomoPauseButton);
        otomoPauseButton.setClickingTogglesState(true);
        otomoPauseButton.onClick = [this]() {
            bool isPaused = otomoPauseButton.getToggleState();
            saveInputParam(WFSParameterIDs::inputOtomoPauseResume, isPaused ? 1 : 0);
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
                saveMuteStates();
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
            if (macroId > 1)
            {
                applyMuteMacro(macroId);
                saveMuteStates();
                saveInputParam(WFSParameterIDs::inputMuteMacro, macroId);
            }
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
        jitterLabel.setVisible(v); jitterSlider.setVisible(v); jitterValueLabel.setVisible(v);
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
        leftCol.removeFromTop(spacing);

        // Jitter
        row = leftCol.removeFromTop(rowHeight);
        jitterLabel.setBounds(row.removeFromLeft(labelWidth));
        jitterValueLabel.setBounds(row.removeFromRight(valueWidth));
        jitterSlider.setBounds(leftCol.removeFromTop(sliderHeight));

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
        int numOutputs = parameters.getNumOutputChannels();
        if (numOutputs <= 0) numOutputs = 16;  // Default

        for (int i = 0; i < 64; ++i)
            muteButtons[i].setVisible(v && i < numOutputs);
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

        // Get configured output count
        int numOutputs = parameters.getNumOutputChannels();
        if (numOutputs <= 0) numOutputs = 16;  // Default

        // Calculate grid dimensions (prefer 8 columns, adapt rows)
        int numColumns = juce::jmin(8, numOutputs);
        int numRows = (numOutputs + numColumns - 1) / numColumns;

        // Mute grid - dynamic size based on output count
        auto gridArea = area.removeFromTop(numRows * (buttonSize + gridSpacing));

        for (int row = 0; row < numRows; ++row)
        {
            auto rowArea = gridArea.removeFromTop(buttonSize + gridSpacing);
            for (int col = 0; col < numColumns; ++col)
            {
                int index = row * numColumns + col;
                if (index < numOutputs)
                {
                    muteButtons[index].setBounds(rowArea.removeFromLeft(buttonSize));
                    rowArea.removeFromLeft(gridSpacing);
                }
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
        isLoadingParameters = true;
        currentChannel = channel;

        auto getParam = [this](const juce::Identifier& id) -> juce::var {
            return parameters.getInputParam(currentChannel - 1, id.toString());
        };

        auto getFloatParam = [&getParam](const juce::Identifier& id, float defaultVal = 0.0f) -> float {
            auto val = getParam(id);
            return val.isVoid() ? defaultVal : static_cast<float>(val);
        };

        auto getIntParam = [&getParam](const juce::Identifier& id, int defaultVal = 0) -> int {
            auto val = getParam(id);
            return val.isVoid() ? defaultVal : static_cast<int>(val);
        };

        auto getStringParam = [&getParam](const juce::Identifier& id, const juce::String& defaultVal = "") -> juce::String {
            auto val = getParam(id);
            return val.isVoid() ? defaultVal : val.toString();
        };

        // ==================== HEADER ====================
        nameEditor.setText(getStringParam(WFSParameterIDs::inputName, "Input " + juce::String(channel)), juce::dontSendNotification);
        clusterSelector.setSelectedId(getIntParam(WFSParameterIDs::inputCluster, 0) + 1, juce::dontSendNotification);

        // ==================== INPUT PROPERTIES TAB ====================
        // Attenuation stored as dB (-92 to 0), default 0dB
        float attenDB = getFloatParam(WFSParameterIDs::inputAttenuation, 0.0f);
        attenDB = juce::jlimit(-92.0f, 0.0f, attenDB);
        // Convert dB to slider value (0-1) using inverse of logarithmic formula
        float minLinear = std::pow(10.0f, -92.0f / 20.0f);
        float targetLinear = std::pow(10.0f, attenDB / 20.0f);
        float attenSliderVal = std::sqrt((targetLinear - minLinear) / (1.0f - minLinear));
        attenuationSlider.setValue(juce::jlimit(0.0f, 1.0f, attenSliderVal));
        attenuationValueLabel.setText(juce::String(attenDB, 1) + " dB", juce::dontSendNotification);

        // Delay/Latency stored as ms (-100 to 100), default 0ms
        float delayMs = getFloatParam(WFSParameterIDs::inputDelayLatency, 0.0f);
        delayMs = juce::jlimit(-100.0f, 100.0f, delayMs);
        delayLatencySlider.setValue(delayMs / 100.0f);  // Convert ms to slider value (-1 to 1)
        juce::String delayLabel = (delayMs < 0) ? "Latency: " : "Delay: ";
        delayLatencyValueLabel.setText(delayLabel + juce::String(std::abs(delayMs), 1) + " ms", juce::dontSendNotification);

        bool minLatency = getIntParam(WFSParameterIDs::inputMinimalLatency, 0) != 0;
        minimalLatencyButton.setToggleState(minLatency, juce::dontSendNotification);
        minimalLatencyButton.setButtonText(minLatency ? "Minimal Latency: ON" : "Minimal Latency: OFF");

        // ==================== POSITION TAB ====================
        posXEditor.setText(juce::String(getFloatParam(WFSParameterIDs::inputPositionX, 0.0f), 2), juce::dontSendNotification);
        posYEditor.setText(juce::String(getFloatParam(WFSParameterIDs::inputPositionY, 0.0f), 2), juce::dontSendNotification);
        posZEditor.setText(juce::String(getFloatParam(WFSParameterIDs::inputPositionZ, 0.0f), 2), juce::dontSendNotification);
        offsetXEditor.setText(juce::String(getFloatParam(WFSParameterIDs::inputOffsetX, 0.0f), 2), juce::dontSendNotification);
        offsetYEditor.setText(juce::String(getFloatParam(WFSParameterIDs::inputOffsetY, 0.0f), 2), juce::dontSendNotification);
        offsetZEditor.setText(juce::String(getFloatParam(WFSParameterIDs::inputOffsetZ, 0.0f), 2), juce::dontSendNotification);

        bool constX = getIntParam(WFSParameterIDs::inputConstraintX, 0) != 0;
        constraintXButton.setToggleState(constX, juce::dontSendNotification);
        constraintXButton.setButtonText(constX ? "Constraint X: ON" : "Constraint X: OFF");

        bool constY = getIntParam(WFSParameterIDs::inputConstraintY, 0) != 0;
        constraintYButton.setToggleState(constY, juce::dontSendNotification);
        constraintYButton.setButtonText(constY ? "Constraint Y: ON" : "Constraint Y: OFF");

        bool constZ = getIntParam(WFSParameterIDs::inputConstraintZ, 0) != 0;
        constraintZButton.setToggleState(constZ, juce::dontSendNotification);
        constraintZButton.setButtonText(constZ ? "Constraint Z: ON" : "Constraint Z: OFF");

        bool flipX = getIntParam(WFSParameterIDs::inputFlipX, 0) != 0;
        flipXButton.setToggleState(flipX, juce::dontSendNotification);
        flipXButton.setButtonText(flipX ? "Flip X: ON" : "Flip X: OFF");

        bool flipY = getIntParam(WFSParameterIDs::inputFlipY, 0) != 0;
        flipYButton.setToggleState(flipY, juce::dontSendNotification);
        flipYButton.setButtonText(flipY ? "Flip Y: ON" : "Flip Y: OFF");

        bool flipZ = getIntParam(WFSParameterIDs::inputFlipZ, 0) != 0;
        flipZButton.setToggleState(flipZ, juce::dontSendNotification);
        flipZButton.setButtonText(flipZ ? "Flip Z: ON" : "Flip Z: OFF");

        bool trackActive = getIntParam(WFSParameterIDs::inputTrackingActive, 0) != 0;
        trackingActiveButton.setToggleState(trackActive, juce::dontSendNotification);
        trackingActiveButton.setButtonText(trackActive ? "Tracking: ON" : "Tracking: OFF");

        trackingIdSelector.setSelectedId(getIntParam(WFSParameterIDs::inputTrackingID, 0) + 1, juce::dontSendNotification);

        float trackSmooth = getFloatParam(WFSParameterIDs::inputTrackingSmooth, 0.0f);
        trackingSmoothDial.setValue(trackSmooth);
        trackingSmoothValueLabel.setText(juce::String(static_cast<int>(trackSmooth * 100.0f)) + " %", juce::dontSendNotification);

        bool maxSpeedActive = getIntParam(WFSParameterIDs::inputMaxSpeedActive, 0) != 0;
        maxSpeedActiveButton.setToggleState(maxSpeedActive, juce::dontSendNotification);
        maxSpeedActiveButton.setButtonText(maxSpeedActive ? "Max Speed: ON" : "Max Speed: OFF");

        float maxSpeedVal = getFloatParam(WFSParameterIDs::inputMaxSpeed, 0.5f);
        maxSpeedDial.setValue(maxSpeedVal);
        float maxSpeedDisplay = 0.01f + maxSpeedVal * 9.99f;
        maxSpeedValueLabel.setText(juce::String(maxSpeedDisplay, 2) + " m/s", juce::dontSendNotification);

        float heightFactor = getFloatParam(WFSParameterIDs::inputHeightFactor, 1.0f);
        heightFactorDial.setValue(heightFactor);
        heightFactorValueLabel.setText(juce::String(static_cast<int>(heightFactor * 100.0f)) + " %", juce::dontSendNotification);

        // ==================== SOUND TAB ====================
        bool attenLaw = getIntParam(WFSParameterIDs::inputAttenuationLaw, 0) != 0;
        attenuationLawButton.setToggleState(attenLaw, juce::dontSendNotification);
        attenuationLawButton.setButtonText(attenLaw ? "Spherical" : "Cylindrical");

        // Distance Attenuation stored as dB/m (-6 to 0), default -0.7
        // Formula: dB = (x * 6.0) - 6.0 => x = (dB + 6) / 6
        float distAttenDB = getFloatParam(WFSParameterIDs::inputDistanceAttenuation, -0.7f);
        distAttenDB = juce::jlimit(-6.0f, 0.0f, distAttenDB);
        float distAttenSliderVal = (distAttenDB + 6.0f) / 6.0f;
        distanceAttenDial.setValue(juce::jlimit(0.0f, 1.0f, distAttenSliderVal));
        distanceAttenValueLabel.setText(juce::String(distAttenDB, 1) + " dB/m", juce::dontSendNotification);

        // Distance Ratio stored as multiplier (0.1 to 10), default 1.0
        // Formula: ratio = pow(10, (x * 2) - 1) => x = (log10(ratio) + 1) / 2
        float distRatioVal = getFloatParam(WFSParameterIDs::inputDistanceRatio, 1.0f);
        distRatioVal = juce::jlimit(0.1f, 10.0f, distRatioVal);
        float distRatioSliderVal = (std::log10(distRatioVal) + 1.0f) / 2.0f;
        distanceRatioDial.setValue(juce::jlimit(0.0f, 1.0f, distRatioSliderVal));
        distanceRatioValueLabel.setText(juce::String(distRatioVal, 2) + "x", juce::dontSendNotification);

        // Common Attenuation stored as percent (0-100), default 100
        // Formula: percent = x * 100 => x = percent / 100
        float commonAttenPct = getFloatParam(WFSParameterIDs::inputCommonAtten, 100.0f);
        commonAttenPct = juce::jlimit(0.0f, 100.0f, commonAttenPct);
        commonAttenDial.setValue(commonAttenPct / 100.0f);
        commonAttenValueLabel.setText(juce::String(static_cast<int>(commonAttenPct)) + " %", juce::dontSendNotification);

        // Directivity stored as degrees (2-360), default 360
        // Inverse of: degrees = (x * 358) + 2 => x = (degrees - 2) / 358
        float directivityDeg = getFloatParam(WFSParameterIDs::inputDirectivity, 360.0f);
        directivityDeg = juce::jlimit(2.0f, 360.0f, directivityDeg);
        float directivitySliderVal = (directivityDeg - 2.0f) / 358.0f;
        directivitySlider.setValue(juce::jlimit(0.0f, 1.0f, directivitySliderVal));
        directivityValueLabel.setText(juce::String(static_cast<int>(directivityDeg)) + juce::String::fromUTF8("°"), juce::dontSendNotification);

        float rotation = getFloatParam(WFSParameterIDs::inputRotation, 0.0f);
        rotationDial.setAngle(rotation * 360.0f);
        int rotDegrees = static_cast<int>(rotation * 360.0f);
        if (rotDegrees < 0) rotDegrees += 360;
        rotationValueLabel.setText(juce::String(rotDegrees) + juce::String::fromUTF8("°"), juce::dontSendNotification);

        // Tilt stored as degrees (-90 to 90), default 0
        // Inverse of: degrees = (x * 180) - 90 => x = (degrees + 90) / 180
        float tiltDeg = getFloatParam(WFSParameterIDs::inputTilt, 0.0f);
        tiltDeg = juce::jlimit(-90.0f, 90.0f, tiltDeg);
        float tiltSliderVal = (tiltDeg + 90.0f) / 180.0f;
        tiltSlider.setValue(juce::jlimit(0.0f, 1.0f, tiltSliderVal));
        tiltValueLabel.setText(juce::String(tiltDeg, 1) + juce::String::fromUTF8("°"), juce::dontSendNotification);

        // HF Shelf stored as dB (-24 to 0), default -6
        // Formula: dB = 20*log10(minLin + (1-minLin)*x^2), where minLin = pow(10, -24/20)
        // Inverse: x = sqrt((targetLin - minLin) / (1 - minLin))
        float hfShelfDB = getFloatParam(WFSParameterIDs::inputHFshelf, -6.0f);
        hfShelfDB = juce::jlimit(-24.0f, 0.0f, hfShelfDB);
        float hfMinLinear = std::pow(10.0f, -24.0f / 20.0f);
        float hfTargetLinear = std::pow(10.0f, hfShelfDB / 20.0f);
        float hfShelfSliderVal = std::sqrt((hfTargetLinear - hfMinLinear) / (1.0f - hfMinLinear));
        hfShelfSlider.setValue(juce::jlimit(0.0f, 1.0f, hfShelfSliderVal));
        hfShelfValueLabel.setText(juce::String(hfShelfDB, 1) + " dB", juce::dontSendNotification);

        // ==================== LIVE SOURCE TAB ====================
        bool lsActive = getIntParam(WFSParameterIDs::inputLSactive, 0) != 0;
        lsActiveButton.setToggleState(lsActive, juce::dontSendNotification);
        lsActiveButton.setButtonText(lsActive ? "Live Source Tamer: ON" : "Live Source Tamer: OFF");

        // LS Radius stored as meters (0-50), default 3
        // Formula: meters = x * 50 => x = meters / 50
        float lsRadiusMeters = getFloatParam(WFSParameterIDs::inputLSradius, 3.0f);
        lsRadiusMeters = juce::jlimit(0.0f, 50.0f, lsRadiusMeters);
        lsRadiusSlider.setValue(lsRadiusMeters / 50.0f);
        lsRadiusValueLabel.setText(juce::String(lsRadiusMeters, 2) + " m", juce::dontSendNotification);

        lsShapeSelector.setSelectedId(getIntParam(WFSParameterIDs::inputLSshape, 0) + 1, juce::dontSendNotification);

        // LS Attenuation stored as dB (-24 to 0), default 0
        // Formula: dB = 20*log10(minLin + (1-minLin)*x^2), where minLin = pow(10, -24/20)
        // Inverse: x = sqrt((targetLin - minLin) / (1 - minLin))
        float lsAttenDB = getFloatParam(WFSParameterIDs::inputLSattenuation, 0.0f);
        lsAttenDB = juce::jlimit(-24.0f, 0.0f, lsAttenDB);
        float lsMinLinear = std::pow(10.0f, -24.0f / 20.0f);
        float lsTargetLinear = std::pow(10.0f, lsAttenDB / 20.0f);
        float lsAttenSliderVal = std::sqrt((lsTargetLinear - lsMinLinear) / (1.0f - lsMinLinear));
        lsAttenuationSlider.setValue(juce::jlimit(0.0f, 1.0f, lsAttenSliderVal));
        lsAttenuationValueLabel.setText(juce::String(lsAttenDB, 1) + " dB", juce::dontSendNotification);

        // Peak Threshold stored as dB (-48 to 0), default -20
        // Formula: dB = 20*log10(minLin + (1-minLin)*x^2), where minLin = pow(10, -48/20)
        // Inverse: x = sqrt((targetLin - minLin) / (1 - minLin))
        float peakThreshDB = getFloatParam(WFSParameterIDs::inputLSpeakThreshold, -20.0f);
        peakThreshDB = juce::jlimit(-48.0f, 0.0f, peakThreshDB);
        float peakMinLinear = std::pow(10.0f, -48.0f / 20.0f);
        float peakTargetLinear = std::pow(10.0f, peakThreshDB / 20.0f);
        float peakThreshSliderVal = std::sqrt((peakTargetLinear - peakMinLinear) / (1.0f - peakMinLinear));
        lsPeakThresholdSlider.setValue(juce::jlimit(0.0f, 1.0f, peakThreshSliderVal));
        lsPeakThresholdValueLabel.setText(juce::String(peakThreshDB, 1) + " dB", juce::dontSendNotification);

        // Peak Ratio stored as ratio (1-10), default 2
        // Formula: ratio = (x * 9) + 1 => x = (ratio - 1) / 9
        float peakRatioVal = getFloatParam(WFSParameterIDs::inputLSpeakRatio, 2.0f);
        peakRatioVal = juce::jlimit(1.0f, 10.0f, peakRatioVal);
        float peakRatioSliderVal = (peakRatioVal - 1.0f) / 9.0f;
        lsPeakRatioDial.setValue(juce::jlimit(0.0f, 1.0f, peakRatioSliderVal));
        lsPeakRatioValueLabel.setText(juce::String(peakRatioVal, 1) + ":1", juce::dontSendNotification);

        // Slow Threshold stored as dB (-48 to 0), default -20
        // Formula: dB = 20*log10(minLin + (1-minLin)*x^2), where minLin = pow(10, -48/20)
        // Inverse: x = sqrt((targetLin - minLin) / (1 - minLin))
        float slowThreshDB = getFloatParam(WFSParameterIDs::inputLSslowThreshold, -20.0f);
        slowThreshDB = juce::jlimit(-48.0f, 0.0f, slowThreshDB);
        float slowMinLinear = std::pow(10.0f, -48.0f / 20.0f);
        float slowTargetLinear = std::pow(10.0f, slowThreshDB / 20.0f);
        float slowThreshSliderVal = std::sqrt((slowTargetLinear - slowMinLinear) / (1.0f - slowMinLinear));
        lsSlowThresholdSlider.setValue(juce::jlimit(0.0f, 1.0f, slowThreshSliderVal));
        lsSlowThresholdValueLabel.setText(juce::String(slowThreshDB, 1) + " dB", juce::dontSendNotification);

        // Slow Ratio stored as ratio (1-10), default 2
        // Formula: ratio = (x * 9) + 1 => x = (ratio - 1) / 9
        float slowRatioVal = getFloatParam(WFSParameterIDs::inputLSslowRatio, 2.0f);
        slowRatioVal = juce::jlimit(1.0f, 10.0f, slowRatioVal);
        float slowRatioSliderVal = (slowRatioVal - 1.0f) / 9.0f;
        lsSlowRatioDial.setValue(juce::jlimit(0.0f, 1.0f, slowRatioSliderVal));
        lsSlowRatioValueLabel.setText(juce::String(slowRatioVal, 1) + ":1", juce::dontSendNotification);

        // ==================== EFFECTS (HACKOUSTICS) TAB ====================
        bool frActive = getIntParam(WFSParameterIDs::inputFRactive, 0) != 0;
        frActiveButton.setToggleState(frActive, juce::dontSendNotification);
        frActiveButton.setButtonText(frActive ? "Floor Reflections: ON" : "Floor Reflections: OFF");

        // FR Attenuation stored as dB (-60 to 0), default -3
        // Formula: dB = 20*log10(minLin + (1-minLin)*x^2), where minLin = pow(10, -60/20)
        // Inverse: x = sqrt((targetLin - minLin) / (1 - minLin))
        float frAttenDB = getFloatParam(WFSParameterIDs::inputFRattenuation, -3.0f);
        frAttenDB = juce::jlimit(-60.0f, 0.0f, frAttenDB);
        float frMinLinear = std::pow(10.0f, -60.0f / 20.0f);
        float frTargetLinear = std::pow(10.0f, frAttenDB / 20.0f);
        float frAttenSliderVal = std::sqrt((frTargetLinear - frMinLinear) / (1.0f - frMinLinear));
        frAttenuationSlider.setValue(juce::jlimit(0.0f, 1.0f, frAttenSliderVal));
        frAttenuationValueLabel.setText(juce::String(frAttenDB, 1) + " dB", juce::dontSendNotification);

        // FR Diffusion stored as percent (0-100), default 20
        // Formula: percent = x * 100 => x = percent / 100
        float frDiffusionPct = getFloatParam(WFSParameterIDs::inputFRdiffusion, 20.0f);
        frDiffusionPct = juce::jlimit(0.0f, 100.0f, frDiffusionPct);
        frDiffusionDial.setValue(frDiffusionPct / 100.0f);
        frDiffusionValueLabel.setText(juce::String(static_cast<int>(frDiffusionPct)) + " %", juce::dontSendNotification);

        bool frLowCutActive = getIntParam(WFSParameterIDs::inputFRlowCutActive, 0) != 0;
        frLowCutActiveButton.setToggleState(frLowCutActive, juce::dontSendNotification);
        frLowCutActiveButton.setButtonText(frLowCutActive ? "Low Cut: ON" : "Low Cut: OFF");

        // FR Low Cut Freq stored as Hz (20-20000), default 100
        // Formula: freq = 20 * pow(10, 3*x) => x = log10(freq/20) / 3
        float frLowCutFreqHz = getFloatParam(WFSParameterIDs::inputFRlowCutFreq, 100.0f);
        frLowCutFreqHz = juce::jlimit(20.0f, 20000.0f, frLowCutFreqHz);
        float frLowCutSliderVal = std::log10(frLowCutFreqHz / 20.0f) / 3.0f;
        frLowCutFreqSlider.setValue(juce::jlimit(0.0f, 1.0f, frLowCutSliderVal));
        frLowCutFreqValueLabel.setText(juce::String(static_cast<int>(frLowCutFreqHz)) + " Hz", juce::dontSendNotification);

        bool frHighShelfActive = getIntParam(WFSParameterIDs::inputFRhighShelfActive, 0) != 0;
        frHighShelfActiveButton.setToggleState(frHighShelfActive, juce::dontSendNotification);
        frHighShelfActiveButton.setButtonText(frHighShelfActive ? "High Shelf: ON" : "High Shelf: OFF");

        // FR High Shelf Freq stored as Hz (20-20000), default 3000
        // Formula: freq = 20 * pow(10, 3*x) => x = log10(freq/20) / 3
        float frHighShelfFreqHz = getFloatParam(WFSParameterIDs::inputFRhighShelfFreq, 3000.0f);
        frHighShelfFreqHz = juce::jlimit(20.0f, 20000.0f, frHighShelfFreqHz);
        float frHighShelfFreqSliderVal = std::log10(frHighShelfFreqHz / 20.0f) / 3.0f;
        frHighShelfFreqSlider.setValue(juce::jlimit(0.0f, 1.0f, frHighShelfFreqSliderVal));
        frHighShelfFreqValueLabel.setText(juce::String(static_cast<int>(frHighShelfFreqHz)) + " Hz", juce::dontSendNotification);

        // FR High Shelf Gain stored as dB (-24 to 0), default -2
        // Formula: dB = 20*log10(minLin + (1-minLin)*x^2), where minLin = pow(10, -24/20)
        // Inverse: x = sqrt((targetLin - minLin) / (1 - minLin))
        float frHighShelfGainDB = getFloatParam(WFSParameterIDs::inputFRhighShelfGain, -2.0f);
        frHighShelfGainDB = juce::jlimit(-24.0f, 0.0f, frHighShelfGainDB);
        float hsMinLinear = std::pow(10.0f, -24.0f / 20.0f);
        float hsTargetLinear = std::pow(10.0f, frHighShelfGainDB / 20.0f);
        float hsGainSliderVal = std::sqrt((hsTargetLinear - hsMinLinear) / (1.0f - hsMinLinear));
        frHighShelfGainSlider.setValue(juce::jlimit(0.0f, 1.0f, hsGainSliderVal));
        frHighShelfGainValueLabel.setText(juce::String(frHighShelfGainDB, 1) + " dB", juce::dontSendNotification);

        // FR High Shelf Slope stored as value (0.1 to 0.9), default 0.4
        // Formula: slope = (x * 0.8) + 0.1 => x = (slope - 0.1) / 0.8
        float frHighShelfSlopeVal = getFloatParam(WFSParameterIDs::inputFRhighShelfSlope, 0.4f);
        frHighShelfSlopeVal = juce::jlimit(0.1f, 0.9f, frHighShelfSlopeVal);
        float frSlopeSliderVal = (frHighShelfSlopeVal - 0.1f) / 0.8f;
        frHighShelfSlopeSlider.setValue(juce::jlimit(0.0f, 1.0f, frSlopeSliderVal));
        frHighShelfSlopeValueLabel.setText(juce::String(frHighShelfSlopeVal, 2), juce::dontSendNotification);

        // ==================== LFO TAB ====================
        bool lfoActive = getIntParam(WFSParameterIDs::inputLFOactive, 0) != 0;
        lfoActiveButton.setToggleState(lfoActive, juce::dontSendNotification);
        lfoActiveButton.setButtonText(lfoActive ? "L.F.O: ON" : "L.F.O: OFF");

        // LFO Period stored as seconds (0.01-100), default 5.0s
        float lfoPeriodSec = getFloatParam(WFSParameterIDs::inputLFOperiod, 5.0f);
        lfoPeriodSec = juce::jlimit(0.01f, 100.0f, lfoPeriodSec);
        // Inverse of: period = pow(10, sqrt(v)*4 - 2) => v = pow((log10(period)+2)/4, 2)
        float lfoPeriodSlider = std::pow((std::log10(lfoPeriodSec) + 2.0f) / 4.0f, 2.0f);
        lfoPeriodDial.setValue(juce::jlimit(0.0f, 1.0f, lfoPeriodSlider));
        lfoPeriodValueLabel.setText(juce::String(lfoPeriodSec, 2) + " s", juce::dontSendNotification);

        // LFO Phase stored as degrees (0-360), default 0
        int lfoPhaseDeg = getIntParam(WFSParameterIDs::inputLFOphase, 0);
        lfoPhaseDeg = ((lfoPhaseDeg % 360) + 360) % 360;  // Normalize to 0-359
        lfoPhaseDial.setAngle(static_cast<float>(lfoPhaseDeg));
        lfoPhaseValueLabel.setText(juce::String(lfoPhaseDeg) + juce::String::fromUTF8("°"), juce::dontSendNotification);

        lfoShapeXSelector.setSelectedId(getIntParam(WFSParameterIDs::inputLFOshapeX, 0) + 1, juce::dontSendNotification);
        lfoShapeYSelector.setSelectedId(getIntParam(WFSParameterIDs::inputLFOshapeY, 0) + 1, juce::dontSendNotification);
        lfoShapeZSelector.setSelectedId(getIntParam(WFSParameterIDs::inputLFOshapeZ, 0) + 1, juce::dontSendNotification);

        // LFO Rate stored as multiplier (0.01-100), default 1.0x
        // Inverse of: rate = pow(10, v*4 - 2) => v = (log10(rate) + 2) / 4
        float lfoRateXVal = getFloatParam(WFSParameterIDs::inputLFOrateX, 1.0f);
        lfoRateXVal = juce::jlimit(0.01f, 100.0f, lfoRateXVal);
        float lfoRateXSliderVal = (std::log10(lfoRateXVal) + 2.0f) / 4.0f;
        lfoRateXSlider.setValue(juce::jlimit(0.0f, 1.0f, lfoRateXSliderVal));
        lfoRateXValueLabel.setText(juce::String(lfoRateXVal, 2) + "x", juce::dontSendNotification);

        float lfoRateYVal = getFloatParam(WFSParameterIDs::inputLFOrateY, 1.0f);
        lfoRateYVal = juce::jlimit(0.01f, 100.0f, lfoRateYVal);
        float lfoRateYSliderVal = (std::log10(lfoRateYVal) + 2.0f) / 4.0f;
        lfoRateYSlider.setValue(juce::jlimit(0.0f, 1.0f, lfoRateYSliderVal));
        lfoRateYValueLabel.setText(juce::String(lfoRateYVal, 2) + "x", juce::dontSendNotification);

        float lfoRateZVal = getFloatParam(WFSParameterIDs::inputLFOrateZ, 1.0f);
        lfoRateZVal = juce::jlimit(0.01f, 100.0f, lfoRateZVal);
        float lfoRateZSliderVal = (std::log10(lfoRateZVal) + 2.0f) / 4.0f;
        lfoRateZSlider.setValue(juce::jlimit(0.0f, 1.0f, lfoRateZSliderVal));
        lfoRateZValueLabel.setText(juce::String(lfoRateZVal, 2) + "x", juce::dontSendNotification);

        // LFO Amplitude stored as meters (0-50), default 1.0m
        float lfoAmpXMeters = getFloatParam(WFSParameterIDs::inputLFOamplitudeX, 1.0f);
        lfoAmpXMeters = juce::jlimit(0.0f, 50.0f, lfoAmpXMeters);
        lfoAmplitudeXSlider.setValue(lfoAmpXMeters / 50.0f);  // Convert to 0-1 slider
        lfoAmplitudeXValueLabel.setText(juce::String(lfoAmpXMeters, 1) + " m", juce::dontSendNotification);

        float lfoAmpYMeters = getFloatParam(WFSParameterIDs::inputLFOamplitudeY, 1.0f);
        lfoAmpYMeters = juce::jlimit(0.0f, 50.0f, lfoAmpYMeters);
        lfoAmplitudeYSlider.setValue(lfoAmpYMeters / 50.0f);
        lfoAmplitudeYValueLabel.setText(juce::String(lfoAmpYMeters, 1) + " m", juce::dontSendNotification);

        float lfoAmpZMeters = getFloatParam(WFSParameterIDs::inputLFOamplitudeZ, 1.0f);
        lfoAmpZMeters = juce::jlimit(0.0f, 50.0f, lfoAmpZMeters);
        lfoAmplitudeZSlider.setValue(lfoAmpZMeters / 50.0f);
        lfoAmplitudeZValueLabel.setText(juce::String(lfoAmpZMeters, 1) + " m", juce::dontSendNotification);

        // LFO Phase X/Y/Z stored as degrees (0-360), default 0
        int phaseXDeg = getIntParam(WFSParameterIDs::inputLFOphaseX, 0);
        phaseXDeg = ((phaseXDeg % 360) + 360) % 360;
        lfoPhaseXDial.setAngle(static_cast<float>(phaseXDeg));
        lfoPhaseXValueLabel.setText(juce::String(phaseXDeg) + juce::String::fromUTF8("°"), juce::dontSendNotification);

        int phaseYDeg = getIntParam(WFSParameterIDs::inputLFOphaseY, 0);
        phaseYDeg = ((phaseYDeg % 360) + 360) % 360;
        lfoPhaseYDial.setAngle(static_cast<float>(phaseYDeg));
        lfoPhaseYValueLabel.setText(juce::String(phaseYDeg) + juce::String::fromUTF8("°"), juce::dontSendNotification);

        int phaseZDeg = getIntParam(WFSParameterIDs::inputLFOphaseZ, 0);
        phaseZDeg = ((phaseZDeg % 360) + 360) % 360;
        lfoPhaseZDial.setAngle(static_cast<float>(phaseZDeg));
        lfoPhaseZValueLabel.setText(juce::String(phaseZDeg) + juce::String::fromUTF8("°"), juce::dontSendNotification);

        lfoGyrophoneSelector.setSelectedId(getIntParam(WFSParameterIDs::inputLFOgyrophone, 1) + 1, juce::dontSendNotification);

        // Jitter stored as meters (0-10), default 0
        // Inverse of: meters = 10 * v^2 => v = sqrt(meters / 10)
        float jitterMeters = getFloatParam(WFSParameterIDs::inputJitter, 0.0f);
        jitterMeters = juce::jlimit(0.0f, 10.0f, jitterMeters);
        float jitterSliderVal = std::sqrt(jitterMeters / 10.0f);
        jitterSlider.setValue(juce::jlimit(0.0f, 1.0f, jitterSliderVal));
        jitterValueLabel.setText(juce::String(jitterMeters, 2) + " m", juce::dontSendNotification);

        // ==================== AUTOMOTION TAB ====================
        otomoDestXEditor.setText(juce::String(getFloatParam(WFSParameterIDs::inputOtomoX, 0.0f), 2), juce::dontSendNotification);
        otomoDestYEditor.setText(juce::String(getFloatParam(WFSParameterIDs::inputOtomoY, 0.0f), 2), juce::dontSendNotification);
        otomoDestZEditor.setText(juce::String(getFloatParam(WFSParameterIDs::inputOtomoZ, 0.0f), 2), juce::dontSendNotification);

        bool absRel = getIntParam(WFSParameterIDs::inputOtomoAbsoluteRelative, 0) != 0;
        otomoAbsRelButton.setToggleState(absRel, juce::dontSendNotification);
        otomoAbsRelButton.setButtonText(absRel ? "Relative" : "Absolute");

        bool stayReturn = getIntParam(WFSParameterIDs::inputOtomoStayReturn, 0) != 0;
        otomoStayReturnButton.setToggleState(stayReturn, juce::dontSendNotification);
        otomoStayReturnButton.setButtonText(stayReturn ? "Return" : "Stay");

        // Speed Profile stored as percent (0-100), default 0
        int speedProfilePct = getIntParam(WFSParameterIDs::inputOtomoSpeedProfile, 0);
        speedProfilePct = juce::jlimit(0, 100, speedProfilePct);
        otomoSpeedProfileDial.setValue(speedProfilePct / 100.0f);
        otomoSpeedProfileValueLabel.setText(juce::String(speedProfilePct) + " %", juce::dontSendNotification);

        bool trigger = getIntParam(WFSParameterIDs::inputOtomoTrigger, 0) != 0;
        otomoTriggerButton.setToggleState(trigger, juce::dontSendNotification);
        otomoTriggerButton.setButtonText(trigger ? "Trigger" : "Manual");

        // Threshold stored as dB (-92 to 0), default -20 dB
        // Inverse of: dB = 20*log10(minLin + (1-minLin)*v^2)
        float threshDB = getFloatParam(WFSParameterIDs::inputOtomoThreshold, -20.0f);
        threshDB = juce::jlimit(-92.0f, 0.0f, threshDB);
        float otomoMinLinear = std::pow(10.0f, -92.0f / 20.0f);
        float threshLinear = std::pow(10.0f, threshDB / 20.0f);
        float threshSlider = std::sqrt((threshLinear - otomoMinLinear) / (1.0f - otomoMinLinear));
        otomoThresholdDial.setValue(juce::jlimit(0.0f, 1.0f, threshSlider));
        otomoThresholdValueLabel.setText(juce::String(threshDB, 1) + " dB", juce::dontSendNotification);

        // Reset stored as dB (-92 to 0), default -60 dB
        float resetDB = getFloatParam(WFSParameterIDs::inputOtomoReset, -60.0f);
        resetDB = juce::jlimit(-92.0f, 0.0f, resetDB);
        float resetLinear = std::pow(10.0f, resetDB / 20.0f);
        float resetSlider = std::sqrt((resetLinear - otomoMinLinear) / (1.0f - otomoMinLinear));
        otomoResetDial.setValue(juce::jlimit(0.0f, 1.0f, resetSlider));
        otomoResetValueLabel.setText(juce::String(resetDB, 1) + " dB", juce::dontSendNotification);

        bool pauseResume = getIntParam(WFSParameterIDs::inputOtomoPauseResume, 0) != 0;
        otomoPauseButton.setToggleState(pauseResume, juce::dontSendNotification);

        // ==================== MUTES TAB ====================
        juce::String muteStr = getStringParam(WFSParameterIDs::inputMutes, "");
        if (muteStr.isNotEmpty())
        {
            juce::StringArray muteValues;
            muteValues.addTokens(muteStr, ",", "");
            for (int i = 0; i < juce::jmin(64, muteValues.size()); ++i)
                muteButtons[i].setToggleState(muteValues[i].getIntValue() != 0, juce::dontSendNotification);
        }
        else
        {
            for (int i = 0; i < 64; ++i)
                muteButtons[i].setToggleState(false, juce::dontSendNotification);
        }

        isLoadingParameters = false;
    }

    // ==================== TEXT EDITOR LISTENER ====================

    void textEditorReturnKeyPressed(juce::TextEditor& editor) override
    {
        editor.giveAwayKeyboardFocus();
    }

    void textEditorFocusLost(juce::TextEditor& editor) override
    {
        if (isLoadingParameters) return;

        // Header - Input Name
        if (&editor == &nameEditor)
        {
            saveInputParam(WFSParameterIDs::inputName, nameEditor.getText());
        }
        // Position tab - Position and Offset editors
        else if (&editor == &posXEditor)
        {
            saveInputParam(WFSParameterIDs::inputPositionX, editor.getText().getFloatValue());
        }
        else if (&editor == &posYEditor)
        {
            saveInputParam(WFSParameterIDs::inputPositionY, editor.getText().getFloatValue());
        }
        else if (&editor == &posZEditor)
        {
            saveInputParam(WFSParameterIDs::inputPositionZ, editor.getText().getFloatValue());
        }
        else if (&editor == &offsetXEditor)
        {
            saveInputParam(WFSParameterIDs::inputOffsetX, editor.getText().getFloatValue());
        }
        else if (&editor == &offsetYEditor)
        {
            saveInputParam(WFSParameterIDs::inputOffsetY, editor.getText().getFloatValue());
        }
        else if (&editor == &offsetZEditor)
        {
            saveInputParam(WFSParameterIDs::inputOffsetZ, editor.getText().getFloatValue());
        }
        // AutomOtion tab - Destination editors
        else if (&editor == &otomoDestXEditor)
        {
            saveInputParam(WFSParameterIDs::inputOtomoX, editor.getText().getFloatValue());
        }
        else if (&editor == &otomoDestYEditor)
        {
            saveInputParam(WFSParameterIDs::inputOtomoY, editor.getText().getFloatValue());
        }
        else if (&editor == &otomoDestZEditor)
        {
            saveInputParam(WFSParameterIDs::inputOtomoZ, editor.getText().getFloatValue());
        }
    }

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

    void storeInputConfiguration()
    {
        auto& fileManager = parameters.getFileManager();
        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage("Please select a project folder in System Config first.");
            return;
        }
        if (fileManager.saveInputConfig())
            showStatusMessage("Input configuration saved.");
        else
            showStatusMessage("Error: " + fileManager.getLastError());
    }

    void reloadInputConfiguration()
    {
        auto& fileManager = parameters.getFileManager();
        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage("Please select a project folder in System Config first.");
            return;
        }
        if (fileManager.loadInputConfig())
        {
            loadChannelParameters(currentChannel);
            showStatusMessage("Input configuration loaded.");
        }
        else
            showStatusMessage("Error: " + fileManager.getLastError());
    }

    void reloadInputConfigBackup()
    {
        auto& fileManager = parameters.getFileManager();
        if (fileManager.loadInputConfigBackup(0))
        {
            loadChannelParameters(currentChannel);
            showStatusMessage("Input configuration loaded from backup.");
        }
        else
            showStatusMessage("Error: " + fileManager.getLastError());
    }

    void importInputConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser>("Import Input Configuration",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.xml");
        auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result.existsAsFile())
            {
                auto& fileManager = parameters.getFileManager();
                if (fileManager.importInputConfig(result))
                {
                    loadChannelParameters(currentChannel);
                    showStatusMessage("Input configuration imported.");
                }
                else
                    showStatusMessage("Error: " + fileManager.getLastError());
            }
        });
    }

    void exportInputConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser>("Export Input Configuration",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.xml");
        auto chooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result != juce::File())
            {
                if (!result.hasFileExtension(".xml"))
                    result = result.withFileExtension(".xml");

                auto& fileManager = parameters.getFileManager();
                if (fileManager.exportInputConfig(result))
                    showStatusMessage("Input configuration exported.");
                else
                    showStatusMessage("Error: " + fileManager.getLastError());
            }
        });
    }

    void storeNewSnapshot()
    {
        // TODO: Implement snapshot creation dialog
        showStatusMessage("Snapshot feature not yet implemented.");
    }

    void reloadSnapshot()
    {
        // TODO: Implement snapshot loading
        showStatusMessage("Snapshot feature not yet implemented.");
    }

    void updateSnapshot()
    {
        // TODO: Implement snapshot update
        showStatusMessage("Snapshot feature not yet implemented.");
    }

    void editSnapshotScope()
    {
        // TODO: Implement snapshot scope editing
        showStatusMessage("Snapshot feature not yet implemented.");
    }

    void deleteSnapshot()
    {
        // TODO: Implement snapshot deletion
        showStatusMessage("Snapshot feature not yet implemented.");
    }

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

    // ==================== VALUETREE LISTENER ====================

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override
    {
        // Check if input channel count changed
        if (tree == configTree && property == WFSParameterIDs::inputChannels)
        {
            int numInputs = parameters.getNumInputChannels();
            if (numInputs > 0)
            {
                channelSelector.setNumChannels(numInputs);
                // If current selection is beyond new limit, reset to 1
                if (channelSelector.getSelectedChannel() > numInputs)
                    channelSelector.setSelectedChannel(1);
            }
        }

        // Check if output channel count changed (affects mute buttons)
        if (tree == configTree && property == WFSParameterIDs::outputChannels)
        {
            // Update mute button visibility and layout if Mutes tab is visible
            if (subTabBar.getCurrentTabIndex() == 7)  // Mutes tab
            {
                setMutesVisible(true);
                layoutMutesTab();
            }
        }

        // Check if this is a parameter change for the current channel (e.g., from OSC)
        // Skip if we're already loading parameters (avoid recursion)
        if (!isLoadingParameters)
        {
            DBG("InputsTab::valueTreePropertyChanged - tree=" << tree.getType().toString()
                << " property=" << property.toString() << " isLoading=" << (isLoadingParameters ? "yes" : "no"));

            // Find if this tree belongs to the current channel's Input tree
            juce::ValueTree parent = tree;
            while (parent.isValid())
            {
                DBG("InputsTab - checking parent type: " << parent.getType().toString());
                if (parent.getType() == WFSParameterIDs::Input)
                {
                    int channelId = parent.getProperty(WFSParameterIDs::id, -1);
                    DBG("InputsTab - found Input parent, channelId=" << channelId << " currentChannel=" << currentChannel);
                    if (channelId == currentChannel)
                    {
                        // This is a parameter change for the current channel - refresh UI
                        DBG("InputsTab - refreshing UI for channel " << currentChannel);
                        juce::MessageManager::callAsync([this]()
                        {
                            loadChannelParameters(currentChannel);
                        });
                    }
                    break;
                }
                parent = parent.getParent();
            }
        }
    }

    void valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&) override {}
    void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged(juce::ValueTree&) override {}

    // ==================== HELPER METHODS ====================

    void showStatusMessage(const juce::String& message)
    {
        if (statusBar != nullptr)
            statusBar->showTemporaryMessage(message, 3000);
    }

    void saveInputParam(const juce::Identifier& paramId, const juce::var& value)
    {
        if (isLoadingParameters) return;
        parameters.setInputParam(currentChannel - 1, paramId.toString(), value);
    }

    void saveMuteStates()
    {
        if (isLoadingParameters) return;
        juce::StringArray muteValues;
        for (int i = 0; i < 64; ++i)
            muteValues.add(muteButtons[i].getToggleState() ? "1" : "0");
        parameters.setInputParam(currentChannel - 1, WFSParameterIDs::inputMutes.toString(), muteValues.joinIntoString(","));
    }

    // ==================== MEMBER VARIABLES ====================

    WfsParameters& parameters;
    juce::ValueTree inputsTree;
    juce::ValueTree configTree;
    bool isLoadingParameters = false;
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
    juce::Label jitterLabel;
    WfsWidthExpansionSlider jitterSlider;
    juce::Label jitterValueLabel;

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
    PlayButton otomoStartButton;
    StopButton otomoStopButton;
    PauseButton otomoPauseButton;

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
