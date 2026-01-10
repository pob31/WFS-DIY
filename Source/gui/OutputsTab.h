#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"
#include "../Parameters/WFSParameterIDs.h"
#include "ChannelSelector.h"
#include "ColorUtilities.h"
#include "ColorScheme.h"
#include "SliderUIComponents.h"
#include "DialUIComponents.h"
#include "StatusBar.h"
#include "OutputArrayHelperWindow.h"
#include "EQDisplayComponent.h"
#include "../Helpers/CoordinateConverter.h"

/**
 * Small colored indicator to show that a parameter is linked across an array.
 * Uses the array color from WfsColorUtilities.
 * - Filled disk for ABSOLUTE mode
 * - Outline circle for RELATIVE mode
 */
class ArrayLinkIndicator : public juce::Component
{
public:
    ArrayLinkIndicator() { setInterceptsMouseClicks(false, false); }

    void setArrayNumber(int arrayNum)
    {
        if (arrayNumber != arrayNum)
        {
            arrayNumber = arrayNum;
            repaint();
        }
    }

    void setFilled(bool shouldBeFilled)
    {
        if (filled != shouldBeFilled)
        {
            filled = shouldBeFilled;
            repaint();
        }
    }

    void setActive(bool shouldBeActive)
    {
        if (active != shouldBeActive)
        {
            active = shouldBeActive;
            repaint();
        }
    }

    bool isActive() const { return active; }

    void paint(juce::Graphics& g) override
    {
        if (!active || arrayNumber < 1)
            return;

        auto colour = WfsColorUtilities::getArrayColor(arrayNumber);
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        float size = juce::jmin(bounds.getWidth(), bounds.getHeight());
        auto dotBounds = bounds.withSizeKeepingCentre(size, size);

        if (filled)
        {
            // ABSOLUTE mode: filled disk
            g.setColour(colour);
            g.fillEllipse(dotBounds);
        }
        else
        {
            // RELATIVE mode: outline circle
            g.setColour(colour);
            g.drawEllipse(dotBounds.reduced(0.5f), 1.0f);
        }
    }

private:
    int arrayNumber = 0;
    bool active = false;
    bool filled = true;  // true = ABSOLUTE (disk), false = RELATIVE (circle)
};

/**
 * Outputs Tab Component
 * Configuration for output channels with sub-tabs for different parameter groups.
 *
 * Structure:
 * - Header: Channel selector + Name editor + Array settings (always visible)
 * - Sub-tabs: Output Parameters (two-column layout), Output EQ
 * - Footer: Store/Reload buttons (always visible)
 */
class OutputsTab : public juce::Component,
                   private juce::TextEditor::Listener,
                   private juce::ChangeListener,
                   private juce::Label::Listener,
                   private juce::ValueTree::Listener,
                   public ColorScheme::Manager::Listener
{
public:
    OutputsTab(WfsParameters& params)
        : parameters(params),
          outputsTree(params.getOutputTree()),
          configTree(params.getConfigTree()),
          ioTree(params.getConfigTree().getChildWithName(WFSParameterIDs::IO))
    {
        // Enable keyboard focus so we can receive focus back after text editing
        setWantsKeyboardFocus(true);

        // Add listener to outputs tree, config tree, and IO tree (for channel count changes)
        outputsTree.addListener(this);
        configTree.addListener(this);
        if (ioTree.isValid())
            ioTree.addListener(this);
        ColorScheme::Manager::getInstance().addListener(this);

        // ==================== HEADER SECTION ====================
        // Channel Selector - use configured output count
        int numOutputs = parameters.getNumOutputChannels();
        channelSelector.setNumChannels(numOutputs > 0 ? numOutputs : 16);  // Default to 16 if not set
        channelSelector.onChannelChanged = [this](int channel) {
            loadChannelParameters(channel);
        };
        // Set color provider to match array colors from Map tab
        channelSelector.setChannelColorProvider([this](int channelId) -> juce::Colour {
            // Get the array assignment for this output (0 = Single, 1-10 = Array 1-10)
            int array = static_cast<int>(parameters.getOutputParam(channelId - 1, "outputArray"));
            if (array == 0)
                return juce::Colour(0xFF2A2A2A);  // Dark gray for "Single" outputs
            else
                return WfsColorUtilities::getArrayColor(array);
        });
        // Set name provider to show output names on selector tiles
        channelSelector.setChannelNameProvider([this](int channelId) -> juce::String {
            juce::String name = parameters.getOutputParam(channelId - 1, "outputName").toString();
            return name.isEmpty() ? juce::String() : name;
        });
        // Set text color provider - white for "Single" outputs, black for array outputs
        channelSelector.setTextColorProvider([this](int channelId) -> juce::Colour {
            int array = static_cast<int>(parameters.getOutputParam(channelId - 1, "outputArray"));
            return (array == 0) ? juce::Colours::white : juce::Colours::black;
        });
        addAndMakeVisible(channelSelector);

        // Output Name
        addAndMakeVisible(nameLabel);
        nameLabel.setText("Name:", juce::dontSendNotification);
        addAndMakeVisible(nameEditor);
        nameEditor.addListener(this);

        // Array selector
        addAndMakeVisible(arrayLabel);
        arrayLabel.setText("Array:", juce::dontSendNotification);
        addAndMakeVisible(arraySelector);
        arraySelector.addItem("Single", 1);
        for (int i = 1; i <= 10; ++i)
            arraySelector.addItem("Array " + juce::String(i), i + 1);
        arraySelector.setSelectedId(1, juce::dontSendNotification);
        arraySelector.onChange = [this]() { updateArrayParameter(); };

        // Apply to Array selector
        addAndMakeVisible(applyToArrayLabel);
        applyToArrayLabel.setText("Apply to Array:", juce::dontSendNotification);
        addAndMakeVisible(applyToArraySelector);
        applyToArraySelector.addItem("OFF", 1);
        applyToArraySelector.addItem("ABSOLUTE", 2);
        applyToArraySelector.addItem("RELATIVE", 3);
        applyToArraySelector.setSelectedId(2, juce::dontSendNotification);
        applyToArraySelector.onChange = [this]() { updateApplyToArrayParameter(); };

        // Map visibility toggle button
        addAndMakeVisible(mapVisibilityButton);
        mapVisibilityButton.setButtonText("Speaker Visible on Map");
        mapVisibilityButton.onClick = [this]() { toggleMapVisibility(); };

        // Wizard of OutZ button (array position helper)
        addAndMakeVisible(arrayPositionHelperButton);
        arrayPositionHelperButton.setButtonText("Wizard of OutZ...");
        arrayPositionHelperButton.onClick = [this]() { openArrayPositionHelper(); };

        // ==================== SUB-TABS ====================
        addAndMakeVisible(subTabBar);
        subTabBar.addTab("Output Parameters", juce::Colour(0xFF2A2A2A), -1);
        subTabBar.addTab("Output EQ", juce::Colour(0xFF2A2A2A), -1);
        subTabBar.setCurrentTabIndex(0);
        subTabBar.addChangeListener(static_cast<juce::ChangeListener*>(this));

        // ==================== OUTPUT PROPERTIES SUB-TAB ====================
        setupOutputPropertiesTab();

        // ==================== POSITION SUB-TAB ====================
        setupPositionTab();

        // ==================== EQ SUB-TAB ====================
        setupEqTab();

        // ==================== FOOTER - STORE/RELOAD BUTTONS ====================
        addAndMakeVisible(storeButton);
        storeButton.setButtonText("Store Output Config");
        storeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF8C3333));  // Reddish
        storeButton.onClick = [this]() { storeOutputConfiguration(); };

        addAndMakeVisible(reloadButton);
        reloadButton.setButtonText("Reload Output Config");
        reloadButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF338C33));  // Greenish
        reloadButton.onClick = [this]() { reloadOutputConfiguration(); };

        addAndMakeVisible(reloadBackupButton);
        reloadBackupButton.setButtonText("Reload Backup");
        reloadBackupButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF266626));  // Darker green
        reloadBackupButton.onClick = [this]() { reloadOutputConfigBackup(); };

        addAndMakeVisible(importButton);
        importButton.setButtonText("Import");
        importButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF338C33));  // Greenish
        importButton.onClick = [this]() { importOutputConfiguration(); };

        addAndMakeVisible(exportButton);
        exportButton.setButtonText("Export");
        exportButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF8C3333));  // Reddish
        exportButton.onClick = [this]() { exportOutputConfiguration(); };

        // Load initial channel parameters
        loadChannelParameters(1);
    }

    ~OutputsTab() override
    {
        ColorScheme::Manager::getInstance().removeListener(this);
        outputsTree.removeListener(this);
        configTree.removeListener(this);
        if (ioTree.isValid())
            ioTree.removeListener(this);
    }

    /** ColorScheme::Manager::Listener callback - refresh colors when theme changes */
    void colorSchemeChanged() override
    {
        // Update TextEditor colors - JUCE TextEditors cache colors internally
        const auto& colors = ColorScheme::get();
        auto updateTextEditor = [&colors](juce::TextEditor& editor) {
            editor.setColour(juce::TextEditor::textColourId, colors.textPrimary);
            editor.setColour(juce::TextEditor::backgroundColourId, colors.surfaceCard);
            editor.setColour(juce::TextEditor::outlineColourId, colors.buttonBorder);
            editor.applyFontToAllText(editor.getFont(), true);
        };

        updateTextEditor(nameEditor);
        updateTextEditor(posXEditor);
        updateTextEditor(posYEditor);
        updateTextEditor(posZEditor);
        updateTextEditor(hParallaxEditor);
        updateTextEditor(vParallaxEditor);

        repaint();
    }

    /** Get the currently selected channel (1-based) */
    int getCurrentChannel() const { return currentChannel; }

    /** Select a specific channel (1-based). Triggers UI update.
     *  Uses programmatic selection to prevent keyboard Enter from triggering overlay.
     */
    void selectChannel(int channel)
    {
        channelSelector.setSelectedChannelProgrammatically(channel);
    }

    /** Get the total number of output channels */
    int getNumChannels() const { return parameters.getNumOutputChannels(); }

    /** Refresh UI from ValueTree - call after config reload */
    void refreshFromValueTree()
    {
        // Re-acquire ioTree reference in case config was replaced (e.g., copyPropertiesAndChildrenFrom)
        auto newIOTree = parameters.getConfigTree().getChildWithName(WFSParameterIDs::IO);
        if (newIOTree != ioTree)
        {
            if (ioTree.isValid())
                ioTree.removeListener(this);
            ioTree = newIOTree;
            if (ioTree.isValid())
                ioTree.addListener(this);
        }

        // Update channel selector count
        int numOutputs = parameters.getNumOutputChannels();
        if (numOutputs > 0)
        {
            channelSelector.setNumChannels(numOutputs);
            if (currentChannel > numOutputs)
                currentChannel = 1;
        }

        loadChannelParameters(currentChannel);
    }

    /** Callback when output config is reloaded - for triggering DSP recalculation */
    std::function<void()> onConfigReloaded;

    /** Cycle to next/previous channel. delta=1 for next, delta=-1 for previous. Wraps around. */
    void cycleChannel(int delta)
    {
        int numChannels = parameters.getNumOutputChannels();
        if (numChannels <= 0) return;

        int newChannel = currentChannel + delta;
        if (newChannel > numChannels) newChannel = 1;
        else if (newChannel < 1) newChannel = numChannels;

        selectChannel(newChannel);
    }

    /** Set array assignment for current output. 0=Single, 1-10=Array 1-10. */
    void setArray(int array)
    {
        array = juce::jlimit(0, 10, array);
        arraySelector.setSelectedId(array + 1, juce::sendNotification);
        if (statusBar != nullptr)
        {
            if (array == 0)
                statusBar->showTemporaryMessage("Output " + juce::String(currentChannel) + " set to Single", 2000);
            else
                statusBar->showTemporaryMessage("Output " + juce::String(currentChannel) + " assigned to Array " + juce::String(array), 2000);
        }
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(ColorScheme::get().background);

        // Header background
        g.setColour(ColorScheme::get().chromeSurface);
        g.fillRect(0, 0, getWidth(), headerHeight);

        // Footer background
        g.setColour(ColorScheme::get().chromeSurface);
        g.fillRect(0, getHeight() - footerHeight, getWidth(), footerHeight);

        // Section dividers
        g.setColour(ColorScheme::get().chromeDivider);
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
        row1.removeFromLeft(spacing * 2);
        mapVisibilityButton.setBounds(row1.removeFromLeft(180));

        // Wizard of OutZ button on the right
        arrayPositionHelperButton.setBounds(row1.removeFromRight(130));

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

        attenuationSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFFFF5722));
        attenuationSlider.onValueChanged = [this](float v) {
            // Convert 0-1 to -92 to 0 dB with logarithmic scaling
            float dB = 20.0f * std::log10(std::pow(10.0f, -92.0f / 20.0f) +
                       ((1.0f - std::pow(10.0f, -92.0f / 20.0f)) * v * v));
            attenuationValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
            saveOutputParam(WFSParameterIDs::outputAttenuation, dB);  // Save real dB value
        };
        addAndMakeVisible(attenuationSlider);

        addAndMakeVisible(attenuationValueLabel);
        attenuationValueLabel.setText("0.0 dB", juce::dontSendNotification);
        setupEditableValueLabel(attenuationValueLabel);

        // Delay/Latency slider (-100 to 100 ms)
        addAndMakeVisible(delayLatencyLabel);
        delayLatencyLabel.setText("Delay/Latency:", juce::dontSendNotification);

        delayLatencySlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFF4CAF50));
        delayLatencySlider.onValueChanged = [this](float v) {
            // Slider range is -1 to 1, map to -100ms to 100ms
            float ms = v * 100.0f;
            juce::String label = (ms < 0) ? "Latency: " : "Delay: ";
            delayLatencyValueLabel.setText(label + juce::String(std::abs(ms), 1) + " ms", juce::dontSendNotification);
            saveOutputParam(WFSParameterIDs::outputDelayLatency, ms);  // Save real ms value
        };
        addAndMakeVisible(delayLatencySlider);

        addAndMakeVisible(delayLatencyValueLabel);
        delayLatencyValueLabel.setText("Delay: 0.0 ms", juce::dontSendNotification);
        setupEditableValueLabel(delayLatencyValueLabel);

        // Min Latency Enable button
        addAndMakeVisible(minLatencyEnableButton);
        minLatencyEnableButton.setButtonText("Minimal Latency: ON");
        minLatencyEnableButton.setClickingTogglesState(true);
        minLatencyEnableButton.setToggleState(true, juce::dontSendNotification);
        minLatencyEnableButton.onClick = [this]() {
            bool enabled = minLatencyEnableButton.getToggleState();
            minLatencyEnableButton.setButtonText(enabled ? "Minimal Latency: ON" : "Minimal Latency: OFF");
            saveOutputParam(WFSParameterIDs::outputMiniLatencyEnable, enabled ? 1 : 0);
        };

        // Live Source Enable button
        addAndMakeVisible(liveSourceEnableButton);
        liveSourceEnableButton.setButtonText("Live Source Atten: ON");
        liveSourceEnableButton.setClickingTogglesState(true);
        liveSourceEnableButton.setToggleState(true, juce::dontSendNotification);
        liveSourceEnableButton.onClick = [this]() {
            bool enabled = liveSourceEnableButton.getToggleState();
            liveSourceEnableButton.setButtonText(enabled ? "Live Source Atten: ON" : "Live Source Atten: OFF");
            saveOutputParam(WFSParameterIDs::outputLSattenEnable, enabled ? 1 : 0);
        };

        // Floor Reflections Enable button
        addAndMakeVisible(floorReflectionsEnableButton);
        floorReflectionsEnableButton.setButtonText("Floor Reflections: ON");
        floorReflectionsEnableButton.setClickingTogglesState(true);
        floorReflectionsEnableButton.setToggleState(true, juce::dontSendNotification);
        floorReflectionsEnableButton.onClick = [this]() {
            bool enabled = floorReflectionsEnableButton.getToggleState();
            floorReflectionsEnableButton.setButtonText(enabled ? "Floor Reflections: ON" : "Floor Reflections: OFF");
            // Array propagation is now handled automatically by setOutputParam
            saveOutputParam(WFSParameterIDs::outputFRenable, enabled ? 1 : 0);
        };

        // Distance Attenuation % slider (0-200%, default 100% in center)
        addAndMakeVisible(distanceAttenLabel);
        distanceAttenLabel.setText("Distance Atten:", juce::dontSendNotification);

        distanceAttenSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFF9C27B0));
        // Bidirectional slider: center (0) = 100%, no need to set initial value
        distanceAttenSlider.onValueChanged = [this](float v) {
            // Slider range is -1 to 1, map to 0% to 200% (center = 100%)
            int percent = static_cast<int>((v + 1.0f) * 100.0f);
            distanceAttenValueLabel.setText(juce::String(percent) + " %", juce::dontSendNotification);
            saveOutputParam(WFSParameterIDs::outputDistanceAttenPercent, percent);
        };
        addAndMakeVisible(distanceAttenSlider);

        addAndMakeVisible(distanceAttenValueLabel);
        distanceAttenValueLabel.setText("100 %", juce::dontSendNotification);
        setupEditableValueLabel(distanceAttenValueLabel);

        // Horizontal Parallax
        addAndMakeVisible(hParallaxLabel);
        hParallaxLabel.setText("Horizontal Parallax:", juce::dontSendNotification);
        addAndMakeVisible(hParallaxEditor);
        hParallaxEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(hParallaxEditor, false, true);
        addAndMakeVisible(hParallaxUnitLabel);
        hParallaxUnitLabel.setText("m", juce::dontSendNotification);

        // Vertical Parallax
        addAndMakeVisible(vParallaxLabel);
        vParallaxLabel.setText("Vertical Parallax:", juce::dontSendNotification);
        addAndMakeVisible(vParallaxEditor);
        vParallaxEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(vParallaxEditor, true, true);
        addAndMakeVisible(vParallaxUnitLabel);
        vParallaxUnitLabel.setText("m", juce::dontSendNotification);

        // Initialize array link indicators (all hidden by default)
        addAndMakeVisible(attenuationIndicator);
        addAndMakeVisible(delayLatencyIndicator);
        addAndMakeVisible(minLatencyIndicator);
        addAndMakeVisible(liveSourceIndicator);
        addAndMakeVisible(floorReflectionsIndicator);
        addAndMakeVisible(distanceAttenIndicator);
        addAndMakeVisible(hParallaxIndicator);
        addAndMakeVisible(vParallaxIndicator);
        addAndMakeVisible(orientationIndicator);
        addAndMakeVisible(angleOnIndicator);
        addAndMakeVisible(angleOffIndicator);
        addAndMakeVisible(pitchIndicator);
        addAndMakeVisible(hfDampingIndicator);
    }

    void setupPositionTab()
    {
        // Coordinate Mode selector
        addAndMakeVisible(coordModeLabel);
        coordModeLabel.setText("Coordinates:", juce::dontSendNotification);
        addAndMakeVisible(coordModeSelector);
        coordModeSelector.addItem("XYZ", 1);
        coordModeSelector.addItem(juce::String(juce::CharPointer_UTF8("r \xce\xb8 Z")), 2);    // r θ Z
        coordModeSelector.addItem(juce::String(juce::CharPointer_UTF8("r \xce\xb8 \xcf\x86")), 3);  // r θ φ
        coordModeSelector.setSelectedId(1, juce::dontSendNotification);
        coordModeSelector.onChange = [this]() {
            int mode = coordModeSelector.getSelectedId() - 1;
            saveOutputParam(WFSParameterIDs::outputCoordinateMode, mode);
            updatePositionLabelsAndValues();
        };

        // Position X
        addAndMakeVisible(posXLabel);
        posXLabel.setText("Position X:", juce::dontSendNotification);
        addAndMakeVisible(posXEditor);
        posXEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(posXEditor, true, true);
        addAndMakeVisible(posXUnitLabel);
        posXUnitLabel.setText("m", juce::dontSendNotification);

        // Position Y
        addAndMakeVisible(posYLabel);
        posYLabel.setText("Position Y:", juce::dontSendNotification);
        addAndMakeVisible(posYEditor);
        posYEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(posYEditor, true, true);
        addAndMakeVisible(posYUnitLabel);
        posYUnitLabel.setText("m", juce::dontSendNotification);

        // Position Z
        addAndMakeVisible(posZLabel);
        posZLabel.setText("Position Z:", juce::dontSendNotification);
        addAndMakeVisible(posZEditor);
        posZEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(posZEditor, true, true);
        addAndMakeVisible(posZUnitLabel);
        posZUnitLabel.setText("m", juce::dontSendNotification);

        // Orientation dial
        addAndMakeVisible(orientationLabel);
        orientationLabel.setText("Orientation:", juce::dontSendNotification);
        orientationDial.setColours(juce::Colours::black, juce::Colours::white, juce::Colours::grey);
        orientationDial.onAngleChanged = [this](float angle) {
            orientationValueLabel.setText(juce::String(static_cast<int>(angle)), juce::dontSendNotification);
            saveOutputParam(WFSParameterIDs::outputOrientation, angle);
        };
        addAndMakeVisible(orientationDial);
        addAndMakeVisible(orientationValueLabel);
        orientationValueLabel.setText("0", juce::dontSendNotification);
        orientationValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(orientationValueLabel);
        addAndMakeVisible(orientationUnitLabel);
        orientationUnitLabel.setText(juce::String::fromUTF8("°"), juce::dontSendNotification);
        orientationUnitLabel.setJustificationType(juce::Justification::left);
        orientationUnitLabel.setMinimumHorizontalScale(1.0f);

        // Angle On slider (1-180°)
        addAndMakeVisible(angleOnLabel);
        angleOnLabel.setText("Angle On:", juce::dontSendNotification);

        angleOnSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFF00BCD4));
        angleOnSlider.setValue(0.47f);  // ~86°
        angleOnSlider.onValueChanged = [this](float v) {
            int degrees = static_cast<int>(v * 179.0f + 1.0f);
            angleOnValueLabel.setText(juce::String(degrees) + juce::String::fromUTF8("°"), juce::dontSendNotification);
            saveOutputParam(WFSParameterIDs::outputAngleOn, degrees);
        };
        addAndMakeVisible(angleOnSlider);
        addAndMakeVisible(angleOnValueLabel);
        angleOnValueLabel.setText(juce::String::fromUTF8("86°"), juce::dontSendNotification);
        setupEditableValueLabel(angleOnValueLabel);

        // Angle Off slider (0-179°)
        addAndMakeVisible(angleOffLabel);
        angleOffLabel.setText("Angle Off:", juce::dontSendNotification);

        angleOffSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFF00BCD4));
        angleOffSlider.setValue(0.5f);  // ~90°
        angleOffSlider.onValueChanged = [this](float v) {
            int degrees = static_cast<int>(v * 179.0f);
            angleOffValueLabel.setText(juce::String(degrees) + juce::String::fromUTF8("°"), juce::dontSendNotification);
            saveOutputParam(WFSParameterIDs::outputAngleOff, degrees);
        };
        addAndMakeVisible(angleOffSlider);
        addAndMakeVisible(angleOffValueLabel);
        angleOffValueLabel.setText(juce::String::fromUTF8("90°"), juce::dontSendNotification);
        setupEditableValueLabel(angleOffValueLabel);

        // Pitch slider (-90 to 90°)
        addAndMakeVisible(pitchLabel);
        pitchLabel.setText("Pitch:", juce::dontSendNotification);

        pitchSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFF2196F3));
        pitchSlider.onValueChanged = [this](float v) {
            // Slider range is -1 to 1, map to -90° to 90°
            int degrees = static_cast<int>(v * 90.0f);
            pitchValueLabel.setText(juce::String(degrees) + juce::String::fromUTF8("°"), juce::dontSendNotification);
            saveOutputParam(WFSParameterIDs::outputPitch, degrees);
        };
        addAndMakeVisible(pitchSlider);
        addAndMakeVisible(pitchValueLabel);
        pitchValueLabel.setText(juce::String::fromUTF8("0°"), juce::dontSendNotification);
        setupEditableValueLabel(pitchValueLabel);

        // HF Damping slider (-6 to 0 dB/m)
        addAndMakeVisible(hfDampingLabel);
        hfDampingLabel.setText("HF Damping:", juce::dontSendNotification);

        hfDampingSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFFFF9800));
        hfDampingSlider.setValue(1.0f);  // 0 dB/m
        hfDampingSlider.onValueChanged = [this](float v) {
            float dBm = v * 6.0f - 6.0f;
            hfDampingValueLabel.setText(juce::String(dBm, 1) + " dB/m", juce::dontSendNotification);
            saveOutputParam(WFSParameterIDs::outputHFdamping, dBm);
        };
        addAndMakeVisible(hfDampingSlider);
        addAndMakeVisible(hfDampingValueLabel);
        hfDampingValueLabel.setText("0.0 dB/m", juce::dontSendNotification);
        setupEditableValueLabel(hfDampingValueLabel);
    }

    void setupEqTab()
    {
        // Global EQ Enable button
        addAndMakeVisible(eqEnableButton);
        eqEnableButton.setButtonText("EQ ON");
        eqEnableButton.setClickingTogglesState(true);
        eqEnableButton.setToggleState(true, juce::dontSendNotification);
        eqEnableButton.onClick = [this]() {
            bool enabled = eqEnableButton.getToggleState();
            eqEnableButton.setButtonText(enabled ? "EQ ON" : "EQ OFF");
            // Update all band appearances when global EQ state changes
            for (int i = 0; i < numEqBands; ++i)
                updateEqBandAppearance(i);
            // Update EQ display grey-out state
            if (eqDisplay != nullptr)
                eqDisplay->setEQEnabled(enabled);
            saveOutputParam(WFSParameterIDs::outputEQenabled, enabled ? 1 : 0);
        };

        // 6 EQ Bands
        for (int i = 0; i < numEqBands; ++i)
        {
            // Band label - colored to match EQ display markers
            addAndMakeVisible(eqBandLabel[i]);
            eqBandLabel[i].setText("Band " + juce::String(i + 1), juce::dontSendNotification);
            eqBandLabel[i].setColour(juce::Label::textColourId, EQDisplayComponent::getBandColour(i));
            eqBandLabel[i].setJustificationType(juce::Justification::centred);

            // Shape dropdown
            addAndMakeVisible(eqBandShapeSelector[i]);
            eqBandShapeSelector[i].addItem("OFF", 1);
            eqBandShapeSelector[i].addItem("Low Cut", 2);
            eqBandShapeSelector[i].addItem("Low Shelf", 3);
            eqBandShapeSelector[i].addItem("Peak/Notch", 4);
            eqBandShapeSelector[i].addItem("Band Pass", 5);
            eqBandShapeSelector[i].addItem("High Shelf", 6);
            eqBandShapeSelector[i].addItem("High Cut", 7);
            eqBandShapeSelector[i].setSelectedId(1, juce::dontSendNotification);  // OFF by default

            // Shape change handler - update appearance and save
            eqBandShapeSelector[i].onChange = [this, i]() {
                int shape = eqBandShapeSelector[i].getSelectedId() - 1;
                saveEqBandParam(i, WFSParameterIDs::eqShape, shape);
                updateEqBandAppearance(i);
            };

            // Frequency slider - colored to match band
            addAndMakeVisible(eqBandFreqLabel[i]);
            eqBandFreqLabel[i].setText("Freq:", juce::dontSendNotification);
            eqBandFreqLabel[i].setColour(juce::Label::textColourId, juce::Colours::grey);

            juce::Colour bandColour = EQDisplayComponent::getBandColour(i);
            eqBandFreqSlider[i].setTrackColours(juce::Colour(0xFF2D2D2D), bandColour);
            eqBandFreqSlider[i].onValueChanged = [this, i](float v) {
                int freq = static_cast<int>(20.0f * std::pow(10.0f, 3.0f * v));
                eqBandFreqValueLabel[i].setText(formatFrequency(freq), juce::dontSendNotification);
                saveEqBandParam(i, WFSParameterIDs::eqFrequency, freq);
            };
            addAndMakeVisible(eqBandFreqSlider[i]);

            addAndMakeVisible(eqBandFreqValueLabel[i]);
            eqBandFreqValueLabel[i].setText("1000 Hz", juce::dontSendNotification);

            // Gain dial - colored to match band
            addAndMakeVisible(eqBandGainLabel[i]);
            eqBandGainLabel[i].setText("Gain", juce::dontSendNotification);
            eqBandGainLabel[i].setColour(juce::Label::textColourId, juce::Colours::grey);
            eqBandGainLabel[i].setJustificationType(juce::Justification::centred);

            eqBandGainDial[i].setTrackColours(juce::Colour(0xFF2D2D2D), bandColour);
            eqBandGainDial[i].onValueChanged = [this, i](float v) {
                float gain = v * 48.0f - 24.0f;  // -24 to +24 dB
                eqBandGainValueLabel[i].setText(juce::String(gain, 1) + " dB", juce::dontSendNotification);
                saveEqBandParam(i, WFSParameterIDs::eqGain, gain);
            };
            addAndMakeVisible(eqBandGainDial[i]);

            addAndMakeVisible(eqBandGainValueLabel[i]);
            eqBandGainValueLabel[i].setText("0.0 dB", juce::dontSendNotification);
            eqBandGainValueLabel[i].setJustificationType(juce::Justification::centred);

            // Q dial - colored to match band
            addAndMakeVisible(eqBandQLabel[i]);
            eqBandQLabel[i].setText("Q", juce::dontSendNotification);
            eqBandQLabel[i].setColour(juce::Label::textColourId, juce::Colours::grey);
            eqBandQLabel[i].setJustificationType(juce::Justification::centred);

            eqBandQDial[i].setTrackColours(juce::Colour(0xFF2D2D2D), bandColour);
            eqBandQDial[i].onValueChanged = [this, i](float v) {
                float q = 0.1f + 0.099f * (std::pow(100.0f, v) - 1.0f);  // 0.1-10.0
                eqBandQValueLabel[i].setText(juce::String(q, 2), juce::dontSendNotification);
                saveEqBandParam(i, WFSParameterIDs::eqQ, q);
            };
            addAndMakeVisible(eqBandQDial[i]);

            addAndMakeVisible(eqBandQValueLabel[i]);
            eqBandQValueLabel[i].setText("0.70", juce::dontSendNotification);
            eqBandQValueLabel[i].setJustificationType(juce::Justification::centred);

            // Initialize appearance (greyed out since default is OFF)
            updateEqBandAppearance(i);
        }

        // EQ array link indicator
        addAndMakeVisible(eqIndicator);
    }

    void updateEqBandAppearance(int bandIndex)
    {
        bool eqEnabled = eqEnableButton.getToggleState();
        int shapeId = eqBandShapeSelector[bandIndex].getSelectedId();
        bool bandIsOff = (shapeId == 1);  // OFF

        // Determine if this is a cut or bandpass filter (no gain control)
        // Output EQ shapes: 1=OFF, 2=LowCut, 3=LowShelf, 4=Peak, 5=BandPass, 6=HighShelf, 7=HighCut
        bool isCutOrBandPass = (shapeId == 2 || shapeId == 5 || shapeId == 7);
        bool showGain = !isCutOrBandPass;

        // Grey out entire band if global EQ is off
        // Grey out band parameters (except shape) if band is off but EQ is on
        float bandLabelAlpha = eqEnabled ? 1.0f : 0.4f;
        float shapeAlpha = eqEnabled ? 1.0f : 0.4f;
        float paramAlpha = (eqEnabled && !bandIsOff) ? 1.0f : 0.4f;

        // Band label and shape dropdown follow global EQ state
        eqBandLabel[bandIndex].setAlpha(bandLabelAlpha);
        eqBandShapeSelector[bandIndex].setAlpha(shapeAlpha);

        // Only update visibility if EQ tab is currently selected
        bool eqTabSelected = (subTabBar.getCurrentTabIndex() == 1);

        // Parameters follow both global EQ and band off state
        if (eqTabSelected)
        {
            eqBandFreqLabel[bandIndex].setVisible(true);
            eqBandFreqSlider[bandIndex].setVisible(true);
            eqBandFreqValueLabel[bandIndex].setVisible(true);
        }
        eqBandFreqLabel[bandIndex].setAlpha(paramAlpha);
        eqBandFreqSlider[bandIndex].setAlpha(paramAlpha);
        eqBandFreqValueLabel[bandIndex].setAlpha(paramAlpha);

        if (eqTabSelected)
        {
            eqBandQLabel[bandIndex].setVisible(true);
            eqBandQDial[bandIndex].setVisible(true);
            eqBandQValueLabel[bandIndex].setVisible(true);
        }
        eqBandQLabel[bandIndex].setAlpha(paramAlpha);
        eqBandQDial[bandIndex].setAlpha(paramAlpha);
        eqBandQValueLabel[bandIndex].setAlpha(paramAlpha);

        // Gain controls - hide for cut/bandpass filters, only show if EQ tab selected
        bool showGainVisible = showGain && eqTabSelected;
        eqBandGainLabel[bandIndex].setVisible(showGainVisible);
        eqBandGainDial[bandIndex].setVisible(showGainVisible);
        eqBandGainValueLabel[bandIndex].setVisible(showGainVisible);
        if (showGain)
        {
            eqBandGainLabel[bandIndex].setAlpha(paramAlpha);
            eqBandGainDial[bandIndex].setAlpha(paramAlpha);
            eqBandGainValueLabel[bandIndex].setAlpha(paramAlpha);
        }
    }

    void setupNumericEditor(juce::TextEditor& editor, bool /*allowNegative*/, bool /*allowDecimal*/)
    {
        // No input restrictions - allow free typing, validate on commit (Enter/focus lost)
        editor.addListener(this);
    }

    void setupEditableValueLabel(juce::Label& label)
    {
        label.setEditable(true, false);  // Single click to edit
        label.setJustificationType(juce::Justification::right);
        label.addListener(this);
    }

    /** Update all array link indicators based on current array and applyToArray settings */
    void updateArrayLinkIndicators()
    {
        // Get array number (0=Single, 1-10=Array 1-10)
        int arrayNum = arraySelector.getSelectedId() - 1;
        // Get apply mode (0=OFF, 1=ABSOLUTE, 2=RELATIVE)
        int applyMode = applyToArraySelector.getSelectedId() - 1;

        // Active when in an array (arrayNum > 0) and apply mode is not OFF (applyMode > 0)
        bool active = (arrayNum > 0) && (applyMode > 0);
        // Filled disk for ABSOLUTE (1), outline circle for RELATIVE (2)
        bool filled = (applyMode == 1);

        // Helper to update an indicator
        auto updateIndicator = [arrayNum, active, filled](ArrayLinkIndicator& ind) {
            ind.setArrayNumber(arrayNum);
            ind.setActive(active);
            ind.setFilled(filled);
        };

        // Update all indicators
        updateIndicator(attenuationIndicator);
        updateIndicator(delayLatencyIndicator);
        updateIndicator(minLatencyIndicator);
        updateIndicator(liveSourceIndicator);
        updateIndicator(floorReflectionsIndicator);
        updateIndicator(distanceAttenIndicator);
        updateIndicator(hParallaxIndicator);
        updateIndicator(vParallaxIndicator);
        updateIndicator(orientationIndicator);
        updateIndicator(angleOnIndicator);
        updateIndicator(angleOffIndicator);
        updateIndicator(pitchIndicator);
        updateIndicator(hfDampingIndicator);
        updateIndicator(eqIndicator);

        // Update visibility based on current tab
        bool onOutputParamsTab = (subTabBar.getCurrentTabIndex() == 0);
        bool onEqTab = (subTabBar.getCurrentTabIndex() == 1);

        attenuationIndicator.setVisible(onOutputParamsTab && active);
        delayLatencyIndicator.setVisible(onOutputParamsTab && active);
        minLatencyIndicator.setVisible(onOutputParamsTab && active);
        liveSourceIndicator.setVisible(onOutputParamsTab && active);
        floorReflectionsIndicator.setVisible(onOutputParamsTab && active);
        distanceAttenIndicator.setVisible(onOutputParamsTab && active);
        hParallaxIndicator.setVisible(onOutputParamsTab && active);
        vParallaxIndicator.setVisible(onOutputParamsTab && active);
        orientationIndicator.setVisible(onOutputParamsTab && active);
        angleOnIndicator.setVisible(onOutputParamsTab && active);
        angleOffIndicator.setVisible(onOutputParamsTab && active);
        pitchIndicator.setVisible(onOutputParamsTab && active);
        hfDampingIndicator.setVisible(onOutputParamsTab && active);
        eqIndicator.setVisible(onEqTab && active);
    }

    // ==================== LAYOUT METHODS ====================

    void layoutCurrentSubTab()
    {
        int tabIndex = subTabBar.getCurrentTabIndex();

        // Hide all components first
        setOutputParametersVisible(false);
        setEqVisible(false);

        // Show and layout current tab
        if (tabIndex == 0)
        {
            setOutputParametersVisible(true);
            layoutOutputParametersTab();
        }
        else if (tabIndex == 1)
        {
            setEqVisible(true);
            layoutEqTab();
        }
    }

    void setOutputParametersVisible(bool visible)
    {
        // Level & Timing components (left column)
        attenuationLabel.setVisible(visible);
        attenuationSlider.setVisible(visible);
        attenuationValueLabel.setVisible(visible);
        delayLatencyLabel.setVisible(visible);
        delayLatencySlider.setVisible(visible);
        delayLatencyValueLabel.setVisible(visible);
        minLatencyEnableButton.setVisible(visible);
        liveSourceEnableButton.setVisible(visible);
        floorReflectionsEnableButton.setVisible(visible);
        distanceAttenLabel.setVisible(visible);
        distanceAttenSlider.setVisible(visible);
        distanceAttenValueLabel.setVisible(visible);
        hParallaxLabel.setVisible(visible);
        hParallaxEditor.setVisible(visible);
        hParallaxUnitLabel.setVisible(visible);
        vParallaxLabel.setVisible(visible);
        vParallaxEditor.setVisible(visible);
        vParallaxUnitLabel.setVisible(visible);

        // Position & Directivity components (right column)
        coordModeLabel.setVisible(visible);
        coordModeSelector.setVisible(visible);
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
        orientationUnitLabel.setVisible(visible);
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

        // Array link indicators - only show if visible AND active
        bool showIndicators = visible && attenuationIndicator.isActive();
        attenuationIndicator.setVisible(showIndicators);
        delayLatencyIndicator.setVisible(showIndicators);
        minLatencyIndicator.setVisible(showIndicators);
        liveSourceIndicator.setVisible(showIndicators);
        floorReflectionsIndicator.setVisible(showIndicators);
        distanceAttenIndicator.setVisible(showIndicators);
        hParallaxIndicator.setVisible(showIndicators);
        vParallaxIndicator.setVisible(showIndicators);
        orientationIndicator.setVisible(showIndicators);
        angleOnIndicator.setVisible(showIndicators);
        angleOffIndicator.setVisible(showIndicators);
        pitchIndicator.setVisible(showIndicators);
        hfDampingIndicator.setVisible(showIndicators);
    }

    void setEqVisible(bool visible)
    {
        // Global EQ Enable button
        eqEnableButton.setVisible(visible);

        // EQ Display
        if (eqDisplay)
            eqDisplay->setVisible(visible);

        // 6 EQ Bands
        for (int i = 0; i < numEqBands; ++i)
        {
            eqBandLabel[i].setVisible(visible);
            eqBandShapeSelector[i].setVisible(visible);
            eqBandFreqLabel[i].setVisible(visible);
            eqBandFreqSlider[i].setVisible(visible);
            eqBandFreqValueLabel[i].setVisible(visible);
            eqBandQLabel[i].setVisible(visible);
            eqBandQDial[i].setVisible(visible);
            eqBandQValueLabel[i].setVisible(visible);

            // Show/hide gain based on filter shape (hide for cut/bandpass filters)
            if (visible)
                updateEqBandAppearance(i);
            else
            {
                eqBandGainLabel[i].setVisible(false);
                eqBandGainDial[i].setVisible(false);
                eqBandGainValueLabel[i].setVisible(false);
            }
        }

        // EQ array link indicator - only show if visible AND active
        eqIndicator.setVisible(visible && eqIndicator.isActive());
    }

    void layoutOutputParametersTab()
    {
        auto area = subTabContentArea;
        const int rowHeight = 30;
        const int sliderHeight = 40;
        const int spacing = 8;
        const int labelWidth = 115;
        const int valueWidth = 60;  // Tight value width like LFO section
        const int indicatorSize = 6;

        // Helper to position indicator as superscript after label text (like a footnote marker)
        auto positionIndicatorForLabel = [indicatorSize](ArrayLinkIndicator& indicator, const juce::Label& label) {
            auto labelBounds = label.getBounds();
            // Get the actual text width for consistent positioning using GlyphArrangement
            juce::GlyphArrangement glyphs;
            glyphs.addLineOfText(label.getFont(), label.getText(), 0.0f, 0.0f);
            int textWidth = static_cast<int>(std::ceil(glyphs.getBoundingBox(0, -1, true).getWidth()));
            int labelX = labelBounds.getX();
            // Position as superscript: after text, at top of label (like Orientation)
            indicator.setBounds(labelX + textWidth + 1,
                               labelBounds.getY(),
                               indicatorSize, indicatorSize);
        };

        // Helper to position indicator in top-right corner of button (inside the curve)
        auto positionIndicatorForButton = [indicatorSize](ArrayLinkIndicator& indicator, const juce::Button& button) {
            auto buttonBounds = button.getBounds();
            // Position in top-right corner, inset to match button curve
            indicator.setBounds(buttonBounds.getRight() - indicatorSize - 6,
                               buttonBounds.getY() + 4,
                               indicatorSize, indicatorSize);
        };

        // ==================== LEFT COLUMN (Level & Timing) ====================
        auto leftCol = area.removeFromLeft(area.getWidth() / 2).reduced(10, 10);

        // Attenuation
        auto row = leftCol.removeFromTop(rowHeight);
        attenuationLabel.setBounds(row.removeFromLeft(labelWidth));
        positionIndicatorForLabel(attenuationIndicator, attenuationLabel);
        attenuationValueLabel.setBounds(row.removeFromRight(valueWidth));
        attenuationSlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing);

        // Delay/Latency
        row = leftCol.removeFromTop(rowHeight);
        delayLatencyLabel.setBounds(row.removeFromLeft(labelWidth));
        positionIndicatorForLabel(delayLatencyIndicator, delayLatencyLabel);
        delayLatencyValueLabel.setBounds(row.removeFromRight(130));  // Wider for "Latency: 100.0 ms"
        delayLatencySlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing);

        // Distance Attenuation
        row = leftCol.removeFromTop(rowHeight);
        distanceAttenLabel.setBounds(row.removeFromLeft(labelWidth));
        positionIndicatorForLabel(distanceAttenIndicator, distanceAttenLabel);
        distanceAttenValueLabel.setBounds(row.removeFromRight(valueWidth));
        distanceAttenSlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing * 2);  // Extra space before buttons

        // Enable buttons - all three on a single row with equal width and doubled spacing
        row = leftCol.removeFromTop(rowHeight);
        const int buttonSpacing = 30;  // Doubled spacing between buttons
        const int totalButtonSpace = row.getWidth() - (buttonSpacing * 2);
        const int buttonWidth = totalButtonSpace / 3;
        minLatencyEnableButton.setBounds(row.removeFromLeft(buttonWidth));
        positionIndicatorForButton(minLatencyIndicator, minLatencyEnableButton);
        row.removeFromLeft(buttonSpacing);
        liveSourceEnableButton.setBounds(row.removeFromLeft(buttonWidth));
        positionIndicatorForButton(liveSourceIndicator, liveSourceEnableButton);
        row.removeFromLeft(buttonSpacing);
        floorReflectionsEnableButton.setBounds(row.removeFromLeft(buttonWidth));
        positionIndicatorForButton(floorReflectionsIndicator, floorReflectionsEnableButton);

        // ==================== RIGHT COLUMN (Position & Directivity) ====================
        auto rightCol = area.reduced(10, 10);

        // Coordinate mode and position row - distribute evenly across full width
        row = rightCol.removeFromTop(rowHeight);
        const int coordLabelWidth = 85;
        const int coordSelectorWidth = 80;
        const int posLabelWidth = 75;  // Fits "Position X:", "Azimuth:", "Elevation:"
        const int posEditorWidth = 65;
        const int posUnitWidth = 25;
        const int coordSpacing = 15;  // Spacing between coordinate groups

        coordModeLabel.setBounds(row.removeFromLeft(coordLabelWidth));
        coordModeSelector.setBounds(row.removeFromLeft(coordSelectorWidth));
        row.removeFromLeft(coordSpacing);
        posXLabel.setBounds(row.removeFromLeft(posLabelWidth));
        posXEditor.setBounds(row.removeFromLeft(posEditorWidth));
        row.removeFromLeft(4);
        posXUnitLabel.setBounds(row.removeFromLeft(posUnitWidth));
        row.removeFromLeft(coordSpacing);
        posYLabel.setBounds(row.removeFromLeft(posLabelWidth));
        posYEditor.setBounds(row.removeFromLeft(posEditorWidth));
        row.removeFromLeft(4);
        posYUnitLabel.setBounds(row.removeFromLeft(posUnitWidth));
        row.removeFromLeft(coordSpacing);
        posZLabel.setBounds(row.removeFromLeft(posLabelWidth));
        posZEditor.setBounds(row.removeFromLeft(posEditorWidth));
        row.removeFromLeft(4);
        posZUnitLabel.setBounds(row.removeFromLeft(posUnitWidth));
        rightCol.removeFromTop(spacing * 3);  // 3x spacing before directivity group

        // Calculate heights for vertical centering of dial with slider group
        const int dialSize = 100;
        const int dialMargin = 40;
        const int sliderGroupHeight = 4 * (rowHeight + sliderHeight) + 3 * spacing;  // 4 sliders with spacing
        const int dialGroupHeight = rowHeight + dialSize + rowHeight;  // label + dial + value
        const int dialTopOffset = (sliderGroupHeight - dialGroupHeight) / 2;

        // Orientation dial on the right side, vertically centered with slider group
        auto dialColumn = rightCol.removeFromRight(dialSize + dialMargin);
        dialColumn.removeFromTop(dialTopOffset);  // Center dial with slider group
        auto orientLabelArea = dialColumn.removeFromTop(rowHeight);
        orientationLabel.setBounds(orientLabelArea);
        orientationLabel.setJustificationType(juce::Justification::centred);
        // Position indicator as superscript relative to centered text
        {
            juce::GlyphArrangement glyphs;
            glyphs.addLineOfText(orientationLabel.getFont(), orientationLabel.getText(), 0.0f, 0.0f);
            int textWidth = static_cast<int>(std::ceil(glyphs.getBoundingBox(0, -1, true).getWidth()));
            int centerX = orientLabelArea.getCentreX();
            orientationIndicator.setBounds(centerX + textWidth / 2 + 1,
                                          orientLabelArea.getY(),
                                          indicatorSize, indicatorSize);
        }
        auto dialArea = dialColumn.removeFromTop(dialSize);
        int orientDialCenterX = dialArea.getCentreX();
        orientationDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
        auto orientValueRow = dialColumn.removeFromTop(rowHeight);
        // Value and unit adjacent, centered as a pair under dial (with overlap to reduce font padding gap)
        const int orientValW = 40, orientUnitW = 30, overlap = 7;
        int orientStartX = orientDialCenterX - (orientValW + orientUnitW - overlap) / 2;
        orientationValueLabel.setBounds(orientStartX, orientValueRow.getY(), orientValW, rowHeight);
        orientationValueLabel.setJustificationType(juce::Justification::right);
        orientationUnitLabel.setBounds(orientStartX + orientValW - overlap, orientValueRow.getY(), orientUnitW, rowHeight);
        orientationUnitLabel.setJustificationType(juce::Justification::left);

        // Angle On
        row = rightCol.removeFromTop(rowHeight);
        angleOnLabel.setBounds(row.removeFromLeft(labelWidth));
        positionIndicatorForLabel(angleOnIndicator, angleOnLabel);
        angleOnValueLabel.setBounds(row.removeFromRight(valueWidth));
        angleOnSlider.setBounds(rightCol.removeFromTop(sliderHeight));
        rightCol.removeFromTop(spacing);

        // Angle Off
        row = rightCol.removeFromTop(rowHeight);
        angleOffLabel.setBounds(row.removeFromLeft(labelWidth));
        positionIndicatorForLabel(angleOffIndicator, angleOffLabel);
        angleOffValueLabel.setBounds(row.removeFromRight(valueWidth));
        angleOffSlider.setBounds(rightCol.removeFromTop(sliderHeight));
        rightCol.removeFromTop(spacing);

        // Pitch
        row = rightCol.removeFromTop(rowHeight);
        pitchLabel.setBounds(row.removeFromLeft(labelWidth));
        positionIndicatorForLabel(pitchIndicator, pitchLabel);
        pitchValueLabel.setBounds(row.removeFromRight(valueWidth));
        pitchSlider.setBounds(rightCol.removeFromTop(sliderHeight));
        rightCol.removeFromTop(spacing);

        // HF Damping
        row = rightCol.removeFromTop(rowHeight);
        hfDampingLabel.setBounds(row.removeFromLeft(labelWidth));
        positionIndicatorForLabel(hfDampingIndicator, hfDampingLabel);
        hfDampingValueLabel.setBounds(row.removeFromRight(valueWidth));
        hfDampingSlider.setBounds(rightCol.removeFromTop(sliderHeight));
        rightCol.removeFromTop(spacing * 3);  // 3x spacing before parallax

        // Parallax editors (both on same row, V Parallax starts at center)
        row = rightCol.removeFromTop(rowHeight);
        const int parallaxEditorWidth = 60;
        const int parallaxUnitWidth = 20;
        const int labelToEditorGap = 10;  // Gap between label and editor

        // Horizontal Parallax - left half
        auto hArea = row.removeFromLeft(row.getWidth() / 2);
        hParallaxLabel.setBounds(hArea.removeFromLeft(130));
        positionIndicatorForLabel(hParallaxIndicator, hParallaxLabel);
        hArea.removeFromLeft(labelToEditorGap);
        hParallaxEditor.setBounds(hArea.removeFromLeft(parallaxEditorWidth));
        hArea.removeFromLeft(4);
        hParallaxUnitLabel.setBounds(hArea.removeFromLeft(parallaxUnitWidth));

        // Vertical Parallax - starts at center of column
        vParallaxLabel.setBounds(row.removeFromLeft(120));
        positionIndicatorForLabel(vParallaxIndicator, vParallaxLabel);
        row.removeFromLeft(labelToEditorGap);
        vParallaxEditor.setBounds(row.removeFromLeft(parallaxEditorWidth));
        row.removeFromLeft(4);
        vParallaxUnitLabel.setBounds(row.removeFromLeft(parallaxUnitWidth));
    }

    void layoutEqTab()
    {
        auto area = subTabContentArea;
        const int buttonHeight = 30;
        const int bandWidth = (area.getWidth() - 40) / numEqBands;
        const int dialSize = 60;
        const int sliderHeight = 35;
        const int labelHeight = 20;
        const int spacing = 5;
        const int indicatorSize = 6;

        // EQ Enable button at top with array link indicator in top-right corner
        eqEnableButton.setBounds(area.removeFromTop(buttonHeight).withWidth(100));
        auto eqBtnBounds = eqEnableButton.getBounds();
        eqIndicator.setBounds(eqBtnBounds.getRight() - indicatorSize - 6,
                             eqBtnBounds.getY() + 4,
                             indicatorSize, indicatorSize);
        area.removeFromTop(spacing * 2);

        // EQ Display component (takes upper portion, min 200px, target ~40% of remaining height)
        if (eqDisplay)
        {
            int displayHeight = juce::jmax(200, area.getHeight() * 2 / 5);
            eqDisplay->setBounds(area.removeFromTop(displayHeight));
            area.removeFromTop(spacing);
        }

        // Layout bands horizontally
        for (int i = 0; i < numEqBands; ++i)
        {
            auto bandArea = area.removeFromLeft(bandWidth).reduced(5, 0);

            eqBandLabel[i].setBounds(bandArea.removeFromTop(labelHeight));
            eqBandShapeSelector[i].setBounds(bandArea.removeFromTop(buttonHeight));
            bandArea.removeFromTop(spacing);

            // Frequency slider
            eqBandFreqLabel[i].setBounds(bandArea.removeFromTop(labelHeight));
            eqBandFreqSlider[i].setBounds(bandArea.removeFromTop(sliderHeight));
            eqBandFreqValueLabel[i].setBounds(bandArea.removeFromTop(labelHeight));
            bandArea.removeFromTop(spacing);

            // Gain and Q dials in a row
            auto dialRow = bandArea.removeFromTop(dialSize + labelHeight * 2);
            int dialSpacing = (dialRow.getWidth() - dialSize * 2) / 3;

            auto gainArea = dialRow.removeFromLeft(dialSize + dialSpacing).reduced(dialSpacing / 2, 0);
            eqBandGainLabel[i].setBounds(gainArea.removeFromTop(labelHeight));
            eqBandGainDial[i].setBounds(gainArea.removeFromTop(dialSize).withSizeKeepingCentre(dialSize, dialSize));
            eqBandGainValueLabel[i].setBounds(gainArea.removeFromTop(labelHeight));

            auto qArea = dialRow.removeFromLeft(dialSize + dialSpacing).reduced(dialSpacing / 2, 0);
            eqBandQLabel[i].setBounds(qArea.removeFromTop(labelHeight));
            eqBandQDial[i].setBounds(qArea.removeFromTop(dialSize).withSizeKeepingCentre(dialSize, dialSize));
            eqBandQValueLabel[i].setBounds(qArea.removeFromTop(labelHeight));
        }
    }

    // ==================== COORDINATE MODE HANDLING ====================

    void updatePositionLabelsAndValues()
    {
        // Get current coordinate mode
        int mode = static_cast<int>(parameters.getOutputParam(currentChannel - 1, "outputCoordinateMode"));
        auto coordMode = static_cast<WFSCoordinates::Mode>(mode);

        // Update selector to match (in case called from loadChannelParameters)
        coordModeSelector.setSelectedId(mode + 1, juce::dontSendNotification);

        // Get labels and units for this mode
        juce::String label1, label2, label3, unit1, unit2, unit3;
        WFSCoordinates::getCoordinateLabels(coordMode, label1, label2, label3, unit1, unit2, unit3);

        // Update labels and units
        posXLabel.setText(label1, juce::dontSendNotification);
        posYLabel.setText(label2, juce::dontSendNotification);
        posZLabel.setText(label3, juce::dontSendNotification);
        posXUnitLabel.setText(unit1, juce::dontSendNotification);
        posYUnitLabel.setText(unit2, juce::dontSendNotification);
        posZUnitLabel.setText(unit3, juce::dontSendNotification);

        // Get Cartesian values from storage
        float x = static_cast<float>(parameters.getOutputParam(currentChannel - 1, "outputPositionX"));
        float y = static_cast<float>(parameters.getOutputParam(currentChannel - 1, "outputPositionY"));
        float z = static_cast<float>(parameters.getOutputParam(currentChannel - 1, "outputPositionZ"));

        // Convert to display coordinates
        float v1, v2, v3;
        WFSCoordinates::cartesianToDisplay(coordMode, x, y, z, v1, v2, v3);

        // Update editors with appropriate precision
        // Distance in meters: 2 decimals, angles in degrees: 1 decimal
        if (coordMode == WFSCoordinates::Mode::Cartesian)
        {
            posXEditor.setText(juce::String(v1, 2), juce::dontSendNotification);
            posYEditor.setText(juce::String(v2, 2), juce::dontSendNotification);
            posZEditor.setText(juce::String(v3, 2), juce::dontSendNotification);
        }
        else if (coordMode == WFSCoordinates::Mode::Cylindrical)
        {
            posXEditor.setText(juce::String(v1, 2), juce::dontSendNotification);  // radius
            posYEditor.setText(juce::String(v2, 1), juce::dontSendNotification);  // theta
            posZEditor.setText(juce::String(v3, 2), juce::dontSendNotification);  // height
        }
        else  // Spherical
        {
            posXEditor.setText(juce::String(v1, 2), juce::dontSendNotification);  // radius
            posYEditor.setText(juce::String(v2, 1), juce::dontSendNotification);  // theta
            posZEditor.setText(juce::String(v3, 1), juce::dontSendNotification);  // phi
        }
    }

    // ==================== PARAMETER MANAGEMENT ====================

    void loadChannelParameters(int channel)
    {
        currentChannel = channel;
        isLoadingParameters = true;  // Prevent saving while loading

        // Helper functions for getting parameters with defaults
        auto getParam = [this, channel](const juce::String& paramName) -> juce::var {
            return parameters.getOutputParam(channel - 1, paramName);
        };

        auto getFloatParam = [&getParam](const juce::String& paramName, float defaultVal) -> float {
            auto val = getParam(paramName);
            return val.isVoid() ? defaultVal : static_cast<float>(val);
        };

        auto getIntParam = [&getParam](const juce::String& paramName, int defaultVal) -> int {
            auto val = getParam(paramName);
            return val.isVoid() ? defaultVal : static_cast<int>(val);
        };

        auto getStringParam = [&getParam](const juce::String& paramName) -> juce::String {
            return getParam(paramName).toString();
        };

        // Load name
        juce::String name = getStringParam("outputName");
        nameEditor.setText(name.isEmpty() ? "Output " + juce::String(channel) : name, juce::dontSendNotification);

        // Load array settings
        int array = getIntParam("outputArray", 0);
        arraySelector.setSelectedId(array + 1, juce::dontSendNotification);
        int applyToArray = getIntParam("outputApplyToArray", 0);
        applyToArraySelector.setSelectedId(applyToArray + 1, juce::dontSendNotification);

        // Output Properties - attenuation stored as dB (-92 to 0), default 0dB
        float attenDB = getFloatParam("outputAttenuation", 0.0f);
        attenDB = juce::jlimit(-92.0f, 0.0f, attenDB);
        // Convert dB to slider value (0-1) using inverse of logarithmic formula
        float minLinear = std::pow(10.0f, -92.0f / 20.0f);
        float targetLinear = std::pow(10.0f, attenDB / 20.0f);
        float attenSliderVal = std::sqrt((targetLinear - minLinear) / (1.0f - minLinear));
        attenuationSlider.setValue(juce::jlimit(0.0f, 1.0f, attenSliderVal));
        attenuationValueLabel.setText(juce::String(attenDB, 1) + " dB", juce::dontSendNotification);

        // Delay/Latency stored as ms (-100 to 100), default 0ms
        float delayMs = getFloatParam("outputDelayLatency", 0.0f);
        delayMs = juce::jlimit(-100.0f, 100.0f, delayMs);
        delayLatencySlider.setValue(delayMs / 100.0f);  // Convert ms to slider value (-1 to 1)
        juce::String delayLabel = (delayMs < 0) ? "Latency: " : "Delay: ";
        delayLatencyValueLabel.setText(delayLabel + juce::String(std::abs(delayMs), 1) + " ms", juce::dontSendNotification);

        bool minLatency = getIntParam("outputMiniLatencyEnable", 1) != 0;  // Default ON
        minLatencyEnableButton.setToggleState(minLatency, juce::dontSendNotification);
        minLatencyEnableButton.setButtonText(minLatency ? "Minimal Latency: ON" : "Minimal Latency: OFF");

        bool lsAtten = getIntParam("outputLSattenEnable", 1) != 0;  // Default ON
        liveSourceEnableButton.setToggleState(lsAtten, juce::dontSendNotification);
        liveSourceEnableButton.setButtonText(lsAtten ? "Live Source Atten: ON" : "Live Source Atten: OFF");

        bool frEnable = getIntParam("outputFRenable", 1) != 0;  // Default ON
        floorReflectionsEnableButton.setToggleState(frEnable, juce::dontSendNotification);
        floorReflectionsEnableButton.setButtonText(frEnable ? "Floor Reflections: ON" : "Floor Reflections: OFF");

        int distAtten = getIntParam("outputDistanceAttenPercent", 100);  // Default 100%
        distanceAttenSlider.setValue((distAtten / 100.0f) - 1.0f);
        distanceAttenValueLabel.setText(juce::String(distAtten) + " %", juce::dontSendNotification);

        float hParallax = getFloatParam("outputHparallax", 0.0f);
        hParallaxEditor.setText(juce::String(hParallax, 2), false);

        float vParallax = getFloatParam("outputVparallax", 0.0f);
        vParallaxEditor.setText(juce::String(vParallax, 2), false);

        // Position - update coordinate mode selector and position editors (handles coordinate conversion)
        updatePositionLabelsAndValues();

        float orientation = getFloatParam("outputOrientation", 0.0f);
        orientationDial.setAngle(orientation);
        orientationValueLabel.setText(juce::String(static_cast<int>(orientation)), juce::dontSendNotification);

        int angleOn = getIntParam("outputAngleOn", 86);  // Default 86°
        angleOnSlider.setValue((angleOn - 1.0f) / 179.0f);
        angleOnValueLabel.setText(juce::String(angleOn) + juce::String::fromUTF8("°"), juce::dontSendNotification);

        int angleOff = getIntParam("outputAngleOff", 90);  // Default 90°
        angleOffSlider.setValue(angleOff / 179.0f);
        angleOffValueLabel.setText(juce::String(angleOff) + juce::String::fromUTF8("°"), juce::dontSendNotification);

        int pitch = getIntParam("outputPitch", 0);  // Default 0°
        pitchSlider.setValue(pitch / 90.0f);
        pitchValueLabel.setText(juce::String(pitch) + juce::String::fromUTF8("°"), juce::dontSendNotification);

        float hfDamping = getFloatParam("outputHFdamping", 0.0f);  // Default 0 dB/m
        hfDampingSlider.setValue((hfDamping + 6.0f) / 6.0f);
        hfDampingValueLabel.setText(juce::String(hfDamping, 1) + " dB/m", juce::dontSendNotification);

        // EQ
        bool eqEnabled = getIntParam("outputEQenabled", 1) != 0;  // Default ON
        eqEnableButton.setToggleState(eqEnabled, juce::dontSendNotification);
        eqEnableButton.setButtonText(eqEnabled ? "EQ ON" : "EQ OFF");

        // Load EQ band parameters
        auto eqTree = parameters.getValueTreeState().getOutputEQSection(channel - 1);
        if (eqTree.isValid())
        {
            for (int i = 0; i < numEqBands; ++i)
            {
                auto band = eqTree.getChild(i);
                if (!band.isValid()) continue;

                int shape = band.getProperty(WFSParameterIDs::eqShape, 0);
                eqBandShapeSelector[i].setSelectedId(shape + 1, juce::dontSendNotification);

                int freq = band.getProperty(WFSParameterIDs::eqFrequency, 1000);
                float freqSlider = std::log10(freq / 20.0f) / 3.0f;
                eqBandFreqSlider[i].setValue(juce::jlimit(0.0f, 1.0f, freqSlider));
                eqBandFreqValueLabel[i].setText(formatFrequency(freq), juce::dontSendNotification);

                float gain = band.getProperty(WFSParameterIDs::eqGain, 0.0f);
                eqBandGainDial[i].setValue((gain + 24.0f) / 48.0f);
                eqBandGainValueLabel[i].setText(juce::String(gain, 1) + " dB", juce::dontSendNotification);

                float q = band.getProperty(WFSParameterIDs::eqQ, 0.7f);
                float qSlider = std::log((q - 0.1f) / 0.099f + 1.0f) / std::log(100.0f);
                eqBandQDial[i].setValue(juce::jlimit(0.0f, 1.0f, qSlider));
                eqBandQValueLabel[i].setText(juce::String(q, 2), juce::dontSendNotification);

                updateEqBandAppearance(i);
            }

            // Create EQ display component only if channel changed or doesn't exist
            // This prevents destroying the component mid-drag when ValueTree changes trigger reload
            if (eqDisplay == nullptr || lastEqDisplayChannel != channel)
            {
                eqDisplay = std::make_unique<EQDisplayComponent>(eqTree, numEqBands, EQDisplayConfig::forOutputEQ());
                addAndMakeVisible(*eqDisplay);
                lastEqDisplayChannel = channel;
            }
            // Update EQ display enabled state
            eqDisplay->setEQEnabled(eqEnabled);
            // Update visibility based on current tab
            bool eqTabVisible = (subTabBar.getCurrentTabIndex() == 1);
            eqDisplay->setVisible(eqTabVisible);
            if (eqTabVisible)
                layoutEqTab();
        }
        else
        {
            for (int i = 0; i < numEqBands; ++i)
                updateEqBandAppearance(i);
        }

        isLoadingParameters = false;
        updateApplyToArrayEnabledState();
        updateMapVisibilityButtonState();
        updateArrayLinkIndicators();
    }

    void saveOutputParam(const juce::Identifier& paramId, const juce::var& value)
    {
        if (isLoadingParameters) return;
        parameters.setOutputParam(currentChannel - 1, paramId.toString(), value);
    }

    void saveEqBandParam(int bandIndex, const juce::Identifier& paramId, const juce::var& value)
    {
        if (isLoadingParameters) return;
        // Array propagation is now handled automatically by setOutputEQBandParam
        parameters.setOutputEQBandParam(currentChannel - 1, bandIndex, paramId.toString(), value);
    }

    juce::String formatFrequency(int freq)
    {
        if (freq >= 1000)
            return juce::String(freq / 1000.0f, 1) + " kHz";
        else
            return juce::String(freq) + " Hz";
    }

    void updateArrayParameter()
    {
        updateApplyToArrayEnabledState();
        updateMapVisibilityButtonState();
        updateArrayLinkIndicators();
        saveOutputParam(WFSParameterIDs::outputArray, arraySelector.getSelectedId() - 1);
    }

    void updateApplyToArrayParameter()
    {
        updateArrayLinkIndicators();
        saveOutputParam(WFSParameterIDs::outputApplyToArray, applyToArraySelector.getSelectedId() - 1);
    }

    void updateApplyToArrayEnabledState()
    {
        bool isPartOfArray = arraySelector.getSelectedId() > 1;
        applyToArraySelector.setEnabled(isPartOfArray);
        applyToArrayLabel.setAlpha(isPartOfArray ? 1.0f : 0.5f);
    }

    void toggleMapVisibility()
    {
        bool isPartOfArray = arraySelector.getSelectedId() > 1;

        if (isPartOfArray)
        {
            // Toggle array visibility for all outputs in the same array
            int array = arraySelector.getSelectedId() - 1;

            // Get current visibility state
            auto currentVal = parameters.getOutputParam(currentChannel - 1, "outputArrayMapVisible");
            bool currentlyVisible = currentVal.isVoid() || static_cast<int>(currentVal) != 0;
            bool newVisible = !currentlyVisible;

            // Apply to all outputs in this array
            int numOutputs = parameters.getNumOutputChannels();
            for (int i = 0; i < numOutputs; ++i)
            {
                int outputArray = static_cast<int>(parameters.getOutputParam(i, "outputArray"));
                if (outputArray == array)
                {
                    parameters.setOutputParam(i, "outputArrayMapVisible", newVisible ? 1 : 0);
                }
            }

            updateMapVisibilityButtonState();
        }
        else
        {
            // Toggle individual speaker visibility
            auto currentVal = parameters.getOutputParam(currentChannel - 1, "outputMapVisible");
            bool currentlyVisible = currentVal.isVoid() || static_cast<int>(currentVal) != 0;
            bool newVisible = !currentlyVisible;

            saveOutputParam(WFSParameterIDs::outputMapVisible, newVisible ? 1 : 0);
            updateMapVisibilityButtonState();
        }
    }

    void updateMapVisibilityButtonState()
    {
        bool isPartOfArray = arraySelector.getSelectedId() > 1;

        if (isPartOfArray)
        {
            auto val = parameters.getOutputParam(currentChannel - 1, "outputArrayMapVisible");
            bool visible = val.isVoid() || static_cast<int>(val) != 0;
            mapVisibilityButton.setButtonText(visible ? "Array Visible on Map" : "Array Hidden on Map");
        }
        else
        {
            auto val = parameters.getOutputParam(currentChannel - 1, "outputMapVisible");
            bool visible = val.isVoid() || static_cast<int>(val) != 0;
            mapVisibilityButton.setButtonText(visible ? "Speaker Visible on Map" : "Speaker Hidden on Map");
        }
    }

    // ==================== TEXT EDITOR LISTENER ====================

    void textEditorReturnKeyPressed(juce::TextEditor& editor) override
    {
        editor.giveAwayKeyboardFocus();
        grabKeyboardFocus();  // Grab focus back so keyboard shortcuts work
    }

    void textEditorEscapeKeyPressed(juce::TextEditor& editor) override
    {
        // Revert to stored value and release focus
        if (&editor == &nameEditor)
            editor.setText(parameters.getOutputParam(currentChannel - 1, "outputName").toString(), false);
        else if (&editor == &posXEditor)
            editor.setText(juce::String((float)parameters.getOutputParam(currentChannel - 1, "outputPositionX"), 2), false);
        else if (&editor == &posYEditor)
            editor.setText(juce::String((float)parameters.getOutputParam(currentChannel - 1, "outputPositionY"), 2), false);
        else if (&editor == &posZEditor)
            editor.setText(juce::String((float)parameters.getOutputParam(currentChannel - 1, "outputPositionZ"), 2), false);
        else if (&editor == &hParallaxEditor)
            editor.setText(juce::String((float)parameters.getOutputParam(currentChannel - 1, "outputHparallax"), 2), false);
        else if (&editor == &vParallaxEditor)
            editor.setText(juce::String((float)parameters.getOutputParam(currentChannel - 1, "outputVparallax"), 2), false);

        editor.giveAwayKeyboardFocus();
        grabKeyboardFocus();  // Grab focus back so keyboard shortcuts work
    }

    void textEditorFocusLost(juce::TextEditor& editor) override
    {
        if (isLoadingParameters) return;

        // Save text editor values to parameters
        if (&editor == &nameEditor)
            saveOutputParam(WFSParameterIDs::outputName, nameEditor.getText());
        else if (&editor == &posXEditor || &editor == &posYEditor || &editor == &posZEditor)
        {
            // Get all three values from editors
            float v1 = posXEditor.getText().getFloatValue();
            float v2 = posYEditor.getText().getFloatValue();
            float v3 = posZEditor.getText().getFloatValue();

            // Get coordinate mode and convert to Cartesian
            int mode = static_cast<int>(parameters.getOutputParam(currentChannel - 1, "outputCoordinateMode"));
            auto coordMode = static_cast<WFSCoordinates::Mode>(mode);
            auto cart = WFSCoordinates::displayToCartesian(coordMode, v1, v2, v3);

            // Save Cartesian values
            saveOutputParam(WFSParameterIDs::outputPositionX, cart.x);
            saveOutputParam(WFSParameterIDs::outputPositionY, cart.y);
            saveOutputParam(WFSParameterIDs::outputPositionZ, cart.z);

            // Update display with values (converted back to display coords)
            updatePositionLabelsAndValues();
        }
        else if (&editor == &hParallaxEditor)
            saveOutputParam(WFSParameterIDs::outputHparallax, hParallaxEditor.getText().getFloatValue());
        else if (&editor == &vParallaxEditor)
            saveOutputParam(WFSParameterIDs::outputVparallax, vParallaxEditor.getText().getFloatValue());
    }

    // ==================== LABEL LISTENER ====================

    void labelTextChanged(juce::Label* label) override
    {
        juce::String text = label->getText();

        // Parse numeric value from text (strips units like "dB", "°", "%", "ms", "dB/m")
        float value = text.retainCharacters("-0123456789.").getFloatValue();

        if (label == &attenuationValueLabel)
        {
            // Attenuation: -92 to 0 dB, need to convert to 0-1 slider value
            // Using inverse of: dB = 20 * log10(10^(-92/20) + ((1 - 10^(-92/20)) * v^2))
            float dB = juce::jlimit(-92.0f, 0.0f, value);
            float minLinear = std::pow(10.0f, -92.0f / 20.0f);
            float targetLinear = std::pow(10.0f, dB / 20.0f);
            float v = std::sqrt((targetLinear - minLinear) / (1.0f - minLinear));
            attenuationSlider.setValue(juce::jlimit(0.0f, 1.0f, v));
            // Force label update
            attenuationValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
        }
        else if (label == &delayLatencyValueLabel)
        {
            // Delay/Latency: -100 to 100 ms, maps to slider -1 to 1
            float ms = juce::jlimit(-100.0f, 100.0f, value);
            delayLatencySlider.setValue(ms / 100.0f);
            // Force label update
            juce::String labelText = (ms < 0) ? "Latency: " : "Delay: ";
            delayLatencyValueLabel.setText(labelText + juce::String(std::abs(ms), 1) + " ms", juce::dontSendNotification);
        }
        else if (label == &distanceAttenValueLabel)
        {
            // Distance Attenuation: 0% to 200%, slider -1 to 1
            int percent = juce::jlimit(0, 200, static_cast<int>(value));
            distanceAttenSlider.setValue((percent / 100.0f) - 1.0f);
            // Force label update
            distanceAttenValueLabel.setText(juce::String(percent) + " %", juce::dontSendNotification);
        }
        else if (label == &orientationValueLabel)
        {
            // Orientation: -180 to 180 degrees (endless dial normalizes automatically)
            int degrees = static_cast<int>(value);
            while (degrees > 180) degrees -= 360;
            while (degrees < -179) degrees += 360;
            orientationDial.setAngle(static_cast<float>(degrees));
            // Force label update (unit label is separate)
            orientationValueLabel.setText(juce::String(degrees), juce::dontSendNotification);
        }
        else if (label == &angleOnValueLabel)
        {
            // Angle On: 1-180°, slider 0-1 maps to 1-180
            int degrees = juce::jlimit(1, 180, static_cast<int>(value));
            angleOnSlider.setValue((degrees - 1.0f) / 179.0f);
            // Force label update
            angleOnValueLabel.setText(juce::String(degrees) + juce::String::fromUTF8("°"), juce::dontSendNotification);
        }
        else if (label == &angleOffValueLabel)
        {
            // Angle Off: 0-179°, slider 0-1 maps to 0-179
            int degrees = juce::jlimit(0, 179, static_cast<int>(value));
            angleOffSlider.setValue(degrees / 179.0f);
            // Force label update
            angleOffValueLabel.setText(juce::String(degrees) + juce::String::fromUTF8("°"), juce::dontSendNotification);
        }
        else if (label == &pitchValueLabel)
        {
            // Pitch: -90 to 90°, slider -1 to 1
            int degrees = juce::jlimit(-90, 90, static_cast<int>(value));
            pitchSlider.setValue(degrees / 90.0f);
            // Force label update
            pitchValueLabel.setText(juce::String(degrees) + juce::String::fromUTF8("°"), juce::dontSendNotification);
        }
        else if (label == &hfDampingValueLabel)
        {
            // HF Damping: -6 to 0 dB/m, slider 0-1 maps to -6 to 0
            float dBm = juce::jlimit(-6.0f, 0.0f, value);
            hfDampingSlider.setValue((dBm + 6.0f) / 6.0f);
            // Force label update
            hfDampingValueLabel.setText(juce::String(dBm, 1) + " dB/m", juce::dontSendNotification);
        }
    }

    // ==================== STORE/RELOAD METHODS ====================

    void showStatusMessage(const juce::String& message)
    {
        if (statusBar != nullptr)
            statusBar->showTemporaryMessage(message, 3000);
    }

    void storeOutputConfiguration()
    {
        auto& fileManager = parameters.getFileManager();
        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage("Please select a project folder in System Config first.");
            return;
        }
        if (fileManager.saveOutputConfig())
            showStatusMessage("Output configuration saved.");
        else
            showStatusMessage("Error: " + fileManager.getLastError());
    }

    void reloadOutputConfiguration()
    {
        auto& fileManager = parameters.getFileManager();
        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage("Please select a project folder in System Config first.");
            return;
        }
        if (fileManager.loadOutputConfig())
        {
            loadChannelParameters(currentChannel);
            showStatusMessage("Output configuration loaded.");

            // Trigger DSP recalculation via callback to MainComponent
            if (onConfigReloaded)
                onConfigReloaded();
        }
        else
            showStatusMessage("Error: " + fileManager.getLastError());
    }

    void reloadOutputConfigBackup()
    {
        auto& fileManager = parameters.getFileManager();
        if (fileManager.loadOutputConfigBackup(0))
        {
            loadChannelParameters(currentChannel);
            showStatusMessage("Output configuration loaded from backup.");

            // Trigger DSP recalculation via callback to MainComponent
            if (onConfigReloaded)
                onConfigReloaded();
        }
        else
            showStatusMessage("Error: " + fileManager.getLastError());
    }

    void importOutputConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser>("Import Output Configuration",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.xml");
        auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result.existsAsFile())
            {
                auto& fileManager = parameters.getFileManager();
                if (fileManager.importOutputConfig(result))
                {
                    loadChannelParameters(currentChannel);
                    showStatusMessage("Output configuration imported.");

                    // Trigger DSP recalculation via callback to MainComponent
                    if (onConfigReloaded)
                        onConfigReloaded();
                }
                else
                    showStatusMessage("Error: " + fileManager.getLastError());
            }
        });
    }

    void exportOutputConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser>("Export Output Configuration",
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
                if (fileManager.exportOutputConfig(result))
                    showStatusMessage("Output configuration exported.");
                else
                    showStatusMessage("Error: " + fileManager.getLastError());
            }
        });
    }

    void openArrayPositionHelper()
    {
        if (arrayHelperWindow == nullptr)
            arrayHelperWindow = std::make_unique<OutputArrayHelperWindow>(parameters);

        arrayHelperWindow->setVisible(true);
        arrayHelperWindow->toFront(true);
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
        helpTextMap[&floorReflectionsEnableButton] = "Enable or Disable the Floor Reflections for this Speaker.";
        helpTextMap[&distanceAttenSlider] = "Ratio of Distance Attenuation for Selected Output. (changes may affect the rest of the array)";
        helpTextMap[&hParallaxEditor] = "Horizontal Distance from Speaker to 'Targeted' Listener. (changes may affect the rest of the array)";
        helpTextMap[&vParallaxEditor] = "Vertical Distance from Speaker to 'Targeted' Listener. Positive when the Speaker is Below the head of the Listener. (changes may affect the rest of the array)";
        helpTextMap[&coordModeSelector] = "Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).";
        helpTextMap[&posXEditor] = "Output Channel Position in Width.";
        helpTextMap[&posYEditor] = "Output Channel Position in Depth.";
        helpTextMap[&posZEditor] = "Output Channel Position in Height.";
        helpTextMap[&orientationDial] = "Output Channel Horizontal Orientation (0 degrees means towards the audience in a frontal configuration). (changes may affect the rest of the array)";
        helpTextMap[&angleOnSlider] = "Output Channel Will Amplify Objects in this Angle in its Back. (changes may affect the rest of the array)";
        helpTextMap[&angleOffSlider] = "Output Channel Will Not Amplify Objects in this Angle in Front of it. (changes may affect the rest of the array)";
        helpTextMap[&pitchSlider] = "Output Channel Vertical Orientation used to Determine which Objects get amplified. (changes may affect the rest of the array)";
        helpTextMap[&hfDampingSlider] = "Loss of High Frequency Depending on Distance from Object to Output. (changes may affect the rest of the array)";
        helpTextMap[&arrayPositionHelperButton] = "Open Wizard of OutZ to Position Speaker Arrays Conveniently.";
        helpTextMap[&mapVisibilityButton] = "Make Visible or Hide The Selected Output on the Map";
        helpTextMap[&storeButton] = "Store Output Configuration to file (with backup).";
        helpTextMap[&reloadButton] = "Reload Output Configuration from file.";
        helpTextMap[&reloadBackupButton] = "Reload Output Configuration from backup file.";
        helpTextMap[&importButton] = "Import Output Configuration from file (with file explorer window).";
        helpTextMap[&exportButton] = "Export Output Configuration to file (with file explorer window).";
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
        oscMethodMap[&floorReflectionsEnableButton] = "/wfs/output/FRenable <channel> <0/1>";
        oscMethodMap[&distanceAttenSlider] = "/wfs/output/DistanceAttenPercent <ID> <value>";
        oscMethodMap[&hParallaxEditor] = "/wfs/output/Hparallax <ID> <value>";
        oscMethodMap[&vParallaxEditor] = "/wfs/output/Vparallax <ID> <value>";
        oscMethodMap[&coordModeSelector] = "/wfs/output/coordinateMode <ID> <value>";
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
            // Use true for ComboBoxes to receive events from their internal child components
            bool wantsEventsFromChildren = (dynamic_cast<juce::ComboBox*>(pair.first) != nullptr);
            pair.first->addMouseListener(this, wantsEventsFromChildren);
        }
    }

    void mouseEnter(const juce::MouseEvent& event) override
    {
        if (statusBar == nullptr) return;

        // Walk up parent chain to find a registered component (needed for ComboBox children)
        juce::Component* component = event.eventComponent;
        while (component != nullptr)
        {
            if (helpTextMap.find(component) != helpTextMap.end())
            {
                statusBar->setHelpText(helpTextMap[component]);
                if (oscMethodMap.find(component) != oscMethodMap.end())
                    statusBar->setOscMethod(oscMethodMap[component]);
                return;
            }
            component = component->getParentComponent();
        }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (statusBar != nullptr)
            statusBar->clearText();
    }

    // ==================== VALUETREE LISTENER ====================

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override
    {
        // Check if output channel count changed (stored in IO tree)
        if (tree == ioTree && property == WFSParameterIDs::outputChannels)
        {
            int numOutputs = parameters.getNumOutputChannels();
            if (numOutputs > 0)
            {
                channelSelector.setNumChannels(numOutputs);
                // If current selection is beyond new limit, reset to 1
                if (channelSelector.getSelectedChannel() > numOutputs)
                    channelSelector.setSelectedChannel(1);
            }
        }

        // Check if this is a parameter change for the current channel (e.g., from OSC)
        // Skip if we're already loading parameters (avoid recursion)
        if (!isLoadingParameters)
        {
            // Find if this tree belongs to the current channel's Output tree
            juce::ValueTree parent = tree;
            while (parent.isValid())
            {
                if (parent.getType() == WFSParameterIDs::Output)
                {
                    int channelId = parent.getProperty(WFSParameterIDs::id, -1);
                    if (channelId == currentChannel)
                    {
                        // This is a parameter change for the current channel - refresh UI
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

    // ==================== MEMBER VARIABLES ====================

    WfsParameters& parameters;
    juce::ValueTree outputsTree;
    juce::ValueTree configTree;
    juce::ValueTree ioTree;
    bool isLoadingParameters = false;
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
    juce::TextButton mapVisibilityButton;

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
    juce::TextButton floorReflectionsEnableButton;
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
    juce::Label coordModeLabel;
    juce::ComboBox coordModeSelector;
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
    juce::Label orientationUnitLabel;
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

    // EQ tab components
    static constexpr int numEqBands = 6;

    // Global EQ Enable
    juce::TextButton eqEnableButton;

    // 6 EQ Bands - each with Shape, Frequency, Gain, Q
    juce::Label eqBandLabel[numEqBands];
    juce::ComboBox eqBandShapeSelector[numEqBands];
    juce::Label eqBandFreqLabel[numEqBands];
    WfsStandardSlider eqBandFreqSlider[numEqBands];
    juce::Label eqBandFreqValueLabel[numEqBands];
    juce::Label eqBandGainLabel[numEqBands];
    WfsBasicDial eqBandGainDial[numEqBands];
    juce::Label eqBandGainValueLabel[numEqBands];
    juce::Label eqBandQLabel[numEqBands];
    WfsBasicDial eqBandQDial[numEqBands];
    juce::Label eqBandQValueLabel[numEqBands];

    // EQ Display Component
    std::unique_ptr<EQDisplayComponent> eqDisplay;
    int lastEqDisplayChannel = -1;  // Track which channel's EQ display is shown

    // Array link indicators - colored dots showing parameter is linked across array
    ArrayLinkIndicator attenuationIndicator;
    ArrayLinkIndicator delayLatencyIndicator;
    ArrayLinkIndicator minLatencyIndicator;
    ArrayLinkIndicator liveSourceIndicator;
    ArrayLinkIndicator floorReflectionsIndicator;
    ArrayLinkIndicator distanceAttenIndicator;
    ArrayLinkIndicator hParallaxIndicator;
    ArrayLinkIndicator vParallaxIndicator;
    ArrayLinkIndicator orientationIndicator;
    ArrayLinkIndicator angleOnIndicator;
    ArrayLinkIndicator angleOffIndicator;
    ArrayLinkIndicator pitchIndicator;
    ArrayLinkIndicator hfDampingIndicator;
    ArrayLinkIndicator eqIndicator;  // Single indicator for all EQ parameters

    // Footer buttons
    juce::TextButton storeButton;
    juce::TextButton reloadButton;
    juce::TextButton reloadBackupButton;
    juce::TextButton importButton;
    juce::TextButton exportButton;

    // Array Position Helper window
    std::unique_ptr<OutputArrayHelperWindow> arrayHelperWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputsTab)
};
