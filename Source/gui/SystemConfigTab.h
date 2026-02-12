#pragma once

#include <JuceHeader.h>
#include <set>
#include "../WfsParameters.h"
#include "../Accessibility/TTSManager.h"
#include "../Localization/LocalizationManager.h"
#include "StatusBar.h"
#include "ColorScheme.h"
#include "SliderUIComponents.h"
#include "dials/WfsRotationDial.h"
#include "buttons/LongPressButton.h"

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

        // Draw icon - broken rectangle (open at bottom) with dot at front center
        auto iconBounds = bounds.reduced(6.0f);
        float lineThickness = 2.0f;
        float gapSize = iconBounds.getWidth() * 0.55f;  // Wider gap in the middle of bottom edge

        g.setColour(ColorScheme::get().textPrimary);

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

        // Draw icon - complete rectangle with dot in center
        auto iconBounds = bounds.reduced(6.0f);
        float lineThickness = 2.0f;

        g.setColour(ColorScheme::get().textPrimary);

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

        // Draw icon - 3D cube with dot in center
        auto iconBounds = bounds.reduced(6.0f);
        float lineThickness = 2.0f;
        float depth = iconBounds.getWidth() * 0.3f;  // Depth offset for 3D effect

        g.setColour(ColorScheme::get().textPrimary);

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
                        private juce::TextEditor::Listener,
                        public ColorScheme::Manager::Listener
{
public:
    // Callback types for notifying MainComponent of changes
    using ProcessingCallback = std::function<void(bool enabled)>;
    using ChannelCountCallback = std::function<void(int inputs, int outputs, int reverbs)>;
    using AudioInterfaceCallback = std::function<void()>;
    using ConfigReloadedCallback = std::function<void()>;

    SystemConfigTab(WfsParameters& params)
        : parameters(params)
    {
        // This tab wants keyboard focus to prevent auto-focus on first TextEditor
        setWantsKeyboardFocus(true);

        // Show Section
        addAndMakeVisible(showNameLabel);
        showNameLabel.setText(LOC("systemConfig.labels.showName"), juce::dontSendNotification);
        addAndMakeVisible(showNameEditor);

        addAndMakeVisible(showLocationLabel);
        showLocationLabel.setText(LOC("systemConfig.labels.showLocation"), juce::dontSendNotification);
        addAndMakeVisible(showLocationEditor);

        // I/O Section
        addAndMakeVisible(inputChannelsLabel);
        inputChannelsLabel.setText(LOC("systemConfig.labels.inputChannels"), juce::dontSendNotification);
        addAndMakeVisible(inputChannelsEditor);

        addAndMakeVisible(outputChannelsLabel);
        outputChannelsLabel.setText(LOC("systemConfig.labels.outputChannels"), juce::dontSendNotification);
        addAndMakeVisible(outputChannelsEditor);

        addAndMakeVisible(reverbChannelsLabel);
        reverbChannelsLabel.setText(LOC("systemConfig.labels.reverbChannels"), juce::dontSendNotification);
        addAndMakeVisible(reverbChannelsEditor);

        addAndMakeVisible(audioPatchingButton);
        audioPatchingButton.setButtonText(LOC("systemConfig.buttons.audioPatch"));
        audioPatchingButton.onClick = [this]() {
            if (onAudioInterfaceWindowRequested)
                onAudioInterfaceWindowRequested();
        };

        // Algorithm selector
        addAndMakeVisible(algorithmLabel);
        algorithmLabel.setText(LOC("systemConfig.labels.algorithm"), juce::dontSendNotification);

        addAndMakeVisible(algorithmSelector);
        algorithmSelector.addItem(LOC("systemConfig.algorithms.inputBuffer"), 1);
        algorithmSelector.addItem(LOC("systemConfig.algorithms.outputBuffer"), 2);
        // algorithmSelector.addItem("GPU InputBuffer (GPU Audio)", 3);  // Commented out - GPU Audio SDK not configured
        algorithmSelector.setSelectedId(1, juce::dontSendNotification);
        algorithmSelector.onChange = [this]() {
            int selectedId = algorithmSelector.getSelectedId();
            parameters.setConfigParam("ProcessingAlgorithm", selectedId);
            // TTS: Announce selection change
            TTSManager::getInstance().announceValueChange("Algorithm", algorithmSelector.getText());
        };

        addAndMakeVisible(processingButton);
        processingButton.setButtonText(LOC("systemConfig.buttons.processingOff"));
        processingButton.onClick = [this]() { toggleProcessing(); };

        addAndMakeVisible(soloModeButton);
        updateSoloModeButtonText();
        soloModeButton.onClick = [this]() { toggleSoloMode(); };

        // Stage Section
        addAndMakeVisible(stageShapeLabel);
        stageShapeLabel.setText(LOC("systemConfig.labels.stageShape"), juce::dontSendNotification);
        addAndMakeVisible(stageShapeSelector);
        stageShapeSelector.addItem(LOC("systemConfig.stageShapes.box"), 1);
        stageShapeSelector.addItem(LOC("systemConfig.stageShapes.cylinder"), 2);
        stageShapeSelector.addItem(LOC("systemConfig.stageShapes.dome"), 3);
        stageShapeSelector.setSelectedId(1, juce::dontSendNotification);
        stageShapeSelector.onChange = [this]() {
            onStageShapeChanged();
            // TTS: Announce selection change
            TTSManager::getInstance().announceValueChange("Stage Shape", stageShapeSelector.getText());
        };

        addAndMakeVisible(stageWidthLabel);
        stageWidthLabel.setText(LOC("systemConfig.labels.stageWidth"), juce::dontSendNotification);
        addAndMakeVisible(stageWidthEditor);
        addAndMakeVisible(stageWidthUnitLabel);
        stageWidthUnitLabel.setText(LOC("units.meters"), juce::dontSendNotification);

        addAndMakeVisible(stageDepthLabel);
        stageDepthLabel.setText(LOC("systemConfig.labels.stageDepth"), juce::dontSendNotification);
        addAndMakeVisible(stageDepthEditor);
        addAndMakeVisible(stageDepthUnitLabel);
        stageDepthUnitLabel.setText(LOC("units.meters"), juce::dontSendNotification);

        addAndMakeVisible(stageHeightLabel);
        stageHeightLabel.setText(LOC("systemConfig.labels.stageHeight"), juce::dontSendNotification);
        addAndMakeVisible(stageHeightEditor);
        addAndMakeVisible(stageHeightUnitLabel);
        stageHeightUnitLabel.setText(LOC("units.meters"), juce::dontSendNotification);

        addAndMakeVisible(stageDiameterLabel);
        stageDiameterLabel.setText(LOC("systemConfig.labels.stageDiameter"), juce::dontSendNotification);
        addAndMakeVisible(stageDiameterEditor);
        addAndMakeVisible(stageDiameterUnitLabel);
        stageDiameterUnitLabel.setText(LOC("units.meters"), juce::dontSendNotification);

        addAndMakeVisible(domeElevationLabel);
        domeElevationLabel.setText(LOC("systemConfig.labels.domeElevation"), juce::dontSendNotification);
        addAndMakeVisible(domeElevationEditor);
        addAndMakeVisible(domeElevationUnitLabel);
        domeElevationUnitLabel.setText(LOC("units.degrees"), juce::dontSendNotification);

        addAndMakeVisible(stageOriginWidthLabel);
        stageOriginWidthLabel.setText(LOC("systemConfig.labels.originWidth"), juce::dontSendNotification);
        addAndMakeVisible(stageOriginWidthEditor);
        addAndMakeVisible(stageOriginWidthUnitLabel);
        stageOriginWidthUnitLabel.setText(LOC("units.meters"), juce::dontSendNotification);

        addAndMakeVisible(stageOriginDepthLabel);
        stageOriginDepthLabel.setText(LOC("systemConfig.labels.originDepth"), juce::dontSendNotification);
        addAndMakeVisible(stageOriginDepthEditor);
        addAndMakeVisible(stageOriginDepthUnitLabel);
        stageOriginDepthUnitLabel.setText(LOC("units.meters"), juce::dontSendNotification);

        addAndMakeVisible(stageOriginHeightLabel);
        stageOriginHeightLabel.setText(LOC("systemConfig.labels.originHeight"), juce::dontSendNotification);
        addAndMakeVisible(stageOriginHeightEditor);
        addAndMakeVisible(stageOriginHeightUnitLabel);
        stageOriginHeightUnitLabel.setText(LOC("units.meters"), juce::dontSendNotification);

        // Origin preset buttons (custom drawn icons)
        addAndMakeVisible(originFrontButton);
        originFrontButton.onClick = [this]() { setOriginToFront(); };

        addAndMakeVisible(originCenterGroundButton);
        originCenterGroundButton.onClick = [this]() { setOriginToCenterGround(); };

        addAndMakeVisible(originCenterButton);
        originCenterButton.onClick = [this]() { setOriginToCenter(); };

        addAndMakeVisible(speedOfSoundLabel);
        speedOfSoundLabel.setText(LOC("systemConfig.labels.speedOfSound"), juce::dontSendNotification);
        addAndMakeVisible(speedOfSoundEditor);
        addAndMakeVisible(speedOfSoundUnitLabel);
        speedOfSoundUnitLabel.setText(LOC("units.metersPerSecond"), juce::dontSendNotification);

        addAndMakeVisible(temperatureLabel);
        temperatureLabel.setText(LOC("systemConfig.labels.temperature"), juce::dontSendNotification);
        addAndMakeVisible(temperatureEditor);
        addAndMakeVisible(temperatureUnitLabel);
        temperatureUnitLabel.setText(LOC("units.celsius"), juce::dontSendNotification);

        // Master Section
        addAndMakeVisible(masterLevelLabel);
        masterLevelLabel.setText(LOC("systemConfig.labels.masterLevel"), juce::dontSendNotification);
        addAndMakeVisible(masterLevelEditor);
        addAndMakeVisible(masterLevelUnitLabel);
        masterLevelUnitLabel.setText(LOC("units.decibels"), juce::dontSendNotification);

        addAndMakeVisible(systemLatencyLabel);
        systemLatencyLabel.setText(LOC("systemConfig.labels.systemLatency"), juce::dontSendNotification);
        addAndMakeVisible(systemLatencyEditor);
        addAndMakeVisible(systemLatencyUnitLabel);
        systemLatencyUnitLabel.setText(LOC("units.milliseconds"), juce::dontSendNotification);

        addAndMakeVisible(haasEffectLabel);
        haasEffectLabel.setText(LOC("systemConfig.labels.haasEffect"), juce::dontSendNotification);
        addAndMakeVisible(haasEffectEditor);
        addAndMakeVisible(haasEffectUnitLabel);
        haasEffectUnitLabel.setText(LOC("units.milliseconds"), juce::dontSendNotification);

        // UI Section - Color Scheme
        addAndMakeVisible(colorSchemeLabel);
        colorSchemeLabel.setText(LOC("systemConfig.labels.colorScheme"), juce::dontSendNotification);

        addAndMakeVisible(colorSchemeSelector);
        colorSchemeSelector.addItem(LOC("systemConfig.colorSchemes.default"), 1);
        colorSchemeSelector.addItem(LOC("systemConfig.colorSchemes.black"), 2);
        colorSchemeSelector.addItem(LOC("systemConfig.colorSchemes.light"), 3);
        colorSchemeSelector.setSelectedId(ColorScheme::getThemeIndex() + 1, juce::dontSendNotification);
        colorSchemeSelector.onChange = [this]() {
            int schemeIndex = colorSchemeSelector.getSelectedId() - 1;  // Convert to 0-based
            ColorScheme::Manager::getInstance().setTheme(schemeIndex);
            parameters.setConfigParam("ColorScheme", schemeIndex);
            // TTS: Announce selection change
            TTSManager::getInstance().announceValueChange("Color Scheme", colorSchemeSelector.getText());
        };

        // Language selector
        addAndMakeVisible(languageLabel);
        languageLabel.setText(LOC("systemConfig.labels.language"), juce::dontSendNotification);

        addAndMakeVisible(languageSelector);
        populateLanguageSelector();
        languageSelector.onChange = [this]() {
            int selectedIdx = languageSelector.getSelectedId() - 1;
            if (selectedIdx >= 0 && selectedIdx < availableLanguages.size())
            {
                juce::String locale = availableLanguages[selectedIdx];
                if (LocalizationManager::getInstance().loadLanguage(locale))
                {
                    // Save language to app settings (not project settings)
                    juce::PropertiesFile::Options options;
                    options.applicationName = "WFS-DIY";
                    options.filenameSuffix = ".settings";
                    options.osxLibrarySubFolder = "Application Support";
                    options.folderName = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                            .getChildFile("WFS-DIY").getFullPathName();
                    juce::PropertiesFile props(options);
                    props.setValue("language", locale);
                    props.saveIfNeeded();

                    if (statusBar != nullptr)
                        statusBar->showTemporaryMessage(
                            LocalizationManager::getInstance().get("systemConfig.messages.languageChanged",
                                {{"language", languageSelector.getText()}}), 3000);
                    // TTS: Announce selection change
                    TTSManager::getInstance().announceValueChange("Language", languageSelector.getText());
                }
            }
        };

        // Binaural Section
        addAndMakeVisible(binauralEnableButton);
        binauralEnableButton.setButtonText(LOC("systemConfig.buttons.binauralOff"));
        binauralEnableButton.onClick = [this]() { toggleBinauralProcessing(); };

        addAndMakeVisible(binauralOutputLabel);
        binauralOutputLabel.setText(LOC("systemConfig.labels.binauralOutput"), juce::dontSendNotification);

        // Get output patch tree for listening to patch changes
        auto audioPatchTree = parameters.getValueTreeState().getState()
            .getChildWithName(WFSParameterIDs::AudioPatch);
        outputPatchTree = audioPatchTree.getChildWithName(WFSParameterIDs::OutputPatch);
        outputPatchTree.addListener(this);

        addAndMakeVisible(binauralOutputSelector);
        rebuildBinauralOutputSelector();  // Build dynamically based on available channels
        binauralOutputSelector.onChange = [this]() {
            int selectedId = binauralOutputSelector.getSelectedId();
            int channel = (selectedId == 1) ? -1 : ((selectedId - 2) * 2 + 1);  // Map back to channel
            parameters.getValueTreeState().setBinauralOutputChannel(channel);
            updateBinauralControlsEnabledState();  // Dim controls when output is Off
        };
        binauralOutputSelector.setTooltip(LOC("systemConfig.help.binauralOutput"));

        // Listener Distance - WfsStandardSlider with TextEditor
        addAndMakeVisible(binauralDistanceLabel);
        binauralDistanceLabel.setText(LOC("systemConfig.labels.binauralDistance"), juce::dontSendNotification);

        addAndMakeVisible(binauralDistanceSlider);
        binauralDistanceSlider.setValue(WFSParameterDefaults::binauralListenerDistanceDefault /
                                        WFSParameterDefaults::binauralListenerDistanceMax);
        binauralDistanceSlider.onValueChanged = [this](float v) {
            float distance = v * WFSParameterDefaults::binauralListenerDistanceMax;
            binauralDistanceEditor.setText(juce::String(distance, 1), juce::dontSendNotification);
            auto& vts = parameters.getValueTreeState();
            vts.getBinauralState().setProperty(WFSParameterIDs::binauralListenerDistance, distance, nullptr);
        };
        binauralDistanceSlider.setTooltip(LOC("systemConfig.help.binauralDistance"));

        addAndMakeVisible(binauralDistanceEditor);
        binauralDistanceEditor.setText(juce::String(WFSParameterDefaults::binauralListenerDistanceDefault, 1), juce::dontSendNotification);
        binauralDistanceEditor.setJustification(juce::Justification::centred);
        binauralDistanceEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
        binauralDistanceEditor.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        binauralDistanceEditor.addListener(this);

        addAndMakeVisible(binauralDistanceUnitLabel);
        binauralDistanceUnitLabel.setText("m", juce::dontSendNotification);

        // Listener Angle - WfsRotationDial with TextEditor
        addAndMakeVisible(binauralAngleLabel);
        binauralAngleLabel.setText(LOC("systemConfig.labels.binauralAngle"), juce::dontSendNotification);

        addAndMakeVisible(binauralAngleDial);
        binauralAngleDial.setAngle((float)WFSParameterDefaults::binauralListenerAngleDefault);
        binauralAngleDial.onAngleChanged = [this](float angle) {
            binauralAngleEditor.setText(juce::String((int)angle), juce::dontSendNotification);
            auto& vts = parameters.getValueTreeState();
            vts.getBinauralState().setProperty(WFSParameterIDs::binauralListenerAngle, (int)angle, nullptr);
        };
        binauralAngleDial.setTooltip(LOC("systemConfig.help.binauralAngle"));

        addAndMakeVisible(binauralAngleEditor);
        binauralAngleEditor.setText(juce::String(WFSParameterDefaults::binauralListenerAngleDefault), juce::dontSendNotification);
        binauralAngleEditor.setJustification(juce::Justification::centred);
        binauralAngleEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
        binauralAngleEditor.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        binauralAngleEditor.addListener(this);

        addAndMakeVisible(binauralAngleUnitLabel);
        binauralAngleUnitLabel.setText(juce::String::fromUTF8("\xc2\xb0"), juce::dontSendNotification);

        // Binaural Level - WfsStandardSlider with TextEditor
        addAndMakeVisible(binauralAttenLabel);
        binauralAttenLabel.setText(LOC("systemConfig.labels.binauralAtten"), juce::dontSendNotification);

        addAndMakeVisible(binauralAttenSlider);
        // Convert dB to 0-1 slider value (0 dB = 1.0, -40 dB = 0.0)
        {
            float attenDefault = (WFSParameterDefaults::binauralAttenuationDefault - WFSParameterDefaults::binauralAttenuationMin) /
                                 (WFSParameterDefaults::binauralAttenuationMax - WFSParameterDefaults::binauralAttenuationMin);
            binauralAttenSlider.setValue(attenDefault);
        }
        binauralAttenSlider.onValueChanged = [this](float v) {
            float dB = WFSParameterDefaults::binauralAttenuationMin +
                       v * (WFSParameterDefaults::binauralAttenuationMax - WFSParameterDefaults::binauralAttenuationMin);
            binauralAttenEditor.setText(juce::String(dB, 1), juce::dontSendNotification);
            auto& vts = parameters.getValueTreeState();
            vts.getBinauralState().setProperty(WFSParameterIDs::binauralAttenuation, dB, nullptr);
        };
        binauralAttenSlider.setTooltip(LOC("systemConfig.help.binauralAtten"));

        addAndMakeVisible(binauralAttenEditor);
        binauralAttenEditor.setText(juce::String(WFSParameterDefaults::binauralAttenuationDefault, 1), juce::dontSendNotification);
        binauralAttenEditor.setJustification(juce::Justification::centred);
        binauralAttenEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
        binauralAttenEditor.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        binauralAttenEditor.addListener(this);

        addAndMakeVisible(binauralAttenUnitLabel);
        binauralAttenUnitLabel.setText("dB", juce::dontSendNotification);

        // Binaural Delay - WfsStandardSlider with TextEditor
        addAndMakeVisible(binauralDelayLabel);
        binauralDelayLabel.setText(LOC("systemConfig.labels.binauralDelay"), juce::dontSendNotification);

        addAndMakeVisible(binauralDelaySlider);
        binauralDelaySlider.setValue(WFSParameterDefaults::binauralDelayDefault / WFSParameterDefaults::binauralDelayMax);
        binauralDelaySlider.onValueChanged = [this](float v) {
            float delayMs = v * WFSParameterDefaults::binauralDelayMax;
            binauralDelayEditor.setText(juce::String(delayMs, 1), juce::dontSendNotification);
            auto& vts = parameters.getValueTreeState();
            vts.getBinauralState().setProperty(WFSParameterIDs::binauralDelay, delayMs, nullptr);
        };
        binauralDelaySlider.setTooltip(LOC("systemConfig.help.binauralDelay"));

        addAndMakeVisible(binauralDelayEditor);
        binauralDelayEditor.setText(juce::String(WFSParameterDefaults::binauralDelayDefault, 1), juce::dontSendNotification);
        binauralDelayEditor.setJustification(juce::Justification::centred);
        binauralDelayEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
        binauralDelayEditor.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        binauralDelayEditor.addListener(this);

        addAndMakeVisible(binauralDelayUnitLabel);
        binauralDelayUnitLabel.setText("ms", juce::dontSendNotification);

        // Store/Reload Section
        addAndMakeVisible(selectProjectFolderButton);
        selectProjectFolderButton.setButtonText(LOC("systemConfig.buttons.selectProjectFolder"));
        selectProjectFolderButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF33668C));  // Blueish
        selectProjectFolderButton.onClick = [this]() { selectProjectFolder(); };

        addAndMakeVisible(storeCompleteConfigButton);
        storeCompleteConfigButton.setButtonText(LOC("systemConfig.buttons.storeComplete"));
        storeCompleteConfigButton.setBaseColour(juce::Colour(0xFF8C3333));  // Reddish
        storeCompleteConfigButton.onLongPress = [this]() { storeCompleteConfiguration(); };

        addAndMakeVisible(reloadCompleteConfigButton);
        reloadCompleteConfigButton.setButtonText(LOC("systemConfig.buttons.reloadComplete"));
        reloadCompleteConfigButton.setBaseColour(juce::Colour(0xFF338C33));  // Greenish
        reloadCompleteConfigButton.onLongPress = [this]() { reloadCompleteConfiguration(); };

        addAndMakeVisible(reloadCompleteConfigBackupButton);
        reloadCompleteConfigBackupButton.setButtonText(LOC("systemConfig.buttons.reloadCompleteBackup"));
        reloadCompleteConfigBackupButton.setBaseColour(juce::Colour(0xFF266626));  // Darker green
        reloadCompleteConfigBackupButton.onLongPress = [this]() { reloadCompleteConfigBackup(); };

        addAndMakeVisible(storeSystemConfigButton);
        storeSystemConfigButton.setButtonText(LOC("systemConfig.buttons.storeSystem"));
        storeSystemConfigButton.setBaseColour(juce::Colour(0xFF8C3333));  // Reddish
        storeSystemConfigButton.onLongPress = [this]() { storeSystemConfiguration(); };

        addAndMakeVisible(reloadSystemConfigButton);
        reloadSystemConfigButton.setButtonText(LOC("systemConfig.buttons.reloadSystem"));
        reloadSystemConfigButton.setBaseColour(juce::Colour(0xFF338C33));  // Greenish
        reloadSystemConfigButton.onLongPress = [this]() { reloadSystemConfiguration(); };

        addAndMakeVisible(reloadSystemConfigBackupButton);
        reloadSystemConfigBackupButton.setButtonText(LOC("systemConfig.buttons.reloadSystemBackup"));
        reloadSystemConfigBackupButton.setBaseColour(juce::Colour(0xFF266626));  // Darker green
        reloadSystemConfigBackupButton.onLongPress = [this]() { reloadSystemConfigBackup(); };

        addAndMakeVisible(importSystemConfigButton);
        importSystemConfigButton.setButtonText(LOC("systemConfig.buttons.importSystem"));
        importSystemConfigButton.setBaseColour(juce::Colour(0xFF338C33));  // Greenish
        importSystemConfigButton.onLongPress = [this]() { importSystemConfiguration(); };

        addAndMakeVisible(exportSystemConfigButton);
        exportSystemConfigButton.setButtonText(LOC("systemConfig.buttons.exportSystem"));
        exportSystemConfigButton.setBaseColour(juce::Colour(0xFF8C3333));  // Reddish
        exportSystemConfigButton.onLongPress = [this]() { exportSystemConfiguration(); };

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
        stageDiameterEditor.addListener(this);
        domeElevationEditor.addListener(this);
        stageOriginWidthEditor.addListener(this);
        stageOriginDepthEditor.addListener(this);
        stageOriginHeightEditor.addListener(this);
        speedOfSoundEditor.addListener(this);
        temperatureEditor.addListener(this);
        masterLevelEditor.addListener(this);
        systemLatencyEditor.addListener(this);
        haasEffectEditor.addListener(this);

        // Listen to parameter changes
        configTree = parameters.getConfigTree();
        configTree.addListener(this);

        // Set explicit tab order: left column first (top-down), then right column (top-down)
        // Left column
        showNameEditor.setExplicitFocusOrder(1);
        showLocationEditor.setExplicitFocusOrder(2);
        inputChannelsEditor.setExplicitFocusOrder(3);
        outputChannelsEditor.setExplicitFocusOrder(4);
        reverbChannelsEditor.setExplicitFocusOrder(5);
        // Right column
        stageWidthEditor.setExplicitFocusOrder(6);
        stageDepthEditor.setExplicitFocusOrder(7);
        stageHeightEditor.setExplicitFocusOrder(8);
        stageOriginWidthEditor.setExplicitFocusOrder(9);
        stageOriginDepthEditor.setExplicitFocusOrder(10);
        stageOriginHeightEditor.setExplicitFocusOrder(11);
        speedOfSoundEditor.setExplicitFocusOrder(12);
        temperatureEditor.setExplicitFocusOrder(13);
        masterLevelEditor.setExplicitFocusOrder(14);
        systemLatencyEditor.setExplicitFocusOrder(15);
        haasEffectEditor.setExplicitFocusOrder(16);

        // Load initial values
        loadParametersToUI();

        // Listen for color scheme changes to update dynamic colors
        ColorScheme::Manager::getInstance().addListener(this);
    }

    ~SystemConfigTab() override
    {
        ColorScheme::Manager::getInstance().removeListener(this);
        configTree.removeListener(this);
        if (outputPatchTree.isValid())
            outputPatchTree.removeListener(this);
    }

    /** ColorScheme::Manager::Listener callback - refresh colors when theme changes */
    void colorSchemeChanged() override
    {
        // Re-apply enabled/disabled colors with current theme colors
        updateIOControlsEnabledState();
        updateBinauralControlsEnabledState();

        // Update all TextEditor colors - JUCE TextEditors cache colors internally
        // and don't automatically refresh from LookAndFeel on theme change
        const auto& colors = ColorScheme::get();
        auto updateTextEditor = [&colors](juce::TextEditor& editor) {
            editor.setColour(juce::TextEditor::textColourId, colors.textPrimary);
            editor.setColour(juce::TextEditor::backgroundColourId, colors.surfaceCard);
            editor.setColour(juce::TextEditor::outlineColourId, colors.buttonBorder);
            editor.applyFontToAllText(editor.getFont(), true);  // Force text color refresh
        };

        updateTextEditor(showNameEditor);
        updateTextEditor(showLocationEditor);
        updateTextEditor(stageWidthEditor);
        updateTextEditor(stageDepthEditor);
        updateTextEditor(stageHeightEditor);
        updateTextEditor(stageDiameterEditor);
        updateTextEditor(domeElevationEditor);
        updateTextEditor(stageOriginWidthEditor);
        updateTextEditor(stageOriginDepthEditor);
        updateTextEditor(stageOriginHeightEditor);
        updateTextEditor(speedOfSoundEditor);
        updateTextEditor(temperatureEditor);
        updateTextEditor(masterLevelEditor);
        updateTextEditor(systemLatencyEditor);
        updateTextEditor(haasEffectEditor);

        // Binaural editors use transparent background
        auto updateBinauralEditor = [&colors](juce::TextEditor& editor) {
            editor.setColour(juce::TextEditor::textColourId, colors.textPrimary);
            editor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
            editor.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
            editor.applyFontToAllText(editor.getFont(), true);
        };
        updateBinauralEditor(binauralDistanceEditor);
        updateBinauralEditor(binauralAngleEditor);
        updateBinauralEditor(binauralAttenEditor);
        updateBinauralEditor(binauralDelayEditor);

        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(ColorScheme::get().background);

        // Footer background (matching Input/Output tabs)
        const int footerHeight = 90;  // Two button rows
        g.setColour(ColorScheme::get().chromeSurface);
        g.fillRect(0, getHeight() - footerHeight, getWidth(), footerHeight);

        // Footer divider line
        g.setColour(ColorScheme::get().chromeDivider);
        g.drawLine(0.0f, (float)(getHeight() - footerHeight), (float)getWidth(), (float)(getHeight() - footerHeight), 1.0f);

        // Calculate responsive column positions
        auto layout = calculateLayout();

        // Section headers (bold like NetworkTab)
        // Column 1: Show, I/O (channels only), UI
        // Column 2: Stage, Master
        // Column 3: WFS Processor, Binaural Renderer
        g.setColour(ColorScheme::get().textPrimary);
        g.setFont(juce::FontOptions().withHeight(14.0f).withStyle("Bold"));
        g.drawText(LOC("systemConfig.sections.show"), layout.col1X, 10, layout.colWidth, 20, juce::Justification::left);
        g.drawText(LOC("systemConfig.sections.io"), layout.col1X, 130, layout.colWidth, 20, juce::Justification::left);
        g.drawText(LOC("systemConfig.sections.ui"), layout.col1X, 290, layout.colWidth, 20, juce::Justification::left);
        g.drawText(LOC("systemConfig.sections.stage"), layout.col2X, 10, layout.colWidth, 20, juce::Justification::left);
        g.drawText(LOC("systemConfig.sections.master"), layout.col2X, 400, layout.colWidth, 20, juce::Justification::left);
        g.drawText(LOC("systemConfig.sections.wfsProcessor"), layout.col3X, 10, layout.colWidth, 20, juce::Justification::left);
        g.drawText(LOC("systemConfig.sections.binauralRenderer"), layout.col3X, 200, layout.colWidth, 20, juce::Justification::left);
    }

    void resized() override
    {
        // Calculate responsive layout
        auto layout = calculateLayout();
        const int labelWidth = layout.labelWidth;
        const int editorWidth = layout.editorWidth;
        const int unitWidth = layout.unitWidth;
        const int rowHeight = layout.rowHeight;
        const int spacing = layout.spacing;
        const int fullWidth = layout.colWidth;

        int x = layout.col1X;
        int y = 40;

        //======================================================================
        // COLUMN 1: Show, I/O (channels only), UI
        //======================================================================

        // Show Section
        showNameLabel.setBounds(x, y, labelWidth, rowHeight);
        showNameEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        showLocationLabel.setBounds(x, y, labelWidth, rowHeight);
        showLocationEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);

        // I/O Section (channels only)
        y = 160; // Start after "I/O" header
        inputChannelsLabel.setBounds(x, y, labelWidth, rowHeight);
        inputChannelsEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        outputChannelsLabel.setBounds(x, y, labelWidth, rowHeight);
        outputChannelsEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        reverbChannelsLabel.setBounds(x, y, labelWidth, rowHeight);
        reverbChannelsEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);

        // UI Section
        y = 320; // Start after "UI" header
        colorSchemeLabel.setBounds(x, y, labelWidth, rowHeight);
        colorSchemeSelector.setBounds(x + labelWidth, y, editorWidth * 2, rowHeight);  // Wider for dropdown text
        y += rowHeight + spacing;

        languageLabel.setBounds(x, y, labelWidth, rowHeight);
        languageSelector.setBounds(x + labelWidth, y, editorWidth, rowHeight);

        //======================================================================
        // COLUMN 2: Stage, Master
        //======================================================================

        x = layout.col2X;
        y = 40;

        // Stage Section
        stageShapeLabel.setBounds(x, y, labelWidth, rowHeight);
        stageShapeSelector.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        // Dimension row 1: Width (box) or Diameter (cylinder/dome)
        stageWidthLabel.setBounds(x, y, labelWidth, rowHeight);
        stageWidthEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        stageWidthUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        stageDiameterLabel.setBounds(x, y, labelWidth, rowHeight);
        stageDiameterEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        stageDiameterUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        y += rowHeight + spacing;

        // Dimension row 2: Depth (box) or Height (cylinder) or Elevation (dome)
        stageDepthLabel.setBounds(x, y, labelWidth, rowHeight);
        stageDepthEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        stageDepthUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        domeElevationLabel.setBounds(x, y, labelWidth, rowHeight);
        domeElevationEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        domeElevationUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        y += rowHeight + spacing;

        // Dimension row 3: Height (box/cylinder only)
        stageHeightLabel.setBounds(x, y, labelWidth, rowHeight);
        stageHeightEditor.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        stageHeightUnitLabel.setBounds(x + labelWidth + editorWidth + spacing, y, unitWidth, rowHeight);
        y += rowHeight + spacing;

        // Origin coordinates with preset buttons
        const int originButtonSize = 30;
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
        y = 430;
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

        //======================================================================
        // COLUMN 3: Audio/Processing, Binaural Renderer
        //======================================================================

        x = layout.col3X;
        y = 40;

        // Audio Interface / Processing Section
        audioPatchingButton.setBounds(x, y, fullWidth, rowHeight);
        y += rowHeight + spacing;

        algorithmLabel.setBounds(x, y, labelWidth, rowHeight);
        algorithmSelector.setBounds(x + labelWidth, y, editorWidth * 2, rowHeight);  // Wider for dropdown text
        y += rowHeight + spacing;

        processingButton.setBounds(x, y, fullWidth, rowHeight);

        // Binaural Renderer Section
        y = 230; // Start after "Binaural Renderer" header at y=200
        const int binauralFullWidth = fullWidth;
        const int binauralValueWidth = 60;
        const int binauralUnitWidth = 30;
        const int sliderHeight = 20;

        binauralEnableButton.setBounds(x, y, binauralFullWidth, rowHeight);
        y += rowHeight + spacing;

        binauralOutputLabel.setBounds(x, y, labelWidth, rowHeight);
        binauralOutputSelector.setBounds(x + labelWidth, y, editorWidth, rowHeight);
        y += rowHeight + spacing;

        // Listener Distance
        binauralDistanceLabel.setBounds(x, y, labelWidth, rowHeight);
        binauralDistanceEditor.setBounds(x + binauralFullWidth - binauralValueWidth - binauralUnitWidth, y, binauralValueWidth, rowHeight);
        binauralDistanceUnitLabel.setBounds(x + binauralFullWidth - binauralUnitWidth, y, binauralUnitWidth, rowHeight);
        y += rowHeight;
        binauralDistanceSlider.setBounds(x, y, binauralFullWidth, sliderHeight);
        y += sliderHeight + spacing;

        // Listener Angle
        const int dialSize = 80;
        int dialCenterX = x + binauralFullWidth / 2;
        binauralAngleLabel.setBounds(x, y, binauralFullWidth, rowHeight);
        binauralAngleLabel.setJustificationType(juce::Justification::centred);
        y += rowHeight;
        binauralAngleDial.setBounds(dialCenterX - dialSize / 2, y, dialSize, dialSize);
        y += dialSize;
        const int angleValW = 40, angleUnitW = 20, overlap = 5;
        int angleStartX = dialCenterX - (angleValW + angleUnitW - overlap) / 2;
        binauralAngleEditor.setBounds(angleStartX, y, angleValW, rowHeight);
        binauralAngleUnitLabel.setBounds(angleStartX + angleValW - overlap, y, angleUnitW, rowHeight);
        y += rowHeight + spacing;

        // Binaural Level
        binauralAttenLabel.setBounds(x, y, labelWidth, rowHeight);
        binauralAttenEditor.setBounds(x + binauralFullWidth - binauralValueWidth - binauralUnitWidth, y, binauralValueWidth, rowHeight);
        binauralAttenUnitLabel.setBounds(x + binauralFullWidth - binauralUnitWidth, y, binauralUnitWidth, rowHeight);
        y += rowHeight;
        binauralAttenSlider.setBounds(x, y, binauralFullWidth, sliderHeight);
        y += sliderHeight + spacing;

        // Binaural Delay
        binauralDelayLabel.setBounds(x, y, labelWidth, rowHeight);
        binauralDelayEditor.setBounds(x + binauralFullWidth - binauralValueWidth - binauralUnitWidth, y, binauralValueWidth, rowHeight);
        binauralDelayUnitLabel.setBounds(x + binauralFullWidth - binauralUnitWidth, y, binauralUnitWidth, rowHeight);
        y += rowHeight;
        binauralDelaySlider.setBounds(x, y, binauralFullWidth, sliderHeight);
        y += sliderHeight + spacing;

        // Solo mode button
        soloModeButton.setBounds(x, y, binauralFullWidth, rowHeight);

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

    void setConfigReloadedCallback(ConfigReloadedCallback callback)
    {
        onConfigReloaded = callback;
    }

    /** Grab focus when this tab becomes visible to prevent auto-focus on first TextEditor */
    void visibilityChanged() override
    {
        // Only grab focus if we're actually on screen (isShowing checks for peer)
        if (isShowing())
            grabKeyboardFocus();
    }

    /** Grab focus when clicking on background (not on a child component) */
    void mouseDown(const juce::MouseEvent& e) override
    {
        // Only grab focus if clicking directly on this component (not a child)
        if (e.eventComponent == this)
            grabKeyboardFocus();
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

        // Binaural Section
        setupNumericEditor(binauralDistanceEditor, false, true);  // 0.0 to 10.0
        setupNumericEditor(binauralAngleEditor, true, false);     // -180 to 180 (integer)
        setupNumericEditor(binauralAttenEditor, true, true);      // -40.0 to 0.0
        setupNumericEditor(binauralDelayEditor, false, true);     // 0.0 to 100.0
    }

    //==============================================================================
    // TextEditor::Listener callbacks

    void textEditorTextChanged(juce::TextEditor&) override
    {
        // Don't update parameters during typing - only on Enter or focus lost
    }

    void textEditorReturnKeyPressed(juce::TextEditor& editor) override
    {
        // Set flag to prevent textEditorFocusLost from re-validating
        isValidatingFromReturnKey = true;
        validateAndClampValue(editor);
        editor.giveAwayKeyboardFocus();
        isValidatingFromReturnKey = false;
    }

    void textEditorEscapeKeyPressed(juce::TextEditor& editor) override
    {
        // Restore original value from parameters without updating
        if (&editor == &showNameEditor)
            editor.setText(parameters.getConfigParam("ShowName").toString(), false);
        else if (&editor == &showLocationEditor)
            editor.setText(parameters.getConfigParam("ShowLocation").toString(), false);
        else if (&editor == &inputChannelsEditor)
            editor.setText(juce::String(parameters.getNumInputChannels()), false);
        else if (&editor == &outputChannelsEditor)
            editor.setText(juce::String(parameters.getNumOutputChannels()), false);
        else if (&editor == &reverbChannelsEditor)
            editor.setText(juce::String(parameters.getNumReverbChannels()), false);
        else if (&editor == &stageWidthEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("StageWidth"), 2), false);
        else if (&editor == &stageDepthEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("StageDepth"), 2), false);
        else if (&editor == &stageHeightEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("StageHeight"), 2), false);
        else if (&editor == &stageDiameterEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("StageDiameter"), 2), false);
        else if (&editor == &domeElevationEditor)
            editor.setText(juce::String((float)parameters.getConfigParam("DomeElevation"), 0), false);
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
        // Binaural editors
        else if (&editor == &binauralDistanceEditor)
        {
            auto binauralState = parameters.getValueTreeState().getBinauralState();
            float distance = (float)binauralState.getProperty(WFSParameterIDs::binauralListenerDistance,
                                                               WFSParameterDefaults::binauralListenerDistanceDefault);
            editor.setText(juce::String(distance, 1), false);
        }
        else if (&editor == &binauralAngleEditor)
        {
            auto binauralState = parameters.getValueTreeState().getBinauralState();
            int angle = (int)binauralState.getProperty(WFSParameterIDs::binauralListenerAngle,
                                                        WFSParameterDefaults::binauralListenerAngleDefault);
            editor.setText(juce::String(angle), false);
        }
        else if (&editor == &binauralAttenEditor)
        {
            auto binauralState = parameters.getValueTreeState().getBinauralState();
            float attenDb = (float)binauralState.getProperty(WFSParameterIDs::binauralAttenuation,
                                                              WFSParameterDefaults::binauralAttenuationDefault);
            editor.setText(juce::String(attenDb, 1), false);
        }
        else if (&editor == &binauralDelayEditor)
        {
            auto binauralState = parameters.getValueTreeState().getBinauralState();
            float delayMs = (float)binauralState.getProperty(WFSParameterIDs::binauralDelay,
                                                              WFSParameterDefaults::binauralDelayDefault);
            editor.setText(juce::String(delayMs, 1), false);
        }

        editor.giveAwayKeyboardFocus();
    }

    void textEditorFocusLost(juce::TextEditor& editor) override
    {
        // Skip if we just validated via Return key (prevents double dialog)
        if (isValidatingFromReturnKey)
            return;
        validateAndClampValue(editor);
    }

    //==============================================================================
    // ValueTree::Listener callbacks

    void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property) override
    {
        // Rebuild binaural selector when output patches change
        if (property == WFSParameterIDs::patchData)
            rebuildBinauralOutputSelector();

        loadParametersToUI();
    }

    //==============================================================================
    // Parameter management

    void loadParametersToUI()
    {
        // String values
        showNameEditor.setText(parameters.getConfigParam("ShowName").toString(), false);
        showLocationEditor.setText(parameters.getConfigParam("ShowLocation").toString(), false);

        // Channel counts - use dedicated getters for reliable values
        inputChannelsEditor.setText(juce::String(parameters.getNumInputChannels()), false);
        outputChannelsEditor.setText(juce::String(parameters.getNumOutputChannels()), false);
        reverbChannelsEditor.setText(juce::String(parameters.getNumReverbChannels()), false);

        // Stage shape selector
        int shapeId = (int)parameters.getConfigParam("StageShape");
        stageShapeSelector.setSelectedId(shapeId + 1, juce::dontSendNotification);  // +1 because ComboBox IDs start at 1

        // Float values - format with 2 decimal places to avoid precision issues
        stageWidthEditor.setText(juce::String((float)parameters.getConfigParam("StageWidth"), 2), false);
        stageDepthEditor.setText(juce::String((float)parameters.getConfigParam("StageDepth"), 2), false);
        stageHeightEditor.setText(juce::String((float)parameters.getConfigParam("StageHeight"), 2), false);
        stageDiameterEditor.setText(juce::String((float)parameters.getConfigParam("StageDiameter"), 2), false);
        domeElevationEditor.setText(juce::String((float)parameters.getConfigParam("DomeElevation"), 0), false);
        stageOriginWidthEditor.setText(juce::String((float)parameters.getConfigParam("StageOriginWidth"), 2), false);
        stageOriginDepthEditor.setText(juce::String((float)parameters.getConfigParam("StageOriginDepth"), 2), false);
        stageOriginHeightEditor.setText(juce::String((float)parameters.getConfigParam("StageOriginHeight"), 2), false);
        speedOfSoundEditor.setText(juce::String((float)parameters.getConfigParam("SpeedOfSound"), 2), false);
        temperatureEditor.setText(juce::String((float)parameters.getConfigParam("Temperature"), 2), false);

        // Update visibility based on shape
        updateStageParameterVisibility();
        masterLevelEditor.setText(juce::String((float)parameters.getConfigParam("MasterLevel"), 2), false);
        systemLatencyEditor.setText(juce::String((float)parameters.getConfigParam("SystemLatency"), 2), false);
        haasEffectEditor.setText(juce::String((float)parameters.getConfigParam("HaasEffect"), 2), false);

        // Algorithm selector
        int algorithmId = (int)parameters.getConfigParam("ProcessingAlgorithm");
        if (algorithmId >= 1 && algorithmId <= 2)  // Valid range for current algorithms
            algorithmSelector.setSelectedId(algorithmId, juce::dontSendNotification);

        // Color scheme selector - update UI and apply the theme
        int colorSchemeId = (int)parameters.getConfigParam("ColorScheme");
        colorSchemeSelector.setSelectedId(colorSchemeId + 1, juce::dontSendNotification);  // +1 for ComboBox
        ColorScheme::Manager::getInstance().setTheme(colorSchemeId);  // Actually apply the theme

        // Processing button state
        processingEnabled = (bool)parameters.getConfigParam("ProcessingEnabled");
        processingButton.setButtonText(processingEnabled ? LOC("systemConfig.buttons.processingOn") : LOC("systemConfig.buttons.processingOff"));

        // Solo mode button
        updateSoloModeButtonText();

        // Binaural section
        auto& vts = parameters.getValueTreeState();
        auto binauralState = vts.getBinauralState();

        // Enable button
        bool binauralEnabled = (bool)binauralState.getProperty(WFSParameterIDs::binauralEnabled,
                                                                WFSParameterDefaults::binauralEnabledDefault);
        binauralEnableButton.setButtonText(binauralEnabled ? LOC("systemConfig.buttons.binauralOn")
                                                            : LOC("systemConfig.buttons.binauralOff"));

        int binauralChannel = (int)binauralState.getProperty(WFSParameterIDs::binauralOutputChannel,
                                                              WFSParameterDefaults::binauralOutputChannelDefault);
        int selectedId = (binauralChannel == -1) ? 1 : ((binauralChannel - 1) / 2 + 2);
        binauralOutputSelector.setSelectedId(selectedId, juce::dontSendNotification);

        // Distance
        float distance = (float)binauralState.getProperty(WFSParameterIDs::binauralListenerDistance,
                                                           WFSParameterDefaults::binauralListenerDistanceDefault);
        binauralDistanceSlider.setValue(distance / WFSParameterDefaults::binauralListenerDistanceMax);
        binauralDistanceEditor.setText(juce::String(distance, 1), juce::dontSendNotification);

        // Angle
        int angle = (int)binauralState.getProperty(WFSParameterIDs::binauralListenerAngle,
                                                    WFSParameterDefaults::binauralListenerAngleDefault);
        binauralAngleDial.setAngle((float)angle);
        binauralAngleEditor.setText(juce::String(angle), juce::dontSendNotification);

        // Attenuation
        float attenDb = (float)binauralState.getProperty(WFSParameterIDs::binauralAttenuation,
                                                          WFSParameterDefaults::binauralAttenuationDefault);
        float attenSliderVal = (attenDb - WFSParameterDefaults::binauralAttenuationMin) /
                               (WFSParameterDefaults::binauralAttenuationMax - WFSParameterDefaults::binauralAttenuationMin);
        binauralAttenSlider.setValue(attenSliderVal);
        binauralAttenEditor.setText(juce::String(attenDb, 1), juce::dontSendNotification);

        // Delay
        float delayMs = (float)binauralState.getProperty(WFSParameterIDs::binauralDelay,
                                                          WFSParameterDefaults::binauralDelayDefault);
        binauralDelaySlider.setValue(delayMs / WFSParameterDefaults::binauralDelayMax);
        binauralDelayEditor.setText(juce::String(delayMs, 1), juce::dontSendNotification);

        // Update I/O controls enabled state based on processing state
        updateIOControlsEnabledState();

        // Update binaural controls enabled state based on output selection
        updateBinauralControlsEnabledState();
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
            int newInputs = text.getIntValue();
            int currentInputs = parameters.getNumInputChannels();
            if (newInputs < currentInputs)
            {
                // Prevent multiple dialogs from appearing
                if (isShowingChannelReductionDialog)
                {
                    inputChannelsEditor.setText(juce::String(currentInputs), false);
                    return;
                }
                isShowingChannelReductionDialog = true;

                // Show JUCE AlertWindow confirmation dialog for reducing channels
                auto options = juce::MessageBoxOptions()
                    .withIconType(juce::MessageBoxIconType::WarningIcon)
                    .withTitle(LOC("systemConfig.dialogs.reduceInputChannels.title"))
                    .withMessage(LocalizationManager::getInstance().get(
                        "systemConfig.dialogs.reduceInputChannels.message",
                        {{"current", juce::String(currentInputs)},
                         {"new", juce::String(newInputs)},
                         {"start", juce::String(newInputs + 1)},
                         {"end", juce::String(currentInputs)}}))
                    .withButton(LOC("systemConfig.dialogs.reduce"))
                    .withButton(LOC("common.cancel"))
                    .withAssociatedComponent(this);

                juce::AlertWindow::showAsync(options, [this, newInputs, currentInputs](int result) {
                    isShowingChannelReductionDialog = false;
                    if (result == 1)  // Reduce pressed
                    {
                        parameters.setNumInputChannels(newInputs);
                        notifyChannelCountChanged();
                    }
                    else  // Cancel pressed - restore original value
                    {
                        inputChannelsEditor.setText(juce::String(currentInputs), false);
                    }
                });
            }
            else
            {
                parameters.setNumInputChannels(newInputs);
                notifyChannelCountChanged();
            }
        }
        else if (&editor == &outputChannelsEditor)
        {
            int newOutputs = text.getIntValue();
            int currentOutputs = parameters.getNumOutputChannels();
            if (newOutputs < currentOutputs)
            {
                // Prevent multiple dialogs from appearing
                if (isShowingChannelReductionDialog)
                {
                    outputChannelsEditor.setText(juce::String(currentOutputs), false);
                    return;
                }
                isShowingChannelReductionDialog = true;

                // Show JUCE AlertWindow confirmation dialog for reducing channels
                auto options = juce::MessageBoxOptions()
                    .withIconType(juce::MessageBoxIconType::WarningIcon)
                    .withTitle(LOC("systemConfig.dialogs.reduceOutputChannels.title"))
                    .withMessage(LocalizationManager::getInstance().get(
                        "systemConfig.dialogs.reduceOutputChannels.message",
                        {{"current", juce::String(currentOutputs)},
                         {"new", juce::String(newOutputs)},
                         {"start", juce::String(newOutputs + 1)},
                         {"end", juce::String(currentOutputs)}}))
                    .withButton(LOC("systemConfig.dialogs.reduce"))
                    .withButton(LOC("common.cancel"))
                    .withAssociatedComponent(this);

                juce::AlertWindow::showAsync(options, [this, newOutputs, currentOutputs](int result) {
                    isShowingChannelReductionDialog = false;
                    if (result == 1)  // Reduce pressed
                    {
                        parameters.setNumOutputChannels(newOutputs);
                        notifyChannelCountChanged();
                    }
                    else  // Cancel pressed - restore original value
                    {
                        outputChannelsEditor.setText(juce::String(currentOutputs), false);
                    }
                });
            }
            else
            {
                parameters.setNumOutputChannels(newOutputs);
                notifyChannelCountChanged();
            }
        }
        else if (&editor == &reverbChannelsEditor)
        {
            int newReverbs = text.getIntValue();
            int currentReverbs = parameters.getNumReverbChannels();
            if (newReverbs < currentReverbs)
            {
                // Prevent multiple dialogs from appearing
                if (isShowingChannelReductionDialog)
                {
                    reverbChannelsEditor.setText(juce::String(currentReverbs), false);
                    return;
                }
                isShowingChannelReductionDialog = true;

                // Show JUCE AlertWindow confirmation dialog for reducing channels
                auto options = juce::MessageBoxOptions()
                    .withIconType(juce::MessageBoxIconType::WarningIcon)
                    .withTitle(LOC("systemConfig.dialogs.reduceReverbChannels.title"))
                    .withMessage(LocalizationManager::getInstance().get(
                        "systemConfig.dialogs.reduceReverbChannels.message",
                        {{"current", juce::String(currentReverbs)},
                         {"new", juce::String(newReverbs)},
                         {"start", juce::String(newReverbs + 1)},
                         {"end", juce::String(currentReverbs)}}))
                    .withButton(LOC("systemConfig.dialogs.reduce"))
                    .withButton(LOC("common.cancel"))
                    .withAssociatedComponent(this);

                juce::AlertWindow::showAsync(options, [this, newReverbs, currentReverbs](int result) {
                    isShowingChannelReductionDialog = false;
                    if (result == 1)  // Reduce pressed
                    {
                        parameters.setNumReverbChannels(newReverbs);
                        notifyChannelCountChanged();
                    }
                    else  // Cancel pressed - restore original value
                    {
                        reverbChannelsEditor.setText(juce::String(currentReverbs), false);
                    }
                });
            }
            else
            {
                parameters.setNumReverbChannels(newReverbs);
                notifyChannelCountChanged();
            }
        }
        else if (&editor == &stageWidthEditor)
            parameters.setConfigParam("StageWidth", text.getFloatValue());
        else if (&editor == &stageDepthEditor)
            parameters.setConfigParam("StageDepth", text.getFloatValue());
        else if (&editor == &stageHeightEditor)
            parameters.setConfigParam("StageHeight", text.getFloatValue());
        else if (&editor == &stageDiameterEditor)
            parameters.setConfigParam("StageDiameter", text.getFloatValue());
        else if (&editor == &domeElevationEditor)
            parameters.setConfigParam("DomeElevation", text.getFloatValue());
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
        // Binaural editors
        else if (&editor == &binauralDistanceEditor)
        {
            float distance = text.getFloatValue();
            auto& vts = parameters.getValueTreeState();
            vts.getBinauralState().setProperty(WFSParameterIDs::binauralListenerDistance, distance, nullptr);
            binauralDistanceSlider.setValue(distance / WFSParameterDefaults::binauralListenerDistanceMax);
        }
        else if (&editor == &binauralAngleEditor)
        {
            int angle = text.getIntValue();
            auto& vts = parameters.getValueTreeState();
            vts.getBinauralState().setProperty(WFSParameterIDs::binauralListenerAngle, angle, nullptr);
            binauralAngleDial.setAngle((float)angle);
        }
        else if (&editor == &binauralAttenEditor)
        {
            float attenDb = text.getFloatValue();
            auto& vts = parameters.getValueTreeState();
            vts.getBinauralState().setProperty(WFSParameterIDs::binauralAttenuation, attenDb, nullptr);
            float sliderVal = (attenDb - WFSParameterDefaults::binauralAttenuationMin) /
                              (WFSParameterDefaults::binauralAttenuationMax - WFSParameterDefaults::binauralAttenuationMin);
            binauralAttenSlider.setValue(sliderVal);
        }
        else if (&editor == &binauralDelayEditor)
        {
            float delayMs = text.getFloatValue();
            auto& vts = parameters.getValueTreeState();
            vts.getBinauralState().setProperty(WFSParameterIDs::binauralDelay, delayMs, nullptr);
            binauralDelaySlider.setValue(delayMs / WFSParameterDefaults::binauralDelayMax);
        }
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
        else if (&editor == &stageDiameterEditor)
            value = juce::jlimit(0.1f, 100.0f, std::abs(value));  // Min 0.1m, max 100m
        else if (&editor == &domeElevationEditor)
            value = juce::jlimit(1.0f, 360.0f, std::abs(value));  // Min 1°, max 360°
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
        // Binaural editors
        else if (&editor == &binauralDistanceEditor)
            value = juce::jlimit(WFSParameterDefaults::binauralListenerDistanceMin,
                                 WFSParameterDefaults::binauralListenerDistanceMax, std::abs(value));
        else if (&editor == &binauralAngleEditor)
            value = juce::jlimit((float)WFSParameterDefaults::binauralListenerAngleMin,
                                 (float)WFSParameterDefaults::binauralListenerAngleMax, value);
        else if (&editor == &binauralAttenEditor)
            value = juce::jlimit(WFSParameterDefaults::binauralAttenuationMin,
                                 WFSParameterDefaults::binauralAttenuationMax, value);
        else if (&editor == &binauralDelayEditor)
            value = juce::jlimit(WFSParameterDefaults::binauralDelayMin,
                                 WFSParameterDefaults::binauralDelayMax, std::abs(value));

        // Update display with clamped value
        if (&editor == &inputChannelsEditor || &editor == &outputChannelsEditor ||
            &editor == &reverbChannelsEditor || &editor == &binauralAngleEditor)
        {
            editor.setText(juce::String((int)value), false);
        }
        else if (&editor == &binauralDistanceEditor || &editor == &binauralAttenEditor ||
                 &editor == &binauralDelayEditor)
        {
            editor.setText(juce::String(value, 1), false);  // 1 decimal place for binaural
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
        processingButton.setButtonText(processingEnabled ? LOC("systemConfig.buttons.processingOn") : LOC("systemConfig.buttons.processingOff"));
        parameters.setConfigParam("ProcessingEnabled", processingEnabled);

        // Lock/unlock I/O controls based on processing state
        updateIOControlsEnabledState();

        // Notify MainComponent of processing state change
        if (onProcessingChanged)
            onProcessingChanged(processingEnabled);
    }

    void toggleBinauralProcessing()
    {
        // Don't allow enabling if no output is selected
        if (binauralOutputSelector.getSelectedId() == 1)
            return;

        auto& vts = parameters.getValueTreeState();
        bool currentState = vts.getBinauralEnabled();
        bool newState = !currentState;
        vts.setBinauralEnabled(newState);
        binauralEnableButton.setButtonText(newState ? LOC("systemConfig.buttons.binauralOn")
                                                     : LOC("systemConfig.buttons.binauralOff"));
        updateBinauralControlsEnabledState();
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

        // Visual feedback - dim disabled controls using theme colors
        auto disabledColour = ColorScheme::get().textDisabled;
        auto enabledColour = ColorScheme::get().textPrimary;

        inputChannelsEditor.setColour(juce::TextEditor::textColourId, enabled ? enabledColour : disabledColour);
        outputChannelsEditor.setColour(juce::TextEditor::textColourId, enabled ? enabledColour : disabledColour);
        reverbChannelsEditor.setColour(juce::TextEditor::textColourId, enabled ? enabledColour : disabledColour);
    }

    /** Rebuild the binaural output selector ComboBox based on which channels are used by output patches */
    void rebuildBinauralOutputSelector()
    {
        int currentChannel = parameters.getValueTreeState().getBinauralOutputChannel();

        // Parse patchData to find used hardware channels
        std::set<int> usedChannels;
        juce::String patchData = outputPatchTree.getProperty(WFSParameterIDs::patchData).toString();
        auto rows = juce::StringArray::fromTokens(patchData, ";", "");
        for (int r = 0; r < rows.size(); ++r)
        {
            auto cols = juce::StringArray::fromTokens(rows[r], ",", "");
            for (int c = 0; c < cols.size(); ++c)
                if (cols[c].getIntValue() == 1)
                    usedChannels.insert(c);  // 0-based
        }

        // Rebuild ComboBox
        binauralOutputSelector.clear(juce::dontSendNotification);
        binauralOutputSelector.addItem("Select...", 1);  // No output selected by default

        for (int i = 1; i <= 63; i += 2)
        {
            // Check if either channel in pair (0-based: i-1, i) is used by output patches
            if (usedChannels.count(i - 1) == 0 && usedChannels.count(i) == 0)
                binauralOutputSelector.addItem(juce::String(i) + "-" + juce::String(i + 1), (i / 2) + 2);
        }

        // Restore or reset selection
        int selectedId = (currentChannel == -1) ? 1 : ((currentChannel - 1) / 2 + 2);
        if (binauralOutputSelector.indexOfItemId(selectedId) >= 0)
            binauralOutputSelector.setSelectedId(selectedId, juce::dontSendNotification);
        else
        {
            // Current selection is no longer valid (channel pair now used by patches)
            binauralOutputSelector.setSelectedId(1, juce::dontSendNotification);
            parameters.getValueTreeState().setBinauralOutputChannel(-1);
        }
    }

    void updateBinauralControlsEnabledState()
    {
        // When binaural output is not selected (selectedId == 1), disable/dim all binaural controls
        bool outputSelected = (binauralOutputSelector.getSelectedId() != 1);

        // If output is deselected, turn off binaural processing
        if (!outputSelected)
        {
            auto& vts = parameters.getValueTreeState();
            if (vts.getBinauralEnabled())
            {
                vts.setBinauralEnabled(false);
                binauralEnableButton.setButtonText(LOC("systemConfig.buttons.binauralOff"));
            }
        }

        // Enable/disable the on/off toggle based on output selection
        binauralEnableButton.setEnabled(outputSelected);
        binauralEnableButton.setAlpha(outputSelected ? 1.0f : 0.5f);

        // Parameter controls dim when output unpatched OR toggle is off
        bool binauralActive = outputSelected && parameters.getValueTreeState().getBinauralEnabled();

        // Visual feedback - dim disabled controls using theme colors
        auto disabledColour = ColorScheme::get().textDisabled;
        auto enabledColour = ColorScheme::get().textPrimary;
        // Labels
        binauralDistanceLabel.setColour(juce::Label::textColourId, binauralActive ? enabledColour : disabledColour);
        binauralDistanceUnitLabel.setColour(juce::Label::textColourId, binauralActive ? enabledColour : disabledColour);
        binauralAngleLabel.setColour(juce::Label::textColourId, binauralActive ? enabledColour : disabledColour);
        binauralAngleUnitLabel.setColour(juce::Label::textColourId, binauralActive ? enabledColour : disabledColour);
        binauralAttenLabel.setColour(juce::Label::textColourId, binauralActive ? enabledColour : disabledColour);
        binauralAttenUnitLabel.setColour(juce::Label::textColourId, binauralActive ? enabledColour : disabledColour);
        binauralDelayLabel.setColour(juce::Label::textColourId, binauralActive ? enabledColour : disabledColour);
        binauralDelayUnitLabel.setColour(juce::Label::textColourId, binauralActive ? enabledColour : disabledColour);

        // Text editors
        binauralDistanceEditor.setColour(juce::TextEditor::textColourId, binauralActive ? enabledColour : disabledColour);
        binauralAngleEditor.setColour(juce::TextEditor::textColourId, binauralActive ? enabledColour : disabledColour);
        binauralAttenEditor.setColour(juce::TextEditor::textColourId, binauralActive ? enabledColour : disabledColour);
        binauralDelayEditor.setColour(juce::TextEditor::textColourId, binauralActive ? enabledColour : disabledColour);

        // Sliders - setEnabled drives the isEnabled() check in paintSlider
        binauralDistanceSlider.setEnabled(binauralActive);
        binauralAttenSlider.setEnabled(binauralActive);
        binauralDelaySlider.setEnabled(binauralActive);
        binauralAngleDial.setEnabled(binauralActive);

        // Solo mode button - visually dim when binaural inactive
        soloModeButton.setColour(juce::TextButton::textColourOffId, binauralActive ? enabledColour : disabledColour);
        soloModeButton.setColour(juce::TextButton::textColourOnId, binauralActive ? enabledColour : disabledColour);
    }

    //==============================================================================
    // Solo mode toggle

    void toggleSoloMode()
    {
        auto& vts = parameters.getValueTreeState();
        int currentMode = vts.getBinauralSoloMode();
        int newMode = (currentMode == 0) ? 1 : 0;  // Toggle between Single (0) and Multi (1)
        vts.setBinauralSoloMode(newMode);
        updateSoloModeButtonText();
    }

    void updateSoloModeButtonText()
    {
        auto& vts = parameters.getValueTreeState();
        int mode = vts.getBinauralSoloMode();
        if (mode == 0)
            soloModeButton.setButtonText(LOC("systemConfig.buttons.soloModeSingle"));
        else
            soloModeButton.setButtonText(LOC("systemConfig.buttons.soloModeMulti"));
    }

    //==============================================================================
    // Stage shape methods

    void onStageShapeChanged()
    {
        int shapeId = stageShapeSelector.getSelectedId() - 1;  // Convert to 0-based
        parameters.setConfigParam("StageShape", shapeId);

        // Don't reset dimensions - preserve user-edited values when switching shapes
        // The origin is recalculated based on current dimensions for the new shape

        // Recalculate origin for the new shape using current dimensions
        setOriginToCenterGround();

        // Update UI visibility and refresh values
        loadParametersToUI();
    }

    void updateStageParameterVisibility()
    {
        int shapeId = stageShapeSelector.getSelectedId() - 1;  // 0=box, 1=cylinder, 2=dome

        // Box: show width, depth, height
        bool isBox = (shapeId == 0);
        stageWidthLabel.setVisible(isBox);
        stageWidthEditor.setVisible(isBox);
        stageWidthUnitLabel.setVisible(isBox);
        stageDepthLabel.setVisible(isBox);
        stageDepthEditor.setVisible(isBox);
        stageDepthUnitLabel.setVisible(isBox);

        // Cylinder/Dome: show diameter
        bool isCircular = (shapeId != 0);
        stageDiameterLabel.setVisible(isCircular);
        stageDiameterEditor.setVisible(isCircular);
        stageDiameterUnitLabel.setVisible(isCircular);

        // Box/Cylinder: show height
        bool hasHeight = (shapeId == 0 || shapeId == 1);
        stageHeightLabel.setVisible(hasHeight);
        stageHeightEditor.setVisible(hasHeight);
        stageHeightUnitLabel.setVisible(hasHeight);

        // Dome: show elevation
        bool isDome = (shapeId == 2);
        domeElevationLabel.setVisible(isDome);
        domeElevationEditor.setVisible(isDome);
        domeElevationUnitLabel.setVisible(isDome);
    }

    //==============================================================================
    // Origin preset methods

    void setOriginToFront()
    {
        // Front-center of stage: X = center, Y = front edge, Z = floor
        int shapeId = stageShapeSelector.getSelectedId() - 1;
        float frontOffset = 0.0f;

        if (shapeId == 0)  // Box
            frontOffset = -((float)parameters.getConfigParam("StageDepth")) * 0.5f;
        else  // Cylinder/Dome
            frontOffset = -((float)parameters.getConfigParam("StageDiameter")) * 0.5f;

        parameters.setConfigParam("StageOriginWidth", 0.0f);
        parameters.setConfigParam("StageOriginDepth", frontOffset);
        parameters.setConfigParam("StageOriginHeight", 0.0f);
    }

    void setOriginToCenterGround()
    {
        // Center at ground level: all offsets zero (center-referenced)
        parameters.setConfigParam("StageOriginWidth", 0.0f);
        parameters.setConfigParam("StageOriginDepth", 0.0f);
        parameters.setConfigParam("StageOriginHeight", 0.0f);
    }

    void setOriginToCenter()
    {
        // Center of stage cube/cylinder/dome: X = center, Y = center, Z = half height
        int shapeId = stageShapeSelector.getSelectedId() - 1;
        float halfHeight = 0.0f;

        if (shapeId == 2)  // Dome - use diameter for spherical center
            halfHeight = ((float)parameters.getConfigParam("StageDiameter")) * 0.5f;
        else  // Box/Cylinder
            halfHeight = ((float)parameters.getConfigParam("StageHeight")) * 0.5f;

        parameters.setConfigParam("StageOriginWidth", 0.0f);
        parameters.setConfigParam("StageOriginDepth", 0.0f);
        parameters.setConfigParam("StageOriginHeight", halfHeight);
    }

    //==============================================================================
    // Store/Reload methods

    void selectProjectFolder()
    {
        auto chooser = std::make_shared<juce::FileChooser>(LOC("systemConfig.dialogs.selectProjectFolder"), projectFolder);
        auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories;

        chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result.exists() && result.isDirectory())
            {
                projectFolder = result;
                parameters.setConfigParam("ProjectFolder", projectFolder.getFullPathName());
                parameters.getFileManager().setProjectFolder(projectFolder);
            }
        });
    }

    void storeCompleteConfiguration()
    {
        auto& fileManager = parameters.getFileManager();

        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage(LOC("systemConfig.messages.selectFolderFirst"));
            return;
        }

        // Backup is created automatically by file manager before overwrite
        if (fileManager.saveCompleteConfig())
            showStatusMessage(LOC("systemConfig.messages.configSaved"));
        else
            showStatusMessage(LocalizationManager::getInstance().get("systemConfig.messages.error", {{"error", fileManager.getLastError()}}));
    }

    void reloadCompleteConfiguration()
    {
        auto& fileManager = parameters.getFileManager();

        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage(LOC("systemConfig.messages.selectFolderFirst"));
            return;
        }

        // Load complete config from individual files (system.xml, network.xml, inputs.xml, outputs.xml, reverbs.xml)
        bool success = fileManager.loadCompleteConfig();

        if (success)
            showStatusMessage(LOC("systemConfig.messages.configLoaded"));
        else
            showStatusMessage(LocalizationManager::getInstance().get("systemConfig.messages.partialLoad", {{"error", fileManager.getLastError()}}));

        // Always refresh UI and trigger recalculation, even on partial load
        // Some files may have loaded successfully (e.g., system and outputs but not reverbs)

        // Notify MainComponent of channel count changes to refresh UI
        notifyChannelCountChanged();

        // Refresh the UI to show loaded values
        loadParametersToUI();

        // Notify MainComponent to refresh all tabs
        if (onConfigReloaded)
            onConfigReloaded();
    }

    void reloadCompleteConfigBackup()
    {
        auto& fileManager = parameters.getFileManager();

        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage(LOC("systemConfig.messages.selectFolderFirst"));
            return;
        }

        auto backups = fileManager.getBackups("complete");
        if (backups.isEmpty())
        {
            showStatusMessage(LOC("systemConfig.messages.noBackupFilesFound"));
            return;
        }

        bool success = fileManager.loadCompleteConfigBackup(0);

        if (success)
            showStatusMessage(LOC("systemConfig.messages.configLoadedFromBackup"));
        else
            showStatusMessage(LocalizationManager::getInstance().get("systemConfig.messages.partialLoadFromBackup", {{"error", fileManager.getLastError()}}));

        // Always refresh UI and trigger recalculation, even on partial load

        // Notify MainComponent of channel count changes to refresh UI
        notifyChannelCountChanged();

        // Refresh the UI to show loaded values
        loadParametersToUI();

        // Notify MainComponent to refresh all tabs
        if (onConfigReloaded)
            onConfigReloaded();
    }

    void storeSystemConfiguration()
    {
        auto& fileManager = parameters.getFileManager();

        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage(LOC("systemConfig.messages.selectFolderFirst"));
            return;
        }

        // Backup is created automatically by file manager before overwrite
        if (fileManager.saveSystemConfig())
            showStatusMessage(LOC("systemConfig.messages.systemConfigSaved"));
        else
            showStatusMessage(LocalizationManager::getInstance().get("systemConfig.messages.error", {{"error", fileManager.getLastError()}}));
    }

    void reloadSystemConfiguration()
    {
        auto& fileManager = parameters.getFileManager();

        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage(LOC("systemConfig.messages.selectFolderFirst"));
            return;
        }

        auto configFile = fileManager.getSystemConfigFile();
        if (!configFile.existsAsFile())
        {
            showStatusMessage(LOC("systemConfig.messages.systemConfigFileNotFound"));
            return;
        }

        if (fileManager.loadSystemConfig())
        {
            showStatusMessage(LOC("systemConfig.messages.systemConfigLoaded"));

            // Update UI from ValueTree
            loadParametersToUI();

            // Notify MainComponent to refresh all tabs
            if (onConfigReloaded)
                onConfigReloaded();
        }
        else
        {
            showStatusMessage(LocalizationManager::getInstance().get("systemConfig.messages.error", {{"error", fileManager.getLastError()}}));
        }
    }

    void reloadSystemConfigBackup()
    {
        auto& fileManager = parameters.getFileManager();

        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage(LOC("systemConfig.messages.selectFolderFirst"));
            return;
        }

        auto backups = fileManager.getBackups("system");
        if (backups.isEmpty())
        {
            showStatusMessage(LOC("systemConfig.messages.noBackupFilesFound"));
            return;
        }

        if (fileManager.loadSystemConfigBackup(0))
        {
            showStatusMessage(LOC("systemConfig.messages.systemConfigLoadedFromBackup"));

            // Update UI from ValueTree
            loadParametersToUI();

            // Notify MainComponent to refresh all tabs
            if (onConfigReloaded)
                onConfigReloaded();
        }
        else
        {
            showStatusMessage(LocalizationManager::getInstance().get("systemConfig.messages.error", {{"error", fileManager.getLastError()}}));
        }
    }

    void importSystemConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser>(LOC("systemConfig.dialogs.importSystemConfig"),
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.xml");
        auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result.existsAsFile())
            {
                auto& fileManager = parameters.getFileManager();
                if (fileManager.importSystemConfig(result))
                {
                    showStatusMessage(LOC("systemConfig.messages.systemConfigImported"));

                    // Update UI from ValueTree
                    loadParametersToUI();

                    // Notify MainComponent to refresh all tabs
                    if (onConfigReloaded)
                        onConfigReloaded();
                }
                else
                {
                    showStatusMessage(LocalizationManager::getInstance().get("systemConfig.messages.error", {{"error", fileManager.getLastError()}}));
                }
            }
        });
    }

    void exportSystemConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser>(LOC("systemConfig.dialogs.exportSystemConfig"),
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

                auto& fileManager = parameters.getFileManager();
                if (fileManager.exportSystemConfig(result))
                    showStatusMessage(LOC("systemConfig.messages.systemConfigExported"));
                else
                    showStatusMessage(LocalizationManager::getInstance().get("systemConfig.messages.error", {{"error", fileManager.getLastError()}}));
            }
        });
    }

    //==============================================================================
    // Language selector helper

    void populateLanguageSelector()
    {
        languageSelector.clear();
        availableLanguages.clear();

        // Get available languages from LocalizationManager
        auto& locMgr = LocalizationManager::getInstance();
        availableLanguages = locMgr.getAvailableLanguages();

        // If no language files found, add English as default
        if (availableLanguages.isEmpty())
            availableLanguages.add("en");

        // Map locale codes to native language names (using Unicode escapes for safety)
        std::map<juce::String, juce::String> languageNames = {
            {"en", "English"},
            {"fr", juce::String::fromUTF8("Fran\xc3\xa7" "ais")},      // Français
            {"de", "Deutsch"},
            {"es", juce::String::fromUTF8("Espa\xc3\xb1" "ol")},       // Español
            {"it", "Italiano"},
            {"pt", juce::String::fromUTF8("Portugu\xc3\xaa" "s")},     // Português
            {"ja", juce::String::fromUTF8("\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e")},  // 日本語
            {"zh", juce::String::fromUTF8("\xe4\xb8\xad\xe6\x96\x87")},              // 中文
            {"ko", juce::String::fromUTF8("\xed\x95\x9c\xea\xb5\xad\xec\x96\xb4")}   // 한국어
        };

        // Populate ComboBox
        int selectedId = 1;
        for (int i = 0; i < availableLanguages.size(); ++i)
        {
            const auto& locale = availableLanguages[i];
            juce::String displayName = languageNames.count(locale) > 0
                ? languageNames.at(locale)
                : locale.toUpperCase();
            languageSelector.addItem(displayName, i + 1);

            if (locale == locMgr.getCurrentLocale())
                selectedId = i + 1;
        }

        languageSelector.setSelectedId(selectedId, juce::dontSendNotification);
    }

    //==============================================================================
    // Status bar helper methods

    void showStatusMessage(const juce::String& message, int durationMs = 3000)
    {
        if (statusBar != nullptr)
            statusBar->showTemporaryMessage(message, durationMs);
    }

    void setupHelpText()
    {
        // Help text from localization
        helpTextMap[&showNameEditor] = LOC("systemConfig.help.showName");
        helpTextMap[&showLocationEditor] = LOC("systemConfig.help.showLocation");
        helpTextMap[&inputChannelsEditor] = LOC("systemConfig.help.inputChannels");
        helpTextMap[&outputChannelsEditor] = LOC("systemConfig.help.outputChannels");
        helpTextMap[&reverbChannelsEditor] = LOC("systemConfig.help.reverbChannels");
        helpTextMap[&audioPatchingButton] = LOC("systemConfig.help.audioPatch");
        helpTextMap[&algorithmSelector] = LOC("systemConfig.help.algorithm");
        helpTextMap[&processingButton] = LOC("systemConfig.help.processing");
        helpTextMap[&soloModeButton] = LOC("systemConfig.help.soloMode");
        helpTextMap[&stageShapeSelector] = LOC("systemConfig.help.stageShape");
        helpTextMap[&stageWidthEditor] = LOC("systemConfig.help.stageWidth");
        helpTextMap[&stageDepthEditor] = LOC("systemConfig.help.stageDepth");
        helpTextMap[&stageHeightEditor] = LOC("systemConfig.help.stageHeight");
        helpTextMap[&stageDiameterEditor] = LOC("systemConfig.help.stageDiameter");
        helpTextMap[&domeElevationEditor] = LOC("systemConfig.help.domeElevation");
        helpTextMap[&stageOriginWidthEditor] = LOC("systemConfig.help.originWidth");
        helpTextMap[&stageOriginDepthEditor] = LOC("systemConfig.help.originDepth");
        helpTextMap[&stageOriginHeightEditor] = LOC("systemConfig.help.originHeight");
        helpTextMap[&originFrontButton] = LOC("systemConfig.help.originFront");
        helpTextMap[&originCenterGroundButton] = LOC("systemConfig.help.originCenterGround");
        helpTextMap[&originCenterButton] = LOC("systemConfig.help.originCenter");
        helpTextMap[&speedOfSoundEditor] = LOC("systemConfig.help.speedOfSound");
        helpTextMap[&temperatureEditor] = LOC("systemConfig.help.temperature");
        helpTextMap[&masterLevelEditor] = LOC("systemConfig.help.masterLevel");
        helpTextMap[&systemLatencyEditor] = LOC("systemConfig.help.systemLatency");
        helpTextMap[&haasEffectEditor] = LOC("systemConfig.help.haasEffect");
        helpTextMap[&colorSchemeSelector] = LOC("systemConfig.help.colorScheme");
        helpTextMap[&languageSelector] = LOC("systemConfig.help.language");
        helpTextMap[&binauralEnableButton] = LOC("systemConfig.help.binauralEnable");
        helpTextMap[&binauralOutputSelector] = LOC("systemConfig.help.binauralOutput");
        helpTextMap[&binauralDistanceSlider] = LOC("systemConfig.help.binauralDistance");
        helpTextMap[&binauralDistanceEditor] = LOC("systemConfig.help.binauralDistance");
        helpTextMap[&binauralAngleDial] = LOC("systemConfig.help.binauralAngle");
        helpTextMap[&binauralAngleEditor] = LOC("systemConfig.help.binauralAngle");
        helpTextMap[&binauralAttenSlider] = LOC("systemConfig.help.binauralAtten");
        helpTextMap[&binauralAttenEditor] = LOC("systemConfig.help.binauralAtten");
        helpTextMap[&binauralDelaySlider] = LOC("systemConfig.help.binauralDelay");
        helpTextMap[&binauralDelayEditor] = LOC("systemConfig.help.binauralDelay");
        helpTextMap[&selectProjectFolderButton] = LOC("systemConfig.help.selectProjectFolder");
        helpTextMap[&storeCompleteConfigButton] = LOC("systemConfig.help.storeComplete");
        helpTextMap[&reloadCompleteConfigButton] = LOC("systemConfig.help.reloadComplete");
        helpTextMap[&reloadCompleteConfigBackupButton] = LOC("systemConfig.help.reloadCompleteBackup");
        helpTextMap[&storeSystemConfigButton] = LOC("systemConfig.help.storeSystem");
        helpTextMap[&reloadSystemConfigButton] = LOC("systemConfig.help.reloadSystem");
        helpTextMap[&reloadSystemConfigBackupButton] = LOC("systemConfig.help.reloadSystemBackup");
        helpTextMap[&importSystemConfigButton] = LOC("systemConfig.help.importSystem");
        helpTextMap[&exportSystemConfigButton] = LOC("systemConfig.help.exportSystem");
    }

    void setupMouseListeners()
    {
        // Enable mouse enter/exit events for all components with help text
        for (auto& pair : helpTextMap)
        {
            pair.first->setMouseCursor(juce::MouseCursor::PointingHandCursor);
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

    //==============================================================================
    // Layout calculation helper

    struct LayoutMetrics {
        int col1X, col2X, col3X;
        int colWidth;
        int labelWidth, editorWidth;
        int unitWidth = 40;
        int rowHeight = 30;
        int spacing = 5;
    };

    LayoutMetrics calculateLayout() const
    {
        LayoutMetrics m;
        const int margin = 20;
        const int columnGap = 50;  // Increased gap between columns
        const int totalWidth = getWidth() - margin * 2;

        m.colWidth = (totalWidth - columnGap * 2) / 3;
        m.col1X = margin;
        m.col2X = m.col1X + m.colWidth + columnGap;
        m.col3X = m.col2X + m.colWidth + columnGap;
        m.labelWidth = juce::jmax(100, m.colWidth * 40 / 100);
        // Editor width accounts for: label + editor + spacing + unit label + spacing + button
        // colWidth = labelWidth + editorWidth + spacing(5) + unitWidth(40) + buffer
        m.editorWidth = juce::jmin(120, m.colWidth - m.labelWidth - m.unitWidth - m.spacing * 2);

        return m;
    }

    //==============================================================================
    // Member variables

    WfsParameters& parameters;
    juce::ValueTree configTree;  // Store for safe listener removal in destructor
    StatusBar* statusBar = nullptr;
    std::map<juce::Component*, juce::String> helpTextMap;
    bool processingEnabled = false;
    bool isValidatingFromReturnKey = false;  // Prevents double validation when Enter triggers both ReturnKey and FocusLost
    bool isShowingChannelReductionDialog = false;  // Prevents multiple dialogs from appearing

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
    juce::TextButton soloModeButton;

    // Stage Section
    juce::Label stageShapeLabel;
    juce::ComboBox stageShapeSelector;
    juce::Label stageWidthLabel;
    juce::TextEditor stageWidthEditor;
    juce::Label stageWidthUnitLabel;
    juce::Label stageDepthLabel;
    juce::TextEditor stageDepthEditor;
    juce::Label stageDepthUnitLabel;
    juce::Label stageHeightLabel;
    juce::TextEditor stageHeightEditor;
    juce::Label stageHeightUnitLabel;
    juce::Label stageDiameterLabel;
    juce::TextEditor stageDiameterEditor;
    juce::Label stageDiameterUnitLabel;
    juce::Label domeElevationLabel;
    juce::TextEditor domeElevationEditor;
    juce::Label domeElevationUnitLabel;
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

    // UI Section
    juce::Label colorSchemeLabel;
    juce::ComboBox colorSchemeSelector;
    juce::Label languageLabel;
    juce::ComboBox languageSelector;
    juce::StringArray availableLanguages;

    // Binaural Section
    juce::TextButton binauralEnableButton;
    juce::Label binauralOutputLabel;
    juce::ComboBox binauralOutputSelector;
    juce::ValueTree outputPatchTree;  // For listening to patch changes
    juce::Label binauralDistanceLabel;
    WfsStandardSlider binauralDistanceSlider;
    juce::TextEditor binauralDistanceEditor;
    juce::Label binauralDistanceUnitLabel;
    juce::Label binauralAngleLabel;
    WfsRotationDial binauralAngleDial;
    juce::TextEditor binauralAngleEditor;
    juce::Label binauralAngleUnitLabel;
    juce::Label binauralAttenLabel;
    WfsStandardSlider binauralAttenSlider;
    juce::TextEditor binauralAttenEditor;
    juce::Label binauralAttenUnitLabel;
    juce::Label binauralDelayLabel;
    WfsStandardSlider binauralDelaySlider;
    juce::TextEditor binauralDelayEditor;
    juce::Label binauralDelayUnitLabel;

    // Store/Reload Section
    juce::TextButton selectProjectFolderButton;
    LongPressButton storeCompleteConfigButton;
    LongPressButton reloadCompleteConfigButton;
    LongPressButton reloadCompleteConfigBackupButton;
    LongPressButton storeSystemConfigButton;
    LongPressButton reloadSystemConfigButton;
    LongPressButton reloadSystemConfigBackupButton;
    LongPressButton importSystemConfigButton;
    LongPressButton exportSystemConfigButton;
    juce::File projectFolder;

    // Callbacks for notifying MainComponent
    ProcessingCallback onProcessingChanged;
    ChannelCountCallback onChannelCountChanged;
    AudioInterfaceCallback onAudioInterfaceWindowRequested;
    ConfigReloadedCallback onConfigReloaded;

    // Helper to notify MainComponent of any channel count change
    void notifyChannelCountChanged()
    {
        if (onChannelCountChanged)
        {
            int inputs = parameters.getNumInputChannels();
            int outputs = parameters.getNumOutputChannels();
            int reverbs = parameters.getNumReverbChannels();
            onChannelCountChanged(inputs, outputs, reverbs);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SystemConfigTab)
};
