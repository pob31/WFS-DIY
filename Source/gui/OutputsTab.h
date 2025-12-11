#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"
#include "ChannelSelector.h"
#include "SliderUIComponents.h"
#include "DialUIComponents.h"
#include "StatusBar.h"

/**
 * Outputs Tab Component
 * Configuration for output channels with sub-tabs for different parameter groups.
 *
 * Structure:
 * - Header: Channel selector + Name editor + Array settings (always visible)
 * - Sub-tabs: Output Properties, Position (EQ to be added later)
 * - Footer: Store/Reload buttons (always visible)
 */
class OutputsTab : public juce::Component,
                   private juce::TextEditor::Listener,
                   private juce::ChangeListener
{
public:
    OutputsTab(WfsParameters& params)
        : parameters(params)
    {
        // ==================== HEADER SECTION ====================
        // Channel Selector
        channelSelector.setNumChannels(64);  // Will be updated from parameters
        channelSelector.onChannelChanged = [this](int channel) {
            loadChannelParameters(channel);
        };
        addAndMakeVisible(channelSelector);

        // Output Name
        addAndMakeVisible(nameLabel);
        nameLabel.setText("Name:", juce::dontSendNotification);
        nameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(nameEditor);
        nameEditor.addListener(this);

        // Array selector
        addAndMakeVisible(arrayLabel);
        arrayLabel.setText("Array:", juce::dontSendNotification);
        arrayLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(arraySelector);
        arraySelector.addItem("Single", 1);
        for (int i = 1; i <= 10; ++i)
            arraySelector.addItem("Array " + juce::String(i), i + 1);
        arraySelector.setSelectedId(1, juce::dontSendNotification);
        arraySelector.onChange = [this]() { updateArrayParameter(); };

        // Apply to Array selector
        addAndMakeVisible(applyToArrayLabel);
        applyToArrayLabel.setText("Apply to Array:", juce::dontSendNotification);
        applyToArrayLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(applyToArraySelector);
        applyToArraySelector.addItem("OFF", 1);
        applyToArraySelector.addItem("ABSOLUTE", 2);
        applyToArraySelector.addItem("RELATIVE", 3);
        applyToArraySelector.setSelectedId(2, juce::dontSendNotification);
        applyToArraySelector.onChange = [this]() { updateApplyToArrayParameter(); };

        // ==================== SUB-TABS ====================
        addAndMakeVisible(subTabBar);
        subTabBar.addTab("Output Properties", juce::Colour(0xFF2A2A2A), -1);
        subTabBar.addTab("Position", juce::Colour(0xFF2A2A2A), -1);
        // EQ tab will be added later
        subTabBar.setCurrentTabIndex(0);
        subTabBar.addChangeListener(static_cast<juce::ChangeListener*>(this));

        // ==================== OUTPUT PROPERTIES SUB-TAB ====================
        setupOutputPropertiesTab();

        // ==================== POSITION SUB-TAB ====================
        setupPositionTab();

        // ==================== FOOTER - STORE/RELOAD BUTTONS ====================
        addAndMakeVisible(storeButton);
        storeButton.setButtonText("Store Output Config");
        storeButton.onClick = [this]() { storeOutputConfiguration(); };

        addAndMakeVisible(reloadButton);
        reloadButton.setButtonText("Reload Output Config");
        reloadButton.onClick = [this]() { reloadOutputConfiguration(); };

        addAndMakeVisible(reloadBackupButton);
        reloadBackupButton.setButtonText("Reload Backup");
        reloadBackupButton.onClick = [this]() { reloadOutputConfigBackup(); };

        addAndMakeVisible(importButton);
        importButton.setButtonText("Import");
        importButton.onClick = [this]() { importOutputConfiguration(); };

        addAndMakeVisible(exportButton);
        exportButton.setButtonText("Export");
        exportButton.onClick = [this]() { exportOutputConfiguration(); };

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

        // First row: Channel selector and Name
        auto row1 = headerArea.removeFromTop(rowHeight);
        channelSelector.setBounds(row1.removeFromLeft(150));
        row1.removeFromLeft(spacing * 2);
        nameLabel.setBounds(row1.removeFromLeft(50));
        nameEditor.setBounds(row1.removeFromLeft(200));
        row1.removeFromLeft(spacing * 4);

        // Array and Apply to Array in same row
        arrayLabel.setBounds(row1.removeFromLeft(50));
        arraySelector.setBounds(row1.removeFromLeft(100));
        row1.removeFromLeft(spacing * 2);
        applyToArrayLabel.setBounds(row1.removeFromLeft(100));
        applyToArraySelector.setBounds(row1.removeFromLeft(100));

        // ==================== FOOTER ====================
        auto footerArea = bounds.removeFromBottom(footerHeight).reduced(padding, padding);
        const int buttonWidth = (footerArea.getWidth() - spacing * 4) / 5;

        storeButton.setBounds(footerArea.removeFromLeft(buttonWidth));
        footerArea.removeFromLeft(spacing);
        reloadButton.setBounds(footerArea.removeFromLeft(buttonWidth));
        footerArea.removeFromLeft(spacing);
        reloadBackupButton.setBounds(footerArea.removeFromLeft(buttonWidth));
        footerArea.removeFromLeft(spacing);
        importButton.setBounds(footerArea.removeFromLeft(buttonWidth));
        footerArea.removeFromLeft(spacing);
        exportButton.setBounds(footerArea.removeFromLeft(buttonWidth));

        // ==================== SUB-TABS AREA ====================
        auto contentArea = bounds.reduced(padding, 0);
        auto tabBarArea = contentArea.removeFromTop(32);
        subTabBar.setBounds(tabBarArea);

        // Content area for sub-tabs
        subTabContentArea = contentArea.reduced(0, padding);

        // Layout sub-tab content based on current tab
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

    void setupOutputPropertiesTab()
    {
        // Attenuation slider (-92 to 0 dB)
        addAndMakeVisible(attenuationLabel);
        attenuationLabel.setText("Attenuation:", juce::dontSendNotification);
        attenuationLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        attenuationSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFFFF5722));
        attenuationSlider.onValueChanged = [this](float v) {
            // Convert 0-1 to -92 to 0 dB with logarithmic scaling
            float dB = 20.0f * std::log10(std::pow(10.0f, -92.0f / 20.0f) +
                       ((1.0f - std::pow(10.0f, -92.0f / 20.0f)) * v * v));
            attenuationValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
        };
        addAndMakeVisible(attenuationSlider);

        addAndMakeVisible(attenuationValueLabel);
        attenuationValueLabel.setText("0.0 dB", juce::dontSendNotification);
        attenuationValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);

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

        // Min Latency Enable button
        addAndMakeVisible(minLatencyEnableButton);
        minLatencyEnableButton.setButtonText("Min Latency: ON");
        minLatencyEnableButton.setClickingTogglesState(true);
        minLatencyEnableButton.setToggleState(true, juce::dontSendNotification);
        minLatencyEnableButton.onClick = [this]() {
            bool enabled = minLatencyEnableButton.getToggleState();
            minLatencyEnableButton.setButtonText(enabled ? "Min Latency: ON" : "Min Latency: OFF");
        };

        // Live Source Enable button
        addAndMakeVisible(liveSourceEnableButton);
        liveSourceEnableButton.setButtonText("Live Source Atten: ON");
        liveSourceEnableButton.setClickingTogglesState(true);
        liveSourceEnableButton.setToggleState(true, juce::dontSendNotification);
        liveSourceEnableButton.onClick = [this]() {
            bool enabled = liveSourceEnableButton.getToggleState();
            liveSourceEnableButton.setButtonText(enabled ? "Live Source Atten: ON" : "Live Source Atten: OFF");
        };

        // Distance Attenuation % slider (0-200%, default 100% in center)
        addAndMakeVisible(distanceAttenLabel);
        distanceAttenLabel.setText("Distance Atten:", juce::dontSendNotification);
        distanceAttenLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        distanceAttenSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFF9C27B0));
        // Bidirectional slider: center (0) = 100%, no need to set initial value
        distanceAttenSlider.onValueChanged = [this](float v) {
            // Slider range is -1 to 1, map to 0% to 200% (center = 100%)
            int percent = static_cast<int>((v + 1.0f) * 100.0f);
            distanceAttenValueLabel.setText(juce::String(percent) + " %", juce::dontSendNotification);
        };
        addAndMakeVisible(distanceAttenSlider);

        addAndMakeVisible(distanceAttenValueLabel);
        distanceAttenValueLabel.setText("100 %", juce::dontSendNotification);
        distanceAttenValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        // Horizontal Parallax
        addAndMakeVisible(hParallaxLabel);
        hParallaxLabel.setText("H Parallax:", juce::dontSendNotification);
        hParallaxLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(hParallaxEditor);
        hParallaxEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(hParallaxEditor, false, true);
        addAndMakeVisible(hParallaxUnitLabel);
        hParallaxUnitLabel.setText("m", juce::dontSendNotification);
        hParallaxUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        // Vertical Parallax
        addAndMakeVisible(vParallaxLabel);
        vParallaxLabel.setText("V Parallax:", juce::dontSendNotification);
        vParallaxLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(vParallaxEditor);
        vParallaxEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(vParallaxEditor, true, true);
        addAndMakeVisible(vParallaxUnitLabel);
        vParallaxUnitLabel.setText("m", juce::dontSendNotification);
        vParallaxUnitLabel.setColour(juce::Label::textColourId, juce::Colours::white);
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

        // Orientation dial
        addAndMakeVisible(orientationLabel);
        orientationLabel.setText("Orientation:", juce::dontSendNotification);
        orientationLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        orientationDial.setColours(juce::Colours::black, juce::Colours::white, juce::Colours::grey);
        orientationDial.onAngleChanged = [this](float angle) {
            orientationValueLabel.setText(juce::String(static_cast<int>(angle)) + juce::String::fromUTF8("°"), juce::dontSendNotification);
        };
        addAndMakeVisible(orientationDial);
        addAndMakeVisible(orientationValueLabel);
        orientationValueLabel.setText(juce::String::fromUTF8("0°"), juce::dontSendNotification);
        orientationValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        orientationValueLabel.setJustificationType(juce::Justification::centred);

        // Angle On slider (1-180°)
        addAndMakeVisible(angleOnLabel);
        angleOnLabel.setText("Angle On:", juce::dontSendNotification);
        angleOnLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        angleOnSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFF00BCD4));
        angleOnSlider.setValue(0.47f);  // ~86°
        angleOnSlider.onValueChanged = [this](float v) {
            int degrees = static_cast<int>(v * 179.0f + 1.0f);
            angleOnValueLabel.setText(juce::String(degrees) + juce::String::fromUTF8("°"), juce::dontSendNotification);
        };
        addAndMakeVisible(angleOnSlider);
        addAndMakeVisible(angleOnValueLabel);
        angleOnValueLabel.setText(juce::String::fromUTF8("86°"), juce::dontSendNotification);
        angleOnValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        // Angle Off slider (0-179°)
        addAndMakeVisible(angleOffLabel);
        angleOffLabel.setText("Angle Off:", juce::dontSendNotification);
        angleOffLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        angleOffSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFF00BCD4));
        angleOffSlider.setValue(0.5f);  // ~90°
        angleOffSlider.onValueChanged = [this](float v) {
            int degrees = static_cast<int>(v * 179.0f);
            angleOffValueLabel.setText(juce::String(degrees) + juce::String::fromUTF8("°"), juce::dontSendNotification);
        };
        addAndMakeVisible(angleOffSlider);
        addAndMakeVisible(angleOffValueLabel);
        angleOffValueLabel.setText(juce::String::fromUTF8("90°"), juce::dontSendNotification);
        angleOffValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        // Pitch slider (-90 to 90°)
        addAndMakeVisible(pitchLabel);
        pitchLabel.setText("Pitch:", juce::dontSendNotification);
        pitchLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        pitchSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFF2196F3));
        pitchSlider.onValueChanged = [this](float v) {
            // Slider range is -1 to 1, map to -90° to 90°
            int degrees = static_cast<int>(v * 90.0f);
            pitchValueLabel.setText(juce::String(degrees) + juce::String::fromUTF8("°"), juce::dontSendNotification);
        };
        addAndMakeVisible(pitchSlider);
        addAndMakeVisible(pitchValueLabel);
        pitchValueLabel.setText(juce::String::fromUTF8("0°"), juce::dontSendNotification);
        pitchValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        // HF Damping slider (-6 to 0 dB/m)
        addAndMakeVisible(hfDampingLabel);
        hfDampingLabel.setText("HF Damping:", juce::dontSendNotification);
        hfDampingLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        hfDampingSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFFFF9800));
        hfDampingSlider.setValue(1.0f);  // 0 dB/m
        hfDampingSlider.onValueChanged = [this](float v) {
            float dBm = v * 6.0f - 6.0f;
            hfDampingValueLabel.setText(juce::String(dBm, 1) + " dB/m", juce::dontSendNotification);
        };
        addAndMakeVisible(hfDampingSlider);
        addAndMakeVisible(hfDampingValueLabel);
        hfDampingValueLabel.setText("0.0 dB/m", juce::dontSendNotification);
        hfDampingValueLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        // Array Position Helper button
        addAndMakeVisible(arrayPositionHelperButton);
        arrayPositionHelperButton.setButtonText("Array Position Helper...");
        arrayPositionHelperButton.onClick = [this]() { openArrayPositionHelper(); };
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

    // ==================== LAYOUT METHODS ====================

    void layoutCurrentSubTab()
    {
        int tabIndex = subTabBar.getCurrentTabIndex();

        // Hide all components first
        setOutputPropertiesVisible(false);
        setPositionVisible(false);

        // Show and layout current tab
        if (tabIndex == 0)
        {
            setOutputPropertiesVisible(true);
            layoutOutputPropertiesTab();
        }
        else if (tabIndex == 1)
        {
            setPositionVisible(true);
            layoutPositionTab();
        }
    }

    void setOutputPropertiesVisible(bool visible)
    {
        attenuationLabel.setVisible(visible);
        attenuationSlider.setVisible(visible);
        attenuationValueLabel.setVisible(visible);
        delayLatencyLabel.setVisible(visible);
        delayLatencySlider.setVisible(visible);
        delayLatencyValueLabel.setVisible(visible);
        minLatencyEnableButton.setVisible(visible);
        liveSourceEnableButton.setVisible(visible);
        distanceAttenLabel.setVisible(visible);
        distanceAttenSlider.setVisible(visible);
        distanceAttenValueLabel.setVisible(visible);
        hParallaxLabel.setVisible(visible);
        hParallaxEditor.setVisible(visible);
        hParallaxUnitLabel.setVisible(visible);
        vParallaxLabel.setVisible(visible);
        vParallaxEditor.setVisible(visible);
        vParallaxUnitLabel.setVisible(visible);
    }

    void setPositionVisible(bool visible)
    {
        posXLabel.setVisible(visible);
        posXEditor.setVisible(visible);
        posXUnitLabel.setVisible(visible);
        posYLabel.setVisible(visible);
        posYEditor.setVisible(visible);
        posYUnitLabel.setVisible(visible);
        posZLabel.setVisible(visible);
        posZEditor.setVisible(visible);
        posZUnitLabel.setVisible(visible);
        orientationLabel.setVisible(visible);
        orientationDial.setVisible(visible);
        orientationValueLabel.setVisible(visible);
        angleOnLabel.setVisible(visible);
        angleOnSlider.setVisible(visible);
        angleOnValueLabel.setVisible(visible);
        angleOffLabel.setVisible(visible);
        angleOffSlider.setVisible(visible);
        angleOffValueLabel.setVisible(visible);
        pitchLabel.setVisible(visible);
        pitchSlider.setVisible(visible);
        pitchValueLabel.setVisible(visible);
        hfDampingLabel.setVisible(visible);
        hfDampingSlider.setVisible(visible);
        hfDampingValueLabel.setVisible(visible);
        arrayPositionHelperButton.setVisible(visible);
    }

    void layoutOutputPropertiesTab()
    {
        auto area = subTabContentArea;
        const int rowHeight = 30;
        const int sliderHeight = 40;
        const int spacing = 10;
        const int labelWidth = 120;
        const int valueWidth = 80;

        // Left column
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
        delayLatencyValueLabel.setBounds(row.removeFromRight(valueWidth + 20));
        leftCol.removeFromTop(spacing / 2);
        delayLatencySlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing);

        // Distance Attenuation
        row = leftCol.removeFromTop(rowHeight);
        distanceAttenLabel.setBounds(row.removeFromLeft(labelWidth));
        distanceAttenValueLabel.setBounds(row.removeFromRight(valueWidth));
        leftCol.removeFromTop(spacing / 2);
        distanceAttenSlider.setBounds(leftCol.removeFromTop(sliderHeight));

        // Right column
        auto rightCol = area.reduced(5, 0);

        // Enable buttons
        row = rightCol.removeFromTop(rowHeight);
        minLatencyEnableButton.setBounds(row.removeFromLeft(180));
        rightCol.removeFromTop(spacing);

        row = rightCol.removeFromTop(rowHeight);
        liveSourceEnableButton.setBounds(row.removeFromLeft(180));
        rightCol.removeFromTop(spacing * 2);

        // Parallax editors
        row = rightCol.removeFromTop(rowHeight);
        hParallaxLabel.setBounds(row.removeFromLeft(80));
        hParallaxEditor.setBounds(row.removeFromLeft(100));
        row.removeFromLeft(5);
        hParallaxUnitLabel.setBounds(row.removeFromLeft(30));
        rightCol.removeFromTop(spacing);

        row = rightCol.removeFromTop(rowHeight);
        vParallaxLabel.setBounds(row.removeFromLeft(80));
        vParallaxEditor.setBounds(row.removeFromLeft(100));
        row.removeFromLeft(5);
        vParallaxUnitLabel.setBounds(row.removeFromLeft(30));
    }

    void layoutPositionTab()
    {
        auto area = subTabContentArea;
        const int rowHeight = 30;
        const int sliderHeight = 40;
        const int spacing = 10;
        const int labelWidth = 100;
        const int valueWidth = 60;
        const int editorWidth = 80;
        const int unitWidth = 30;

        // Left column - Position editors and sliders
        auto leftCol = area.removeFromLeft(area.getWidth() * 2 / 3).reduced(5, 0);

        // Position X, Y, Z in a row
        auto posRow = leftCol.removeFromTop(rowHeight);
        posXLabel.setBounds(posRow.removeFromLeft(labelWidth - 20));
        posXEditor.setBounds(posRow.removeFromLeft(editorWidth));
        posXUnitLabel.setBounds(posRow.removeFromLeft(unitWidth));
        posRow.removeFromLeft(spacing);
        posYLabel.setBounds(posRow.removeFromLeft(labelWidth - 20));
        posYEditor.setBounds(posRow.removeFromLeft(editorWidth));
        posYUnitLabel.setBounds(posRow.removeFromLeft(unitWidth));
        posRow.removeFromLeft(spacing);
        posZLabel.setBounds(posRow.removeFromLeft(labelWidth - 20));
        posZEditor.setBounds(posRow.removeFromLeft(editorWidth));
        posZUnitLabel.setBounds(posRow.removeFromLeft(unitWidth));
        leftCol.removeFromTop(spacing * 2);

        // Angle On
        auto row = leftCol.removeFromTop(rowHeight);
        angleOnLabel.setBounds(row.removeFromLeft(labelWidth));
        angleOnValueLabel.setBounds(row.removeFromRight(valueWidth));
        leftCol.removeFromTop(spacing / 2);
        angleOnSlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing);

        // Angle Off
        row = leftCol.removeFromTop(rowHeight);
        angleOffLabel.setBounds(row.removeFromLeft(labelWidth));
        angleOffValueLabel.setBounds(row.removeFromRight(valueWidth));
        leftCol.removeFromTop(spacing / 2);
        angleOffSlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing);

        // Pitch
        row = leftCol.removeFromTop(rowHeight);
        pitchLabel.setBounds(row.removeFromLeft(labelWidth));
        pitchValueLabel.setBounds(row.removeFromRight(valueWidth));
        leftCol.removeFromTop(spacing / 2);
        pitchSlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing);

        // HF Damping
        row = leftCol.removeFromTop(rowHeight);
        hfDampingLabel.setBounds(row.removeFromLeft(labelWidth));
        hfDampingValueLabel.setBounds(row.removeFromRight(valueWidth));
        leftCol.removeFromTop(spacing / 2);
        hfDampingSlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing);

        // Array Position Helper button
        arrayPositionHelperButton.setBounds(leftCol.removeFromTop(rowHeight).withWidth(200));

        // Right column - Orientation dial
        auto rightCol = area.reduced(5, 0);
        orientationLabel.setBounds(rightCol.removeFromTop(rowHeight));

        const int dialSize = juce::jmin(150, rightCol.getHeight() - 40);
        auto dialArea = rightCol.removeFromTop(dialSize);
        orientationDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));

        orientationValueLabel.setBounds(rightCol.removeFromTop(rowHeight));
    }

    // ==================== PARAMETER MANAGEMENT ====================

    void loadChannelParameters(int channel)
    {
        currentChannel = channel;
        // TODO: Load actual parameters from WfsParameters
        // For now, set defaults
        nameEditor.setText("Output " + juce::String(channel), juce::dontSendNotification);
        arraySelector.setSelectedId(1, juce::dontSendNotification);
        applyToArraySelector.setSelectedId(2, juce::dontSendNotification);

        // Update Apply to Array enabled state based on array selection
        updateApplyToArrayEnabledState();
    }

    void updateArrayParameter()
    {
        updateApplyToArrayEnabledState();
        // TODO: Save to parameters
    }

    void updateApplyToArrayParameter()
    {
        // TODO: Save to parameters
    }

    void updateApplyToArrayEnabledState()
    {
        bool isPartOfArray = arraySelector.getSelectedId() > 1;
        applyToArraySelector.setEnabled(isPartOfArray);
        applyToArrayLabel.setAlpha(isPartOfArray ? 1.0f : 0.5f);
    }

    // ==================== TEXT EDITOR LISTENER ====================

    void textEditorReturnKeyPressed(juce::TextEditor& editor) override
    {
        editor.giveAwayKeyboardFocus();
    }

    void textEditorFocusLost(juce::TextEditor&) override
    {
        // TODO: Validate and save values
    }

    // ==================== STORE/RELOAD METHODS ====================

    void storeOutputConfiguration()
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
            "Store Output Configuration", "Output configuration stored.");
    }

    void reloadOutputConfiguration()
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
            "Reload Output Configuration", "Output configuration reloaded.");
    }

    void reloadOutputConfigBackup()
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
            "Reload Backup", "Output configuration backup reloaded.");
    }

    void importOutputConfiguration()
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
            "Import", "Import output configuration.");
    }

    void exportOutputConfiguration()
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
            "Export", "Export output configuration.");
    }

    void openArrayPositionHelper()
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
            "Array Position Helper", "Array position helper window will open here.");
    }

    //==============================================================================
    // Status bar helper methods

    void setupHelpText()
    {
        // Based on WFS-UI_output.csv help text column
        helpTextMap[&channelSelector] = "Output Channel Number and Selection.";
        helpTextMap[&nameEditor] = "Displayed Output Channel Name (editable).";
        helpTextMap[&arraySelector] = "Selected Output Channel is part of Array.";
        helpTextMap[&applyToArraySelector] = "Apply Changes to the rest of the Array (Absolute value or Relative changes).";
        helpTextMap[&attenuationSlider] = "Output Channel Attenuation. (changes may affect the rest of the array)";
        helpTextMap[&delayLatencySlider] = "Output Channel Delay (positive values) or Latency Compensation (negative values). (changes may affect the rest of the array)";
        helpTextMap[&minLatencyEnableButton] = "Disables Minimal Latency Mode for Selected Output. (changes may affect the rest of the array)";
        helpTextMap[&liveSourceEnableButton] = "Disables Live Source Attenuation for Selected Output. (changes may affect the rest of the array)";
        helpTextMap[&distanceAttenSlider] = "Ratio of Distance Attenuation for Selected Output. (changes may affect the rest of the array)";
        helpTextMap[&hParallaxEditor] = "Horizontal Distance from Speaker to 'Targeted' Listener. (changes may affect the rest of the array)";
        helpTextMap[&vParallaxEditor] = "Vertical Distance from Speaker to 'Targeted' Listener. Positive when the Speaker is Below the head of the Listener. (changes may affect the rest of the array)";
        helpTextMap[&posXEditor] = "Output Channel Position in Width.";
        helpTextMap[&posYEditor] = "Output Channel Position in Depth.";
        helpTextMap[&posZEditor] = "Output Channel Position in Height.";
        helpTextMap[&orientationDial] = "Output Channel Horizontal Orientation (0 degrees means towards the audience in a frontal configuration). (changes may affect the rest of the array)";
        helpTextMap[&angleOnSlider] = "Output Channel Will Amplify Objects in this Angle in its Back. (changes may affect the rest of the array)";
        helpTextMap[&angleOffSlider] = "Output Channel Will Not Amplify Objects in this Angle in Front of it. (changes may affect the rest of the array)";
        helpTextMap[&pitchSlider] = "Output Channel Vertical Orientation used to Determine which Objects get amplified. (changes may affect the rest of the array)";
        helpTextMap[&hfDampingSlider] = "Loss of High Frequency Depending on Distance from Object to Output. (changes may affect the rest of the array)";
        helpTextMap[&arrayPositionHelperButton] = "Open Helper Window to Position Speaker Arrays Conveniently.";
        helpTextMap[&storeButton] = "Store Output Configuration to file (overwrite with confirmation)";
        helpTextMap[&reloadButton] = "Reload Output Configuration from file (with confirmation)";
        helpTextMap[&reloadBackupButton] = "Reload Output Configuration from backup file (with confirmation)";
        helpTextMap[&importButton] = "Import Output Configuration from file (with file explorer window)";
        helpTextMap[&exportButton] = "Export Output Configuration to file (with file explorer window)";
    }

    void setupOscMethods()
    {
        // Based on WFS-UI_output.csv OSC path column
        oscMethodMap[&channelSelector] = "/wfs/output/selected <ID>";
        oscMethodMap[&nameEditor] = "/wfs/output/name <ID> <value>";
        oscMethodMap[&arraySelector] = "/wfs/output/array <ID> <value>";
        oscMethodMap[&applyToArraySelector] = "/wfs/output/applyToArray <ID> <value>";
        oscMethodMap[&attenuationSlider] = "/wfs/output/attenuation <ID> <value>";
        oscMethodMap[&delayLatencySlider] = "/wfs/output/delayLatency <ID> <value>";
        oscMethodMap[&minLatencyEnableButton] = "/wfs/output/miniLatencyEnable <ID> <value>";
        oscMethodMap[&liveSourceEnableButton] = "/wfs/output/LSenable <ID> <value>";
        oscMethodMap[&distanceAttenSlider] = "/wfs/output/DistanceAttenPercent <ID> <value>";
        oscMethodMap[&hParallaxEditor] = "/wfs/output/Hparallax <ID> <value>";
        oscMethodMap[&vParallaxEditor] = "/wfs/output/Vparallax <ID> <value>";
        oscMethodMap[&posXEditor] = "/wfs/output/positionX <ID> <value>";
        oscMethodMap[&posYEditor] = "/wfs/output/positionY <ID> <value>";
        oscMethodMap[&posZEditor] = "/wfs/output/positionZ <ID> <value>";
        oscMethodMap[&orientationDial] = "/wfs/output/orientation <ID> <value>";
        oscMethodMap[&angleOnSlider] = "/wfs/output/angleOn <ID> <value>";
        oscMethodMap[&angleOffSlider] = "/wfs/output/angleOff <ID> <value>";
        oscMethodMap[&pitchSlider] = "/wfs/output/pitch <ID> <value>";
        oscMethodMap[&hfDampingSlider] = "/wfs/output/HFdamping <ID> <value>";
    }

    void setupMouseListeners()
    {
        // Enable mouse enter/exit events for all components with help text
        for (auto& pair : helpTextMap)
        {
            pair.first->addMouseListener(this, false);
        }
    }

    void mouseEnter(const juce::MouseEvent& event) override
    {
        if (statusBar == nullptr) return;

        auto* component = event.eventComponent;

        // Set help text if available
        if (helpTextMap.find(component) != helpTextMap.end())
        {
            statusBar->setHelpText(helpTextMap[component]);
        }

        // Set OSC method if available
        if (oscMethodMap.find(component) != oscMethodMap.end())
        {
            statusBar->setOscMethod(oscMethodMap[component]);
        }
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
    static constexpr int footerHeight = 50;
    juce::Rectangle<int> subTabContentArea;

    // Header components
    ChannelSelectorButton channelSelector { "Output" };
    juce::Label nameLabel;
    juce::TextEditor nameEditor;
    juce::Label arrayLabel;
    juce::ComboBox arraySelector;
    juce::Label applyToArrayLabel;
    juce::ComboBox applyToArraySelector;

    // Sub-tab bar
    juce::TabbedButtonBar subTabBar { juce::TabbedButtonBar::TabsAtTop };

    // Output Properties tab components
    juce::Label attenuationLabel;
    WfsStandardSlider attenuationSlider;
    juce::Label attenuationValueLabel;
    juce::Label delayLatencyLabel;
    WfsBidirectionalSlider delayLatencySlider;
    juce::Label delayLatencyValueLabel;
    juce::TextButton minLatencyEnableButton;
    juce::TextButton liveSourceEnableButton;
    juce::Label distanceAttenLabel;
    WfsBidirectionalSlider distanceAttenSlider;
    juce::Label distanceAttenValueLabel;
    juce::Label hParallaxLabel;
    juce::TextEditor hParallaxEditor;
    juce::Label hParallaxUnitLabel;
    juce::Label vParallaxLabel;
    juce::TextEditor vParallaxEditor;
    juce::Label vParallaxUnitLabel;

    // Position tab components
    juce::Label posXLabel;
    juce::TextEditor posXEditor;
    juce::Label posXUnitLabel;
    juce::Label posYLabel;
    juce::TextEditor posYEditor;
    juce::Label posYUnitLabel;
    juce::Label posZLabel;
    juce::TextEditor posZEditor;
    juce::Label posZUnitLabel;
    juce::Label orientationLabel;
    WfsEndlessDial orientationDial;
    juce::Label orientationValueLabel;
    juce::Label angleOnLabel;
    WfsWidthExpansionSlider angleOnSlider;
    juce::Label angleOnValueLabel;
    juce::Label angleOffLabel;
    WfsWidthExpansionSlider angleOffSlider;
    juce::Label angleOffValueLabel;
    juce::Label pitchLabel;
    WfsBidirectionalSlider pitchSlider;
    juce::Label pitchValueLabel;
    juce::Label hfDampingLabel;
    WfsStandardSlider hfDampingSlider;
    juce::Label hfDampingValueLabel;
    juce::TextButton arrayPositionHelperButton;

    // Footer buttons
    juce::TextButton storeButton;
    juce::TextButton reloadButton;
    juce::TextButton reloadBackupButton;
    juce::TextButton importButton;
    juce::TextButton exportButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputsTab)
};
