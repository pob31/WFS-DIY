#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Accessibility/TTSManager.h"
#include "ChannelSelector.h"
#include "ColorUtilities.h"
#include "ColorScheme.h"
#include "SliderUIComponents.h"
#include "DialUIComponents.h"
#include "StatusBar.h"
#include "OutputArrayHelperWindow.h"
#include "EQDisplayComponent.h"
#include "../Helpers/CoordinateConverter.h"
#include "../Localization/LocalizationManager.h"
#include "buttons/LongPressButton.h"
#include "buttons/EQBandToggle.h"

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
          ioTree(params.getConfigTree().getChildWithName(WFSParameterIDs::IO)),
          binauralTree(params.getValueTreeState().getBinauralState())
    {
        // Enable keyboard focus so we can receive focus back after text editing
        setWantsKeyboardFocus(true);

        // Add listener to outputs tree, config tree, IO tree, and binaural tree (for solo state changes)
        outputsTree.addListener(this);
        configTree.addListener(this);
        if (ioTree.isValid())
            ioTree.addListener(this);
        if (binauralTree.isValid())
            binauralTree.addListener(this);
        ColorScheme::Manager::getInstance().addListener(this);

        // ==================== HEADER SECTION ====================
        // Channel Selector - use configured output count
        int numOutputs = parameters.getNumOutputChannels();
        channelSelector.setNumChannels(numOutputs > 0 ? numOutputs : 16);  // Default to 16 if not set
        channelSelector.onChannelChanged = [this](int channel) {
            loadChannelParameters(channel);
            if (onChannelSelected)
                onChannelSelected (channel);
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
        nameLabel.setText(LOC("outputs.labels.name"), juce::dontSendNotification);
        addAndMakeVisible(nameEditor);
        nameEditor.addListener(this);

        // Array selector
        addAndMakeVisible(arrayLabel);
        arrayLabel.setText(LOC("outputs.labels.array"), juce::dontSendNotification);
        addAndMakeVisible(arraySelector);
        arraySelector.addItem(LOC("outputs.arrayModes.single"), 1);
        for (int i = 1; i <= 10; ++i)
            arraySelector.addItem(LOC("outputs.arrayModes.array") + " " + juce::String(i), i + 1);
        arraySelector.setSelectedId(1, juce::dontSendNotification);
        arraySelector.onChange = [this]() {
            updateArrayParameter();
            // TTS: Announce selection change
            TTSManager::getInstance().announceValueChange("Array", arraySelector.getText());
        };

        // Apply to Array selector
        addAndMakeVisible(applyToArrayLabel);
        applyToArrayLabel.setText(LOC("outputs.labels.applyToArray"), juce::dontSendNotification);
        addAndMakeVisible(applyToArraySelector);
        applyToArraySelector.addItem(LOC("outputs.arrayModes.off"), 1);
        applyToArraySelector.addItem(LOC("outputs.arrayModes.absolute"), 2);
        applyToArraySelector.addItem(LOC("outputs.arrayModes.relative"), 3);
        applyToArraySelector.setSelectedId(2, juce::dontSendNotification);
        applyToArraySelector.onChange = [this]() {
            updateApplyToArrayParameter();
            // TTS: Announce selection change
            TTSManager::getInstance().announceValueChange("Apply to Array", applyToArraySelector.getText());
        };

        // Map visibility toggle button
        addAndMakeVisible(mapVisibilityButton);
        mapVisibilityButton.setButtonText(LOC("outputs.buttons.speakerVisible"));
        mapVisibilityButton.onClick = [this]() { toggleMapVisibility(); };

        // Level Meter button
        addAndMakeVisible(levelMeterButton);
        levelMeterButton.setButtonText(LOC("systemConfig.buttons.levelMeter"));
        levelMeterButton.onClick = [this]() {
            if (onLevelMeterWindowRequested)
                onLevelMeterWindowRequested();
        };

        // Wizard of OutZ button (array position helper)
        addAndMakeVisible(arrayPositionHelperButton);
        arrayPositionHelperButton.setButtonText(LOC("outputs.buttons.wizardOfOutZ"));
        arrayPositionHelperButton.onClick = [this]() { openArrayPositionHelper(); };

        // ==================== SUB-TABS ====================
        addAndMakeVisible(subTabBar);
        subTabBar.addTab(LOC("outputs.tabs.parameters"), juce::Colour(0xFF2A2A2A), -1);
        subTabBar.addTab(LOC("outputs.tabs.eq"), juce::Colour(0xFF2A2A2A), -1);
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
        storeButton.setButtonText(LOC("outputs.buttons.storeConfig"));
        storeButton.setBaseColour(juce::Colour(0xFF8C3333));  // Reddish
        storeButton.onLongPress = [this]() { storeOutputConfiguration(); };

        addAndMakeVisible(reloadButton);
        reloadButton.setButtonText(LOC("outputs.buttons.reloadConfig"));
        reloadButton.setBaseColour(juce::Colour(0xFF338C33));  // Greenish
        reloadButton.onLongPress = [this]() { reloadOutputConfiguration(); };

        addAndMakeVisible(reloadBackupButton);
        reloadBackupButton.setButtonText(LOC("outputs.buttons.reloadBackup"));
        reloadBackupButton.setBaseColour(juce::Colour(0xFF266626));  // Darker green
        reloadBackupButton.onLongPress = [this]() { reloadOutputConfigBackup(); };

        addAndMakeVisible(importButton);
        importButton.setButtonText(LOC("outputs.buttons.import"));
        importButton.setBaseColour(juce::Colour(0xFF338C33));  // Greenish
        importButton.onLongPress = [this]() { importOutputConfiguration(); };

        addAndMakeVisible(exportButton);
        exportButton.setButtonText(LOC("outputs.buttons.export"));
        exportButton.setBaseColour(juce::Colour(0xFF8C3333));  // Reddish
        exportButton.onLongPress = [this]() { exportOutputConfiguration(); };

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
        if (binauralTree.isValid())
            binauralTree.removeListener(this);
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

    /** Callback when Level Meter window is requested */
    std::function<void()> onLevelMeterWindowRequested;

    /** Callback when output channel selection changes (1-based channel ID). */
    std::function<void(int)> onChannelSelected;

    /** Callback when the subtab changes (0=Parameters, 1=EQ). */
    std::function<void(int)> onSubTabChanged;

    /** Programmatically switch the active subtab (for Stream Deck navigation). */
    void setSubTabIndex (int index) { subTabBar.setCurrentTabIndex (index); }

    /** Programmatically select a band on the EQ display (for Stream Deck sync). */
    void selectEqBand (int bandIndex)
    {
        if (eqDisplay)
            eqDisplay->setSelectedBand (bandIndex);
    }

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
                statusBar->showTemporaryMessage(LOC("outputs.messages.setToSingle").replace("{num}", juce::String(currentChannel)), 2000);
            else
                statusBar->showTemporaryMessage(LOC("outputs.messages.assignedToArray").replace("{num}", juce::String(currentChannel)).replace("{array}", juce::String(array)), 2000);
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
        layoutScale = static_cast<float>(getHeight()) / 932.0f;
        headerHeight = scaled(60);
        footerHeight = scaled(50);
        auto bounds = getLocalBounds();
        const int padding = scaled(10);
        const int rowHeight = scaled(30);
        const int spacing = scaled(5);

        // ==================== HEADER ====================
        auto headerArea = bounds.removeFromTop(headerHeight).reduced(padding, padding);

        // First row: Channel selector and Name
        auto row1 = headerArea.removeFromTop(rowHeight);
        channelSelector.setBounds(row1.removeFromLeft(scaled(150)));
        row1.removeFromLeft(spacing * 2);
        nameLabel.setBounds(row1.removeFromLeft(scaled(50)));
        nameEditor.setBounds(row1.removeFromLeft(scaled(200)));
        row1.removeFromLeft(spacing * 4);

        // Array and Apply to Array in same row
        arrayLabel.setBounds(row1.removeFromLeft(scaled(50)));
        arraySelector.setBounds(row1.removeFromLeft(scaled(100)));
        row1.removeFromLeft(spacing * 2);
        applyToArrayLabel.setBounds(row1.removeFromLeft(scaled(100)));
        applyToArraySelector.setBounds(row1.removeFromLeft(scaled(100)));
        row1.removeFromLeft(spacing * 2);
        mapVisibilityButton.setBounds(row1.removeFromLeft(scaled(180)));

        // Right-aligned buttons (from right to left)
        arrayPositionHelperButton.setBounds(row1.removeFromRight(scaled(130)));
        row1.removeFromRight(spacing);
        levelMeterButton.setBounds(row1.removeFromRight(scaled(100)));

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
        auto tabBarArea = bounds.removeFromTop(scaled(32));  // No left padding for tab bar
        subTabBar.setBounds(tabBarArea);

        // Content area for sub-tabs (with padding)
        auto contentArea = bounds.reduced(padding, 0);
        subTabContentArea = contentArea.reduced(0, padding);

        // Layout sub-tab content based on current tab
        layoutCurrentSubTab();
        WfsLookAndFeel::scaleTextEditorFonts(*this, layoutScale);
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

        if (onSubTabChanged)
            onSubTabChanged (subTabBar.getCurrentTabIndex());

        // TTS: Announce subtab change for accessibility
        int tabIndex = subTabBar.getCurrentTabIndex();
        if (tabIndex >= 0 && tabIndex < subTabBar.getNumTabs())
        {
            juce::String tabName = subTabBar.getTabButton(tabIndex)->getButtonText();
            TTSManager::getInstance().announceImmediate(tabName + " tab",
                juce::AccessibilityHandler::AnnouncementPriority::medium);
        }
    }

    // ==================== SETUP METHODS ====================

    void setupOutputPropertiesTab()
    {
        // Attenuation slider (-92 to 0 dB)
        addAndMakeVisible(attenuationLabel);
        attenuationLabel.setText(LOC("outputs.labels.attenuation"), juce::dontSendNotification);

        attenuationSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFF4A90D9));
        attenuationSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Output Attenuation");
        };
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
        delayLatencyLabel.setText(LOC("outputs.labels.delayLatency"), juce::dontSendNotification);

        delayLatencySlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFFD4A017));
        delayLatencySlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Output Delay/Latency");
        };
        delayLatencySlider.onValueChanged = [this](float v) {
            // Slider range is -1 to 1, map to -100ms to 100ms
            float ms = v * 100.0f;
            juce::String label = (ms < 0) ? LOC("outputs.labels.latency") : LOC("outputs.labels.delay");
            delayLatencyValueLabel.setText(label + " " + juce::String(std::abs(ms), 1) + " " + LOC("outputs.units.ms"), juce::dontSendNotification);
            saveOutputParam(WFSParameterIDs::outputDelayLatency, ms);  // Save real ms value
        };
        addAndMakeVisible(delayLatencySlider);

        addAndMakeVisible(delayLatencyValueLabel);
        delayLatencyValueLabel.setText(LOC("outputs.labels.delay") + " 0.0 " + LOC("outputs.units.ms"), juce::dontSendNotification);
        setupEditableValueLabel(delayLatencyValueLabel);

        // Min Latency Enable button
        addAndMakeVisible(minLatencyEnableButton);
        minLatencyEnableButton.setButtonText(LOC("outputs.toggles.minLatencyOn"));
        minLatencyEnableButton.setClickingTogglesState(true);
        minLatencyEnableButton.setToggleState(true, juce::dontSendNotification);
        minLatencyEnableButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFFD4A017));  // Yellow (time)
        minLatencyEnableButton.onClick = [this]() {
            bool enabled = minLatencyEnableButton.getToggleState();
            minLatencyEnableButton.setButtonText(enabled ? LOC("outputs.toggles.minLatencyOn") : LOC("outputs.toggles.minLatencyOff"));
            saveOutputParam(WFSParameterIDs::outputMiniLatencyEnable, enabled ? 1 : 0);
        };

        // Live Source Enable button
        addAndMakeVisible(liveSourceEnableButton);
        liveSourceEnableButton.setButtonText(LOC("outputs.toggles.liveSourceOn"));
        liveSourceEnableButton.setClickingTogglesState(true);
        liveSourceEnableButton.setToggleState(true, juce::dontSendNotification);
        liveSourceEnableButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF4A90D9));  // Blue (level)
        liveSourceEnableButton.onClick = [this]() {
            bool enabled = liveSourceEnableButton.getToggleState();
            liveSourceEnableButton.setButtonText(enabled ? LOC("outputs.toggles.liveSourceOn") : LOC("outputs.toggles.liveSourceOff"));
            saveOutputParam(WFSParameterIDs::outputLSattenEnable, enabled ? 1 : 0);
        };

        // Floor Reflections Enable button
        addAndMakeVisible(floorReflectionsEnableButton);
        floorReflectionsEnableButton.setButtonText(LOC("outputs.toggles.floorReflectionsOn"));
        floorReflectionsEnableButton.setClickingTogglesState(true);
        floorReflectionsEnableButton.setToggleState(true, juce::dontSendNotification);
        floorReflectionsEnableButton.onClick = [this]() {
            bool enabled = floorReflectionsEnableButton.getToggleState();
            floorReflectionsEnableButton.setButtonText(enabled ? LOC("outputs.toggles.floorReflectionsOn") : LOC("outputs.toggles.floorReflectionsOff"));
            // Array propagation is now handled automatically by setOutputParam
            saveOutputParam(WFSParameterIDs::outputFRenable, enabled ? 1 : 0);
        };

        // Distance Attenuation % slider (0-200%, default 100% in center)
        addAndMakeVisible(distanceAttenLabel);
        distanceAttenLabel.setText(LOC("outputs.labels.distanceAtten"), juce::dontSendNotification);

        distanceAttenSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFF4A90D9));
        // Bidirectional slider: center (0) = 100%, no need to set initial value
        distanceAttenSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Output Distance Attenuation");
        };
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
        hParallaxLabel.setText(LOC("outputs.labels.hParallax"), juce::dontSendNotification);
        addAndMakeVisible(hParallaxEditor);
        hParallaxEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(hParallaxEditor, false, true);
        addAndMakeVisible(hParallaxUnitLabel);
        hParallaxUnitLabel.setText(LOC("outputs.units.meters"), juce::dontSendNotification);

        // Vertical Parallax
        addAndMakeVisible(vParallaxLabel);
        vParallaxLabel.setText(LOC("outputs.labels.vParallax"), juce::dontSendNotification);
        addAndMakeVisible(vParallaxEditor);
        vParallaxEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(vParallaxEditor, true, true);
        addAndMakeVisible(vParallaxUnitLabel);
        vParallaxUnitLabel.setText(LOC("outputs.units.meters"), juce::dontSendNotification);

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
        coordModeLabel.setText(LOC("outputs.labels.coordinates"), juce::dontSendNotification);
        addAndMakeVisible(coordModeSelector);
        coordModeSelector.addItem(LOC("outputs.coordModes.xyz"), 1);
        coordModeSelector.addItem(juce::String(juce::CharPointer_UTF8("r \xce\xb8 Z")), 2);    // r θ Z
        coordModeSelector.addItem(juce::String(juce::CharPointer_UTF8("r \xce\xb8 \xcf\x86")), 3);  // r θ φ
        coordModeSelector.setSelectedId(1, juce::dontSendNotification);
        coordModeSelector.onChange = [this]() {
            int mode = coordModeSelector.getSelectedId() - 1;
            saveOutputParam(WFSParameterIDs::outputCoordinateMode, mode);
            updatePositionLabelsAndValues();
            // TTS: Announce selection change
            TTSManager::getInstance().announceValueChange("Coordinate Mode", coordModeSelector.getText());
        };

        // Position X
        addAndMakeVisible(posXLabel);
        posXLabel.setText(LOC("outputs.labels.positionX"), juce::dontSendNotification);
        addAndMakeVisible(posXEditor);
        posXEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(posXEditor, true, true);
        addAndMakeVisible(posXUnitLabel);
        posXUnitLabel.setText(LOC("outputs.units.meters"), juce::dontSendNotification);

        // Position Y
        addAndMakeVisible(posYLabel);
        posYLabel.setText(LOC("outputs.labels.positionY"), juce::dontSendNotification);
        addAndMakeVisible(posYEditor);
        posYEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(posYEditor, true, true);
        addAndMakeVisible(posYUnitLabel);
        posYUnitLabel.setText(LOC("outputs.units.meters"), juce::dontSendNotification);

        // Position Z
        addAndMakeVisible(posZLabel);
        posZLabel.setText(LOC("outputs.labels.positionZ"), juce::dontSendNotification);
        addAndMakeVisible(posZEditor);
        posZEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(posZEditor, true, true);
        addAndMakeVisible(posZUnitLabel);
        posZUnitLabel.setText(LOC("outputs.units.meters"), juce::dontSendNotification);

        // Directional dial (orientation + angle on/off visualization)
        addAndMakeVisible(orientationLabel);
        orientationLabel.setText(LOC("outputs.labels.orientation"), juce::dontSendNotification);

        directionalDial.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Output Orientation");
        };
        directionalDial.onOrientationChanged = [this](float angle) {
            orientationValueLabel.setText(juce::String(static_cast<int>(angle)), juce::dontSendNotification);
            saveOutputParam(WFSParameterIDs::outputOrientation, angle);
        };
        directionalDial.onAngleOnChanged = [this](int degrees) {
            int angleOff = directionalDial.getAngleOff();
            // Enforce constraint: angleOn + angleOff <= 180
            if (degrees + angleOff > 180)
            {
                angleOff = 180 - degrees;
                directionalDial.setAngleOff(angleOff);
                angleOffSlider.setValue(angleOff / 179.0f);
                angleOffValueLabel.setText(juce::String(angleOff) + juce::String::fromUTF8("°"), juce::dontSendNotification);
                saveOutputParam(WFSParameterIDs::outputAngleOff, angleOff);
            }
            angleOnSlider.setValue((degrees - 1.0f) / 179.0f);
            angleOnValueLabel.setText(juce::String(degrees) + juce::String::fromUTF8("°"), juce::dontSendNotification);
            saveOutputParam(WFSParameterIDs::outputAngleOn, degrees);
        };
        directionalDial.onAngleOffChanged = [this](int degrees) {
            int angleOn = directionalDial.getAngleOn();
            // Enforce constraint: angleOn + angleOff <= 180
            if (angleOn + degrees > 180)
            {
                angleOn = 180 - degrees;
                directionalDial.setAngleOn(angleOn);
                angleOnSlider.setValue((angleOn - 1.0f) / 179.0f);
                angleOnValueLabel.setText(juce::String(angleOn) + juce::String::fromUTF8("°"), juce::dontSendNotification);
                saveOutputParam(WFSParameterIDs::outputAngleOn, angleOn);
            }
            angleOffSlider.setValue(degrees / 179.0f);
            angleOffValueLabel.setText(juce::String(degrees) + juce::String::fromUTF8("°"), juce::dontSendNotification);
            saveOutputParam(WFSParameterIDs::outputAngleOff, degrees);
        };
        addAndMakeVisible(directionalDial);
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
        angleOnLabel.setText(LOC("outputs.labels.angleOn"), juce::dontSendNotification);

        angleOnSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFF4CAF50));  // Green
        angleOnSlider.setValue(0.47f);  // ~86°
        angleOnSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Output Angle On");
        };
        angleOnSlider.onValueChanged = [this](float v) {
            int angleOn = static_cast<int>(v * 179.0f + 1.0f);
            int angleOff = static_cast<int>(angleOffSlider.getValue() * 179.0f);

            // Enforce constraint: angleOn + angleOff <= 180
            if (angleOn + angleOff > 180)
            {
                angleOff = 180 - angleOn;
                angleOffSlider.setValue(angleOff / 179.0f);
                angleOffValueLabel.setText(juce::String(angleOff) + juce::String::fromUTF8("°"), juce::dontSendNotification);
                saveOutputParam(WFSParameterIDs::outputAngleOff, angleOff);
                directionalDial.setAngleOff(angleOff);
            }

            angleOnValueLabel.setText(juce::String(angleOn) + juce::String::fromUTF8("°"), juce::dontSendNotification);
            saveOutputParam(WFSParameterIDs::outputAngleOn, angleOn);
            directionalDial.setAngleOn(angleOn);
        };
        addAndMakeVisible(angleOnSlider);
        addAndMakeVisible(angleOnValueLabel);
        angleOnValueLabel.setText(juce::String::fromUTF8("86°"), juce::dontSendNotification);
        setupEditableValueLabel(angleOnValueLabel);

        // Angle Off slider (0-179°)
        addAndMakeVisible(angleOffLabel);
        angleOffLabel.setText(LOC("outputs.labels.angleOff"), juce::dontSendNotification);

        angleOffSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFFE53935));  // Red
        angleOffSlider.setValue(0.5f);  // ~90°
        angleOffSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Output Angle Off");
        };
        angleOffSlider.onValueChanged = [this](float v) {
            int angleOff = static_cast<int>(v * 179.0f);
            int angleOn = static_cast<int>(angleOnSlider.getValue() * 179.0f + 1.0f);

            // Enforce constraint: angleOn + angleOff <= 180
            if (angleOn + angleOff > 180)
            {
                angleOn = 180 - angleOff;
                angleOnSlider.setValue((angleOn - 1.0f) / 179.0f);
                angleOnValueLabel.setText(juce::String(angleOn) + juce::String::fromUTF8("°"), juce::dontSendNotification);
                saveOutputParam(WFSParameterIDs::outputAngleOn, angleOn);
                directionalDial.setAngleOn(angleOn);
            }

            angleOffValueLabel.setText(juce::String(angleOff) + juce::String::fromUTF8("°"), juce::dontSendNotification);
            saveOutputParam(WFSParameterIDs::outputAngleOff, angleOff);
            directionalDial.setAngleOff(angleOff);
        };
        addAndMakeVisible(angleOffSlider);
        addAndMakeVisible(angleOffValueLabel);
        angleOffValueLabel.setText(juce::String::fromUTF8("90°"), juce::dontSendNotification);
        setupEditableValueLabel(angleOffValueLabel);

        // Pitch slider (-90 to 90°)
        addAndMakeVisible(pitchLabel);
        pitchLabel.setText(LOC("outputs.labels.pitch"), juce::dontSendNotification);

        pitchSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFF26A69A));
        pitchSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Output Pitch");
        };
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
        hfDampingLabel.setText(LOC("outputs.labels.hfDamping"), juce::dontSendNotification);

        hfDampingSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFFE07878));
        hfDampingSlider.setValue(1.0f);  // 0 dB/m
        hfDampingSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Output HF Damping");
        };
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
        eqEnableButton.setButtonText(LOC("eq.status.on"));
        eqEnableButton.setClickingTogglesState(true);
        eqEnableButton.setToggleState(true, juce::dontSendNotification);
        eqEnableButton.onClick = [this]() {
            bool enabled = eqEnableButton.getToggleState();
            eqEnableButton.setButtonText(enabled ? LOC("eq.status.on") : LOC("eq.status.off"));
            for (int i = 0; i < numEqBands; ++i)
                updateEqBandAppearance(i);
            if (eqDisplay != nullptr)
                eqDisplay->setEQEnabled(enabled);
            saveOutputParam(WFSParameterIDs::outputEQenabled, enabled ? 1 : 0);
        };

        // Flatten EQ long-press button
        addAndMakeVisible(eqFlattenButton);
        eqFlattenButton.setButtonText(LOC("eq.buttons.flattenEQ"));
        eqFlattenButton.onLongPress = [this]() {
            for (int i = 0; i < numEqBands; ++i)
                resetEqBand(i);
        };

        // 6 EQ Bands
        for (int i = 0; i < numEqBands; ++i)
        {
            // Band label - colored to match EQ display markers
            addAndMakeVisible(eqBandLabel[i]);
            eqBandLabel[i].setText(LOC("eq.labels.band") + " " + juce::String(i + 1), juce::dontSendNotification);
            eqBandLabel[i].setColour(juce::Label::textColourId, EQDisplayComponent::getBandColour(i));
            eqBandLabel[i].setJustificationType(juce::Justification::centredLeft);

            // Band on/off toggle indicator
            addAndMakeVisible(eqBandToggle[i]);
            eqBandToggle[i].setBandColour(EQDisplayComponent::getBandColour(i));
            eqBandToggle[i].setToggleState(false, juce::dontSendNotification);
            eqBandToggle[i].onClick = [this, i]() {
                bool on = eqBandToggle[i].getToggleState();
                int shape = on ? eqBandShapeSelector[i].getSelectedId() : 0;
                saveEqBandParam(i, WFSParameterIDs::eqShape, shape);
                updateEqBandAppearance(i);
            };

            // Reset band long-press button
            addAndMakeVisible(eqBandResetButton[i]);
            eqBandResetButton[i].setButtonText(LOC("eq.buttons.resetBand"));
            eqBandResetButton[i].onLongPress = [this, i]() { resetEqBand(i); };

            // Shape dropdown (no "Off" - toggle handles on/off)
            addAndMakeVisible(eqBandShapeSelector[i]);
            eqBandShapeSelector[i].addItem(LOC("eq.filterTypes.lowCut"), 1);
            eqBandShapeSelector[i].addItem(LOC("eq.filterTypes.lowShelf"), 2);
            eqBandShapeSelector[i].addItem(LOC("eq.filterTypes.peakNotch"), 3);
            eqBandShapeSelector[i].addItem(LOC("eq.filterTypes.bandPass"), 4);
            eqBandShapeSelector[i].addItem(LOC("eq.filterTypes.allPass"), 7);
            eqBandShapeSelector[i].addItem(LOC("eq.filterTypes.highShelf"), 5);
            eqBandShapeSelector[i].addItem(LOC("eq.filterTypes.highCut"), 6);
            eqBandShapeSelector[i].setSelectedId(WFSParameterDefaults::eqBandComboDefaults[i], juce::dontSendNotification);

            // Shape change handler - only save if band is ON
            eqBandShapeSelector[i].onChange = [this, i]() {
                if (eqBandToggle[i].getToggleState())
                {
                    int shape = eqBandShapeSelector[i].getSelectedId();
                    saveEqBandParam(i, WFSParameterIDs::eqShape, shape);
                }
                updateEqBandAppearance(i);
                TTSManager::getInstance().announceValueChange("EQ Band " + juce::String(i + 1) + " Shape", eqBandShapeSelector[i].getText());
            };

            // Frequency slider - colored to match band
            addAndMakeVisible(eqBandFreqLabel[i]);
            eqBandFreqLabel[i].setText(LOC("eq.labels.freq"), juce::dontSendNotification);
            eqBandFreqLabel[i].setColour(juce::Label::textColourId, juce::Colours::grey);

            juce::Colour bandColour = EQDisplayComponent::getBandColour(i);
            eqBandFreqSlider[i].setTrackColours(juce::Colour(0xFF2D2D2D), bandColour);
            eqBandFreqSlider[i].onGestureStart = [this, i]() {
                parameters.getValueTreeState().beginUndoTransaction ("Output EQ Freq Band " + juce::String(i + 1));
            };
            eqBandFreqSlider[i].onValueChanged = [this, i](float v) {
                int freq = static_cast<int>(20.0f * std::pow(10.0f, 3.0f * v));
                eqBandFreqValueLabel[i].setText(formatFrequency(freq), juce::dontSendNotification);
                saveEqBandParam(i, WFSParameterIDs::eqFrequency, freq);
            };
            addAndMakeVisible(eqBandFreqSlider[i]);

            addAndMakeVisible(eqBandFreqValueLabel[i]);
            eqBandFreqValueLabel[i].setText("1000 Hz", juce::dontSendNotification);
            setupEditableValueLabel(eqBandFreqValueLabel[i]);

            // Gain dial - colored to match band
            addAndMakeVisible(eqBandGainLabel[i]);
            eqBandGainLabel[i].setText(LOC("eq.labels.gain"), juce::dontSendNotification);
            eqBandGainLabel[i].setColour(juce::Label::textColourId, juce::Colours::grey);
            eqBandGainLabel[i].setJustificationType(juce::Justification::centred);

            eqBandGainDial[i].setTrackColours(juce::Colour(0xFF2D2D2D), bandColour);
            eqBandGainDial[i].onGestureStart = [this, i]() {
                parameters.getValueTreeState().beginUndoTransaction ("Output EQ Gain Band " + juce::String(i + 1));
            };
            eqBandGainDial[i].onValueChanged = [this, i](float v) {
                float gain = v * 48.0f - 24.0f;  // -24 to +24 dB
                eqBandGainValueLabel[i].setText(juce::String(gain, 1) + " dB", juce::dontSendNotification);
                saveEqBandParam(i, WFSParameterIDs::eqGain, gain);
            };
            addAndMakeVisible(eqBandGainDial[i]);

            addAndMakeVisible(eqBandGainValueLabel[i]);
            eqBandGainValueLabel[i].setText("0.0 dB", juce::dontSendNotification);
            eqBandGainValueLabel[i].setEditable(true, false);
            eqBandGainValueLabel[i].addListener(this);
            eqBandGainValueLabel[i].setJustificationType(juce::Justification::centred);

            // Q dial - colored to match band
            addAndMakeVisible(eqBandQLabel[i]);
            eqBandQLabel[i].setText(LOC("eq.labels.q"), juce::dontSendNotification);
            eqBandQLabel[i].setColour(juce::Label::textColourId, juce::Colours::grey);
            eqBandQLabel[i].setJustificationType(juce::Justification::centred);

            eqBandQDial[i].setTrackColours(juce::Colour(0xFF2D2D2D), bandColour);
            eqBandQDial[i].onGestureStart = [this, i]() {
                parameters.getValueTreeState().beginUndoTransaction ("Output EQ Q Band " + juce::String(i + 1));
            };
            eqBandQDial[i].onValueChanged = [this, i](float v) {
                float q = 0.1f + 0.099f * (std::pow(100.0f, v) - 1.0f);  // 0.1-10.0
                eqBandQValueLabel[i].setText(juce::String(q, 2), juce::dontSendNotification);
                saveEqBandParam(i, WFSParameterIDs::eqQ, q);
            };
            addAndMakeVisible(eqBandQDial[i]);

            addAndMakeVisible(eqBandQValueLabel[i]);
            eqBandQValueLabel[i].setText("0.70", juce::dontSendNotification);
            eqBandQValueLabel[i].setEditable(true, false);
            eqBandQValueLabel[i].addListener(this);
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
        bool bandIsOff = !eqBandToggle[bandIndex].getToggleState();

        // Determine if this is a cut, bandpass, or allpass filter (no gain control)
        // Output EQ shapes: 1=LowCut, 2=LowShelf, 3=Peak, 4=BandPass, 5=HighShelf, 6=HighCut, 7=AllPass
        int shapeId = eqBandShapeSelector[bandIndex].getSelectedId();
        bool isCutOrBandPass = (shapeId == 1 || shapeId == 4 || shapeId == 6 || shapeId == 7);
        bool showGain = !isCutOrBandPass;

        // Grey out entire band if global EQ is off
        // Grey out band parameters (except shape/toggle) if band is off but EQ is on
        float bandLabelAlpha = eqEnabled ? 1.0f : 0.4f;
        float toggleAlpha = eqEnabled ? 1.0f : 0.4f;
        float shapeAlpha = eqEnabled ? 1.0f : 0.4f;
        float paramAlpha = (eqEnabled && !bandIsOff) ? 1.0f : 0.4f;

        // Band label, toggle, shape dropdown, and reset button follow global EQ state
        eqBandLabel[bandIndex].setAlpha(bandLabelAlpha);
        eqBandToggle[bandIndex].setAlpha(toggleAlpha);
        eqBandShapeSelector[bandIndex].setAlpha(bandIsOff ? 0.4f : shapeAlpha);
        eqBandResetButton[bandIndex].setAlpha(bandLabelAlpha);

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

    void resetEqBand(int i)
    {
        using namespace WFSParameterDefaults;
        isLoadingParameters = true;

        int defaultShape = eqBandShapes[i];
        float defaultFreq = eqBandFrequencies[i];

        // Set toggle state based on default shape
        eqBandToggle[i].setToggleState(defaultShape != 0, juce::dontSendNotification);

        // Combobox: show per-band combo default
        eqBandShapeSelector[i].setSelectedId(eqBandComboDefaults[i], juce::dontSendNotification);

        // Frequency
        float freqSlider = std::log10(defaultFreq / 20.0f) / 3.0f;
        eqBandFreqSlider[i].setValue(juce::jlimit(0.0f, 1.0f, freqSlider));
        eqBandFreqValueLabel[i].setText(formatFrequency(static_cast<int>(defaultFreq)), juce::dontSendNotification);

        // Gain: 0 dB = 0.5 dial value
        eqBandGainDial[i].setValue(0.5f);
        eqBandGainValueLabel[i].setText("0.0 dB", juce::dontSendNotification);

        // Q: 0.7 default - inverse mapping: log((0.7 - 0.1) / 0.099 + 1) / log(100)
        float qSlider = std::log((eqQDefault - 0.1f) / 0.099f + 1.0f) / std::log(100.0f);
        eqBandQDial[i].setValue(juce::jlimit(0.0f, 1.0f, qSlider));
        eqBandQValueLabel[i].setText("0.70", juce::dontSendNotification);

        isLoadingParameters = false;

        // Save all values
        saveEqBandParam(i, WFSParameterIDs::eqShape, defaultShape);
        saveEqBandParam(i, WFSParameterIDs::eqFrequency, static_cast<int>(defaultFreq));
        saveEqBandParam(i, WFSParameterIDs::eqGain, 0.0f);
        saveEqBandParam(i, WFSParameterIDs::eqQ, eqQDefault);

        updateEqBandAppearance(i);
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
        directionalDial.setVisible(visible);
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
        // Global EQ Enable + Flatten buttons
        eqEnableButton.setVisible(visible);
        eqFlattenButton.setVisible(visible);

        // EQ Display
        if (eqDisplay)
            eqDisplay->setVisible(visible);

        // 6 EQ Bands
        for (int i = 0; i < numEqBands; ++i)
        {
            eqBandLabel[i].setVisible(visible);
            eqBandToggle[i].setVisible(visible);
            eqBandShapeSelector[i].setVisible(visible);
            eqBandResetButton[i].setVisible(visible);
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
        const int rowHeight = scaled(30);
        const int sliderHeight = scaled(40);
        const int spacing = scaled(8);
        const int labelWidth = scaled(115);
        const int valueWidth = scaled(60);  // Tight value width like LFO section
        const int indicatorSize = scaled(6);

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
        delayLatencyValueLabel.setBounds(row.removeFromRight(scaled(130)));  // Wider for "Latency: 100.0 ms"
        delayLatencySlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing * 2);

        // Distance Attenuation
        row = leftCol.removeFromTop(rowHeight);
        distanceAttenLabel.setBounds(row.removeFromLeft(labelWidth));
        positionIndicatorForLabel(distanceAttenIndicator, distanceAttenLabel);
        distanceAttenValueLabel.setBounds(row.removeFromRight(valueWidth));
        distanceAttenSlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing * 4);  // Extra space before buttons

        // Enable buttons - all three on a single row, aligned with sliders above
        row = leftCol.removeFromTop(rowHeight);
        const int buttonSpacing = scaled(15);  // Spacing between buttons
        const int buttonWidth = (row.getWidth() - buttonSpacing * 2) / 3;
        minLatencyEnableButton.setBounds(row.removeFromLeft(buttonWidth));
        positionIndicatorForButton(minLatencyIndicator, minLatencyEnableButton);
        row.removeFromLeft(buttonSpacing);
        liveSourceEnableButton.setBounds(row.removeFromLeft(buttonWidth));
        positionIndicatorForButton(liveSourceIndicator, liveSourceEnableButton);
        row.removeFromLeft(buttonSpacing);
        floorReflectionsEnableButton.setBounds(row);  // Use remaining space to align with slider right edge
        positionIndicatorForButton(floorReflectionsIndicator, floorReflectionsEnableButton);

        // ==================== RIGHT COLUMN (Position & Directivity) ====================
        auto rightCol = area.reduced(10, 10);

        // Coordinate mode and position row - distribute evenly across full width
        row = rightCol.removeFromTop(rowHeight);
        const int coordLabelWidth = scaled(85);
        const int coordSelectorWidth = scaled(80);
        const int posLabelWidth = scaled(75);  // Fits "Position X:", "Azimuth:", "Elevation:"
        const int posEditorWidth = scaled(65);
        const int posUnitWidth = scaled(25);
        const int coordSpacing = scaled(15);  // Spacing between coordinate groups

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
        rightCol.removeFromTop(spacing * 6);  // Extra space before directivity group

        // Calculate heights for vertical centering of dial with slider group
        const int dialSize = juce::jmax(60, static_cast<int>(100.0f * layoutScale));
        const int dialMargin = scaled(40);
        const int sliderGroupHeight = 3 * (rowHeight + sliderHeight) + 2 * spacing;  // 3 directivity sliders (Angle On/Off, Pitch)
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
        directionalDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
        auto orientValueRow = dialColumn.removeFromTop(rowHeight);
        // Value and unit adjacent, centered as a pair under dial (with overlap to reduce font padding gap)
        const int orientValW = scaled(40), orientUnitW = scaled(30), overlap = scaled(7);
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
        rightCol.removeFromTop(spacing * 4);

        // HF Damping
        row = rightCol.removeFromTop(rowHeight);
        hfDampingLabel.setBounds(row.removeFromLeft(labelWidth));
        positionIndicatorForLabel(hfDampingIndicator, hfDampingLabel);
        hfDampingValueLabel.setBounds(row.removeFromRight(valueWidth));
        hfDampingSlider.setBounds(rightCol.removeFromTop(sliderHeight));
        rightCol.removeFromTop(spacing * 6);  // Extra space before parallax

        // Parallax editors (both on same row, V Parallax starts at center)
        row = rightCol.removeFromTop(rowHeight);
        const int parallaxEditorWidth = scaled(60);
        const int parallaxUnitWidth = scaled(20);
        const int labelToEditorGap = scaled(10);  // Gap between label and editor

        // Horizontal Parallax - left half
        auto hArea = row.removeFromLeft(row.getWidth() / 2);
        hParallaxLabel.setBounds(hArea.removeFromLeft(scaled(130)));
        positionIndicatorForLabel(hParallaxIndicator, hParallaxLabel);
        hArea.removeFromLeft(labelToEditorGap);
        hParallaxEditor.setBounds(hArea.removeFromLeft(parallaxEditorWidth));
        hArea.removeFromLeft(4);
        hParallaxUnitLabel.setBounds(hArea.removeFromLeft(parallaxUnitWidth));

        // Vertical Parallax - starts at center of column
        vParallaxLabel.setBounds(row.removeFromLeft(scaled(120)));
        positionIndicatorForLabel(vParallaxIndicator, vParallaxLabel);
        row.removeFromLeft(labelToEditorGap);
        vParallaxEditor.setBounds(row.removeFromLeft(parallaxEditorWidth));
        row.removeFromLeft(4);
        vParallaxUnitLabel.setBounds(row.removeFromLeft(parallaxUnitWidth));
    }

    void layoutEqTab()
    {
        auto area = subTabContentArea;
        const int buttonHeight = scaled(30);
        const int bandWidth = area.getWidth() / numEqBands;
        const int dialSize = juce::jmax(40, static_cast<int>(65.0f * layoutScale));
        const int sliderHeight = scaled(35);
        const int labelHeight = scaled(20);
        const int spacing = scaled(5);
        const int indicatorSize = scaled(6);
        const int toggleSize = scaled(18);

        // Top row: EQ Enable button (left) + Flatten EQ button (right)
        auto topRow = area.removeFromTop(buttonHeight);
        eqEnableButton.setBounds(topRow.removeFromLeft(scaled(100)));
        auto eqBtnBounds = eqEnableButton.getBounds();
        eqIndicator.setBounds(eqBtnBounds.getRight() - indicatorSize - 6,
                             eqBtnBounds.getY() + 4,
                             indicatorSize, indicatorSize);
        eqFlattenButton.setBounds(topRow.removeFromRight(scaled(100)));
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

            // Row 1: Band label
            eqBandLabel[i].setBounds(bandArea.removeFromTop(labelHeight));

            // Row 2: Toggle + Shape combobox + Reset button
            auto shapeRow = bandArea.removeFromTop(buttonHeight);
            eqBandToggle[i].setBounds(shapeRow.removeFromLeft(toggleSize).withSizeKeepingCentre(toggleSize, toggleSize));
            shapeRow.removeFromLeft(scaled(4));  // gap
            eqBandResetButton[i].setBounds(shapeRow.removeFromRight(scaled(50)));
            eqBandShapeSelector[i].setBounds(shapeRow);
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

        // Update help text to match coordinate mode
        juce::String n1 = label1.trimCharactersAtEnd(":"), n2 = label2.trimCharactersAtEnd(":"), n3 = label3.trimCharactersAtEnd(":");
        helpTextMap[&posXEditor] = LOC("outputs.help.position1").replace("{name}", n1).replace("{unit}", unit1);
        helpTextMap[&posYEditor] = LOC("outputs.help.position2").replace("{name}", n2).replace("{unit}", unit2);
        helpTextMap[&posZEditor] = LOC("outputs.help.position3").replace("{name}", n3).replace("{unit}", unit3);

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
        juce::String delayLabel = (delayMs < 0) ? LOC("outputs.labels.latency") : LOC("outputs.labels.delay");
        delayLatencyValueLabel.setText(delayLabel + " " + juce::String(std::abs(delayMs), 1) + " " + LOC("outputs.units.ms"), juce::dontSendNotification);

        bool minLatency = getIntParam("outputMiniLatencyEnable", 1) != 0;  // Default ON
        minLatencyEnableButton.setToggleState(minLatency, juce::dontSendNotification);
        minLatencyEnableButton.setButtonText(minLatency ? LOC("outputs.toggles.minLatencyOn") : LOC("outputs.toggles.minLatencyOff"));

        bool lsAtten = getIntParam("outputLSattenEnable", 1) != 0;  // Default ON
        liveSourceEnableButton.setToggleState(lsAtten, juce::dontSendNotification);
        liveSourceEnableButton.setButtonText(lsAtten ? LOC("outputs.toggles.liveSourceOn") : LOC("outputs.toggles.liveSourceOff"));

        bool frEnable = getIntParam("outputFRenable", 1) != 0;  // Default ON
        floorReflectionsEnableButton.setToggleState(frEnable, juce::dontSendNotification);
        floorReflectionsEnableButton.setButtonText(frEnable ? LOC("outputs.toggles.floorReflectionsOn") : LOC("outputs.toggles.floorReflectionsOff"));

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
        directionalDial.setOrientation(orientation);
        orientationValueLabel.setText(juce::String(static_cast<int>(orientation)), juce::dontSendNotification);

        int angleOn = getIntParam("outputAngleOn", 86);  // Default 86°
        angleOnSlider.setValue((angleOn - 1.0f) / 179.0f);
        angleOnValueLabel.setText(juce::String(angleOn) + juce::String::fromUTF8("°"), juce::dontSendNotification);
        directionalDial.setAngleOn(angleOn);

        int angleOff = getIntParam("outputAngleOff", 90);  // Default 90°
        angleOffSlider.setValue(angleOff / 179.0f);
        angleOffValueLabel.setText(juce::String(angleOff) + juce::String::fromUTF8("°"), juce::dontSendNotification);
        directionalDial.setAngleOff(angleOff);

        int pitch = getIntParam("outputPitch", 0);  // Default 0°
        pitchSlider.setValue(pitch / 90.0f);
        pitchValueLabel.setText(juce::String(pitch) + juce::String::fromUTF8("°"), juce::dontSendNotification);

        float hfDamping = getFloatParam("outputHFdamping", 0.0f);  // Default 0 dB/m
        hfDampingSlider.setValue((hfDamping + 6.0f) / 6.0f);
        hfDampingValueLabel.setText(juce::String(hfDamping, 1) + " dB/m", juce::dontSendNotification);

        // EQ
        bool eqEnabled = getIntParam("outputEQenabled", 1) != 0;  // Default ON
        eqEnableButton.setToggleState(eqEnabled, juce::dontSendNotification);
        eqEnableButton.setButtonText(eqEnabled ? LOC("eq.status.on") : LOC("eq.status.off"));

        // Load EQ band parameters
        auto eqTree = parameters.getValueTreeState().getOutputEQSection(channel - 1);
        if (eqTree.isValid())
        {
            for (int i = 0; i < numEqBands; ++i)
            {
                auto band = eqTree.getChild(i);
                if (!band.isValid()) continue;

                int shape = band.getProperty(WFSParameterIDs::eqShape, 0);
                bool bandOn = (shape != 0);
                eqBandToggle[i].setToggleState(bandOn, juce::dontSendNotification);

                // Combobox: only update when band is on (preserve user's selection when off)
                if (bandOn)
                    eqBandShapeSelector[i].setSelectedId(shape, juce::dontSendNotification);

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
                eqDisplay->setUndoManager(parameters.getUndoManagerForDomain(UndoDomain::Output));
                lastEqDisplayChannel = channel;

                // Set up callback for array propagation when interacting with the EQ graph
                eqDisplay->onParameterChanged = [this](int bandIndex, const juce::Identifier& paramId, const juce::var& value) {
                    if (!isLoadingParameters)
                        saveEqBandParam(bandIndex, paramId, value);
                };
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
            mapVisibilityButton.setButtonText(visible ? LOC("outputs.buttons.arrayVisible") : LOC("outputs.buttons.arrayHidden"));
        }
        else
        {
            auto val = parameters.getOutputParam(currentChannel - 1, "outputMapVisible");
            bool visible = val.isVoid() || static_cast<int>(val) != 0;
            mapVisibilityButton.setButtonText(visible ? LOC("outputs.buttons.speakerVisible") : LOC("outputs.buttons.speakerHidden"));
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
        if (isLoadingParameters) return;

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
            juce::String labelText = (ms < 0) ? LOC("outputs.labels.latency") : LOC("outputs.labels.delay");
            delayLatencyValueLabel.setText(labelText + " " + juce::String(std::abs(ms), 1) + " " + LOC("outputs.units.ms"), juce::dontSendNotification);
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
            // Orientation: -180 to 180 degrees (dial normalizes automatically)
            int degrees = static_cast<int>(value);
            while (degrees > 180) degrees -= 360;
            while (degrees < -179) degrees += 360;
            directionalDial.setOrientation(static_cast<float>(degrees));
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
        // EQ band labels
        else
        {
            for (int i = 0; i < numEqBands; ++i)
            {
                if (label == &eqBandFreqValueLabel[i])
                {
                    int freq = juce::jlimit(20, 20000, static_cast<int>(value));
                    float v = std::log10(freq / 20.0f) / 3.0f;
                    parameters.getValueTreeState().beginUndoTransaction ("Output EQ Freq Band " + juce::String(i + 1));
                    eqBandFreqSlider[i].setValue(juce::jlimit(0.0f, 1.0f, v));
                    eqBandFreqValueLabel[i].setText(formatFrequency(freq), juce::dontSendNotification);
                    break;
                }
                else if (label == &eqBandGainValueLabel[i])
                {
                    float gain = juce::jlimit(-24.0f, 24.0f, value);
                    float v = (gain + 24.0f) / 48.0f;
                    parameters.getValueTreeState().beginUndoTransaction ("Output EQ Gain Band " + juce::String(i + 1));
                    eqBandGainDial[i].setValue(juce::jlimit(0.0f, 1.0f, v));
                    eqBandGainValueLabel[i].setText(juce::String(gain, 1) + " dB", juce::dontSendNotification);
                    break;
                }
                else if (label == &eqBandQValueLabel[i])
                {
                    float q = juce::jlimit(0.1f, 10.0f, value);
                    float v = std::log((q - 0.1f) / 0.099f + 1.0f) / std::log(100.0f);
                    parameters.getValueTreeState().beginUndoTransaction ("Output EQ Q Band " + juce::String(i + 1));
                    eqBandQDial[i].setValue(juce::jlimit(0.0f, 1.0f, v));
                    eqBandQValueLabel[i].setText(juce::String(q, 2), juce::dontSendNotification);
                    break;
                }
            }
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
            showStatusMessage(LOC("outputs.messages.selectFolderFirst"));
            return;
        }
        if (fileManager.saveOutputConfig())
            showStatusMessage(LOC("outputs.messages.configSaved"));
        else
            showStatusMessage(LOC("outputs.messages.error").replace("{error}", fileManager.getLastError()));
    }

    void reloadOutputConfiguration()
    {
        auto& fileManager = parameters.getFileManager();
        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage(LOC("outputs.messages.selectFolderFirst"));
            return;
        }
        if (fileManager.loadOutputConfig())
        {
            loadChannelParameters(currentChannel);
            showStatusMessage(LOC("outputs.messages.configLoaded"));

            // Trigger DSP recalculation via callback to MainComponent
            if (onConfigReloaded)
                onConfigReloaded();
        }
        else
            showStatusMessage(LOC("outputs.messages.error").replace("{error}", fileManager.getLastError()));
    }

    void reloadOutputConfigBackup()
    {
        auto& fileManager = parameters.getFileManager();
        if (fileManager.loadOutputConfigBackup(0))
        {
            loadChannelParameters(currentChannel);
            showStatusMessage(LOC("outputs.messages.backupLoaded"));

            // Trigger DSP recalculation via callback to MainComponent
            if (onConfigReloaded)
                onConfigReloaded();
        }
        else
            showStatusMessage(LOC("outputs.messages.error").replace("{error}", fileManager.getLastError()));
    }

    void importOutputConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser>(LOC("outputs.dialogs.import"),
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
                    showStatusMessage(LOC("outputs.messages.configImported"));

                    // Trigger DSP recalculation via callback to MainComponent
                    if (onConfigReloaded)
                        onConfigReloaded();
                }
                else
                    showStatusMessage(LOC("outputs.messages.error").replace("{error}", fileManager.getLastError()));
            }
        });
    }

    void exportOutputConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser>(LOC("outputs.dialogs.export"),
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
                    showStatusMessage(LOC("outputs.messages.configExported"));
                else
                    showStatusMessage(LOC("outputs.messages.error").replace("{error}", fileManager.getLastError()));
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
        helpTextMap[&channelSelector] = LOC("outputs.help.channelSelector");
        helpTextMap[&nameEditor] = LOC("outputs.help.nameEditor");
        helpTextMap[&arraySelector] = LOC("outputs.help.arraySelector");
        helpTextMap[&applyToArraySelector] = LOC("outputs.help.applyToArray");
        helpTextMap[&attenuationSlider] = LOC("outputs.help.attenuation");
        helpTextMap[&delayLatencySlider] = LOC("outputs.help.delayLatency");
        helpTextMap[&minLatencyEnableButton] = LOC("outputs.help.minLatency");
        helpTextMap[&liveSourceEnableButton] = LOC("outputs.help.liveSource");
        helpTextMap[&floorReflectionsEnableButton] = LOC("outputs.help.floorReflections");
        helpTextMap[&distanceAttenSlider] = LOC("outputs.help.distanceAtten");
        helpTextMap[&hParallaxEditor] = LOC("outputs.help.hParallax");
        helpTextMap[&vParallaxEditor] = LOC("outputs.help.vParallax");
        helpTextMap[&coordModeSelector] = LOC("outputs.help.coordMode");
        // Position help text set dynamically in updatePositionLabelsAndValues()
        helpTextMap[&directionalDial] = LOC("outputs.help.directional");
        helpTextMap[&angleOnSlider] = LOC("outputs.help.angleOn");
        helpTextMap[&angleOffSlider] = LOC("outputs.help.angleOff");
        helpTextMap[&pitchSlider] = LOC("outputs.help.pitch");
        helpTextMap[&hfDampingSlider] = LOC("outputs.help.hfDamping");
        helpTextMap[&arrayPositionHelperButton] = LOC("outputs.help.wizardOfOutZ");
        helpTextMap[&mapVisibilityButton] = LOC("outputs.help.mapVisibility");
        // EQ controls
        helpTextMap[&eqEnableButton] = LOC("outputs.help.eqEnable");
        helpTextMap[&eqFlattenButton] = LOC("outputs.help.eqFlatten");
        for (int i = 0; i < numEqBands; ++i)
        {
            helpTextMap[&eqBandToggle[i]] = LOC("outputs.help.eqBandToggle").replace("{band}", juce::String(i + 1));
            helpTextMap[&eqBandShapeSelector[i]] = LOC("outputs.help.eqShape").replace("{band}", juce::String(i + 1));
            helpTextMap[&eqBandFreqSlider[i]] = LOC("outputs.help.eqFreq").replace("{band}", juce::String(i + 1));
            helpTextMap[&eqBandGainDial[i]] = LOC("outputs.help.eqGain").replace("{band}", juce::String(i + 1));
            helpTextMap[&eqBandQDial[i]] = LOC("outputs.help.eqQ").replace("{band}", juce::String(i + 1));
            helpTextMap[&eqBandResetButton[i]] = LOC("outputs.help.eqResetBand").replace("{band}", juce::String(i + 1));
        }
        helpTextMap[&storeButton] = LOC("outputs.help.storeConfig");
        helpTextMap[&reloadButton] = LOC("outputs.help.reloadConfig");
        helpTextMap[&reloadBackupButton] = LOC("outputs.help.reloadBackup");
        helpTextMap[&importButton] = LOC("outputs.help.importConfig");
        helpTextMap[&exportButton] = LOC("outputs.help.exportConfig");
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
        oscMethodMap[&directionalDial] = "/wfs/output/orientation <ID> <value>";
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
                const auto& helpText = helpTextMap[component];
                statusBar->setHelpText(helpText);
                if (oscMethodMap.find(component) != oscMethodMap.end())
                    statusBar->setOscMethod(oscMethodMap[component]);

                // TTS: Announce parameter name and current value for accessibility
                juce::String paramName = TTSManager::extractParameterName(helpText);
                juce::String currentValue = TTSManager::getComponentValue(component);
                TTSManager::getInstance().onComponentEnter(paramName, currentValue, helpText);
                return;
            }
            component = component->getParentComponent();
        }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (statusBar != nullptr)
            statusBar->clearText();

        // TTS: Cancel any pending announcements
        TTSManager::getInstance().onComponentExit();
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
    juce::ValueTree binauralTree;
    bool isLoadingParameters = false;
    StatusBar* statusBar = nullptr;
    std::map<juce::Component*, juce::String> helpTextMap;
    std::map<juce::Component*, juce::String> oscMethodMap;
    int currentChannel = 1;

    int headerHeight = 60;
    int footerHeight = 50;
    juce::Rectangle<int> subTabContentArea;
    float layoutScale = 1.0f;  // Proportional scaling factor (1.0 = 1080p reference)

    /** Scale a reference pixel value by layoutScale with a 65% minimum floor */
    int scaled(int ref) const { return juce::jmax(static_cast<int>(ref * 0.65f), static_cast<int>(ref * layoutScale)); }

    // Header components
    ChannelSelectorButton channelSelector { "Output" };
    juce::Label nameLabel;
    juce::TextEditor nameEditor;
    juce::Label arrayLabel;
    juce::ComboBox arraySelector;
    juce::Label applyToArrayLabel;
    juce::ComboBox applyToArraySelector;
    juce::TextButton mapVisibilityButton;
    juce::TextButton levelMeterButton;

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
    WfsDirectionalDial directionalDial;
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

    // Global EQ Enable + Flatten
    juce::TextButton eqEnableButton;
    LongPressButton eqFlattenButton;

    // 6 EQ Bands - each with Toggle, Shape, Frequency, Gain, Q, Reset
    juce::Label eqBandLabel[numEqBands];
    EQBandToggle eqBandToggle[numEqBands];
    juce::ComboBox eqBandShapeSelector[numEqBands];
    LongPressButton eqBandResetButton[numEqBands];
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
    LongPressButton storeButton;
    LongPressButton reloadButton;
    LongPressButton reloadBackupButton;
    LongPressButton importButton;
    LongPressButton exportButton;

    // Array Position Helper window
    std::unique_ptr<OutputArrayHelperWindow> arrayHelperWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputsTab)
};
