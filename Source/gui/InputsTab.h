#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Accessibility/TTSManager.h"
#include "ChannelSelector.h"
#include "ColorScheme.h"
#include "ColorUtilities.h"
#include "SliderUIComponents.h"
#include "DialUIComponents.h"
#include "StatusBar.h"
#include "WfsJoystickComponent.h"
#include "sliders/WfsAutoCenterSlider.h"
#include "InputVisualisationComponent.h"
#include "dials/WfsLFOIndicators.h"
#include "../DSP/AutomOtionProcessor.h"
#include "../Helpers/CoordinateConverter.h"
#include "SetAllInputsWindow.h"
#include "SnapshotScopeWindow.h"
#include "../Localization/LocalizationManager.h"

//==============================================================================
// Custom Transport Button - Play (right-pointing triangle)
class PlayButton : public juce::Button
{
public:
    PlayButton() : juce::Button("Play") {}

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);

        // Background - use theme colors
        if (shouldDrawButtonAsDown)
            g.setColour(ColorScheme::get().buttonPressed);
        else if (shouldDrawButtonAsHighlighted)
            g.setColour(ColorScheme::get().buttonHover);
        else
            g.setColour(ColorScheme::get().buttonNormal);

        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(ColorScheme::get().buttonBorder);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        // Draw play triangle
        auto iconBounds = bounds.reduced(10.0f);
        juce::Path triangle;
        triangle.addTriangle(
            iconBounds.getX(), iconBounds.getY(),
            iconBounds.getX(), iconBounds.getBottom(),
            iconBounds.getRight(), iconBounds.getCentreY());

        g.setColour(ColorScheme::get().textPrimary);
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

        // Background - use theme colors
        if (shouldDrawButtonAsDown)
            g.setColour(ColorScheme::get().buttonPressed);
        else if (shouldDrawButtonAsHighlighted)
            g.setColour(ColorScheme::get().buttonHover);
        else
            g.setColour(ColorScheme::get().buttonNormal);

        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(ColorScheme::get().buttonBorder);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        // Draw stop square
        auto iconBounds = bounds.reduced(10.0f);
        g.setColour(ColorScheme::get().textPrimary);
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

        // Background - use theme colors, toggle state affects color
        if (shouldDrawButtonAsDown || getToggleState())
            g.setColour(ColorScheme::get().buttonPressed);
        else if (shouldDrawButtonAsHighlighted)
            g.setColour(ColorScheme::get().buttonHover);
        else
            g.setColour(ColorScheme::get().buttonNormal);

        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(ColorScheme::get().buttonBorder);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        // Draw pause bars (two vertical rectangles)
        auto iconBounds = bounds.reduced(10.0f);
        float barWidth = iconBounds.getWidth() * 0.3f;
        float gap = iconBounds.getWidth() * 0.4f;

        g.setColour(ColorScheme::get().textPrimary);
        g.fillRect(iconBounds.getX(), iconBounds.getY(), barWidth, iconBounds.getHeight());
        g.fillRect(iconBounds.getX() + barWidth + gap, iconBounds.getY(), barWidth, iconBounds.getHeight());
    }
};

//==============================================================================
// Long-Press Button for "Set All Inputs" window
// Activates on release after holding for 2+ seconds. Cancels if pointer leaves button.
class SetAllInputsLongPressButton : public juce::TextButton,
                                     private juce::Timer
{
public:
    SetAllInputsLongPressButton()
    {
        setButtonText(LOC("inputs.buttons.setAllInputs"));
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isLeftButtonDown())
        {
            pressStartTime = juce::Time::getCurrentTime();
            isLongPressActive = true;
            thresholdReached = false;
            startTimer(50);  // Check every 50ms
        }
        TextButton::mouseDown(e);
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        stopTimer();

        // Trigger action only if threshold was reached and pointer is still over button
        if (thresholdReached && isLongPressActive && contains(e.getPosition()))
        {
            if (onLongPress)
                onLongPress();
        }

        isLongPressActive = false;
        thresholdReached = false;
        repaint();
        TextButton::mouseUp(e);
    }

    void mouseExit(const juce::MouseEvent& e) override
    {
        // Cancel long-press if pointer leaves button
        if (isLongPressActive)
        {
            stopTimer();
            isLongPressActive = false;
            thresholdReached = false;
            repaint();
        }
        TextButton::mouseExit(e);
    }

    std::function<void()> onLongPress;

    static constexpr int longPressDurationMs = 2000;  // 2 seconds

private:
    void timerCallback() override
    {
        if (isLongPressActive && !thresholdReached)
        {
            auto elapsed = (juce::Time::getCurrentTime() - pressStartTime).inMilliseconds();
            if (elapsed >= longPressDurationMs)
            {
                thresholdReached = true;
                stopTimer();
            }
        }
        repaint();  // Update progress indicator
    }

    void paintButton(juce::Graphics& g, bool shouldHighlight, bool shouldBeDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);

        // Background
        if (shouldBeDown)
            g.setColour(ColorScheme::get().buttonPressed);
        else if (shouldHighlight)
            g.setColour(ColorScheme::get().buttonHover);
        else
            g.setColour(ColorScheme::get().buttonNormal);

        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(ColorScheme::get().buttonBorder);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        // Progress indicator during long press (fills from left to right)
        if (isLongPressActive && !thresholdReached)
        {
            auto elapsed = (juce::Time::getCurrentTime() - pressStartTime).inMilliseconds();
            float progress = juce::jlimit(0.0f, 1.0f, static_cast<float>(elapsed) / static_cast<float>(longPressDurationMs));

            g.setColour(ColorScheme::get().accentBlue.withAlpha(0.5f));
            auto progressBounds = bounds;
            progressBounds = progressBounds.removeFromLeft(bounds.getWidth() * progress);
            g.fillRoundedRectangle(progressBounds, 4.0f);
        }

        // Show green when threshold reached (ready to release)
        if (thresholdReached && isLongPressActive)
        {
            g.setColour(ColorScheme::get().accentGreen.withAlpha(0.5f));
            g.fillRoundedRectangle(bounds, 4.0f);
        }

        // Text
        g.setColour(ColorScheme::get().textPrimary);
        g.setFont(juce::FontOptions(14.0f));
        g.drawText(getButtonText(), bounds, juce::Justification::centred);
    }

    juce::Time pressStartTime;
    bool isLongPressActive = false;
    bool thresholdReached = false;
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
                  private juce::ValueTree::Listener,
                  public ColorScheme::Manager::Listener
{
public:
    InputsTab(WfsParameters& params)
        : parameters(params),
          inputsTree(params.getInputTree()),
          configTree(params.getConfigTree()),
          ioTree(params.getConfigTree().getChildWithName(WFSParameterIDs::IO)),
          binauralTree(params.getValueTreeState().getBinauralState())
    {
        // Enable keyboard focus so we can receive focus back after text editing
        setWantsKeyboardFocus(true);

        // Add listener to inputs tree, config tree, IO tree, and binaural tree (for solo state changes)
        inputsTree.addListener(this);
        configTree.addListener(this);
        if (ioTree.isValid())
            ioTree.addListener(this);
        if (binauralTree.isValid())
            binauralTree.addListener(this);
        ColorScheme::Manager::getInstance().addListener(this);

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
        // Set color provider to match input marker colors from Map tab
        channelSelector.setChannelColorProvider([](int channelId) -> juce::Colour {
            return WfsColorUtilities::getInputColor(channelId);
        });
        // Set text color provider for readable text on light/dark backgrounds
        channelSelector.setTextColorProvider([](int channelId) -> juce::Colour {
            auto bgColor = WfsColorUtilities::getInputColor(channelId);
            return WfsColorUtilities::getContrastingTextColor(bgColor);
        });
        // Set name provider to show input names on selector tiles
        channelSelector.setChannelNameProvider([this](int channelId) -> juce::String {
            juce::String name = parameters.getInputParam(channelId - 1, "inputName").toString();
            return name.isEmpty() ? juce::String() : name;
        });
        addAndMakeVisible(channelSelector);

        // Input Name
        addAndMakeVisible(nameLabel);
        nameLabel.setText(LOC("inputs.labels.name"), juce::dontSendNotification);
        addAndMakeVisible(nameEditor);
        nameEditor.addListener(this);

        // Cluster selector
        addAndMakeVisible(clusterLabel);
        clusterLabel.setText(LOC("inputs.labels.cluster"), juce::dontSendNotification);
        addAndMakeVisible(clusterSelector);
        clusterSelector.addItem(LOC("inputs.clusters.single"), 1);
        for (int i = 1; i <= 10; ++i)
            clusterSelector.addItem(LOC("inputs.clusters.clusterPrefix") + " " + juce::String(i), i + 1);
        clusterSelector.setSelectedId(1, juce::dontSendNotification);
        clusterSelector.onChange = [this]() {
            int newCluster = clusterSelector.getSelectedId() - 1;
            int previousCluster = static_cast<int>(parameters.getInputParam(currentChannel - 1, "inputCluster"));

            if (newCluster > 0)
            {
                // Check tracking constraint asynchronously
                checkTrackingConstraintAsync(newCluster, previousCluster);
            }
            else
            {
                saveInputParam(WFSParameterIDs::inputCluster, newCluster);
            }
            // TTS: Announce selection change
            TTSManager::getInstance().announceValueChange("Cluster", clusterSelector.getText());
        };

        // Map lock button
        addAndMakeVisible(mapLockButton);
        mapLockButton.onClick = [this]() { toggleMapLock(); };

        // Map visibility button
        addAndMakeVisible(mapVisibilityButton);
        mapVisibilityButton.onClick = [this]() { toggleMapVisibility(); };

        // Level Meter button
        addAndMakeVisible(levelMeterButton);
        levelMeterButton.setButtonText(LOC("systemConfig.buttons.levelMeter"));
        levelMeterButton.onClick = [this]() {
            if (onLevelMeterWindowRequested)
                onLevelMeterWindowRequested();
        };

        // Clear Solo button
        addAndMakeVisible(clearSoloButton);
        clearSoloButton.setButtonText(LOC("systemConfig.buttons.clearSolo"));
        clearSoloButton.onClick = [this]() {
            parameters.getValueTreeState().clearAllSoloStates();
            updateClearSoloButtonState();
        };

        // Solo button for binaural monitoring
        addAndMakeVisible(soloButton);
        soloButton.setButtonText("Solo");
        soloButton.setClickingTogglesState(true);
        soloButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFFFFD700));  // Yellow when on
        soloButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);        // Black text when on
        soloButton.onClick = [this]() { toggleSolo(); };

        // Solo mode toggle button (Single/Multi)
        addAndMakeVisible(soloModeButton);
        updateSoloModeButtonText();
        soloModeButton.onClick = [this]() { toggleSoloMode(); };

        // Set All Inputs button (long-press to open)
        addAndMakeVisible(setAllInputsButton);
        setAllInputsButton.onLongPress = [this]() { openSetAllInputsWindow(); };

        // ==================== SUB-TABS ====================
        addAndMakeVisible(subTabBar);
        subTabBar.addTab(LOC("inputs.tabs.inputParams"), juce::Colour(0xFF2A2A2A), -1);
        subTabBar.addTab(LOC("inputs.tabs.liveSourceHackoustics"), juce::Colour(0xFF2A2A2A), -1);
        subTabBar.addTab(LOC("inputs.tabs.movements"), juce::Colour(0xFF2A2A2A), -1);
        subTabBar.addTab(LOC("inputs.tabs.visualisation"), juce::Colour(0xFF2A2A2A), -1);
        subTabBar.setMinimumTabScaleFactor(1.0);  // Prevent tab shrinking - maintain full text width
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
        setupVisualisationTab();
        setupMutesTab();

        // ==================== FOOTER - STORE/RELOAD BUTTONS ====================
        addAndMakeVisible(storeButton);
        storeButton.setButtonText(LOC("inputs.buttons.storeConfig"));
        storeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF8C3333));  // Reddish
        storeButton.onClick = [this]() { storeInputConfiguration(); };

        addAndMakeVisible(reloadButton);
        reloadButton.setButtonText(LOC("inputs.buttons.reloadConfig"));
        reloadButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF338C33));  // Greenish
        reloadButton.onClick = [this]() { reloadInputConfiguration(); };

        addAndMakeVisible(reloadBackupButton);
        reloadBackupButton.setButtonText(LOC("inputs.buttons.reloadBackup"));
        reloadBackupButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF266626));  // Darker green
        reloadBackupButton.onClick = [this]() { reloadInputConfigBackup(); };

        addAndMakeVisible(importButton);
        importButton.setButtonText(LOC("inputs.buttons.import"));
        importButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF338C33));  // Greenish
        importButton.onClick = [this]() { importInputConfiguration(); };

        addAndMakeVisible(exportButton);
        exportButton.setButtonText(LOC("inputs.buttons.export"));
        exportButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF8C3333));  // Reddish
        exportButton.onClick = [this]() { exportInputConfiguration(); };

        // Snapshot management
        addAndMakeVisible(storeSnapshotButton);
        storeSnapshotButton.setButtonText(LOC("inputs.buttons.storeSnapshot"));
        storeSnapshotButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF996633));  // Yellow-orange
        storeSnapshotButton.onClick = [this]() { storeNewSnapshot(); };

        addAndMakeVisible(snapshotSelector);
        snapshotSelector.addItem(LOC("inputs.snapshots.selectSnapshot"), 1);
        // Snapshots would be populated dynamically

        addAndMakeVisible(reloadSnapshotButton);
        reloadSnapshotButton.setButtonText(LOC("inputs.buttons.reloadSnapshot"));
        reloadSnapshotButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF669933));  // Yellow-green
        reloadSnapshotButton.onClick = [this]() { reloadSnapshot(); };

        addAndMakeVisible(updateSnapshotButton);
        updateSnapshotButton.setButtonText(LOC("inputs.buttons.updateSnapshot"));
        updateSnapshotButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF996633));  // Yellow-orange
        updateSnapshotButton.onClick = [this]() { updateSnapshot(); };

        addAndMakeVisible(editScopeButton);
        editScopeButton.setButtonText(LOC("inputs.buttons.editScope"));
        editScopeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF33668C));  // Light blue
        editScopeButton.onClick = [this]() { editSnapshotScope(); };

        addAndMakeVisible(deleteSnapshotButton);
        deleteSnapshotButton.setButtonText(LOC("inputs.buttons.deleteSnapshot"));
        deleteSnapshotButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF661A33));  // Burgundy
        deleteSnapshotButton.onClick = [this]() { deleteSnapshot(); };

        // Load initial channel parameters
        loadChannelParameters(1);
    }

    ~InputsTab() override
    {
        ColorScheme::Manager::getInstance().removeListener(this);
        inputsTree.removeListener(this);
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
        nameEditor.setColour(juce::TextEditor::textColourId, colors.textPrimary);
        nameEditor.setColour(juce::TextEditor::backgroundColourId, colors.surfaceCard);
        nameEditor.setColour(juce::TextEditor::outlineColourId, colors.buttonBorder);
        nameEditor.applyFontToAllText(nameEditor.getFont(), true);

        repaint();
    }

    /**
     * Callback when channel selection changes.
     * MainComponent can use this to notify OSCManager for REMOTE protocol.
     */
    std::function<void(int channelId)> onChannelSelected;

    /** Get the currently selected channel (1-based) */
    int getCurrentChannel() const { return currentChannel; }

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
        int numInputs = parameters.getNumInputChannels();
        if (numInputs > 0)
        {
            channelSelector.setNumChannels(numInputs);
            if (currentChannel > numInputs)
                currentChannel = 1;
        }

        loadChannelParameters(currentChannel);
    }

    /** Callback when input config is reloaded - for triggering DSP recalculation */
    std::function<void()> onConfigReloaded;

    /** Callback when Level Meter window is requested */
    std::function<void()> onLevelMeterWindowRequested;

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

    /** Get the currently selected input channel (0-indexed) */
    int getSelectedInputIndex() const { return channelSelector.getSelectedChannel() - 1; }

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

    /** Set cluster assignment for current input. 0=Single, 1-10=Cluster 1-10. */
    void setCluster(int cluster)
    {
        cluster = juce::jlimit(0, 10, cluster);
        clusterSelector.setSelectedId(cluster + 1, juce::sendNotification);
        if (statusBar != nullptr)
        {
            if (cluster == 0)
                statusBar->showTemporaryMessage("Input " + juce::String(currentChannel) + " set to Single", 2000);
            else
                statusBar->showTemporaryMessage("Input " + juce::String(currentChannel) + " assigned to Cluster " + juce::String(cluster), 2000);
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

        auto row1 = headerArea.removeFromTop(rowHeight);
        channelSelector.setBounds(row1.removeFromLeft(150));
        row1.removeFromLeft(spacing * 2);
        nameLabel.setBounds(row1.removeFromLeft(50));
        nameEditor.setBounds(row1.removeFromLeft(200));
        row1.removeFromLeft(spacing * 4);
        clusterLabel.setBounds(row1.removeFromLeft(60));
        clusterSelector.setBounds(row1.removeFromLeft(100));
        row1.removeFromLeft(spacing * 2);
        mapLockButton.setBounds(row1.removeFromLeft(120));
        row1.removeFromLeft(spacing);
        mapVisibilityButton.setBounds(row1.removeFromLeft(160));

        // Right-aligned buttons (from right to left)
        // Desired order left-to-right: [Solo] [Clear Solo] [Single/Multi] [Level Meters] [Set all Inputs...]
        setAllInputsButton.setBounds(row1.removeFromRight(130));
        row1.removeFromRight(spacing);
        levelMeterButton.setBounds(row1.removeFromRight(100));
        row1.removeFromRight(spacing);
        soloModeButton.setBounds(row1.removeFromRight(70));  // Single/Multi toggle
        row1.removeFromRight(spacing);
        clearSoloButton.setBounds(row1.removeFromRight(90));
        row1.removeFromRight(spacing);
        soloButton.setBounds(row1.removeFromRight(50));

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
        // Use full width for tab bar to fit longer tab names
        auto tabBarArea = bounds.removeFromTop(32);
        subTabBar.setBounds(tabBarArea);

        auto contentArea = bounds.reduced(padding, 0);
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

    /** Set the AutomOtion processor for controlling programmed movements */
    void setAutoMotionProcessor(AutomOtionProcessor* processor)
    {
        automOtionProcessor = processor;
    }

    /**
     * Configure the visualisation component with output and reverb counts.
     * Call this after system configuration is loaded.
     */
    void configureVisualisation(int numOutputs, int numReverbs)
    {
        visualisationComponent.configure(numOutputs, numReverbs, &parameters);
        visualisationComponent.setSelectedInput(currentChannel - 1);
    }

    /**
     * Refresh the visualisation array color indicators.
     * Call this when output array assignments change.
     */
    void refreshVisualisationArrayColors()
    {
        visualisationComponent.refreshArrayColors();
    }

    /**
     * Update the visualisation with current DSP matrix values.
     * Call this from a timer at ~50Hz.
     *
     * @param delaysMs Delay times array [input * numOutputs + output]
     * @param levels Level values array (linear 0-1)
     * @param hfDb HF attenuation array (dB, negative)
     * @param reverbDelaysMs Input→Reverb delay times [input * numReverbs + reverb]
     * @param reverbLevels Input→Reverb levels (linear 0-1)
     * @param reverbHfDb Input→Reverb HF attenuation (dB)
     */
    void updateVisualisation(const float* delaysMs, const float* levels, const float* hfDb,
                             const float* reverbDelaysMs, const float* reverbLevels, const float* reverbHfDb)
    {
        visualisationComponent.updateValues(delaysMs, levels, hfDb,
                                            reverbDelaysMs, reverbLevels, reverbHfDb);
    }

    /**
     * Update LFO indicator display for the selected input.
     * Called at 50Hz when LFO is active.
     * @param progress Ramp progress (0 to 1)
     * @param isActive Whether LFO is currently active
     * @param normalizedX Output X (-1 to +1)
     * @param normalizedY Output Y (-1 to +1)
     * @param normalizedZ Output Z (-1 to +1)
     */
    void updateLFOIndicators(float progress, bool isActive, float normalizedX, float normalizedY, float normalizedZ)
    {
        lfoProgressDial.setProgress(progress);
        lfoProgressDial.setActive(isActive);
        lfoOutputXSlider.setValue(normalizedX);
        lfoOutputYSlider.setValue(normalizedY);
        lfoOutputZSlider.setValue(normalizedZ);
    }

    /** Get a reference to the visualisation component for direct updates */
    InputVisualisationComponent& getVisualisationComponent() { return visualisationComponent; }

private:
    // ==================== CHANGE LISTENER ====================

    void changeListenerCallback(juce::ChangeBroadcaster*) override
    {
        layoutCurrentSubTab();
        repaint();

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

    void setupInputPropertiesTab()
    {
        // Attenuation slider (-92 to 0 dB)
        addAndMakeVisible(attenuationLabel);
        attenuationLabel.setText(LOC("inputs.labels.attenuation"), juce::dontSendNotification);

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
        attenuationValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(attenuationValueLabel);

        // Delay/Latency slider (-100 to 100 ms)
        addAndMakeVisible(delayLatencyLabel);
        delayLatencyLabel.setText(LOC("inputs.labels.delayLatency"), juce::dontSendNotification);

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
        delayLatencyValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(delayLatencyValueLabel);

        // Minimal Latency button
        addAndMakeVisible(minimalLatencyButton);
        minimalLatencyButton.setButtonText(LOC("inputs.toggles.acousticPrecedence"));
        minimalLatencyButton.setClickingTogglesState(true);
        minimalLatencyButton.onClick = [this]() {
            bool minLat = minimalLatencyButton.getToggleState();
            minimalLatencyButton.setButtonText(minLat ? LOC("inputs.toggles.minimalLatency") : LOC("inputs.toggles.acousticPrecedence"));
            saveInputParam(WFSParameterIDs::inputMinimalLatency, minLat ? 1 : 0);
        };
    }

    void setupPositionTab()
    {
        // Coordinate Mode selector
        addAndMakeVisible(coordModeLabel);
        coordModeLabel.setText(LOC("inputs.labels.coord"), juce::dontSendNotification);
        addAndMakeVisible(coordModeSelector);
        coordModeSelector.addItem(LOC("inputs.coordinates.xyz"), 1);
        coordModeSelector.addItem(juce::String(juce::CharPointer_UTF8("r \xce\xb8 Z")), 2);    // r θ Z
        coordModeSelector.addItem(juce::String(juce::CharPointer_UTF8("r \xce\xb8 \xcf\x86")), 3);  // r θ φ
        coordModeSelector.setSelectedId(1, juce::dontSendNotification);
        coordModeSelector.onChange = [this]() {
            int mode = coordModeSelector.getSelectedId() - 1;
            saveInputParam(WFSParameterIDs::inputCoordinateMode, mode);
            updatePositionLabelsAndValues();
            updateConstraintVisibility();
            resized();  // Trigger layout update for visibility changes
            // Snap to distance constraint if enabled in non-Cartesian mode
            if (mode != 0 && constraintDistanceButton.getToggleState())
                applyDistanceConstraintSnap();
            // TTS: Announce selection change
            TTSManager::getInstance().announceValueChange("Coordinate Mode", coordModeSelector.getText());
        };

        // Position X
        addAndMakeVisible(posXLabel);
        posXLabel.setText(LOC("inputs.labels.positionX"), juce::dontSendNotification);
        addAndMakeVisible(posXEditor);
        posXEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(posXEditor, true, true);
        addAndMakeVisible(posXUnitLabel);
        posXUnitLabel.setText(LOC("units.meters"), juce::dontSendNotification);

        // Position Y
        addAndMakeVisible(posYLabel);
        posYLabel.setText(LOC("inputs.labels.positionY"), juce::dontSendNotification);
        addAndMakeVisible(posYEditor);
        posYEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(posYEditor, true, true);
        addAndMakeVisible(posYUnitLabel);
        posYUnitLabel.setText(LOC("units.meters"), juce::dontSendNotification);

        // Position Z
        addAndMakeVisible(posZLabel);
        posZLabel.setText(LOC("inputs.labels.positionZ"), juce::dontSendNotification);
        addAndMakeVisible(posZEditor);
        posZEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(posZEditor, true, true);
        addAndMakeVisible(posZUnitLabel);
        posZUnitLabel.setText(LOC("units.meters"), juce::dontSendNotification);

        // Offset X
        addAndMakeVisible(offsetXLabel);
        offsetXLabel.setText(LOC("inputs.labels.offsetX"), juce::dontSendNotification);
        addAndMakeVisible(offsetXEditor);
        offsetXEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(offsetXEditor, true, true);
        addAndMakeVisible(offsetXUnitLabel);
        offsetXUnitLabel.setText(LOC("units.meters"), juce::dontSendNotification);

        // Offset Y
        addAndMakeVisible(offsetYLabel);
        offsetYLabel.setText(LOC("inputs.labels.offsetY"), juce::dontSendNotification);
        addAndMakeVisible(offsetYEditor);
        offsetYEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(offsetYEditor, true, true);
        addAndMakeVisible(offsetYUnitLabel);
        offsetYUnitLabel.setText(LOC("units.meters"), juce::dontSendNotification);

        // Offset Z
        addAndMakeVisible(offsetZLabel);
        offsetZLabel.setText(LOC("inputs.labels.offsetZ"), juce::dontSendNotification);
        addAndMakeVisible(offsetZEditor);
        offsetZEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(offsetZEditor, true, true);
        addAndMakeVisible(offsetZUnitLabel);
        offsetZUnitLabel.setText(LOC("units.meters"), juce::dontSendNotification);

        // Constraint buttons
        addAndMakeVisible(constraintXButton);
        constraintXButton.setButtonText(LOC("inputs.toggles.constraintXOn"));
        constraintXButton.setClickingTogglesState(true);
        constraintXButton.setToggleState(true, juce::dontSendNotification);
        constraintXButton.onClick = [this]() {
            bool enabled = constraintXButton.getToggleState();
            constraintXButton.setButtonText(enabled ? LOC("inputs.toggles.constraintXOn") : LOC("inputs.toggles.constraintXOff"));
            saveInputParam(WFSParameterIDs::inputConstraintX, enabled ? 1 : 0);
        };

        addAndMakeVisible(constraintYButton);
        constraintYButton.setButtonText(LOC("inputs.toggles.constraintYOn"));
        constraintYButton.setClickingTogglesState(true);
        constraintYButton.setToggleState(true, juce::dontSendNotification);
        constraintYButton.onClick = [this]() {
            bool enabled = constraintYButton.getToggleState();
            constraintYButton.setButtonText(enabled ? LOC("inputs.toggles.constraintYOn") : LOC("inputs.toggles.constraintYOff"));
            saveInputParam(WFSParameterIDs::inputConstraintY, enabled ? 1 : 0);
        };

        addAndMakeVisible(constraintZButton);
        constraintZButton.setButtonText(LOC("inputs.toggles.constraintZOn"));
        constraintZButton.setClickingTogglesState(true);
        constraintZButton.setToggleState(true, juce::dontSendNotification);
        constraintZButton.onClick = [this]() {
            bool enabled = constraintZButton.getToggleState();
            constraintZButton.setButtonText(enabled ? LOC("inputs.toggles.constraintZOn") : LOC("inputs.toggles.constraintZOff"));
            saveInputParam(WFSParameterIDs::inputConstraintZ, enabled ? 1 : 0);
        };

        // Distance constraint (for Cylindrical/Spherical modes)
        addAndMakeVisible(constraintDistanceButton);
        constraintDistanceButton.setButtonText(LOC("inputs.toggles.constraintROff"));
        constraintDistanceButton.setClickingTogglesState(true);
        constraintDistanceButton.setToggleState(false, juce::dontSendNotification);
        constraintDistanceButton.onClick = [this]() {
            bool enabled = constraintDistanceButton.getToggleState();
            constraintDistanceButton.setButtonText(enabled ? LOC("inputs.toggles.constraintROn") : LOC("inputs.toggles.constraintROff"));
            // Dim slider when constraint is off
            distanceRangeSlider.setEnabled(enabled);
            distanceMinEditor.setEnabled(enabled);
            distanceMaxEditor.setEnabled(enabled);
            saveInputParam(WFSParameterIDs::inputConstraintDistance, enabled ? 1 : 0);
            // Snap position to valid range when enabled
            if (enabled)
                applyDistanceConstraintSnap();
        };

        // Distance range slider
        addAndMakeVisible(distanceRangeSlider);
        distanceRangeSlider.setTrackColours(juce::Colour(0xFF1C1C1C), juce::Colour(0xFF00BCD4));
        distanceRangeSlider.onValuesChanged = [this](float minVal, float maxVal) {
            distanceMinEditor.setText(juce::String(minVal, 2), juce::dontSendNotification);
            distanceMaxEditor.setText(juce::String(maxVal, 2), juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputConstraintDistanceMin, minVal);
            saveInputParam(WFSParameterIDs::inputConstraintDistanceMax, maxVal);
            // Snap if constraints active
            if (constraintDistanceButton.getToggleState())
                applyDistanceConstraintSnap();
        };

        // Distance Min editor
        addAndMakeVisible(distanceMinLabel);
        distanceMinLabel.setText(LOC("inputs.labels.min"), juce::dontSendNotification);
        addAndMakeVisible(distanceMinEditor);
        distanceMinEditor.setText("0.00", juce::dontSendNotification);
        distanceMinEditor.setInputRestrictions(6, "0123456789.-");
        distanceMinEditor.onReturnKey = [this]() {
            float val = distanceMinEditor.getText().getFloatValue();
            val = juce::jlimit(0.0f, 50.0f, val);
            distanceRangeSlider.setValues(val, distanceRangeSlider.getThumb2Value());
            saveInputParam(WFSParameterIDs::inputConstraintDistanceMin, distanceRangeSlider.getMinValue());
            saveInputParam(WFSParameterIDs::inputConstraintDistanceMax, distanceRangeSlider.getMaxValue());
            if (constraintDistanceButton.getToggleState())
                applyDistanceConstraintSnap();
        };
        distanceMinEditor.onFocusLost = [this]() {
            float val = distanceMinEditor.getText().getFloatValue();
            val = juce::jlimit(0.0f, 50.0f, val);
            distanceRangeSlider.setValues(val, distanceRangeSlider.getThumb2Value());
            saveInputParam(WFSParameterIDs::inputConstraintDistanceMin, distanceRangeSlider.getMinValue());
            saveInputParam(WFSParameterIDs::inputConstraintDistanceMax, distanceRangeSlider.getMaxValue());
            if (constraintDistanceButton.getToggleState())
                applyDistanceConstraintSnap();
        };
        addAndMakeVisible(distanceMinUnitLabel);
        distanceMinUnitLabel.setText(LOC("units.meters"), juce::dontSendNotification);

        // Distance Max editor
        addAndMakeVisible(distanceMaxLabel);
        distanceMaxLabel.setText(LOC("inputs.labels.max"), juce::dontSendNotification);
        addAndMakeVisible(distanceMaxEditor);
        distanceMaxEditor.setText("50.00", juce::dontSendNotification);
        distanceMaxEditor.setInputRestrictions(6, "0123456789.-");
        distanceMaxEditor.onReturnKey = [this]() {
            float val = distanceMaxEditor.getText().getFloatValue();
            val = juce::jlimit(0.0f, 50.0f, val);
            distanceRangeSlider.setValues(distanceRangeSlider.getThumb1Value(), val);
            saveInputParam(WFSParameterIDs::inputConstraintDistanceMin, distanceRangeSlider.getMinValue());
            saveInputParam(WFSParameterIDs::inputConstraintDistanceMax, distanceRangeSlider.getMaxValue());
            if (constraintDistanceButton.getToggleState())
                applyDistanceConstraintSnap();
        };
        distanceMaxEditor.onFocusLost = [this]() {
            float val = distanceMaxEditor.getText().getFloatValue();
            val = juce::jlimit(0.0f, 50.0f, val);
            distanceRangeSlider.setValues(distanceRangeSlider.getThumb1Value(), val);
            saveInputParam(WFSParameterIDs::inputConstraintDistanceMin, distanceRangeSlider.getMinValue());
            saveInputParam(WFSParameterIDs::inputConstraintDistanceMax, distanceRangeSlider.getMaxValue());
            if (constraintDistanceButton.getToggleState())
                applyDistanceConstraintSnap();
        };
        addAndMakeVisible(distanceMaxUnitLabel);
        distanceMaxUnitLabel.setText(LOC("units.meters"), juce::dontSendNotification);

        // Initially hide distance controls (shown only in Cylindrical/Spherical modes)
        // Also disable slider/editors since constraint starts OFF
        constraintDistanceButton.setVisible(false);
        distanceRangeSlider.setVisible(false);
        distanceRangeSlider.setEnabled(false);
        distanceMinLabel.setVisible(false);
        distanceMinEditor.setVisible(false);
        distanceMinEditor.setEnabled(false);
        distanceMinUnitLabel.setVisible(false);
        distanceMaxLabel.setVisible(false);
        distanceMaxEditor.setVisible(false);
        distanceMaxEditor.setEnabled(false);
        distanceMaxUnitLabel.setVisible(false);

        // Flip buttons
        addAndMakeVisible(flipXButton);
        flipXButton.setButtonText(LOC("inputs.toggles.flipXOff"));
        flipXButton.setClickingTogglesState(true);
        flipXButton.onClick = [this]() {
            bool enabled = flipXButton.getToggleState();
            flipXButton.setButtonText(enabled ? LOC("inputs.toggles.flipXOn") : LOC("inputs.toggles.flipXOff"));
            saveInputParam(WFSParameterIDs::inputFlipX, enabled ? 1 : 0);
        };

        addAndMakeVisible(flipYButton);
        flipYButton.setButtonText(LOC("inputs.toggles.flipYOff"));
        flipYButton.setClickingTogglesState(true);
        flipYButton.onClick = [this]() {
            bool enabled = flipYButton.getToggleState();
            flipYButton.setButtonText(enabled ? LOC("inputs.toggles.flipYOn") : LOC("inputs.toggles.flipYOff"));
            saveInputParam(WFSParameterIDs::inputFlipY, enabled ? 1 : 0);
        };

        addAndMakeVisible(flipZButton);
        flipZButton.setButtonText(LOC("inputs.toggles.flipZOff"));
        flipZButton.setClickingTogglesState(true);
        flipZButton.onClick = [this]() {
            bool enabled = flipZButton.getToggleState();
            flipZButton.setButtonText(enabled ? LOC("inputs.toggles.flipZOn") : LOC("inputs.toggles.flipZOff"));
            saveInputParam(WFSParameterIDs::inputFlipZ, enabled ? 1 : 0);
        };

        // Tracking
        addAndMakeVisible(trackingActiveButton);
        trackingActiveButton.setButtonText(LOC("inputs.toggles.trackingOff"));
        trackingActiveButton.setClickingTogglesState(true);
        trackingActiveButton.onClick = [this]() {
            bool enabled = trackingActiveButton.getToggleState();
            if (enabled)
            {
                // Check if enabling tracking would conflict with another input in the same cluster
                checkLocalTrackingConstraintAsync();
            }
            else
            {
                trackingActiveButton.setButtonText(LOC("inputs.toggles.trackingOff"));
                saveInputParam(WFSParameterIDs::inputTrackingActive, 0);
            }
        };

        // Tracking ID selector (1-32)
        addAndMakeVisible(trackingIdLabel);
        trackingIdLabel.setText(LOC("inputs.labels.trackingId"), juce::dontSendNotification);
        addAndMakeVisible(trackingIdSelector);
        for (int i = 1; i <= 32; ++i)
            trackingIdSelector.addItem(juce::String(i), i);
        trackingIdSelector.setSelectedId(1, juce::dontSendNotification);
        trackingIdSelector.onChange = [this]() {
            saveInputParam(WFSParameterIDs::inputTrackingID, trackingIdSelector.getSelectedId());
            // TTS: Announce selection change
            TTSManager::getInstance().announceValueChange("Tracking ID", trackingIdSelector.getText());
        };

        // Tracking Smoothing dial (0-100%)
        addAndMakeVisible(trackingSmoothLabel);
        trackingSmoothLabel.setText(LOC("inputs.labels.trackingSmooth"), juce::dontSendNotification);
        trackingSmoothLabel.setJustificationType(juce::Justification::centred);
        trackingSmoothDial.setColours(juce::Colours::black, juce::Colour(0xFF00BCD4), juce::Colours::grey);
        trackingSmoothDial.setValue(1.0f);  // Default 100%
        trackingSmoothDial.onValueChanged = [this](float v) {
            int percent = static_cast<int>(v * 100.0f);
            trackingSmoothValueLabel.setText(juce::String(percent), juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputTrackingSmooth, percent);
        };
        addAndMakeVisible(trackingSmoothDial);
        addAndMakeVisible(trackingSmoothValueLabel);
        trackingSmoothValueLabel.setText("100", juce::dontSendNotification);
        trackingSmoothValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(trackingSmoothValueLabel);
        addAndMakeVisible(trackingSmoothUnitLabel);
        trackingSmoothUnitLabel.setText("%", juce::dontSendNotification);
        trackingSmoothUnitLabel.setJustificationType(juce::Justification::left);
        trackingSmoothUnitLabel.setMinimumHorizontalScale(1.0f);

        // Max Speed
        addAndMakeVisible(maxSpeedActiveButton);
        maxSpeedActiveButton.setButtonText(LOC("inputs.toggles.maxSpeedOff"));
        maxSpeedActiveButton.setClickingTogglesState(true);
        maxSpeedActiveButton.onClick = [this]() {
            bool enabled = maxSpeedActiveButton.getToggleState();
            maxSpeedActiveButton.setButtonText(enabled ? LOC("inputs.toggles.maxSpeedOn") : LOC("inputs.toggles.maxSpeedOff"));
            saveInputParam(WFSParameterIDs::inputMaxSpeedActive, enabled ? 1 : 0);
        };

        // Max Speed dial (0.01-20.0 m/s)
        addAndMakeVisible(maxSpeedLabel);
        maxSpeedLabel.setText(LOC("inputs.labels.maxSpeed"), juce::dontSendNotification);
        maxSpeedLabel.setJustificationType(juce::Justification::centred);
        maxSpeedDial.setColours(juce::Colours::black, juce::Colour(0xFFFF9800), juce::Colours::grey);
        maxSpeedDial.onValueChanged = [this](float v) {
            float speed = v * 19.99f + 0.01f;
            maxSpeedValueLabel.setText(juce::String(speed, 2), juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputMaxSpeed, speed);
        };
        addAndMakeVisible(maxSpeedDial);
        addAndMakeVisible(maxSpeedValueLabel);
        maxSpeedValueLabel.setText("1.00", juce::dontSendNotification);
        maxSpeedValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(maxSpeedValueLabel);
        addAndMakeVisible(maxSpeedUnitLabel);
        maxSpeedUnitLabel.setText(LOC("units.metersPerSecond"), juce::dontSendNotification);
        maxSpeedUnitLabel.setJustificationType(juce::Justification::left);
        maxSpeedUnitLabel.setMinimumHorizontalScale(1.0f);

        // Path Mode toggle (follows drawn path instead of straight line)
        addAndMakeVisible(pathModeButton);
        pathModeButton.setButtonText(LOC("inputs.toggles.pathModeOff"));
        pathModeButton.setClickingTogglesState(true);
        pathModeButton.onClick = [this]() {
            bool enabled = pathModeButton.getToggleState();
            pathModeButton.setButtonText(enabled ? LOC("inputs.toggles.pathModeOn") : LOC("inputs.toggles.pathModeOff"));
            saveInputParam(WFSParameterIDs::inputPathModeActive, enabled ? 1 : 0);
        };

        // Height Factor dial
        addAndMakeVisible(heightFactorLabel);
        heightFactorLabel.setText(LOC("inputs.labels.heightFactor"), juce::dontSendNotification);
        heightFactorLabel.setJustificationType(juce::Justification::centred);
        heightFactorDial.setColours(juce::Colours::black, juce::Colour(0xFF4CAF50), juce::Colours::grey);
        heightFactorDial.onValueChanged = [this](float v) {
            int percent = static_cast<int>(v * 100.0f);
            heightFactorValueLabel.setText(juce::String(percent), juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputHeightFactor, percent);
        };
        addAndMakeVisible(heightFactorDial);
        addAndMakeVisible(heightFactorValueLabel);
        heightFactorValueLabel.setText("0", juce::dontSendNotification);
        heightFactorValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(heightFactorValueLabel);
        addAndMakeVisible(heightFactorUnitLabel);
        heightFactorUnitLabel.setText("%", juce::dontSendNotification);
        heightFactorUnitLabel.setJustificationType(juce::Justification::left);
        heightFactorUnitLabel.setMinimumHorizontalScale(1.0f);

        // Position Joystick (for X/Y real-time control)
        addAndMakeVisible(positionJoystick);
        positionJoystick.setOuterColour(juce::Colour(0xFF3A3A3A));
        positionJoystick.setThumbColour(juce::Colour(0xFFFF9800));
        positionJoystick.setReportingIntervalHz(50.0);  // 50Hz = 20ms updates
        positionJoystick.setOnPositionChanged([this](float x, float y) {
            // Skip if joystick is centered - don't interfere with manual text editing
            if (x == 0.0f && y == 0.0f)
                return;

            // Scale: 2.5m/s max at 50Hz = 0.05m per update at max joystick position
            const float scale = 0.05f;
            float deltaX = x * scale;
            float deltaY = y * scale;

            // Check if tracking is active (global toggle, protocol not disabled, and local toggle)
            bool globalTrackingOn = (int)parameters.getConfigParam("trackingEnabled") != 0;
            bool protocolEnabled = (int)parameters.getConfigParam("trackingProtocol") != 0;
            bool localTracking = trackingActiveButton.getToggleState();
            bool useOffset = globalTrackingOn && protocolEnabled && localTracking;

            // Apply flip inversion when modifying position directly (not offset)
            if (!useOffset)
            {
                if (flipXButton.getToggleState()) deltaX = -deltaX;
                if (flipYButton.getToggleState()) deltaY = -deltaY;
            }

            // Check constraint states
            bool constrainX = constraintXButton.getToggleState();
            bool constrainY = constraintYButton.getToggleState();
            bool constrainDist = constraintDistanceButton.getToggleState();
            int coordMode = static_cast<int>(parameters.getInputParam(currentChannel - 1, "inputCoordinateMode"));
            bool useDistanceConstraint = (coordMode == 1 || coordMode == 2) && constrainDist;

            if (useOffset)
            {
                // Update Offset X/Y when tracking is fully active
                float currentOffsetX = offsetXEditor.getText().getFloatValue();
                float currentOffsetY = offsetYEditor.getText().getFloatValue();
                float newOffsetX = currentOffsetX + deltaX;
                float newOffsetY = currentOffsetY + deltaY;

                // Get base position for constraint calculation
                // IMPORTANT: Read Cartesian from storage, not from display editors
                // (In cylindrical/spherical modes, editors show radius/theta/phi, not X/Y/Z)
                float posX = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputPositionX"));
                float posY = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputPositionY"));
                float totalX = posX + newOffsetX;
                float totalY = posY + newOffsetY;

                if (useDistanceConstraint)
                {
                    // Apply distance constraint (circular/spherical bounds)
                    float minDist = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputConstraintDistanceMin"));
                    float maxDist = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputConstraintDistanceMax"));
                    float currentDist = std::sqrt(totalX * totalX + totalY * totalY);
                    if (currentDist < 0.0001f) currentDist = 0.0001f;
                    float targetDist = juce::jlimit(minDist, maxDist, currentDist);
                    if (!juce::approximatelyEqual(currentDist, targetDist))
                    {
                        float distScale = targetDist / currentDist;
                        totalX *= distScale;
                        totalY *= distScale;
                    }
                    newOffsetX = totalX - posX;
                    newOffsetY = totalY - posY;
                }
                else
                {
                    // Apply rectangular constraints if enabled
                    if (constrainX)
                    {
                        totalX = juce::jlimit(getStageMinX(), getStageMaxX(), totalX);
                        newOffsetX = totalX - posX;
                    }
                    if (constrainY)
                    {
                        totalY = juce::jlimit(getStageMinY(), getStageMaxY(), totalY);
                        newOffsetY = totalY - posY;
                    }
                }

                offsetXEditor.setText(juce::String(newOffsetX, 2), juce::dontSendNotification);
                offsetYEditor.setText(juce::String(newOffsetY, 2), juce::dontSendNotification);
                saveInputParam(WFSParameterIDs::inputOffsetX, newOffsetX);
                saveInputParam(WFSParameterIDs::inputOffsetY, newOffsetY);
            }
            else
            {
                // Update Position X/Y when tracking is disabled
                // IMPORTANT: Always read from storage (Cartesian), not from display editors
                // (In cylindrical/spherical modes, editors show radius/theta/phi, not X/Y/Z)
                float currentX = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputPositionX"));
                float currentY = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputPositionY"));
                float newX = currentX + deltaX;
                float newY = currentY + deltaY;

                if (useDistanceConstraint)
                {
                    // Apply distance constraint (circular/spherical bounds)
                    float minDist = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputConstraintDistanceMin"));
                    float maxDist = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputConstraintDistanceMax"));
                    float currentDist = std::sqrt(newX * newX + newY * newY);
                    if (currentDist < 0.0001f) currentDist = 0.0001f;
                    float targetDist = juce::jlimit(minDist, maxDist, currentDist);
                    if (!juce::approximatelyEqual(currentDist, targetDist))
                    {
                        float distScale = targetDist / currentDist;
                        newX *= distScale;
                        newY *= distScale;
                    }
                }
                else
                {
                    // Apply rectangular constraints if enabled
                    if (constrainX)
                        newX = juce::jlimit(getStageMinX(), getStageMaxX(), newX);
                    if (constrainY)
                        newY = juce::jlimit(getStageMinY(), getStageMaxY(), newY);
                }

                // Save Cartesian coordinates
                saveInputParam(WFSParameterIDs::inputPositionX, newX);
                saveInputParam(WFSParameterIDs::inputPositionY, newY);

                // Update display editors with proper coordinate conversion
                float z = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputPositionZ"));
                float v1, v2, v3;
                WFSCoordinates::cartesianToDisplay(static_cast<WFSCoordinates::Mode>(coordMode), newX, newY, z, v1, v2, v3);
                posXEditor.setText(juce::String(v1, 2), juce::dontSendNotification);
                posYEditor.setText(juce::String(coordMode == 0 ? v2 : v2, coordMode == 0 ? 2 : 1), juce::dontSendNotification);
            }
        });
        addAndMakeVisible(positionJoystickLabel);
        positionJoystickLabel.setText(LOC("inputs.labels.xyJoystick"), juce::dontSendNotification);
        positionJoystickLabel.setJustificationType(juce::Justification::centred);

        // Position Z Slider (vertical, auto-center with continuous polling like joystick)
        addAndMakeVisible(positionZSlider);
        positionZSlider.setTrackColours(juce::Colour(0xFF3A3A3A), juce::Colour(0xFF4CAF50));
        positionZSlider.setThumbColour(juce::Colours::white);
        positionZSlider.setReportingIntervalHz(50.0);  // 50Hz = 20ms updates (same as joystick)
        positionZSlider.onPositionPolled = [this](float v) {
            // Skip if slider is centered - don't interfere with manual text editing
            if (v == 0.0f)
                return;

            // Scale: 2.5m/s max at 50Hz = 0.05m per update at max slider position
            const float scale = 0.05f;
            float deltaZ = v * scale;

            // Check if tracking is active (global toggle, protocol not disabled, and local toggle)
            bool globalTrackingOn = (int)parameters.getConfigParam("trackingEnabled") != 0;
            bool protocolEnabled = (int)parameters.getConfigParam("trackingProtocol") != 0;
            bool localTracking = trackingActiveButton.getToggleState();
            bool useOffset = globalTrackingOn && protocolEnabled && localTracking;

            // Apply flip inversion when modifying position directly (not offset)
            if (!useOffset && flipZButton.getToggleState())
                deltaZ = -deltaZ;

            // Check constraint states
            bool constrainZ = constraintZButton.getToggleState();
            bool constrainDist = constraintDistanceButton.getToggleState();
            int coordMode = static_cast<int>(parameters.getInputParam(currentChannel - 1, "inputCoordinateMode"));
            bool useDistanceConstraint = (coordMode == 2) && constrainDist;  // Only spherical for Z

            if (useOffset)
            {
                // Update Offset Z when tracking is fully active
                float currentOffsetZ = offsetZEditor.getText().getFloatValue();
                float newOffsetZ = currentOffsetZ + deltaZ;

                if (useDistanceConstraint)
                {
                    // Spherical mode: Z affects total distance, so apply distance constraint
                    // IMPORTANT: Read Cartesian from storage, not from display editors
                    float posX = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputPositionX"));
                    float posY = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputPositionY"));
                    float posZ = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputPositionZ"));
                    float offsetX = offsetXEditor.getText().getFloatValue();
                    float offsetY = offsetYEditor.getText().getFloatValue();
                    float totalX = posX + offsetX;
                    float totalY = posY + offsetY;
                    float totalZ = posZ + newOffsetZ;

                    float minDist = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputConstraintDistanceMin"));
                    float maxDist = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputConstraintDistanceMax"));
                    float currentDist = std::sqrt(totalX * totalX + totalY * totalY + totalZ * totalZ);
                    if (currentDist < 0.0001f) currentDist = 0.0001f;
                    float targetDist = juce::jlimit(minDist, maxDist, currentDist);
                    if (!juce::approximatelyEqual(currentDist, targetDist))
                    {
                        float distScale = targetDist / currentDist;
                        totalX *= distScale;
                        totalY *= distScale;
                        totalZ *= distScale;
                        // Update all offsets
                        offsetXEditor.setText(juce::String(totalX - posX, 2), juce::dontSendNotification);
                        offsetYEditor.setText(juce::String(totalY - posY, 2), juce::dontSendNotification);
                        saveInputParam(WFSParameterIDs::inputOffsetX, totalX - posX);
                        saveInputParam(WFSParameterIDs::inputOffsetY, totalY - posY);
                        newOffsetZ = totalZ - posZ;
                    }
                }
                else if (constrainZ)
                {
                    // Rectangular Z constraint
                    float posZ = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputPositionZ"));
                    float totalZ = posZ + newOffsetZ;
                    totalZ = juce::jlimit(getStageMinZ(), getStageMaxZ(), totalZ);
                    newOffsetZ = totalZ - posZ;
                }

                offsetZEditor.setText(juce::String(newOffsetZ, 2), juce::dontSendNotification);
                saveInputParam(WFSParameterIDs::inputOffsetZ, newOffsetZ);
            }
            else
            {
                // Update Position Z when tracking is disabled
                // IMPORTANT: Read Cartesian from storage, not from display editors
                float currentZ = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputPositionZ"));
                float newZ = currentZ + deltaZ;

                if (useDistanceConstraint)
                {
                    // Spherical mode: Z affects total distance, so apply distance constraint
                    float posX = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputPositionX"));
                    float posY = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputPositionY"));

                    float minDist = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputConstraintDistanceMin"));
                    float maxDist = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputConstraintDistanceMax"));
                    float currentDist = std::sqrt(posX * posX + posY * posY + newZ * newZ);
                    if (currentDist < 0.0001f) currentDist = 0.0001f;
                    float targetDist = juce::jlimit(minDist, maxDist, currentDist);
                    if (!juce::approximatelyEqual(currentDist, targetDist))
                    {
                        float distScale = targetDist / currentDist;
                        float newX = posX * distScale;
                        float newY = posY * distScale;
                        newZ *= distScale;
                        // Save X and Y positions
                        saveInputParam(WFSParameterIDs::inputPositionX, newX);
                        saveInputParam(WFSParameterIDs::inputPositionY, newY);
                        // Update display editors with proper coordinate conversion
                        float v1, v2, v3;
                        WFSCoordinates::cartesianToDisplay(static_cast<WFSCoordinates::Mode>(coordMode), newX, newY, newZ, v1, v2, v3);
                        posXEditor.setText(juce::String(v1, 2), juce::dontSendNotification);
                        posYEditor.setText(juce::String(coordMode == 0 ? v2 : v2, coordMode == 0 ? 2 : 1), juce::dontSendNotification);
                    }
                }
                else if (constrainZ)
                {
                    // Rectangular Z constraint
                    newZ = juce::jlimit(getStageMinZ(), getStageMaxZ(), newZ);
                }

                // Save Z and update display
                saveInputParam(WFSParameterIDs::inputPositionZ, newZ);
                // For Z editor: in spherical mode it shows phi (elevation angle), in others it shows Z directly
                float posX = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputPositionX"));
                float posY = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputPositionY"));
                float v1, v2, v3;
                WFSCoordinates::cartesianToDisplay(static_cast<WFSCoordinates::Mode>(coordMode), posX, posY, newZ, v1, v2, v3);
                posZEditor.setText(juce::String(coordMode == 2 ? v3 : newZ, coordMode == 2 ? 1 : 2), juce::dontSendNotification);
            }
        };
        addAndMakeVisible(positionZSliderLabel);
        positionZSliderLabel.setText(LOC("inputs.labels.zSlider"), juce::dontSendNotification);
        positionZSliderLabel.setJustificationType(juce::Justification::centred);
    }

    void setupSoundTab()
    {
        // Attenuation Law label and button
        addAndMakeVisible(attenuationLawLabel);
        attenuationLawLabel.setText(LOC("inputs.labels.attenuationLaw"), juce::dontSendNotification);
        attenuationLawLabel.setJustificationType(juce::Justification::centred);

        addAndMakeVisible(attenuationLawButton);
        attenuationLawButton.setButtonText(LOC("inputs.toggles.attenuationLawLog"));
        attenuationLawButton.setClickingTogglesState(true);
        attenuationLawButton.onClick = [this]() {
            bool is1OverD = attenuationLawButton.getToggleState();
            attenuationLawButton.setButtonText(is1OverD ? "1/d" : "Log");
            // Show/hide Distance Atten vs Distance Ratio based on law (now in Input Parameters tab)
            distanceAttenLabel.setVisible(!is1OverD && subTabBar.getCurrentTabIndex() == 0);
            distanceAttenDial.setVisible(!is1OverD && subTabBar.getCurrentTabIndex() == 0);
            distanceAttenValueLabel.setVisible(!is1OverD && subTabBar.getCurrentTabIndex() == 0);
            distanceAttenUnitLabel.setVisible(!is1OverD && subTabBar.getCurrentTabIndex() == 0);
            distanceRatioLabel.setVisible(is1OverD && subTabBar.getCurrentTabIndex() == 0);
            distanceRatioDial.setVisible(is1OverD && subTabBar.getCurrentTabIndex() == 0);
            distanceRatioValueLabel.setVisible(is1OverD && subTabBar.getCurrentTabIndex() == 0);
            distanceRatioUnitLabel.setVisible(is1OverD && subTabBar.getCurrentTabIndex() == 0);
            saveInputParam(WFSParameterIDs::inputAttenuationLaw, is1OverD ? 1 : 0);
        };

        // Distance Attenuation dial (visible when attenuationLaw == Log)
        addAndMakeVisible(distanceAttenLabel);
        distanceAttenLabel.setText(LOC("inputs.labels.distanceAtten"), juce::dontSendNotification);
        distanceAttenLabel.setJustificationType(juce::Justification::centred);
        distanceAttenDial.setColours(juce::Colours::black, juce::Colour(0xFF9C27B0), juce::Colours::grey);
        distanceAttenDial.onValueChanged = [this](float v) {
            float dBm = (v * 6.0f) - 6.0f;
            distanceAttenValueLabel.setText(juce::String(dBm, 1), juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputDistanceAttenuation, dBm);
        };
        addAndMakeVisible(distanceAttenDial);
        addAndMakeVisible(distanceAttenValueLabel);
        distanceAttenValueLabel.setText("-0.7", juce::dontSendNotification);
        distanceAttenValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(distanceAttenValueLabel);
        addAndMakeVisible(distanceAttenUnitLabel);
        distanceAttenUnitLabel.setText(LOC("units.decibelPerMeter"), juce::dontSendNotification);
        distanceAttenUnitLabel.setJustificationType(juce::Justification::left);
        distanceAttenUnitLabel.setMinimumHorizontalScale(1.0f);

        // Distance Ratio dial (visible when attenuationLaw == 1/d)
        addAndMakeVisible(distanceRatioLabel);
        distanceRatioLabel.setText(LOC("inputs.labels.distanceRatio"), juce::dontSendNotification);
        distanceRatioLabel.setJustificationType(juce::Justification::centred);
        distanceRatioDial.setColours(juce::Colours::black, juce::Colour(0xFF9C27B0), juce::Colours::grey);
        distanceRatioDial.onValueChanged = [this](float v) {
            // Formula: pow(10.0,(x*2.0)-1.0) maps 0-1 to 0.1-10.0
            float ratio = std::pow(10.0f, (v * 2.0f) - 1.0f);
            distanceRatioValueLabel.setText(juce::String(ratio, 2), juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputDistanceRatio, ratio);
        };
        distanceRatioDial.setValue(0.5f);  // Default 1.0x
        addAndMakeVisible(distanceRatioDial);
        addAndMakeVisible(distanceRatioValueLabel);
        distanceRatioValueLabel.setText("1.00", juce::dontSendNotification);
        distanceRatioValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(distanceRatioValueLabel);
        addAndMakeVisible(distanceRatioUnitLabel);
        distanceRatioUnitLabel.setText("x", juce::dontSendNotification);
        distanceRatioUnitLabel.setJustificationType(juce::Justification::left);
        distanceRatioUnitLabel.setMinimumHorizontalScale(1.0f);
        // Initially hidden (Log is default)
        distanceRatioLabel.setVisible(false);
        distanceRatioDial.setVisible(false);
        distanceRatioValueLabel.setVisible(false);
        distanceRatioUnitLabel.setVisible(false);

        // Common Attenuation dial
        addAndMakeVisible(commonAttenLabel);
        commonAttenLabel.setText(LOC("inputs.labels.commonAtten"), juce::dontSendNotification);
        commonAttenLabel.setJustificationType(juce::Justification::centred);
        commonAttenDial.setColours(juce::Colours::black, juce::Colour(0xFF2196F3), juce::Colours::grey);
        commonAttenDial.setValue(1.0f);
        commonAttenDial.onValueChanged = [this](float v) {
            int percent = static_cast<int>(v * 100.0f);
            commonAttenValueLabel.setText(juce::String(percent), juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputCommonAtten, percent);
        };
        addAndMakeVisible(commonAttenDial);
        addAndMakeVisible(commonAttenValueLabel);
        commonAttenValueLabel.setText("100", juce::dontSendNotification);
        commonAttenValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(commonAttenValueLabel);
        addAndMakeVisible(commonAttenUnitLabel);
        commonAttenUnitLabel.setText("%", juce::dontSendNotification);
        commonAttenUnitLabel.setJustificationType(juce::Justification::left);
        commonAttenUnitLabel.setMinimumHorizontalScale(1.0f);

        // Directivity slider
        addAndMakeVisible(directivityLabel);
        directivityLabel.setText(LOC("inputs.labels.directivity"), juce::dontSendNotification);
        directivitySlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFF00BCD4));
        directivitySlider.setValue(1.0f);
        directivitySlider.onValueChanged = [this](float v) {
            int degrees = static_cast<int>((v * 358.0f) + 2.0f);
            directivityValueLabel.setText(juce::String(degrees) + juce::String::fromUTF8("°"), juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputDirectivity, degrees);
            inputDirectivityDial.setDirectivity(static_cast<float>(degrees));
        };
        addAndMakeVisible(directivitySlider);
        addAndMakeVisible(directivityValueLabel);
        directivityValueLabel.setText(juce::String::fromUTF8("360°"), juce::dontSendNotification);
        directivityValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(directivityValueLabel);

        // Rotation/Directivity combined dial
        addAndMakeVisible(rotationLabel);
        rotationLabel.setText(LOC("inputs.labels.rotation"), juce::dontSendNotification);
        rotationLabel.setJustificationType(juce::Justification::centred);
        inputDirectivityDial.onRotationChanged = [this](float angle) {
            rotationValueLabel.setText(juce::String(static_cast<int>(angle)), juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputRotation, static_cast<int>(angle));
        };
        addAndMakeVisible(inputDirectivityDial);
        addAndMakeVisible(rotationValueLabel);
        rotationValueLabel.setText("0", juce::dontSendNotification);
        rotationValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(rotationValueLabel);
        addAndMakeVisible(rotationUnitLabel);
        rotationUnitLabel.setText(juce::String::fromUTF8("°"), juce::dontSendNotification);
        rotationUnitLabel.setJustificationType(juce::Justification::left);
        rotationUnitLabel.setMinimumHorizontalScale(1.0f);

        // Tilt slider
        addAndMakeVisible(tiltLabel);
        tiltLabel.setText(LOC("inputs.labels.tilt"), juce::dontSendNotification);
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
        tiltValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(tiltValueLabel);

        // HF Shelf slider
        addAndMakeVisible(hfShelfLabel);
        hfShelfLabel.setText(LOC("inputs.labels.hfShelf"), juce::dontSendNotification);
        hfShelfSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFFFF9800));
        hfShelfSlider.onValueChanged = [this](float v) {
            float dB = 20.0f * std::log10(std::pow(10.0f, -24.0f / 20.0f) +
                       ((1.0f - std::pow(10.0f, -24.0f / 20.0f)) * v * v));
            hfShelfValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputHFshelf, dB);
            inputDirectivityDial.setHfShelf(dB);
        };
        addAndMakeVisible(hfShelfSlider);
        addAndMakeVisible(hfShelfValueLabel);
        hfShelfValueLabel.setText("-6.0 dB", juce::dontSendNotification);
        hfShelfValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(hfShelfValueLabel);
    }

    void setupLiveSourceTab()
    {
        // Live Source Active button
        addAndMakeVisible(lsActiveButton);
        lsActiveButton.setButtonText(LOC("inputs.toggles.liveSourceTamerOff"));
        lsActiveButton.setClickingTogglesState(true);
        lsActiveButton.onClick = [this]() {
            bool enabled = lsActiveButton.getToggleState();
            lsActiveButton.setButtonText(enabled ? LOC("inputs.toggles.liveSourceTamerOn") : LOC("inputs.toggles.liveSourceTamerOff"));
            setLiveSourceParametersAlpha(enabled ? 1.0f : 0.5f);
            saveInputParam(WFSParameterIDs::inputLSactive, enabled ? 1 : 0);
        };

        // Radius slider
        addAndMakeVisible(lsRadiusLabel);
        lsRadiusLabel.setText(LOC("inputs.labels.radius"), juce::dontSendNotification);
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
        lsRadiusValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(lsRadiusValueLabel);

        // Shape selector
        addAndMakeVisible(lsShapeLabel);
        lsShapeLabel.setText(LOC("inputs.labels.shape"), juce::dontSendNotification);
        addAndMakeVisible(lsShapeSelector);
        lsShapeSelector.addItem(LOC("inputs.liveSource.linear"), 1);
        lsShapeSelector.addItem(LOC("inputs.liveSource.log"), 2);
        lsShapeSelector.addItem(juce::CharPointer_UTF8("square d\xc2\xb2"), 3);
        lsShapeSelector.addItem(LOC("inputs.liveSource.sine"), 4);
        lsShapeSelector.setSelectedId(1, juce::dontSendNotification);
        lsShapeSelector.onChange = [this]() {
            saveInputParam(WFSParameterIDs::inputLSshape, lsShapeSelector.getSelectedId() - 1);
            // TTS: Announce selection change
            TTSManager::getInstance().announceValueChange("Live Source Shape", lsShapeSelector.getText());
        };

        // Attenuation slider
        addAndMakeVisible(lsAttenuationLabel);
        lsAttenuationLabel.setText(LOC("inputs.labels.attenuation"), juce::dontSendNotification);
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
        lsAttenuationValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(lsAttenuationValueLabel);

        // Peak Threshold slider
        addAndMakeVisible(lsPeakThresholdLabel);
        lsPeakThresholdLabel.setText(LOC("inputs.labels.peakThreshold"), juce::dontSendNotification);
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
        lsPeakThresholdValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(lsPeakThresholdValueLabel);

        // Peak Ratio dial
        addAndMakeVisible(lsPeakRatioLabel);
        lsPeakRatioLabel.setText(LOC("inputs.labels.peakRatio"), juce::dontSendNotification);
        lsPeakRatioLabel.setJustificationType(juce::Justification::centred);
        lsPeakRatioDial.setColours(juce::Colours::black, juce::Colour(0xFFE91E63), juce::Colours::grey);
        lsPeakRatioDial.onValueChanged = [this](float v) {
            float ratio = (v * 9.0f) + 1.0f;
            lsPeakRatioValueLabel.setText(juce::String(ratio, 1), juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputLSpeakRatio, ratio);
        };
        addAndMakeVisible(lsPeakRatioDial);
        addAndMakeVisible(lsPeakRatioUnitLabel);
        lsPeakRatioUnitLabel.setText("1:", juce::dontSendNotification);
        lsPeakRatioUnitLabel.setJustificationType(juce::Justification::right);
        lsPeakRatioUnitLabel.setMinimumHorizontalScale(1.0f);
        addAndMakeVisible(lsPeakRatioValueLabel);
        lsPeakRatioValueLabel.setText("2.0", juce::dontSendNotification);
        lsPeakRatioValueLabel.setJustificationType(juce::Justification::left);
        setupEditableValueLabel(lsPeakRatioValueLabel);

        // Slow Threshold slider
        addAndMakeVisible(lsSlowThresholdLabel);
        lsSlowThresholdLabel.setText(LOC("inputs.labels.slowThreshold"), juce::dontSendNotification);
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
        lsSlowThresholdValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(lsSlowThresholdValueLabel);

        // Slow Ratio dial
        addAndMakeVisible(lsSlowRatioLabel);
        lsSlowRatioLabel.setText(LOC("inputs.labels.slowRatio"), juce::dontSendNotification);
        lsSlowRatioLabel.setJustificationType(juce::Justification::centred);
        lsSlowRatioDial.setColours(juce::Colours::black, juce::Colour(0xFF9C27B0), juce::Colours::grey);
        lsSlowRatioDial.onValueChanged = [this](float v) {
            float ratio = (v * 9.0f) + 1.0f;
            lsSlowRatioValueLabel.setText(juce::String(ratio, 1), juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputLSslowRatio, ratio);
        };
        addAndMakeVisible(lsSlowRatioDial);
        addAndMakeVisible(lsSlowRatioUnitLabel);
        lsSlowRatioUnitLabel.setText("1:", juce::dontSendNotification);
        lsSlowRatioUnitLabel.setJustificationType(juce::Justification::right);
        lsSlowRatioUnitLabel.setMinimumHorizontalScale(1.0f);
        addAndMakeVisible(lsSlowRatioValueLabel);
        lsSlowRatioValueLabel.setText("2.0", juce::dontSendNotification);
        lsSlowRatioValueLabel.setJustificationType(juce::Justification::left);
        setupEditableValueLabel(lsSlowRatioValueLabel);
    }

    void setupEffectsTab()
    {
        // Floor Reflections Active
        addAndMakeVisible(frActiveButton);
        frActiveButton.setButtonText(LOC("inputs.toggles.floorReflectionsOff"));
        frActiveButton.setClickingTogglesState(true);
        frActiveButton.onClick = [this]() {
            bool enabled = frActiveButton.getToggleState();
            frActiveButton.setButtonText(enabled ? LOC("inputs.toggles.floorReflectionsOn") : LOC("inputs.toggles.floorReflectionsOff"));
            setFloorReflectionsParametersAlpha(enabled ? 1.0f : 0.5f);
            updateLowCutAlpha();
            updateHighShelfAlpha();
            saveInputParam(WFSParameterIDs::inputFRactive, enabled ? 1 : 0);
        };

        // Floor Reflections Attenuation slider
        addAndMakeVisible(frAttenuationLabel);
        frAttenuationLabel.setText(LOC("inputs.labels.attenuation"), juce::dontSendNotification);
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
        frAttenuationValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(frAttenuationValueLabel);

        // Floor Reflections Diffusion dial
        addAndMakeVisible(frDiffusionLabel);
        frDiffusionLabel.setText(LOC("inputs.labels.diffusion"), juce::dontSendNotification);
        frDiffusionLabel.setJustificationType(juce::Justification::centred);
        frDiffusionDial.setColours(juce::Colours::black, juce::Colour(0xFF795548), juce::Colours::grey);
        frDiffusionDial.setValue(0.2f);
        frDiffusionDial.onValueChanged = [this](float v) {
            int percent = static_cast<int>(v * 100.0f);
            frDiffusionValueLabel.setText(juce::String(percent), juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputFRdiffusion, percent);
        };
        addAndMakeVisible(frDiffusionDial);
        addAndMakeVisible(frDiffusionValueLabel);
        frDiffusionValueLabel.setText("20", juce::dontSendNotification);
        frDiffusionValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(frDiffusionValueLabel);
        addAndMakeVisible(frDiffusionUnitLabel);
        frDiffusionUnitLabel.setText("%", juce::dontSendNotification);
        frDiffusionUnitLabel.setJustificationType(juce::Justification::left);
        frDiffusionUnitLabel.setMinimumHorizontalScale(1.0f);

        // FR Low Cut Active
        addAndMakeVisible(frLowCutActiveButton);
        frLowCutActiveButton.setButtonText(LOC("inputs.toggles.lowCutOn"));
        frLowCutActiveButton.setClickingTogglesState(true);
        frLowCutActiveButton.setToggleState(true, juce::dontSendNotification);
        frLowCutActiveButton.onClick = [this]() {
            bool enabled = frLowCutActiveButton.getToggleState();
            frLowCutActiveButton.setButtonText(enabled ? LOC("inputs.toggles.lowCutOn") : LOC("inputs.toggles.lowCutOff"));
            updateLowCutAlpha();
            saveInputParam(WFSParameterIDs::inputFRlowCutActive, enabled ? 1 : 0);
        };

        // Low Cut Frequency slider (20-20000 Hz)
        addAndMakeVisible(frLowCutFreqLabel);
        frLowCutFreqLabel.setText(LOC("inputs.labels.frequency"), juce::dontSendNotification);
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
        frLowCutFreqValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(frLowCutFreqValueLabel);

        // FR High Shelf Active
        addAndMakeVisible(frHighShelfActiveButton);
        frHighShelfActiveButton.setButtonText(LOC("inputs.toggles.highShelfOn"));
        frHighShelfActiveButton.setClickingTogglesState(true);
        frHighShelfActiveButton.setToggleState(true, juce::dontSendNotification);
        frHighShelfActiveButton.onClick = [this]() {
            bool enabled = frHighShelfActiveButton.getToggleState();
            frHighShelfActiveButton.setButtonText(enabled ? LOC("inputs.toggles.highShelfOn") : LOC("inputs.toggles.highShelfOff"));
            updateHighShelfAlpha();
            saveInputParam(WFSParameterIDs::inputFRhighShelfActive, enabled ? 1 : 0);
        };

        // High Shelf Frequency slider (20-20000 Hz)
        addAndMakeVisible(frHighShelfFreqLabel);
        frHighShelfFreqLabel.setText(LOC("inputs.labels.frequency"), juce::dontSendNotification);
        frHighShelfFreqSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFF607D8B));
        frHighShelfFreqSlider.onValueChanged = [this](float v) {
            int freq = static_cast<int>(20.0f * std::pow(10.0f, 3.0f * v));
            frHighShelfFreqValueLabel.setText(juce::String(freq) + " Hz", juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputFRhighShelfFreq, freq);
        };
        addAndMakeVisible(frHighShelfFreqSlider);
        addAndMakeVisible(frHighShelfFreqValueLabel);
        frHighShelfFreqValueLabel.setText("3000 Hz", juce::dontSendNotification);
        frHighShelfFreqValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(frHighShelfFreqValueLabel);

        // High Shelf Gain slider (-24 to 0 dB)
        addAndMakeVisible(frHighShelfGainLabel);
        frHighShelfGainLabel.setText(LOC("inputs.labels.gain"), juce::dontSendNotification);
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
        frHighShelfGainValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(frHighShelfGainValueLabel);

        // High Shelf Slope slider (0.1-0.9)
        addAndMakeVisible(frHighShelfSlopeLabel);
        frHighShelfSlopeLabel.setText(LOC("inputs.labels.slope"), juce::dontSendNotification);
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
        frHighShelfSlopeValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(frHighShelfSlopeValueLabel);

        // Mute Sends to Reverbs
        addAndMakeVisible(muteReverbSendsButton);
        muteReverbSendsButton.setButtonText(LOC("inputs.toggles.reverbSendsUnmuted"));
        muteReverbSendsButton.setClickingTogglesState(true);
        muteReverbSendsButton.onClick = [this]() {
            bool muted = muteReverbSendsButton.getToggleState();
            muteReverbSendsButton.setButtonText(muted ? LOC("inputs.toggles.reverbSendsMuted") : LOC("inputs.toggles.reverbSendsUnmuted"));
            saveInputParam(WFSParameterIDs::inputMuteReverbSends, muted ? 1 : 0);
        };
    }

    void setupLfoTab()
    {
        // LFO Active button
        addAndMakeVisible(lfoActiveButton);
        lfoActiveButton.setButtonText(LOC("inputs.toggles.lfoOff"));
        lfoActiveButton.setClickingTogglesState(true);
        lfoActiveButton.onClick = [this]() {
            bool enabled = lfoActiveButton.getToggleState();
            lfoActiveButton.setButtonText(enabled ? LOC("inputs.toggles.lfoOn") : LOC("inputs.toggles.lfoOff"));
            saveInputParam(WFSParameterIDs::inputLFOactive, enabled ? 1 : 0);
            updateLfoAlpha();
        };

        // Period dial (0.01-100.0 s) - Formula: pow(10.0,sqrt(x)*4.0-2.0)
        addAndMakeVisible(lfoPeriodLabel);
        lfoPeriodLabel.setText(LOC("inputs.labels.period"), juce::dontSendNotification);
        lfoPeriodLabel.setJustificationType(juce::Justification::centred);
        lfoPeriodDial.setColours(juce::Colours::black, juce::Colour(0xFF00BCD4), juce::Colours::grey);
        lfoPeriodDial.onValueChanged = [this](float v) {
            float period = std::pow(10.0f, std::sqrt(v) * 4.0f - 2.0f);
            lfoPeriodValueLabel.setText(juce::String(period, 2), juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputLFOperiod, period);  // Save real period in seconds
        };
        addAndMakeVisible(lfoPeriodDial);
        addAndMakeVisible(lfoPeriodValueLabel);
        lfoPeriodValueLabel.setText("5.00", juce::dontSendNotification);
        lfoPeriodValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(lfoPeriodValueLabel);
        addAndMakeVisible(lfoPeriodUnitLabel);
        lfoPeriodUnitLabel.setText("s", juce::dontSendNotification);
        lfoPeriodUnitLabel.setJustificationType(juce::Justification::left);
        lfoPeriodUnitLabel.setMinimumHorizontalScale(1.0f);

        // Main Phase dial (-180° to 180°) - uses WfsRotationDial
        addAndMakeVisible(lfoPhaseLabel);
        lfoPhaseLabel.setText(LOC("inputs.labels.phase"), juce::dontSendNotification);
        lfoPhaseLabel.setJustificationType(juce::Justification::centred);
        lfoPhaseDial.setColours(juce::Colours::black, juce::Colour(0xFF4CAF50), juce::Colours::grey);
        lfoPhaseDial.onAngleChanged = [this](float angle) {
            int degrees = static_cast<int>(angle);
            lfoPhaseValueLabel.setText(juce::String(degrees), juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputLFOphase, degrees);  // Save as -180 to 180
        };
        addAndMakeVisible(lfoPhaseDial);
        addAndMakeVisible(lfoPhaseValueLabel);
        lfoPhaseValueLabel.setText("0", juce::dontSendNotification);
        lfoPhaseValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(lfoPhaseValueLabel);
        addAndMakeVisible(lfoPhaseUnitLabel);
        lfoPhaseUnitLabel.setText(juce::String::fromUTF8("°"), juce::dontSendNotification);
        lfoPhaseUnitLabel.setJustificationType(juce::Justification::left);
        lfoPhaseUnitLabel.setMinimumHorizontalScale(1.0f);

        // Shape X/Y/Z dropdowns - use localized shape names
        juce::StringArray lfoShapeKeys = {"off", "sine", "square", "sawtooth", "triangle", "keystone", "log", "exp", "random"};

        addAndMakeVisible(lfoShapeXLabel);
        lfoShapeXLabel.setText(LOC("inputs.labels.shapeX"), juce::dontSendNotification);
        addAndMakeVisible(lfoShapeXSelector);
        for (int i = 0; i < lfoShapeKeys.size(); ++i)
            lfoShapeXSelector.addItem(LOC("inputs.lfo.shapes." + lfoShapeKeys[i]), i + 1);
        lfoShapeXSelector.setSelectedId(1, juce::dontSendNotification);
        lfoShapeXSelector.onChange = [this]() {
            saveInputParam(WFSParameterIDs::inputLFOshapeX, lfoShapeXSelector.getSelectedId() - 1);
            updateLfoAlpha();
            // TTS: Announce selection change
            TTSManager::getInstance().announceValueChange("LFO Shape X", lfoShapeXSelector.getText());
        };

        addAndMakeVisible(lfoShapeYLabel);
        lfoShapeYLabel.setText(LOC("inputs.labels.shapeY"), juce::dontSendNotification);
        addAndMakeVisible(lfoShapeYSelector);
        for (int i = 0; i < lfoShapeKeys.size(); ++i)
            lfoShapeYSelector.addItem(LOC("inputs.lfo.shapes." + lfoShapeKeys[i]), i + 1);
        lfoShapeYSelector.setSelectedId(1, juce::dontSendNotification);
        lfoShapeYSelector.onChange = [this]() {
            saveInputParam(WFSParameterIDs::inputLFOshapeY, lfoShapeYSelector.getSelectedId() - 1);
            updateLfoAlpha();
            // TTS: Announce selection change
            TTSManager::getInstance().announceValueChange("LFO Shape Y", lfoShapeYSelector.getText());
        };

        addAndMakeVisible(lfoShapeZLabel);
        lfoShapeZLabel.setText(LOC("inputs.labels.shapeZ"), juce::dontSendNotification);
        addAndMakeVisible(lfoShapeZSelector);
        for (int i = 0; i < lfoShapeKeys.size(); ++i)
            lfoShapeZSelector.addItem(LOC("inputs.lfo.shapes." + lfoShapeKeys[i]), i + 1);
        lfoShapeZSelector.setSelectedId(1, juce::dontSendNotification);
        lfoShapeZSelector.onChange = [this]() {
            saveInputParam(WFSParameterIDs::inputLFOshapeZ, lfoShapeZSelector.getSelectedId() - 1);
            updateLfoAlpha();
            // TTS: Announce selection change
            TTSManager::getInstance().announceValueChange("LFO Shape Z", lfoShapeZSelector.getText());
        };

        // Rate X/Y/Z sliders (0.01-100, formula: pow(10.0,(x*4.0)-2.0))
        addAndMakeVisible(lfoRateXLabel);
        lfoRateXLabel.setText(LOC("inputs.labels.rateX"), juce::dontSendNotification);
        lfoRateXSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFFE91E63));
        lfoRateXSlider.onValueChanged = [this](float v) {
            float rate = std::pow(10.0f, (v * 4.0f) - 2.0f);
            lfoRateXValueLabel.setText(juce::String(rate, 2) + "x", juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputLFOrateX, rate);  // Save real rate multiplier
        };
        addAndMakeVisible(lfoRateXSlider);
        addAndMakeVisible(lfoRateXValueLabel);
        lfoRateXValueLabel.setText("1.00x", juce::dontSendNotification);
        lfoRateXValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(lfoRateXValueLabel);

        addAndMakeVisible(lfoRateYLabel);
        lfoRateYLabel.setText(LOC("inputs.labels.rateY"), juce::dontSendNotification);
        lfoRateYSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFFE91E63));
        lfoRateYSlider.onValueChanged = [this](float v) {
            float rate = std::pow(10.0f, (v * 4.0f) - 2.0f);
            lfoRateYValueLabel.setText(juce::String(rate, 2) + "x", juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputLFOrateY, rate);  // Save real rate multiplier
        };
        addAndMakeVisible(lfoRateYSlider);
        addAndMakeVisible(lfoRateYValueLabel);
        lfoRateYValueLabel.setText("1.00x", juce::dontSendNotification);
        lfoRateYValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(lfoRateYValueLabel);

        addAndMakeVisible(lfoRateZLabel);
        lfoRateZLabel.setText(LOC("inputs.labels.rateZ"), juce::dontSendNotification);
        lfoRateZSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFFE91E63));
        lfoRateZSlider.onValueChanged = [this](float v) {
            float rate = std::pow(10.0f, (v * 4.0f) - 2.0f);
            lfoRateZValueLabel.setText(juce::String(rate, 2) + "x", juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputLFOrateZ, rate);  // Save real rate multiplier
        };
        addAndMakeVisible(lfoRateZSlider);
        addAndMakeVisible(lfoRateZValueLabel);
        lfoRateZValueLabel.setText("1.00x", juce::dontSendNotification);
        lfoRateZValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(lfoRateZValueLabel);

        // Amplitude X/Y/Z sliders (0-50 m)
        addAndMakeVisible(lfoAmplitudeXLabel);
        lfoAmplitudeXLabel.setText(LOC("inputs.labels.amplitudeX"), juce::dontSendNotification);
        lfoAmplitudeXSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFF9C27B0));
        lfoAmplitudeXSlider.onValueChanged = [this](float v) {
            float amp = v * 50.0f;
            lfoAmplitudeXValueLabel.setText(juce::String(amp, 1) + " m", juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputLFOamplitudeX, amp);  // Save real meters
        };
        addAndMakeVisible(lfoAmplitudeXSlider);
        addAndMakeVisible(lfoAmplitudeXValueLabel);
        lfoAmplitudeXValueLabel.setText("1.0 m", juce::dontSendNotification);
        lfoAmplitudeXValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(lfoAmplitudeXValueLabel);

        addAndMakeVisible(lfoAmplitudeYLabel);
        lfoAmplitudeYLabel.setText(LOC("inputs.labels.amplitudeY"), juce::dontSendNotification);
        lfoAmplitudeYSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFF9C27B0));
        lfoAmplitudeYSlider.onValueChanged = [this](float v) {
            float amp = v * 50.0f;
            lfoAmplitudeYValueLabel.setText(juce::String(amp, 1) + " m", juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputLFOamplitudeY, amp);  // Save real meters
        };
        addAndMakeVisible(lfoAmplitudeYSlider);
        addAndMakeVisible(lfoAmplitudeYValueLabel);
        lfoAmplitudeYValueLabel.setText("1.0 m", juce::dontSendNotification);
        lfoAmplitudeYValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(lfoAmplitudeYValueLabel);

        addAndMakeVisible(lfoAmplitudeZLabel);
        lfoAmplitudeZLabel.setText(LOC("inputs.labels.amplitudeZ"), juce::dontSendNotification);
        lfoAmplitudeZSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFF9C27B0));
        lfoAmplitudeZSlider.onValueChanged = [this](float v) {
            float amp = v * 50.0f;
            lfoAmplitudeZValueLabel.setText(juce::String(amp, 1) + " m", juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputLFOamplitudeZ, amp);  // Save real meters
        };
        addAndMakeVisible(lfoAmplitudeZSlider);
        addAndMakeVisible(lfoAmplitudeZValueLabel);
        lfoAmplitudeZValueLabel.setText("1.0 m", juce::dontSendNotification);
        lfoAmplitudeZValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(lfoAmplitudeZValueLabel);

        // Phase X/Y/Z dials (-180° to 180°)
        addAndMakeVisible(lfoPhaseXLabel);
        lfoPhaseXLabel.setText(LOC("inputs.labels.phaseX"), juce::dontSendNotification);
        lfoPhaseXLabel.setJustificationType(juce::Justification::centred);
        lfoPhaseXDial.setColours(juce::Colours::black, juce::Colour(0xFFFF9800), juce::Colours::grey);
        lfoPhaseXDial.onAngleChanged = [this](float angle) {
            int degrees = static_cast<int>(angle);
            lfoPhaseXValueLabel.setText(juce::String(degrees), juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputLFOphaseX, degrees);  // Save as -180 to 180
        };
        addAndMakeVisible(lfoPhaseXDial);
        addAndMakeVisible(lfoPhaseXValueLabel);
        lfoPhaseXValueLabel.setText("0", juce::dontSendNotification);
        lfoPhaseXValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(lfoPhaseXValueLabel);
        addAndMakeVisible(lfoPhaseXUnitLabel);
        lfoPhaseXUnitLabel.setText(juce::String::fromUTF8("°"), juce::dontSendNotification);
        lfoPhaseXUnitLabel.setJustificationType(juce::Justification::left);
        lfoPhaseXUnitLabel.setMinimumHorizontalScale(1.0f);

        addAndMakeVisible(lfoPhaseYLabel);
        lfoPhaseYLabel.setText(LOC("inputs.labels.phaseY"), juce::dontSendNotification);
        lfoPhaseYLabel.setJustificationType(juce::Justification::centred);
        lfoPhaseYDial.setColours(juce::Colours::black, juce::Colour(0xFFFF9800), juce::Colours::grey);
        lfoPhaseYDial.onAngleChanged = [this](float angle) {
            int degrees = static_cast<int>(angle);
            lfoPhaseYValueLabel.setText(juce::String(degrees), juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputLFOphaseY, degrees);  // Save as -180 to 180
        };
        addAndMakeVisible(lfoPhaseYDial);
        addAndMakeVisible(lfoPhaseYValueLabel);
        lfoPhaseYValueLabel.setText("0", juce::dontSendNotification);
        lfoPhaseYValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(lfoPhaseYValueLabel);
        addAndMakeVisible(lfoPhaseYUnitLabel);
        lfoPhaseYUnitLabel.setText(juce::String::fromUTF8("°"), juce::dontSendNotification);
        lfoPhaseYUnitLabel.setJustificationType(juce::Justification::left);
        lfoPhaseYUnitLabel.setMinimumHorizontalScale(1.0f);

        addAndMakeVisible(lfoPhaseZLabel);
        lfoPhaseZLabel.setText(LOC("inputs.labels.phaseZ"), juce::dontSendNotification);
        lfoPhaseZLabel.setJustificationType(juce::Justification::centred);
        lfoPhaseZDial.setColours(juce::Colours::black, juce::Colour(0xFFFF9800), juce::Colours::grey);
        lfoPhaseZDial.onAngleChanged = [this](float angle) {
            int degrees = static_cast<int>(angle);
            lfoPhaseZValueLabel.setText(juce::String(degrees), juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputLFOphaseZ, degrees);  // Save as -180 to 180
        };
        addAndMakeVisible(lfoPhaseZDial);
        addAndMakeVisible(lfoPhaseZValueLabel);
        lfoPhaseZValueLabel.setText("0", juce::dontSendNotification);
        lfoPhaseZValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(lfoPhaseZValueLabel);
        addAndMakeVisible(lfoPhaseZUnitLabel);
        lfoPhaseZUnitLabel.setText(juce::String::fromUTF8("°"), juce::dontSendNotification);
        lfoPhaseZUnitLabel.setJustificationType(juce::Justification::left);
        lfoPhaseZUnitLabel.setMinimumHorizontalScale(1.0f);

        // Gyrophone dropdown
        addAndMakeVisible(lfoGyrophoneLabel);
        lfoGyrophoneLabel.setText(LOC("inputs.labels.gyrophone"), juce::dontSendNotification);
        addAndMakeVisible(lfoGyrophoneSelector);
        lfoGyrophoneSelector.addItem(LOC("inputs.lfo.gyrophone.antiClockwise"), 1);
        lfoGyrophoneSelector.addItem(LOC("inputs.lfo.gyrophone.off"), 2);
        lfoGyrophoneSelector.addItem(LOC("inputs.lfo.gyrophone.clockwise"), 3);
        lfoGyrophoneSelector.setSelectedId(2, juce::dontSendNotification);
        lfoGyrophoneSelector.onChange = [this]() {
            saveInputParam(WFSParameterIDs::inputLFOgyrophone, lfoGyrophoneSelector.getSelectedId() - 2);
            // TTS: Announce selection change
            TTSManager::getInstance().announceValueChange("Gyrophone", lfoGyrophoneSelector.getText());
        };

        // Jitter slider
        addAndMakeVisible(jitterLabel);
        jitterLabel.setText(LOC("inputs.labels.jitter"), juce::dontSendNotification);
        jitterSlider.setTrackColours(juce::Colour(0xFF1E1E1E), juce::Colour(0xFFCDDC39));
        jitterSlider.onValueChanged = [this](float v) {
            float meters = 10.0f * v * v;
            jitterValueLabel.setText(juce::String(meters, 2) + " m", juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputJitter, meters);  // Save real meters
        };
        addAndMakeVisible(jitterSlider);
        addAndMakeVisible(jitterValueLabel);
        jitterValueLabel.setText("0.00 m", juce::dontSendNotification);
        jitterValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(jitterValueLabel);

        // LFO Progress dial (read-only)
        addAndMakeVisible(lfoProgressDial);
        lfoProgressDial.setColours(juce::Colours::black, juce::Colour(0xFF00BCD4));

        // LFO Output sliders (read-only feedback)
        addAndMakeVisible(lfoOutputXLabel);
        lfoOutputXLabel.setText(LOC("inputs.labels.outX"), juce::dontSendNotification);
        lfoOutputXLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
        addAndMakeVisible(lfoOutputXSlider);
        lfoOutputXSlider.setTrackColour(juce::Colour(0xFFE91E63));  // Pink for X

        addAndMakeVisible(lfoOutputYLabel);
        lfoOutputYLabel.setText(LOC("inputs.labels.outY"), juce::dontSendNotification);
        lfoOutputYLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
        addAndMakeVisible(lfoOutputYSlider);
        lfoOutputYSlider.setTrackColour(juce::Colour(0xFF4CAF50));  // Green for Y

        addAndMakeVisible(lfoOutputZLabel);
        lfoOutputZLabel.setText(LOC("inputs.labels.outZ"), juce::dontSendNotification);
        lfoOutputZLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
        addAndMakeVisible(lfoOutputZSlider);
        lfoOutputZSlider.setTrackColour(juce::Colour(0xFF2196F3));  // Blue for Z
    }

    void setupAutomotionTab()
    {
        // AutomOtion title label
        addAndMakeVisible(otomoTitleLabel);
        otomoTitleLabel.setText("AutomOtion", juce::dontSendNotification);
        otomoTitleLabel.setFont(juce::FontOptions(16.0f).withStyle("Bold"));
        otomoTitleLabel.setJustificationType(juce::Justification::centredLeft);

        // Coordinate mode selector for destinations
        addAndMakeVisible(otomoCoordModeSelector);
        otomoCoordModeSelector.addItem(LOC("inputs.coordinates.xyz"), 1);  // Cartesian
        otomoCoordModeSelector.addItem(juce::String(juce::CharPointer_UTF8("r \xce\xb8 Z")), 2);    // Cylindrical: r θ Z
        otomoCoordModeSelector.addItem(juce::String(juce::CharPointer_UTF8("r \xce\xb8 \xcf\x86")), 3);  // Spherical: r θ φ
        otomoCoordModeSelector.setSelectedId(1, juce::dontSendNotification);
        otomoCoordModeSelector.onChange = [this]() {
            int mode = otomoCoordModeSelector.getSelectedId() - 1;  // 0, 1, or 2
            saveInputParam(WFSParameterIDs::inputOtomoCoordinateMode, mode);
            updateOtomoLabelsAndValues();
            updateOtomoDestinationEditors();
            updateOtomoCurveVisibility();
            resized();
        };

        // Destination X/Y/Z number boxes (using short labels)
        addAndMakeVisible(otomoDestXLabel);
        otomoDestXLabel.setText("X:", juce::dontSendNotification);
        addAndMakeVisible(otomoDestXEditor);
        otomoDestXEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(otomoDestXEditor, true, true);
        addAndMakeVisible(otomoDestXUnitLabel);
        otomoDestXUnitLabel.setText(LOC("units.meters"), juce::dontSendNotification);

        addAndMakeVisible(otomoDestYLabel);
        otomoDestYLabel.setText("Y:", juce::dontSendNotification);
        addAndMakeVisible(otomoDestYEditor);
        otomoDestYEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(otomoDestYEditor, true, true);
        addAndMakeVisible(otomoDestYUnitLabel);
        otomoDestYUnitLabel.setText(LOC("units.meters"), juce::dontSendNotification);

        addAndMakeVisible(otomoDestZLabel);
        otomoDestZLabel.setText("Z:", juce::dontSendNotification);
        addAndMakeVisible(otomoDestZEditor);
        otomoDestZEditor.setText("0.00", juce::dontSendNotification);
        setupNumericEditor(otomoDestZEditor, true, true);
        addAndMakeVisible(otomoDestZUnitLabel);
        otomoDestZUnitLabel.setText(LOC("units.meters"), juce::dontSendNotification);

        // Absolute/Relative button
        addAndMakeVisible(otomoAbsRelButton);
        otomoAbsRelButton.setButtonText(LOC("inputs.toggles.absolute"));
        otomoAbsRelButton.setClickingTogglesState(true);
        otomoAbsRelButton.onClick = [this]() {
            bool isRelative = otomoAbsRelButton.getToggleState();
            otomoAbsRelButton.setButtonText(isRelative ? LOC("inputs.toggles.relative") : LOC("inputs.toggles.absolute"));
            saveInputParam(WFSParameterIDs::inputOtomoAbsoluteRelative, isRelative ? 1 : 0);
        };

        // Stay/Return button
        addAndMakeVisible(otomoStayReturnButton);
        otomoStayReturnButton.setButtonText(LOC("inputs.toggles.stay"));
        otomoStayReturnButton.setClickingTogglesState(true);
        otomoStayReturnButton.onClick = [this]() {
            bool isReturn = otomoStayReturnButton.getToggleState();
            otomoStayReturnButton.setButtonText(isReturn ? LOC("inputs.toggles.return") : LOC("inputs.toggles.stay"));
            saveInputParam(WFSParameterIDs::inputOtomoStayReturn, isReturn ? 1 : 0);
        };

        // Duration dial (0.1 to 3600 seconds, logarithmic)
        addAndMakeVisible(otomoDurationLabel);
        otomoDurationLabel.setText(LOC("inputs.labels.duration"), juce::dontSendNotification);
        otomoDurationLabel.setJustificationType(juce::Justification::centred);
        otomoDurationDial.setColours(juce::Colours::black, juce::Colour(0xFF4CAF50), juce::Colours::grey);
        otomoDurationDial.onValueChanged = [this](float v) {
            // Logarithmic scale: 0.1s to 3600s
            // Formula: pow(10, sqrt(v) * 3.556 - 1) gives range ~0.1 to ~3600
            float duration = std::pow(10.0f, std::sqrt(v) * 3.556f - 1.0f);
            duration = juce::jlimit(0.1f, 3600.0f, duration);
            // Format display based on value
            juce::String displayText;
            if (duration < 10.0f)
                displayText = juce::String(duration, 2) + " s";
            else if (duration < 60.0f)
                displayText = juce::String(duration, 1) + " s";
            else if (duration < 3600.0f)
                displayText = juce::String(static_cast<int>(duration / 60)) + "m " + juce::String(static_cast<int>(duration) % 60) + "s";
            else
                displayText = "1h";
            otomoDurationValueLabel.setText(displayText, juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputOtomoDuration, duration);
        };
        addAndMakeVisible(otomoDurationDial);
        addAndMakeVisible(otomoDurationValueLabel);
        otomoDurationValueLabel.setText("5.00 s", juce::dontSendNotification);
        otomoDurationValueLabel.setJustificationType(juce::Justification::centred);
        setupEditableValueLabel(otomoDurationValueLabel);

        // Curve dial (-100 to +100, bipolar)
        addAndMakeVisible(otomoCurveLabel);
        otomoCurveLabel.setText(LOC("inputs.labels.curve"), juce::dontSendNotification);
        otomoCurveLabel.setJustificationType(juce::Justification::centred);
        otomoCurveDial.setColours(juce::Colours::black, juce::Colour(0xFFFF9800), juce::Colours::grey);
        otomoCurveDial.onValueChanged = [this](float v) {
            // Bipolar: -100 to +100
            int curve = static_cast<int>((v * 200.0f) - 100.0f);
            otomoCurveValueLabel.setText(juce::String(curve), juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputOtomoCurve, curve);
        };
        addAndMakeVisible(otomoCurveDial);
        addAndMakeVisible(otomoCurveValueLabel);
        otomoCurveValueLabel.setText("0", juce::dontSendNotification);
        otomoCurveValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(otomoCurveValueLabel);
        addAndMakeVisible(otomoCurveUnitLabel);
        otomoCurveUnitLabel.setText("%", juce::dontSendNotification);
        otomoCurveUnitLabel.setJustificationType(juce::Justification::left);
        otomoCurveUnitLabel.setMinimumHorizontalScale(1.0f);

        // Speed Profile dial (0-100%)
        addAndMakeVisible(otomoSpeedProfileLabel);
        otomoSpeedProfileLabel.setText(LOC("inputs.labels.speedProfile"), juce::dontSendNotification);
        otomoSpeedProfileLabel.setJustificationType(juce::Justification::centred);
        otomoSpeedProfileDial.setColours(juce::Colours::black, juce::Colour(0xFF2196F3), juce::Colours::grey);
        otomoSpeedProfileDial.onValueChanged = [this](float v) {
            int percent = static_cast<int>(v * 100.0f);
            otomoSpeedProfileValueLabel.setText(juce::String(percent), juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputOtomoSpeedProfile, percent);  // Save real percent
        };
        addAndMakeVisible(otomoSpeedProfileDial);
        addAndMakeVisible(otomoSpeedProfileValueLabel);
        otomoSpeedProfileValueLabel.setText("0", juce::dontSendNotification);
        otomoSpeedProfileValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(otomoSpeedProfileValueLabel);
        addAndMakeVisible(otomoSpeedProfileUnitLabel);
        otomoSpeedProfileUnitLabel.setText("%", juce::dontSendNotification);
        otomoSpeedProfileUnitLabel.setJustificationType(juce::Justification::left);
        otomoSpeedProfileUnitLabel.setMinimumHorizontalScale(1.0f);

        // Trigger button (Manual/Trigger)
        addAndMakeVisible(otomoTriggerButton);
        otomoTriggerButton.setButtonText(LOC("inputs.toggles.manual"));
        otomoTriggerButton.setClickingTogglesState(true);
        otomoTriggerButton.onClick = [this]() {
            bool isTrigger = otomoTriggerButton.getToggleState();
            otomoTriggerButton.setButtonText(isTrigger ? LOC("inputs.toggles.triggered") : LOC("inputs.toggles.manual"));
            saveInputParam(WFSParameterIDs::inputOtomoTrigger, isTrigger ? 1 : 0);
            updateOtomoTriggerAppearance();
        };

        // Trigger Threshold dial (-92 to 0 dB)
        addAndMakeVisible(otomoThresholdLabel);
        otomoThresholdLabel.setText(LOC("inputs.labels.threshold"), juce::dontSendNotification);
        otomoThresholdLabel.setJustificationType(juce::Justification::centred);
        otomoThresholdDial.setColours(juce::Colours::black, juce::Colour(0xFFE91E63), juce::Colours::grey);
        otomoThresholdDial.onValueChanged = [this](float v) {
            float dB = 20.0f * std::log10(std::pow(10.0f, -92.0f / 20.0f) +
                       ((1.0f - std::pow(10.0f, -92.0f / 20.0f)) * v * v));
            otomoThresholdValueLabel.setText(juce::String(dB, 1), juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputOtomoThreshold, dB);  // Save real dB value
        };
        addAndMakeVisible(otomoThresholdDial);
        addAndMakeVisible(otomoThresholdValueLabel);
        otomoThresholdValueLabel.setText("-20.0", juce::dontSendNotification);
        otomoThresholdValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(otomoThresholdValueLabel);
        addAndMakeVisible(otomoThresholdUnitLabel);
        otomoThresholdUnitLabel.setText("dB", juce::dontSendNotification);
        otomoThresholdUnitLabel.setJustificationType(juce::Justification::left);
        otomoThresholdUnitLabel.setMinimumHorizontalScale(1.0f);

        // Trigger Reset dial (-92 to 0 dB)
        addAndMakeVisible(otomoResetLabel);
        otomoResetLabel.setText(LOC("inputs.labels.reset"), juce::dontSendNotification);
        otomoResetLabel.setJustificationType(juce::Justification::centred);
        otomoResetDial.setColours(juce::Colours::black, juce::Colour(0xFF9C27B0), juce::Colours::grey);
        otomoResetDial.onValueChanged = [this](float v) {
            float dB = 20.0f * std::log10(std::pow(10.0f, -92.0f / 20.0f) +
                       ((1.0f - std::pow(10.0f, -92.0f / 20.0f)) * v * v));
            otomoResetValueLabel.setText(juce::String(dB, 1), juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputOtomoReset, dB);  // Save real dB value
        };
        addAndMakeVisible(otomoResetDial);
        addAndMakeVisible(otomoResetValueLabel);
        otomoResetValueLabel.setText("-60.0", juce::dontSendNotification);
        otomoResetValueLabel.setJustificationType(juce::Justification::right);
        setupEditableValueLabel(otomoResetValueLabel);
        addAndMakeVisible(otomoResetUnitLabel);
        otomoResetUnitLabel.setText("dB", juce::dontSendNotification);
        otomoResetUnitLabel.setJustificationType(juce::Justification::left);
        otomoResetUnitLabel.setMinimumHorizontalScale(1.0f);

        // Transport buttons (custom drawn icons)
        addAndMakeVisible(otomoStartButton);
        otomoStartButton.onClick = [this]() {
            if (automOtionProcessor != nullptr && currentChannel > 0)
                automOtionProcessor->startMotion(currentChannel - 1);
        };

        addAndMakeVisible(otomoStopButton);
        otomoStopButton.onClick = [this]() {
            if (automOtionProcessor != nullptr && currentChannel > 0)
                automOtionProcessor->stopMotion(currentChannel - 1);
        };

        addAndMakeVisible(otomoPauseButton);
        otomoPauseButton.setClickingTogglesState(true);
        otomoPauseButton.onClick = [this]() {
            if (automOtionProcessor != nullptr && currentChannel > 0)
            {
                bool isPaused = otomoPauseButton.getToggleState();
                if (isPaused)
                    automOtionProcessor->pauseMotion(currentChannel - 1);
                else
                    automOtionProcessor->resumeMotion(currentChannel - 1);
            }
            saveInputParam(WFSParameterIDs::inputOtomoPauseResume, otomoPauseButton.getToggleState() ? 0 : 1);
        };

        // Global controls
        addAndMakeVisible(otomoStopAllButton);
        otomoStopAllButton.setButtonText(LOC("inputs.buttons.stopAll"));
        otomoStopAllButton.onClick = [this]() {
            if (automOtionProcessor != nullptr)
                automOtionProcessor->stopAllMotion();
        };

        addAndMakeVisible(otomoPauseResumeAllButton);
        otomoPauseResumeAllButton.setButtonText(LOC("inputs.buttons.pauseAll"));
        otomoPauseResumeAllButton.setClickingTogglesState(true);
        otomoPauseResumeAllButton.onClick = [this]() {
            if (automOtionProcessor != nullptr)
            {
                if (otomoPauseResumeAllButton.getToggleState())
                {
                    automOtionProcessor->pauseAllMotion();
                    otomoPauseResumeAllButton.setButtonText(LOC("inputs.buttons.resumeAll"));
                }
                else
                {
                    automOtionProcessor->resumeAllMotion();
                    otomoPauseResumeAllButton.setButtonText(LOC("inputs.buttons.pauseAll"));
                }
            }
        };
    }

    void setupVisualisationTab()
    {
        addAndMakeVisible(visualisationComponent);
        // Configuration will be done when WFSCalculationEngine is connected
    }

    void setupMutesTab()
    {
        // Create 64 mute toggle buttons (8x8 grid)
        for (int i = 0; i < 64; ++i)
        {
            muteButtons[i].setButtonText(juce::String(i + 1));
            muteButtons[i].setClickingTogglesState(true);
            // Normal state uses theme color from WfsLookAndFeel, "on" state is orange for muted indication
            muteButtons[i].setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFFFF5722));
            muteButtons[i].onClick = [this, i]() {
                saveMuteStates();
            };
            addAndMakeVisible(muteButtons[i]);
        }

        // Mute Macros selector
        addAndMakeVisible(muteMacrosLabel);
        muteMacrosLabel.setText(LOC("inputs.labels.muteMacros"), juce::dontSendNotification);

        addAndMakeVisible(muteMacrosSelector);
        muteMacrosSelector.addItem(LOC("inputs.muteMacros.selectMacro"), 1);
        muteMacrosSelector.addItem(LOC("inputs.muteMacros.muteAll"), 2);
        muteMacrosSelector.addItem(LOC("inputs.muteMacros.unmuteAll"), 3);
        muteMacrosSelector.addItem(LOC("inputs.muteMacros.invertMutes"), 4);
        muteMacrosSelector.addItem(LOC("inputs.muteMacros.muteOdd"), 5);
        muteMacrosSelector.addItem(LOC("inputs.muteMacros.muteEven"), 6);
        for (int i = 1; i <= 10; ++i)
        {
            muteMacrosSelector.addItem(LOC("inputs.muteMacros.muteArrayPrefix") + " " + juce::String(i), 6 + (i * 2) - 1);
            muteMacrosSelector.addItem(LOC("inputs.muteMacros.unmuteArrayPrefix") + " " + juce::String(i), 6 + (i * 2));
        }
        muteMacrosSelector.setSelectedId(1, juce::dontSendNotification);
        muteMacrosSelector.onChange = [this]() {
            int macroId = muteMacrosSelector.getSelectedId();
            if (macroId > 1)
            {
                // TTS: Announce macro applied (before resetting selector)
                TTSManager::getInstance().announceValueChange("Mute Macro", muteMacrosSelector.getText() + " applied");
                applyMuteMacro(macroId);
                saveMuteStates();
                saveInputParam(WFSParameterIDs::inputMuteMacro, macroId);
            }
            muteMacrosSelector.setSelectedId(1, juce::dontSendNotification);
        };

        // Array Attenuation section
        addAndMakeVisible(arrayAttenLabel);
        arrayAttenLabel.setText(LOC("inputs.labels.arrayAttenuation"), juce::dontSendNotification);

        for (int i = 0; i < 10; ++i)
        {
            // Get array color
            juce::Colour arrayColor = WfsColorUtilities::getArrayColor(i + 1);

            // Dial label (Array 1, Array 2, etc.)
            arrayAttenDialLabels[i].setText(LOC("inputs.arrayPrefix") + " " + juce::String(i + 1), juce::dontSendNotification);
            arrayAttenDialLabels[i].setJustificationType(juce::Justification::centred);
            addAndMakeVisible(arrayAttenDialLabels[i]);

            // Dial with array color
            arrayAttenDials[i].setColours(juce::Colours::black, arrayColor, juce::Colours::grey);
            arrayAttenDials[i].onValueChanged = [this, i](float v) {
                // Convert dial value (0-1) to dB (-60 to 0) using sqrt scaling
                // Forward: linear = minLinear + v^2 * (1 - minLinear), then dB = 20*log10(linear)
                constexpr float arrayAttenMinLinear = 0.001f;  // -60 dB = 10^(-60/20) = 0.001
                float linear = arrayAttenMinLinear + v * v * (1.0f - arrayAttenMinLinear);
                float dB = 20.0f * std::log10(linear);
                arrayAttenValueLabels[i].setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
                // Get the parameter ID based on index
                const juce::Identifier* ids[] = {
                    &WFSParameterIDs::inputArrayAtten1, &WFSParameterIDs::inputArrayAtten2,
                    &WFSParameterIDs::inputArrayAtten3, &WFSParameterIDs::inputArrayAtten4,
                    &WFSParameterIDs::inputArrayAtten5, &WFSParameterIDs::inputArrayAtten6,
                    &WFSParameterIDs::inputArrayAtten7, &WFSParameterIDs::inputArrayAtten8,
                    &WFSParameterIDs::inputArrayAtten9, &WFSParameterIDs::inputArrayAtten10
                };
                saveInputParam(*ids[i], dB);
            };
            addAndMakeVisible(arrayAttenDials[i]);

            // Value label
            arrayAttenValueLabels[i].setText("0.0 dB", juce::dontSendNotification);
            arrayAttenValueLabels[i].setJustificationType(juce::Justification::centred);
            setupEditableValueLabel(arrayAttenValueLabels[i]);
            addAndMakeVisible(arrayAttenValueLabels[i]);
        }

        // Sidelines section (auto-mute at stage edges)
        addAndMakeVisible(sidelinesActiveButton);
        sidelinesActiveButton.setButtonText(LOC("inputs.toggles.sidelinesOff"));
        sidelinesActiveButton.setClickingTogglesState(true);
        sidelinesActiveButton.onClick = [this]() {
            bool active = sidelinesActiveButton.getToggleState();
            sidelinesActiveButton.setButtonText(active ? LOC("inputs.toggles.sidelinesOn") : LOC("inputs.toggles.sidelinesOff"));
            // Grey out dial when inactive (but keep it editable)
            sidelinesFringeDial.setAlpha(active ? 1.0f : 0.5f);
            sidelinesFringeLabel.setAlpha(active ? 1.0f : 0.5f);
            sidelinesFringeValueLabel.setAlpha(active ? 1.0f : 0.5f);
            saveInputParam(WFSParameterIDs::inputSidelinesActive, active ? 1 : 0);
        };

        addAndMakeVisible(sidelinesFringeLabel);
        sidelinesFringeLabel.setText(LOC("inputs.labels.fringe"), juce::dontSendNotification);
        sidelinesFringeLabel.setJustificationType(juce::Justification::centred);

        addAndMakeVisible(sidelinesFringeDial);
        sidelinesFringeDial.setColours(juce::Colours::black, juce::Colour(0xFF00C853), juce::Colours::grey);
        sidelinesFringeDial.onValueChanged = [this](float v) {
            // Map dial value (0-1) to meters (0.1-10.0) - linear
            float fringe = WFSParameterDefaults::inputSidelinesFringeMin +
                           v * (WFSParameterDefaults::inputSidelinesFringeMax - WFSParameterDefaults::inputSidelinesFringeMin);
            sidelinesFringeValueLabel.setText(juce::String(fringe, 2) + " m", juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputSidelinesFringe, fringe);
        };

        addAndMakeVisible(sidelinesFringeValueLabel);
        sidelinesFringeValueLabel.setText("1.00 m", juce::dontSendNotification);
        sidelinesFringeValueLabel.setJustificationType(juce::Justification::centred);
        setupEditableValueLabel(sidelinesFringeValueLabel);
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
                // Array mute/unmute macros (macroId >= 7)
                // IDs: Array 1 mute=7, unmute=8; Array 2 mute=9, unmute=10; etc.
                if (macroId >= 7)
                {
                    bool shouldMute = ((macroId - 7) % 2 == 0);
                    int arrayNumber = (macroId - 7) / 2 + 1;  // 1-10

                    int numOutputs = parameters.getNumOutputChannels();
                    if (numOutputs <= 0) numOutputs = 16;

                    for (int outIdx = 0; outIdx < numOutputs && outIdx < 64; ++outIdx)
                    {
                        int outputArray = static_cast<int>(parameters.getOutputParam(outIdx, "outputArray"));
                        if (outputArray == arrayNumber)
                            muteButtons[outIdx].setToggleState(shouldMute, juce::sendNotification);
                    }
                }
                break;
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
        // Keep existing justification (labels set centred should stay centred)
        label.addListener(this);
    }

    // Helper to layout dial value and unit labels under dial
    // Places value and unit adjacent, centered as a pair under the dial
    // Uses slight overlap to compensate for JUCE font padding
    void layoutDialValueUnit(juce::Label& valueLabel, juce::Label& unitLabel,
                             int dialCenterX, int y, int height,
                             int valueWidth = 40, int unitWidth = 40)
    {
        const int overlap = 7;  // Pixels to overlap to reduce visual gap from font padding
        int totalWidth = valueWidth + unitWidth - overlap;
        int startX = dialCenterX - totalWidth / 2;
        valueLabel.setBounds(startX, y, valueWidth, height);
        valueLabel.setJustificationType(juce::Justification::right);
        unitLabel.setBounds(startX + valueWidth - overlap, y, unitWidth, height);
        unitLabel.setJustificationType(juce::Justification::left);
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
        setVisualisationVisible(false);
        setMutesVisible(false);

        // Show current - new 4-tab structure
        if (tabIndex == 0)
        {
            // Input Parameters: Column 1 (Input+Position), Column 2 (Sound+Mutes)
            setInputPropertiesVisible(true);
            setPositionVisible(true);
            updateConstraintVisibility();  // Set constraint button visibility based on coord mode
            setSoundVisible(true);
            setMutesVisible(true);
            layoutInputParametersTab();
        }
        else if (tabIndex == 1)
        {
            // Live Source & Hackoustics: Column 1 (Live Source), Column 2 (Hackoustics)
            setLiveSourceVisible(true);
            setEffectsVisible(true);
            layoutLiveSourceHackousticsTab();
        }
        else if (tabIndex == 2)
        {
            // Movements: Column 1 (LFO), Column 2 (AutomOtion)
            setLfoVisible(true);
            setAutomotionVisible(true);
            layoutMovementsTab();
        }
        else if (tabIndex == 3)
        {
            // Visualisation - unchanged
            setVisualisationVisible(true);
            layoutVisualisationTab();
        }
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
        coordModeLabel.setVisible(v); coordModeSelector.setVisible(v);
        posXLabel.setVisible(v); posXEditor.setVisible(v); posXUnitLabel.setVisible(v);
        posYLabel.setVisible(v); posYEditor.setVisible(v); posYUnitLabel.setVisible(v);
        posZLabel.setVisible(v); posZEditor.setVisible(v); posZUnitLabel.setVisible(v);
        offsetXLabel.setVisible(v); offsetXEditor.setVisible(v); offsetXUnitLabel.setVisible(v);
        offsetYLabel.setVisible(v); offsetYEditor.setVisible(v); offsetYUnitLabel.setVisible(v);
        offsetZLabel.setVisible(v); offsetZEditor.setVisible(v); offsetZUnitLabel.setVisible(v);
        // Don't set constraint button visibility here - updateConstraintVisibility() handles it based on coord mode
        // Just set them invisible when hiding the whole section
        if (!v)
        {
            constraintXButton.setVisible(false);
            constraintYButton.setVisible(false);
            constraintZButton.setVisible(false);
            constraintDistanceButton.setVisible(false);
            distanceRangeSlider.setVisible(false);
            distanceMinLabel.setVisible(false);
            distanceMinEditor.setVisible(false);
            distanceMinUnitLabel.setVisible(false);
            distanceMaxLabel.setVisible(false);
            distanceMaxEditor.setVisible(false);
            distanceMaxUnitLabel.setVisible(false);
        }
        flipXButton.setVisible(v); flipYButton.setVisible(v); flipZButton.setVisible(v);
        trackingActiveButton.setVisible(v);
        trackingIdLabel.setVisible(v); trackingIdSelector.setVisible(v);
        trackingSmoothLabel.setVisible(v); trackingSmoothDial.setVisible(v); trackingSmoothValueLabel.setVisible(v); trackingSmoothUnitLabel.setVisible(v);
        maxSpeedActiveButton.setVisible(v);
        maxSpeedLabel.setVisible(v); maxSpeedDial.setVisible(v); maxSpeedValueLabel.setVisible(v); maxSpeedUnitLabel.setVisible(v);
        pathModeButton.setVisible(v);
        heightFactorLabel.setVisible(v); heightFactorDial.setVisible(v); heightFactorValueLabel.setVisible(v); heightFactorUnitLabel.setVisible(v);
        positionJoystick.setVisible(v); positionJoystickLabel.setVisible(v);
        positionZSlider.setVisible(v); positionZSliderLabel.setVisible(v);
    }

    void setSoundVisible(bool v)
    {
        attenuationLawLabel.setVisible(v);
        attenuationLawButton.setVisible(v);
        // Show Distance Atten or Distance Ratio based on attenuation law
        bool is1OverD = attenuationLawButton.getToggleState();
        distanceAttenLabel.setVisible(v && !is1OverD);
        distanceAttenDial.setVisible(v && !is1OverD);
        distanceAttenValueLabel.setVisible(v && !is1OverD);
        distanceAttenUnitLabel.setVisible(v && !is1OverD);
        distanceRatioLabel.setVisible(v && is1OverD);
        distanceRatioDial.setVisible(v && is1OverD);
        distanceRatioValueLabel.setVisible(v && is1OverD);
        distanceRatioUnitLabel.setVisible(v && is1OverD);
        commonAttenLabel.setVisible(v); commonAttenDial.setVisible(v); commonAttenValueLabel.setVisible(v); commonAttenUnitLabel.setVisible(v);
        directivityLabel.setVisible(v); directivitySlider.setVisible(v); directivityValueLabel.setVisible(v);
        rotationLabel.setVisible(v); inputDirectivityDial.setVisible(v); rotationValueLabel.setVisible(v); rotationUnitLabel.setVisible(v);
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
        lsPeakRatioLabel.setVisible(v); lsPeakRatioDial.setVisible(v); lsPeakRatioValueLabel.setVisible(v); lsPeakRatioUnitLabel.setVisible(v);
        lsSlowThresholdLabel.setVisible(v); lsSlowThresholdSlider.setVisible(v); lsSlowThresholdValueLabel.setVisible(v);
        lsSlowRatioLabel.setVisible(v); lsSlowRatioDial.setVisible(v); lsSlowRatioValueLabel.setVisible(v); lsSlowRatioUnitLabel.setVisible(v);
    }

    void setEffectsVisible(bool v)
    {
        frActiveButton.setVisible(v);
        frAttenuationLabel.setVisible(v); frAttenuationSlider.setVisible(v); frAttenuationValueLabel.setVisible(v);
        frDiffusionLabel.setVisible(v); frDiffusionDial.setVisible(v); frDiffusionValueLabel.setVisible(v); frDiffusionUnitLabel.setVisible(v);
        frLowCutActiveButton.setVisible(v);
        frLowCutFreqLabel.setVisible(v); frLowCutFreqSlider.setVisible(v); frLowCutFreqValueLabel.setVisible(v);
        frHighShelfActiveButton.setVisible(v);
        frHighShelfFreqLabel.setVisible(v); frHighShelfFreqSlider.setVisible(v); frHighShelfFreqValueLabel.setVisible(v);
        frHighShelfGainLabel.setVisible(v); frHighShelfGainSlider.setVisible(v); frHighShelfGainValueLabel.setVisible(v);
        frHighShelfSlopeLabel.setVisible(v); frHighShelfSlopeSlider.setVisible(v); frHighShelfSlopeValueLabel.setVisible(v);
        muteReverbSendsButton.setVisible(v);
    }

    void setLiveSourceParametersAlpha(float alpha)
    {
        // Dim all Live Source Tamer parameters (but keep them editable)
        lsRadiusLabel.setAlpha(alpha);
        lsRadiusSlider.setAlpha(alpha);
        lsRadiusValueLabel.setAlpha(alpha);
        lsShapeLabel.setAlpha(alpha);
        lsShapeSelector.setAlpha(alpha);
        lsAttenuationLabel.setAlpha(alpha);
        lsAttenuationSlider.setAlpha(alpha);
        lsAttenuationValueLabel.setAlpha(alpha);
        lsPeakThresholdLabel.setAlpha(alpha);
        lsPeakThresholdSlider.setAlpha(alpha);
        lsPeakThresholdValueLabel.setAlpha(alpha);
        lsPeakRatioLabel.setAlpha(alpha);
        lsPeakRatioDial.setAlpha(alpha);
        lsPeakRatioUnitLabel.setAlpha(alpha);
        lsPeakRatioValueLabel.setAlpha(alpha);
        lsSlowThresholdLabel.setAlpha(alpha);
        lsSlowThresholdSlider.setAlpha(alpha);
        lsSlowThresholdValueLabel.setAlpha(alpha);
        lsSlowRatioLabel.setAlpha(alpha);
        lsSlowRatioDial.setAlpha(alpha);
        lsSlowRatioUnitLabel.setAlpha(alpha);
        lsSlowRatioValueLabel.setAlpha(alpha);
    }

    void setFloorReflectionsParametersAlpha(float alpha)
    {
        // Dim all Floor Reflections parameters (but keep them editable)
        frAttenuationLabel.setAlpha(alpha);
        frAttenuationSlider.setAlpha(alpha);
        frAttenuationValueLabel.setAlpha(alpha);
        frDiffusionLabel.setAlpha(alpha);
        frDiffusionDial.setAlpha(alpha);
        frDiffusionValueLabel.setAlpha(alpha);
        frDiffusionUnitLabel.setAlpha(alpha);
        frLowCutActiveButton.setAlpha(alpha);
        frLowCutFreqLabel.setAlpha(alpha);
        frLowCutFreqSlider.setAlpha(alpha);
        frLowCutFreqValueLabel.setAlpha(alpha);
        frHighShelfActiveButton.setAlpha(alpha);
        frHighShelfFreqLabel.setAlpha(alpha);
        frHighShelfFreqSlider.setAlpha(alpha);
        frHighShelfFreqValueLabel.setAlpha(alpha);
        frHighShelfGainLabel.setAlpha(alpha);
        frHighShelfGainSlider.setAlpha(alpha);
        frHighShelfGainValueLabel.setAlpha(alpha);
        frHighShelfSlopeLabel.setAlpha(alpha);
        frHighShelfSlopeSlider.setAlpha(alpha);
        frHighShelfSlopeValueLabel.setAlpha(alpha);
    }

    void setLowCutParametersAlpha(float alpha)
    {
        // Dim Low Cut frequency slider (respects both FR master and Low Cut toggle)
        frLowCutFreqLabel.setAlpha(alpha);
        frLowCutFreqSlider.setAlpha(alpha);
        frLowCutFreqValueLabel.setAlpha(alpha);
    }

    void setHighShelfParametersAlpha(float alpha)
    {
        // Dim High Shelf parameters (respects both FR master and High Shelf toggle)
        frHighShelfFreqLabel.setAlpha(alpha);
        frHighShelfFreqSlider.setAlpha(alpha);
        frHighShelfFreqValueLabel.setAlpha(alpha);
        frHighShelfGainLabel.setAlpha(alpha);
        frHighShelfGainSlider.setAlpha(alpha);
        frHighShelfGainValueLabel.setAlpha(alpha);
        frHighShelfSlopeLabel.setAlpha(alpha);
        frHighShelfSlopeSlider.setAlpha(alpha);
        frHighShelfSlopeValueLabel.setAlpha(alpha);
    }

    void updateLowCutAlpha()
    {
        // Low Cut params are full opacity only when BOTH FR and Low Cut are enabled
        bool frEnabled = frActiveButton.getToggleState();
        bool lowCutEnabled = frLowCutActiveButton.getToggleState();
        float alpha = (frEnabled && lowCutEnabled) ? 1.0f : 0.5f;
        setLowCutParametersAlpha(alpha);
    }

    void updateHighShelfAlpha()
    {
        // High Shelf params are full opacity only when BOTH FR and High Shelf are enabled
        bool frEnabled = frActiveButton.getToggleState();
        bool highShelfEnabled = frHighShelfActiveButton.getToggleState();
        float alpha = (frEnabled && highShelfEnabled) ? 1.0f : 0.5f;
        setHighShelfParametersAlpha(alpha);
    }

    void setLfoParametersAlpha(float alpha)
    {
        // Dim all LFO parameters except the main active button
        // Note: Jitter is independent from LFO and is NOT dimmed here
        lfoPeriodLabel.setAlpha(alpha);
        lfoPeriodDial.setAlpha(alpha);
        lfoPeriodValueLabel.setAlpha(alpha);
        lfoPeriodUnitLabel.setAlpha(alpha);
        lfoPhaseLabel.setAlpha(alpha);
        lfoPhaseDial.setAlpha(alpha);
        lfoPhaseValueLabel.setAlpha(alpha);
        lfoPhaseUnitLabel.setAlpha(alpha);
        lfoProgressDial.setAlpha(alpha);
        lfoGyrophoneLabel.setAlpha(alpha);
        lfoGyrophoneSelector.setAlpha(alpha);
    }

    void setLfoAxisXAlpha(float alpha)
    {
        lfoShapeXLabel.setAlpha(alpha);
        lfoShapeXSelector.setAlpha(alpha);
        lfoAmplitudeXLabel.setAlpha(alpha);
        lfoAmplitudeXSlider.setAlpha(alpha);
        lfoAmplitudeXValueLabel.setAlpha(alpha);
        lfoRateXLabel.setAlpha(alpha);
        lfoRateXSlider.setAlpha(alpha);
        lfoRateXValueLabel.setAlpha(alpha);
        lfoPhaseXLabel.setAlpha(alpha);
        lfoPhaseXDial.setAlpha(alpha);
        lfoPhaseXValueLabel.setAlpha(alpha);
        lfoPhaseXUnitLabel.setAlpha(alpha);
        lfoOutputXLabel.setAlpha(alpha);
        lfoOutputXSlider.setAlpha(alpha);
    }

    void setLfoAxisYAlpha(float alpha)
    {
        lfoShapeYLabel.setAlpha(alpha);
        lfoShapeYSelector.setAlpha(alpha);
        lfoAmplitudeYLabel.setAlpha(alpha);
        lfoAmplitudeYSlider.setAlpha(alpha);
        lfoAmplitudeYValueLabel.setAlpha(alpha);
        lfoRateYLabel.setAlpha(alpha);
        lfoRateYSlider.setAlpha(alpha);
        lfoRateYValueLabel.setAlpha(alpha);
        lfoPhaseYLabel.setAlpha(alpha);
        lfoPhaseYDial.setAlpha(alpha);
        lfoPhaseYValueLabel.setAlpha(alpha);
        lfoPhaseYUnitLabel.setAlpha(alpha);
        lfoOutputYLabel.setAlpha(alpha);
        lfoOutputYSlider.setAlpha(alpha);
    }

    void setLfoAxisZAlpha(float alpha)
    {
        lfoShapeZLabel.setAlpha(alpha);
        lfoShapeZSelector.setAlpha(alpha);
        lfoAmplitudeZLabel.setAlpha(alpha);
        lfoAmplitudeZSlider.setAlpha(alpha);
        lfoAmplitudeZValueLabel.setAlpha(alpha);
        lfoRateZLabel.setAlpha(alpha);
        lfoRateZSlider.setAlpha(alpha);
        lfoRateZValueLabel.setAlpha(alpha);
        lfoPhaseZLabel.setAlpha(alpha);
        lfoPhaseZDial.setAlpha(alpha);
        lfoPhaseZValueLabel.setAlpha(alpha);
        lfoPhaseZUnitLabel.setAlpha(alpha);
        lfoOutputZLabel.setAlpha(alpha);
        lfoOutputZSlider.setAlpha(alpha);
    }

    void updateLfoAlpha()
    {
        bool lfoEnabled = lfoActiveButton.getToggleState();
        float mainAlpha = lfoEnabled ? 1.0f : 0.5f;
        setLfoParametersAlpha(mainAlpha);

        // Per-axis dimming: dim if LFO is off OR if that axis shape is OFF (id == 1)
        float xAlpha = (lfoEnabled && lfoShapeXSelector.getSelectedId() != 1) ? 1.0f : 0.5f;
        float yAlpha = (lfoEnabled && lfoShapeYSelector.getSelectedId() != 1) ? 1.0f : 0.5f;
        float zAlpha = (lfoEnabled && lfoShapeZSelector.getSelectedId() != 1) ? 1.0f : 0.5f;

        setLfoAxisXAlpha(xAlpha);
        setLfoAxisYAlpha(yAlpha);
        setLfoAxisZAlpha(zAlpha);
    }

    void layoutInputPropertiesTab()
    {
        auto area = subTabContentArea;
        const int rowHeight = 30;
        const int sliderHeight = 40;
        const int spacing = 8;
        const int labelWidth = 115;
        const int valueWidth = 60;

        auto leftCol = area.removeFromLeft(area.getWidth() / 2).reduced(10, 10);

        // Attenuation
        auto row = leftCol.removeFromTop(rowHeight);
        attenuationLabel.setBounds(row.removeFromLeft(labelWidth));
        attenuationValueLabel.setBounds(row.removeFromRight(valueWidth));
        attenuationSlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing);

        // Delay/Latency
        row = leftCol.removeFromTop(rowHeight);
        delayLatencyLabel.setBounds(row.removeFromLeft(labelWidth));
        delayLatencyValueLabel.setBounds(row.removeFromRight(130));  // Wider for "Latency: 100.0 ms"
        delayLatencySlider.setBounds(leftCol.removeFromTop(sliderHeight));
        leftCol.removeFromTop(spacing * 2);  // Extra padding before toggle

        // Minimal Latency - centered beneath slider
        row = leftCol.removeFromTop(rowHeight);
        const int buttonWidth = 200;
        const int buttonX = (row.getWidth() - buttonWidth) / 2;
        minimalLatencyButton.setBounds(row.getX() + buttonX, row.getY(), buttonWidth, rowHeight);
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

        // Coordinate mode selector row
        auto row = leftCol.removeFromTop(rowHeight);
        coordModeLabel.setBounds(row.removeFromLeft(50));
        coordModeSelector.setBounds(row.removeFromLeft(80));
        leftCol.removeFromTop(spacing);

        // Position row
        row = leftCol.removeFromTop(rowHeight);
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

        // Constraint buttons (visibility depends on coordinate mode)
        row = leftCol.removeFromTop(rowHeight);
        auto constraintXPos = row.removeFromLeft(buttonWidth);
        constraintXButton.setBounds(constraintXPos);
        constraintDistanceButton.setBounds(constraintXPos);  // Overlay (mutually exclusive)
        row.removeFromLeft(spacing);
        constraintYButton.setBounds(row.removeFromLeft(buttonWidth));
        row.removeFromLeft(spacing);
        constraintZButton.setBounds(row.removeFromLeft(buttonWidth));
        leftCol.removeFromTop(spacing);

        // Distance constraint slider (for Cylindrical/Spherical modes)
        row = leftCol.removeFromTop(rowHeight);
        distanceMinLabel.setBounds(row.removeFromLeft(35));
        distanceMinEditor.setBounds(row.removeFromLeft(55));
        distanceMinUnitLabel.setBounds(row.removeFromLeft(20));
        row.removeFromLeft(spacing);
        distanceRangeSlider.setBounds(row.removeFromLeft(140));
        row.removeFromLeft(spacing);
        distanceMaxLabel.setBounds(row.removeFromLeft(35));
        distanceMaxEditor.setBounds(row.removeFromLeft(55));
        distanceMaxUnitLabel.setBounds(row.removeFromLeft(20));
        leftCol.removeFromTop(spacing);

        // Flip buttons
        row = leftCol.removeFromTop(rowHeight);
        flipXButton.setBounds(row.removeFromLeft(buttonWidth));
        row.removeFromLeft(spacing);
        flipYButton.setBounds(row.removeFromLeft(buttonWidth));
        row.removeFromLeft(spacing);
        flipZButton.setBounds(row.removeFromLeft(buttonWidth));
        leftCol.removeFromTop(spacing * 2);

        // Position Joystick and Z Slider
        const int joystickSize = 180;
        const int zSliderWidth = 40;
        auto joystickArea = leftCol.removeFromTop(joystickSize + rowHeight);  // Include label height
        positionJoystickLabel.setBounds(joystickArea.removeFromTop(rowHeight));
        auto joystickRow = joystickArea;
        positionJoystick.setBounds(joystickRow.removeFromLeft(joystickSize));
        joystickRow.removeFromLeft(spacing);
        // Z slider next to joystick
        auto zSliderArea = joystickRow.removeFromLeft(zSliderWidth + spacing);
        positionZSliderLabel.setBounds(zSliderArea.removeFromTop(20));
        positionZSlider.setBounds(zSliderArea);

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

        // Path Mode button
        row = rightCol.removeFromTop(rowHeight);
        pathModeButton.setBounds(row.removeFromLeft(150));
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
        inputDirectivityDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
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
        lfoPeriodLabel.setVisible(v); lfoPeriodDial.setVisible(v); lfoPeriodValueLabel.setVisible(v); lfoPeriodUnitLabel.setVisible(v);
        lfoPhaseLabel.setVisible(v); lfoPhaseDial.setVisible(v); lfoPhaseValueLabel.setVisible(v); lfoPhaseUnitLabel.setVisible(v);
        lfoShapeXLabel.setVisible(v); lfoShapeXSelector.setVisible(v);
        lfoShapeYLabel.setVisible(v); lfoShapeYSelector.setVisible(v);
        lfoShapeZLabel.setVisible(v); lfoShapeZSelector.setVisible(v);
        lfoRateXLabel.setVisible(v); lfoRateXSlider.setVisible(v); lfoRateXValueLabel.setVisible(v);
        lfoRateYLabel.setVisible(v); lfoRateYSlider.setVisible(v); lfoRateYValueLabel.setVisible(v);
        lfoRateZLabel.setVisible(v); lfoRateZSlider.setVisible(v); lfoRateZValueLabel.setVisible(v);
        lfoAmplitudeXLabel.setVisible(v); lfoAmplitudeXSlider.setVisible(v); lfoAmplitudeXValueLabel.setVisible(v);
        lfoAmplitudeYLabel.setVisible(v); lfoAmplitudeYSlider.setVisible(v); lfoAmplitudeYValueLabel.setVisible(v);
        lfoAmplitudeZLabel.setVisible(v); lfoAmplitudeZSlider.setVisible(v); lfoAmplitudeZValueLabel.setVisible(v);
        lfoPhaseXLabel.setVisible(v); lfoPhaseXDial.setVisible(v); lfoPhaseXValueLabel.setVisible(v); lfoPhaseXUnitLabel.setVisible(v);
        lfoPhaseYLabel.setVisible(v); lfoPhaseYDial.setVisible(v); lfoPhaseYValueLabel.setVisible(v); lfoPhaseYUnitLabel.setVisible(v);
        lfoPhaseZLabel.setVisible(v); lfoPhaseZDial.setVisible(v); lfoPhaseZValueLabel.setVisible(v); lfoPhaseZUnitLabel.setVisible(v);
        lfoGyrophoneLabel.setVisible(v); lfoGyrophoneSelector.setVisible(v);
        jitterLabel.setVisible(v); jitterSlider.setVisible(v); jitterValueLabel.setVisible(v);
        // LFO progress dial and output indicators
        lfoProgressDial.setVisible(v);
        lfoOutputXLabel.setVisible(v); lfoOutputXSlider.setVisible(v);
        lfoOutputYLabel.setVisible(v); lfoOutputYSlider.setVisible(v);
        lfoOutputZLabel.setVisible(v); lfoOutputZSlider.setVisible(v);
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

        // Period dial with progress indicator
        lfoPeriodLabel.setBounds(leftCol.removeFromTop(rowHeight));
        auto dialRow = leftCol.removeFromTop(dialSize);
        lfoPeriodDial.setBounds(dialRow.removeFromLeft(dialSize).withSizeKeepingCentre(dialSize, dialSize));
        dialRow.removeFromLeft(spacing);
        lfoProgressDial.setBounds(dialRow.removeFromLeft(dialSize).withSizeKeepingCentre(dialSize, dialSize));
        lfoPeriodValueLabel.setBounds(leftCol.removeFromTop(rowHeight));
        leftCol.removeFromTop(spacing);

        // Main Phase dial
        lfoPhaseLabel.setBounds(leftCol.removeFromTop(rowHeight));
        auto dialArea = leftCol.removeFromTop(dialSize);
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
        middleCol.removeFromTop(spacing * 2);

        // LFO Output sliders (read-only feedback)
        row = middleCol.removeFromTop(rowHeight);
        lfoOutputXLabel.setBounds(row.removeFromLeft(labelWidth));
        lfoOutputXSlider.setBounds(middleCol.removeFromTop(sliderHeight));
        middleCol.removeFromTop(spacing);

        row = middleCol.removeFromTop(rowHeight);
        lfoOutputYLabel.setBounds(row.removeFromLeft(labelWidth));
        lfoOutputYSlider.setBounds(middleCol.removeFromTop(sliderHeight));
        middleCol.removeFromTop(spacing);

        row = middleCol.removeFromTop(rowHeight);
        lfoOutputZLabel.setBounds(row.removeFromLeft(labelWidth));
        lfoOutputZSlider.setBounds(middleCol.removeFromTop(sliderHeight));

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
        otomoTitleLabel.setVisible(v);
        otomoCoordModeSelector.setVisible(v);
        otomoDestXLabel.setVisible(v); otomoDestXEditor.setVisible(v); otomoDestXUnitLabel.setVisible(v);
        otomoDestYLabel.setVisible(v); otomoDestYEditor.setVisible(v); otomoDestYUnitLabel.setVisible(v);
        otomoDestZLabel.setVisible(v); otomoDestZEditor.setVisible(v); otomoDestZUnitLabel.setVisible(v);
        otomoAbsRelButton.setVisible(v);
        otomoStayReturnButton.setVisible(v);
        otomoDurationLabel.setVisible(v); otomoDurationDial.setVisible(v); otomoDurationValueLabel.setVisible(v);
        // Curve visibility depends on coordinate mode (hidden in Cylindrical/Spherical)
        bool showCurve = v && (otomoCoordModeSelector.getSelectedId() == 1);  // 1 = Cartesian
        otomoCurveLabel.setVisible(showCurve); otomoCurveDial.setVisible(showCurve); otomoCurveValueLabel.setVisible(showCurve); otomoCurveUnitLabel.setVisible(showCurve);
        otomoSpeedProfileLabel.setVisible(v); otomoSpeedProfileDial.setVisible(v); otomoSpeedProfileValueLabel.setVisible(v); otomoSpeedProfileUnitLabel.setVisible(v);
        otomoTriggerButton.setVisible(v);
        // Threshold and Reset are dimmed (not hidden) in Manual mode - handle alpha separately
        otomoThresholdLabel.setVisible(v); otomoThresholdDial.setVisible(v); otomoThresholdValueLabel.setVisible(v); otomoThresholdUnitLabel.setVisible(v);
        otomoResetLabel.setVisible(v); otomoResetDial.setVisible(v); otomoResetValueLabel.setVisible(v); otomoResetUnitLabel.setVisible(v);
        otomoStartButton.setVisible(v);
        otomoStopButton.setVisible(v);
        otomoPauseButton.setVisible(v);
        otomoStopAllButton.setVisible(v);
        otomoPauseResumeAllButton.setVisible(v);
        if (v)
            updateOtomoTriggerAppearance();
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
        const int dialSize = 60;
        const int transportButtonSize = 40;

        // Split into three columns for more dials
        auto leftCol = area.removeFromLeft(area.getWidth() / 3).reduced(5, 0);
        auto midCol = area.removeFromLeft(area.getWidth() / 2).reduced(5, 0);
        auto rightCol = area.reduced(5, 0);

        // Left column: Destination, buttons, transport
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

        // Transport buttons (per-input)
        row = leftCol.removeFromTop(transportButtonSize);
        otomoStartButton.setBounds(row.removeFromLeft(transportButtonSize));
        row.removeFromLeft(spacing);
        otomoPauseButton.setBounds(row.removeFromLeft(transportButtonSize));
        row.removeFromLeft(spacing);
        otomoStopButton.setBounds(row.removeFromLeft(transportButtonSize));
        leftCol.removeFromTop(spacing * 2);

        // Global buttons
        row = leftCol.removeFromTop(rowHeight);
        otomoStopAllButton.setBounds(row.removeFromLeft(buttonWidth));
        row.removeFromLeft(spacing);
        otomoPauseResumeAllButton.setBounds(row.removeFromLeft(buttonWidth));

        // Middle column: Duration, Curve, Speed Profile dials
        otomoDurationLabel.setBounds(midCol.removeFromTop(rowHeight));
        auto dialArea = midCol.removeFromTop(dialSize);
        otomoDurationDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
        otomoDurationValueLabel.setBounds(midCol.removeFromTop(rowHeight));
        midCol.removeFromTop(spacing);

        otomoCurveLabel.setBounds(midCol.removeFromTop(rowHeight));
        dialArea = midCol.removeFromTop(dialSize);
        otomoCurveDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
        otomoCurveValueLabel.setBounds(midCol.removeFromTop(rowHeight));
        midCol.removeFromTop(spacing);

        otomoSpeedProfileLabel.setBounds(midCol.removeFromTop(rowHeight));
        dialArea = midCol.removeFromTop(dialSize);
        otomoSpeedProfileDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
        otomoSpeedProfileValueLabel.setBounds(midCol.removeFromTop(rowHeight));

        // Right column: Threshold and Reset dials (audio trigger)
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

    void setVisualisationVisible(bool v)
    {
        visualisationComponent.setVisible(v);
    }

    void layoutVisualisationTab()
    {
        visualisationComponent.setBounds(subTabContentArea);
    }

    // ==================== COMBINED LAYOUT METHODS (4-tab structure) ====================

    void layoutInputParametersTab()
    {
        // 2-column layout: Column 1 (Input+Position), Column 2 (Sound+Mutes)
        auto area = subTabContentArea;
        const int rowHeight = 30;      // Match OutputsTab
        const int sliderHeight = 40;   // Match OutputsTab
        const int spacing = 8;         // Match OutputsTab
        const int labelWidth = 115;    // Match OutputsTab
        const int valueWidth = 60;     // Match OutputsTab
        const int dialSize = 55;

        auto col1 = area.removeFromLeft(area.getWidth() / 2).reduced(10, 10);  // Match OutputsTab padding
        auto col2 = area.reduced(5, 0);

        // ========== COLUMN 1: Input + Position ==========

        // --- Input section ---
        // Attenuation
        auto row = col1.removeFromTop(rowHeight);
        attenuationLabel.setBounds(row.removeFromLeft(labelWidth));
        attenuationValueLabel.setBounds(row.removeFromRight(valueWidth));
        attenuationSlider.setBounds(col1.removeFromTop(sliderHeight));
        col1.removeFromTop(spacing);

        // Delay/Latency
        row = col1.removeFromTop(rowHeight);
        delayLatencyLabel.setBounds(row.removeFromLeft(labelWidth));
        delayLatencyValueLabel.setBounds(row.removeFromRight(130));  // Wider for "Latency: 100.0 ms"
        delayLatencySlider.setBounds(col1.removeFromTop(sliderHeight));
        col1.removeFromTop(spacing * 2);  // Extra padding before toggle

        // Minimal Latency button - centered beneath slider
        row = col1.removeFromTop(rowHeight);
        const int buttonWidth = 150;
        const int buttonX = (row.getWidth() - buttonWidth) / 2;
        minimalLatencyButton.setBounds(row.getX() + buttonX, row.getY(), buttonWidth, rowHeight);
        col1.removeFromTop(spacing * 2);

        // --- Position section (3-row layout with joystick on right) ---
        const int joystickSize = 140;
        const int zSliderWidth = 40;
        const int posBlockHeight = joystickSize + 20;  // Match joystick height + label

        // Create position block area with joystick on right
        auto posBlock = col1.removeFromTop(posBlockHeight);

        // Right side: Joystick and Z slider
        const int zLabelWidth = 20;
        const int joystickPadding = 8;  // Padding to prevent grey disc clipping
        auto joystickBlock = posBlock.removeFromRight(joystickSize + joystickPadding * 2 + spacing + zSliderWidth + zLabelWidth);

        // X/Y label at top-left of joystick
        auto labelRow = joystickBlock.removeFromTop(18);
        labelRow.removeFromLeft(joystickPadding);  // Align label with joystick
        positionJoystickLabel.setBounds(labelRow.removeFromLeft(30));

        // Joystick area with horizontal padding
        auto joystickRow = joystickBlock;
        joystickRow.removeFromLeft(joystickPadding);
        positionJoystick.setBounds(joystickRow.removeFromLeft(joystickSize));
        joystickRow.removeFromLeft(joystickPadding);
        joystickRow.removeFromLeft(spacing);

        // Z slider with label to the right at middle position
        auto zSliderArea = joystickRow.removeFromLeft(zSliderWidth);
        positionZSlider.setBounds(zSliderArea);
        // Z label positioned at vertical center of slider (middle position indicator)
        auto zLabelArea = joystickRow;
        int sliderMidY = zSliderArea.getY() + zSliderArea.getHeight() / 2 - 8;
        positionZSliderLabel.setBounds(zLabelArea.getX(), sliderMidY, zLabelWidth, 16);

        // Left side: Position/Offset/Constraints/Flips organized by axis
        const int posLabelWidth = 75;    // "Position X:" fits fully
        const int posEditorWidth = 55;
        const int posUnitWidth = 25;     // "m" unit label
        const int constraintBtnWidth = 100;  // Enlarged constraint buttons
        const int flipBtnWidth = 80;
        const int rowGap = 20;  // Increased vertical padding between rows

        // Align Y row (second row) center with joystick center
        // Joystick center is at: labelRowHeight(18) + joystickSize/2
        const int joystickCenterY = 18 + joystickSize / 2;  // 88px from posBlock top
        // Y row center = topPadding + rowHeight + rowGap + rowHeight/2
        // Solve for topPadding: topPadding = joystickCenterY - rowHeight - rowGap - rowHeight/2
        const int topPadding = joystickCenterY - rowHeight - rowGap - rowHeight / 2;
        posBlock.removeFromTop(topPadding);

        // Row 1: Coord mode + X axis (Position X, Offset X, Constraint X, Flip X)
        row = posBlock.removeFromTop(rowHeight);
        coordModeLabel.setBounds(row.removeFromLeft(40));
        coordModeSelector.setBounds(row.removeFromLeft(70));
        row.removeFromLeft(spacing);
        posXLabel.setBounds(row.removeFromLeft(posLabelWidth));
        posXEditor.setBounds(row.removeFromLeft(posEditorWidth));
        posXUnitLabel.setBounds(row.removeFromLeft(posUnitWidth));
        row.removeFromLeft(spacing);
        offsetXLabel.setBounds(row.removeFromLeft(posLabelWidth));
        offsetXEditor.setBounds(row.removeFromLeft(posEditorWidth));
        offsetXUnitLabel.setBounds(row.removeFromLeft(posUnitWidth));
        row.removeFromLeft(spacing);
        auto constraintXPos = row.removeFromLeft(constraintBtnWidth);
        constraintXButton.setBounds(constraintXPos);
        constraintDistanceButton.setBounds(constraintXPos);  // Overlay on same position (mutually exclusive)
        row.removeFromLeft(spacing);
        flipXButton.setBounds(row.removeFromLeft(flipBtnWidth));
        posBlock.removeFromTop(rowGap);

        // Row 2: Y axis (Position Y, Offset Y, Constraint Y, Flip Y)
        row = posBlock.removeFromTop(rowHeight);
        row.removeFromLeft(40 + 70 + spacing);  // Skip coord mode space
        posYLabel.setBounds(row.removeFromLeft(posLabelWidth));
        posYEditor.setBounds(row.removeFromLeft(posEditorWidth));
        posYUnitLabel.setBounds(row.removeFromLeft(posUnitWidth));
        row.removeFromLeft(spacing);
        offsetYLabel.setBounds(row.removeFromLeft(posLabelWidth));
        offsetYEditor.setBounds(row.removeFromLeft(posEditorWidth));
        offsetYUnitLabel.setBounds(row.removeFromLeft(posUnitWidth));
        row.removeFromLeft(spacing);
        constraintYButton.setBounds(row.removeFromLeft(constraintBtnWidth));
        row.removeFromLeft(spacing);
        flipYButton.setBounds(row.removeFromLeft(flipBtnWidth));
        posBlock.removeFromTop(rowGap);

        // Row 3: Z axis (Position Z, Offset Z, Constraint Z, Flip Z)
        row = posBlock.removeFromTop(rowHeight);
        row.removeFromLeft(40 + 70 + spacing);  // Skip coord mode space
        posZLabel.setBounds(row.removeFromLeft(posLabelWidth));
        posZEditor.setBounds(row.removeFromLeft(posEditorWidth));
        posZUnitLabel.setBounds(row.removeFromLeft(posUnitWidth));
        row.removeFromLeft(spacing);
        offsetZLabel.setBounds(row.removeFromLeft(posLabelWidth));
        offsetZEditor.setBounds(row.removeFromLeft(posEditorWidth));
        offsetZUnitLabel.setBounds(row.removeFromLeft(posUnitWidth));
        row.removeFromLeft(spacing);
        constraintZButton.setBounds(row.removeFromLeft(constraintBtnWidth));
        row.removeFromLeft(spacing);
        flipZButton.setBounds(row.removeFromLeft(flipBtnWidth));

        // Distance constraint slider row (after position block, for Cylindrical/Spherical modes)
        // Center the slider row horizontally with Constraint R button (which replaces Constraint X)
        col1.removeFromTop(spacing);
        row = col1.removeFromTop(rowHeight);
        // Constraint R button starts after pos/offset fields and is centered
        const int constraintStart = 40 + 70 + spacing + (posLabelWidth + posEditorWidth + posUnitWidth + spacing) * 2;
        const int constraintRCenterX = constraintStart + constraintBtnWidth / 2;  // Center of single button
        const int sliderRowWidth = 35 + 55 + 20 + spacing + 180 + spacing + 35 + 55 + 20;
        const int sliderRowStartX = constraintRCenterX - sliderRowWidth / 2;
        row.removeFromLeft(sliderRowStartX);
        distanceMinLabel.setBounds(row.removeFromLeft(35));
        distanceMinEditor.setBounds(row.removeFromLeft(55));
        distanceMinUnitLabel.setBounds(row.removeFromLeft(20));
        row.removeFromLeft(spacing);
        distanceRangeSlider.setBounds(row.removeFromLeft(180));
        row.removeFromLeft(spacing);
        distanceMaxLabel.setBounds(row.removeFromLeft(35));
        distanceMaxEditor.setBounds(row.removeFromLeft(55));
        distanceMaxUnitLabel.setBounds(row.removeFromLeft(20));

        col1.removeFromTop(spacing);

        // Four-column layout for Sidelines, Tracking, Max Speed, Height Factor
        // Spans left half width (col1)
        const int fourColWidth = col1.getWidth() / 4;
        const int controlBlockHeight = rowHeight * 2 + spacing * 2 + dialSize + rowHeight * 2;
        auto controlBlock = col1.removeFromTop(controlBlockHeight);
        const int uniformButtonWidth = 130;  // Same width for all buttons
        const int dialLabelWidth = 120;      // Wide enough for "Tracking Smoothing:"

        // Column 1: Sidelines
        auto sidelinesCol = controlBlock.removeFromLeft(fourColWidth);
        int colCenterX = sidelinesCol.getX() + fourColWidth / 2;

        // Row 1: Sidelines button (centered)
        row = sidelinesCol.removeFromTop(rowHeight);
        sidelinesActiveButton.setBounds(colCenterX - uniformButtonWidth / 2, row.getY(), uniformButtonWidth, rowHeight);
        sidelinesCol.removeFromTop(spacing);

        // Row 2: empty (skip to align with other columns)
        sidelinesCol.removeFromTop(rowHeight + spacing);

        // Dial section: label, dial, value
        sidelinesFringeLabel.setBounds(colCenterX - dialLabelWidth / 2, sidelinesCol.getY(), dialLabelWidth, rowHeight);
        sidelinesCol.removeFromTop(rowHeight);
        sidelinesFringeDial.setBounds(colCenterX - dialSize / 2, sidelinesCol.getY(), dialSize, dialSize);
        sidelinesCol.removeFromTop(dialSize);
        sidelinesFringeValueLabel.setBounds(colCenterX - 35, sidelinesCol.getY(), 70, rowHeight);

        // Column 2: Tracking
        auto trackCol = controlBlock.removeFromLeft(fourColWidth);
        colCenterX = trackCol.getX() + fourColWidth / 2;

        // Row 1: Tracking button (centered)
        row = trackCol.removeFromTop(rowHeight);
        trackingActiveButton.setBounds(colCenterX - uniformButtonWidth / 2, row.getY(), uniformButtonWidth, rowHeight);
        trackCol.removeFromTop(spacing);

        // Row 2: Tracking ID (centered)
        row = trackCol.removeFromTop(rowHeight);
        int idRowWidth = 75 + 50;  // label + selector
        int idRowX = colCenterX - idRowWidth / 2;
        trackingIdLabel.setBounds(idRowX, row.getY(), 75, rowHeight);
        trackingIdSelector.setBounds(idRowX + 75, row.getY(), 50, rowHeight);
        trackCol.removeFromTop(spacing);

        // Dial section: label, dial, value+unit (value right-justified to center, unit left-justified from center)
        trackingSmoothLabel.setBounds(colCenterX - dialLabelWidth / 2, trackCol.getY(), dialLabelWidth, rowHeight);
        trackCol.removeFromTop(rowHeight);
        trackingSmoothDial.setBounds(colCenterX - dialSize / 2, trackCol.getY(), dialSize, dialSize);
        trackCol.removeFromTop(dialSize);
        layoutDialValueUnit(trackingSmoothValueLabel, trackingSmoothUnitLabel, colCenterX, trackCol.getY(), rowHeight);

        // Column 3: Max Speed
        auto speedCol = controlBlock.removeFromLeft(fourColWidth);
        colCenterX = speedCol.getX() + fourColWidth / 2;

        // Row 1: Max Speed button (centered)
        row = speedCol.removeFromTop(rowHeight);
        maxSpeedActiveButton.setBounds(colCenterX - uniformButtonWidth / 2, row.getY(), uniformButtonWidth, rowHeight);
        speedCol.removeFromTop(spacing);

        // Row 2: Path Mode button (centered, same width)
        row = speedCol.removeFromTop(rowHeight);
        pathModeButton.setBounds(colCenterX - uniformButtonWidth / 2, row.getY(), uniformButtonWidth, rowHeight);
        speedCol.removeFromTop(spacing);

        // Dial section: label, dial, value+unit
        maxSpeedLabel.setBounds(colCenterX - dialLabelWidth / 2, speedCol.getY(), dialLabelWidth, rowHeight);
        speedCol.removeFromTop(rowHeight);
        maxSpeedDial.setBounds(colCenterX - dialSize / 2, speedCol.getY(), dialSize, dialSize);
        speedCol.removeFromTop(dialSize);
        layoutDialValueUnit(maxSpeedValueLabel, maxSpeedUnitLabel, colCenterX, speedCol.getY(), rowHeight, 40, 35);

        // Column 4: Height Factor
        auto heightCol = controlBlock;
        colCenterX = heightCol.getX() + heightCol.getWidth() / 2;

        // Skip first two rows (no buttons in this column)
        heightCol.removeFromTop(rowHeight + spacing + rowHeight + spacing);

        // Dial section: label, dial, value+unit
        heightFactorLabel.setBounds(colCenterX - dialLabelWidth / 2, heightCol.getY(), dialLabelWidth, rowHeight);
        heightCol.removeFromTop(rowHeight);
        heightFactorDial.setBounds(colCenterX - dialSize / 2, heightCol.getY(), dialSize, dialSize);
        heightCol.removeFromTop(dialSize);
        layoutDialValueUnit(heightFactorValueLabel, heightFactorUnitLabel, colCenterX, heightCol.getY(), rowHeight);

        // ========== COLUMN 2: Sound + Mutes ==========

        // --- Top row: Attenuation Law, Distance Atten, Common Atten (tighter layout) ---
        const int topBlockHeight = dialSize + rowHeight * 2;
        auto topBlock = col2.removeFromTop(topBlockHeight);

        // Calculate item widths and total needed width
        const int attenLawWidth = 140;  // Label/button width
        const int dialSectionWidth = 110;  // Label width for dial sections
        const int itemSpacing = spacing * 4;  // Spacing between items
        const int totalTopRowWidth = attenLawWidth + dialSectionWidth * 2 + itemSpacing * 2;

        // Center the group within topBlock
        int topRowStartX = topBlock.getX() + (topBlock.getWidth() - totalTopRowWidth) / 2;
        int topRowY = topBlock.getY();

        // Column 1: Attenuation Law - label aligned with dial labels, button centered with dials
        int attenLawCenterX = topRowStartX + attenLawWidth / 2;
        attenuationLawLabel.setBounds(attenLawCenterX - 70, topRowY, 140, rowHeight);
        // Button vertically centered with dials (dials start at topRowY + rowHeight)
        int dialCenterY = topRowY + rowHeight + dialSize / 2;
        attenuationLawButton.setBounds(attenLawCenterX - 60, dialCenterY - rowHeight / 2, 120, rowHeight);

        // Column 2: Distance Atten dial
        int distCenterX = topRowStartX + attenLawWidth + itemSpacing + dialSectionWidth / 2;
        distanceAttenLabel.setBounds(distCenterX - 55, topRowY, 110, rowHeight);
        distanceRatioLabel.setBounds(distanceAttenLabel.getBounds());
        distanceAttenDial.setBounds(distCenterX - dialSize / 2, topRowY + rowHeight, dialSize, dialSize);
        distanceRatioDial.setBounds(distanceAttenDial.getBounds());
        layoutDialValueUnit(distanceAttenValueLabel, distanceAttenUnitLabel, distCenterX, topRowY + rowHeight + dialSize, rowHeight, 35, 50);
        layoutDialValueUnit(distanceRatioValueLabel, distanceRatioUnitLabel, distCenterX, topRowY + rowHeight + dialSize, rowHeight, 35, 25);

        // Column 3: Common Atten dial
        int commonCenterX = topRowStartX + attenLawWidth + itemSpacing + dialSectionWidth + itemSpacing + dialSectionWidth / 2;
        commonAttenLabel.setBounds(commonCenterX - 55, topRowY, 110, rowHeight);
        commonAttenDial.setBounds(commonCenterX - dialSize / 2, topRowY + rowHeight, dialSize, dialSize);
        layoutDialValueUnit(commonAttenValueLabel, commonAttenUnitLabel, commonCenterX, topRowY + rowHeight + dialSize, rowHeight);

        // Reduced padding before sliders section
        col2.removeFromTop(spacing);

        // --- Sliders + Rotation dial section ---
        const int largeRotationDial = dialSize * 2;  // Twice as large
        const int slidersWidth = col2.getWidth() - largeRotationDial - spacing * 2;
        const int slidersBlockHeight = (rowHeight + sliderHeight + spacing) * 3;
        auto slidersBlock = col2.removeFromTop(slidersBlockHeight);

        // Left side: Three sliders
        auto slidersArea = slidersBlock.removeFromLeft(slidersWidth);

        // Directivity
        row = slidersArea.removeFromTop(rowHeight);
        directivityLabel.setBounds(row.removeFromLeft(70));
        directivityValueLabel.setBounds(row.removeFromRight(90));
        directivitySlider.setBounds(slidersArea.removeFromTop(sliderHeight));
        slidersArea.removeFromTop(spacing);

        // Tilt
        row = slidersArea.removeFromTop(rowHeight);
        tiltLabel.setBounds(row.removeFromLeft(70));
        tiltValueLabel.setBounds(row.removeFromRight(90));
        tiltSlider.setBounds(slidersArea.removeFromTop(sliderHeight));
        slidersArea.removeFromTop(spacing);

        // HF Shelf
        row = slidersArea.removeFromTop(rowHeight);
        hfShelfLabel.setBounds(row.removeFromLeft(70));
        hfShelfValueLabel.setBounds(row.removeFromRight(90));
        hfShelfSlider.setBounds(slidersArea.removeFromTop(sliderHeight));

        // Right side: Large Rotation dial (centered vertically)
        slidersBlock.removeFromLeft(spacing);
        auto rotArea = slidersBlock;
        int rotCenterX = rotArea.getX() + rotArea.getWidth() / 2;
        int rotCenterY = rotArea.getY() + rotArea.getHeight() / 2;
        rotationLabel.setBounds(rotCenterX - 50, rotArea.getY(), 100, rowHeight);
        inputDirectivityDial.setBounds(rotCenterX - largeRotationDial / 2, rotCenterY - largeRotationDial / 2, largeRotationDial, largeRotationDial);
        layoutDialValueUnit(rotationValueLabel, rotationUnitLabel, rotCenterX, rotArea.getBottom() - rowHeight, rowHeight, 40, 25);

        // Spacing after HF Shelf
        col2.removeFromTop(spacing);

        // --- Array Attenuation - all 10 dials on single line ---
        const int smallDialSize = 36;
        const int arrayDialSpacing = (col2.getWidth() - smallDialSize * 10) / 10;
        const int arrayLabelWidth = smallDialSize + arrayDialSpacing;  // Full width per dial slot
        arrayAttenLabel.setBounds(col2.removeFromTop(rowHeight).removeFromLeft(150));

        auto arrayRow = col2.removeFromTop(smallDialSize + 30);
        for (int i = 0; i < 10; ++i)
        {
            int slotX = arrayRow.getX() + i * (smallDialSize + arrayDialSpacing);
            int dialX = slotX + arrayDialSpacing / 2;
            int labelCenterX = dialX + smallDialSize / 2;  // Center of dial
            arrayAttenDialLabels[i].setBounds(labelCenterX - arrayLabelWidth / 2, arrayRow.getY(), arrayLabelWidth, 12);
            arrayAttenDials[i].setBounds(dialX, arrayRow.getY() + 12, smallDialSize, smallDialSize);
            arrayAttenValueLabels[i].setBounds(labelCenterX - arrayLabelWidth / 2, arrayRow.getY() + 12 + smallDialSize, arrayLabelWidth, 12);
        }
        col2.removeFromTop(spacing);

        // --- Mute Macros selector ---
        row = col2.removeFromTop(rowHeight);
        muteMacrosLabel.setBounds(row.removeFromLeft(90));
        muteMacrosSelector.setBounds(row.removeFromLeft(150));
        col2.removeFromTop(spacing);

        // --- Mutes section (single line, fill width) ---
        const int muteButtonSize = 36;
        const int muteSpacing = 4;
        int numOutputs = parameters.getNumOutputChannels();
        if (numOutputs <= 0) numOutputs = 16;

        // Calculate how many fit per row
        int muteButtonsPerRow = (col2.getWidth() + muteSpacing) / (muteButtonSize + muteSpacing);
        if (muteButtonsPerRow <= 0) muteButtonsPerRow = 1;  // Prevent division by zero
        int muteRows = (numOutputs + muteButtonsPerRow - 1) / muteButtonsPerRow;

        auto muteGridArea = col2.removeFromTop(muteRows * (muteButtonSize + muteSpacing));
        for (int r = 0; r < muteRows; ++r)
        {
            auto rowArea = muteGridArea.removeFromTop(muteButtonSize + muteSpacing);
            for (int c = 0; c < muteButtonsPerRow; ++c)
            {
                int index = r * muteButtonsPerRow + c;
                if (index < numOutputs)
                {
                    muteButtons[index].setBounds(rowArea.removeFromLeft(muteButtonSize));
                    rowArea.removeFromLeft(muteSpacing);
                }
            }
        }

        // Extra padding after mutes
        col2.removeFromTop(spacing * 2);
    }

    void layoutLiveSourceHackousticsTab()
    {
        // 2-column layout: Column 1 (Live Source), Column 2 (Hackoustics)
        auto area = subTabContentArea;
        const int rowHeight = 26;
        const int sliderHeight = 32;
        const int spacing = 6;
        const int labelWidth = 100;
        const int valueWidth = 60;  // Tight value width like LFO section
        const int dialSize = 65;
        const int buttonWidth = 120;

        auto col1 = area.removeFromLeft(area.getWidth() / 2).reduced(5, 0);
        auto col2 = area.reduced(5, 0);

        // ========== COLUMN 1: Live Source ==========
        lsActiveButton.setBounds(col1.removeFromTop(rowHeight).withWidth(180));
        col1.removeFromTop(spacing * 2);  // Extra padding after toggle

        // Shape selector
        auto row = col1.removeFromTop(rowHeight);
        lsShapeLabel.setBounds(row.removeFromLeft(labelWidth));
        lsShapeSelector.setBounds(row.removeFromLeft(100));
        col1.removeFromTop(spacing * 2);  // Extra padding after shape

        // Radius
        row = col1.removeFromTop(rowHeight);
        lsRadiusLabel.setBounds(row.removeFromLeft(labelWidth));
        lsRadiusValueLabel.setBounds(row.removeFromRight(valueWidth));
        lsRadiusSlider.setBounds(col1.removeFromTop(sliderHeight));
        col1.removeFromTop(spacing * 2);  // Extra padding after radius

        // Attenuation
        row = col1.removeFromTop(rowHeight);
        lsAttenuationLabel.setBounds(row.removeFromLeft(labelWidth));
        lsAttenuationValueLabel.setBounds(row.removeFromRight(valueWidth));
        lsAttenuationSlider.setBounds(col1.removeFromTop(sliderHeight));
        col1.removeFromTop(spacing * 2);  // Extra padding before thresholds

        // Peak Threshold + Peak Ratio (side by side, vertically aligned)
        const int sliderPortion = static_cast<int>(col1.getWidth() * 0.68f);
        const int thresholdBlockHeight = dialSize + rowHeight * 2;

        auto peakBlock = col1.removeFromTop(thresholdBlockHeight);
        auto peakSliderArea = peakBlock.removeFromLeft(sliderPortion);
        peakBlock.removeFromLeft(spacing * 2);
        auto peakDialArea = peakBlock;

        // Peak dial section (right) - label, dial, prefix+value stacked vertically (ratio format "1:2.0")
        lsPeakRatioLabel.setBounds(peakDialArea.removeFromTop(rowHeight));
        auto peakRatioDialBounds = peakDialArea.removeFromTop(dialSize);
        lsPeakRatioDial.setBounds(peakRatioDialBounds.withSizeKeepingCentre(dialSize, dialSize));
        int peakDialCenterX = peakRatioDialBounds.getX() + peakRatioDialBounds.getWidth() / 2;
        // Ratio format: prefix "1:" on left (right-justified), value on right (left-justified), centered as pair with overlap
        const int peakPrefixW = 28, peakValueW = 35, peakOverlap = 7;
        int peakStartX = peakDialCenterX - (peakPrefixW + peakValueW - peakOverlap) / 2;
        lsPeakRatioUnitLabel.setBounds(peakStartX, peakDialArea.getY(), peakPrefixW, rowHeight);
        lsPeakRatioValueLabel.setBounds(peakStartX + peakPrefixW - peakOverlap, peakDialArea.getY(), peakValueW, rowHeight);

        // Peak slider section (left) - label + value on same row, slider below
        row = peakSliderArea.removeFromTop(rowHeight);
        lsPeakThresholdLabel.setBounds(row.removeFromLeft(labelWidth));
        lsPeakThresholdValueLabel.setBounds(row.removeFromRight(valueWidth));
        int sliderVerticalOffset = (dialSize - sliderHeight) / 2;  // Center slider with dial
        peakSliderArea.removeFromTop(sliderVerticalOffset);
        lsPeakThresholdSlider.setBounds(peakSliderArea.removeFromTop(sliderHeight));

        col1.removeFromTop(spacing * 2);

        // Slow Threshold + Slow Ratio (side by side, vertically aligned)
        auto slowBlock = col1.removeFromTop(thresholdBlockHeight);
        auto slowSliderArea = slowBlock.removeFromLeft(sliderPortion);
        slowBlock.removeFromLeft(spacing * 2);
        auto slowDialArea = slowBlock;

        // Slow dial section (right) - label, dial, prefix+value stacked vertically (ratio format "1:2.0")
        lsSlowRatioLabel.setBounds(slowDialArea.removeFromTop(rowHeight));
        auto slowRatioDialBounds = slowDialArea.removeFromTop(dialSize);
        lsSlowRatioDial.setBounds(slowRatioDialBounds.withSizeKeepingCentre(dialSize, dialSize));
        int slowDialCenterX = slowRatioDialBounds.getX() + slowRatioDialBounds.getWidth() / 2;
        // Ratio format: prefix "1:" on left (right-justified), value on right (left-justified), centered as pair with overlap
        const int slowPrefixW = 28, slowValueW = 35, slowOverlap = 7;
        int slowStartX = slowDialCenterX - (slowPrefixW + slowValueW - slowOverlap) / 2;
        lsSlowRatioUnitLabel.setBounds(slowStartX, slowDialArea.getY(), slowPrefixW, rowHeight);
        lsSlowRatioValueLabel.setBounds(slowStartX + slowPrefixW - slowOverlap, slowDialArea.getY(), slowValueW, rowHeight);

        // Slow slider section (left) - label + value on same row, slider below
        row = slowSliderArea.removeFromTop(rowHeight);
        lsSlowThresholdLabel.setBounds(row.removeFromLeft(labelWidth));
        lsSlowThresholdValueLabel.setBounds(row.removeFromRight(valueWidth));
        slowSliderArea.removeFromTop(sliderVerticalOffset);
        lsSlowThresholdSlider.setBounds(slowSliderArea.removeFromTop(sliderHeight));

        // ========== COLUMN 2: Hackoustics (Floor Reflections) ==========
        frActiveButton.setBounds(col2.removeFromTop(rowHeight).withWidth(180));
        col2.removeFromTop(spacing);

        // Attenuation
        row = col2.removeFromTop(rowHeight);
        frAttenuationLabel.setBounds(row.removeFromLeft(labelWidth));
        frAttenuationValueLabel.setBounds(row.removeFromRight(valueWidth));
        frAttenuationSlider.setBounds(col2.removeFromTop(sliderHeight));
        col2.removeFromTop(spacing);

        // Diffusion dial (centered in column) with split value/unit
        int diffusionBlockHeight = dialSize + rowHeight * 2;
        auto diffBlock = col2.removeFromTop(diffusionBlockHeight);
        int diffCenterX = diffBlock.getX() + diffBlock.getWidth() / 2;
        frDiffusionLabel.setBounds(diffCenterX - 50, diffBlock.getY(), 100, rowHeight);
        frDiffusionDial.setBounds(diffCenterX - dialSize / 2, diffBlock.getY() + rowHeight, dialSize, dialSize);
        layoutDialValueUnit(frDiffusionValueLabel, frDiffusionUnitLabel, diffCenterX, diffBlock.getY() + rowHeight + dialSize, rowHeight, 30, 25);

        col2.removeFromTop(spacing * 2);

        // Low Cut section (toggle on its own line)
        frLowCutActiveButton.setBounds(col2.removeFromTop(rowHeight).withWidth(buttonWidth));
        col2.removeFromTop(spacing);

        // Low Cut Frequency (label + value on its own line)
        row = col2.removeFromTop(rowHeight);
        frLowCutFreqLabel.setBounds(row.removeFromLeft(labelWidth));
        frLowCutFreqValueLabel.setBounds(row.removeFromRight(valueWidth));
        frLowCutFreqSlider.setBounds(col2.removeFromTop(sliderHeight));

        col2.removeFromTop(spacing * 2);

        // High Shelf section (toggle on its own line)
        frHighShelfActiveButton.setBounds(col2.removeFromTop(rowHeight).withWidth(buttonWidth + 20));
        col2.removeFromTop(spacing);

        // High Shelf Frequency
        row = col2.removeFromTop(rowHeight);
        frHighShelfFreqLabel.setBounds(row.removeFromLeft(labelWidth));
        frHighShelfFreqValueLabel.setBounds(row.removeFromRight(valueWidth));
        frHighShelfFreqSlider.setBounds(col2.removeFromTop(sliderHeight));
        col2.removeFromTop(spacing);

        // High Shelf Gain
        row = col2.removeFromTop(rowHeight);
        frHighShelfGainLabel.setBounds(row.removeFromLeft(labelWidth));
        frHighShelfGainValueLabel.setBounds(row.removeFromRight(valueWidth));
        frHighShelfGainSlider.setBounds(col2.removeFromTop(sliderHeight));
        col2.removeFromTop(spacing);

        // High Shelf Slope
        row = col2.removeFromTop(rowHeight);
        frHighShelfSlopeLabel.setBounds(row.removeFromLeft(labelWidth));
        frHighShelfSlopeValueLabel.setBounds(row.removeFromRight(valueWidth));
        frHighShelfSlopeSlider.setBounds(col2.removeFromTop(sliderHeight));

        col2.removeFromTop(spacing * 3);  // Extra padding before Sends to Reverbs

        // Mute Sends to Reverbs (centered in column)
        auto reverbSendsRow = col2.removeFromTop(rowHeight);
        int buttonCenterX = reverbSendsRow.getX() + reverbSendsRow.getWidth() / 2;
        muteReverbSendsButton.setBounds(buttonCenterX - 100, reverbSendsRow.getY(), 200, rowHeight);
    }

    void layoutMovementsTab()
    {
        // 2-column layout: Column 1 (LFO), Column 2 (AutomOtion)
        auto area = subTabContentArea;
        const int rowHeight = 22;
        const int sliderHeight = 20;
        const int spacing = 4;
        const int labelWidth = 65;
        const int valueWidth = 55;
        const int selectorWidth = 90;
        const int dialSize = 50;
        const int buttonWidth = 95;
        const int transportButtonSize = 35;

        auto col1 = area.removeFromLeft(area.getWidth() / 2).reduced(5, 0);
        auto col2 = area.reduced(5, 0);

        // ========== COLUMN 1: LFO (new compact layout) ==========

        // --- Header row: Toggle, Period dial, Phase dial, Progress dial, Gyrophone (full width) ---
        const int headerDialSize = 40;
        const int headerLabelHeight = 16;
        const int headerValueHeight = 16;
        const int headerRowHeight = headerLabelHeight + headerDialSize + headerValueHeight;
        auto headerRow = col1.removeFromTop(headerRowHeight);
        const int headerWidth = headerRow.getWidth();

        // Divide header into 5 sections
        const int toggleWidth = 70;
        const int dialBlockWidth = headerDialSize + 15;
        const int gyroWidth = selectorWidth + 10;
        const int headerSpacing = (headerWidth - toggleWidth - 3 * dialBlockWidth - gyroWidth) / 4;

        // Calculate vertical center for UI elements (below labels)
        const int uiCenterY = headerLabelHeight + headerDialSize / 2;

        // Toggle button at left (vertically centered with dials)
        auto toggleArea = headerRow.removeFromLeft(toggleWidth);
        int toggleY = uiCenterY - rowHeight / 2;
        lfoActiveButton.setBounds(toggleArea.getX(), headerRow.getY() + toggleY, toggleWidth, rowHeight);
        headerRow.removeFromLeft(headerSpacing);

        // Period: label at top, dial centered, value+unit at bottom
        auto periodArea = headerRow.removeFromLeft(dialBlockWidth);
        lfoPeriodLabel.setBounds(periodArea.removeFromTop(headerLabelHeight));
        auto periodDialBounds = periodArea.removeFromTop(headerDialSize);
        lfoPeriodDial.setBounds(periodDialBounds.withSizeKeepingCentre(headerDialSize, headerDialSize));
        int periodCenterX = periodDialBounds.getX() + periodDialBounds.getWidth() / 2;
        layoutDialValueUnit(lfoPeriodValueLabel, lfoPeriodUnitLabel, periodCenterX, periodArea.getY(), periodArea.getHeight(), 32, 25);
        headerRow.removeFromLeft(headerSpacing);

        // Phase: label at top, dial centered, value+unit at bottom
        auto phaseArea = headerRow.removeFromLeft(dialBlockWidth);
        lfoPhaseLabel.setBounds(phaseArea.removeFromTop(headerLabelHeight));
        auto phaseDialBounds = phaseArea.removeFromTop(headerDialSize);
        lfoPhaseDial.setBounds(phaseDialBounds.withSizeKeepingCentre(headerDialSize, headerDialSize));
        int phaseCenterX = phaseDialBounds.getX() + phaseDialBounds.getWidth() / 2;
        layoutDialValueUnit(lfoPhaseValueLabel, lfoPhaseUnitLabel, phaseCenterX, phaseArea.getY(), phaseArea.getHeight(), 35, 20);
        headerRow.removeFromLeft(headerSpacing);

        // Progress indicator (no label, aligned with dials)
        auto progressArea = headerRow.removeFromLeft(dialBlockWidth);
        progressArea.removeFromTop(headerLabelHeight);  // Align with other dials
        lfoProgressDial.setBounds(progressArea.removeFromTop(headerDialSize).withSizeKeepingCentre(headerDialSize, headerDialSize));
        headerRow.removeFromLeft(headerSpacing);

        // Gyrophone selector at right of header (label at top, selector centered with dials)
        auto gyroArea = headerRow;
        lfoGyrophoneLabel.setBounds(gyroArea.removeFromTop(headerLabelHeight));
        int selectorY = (headerDialSize - rowHeight) / 2;  // Center selector vertically in dial area
        auto selectorRect = gyroArea.removeFromTop(headerDialSize);
        lfoGyrophoneSelector.setBounds(selectorRect.getX(), selectorRect.getY() + selectorY, selectorWidth, rowHeight);

        col1.removeFromTop(spacing);

        // --- Axis rows: X, Y, Z ---
        // Each row: [Shape selector] [Amp/Rate sliders stacked] [Phase dial] [Output slider]
        // All sliders same width
        const int axisRowHeight = 92;   // Height for amp/rate stack with increased spacing
        const int axisRowSpacing = 24;  // Doubled spacing between axis rows
        const int ampRateSpacing = 8;   // Doubled spacing between amp and rate sliders
        const int axisDial = 40;
        const int shapeWidth = 75;
        const int phaseDialWidth = axisDial + 25;  // Wider to fit "Phase X:" label

        // Calculate uniform slider width: total width minus fixed elements, divided among 3 sliders
        const int fixedWidth = shapeWidth + phaseDialWidth + spacing * 4;
        const int totalSliderSpace = col1.getWidth() - fixedWidth;
        const int uniformSliderWidth = totalSliderSpace / 3;  // Amp, Rate (stacked), and Out share space

        // Helper lambda to layout an axis row
        auto layoutAxisRow = [&](juce::Label& shapeLabel, juce::ComboBox& shapeSelector,
                                  juce::Label& ampLabel, WfsStandardSlider& ampSlider, juce::Label& ampValue,
                                  juce::Label& rateLabel, WfsStandardSlider& rateSlider, juce::Label& rateValue,
                                  juce::Label& phaseLabel, WfsRotationDial& phaseDial, juce::Label& phaseValue, juce::Label& phaseUnit,
                                  juce::Label& outLabel, WfsLFOOutputSlider& outSlider) {
            auto axisRow = col1.removeFromTop(axisRowHeight);

            // Shape selector (left) - vertically centered
            auto shapeArea = axisRow.removeFromLeft(shapeWidth);
            const int shapeBlockHeight = rowHeight + rowHeight;  // label + selector
            const int shapeCenterOffset = (axisRowHeight - shapeBlockHeight) / 2;
            shapeArea.removeFromTop(shapeCenterOffset);
            shapeLabel.setBounds(shapeArea.removeFromTop(rowHeight));
            shapeSelector.setBounds(shapeArea.removeFromTop(rowHeight).withWidth(shapeWidth - 5));
            axisRow.removeFromLeft(spacing);

            // Amplitude + Rate sliders (stacked) - use 2/3 of slider space
            const int ampRateWidth = uniformSliderWidth * 2;
            auto sliderArea = axisRow.removeFromLeft(ampRateWidth);
            // Amplitude row
            auto ampRow = sliderArea.removeFromTop(rowHeight);
            ampLabel.setBounds(ampRow.removeFromLeft(70));  // Wider to fit "Amplitude:"
            ampValue.setBounds(ampRow.removeFromRight(50));
            ampSlider.setBounds(sliderArea.removeFromTop(sliderHeight));
            sliderArea.removeFromTop(ampRateSpacing);  // Doubled spacing between amp and rate
            // Rate row
            auto rateRow = sliderArea.removeFromTop(rowHeight);
            rateLabel.setBounds(rateRow.removeFromLeft(70));  // Match amplitude label width
            rateValue.setBounds(rateRow.removeFromRight(50));
            rateSlider.setBounds(sliderArea.removeFromTop(sliderHeight));
            axisRow.removeFromLeft(spacing);

            // Phase dial - vertically centered with split value/unit
            auto phaseDialArea = axisRow.removeFromLeft(phaseDialWidth);
            const int phaseBlockHeight = rowHeight + axisDial + rowHeight - 4;  // label + dial + value
            const int phaseCenterOffset = (axisRowHeight - phaseBlockHeight) / 2;
            phaseDialArea.removeFromTop(phaseCenterOffset);
            phaseLabel.setBounds(phaseDialArea.removeFromTop(rowHeight - 2));
            auto axisPhaseDialBounds = phaseDialArea.removeFromTop(axisDial);
            phaseDial.setBounds(axisPhaseDialBounds.withSizeKeepingCentre(axisDial, axisDial));
            int axisPhaseDialCenterX = axisPhaseDialBounds.getX() + axisPhaseDialBounds.getWidth() / 2;
            layoutDialValueUnit(phaseValue, phaseUnit, axisPhaseDialCenterX, phaseDialArea.getY(), rowHeight - 2, 35, 20);
            axisRow.removeFromLeft(spacing);

            // Output slider - use remaining space (1/3 of slider space)
            auto outArea = axisRow;
            outLabel.setBounds(outArea.removeFromTop(rowHeight));
            outSlider.setBounds(outArea.removeFromTop(sliderHeight * 2));
        };

        // X axis row
        layoutAxisRow(lfoShapeXLabel, lfoShapeXSelector,
                      lfoAmplitudeXLabel, lfoAmplitudeXSlider, lfoAmplitudeXValueLabel,
                      lfoRateXLabel, lfoRateXSlider, lfoRateXValueLabel,
                      lfoPhaseXLabel, lfoPhaseXDial, lfoPhaseXValueLabel, lfoPhaseXUnitLabel,
                      lfoOutputXLabel, lfoOutputXSlider);
        col1.removeFromTop(axisRowSpacing);

        // Y axis row
        layoutAxisRow(lfoShapeYLabel, lfoShapeYSelector,
                      lfoAmplitudeYLabel, lfoAmplitudeYSlider, lfoAmplitudeYValueLabel,
                      lfoRateYLabel, lfoRateYSlider, lfoRateYValueLabel,
                      lfoPhaseYLabel, lfoPhaseYDial, lfoPhaseYValueLabel, lfoPhaseYUnitLabel,
                      lfoOutputYLabel, lfoOutputYSlider);
        col1.removeFromTop(axisRowSpacing);

        // Z axis row
        layoutAxisRow(lfoShapeZLabel, lfoShapeZSelector,
                      lfoAmplitudeZLabel, lfoAmplitudeZSlider, lfoAmplitudeZValueLabel,
                      lfoRateZLabel, lfoRateZSlider, lfoRateZValueLabel,
                      lfoPhaseZLabel, lfoPhaseZDial, lfoPhaseZValueLabel, lfoPhaseZUnitLabel,
                      lfoOutputZLabel, lfoOutputZSlider);

        // --- Jitter slider at bottom (separate effect, more spacing) ---
        col1.removeFromTop(spacing * 8);  // Much more spacing to separate from LFO
        auto row = col1.removeFromTop(rowHeight);
        jitterLabel.setBounds(row.removeFromLeft(labelWidth));
        jitterValueLabel.setBounds(row.removeFromRight(valueWidth));
        jitterSlider.setBounds(col1.removeFromTop(sliderHeight));

        // ========== COLUMN 2: AutomOtion ==========
        const int otomoRowSpacing = spacing * 5;  // Large vertical padding between rows

        // Title row
        row = col2.removeFromTop(rowHeight + 4);  // Slightly taller for title
        otomoTitleLabel.setBounds(row);
        col2.removeFromTop(otomoRowSpacing);

        // Row 1: Destination row spread across full column width
        // [Coord Mode ▼] [X: [__] m] [Y: [__] m] [Z: [__] m] [Absolute] [Stay]
        const int otomoSelectorWidth = 90;   // Wider combobox
        const int otomoToggleWidth = 80;     // Wider toggles
        const int compactLabelWidth = 24;    // Wide enough for Greek letters (θ:, φ:)
        const int compactEditorWidth = 55;   // Narrower number boxes
        const int compactUnitWidth = 22;     // Wider for unit display

        // Calculate spacing to spread elements across column
        int row1FixedWidth = otomoSelectorWidth + (compactLabelWidth + compactEditorWidth + compactUnitWidth) * 3 + otomoToggleWidth * 2;
        int row1AvailableSpace = col2.getWidth() - row1FixedWidth;
        int row1ElementSpacing = row1AvailableSpace / 6;  // 6 gaps between elements

        row = col2.removeFromTop(rowHeight);
        otomoCoordModeSelector.setBounds(row.removeFromLeft(otomoSelectorWidth));
        row.removeFromLeft(row1ElementSpacing);
        otomoDestXLabel.setBounds(row.removeFromLeft(compactLabelWidth));
        otomoDestXEditor.setBounds(row.removeFromLeft(compactEditorWidth));
        otomoDestXUnitLabel.setBounds(row.removeFromLeft(compactUnitWidth));
        row.removeFromLeft(row1ElementSpacing);
        otomoDestYLabel.setBounds(row.removeFromLeft(compactLabelWidth));
        otomoDestYEditor.setBounds(row.removeFromLeft(compactEditorWidth));
        otomoDestYUnitLabel.setBounds(row.removeFromLeft(compactUnitWidth));
        row.removeFromLeft(row1ElementSpacing);
        otomoDestZLabel.setBounds(row.removeFromLeft(compactLabelWidth));
        otomoDestZEditor.setBounds(row.removeFromLeft(compactEditorWidth));
        otomoDestZUnitLabel.setBounds(row.removeFromLeft(compactUnitWidth));
        row.removeFromLeft(row1ElementSpacing);
        otomoAbsRelButton.setBounds(row.removeFromLeft(otomoToggleWidth));
        row.removeFromLeft(row1ElementSpacing);
        otomoStayReturnButton.setBounds(row.removeFromLeft(otomoToggleWidth));
        col2.removeFromTop(otomoRowSpacing);

        // Row 2: Dials - Duration, Curve (if Cartesian), Speed Profile - spread across column
        const int otomoDialWidth = dialSize + 30;  // Wide enough for "Speed Profile:" label
        int row2FixedWidth = otomoDialWidth * 3;
        int row2AvailableSpace = col2.getWidth() - row2FixedWidth;
        int row2ElementSpacing = row2AvailableSpace / 4;  // Spacing at start, between dials, at end

        auto otomoDials1 = col2.removeFromTop(dialSize + rowHeight * 2 - 5);
        otomoDials1.removeFromLeft(row2ElementSpacing);  // Left padding

        auto durDialArea = otomoDials1.removeFromLeft(otomoDialWidth);
        otomoDurationLabel.setBounds(durDialArea.removeFromTop(rowHeight));
        otomoDurationDial.setBounds(durDialArea.removeFromTop(dialSize).withSizeKeepingCentre(dialSize, dialSize));
        otomoDurationValueLabel.setBounds(durDialArea.removeFromTop(rowHeight));
        otomoDials1.removeFromLeft(row2ElementSpacing);

        auto curveDialArea = otomoDials1.removeFromLeft(otomoDialWidth);
        otomoCurveLabel.setBounds(curveDialArea.removeFromTop(rowHeight));
        auto curveDialBounds = curveDialArea.removeFromTop(dialSize);
        otomoCurveDial.setBounds(curveDialBounds.withSizeKeepingCentre(dialSize, dialSize));
        int curveCenterX = curveDialBounds.getX() + curveDialBounds.getWidth() / 2;
        layoutDialValueUnit(otomoCurveValueLabel, otomoCurveUnitLabel, curveCenterX, curveDialArea.getY(), rowHeight, 30, 25);
        otomoDials1.removeFromLeft(row2ElementSpacing);

        auto speedDialArea = otomoDials1.removeFromLeft(otomoDialWidth);
        otomoSpeedProfileLabel.setBounds(speedDialArea.removeFromTop(rowHeight));
        auto speedDialBounds = speedDialArea.removeFromTop(dialSize);
        otomoSpeedProfileDial.setBounds(speedDialBounds.withSizeKeepingCentre(dialSize, dialSize));
        int speedCenterX = speedDialBounds.getX() + speedDialBounds.getWidth() / 2;
        layoutDialValueUnit(otomoSpeedProfileValueLabel, otomoSpeedProfileUnitLabel, speedCenterX, speedDialArea.getY(), rowHeight, 30, 25);
        col2.removeFromTop(otomoRowSpacing);

        // Row 3: Trigger row - [Manual/Trigger button] [Threshold dial] [Reset dial] - spread across column
        const int triggerDialSize = 50;
        const int triggerDialWidth = triggerDialSize + 30;
        int row3FixedWidth = buttonWidth + triggerDialWidth * 2;
        int row3AvailableSpace = col2.getWidth() - row3FixedWidth;
        int row3ElementSpacing = row3AvailableSpace / 4;  // Spacing at start, between elements, at end

        auto triggerRow = col2.removeFromTop(triggerDialSize + rowHeight * 2 - 5);
        triggerRow.removeFromLeft(row3ElementSpacing);  // Left padding

        // Trigger button, vertically centered
        auto triggerBtnArea = triggerRow.removeFromLeft(buttonWidth);
        int triggerBtnY = (triggerRow.getHeight() - rowHeight) / 2;
        otomoTriggerButton.setBounds(triggerBtnArea.getX(), triggerRow.getY() + triggerBtnY, buttonWidth, rowHeight);
        triggerRow.removeFromLeft(row3ElementSpacing);

        // Threshold dial
        auto threshDialArea = triggerRow.removeFromLeft(triggerDialWidth);
        otomoThresholdLabel.setBounds(threshDialArea.removeFromTop(rowHeight));
        auto threshDialBounds = threshDialArea.removeFromTop(triggerDialSize);
        otomoThresholdDial.setBounds(threshDialBounds.withSizeKeepingCentre(triggerDialSize, triggerDialSize));
        int threshCenterX = threshDialBounds.getX() + threshDialBounds.getWidth() / 2;
        layoutDialValueUnit(otomoThresholdValueLabel, otomoThresholdUnitLabel, threshCenterX, threshDialArea.getY(), rowHeight, 42, 30);
        triggerRow.removeFromLeft(row3ElementSpacing);

        // Reset dial
        auto resetDialArea = triggerRow.removeFromLeft(triggerDialWidth);
        otomoResetLabel.setBounds(resetDialArea.removeFromTop(rowHeight));
        auto resetDialBounds = resetDialArea.removeFromTop(triggerDialSize);
        otomoResetDial.setBounds(resetDialBounds.withSizeKeepingCentre(triggerDialSize, triggerDialSize));
        int resetCenterX = resetDialBounds.getX() + resetDialBounds.getWidth() / 2;
        layoutDialValueUnit(otomoResetValueLabel, otomoResetUnitLabel, resetCenterX, resetDialArea.getY(), rowHeight, 42, 30);
        col2.removeFromTop(otomoRowSpacing);

        // Row 4: Transport buttons - spread across column
        int row4FixedWidth = transportButtonSize * 3 + buttonWidth * 2;
        int row4AvailableSpace = col2.getWidth() - row4FixedWidth;
        int row4ElementSpacing = row4AvailableSpace / 6;  // Spacing between elements

        row = col2.removeFromTop(transportButtonSize);
        row.removeFromLeft(row4ElementSpacing);  // Left padding
        otomoStartButton.setBounds(row.removeFromLeft(transportButtonSize));
        row.removeFromLeft(row4ElementSpacing);
        otomoPauseButton.setBounds(row.removeFromLeft(transportButtonSize));
        row.removeFromLeft(row4ElementSpacing);
        otomoStopButton.setBounds(row.removeFromLeft(transportButtonSize));
        row.removeFromLeft(row4ElementSpacing);
        otomoStopAllButton.setBounds(row.removeFromLeft(buttonWidth));
        row.removeFromLeft(row4ElementSpacing);
        otomoPauseResumeAllButton.setBounds(row.removeFromLeft(buttonWidth));
    }

    void setMutesVisible(bool v)
    {
        int numOutputs = parameters.getNumOutputChannels();
        if (numOutputs <= 0) numOutputs = 16;  // Default

        for (int i = 0; i < 64; ++i)
            muteButtons[i].setVisible(v && i < numOutputs);
        muteMacrosLabel.setVisible(v);
        muteMacrosSelector.setVisible(v);

        // Array attenuation controls
        arrayAttenLabel.setVisible(v);

        // Check which arrays have outputs assigned
        std::array<bool, 10> arrayHasOutputs = {false};
        for (int outIdx = 0; outIdx < numOutputs; ++outIdx)
        {
            int arrayNum = static_cast<int>(parameters.getOutputParam(outIdx, "outputArray"));
            if (arrayNum >= 1 && arrayNum <= 10)
                arrayHasOutputs[static_cast<size_t>(arrayNum - 1)] = true;
        }

        for (int i = 0; i < 10; ++i)
        {
            arrayAttenDialLabels[i].setVisible(v);
            arrayAttenDials[i].setVisible(v);
            arrayAttenValueLabels[i].setVisible(v);

            // Dim controls for empty arrays (alpha = 0.3 for empty, 1.0 for populated)
            float alpha = arrayHasOutputs[static_cast<size_t>(i)] ? 1.0f : 0.3f;
            arrayAttenDialLabels[i].setAlpha(alpha);
            arrayAttenDials[i].setAlpha(alpha);
            arrayAttenValueLabels[i].setAlpha(alpha);
        }

        // Sidelines controls
        sidelinesActiveButton.setVisible(v);
        sidelinesFringeLabel.setVisible(v);
        sidelinesFringeDial.setVisible(v);
        sidelinesFringeValueLabel.setVisible(v);
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

        area.removeFromTop(20);

        // Array Attenuation section
        const int dialSize = 50;
        const int dialSpacing = 8;
        const int labelHeight = 18;
        const int valueHeight = 18;
        const int dialTotalHeight = labelHeight + dialSize + valueHeight;

        // Section label
        auto labelRow = area.removeFromTop(rowHeight);
        arrayAttenLabel.setBounds(labelRow.removeFromLeft(150));

        area.removeFromTop(5);

        // Layout 10 dials in a single row
        auto dialsRow = area.removeFromTop(dialTotalHeight);
        for (int i = 0; i < 10; ++i)
        {
            auto dialArea = dialsRow.removeFromLeft(dialSize + dialSpacing);
            dialArea.removeFromRight(dialSpacing);

            arrayAttenDialLabels[i].setBounds(dialArea.removeFromTop(labelHeight));
            auto dialRect = dialArea.removeFromTop(dialSize);
            arrayAttenDials[i].setBounds(dialRect.withSizeKeepingCentre(dialSize, dialSize));
            arrayAttenValueLabels[i].setBounds(dialArea.removeFromTop(valueHeight));
        }

        area.removeFromTop(20);

        // Sidelines section
        auto sidelinesRow = area.removeFromTop(rowHeight + 10);
        sidelinesActiveButton.setBounds(sidelinesRow.removeFromLeft(100));
        sidelinesRow.removeFromLeft(20);
        sidelinesFringeLabel.setBounds(sidelinesRow.removeFromLeft(50));
        sidelinesFringeDial.setBounds(sidelinesRow.removeFromLeft(50));
        sidelinesRow.removeFromLeft(5);
        sidelinesFringeValueLabel.setBounds(sidelinesRow.removeFromLeft(70));
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
        // Update coordinate mode selector and position editors (handles coordinate conversion)
        updatePositionLabelsAndValues();
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

        // Distance constraint (for Cylindrical/Spherical modes)
        bool constDist = getIntParam(WFSParameterIDs::inputConstraintDistance, 0) != 0;
        constraintDistanceButton.setToggleState(constDist, juce::dontSendNotification);
        constraintDistanceButton.setButtonText(constDist ? "Constraint R: ON" : "Constraint R: OFF");
        float distMin = getFloatParam(WFSParameterIDs::inputConstraintDistanceMin, 0.0f);
        float distMax = getFloatParam(WFSParameterIDs::inputConstraintDistanceMax, 50.0f);
        distanceRangeSlider.setValues(distMin, distMax);
        distanceMinEditor.setText(juce::String(distanceRangeSlider.getMinValue(), 2), juce::dontSendNotification);
        distanceMaxEditor.setText(juce::String(distanceRangeSlider.getMaxValue(), 2), juce::dontSendNotification);
        // Dim slider when constraint is off
        distanceRangeSlider.setEnabled(constDist);
        distanceMinEditor.setEnabled(constDist);
        distanceMaxEditor.setEnabled(constDist);
        updateConstraintVisibility();
        resized();  // Trigger layout update for visibility changes

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

        trackingIdSelector.setSelectedId(getIntParam(WFSParameterIDs::inputTrackingID, 1), juce::dontSendNotification);

        // Tracking Smooth stored as percent (0-100), default 0%
        float trackSmoothPct = getFloatParam(WFSParameterIDs::inputTrackingSmooth, 0.0f);
        trackSmoothPct = juce::jlimit(0.0f, 100.0f, trackSmoothPct);
        trackingSmoothDial.setValue(trackSmoothPct / 100.0f);
        trackingSmoothValueLabel.setText(juce::String(static_cast<int>(trackSmoothPct)), juce::dontSendNotification);

        bool maxSpeedActive = getIntParam(WFSParameterIDs::inputMaxSpeedActive, 0) != 0;
        maxSpeedActiveButton.setToggleState(maxSpeedActive, juce::dontSendNotification);
        maxSpeedActiveButton.setButtonText(maxSpeedActive ? "Max Speed: ON" : "Max Speed: OFF");

        // Max Speed stored as m/s (0.01-20.0), default ~10 m/s
        // Inverse of: speed = v * 19.99 + 0.01 => v = (speed - 0.01) / 19.99
        float maxSpeedMs = getFloatParam(WFSParameterIDs::inputMaxSpeed, 10.0f);
        maxSpeedMs = juce::jlimit(0.01f, 20.0f, maxSpeedMs);
        float maxSpeedSliderVal = (maxSpeedMs - 0.01f) / 19.99f;
        maxSpeedDial.setValue(juce::jlimit(0.0f, 1.0f, maxSpeedSliderVal));
        maxSpeedValueLabel.setText(juce::String(maxSpeedMs, 2), juce::dontSendNotification);

        bool pathModeActive = getIntParam(WFSParameterIDs::inputPathModeActive, 0) != 0;
        pathModeButton.setToggleState(pathModeActive, juce::dontSendNotification);
        pathModeButton.setButtonText(pathModeActive ? "Path Mode: ON" : "Path Mode: OFF");

        // Height Factor stored as percent (0-100), default 100%
        float heightFactorPct = getFloatParam(WFSParameterIDs::inputHeightFactor, 100.0f);
        heightFactorPct = juce::jlimit(0.0f, 100.0f, heightFactorPct);
        heightFactorDial.setValue(heightFactorPct / 100.0f);
        heightFactorValueLabel.setText(juce::String(static_cast<int>(heightFactorPct)), juce::dontSendNotification);

        // ==================== SOUND TAB ====================
        bool attenLaw = getIntParam(WFSParameterIDs::inputAttenuationLaw, 0) != 0;
        attenuationLawButton.setToggleState(attenLaw, juce::dontSendNotification);
        attenuationLawButton.setButtonText(attenLaw ? "1/d" : "Log");
        // Update dial visibility based on attenuation law (Log shows dB/m, 1/d shows ratio)
        bool showInputParams = subTabBar.getCurrentTabIndex() == 0;
        distanceAttenLabel.setVisible(!attenLaw && showInputParams);
        distanceAttenDial.setVisible(!attenLaw && showInputParams);
        distanceAttenValueLabel.setVisible(!attenLaw && showInputParams);
        distanceAttenUnitLabel.setVisible(!attenLaw && showInputParams);
        distanceRatioLabel.setVisible(attenLaw && showInputParams);
        distanceRatioDial.setVisible(attenLaw && showInputParams);
        distanceRatioValueLabel.setVisible(attenLaw && showInputParams);
        distanceRatioUnitLabel.setVisible(attenLaw && showInputParams);

        // Distance Attenuation stored as dB/m (-6 to 0), default -0.7
        // Formula: dB = (x * 6.0) - 6.0 => x = (dB + 6) / 6
        float distAttenDB = getFloatParam(WFSParameterIDs::inputDistanceAttenuation, -0.7f);
        distAttenDB = juce::jlimit(-6.0f, 0.0f, distAttenDB);
        float distAttenSliderVal = (distAttenDB + 6.0f) / 6.0f;
        distanceAttenDial.setValue(juce::jlimit(0.0f, 1.0f, distAttenSliderVal));
        distanceAttenValueLabel.setText(juce::String(distAttenDB, 1), juce::dontSendNotification);

        // Distance Ratio stored as multiplier (0.1 to 10), default 1.0
        // Formula: ratio = pow(10, (x * 2) - 1) => x = (log10(ratio) + 1) / 2
        float distRatioVal = getFloatParam(WFSParameterIDs::inputDistanceRatio, 1.0f);
        distRatioVal = juce::jlimit(0.1f, 10.0f, distRatioVal);
        float distRatioSliderVal = (std::log10(distRatioVal) + 1.0f) / 2.0f;
        distanceRatioDial.setValue(juce::jlimit(0.0f, 1.0f, distRatioSliderVal));
        distanceRatioValueLabel.setText(juce::String(distRatioVal, 2), juce::dontSendNotification);

        // Common Attenuation stored as percent (0-100), default 100
        // Formula: percent = x * 100 => x = percent / 100
        float commonAttenPct = getFloatParam(WFSParameterIDs::inputCommonAtten, 100.0f);
        commonAttenPct = juce::jlimit(0.0f, 100.0f, commonAttenPct);
        commonAttenDial.setValue(commonAttenPct / 100.0f);
        commonAttenValueLabel.setText(juce::String(static_cast<int>(commonAttenPct)), juce::dontSendNotification);

        // Directivity stored as degrees (2-360), default 360
        // Inverse of: degrees = (x * 358) + 2 => x = (degrees - 2) / 358
        float directivityDeg = getFloatParam(WFSParameterIDs::inputDirectivity, 360.0f);
        directivityDeg = juce::jlimit(2.0f, 360.0f, directivityDeg);
        float directivitySliderVal = (directivityDeg - 2.0f) / 358.0f;
        directivitySlider.setValue(juce::jlimit(0.0f, 1.0f, directivitySliderVal));
        directivityValueLabel.setText(juce::String(static_cast<int>(directivityDeg)) + juce::String::fromUTF8("°"), juce::dontSendNotification);
        inputDirectivityDial.setDirectivity(directivityDeg);

        // Rotation stored as degrees (-180 to 180)
        float rotation = getFloatParam(WFSParameterIDs::inputRotation, 0.0f);
        inputDirectivityDial.setRotation(rotation);
        rotationValueLabel.setText(juce::String(static_cast<int>(rotation)), juce::dontSendNotification);

        // Tilt stored as degrees (-90 to 90), default 0
        // Bidirectional slider: v = degrees / 90 (maps -90..90 to -1..1)
        float tiltDeg = getFloatParam(WFSParameterIDs::inputTilt, 0.0f);
        tiltDeg = juce::jlimit(-90.0f, 90.0f, tiltDeg);
        float tiltSliderVal = tiltDeg / 90.0f;
        tiltSlider.setValue(juce::jlimit(-1.0f, 1.0f, tiltSliderVal));
        tiltValueLabel.setText(juce::String(static_cast<int>(tiltDeg)) + juce::String::fromUTF8("°"), juce::dontSendNotification);

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
        inputDirectivityDial.setHfShelf(hfShelfDB);

        // ==================== LIVE SOURCE TAB ====================
        bool lsActive = getIntParam(WFSParameterIDs::inputLSactive, 0) != 0;
        lsActiveButton.setToggleState(lsActive, juce::dontSendNotification);
        lsActiveButton.setButtonText(lsActive ? LOC("inputs.toggles.liveSourceTamerOn") : LOC("inputs.toggles.liveSourceTamerOff"));
        setLiveSourceParametersAlpha(lsActive ? 1.0f : 0.5f);

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
        lsPeakRatioValueLabel.setText(juce::String(peakRatioVal, 1), juce::dontSendNotification);

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
        lsSlowRatioValueLabel.setText(juce::String(slowRatioVal, 1), juce::dontSendNotification);

        // ==================== EFFECTS (HACKOUSTICS) TAB ====================
        bool frActive = getIntParam(WFSParameterIDs::inputFRactive, 0) != 0;
        frActiveButton.setToggleState(frActive, juce::dontSendNotification);
        frActiveButton.setButtonText(frActive ? LOC("inputs.toggles.floorReflectionsOn") : LOC("inputs.toggles.floorReflectionsOff"));
        setFloorReflectionsParametersAlpha(frActive ? 1.0f : 0.5f);

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
        frDiffusionValueLabel.setText(juce::String(static_cast<int>(frDiffusionPct)), juce::dontSendNotification);

        bool frLowCutActive = getIntParam(WFSParameterIDs::inputFRlowCutActive, 0) != 0;
        frLowCutActiveButton.setToggleState(frLowCutActive, juce::dontSendNotification);
        frLowCutActiveButton.setButtonText(frLowCutActive ? LOC("inputs.toggles.lowCutOn") : LOC("inputs.toggles.lowCutOff"));

        // FR Low Cut Freq stored as Hz (20-20000), default 100
        // Formula: freq = 20 * pow(10, 3*x) => x = log10(freq/20) / 3
        float frLowCutFreqHz = getFloatParam(WFSParameterIDs::inputFRlowCutFreq, 100.0f);
        frLowCutFreqHz = juce::jlimit(20.0f, 20000.0f, frLowCutFreqHz);
        float frLowCutSliderVal = std::log10(frLowCutFreqHz / 20.0f) / 3.0f;
        frLowCutFreqSlider.setValue(juce::jlimit(0.0f, 1.0f, frLowCutSliderVal));
        frLowCutFreqValueLabel.setText(juce::String(static_cast<int>(frLowCutFreqHz)) + " Hz", juce::dontSendNotification);

        bool frHighShelfActive = getIntParam(WFSParameterIDs::inputFRhighShelfActive, 0) != 0;
        frHighShelfActiveButton.setToggleState(frHighShelfActive, juce::dontSendNotification);
        frHighShelfActiveButton.setButtonText(frHighShelfActive ? LOC("inputs.toggles.highShelfOn") : LOC("inputs.toggles.highShelfOff"));

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

        // Update Low Cut and High Shelf alpha based on master FR and sub-toggle states
        updateLowCutAlpha();
        updateHighShelfAlpha();

        // Mute Sends to Reverbs (default OFF = 0)
        bool muteReverbSends = getIntParam(WFSParameterIDs::inputMuteReverbSends, 0) != 0;
        muteReverbSendsButton.setToggleState(muteReverbSends, juce::dontSendNotification);
        muteReverbSendsButton.setButtonText(muteReverbSends ? LOC("inputs.toggles.reverbSendsMuted") : LOC("inputs.toggles.reverbSendsUnmuted"));

        // ==================== LFO TAB ====================
        bool lfoActive = getIntParam(WFSParameterIDs::inputLFOactive, 0) != 0;
        lfoActiveButton.setToggleState(lfoActive, juce::dontSendNotification);
        lfoActiveButton.setButtonText(lfoActive ? LOC("inputs.toggles.lfoOn") : LOC("inputs.toggles.lfoOff"));

        // LFO Period stored as seconds (0.01-100), default 5.0s
        float lfoPeriodSec = getFloatParam(WFSParameterIDs::inputLFOperiod, 5.0f);
        lfoPeriodSec = juce::jlimit(0.01f, 100.0f, lfoPeriodSec);
        // Inverse of: period = pow(10, sqrt(v)*4 - 2) => v = pow((log10(period)+2)/4, 2)
        float lfoPeriodSlider = std::pow((std::log10(lfoPeriodSec) + 2.0f) / 4.0f, 2.0f);
        lfoPeriodDial.setValue(juce::jlimit(0.0f, 1.0f, lfoPeriodSlider));
        lfoPeriodValueLabel.setText(juce::String(lfoPeriodSec, 2), juce::dontSendNotification);

        // LFO Phase stored as degrees (-180 to 180), default 0
        int lfoPhaseDeg = getIntParam(WFSParameterIDs::inputLFOphase, 0);
        lfoPhaseDeg = juce::jlimit(-180, 180, lfoPhaseDeg);
        lfoPhaseDial.setAngle(static_cast<float>(lfoPhaseDeg));
        lfoPhaseValueLabel.setText(juce::String(lfoPhaseDeg), juce::dontSendNotification);

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

        // LFO Phase X/Y/Z stored as degrees (-180 to 180), default 0
        int phaseXDeg = getIntParam(WFSParameterIDs::inputLFOphaseX, 0);
        phaseXDeg = juce::jlimit(-180, 180, phaseXDeg);
        lfoPhaseXDial.setAngle(static_cast<float>(phaseXDeg));
        lfoPhaseXValueLabel.setText(juce::String(phaseXDeg), juce::dontSendNotification);

        int phaseYDeg = getIntParam(WFSParameterIDs::inputLFOphaseY, 0);
        phaseYDeg = juce::jlimit(-180, 180, phaseYDeg);
        lfoPhaseYDial.setAngle(static_cast<float>(phaseYDeg));
        lfoPhaseYValueLabel.setText(juce::String(phaseYDeg), juce::dontSendNotification);

        int phaseZDeg = getIntParam(WFSParameterIDs::inputLFOphaseZ, 0);
        phaseZDeg = juce::jlimit(-180, 180, phaseZDeg);
        lfoPhaseZDial.setAngle(static_cast<float>(phaseZDeg));
        lfoPhaseZValueLabel.setText(juce::String(phaseZDeg), juce::dontSendNotification);

        lfoGyrophoneSelector.setSelectedId(getIntParam(WFSParameterIDs::inputLFOgyrophone, 0) + 2, juce::dontSendNotification);

        // Jitter stored as meters (0-10), default 0
        // Inverse of: meters = 10 * v^2 => v = sqrt(meters / 10)
        float jitterMeters = getFloatParam(WFSParameterIDs::inputJitter, 0.0f);
        jitterMeters = juce::jlimit(0.0f, 10.0f, jitterMeters);
        float jitterSliderVal = std::sqrt(jitterMeters / 10.0f);
        jitterSlider.setValue(juce::jlimit(0.0f, 1.0f, jitterSliderVal));
        jitterValueLabel.setText(juce::String(jitterMeters, 2) + " m", juce::dontSendNotification);

        // Update LFO section alpha based on active state and shape selections
        updateLfoAlpha();

        // ==================== AUTOMOTION TAB ====================
        // Load coordinate mode first
        int otomoCoordMode = getIntParam(WFSParameterIDs::inputOtomoCoordinateMode, 0);
        otomoCoordModeSelector.setSelectedId(otomoCoordMode + 1, juce::dontSendNotification);  // 1=Cartesian, 2=Cyl, 3=Sph
        updateOtomoLabelsAndValues();
        updateOtomoDestinationEditors();  // Load destination values based on coordinate mode
        updateOtomoCurveVisibility();

        bool absRel = getIntParam(WFSParameterIDs::inputOtomoAbsoluteRelative, 0) != 0;
        otomoAbsRelButton.setToggleState(absRel, juce::dontSendNotification);
        otomoAbsRelButton.setButtonText(absRel ? "Relative" : "Absolute");

        bool stayReturn = getIntParam(WFSParameterIDs::inputOtomoStayReturn, 0) != 0;
        otomoStayReturnButton.setToggleState(stayReturn, juce::dontSendNotification);
        otomoStayReturnButton.setButtonText(stayReturn ? "Return" : "Stay");

        // Duration stored as seconds (0.1-3600), default 5.0
        // Inverse of: duration = pow(10, sqrt(v) * 3.556 - 1)
        float duration = getFloatParam(WFSParameterIDs::inputOtomoDuration, 5.0f);
        duration = juce::jlimit(0.1f, 3600.0f, duration);
        // Inverse: v = (log10(duration) + 1)^2 / 3.556^2
        float durationDial = std::pow((std::log10(duration) + 1.0f) / 3.556f, 2.0f);
        otomoDurationDial.setValue(juce::jlimit(0.0f, 1.0f, durationDial));
        // Format display
        juce::String durationText;
        if (duration < 10.0f)
            durationText = juce::String(duration, 2) + " s";
        else if (duration < 60.0f)
            durationText = juce::String(duration, 1) + " s";
        else if (duration < 3600.0f)
            durationText = juce::String(static_cast<int>(duration / 60)) + "m " + juce::String(static_cast<int>(duration) % 60) + "s";
        else
            durationText = "1h";
        otomoDurationValueLabel.setText(durationText, juce::dontSendNotification);

        // Curve stored as -100 to +100, default 0
        int curve = getIntParam(WFSParameterIDs::inputOtomoCurve, 0);
        curve = juce::jlimit(-100, 100, curve);
        // Inverse of: curve = (v * 200) - 100 => v = (curve + 100) / 200
        float curveDial = (static_cast<float>(curve) + 100.0f) / 200.0f;
        otomoCurveDial.setValue(juce::jlimit(0.0f, 1.0f, curveDial));
        otomoCurveValueLabel.setText(juce::String(curve), juce::dontSendNotification);

        // Speed Profile stored as percent (0-100), default 0
        int speedProfilePct = getIntParam(WFSParameterIDs::inputOtomoSpeedProfile, 0);
        speedProfilePct = juce::jlimit(0, 100, speedProfilePct);
        otomoSpeedProfileDial.setValue(speedProfilePct / 100.0f);
        otomoSpeedProfileValueLabel.setText(juce::String(speedProfilePct), juce::dontSendNotification);

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
        otomoThresholdValueLabel.setText(juce::String(threshDB, 1), juce::dontSendNotification);

        // Reset stored as dB (-92 to 0), default -60 dB
        float resetDB = getFloatParam(WFSParameterIDs::inputOtomoReset, -60.0f);
        resetDB = juce::jlimit(-92.0f, 0.0f, resetDB);
        float resetLinear = std::pow(10.0f, resetDB / 20.0f);
        float resetSlider = std::sqrt((resetLinear - otomoMinLinear) / (1.0f - otomoMinLinear));
        otomoResetDial.setValue(juce::jlimit(0.0f, 1.0f, resetSlider));
        otomoResetValueLabel.setText(juce::String(resetDB, 1), juce::dontSendNotification);

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

        // Array attenuation dials
        static const juce::Identifier arrayAttenIds[10] = {
            WFSParameterIDs::inputArrayAtten1, WFSParameterIDs::inputArrayAtten2,
            WFSParameterIDs::inputArrayAtten3, WFSParameterIDs::inputArrayAtten4,
            WFSParameterIDs::inputArrayAtten5, WFSParameterIDs::inputArrayAtten6,
            WFSParameterIDs::inputArrayAtten7, WFSParameterIDs::inputArrayAtten8,
            WFSParameterIDs::inputArrayAtten9, WFSParameterIDs::inputArrayAtten10
        };
        constexpr float arrayAttenMinLin = 0.001f;  // -60 dB
        for (int i = 0; i < 10; ++i)
        {
            float attenDb = getFloatParam(arrayAttenIds[i], 0.0f);
            attenDb = juce::jlimit(-60.0f, 0.0f, attenDb);
            // Convert dB to dial value using sqrt scaling inverse
            float linear = std::pow(10.0f, attenDb / 20.0f);
            float dialValue = std::sqrt((linear - arrayAttenMinLin) / (1.0f - arrayAttenMinLin));
            arrayAttenDials[i].setValue(juce::jlimit(0.0f, 1.0f, dialValue));
            arrayAttenValueLabels[i].setText(juce::String(attenDb, 1) + " dB", juce::dontSendNotification);
        }

        // Sidelines parameters
        bool sidelinesActive = getIntParam(WFSParameterIDs::inputSidelinesActive, 0) != 0;
        sidelinesActiveButton.setToggleState(sidelinesActive, juce::dontSendNotification);
        sidelinesActiveButton.setButtonText(sidelinesActive ? LOC("inputs.toggles.sidelinesOn") : LOC("inputs.toggles.sidelinesOff"));
        // Grey out dial when inactive
        sidelinesFringeDial.setAlpha(sidelinesActive ? 1.0f : 0.5f);
        sidelinesFringeLabel.setAlpha(sidelinesActive ? 1.0f : 0.5f);
        sidelinesFringeValueLabel.setAlpha(sidelinesActive ? 1.0f : 0.5f);
        float sidelinesFringe = getFloatParam(WFSParameterIDs::inputSidelinesFringe, WFSParameterDefaults::inputSidelinesFringeDefault);
        sidelinesFringe = juce::jlimit(WFSParameterDefaults::inputSidelinesFringeMin, WFSParameterDefaults::inputSidelinesFringeMax, sidelinesFringe);
        // Convert meters to dial value (0-1)
        float dialValue = (sidelinesFringe - WFSParameterDefaults::inputSidelinesFringeMin) /
                          (WFSParameterDefaults::inputSidelinesFringeMax - WFSParameterDefaults::inputSidelinesFringeMin);
        sidelinesFringeDial.setValue(juce::jlimit(0.0f, 1.0f, dialValue));
        sidelinesFringeValueLabel.setText(juce::String(sidelinesFringe, 2) + " m", juce::dontSendNotification);

        // Update visualisation component's selected input
        visualisationComponent.setSelectedInput(currentChannel - 1);

        isLoadingParameters = false;
        updateMapButtonStates();
        updateSoloButtonState();
        updateSoloModeButtonText();
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
            editor.setText(parameters.getInputParam(currentChannel - 1, "inputName").toString(), false);
        else if (&editor == &posXEditor || &editor == &posYEditor || &editor == &posZEditor)
            updatePositionLabelsAndValues();  // Revert all position editors to stored values
        else if (&editor == &offsetXEditor)
            editor.setText(juce::String((float)parameters.getInputParam(currentChannel - 1, "inputOffsetX"), 2), false);
        else if (&editor == &offsetYEditor)
            editor.setText(juce::String((float)parameters.getInputParam(currentChannel - 1, "inputOffsetY"), 2), false);
        else if (&editor == &offsetZEditor)
            editor.setText(juce::String((float)parameters.getInputParam(currentChannel - 1, "inputOffsetZ"), 2), false);
        else if (&editor == &otomoDestXEditor || &editor == &otomoDestYEditor || &editor == &otomoDestZEditor)
            updateOtomoDestinationEditors();  // Revert all destination editors to stored values

        editor.giveAwayKeyboardFocus();
        grabKeyboardFocus();  // Grab focus back so keyboard shortcuts work
    }

    void textEditorFocusLost(juce::TextEditor& editor) override
    {
        if (isLoadingParameters) return;

        // Header - Input Name
        if (&editor == &nameEditor)
        {
            saveInputParam(WFSParameterIDs::inputName, nameEditor.getText());
        }
        // Position tab - Position editors with coordinate conversion and constraint support
        else if (&editor == &posXEditor || &editor == &posYEditor || &editor == &posZEditor)
        {
            // Get all three values from editors
            float v1 = posXEditor.getText().getFloatValue();
            float v2 = posYEditor.getText().getFloatValue();
            float v3 = posZEditor.getText().getFloatValue();

            // Get coordinate mode and convert to Cartesian
            int mode = static_cast<int>(parameters.getInputParam(currentChannel - 1, "inputCoordinateMode"));
            auto coordMode = static_cast<WFSCoordinates::Mode>(mode);
            auto cart = WFSCoordinates::displayToCartesian(coordMode, v1, v2, v3);

            // Check if distance constraint should be used
            bool constrainDist = constraintDistanceButton.getToggleState();
            bool useDistanceConstraint = (mode == 1 || mode == 2) && constrainDist;

            if (useDistanceConstraint)
            {
                // Apply distance constraint (circular for cylindrical, spherical for spherical)
                float minDist = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputConstraintDistanceMin"));
                float maxDist = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputConstraintDistanceMax"));
                float currentDist = (mode == 1)
                    ? std::sqrt(cart.x * cart.x + cart.y * cart.y)
                    : std::sqrt(cart.x * cart.x + cart.y * cart.y + cart.z * cart.z);
                if (currentDist < 0.0001f) currentDist = 0.0001f;
                float targetDist = juce::jlimit(minDist, maxDist, currentDist);
                if (!juce::approximatelyEqual(currentDist, targetDist))
                {
                    float distScale = targetDist / currentDist;
                    cart.x *= distScale;
                    cart.y *= distScale;
                    if (mode == 2)  // Spherical: also scale Z
                        cart.z *= distScale;
                }
                // Apply Z rectangular constraint in cylindrical mode (distance only constrains XY)
                if (mode == 1 && constraintZButton.getToggleState())
                    cart.z = juce::jlimit(getStageMinZ(), getStageMaxZ(), cart.z);
            }
            else
            {
                // Apply rectangular constraints in Cartesian space
                if (constraintXButton.getToggleState())
                    cart.x = juce::jlimit(getStageMinX(), getStageMaxX(), cart.x);
                if (constraintYButton.getToggleState())
                    cart.y = juce::jlimit(getStageMinY(), getStageMaxY(), cart.y);
                if (constraintZButton.getToggleState())
                    cart.z = juce::jlimit(getStageMinZ(), getStageMaxZ(), cart.z);
            }

            // Save Cartesian values
            saveInputParam(WFSParameterIDs::inputPositionX, cart.x);
            saveInputParam(WFSParameterIDs::inputPositionY, cart.y);
            saveInputParam(WFSParameterIDs::inputPositionZ, cart.z);

            // Update display with constrained values (converted back to display coords)
            updatePositionLabelsAndValues();
        }
        // Offset editors - also apply constraints (position + offset must be within bounds)
        else if (&editor == &offsetXEditor || &editor == &offsetYEditor || &editor == &offsetZEditor)
        {
            // Get all offset values
            float offsetX = offsetXEditor.getText().getFloatValue();
            float offsetY = offsetYEditor.getText().getFloatValue();
            float offsetZ = offsetZEditor.getText().getFloatValue();

            // Get all position values
            float posX = posXEditor.getText().getFloatValue();
            float posY = posYEditor.getText().getFloatValue();
            float posZ = posZEditor.getText().getFloatValue();

            // Calculate total positions
            float totalX = posX + offsetX;
            float totalY = posY + offsetY;
            float totalZ = posZ + offsetZ;

            // Check if distance constraint should be used
            int mode = static_cast<int>(parameters.getInputParam(currentChannel - 1, "inputCoordinateMode"));
            bool constrainDist = constraintDistanceButton.getToggleState();
            bool useDistanceConstraint = (mode == 1 || mode == 2) && constrainDist;

            if (useDistanceConstraint)
            {
                // Apply distance constraint (circular for cylindrical, spherical for spherical)
                float minDist = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputConstraintDistanceMin"));
                float maxDist = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputConstraintDistanceMax"));
                float currentDist = (mode == 1)
                    ? std::sqrt(totalX * totalX + totalY * totalY)
                    : std::sqrt(totalX * totalX + totalY * totalY + totalZ * totalZ);
                if (currentDist < 0.0001f) currentDist = 0.0001f;
                float targetDist = juce::jlimit(minDist, maxDist, currentDist);
                if (!juce::approximatelyEqual(currentDist, targetDist))
                {
                    float distScale = targetDist / currentDist;
                    totalX *= distScale;
                    totalY *= distScale;
                    if (mode == 2)  // Spherical: also scale Z
                        totalZ *= distScale;
                    offsetX = totalX - posX;
                    offsetY = totalY - posY;
                    offsetZ = totalZ - posZ;
                }
                // Apply Z rectangular constraint in cylindrical mode
                if (mode == 1 && constraintZButton.getToggleState())
                {
                    totalZ = juce::jlimit(getStageMinZ(), getStageMaxZ(), totalZ);
                    offsetZ = totalZ - posZ;
                }
            }
            else
            {
                // Apply rectangular constraints
                if (constraintXButton.getToggleState())
                {
                    totalX = juce::jlimit(getStageMinX(), getStageMaxX(), totalX);
                    offsetX = totalX - posX;
                }
                if (constraintYButton.getToggleState())
                {
                    totalY = juce::jlimit(getStageMinY(), getStageMaxY(), totalY);
                    offsetY = totalY - posY;
                }
                if (constraintZButton.getToggleState())
                {
                    totalZ = juce::jlimit(getStageMinZ(), getStageMaxZ(), totalZ);
                    offsetZ = totalZ - posZ;
                }
            }

            // Update displays and save all offset values
            offsetXEditor.setText(juce::String(offsetX, 2), juce::dontSendNotification);
            offsetYEditor.setText(juce::String(offsetY, 2), juce::dontSendNotification);
            offsetZEditor.setText(juce::String(offsetZ, 2), juce::dontSendNotification);
            saveInputParam(WFSParameterIDs::inputOffsetX, offsetX);
            saveInputParam(WFSParameterIDs::inputOffsetY, offsetY);
            saveInputParam(WFSParameterIDs::inputOffsetZ, offsetZ);
        }
        // AutomOtion tab - Destination editors with coordinate mode support
        else if (&editor == &otomoDestXEditor || &editor == &otomoDestYEditor || &editor == &otomoDestZEditor)
        {
            // Get all three values from editors
            float v1 = otomoDestXEditor.getText().getFloatValue();
            float v2 = otomoDestYEditor.getText().getFloatValue();
            float v3 = otomoDestZEditor.getText().getFloatValue();

            // Get coordinate mode from AutomOtion selector
            int mode = otomoCoordModeSelector.getSelectedId() - 1;  // 0=Cartesian, 1=Cylindrical, 2=Spherical

            // Apply bounds and save based on coordinate mode
            if (mode == 0)  // Cartesian
            {
                v1 = juce::jlimit(-50.0f, 50.0f, v1);  // X
                v2 = juce::jlimit(-50.0f, 50.0f, v2);  // Y
                v3 = juce::jlimit(-50.0f, 50.0f, v3);  // Z

                // Save Cartesian values directly
                saveInputParam(WFSParameterIDs::inputOtomoX, v1);
                saveInputParam(WFSParameterIDs::inputOtomoY, v2);
                saveInputParam(WFSParameterIDs::inputOtomoZ, v3);

                // Also save coordinate mode
                saveInputParam(WFSParameterIDs::inputOtomoCoordinateMode, 0);
            }
            else if (mode == 1)  // Cylindrical
            {
                v1 = juce::jlimit(0.0f, 50.0f, v1);     // Radius (0 to 50m)
                v2 = juce::jlimit(-3600.0f, 3600.0f, v2);  // Theta (allow 10 full rotations)
                v3 = juce::jlimit(-50.0f, 50.0f, v3);   // Height

                // Save cylindrical values to polar parameters
                saveInputParam(WFSParameterIDs::inputOtomoR, v1);
                saveInputParam(WFSParameterIDs::inputOtomoTheta, v2);
                saveInputParam(WFSParameterIDs::inputOtomoZ, v3);  // Z is shared with Cartesian

                // Also save coordinate mode
                saveInputParam(WFSParameterIDs::inputOtomoCoordinateMode, 1);
            }
            else  // Spherical (mode == 2)
            {
                v1 = juce::jlimit(0.0f, 50.0f, v1);     // Radius (0 to 50m)
                v2 = juce::jlimit(-3600.0f, 3600.0f, v2);  // Theta (allow 10 full rotations)
                v3 = juce::jlimit(-3600.0f, 3600.0f, v3);  // Phi (allow 10 full rotations)

                // Save spherical values to polar parameters
                saveInputParam(WFSParameterIDs::inputOtomoRsph, v1);
                saveInputParam(WFSParameterIDs::inputOtomoTheta, v2);  // Theta is shared with cylindrical
                saveInputParam(WFSParameterIDs::inputOtomoPhi, v3);

                // Also save coordinate mode
                saveInputParam(WFSParameterIDs::inputOtomoCoordinateMode, 2);
            }

            // Update display with bounded values
            updateOtomoDestinationEditors();
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
            // Force label update
            attenuationValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
        }
        else if (label == &delayLatencyValueLabel)
        {
            float ms = juce::jlimit(-100.0f, 100.0f, value);
            delayLatencySlider.setValue(ms / 100.0f);
            // Force label update
            juce::String labelText = (ms < 0) ? "Latency: " : "Delay: ";
            delayLatencyValueLabel.setText(labelText + juce::String(std::abs(ms), 1) + " ms", juce::dontSendNotification);
        }
        // Position tab
        else if (label == &trackingSmoothValueLabel)
        {
            int percent = juce::jlimit(0, 100, static_cast<int>(value));
            trackingSmoothDial.setValue(percent / 100.0f);
            // Force label update (number only, unit is separate)
            trackingSmoothValueLabel.setText(juce::String(percent), juce::dontSendNotification);
        }
        else if (label == &maxSpeedValueLabel)
        {
            float speed = juce::jlimit(0.01f, 20.0f, value);
            // Inverse of: speed = v * 19.99 + 0.01
            maxSpeedDial.setValue((speed - 0.01f) / 19.99f);
            // Force label update (number only, unit is separate)
            maxSpeedValueLabel.setText(juce::String(speed, 2), juce::dontSendNotification);
        }
        else if (label == &heightFactorValueLabel)
        {
            int percent = juce::jlimit(0, 100, static_cast<int>(value));
            heightFactorDial.setValue(percent / 100.0f);
            // Force label update (number only, unit is separate)
            heightFactorValueLabel.setText(juce::String(percent), juce::dontSendNotification);
        }
        // Sound tab
        else if (label == &distanceAttenValueLabel)
        {
            float dBm = juce::jlimit(-6.0f, 0.0f, value);
            // Inverse of: dBm = (v * 6.0) - 6.0
            distanceAttenDial.setValue((dBm + 6.0f) / 6.0f);
            // Force label update (unit label is separate)
            distanceAttenValueLabel.setText(juce::String(dBm, 1), juce::dontSendNotification);
        }
        else if (label == &distanceRatioValueLabel)
        {
            float ratio = juce::jlimit(0.1f, 10.0f, value);
            // Inverse of: ratio = pow(10, (v * 2) - 1)
            distanceRatioDial.setValue((std::log10(ratio) + 1.0f) / 2.0f);
            // Force label update (unit label is separate)
            distanceRatioValueLabel.setText(juce::String(ratio, 2), juce::dontSendNotification);
        }
        else if (label == &commonAttenValueLabel)
        {
            int percent = juce::jlimit(0, 100, static_cast<int>(value));
            commonAttenDial.setValue(percent / 100.0f);
            // Force label update (unit label is separate)
            commonAttenValueLabel.setText(juce::String(percent), juce::dontSendNotification);
        }
        else if (label == &directivityValueLabel)
        {
            int degrees = juce::jlimit(2, 360, static_cast<int>(value));
            // Inverse of: degrees = v * 358 + 2 (CSV formula)
            directivitySlider.setValue((degrees - 2.0f) / 358.0f);
            // Force label update
            directivityValueLabel.setText(juce::String(degrees) + juce::String::fromUTF8("°"), juce::dontSendNotification);
        }
        else if (label == &rotationValueLabel)
        {
            int degrees = juce::jlimit(-180, 180, static_cast<int>(value));
            inputDirectivityDial.setRotation(static_cast<float>(degrees));
            rotationValueLabel.setText(juce::String(degrees), juce::dontSendNotification);
        }
        else if (label == &tiltValueLabel)
        {
            int degrees = juce::jlimit(-90, 90, static_cast<int>(value));
            // Inverse of: degrees = v * 90 (bidirectional slider, v is -1 to 1)
            float sliderVal = juce::jlimit(-1.0f, 1.0f, degrees / 90.0f);
            tiltSlider.setValue(sliderVal);
            // Force label update in case slider value didn't change
            tiltValueLabel.setText(juce::String(degrees) + juce::String::fromUTF8("°"), juce::dontSendNotification);
        }
        else if (label == &hfShelfValueLabel)
        {
            float dB = juce::jlimit(-24.0f, 0.0f, value);
            // Inverse of: dB = 20*log10(minLin + (1-minLin)*v^2) where minLin = 10^(-24/20)
            float minLinear = std::pow(10.0f, -24.0f / 20.0f);
            float targetLinear = std::pow(10.0f, dB / 20.0f);
            float v = std::sqrt((targetLinear - minLinear) / (1.0f - minLinear));
            hfShelfSlider.setValue(juce::jlimit(0.0f, 1.0f, v));
            // Force label update
            hfShelfValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
        }
        // Live Source tab
        else if (label == &lsRadiusValueLabel)
        {
            float meters = juce::jlimit(0.0f, 50.0f, value);
            // Inverse of: meters = v * 50.0
            lsRadiusSlider.setValue(meters / 50.0f);
            // Force label update
            lsRadiusValueLabel.setText(juce::String(meters, 1) + " m", juce::dontSendNotification);
        }
        else if (label == &lsAttenuationValueLabel)
        {
            float dB = juce::jlimit(-24.0f, 0.0f, value);
            // Inverse of: dB = 20*log10(minLin + (1-minLin)*v^2) where minLin = 10^(-24/20)
            float minLinear = std::pow(10.0f, -24.0f / 20.0f);
            float targetLinear = std::pow(10.0f, dB / 20.0f);
            float v = std::sqrt((targetLinear - minLinear) / (1.0f - minLinear));
            lsAttenuationSlider.setValue(juce::jlimit(0.0f, 1.0f, v));
            // Force label update
            lsAttenuationValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
        }
        else if (label == &lsPeakThresholdValueLabel)
        {
            float dB = juce::jlimit(-48.0f, 0.0f, value);
            // Inverse of: dB = 20*log10(minLin + (1-minLin)*v^2) where minLin = 10^(-48/20)
            float minLinear = std::pow(10.0f, -48.0f / 20.0f);
            float targetLinear = std::pow(10.0f, dB / 20.0f);
            float v = std::sqrt((targetLinear - minLinear) / (1.0f - minLinear));
            lsPeakThresholdSlider.setValue(juce::jlimit(0.0f, 1.0f, v));
            // Force label update
            lsPeakThresholdValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
        }
        else if (label == &lsPeakRatioValueLabel)
        {
            float ratio = juce::jlimit(1.0f, 10.0f, value);
            // Inverse of: ratio = v * 9.0 + 1.0
            lsPeakRatioDial.setValue((ratio - 1.0f) / 9.0f);
            // Force label update
            lsPeakRatioValueLabel.setText(juce::String(ratio, 1), juce::dontSendNotification);
        }
        else if (label == &lsSlowThresholdValueLabel)
        {
            float dB = juce::jlimit(-48.0f, 0.0f, value);
            // Inverse of: dB = 20*log10(minLin + (1-minLin)*v^2) where minLin = 10^(-48/20)
            float minLinear = std::pow(10.0f, -48.0f / 20.0f);
            float targetLinear = std::pow(10.0f, dB / 20.0f);
            float v = std::sqrt((targetLinear - minLinear) / (1.0f - minLinear));
            lsSlowThresholdSlider.setValue(juce::jlimit(0.0f, 1.0f, v));
            // Force label update
            lsSlowThresholdValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
        }
        else if (label == &lsSlowRatioValueLabel)
        {
            float ratio = juce::jlimit(1.0f, 10.0f, value);
            lsSlowRatioDial.setValue((ratio - 1.0f) / 9.0f);
            // Force label update
            lsSlowRatioValueLabel.setText(juce::String(ratio, 1), juce::dontSendNotification);
        }
        // Effects/Hackoustics tab
        else if (label == &frAttenuationValueLabel)
        {
            float dB = juce::jlimit(-60.0f, 0.0f, value);
            // Inverse of: dB = 20*log10(minLin + (1-minLin)*v^2) where minLin = 10^(-60/20)
            float minLinear = std::pow(10.0f, -60.0f / 20.0f);
            float targetLinear = std::pow(10.0f, dB / 20.0f);
            float v = std::sqrt((targetLinear - minLinear) / (1.0f - minLinear));
            frAttenuationSlider.setValue(juce::jlimit(0.0f, 1.0f, v));
            // Force label update
            frAttenuationValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
        }
        else if (label == &frDiffusionValueLabel)
        {
            int percent = juce::jlimit(0, 100, static_cast<int>(value));
            frDiffusionDial.setValue(percent / 100.0f);
            // Force label update
            frDiffusionValueLabel.setText(juce::String(percent), juce::dontSendNotification);
        }
        else if (label == &frLowCutFreqValueLabel)
        {
            int freq = juce::jlimit(20, 20000, static_cast<int>(value));
            // Inverse of: freq = 20 * pow(10, 3*v)
            float v = std::log10(freq / 20.0f) / 3.0f;
            frLowCutFreqSlider.setValue(juce::jlimit(0.0f, 1.0f, v));
            // Force label update
            frLowCutFreqValueLabel.setText(juce::String(freq) + " Hz", juce::dontSendNotification);
        }
        else if (label == &frHighShelfFreqValueLabel)
        {
            int freq = juce::jlimit(20, 20000, static_cast<int>(value));
            // Inverse of: freq = 20 * pow(10, 3*v)
            float v = std::log10(freq / 20.0f) / 3.0f;
            frHighShelfFreqSlider.setValue(juce::jlimit(0.0f, 1.0f, v));
            // Force label update
            frHighShelfFreqValueLabel.setText(juce::String(freq) + " Hz", juce::dontSendNotification);
        }
        else if (label == &frHighShelfGainValueLabel)
        {
            float dB = juce::jlimit(-24.0f, 0.0f, value);
            // Inverse of: dB = 20*log10(minLin + (1-minLin)*v^2) where minLin = 10^(-24/20)
            float minLinear = std::pow(10.0f, -24.0f / 20.0f);
            float targetLinear = std::pow(10.0f, dB / 20.0f);
            float v = std::sqrt((targetLinear - minLinear) / (1.0f - minLinear));
            frHighShelfGainSlider.setValue(juce::jlimit(0.0f, 1.0f, v));
            // Force label update
            frHighShelfGainValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
        }
        else if (label == &frHighShelfSlopeValueLabel)
        {
            float slope = juce::jlimit(0.1f, 0.9f, value);
            // Inverse of: slope = (v * 0.8) + 0.1
            frHighShelfSlopeSlider.setValue((slope - 0.1f) / 0.8f);
            // Force label update
            frHighShelfSlopeValueLabel.setText(juce::String(slope, 2), juce::dontSendNotification);
        }
        else if (label == &jitterValueLabel)
        {
            float meters = juce::jlimit(0.0f, 10.0f, value);
            // Inverse of: meters = 10.0 * v^2, so v = sqrt(meters / 10.0)
            float sliderVal = std::sqrt(meters / 10.0f);
            jitterSlider.setValue(juce::jlimit(0.0f, 1.0f, sliderVal));
            // Force label update in case slider value didn't change
            jitterValueLabel.setText(juce::String(meters, 2) + " m", juce::dontSendNotification);
        }
        // LFO tab
        else if (label == &lfoPeriodValueLabel)
        {
            float period = juce::jlimit(0.01f, 100.0f, value);
            // Inverse of: period = pow(10, sqrt(v)*4 - 2), so sqrt(v) = (log10(period)+2)/4
            float sqrtV = (std::log10(period) + 2.0f) / 4.0f;
            float v = sqrtV * sqrtV;
            lfoPeriodDial.setValue(juce::jlimit(0.0f, 1.0f, v));
            // Force label update
            lfoPeriodValueLabel.setText(juce::String(period, 2), juce::dontSendNotification);
        }
        else if (label == &lfoPhaseValueLabel)
        {
            int degrees = juce::jlimit(0, 360, static_cast<int>(value));
            lfoPhaseDial.setAngle(static_cast<float>(degrees));
            // Force label update
            lfoPhaseValueLabel.setText(juce::String(degrees), juce::dontSendNotification);
        }
        else if (label == &lfoRateXValueLabel)
        {
            // Inverse of: rate = pow(10, v*4 - 2), range 0.01-100
            float rate = juce::jlimit(0.01f, 100.0f, value);
            float v = (std::log10(rate) + 2.0f) / 4.0f;
            lfoRateXSlider.setValue(juce::jlimit(0.0f, 1.0f, v));
            // Force label update
            lfoRateXValueLabel.setText(juce::String(rate, 2) + "x", juce::dontSendNotification);
        }
        else if (label == &lfoRateYValueLabel)
        {
            float rate = juce::jlimit(0.01f, 100.0f, value);
            float v = (std::log10(rate) + 2.0f) / 4.0f;
            lfoRateYSlider.setValue(juce::jlimit(0.0f, 1.0f, v));
            // Force label update
            lfoRateYValueLabel.setText(juce::String(rate, 2) + "x", juce::dontSendNotification);
        }
        else if (label == &lfoRateZValueLabel)
        {
            float rate = juce::jlimit(0.01f, 100.0f, value);
            float v = (std::log10(rate) + 2.0f) / 4.0f;
            lfoRateZSlider.setValue(juce::jlimit(0.0f, 1.0f, v));
            // Force label update
            lfoRateZValueLabel.setText(juce::String(rate, 2) + "x", juce::dontSendNotification);
        }
        else if (label == &lfoAmplitudeXValueLabel)
        {
            // Inverse of: amp = v * 50, range 0-50m
            float amp = juce::jlimit(0.0f, 50.0f, value);
            lfoAmplitudeXSlider.setValue(amp / 50.0f);
            // Force label update
            lfoAmplitudeXValueLabel.setText(juce::String(amp, 1) + " m", juce::dontSendNotification);
        }
        else if (label == &lfoAmplitudeYValueLabel)
        {
            float amp = juce::jlimit(0.0f, 50.0f, value);
            lfoAmplitudeYSlider.setValue(amp / 50.0f);
            // Force label update
            lfoAmplitudeYValueLabel.setText(juce::String(amp, 1) + " m", juce::dontSendNotification);
        }
        else if (label == &lfoAmplitudeZValueLabel)
        {
            float amp = juce::jlimit(0.0f, 50.0f, value);
            lfoAmplitudeZSlider.setValue(amp / 50.0f);
            // Force label update
            lfoAmplitudeZValueLabel.setText(juce::String(amp, 1) + " m", juce::dontSendNotification);
        }
        else if (label == &lfoPhaseXValueLabel)
        {
            int degrees = juce::jlimit(0, 360, static_cast<int>(value));
            lfoPhaseXDial.setAngle(static_cast<float>(degrees));
            // Force label update
            lfoPhaseXValueLabel.setText(juce::String(degrees), juce::dontSendNotification);
        }
        else if (label == &lfoPhaseYValueLabel)
        {
            int degrees = juce::jlimit(0, 360, static_cast<int>(value));
            lfoPhaseYDial.setAngle(static_cast<float>(degrees));
            // Force label update
            lfoPhaseYValueLabel.setText(juce::String(degrees), juce::dontSendNotification);
        }
        else if (label == &lfoPhaseZValueLabel)
        {
            int degrees = juce::jlimit(0, 360, static_cast<int>(value));
            lfoPhaseZDial.setAngle(static_cast<float>(degrees));
            // Force label update
            lfoPhaseZValueLabel.setText(juce::String(degrees), juce::dontSendNotification);
        }
        // AutomOtion tab
        else if (label == &otomoSpeedProfileValueLabel)
        {
            int percent = juce::jlimit(0, 100, static_cast<int>(value));
            otomoSpeedProfileDial.setValue(percent / 100.0f);
            // Force label update
            otomoSpeedProfileValueLabel.setText(juce::String(percent), juce::dontSendNotification);
        }
        else if (label == &otomoThresholdValueLabel)
        {
            // Range is -92 to 0 dB, using sqrt scaling for perceptual control
            float dB = juce::jlimit(-92.0f, 0.0f, value);
            float otomoMinLinear = std::pow(10.0f, -92.0f / 20.0f);
            float linear = std::pow(10.0f, dB / 20.0f);
            float dialValue = std::sqrt((linear - otomoMinLinear) / (1.0f - otomoMinLinear));
            otomoThresholdDial.setValue(juce::jlimit(0.0f, 1.0f, dialValue));
            // Force label update
            otomoThresholdValueLabel.setText(juce::String(dB, 1), juce::dontSendNotification);
        }
        else if (label == &otomoResetValueLabel)
        {
            // Range is -92 to 0 dB, using sqrt scaling for perceptual control
            float dB = juce::jlimit(-92.0f, 0.0f, value);
            float otomoMinLinear = std::pow(10.0f, -92.0f / 20.0f);
            float linear = std::pow(10.0f, dB / 20.0f);
            float dialValue = std::sqrt((linear - otomoMinLinear) / (1.0f - otomoMinLinear));
            otomoResetDial.setValue(juce::jlimit(0.0f, 1.0f, dialValue));
            // Force label update
            otomoResetValueLabel.setText(juce::String(dB, 1), juce::dontSendNotification);
        }
        // Sidelines fringe (Mutes tab)
        else if (label == &sidelinesFringeValueLabel)
        {
            float fringe = juce::jlimit(WFSParameterDefaults::inputSidelinesFringeMin,
                                        WFSParameterDefaults::inputSidelinesFringeMax, value);
            // Convert meters to dial value (0-1)
            float dialValue = (fringe - WFSParameterDefaults::inputSidelinesFringeMin) /
                              (WFSParameterDefaults::inputSidelinesFringeMax - WFSParameterDefaults::inputSidelinesFringeMin);
            sidelinesFringeDial.setValue(juce::jlimit(0.0f, 1.0f, dialValue));
            // Force label update
            sidelinesFringeValueLabel.setText(juce::String(fringe, 2) + " m", juce::dontSendNotification);
        }
        // Array attenuation dials (Mutes tab)
        else
        {
            for (int i = 0; i < 10; ++i)
            {
                if (label == &arrayAttenValueLabels[i])
                {
                    // Range is -60 to 0 dB, using sqrt scaling for perceptual control
                    float dB = juce::jlimit(-60.0f, 0.0f, value);
                    constexpr float minLinear = 0.001f;  // -60 dB
                    float linear = std::pow(10.0f, dB / 20.0f);
                    float dialValue = std::sqrt((linear - minLinear) / (1.0f - minLinear));
                    arrayAttenDials[i].setValue(juce::jlimit(0.0f, 1.0f, dialValue));
                    // Force label update
                    arrayAttenValueLabels[i].setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
                    break;
                }
            }
        }
    }

    // ==================== STORE/RELOAD METHODS ====================

    void storeInputConfiguration()
    {
        auto& fileManager = parameters.getFileManager();
        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage(LOC("inputs.messages.selectFolderFirst"));
            return;
        }
        if (fileManager.saveInputConfig())
            showStatusMessage(LOC("inputs.messages.configSaved"));
        else
            showStatusMessage(LOC("inputs.messages.error").replace("{error}", fileManager.getLastError()));
    }

    void reloadInputConfiguration()
    {
        auto& fileManager = parameters.getFileManager();
        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage(LOC("inputs.messages.selectFolderFirst"));
            return;
        }
        if (fileManager.loadInputConfig())
        {
            loadChannelParameters(currentChannel);
            showStatusMessage(LOC("inputs.messages.configLoaded"));

            // Trigger DSP recalculation via callback to MainComponent
            if (onConfigReloaded)
                onConfigReloaded();
        }
        else
            showStatusMessage(LOC("inputs.messages.error").replace("{error}", fileManager.getLastError()));
    }

    void reloadInputConfigBackup()
    {
        auto& fileManager = parameters.getFileManager();
        if (fileManager.loadInputConfigBackup(0))
        {
            loadChannelParameters(currentChannel);
            showStatusMessage(LOC("inputs.messages.backupLoaded"));

            // Trigger DSP recalculation via callback to MainComponent
            if (onConfigReloaded)
                onConfigReloaded();
        }
        else
            showStatusMessage(LOC("inputs.messages.error").replace("{error}", fileManager.getLastError()));
    }

    void importInputConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser>(LOC("inputs.dialogs.importConfig"),
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
                    showStatusMessage(LOC("inputs.messages.configImported"));

                    // Trigger DSP recalculation via callback to MainComponent
                    if (onConfigReloaded)
                        onConfigReloaded();
                }
                else
                    showStatusMessage(LOC("inputs.messages.error").replace("{error}", fileManager.getLastError()));
            }
        });
    }

    void exportInputConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser>(LOC("inputs.dialogs.exportConfig"),
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
                    showStatusMessage(LOC("inputs.messages.configExported"));
                else
                    showStatusMessage(LOC("inputs.messages.error").replace("{error}", fileManager.getLastError()));
            }
        });
    }

    void storeNewSnapshot()
    {
        auto& fileManager = parameters.getFileManager();
        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage(LOC("inputs.messages.selectFolderFirst"));
            return;
        }

        auto defaultName = WFSFileManager::getDefaultSnapshotName();

        auto* dialog = new juce::AlertWindow(
            "Store New Snapshot",
            "Enter a name for the new snapshot:",
            juce::MessageBoxIconType::NoIcon);

        dialog->addTextEditor("name", defaultName, "Name:");
        dialog->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
        dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

        dialog->enterModalState(true, juce::ModalCallbackFunction::create(
            [this, dialog](int result)
            {
                if (result == 1)
                {
                    auto name = dialog->getTextEditorContents("name");
                    if (name.isNotEmpty())
                    {
                        // Use current scope if configured, otherwise create default
                        WFSFileManager::ExtendedSnapshotScope scope;
                        if (currentScopeInitialized)
                        {
                            scope = currentScope;
                        }
                        else
                        {
                            scope.initializeDefaults(parameters.getNumInputChannels());
                        }
                        snapshotScopes[name] = scope;

                        auto& fileManager = parameters.getFileManager();
                        if (fileManager.saveInputSnapshotWithExtendedScope(name, scope))
                        {
                            refreshSnapshotList();
                            snapshotSelector.setText(name, juce::dontSendNotification);
                            showStatusMessage("Snapshot '" + name + "' stored.");
                        }
                        else
                        {
                            showStatusMessage(LOC("inputs.messages.error").replace("{error}", fileManager.getLastError()));
                        }
                    }
                }
                delete dialog;
            }
        ), true);
    }

    void reloadSnapshot()
    {
        auto selectedSnapshot = snapshotSelector.getText();
        if (selectedSnapshot.isEmpty() || selectedSnapshot == "Select Snapshot...")
        {
            showStatusMessage(LOC("inputs.messages.noSnapshotSelected"));
            return;
        }

        auto& fileManager = parameters.getFileManager();

        // Load scope if not cached
        if (snapshotScopes.find(selectedSnapshot) == snapshotScopes.end())
        {
            snapshotScopes[selectedSnapshot] = fileManager.getExtendedSnapshotScope(selectedSnapshot);
        }

        auto& scope = snapshotScopes[selectedSnapshot];

        if (fileManager.loadInputSnapshotWithExtendedScope(selectedSnapshot, scope))
        {
            loadChannelParameters(currentChannel);
            showStatusMessage("Snapshot '" + selectedSnapshot + "' loaded.");
            if (onConfigReloaded)
                onConfigReloaded();
        }
        else
        {
            showStatusMessage(LOC("inputs.messages.error").replace("{error}", fileManager.getLastError()));
        }
    }

    void updateSnapshot()
    {
        auto selectedSnapshot = snapshotSelector.getText();
        if (selectedSnapshot.isEmpty() || selectedSnapshot == "Select Snapshot...")
        {
            showStatusMessage(LOC("inputs.messages.noSnapshotSelected"));
            return;
        }

        juce::AlertWindow::showOkCancelBox(
            juce::AlertWindow::QuestionIcon,
            "Update Snapshot",
            "Update snapshot '" + selectedSnapshot + "' with current settings?\nA backup will be created.",
            "Update",
            "Cancel",
            nullptr,
            juce::ModalCallbackFunction::create([this, selectedSnapshot](int result) {
                if (result == 1)
                {
                    auto& fileManager = parameters.getFileManager();

                    // Get existing scope or create default
                    if (snapshotScopes.find(selectedSnapshot) == snapshotScopes.end())
                        snapshotScopes[selectedSnapshot] = fileManager.getExtendedSnapshotScope(selectedSnapshot);

                    auto& scope = snapshotScopes[selectedSnapshot];

                    // Create backup then save
                    auto file = fileManager.getInputSnapshotsFolder().getChildFile(selectedSnapshot + ".xml");
                    fileManager.createBackup(file);

                    if (fileManager.saveInputSnapshotWithExtendedScope(selectedSnapshot, scope))
                        showStatusMessage("Snapshot '" + selectedSnapshot + "' updated.");
                    else
                        showStatusMessage(LOC("inputs.messages.error").replace("{error}", fileManager.getLastError()));
                }
            })
        );
    }

    void editSnapshotScope()
    {
        auto selectedSnapshot = snapshotSelector.getText();
        bool hasSelectedSnapshot = !selectedSnapshot.isEmpty() && selectedSnapshot != "Select Snapshot...";

        auto& fileManager = parameters.getFileManager();

        // Determine which scope to edit
        WFSFileManager::ExtendedSnapshotScope* scopePtr = nullptr;
        juce::String windowTitle;

        if (hasSelectedSnapshot)
        {
            // Load scope for selected snapshot if not cached
            if (snapshotScopes.find(selectedSnapshot) == snapshotScopes.end())
            {
                snapshotScopes[selectedSnapshot] = fileManager.getExtendedSnapshotScope(selectedSnapshot);
            }
            scopePtr = &snapshotScopes[selectedSnapshot];
            windowTitle = selectedSnapshot;
        }
        else
        {
            // Use current scope (for new snapshots)
            if (!currentScopeInitialized)
            {
                currentScope.initializeDefaults(parameters.getNumInputChannels());
                currentScopeInitialized = true;
            }
            scopePtr = &currentScope;
            windowTitle = "(New Snapshot)";
        }

        if (snapshotScopeWindow == nullptr || !snapshotScopeWindow->isVisible())
        {
            snapshotScopeWindow = std::make_unique<SnapshotScopeWindow>(parameters, windowTitle, *scopePtr);
            snapshotScopeWindow->onWindowClosed = [this, hasSelectedSnapshot, selectedSnapshot](bool saved) {
                if (saved)
                {
                    if (hasSelectedSnapshot)
                    {
                        auto& fileManager = parameters.getFileManager();
                        if (fileManager.setExtendedSnapshotScope(selectedSnapshot, snapshotScopes[selectedSnapshot]))
                            showStatusMessage("Snapshot scope saved.");
                        else
                            showStatusMessage(LOC("inputs.messages.error").replace("{error}", fileManager.getLastError()));
                    }
                    else
                    {
                        showStatusMessage("Scope configured for next snapshot.");
                    }
                }
                snapshotScopeWindow.reset();
            };
        }
        else
        {
            snapshotScopeWindow->toFront(true);
        }
    }

    void deleteSnapshot()
    {
        auto selectedSnapshot = snapshotSelector.getText();
        if (selectedSnapshot.isEmpty() || selectedSnapshot == "Select Snapshot...")
        {
            showStatusMessage(LOC("inputs.messages.noSnapshotSelected"));
            return;
        }

        juce::AlertWindow::showOkCancelBox(
            juce::AlertWindow::WarningIcon,
            "Delete Snapshot",
            "Delete snapshot '" + selectedSnapshot + "'?\nThis cannot be undone.",
            "Delete",
            "Cancel",
            nullptr,
            juce::ModalCallbackFunction::create([this, selectedSnapshot](int result) {
                if (result == 1)
                {
                    auto& fileManager = parameters.getFileManager();
                    if (fileManager.deleteInputSnapshot(selectedSnapshot))
                    {
                        snapshotScopes.erase(selectedSnapshot);
                        refreshSnapshotList();
                        showStatusMessage("Snapshot '" + selectedSnapshot + "' deleted.");
                    }
                    else
                    {
                        showStatusMessage(LOC("inputs.messages.error").replace("{error}", fileManager.getLastError()));
                    }
                }
            })
        );
    }

    void refreshSnapshotList()
    {
        auto& fileManager = parameters.getFileManager();
        auto names = fileManager.getInputSnapshotNames();

        snapshotSelector.clear(juce::dontSendNotification);
        snapshotSelector.addItem(LOC("inputs.snapshots.selectSnapshot"), 1);

        int id = 2;
        for (const auto& name : names)
        {
            snapshotSelector.addItem(name, id++);
        }
    }

    //==============================================================================
    // Stage bounds helper methods for constraint enforcement

    // Stage bounds (center-referenced for X/Y, floor-referenced for Z)
    // For circular shapes (cylinder/dome), use diameter instead of width/depth
    float getStageMinX() const
    {
        int shape = static_cast<int>(parameters.getConfigParam("StageShape"));
        float halfSize = (shape == 0)
            ? static_cast<float>(parameters.getConfigParam("StageWidth")) / 2.0f
            : static_cast<float>(parameters.getConfigParam("StageDiameter")) / 2.0f;
        float originWidth = static_cast<float>(parameters.getConfigParam("StageOriginWidth"));
        return -halfSize - originWidth;
    }

    float getStageMaxX() const
    {
        int shape = static_cast<int>(parameters.getConfigParam("StageShape"));
        float halfSize = (shape == 0)
            ? static_cast<float>(parameters.getConfigParam("StageWidth")) / 2.0f
            : static_cast<float>(parameters.getConfigParam("StageDiameter")) / 2.0f;
        float originWidth = static_cast<float>(parameters.getConfigParam("StageOriginWidth"));
        return halfSize - originWidth;
    }

    float getStageMinY() const
    {
        int shape = static_cast<int>(parameters.getConfigParam("StageShape"));
        float halfSize = (shape == 0)
            ? static_cast<float>(parameters.getConfigParam("StageDepth")) / 2.0f
            : static_cast<float>(parameters.getConfigParam("StageDiameter")) / 2.0f;
        float originDepth = static_cast<float>(parameters.getConfigParam("StageOriginDepth"));
        return -halfSize - originDepth;
    }

    float getStageMaxY() const
    {
        int shape = static_cast<int>(parameters.getConfigParam("StageShape"));
        float halfSize = (shape == 0)
            ? static_cast<float>(parameters.getConfigParam("StageDepth")) / 2.0f
            : static_cast<float>(parameters.getConfigParam("StageDiameter")) / 2.0f;
        float originDepth = static_cast<float>(parameters.getConfigParam("StageOriginDepth"));
        return halfSize - originDepth;
    }

    float getStageMinZ() const
    {
        float originHeight = static_cast<float>(parameters.getConfigParam("StageOriginHeight"));
        return -originHeight;
    }

    float getStageMaxZ() const
    {
        float stageHeight = static_cast<float>(parameters.getConfigParam("StageHeight"));
        float originHeight = static_cast<float>(parameters.getConfigParam("StageOriginHeight"));
        return stageHeight - originHeight;
    }

    //==============================================================================
    // Coordinate mode helpers

    /** Update position labels and values based on current coordinate mode */
    void updatePositionLabelsAndValues()
    {
        // Get current coordinate mode
        int mode = static_cast<int>(parameters.getInputParam(currentChannel - 1, "inputCoordinateMode"));
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
        float x = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputPositionX"));
        float y = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputPositionY"));
        float z = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputPositionZ"));

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

    /** Update AutomOtion destination labels and values based on coordinate mode */
    void updateOtomoLabelsAndValues()
    {
        int mode = otomoCoordModeSelector.getSelectedId() - 1;  // 0=Cartesian, 1=Cylindrical, 2=Spherical
        auto coordMode = static_cast<WFSCoordinates::Mode>(mode);

        // Get short labels (X:, Y:, Z: or r:, θ:, Z: or r:, θ:, φ:)
        juce::String short1, short2, short3;
        WFSCoordinates::getShortLabels(coordMode, short1, short2, short3);

        otomoDestXLabel.setText(short1, juce::dontSendNotification);
        otomoDestYLabel.setText(short2, juce::dontSendNotification);
        otomoDestZLabel.setText(short3, juce::dontSendNotification);

        // Get units (m, m, m or m, °, m or m, °, °)
        juce::String label1, label2, label3, unit1, unit2, unit3;
        WFSCoordinates::getCoordinateLabels(coordMode, label1, label2, label3, unit1, unit2, unit3);

        otomoDestXUnitLabel.setText(unit1, juce::dontSendNotification);
        otomoDestYUnitLabel.setText(unit2, juce::dontSendNotification);
        otomoDestZUnitLabel.setText(unit3, juce::dontSendNotification);
    }

    /** Update AutomOtion destination editor values based on coordinate mode */
    void updateOtomoDestinationEditors()
    {
        int mode = otomoCoordModeSelector.getSelectedId() - 1;  // 0=Cartesian, 1=Cylindrical, 2=Spherical

        float v1, v2, v3;

        // Load values based on coordinate mode
        if (mode == 0)  // Cartesian
        {
            v1 = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputOtomoX"));
            v2 = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputOtomoY"));
            v3 = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputOtomoZ"));

            otomoDestXEditor.setText(juce::String(v1, 2), juce::dontSendNotification);
            otomoDestYEditor.setText(juce::String(v2, 2), juce::dontSendNotification);
            otomoDestZEditor.setText(juce::String(v3, 2), juce::dontSendNotification);
        }
        else if (mode == 1)  // Cylindrical
        {
            v1 = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputOtomoR"));
            v2 = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputOtomoTheta"));
            v3 = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputOtomoZ"));

            otomoDestXEditor.setText(juce::String(v1, 2), juce::dontSendNotification);  // radius
            otomoDestYEditor.setText(juce::String(v2, 1), juce::dontSendNotification);  // theta
            otomoDestZEditor.setText(juce::String(v3, 2), juce::dontSendNotification);  // height
        }
        else  // Spherical (mode == 2)
        {
            v1 = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputOtomoRsph"));
            v2 = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputOtomoTheta"));
            v3 = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputOtomoPhi"));

            otomoDestXEditor.setText(juce::String(v1, 2), juce::dontSendNotification);  // radius
            otomoDestYEditor.setText(juce::String(v2, 1), juce::dontSendNotification);  // theta
            otomoDestZEditor.setText(juce::String(v3, 1), juce::dontSendNotification);  // phi
        }
    }

    /** Update AutomOtion trigger controls appearance (dim in Manual mode) */
    void updateOtomoTriggerAppearance()
    {
        bool isTriggerMode = otomoTriggerButton.getToggleState();
        float alpha = isTriggerMode ? 1.0f : 0.4f;

        otomoThresholdLabel.setAlpha(alpha);
        otomoThresholdDial.setAlpha(alpha);
        otomoThresholdValueLabel.setAlpha(alpha);
        otomoThresholdUnitLabel.setAlpha(alpha);
        otomoResetLabel.setAlpha(alpha);
        otomoResetDial.setAlpha(alpha);
        otomoResetValueLabel.setAlpha(alpha);
        otomoResetUnitLabel.setAlpha(alpha);
    }

    /** Update AutomOtion curve visibility based on coordinate mode and current tab */
    void updateOtomoCurveVisibility()
    {
        // Only show curve dial on Movements tab (index 2) when in Cartesian mode
        bool isMovementsTab = (subTabBar.getCurrentTabIndex() == 2);
        bool isCartesian = (otomoCoordModeSelector.getSelectedId() == 1);  // 1 = Cartesian
        bool showCurve = isMovementsTab && isCartesian;
        otomoCurveLabel.setVisible(showCurve);
        otomoCurveDial.setVisible(showCurve);
        otomoCurveValueLabel.setVisible(showCurve);
        otomoCurveUnitLabel.setVisible(showCurve);
    }

    /** Update constraint button visibility based on coordinate mode */
    void updateConstraintVisibility()
    {
        int mode = coordModeSelector.getSelectedId() - 1;  // 0=Cartesian, 1=Cylindrical, 2=Spherical

        bool isCartesian = (mode == 0);
        bool isCylindrical = (mode == 1);
        bool isSpherical = (mode == 2);

        // X/Y constraints: visible only in Cartesian mode
        constraintXButton.setVisible(isCartesian);
        constraintYButton.setVisible(isCartesian);

        // Z constraint: visible in Cartesian and Cylindrical (hide in Spherical)
        constraintZButton.setVisible(isCartesian || isCylindrical);

        // Distance constraints: visible in Cylindrical and Spherical
        bool showDistance = isCylindrical || isSpherical;
        constraintDistanceButton.setVisible(showDistance);
        distanceRangeSlider.setVisible(showDistance);
        distanceMinLabel.setVisible(showDistance);
        distanceMinEditor.setVisible(showDistance);
        distanceMinUnitLabel.setVisible(showDistance);
        distanceMaxLabel.setVisible(showDistance);
        distanceMaxEditor.setVisible(showDistance);
        distanceMaxUnitLabel.setVisible(showDistance);
    }

    /** Calculate distance from origin based on coordinate mode */
    float calculateDistanceFromOrigin(float x, float y, float z, int coordMode) const
    {
        if (coordMode == 1)  // Cylindrical: sqrt(x^2 + y^2)
            return std::sqrt(x * x + y * y);
        else if (coordMode == 2)  // Spherical: sqrt(x^2 + y^2 + z^2)
            return std::sqrt(x * x + y * y + z * z);
        return 0.0f;
    }

    /** Apply distance constraint, modifying Cartesian position in-place */
    void applyDistanceConstraint(float& x, float& y, float& z, int coordMode,
                                 float minDist, float maxDist)
    {
        float currentDist = calculateDistanceFromOrigin(x, y, z, coordMode);
        if (currentDist < 0.0001f) currentDist = 0.0001f;  // Avoid division by zero

        float targetDist = juce::jlimit(minDist, maxDist, currentDist);

        if (!juce::approximatelyEqual(currentDist, targetDist))
        {
            float distScale = targetDist / currentDist;

            if (coordMode == 1)  // Cylindrical: scale X/Y only
            {
                x *= distScale;
                y *= distScale;
            }
            else if (coordMode == 2)  // Spherical: scale all axes
            {
                x *= distScale;
                y *= distScale;
                z *= distScale;
            }
        }
    }

    /** Snap current position to valid distance range */
    void applyDistanceConstraintSnap()
    {
        if (currentChannel <= 0) return;

        int coordMode = coordModeSelector.getSelectedId() - 1;
        if (coordMode == 0) return;  // No distance constraint in Cartesian

        float minDist = distanceRangeSlider.getMinValue();
        float maxDist = distanceRangeSlider.getMaxValue();

        // Get Cartesian values from storage
        float x = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputPositionX"));
        float y = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputPositionY"));
        float z = static_cast<float>(parameters.getInputParam(currentChannel - 1, "inputPositionZ"));

        applyDistanceConstraint(x, y, z, coordMode, minDist, maxDist);

        // Save and update display
        saveInputParam(WFSParameterIDs::inputPositionX, x);
        saveInputParam(WFSParameterIDs::inputPositionY, y);
        if (coordMode == 2)  // Spherical affects Z too
            saveInputParam(WFSParameterIDs::inputPositionZ, z);

        updatePositionLabelsAndValues();
    }

    //==============================================================================
    // Status bar helper methods

    void setupHelpText()
    {
        helpTextMap[&channelSelector] = "Input Channel Number and Selection.";
        helpTextMap[&nameEditor] = "Displayed Input Channel Name (editable).";
        helpTextMap[&clusterSelector] = "Object is Part of a Cluster.";
        helpTextMap[&mapLockButton] = "Prevent Interaction on the Map Tab";
        helpTextMap[&mapVisibilityButton] = "Make Visible or Hide The Selected Input on the Map";
        helpTextMap[&soloButton] = LOC("inputs.help.solo");
        helpTextMap[&soloModeButton] = LOC("inputs.help.soloMode");
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
        helpTextMap[&constraintDistanceButton] = "Limit Position to Distance Range from Origin (for Cylindrical/Spherical modes).";
        helpTextMap[&distanceRangeSlider] = "Set Minimum and Maximum Distance from Origin.";
        helpTextMap[&distanceMinEditor] = "Minimum Distance from Origin in Meters.";
        helpTextMap[&distanceMaxEditor] = "Maximum Distance from Origin in Meters.";
        helpTextMap[&flipXButton] = "X will be Symetrical to the Origin. Keyboard Nudging will be Inverted.";
        helpTextMap[&flipYButton] = "Y will be Symetrical to the Origin. Keyboard Nudging will be Inverted.";
        helpTextMap[&flipZButton] = "Z will be Symetrical to the Origin. Keyboard Nudging will be Inverted.";
        helpTextMap[&trackingActiveButton] = "Enable or Disable Tracking for Object.";
        helpTextMap[&trackingIdSelector] = "Tracker ID for Object.";
        helpTextMap[&trackingSmoothDial] = "Smoothing of Tracking Data for Object.";
        helpTextMap[&maxSpeedActiveButton] = "Enable or Disable Speed Limiting for Object.";
        helpTextMap[&maxSpeedDial] = "Maximum Speed Limit for Object.";
        helpTextMap[&pathModeButton] = "Enable Path Mode to Follow Drawn Movement Paths Instead of Direct Lines.";
        helpTextMap[&heightFactorDial] = "Take Elevation of Object into Account Fully, Partially or Not.";
        helpTextMap[&coordModeSelector] = "Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).";
        helpTextMap[&positionJoystick] = "Drag to adjust X/Y position in real-time. Returns to center on release.";
        helpTextMap[&positionZSlider] = "Drag to adjust Z (height) position in real-time. Returns to center on release.";
        helpTextMap[&attenuationLawButton] = "Attenuation Law Model (Linear Decrease of Volume with Distance Between Object and Speaker or Squared).";
        helpTextMap[&distanceAttenDial] = "Attenuation per Meter Between Object and Speaker.";
        helpTextMap[&distanceRatioDial] = "Attenuation Ratio for Squared Model.";
        helpTextMap[&commonAttenDial] = "Percentage of the Common Part of the Attenuation for selected Object Relative to All Outputs.";
        helpTextMap[&directivitySlider] = "How Wide is the Brightness of The Object.";
        helpTextMap[&inputDirectivityDial] = "Where is the Object pointing to in the Horizontal Plane.";
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
        helpTextMap[&muteReverbSendsButton] = "Mute sends from this input to all reverb channels.";
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
        helpTextMap[&otomoDurationDial] = "Duration of the Movement in Seconds (0.1s to 1 hour).";
        helpTextMap[&otomoCurveDial] = "Bend the Path to the Left (Negative) or Right (Positive) of the Direction of Travel.";
        helpTextMap[&otomoStopAllButton] = "Stop All Active Movements Globally.";
        helpTextMap[&otomoPauseResumeAllButton] = "Pause or Resume All Active Movements Globally.";
        // Mutes tab
        for (int i = 0; i < 64; ++i)
            helpTextMap[&muteButtons[i]] = "Mute Output " + juce::String(i + 1) + " for this Object.";
        helpTextMap[&muteMacrosSelector] = "Mute Macros for Fast Muting and Unmuting of Arrays.";
        // Array attenuation
        for (int i = 0; i < 10; ++i)
            helpTextMap[&arrayAttenDials[i]] = "Attenuation for Array " + juce::String(i + 1) + " (-60 to 0 dB).";
        // Sidelines
        helpTextMap[&sidelinesActiveButton] = "Enable Automatic Muting when Source Approaches Stage Edges. Does Not Apply to Downstage (Front) Edge.";
        helpTextMap[&sidelinesFringeDial] = "Fringe Zone Size in Meters. Outer Half is Full Mute, Inner Half Fades Linearly.";
        helpTextMap[&storeButton] = "Store Input Configuration to file (with backup).";
        helpTextMap[&reloadButton] = "Reload Input Configuration from file.";
        helpTextMap[&reloadBackupButton] = "Reload Input Configuration from backup file.";
        helpTextMap[&importButton] = "Import Input Configuration from file (with file explorer window).";
        helpTextMap[&exportButton] = "Export Input Configuration to file (with file explorer window).";
        helpTextMap[&storeSnapshotButton] = "Store new Input Snapshot for All Objects.";
        helpTextMap[&snapshotSelector] = "Select Input Snapshot Without Loading.";
        helpTextMap[&reloadSnapshotButton] = "Reload Selected Input Snapshot for All Objects Taking the Scope into Account.";
        helpTextMap[&updateSnapshotButton] = "Update Selected Input Snapshot (with backup).";
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
        oscMethodMap[&pathModeButton] = "/wfs/input/pathModeActive <ID> <value>";
        oscMethodMap[&heightFactorDial] = "/wfs/input/heightFactor <ID> <value>";
        oscMethodMap[&attenuationLawButton] = "/wfs/input/attenuationLaw <ID> <value>";
        oscMethodMap[&distanceAttenDial] = "/wfs/input/distanceAttenuation <ID> <value>";
        oscMethodMap[&distanceRatioDial] = "/wfs/input/distanceRatio <ID> <value>";
        oscMethodMap[&commonAttenDial] = "/wfs/input/commonAtten <ID> <value>";
        oscMethodMap[&directivitySlider] = "/wfs/input/directivity <ID> <value>";
        oscMethodMap[&inputDirectivityDial] = "/wfs/input/rotation <ID> <value>";
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
        oscMethodMap[&muteReverbSendsButton] = "/wfs/input/muteReverbSends <ID> <value>";
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
        oscMethodMap[&otomoStopButton] = "/wfs/input/otomoStop <ID>";
        oscMethodMap[&otomoPauseButton] = "/wfs/input/otomoPause <ID>";
        oscMethodMap[&otomoDurationDial] = "/wfs/input/otomoDuration <ID> <value>";
        oscMethodMap[&otomoCurveDial] = "/wfs/input/otomoCurve <ID> <value>";
        oscMethodMap[&otomoStopAllButton] = "/wfs/input/otomoStopAll";
        oscMethodMap[&otomoPauseResumeAllButton] = "/wfs/input/otomoPauseResumeAll";
        // Mutes tab
        for (int i = 0; i < 64; ++i)
            oscMethodMap[&muteButtons[i]] = "/wfs/input/mutes <ID> " + juce::String(i + 1) + " <value>";
        oscMethodMap[&muteMacrosSelector] = "/wfs/input/muteMacro <ID> <value>";
        // Array attenuation
        for (int i = 0; i < 10; ++i)
            oscMethodMap[&arrayAttenDials[i]] = "/wfs/input/arrayAtten" + juce::String(i + 1) + " <ID> <value>";
        // Sidelines
        oscMethodMap[&sidelinesActiveButton] = "/wfs/input/sidelinesEnable <ID> <value>";
        oscMethodMap[&sidelinesFringeDial] = "/wfs/input/sidelinesFringe <ID> <value>";
    }

    void setupMouseListeners()
    {
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
        // Check if solo states changed (stored in binaural tree)
        if (tree == binauralTree && property == WFSParameterIDs::inputSoloStates)
        {
            juce::MessageManager::callAsync([this]()
            {
                updateSoloButtonState();
                updateClearSoloButtonState();
            });
            return;
        }

        // Check if input channel count changed (stored in IO tree)
        if (tree == ioTree && property == WFSParameterIDs::inputChannels)
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

        // Check if output channel count changed (affects mute buttons, stored in IO tree)
        if (tree == ioTree && property == WFSParameterIDs::outputChannels)
        {
            // Update mute button visibility and layout if Input Parameters tab is visible (contains mutes)
            if (subTabBar.getCurrentTabIndex() == 0)
            {
                setMutesVisible(true);
                layoutInputParametersTab();
            }
        }

        // Check if this is a parameter change for the current channel (e.g., from OSC)
        // Skip if we're already loading parameters (avoid recursion)
        if (!isLoadingParameters)
        {
            // Find if this tree belongs to the current channel's Input tree
            juce::ValueTree parent = tree;
            while (parent.isValid())
            {
                if (parent.getType() == WFSParameterIDs::Input)
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

    void toggleMapLock()
    {
        auto currentVal = parameters.getInputParam(currentChannel - 1, "inputMapLocked");
        bool currentlyLocked = !currentVal.isVoid() && static_cast<int>(currentVal) != 0;
        bool newLocked = !currentlyLocked;

        saveInputParam(WFSParameterIDs::inputMapLocked, newLocked ? 1 : 0);
        updateMapButtonStates();
    }

    void toggleMapVisibility()
    {
        auto currentVal = parameters.getInputParam(currentChannel - 1, "inputMapVisible");
        bool currentlyVisible = currentVal.isVoid() || static_cast<int>(currentVal) != 0;
        bool newVisible = !currentlyVisible;

        saveInputParam(WFSParameterIDs::inputMapVisible, newVisible ? 1 : 0);
        updateMapButtonStates();
    }

    void toggleSolo()
    {
        auto& vts = parameters.getValueTreeState();
        bool currentSoloed = vts.isInputSoloed(currentChannel - 1);
        vts.setInputSoloed(currentChannel - 1, !currentSoloed);
        updateSoloButtonState();
    }

    void updateSoloButtonState()
    {
        auto& vts = parameters.getValueTreeState();
        bool isSoloed = vts.isInputSoloed(currentChannel - 1);
        soloButton.setToggleState(isSoloed, juce::dontSendNotification);

        // Yellow in Single mode, Orange in Multi mode
        bool isMultiMode = (vts.getBinauralSoloMode() == 1);
        juce::Colour buttonOnColour = isMultiMode ? juce::Colour(0xFFFF8C00) : juce::Colour(0xFFFFD700);
        soloButton.setColour(juce::TextButton::buttonOnColourId, buttonOnColour);

        // Also update Clear Solo button state
        updateClearSoloButtonState();
    }

    void updateClearSoloButtonState()
    {
        // Check if any inputs are soloed
        auto& vts = parameters.getValueTreeState();
        int numInputs = parameters.getNumInputChannels();
        bool anySoloed = false;
        for (int i = 0; i < numInputs; ++i)
        {
            if (vts.isInputSoloed(i))
            {
                anySoloed = true;
                break;
            }
        }

        // Dim the button when no solos are engaged
        auto disabledColour = ColorScheme::get().textDisabled;
        auto enabledColour = ColorScheme::get().textPrimary;
        clearSoloButton.setColour(juce::TextButton::textColourOffId, anySoloed ? enabledColour : disabledColour);
        clearSoloButton.setColour(juce::TextButton::textColourOnId, anySoloed ? enabledColour : disabledColour);
    }

    void toggleSoloMode()
    {
        auto& vts = parameters.getValueTreeState();
        int currentMode = vts.getBinauralSoloMode();
        int newMode = (currentMode == 0) ? 1 : 0;  // Toggle between Single (0) and Multi (1)
        vts.setBinauralSoloMode(newMode);
        updateSoloModeButtonText();
        updateSoloButtonState();  // Update solo button color
    }

    void updateSoloModeButtonText()
    {
        auto& vts = parameters.getValueTreeState();
        int mode = vts.getBinauralSoloMode();
        if (mode == 0)
            soloModeButton.setButtonText(LOC("inputs.buttons.soloModeSingle"));
        else
            soloModeButton.setButtonText(LOC("inputs.buttons.soloModeMulti"));
    }

    void openSetAllInputsWindow()
    {
        if (setAllInputsWindow == nullptr || !setAllInputsWindow->isVisible())
        {
            setAllInputsWindow = std::make_unique<SetAllInputsWindow>(parameters);
        }
        else
        {
            setAllInputsWindow->toFront(true);
        }
    }

    void updateMapButtonStates()
    {
        // Lock button - show lock icon and state
        auto lockedVal = parameters.getInputParam(currentChannel - 1, "inputMapLocked");
        bool isLocked = !lockedVal.isVoid() && static_cast<int>(lockedVal) != 0;
        // Use Unicode lock symbols
        juce::String lockIcon = isLocked ? juce::String::fromUTF8("\xf0\x9f\x94\x92") : juce::String::fromUTF8("\xf0\x9f\x94\x93");
        mapLockButton.setButtonText(lockIcon + " " + LOC("inputs.buttons.lockOnMap"));

        // Visibility button
        auto visibleVal = parameters.getInputParam(currentChannel - 1, "inputMapVisible");
        bool isVisible = visibleVal.isVoid() || static_cast<int>(visibleVal) != 0;
        mapVisibilityButton.setButtonText(isVisible ? LOC("inputs.buttons.visibleOnMap") : LOC("inputs.buttons.hiddenOnMap"));
    }

    /**
     * Check tracking constraint when assigning input to a cluster (async version).
     * Only one input with tracking enabled is allowed per cluster.
     * Shows dialog if conflict detected, handles result asynchronously.
     */
    void checkTrackingConstraintAsync(int targetCluster, int previousCluster)
    {
        // Check if current input has tracking enabled
        int globalTracking = static_cast<int>(parameters.getConfigParam("trackingEnabled"));
        int protocolEnabled = static_cast<int>(parameters.getConfigParam("trackingProtocol"));
        int localTracking = static_cast<int>(parameters.getInputParam(currentChannel - 1, "inputTrackingActive"));

        bool inputHasTracking = (globalTracking != 0) && (protocolEnabled != 0) && (localTracking != 0);

        if (!inputHasTracking)
        {
            // No conflict, proceed with assignment
            saveInputParam(WFSParameterIDs::inputCluster, targetCluster);
            return;
        }

        // Check if cluster already has a tracked input
        int numInputs = parameters.getNumInputChannels();
        int existingTrackedInput = -1;

        for (int i = 0; i < numInputs; ++i)
        {
            if (i == currentChannel - 1)
                continue;  // Skip current input

            int cluster = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
            if (cluster == targetCluster)
            {
                int inputLocalTracking = static_cast<int>(parameters.getInputParam(i, "inputTrackingActive"));
                if ((globalTracking != 0) && (protocolEnabled != 0) && (inputLocalTracking != 0))
                {
                    existingTrackedInput = i;
                    break;
                }
            }
        }

        if (existingTrackedInput < 0)
        {
            // No conflict, proceed with assignment
            saveInputParam(WFSParameterIDs::inputCluster, targetCluster);
            return;
        }

        // Show conflict dialog asynchronously
        juce::AlertWindow::showOkCancelBox(
            juce::AlertWindow::WarningIcon,
            "Tracking Conflict",
            "Input " + juce::String(currentChannel) + " has tracking enabled, but Input " +
            juce::String(existingTrackedInput + 1) + " in Cluster " + juce::String(targetCluster) +
            " is already tracked.\n\nOnly one tracked input per cluster is allowed.",
            "Continue (disable tracking)",
            "Cancel",
            nullptr,
            juce::ModalCallbackFunction::create([this, targetCluster, previousCluster](int result) {
                if (result == 1)  // Continue
                {
                    // Disable tracking on current input
                    saveInputParam(WFSParameterIDs::inputTrackingActive, 0);
                    trackingActiveButton.setToggleState(false, juce::dontSendNotification);
                    showStatusMessage("Tracking disabled for Input " + juce::String(currentChannel));
                    // Now proceed with cluster assignment
                    saveInputParam(WFSParameterIDs::inputCluster, targetCluster);
                }
                else  // Cancel
                {
                    // Revert cluster selector to previous value
                    clusterSelector.setSelectedId(previousCluster + 1, juce::dontSendNotification);
                }
            })
        );
    }

    /**
     * Check if enabling local tracking on current input would conflict with
     * another input in the same cluster that already has tracking enabled.
     */
    void checkLocalTrackingConstraintAsync()
    {
        // Check what cluster this input belongs to
        int inputCluster = static_cast<int>(parameters.getInputParam(currentChannel - 1, "inputCluster"));

        if (inputCluster == 0)
        {
            // Input is "Single" (not in any cluster), no conflict possible
            trackingActiveButton.setButtonText(LOC("inputs.toggles.trackingOn"));
            saveInputParam(WFSParameterIDs::inputTrackingActive, 1);
            return;
        }

        // Check if global tracking and protocol are enabled (which would make tracking "fully active")
        int globalTracking = static_cast<int>(parameters.getConfigParam("trackingEnabled"));
        int protocolEnabled = static_cast<int>(parameters.getConfigParam("trackingProtocol"));

        if (globalTracking == 0 || protocolEnabled == 0)
        {
            // Global tracking or protocol not enabled, so enabling local tracking won't create a conflict yet
            trackingActiveButton.setButtonText(LOC("inputs.toggles.trackingOn"));
            saveInputParam(WFSParameterIDs::inputTrackingActive, 1);
            return;
        }

        // Check if another input in the same cluster already has tracking enabled
        int numInputs = parameters.getNumInputChannels();
        int existingTrackedInput = -1;

        for (int i = 0; i < numInputs; ++i)
        {
            if (i == currentChannel - 1)
                continue;  // Skip current input

            int cluster = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
            if (cluster == inputCluster)
            {
                int inputLocalTracking = static_cast<int>(parameters.getInputParam(i, "inputTrackingActive"));
                if (inputLocalTracking != 0)
                {
                    existingTrackedInput = i;
                    break;
                }
            }
        }

        if (existingTrackedInput < 0)
        {
            // No conflict, proceed with enabling tracking
            trackingActiveButton.setButtonText(LOC("inputs.toggles.trackingOn"));
            saveInputParam(WFSParameterIDs::inputTrackingActive, 1);
            return;
        }

        // Conflict detected - show warning dialog
        juce::AlertWindow::showOkCancelBox(
            juce::AlertWindow::WarningIcon,
            "Tracking Conflict",
            "Input " + juce::String(existingTrackedInput + 1) + " in Cluster " + juce::String(inputCluster) +
            " already has tracking enabled.\n\nOnly one tracked input per cluster is allowed.\n\n"
            "Do you want to disable tracking on Input " + juce::String(existingTrackedInput + 1) +
            " and enable it on Input " + juce::String(currentChannel) + "?",
            "Yes, switch tracking",
            "Cancel",
            nullptr,
            juce::ModalCallbackFunction::create([this, existingTrackedInput](int result) {
                if (result == 1)  // Yes
                {
                    // Disable tracking on existing input
                    parameters.setInputParam(existingTrackedInput, "inputTrackingActive", 0);
                    // Enable tracking on current input
                    trackingActiveButton.setButtonText(LOC("inputs.toggles.trackingOn"));
                    saveInputParam(WFSParameterIDs::inputTrackingActive, 1);
                    showStatusMessage("Tracking switched from Input " + juce::String(existingTrackedInput + 1) +
                                      " to Input " + juce::String(currentChannel));
                }
                else  // Cancel
                {
                    // Revert button state
                    trackingActiveButton.setToggleState(false, juce::dontSendNotification);
                    trackingActiveButton.setButtonText(LOC("inputs.toggles.trackingOff"));
                }
            })
        );
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
    juce::ValueTree ioTree;
    juce::ValueTree binauralTree;
    bool isLoadingParameters = false;
    StatusBar* statusBar = nullptr;
    AutomOtionProcessor* automOtionProcessor = nullptr;
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
    juce::TextButton mapLockButton;
    juce::TextButton mapVisibilityButton;
    juce::TextButton levelMeterButton;
    juce::TextButton clearSoloButton;
    juce::TextButton soloButton;
    juce::TextButton soloModeButton;
    SetAllInputsLongPressButton setAllInputsButton;
    std::unique_ptr<SetAllInputsWindow> setAllInputsWindow;

    // Snapshot scope
    std::unique_ptr<SnapshotScopeWindow> snapshotScopeWindow;
    std::map<juce::String, WFSFileManager::ExtendedSnapshotScope> snapshotScopes;
    WFSFileManager::ExtendedSnapshotScope currentScope;  // Used when no snapshot selected
    bool currentScopeInitialized = false;

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
    juce::Label coordModeLabel;
    juce::ComboBox coordModeSelector;
    juce::Label posXLabel, posYLabel, posZLabel;
    juce::TextEditor posXEditor, posYEditor, posZEditor;
    juce::Label posXUnitLabel, posYUnitLabel, posZUnitLabel;
    juce::Label offsetXLabel, offsetYLabel, offsetZLabel;
    juce::TextEditor offsetXEditor, offsetYEditor, offsetZEditor;
    juce::Label offsetXUnitLabel, offsetYUnitLabel, offsetZUnitLabel;
    juce::TextButton constraintXButton, constraintYButton, constraintZButton;
    juce::TextButton constraintDistanceButton;
    WfsRangeSlider distanceRangeSlider { 0.0f, 50.0f };
    juce::Label distanceMinLabel, distanceMaxLabel;
    juce::TextEditor distanceMinEditor, distanceMaxEditor;
    juce::Label distanceMinUnitLabel, distanceMaxUnitLabel;
    juce::TextButton flipXButton, flipYButton, flipZButton;
    juce::TextButton trackingActiveButton;
    juce::Label trackingIdLabel;
    juce::ComboBox trackingIdSelector;
    juce::Label trackingSmoothLabel;
    WfsBasicDial trackingSmoothDial;
    juce::Label trackingSmoothValueLabel;
    juce::Label trackingSmoothUnitLabel;
    juce::TextButton maxSpeedActiveButton;
    juce::Label maxSpeedLabel;
    WfsBasicDial maxSpeedDial;
    juce::Label maxSpeedValueLabel;
    juce::Label maxSpeedUnitLabel;
    juce::TextButton pathModeButton;
    juce::Label heightFactorLabel;
    WfsBasicDial heightFactorDial;
    juce::Label heightFactorValueLabel;
    juce::Label heightFactorUnitLabel;
    // Position joystick and Z slider for real-time control
    WfsJoystickComponent positionJoystick;
    juce::Label positionJoystickLabel;
    WfsAutoCenterSlider positionZSlider { WfsAutoCenterSlider::Orientation::vertical };
    juce::Label positionZSliderLabel;

    // Sound tab
    juce::Label attenuationLawLabel;
    juce::TextButton attenuationLawButton;
    juce::Label distanceAttenLabel;
    WfsBasicDial distanceAttenDial;
    juce::Label distanceAttenValueLabel;
    juce::Label distanceAttenUnitLabel;
    juce::Label distanceRatioLabel;
    WfsBasicDial distanceRatioDial;
    juce::Label distanceRatioValueLabel;
    juce::Label distanceRatioUnitLabel;
    juce::Label commonAttenLabel;
    WfsBasicDial commonAttenDial;
    juce::Label commonAttenValueLabel;
    juce::Label commonAttenUnitLabel;
    juce::Label directivityLabel;
    WfsWidthExpansionSlider directivitySlider;
    juce::Label directivityValueLabel;
    juce::Label rotationLabel;
    WfsInputDirectivityDial inputDirectivityDial;
    juce::Label rotationValueLabel;
    juce::Label rotationUnitLabel;
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
    juce::Label lsPeakRatioUnitLabel;
    juce::Label lsSlowThresholdLabel;
    WfsStandardSlider lsSlowThresholdSlider;
    juce::Label lsSlowThresholdValueLabel;
    juce::Label lsSlowRatioLabel;
    WfsBasicDial lsSlowRatioDial;
    juce::Label lsSlowRatioValueLabel;
    juce::Label lsSlowRatioUnitLabel;

    // Effects tab
    juce::TextButton frActiveButton;
    juce::Label frAttenuationLabel;
    WfsStandardSlider frAttenuationSlider;
    juce::Label frAttenuationValueLabel;
    juce::Label frDiffusionLabel;
    WfsBasicDial frDiffusionDial;
    juce::Label frDiffusionValueLabel;
    juce::Label frDiffusionUnitLabel;
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
    juce::TextButton muteReverbSendsButton;

    // L.F.O tab
    juce::TextButton lfoActiveButton;
    juce::Label lfoPeriodLabel;
    WfsBasicDial lfoPeriodDial;
    juce::Label lfoPeriodValueLabel;
    juce::Label lfoPeriodUnitLabel;
    juce::Label lfoPhaseLabel;
    WfsRotationDial lfoPhaseDial;
    juce::Label lfoPhaseValueLabel;
    juce::Label lfoPhaseUnitLabel;
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
    juce::Label lfoPhaseXUnitLabel, lfoPhaseYUnitLabel, lfoPhaseZUnitLabel;
    juce::Label lfoGyrophoneLabel;
    juce::ComboBox lfoGyrophoneSelector;
    juce::Label jitterLabel;
    WfsWidthExpansionSlider jitterSlider;
    juce::Label jitterValueLabel;

    // LFO indicators (read-only feedback)
    WfsLFOProgressDial lfoProgressDial;
    juce::Label lfoOutputXLabel, lfoOutputYLabel, lfoOutputZLabel;
    WfsLFOOutputSlider lfoOutputXSlider, lfoOutputYSlider, lfoOutputZSlider;

    // AutomOtion tab
    juce::Label otomoTitleLabel;
    juce::ComboBox otomoCoordModeSelector;
    juce::Label otomoDestXLabel, otomoDestYLabel, otomoDestZLabel;
    juce::TextEditor otomoDestXEditor, otomoDestYEditor, otomoDestZEditor;
    juce::Label otomoDestXUnitLabel, otomoDestYUnitLabel, otomoDestZUnitLabel;
    juce::TextButton otomoAbsRelButton;
    juce::TextButton otomoStayReturnButton;
    juce::Label otomoDurationLabel;
    WfsBasicDial otomoDurationDial;
    juce::Label otomoDurationValueLabel;
    juce::Label otomoDurationUnitLabel;
    juce::Label otomoCurveLabel;
    WfsBasicDial otomoCurveDial;
    juce::Label otomoCurveValueLabel;
    juce::Label otomoCurveUnitLabel;
    juce::Label otomoSpeedProfileLabel;
    WfsBasicDial otomoSpeedProfileDial;
    juce::Label otomoSpeedProfileValueLabel;
    juce::Label otomoSpeedProfileUnitLabel;
    juce::TextButton otomoTriggerButton;
    juce::Label otomoThresholdLabel;
    WfsBasicDial otomoThresholdDial;
    juce::Label otomoThresholdValueLabel;
    juce::Label otomoThresholdUnitLabel;
    juce::Label otomoResetLabel;
    WfsBasicDial otomoResetDial;
    juce::Label otomoResetValueLabel;
    juce::Label otomoResetUnitLabel;
    PlayButton otomoStartButton;
    StopButton otomoStopButton;
    PauseButton otomoPauseButton;
    juce::TextButton otomoStopAllButton;
    juce::TextButton otomoPauseResumeAllButton;

    // Visualisation tab
    InputVisualisationComponent visualisationComponent;

    // Mutes tab
    juce::TextButton muteButtons[64];
    juce::Label muteMacrosLabel;
    juce::ComboBox muteMacrosSelector;

    // Array attenuation (per-array level control)
    juce::Label arrayAttenLabel;
    juce::Label arrayAttenDialLabels[10];
    WfsBasicDial arrayAttenDials[10];
    juce::Label arrayAttenValueLabels[10];
    juce::Label arrayAttenUnitLabels[10];

    // Sidelines (auto-mute at stage edges)
    juce::TextButton sidelinesActiveButton;
    juce::Label sidelinesFringeLabel;
    WfsBasicDial sidelinesFringeDial;
    juce::Label sidelinesFringeValueLabel;
    juce::Label sidelinesFringeUnitLabel;

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
