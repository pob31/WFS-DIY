#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "../Accessibility/TTSManager.h"
#include "../Localization/LocalizationManager.h"
#include "ChannelSelector.h"
#include "ColorScheme.h"
#include "SliderUIComponents.h"
#include "DialUIComponents.h"
#include "StatusBar.h"
#include "EQDisplayComponent.h"
#include "../Helpers/CoordinateConverter.h"
#include "buttons/LongPressButton.h"
#include "buttons/EQBandToggle.h"

/**
 * Reverb Tab Component
 * Configuration for reverb channels with 6 sub-tabs
 */
class ReverbTab : public juce::Component,
                  private juce::TextEditor::Listener,
                  private juce::ChangeListener,
                  private juce::Label::Listener,
                  private juce::ValueTree::Listener,
                  public ColorScheme::Manager::Listener
{
public:
    ReverbTab (WfsParameters& params)
        : parameters (params),
          reverbsTree (params.getReverbTree()),
          configTree (params.getConfigTree()),
          ioTree (params.getConfigTree().getChildWithName (WFSParameterIDs::IO))
    {
        // Enable keyboard focus so we can receive focus back after text editing
        setWantsKeyboardFocus(true);

        reverbsTree.addListener (this);
        configTree.addListener (this);
        if (ioTree.isValid())
            ioTree.addListener (this);
        ColorScheme::Manager::getInstance().addListener(this);

        setupHeader();
        setupSubTabs();
        setupReverbSubTab();
        setupPositionSubTab();
        setupReverbFeedSubTab();
        setupEQSubTab();
        setupPreCompressorControls();
        setupAlgorithmSubTab();
        setupPostProcessingSubTab();
        setupPostExpanderControls();
        setupReverbReturnSubTab();
        setupFooter();
        setupHelpText();
        setupOscMethods();
        setupMouseListeners();

        // Setup "no channels" message
        noChannelsLabel.setText (LOC("reverbs.noChannels"),
                                 juce::dontSendNotification);
        noChannelsLabel.setJustificationType (juce::Justification::centred);
        noChannelsLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
        addChildComponent (noChannelsLabel);  // Hidden by default

        int numReverbs = parameters.getNumReverbChannels();
        channelSelector.setNumChannels (numReverbs > 0 ? numReverbs : 1);

        if (numReverbs > 0)
            loadChannelParameters (1);

        loadAlgorithmParameters();
        loadPreCompParameters();
        loadPostEQParameters();
        loadPostExpParameters();
        updateVisibility();
    }

    ~ReverbTab() override
    {
        ColorScheme::Manager::getInstance().removeListener(this);
        reverbsTree.removeListener (this);
        configTree.removeListener (this);
        if (ioTree.isValid())
            ioTree.removeListener (this);
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
        updateTextEditor(returnOffsetXEditor);
        updateTextEditor(returnOffsetYEditor);
        updateTextEditor(returnOffsetZEditor);

        repaint();
    }

    //==========================================================================
    // Public Interface
    //==========================================================================

    int getCurrentChannel() const { return currentChannel; }

    void selectChannel (int channel)
    {
        channelSelector.setSelectedChannelProgrammatically (channel);
    }

    int getNumChannels() const { return channelSelector.getNumChannels(); }

    /** Refresh UI from ValueTree - call after config reload */
    void refreshFromValueTree()
    {
        // Re-acquire reverbsTree reference in case config was replaced
        auto newReverbsTree = parameters.getReverbTree();
        if (newReverbsTree != reverbsTree)
        {
            reverbsTree.removeListener(this);
            reverbsTree = newReverbsTree;
            reverbsTree.addListener(this);
        }

        // Re-acquire configTree reference
        auto newConfigTree = parameters.getConfigTree();
        if (newConfigTree != configTree)
        {
            configTree.removeListener(this);
            configTree = newConfigTree;
            configTree.addListener(this);
        }

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

        // Reset EQ displays - they hold references to old ValueTrees which are now stale
        // They will be recreated below with the new trees
        eqDisplay.reset();
        lastEqDisplayChannel = -1;
        postEqDisplay.reset();

        // Update channel selector count
        int numReverbs = parameters.getNumReverbChannels();
        if (numReverbs > 0)
        {
            channelSelector.setNumChannels(numReverbs);
            if (currentChannel > numReverbs)
                currentChannel = 1;

            // Load channel parameters to update UI controls
            loadChannelParameters(currentChannel);

            // ALWAYS ensure eqDisplay is created and laid out, regardless of current sub-tab
            // This prevents timing issues where the display isn't ready when switching tabs
            if (eqDisplay == nullptr && currentChannel > 0)
            {
                auto eqTree = parameters.getValueTreeState().ensureReverbEQSection (currentChannel - 1);
                if (eqTree.isValid())
                {
                    eqDisplay = std::make_unique<EQDisplayComponent> (eqTree, numEqBands, EQDisplayConfig::forReverbPreEQ());
                    addAndMakeVisible (*eqDisplay);
                    eqDisplay->setUndoManager (parameters.getUndoManagerForDomain (UndoDomain::Reverb));
                    lastEqDisplayChannel = currentChannel;
                    eqDisplay->setEQEnabled (eqEnableButton.getToggleState());
                }
            }
        }

        updateVisibility();
        resized();  // Re-layout components after visibility change

        // If we're on the EQ tab, force re-layout to ensure eqDisplay gets bounds
        if (subTabBar.getCurrentTabIndex() == 1)
            layoutEQSubTab();

        repaint();
    }

    /** Callback when reverb config is reloaded - for triggering DSP recalculation */
    std::function<void()> onConfigReloaded;

    void cycleChannel (int delta)
    {
        int numChannels = channelSelector.getNumChannels();
        if (numChannels == 0) return;

        int newChannel = currentChannel + delta;
        if (newChannel > numChannels) newChannel = 1;
        else if (newChannel < 1) newChannel = numChannels;
        selectChannel (newChannel);
    }

    void setStatusBar (StatusBar* bar)
    {
        statusBar = bar;
    }

    std::function<void (int)> onChannelSelected;

    //==========================================================================
    // Component Overrides
    //==========================================================================

    void paint (juce::Graphics& g) override
    {
        g.fillAll (ColorScheme::get().background);

        // Header background
        g.setColour (ColorScheme::get().chromeSurface);
        g.fillRect (0, 0, getWidth(), headerHeight);

        // Footer background
        g.setColour (ColorScheme::get().chromeSurface);
        g.fillRect (0, getHeight() - footerHeight, getWidth(), footerHeight);

        // Section dividers
        g.setColour (ColorScheme::get().chromeDivider);
        g.drawLine (0.0f, static_cast<float> (headerHeight),
                    static_cast<float> (getWidth()), static_cast<float> (headerHeight), 1.0f);
        g.drawLine (0.0f, static_cast<float> (getHeight() - footerHeight),
                    static_cast<float> (getWidth()), static_cast<float> (getHeight() - footerHeight), 1.0f);
    }

    void resized() override
    {
        layoutScale = static_cast<float>(getHeight()) / 932.0f;
        headerHeight = scaled(60);
        footerHeight = scaled(50);
        auto bounds = getLocalBounds();
        const int padding = scaled(10);

        // Footer (always visible for Import functionality)
        auto footerArea = bounds.removeFromBottom (footerHeight).reduced (padding, padding);
        layoutFooter (footerArea);

        // Position the "no channels" message in the center of remaining space
        noChannelsLabel.setBounds (bounds.reduced (scaled(40)));

        // Only layout header and sub-tabs if we have channels
        int numReverbs = parameters.getNumReverbChannels();
        if (numReverbs > 0)
        {
            // Header
            auto headerArea = bounds.removeFromTop (headerHeight).reduced (padding, padding);
            layoutHeader (headerArea);

            // Sub-tabs area
            auto tabBarArea = bounds.removeFromTop (scaled(32));
            subTabBar.setBounds (tabBarArea);

            auto contentArea = bounds.reduced (padding, 0);
            subTabContentArea = contentArea.reduced (0, padding);
            layoutCurrentSubTab();
            WfsLookAndFeel::scaleTextEditorFonts(*this, layoutScale);
        }
    }

private:
    //==========================================================================
    // Setup Methods
    //==========================================================================

    void setupHeader()
    {
        addAndMakeVisible (channelSelector);
        channelSelector.onChannelChanged = [this] (int channel)
        {
            loadChannelParameters (channel);
            if (onChannelSelected)
                onChannelSelected (channel);
        };

        addAndMakeVisible (nameLabel);
        nameLabel.setText (LOC("reverbs.labels.name"), juce::dontSendNotification);

        addAndMakeVisible (nameEditor);
        nameEditor.addListener (this);

        // Map visibility toggle button
        addAndMakeVisible (mapVisibilityButton);
        updateMapVisibilityButtonState();
        mapVisibilityButton.onClick = [this]() { toggleMapVisibility(); };
    }

    void setupSubTabs()
    {
        addAndMakeVisible (subTabBar);
        subTabBar.addTab (LOC("reverbs.tabs.channelParams"), juce::Colour (0xFF2A2A2A), -1);
        subTabBar.addTab (LOC("reverbs.tabs.preProcessing"), juce::Colour (0xFF2A2A2A), -1);
        subTabBar.addTab (LOC("reverbs.tabs.algorithm"), juce::Colour (0xFF2A2A2A), -1);
        subTabBar.addTab (LOC("reverbs.tabs.postProcessing"), juce::Colour (0xFF2A2A2A), -1);
        subTabBar.addChangeListener (static_cast<juce::ChangeListener*> (this));
    }

    void setupReverbSubTab()
    {
        // Attenuation
        addAndMakeVisible (attenuationLabel);
        attenuationLabel.setText (LOC("reverbs.labels.attenuation"), juce::dontSendNotification);

        attenuationSlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFF4A90D9));
        attenuationSlider.onValueChanged = [this] (float v)
        {
            float dB = 20.0f * std::log10 (std::pow (10.0f, -92.0f / 20.0f) +
                       ((1.0f - std::pow (10.0f, -92.0f / 20.0f)) * v * v));
            attenuationValueLabel.setText (juce::String (dB, 1) + " dB", juce::dontSendNotification);
            saveReverbParam (WFSParameterIDs::reverbAttenuation, dB);
        };
        attenuationSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Reverb Attenuation");
        };
        addAndMakeVisible (attenuationSlider);

        addAndMakeVisible (attenuationValueLabel);
        attenuationValueLabel.setText ("0.0 dB", juce::dontSendNotification);
        attenuationValueLabel.setJustificationType (juce::Justification::right);
        setupEditableValueLabel (attenuationValueLabel);

        // Delay/Latency
        addAndMakeVisible (delayLatencyLabel);
        delayLatencyLabel.setText (LOC("reverbs.labels.delayLatency"), juce::dontSendNotification);

        delayLatencySlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFFD4A017));
        delayLatencySlider.onValueChanged = [this] (float v)
        {
            float ms = v * 100.0f;  // -100 to +100 ms (v is -1 to 1)
            delayLatencyValueLabel.setText (juce::String (ms, 1) + " ms", juce::dontSendNotification);
            saveReverbParam (WFSParameterIDs::reverbDelayLatency, ms);
        };
        delayLatencySlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Reverb Delay/Latency");
        };
        addAndMakeVisible (delayLatencySlider);

        addAndMakeVisible (delayLatencyValueLabel);
        delayLatencyValueLabel.setText ("0.0 ms", juce::dontSendNotification);
        delayLatencyValueLabel.setJustificationType (juce::Justification::right);
        setupEditableValueLabel (delayLatencyValueLabel);
    }

    void setupPositionSubTab()
    {
        // Coordinate Mode selector
        addAndMakeVisible(coordModeLabel);
        coordModeLabel.setText(LOC("reverbs.labels.coordinates"), juce::dontSendNotification);
        addAndMakeVisible(coordModeSelector);
        coordModeSelector.addItem(LOC("reverbs.coordModes.xyz"), 1);
        coordModeSelector.addItem(juce::String(juce::CharPointer_UTF8("r \xce\xb8 Z")), 2);    // r θ Z
        coordModeSelector.addItem(juce::String(juce::CharPointer_UTF8("r \xce\xb8 \xcf\x86")), 3);  // r θ φ
        coordModeSelector.setSelectedId(1, juce::dontSendNotification);
        coordModeSelector.onChange = [this]() {
            int mode = coordModeSelector.getSelectedId() - 1;
            saveReverbParam(WFSParameterIDs::reverbCoordinateMode, mode);
            updatePositionLabelsAndValues();
            // TTS: Announce selection change
            TTSManager::getInstance().announceValueChange("Coordinate Mode", coordModeSelector.getText());
        };

        // Position X/Y/Z
        juce::String posLabels[] = { LOC("reverbs.labels.positionX"), LOC("reverbs.labels.positionY"), LOC("reverbs.labels.positionZ") };
        juce::Label* posLabelPtrs[] = { &posXLabel, &posYLabel, &posZLabel };
        juce::TextEditor* posEditorPtrs[] = { &posXEditor, &posYEditor, &posZEditor };
        juce::Label* posUnitPtrs[] = { &posXUnitLabel, &posYUnitLabel, &posZUnitLabel };

        for (int i = 0; i < 3; ++i)
        {
            addAndMakeVisible (*posLabelPtrs[i]);
            posLabelPtrs[i]->setText (posLabels[i], juce::dontSendNotification);

            addAndMakeVisible (*posEditorPtrs[i]);
            posEditorPtrs[i]->setInputRestrictions (10, "-0123456789.");
            posEditorPtrs[i]->addListener (this);

            addAndMakeVisible (*posUnitPtrs[i]);
            posUnitPtrs[i]->setText ("m", juce::dontSendNotification);
            posUnitPtrs[i]->setColour (juce::Label::textColourId, juce::Colours::grey);
        }

        // Return Offset X/Y/Z
        juce::String offsetLabels[] = { LOC("reverbs.labels.returnOffsetX"), LOC("reverbs.labels.returnOffsetY"), LOC("reverbs.labels.returnOffsetZ") };
        juce::Label* offsetLabelPtrs[] = { &returnOffsetXLabel, &returnOffsetYLabel, &returnOffsetZLabel };
        juce::TextEditor* offsetEditorPtrs[] = { &returnOffsetXEditor, &returnOffsetYEditor, &returnOffsetZEditor };
        juce::Label* offsetUnitPtrs[] = { &returnOffsetXUnitLabel, &returnOffsetYUnitLabel, &returnOffsetZUnitLabel };

        for (int i = 0; i < 3; ++i)
        {
            addAndMakeVisible (*offsetLabelPtrs[i]);
            offsetLabelPtrs[i]->setText (offsetLabels[i], juce::dontSendNotification);

            addAndMakeVisible (*offsetEditorPtrs[i]);
            offsetEditorPtrs[i]->setInputRestrictions (10, "-0123456789.");
            offsetEditorPtrs[i]->addListener (this);

            addAndMakeVisible (*offsetUnitPtrs[i]);
            offsetUnitPtrs[i]->setText ("m", juce::dontSendNotification);
            offsetUnitPtrs[i]->setColour (juce::Label::textColourId, juce::Colours::grey);
        }
    }

    void setupReverbFeedSubTab()
    {
        // Column title labels
        addAndMakeVisible (reverbFeedTitleLabel);
        reverbFeedTitleLabel.setText (LOC("reverbs.sections.reverbFeed"), juce::dontSendNotification);
        reverbFeedTitleLabel.setFont (juce::FontOptions().withHeight (18.0f).withStyle ("Bold"));

        addAndMakeVisible (reverbReturnTitleLabel);
        reverbReturnTitleLabel.setText (LOC("reverbs.sections.reverbReturn"), juce::dontSendNotification);
        reverbReturnTitleLabel.setFont (juce::FontOptions().withHeight (18.0f).withStyle ("Bold"));

        // Directional dial (orientation + angle on + angle off)
        addAndMakeVisible (orientationLabel);
        orientationLabel.setText (LOC("reverbs.labels.orientation"), juce::dontSendNotification);

        directionalDial.onOrientationChanged = [this] (float angle)
        {
            orientationValueLabel.setText (juce::String (static_cast<int> (angle)), juce::dontSendNotification);
            saveReverbParam (WFSParameterIDs::reverbOrientation, angle);
        };
        directionalDial.onAngleOnChanged = [this] (int degrees)
        {
            angleOnSlider.setValue ((degrees - 1.0f) / 179.0f);
            angleOnValueLabel.setText (juce::String (degrees) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
            saveReverbParam (WFSParameterIDs::reverbAngleOn, degrees);

            // Enforce constraint: angleOn + angleOff <= 180
            int angleOff = directionalDial.getAngleOff();
            if (degrees + angleOff > 180)
            {
                angleOff = 180 - degrees;
                directionalDial.setAngleOff (angleOff);
                angleOffSlider.setValue (angleOff / 179.0f);
                angleOffValueLabel.setText (juce::String (angleOff) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
                saveReverbParam (WFSParameterIDs::reverbAngleOff, angleOff);
            }
        };
        directionalDial.onAngleOffChanged = [this] (int degrees)
        {
            angleOffSlider.setValue (degrees / 179.0f);
            angleOffValueLabel.setText (juce::String (degrees) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
            saveReverbParam (WFSParameterIDs::reverbAngleOff, degrees);

            // Enforce constraint: angleOn + angleOff <= 180
            int angleOn = directionalDial.getAngleOn();
            if (angleOn + degrees > 180)
            {
                angleOn = 180 - degrees;
                directionalDial.setAngleOn (angleOn);
                angleOnSlider.setValue ((angleOn - 1.0f) / 179.0f);
                angleOnValueLabel.setText (juce::String (angleOn) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
                saveReverbParam (WFSParameterIDs::reverbAngleOn, angleOn);
            }
        };
        directionalDial.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Reverb Directional");
        };
        addAndMakeVisible (directionalDial);

        addAndMakeVisible (orientationValueLabel);
        orientationValueLabel.setText ("0", juce::dontSendNotification);
        orientationValueLabel.setJustificationType (juce::Justification::right);
        setupEditableValueLabel (orientationValueLabel);
        addAndMakeVisible (orientationUnitLabel);
        orientationUnitLabel.setText (juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
        orientationUnitLabel.setJustificationType (juce::Justification::left);
        orientationUnitLabel.setMinimumHorizontalScale (1.0f);

        // Angle On slider
        addAndMakeVisible (angleOnLabel);
        angleOnLabel.setText (LOC("reverbs.labels.angleOn"), juce::dontSendNotification);

        angleOnSlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFF4CAF50));  // Green to match dial
        angleOnSlider.onValueChanged = [this] (float v)
        {
            int degrees = static_cast<int> (v * 179.0f + 1.0f);  // 1-180
            angleOnValueLabel.setText (juce::String (degrees) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
            directionalDial.setAngleOn (degrees);
            saveReverbParam (WFSParameterIDs::reverbAngleOn, degrees);

            // Enforce constraint: angleOn + angleOff <= 180
            int angleOff = directionalDial.getAngleOff();
            if (degrees + angleOff > 180)
            {
                angleOff = 180 - degrees;
                directionalDial.setAngleOff (angleOff);
                angleOffSlider.setValue (angleOff / 179.0f);
                angleOffValueLabel.setText (juce::String (angleOff) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
                saveReverbParam (WFSParameterIDs::reverbAngleOff, angleOff);
            }
        };
        angleOnSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Reverb Angle On");
        };
        addAndMakeVisible (angleOnSlider);

        addAndMakeVisible (angleOnValueLabel);
        angleOnValueLabel.setText ("86" + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
        angleOnValueLabel.setJustificationType (juce::Justification::right);
        setupEditableValueLabel (angleOnValueLabel);

        // Angle Off slider
        addAndMakeVisible (angleOffLabel);
        angleOffLabel.setText (LOC("reverbs.labels.angleOff"), juce::dontSendNotification);

        angleOffSlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFFE53935));  // Red to match dial
        angleOffSlider.onValueChanged = [this] (float v)
        {
            int degrees = static_cast<int> (v * 179.0f);  // 0-179
            angleOffValueLabel.setText (juce::String (degrees) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
            directionalDial.setAngleOff (degrees);
            saveReverbParam (WFSParameterIDs::reverbAngleOff, degrees);

            // Enforce constraint: angleOn + angleOff <= 180
            int angleOn = directionalDial.getAngleOn();
            if (angleOn + degrees > 180)
            {
                angleOn = 180 - degrees;
                directionalDial.setAngleOn (angleOn);
                angleOnSlider.setValue ((angleOn - 1.0f) / 179.0f);
                angleOnValueLabel.setText (juce::String (angleOn) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
                saveReverbParam (WFSParameterIDs::reverbAngleOn, angleOn);
            }
        };
        angleOffSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Reverb Angle Off");
        };
        addAndMakeVisible (angleOffSlider);

        addAndMakeVisible (angleOffValueLabel);
        angleOffValueLabel.setText ("90" + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
        angleOffValueLabel.setJustificationType (juce::Justification::right);
        setupEditableValueLabel (angleOffValueLabel);

        // Pitch slider
        addAndMakeVisible (pitchLabel);
        pitchLabel.setText (LOC("reverbs.labels.pitch"), juce::dontSendNotification);

        pitchSlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFF26A69A));
        pitchSlider.onValueChanged = [this] (float v)
        {
            int degrees = static_cast<int> (v * 90.0f);  // -90 to +90 (v is -1 to 1)
            pitchValueLabel.setText (juce::String (degrees) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
            saveReverbParam (WFSParameterIDs::reverbPitch, degrees);
        };
        pitchSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Reverb Pitch");
        };
        addAndMakeVisible (pitchSlider);

        addAndMakeVisible (pitchValueLabel);
        pitchValueLabel.setText ("0" + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
        pitchValueLabel.setJustificationType (juce::Justification::right);
        setupEditableValueLabel (pitchValueLabel);

        // HF Damping slider
        addAndMakeVisible (hfDampingLabel);
        hfDampingLabel.setText (LOC("reverbs.labels.hfDamping"), juce::dontSendNotification);

        hfDampingSlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFFE07878));
        hfDampingSlider.onValueChanged = [this] (float v)
        {
            float dB = v * 6.0f - 6.0f;  // -6 to 0 dB/m
            hfDampingValueLabel.setText (juce::String (dB, 1) + " dB/m", juce::dontSendNotification);
            saveReverbParam (WFSParameterIDs::reverbHFdamping, dB);
        };
        hfDampingSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Reverb HF Damping");
        };
        addAndMakeVisible (hfDampingSlider);

        addAndMakeVisible (hfDampingValueLabel);
        hfDampingValueLabel.setText ("0.0 dB/m", juce::dontSendNotification);
        hfDampingValueLabel.setJustificationType (juce::Justification::right);
        setupEditableValueLabel (hfDampingValueLabel);

        // Toggle buttons
        addAndMakeVisible (miniLatencyEnableButton);
        miniLatencyEnableButton.setButtonText (LOC("reverbs.toggles.minLatencyOff"));
        miniLatencyEnableButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF2D2D2D));
        miniLatencyEnableButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xFF2D2D2D));
        miniLatencyEnableButton.onClick = [this]
        {
            bool enabled = !miniLatencyEnableButton.getToggleState();
            miniLatencyEnableButton.setToggleState (enabled, juce::dontSendNotification);
            miniLatencyEnableButton.setButtonText (enabled ? LOC("reverbs.toggles.minLatencyOn") : LOC("reverbs.toggles.minLatencyOff"));
            juce::Colour btnColour = enabled ? juce::Colour (0xFFD4A017) : juce::Colour (0xFF2D2D2D);
            miniLatencyEnableButton.setColour (juce::TextButton::buttonColourId, btnColour);
            miniLatencyEnableButton.setColour (juce::TextButton::buttonOnColourId, btnColour);
            saveReverbParam (WFSParameterIDs::reverbMiniLatencyEnable, enabled ? 1 : 0);
        };

        addAndMakeVisible (lsEnableButton);
        lsEnableButton.setButtonText (LOC("reverbs.toggles.liveSourceOff"));
        lsEnableButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF2D2D2D));
        lsEnableButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xFF2D2D2D));
        lsEnableButton.onClick = [this]
        {
            bool enabled = !lsEnableButton.getToggleState();
            lsEnableButton.setToggleState (enabled, juce::dontSendNotification);
            lsEnableButton.setButtonText (enabled ? LOC("reverbs.toggles.liveSourceOn") : LOC("reverbs.toggles.liveSourceOff"));
            juce::Colour btnColour = enabled ? juce::Colour (0xFF4A90D9) : juce::Colour (0xFF2D2D2D);
            lsEnableButton.setColour (juce::TextButton::buttonColourId, btnColour);
            lsEnableButton.setColour (juce::TextButton::buttonOnColourId, btnColour);
            saveReverbParam (WFSParameterIDs::reverbLSenable, enabled ? 1 : 0);
        };

        // Distance Attenuation Enable slider
        addAndMakeVisible (distanceAttenEnableLabel);
        distanceAttenEnableLabel.setText (LOC("reverbs.labels.distanceAttenPercent"), juce::dontSendNotification);

        distanceAttenEnableSlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFF4A90D9));
        distanceAttenEnableSlider.onValueChanged = [this] (float v)
        {
            int percent = static_cast<int> ((v + 1.0f) * 100.0f);  // 0-200% (v is -1 to 1, center is 100%)
            distanceAttenEnableValueLabel.setText (juce::String (percent) + "%", juce::dontSendNotification);
            saveReverbParam (WFSParameterIDs::reverbDistanceAttenEnable, percent);
        };
        distanceAttenEnableSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Reverb Distance Atten Enable");
        };
        addAndMakeVisible (distanceAttenEnableSlider);

        addAndMakeVisible (distanceAttenEnableValueLabel);
        distanceAttenEnableValueLabel.setText ("100%", juce::dontSendNotification);
        distanceAttenEnableValueLabel.setJustificationType (juce::Justification::right);
        setupEditableValueLabel (distanceAttenEnableValueLabel);
    }

    void setupEQSubTab()
    {
        // EQ Enable button
        addAndMakeVisible (eqEnableButton);
        eqEnableButton.setButtonText (LOC("eq.status.on"));
        eqEnableButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF4CAF50));
        eqEnableButton.onClick = [this]
        {
            bool enabled = !eqEnableButton.getToggleState();
            eqEnableButton.setToggleState (enabled, juce::dontSendNotification);
            eqEnableButton.setButtonText (enabled ? LOC("eq.status.on") : LOC("eq.status.off"));
            eqEnableButton.setColour (juce::TextButton::buttonColourId,
                                      enabled ? juce::Colour (0xFF4CAF50) : juce::Colour (0xFF2D2D2D));
            for (int i = 0; i < numEqBands; ++i)
                updateEQBandAppearance (i);
            if (eqDisplay != nullptr)
                eqDisplay->setEQEnabled (enabled);
            saveReverbParam (WFSParameterIDs::reverbPreEQenable, enabled ? 1 : 0);
        };

        // Flatten EQ long-press button
        addAndMakeVisible (eqFlattenButton);
        eqFlattenButton.setButtonText (LOC("eq.buttons.flattenEQ"));
        eqFlattenButton.onLongPress = [this]() {
            for (int i = 0; i < numEqBands; ++i)
                resetPreEQBand (i);
        };

        // 4 EQ bands
        for (int i = 0; i < numEqBands; ++i)
        {
            // Band label - colored to match EQ display markers
            addAndMakeVisible (eqBandLabel[i]);
            eqBandLabel[i].setText (LOC("eq.labels.band") + " " + juce::String (i + 1), juce::dontSendNotification);
            eqBandLabel[i].setColour (juce::Label::textColourId, EQDisplayComponent::getBandColour(i));
            eqBandLabel[i].setJustificationType (juce::Justification::centredLeft);

            // Band on/off toggle indicator
            addAndMakeVisible (eqBandToggle[i]);
            eqBandToggle[i].setBandColour (EQDisplayComponent::getBandColour(i));
            eqBandToggle[i].setToggleState (false, juce::dontSendNotification);
            eqBandToggle[i].onClick = [this, i]() {
                bool on = eqBandToggle[i].getToggleState();
                int shape = on ? eqBandShapeSelector[i].getSelectedId() : 0;
                saveEQBandParam (i, WFSParameterIDs::reverbPreEQshape, shape);
                updateEQBandAppearance (i);
            };

            // Reset band long-press button
            addAndMakeVisible (eqBandResetButton[i]);
            eqBandResetButton[i].setButtonText (LOC("eq.buttons.resetBand"));
            eqBandResetButton[i].onLongPress = [this, i]() { resetPreEQBand (i); };

            // Shape selector (no "Off" - toggle handles on/off)
            addAndMakeVisible (eqBandShapeSelector[i]);
            eqBandShapeSelector[i].addItem (LOC("eq.filterTypes.lowCut"), 1);
            eqBandShapeSelector[i].addItem (LOC("eq.filterTypes.lowShelf"), 2);
            eqBandShapeSelector[i].addItem (LOC("eq.filterTypes.peakNotch"), 3);
            eqBandShapeSelector[i].addItem (LOC("eq.filterTypes.bandPass"), 6);
            eqBandShapeSelector[i].addItem (LOC("eq.filterTypes.highShelf"), 4);
            eqBandShapeSelector[i].addItem (LOC("eq.filterTypes.highCut"), 5);
            eqBandShapeSelector[i].setSelectedId (WFSParameterDefaults::reverbPreEQBandComboDefaults[i], juce::dontSendNotification);
            eqBandShapeSelector[i].onChange = [this, i]
            {
                if (eqBandToggle[i].getToggleState())
                {
                    int shape = eqBandShapeSelector[i].getSelectedId();
                    saveEQBandParam (i, WFSParameterIDs::reverbPreEQshape, shape);
                }
                updateEQBandAppearance (i);
                TTSManager::getInstance().announceValueChange("EQ Band " + juce::String(i + 1) + " Shape", eqBandShapeSelector[i].getText());
            };

            // Frequency slider - colored to match band
            addAndMakeVisible (eqBandFreqLabel[i]);
            eqBandFreqLabel[i].setText (LOC("eq.labels.freq"), juce::dontSendNotification);
            eqBandFreqLabel[i].setColour (juce::Label::textColourId, juce::Colours::grey);

            juce::Colour bandColour = EQDisplayComponent::getBandColour(i);
            eqBandFreqSlider[i].setTrackColours (juce::Colour (0xFF2D2D2D), bandColour);
            eqBandFreqSlider[i].onValueChanged = [this, i] (float v)
            {
                int freq = static_cast<int> (20.0f * std::pow (10.0f, 3.0f * v));
                eqBandFreqValueLabel[i].setText (formatFrequency (freq), juce::dontSendNotification);
                saveEQBandParam (i, WFSParameterIDs::reverbPreEQfreq, freq);
            };
            eqBandFreqSlider[i].onGestureStart = [this, i]() {
                parameters.getValueTreeState().beginUndoTransaction ("Pre-EQ Band " + juce::String (i + 1) + " Freq");
            };
            addAndMakeVisible (eqBandFreqSlider[i]);

            addAndMakeVisible (eqBandFreqValueLabel[i]);
            eqBandFreqValueLabel[i].setText ("1000 Hz", juce::dontSendNotification);
            setupEditableValueLabel (eqBandFreqValueLabel[i]);

            // Gain dial - colored to match band
            addAndMakeVisible (eqBandGainLabel[i]);
            eqBandGainLabel[i].setText (LOC("eq.labels.gain"), juce::dontSendNotification);
            eqBandGainLabel[i].setColour (juce::Label::textColourId, juce::Colours::grey);
            eqBandGainLabel[i].setJustificationType (juce::Justification::centred);

            eqBandGainDial[i].setTrackColours (juce::Colour (0xFF2D2D2D), bandColour);
            eqBandGainDial[i].onValueChanged = [this, i] (float v)
            {
                float gain = v * 48.0f - 24.0f;  // -24 to +24 dB
                eqBandGainValueLabel[i].setText (juce::String (gain, 1) + " dB", juce::dontSendNotification);
                saveEQBandParam (i, WFSParameterIDs::reverbPreEQgain, gain);
            };
            eqBandGainDial[i].onGestureStart = [this, i]() {
                parameters.getValueTreeState().beginUndoTransaction ("Pre-EQ Band " + juce::String (i + 1) + " Gain");
            };
            addAndMakeVisible (eqBandGainDial[i]);

            addAndMakeVisible (eqBandGainValueLabel[i]);
            eqBandGainValueLabel[i].setText ("0.0 dB", juce::dontSendNotification);
            eqBandGainValueLabel[i].setEditable (true, false);
            eqBandGainValueLabel[i].addListener (this);
            eqBandGainValueLabel[i].setJustificationType (juce::Justification::centred);

            // Q dial - colored to match band
            addAndMakeVisible (eqBandQLabel[i]);
            eqBandQLabel[i].setText (LOC("eq.labels.q"), juce::dontSendNotification);
            eqBandQLabel[i].setColour (juce::Label::textColourId, juce::Colours::grey);
            eqBandQLabel[i].setJustificationType (juce::Justification::centred);

            eqBandQDial[i].setTrackColours (juce::Colour (0xFF2D2D2D), bandColour);
            eqBandQDial[i].onValueChanged = [this, i] (float v)
            {
                float q = 0.1f + 0.21f * (std::pow (100.0f, v) - 1.0f);  // 0.1-20.0
                eqBandQValueLabel[i].setText (juce::String (q, 2), juce::dontSendNotification);
                saveEQBandParam (i, WFSParameterIDs::reverbPreEQq, q);
            };
            eqBandQDial[i].onGestureStart = [this, i]() {
                parameters.getValueTreeState().beginUndoTransaction ("Pre-EQ Band " + juce::String (i + 1) + " Q");
            };
            addAndMakeVisible (eqBandQDial[i]);

            addAndMakeVisible (eqBandQValueLabel[i]);
            eqBandQValueLabel[i].setText ("0.70", juce::dontSendNotification);
            eqBandQValueLabel[i].setEditable (true, false);
            eqBandQValueLabel[i].addListener (this);
            eqBandQValueLabel[i].setJustificationType (juce::Justification::centred);
        }
    }

    void setupPreCompressorControls()
    {
        using namespace WFSParameterIDs;
        using namespace WFSParameterDefaults;

        // Section label
        addAndMakeVisible (preCompSectionLabel);
        preCompSectionLabel.setText (LOC("reverbs.preProcessing.compressor"), juce::dontSendNotification);
        preCompSectionLabel.setFont (juce::FontOptions().withHeight (16.0f).withStyle ("Bold"));
        preCompSectionLabel.setColour (juce::Label::textColourId, juce::Colours::white);

        // Bypass button
        addAndMakeVisible (preCompBypassButton);
        preCompBypassButton.setButtonText (LOC("reverbs.preProcessing.compressorOff"));
        preCompBypassButton.setClickingTogglesState (true);
        preCompBypassButton.setToggleState (true, juce::dontSendNotification);  // Default: bypassed
        preCompBypassButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF2D2D2D));
        preCompBypassButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xFF2D2D2D));
        preCompBypassButton.onClick = [this]
        {
            bool bypassed = preCompBypassButton.getToggleState();
            preCompBypassButton.setButtonText (bypassed ? LOC("reverbs.preProcessing.compressorOff") : LOC("reverbs.preProcessing.compressorOn"));
            preCompBypassButton.setColour (juce::TextButton::buttonColourId,
                bypassed ? juce::Colour (0xFF2D2D2D) : juce::Colour (0xFF4CAF50));
            preCompBypassButton.setColour (juce::TextButton::buttonOnColourId,
                bypassed ? juce::Colour (0xFF2D2D2D) : juce::Colour (0xFF4CAF50));
            updatePreCompAppearance();
            savePreCompParam (reverbPreCompBypass, bypassed ? 1 : 0);
        };

        // Threshold dial
        addAndMakeVisible (preCompThresholdLabel);
        preCompThresholdLabel.setText (LOC("reverbs.preProcessing.threshold"), juce::dontSendNotification);
        preCompThresholdLabel.setJustificationType (juce::Justification::centred);
        preCompThresholdDial.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFF4CAF50));
        preCompThresholdDial.onValueChanged = [this] (float v)
        {
            float threshold = reverbPreCompThresholdMin + (reverbPreCompThresholdMax - reverbPreCompThresholdMin) * v;
            preCompThresholdValueLabel.setText (juce::String (threshold, 1) + " dB", juce::dontSendNotification);
            savePreCompParam (reverbPreCompThreshold, threshold);
        };
        preCompThresholdDial.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Pre-Comp Threshold");
        };
        addAndMakeVisible (preCompThresholdDial);
        addAndMakeVisible (preCompThresholdValueLabel);
        preCompThresholdValueLabel.setEditable (true, false);
        preCompThresholdValueLabel.addListener (this);
        preCompThresholdValueLabel.setJustificationType (juce::Justification::centred);
        preCompThresholdValueLabel.setText ("-12.0 dB", juce::dontSendNotification);

        // Ratio dial
        addAndMakeVisible (preCompRatioLabel);
        preCompRatioLabel.setText (LOC("reverbs.preProcessing.ratio"), juce::dontSendNotification);
        preCompRatioLabel.setJustificationType (juce::Justification::centred);
        preCompRatioDial.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFF4CAF50));
        preCompRatioDial.onValueChanged = [this] (float v)
        {
            float ratio = reverbPreCompRatioMin + (reverbPreCompRatioMax - reverbPreCompRatioMin) * v;
            preCompRatioValueLabel.setText (juce::String (ratio, 1) + ":1", juce::dontSendNotification);
            savePreCompParam (reverbPreCompRatio, ratio);
        };
        preCompRatioDial.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Pre-Comp Ratio");
        };
        addAndMakeVisible (preCompRatioDial);
        addAndMakeVisible (preCompRatioValueLabel);
        preCompRatioValueLabel.setEditable (true, false);
        preCompRatioValueLabel.addListener (this);
        preCompRatioValueLabel.setJustificationType (juce::Justification::centred);
        preCompRatioValueLabel.setText ("2.0:1", juce::dontSendNotification);

        // Attack dial (logarithmic)
        addAndMakeVisible (preCompAttackLabel);
        preCompAttackLabel.setText (LOC("reverbs.preProcessing.attack"), juce::dontSendNotification);
        preCompAttackLabel.setJustificationType (juce::Justification::centred);
        preCompAttackDial.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFF4CAF50));
        preCompAttackDial.onValueChanged = [this] (float v)
        {
            float attack = reverbPreCompAttackMin * std::pow (reverbPreCompAttackMax / reverbPreCompAttackMin, v);
            preCompAttackValueLabel.setText (juce::String (attack, 1) + " ms", juce::dontSendNotification);
            savePreCompParam (reverbPreCompAttack, attack);
        };
        preCompAttackDial.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Pre-Comp Attack");
        };
        addAndMakeVisible (preCompAttackDial);
        addAndMakeVisible (preCompAttackValueLabel);
        preCompAttackValueLabel.setEditable (true, false);
        preCompAttackValueLabel.addListener (this);
        preCompAttackValueLabel.setJustificationType (juce::Justification::centred);
        preCompAttackValueLabel.setText ("10.0 ms", juce::dontSendNotification);

        // Release dial (logarithmic)
        addAndMakeVisible (preCompReleaseLabel);
        preCompReleaseLabel.setText (LOC("reverbs.preProcessing.release"), juce::dontSendNotification);
        preCompReleaseLabel.setJustificationType (juce::Justification::centred);
        preCompReleaseDial.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFF4CAF50));
        preCompReleaseDial.onValueChanged = [this] (float v)
        {
            float release = reverbPreCompReleaseMin * std::pow (reverbPreCompReleaseMax / reverbPreCompReleaseMin, v);
            preCompReleaseValueLabel.setText (juce::String (release, 0) + " ms", juce::dontSendNotification);
            savePreCompParam (reverbPreCompRelease, release);
        };
        preCompReleaseDial.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Pre-Comp Release");
        };
        addAndMakeVisible (preCompReleaseDial);
        addAndMakeVisible (preCompReleaseValueLabel);
        preCompReleaseValueLabel.setEditable (true, false);
        preCompReleaseValueLabel.addListener (this);
        preCompReleaseValueLabel.setJustificationType (juce::Justification::centred);
        preCompReleaseValueLabel.setText ("100 ms", juce::dontSendNotification);
    }

    void setupPostExpanderControls()
    {
        using namespace WFSParameterIDs;
        using namespace WFSParameterDefaults;

        // Section label
        addAndMakeVisible (postExpSectionLabel);
        postExpSectionLabel.setText (LOC("reverbs.postProcessing.expander"), juce::dontSendNotification);
        postExpSectionLabel.setFont (juce::FontOptions().withHeight (16.0f).withStyle ("Bold"));
        postExpSectionLabel.setColour (juce::Label::textColourId, juce::Colours::white);

        // Bypass button
        addAndMakeVisible (postExpBypassButton);
        postExpBypassButton.setButtonText (LOC("reverbs.postProcessing.expanderOff"));
        postExpBypassButton.setClickingTogglesState (true);
        postExpBypassButton.setToggleState (true, juce::dontSendNotification);  // Default: bypassed
        postExpBypassButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF2D2D2D));
        postExpBypassButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xFF2D2D2D));
        postExpBypassButton.onClick = [this]
        {
            bool bypassed = postExpBypassButton.getToggleState();
            postExpBypassButton.setButtonText (bypassed ? LOC("reverbs.postProcessing.expanderOff") : LOC("reverbs.postProcessing.expanderOn"));
            postExpBypassButton.setColour (juce::TextButton::buttonColourId,
                bypassed ? juce::Colour (0xFF2D2D2D) : juce::Colour (0xFF4CAF50));
            postExpBypassButton.setColour (juce::TextButton::buttonOnColourId,
                bypassed ? juce::Colour (0xFF2D2D2D) : juce::Colour (0xFF4CAF50));
            updatePostExpAppearance();
            savePostExpParam (reverbPostExpBypass, bypassed ? 1 : 0);
        };

        // Threshold dial
        addAndMakeVisible (postExpThresholdLabel);
        postExpThresholdLabel.setText (LOC("reverbs.postProcessing.threshold"), juce::dontSendNotification);
        postExpThresholdLabel.setJustificationType (juce::Justification::centred);
        postExpThresholdDial.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFFFF9800));
        postExpThresholdDial.onValueChanged = [this] (float v)
        {
            float threshold = reverbPostExpThresholdMin + (reverbPostExpThresholdMax - reverbPostExpThresholdMin) * v;
            postExpThresholdValueLabel.setText (juce::String (threshold, 1) + " dB", juce::dontSendNotification);
            savePostExpParam (reverbPostExpThreshold, threshold);
        };
        postExpThresholdDial.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Post-Exp Threshold");
        };
        addAndMakeVisible (postExpThresholdDial);
        addAndMakeVisible (postExpThresholdValueLabel);
        postExpThresholdValueLabel.setEditable (true, false);
        postExpThresholdValueLabel.addListener (this);
        postExpThresholdValueLabel.setJustificationType (juce::Justification::centred);
        postExpThresholdValueLabel.setText ("-40.0 dB", juce::dontSendNotification);

        // Ratio dial
        addAndMakeVisible (postExpRatioLabel);
        postExpRatioLabel.setText (LOC("reverbs.postProcessing.ratio"), juce::dontSendNotification);
        postExpRatioLabel.setJustificationType (juce::Justification::centred);
        postExpRatioDial.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFFFF9800));
        postExpRatioDial.onValueChanged = [this] (float v)
        {
            float ratio = reverbPostExpRatioMin + (reverbPostExpRatioMax - reverbPostExpRatioMin) * v;
            postExpRatioValueLabel.setText ("1:" + juce::String (ratio, 1), juce::dontSendNotification);
            savePostExpParam (reverbPostExpRatio, ratio);
        };
        postExpRatioDial.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Post-Exp Ratio");
        };
        addAndMakeVisible (postExpRatioDial);
        addAndMakeVisible (postExpRatioValueLabel);
        postExpRatioValueLabel.setEditable (true, false);
        postExpRatioValueLabel.addListener (this);
        postExpRatioValueLabel.setJustificationType (juce::Justification::centred);
        postExpRatioValueLabel.setText ("1:2.0", juce::dontSendNotification);

        // Attack dial (logarithmic)
        addAndMakeVisible (postExpAttackLabel);
        postExpAttackLabel.setText (LOC("reverbs.postProcessing.attack"), juce::dontSendNotification);
        postExpAttackLabel.setJustificationType (juce::Justification::centred);
        postExpAttackDial.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFFFF9800));
        postExpAttackDial.onValueChanged = [this] (float v)
        {
            float attack = reverbPostExpAttackMin * std::pow (reverbPostExpAttackMax / reverbPostExpAttackMin, v);
            postExpAttackValueLabel.setText (juce::String (attack, 1) + " ms", juce::dontSendNotification);
            savePostExpParam (reverbPostExpAttack, attack);
        };
        postExpAttackDial.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Post-Exp Attack");
        };
        addAndMakeVisible (postExpAttackDial);
        addAndMakeVisible (postExpAttackValueLabel);
        postExpAttackValueLabel.setEditable (true, false);
        postExpAttackValueLabel.addListener (this);
        postExpAttackValueLabel.setJustificationType (juce::Justification::centred);
        postExpAttackValueLabel.setText ("1.0 ms", juce::dontSendNotification);

        // Release dial (logarithmic)
        addAndMakeVisible (postExpReleaseLabel);
        postExpReleaseLabel.setText (LOC("reverbs.postProcessing.release"), juce::dontSendNotification);
        postExpReleaseLabel.setJustificationType (juce::Justification::centred);
        postExpReleaseDial.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFFFF9800));
        postExpReleaseDial.onValueChanged = [this] (float v)
        {
            float release = reverbPostExpReleaseMin * std::pow (reverbPostExpReleaseMax / reverbPostExpReleaseMin, v);
            postExpReleaseValueLabel.setText (juce::String (release, 0) + " ms", juce::dontSendNotification);
            savePostExpParam (reverbPostExpRelease, release);
        };
        postExpReleaseDial.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Post-Exp Release");
        };
        addAndMakeVisible (postExpReleaseDial);
        addAndMakeVisible (postExpReleaseValueLabel);
        postExpReleaseValueLabel.setEditable (true, false);
        postExpReleaseValueLabel.addListener (this);
        postExpReleaseValueLabel.setJustificationType (juce::Justification::centred);
        postExpReleaseValueLabel.setText ("200 ms", juce::dontSendNotification);
    }

    void setupAlgorithmSubTab()
    {
        // Algorithm type selector buttons (mutually exclusive)
        addAndMakeVisible (algoSDNButton);
        algoSDNButton.setButtonText (LOC("reverbs.algorithm.sdn"));
        algoSDNButton.setClickingTogglesState (true);
        algoSDNButton.setToggleState (true, juce::dontSendNotification);
        algoSDNButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF4CAF50));
        algoSDNButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xFF4CAF50));
        algoSDNButton.onClick = [this] { selectAlgorithm (0); };

        addAndMakeVisible (algoFDNButton);
        algoFDNButton.setButtonText (LOC("reverbs.algorithm.fdn"));
        algoFDNButton.setClickingTogglesState (true);
        algoFDNButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF2D2D2D));
        algoFDNButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xFF2D2D2D));
        algoFDNButton.onClick = [this] { selectAlgorithm (1); };

        addAndMakeVisible (algoIRButton);
        algoIRButton.setButtonText (LOC("reverbs.algorithm.ir"));
        algoIRButton.setClickingTogglesState (true);
        algoIRButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF2D2D2D));
        algoIRButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xFF2D2D2D));
        algoIRButton.onClick = [this] { selectAlgorithm (2); };

        // Decay section label
        addAndMakeVisible (algoDecaySectionLabel);
        algoDecaySectionLabel.setText (LOC("reverbs.algorithm.decaySection"), juce::dontSendNotification);
        algoDecaySectionLabel.setFont (juce::FontOptions().withHeight (16.0f).withStyle ("Bold"));

        // RT60
        addAndMakeVisible (algoRT60Label);
        algoRT60Label.setText (LOC("reverbs.algorithm.rt60"), juce::dontSendNotification);

        algoRT60Slider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFF4CAF50));
        algoRT60Slider.onValueChanged = [this] (float v)
        {
            float rt60 = WFSParameterDefaults::reverbRT60Min
                * std::pow (WFSParameterDefaults::reverbRT60Max / WFSParameterDefaults::reverbRT60Min, v);
            algoRT60ValueLabel.setText (juce::String (rt60, 2) + " s", juce::dontSendNotification);
            saveAlgorithmParam (WFSParameterIDs::reverbRT60, rt60);
        };
        algoRT60Slider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Reverb RT60");
        };
        addAndMakeVisible (algoRT60Slider);

        addAndMakeVisible (algoRT60ValueLabel);
        algoRT60ValueLabel.setText (juce::String (WFSParameterDefaults::reverbRT60Default, 2) + " s", juce::dontSendNotification);
        setupEditableValueLabel (algoRT60ValueLabel);

        // RT60 Low Mult
        addAndMakeVisible (algoRT60LowMultLabel);
        algoRT60LowMultLabel.setText (LOC("reverbs.algorithm.rt60LowMult"), juce::dontSendNotification);

        algoRT60LowMultSlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFF2196F3));
        algoRT60LowMultSlider.onValueChanged = [this] (float v)
        {
            float mult = WFSParameterDefaults::reverbRT60LowMultMin
                * std::pow (WFSParameterDefaults::reverbRT60LowMultMax / WFSParameterDefaults::reverbRT60LowMultMin, v);
            algoRT60LowMultValueLabel.setText (juce::String (mult, 2) + "x", juce::dontSendNotification);
            saveAlgorithmParam (WFSParameterIDs::reverbRT60LowMult, mult);
        };
        algoRT60LowMultSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Reverb RT60 Low Mult");
        };
        addAndMakeVisible (algoRT60LowMultSlider);

        addAndMakeVisible (algoRT60LowMultValueLabel);
        algoRT60LowMultValueLabel.setText (juce::String (WFSParameterDefaults::reverbRT60LowMultDefault, 2) + "x", juce::dontSendNotification);
        setupEditableValueLabel (algoRT60LowMultValueLabel);

        // RT60 High Mult
        addAndMakeVisible (algoRT60HighMultLabel);
        algoRT60HighMultLabel.setText (LOC("reverbs.algorithm.rt60HighMult"), juce::dontSendNotification);

        algoRT60HighMultSlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFFFF5722));
        algoRT60HighMultSlider.onValueChanged = [this] (float v)
        {
            float mult = WFSParameterDefaults::reverbRT60HighMultMin
                * std::pow (WFSParameterDefaults::reverbRT60HighMultMax / WFSParameterDefaults::reverbRT60HighMultMin, v);
            algoRT60HighMultValueLabel.setText (juce::String (mult, 2) + "x", juce::dontSendNotification);
            saveAlgorithmParam (WFSParameterIDs::reverbRT60HighMult, mult);
        };
        algoRT60HighMultSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Reverb RT60 High Mult");
        };
        addAndMakeVisible (algoRT60HighMultSlider);

        addAndMakeVisible (algoRT60HighMultValueLabel);
        algoRT60HighMultValueLabel.setText (juce::String (WFSParameterDefaults::reverbRT60HighMultDefault, 2) + "x", juce::dontSendNotification);
        setupEditableValueLabel (algoRT60HighMultValueLabel);

        // Crossover Low
        addAndMakeVisible (algoCrossoverLowLabel);
        algoCrossoverLowLabel.setText (LOC("reverbs.algorithm.crossoverLow"), juce::dontSendNotification);

        algoCrossoverLowSlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFF9C27B0));
        algoCrossoverLowSlider.onValueChanged = [this] (float v)
        {
            float freq = WFSParameterDefaults::reverbCrossoverLowMin
                * std::pow (WFSParameterDefaults::reverbCrossoverLowMax / WFSParameterDefaults::reverbCrossoverLowMin, v);
            algoCrossoverLowValueLabel.setText (formatFrequency (static_cast<int> (freq)), juce::dontSendNotification);
            saveAlgorithmParam (WFSParameterIDs::reverbCrossoverLow, freq);
        };
        algoCrossoverLowSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Reverb Crossover Low");
        };
        addAndMakeVisible (algoCrossoverLowSlider);

        addAndMakeVisible (algoCrossoverLowValueLabel);
        algoCrossoverLowValueLabel.setText (formatFrequency (static_cast<int> (WFSParameterDefaults::reverbCrossoverLowDefault)), juce::dontSendNotification);
        setupEditableValueLabel (algoCrossoverLowValueLabel);

        // Crossover High
        addAndMakeVisible (algoCrossoverHighLabel);
        algoCrossoverHighLabel.setText (LOC("reverbs.algorithm.crossoverHigh"), juce::dontSendNotification);

        algoCrossoverHighSlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFF9C27B0));
        algoCrossoverHighSlider.onValueChanged = [this] (float v)
        {
            float freq = WFSParameterDefaults::reverbCrossoverHighMin
                * std::pow (WFSParameterDefaults::reverbCrossoverHighMax / WFSParameterDefaults::reverbCrossoverHighMin, v);
            algoCrossoverHighValueLabel.setText (formatFrequency (static_cast<int> (freq)), juce::dontSendNotification);
            saveAlgorithmParam (WFSParameterIDs::reverbCrossoverHigh, freq);
        };
        algoCrossoverHighSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Reverb Crossover High");
        };
        addAndMakeVisible (algoCrossoverHighSlider);

        addAndMakeVisible (algoCrossoverHighValueLabel);
        algoCrossoverHighValueLabel.setText (formatFrequency (static_cast<int> (WFSParameterDefaults::reverbCrossoverHighDefault)), juce::dontSendNotification);
        setupEditableValueLabel (algoCrossoverHighValueLabel);

        // Diffusion
        addAndMakeVisible (algoDiffusionLabel);
        algoDiffusionLabel.setText (LOC("reverbs.algorithm.diffusion"), juce::dontSendNotification);

        algoDiffusionSlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFF00BCD4));
        algoDiffusionSlider.onValueChanged = [this] (float v)
        {
            algoDiffusionValueLabel.setText (juce::String (static_cast<int> (v * 100)) + "%", juce::dontSendNotification);
            saveAlgorithmParam (WFSParameterIDs::reverbDiffusion, v);
        };
        algoDiffusionSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Reverb Diffusion");
        };
        addAndMakeVisible (algoDiffusionSlider);

        addAndMakeVisible (algoDiffusionValueLabel);
        algoDiffusionValueLabel.setText (juce::String (static_cast<int> (WFSParameterDefaults::reverbDiffusionDefault * 100)) + "%", juce::dontSendNotification);
        setupEditableValueLabel (algoDiffusionValueLabel);

        // SDN section
        addAndMakeVisible (algoSDNSectionLabel);
        algoSDNSectionLabel.setText (LOC("reverbs.algorithm.sdnSection"), juce::dontSendNotification);
        algoSDNSectionLabel.setFont (juce::FontOptions().withHeight (16.0f).withStyle ("Bold"));

        addAndMakeVisible (algoSDNScaleLabel);
        algoSDNScaleLabel.setText (LOC("reverbs.algorithm.scale"), juce::dontSendNotification);

        algoSDNScaleSlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFFFF9800));
        algoSDNScaleSlider.onValueChanged = [this] (float v)
        {
            float scale = WFSParameterDefaults::reverbSDNscaleMin
                + v * (WFSParameterDefaults::reverbSDNscaleMax - WFSParameterDefaults::reverbSDNscaleMin);
            algoSDNScaleValueLabel.setText (juce::String (scale, 2) + "x", juce::dontSendNotification);
            saveAlgorithmParam (WFSParameterIDs::reverbSDNscale, scale);
        };
        algoSDNScaleSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Reverb SDN Scale");
        };
        addAndMakeVisible (algoSDNScaleSlider);

        addAndMakeVisible (algoSDNScaleValueLabel);
        algoSDNScaleValueLabel.setText (juce::String (WFSParameterDefaults::reverbSDNscaleDefault, 2) + "x", juce::dontSendNotification);
        setupEditableValueLabel (algoSDNScaleValueLabel);

        // FDN section
        addAndMakeVisible (algoFDNSectionLabel);
        algoFDNSectionLabel.setText (LOC("reverbs.algorithm.fdnSection"), juce::dontSendNotification);
        algoFDNSectionLabel.setFont (juce::FontOptions().withHeight (16.0f).withStyle ("Bold"));

        addAndMakeVisible (algoFDNSizeLabel);
        algoFDNSizeLabel.setText (LOC("reverbs.algorithm.size"), juce::dontSendNotification);

        algoFDNSizeSlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFFFF9800));
        algoFDNSizeSlider.onValueChanged = [this] (float v)
        {
            float size = WFSParameterDefaults::reverbFDNsizeMin
                + v * (WFSParameterDefaults::reverbFDNsizeMax - WFSParameterDefaults::reverbFDNsizeMin);
            algoFDNSizeValueLabel.setText (juce::String (size, 2) + "x", juce::dontSendNotification);
            saveAlgorithmParam (WFSParameterIDs::reverbFDNsize, size);
        };
        algoFDNSizeSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Reverb FDN Size");
        };
        addAndMakeVisible (algoFDNSizeSlider);

        addAndMakeVisible (algoFDNSizeValueLabel);
        algoFDNSizeValueLabel.setText (juce::String (WFSParameterDefaults::reverbFDNsizeDefault, 2) + "x", juce::dontSendNotification);
        setupEditableValueLabel (algoFDNSizeValueLabel);

        // IR section
        addAndMakeVisible (algoIRSectionLabel);
        algoIRSectionLabel.setText (LOC("reverbs.algorithm.irSection"), juce::dontSendNotification);
        algoIRSectionLabel.setFont (juce::FontOptions().withHeight (16.0f).withStyle ("Bold"));

        addAndMakeVisible (algoIRFileLabel);
        algoIRFileLabel.setText (LOC("reverbs.algorithm.irFile"), juce::dontSendNotification);

        addAndMakeVisible (algoIRFileSelector);
        algoIRFileSelector.onChange = [this] { handleIRFileSelection(); };

        addAndMakeVisible (algoIRTrimLabel);
        algoIRTrimLabel.setText (LOC("reverbs.algorithm.irTrim"), juce::dontSendNotification);

        algoIRTrimSlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFFFF9800));
        algoIRTrimSlider.onValueChanged = [this] (float v)
        {
            float trim = v * WFSParameterDefaults::reverbIRtrimMax;
            algoIRTrimValueLabel.setText (juce::String (trim, 1) + " ms", juce::dontSendNotification);
            saveAlgorithmParam (WFSParameterIDs::reverbIRtrim, trim);
        };
        algoIRTrimSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Reverb IR Trim");
        };
        addAndMakeVisible (algoIRTrimSlider);

        addAndMakeVisible (algoIRTrimValueLabel);
        algoIRTrimValueLabel.setText (juce::String (WFSParameterDefaults::reverbIRtrimDefault, 1) + " ms", juce::dontSendNotification);
        setupEditableValueLabel (algoIRTrimValueLabel);

        addAndMakeVisible (algoIRLengthLabel);
        algoIRLengthLabel.setText (LOC("reverbs.algorithm.irLength"), juce::dontSendNotification);

        algoIRLengthSlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFF4CAF50));
        algoIRLengthSlider.onValueChanged = [this] (float v)
        {
            float length = WFSParameterDefaults::reverbIRlengthMin
                + v * (WFSParameterDefaults::reverbIRlengthMax - WFSParameterDefaults::reverbIRlengthMin);
            algoIRLengthValueLabel.setText (juce::String (length, 1) + " s", juce::dontSendNotification);
            saveAlgorithmParam (WFSParameterIDs::reverbIRlength, length);
        };
        algoIRLengthSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Reverb IR Length");
        };
        addAndMakeVisible (algoIRLengthSlider);

        addAndMakeVisible (algoIRLengthValueLabel);
        algoIRLengthValueLabel.setText (juce::String (WFSParameterDefaults::reverbIRlengthDefault, 1) + " s", juce::dontSendNotification);
        setupEditableValueLabel (algoIRLengthValueLabel);

        addAndMakeVisible (algoPerNodeButton);
        algoPerNodeButton.setButtonText (LOC("reverbs.algorithm.perNodeOff"));
        algoPerNodeButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF2D2D2D));
        algoPerNodeButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xFF2D2D2D));
        algoPerNodeButton.onClick = [this]
        {
            bool enabled = !algoPerNodeButton.getToggleState();
            algoPerNodeButton.setToggleState (enabled, juce::dontSendNotification);
            algoPerNodeButton.setButtonText (enabled ? LOC("reverbs.algorithm.perNodeOn") : LOC("reverbs.algorithm.perNodeOff"));
            juce::Colour btnColour = enabled ? juce::Colour (0xFF4CAF50) : juce::Colour (0xFF2D2D2D);
            algoPerNodeButton.setColour (juce::TextButton::buttonColourId, btnColour);
            algoPerNodeButton.setColour (juce::TextButton::buttonOnColourId, btnColour);
            saveAlgorithmParam (WFSParameterIDs::reverbPerNodeIR, enabled ? 1 : 0);
        };

        // Wet Level (always visible)
        addAndMakeVisible (algoWetLevelLabel);
        algoWetLevelLabel.setText (LOC("reverbs.algorithm.wetLevel"), juce::dontSendNotification);

        algoWetLevelSlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFF4CAF50));
        algoWetLevelSlider.onValueChanged = [this] (float v)
        {
            // v in [0, 1] maps to [-60, +12] dB
            float dB = -60.0f + v * 72.0f;
            algoWetLevelValueLabel.setText (juce::String (dB, 1) + " dB", juce::dontSendNotification);
            saveAlgorithmParam (WFSParameterIDs::reverbWetLevel, dB);
        };
        algoWetLevelSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Reverb Wet Level");
        };
        addAndMakeVisible (algoWetLevelSlider);

        addAndMakeVisible (algoWetLevelValueLabel);
        algoWetLevelValueLabel.setText (juce::String (WFSParameterDefaults::reverbWetLevelDefault, 1) + " dB", juce::dontSendNotification);
        setupEditableValueLabel (algoWetLevelValueLabel);
    }

    void setupPostProcessingSubTab()
    {
        // Post-Processing EQ Enable button
        addAndMakeVisible (postEqEnableButton);
        postEqEnableButton.setButtonText (LOC("eq.status.on"));
        postEqEnableButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF4CAF50));
        postEqEnableButton.onClick = [this]
        {
            bool enabled = !postEqEnableButton.getToggleState();
            postEqEnableButton.setToggleState (enabled, juce::dontSendNotification);
            postEqEnableButton.setButtonText (enabled ? LOC("eq.status.on") : LOC("eq.status.off"));
            postEqEnableButton.setColour (juce::TextButton::buttonColourId,
                                          enabled ? juce::Colour (0xFF4CAF50) : juce::Colour (0xFF2D2D2D));
            for (int i = 0; i < numPostEqBands; ++i)
                updatePostEQBandAppearance (i);
            if (postEqDisplay != nullptr)
                postEqDisplay->setEQEnabled (enabled);
            savePostEQParam (WFSParameterIDs::reverbPostEQenable, enabled ? 1 : 0);
        };

        // Flatten Post-EQ long-press button
        addAndMakeVisible (postEqFlattenButton);
        postEqFlattenButton.setButtonText (LOC("eq.buttons.flattenEQ"));
        postEqFlattenButton.onLongPress = [this]() {
            for (int i = 0; i < numPostEqBands; ++i)
                resetPostEQBand (i);
        };

        // 4 Post-EQ bands
        for (int i = 0; i < numPostEqBands; ++i)
        {
            // Band label
            addAndMakeVisible (postEqBandLabel[i]);
            postEqBandLabel[i].setText (LOC("eq.labels.band") + " " + juce::String (i + 1), juce::dontSendNotification);
            postEqBandLabel[i].setColour (juce::Label::textColourId, EQDisplayComponent::getBandColour(i));
            postEqBandLabel[i].setJustificationType (juce::Justification::centredLeft);

            // Band on/off toggle indicator
            addAndMakeVisible (postEqBandToggle[i]);
            postEqBandToggle[i].setBandColour (EQDisplayComponent::getBandColour(i));
            postEqBandToggle[i].setToggleState (false, juce::dontSendNotification);
            postEqBandToggle[i].onClick = [this, i]() {
                bool on = postEqBandToggle[i].getToggleState();
                int shape = on ? postEqBandShapeSelector[i].getSelectedId() : 0;
                savePostEQBandParam (i, WFSParameterIDs::reverbPostEQshape, shape);
                updatePostEQBandAppearance (i);
            };

            // Reset band long-press button
            addAndMakeVisible (postEqBandResetButton[i]);
            postEqBandResetButton[i].setButtonText (LOC("eq.buttons.resetBand"));
            postEqBandResetButton[i].onLongPress = [this, i]() { resetPostEQBand (i); };

            // Shape selector (no "Off" - toggle handles on/off)
            addAndMakeVisible (postEqBandShapeSelector[i]);
            postEqBandShapeSelector[i].addItem (LOC("eq.filterTypes.lowCut"), 1);
            postEqBandShapeSelector[i].addItem (LOC("eq.filterTypes.lowShelf"), 2);
            postEqBandShapeSelector[i].addItem (LOC("eq.filterTypes.peakNotch"), 3);
            postEqBandShapeSelector[i].addItem (LOC("eq.filterTypes.bandPass"), 6);
            postEqBandShapeSelector[i].addItem (LOC("eq.filterTypes.highShelf"), 4);
            postEqBandShapeSelector[i].addItem (LOC("eq.filterTypes.highCut"), 5);
            postEqBandShapeSelector[i].setSelectedId (WFSParameterDefaults::reverbPostEQBandComboDefaults[i], juce::dontSendNotification);
            postEqBandShapeSelector[i].onChange = [this, i]
            {
                if (postEqBandToggle[i].getToggleState())
                {
                    int shape = postEqBandShapeSelector[i].getSelectedId();
                    savePostEQBandParam (i, WFSParameterIDs::reverbPostEQshape, shape);
                }
                updatePostEQBandAppearance (i);
                TTSManager::getInstance().announceValueChange("Post-EQ Band " + juce::String(i + 1) + " Shape", postEqBandShapeSelector[i].getText());
            };

            // Frequency slider
            addAndMakeVisible (postEqBandFreqLabel[i]);
            postEqBandFreqLabel[i].setText (LOC("eq.labels.freq"), juce::dontSendNotification);
            postEqBandFreqLabel[i].setColour (juce::Label::textColourId, juce::Colours::grey);

            juce::Colour bandColour = EQDisplayComponent::getBandColour(i);
            postEqBandFreqSlider[i].setTrackColours (juce::Colour (0xFF2D2D2D), bandColour);
            postEqBandFreqSlider[i].onValueChanged = [this, i] (float v)
            {
                int freq = static_cast<int> (20.0f * std::pow (10.0f, 3.0f * v));
                postEqBandFreqValueLabel[i].setText (formatFrequency (freq), juce::dontSendNotification);
                savePostEQBandParam (i, WFSParameterIDs::reverbPostEQfreq, freq);
            };
            postEqBandFreqSlider[i].onGestureStart = [this, i]() {
                parameters.getValueTreeState().beginUndoTransaction ("Post-EQ Band " + juce::String (i + 1) + " Freq");
            };
            addAndMakeVisible (postEqBandFreqSlider[i]);

            addAndMakeVisible (postEqBandFreqValueLabel[i]);
            postEqBandFreqValueLabel[i].setText ("1000 Hz", juce::dontSendNotification);
            setupEditableValueLabel (postEqBandFreqValueLabel[i]);

            // Gain dial
            addAndMakeVisible (postEqBandGainLabel[i]);
            postEqBandGainLabel[i].setText (LOC("eq.labels.gain"), juce::dontSendNotification);
            postEqBandGainLabel[i].setColour (juce::Label::textColourId, juce::Colours::grey);
            postEqBandGainLabel[i].setJustificationType (juce::Justification::centred);

            postEqBandGainDial[i].setTrackColours (juce::Colour (0xFF2D2D2D), bandColour);
            postEqBandGainDial[i].onValueChanged = [this, i] (float v)
            {
                float gain = v * 48.0f - 24.0f;
                postEqBandGainValueLabel[i].setText (juce::String (gain, 1) + " dB", juce::dontSendNotification);
                savePostEQBandParam (i, WFSParameterIDs::reverbPostEQgain, gain);
            };
            postEqBandGainDial[i].onGestureStart = [this, i]() {
                parameters.getValueTreeState().beginUndoTransaction ("Post-EQ Band " + juce::String (i + 1) + " Gain");
            };
            addAndMakeVisible (postEqBandGainDial[i]);

            addAndMakeVisible (postEqBandGainValueLabel[i]);
            postEqBandGainValueLabel[i].setText ("0.0 dB", juce::dontSendNotification);
            postEqBandGainValueLabel[i].setEditable (true, false);
            postEqBandGainValueLabel[i].addListener (this);
            postEqBandGainValueLabel[i].setJustificationType (juce::Justification::centred);

            // Q dial
            addAndMakeVisible (postEqBandQLabel[i]);
            postEqBandQLabel[i].setText (LOC("eq.labels.q"), juce::dontSendNotification);
            postEqBandQLabel[i].setColour (juce::Label::textColourId, juce::Colours::grey);
            postEqBandQLabel[i].setJustificationType (juce::Justification::centred);

            postEqBandQDial[i].setTrackColours (juce::Colour (0xFF2D2D2D), bandColour);
            postEqBandQDial[i].onValueChanged = [this, i] (float v)
            {
                float q = 0.1f + 0.21f * (std::pow (100.0f, v) - 1.0f);
                postEqBandQValueLabel[i].setText (juce::String (q, 2), juce::dontSendNotification);
                savePostEQBandParam (i, WFSParameterIDs::reverbPostEQq, q);
            };
            postEqBandQDial[i].onGestureStart = [this, i]() {
                parameters.getValueTreeState().beginUndoTransaction ("Post-EQ Band " + juce::String (i + 1) + " Q");
            };
            addAndMakeVisible (postEqBandQDial[i]);

            addAndMakeVisible (postEqBandQValueLabel[i]);
            postEqBandQValueLabel[i].setText ("0.70", juce::dontSendNotification);
            postEqBandQValueLabel[i].setEditable (true, false);
            postEqBandQValueLabel[i].addListener (this);
            postEqBandQValueLabel[i].setJustificationType (juce::Justification::centred);
        }
    }

    void setupReverbReturnSubTab()
    {
        // Distance Attenuation dial
        addAndMakeVisible (distanceAttenLabel);
        distanceAttenLabel.setText (LOC("reverbs.labels.distanceAtten"), juce::dontSendNotification);

        distanceAttenDial.onValueChanged = [this] (float v)
        {
            float dB = v * 6.0f - 6.0f;  // -6 to 0 dB/m
            distanceAttenValueLabel.setText (juce::String (dB, 1), juce::dontSendNotification);
            saveReverbParam (WFSParameterIDs::reverbDistanceAttenuation, dB);
        };
        distanceAttenDial.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Reverb Distance Attenuation");
        };
        distanceAttenDial.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFF4A90D9));  // Blue (level)
        addAndMakeVisible (distanceAttenDial);

        addAndMakeVisible (distanceAttenValueLabel);
        distanceAttenValueLabel.setText ("-0.7", juce::dontSendNotification);
        distanceAttenValueLabel.setJustificationType (juce::Justification::right);
        setupEditableValueLabel (distanceAttenValueLabel);
        addAndMakeVisible (distanceAttenUnitLabel);
        distanceAttenUnitLabel.setText ("dB/m", juce::dontSendNotification);
        distanceAttenUnitLabel.setJustificationType (juce::Justification::left);
        distanceAttenUnitLabel.setMinimumHorizontalScale (1.0f);

        // Common Attenuation dial
        addAndMakeVisible (commonAttenLabel);
        commonAttenLabel.setText (LOC("reverbs.labels.commonAtten"), juce::dontSendNotification);

        commonAttenDial.onValueChanged = [this] (float v)
        {
            int percent = static_cast<int> (v * 100.0f);  // 0-100%
            commonAttenValueLabel.setText (juce::String (percent), juce::dontSendNotification);
            saveReverbParam (WFSParameterIDs::reverbCommonAtten, percent);
        };
        commonAttenDial.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction ("Reverb Common Attenuation");
        };
        commonAttenDial.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFF4A90D9));  // Blue (level)
        addAndMakeVisible (commonAttenDial);

        addAndMakeVisible (commonAttenValueLabel);
        commonAttenValueLabel.setText ("100", juce::dontSendNotification);
        commonAttenValueLabel.setJustificationType (juce::Justification::right);
        setupEditableValueLabel (commonAttenValueLabel);
        addAndMakeVisible (commonAttenUnitLabel);
        commonAttenUnitLabel.setText ("%", juce::dontSendNotification);
        commonAttenUnitLabel.setJustificationType (juce::Justification::left);
        commonAttenUnitLabel.setMinimumHorizontalScale (1.0f);

        // Mute buttons (styled like InputsTab)
        addAndMakeVisible (mutesLabel);
        mutesLabel.setText (LOC("reverbs.labels.outputMutes"), juce::dontSendNotification);

        for (int i = 0; i < maxMuteButtons; ++i)
        {
            muteButtons[i].setButtonText (juce::String (i + 1));
            muteButtons[i].setClickingTogglesState (true);
            // Normal state uses theme color from WfsLookAndFeel, "on" state is orange for muted indication
            muteButtons[i].setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xFFFF5722));
            muteButtons[i].onClick = [this, i]
            {
                saveMuteStates();
            };
            addAndMakeVisible (muteButtons[i]);
        }

        // Mute Macro selector
        addAndMakeVisible (muteMacrosLabel);
        muteMacrosLabel.setText (LOC("reverbs.labels.muteMacro"), juce::dontSendNotification);

        addAndMakeVisible (muteMacrosSelector);
        muteMacrosSelector.addItem (LOC("reverbs.muteMacros.select"), 1);
        muteMacrosSelector.addItem (LOC("reverbs.muteMacros.muteAll"), 2);
        muteMacrosSelector.addItem (LOC("reverbs.muteMacros.unmuteAll"), 3);
        muteMacrosSelector.addItem (LOC("reverbs.muteMacros.invertMutes"), 4);
        muteMacrosSelector.addItem (LOC("reverbs.muteMacros.muteOdd"), 5);
        muteMacrosSelector.addItem (LOC("reverbs.muteMacros.muteEven"), 6);
        for (int arr = 1; arr <= 10; ++arr)
        {
            muteMacrosSelector.addItem (LOC("reverbs.muteMacros.muteArray") + " " + juce::String (arr), 6 + (arr - 1) * 2 + 1);
            muteMacrosSelector.addItem (LOC("reverbs.muteMacros.unmuteArray") + " " + juce::String (arr), 6 + (arr - 1) * 2 + 2);
        }
        muteMacrosSelector.setSelectedId (1);
        muteMacrosSelector.onChange = [this]
        {
            int macroId = muteMacrosSelector.getSelectedId();
            if (macroId > 1)
            {
                // TTS: Announce macro applied (before resetting selector)
                TTSManager::getInstance().announceValueChange("Mute Macro", muteMacrosSelector.getText() + " applied");
                applyMuteMacro (macroId);
                muteMacrosSelector.setSelectedId (1, juce::dontSendNotification);
            }
        };
    }

    void setupFooter()
    {
        addAndMakeVisible (storeButton);
        storeButton.setButtonText (LOC("reverbs.buttons.storeConfig"));
        storeButton.setBaseColour (juce::Colour (0xFF8C3333));  // Reddish
        storeButton.onLongPress = [this] { storeReverbConfiguration(); };

        addAndMakeVisible (reloadButton);
        reloadButton.setButtonText (LOC("reverbs.buttons.reloadConfig"));
        reloadButton.setBaseColour (juce::Colour (0xFF338C33));  // Greenish
        reloadButton.onLongPress = [this] { reloadReverbConfiguration(); };

        addAndMakeVisible (reloadBackupButton);
        reloadBackupButton.setButtonText (LOC("reverbs.buttons.reloadBackup"));
        reloadBackupButton.setBaseColour (juce::Colour (0xFF266626));  // Darker green
        reloadBackupButton.onLongPress = [this] { reloadReverbConfigBackup(); };

        addAndMakeVisible (importButton);
        importButton.setButtonText (LOC("reverbs.buttons.import"));
        importButton.setBaseColour (juce::Colour (0xFF338C33));  // Greenish
        importButton.onLongPress = [this] { importReverbConfiguration(); };

        addAndMakeVisible (exportButton);
        exportButton.setButtonText (LOC("reverbs.buttons.export"));
        exportButton.setBaseColour (juce::Colour (0xFF8C3333));  // Reddish
        exportButton.onLongPress = [this] { exportReverbConfiguration(); };
    }

    void setupEditableValueLabel (juce::Label& label)
    {
        label.setEditable (true, false);
        label.setJustificationType (juce::Justification::right);
        label.addListener (this);
    }

    void setupHelpText()
    {
        helpTextMap[&channelSelector] = LOC("reverbs.help.channelSelector");
        helpTextMap[&nameEditor] = LOC("reverbs.help.nameEditor");
        helpTextMap[&mapVisibilityButton] = LOC("reverbs.help.mapVisibility");
        helpTextMap[&attenuationSlider] = LOC("reverbs.help.attenuation");
        helpTextMap[&delayLatencySlider] = LOC("reverbs.help.delayLatency");
        helpTextMap[&directionalDial] = LOC("reverbs.help.orientation");
        helpTextMap[&angleOnSlider] = LOC("reverbs.help.angleOn");
        helpTextMap[&angleOffSlider] = LOC("reverbs.help.angleOff");
        helpTextMap[&pitchSlider] = LOC("reverbs.help.pitch");
        helpTextMap[&hfDampingSlider] = LOC("reverbs.help.hfDamping");
        helpTextMap[&distanceAttenEnableSlider] = LOC("reverbs.help.distanceAttenEnable");
        helpTextMap[&miniLatencyEnableButton] = LOC("reverbs.help.miniLatencyTooltip");
        helpTextMap[&lsEnableButton] = LOC("reverbs.help.liveSourceTooltip");
        helpTextMap[&coordModeSelector] = LOC("reverbs.help.coordMode");
        // Position/offset help text set dynamically in updatePositionLabelsAndValues()
        helpTextMap[&eqEnableButton] = LOC("reverbs.help.eqEnable");
        helpTextMap[&eqFlattenButton] = LOC("reverbs.help.eqFlatten");
        for (int i = 0; i < numEqBands; ++i)
        {
            helpTextMap[&eqBandToggle[i]] = LOC("reverbs.help.eqBandToggle").replace("{band}", juce::String(i + 1));
            helpTextMap[&eqBandShapeSelector[i]] = LOC("reverbs.help.eqShape").replace("{band}", juce::String(i + 1));
            helpTextMap[&eqBandFreqSlider[i]] = LOC("reverbs.help.eqFreq").replace("{band}", juce::String(i + 1));
            helpTextMap[&eqBandGainDial[i]] = LOC("reverbs.help.eqGain").replace("{band}", juce::String(i + 1));
            helpTextMap[&eqBandQDial[i]] = LOC("reverbs.help.eqQ").replace("{band}", juce::String(i + 1));
            helpTextMap[&eqBandResetButton[i]] = LOC("reverbs.help.eqResetBand").replace("{band}", juce::String(i + 1));
        }
        helpTextMap[&distanceAttenDial] = LOC("reverbs.help.distanceAtten");
        helpTextMap[&commonAttenDial] = LOC("reverbs.help.commonAtten");
        helpTextMap[&muteMacrosSelector] = LOC("reverbs.help.muteMacros");
        for (int i = 0; i < maxMuteButtons; ++i)
            helpTextMap[&muteButtons[i]] = LOC("reverbs.help.muteButton");
        helpTextMap[&storeButton] = LOC("reverbs.help.storeConfig");
        helpTextMap[&reloadButton] = LOC("reverbs.help.reloadConfig");
        helpTextMap[&reloadBackupButton] = LOC("reverbs.help.reloadBackup");
        helpTextMap[&importButton] = LOC("reverbs.help.importConfig");
        helpTextMap[&exportButton] = LOC("reverbs.help.exportConfig");

        // Algorithm sub-tab help text
        helpTextMap[&algoSDNButton] = LOC("reverbs.help.algoSDN");
        helpTextMap[&algoFDNButton] = LOC("reverbs.help.algoFDN");
        helpTextMap[&algoIRButton] = LOC("reverbs.help.algoIR");
        helpTextMap[&algoRT60Slider] = LOC("reverbs.help.algoRT60");
        helpTextMap[&algoRT60LowMultSlider] = LOC("reverbs.help.algoRT60LowMult");
        helpTextMap[&algoRT60HighMultSlider] = LOC("reverbs.help.algoRT60HighMult");
        helpTextMap[&algoCrossoverLowSlider] = LOC("reverbs.help.algoCrossoverLow");
        helpTextMap[&algoCrossoverHighSlider] = LOC("reverbs.help.algoCrossoverHigh");
        helpTextMap[&algoDiffusionSlider] = LOC("reverbs.help.algoDiffusion");
        helpTextMap[&algoSDNScaleSlider] = LOC("reverbs.help.algoSDNScale");
        helpTextMap[&algoFDNSizeSlider] = LOC("reverbs.help.algoFDNSize");
        helpTextMap[&algoIRFileSelector] = LOC("reverbs.help.algoIRFile");
        helpTextMap[&algoIRTrimSlider] = LOC("reverbs.help.algoIRTrim");
        helpTextMap[&algoIRLengthSlider] = LOC("reverbs.help.algoIRLength");
        helpTextMap[&algoPerNodeButton] = LOC("reverbs.help.algoPerNode");
        helpTextMap[&algoWetLevelSlider] = LOC("reverbs.help.algoWetLevel");

        // Pre-Compressor help text
        helpTextMap[&preCompBypassButton] = LOC("reverbs.help.preCompBypass");
        helpTextMap[&preCompThresholdDial] = LOC("reverbs.help.preCompThreshold");
        helpTextMap[&preCompRatioDial] = LOC("reverbs.help.preCompRatio");
        helpTextMap[&preCompAttackDial] = LOC("reverbs.help.preCompAttack");
        helpTextMap[&preCompReleaseDial] = LOC("reverbs.help.preCompRelease");

        // Post-Processing sub-tab help text
        helpTextMap[&postEqEnableButton] = LOC("reverbs.help.postEqEnable");
        helpTextMap[&postEqFlattenButton] = LOC("reverbs.help.postEqFlatten");
        for (int i = 0; i < numPostEqBands; ++i)
        {
            helpTextMap[&postEqBandToggle[i]] = LOC("reverbs.help.postEqBandToggle").replace("{band}", juce::String(i + 1));
            helpTextMap[&postEqBandShapeSelector[i]] = LOC("reverbs.help.postEqShape").replace("{band}", juce::String(i + 1));
            helpTextMap[&postEqBandFreqSlider[i]] = LOC("reverbs.help.postEqFreq").replace("{band}", juce::String(i + 1));
            helpTextMap[&postEqBandGainDial[i]] = LOC("reverbs.help.postEqGain").replace("{band}", juce::String(i + 1));
            helpTextMap[&postEqBandQDial[i]] = LOC("reverbs.help.postEqQ").replace("{band}", juce::String(i + 1));
            helpTextMap[&postEqBandResetButton[i]] = LOC("reverbs.help.postEqResetBand").replace("{band}", juce::String(i + 1));
        }

        // Post-Expander help text
        helpTextMap[&postExpBypassButton] = LOC("reverbs.help.postExpBypass");
        helpTextMap[&postExpThresholdDial] = LOC("reverbs.help.postExpThreshold");
        helpTextMap[&postExpRatioDial] = LOC("reverbs.help.postExpRatio");
        helpTextMap[&postExpAttackDial] = LOC("reverbs.help.postExpAttack");
        helpTextMap[&postExpReleaseDial] = LOC("reverbs.help.postExpRelease");
    }

    void setupOscMethods()
    {
        oscMethodMap[&channelSelector] = "/wfs/reverb/selected <ID>";
        oscMethodMap[&nameEditor] = "/wfs/reverb/name <ID> <value>";
        oscMethodMap[&attenuationSlider] = "/wfs/reverb/attenuation <ID> <value>";
        oscMethodMap[&delayLatencySlider] = "/wfs/reverb/delayLatency <ID> <value>";
        oscMethodMap[&directionalDial] = "/wfs/reverb/orientation <ID> <value>";
        oscMethodMap[&angleOnSlider] = "/wfs/reverb/angleOn <ID> <value>";
        oscMethodMap[&angleOffSlider] = "/wfs/reverb/angleOff <ID> <value>";
        oscMethodMap[&pitchSlider] = "/wfs/reverb/pitch <ID> <value>";
        oscMethodMap[&hfDampingSlider] = "/wfs/reverb/HFdamping <ID> <value>";
        oscMethodMap[&coordModeSelector] = "/wfs/reverb/coordinateMode <ID> <value>";
        oscMethodMap[&distanceAttenDial] = "/wfs/reverb/distanceAttenuation <ID> <value>";
        oscMethodMap[&commonAttenDial] = "/wfs/reverb/commonAtten <ID> <value>";

        // Algorithm sub-tab OSC methods (global, no channel ID)
        oscMethodMap[&algoSDNButton] = "/wfs/config/reverb/algoType <value>";
        oscMethodMap[&algoFDNButton] = "/wfs/config/reverb/algoType <value>";
        oscMethodMap[&algoIRButton] = "/wfs/config/reverb/algoType <value>";
        oscMethodMap[&algoRT60Slider] = "/wfs/config/reverb/rt60 <value>";
        oscMethodMap[&algoRT60LowMultSlider] = "/wfs/config/reverb/rt60LowMult <value>";
        oscMethodMap[&algoRT60HighMultSlider] = "/wfs/config/reverb/rt60HighMult <value>";
        oscMethodMap[&algoCrossoverLowSlider] = "/wfs/config/reverb/crossoverLow <value>";
        oscMethodMap[&algoCrossoverHighSlider] = "/wfs/config/reverb/crossoverHigh <value>";
        oscMethodMap[&algoDiffusionSlider] = "/wfs/config/reverb/diffusion <value>";
        oscMethodMap[&algoSDNScaleSlider] = "/wfs/config/reverb/sdnScale <value>";
        oscMethodMap[&algoFDNSizeSlider] = "/wfs/config/reverb/fdnSize <value>";
        oscMethodMap[&algoIRTrimSlider] = "/wfs/config/reverb/irTrim <value>";
        oscMethodMap[&algoIRLengthSlider] = "/wfs/config/reverb/irLength <value>";
        oscMethodMap[&algoPerNodeButton] = "/wfs/config/reverb/perNodeIR <value>";
        oscMethodMap[&algoWetLevelSlider] = "/wfs/config/reverb/wetLevel <value>";

        // Pre-Compressor OSC methods (global, no channel ID)
        oscMethodMap[&preCompBypassButton] = "/wfs/config/reverb/preCompBypass <value>";
        oscMethodMap[&preCompThresholdDial] = "/wfs/config/reverb/preCompThreshold <value>";
        oscMethodMap[&preCompRatioDial] = "/wfs/config/reverb/preCompRatio <value>";
        oscMethodMap[&preCompAttackDial] = "/wfs/config/reverb/preCompAttack <value>";
        oscMethodMap[&preCompReleaseDial] = "/wfs/config/reverb/preCompRelease <value>";

        // Post-Processing sub-tab OSC methods (global, no channel ID)
        oscMethodMap[&postEqEnableButton] = "/wfs/config/reverb/postEQenable <value>";
        for (int i = 0; i < numPostEqBands; ++i)
        {
            oscMethodMap[&postEqBandFreqSlider[i]] = "/wfs/config/reverb/postEQfreq <value>";
            oscMethodMap[&postEqBandGainDial[i]] = "/wfs/config/reverb/postEQgain <value>";
            oscMethodMap[&postEqBandQDial[i]] = "/wfs/config/reverb/postEQq <value>";
        }

        // Post-Expander OSC methods (global, no channel ID)
        oscMethodMap[&postExpBypassButton] = "/wfs/config/reverb/postExpBypass <value>";
        oscMethodMap[&postExpThresholdDial] = "/wfs/config/reverb/postExpThreshold <value>";
        oscMethodMap[&postExpRatioDial] = "/wfs/config/reverb/postExpRatio <value>";
        oscMethodMap[&postExpAttackDial] = "/wfs/config/reverb/postExpAttack <value>";
        oscMethodMap[&postExpReleaseDial] = "/wfs/config/reverb/postExpRelease <value>";
    }

    void setupMouseListeners()
    {
        for (auto& pair : helpTextMap)
        {
            // Use true for ComboBoxes to receive events from their internal child components
            bool wantsEventsFromChildren = (dynamic_cast<juce::ComboBox*>(pair.first) != nullptr);
            pair.first->addMouseListener (this, wantsEventsFromChildren);
        }
    }

    //==========================================================================
    // Layout Methods
    //==========================================================================

    void layoutHeader (juce::Rectangle<int> area)
    {
        const int rowHeight = scaled(30);
        const int spacing = scaled(5);

        auto row = area.removeFromTop (rowHeight);

        channelSelector.setBounds (row.removeFromLeft (scaled(150)));
        row.removeFromLeft (spacing * 2);

        nameLabel.setBounds (row.removeFromLeft (scaled(50)));
        nameEditor.setBounds (row.removeFromLeft (scaled(200)));

        row.removeFromLeft (spacing * 4);
        mapVisibilityButton.setBounds (row.removeFromLeft (scaled(180)));
    }

    void layoutFooter (juce::Rectangle<int> area)
    {
        const int spacing = scaled(5);
        const int buttonWidth = (area.getWidth() - spacing * 4) / 5;

        storeButton.setBounds (area.removeFromLeft (buttonWidth));
        area.removeFromLeft (spacing);
        reloadButton.setBounds (area.removeFromLeft (buttonWidth));
        area.removeFromLeft (spacing);
        reloadBackupButton.setBounds (area.removeFromLeft (buttonWidth));
        area.removeFromLeft (spacing);
        importButton.setBounds (area.removeFromLeft (buttonWidth));
        area.removeFromLeft (spacing);
        exportButton.setBounds (area.removeFromLeft (buttonWidth));
    }

    void layoutCurrentSubTab()
    {
        int tabIndex = subTabBar.getCurrentTabIndex();

        // Hide all sub-tab components
        setChannelParametersVisible (false);
        setEQVisible (false);
        setAlgorithmVisible (false);
        setPostProcessingVisible (false);

        switch (tabIndex)
        {
            case 0: setChannelParametersVisible (true); layoutChannelParametersTab(); break;
            case 1: setEQVisible (true); layoutEQSubTab(); break;
            case 2: setAlgorithmVisible (true); layoutAlgorithmSubTab(); break;
            case 3: setPostProcessingVisible (true); layoutPostProcessingSubTab(); break;
        }
    }

    void layoutPreCompressor (juce::Rectangle<int> area)
    {
        const int rowHeight = scaled(30);
        const int dialSize = juce::jmax(60, static_cast<int>(100.0f * layoutScale));
        const int labelHeight = scaled(20);
        const int spacing = scaled(5);

        area.removeFromTop (spacing * 2);

        // Section label + bypass button row
        auto headerRow = area.removeFromTop (rowHeight);
        preCompSectionLabel.setBounds (headerRow.removeFromLeft (scaled(120)));
        preCompBypassButton.setBounds (headerRow.removeFromLeft (scaled(150)));
        area.removeFromTop (spacing);

        // 4 dials in a horizontal row: label / dial / value
        auto dialRow = area.removeFromTop (labelHeight + dialSize + labelHeight);
        int dialColumnWidth = dialRow.getWidth() / 4;

        const int valueLabelWidth = dialSize + 16;

        auto threshArea = dialRow.removeFromLeft (dialColumnWidth);
        preCompThresholdLabel.setBounds (threshArea.removeFromTop (labelHeight));
        preCompThresholdValueLabel.setBounds (threshArea.removeFromBottom (labelHeight).withSizeKeepingCentre (valueLabelWidth, labelHeight));
        preCompThresholdDial.setBounds (threshArea.withSizeKeepingCentre (dialSize, dialSize));

        auto ratioArea = dialRow.removeFromLeft (dialColumnWidth);
        preCompRatioLabel.setBounds (ratioArea.removeFromTop (labelHeight));
        preCompRatioValueLabel.setBounds (ratioArea.removeFromBottom (labelHeight).withSizeKeepingCentre (valueLabelWidth, labelHeight));
        preCompRatioDial.setBounds (ratioArea.withSizeKeepingCentre (dialSize, dialSize));

        auto attackArea = dialRow.removeFromLeft (dialColumnWidth);
        preCompAttackLabel.setBounds (attackArea.removeFromTop (labelHeight));
        preCompAttackValueLabel.setBounds (attackArea.removeFromBottom (labelHeight).withSizeKeepingCentre (valueLabelWidth, labelHeight));
        preCompAttackDial.setBounds (attackArea.withSizeKeepingCentre (dialSize, dialSize));

        auto releaseArea = dialRow;
        preCompReleaseLabel.setBounds (releaseArea.removeFromTop (labelHeight));
        preCompReleaseValueLabel.setBounds (releaseArea.removeFromBottom (labelHeight).withSizeKeepingCentre (valueLabelWidth, labelHeight));
        preCompReleaseDial.setBounds (releaseArea.withSizeKeepingCentre (dialSize, dialSize));
    }

    void layoutEQSubTab()
    {
        auto fullArea = subTabContentArea;
        const int buttonHeight = scaled(30);
        const int dialSize = juce::jmax(40, static_cast<int>(65.0f * layoutScale));
        const int sliderHeight = scaled(35);
        const int labelHeight = scaled(20);
        const int spacing = scaled(5);
        const int toggleSize = scaled(18);

        // Reserve bottom portion for compressor section (must match layoutPreCompressor content)
        const int compDialSize = juce::jmax(60, static_cast<int>(100.0f * layoutScale));
        const int compressorHeight = scaled(30) + scaled(5) * 3 + scaled(20) * 2 + compDialSize;
        auto compArea = fullArea.removeFromBottom (compressorHeight);
        auto area = fullArea;

        const int bandWidth = area.getWidth() / numEqBands;

        // Top row: EQ Enable button left, Flatten button right
        auto topRow = area.removeFromTop (buttonHeight);
        eqEnableButton.setBounds (topRow.removeFromLeft (scaled(100)));
        eqFlattenButton.setBounds (topRow.removeFromRight (scaled(100)));
        area.removeFromTop (spacing * 2);

        // Create EQ Display if it doesn't exist yet (fallback creation)
        if (eqDisplay == nullptr && currentChannel > 0)
        {
            // Use ensureReverbEQSection to create EQ section if missing (e.g., old config files)
            auto eqTree = parameters.getValueTreeState().ensureReverbEQSection (currentChannel - 1);
            if (eqTree.isValid())
            {
                eqDisplay = std::make_unique<EQDisplayComponent> (eqTree, numEqBands, EQDisplayConfig::forReverbPreEQ());
                addAndMakeVisible (*eqDisplay);
                eqDisplay->setUndoManager (parameters.getUndoManagerForDomain (UndoDomain::Reverb));
                lastEqDisplayChannel = currentChannel;

                bool eqEnabled = eqEnableButton.getToggleState();
                eqDisplay->setEQEnabled (eqEnabled);
            }
        }

        // EQ Display component (takes upper portion, min 180px, target ~35% of remaining height)
        if (eqDisplay)
        {
            int displayHeight = juce::jmax (180, area.getHeight() * 35 / 100);
            eqDisplay->setBounds (area.removeFromTop (displayHeight));
            area.removeFromTop (spacing);
        }

        // Layout bands horizontally
        for (int i = 0; i < numEqBands; ++i)
        {
            auto bandArea = area.removeFromLeft (bandWidth).reduced (scaled(5), 0);

            // Band label row
            eqBandLabel[i].setBounds (bandArea.removeFromTop (labelHeight));

            // Shape row: toggle on left, combobox in middle, reset on right
            auto shapeRow = bandArea.removeFromTop (buttonHeight);
            eqBandToggle[i].setBounds (shapeRow.removeFromLeft (toggleSize).withSizeKeepingCentre (toggleSize, toggleSize));
            shapeRow.removeFromLeft (scaled(4));
            eqBandResetButton[i].setBounds (shapeRow.removeFromRight (scaled(50)));
            eqBandShapeSelector[i].setBounds (shapeRow);
            bandArea.removeFromTop (spacing);

            // Frequency slider
            eqBandFreqLabel[i].setBounds (bandArea.removeFromTop (labelHeight));
            eqBandFreqSlider[i].setBounds (bandArea.removeFromTop (sliderHeight));
            eqBandFreqValueLabel[i].setBounds (bandArea.removeFromTop (labelHeight));
            bandArea.removeFromTop (spacing);

            // Gain and Q dials in a row
            auto dialRow = bandArea.removeFromTop (dialSize + labelHeight * 2);
            int dialSpacing = (dialRow.getWidth() - dialSize * 2) / 3;

            auto gainArea = dialRow.removeFromLeft (dialSize + dialSpacing).reduced (dialSpacing / 2, 0);
            eqBandGainLabel[i].setBounds (gainArea.removeFromTop (labelHeight));
            eqBandGainDial[i].setBounds (gainArea.removeFromTop (dialSize).withSizeKeepingCentre (dialSize, dialSize));
            eqBandGainValueLabel[i].setBounds (gainArea.removeFromTop (labelHeight));

            auto qArea = dialRow.removeFromLeft (dialSize + dialSpacing).reduced (dialSpacing / 2, 0);
            eqBandQLabel[i].setBounds (qArea.removeFromTop (labelHeight));
            eqBandQDial[i].setBounds (qArea.removeFromTop (dialSize).withSizeKeepingCentre (dialSize, dialSize));
            eqBandQValueLabel[i].setBounds (qArea.removeFromTop (labelHeight));
        }

        // Layout pre-compressor section
        layoutPreCompressor (compArea);
    }

    void layoutAlgorithmSubTab()
    {
        auto area = subTabContentArea.reduced (scaled(10), scaled(10));
        const int rowHeight = scaled(30);
        const int sliderHeight = scaled(35);
        const int spacing = scaled(8);
        const int labelWidth = scaled(120);
        const int valueWidth = scaled(80);
        const int buttonWidth = scaled(60);
        const int titleHeight = scaled(25);

        // Algorithm type selector row
        auto selectorRow = area.removeFromTop (rowHeight);
        algoSDNButton.setBounds (selectorRow.removeFromLeft (buttonWidth));
        selectorRow.removeFromLeft (spacing);
        algoFDNButton.setBounds (selectorRow.removeFromLeft (buttonWidth));
        selectorRow.removeFromLeft (spacing);
        algoIRButton.setBounds (selectorRow.removeFromLeft (buttonWidth));
        area.removeFromTop (spacing * 2);

        // Two-column layout
        int colWidth = area.getWidth() / 2;
        auto col1 = area.removeFromLeft (colWidth);
        auto col2 = area.reduced (scaled(5), 0);

        // === Left Column: Decay + SDN/FDN ===
        if (algoDecaySectionLabel.isVisible())
        {
            algoDecaySectionLabel.setBounds (col1.removeFromTop (titleHeight));
            col1.removeFromTop (spacing);

            auto row = col1.removeFromTop (rowHeight);
            algoRT60Label.setBounds (row.removeFromLeft (labelWidth));
            algoRT60ValueLabel.setBounds (row.removeFromRight (valueWidth));
            col1.removeFromTop (scaled(3));
            algoRT60Slider.setBounds (col1.removeFromTop (sliderHeight));
            col1.removeFromTop (spacing);

            row = col1.removeFromTop (rowHeight);
            algoRT60LowMultLabel.setBounds (row.removeFromLeft (labelWidth));
            algoRT60LowMultValueLabel.setBounds (row.removeFromRight (valueWidth));
            col1.removeFromTop (scaled(3));
            algoRT60LowMultSlider.setBounds (col1.removeFromTop (sliderHeight));
            col1.removeFromTop (spacing);

            row = col1.removeFromTop (rowHeight);
            algoRT60HighMultLabel.setBounds (row.removeFromLeft (labelWidth));
            algoRT60HighMultValueLabel.setBounds (row.removeFromRight (valueWidth));
            col1.removeFromTop (scaled(3));
            algoRT60HighMultSlider.setBounds (col1.removeFromTop (sliderHeight));
            col1.removeFromTop (spacing);

            row = col1.removeFromTop (rowHeight);
            algoCrossoverLowLabel.setBounds (row.removeFromLeft (labelWidth));
            algoCrossoverLowValueLabel.setBounds (row.removeFromRight (valueWidth));
            col1.removeFromTop (scaled(3));
            algoCrossoverLowSlider.setBounds (col1.removeFromTop (sliderHeight));
            col1.removeFromTop (spacing);

            row = col1.removeFromTop (rowHeight);
            algoCrossoverHighLabel.setBounds (row.removeFromLeft (labelWidth));
            algoCrossoverHighValueLabel.setBounds (row.removeFromRight (valueWidth));
            col1.removeFromTop (scaled(3));
            algoCrossoverHighSlider.setBounds (col1.removeFromTop (sliderHeight));
            col1.removeFromTop (spacing);

            row = col1.removeFromTop (rowHeight);
            algoDiffusionLabel.setBounds (row.removeFromLeft (labelWidth));
            algoDiffusionValueLabel.setBounds (row.removeFromRight (valueWidth));
            col1.removeFromTop (scaled(3));
            algoDiffusionSlider.setBounds (col1.removeFromTop (sliderHeight));
            col1.removeFromTop (spacing * 2);
        }

        if (algoSDNSectionLabel.isVisible())
        {
            algoSDNSectionLabel.setBounds (col1.removeFromTop (titleHeight));
            col1.removeFromTop (spacing);

            auto row = col1.removeFromTop (rowHeight);
            algoSDNScaleLabel.setBounds (row.removeFromLeft (labelWidth));
            algoSDNScaleValueLabel.setBounds (row.removeFromRight (valueWidth));
            col1.removeFromTop (scaled(3));
            algoSDNScaleSlider.setBounds (col1.removeFromTop (sliderHeight));
        }

        if (algoFDNSectionLabel.isVisible())
        {
            algoFDNSectionLabel.setBounds (col1.removeFromTop (titleHeight));
            col1.removeFromTop (spacing);

            auto row = col1.removeFromTop (rowHeight);
            algoFDNSizeLabel.setBounds (row.removeFromLeft (labelWidth));
            algoFDNSizeValueLabel.setBounds (row.removeFromRight (valueWidth));
            col1.removeFromTop (scaled(3));
            algoFDNSizeSlider.setBounds (col1.removeFromTop (sliderHeight));
        }

        // === Right Column: IR + Output ===
        if (algoIRSectionLabel.isVisible())
        {
            algoIRSectionLabel.setBounds (col2.removeFromTop (titleHeight));
            col2.removeFromTop (spacing);

            auto row = col2.removeFromTop (rowHeight);
            algoIRFileLabel.setBounds (row.removeFromLeft (labelWidth));
            algoIRFileSelector.setBounds (row);
            col2.removeFromTop (spacing);

            row = col2.removeFromTop (rowHeight);
            algoIRTrimLabel.setBounds (row.removeFromLeft (labelWidth));
            algoIRTrimValueLabel.setBounds (row.removeFromRight (valueWidth));
            col2.removeFromTop (scaled(3));
            algoIRTrimSlider.setBounds (col2.removeFromTop (sliderHeight));
            col2.removeFromTop (spacing);

            row = col2.removeFromTop (rowHeight);
            algoIRLengthLabel.setBounds (row.removeFromLeft (labelWidth));
            algoIRLengthValueLabel.setBounds (row.removeFromRight (valueWidth));
            col2.removeFromTop (scaled(3));
            algoIRLengthSlider.setBounds (col2.removeFromTop (sliderHeight));
            col2.removeFromTop (spacing);

            algoPerNodeButton.setBounds (col2.removeFromTop (rowHeight).withWidth (scaled(180)));
            col2.removeFromTop (spacing * 2);
        }

        // Output section (always visible, right column)
        auto wetRow = col2.removeFromTop (rowHeight);
        algoWetLevelLabel.setBounds (wetRow.removeFromLeft (labelWidth));
        algoWetLevelValueLabel.setBounds (wetRow.removeFromRight (valueWidth));
        col2.removeFromTop (scaled(3));
        algoWetLevelSlider.setBounds (col2.removeFromTop (sliderHeight));
    }

    void layoutPostExpander (juce::Rectangle<int> area)
    {
        const int rowHeight = scaled(30);
        const int dialSize = juce::jmax(60, static_cast<int>(100.0f * layoutScale));
        const int labelHeight = scaled(20);
        const int spacing = scaled(5);

        area.removeFromTop (spacing * 2);

        // Section label + bypass button row
        auto headerRow = area.removeFromTop (rowHeight);
        postExpSectionLabel.setBounds (headerRow.removeFromLeft (scaled(120)));
        postExpBypassButton.setBounds (headerRow.removeFromLeft (scaled(150)));
        area.removeFromTop (spacing);

        // 4 dials in a horizontal row: label / dial / value
        auto dialRow = area.removeFromTop (labelHeight + dialSize + labelHeight);
        int dialColumnWidth = dialRow.getWidth() / 4;
        const int valueLabelWidth = dialSize + 16;

        auto threshArea = dialRow.removeFromLeft (dialColumnWidth);
        postExpThresholdLabel.setBounds (threshArea.removeFromTop (labelHeight));
        postExpThresholdValueLabel.setBounds (threshArea.removeFromBottom (labelHeight).withSizeKeepingCentre (valueLabelWidth, labelHeight));
        postExpThresholdDial.setBounds (threshArea.withSizeKeepingCentre (dialSize, dialSize));

        auto ratioArea = dialRow.removeFromLeft (dialColumnWidth);
        postExpRatioLabel.setBounds (ratioArea.removeFromTop (labelHeight));
        postExpRatioValueLabel.setBounds (ratioArea.removeFromBottom (labelHeight).withSizeKeepingCentre (valueLabelWidth, labelHeight));
        postExpRatioDial.setBounds (ratioArea.withSizeKeepingCentre (dialSize, dialSize));

        auto attackArea = dialRow.removeFromLeft (dialColumnWidth);
        postExpAttackLabel.setBounds (attackArea.removeFromTop (labelHeight));
        postExpAttackValueLabel.setBounds (attackArea.removeFromBottom (labelHeight).withSizeKeepingCentre (valueLabelWidth, labelHeight));
        postExpAttackDial.setBounds (attackArea.withSizeKeepingCentre (dialSize, dialSize));

        auto releaseArea = dialRow;
        postExpReleaseLabel.setBounds (releaseArea.removeFromTop (labelHeight));
        postExpReleaseValueLabel.setBounds (releaseArea.removeFromBottom (labelHeight).withSizeKeepingCentre (valueLabelWidth, labelHeight));
        postExpReleaseDial.setBounds (releaseArea.withSizeKeepingCentre (dialSize, dialSize));
    }

    void layoutPostProcessingSubTab()
    {
        auto fullArea = subTabContentArea;
        const int buttonHeight = scaled(30);
        const int dialSize = juce::jmax(40, static_cast<int>(65.0f * layoutScale));
        const int sliderHeight = scaled(35);
        const int labelHeight = scaled(20);
        const int spacing = scaled(5);
        const int toggleSize = scaled(18);

        // Reserve bottom portion for expander section (must match layoutPostExpander content)
        const int expDialSize = juce::jmax(60, static_cast<int>(100.0f * layoutScale));
        const int expanderHeight = scaled(30) + scaled(5) * 3 + scaled(20) * 2 + expDialSize;
        auto expArea = fullArea.removeFromBottom (expanderHeight);
        auto area = fullArea;

        const int bandWidth = area.getWidth() / numPostEqBands;

        // Top row: Post-EQ Enable button left, Flatten button right
        auto topRow = area.removeFromTop (buttonHeight);
        postEqEnableButton.setBounds (topRow.removeFromLeft (scaled(100)));
        postEqFlattenButton.setBounds (topRow.removeFromRight (scaled(100)));
        area.removeFromTop (spacing * 2);

        // Create Post-EQ Display if it doesn't exist yet
        if (postEqDisplay == nullptr)
        {
            auto postEqTree = parameters.getValueTreeState().ensureReverbPostEQSection();
            if (postEqTree.isValid())
            {
                postEqDisplay = std::make_unique<EQDisplayComponent> (postEqTree, numPostEqBands, EQDisplayConfig::forReverbPostEQ());
                addAndMakeVisible (*postEqDisplay);
                postEqDisplay->setUndoManager (parameters.getUndoManagerForDomain (UndoDomain::Reverb));

                bool eqEnabled = postEqEnableButton.getToggleState();
                postEqDisplay->setEQEnabled (eqEnabled);
            }
        }

        // Post-EQ Display component
        if (postEqDisplay)
        {
            int displayHeight = juce::jmax (180, area.getHeight() * 35 / 100);
            postEqDisplay->setBounds (area.removeFromTop (displayHeight));
            area.removeFromTop (spacing);
        }

        // Layout bands horizontally
        for (int i = 0; i < numPostEqBands; ++i)
        {
            auto bandArea = area.removeFromLeft (bandWidth).reduced (scaled(5), 0);

            // Band label row
            postEqBandLabel[i].setBounds (bandArea.removeFromTop (labelHeight));

            // Shape row: toggle on left, combobox in middle, reset on right
            auto shapeRow = bandArea.removeFromTop (buttonHeight);
            postEqBandToggle[i].setBounds (shapeRow.removeFromLeft (toggleSize).withSizeKeepingCentre (toggleSize, toggleSize));
            shapeRow.removeFromLeft (scaled(4));
            postEqBandResetButton[i].setBounds (shapeRow.removeFromRight (scaled(50)));
            postEqBandShapeSelector[i].setBounds (shapeRow);
            bandArea.removeFromTop (spacing);

            // Frequency slider
            postEqBandFreqLabel[i].setBounds (bandArea.removeFromTop (labelHeight));
            postEqBandFreqSlider[i].setBounds (bandArea.removeFromTop (sliderHeight));
            postEqBandFreqValueLabel[i].setBounds (bandArea.removeFromTop (labelHeight));
            bandArea.removeFromTop (spacing);

            // Gain and Q dials in a row
            auto dialRow = bandArea.removeFromTop (dialSize + labelHeight * 2);
            int dialSpacing = (dialRow.getWidth() - dialSize * 2) / 3;

            auto gainArea = dialRow.removeFromLeft (dialSize + dialSpacing).reduced (dialSpacing / 2, 0);
            postEqBandGainLabel[i].setBounds (gainArea.removeFromTop (labelHeight));
            postEqBandGainDial[i].setBounds (gainArea.removeFromTop (dialSize).withSizeKeepingCentre (dialSize, dialSize));
            postEqBandGainValueLabel[i].setBounds (gainArea.removeFromTop (labelHeight));

            auto qArea = dialRow.removeFromLeft (dialSize + dialSpacing).reduced (dialSpacing / 2, 0);
            postEqBandQLabel[i].setBounds (qArea.removeFromTop (labelHeight));
            postEqBandQDial[i].setBounds (qArea.removeFromTop (dialSize).withSizeKeepingCentre (dialSize, dialSize));
            postEqBandQValueLabel[i].setBounds (qArea.removeFromTop (labelHeight));
        }

        // Layout post-expander section
        layoutPostExpander (expArea);
    }

    void layoutChannelParametersTab()
    {
        auto area = subTabContentArea.reduced (scaled(10), scaled(10));
        const int rowHeight = scaled(30);
        const int sliderHeight = scaled(40);
        const int spacing = scaled(10);
        const int labelWidth = scaled(115);
        const int valueWidth = scaled(60);  // Tight value width like LFO section
        const int editorWidth = scaled(70);
        const int unitWidth = scaled(25);
        const int dialSize = juce::jmax(60, static_cast<int>(100.0f * layoutScale));
        const int titleHeight = scaled(25);

        // Divide into 3 columns
        int colWidth = area.getWidth() / 3;
        auto col1 = area.removeFromLeft (colWidth).reduced (scaled(5), 0);
        auto col2 = area.removeFromLeft (colWidth).reduced (scaled(5), 0);
        auto col3 = area.reduced (scaled(5), 0);

        // =====================================================================
        // Column 1: Reverb + Position
        // =====================================================================

        // Attenuation
        auto row = col1.removeFromTop (rowHeight);
        attenuationLabel.setBounds (row.removeFromLeft (labelWidth));
        attenuationValueLabel.setBounds (row.removeFromRight (valueWidth));
        col1.removeFromTop (scaled(3));
        attenuationSlider.setBounds (col1.removeFromTop (sliderHeight));
        col1.removeFromTop (spacing);

        // Delay/Latency
        row = col1.removeFromTop (rowHeight);
        delayLatencyLabel.setBounds (row.removeFromLeft (labelWidth));
        delayLatencyValueLabel.setBounds (row.removeFromRight (scaled(130)));  // Wider for "Latency: 100.0 ms"
        col1.removeFromTop (scaled(3));
        delayLatencySlider.setBounds (col1.removeFromTop (sliderHeight));
        col1.removeFromTop (spacing);

        // Coordinate mode selector
        auto coordModeRow = col1.removeFromTop (rowHeight);
        coordModeLabel.setBounds (coordModeRow.removeFromLeft (scaled(50)));
        coordModeSelector.setBounds (coordModeRow.removeFromLeft (scaled(80)));
        col1.removeFromTop (spacing);

        // Position X/Y/Z with Return Offset on same rows
        juce::Label* posLabelPtrs[] = { &posXLabel, &posYLabel, &posZLabel };
        juce::TextEditor* posEditorPtrs[] = { &posXEditor, &posYEditor, &posZEditor };
        juce::Label* posUnitPtrs[] = { &posXUnitLabel, &posYUnitLabel, &posZUnitLabel };
        juce::Label* offsetLabelPtrs[] = { &returnOffsetXLabel, &returnOffsetYLabel, &returnOffsetZLabel };
        juce::TextEditor* offsetEditorPtrs[] = { &returnOffsetXEditor, &returnOffsetYEditor, &returnOffsetZEditor };
        juce::Label* offsetUnitPtrs[] = { &returnOffsetXUnitLabel, &returnOffsetYUnitLabel, &returnOffsetZUnitLabel };

        for (int i = 0; i < 3; ++i)
        {
            row = col1.removeFromTop (rowHeight);
            // Position on left side
            posLabelPtrs[i]->setBounds (row.removeFromLeft (labelWidth));
            posEditorPtrs[i]->setBounds (row.removeFromLeft (editorWidth));
            row.removeFromLeft (scaled(3));
            posUnitPtrs[i]->setBounds (row.removeFromLeft (unitWidth));
            // Larger gap between position and offset columns
            row.removeFromLeft (scaled(25));
            // Return Offset on right side
            offsetLabelPtrs[i]->setBounds (row.removeFromLeft (labelWidth));
            offsetEditorPtrs[i]->setBounds (row.removeFromLeft (editorWidth));
            row.removeFromLeft (scaled(3));
            offsetUnitPtrs[i]->setBounds (row.removeFromLeft (unitWidth));
            col1.removeFromTop (scaled(5));
        }

        // =====================================================================
        // Column 2: Reverb Feed
        // =====================================================================

        // Column title
        reverbFeedTitleLabel.setBounds (col2.removeFromTop (titleHeight));
        col2.removeFromTop (spacing);

        // Directional dial on the right, sliders on the left
        {
            const int ddDialSize = scaled(90);
            const int ddDialMargin = scaled(20);
            const int sliderGroupHeight = 3 * (rowHeight + 3 + sliderHeight) + 2 * spacing;  // 3 sliders with spacing
            const int dialGroupHeight = rowHeight + ddDialSize + rowHeight;  // label + dial + value
            const int dialTopOffset = (sliderGroupHeight - dialGroupHeight) / 2;

            // Save col2 bounds before carving out dial column
            auto col2Full = col2;

            // Carve out dial column from the right
            auto dialColumn = col2.removeFromRight (ddDialSize + ddDialMargin);
            dialColumn.removeFromTop (juce::jmax (0, dialTopOffset));

            // Orientation label centered above dial
            auto orientLabelArea = dialColumn.removeFromTop (rowHeight);
            orientationLabel.setBounds (orientLabelArea);
            orientationLabel.setJustificationType (juce::Justification::centred);

            // Directional dial
            auto dialArea = dialColumn.removeFromTop (ddDialSize);
            int orientDialCenterX = dialArea.getCentreX();
            directionalDial.setBounds (dialArea.withSizeKeepingCentre (ddDialSize, ddDialSize));

            // Value + unit centered under dial
            auto orientValueRow = dialColumn.removeFromTop (rowHeight);
            const int orientValW = scaled(40), orientUnitW = scaled(30), orientOverlap = scaled(7);
            int orientStartX = orientDialCenterX - (orientValW + orientUnitW - orientOverlap) / 2;
            orientationValueLabel.setBounds (orientStartX, orientValueRow.getY(), orientValW, rowHeight);
            orientationUnitLabel.setBounds (orientStartX + orientValW - orientOverlap, orientValueRow.getY(), orientUnitW, rowHeight);

            // Angle On slider (in remaining col2 width)
            row = col2.removeFromTop (rowHeight);
            angleOnLabel.setBounds (row.removeFromLeft (labelWidth));
            angleOnValueLabel.setBounds (row.removeFromRight (valueWidth));
            col2.removeFromTop (scaled(3));
            angleOnSlider.setBounds (col2.removeFromTop (sliderHeight));
            col2.removeFromTop (spacing);

            // Angle Off slider
            row = col2.removeFromTop (rowHeight);
            angleOffLabel.setBounds (row.removeFromLeft (labelWidth));
            angleOffValueLabel.setBounds (row.removeFromRight (valueWidth));
            col2.removeFromTop (scaled(3));
            angleOffSlider.setBounds (col2.removeFromTop (sliderHeight));
            col2.removeFromTop (spacing);

            // Pitch slider
            row = col2.removeFromTop (rowHeight);
            pitchLabel.setBounds (row.removeFromLeft (labelWidth));
            pitchValueLabel.setBounds (row.removeFromRight (valueWidth));
            col2.removeFromTop (scaled(3));
            pitchSlider.setBounds (col2.removeFromTop (sliderHeight));
            col2.removeFromTop (spacing);

            // Restore col2 to full width for remaining controls
            col2 = col2Full;
            col2.removeFromTop (titleHeight + spacing + sliderGroupHeight + spacing);
        }

        // HF Damping slider
        row = col2.removeFromTop (rowHeight);
        hfDampingLabel.setBounds (row.removeFromLeft (labelWidth));
        hfDampingValueLabel.setBounds (row.removeFromRight (valueWidth));
        col2.removeFromTop (scaled(3));
        hfDampingSlider.setBounds (col2.removeFromTop (sliderHeight));
        col2.removeFromTop (spacing);

        // Distance Atten Enable slider
        row = col2.removeFromTop (rowHeight);
        distanceAttenEnableLabel.setBounds (row.removeFromLeft (labelWidth));
        distanceAttenEnableValueLabel.setBounds (row.removeFromRight (valueWidth));
        col2.removeFromTop (scaled(3));
        distanceAttenEnableSlider.setBounds (col2.removeFromTop (sliderHeight));
        col2.removeFromTop (spacing);

        // Buttons - side by side on same row
        int buttonWidth = (col2.getWidth() - spacing) / 2;
        auto buttonRow = col2.removeFromTop (rowHeight);
        miniLatencyEnableButton.setBounds (buttonRow.removeFromLeft (buttonWidth));
        buttonRow.removeFromLeft (spacing);
        lsEnableButton.setBounds (buttonRow.removeFromLeft (buttonWidth));

        // =====================================================================
        // Column 3: Reverb Return
        // =====================================================================

        // Column title
        reverbReturnTitleLabel.setBounds (col3.removeFromTop (titleHeight));
        col3.removeFromTop (spacing);

        // Distance Attenuation and Common Attenuation dials side by side
        int halfColWidth = col3.getWidth() / 2;
        auto dialsRow = col3.removeFromTop (dialSize + rowHeight * 2 + spacing);

        // Left half: Distance Attenuation
        auto leftDialArea = dialsRow.removeFromLeft (halfColWidth);
        distanceAttenLabel.setBounds (leftDialArea.removeFromTop (rowHeight).withSizeKeepingCentre (dialSize + scaled(40), rowHeight));
        distanceAttenLabel.setJustificationType (juce::Justification::centred);
        auto leftDialBounds = leftDialArea.removeFromTop (dialSize);
        int distCenterX = leftDialBounds.getCentreX();
        distanceAttenDial.setBounds (leftDialBounds.withSizeKeepingCentre (dialSize, dialSize));
        auto distValueRow = leftDialArea.removeFromTop (rowHeight);
        // Value and unit adjacent, centered as a pair under dial (with overlap to reduce font padding gap)
        const int distValW = scaled(35), distUnitW = scaled(50), distOverlap = scaled(7);
        int distStartX = distCenterX - (distValW + distUnitW - distOverlap) / 2;
        distanceAttenValueLabel.setBounds (distStartX, distValueRow.getY(), distValW, rowHeight);
        distanceAttenUnitLabel.setBounds (distStartX + distValW - distOverlap, distValueRow.getY(), distUnitW, rowHeight);

        // Right half: Common Attenuation
        auto rightDialArea = dialsRow;
        commonAttenLabel.setBounds (rightDialArea.removeFromTop (rowHeight).withSizeKeepingCentre (dialSize + scaled(40), rowHeight));
        commonAttenLabel.setJustificationType (juce::Justification::centred);
        auto rightDialBounds = rightDialArea.removeFromTop (dialSize);
        int commonCenterX = rightDialBounds.getCentreX();
        commonAttenDial.setBounds (rightDialBounds.withSizeKeepingCentre (dialSize, dialSize));
        auto commonValueRow = rightDialArea.removeFromTop (rowHeight);
        // Value and unit adjacent, centered as a pair under dial (with overlap to reduce font padding gap)
        const int commonValW = scaled(40), commonUnitW = scaled(30), commonOverlap = scaled(7);
        int commonStartX = commonCenterX - (commonValW + commonUnitW - commonOverlap) / 2;
        commonAttenValueLabel.setBounds (commonStartX, commonValueRow.getY(), commonValW, rowHeight);
        commonAttenUnitLabel.setBounds (commonStartX + commonValW - commonOverlap, commonValueRow.getY(), commonUnitW, rowHeight);

        col3.removeFromTop (spacing);

        // Mute Macro selector
        muteMacrosLabel.setBounds (col3.removeFromTop (rowHeight));
        muteMacrosSelector.setBounds (col3.removeFromTop (scaled(30)));
        col3.removeFromTop (spacing);

        // Mutes section
        mutesLabel.setBounds (col3.removeFromTop (titleHeight));
        col3.removeFromTop (scaled(5));

        // Layout mute buttons in a grid - use full column width
        int numOutputs = parameters.getNumOutputChannels();
        if (numOutputs <= 0) numOutputs = 16;
        const int buttonSize = scaled(35);
        const int buttonSpacing = scaled(3);
        // Calculate how many buttons fit in the column width
        int numColumns = (col3.getWidth() + buttonSpacing) / (buttonSize + buttonSpacing);
        numColumns = juce::jmax (1, numColumns);  // At least 1 column

        for (int i = 0; i < maxMuteButtons; ++i)
        {
            if (i < numOutputs)
            {
                int colIdx = i % numColumns;
                int rowIdx = i / numColumns;
                int x = colIdx * (buttonSize + buttonSpacing);
                int y = rowIdx * (buttonSize + buttonSpacing);
                muteButtons[i].setBounds (col3.getX() + x, col3.getY() + y, buttonSize, buttonSize);
                muteButtons[i].setVisible (true);
            }
            else
            {
                muteButtons[i].setVisible (false);
            }
        }
    }

    //==========================================================================
    // Visibility Methods
    //==========================================================================

    void setEQVisible (bool visible)
    {
        eqEnableButton.setVisible (visible);
        eqFlattenButton.setVisible (visible);

        // EQ Display - create if needed and visible
        if (visible && eqDisplay == nullptr && currentChannel > 0)
        {
            // Use ensureReverbEQSection to create EQ section if missing (e.g., old config files)
            auto eqTree = parameters.getValueTreeState().ensureReverbEQSection (currentChannel - 1);
            if (eqTree.isValid())
            {
                eqDisplay = std::make_unique<EQDisplayComponent> (eqTree, numEqBands, EQDisplayConfig::forReverbPreEQ());
                addAndMakeVisible (*eqDisplay);
                eqDisplay->setUndoManager (parameters.getUndoManagerForDomain (UndoDomain::Reverb));
                lastEqDisplayChannel = currentChannel;

                int eqEnabled = currentChannel > 0 ?
                    static_cast<int>(parameters.getValueTreeState().getReverbParameter (currentChannel - 1, WFSParameterIDs::reverbPreEQenable)) : 1;
                eqDisplay->setEQEnabled (eqEnabled != 0);
            }
        }
        if (eqDisplay)
            eqDisplay->setVisible (visible);

        for (int i = 0; i < numEqBands; ++i)
        {
            eqBandLabel[i].setVisible (visible);
            eqBandToggle[i].setVisible (visible);
            eqBandShapeSelector[i].setVisible (visible);
            eqBandResetButton[i].setVisible (visible);
            eqBandFreqLabel[i].setVisible (visible);
            eqBandFreqSlider[i].setVisible (visible);
            eqBandFreqValueLabel[i].setVisible (visible);
            eqBandQLabel[i].setVisible (visible);
            eqBandQDial[i].setVisible (visible);
            eqBandQValueLabel[i].setVisible (visible);

            // Show/hide gain based on filter shape (hide for cut filters)
            if (visible)
                updateEQBandAppearance (i);
            else
            {
                eqBandGainLabel[i].setVisible (false);
                eqBandGainDial[i].setVisible (false);
                eqBandGainValueLabel[i].setVisible (false);
            }
        }

        // Pre-Compressor visibility
        preCompSectionLabel.setVisible (visible);
        preCompBypassButton.setVisible (visible);
        preCompThresholdLabel.setVisible (visible);
        preCompThresholdDial.setVisible (visible);
        preCompThresholdValueLabel.setVisible (visible);
        preCompRatioLabel.setVisible (visible);
        preCompRatioDial.setVisible (visible);
        preCompRatioValueLabel.setVisible (visible);
        preCompAttackLabel.setVisible (visible);
        preCompAttackDial.setVisible (visible);
        preCompAttackValueLabel.setVisible (visible);
        preCompReleaseLabel.setVisible (visible);
        preCompReleaseDial.setVisible (visible);
        preCompReleaseValueLabel.setVisible (visible);

        if (visible)
            updatePreCompAppearance();
    }

    void updateEQBandAppearance (int bandIndex)
    {
        bool eqEnabled = eqEnableButton.getToggleState();
        int shapeId = eqBandShapeSelector[bandIndex].getSelectedId();
        bool bandIsOff = !eqBandToggle[bandIndex].getToggleState();

        // Determine if this is a cut or bandpass filter (no gain control)
        // Reverb EQ shapes: 1=LowCut, 2=LowShelf, 3=Peak, 4=HighShelf, 5=HighCut, 6=BandPass
        bool isCutOrBandPass = (shapeId == 1 || shapeId == 5 || shapeId == 6);
        bool showGain = !isCutOrBandPass;

        // Grey out entire band if global EQ is off
        // Grey out band parameters (except shape) if band is off but EQ is on
        float globalAlpha = eqEnabled ? 1.0f : 0.4f;
        float bandLabelAlpha = globalAlpha;
        float toggleAlpha = globalAlpha;
        float shapeAlpha = (eqEnabled && !bandIsOff) ? 1.0f : 0.4f;
        float paramAlpha = (eqEnabled && !bandIsOff) ? 1.0f : 0.4f;

        // Band label, toggle, and reset follow global EQ state
        eqBandLabel[bandIndex].setAlpha (bandLabelAlpha);
        eqBandToggle[bandIndex].setAlpha (toggleAlpha);
        eqBandResetButton[bandIndex].setAlpha (globalAlpha);
        eqBandShapeSelector[bandIndex].setAlpha (shapeAlpha);

        // Only update visibility if EQ tab is currently selected
        bool eqTabSelected = (subTabBar.getCurrentTabIndex() == 1);

        // Parameters follow both global EQ and band off state
        if (eqTabSelected)
        {
            eqBandFreqLabel[bandIndex].setVisible (true);
            eqBandFreqSlider[bandIndex].setVisible (true);
            eqBandFreqValueLabel[bandIndex].setVisible (true);
        }
        eqBandFreqLabel[bandIndex].setAlpha (paramAlpha);
        eqBandFreqSlider[bandIndex].setAlpha (paramAlpha);
        eqBandFreqValueLabel[bandIndex].setAlpha (paramAlpha);

        if (eqTabSelected)
        {
            eqBandQLabel[bandIndex].setVisible (true);
            eqBandQDial[bandIndex].setVisible (true);
            eqBandQValueLabel[bandIndex].setVisible (true);
        }
        eqBandQLabel[bandIndex].setAlpha (paramAlpha);
        eqBandQDial[bandIndex].setAlpha (paramAlpha);
        eqBandQValueLabel[bandIndex].setAlpha (paramAlpha);

        // Gain controls - hide for cut/bandpass filters, only show if EQ tab selected
        bool showGainVisible = showGain && eqTabSelected;
        eqBandGainLabel[bandIndex].setVisible (showGainVisible);
        eqBandGainDial[bandIndex].setVisible (showGainVisible);
        eqBandGainValueLabel[bandIndex].setVisible (showGainVisible);
        if (showGain)
        {
            eqBandGainLabel[bandIndex].setAlpha (paramAlpha);
            eqBandGainDial[bandIndex].setAlpha (paramAlpha);
            eqBandGainValueLabel[bandIndex].setAlpha (paramAlpha);
        }
    }

    void resetPreEQBand (int i)
    {
        using namespace WFSParameterDefaults;
        isLoadingParameters = true;
        int defaultShape = reverbPreEQBandShapes[i];
        int defaultFreq = reverbPreEQBandFrequencies[i];
        eqBandToggle[i].setToggleState (defaultShape != 0, juce::dontSendNotification);
        eqBandShapeSelector[i].setSelectedId (reverbPreEQBandComboDefaults[i], juce::dontSendNotification);
        float freqSlider = std::log10 (defaultFreq / 20.0f) / 3.0f;
        eqBandFreqSlider[i].setValue (juce::jlimit (0.0f, 1.0f, freqSlider));
        eqBandFreqValueLabel[i].setText (formatFrequency (defaultFreq), juce::dontSendNotification);
        eqBandGainDial[i].setValue (0.5f);
        eqBandGainValueLabel[i].setText ("0.0 dB", juce::dontSendNotification);
        float qSlider = std::log ((reverbPreEQqDefault - 0.1f) / 0.21f + 1.0f) / std::log (100.0f);
        eqBandQDial[i].setValue (juce::jlimit (0.0f, 1.0f, qSlider));
        eqBandQValueLabel[i].setText ("0.70", juce::dontSendNotification);
        isLoadingParameters = false;
        saveEQBandParam (i, WFSParameterIDs::reverbPreEQshape, defaultShape);
        saveEQBandParam (i, WFSParameterIDs::reverbPreEQfreq, defaultFreq);
        saveEQBandParam (i, WFSParameterIDs::reverbPreEQgain, 0.0f);
        saveEQBandParam (i, WFSParameterIDs::reverbPreEQq, reverbPreEQqDefault);
        updateEQBandAppearance (i);
    }

    void setAlgorithmVisible (bool visible)
    {
        algoSDNButton.setVisible (visible);
        algoFDNButton.setVisible (visible);
        algoIRButton.setVisible (visible);

        // Always-visible controls
        algoWetLevelLabel.setVisible (visible);
        algoWetLevelSlider.setVisible (visible);
        algoWetLevelValueLabel.setVisible (visible);

        if (visible)
        {
            updateAlgorithmVisibility();
        }
        else
        {
            // Hide all algorithm-specific controls
            algoDecaySectionLabel.setVisible (false);
            algoRT60Label.setVisible (false);
            algoRT60Slider.setVisible (false);
            algoRT60ValueLabel.setVisible (false);
            algoRT60LowMultLabel.setVisible (false);
            algoRT60LowMultSlider.setVisible (false);
            algoRT60LowMultValueLabel.setVisible (false);
            algoRT60HighMultLabel.setVisible (false);
            algoRT60HighMultSlider.setVisible (false);
            algoRT60HighMultValueLabel.setVisible (false);
            algoCrossoverLowLabel.setVisible (false);
            algoCrossoverLowSlider.setVisible (false);
            algoCrossoverLowValueLabel.setVisible (false);
            algoCrossoverHighLabel.setVisible (false);
            algoCrossoverHighSlider.setVisible (false);
            algoCrossoverHighValueLabel.setVisible (false);
            algoDiffusionLabel.setVisible (false);
            algoDiffusionSlider.setVisible (false);
            algoDiffusionValueLabel.setVisible (false);

            algoSDNSectionLabel.setVisible (false);
            algoSDNScaleLabel.setVisible (false);
            algoSDNScaleSlider.setVisible (false);
            algoSDNScaleValueLabel.setVisible (false);

            algoFDNSectionLabel.setVisible (false);
            algoFDNSizeLabel.setVisible (false);
            algoFDNSizeSlider.setVisible (false);
            algoFDNSizeValueLabel.setVisible (false);

            algoIRSectionLabel.setVisible (false);
            algoIRFileLabel.setVisible (false);
            algoIRFileSelector.setVisible (false);
            algoIRTrimLabel.setVisible (false);
            algoIRTrimSlider.setVisible (false);
            algoIRTrimValueLabel.setVisible (false);
            algoIRLengthLabel.setVisible (false);
            algoIRLengthSlider.setVisible (false);
            algoIRLengthValueLabel.setVisible (false);
            algoPerNodeButton.setVisible (false);
        }
    }

    void setPostProcessingVisible (bool visible)
    {
        postEqEnableButton.setVisible (visible);
        postEqFlattenButton.setVisible (visible);

        // Post-EQ Display - create if needed and visible
        if (visible && postEqDisplay == nullptr)
        {
            auto postEqTree = parameters.getValueTreeState().ensureReverbPostEQSection();
            if (postEqTree.isValid())
            {
                postEqDisplay = std::make_unique<EQDisplayComponent> (postEqTree, numPostEqBands, EQDisplayConfig::forReverbPostEQ());
                addAndMakeVisible (*postEqDisplay);
                postEqDisplay->setUndoManager (parameters.getUndoManagerForDomain (UndoDomain::Reverb));

                int eqEnabled = postEqEnableButton.getToggleState() ? 1 : 0;
                postEqDisplay->setEQEnabled (eqEnabled != 0);
            }
        }
        if (postEqDisplay)
            postEqDisplay->setVisible (visible);

        for (int i = 0; i < numPostEqBands; ++i)
        {
            postEqBandLabel[i].setVisible (visible);
            postEqBandToggle[i].setVisible (visible);
            postEqBandShapeSelector[i].setVisible (visible);
            postEqBandResetButton[i].setVisible (visible);
            postEqBandFreqLabel[i].setVisible (visible);
            postEqBandFreqSlider[i].setVisible (visible);
            postEqBandFreqValueLabel[i].setVisible (visible);
            postEqBandQLabel[i].setVisible (visible);
            postEqBandQDial[i].setVisible (visible);
            postEqBandQValueLabel[i].setVisible (visible);

            if (visible)
                updatePostEQBandAppearance (i);
            else
            {
                postEqBandGainLabel[i].setVisible (false);
                postEqBandGainDial[i].setVisible (false);
                postEqBandGainValueLabel[i].setVisible (false);
            }
        }

        // Post-Expander visibility
        postExpSectionLabel.setVisible (visible);
        postExpBypassButton.setVisible (visible);
        postExpThresholdLabel.setVisible (visible);
        postExpThresholdDial.setVisible (visible);
        postExpThresholdValueLabel.setVisible (visible);
        postExpRatioLabel.setVisible (visible);
        postExpRatioDial.setVisible (visible);
        postExpRatioValueLabel.setVisible (visible);
        postExpAttackLabel.setVisible (visible);
        postExpAttackDial.setVisible (visible);
        postExpAttackValueLabel.setVisible (visible);
        postExpReleaseLabel.setVisible (visible);
        postExpReleaseDial.setVisible (visible);
        postExpReleaseValueLabel.setVisible (visible);

        if (visible)
            updatePostExpAppearance();
    }

    void setChannelParametersVisible (bool visible)
    {
        // Column title labels
        reverbFeedTitleLabel.setVisible (visible);
        reverbReturnTitleLabel.setVisible (visible);

        // Reverb components
        attenuationLabel.setVisible (visible);
        attenuationSlider.setVisible (visible);
        attenuationValueLabel.setVisible (visible);
        delayLatencyLabel.setVisible (visible);
        delayLatencySlider.setVisible (visible);
        delayLatencyValueLabel.setVisible (visible);

        // Position components
        coordModeLabel.setVisible (visible);
        coordModeSelector.setVisible (visible);
        posXLabel.setVisible (visible); posYLabel.setVisible (visible); posZLabel.setVisible (visible);
        posXEditor.setVisible (visible); posYEditor.setVisible (visible); posZEditor.setVisible (visible);
        posXUnitLabel.setVisible (visible); posYUnitLabel.setVisible (visible); posZUnitLabel.setVisible (visible);
        returnOffsetXLabel.setVisible (visible); returnOffsetYLabel.setVisible (visible); returnOffsetZLabel.setVisible (visible);
        returnOffsetXEditor.setVisible (visible); returnOffsetYEditor.setVisible (visible); returnOffsetZEditor.setVisible (visible);
        returnOffsetXUnitLabel.setVisible (visible); returnOffsetYUnitLabel.setVisible (visible); returnOffsetZUnitLabel.setVisible (visible);

        // Reverb Feed components
        orientationLabel.setVisible (visible);
        directionalDial.setVisible (visible);
        orientationValueLabel.setVisible (visible);
        orientationUnitLabel.setVisible (visible);
        angleOnLabel.setVisible (visible); angleOnSlider.setVisible (visible); angleOnValueLabel.setVisible (visible);
        angleOffLabel.setVisible (visible); angleOffSlider.setVisible (visible); angleOffValueLabel.setVisible (visible);
        pitchLabel.setVisible (visible); pitchSlider.setVisible (visible); pitchValueLabel.setVisible (visible);
        hfDampingLabel.setVisible (visible); hfDampingSlider.setVisible (visible); hfDampingValueLabel.setVisible (visible);
        miniLatencyEnableButton.setVisible (visible);
        lsEnableButton.setVisible (visible);
        distanceAttenEnableLabel.setVisible (visible);
        distanceAttenEnableSlider.setVisible (visible);
        distanceAttenEnableValueLabel.setVisible (visible);

        // Reverb Return components
        distanceAttenLabel.setVisible (visible);
        distanceAttenDial.setVisible (visible);
        distanceAttenValueLabel.setVisible (visible);
        distanceAttenUnitLabel.setVisible (visible);
        commonAttenLabel.setVisible (visible);
        commonAttenDial.setVisible (visible);
        commonAttenValueLabel.setVisible (visible);
        commonAttenUnitLabel.setVisible (visible);
        mutesLabel.setVisible (visible);
        muteMacrosLabel.setVisible (visible);
        muteMacrosSelector.setVisible (visible);
        for (int i = 0; i < maxMuteButtons; ++i)
            muteButtons[i].setVisible (visible && i < parameters.getNumOutputChannels());
    }

    //==========================================================================
    // Coordinate Mode Handling
    //==========================================================================

    void updatePositionLabelsAndValues()
    {
        // Get current coordinate mode
        int mode = static_cast<int>(parameters.getReverbParam(currentChannel - 1, "reverbCoordinateMode"));
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
        helpTextMap[&posXEditor] = LOC("reverbs.help.position1").replace("{name}", n1).replace("{unit}", unit1);
        helpTextMap[&posYEditor] = LOC("reverbs.help.position2").replace("{name}", n2).replace("{unit}", unit2);
        helpTextMap[&posZEditor] = LOC("reverbs.help.position3").replace("{name}", n3).replace("{unit}", unit3);
        helpTextMap[&returnOffsetXEditor] = LOC("reverbs.help.returnOffset1").replace("{name}", n1).replace("{unit}", unit1);
        helpTextMap[&returnOffsetYEditor] = LOC("reverbs.help.returnOffset2").replace("{name}", n2).replace("{unit}", unit2);
        helpTextMap[&returnOffsetZEditor] = LOC("reverbs.help.returnOffset3").replace("{name}", n3).replace("{unit}", unit3);

        // Get Cartesian values from storage
        float x = static_cast<float>(parameters.getReverbParam(currentChannel - 1, "reverbPositionX"));
        float y = static_cast<float>(parameters.getReverbParam(currentChannel - 1, "reverbPositionY"));
        float z = static_cast<float>(parameters.getReverbParam(currentChannel - 1, "reverbPositionZ"));

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

    //==========================================================================
    // Parameter Methods
    //==========================================================================

    void loadChannelParameters (int channel)
    {
        isLoadingParameters = true;
        currentChannel = channel;

        auto getParam = [this] (const juce::Identifier& id) -> juce::var
        {
            return parameters.getReverbParam (currentChannel - 1, id.toString());
        };

        auto getFloatParam = [&getParam] (const juce::Identifier& id, float defaultVal = 0.0f) -> float
        {
            auto val = getParam (id);
            return val.isVoid() ? defaultVal : static_cast<float> (val);
        };

        auto getIntParam = [&getParam] (const juce::Identifier& id, int defaultVal = 0) -> int
        {
            auto val = getParam (id);
            return val.isVoid() ? defaultVal : static_cast<int> (val);
        };

        // Name
        juce::String name = getParam (WFSParameterIDs::reverbName).toString();
        if (name.isEmpty())
            name = "Reverb " + juce::String (channel);
        nameEditor.setText (name, juce::dontSendNotification);

        // Attenuation
        float attenDB = getFloatParam (WFSParameterIDs::reverbAttenuation, 0.0f);
        attenDB = juce::jlimit (-92.0f, 0.0f, attenDB);
        float minLinear = std::pow (10.0f, -92.0f / 20.0f);
        float targetLinear = std::pow (10.0f, attenDB / 20.0f);
        float attenSliderVal = std::sqrt ((targetLinear - minLinear) / (1.0f - minLinear));
        attenuationSlider.setValue (juce::jlimit (0.0f, 1.0f, attenSliderVal));
        attenuationValueLabel.setText (juce::String (attenDB, 1) + " dB", juce::dontSendNotification);

        // Delay/Latency
        float delayMs = getFloatParam (WFSParameterIDs::reverbDelayLatency, 0.0f);
        delayLatencySlider.setValue (delayMs / 100.0f);  // v = ms / 100 (bidirectional -1 to 1)
        delayLatencyValueLabel.setText (juce::String (delayMs, 1) + " ms", juce::dontSendNotification);

        // Position - update coordinate mode selector and position editors (handles coordinate conversion)
        updatePositionLabelsAndValues();

        // Return Offset
        returnOffsetXEditor.setText (juce::String (getFloatParam (WFSParameterIDs::reverbReturnOffsetX, 0.0f), 2), juce::dontSendNotification);
        returnOffsetYEditor.setText (juce::String (getFloatParam (WFSParameterIDs::reverbReturnOffsetY, 0.0f), 2), juce::dontSendNotification);
        returnOffsetZEditor.setText (juce::String (getFloatParam (WFSParameterIDs::reverbReturnOffsetZ, 0.0f), 2), juce::dontSendNotification);

        // Reverb Feed
        int orientation = getIntParam (WFSParameterIDs::reverbOrientation, 0);
        directionalDial.setOrientation (static_cast<float> (orientation));
        orientationValueLabel.setText (juce::String (orientation), juce::dontSendNotification);

        int angleOn = getIntParam (WFSParameterIDs::reverbAngleOn, 86);
        angleOnSlider.setValue ((angleOn - 1.0f) / 179.0f);
        angleOnValueLabel.setText (juce::String (angleOn) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
        directionalDial.setAngleOn (angleOn);

        int angleOff = getIntParam (WFSParameterIDs::reverbAngleOff, 90);
        angleOffSlider.setValue (angleOff / 179.0f);
        angleOffValueLabel.setText (juce::String (angleOff) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
        directionalDial.setAngleOff (angleOff);

        int pitch = getIntParam (WFSParameterIDs::reverbPitch, 0);
        pitchSlider.setValue (pitch / 90.0f);  // v = pitch / 90 (bidirectional -1 to 1)
        pitchValueLabel.setText (juce::String (pitch) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);

        float hfDamping = getFloatParam (WFSParameterIDs::reverbHFdamping, 0.0f);
        hfDampingSlider.setValue ((hfDamping + 6.0f) / 6.0f);
        hfDampingValueLabel.setText (juce::String (hfDamping, 1) + " dB/m", juce::dontSendNotification);

        int miniLatency = getIntParam (WFSParameterIDs::reverbMiniLatencyEnable, 1);
        miniLatencyEnableButton.setToggleState (miniLatency != 0, juce::dontSendNotification);
        miniLatencyEnableButton.setButtonText (miniLatency != 0 ? LOC("reverbs.toggles.minLatencyOn") : LOC("reverbs.toggles.minLatencyOff"));
        {
            juce::Colour btnColour = miniLatency != 0 ? juce::Colour (0xFFD4A017) : juce::Colour (0xFF2D2D2D);
            miniLatencyEnableButton.setColour (juce::TextButton::buttonColourId, btnColour);
            miniLatencyEnableButton.setColour (juce::TextButton::buttonOnColourId, btnColour);
        }

        int lsEnable = getIntParam (WFSParameterIDs::reverbLSenable, 1);
        lsEnableButton.setToggleState (lsEnable != 0, juce::dontSendNotification);
        lsEnableButton.setButtonText (lsEnable != 0 ? LOC("reverbs.toggles.liveSourceOn") : LOC("reverbs.toggles.liveSourceOff"));
        {
            juce::Colour btnColour = lsEnable != 0 ? juce::Colour (0xFF4A90D9) : juce::Colour (0xFF2D2D2D);
            lsEnableButton.setColour (juce::TextButton::buttonColourId, btnColour);
            lsEnableButton.setColour (juce::TextButton::buttonOnColourId, btnColour);
        }

        int distanceAttenEnable = getIntParam (WFSParameterIDs::reverbDistanceAttenEnable, 100);
        distanceAttenEnableSlider.setValue ((distanceAttenEnable - 100.0f) / 100.0f);  // v = (percent - 100) / 100 (bidirectional -1 to 1)
        distanceAttenEnableValueLabel.setText (juce::String (distanceAttenEnable) + "%", juce::dontSendNotification);

        // EQ
        int eqEnabled = getIntParam (WFSParameterIDs::reverbPreEQenable, 1);
        eqEnableButton.setToggleState (eqEnabled != 0, juce::dontSendNotification);
        eqEnableButton.setButtonText (eqEnabled != 0 ? LOC("eq.status.on") : LOC("eq.status.off"));
        eqEnableButton.setColour (juce::TextButton::buttonColourId,
                                  eqEnabled != 0 ? juce::Colour (0xFF4CAF50) : juce::Colour (0xFF2D2D2D));

        loadEQBandParameters();

        // Create EQ display component only if channel changed or doesn't exist
        // This prevents destroying the component mid-drag when ValueTree changes trigger reload
        // Use ensureReverbEQSection to create EQ section if missing (e.g., old config files)
        auto eqTree = parameters.getValueTreeState().ensureReverbEQSection (channel - 1);
        if (eqTree.isValid())
        {
            if (eqDisplay == nullptr || lastEqDisplayChannel != channel)
            {
                eqDisplay = std::make_unique<EQDisplayComponent> (eqTree, numEqBands, EQDisplayConfig::forReverbPreEQ());
                addAndMakeVisible (*eqDisplay);
                eqDisplay->setUndoManager (parameters.getUndoManagerForDomain (UndoDomain::Reverb));
                lastEqDisplayChannel = channel;
            }
            // Update EQ display enabled state
            eqDisplay->setEQEnabled (eqEnabled != 0);
            // Update visibility based on current tab
            bool eqTabVisible = (subTabBar.getCurrentTabIndex() == 1);  // EQ is sub-tab index 1
            eqDisplay->setVisible (eqTabVisible);
            if (eqTabVisible)
                layoutEQSubTab();
        }

        // Reverb Return
        float distanceAtten = getFloatParam (WFSParameterIDs::reverbDistanceAttenuation, -0.7f);
        distanceAttenDial.setValue ((distanceAtten + 6.0f) / 6.0f);
        distanceAttenValueLabel.setText (juce::String (distanceAtten, 1), juce::dontSendNotification);

        int commonAtten = getIntParam (WFSParameterIDs::reverbCommonAtten, 100);
        commonAttenDial.setValue (commonAtten / 100.0f);
        commonAttenValueLabel.setText (juce::String (commonAtten), juce::dontSendNotification);

        loadMuteStates();

        isLoadingParameters = false;
    }

    void loadEQBandParameters()
    {
        auto& vts = parameters.getValueTreeState();
        auto eqSection = vts.getReverbEQSection (currentChannel - 1);

        for (int i = 0; i < numEqBands; ++i)
        {
            auto band = vts.getReverbEQBand (currentChannel - 1, i);
            if (!band.isValid())
                continue;

            int shape = band.getProperty (WFSParameterIDs::reverbPreEQshape, 0);
            bool bandOn = (shape != 0);
            eqBandToggle[i].setToggleState (bandOn, juce::dontSendNotification);
            // Only update combobox when band is on (preserve user's selection when off)
            if (bandOn)
                eqBandShapeSelector[i].setSelectedId (shape, juce::dontSendNotification);

            int freq = band.getProperty (WFSParameterIDs::reverbPreEQfreq, 1000);
            float freqSlider = std::log10 (freq / 20.0f) / 3.0f;
            eqBandFreqSlider[i].setValue (juce::jlimit (0.0f, 1.0f, freqSlider));
            eqBandFreqValueLabel[i].setText (formatFrequency (freq), juce::dontSendNotification);

            float gain = band.getProperty (WFSParameterIDs::reverbPreEQgain, 0.0f);
            eqBandGainDial[i].setValue ((gain + 24.0f) / 48.0f);
            eqBandGainValueLabel[i].setText (juce::String (gain, 1) + " dB", juce::dontSendNotification);

            float q = band.getProperty (WFSParameterIDs::reverbPreEQq, 0.7f);
            float qSlider = std::log ((q - 0.1f) / 0.21f + 1.0f) / std::log (100.0f);
            eqBandQDial[i].setValue (juce::jlimit (0.0f, 1.0f, qSlider));
            eqBandQValueLabel[i].setText (juce::String (q, 2), juce::dontSendNotification);

            // Update band appearance based on EQ state and filter shape
            updateEQBandAppearance (i);
        }
    }

    void loadMuteStates()
    {
        auto& vts = parameters.getValueTreeState();
        auto returnSection = vts.getReverbReturnSection (currentChannel - 1);
        if (!returnSection.isValid())
            return;

        juce::String mutesStr = returnSection.getProperty (WFSParameterIDs::reverbMutes).toString();
        juce::StringArray muteValues;
        muteValues.addTokens (mutesStr, ",", "");

        for (int i = 0; i < maxMuteButtons && i < muteValues.size(); ++i)
            muteButtons[i].setToggleState (muteValues[i].getIntValue() != 0, juce::dontSendNotification);
    }

    void saveReverbParam (const juce::Identifier& paramId, const juce::var& value)
    {
        if (isLoadingParameters) return;

        auto& vts = parameters.getValueTreeState();
        int channelIndex = currentChannel - 1;
        juce::ValueTree section;

        // Map parameter IDs to their specific sections for reliable access
        // Channel section parameters
        if (paramId == WFSParameterIDs::reverbName ||
            paramId == WFSParameterIDs::reverbAttenuation ||
            paramId == WFSParameterIDs::reverbDelayLatency)
        {
            section = vts.getReverbChannelSection (channelIndex);
        }
        // Position section parameters
        else if (paramId == WFSParameterIDs::reverbPositionX ||
                 paramId == WFSParameterIDs::reverbPositionY ||
                 paramId == WFSParameterIDs::reverbPositionZ ||
                 paramId == WFSParameterIDs::reverbReturnOffsetX ||
                 paramId == WFSParameterIDs::reverbReturnOffsetY ||
                 paramId == WFSParameterIDs::reverbReturnOffsetZ ||
                 paramId == WFSParameterIDs::reverbCoordinateMode)
        {
            section = vts.getReverbPositionSection (channelIndex);
        }
        // Feed section parameters
        else if (paramId == WFSParameterIDs::reverbOrientation ||
                 paramId == WFSParameterIDs::reverbAngleOn ||
                 paramId == WFSParameterIDs::reverbAngleOff ||
                 paramId == WFSParameterIDs::reverbPitch ||
                 paramId == WFSParameterIDs::reverbHFdamping ||
                 paramId == WFSParameterIDs::reverbMiniLatencyEnable ||
                 paramId == WFSParameterIDs::reverbLSenable ||
                 paramId == WFSParameterIDs::reverbDistanceAttenEnable)
        {
            section = vts.getReverbFeedSection (channelIndex);
        }
        // EQ section parameter (EQ enable toggle)
        else if (paramId == WFSParameterIDs::reverbPreEQenable)
        {
            section = vts.getReverbEQSection (channelIndex);
        }
        // ReverbReturn section parameters
        else if (paramId == WFSParameterIDs::reverbDistanceAttenuation ||
                 paramId == WFSParameterIDs::reverbCommonAtten ||
                 paramId == WFSParameterIDs::reverbMutes ||
                 paramId == WFSParameterIDs::reverbMuteMacro)
        {
            section = vts.getReverbReturnSection (channelIndex);
        }

        if (section.isValid())
            section.setProperty (paramId, value, vts.getUndoManager());
    }

    void saveEQBandParam (int bandIndex, const juce::Identifier& paramId, const juce::var& value)
    {
        if (isLoadingParameters) return;

        auto& vts = parameters.getValueTreeState();
        auto band = vts.getReverbEQBand (currentChannel - 1, bandIndex);
        if (band.isValid())
            band.setProperty (paramId, value, vts.getUndoManager());
    }

    void saveAlgorithmParam (const juce::Identifier& paramId, const juce::var& value)
    {
        if (isLoadingParameters) return;

        auto& vts = parameters.getValueTreeState();
        auto section = vts.ensureReverbAlgorithmSection();
        if (section.isValid())
            section.setProperty (paramId, value, vts.getUndoManager());
    }

    void savePostEQParam (const juce::Identifier& paramId, const juce::var& value)
    {
        if (isLoadingParameters) return;

        auto& vts = parameters.getValueTreeState();
        auto section = vts.ensureReverbPostEQSection();
        if (section.isValid())
            section.setProperty (paramId, value, vts.getUndoManager());
    }

    void savePostEQBandParam (int bandIndex, const juce::Identifier& paramId, const juce::var& value)
    {
        if (isLoadingParameters) return;

        auto& vts = parameters.getValueTreeState();
        auto band = vts.getReverbPostEQBand (bandIndex);
        if (band.isValid())
            band.setProperty (paramId, value, vts.getUndoManager());
    }

    void updatePostEQBandAppearance (int bandIndex)
    {
        bool eqEnabled = postEqEnableButton.getToggleState();
        int shapeId = postEqBandShapeSelector[bandIndex].getSelectedId();
        bool bandIsOff = !postEqBandToggle[bandIndex].getToggleState();

        // Reverb Post-EQ shapes: 1=LowCut, 2=LowShelf, 3=Peak, 4=HighShelf, 5=HighCut, 6=BandPass
        bool isCutOrBandPass = (shapeId == 1 || shapeId == 5 || shapeId == 6);
        bool showGain = !isCutOrBandPass;

        float globalAlpha = eqEnabled ? 1.0f : 0.4f;
        float bandLabelAlpha = globalAlpha;
        float toggleAlpha = globalAlpha;
        float shapeAlpha = (eqEnabled && !bandIsOff) ? 1.0f : 0.4f;
        float paramAlpha = (eqEnabled && !bandIsOff) ? 1.0f : 0.4f;

        postEqBandLabel[bandIndex].setAlpha (bandLabelAlpha);
        postEqBandToggle[bandIndex].setAlpha (toggleAlpha);
        postEqBandResetButton[bandIndex].setAlpha (globalAlpha);
        postEqBandShapeSelector[bandIndex].setAlpha (shapeAlpha);

        bool postEqTabSelected = (subTabBar.getCurrentTabIndex() == 3);

        if (postEqTabSelected)
        {
            postEqBandFreqLabel[bandIndex].setVisible (true);
            postEqBandFreqSlider[bandIndex].setVisible (true);
            postEqBandFreqValueLabel[bandIndex].setVisible (true);
        }
        postEqBandFreqLabel[bandIndex].setAlpha (paramAlpha);
        postEqBandFreqSlider[bandIndex].setAlpha (paramAlpha);
        postEqBandFreqValueLabel[bandIndex].setAlpha (paramAlpha);

        if (postEqTabSelected)
        {
            postEqBandQLabel[bandIndex].setVisible (true);
            postEqBandQDial[bandIndex].setVisible (true);
            postEqBandQValueLabel[bandIndex].setVisible (true);
        }
        postEqBandQLabel[bandIndex].setAlpha (paramAlpha);
        postEqBandQDial[bandIndex].setAlpha (paramAlpha);
        postEqBandQValueLabel[bandIndex].setAlpha (paramAlpha);

        bool showGainVisible = showGain && postEqTabSelected;
        postEqBandGainLabel[bandIndex].setVisible (showGainVisible);
        postEqBandGainDial[bandIndex].setVisible (showGainVisible);
        postEqBandGainValueLabel[bandIndex].setVisible (showGainVisible);
        if (showGain)
        {
            postEqBandGainLabel[bandIndex].setAlpha (paramAlpha);
            postEqBandGainDial[bandIndex].setAlpha (paramAlpha);
            postEqBandGainValueLabel[bandIndex].setAlpha (paramAlpha);
        }
    }

    void resetPostEQBand (int i)
    {
        using namespace WFSParameterDefaults;
        isLoadingParameters = true;
        int defaultShape = reverbPostEQBandShapes[i];
        int defaultFreq = reverbPostEQBandFrequencies[i];
        postEqBandToggle[i].setToggleState (defaultShape != 0, juce::dontSendNotification);
        postEqBandShapeSelector[i].setSelectedId (reverbPostEQBandComboDefaults[i], juce::dontSendNotification);
        float freqSlider = std::log10 (defaultFreq / 20.0f) / 3.0f;
        postEqBandFreqSlider[i].setValue (juce::jlimit (0.0f, 1.0f, freqSlider));
        postEqBandFreqValueLabel[i].setText (formatFrequency (defaultFreq), juce::dontSendNotification);
        postEqBandGainDial[i].setValue (0.5f);
        postEqBandGainValueLabel[i].setText ("0.0 dB", juce::dontSendNotification);
        float qSlider = std::log ((reverbPostEQqDefault - 0.1f) / 0.21f + 1.0f) / std::log (100.0f);
        postEqBandQDial[i].setValue (juce::jlimit (0.0f, 1.0f, qSlider));
        postEqBandQValueLabel[i].setText ("0.70", juce::dontSendNotification);
        isLoadingParameters = false;
        savePostEQBandParam (i, WFSParameterIDs::reverbPostEQshape, defaultShape);
        savePostEQBandParam (i, WFSParameterIDs::reverbPostEQfreq, defaultFreq);
        savePostEQBandParam (i, WFSParameterIDs::reverbPostEQgain, 0.0f);
        savePostEQBandParam (i, WFSParameterIDs::reverbPostEQq, reverbPostEQqDefault);
        updatePostEQBandAppearance (i);
    }

    void loadPostEQParameters()
    {
        auto& vts = parameters.getValueTreeState();
        auto postEQ = vts.ensureReverbPostEQSection();
        if (!postEQ.isValid())
            return;

        int eqEnabled = static_cast<int> (postEQ.getProperty (WFSParameterIDs::reverbPostEQenable, 1));
        postEqEnableButton.setToggleState (eqEnabled != 0, juce::dontSendNotification);
        postEqEnableButton.setButtonText (eqEnabled != 0 ? LOC("eq.status.on") : LOC("eq.status.off"));
        postEqEnableButton.setColour (juce::TextButton::buttonColourId,
                                      eqEnabled != 0 ? juce::Colour (0xFF4CAF50) : juce::Colour (0xFF2D2D2D));

        for (int i = 0; i < numPostEqBands; ++i)
        {
            auto band = vts.getReverbPostEQBand (i);
            if (!band.isValid())
                continue;

            int shape = band.getProperty (WFSParameterIDs::reverbPostEQshape, 0);
            bool bandOn = (shape != 0);
            postEqBandToggle[i].setToggleState (bandOn, juce::dontSendNotification);
            // Only update combobox when band is on (preserve user's selection when off)
            if (bandOn)
                postEqBandShapeSelector[i].setSelectedId (shape, juce::dontSendNotification);

            int freq = band.getProperty (WFSParameterIDs::reverbPostEQfreq, 1000);
            float freqSlider = std::log10 (freq / 20.0f) / 3.0f;
            postEqBandFreqSlider[i].setValue (juce::jlimit (0.0f, 1.0f, freqSlider));
            postEqBandFreqValueLabel[i].setText (formatFrequency (freq), juce::dontSendNotification);

            float gain = band.getProperty (WFSParameterIDs::reverbPostEQgain, 0.0f);
            postEqBandGainDial[i].setValue ((gain + 24.0f) / 48.0f);
            postEqBandGainValueLabel[i].setText (juce::String (gain, 1) + " dB", juce::dontSendNotification);

            float q = band.getProperty (WFSParameterIDs::reverbPostEQq, 0.7f);
            float qSlider = std::log ((q - 0.1f) / 0.21f + 1.0f) / std::log (100.0f);
            postEqBandQDial[i].setValue (juce::jlimit (0.0f, 1.0f, qSlider));
            postEqBandQValueLabel[i].setText (juce::String (q, 2), juce::dontSendNotification);

            updatePostEQBandAppearance (i);
        }

        // Update Post-EQ display
        if (postEqDisplay != nullptr)
        {
            postEqDisplay->setEQEnabled (eqEnabled != 0);
            bool postEqTabVisible = (subTabBar.getCurrentTabIndex() == 3);
            postEqDisplay->setVisible (postEqTabVisible);
            if (postEqTabVisible)
                layoutPostProcessingSubTab();
        }
    }

    // =========================================================================
    // Pre-Compressor save/load/appearance
    // =========================================================================

    void savePreCompParam (const juce::Identifier& paramId, const juce::var& value)
    {
        if (isLoadingParameters) return;
        auto& vts = parameters.getValueTreeState();
        auto preComp = vts.ensureReverbPreCompSection();
        if (preComp.isValid())
            preComp.setProperty (paramId, value, vts.getUndoManager());
    }

    void updatePreCompAppearance()
    {
        bool bypassed = preCompBypassButton.getToggleState();
        float alpha = bypassed ? 0.4f : 1.0f;
        preCompThresholdLabel.setAlpha (alpha);
        preCompThresholdDial.setAlpha (alpha);
        preCompThresholdValueLabel.setAlpha (alpha);
        preCompRatioLabel.setAlpha (alpha);
        preCompRatioDial.setAlpha (alpha);
        preCompRatioValueLabel.setAlpha (alpha);
        preCompAttackLabel.setAlpha (alpha);
        preCompAttackDial.setAlpha (alpha);
        preCompAttackValueLabel.setAlpha (alpha);
        preCompReleaseLabel.setAlpha (alpha);
        preCompReleaseDial.setAlpha (alpha);
        preCompReleaseValueLabel.setAlpha (alpha);
    }

    void loadPreCompParameters()
    {
        using namespace WFSParameterIDs;
        using namespace WFSParameterDefaults;
        auto& vts = parameters.getValueTreeState();
        auto preComp = vts.ensureReverbPreCompSection();
        if (!preComp.isValid()) return;

        int bypassed = static_cast<int> (preComp.getProperty (reverbPreCompBypass, reverbPreCompBypassDefault));
        preCompBypassButton.setToggleState (bypassed != 0, juce::dontSendNotification);
        preCompBypassButton.setButtonText (bypassed != 0 ? LOC("reverbs.preProcessing.compressorOff") : LOC("reverbs.preProcessing.compressorOn"));
        preCompBypassButton.setColour (juce::TextButton::buttonColourId,
            bypassed != 0 ? juce::Colour (0xFF2D2D2D) : juce::Colour (0xFF4CAF50));
        preCompBypassButton.setColour (juce::TextButton::buttonOnColourId,
            bypassed != 0 ? juce::Colour (0xFF2D2D2D) : juce::Colour (0xFF4CAF50));

        float threshold = static_cast<float> (static_cast<double> (preComp.getProperty (reverbPreCompThreshold, reverbPreCompThresholdDefault)));
        float thresholdNorm = (threshold - reverbPreCompThresholdMin) / (reverbPreCompThresholdMax - reverbPreCompThresholdMin);
        preCompThresholdDial.setValue (juce::jlimit (0.0f, 1.0f, thresholdNorm));
        preCompThresholdValueLabel.setText (juce::String (threshold, 1) + " dB", juce::dontSendNotification);

        float ratio = static_cast<float> (static_cast<double> (preComp.getProperty (reverbPreCompRatio, reverbPreCompRatioDefault)));
        float ratioNorm = (ratio - reverbPreCompRatioMin) / (reverbPreCompRatioMax - reverbPreCompRatioMin);
        preCompRatioDial.setValue (juce::jlimit (0.0f, 1.0f, ratioNorm));
        preCompRatioValueLabel.setText (juce::String (ratio, 1) + ":1", juce::dontSendNotification);

        float attack = static_cast<float> (static_cast<double> (preComp.getProperty (reverbPreCompAttack, reverbPreCompAttackDefault)));
        float attackNorm = std::log (attack / reverbPreCompAttackMin) / std::log (reverbPreCompAttackMax / reverbPreCompAttackMin);
        preCompAttackDial.setValue (juce::jlimit (0.0f, 1.0f, attackNorm));
        preCompAttackValueLabel.setText (juce::String (attack, 1) + " ms", juce::dontSendNotification);

        float release = static_cast<float> (static_cast<double> (preComp.getProperty (reverbPreCompRelease, reverbPreCompReleaseDefault)));
        float releaseNorm = std::log (release / reverbPreCompReleaseMin) / std::log (reverbPreCompReleaseMax / reverbPreCompReleaseMin);
        preCompReleaseDial.setValue (juce::jlimit (0.0f, 1.0f, releaseNorm));
        preCompReleaseValueLabel.setText (juce::String (release, 0) + " ms", juce::dontSendNotification);

        updatePreCompAppearance();
    }

    // =========================================================================
    // Post-Expander save/load/appearance
    // =========================================================================

    void savePostExpParam (const juce::Identifier& paramId, const juce::var& value)
    {
        if (isLoadingParameters) return;
        auto& vts = parameters.getValueTreeState();
        auto postExp = vts.ensureReverbPostExpSection();
        if (postExp.isValid())
            postExp.setProperty (paramId, value, vts.getUndoManager());
    }

    void updatePostExpAppearance()
    {
        bool bypassed = postExpBypassButton.getToggleState();
        float alpha = bypassed ? 0.4f : 1.0f;
        postExpThresholdLabel.setAlpha (alpha);
        postExpThresholdDial.setAlpha (alpha);
        postExpThresholdValueLabel.setAlpha (alpha);
        postExpRatioLabel.setAlpha (alpha);
        postExpRatioDial.setAlpha (alpha);
        postExpRatioValueLabel.setAlpha (alpha);
        postExpAttackLabel.setAlpha (alpha);
        postExpAttackDial.setAlpha (alpha);
        postExpAttackValueLabel.setAlpha (alpha);
        postExpReleaseLabel.setAlpha (alpha);
        postExpReleaseDial.setAlpha (alpha);
        postExpReleaseValueLabel.setAlpha (alpha);
    }

    void loadPostExpParameters()
    {
        using namespace WFSParameterIDs;
        using namespace WFSParameterDefaults;
        auto& vts = parameters.getValueTreeState();
        auto postExp = vts.ensureReverbPostExpSection();
        if (!postExp.isValid()) return;

        int bypassed = static_cast<int> (postExp.getProperty (reverbPostExpBypass, reverbPostExpBypassDefault));
        postExpBypassButton.setToggleState (bypassed != 0, juce::dontSendNotification);
        postExpBypassButton.setButtonText (bypassed != 0 ? LOC("reverbs.postProcessing.expanderOff") : LOC("reverbs.postProcessing.expanderOn"));
        postExpBypassButton.setColour (juce::TextButton::buttonColourId,
            bypassed != 0 ? juce::Colour (0xFF2D2D2D) : juce::Colour (0xFF4CAF50));
        postExpBypassButton.setColour (juce::TextButton::buttonOnColourId,
            bypassed != 0 ? juce::Colour (0xFF2D2D2D) : juce::Colour (0xFF4CAF50));

        float threshold = static_cast<float> (static_cast<double> (postExp.getProperty (reverbPostExpThreshold, reverbPostExpThresholdDefault)));
        float thresholdNorm = (threshold - reverbPostExpThresholdMin) / (reverbPostExpThresholdMax - reverbPostExpThresholdMin);
        postExpThresholdDial.setValue (juce::jlimit (0.0f, 1.0f, thresholdNorm));
        postExpThresholdValueLabel.setText (juce::String (threshold, 1) + " dB", juce::dontSendNotification);

        float ratio = static_cast<float> (static_cast<double> (postExp.getProperty (reverbPostExpRatio, reverbPostExpRatioDefault)));
        float ratioNorm = (ratio - reverbPostExpRatioMin) / (reverbPostExpRatioMax - reverbPostExpRatioMin);
        postExpRatioDial.setValue (juce::jlimit (0.0f, 1.0f, ratioNorm));
        postExpRatioValueLabel.setText ("1:" + juce::String (ratio, 1), juce::dontSendNotification);

        float attack = static_cast<float> (static_cast<double> (postExp.getProperty (reverbPostExpAttack, reverbPostExpAttackDefault)));
        float attackNorm = std::log (attack / reverbPostExpAttackMin) / std::log (reverbPostExpAttackMax / reverbPostExpAttackMin);
        postExpAttackDial.setValue (juce::jlimit (0.0f, 1.0f, attackNorm));
        postExpAttackValueLabel.setText (juce::String (attack, 1) + " ms", juce::dontSendNotification);

        float release = static_cast<float> (static_cast<double> (postExp.getProperty (reverbPostExpRelease, reverbPostExpReleaseDefault)));
        float releaseNorm = std::log (release / reverbPostExpReleaseMin) / std::log (reverbPostExpReleaseMax / reverbPostExpReleaseMin);
        postExpReleaseDial.setValue (juce::jlimit (0.0f, 1.0f, releaseNorm));
        postExpReleaseValueLabel.setText (juce::String (release, 0) + " ms", juce::dontSendNotification);

        updatePostExpAppearance();
    }

    void selectAlgorithm (int type)
    {
        algoSDNButton.setToggleState (type == 0, juce::dontSendNotification);
        algoFDNButton.setToggleState (type == 1, juce::dontSendNotification);
        algoIRButton.setToggleState (type == 2, juce::dontSendNotification);
        updateAlgorithmButtonColors();
        saveAlgorithmParam (WFSParameterIDs::reverbAlgoType, type);
        updateAlgorithmVisibility();
        resized();
    }

    void updateAlgorithmButtonColors()
    {
        auto updateBtnColor = [] (juce::TextButton& btn)
        {
            juce::Colour col = btn.getToggleState() ? juce::Colour (0xFF4CAF50) : juce::Colour (0xFF2D2D2D);
            btn.setColour (juce::TextButton::buttonColourId, col);
            btn.setColour (juce::TextButton::buttonOnColourId, col);
        };
        updateBtnColor (algoSDNButton);
        updateBtnColor (algoFDNButton);
        updateBtnColor (algoIRButton);
    }

    void updateAlgorithmVisibility()
    {
        int algoType = 0;
        if (algoFDNButton.getToggleState()) algoType = 1;
        else if (algoIRButton.getToggleState()) algoType = 2;

        bool isSDN = (algoType == 0);
        bool isFDN = (algoType == 1);
        bool isIR  = (algoType == 2);
        bool isSDNorFDN = isSDN || isFDN;

        // Decay section visible for SDN and FDN
        algoDecaySectionLabel.setVisible (isSDNorFDN);
        algoRT60Label.setVisible (isSDNorFDN);
        algoRT60Slider.setVisible (isSDNorFDN);
        algoRT60ValueLabel.setVisible (isSDNorFDN);
        algoRT60LowMultLabel.setVisible (isSDNorFDN);
        algoRT60LowMultSlider.setVisible (isSDNorFDN);
        algoRT60LowMultValueLabel.setVisible (isSDNorFDN);
        algoRT60HighMultLabel.setVisible (isSDNorFDN);
        algoRT60HighMultSlider.setVisible (isSDNorFDN);
        algoRT60HighMultValueLabel.setVisible (isSDNorFDN);
        algoCrossoverLowLabel.setVisible (isSDNorFDN);
        algoCrossoverLowSlider.setVisible (isSDNorFDN);
        algoCrossoverLowValueLabel.setVisible (isSDNorFDN);
        algoCrossoverHighLabel.setVisible (isSDNorFDN);
        algoCrossoverHighSlider.setVisible (isSDNorFDN);
        algoCrossoverHighValueLabel.setVisible (isSDNorFDN);
        algoDiffusionLabel.setVisible (isSDNorFDN);
        algoDiffusionSlider.setVisible (isSDNorFDN);
        algoDiffusionValueLabel.setVisible (isSDNorFDN);

        // SDN section
        algoSDNSectionLabel.setVisible (isSDN);
        algoSDNScaleLabel.setVisible (isSDN);
        algoSDNScaleSlider.setVisible (isSDN);
        algoSDNScaleValueLabel.setVisible (isSDN);

        // FDN section
        algoFDNSectionLabel.setVisible (isFDN);
        algoFDNSizeLabel.setVisible (isFDN);
        algoFDNSizeSlider.setVisible (isFDN);
        algoFDNSizeValueLabel.setVisible (isFDN);

        // IR section
        algoIRSectionLabel.setVisible (isIR);
        algoIRFileLabel.setVisible (isIR);
        algoIRFileSelector.setVisible (isIR);
        algoIRTrimLabel.setVisible (isIR);
        algoIRTrimSlider.setVisible (isIR);
        algoIRTrimValueLabel.setVisible (isIR);
        algoIRLengthLabel.setVisible (isIR);
        algoIRLengthSlider.setVisible (isIR);
        algoIRLengthValueLabel.setVisible (isIR);
        algoPerNodeButton.setVisible (isIR);
    }

    void loadAlgorithmParameters()
    {
        isLoadingParameters = true;

        auto& vts = parameters.getValueTreeState();
        auto section = vts.ensureReverbAlgorithmSection();

        if (!section.isValid())
        {
            isLoadingParameters = false;
            return;
        }

        auto getFloat = [&section] (const juce::Identifier& id, float defaultVal) -> float
        {
            auto val = section.getProperty (id);
            return val.isVoid() ? defaultVal : static_cast<float> (val);
        };

        auto getInt = [&section] (const juce::Identifier& id, int defaultVal) -> int
        {
            auto val = section.getProperty (id);
            return val.isVoid() ? defaultVal : static_cast<int> (val);
        };

        // Algorithm type
        int algoType = getInt (WFSParameterIDs::reverbAlgoType, WFSParameterDefaults::reverbAlgoTypeDefault);
        algoSDNButton.setToggleState (algoType == 0, juce::dontSendNotification);
        algoFDNButton.setToggleState (algoType == 1, juce::dontSendNotification);
        algoIRButton.setToggleState (algoType == 2, juce::dontSendNotification);
        updateAlgorithmButtonColors();

        // RT60
        float rt60 = getFloat (WFSParameterIDs::reverbRT60, WFSParameterDefaults::reverbRT60Default);
        float rt60Slider = std::log (rt60 / WFSParameterDefaults::reverbRT60Min)
            / std::log (WFSParameterDefaults::reverbRT60Max / WFSParameterDefaults::reverbRT60Min);
        algoRT60Slider.setValue (juce::jlimit (0.0f, 1.0f, rt60Slider));
        algoRT60ValueLabel.setText (juce::String (rt60, 2) + " s", juce::dontSendNotification);

        // RT60 Low Mult
        float rt60Low = getFloat (WFSParameterIDs::reverbRT60LowMult, WFSParameterDefaults::reverbRT60LowMultDefault);
        float rt60LowSlider = std::log (rt60Low / WFSParameterDefaults::reverbRT60LowMultMin)
            / std::log (WFSParameterDefaults::reverbRT60LowMultMax / WFSParameterDefaults::reverbRT60LowMultMin);
        algoRT60LowMultSlider.setValue (juce::jlimit (0.0f, 1.0f, rt60LowSlider));
        algoRT60LowMultValueLabel.setText (juce::String (rt60Low, 2) + "x", juce::dontSendNotification);

        // RT60 High Mult
        float rt60High = getFloat (WFSParameterIDs::reverbRT60HighMult, WFSParameterDefaults::reverbRT60HighMultDefault);
        float rt60HighSlider = std::log (rt60High / WFSParameterDefaults::reverbRT60HighMultMin)
            / std::log (WFSParameterDefaults::reverbRT60HighMultMax / WFSParameterDefaults::reverbRT60HighMultMin);
        algoRT60HighMultSlider.setValue (juce::jlimit (0.0f, 1.0f, rt60HighSlider));
        algoRT60HighMultValueLabel.setText (juce::String (rt60High, 2) + "x", juce::dontSendNotification);

        // Crossover Low
        float xoverLow = getFloat (WFSParameterIDs::reverbCrossoverLow, WFSParameterDefaults::reverbCrossoverLowDefault);
        float xoverLowSlider = std::log (xoverLow / WFSParameterDefaults::reverbCrossoverLowMin)
            / std::log (WFSParameterDefaults::reverbCrossoverLowMax / WFSParameterDefaults::reverbCrossoverLowMin);
        algoCrossoverLowSlider.setValue (juce::jlimit (0.0f, 1.0f, xoverLowSlider));
        algoCrossoverLowValueLabel.setText (formatFrequency (static_cast<int> (xoverLow)), juce::dontSendNotification);

        // Crossover High
        float xoverHigh = getFloat (WFSParameterIDs::reverbCrossoverHigh, WFSParameterDefaults::reverbCrossoverHighDefault);
        float xoverHighSlider = std::log (xoverHigh / WFSParameterDefaults::reverbCrossoverHighMin)
            / std::log (WFSParameterDefaults::reverbCrossoverHighMax / WFSParameterDefaults::reverbCrossoverHighMin);
        algoCrossoverHighSlider.setValue (juce::jlimit (0.0f, 1.0f, xoverHighSlider));
        algoCrossoverHighValueLabel.setText (formatFrequency (static_cast<int> (xoverHigh)), juce::dontSendNotification);

        // Diffusion
        float diffusion = getFloat (WFSParameterIDs::reverbDiffusion, WFSParameterDefaults::reverbDiffusionDefault);
        algoDiffusionSlider.setValue (juce::jlimit (0.0f, 1.0f, diffusion));
        algoDiffusionValueLabel.setText (juce::String (static_cast<int> (diffusion * 100)) + "%", juce::dontSendNotification);

        // SDN Scale
        float sdnScale = getFloat (WFSParameterIDs::reverbSDNscale, WFSParameterDefaults::reverbSDNscaleDefault);
        float sdnScaleSlider = (sdnScale - WFSParameterDefaults::reverbSDNscaleMin)
            / (WFSParameterDefaults::reverbSDNscaleMax - WFSParameterDefaults::reverbSDNscaleMin);
        algoSDNScaleSlider.setValue (juce::jlimit (0.0f, 1.0f, sdnScaleSlider));
        algoSDNScaleValueLabel.setText (juce::String (sdnScale, 2) + "x", juce::dontSendNotification);

        // FDN Size
        float fdnSize = getFloat (WFSParameterIDs::reverbFDNsize, WFSParameterDefaults::reverbFDNsizeDefault);
        float fdnSizeSlider = (fdnSize - WFSParameterDefaults::reverbFDNsizeMin)
            / (WFSParameterDefaults::reverbFDNsizeMax - WFSParameterDefaults::reverbFDNsizeMin);
        algoFDNSizeSlider.setValue (juce::jlimit (0.0f, 1.0f, fdnSizeSlider));
        algoFDNSizeValueLabel.setText (juce::String (fdnSize, 2) + "x", juce::dontSendNotification);

        // IR file selector
        refreshIRFileList();

        // IR Trim
        float irTrim = getFloat (WFSParameterIDs::reverbIRtrim, WFSParameterDefaults::reverbIRtrimDefault);
        algoIRTrimSlider.setValue (juce::jlimit (0.0f, 1.0f, irTrim / WFSParameterDefaults::reverbIRtrimMax));
        algoIRTrimValueLabel.setText (juce::String (irTrim, 1) + " ms", juce::dontSendNotification);

        // IR Length
        float irLength = getFloat (WFSParameterIDs::reverbIRlength, WFSParameterDefaults::reverbIRlengthDefault);
        float irLengthSlider = (irLength - WFSParameterDefaults::reverbIRlengthMin)
            / (WFSParameterDefaults::reverbIRlengthMax - WFSParameterDefaults::reverbIRlengthMin);
        algoIRLengthSlider.setValue (juce::jlimit (0.0f, 1.0f, irLengthSlider));
        algoIRLengthValueLabel.setText (juce::String (irLength, 1) + " s", juce::dontSendNotification);

        // Per-node IR
        int perNode = getInt (WFSParameterIDs::reverbPerNodeIR, WFSParameterDefaults::reverbPerNodeIRDefault);
        algoPerNodeButton.setToggleState (perNode != 0, juce::dontSendNotification);
        algoPerNodeButton.setButtonText (perNode != 0 ? LOC("reverbs.algorithm.perNodeOn") : LOC("reverbs.algorithm.perNodeOff"));
        juce::Colour perNodeColour = perNode != 0 ? juce::Colour (0xFF4CAF50) : juce::Colour (0xFF2D2D2D);
        algoPerNodeButton.setColour (juce::TextButton::buttonColourId, perNodeColour);
        algoPerNodeButton.setColour (juce::TextButton::buttonOnColourId, perNodeColour);

        // Wet Level
        float wetLevel = getFloat (WFSParameterIDs::reverbWetLevel, WFSParameterDefaults::reverbWetLevelDefault);
        float wetSlider = (wetLevel + 60.0f) / 72.0f;
        algoWetLevelSlider.setValue (juce::jlimit (0.0f, 1.0f, wetSlider));
        algoWetLevelValueLabel.setText (juce::String (wetLevel, 1) + " dB", juce::dontSendNotification);

        updateAlgorithmVisibility();
        isLoadingParameters = false;
    }

    void refreshIRFileList()
    {
        auto& fileManager = parameters.getFileManager();

        // Remember current selection from ValueTree
        auto& vts = parameters.getValueTreeState();
        auto section = vts.ensureReverbAlgorithmSection();
        juce::String currentIRFile = section.isValid()
            ? section.getProperty (WFSParameterIDs::reverbIRfile).toString()
            : juce::String();

        // Rebuild the ComboBox
        algoIRFileSelector.clear (juce::dontSendNotification);

        // Item 1: "No IR loaded"
        algoIRFileSelector.addItem (LOC("reverbs.algorithm.noFileLoaded"), 1);

        // Scan IR folder for audio files (only if project folder is set)
        int itemId = 2;
        int selectedId = 1;  // Default to "No IR loaded"

        if (fileManager.hasValidProjectFolder())
        {
            auto irFolder = fileManager.getIRFolder();
            juce::StringArray irFiles;

            if (irFolder.isDirectory())
            {
                for (const auto& entry : juce::RangedDirectoryIterator (irFolder, false, "*.wav;*.aif;*.aiff;*.flac", juce::File::findFiles))
                    irFiles.add (entry.getFile().getFileName());

                irFiles.sort (true);

                for (const auto& fileName : irFiles)
                {
                    algoIRFileSelector.addItem (fileName, itemId);
                    if (fileName == currentIRFile)
                        selectedId = itemId;
                    itemId++;
                }
            }
        }

        // Separator + "Import IR..."
        algoIRFileSelector.addSeparator();
        algoIRFileSelector.addItem (LOC("reverbs.algorithm.irImport"), itemId);

        algoIRFileSelector.setSelectedId (selectedId, juce::dontSendNotification);
    }

    void handleIRFileSelection()
    {
        if (isLoadingParameters) return;

        int selectedId = algoIRFileSelector.getSelectedId();
        juce::String selectedText = algoIRFileSelector.getText();

        // Check if "Import IR..." was selected (last item, after separator)
        int numItems = algoIRFileSelector.getNumItems();
        if (selectedId == algoIRFileSelector.getItemId (numItems - 1))
        {
            importIRFile();
            return;
        }

        // "No IR loaded" (id == 1)
        if (selectedId == 1)
        {
            saveAlgorithmParam (WFSParameterIDs::reverbIRfile, juce::String());
            return;
        }

        // An existing IR file was selected
        saveAlgorithmParam (WFSParameterIDs::reverbIRfile, selectedText);
    }

    void importIRFile()
    {
        auto& fileManager = parameters.getFileManager();
        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage (LOC("reverbs.algorithm.irNoProject"));
            refreshIRFileList();  // Reset combobox selection
            return;
        }

        irFileChooser = std::make_shared<juce::FileChooser> (
            LOC("reverbs.algorithm.irImport"),
            juce::File::getSpecialLocation (juce::File::userHomeDirectory),
            "*.wav;*.aif;*.aiff;*.flac");

        irFileChooser->launchAsync (
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                auto result = fc.getResult();
                if (!result.existsAsFile())
                {
                    refreshIRFileList();  // Reset combobox selection
                    return;
                }

                auto& fm = parameters.getFileManager();
                auto irFolder = fm.getIRFolder();
                irFolder.createDirectory();

                auto destFile = irFolder.getChildFile (result.getFileName());

                // If file already exists, just select it
                if (!destFile.existsAsFile())
                    result.copyFileTo (destFile);

                // Save the filename and refresh
                saveAlgorithmParam (WFSParameterIDs::reverbIRfile, destFile.getFileName());
                refreshIRFileList();
                showStatusMessage (LOC("reverbs.algorithm.irImportSuccess")
                    .replace ("{file}", destFile.getFileName()));
            });
    }

    void saveMuteStates()
    {
        if (isLoadingParameters) return;

        int numOutputs = parameters.getNumOutputChannels();
        if (numOutputs <= 0) numOutputs = 16;

        juce::StringArray muteValues;
        for (int i = 0; i < numOutputs; ++i)
            muteValues.add (muteButtons[i].getToggleState() ? "1" : "0");

        auto& vts = parameters.getValueTreeState();
        auto returnSection = vts.getReverbReturnSection (currentChannel - 1);
        if (returnSection.isValid())
            returnSection.setProperty (WFSParameterIDs::reverbMutes, muteValues.joinIntoString (","), vts.getUndoManager());
    }

    void applyMuteMacro (int macroId)
    {
        int numOutputs = parameters.getNumOutputChannels();
        if (numOutputs <= 0) numOutputs = 16;

        switch (macroId)
        {
            case 2:  // MUTE ALL
                for (int i = 0; i < numOutputs; ++i)
                    muteButtons[i].setToggleState (true, juce::dontSendNotification);
                break;

            case 3:  // UNMUTE ALL
                for (int i = 0; i < numOutputs; ++i)
                    muteButtons[i].setToggleState (false, juce::dontSendNotification);
                break;

            case 4:  // INVERT
                for (int i = 0; i < numOutputs; ++i)
                    muteButtons[i].setToggleState (!muteButtons[i].getToggleState(), juce::dontSendNotification);
                break;

            case 5:  // MUTE ODD
                for (int i = 0; i < numOutputs; ++i)
                    muteButtons[i].setToggleState ((i % 2) == 0, juce::dontSendNotification);
                break;

            case 6:  // MUTE EVEN
                for (int i = 0; i < numOutputs; ++i)
                    muteButtons[i].setToggleState ((i % 2) == 1, juce::dontSendNotification);
                break;

            default:
                // Array mute/unmute (macroId 7-26)
                if (macroId >= 7)
                {
                    int arrayIndex = (macroId - 7) / 2;      // 0-9 for Arrays 1-10
                    bool mute = ((macroId - 7) % 2) == 0;    // even=mute, odd=unmute
                    int targetArray = arrayIndex + 1;        // Array number (1-10)

                    auto& vts = parameters.getValueTreeState();
                    for (int i = 0; i < numOutputs; ++i)
                    {
                        juce::var arrayVar = vts.getOutputParameter (i, WFSParameterIDs::outputArray);
                        int outputArrayNum = arrayVar.isInt() ? static_cast<int> (arrayVar) : 0;

                        if (outputArrayNum == targetArray)
                            muteButtons[i].setToggleState (mute, juce::dontSendNotification);
                    }
                }
                break;
        }

        saveMuteStates();
    }

    //==========================================================================
    // File Operations
    //==========================================================================

    void storeReverbConfiguration()
    {
        auto& fileManager = parameters.getFileManager();
        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage (LOC("reverbs.messages.selectFolderFirst"));
            return;
        }
        if (fileManager.saveReverbConfig())
            showStatusMessage (LOC("reverbs.messages.configSaved"));
        else
            showStatusMessage (LOC("reverbs.messages.error").replace("{error}", fileManager.getLastError()));
    }

    void reloadReverbConfiguration()
    {
        auto& fileManager = parameters.getFileManager();
        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage (LOC("reverbs.messages.selectFolderFirst"));
            return;
        }
        if (fileManager.loadReverbConfig())
        {
            loadChannelParameters (currentChannel);
            loadAlgorithmParameters();
            loadPreCompParameters();
            loadPostEQParameters();
            loadPostExpParameters();
            showStatusMessage (LOC("reverbs.messages.configLoaded"));

            // Trigger DSP recalculation via callback to MainComponent
            if (onConfigReloaded)
                onConfigReloaded();
        }
        else
            showStatusMessage (LOC("reverbs.messages.error").replace("{error}", fileManager.getLastError()));
    }

    void reloadReverbConfigBackup()
    {
        auto& fileManager = parameters.getFileManager();
        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage (LOC("reverbs.messages.selectFolderFirst"));
            return;
        }
        if (fileManager.loadReverbConfigBackup (0))
        {
            loadChannelParameters (currentChannel);
            loadAlgorithmParameters();
            loadPreCompParameters();
            loadPostEQParameters();
            loadPostExpParameters();
            showStatusMessage (LOC("reverbs.messages.backupLoaded"));

            // Trigger DSP recalculation via callback to MainComponent
            if (onConfigReloaded)
                onConfigReloaded();
        }
        else
            showStatusMessage (LOC("reverbs.messages.error").replace("{error}", fileManager.getLastError()));
    }

    void importReverbConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser> (LOC("reverbs.dialogs.import"),
            juce::File::getSpecialLocation (juce::File::userHomeDirectory), "*.xml");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser] (const juce::FileChooser& fc)
            {
                auto result = fc.getResult();
                if (result.existsAsFile())
                {
                    auto& fileManager = parameters.getFileManager();
                    if (fileManager.importReverbConfig (result))
                    {
                        loadChannelParameters (currentChannel);
                        loadAlgorithmParameters();
                        loadPreCompParameters();
                        loadPostEQParameters();
                        loadPostExpParameters();
                        showStatusMessage (LOC("reverbs.messages.configImported"));

                        // Trigger DSP recalculation via callback to MainComponent
                        if (onConfigReloaded)
                            onConfigReloaded();
                    }
                    else
                        showStatusMessage (LOC("reverbs.messages.error").replace("{error}", fileManager.getLastError()));
                }
            });
    }

    void exportReverbConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser> (LOC("reverbs.dialogs.export"),
            juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("reverbs.xml"), "*.xml");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser] (const juce::FileChooser& fc)
            {
                auto result = fc.getResult();
                if (result != juce::File())
                {
                    auto& fileManager = parameters.getFileManager();
                    if (fileManager.exportReverbConfig (result))
                        showStatusMessage (LOC("reverbs.messages.configExported"));
                    else
                        showStatusMessage (LOC("reverbs.messages.error").replace("{error}", fileManager.getLastError()));
                }
            });
    }

    //==========================================================================
    // Listener Implementations
    //==========================================================================

    void changeListenerCallback (juce::ChangeBroadcaster*) override
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

    void textEditorReturnKeyPressed (juce::TextEditor& editor) override
    {
        textEditorFocusLost (editor);
        editor.giveAwayKeyboardFocus();
        grabKeyboardFocus();  // Grab focus back so keyboard shortcuts work
    }

    void textEditorEscapeKeyPressed (juce::TextEditor& editor) override
    {
        // Revert to stored value and release focus
        if (&editor == &nameEditor)
            editor.setText(parameters.getReverbParam(currentChannel - 1, "reverbName").toString(), false);
        else if (&editor == &posXEditor)
            editor.setText(juce::String((float)parameters.getReverbParam(currentChannel - 1, "reverbPositionX"), 2), false);
        else if (&editor == &posYEditor)
            editor.setText(juce::String((float)parameters.getReverbParam(currentChannel - 1, "reverbPositionY"), 2), false);
        else if (&editor == &posZEditor)
            editor.setText(juce::String((float)parameters.getReverbParam(currentChannel - 1, "reverbPositionZ"), 2), false);
        else if (&editor == &returnOffsetXEditor)
            editor.setText(juce::String((float)parameters.getReverbParam(currentChannel - 1, "reverbReturnOffsetX"), 2), false);
        else if (&editor == &returnOffsetYEditor)
            editor.setText(juce::String((float)parameters.getReverbParam(currentChannel - 1, "reverbReturnOffsetY"), 2), false);
        else if (&editor == &returnOffsetZEditor)
            editor.setText(juce::String((float)parameters.getReverbParam(currentChannel - 1, "reverbReturnOffsetZ"), 2), false);

        editor.giveAwayKeyboardFocus();
        grabKeyboardFocus();  // Grab focus back so keyboard shortcuts work
    }

    void textEditorFocusLost (juce::TextEditor& editor) override
    {
        if (isLoadingParameters) return;

        if (&editor == &nameEditor)
            saveReverbParam (WFSParameterIDs::reverbName, nameEditor.getText());
        else if (&editor == &posXEditor || &editor == &posYEditor || &editor == &posZEditor)
        {
            // Get all three values from editors
            float v1 = posXEditor.getText().getFloatValue();
            float v2 = posYEditor.getText().getFloatValue();
            float v3 = posZEditor.getText().getFloatValue();

            // Get coordinate mode and convert to Cartesian
            int mode = static_cast<int>(parameters.getReverbParam(currentChannel - 1, "reverbCoordinateMode"));
            auto coordMode = static_cast<WFSCoordinates::Mode>(mode);
            auto cart = WFSCoordinates::displayToCartesian(coordMode, v1, v2, v3);

            // Save Cartesian values
            saveReverbParam(WFSParameterIDs::reverbPositionX, cart.x);
            saveReverbParam(WFSParameterIDs::reverbPositionY, cart.y);
            saveReverbParam(WFSParameterIDs::reverbPositionZ, cart.z);

            // Update display with values (converted back to display coords)
            updatePositionLabelsAndValues();
        }
        else if (&editor == &returnOffsetXEditor)
            saveReverbParam (WFSParameterIDs::reverbReturnOffsetX, editor.getText().getFloatValue());
        else if (&editor == &returnOffsetYEditor)
            saveReverbParam (WFSParameterIDs::reverbReturnOffsetY, editor.getText().getFloatValue());
        else if (&editor == &returnOffsetZEditor)
            saveReverbParam (WFSParameterIDs::reverbReturnOffsetZ, editor.getText().getFloatValue());
    }

    void labelTextChanged (juce::Label* label) override
    {
        if (isLoadingParameters) return;

        juce::String text = label->getText();
        float value = text.retainCharacters ("-0123456789.").getFloatValue();

        if (label == &attenuationValueLabel)
        {
            float dB = juce::jlimit (-92.0f, 0.0f, value);
            float minLinear = std::pow (10.0f, -92.0f / 20.0f);
            float targetLinear = std::pow (10.0f, dB / 20.0f);
            float v = std::sqrt ((targetLinear - minLinear) / (1.0f - minLinear));
            attenuationSlider.setValue (juce::jlimit (0.0f, 1.0f, v));
            // Force label update
            attenuationValueLabel.setText (juce::String (dB, 1) + " dB", juce::dontSendNotification);
        }
        else if (label == &delayLatencyValueLabel)
        {
            float ms = juce::jlimit (-100.0f, 100.0f, value);
            delayLatencySlider.setValue (ms / 100.0f);  // v = ms / 100 (bidirectional -1 to 1)
            // Force label update
            juce::String labelText = (ms < 0) ? "Latency: " : "Delay: ";
            delayLatencyValueLabel.setText (labelText + juce::String (std::abs (ms), 1) + " ms", juce::dontSendNotification);
        }
        else if (label == &orientationValueLabel)
        {
            int degrees = juce::jlimit (-179, 180, static_cast<int> (value));
            directionalDial.setOrientation (static_cast<float> (degrees));
            orientationValueLabel.setText (juce::String (degrees), juce::dontSendNotification);
        }
        else if (label == &angleOnValueLabel)
        {
            int degrees = juce::jlimit (1, 180, static_cast<int> (value));
            angleOnSlider.setValue ((degrees - 1.0f) / 179.0f);
            directionalDial.setAngleOn (degrees);
            angleOnValueLabel.setText (juce::String (degrees) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
        }
        else if (label == &angleOffValueLabel)
        {
            int degrees = juce::jlimit (0, 179, static_cast<int> (value));
            angleOffSlider.setValue (degrees / 179.0f);
            directionalDial.setAngleOff (degrees);
            angleOffValueLabel.setText (juce::String (degrees) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
        }
        else if (label == &pitchValueLabel)
        {
            int degrees = juce::jlimit (-90, 90, static_cast<int> (value));
            pitchSlider.setValue (degrees / 90.0f);
            pitchValueLabel.setText (juce::String (degrees) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
        }
        else if (label == &hfDampingValueLabel)
        {
            float dB = juce::jlimit (-6.0f, 0.0f, value);
            hfDampingSlider.setValue ((dB + 6.0f) / 6.0f);
            hfDampingValueLabel.setText (juce::String (dB, 1) + " dB/m", juce::dontSendNotification);
        }
        else if (label == &distanceAttenEnableValueLabel)
        {
            int percent = juce::jlimit (0, 200, static_cast<int> (value));
            distanceAttenEnableSlider.setValue (percent / 100.0f - 1.0f);  // 0-200% maps to -1 to +1
            distanceAttenEnableValueLabel.setText (juce::String (percent) + "%", juce::dontSendNotification);
        }
        else if (label == &distanceAttenValueLabel)
        {
            float dB = juce::jlimit (-6.0f, 0.0f, value);
            distanceAttenDial.setValue ((dB + 6.0f) / 6.0f);
            distanceAttenValueLabel.setText (juce::String (dB, 1), juce::dontSendNotification);
        }
        else if (label == &commonAttenValueLabel)
        {
            int percent = juce::jlimit (0, 100, static_cast<int> (value));
            commonAttenDial.setValue (percent / 100.0f);
            commonAttenValueLabel.setText (juce::String (percent), juce::dontSendNotification);
        }
        // Pre-EQ band labels
        else
        {
            bool handled = false;
            for (int i = 0; i < numEqBands && !handled; ++i)
            {
                if (label == &eqBandFreqValueLabel[i])
                {
                    int freq = juce::jlimit (20, 20000, static_cast<int> (value));
                    float v = std::log10 (freq / 20.0f) / 3.0f;
                    parameters.getValueTreeState().beginUndoTransaction ("Pre-EQ Band " + juce::String (i + 1) + " Freq");
                    eqBandFreqSlider[i].setValue (juce::jlimit (0.0f, 1.0f, v));
                    eqBandFreqValueLabel[i].setText (formatFrequency (freq), juce::dontSendNotification);
                    handled = true;
                }
                else if (label == &eqBandGainValueLabel[i])
                {
                    float gain = juce::jlimit (-24.0f, 24.0f, value);
                    float v = (gain + 24.0f) / 48.0f;
                    parameters.getValueTreeState().beginUndoTransaction ("Pre-EQ Band " + juce::String (i + 1) + " Gain");
                    eqBandGainDial[i].setValue (juce::jlimit (0.0f, 1.0f, v));
                    eqBandGainValueLabel[i].setText (juce::String (gain, 1) + " dB", juce::dontSendNotification);
                    handled = true;
                }
                else if (label == &eqBandQValueLabel[i])
                {
                    float q = juce::jlimit (0.1f, 20.0f, value);
                    float v = std::log ((q - 0.1f) / 0.21f + 1.0f) / std::log (100.0f);
                    parameters.getValueTreeState().beginUndoTransaction ("Pre-EQ Band " + juce::String (i + 1) + " Q");
                    eqBandQDial[i].setValue (juce::jlimit (0.0f, 1.0f, v));
                    eqBandQValueLabel[i].setText (juce::String (q, 2), juce::dontSendNotification);
                    handled = true;
                }
            }
            // Post-EQ band labels
            for (int i = 0; i < numPostEqBands && !handled; ++i)
            {
                if (label == &postEqBandFreqValueLabel[i])
                {
                    int freq = juce::jlimit (20, 20000, static_cast<int> (value));
                    float v = std::log10 (freq / 20.0f) / 3.0f;
                    parameters.getValueTreeState().beginUndoTransaction ("Post-EQ Band " + juce::String (i + 1) + " Freq");
                    postEqBandFreqSlider[i].setValue (juce::jlimit (0.0f, 1.0f, v));
                    postEqBandFreqValueLabel[i].setText (formatFrequency (freq), juce::dontSendNotification);
                    handled = true;
                }
                else if (label == &postEqBandGainValueLabel[i])
                {
                    float gain = juce::jlimit (-24.0f, 24.0f, value);
                    float v = (gain + 24.0f) / 48.0f;
                    parameters.getValueTreeState().beginUndoTransaction ("Post-EQ Band " + juce::String (i + 1) + " Gain");
                    postEqBandGainDial[i].setValue (juce::jlimit (0.0f, 1.0f, v));
                    postEqBandGainValueLabel[i].setText (juce::String (gain, 1) + " dB", juce::dontSendNotification);
                    handled = true;
                }
                else if (label == &postEqBandQValueLabel[i])
                {
                    float q = juce::jlimit (0.1f, 20.0f, value);
                    float v = std::log ((q - 0.1f) / 0.21f + 1.0f) / std::log (100.0f);
                    parameters.getValueTreeState().beginUndoTransaction ("Post-EQ Band " + juce::String (i + 1) + " Q");
                    postEqBandQDial[i].setValue (juce::jlimit (0.0f, 1.0f, v));
                    postEqBandQValueLabel[i].setText (juce::String (q, 2), juce::dontSendNotification);
                    handled = true;
                }
            }
            if (!handled)
            {
                using namespace WFSParameterDefaults;
                // Pre-Compressor labels
                if (label == &preCompThresholdValueLabel)
                {
                    float threshold = juce::jlimit (reverbPreCompThresholdMin, reverbPreCompThresholdMax, value);
                    float v = (threshold - reverbPreCompThresholdMin) / (reverbPreCompThresholdMax - reverbPreCompThresholdMin);
                    parameters.getValueTreeState().beginUndoTransaction ("Pre-Comp Threshold");
                    preCompThresholdDial.setValue (juce::jlimit (0.0f, 1.0f, v));
                    preCompThresholdValueLabel.setText (juce::String (threshold, 1) + " dB", juce::dontSendNotification);
                }
                else if (label == &preCompRatioValueLabel)
                {
                    float ratio = juce::jlimit (reverbPreCompRatioMin, reverbPreCompRatioMax, value);
                    float v = (ratio - reverbPreCompRatioMin) / (reverbPreCompRatioMax - reverbPreCompRatioMin);
                    parameters.getValueTreeState().beginUndoTransaction ("Pre-Comp Ratio");
                    preCompRatioDial.setValue (juce::jlimit (0.0f, 1.0f, v));
                    preCompRatioValueLabel.setText (juce::String (ratio, 1) + ":1", juce::dontSendNotification);
                }
                else if (label == &preCompAttackValueLabel)
                {
                    float attack = juce::jlimit (reverbPreCompAttackMin, reverbPreCompAttackMax, value);
                    float v = std::log (attack / reverbPreCompAttackMin) / std::log (reverbPreCompAttackMax / reverbPreCompAttackMin);
                    parameters.getValueTreeState().beginUndoTransaction ("Pre-Comp Attack");
                    preCompAttackDial.setValue (juce::jlimit (0.0f, 1.0f, v));
                    preCompAttackValueLabel.setText (juce::String (attack, 1) + " ms", juce::dontSendNotification);
                }
                else if (label == &preCompReleaseValueLabel)
                {
                    float release = juce::jlimit (reverbPreCompReleaseMin, reverbPreCompReleaseMax, value);
                    float v = std::log (release / reverbPreCompReleaseMin) / std::log (reverbPreCompReleaseMax / reverbPreCompReleaseMin);
                    parameters.getValueTreeState().beginUndoTransaction ("Pre-Comp Release");
                    preCompReleaseDial.setValue (juce::jlimit (0.0f, 1.0f, v));
                    preCompReleaseValueLabel.setText (juce::String (release, 0) + " ms", juce::dontSendNotification);
                }
                // Post-Expander labels
                else if (label == &postExpThresholdValueLabel)
                {
                    float threshold = juce::jlimit (reverbPostExpThresholdMin, reverbPostExpThresholdMax, value);
                    float v = (threshold - reverbPostExpThresholdMin) / (reverbPostExpThresholdMax - reverbPostExpThresholdMin);
                    parameters.getValueTreeState().beginUndoTransaction ("Post-Exp Threshold");
                    postExpThresholdDial.setValue (juce::jlimit (0.0f, 1.0f, v));
                    postExpThresholdValueLabel.setText (juce::String (threshold, 1) + " dB", juce::dontSendNotification);
                }
                else if (label == &postExpRatioValueLabel)
                {
                    float ratio = juce::jlimit (reverbPostExpRatioMin, reverbPostExpRatioMax, value);
                    float v = (ratio - reverbPostExpRatioMin) / (reverbPostExpRatioMax - reverbPostExpRatioMin);
                    parameters.getValueTreeState().beginUndoTransaction ("Post-Exp Ratio");
                    postExpRatioDial.setValue (juce::jlimit (0.0f, 1.0f, v));
                    postExpRatioValueLabel.setText ("1:" + juce::String (ratio, 1), juce::dontSendNotification);
                }
                else if (label == &postExpAttackValueLabel)
                {
                    float attack = juce::jlimit (reverbPostExpAttackMin, reverbPostExpAttackMax, value);
                    float v = std::log (attack / reverbPostExpAttackMin) / std::log (reverbPostExpAttackMax / reverbPostExpAttackMin);
                    parameters.getValueTreeState().beginUndoTransaction ("Post-Exp Attack");
                    postExpAttackDial.setValue (juce::jlimit (0.0f, 1.0f, v));
                    postExpAttackValueLabel.setText (juce::String (attack, 1) + " ms", juce::dontSendNotification);
                }
                else if (label == &postExpReleaseValueLabel)
                {
                    float release = juce::jlimit (reverbPostExpReleaseMin, reverbPostExpReleaseMax, value);
                    float v = std::log (release / reverbPostExpReleaseMin) / std::log (reverbPostExpReleaseMax / reverbPostExpReleaseMin);
                    parameters.getValueTreeState().beginUndoTransaction ("Post-Exp Release");
                    postExpReleaseDial.setValue (juce::jlimit (0.0f, 1.0f, v));
                    postExpReleaseValueLabel.setText (juce::String (release, 0) + " ms", juce::dontSendNotification);
                }
                // Algorithm labels
                else if (label == &algoRT60ValueLabel)
                {
                    float rt60 = juce::jlimit (reverbRT60Min, reverbRT60Max, value);
                    float v = std::log (rt60 / reverbRT60Min) / std::log (reverbRT60Max / reverbRT60Min);
                    parameters.getValueTreeState().beginUndoTransaction ("Reverb RT60");
                    algoRT60Slider.setValue (juce::jlimit (0.0f, 1.0f, v));
                    algoRT60ValueLabel.setText (juce::String (rt60, 2) + " s", juce::dontSendNotification);
                }
                else if (label == &algoRT60LowMultValueLabel)
                {
                    float mult = juce::jlimit (reverbRT60LowMultMin, reverbRT60LowMultMax, value);
                    float v = std::log (mult / reverbRT60LowMultMin) / std::log (reverbRT60LowMultMax / reverbRT60LowMultMin);
                    parameters.getValueTreeState().beginUndoTransaction ("Reverb RT60 Low Mult");
                    algoRT60LowMultSlider.setValue (juce::jlimit (0.0f, 1.0f, v));
                    algoRT60LowMultValueLabel.setText (juce::String (mult, 2) + "x", juce::dontSendNotification);
                }
                else if (label == &algoRT60HighMultValueLabel)
                {
                    float mult = juce::jlimit (reverbRT60HighMultMin, reverbRT60HighMultMax, value);
                    float v = std::log (mult / reverbRT60HighMultMin) / std::log (reverbRT60HighMultMax / reverbRT60HighMultMin);
                    parameters.getValueTreeState().beginUndoTransaction ("Reverb RT60 High Mult");
                    algoRT60HighMultSlider.setValue (juce::jlimit (0.0f, 1.0f, v));
                    algoRT60HighMultValueLabel.setText (juce::String (mult, 2) + "x", juce::dontSendNotification);
                }
                else if (label == &algoCrossoverLowValueLabel)
                {
                    float freq = juce::jlimit (reverbCrossoverLowMin, reverbCrossoverLowMax, value);
                    float v = std::log (freq / reverbCrossoverLowMin) / std::log (reverbCrossoverLowMax / reverbCrossoverLowMin);
                    parameters.getValueTreeState().beginUndoTransaction ("Reverb Crossover Low");
                    algoCrossoverLowSlider.setValue (juce::jlimit (0.0f, 1.0f, v));
                    algoCrossoverLowValueLabel.setText (formatFrequency (static_cast<int> (freq)), juce::dontSendNotification);
                }
                else if (label == &algoCrossoverHighValueLabel)
                {
                    float freq = juce::jlimit (reverbCrossoverHighMin, reverbCrossoverHighMax, value);
                    float v = std::log (freq / reverbCrossoverHighMin) / std::log (reverbCrossoverHighMax / reverbCrossoverHighMin);
                    parameters.getValueTreeState().beginUndoTransaction ("Reverb Crossover High");
                    algoCrossoverHighSlider.setValue (juce::jlimit (0.0f, 1.0f, v));
                    algoCrossoverHighValueLabel.setText (formatFrequency (static_cast<int> (freq)), juce::dontSendNotification);
                }
                else if (label == &algoDiffusionValueLabel)
                {
                    int percent = juce::jlimit (0, 100, static_cast<int> (value));
                    float v = percent / 100.0f;
                    parameters.getValueTreeState().beginUndoTransaction ("Reverb Diffusion");
                    algoDiffusionSlider.setValue (v);
                    algoDiffusionValueLabel.setText (juce::String (percent) + "%", juce::dontSendNotification);
                }
                else if (label == &algoSDNScaleValueLabel)
                {
                    float scale = juce::jlimit (reverbSDNscaleMin, reverbSDNscaleMax, value);
                    float v = (scale - reverbSDNscaleMin) / (reverbSDNscaleMax - reverbSDNscaleMin);
                    parameters.getValueTreeState().beginUndoTransaction ("Reverb SDN Scale");
                    algoSDNScaleSlider.setValue (juce::jlimit (0.0f, 1.0f, v));
                    algoSDNScaleValueLabel.setText (juce::String (scale, 2) + "x", juce::dontSendNotification);
                }
                else if (label == &algoFDNSizeValueLabel)
                {
                    float size = juce::jlimit (reverbFDNsizeMin, reverbFDNsizeMax, value);
                    float v = (size - reverbFDNsizeMin) / (reverbFDNsizeMax - reverbFDNsizeMin);
                    parameters.getValueTreeState().beginUndoTransaction ("Reverb FDN Size");
                    algoFDNSizeSlider.setValue (juce::jlimit (0.0f, 1.0f, v));
                    algoFDNSizeValueLabel.setText (juce::String (size, 2) + "x", juce::dontSendNotification);
                }
                else if (label == &algoIRTrimValueLabel)
                {
                    float trim = juce::jlimit (reverbIRtrimMin, reverbIRtrimMax, value);
                    float v = trim / reverbIRtrimMax;
                    parameters.getValueTreeState().beginUndoTransaction ("Reverb IR Trim");
                    algoIRTrimSlider.setValue (juce::jlimit (0.0f, 1.0f, v));
                    algoIRTrimValueLabel.setText (juce::String (trim, 1) + " ms", juce::dontSendNotification);
                }
                else if (label == &algoIRLengthValueLabel)
                {
                    float length = juce::jlimit (reverbIRlengthMin, reverbIRlengthMax, value);
                    float v = (length - reverbIRlengthMin) / (reverbIRlengthMax - reverbIRlengthMin);
                    parameters.getValueTreeState().beginUndoTransaction ("Reverb IR Length");
                    algoIRLengthSlider.setValue (juce::jlimit (0.0f, 1.0f, v));
                    algoIRLengthValueLabel.setText (juce::String (length, 1) + " s", juce::dontSendNotification);
                }
                else if (label == &algoWetLevelValueLabel)
                {
                    float dB = juce::jlimit (-60.0f, 12.0f, value);
                    float v = (dB + 60.0f) / 72.0f;
                    parameters.getValueTreeState().beginUndoTransaction ("Reverb Wet Level");
                    algoWetLevelSlider.setValue (juce::jlimit (0.0f, 1.0f, v));
                    algoWetLevelValueLabel.setText (juce::String (dB, 1) + " dB", juce::dontSendNotification);
                }
            }
        }
    }

    void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override
    {
        // Check if reverb channel count changed (listening to IO tree directly)
        if (tree == ioTree && property == WFSParameterIDs::reverbChannels)
        {
            int numReverbs = parameters.getNumReverbChannels();
            if (numReverbs > 0)
            {
                channelSelector.setNumChannels (numReverbs);
                if (channelSelector.getSelectedChannel() > numReverbs)
                    channelSelector.setSelectedChannel (1);

                // Load parameters for the current channel if we just got channels
                loadChannelParameters (channelSelector.getSelectedChannel());
            }
            updateVisibility();
            resized();  // Re-layout components after visibility change
        }

        // Check if this is a ReverbAlgorithm parameter change (global)
        if (!isLoadingParameters && tree.getType() == WFSParameterIDs::ReverbAlgorithm)
        {
            juce::MessageManager::callAsync ([this]()
            {
                loadAlgorithmParameters();
            });
            return;
        }

        // Check if this is a ReverbPreComp parameter change (global)
        if (!isLoadingParameters && tree.getType() == WFSParameterIDs::ReverbPreComp)
        {
            juce::MessageManager::callAsync ([this]()
            {
                loadPreCompParameters();
            });
            return;
        }

        // Check if this is a ReverbPostEQ parameter change (global)
        if (!isLoadingParameters && (tree.getType() == WFSParameterIDs::ReverbPostEQ ||
                                     tree.getType() == WFSParameterIDs::PostEQBand))
        {
            juce::MessageManager::callAsync ([this]()
            {
                loadPostEQParameters();
            });
            return;
        }

        // Check if this is a ReverbPostExp parameter change (global)
        if (!isLoadingParameters && tree.getType() == WFSParameterIDs::ReverbPostExp)
        {
            juce::MessageManager::callAsync ([this]()
            {
                loadPostExpParameters();
            });
            return;
        }

        // Check if this is a parameter change for the current reverb channel
        if (!isLoadingParameters)
        {
            juce::ValueTree parent = tree;
            while (parent.isValid())
            {
                if (parent.getType() == WFSParameterIDs::Reverb)
                {
                    int channelId = parent.getProperty (WFSParameterIDs::id, -1);
                    if (channelId == currentChannel)
                    {
                        juce::MessageManager::callAsync ([this]()
                        {
                            loadChannelParameters (currentChannel);
                        });
                    }
                    break;
                }
                parent = parent.getParent();
            }
        }
    }

    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override {}
    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override {}
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged (juce::ValueTree&) override {}

    //==========================================================================
    // Mouse Handling
    //==========================================================================

    void mouseEnter (const juce::MouseEvent& event) override
    {
        if (statusBar == nullptr) return;

        // Walk up parent chain to find a registered component (needed for ComboBox children)
        juce::Component* component = event.eventComponent;
        while (component != nullptr)
        {
            if (helpTextMap.find (component) != helpTextMap.end())
            {
                const auto& helpText = helpTextMap[component];
                statusBar->setHelpText (helpText);
                if (oscMethodMap.find (component) != oscMethodMap.end())
                    statusBar->setOscMethod (oscMethodMap[component]);

                // TTS: Announce parameter name and current value for accessibility
                juce::String paramName = TTSManager::extractParameterName (helpText);
                juce::String currentValue = TTSManager::getComponentValue (component);
                TTSManager::getInstance().onComponentEnter (paramName, currentValue, helpText);
                return;
            }
            component = component->getParentComponent();
        }
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        if (statusBar != nullptr)
        {
            statusBar->setHelpText ("");
            statusBar->setOscMethod ("");
        }

        // TTS: Cancel any pending announcements
        TTSManager::getInstance().onComponentExit();
    }

    //==========================================================================
    // Helper Methods
    //==========================================================================

    void showStatusMessage (const juce::String& message)
    {
        if (statusBar != nullptr)
            statusBar->showTemporaryMessage (message, 3000);
    }

    static juce::String formatFrequency (int freq)
    {
        if (freq >= 1000)
            return juce::String (freq / 1000.0f, 1) + " kHz";
        return juce::String (freq) + " Hz";
    }

    void toggleMapVisibility()
    {
        // Toggle global reverb visibility
        auto currentVal = parameters.getConfigParam ("reverbsMapVisible");
        bool currentlyVisible = currentVal.isVoid() || static_cast<int> (currentVal) != 0;
        bool newVisible = !currentlyVisible;

        parameters.setConfigParam ("reverbsMapVisible", newVisible ? 1 : 0);
        updateMapVisibilityButtonState();
    }

    void updateMapVisibilityButtonState()
    {
        auto val = parameters.getConfigParam ("reverbsMapVisible");
        bool visible = val.isVoid() || static_cast<int> (val) != 0;
        mapVisibilityButton.setButtonText (visible ? LOC("reverbs.buttons.visibleOnMap") : LOC("reverbs.buttons.hiddenOnMap"));
    }

    void updateVisibility()
    {
        int numReverbs = parameters.getNumReverbChannels();
        bool hasChannels = (numReverbs > 0);

        // Show/hide the "no channels" message
        noChannelsLabel.setVisible (!hasChannels);

        // Show/hide header controls
        channelSelector.setVisible (hasChannels);
        nameLabel.setVisible (hasChannels);
        nameEditor.setVisible (hasChannels);
        mapVisibilityButton.setVisible (hasChannels);

        // Show/hide sub-tab bar and all sub-tab content
        subTabBar.setVisible (hasChannels);

        // Reverb sub-tab
        attenuationLabel.setVisible (hasChannels);
        attenuationSlider.setVisible (hasChannels);
        attenuationValueLabel.setVisible (hasChannels);
        delayLatencyLabel.setVisible (hasChannels);
        delayLatencySlider.setVisible (hasChannels);
        delayLatencyValueLabel.setVisible (hasChannels);

        // Position sub-tab
        posXLabel.setVisible (hasChannels);
        posYLabel.setVisible (hasChannels);
        posZLabel.setVisible (hasChannels);
        posXEditor.setVisible (hasChannels);
        posYEditor.setVisible (hasChannels);
        posZEditor.setVisible (hasChannels);
        posXUnitLabel.setVisible (hasChannels);
        posYUnitLabel.setVisible (hasChannels);
        posZUnitLabel.setVisible (hasChannels);
        returnOffsetXLabel.setVisible (hasChannels);
        returnOffsetYLabel.setVisible (hasChannels);
        returnOffsetZLabel.setVisible (hasChannels);
        returnOffsetXEditor.setVisible (hasChannels);
        returnOffsetYEditor.setVisible (hasChannels);
        returnOffsetZEditor.setVisible (hasChannels);
        returnOffsetXUnitLabel.setVisible (hasChannels);
        returnOffsetYUnitLabel.setVisible (hasChannels);
        returnOffsetZUnitLabel.setVisible (hasChannels);

        // Reverb Feed sub-tab
        orientationLabel.setVisible (hasChannels);
        directionalDial.setVisible (hasChannels);
        orientationValueLabel.setVisible (hasChannels);
        orientationUnitLabel.setVisible (hasChannels);
        angleOnLabel.setVisible (hasChannels);
        angleOffLabel.setVisible (hasChannels);
        angleOnSlider.setVisible (hasChannels);
        angleOffSlider.setVisible (hasChannels);
        angleOnValueLabel.setVisible (hasChannels);
        angleOffValueLabel.setVisible (hasChannels);
        pitchLabel.setVisible (hasChannels);
        pitchSlider.setVisible (hasChannels);
        pitchValueLabel.setVisible (hasChannels);
        hfDampingLabel.setVisible (hasChannels);
        hfDampingSlider.setVisible (hasChannels);
        hfDampingValueLabel.setVisible (hasChannels);
        miniLatencyEnableButton.setVisible (hasChannels);
        lsEnableButton.setVisible (hasChannels);
        distanceAttenEnableLabel.setVisible (hasChannels);
        distanceAttenEnableSlider.setVisible (hasChannels);
        distanceAttenEnableValueLabel.setVisible (hasChannels);

        // EQ sub-tab
        eqEnableButton.setVisible (hasChannels);
        eqFlattenButton.setVisible (hasChannels);
        for (int i = 0; i < numEqBands; ++i)
        {
            eqBandLabel[i].setVisible (hasChannels);
            eqBandToggle[i].setVisible (hasChannels);
            eqBandShapeSelector[i].setVisible (hasChannels);
            eqBandResetButton[i].setVisible (hasChannels);
            eqBandFreqLabel[i].setVisible (hasChannels);
            eqBandFreqSlider[i].setVisible (hasChannels);
            eqBandFreqValueLabel[i].setVisible (hasChannels);
            eqBandGainLabel[i].setVisible (hasChannels);
            eqBandGainDial[i].setVisible (hasChannels);
            eqBandGainValueLabel[i].setVisible (hasChannels);
            eqBandQLabel[i].setVisible (hasChannels);
            eqBandQDial[i].setVisible (hasChannels);
            eqBandQValueLabel[i].setVisible (hasChannels);
        }

        // Algorithm sub-tab
        if (!hasChannels)
            setAlgorithmVisible (false);

        // Post-Processing sub-tab
        if (!hasChannels)
            setPostProcessingVisible (false);
        postEqEnableButton.setVisible (hasChannels);
        postEqFlattenButton.setVisible (hasChannels);
        for (int i = 0; i < numPostEqBands; ++i)
        {
            postEqBandLabel[i].setVisible (hasChannels);
            postEqBandToggle[i].setVisible (hasChannels);
            postEqBandShapeSelector[i].setVisible (hasChannels);
            postEqBandResetButton[i].setVisible (hasChannels);
            postEqBandFreqLabel[i].setVisible (hasChannels);
            postEqBandFreqSlider[i].setVisible (hasChannels);
            postEqBandFreqValueLabel[i].setVisible (hasChannels);
            postEqBandGainLabel[i].setVisible (hasChannels);
            postEqBandGainDial[i].setVisible (hasChannels);
            postEqBandGainValueLabel[i].setVisible (hasChannels);
            postEqBandQLabel[i].setVisible (hasChannels);
            postEqBandQDial[i].setVisible (hasChannels);
            postEqBandQValueLabel[i].setVisible (hasChannels);
        }

        // Reverb Return sub-tab
        distanceAttenLabel.setVisible (hasChannels);
        distanceAttenDial.setVisible (hasChannels);
        distanceAttenValueLabel.setVisible (hasChannels);
        distanceAttenUnitLabel.setVisible (hasChannels);
        commonAttenLabel.setVisible (hasChannels);
        commonAttenDial.setVisible (hasChannels);
        commonAttenValueLabel.setVisible (hasChannels);
        commonAttenUnitLabel.setVisible (hasChannels);
        mutesLabel.setVisible (hasChannels);
        for (int i = 0; i < maxMuteButtons; ++i)
            muteButtons[i].setVisible (hasChannels && i < parameters.getNumOutputChannels());
        muteMacrosLabel.setVisible (hasChannels);
        muteMacrosSelector.setVisible (hasChannels);

        // Footer buttons remain visible (for Import functionality)

        // After setting base visibility, apply subtab-specific visibility
        // This ensures only the current subtab's components are visible
        if (hasChannels)
            layoutCurrentSubTab();
    }

    //==========================================================================
    // Member Variables
    //==========================================================================

    WfsParameters& parameters;
    juce::ValueTree reverbsTree;
    juce::ValueTree configTree;
    juce::ValueTree ioTree;
    bool isLoadingParameters = false;
    StatusBar* statusBar = nullptr;
    int currentChannel = 1;

    std::map<juce::Component*, juce::String> helpTextMap;
    std::map<juce::Component*, juce::String> oscMethodMap;

    int headerHeight = 60;
    int footerHeight = 50;
    static constexpr int numEqBands = 4;
    static constexpr int maxMuteButtons = 64;
    juce::Rectangle<int> subTabContentArea;
    float layoutScale = 1.0f;  // Proportional scaling factor (1.0 = 1080p reference)
    /** Scale a reference pixel value by layoutScale with a 65% minimum floor */
    int scaled(int ref) const { return juce::jmax(static_cast<int>(ref * 0.65f), static_cast<int>(ref * layoutScale)); }

    // Header
    ChannelSelectorButton channelSelector { "Reverb" };
    juce::Label nameLabel;
    juce::TextEditor nameEditor;
    juce::TextButton mapVisibilityButton;

    // Sub-tab bar
    juce::TabbedButtonBar subTabBar { juce::TabbedButtonBar::TabsAtTop };

    // Reverb sub-tab
    juce::Label attenuationLabel;
    WfsStandardSlider attenuationSlider;
    juce::Label attenuationValueLabel;
    juce::Label delayLatencyLabel;
    WfsBidirectionalSlider delayLatencySlider;
    juce::Label delayLatencyValueLabel;

    // Position sub-tab
    juce::Label coordModeLabel;
    juce::ComboBox coordModeSelector;
    juce::Label posXLabel, posYLabel, posZLabel;
    juce::TextEditor posXEditor, posYEditor, posZEditor;
    juce::Label posXUnitLabel, posYUnitLabel, posZUnitLabel;
    juce::Label returnOffsetXLabel, returnOffsetYLabel, returnOffsetZLabel;
    juce::TextEditor returnOffsetXEditor, returnOffsetYEditor, returnOffsetZEditor;
    juce::Label returnOffsetXUnitLabel, returnOffsetYUnitLabel, returnOffsetZUnitLabel;

    // Column title labels for Channel Parameters tab
    juce::Label reverbFeedTitleLabel;
    juce::Label reverbReturnTitleLabel;

    // Reverb Feed sub-tab
    juce::Label orientationLabel;
    WfsDirectionalDial directionalDial;
    juce::Label orientationValueLabel;
    juce::Label orientationUnitLabel;
    juce::Label angleOnLabel, angleOffLabel;
    WfsWidthExpansionSlider angleOnSlider, angleOffSlider;
    juce::Label angleOnValueLabel, angleOffValueLabel;
    juce::Label pitchLabel;
    WfsBidirectionalSlider pitchSlider;
    juce::Label pitchValueLabel;
    juce::Label hfDampingLabel;
    WfsStandardSlider hfDampingSlider;
    juce::Label hfDampingValueLabel;
    juce::TextButton miniLatencyEnableButton;
    juce::TextButton lsEnableButton;
    juce::Label distanceAttenEnableLabel;
    WfsBidirectionalSlider distanceAttenEnableSlider;
    juce::Label distanceAttenEnableValueLabel;

    // EQ sub-tab
    LongPressButton eqFlattenButton;
    juce::TextButton eqEnableButton;
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

    // Pre-Compressor controls (below Pre-EQ in Pre-Processing tab)
    juce::Label preCompSectionLabel;
    juce::TextButton preCompBypassButton;
    juce::Label preCompThresholdLabel;
    WfsBasicDial preCompThresholdDial;
    juce::Label preCompThresholdValueLabel;
    juce::Label preCompRatioLabel;
    WfsBasicDial preCompRatioDial;
    juce::Label preCompRatioValueLabel;
    juce::Label preCompAttackLabel;
    WfsBasicDial preCompAttackDial;
    juce::Label preCompAttackValueLabel;
    juce::Label preCompReleaseLabel;
    WfsBasicDial preCompReleaseDial;
    juce::Label preCompReleaseValueLabel;

    // Algorithm sub-tab
    juce::TextButton algoSDNButton, algoFDNButton, algoIRButton;

    // Decay section (SDN & FDN)
    juce::Label algoDecaySectionLabel;
    juce::Label algoRT60Label;
    WfsStandardSlider algoRT60Slider;
    juce::Label algoRT60ValueLabel;
    juce::Label algoRT60LowMultLabel;
    WfsStandardSlider algoRT60LowMultSlider;
    juce::Label algoRT60LowMultValueLabel;
    juce::Label algoRT60HighMultLabel;
    WfsStandardSlider algoRT60HighMultSlider;
    juce::Label algoRT60HighMultValueLabel;
    juce::Label algoCrossoverLowLabel;
    WfsStandardSlider algoCrossoverLowSlider;
    juce::Label algoCrossoverLowValueLabel;
    juce::Label algoCrossoverHighLabel;
    WfsStandardSlider algoCrossoverHighSlider;
    juce::Label algoCrossoverHighValueLabel;
    juce::Label algoDiffusionLabel;
    WfsStandardSlider algoDiffusionSlider;
    juce::Label algoDiffusionValueLabel;

    // SDN-specific section
    juce::Label algoSDNSectionLabel;
    juce::Label algoSDNScaleLabel;
    WfsStandardSlider algoSDNScaleSlider;
    juce::Label algoSDNScaleValueLabel;

    // FDN-specific section
    juce::Label algoFDNSectionLabel;
    juce::Label algoFDNSizeLabel;
    WfsStandardSlider algoFDNSizeSlider;
    juce::Label algoFDNSizeValueLabel;

    // IR-specific section
    juce::Label algoIRSectionLabel;
    juce::Label algoIRFileLabel;
    juce::ComboBox algoIRFileSelector;
    std::shared_ptr<juce::FileChooser> irFileChooser;
    juce::Label algoIRTrimLabel;
    WfsStandardSlider algoIRTrimSlider;
    juce::Label algoIRTrimValueLabel;
    juce::Label algoIRLengthLabel;
    WfsStandardSlider algoIRLengthSlider;
    juce::Label algoIRLengthValueLabel;
    juce::TextButton algoPerNodeButton;

    // Output section (always visible)
    juce::Label algoWetLevelLabel;
    WfsStandardSlider algoWetLevelSlider;
    juce::Label algoWetLevelValueLabel;

    // Post-Processing sub-tab
    static constexpr int numPostEqBands = 4;
    LongPressButton postEqFlattenButton;
    juce::TextButton postEqEnableButton;
    juce::Label postEqBandLabel[numPostEqBands];
    EQBandToggle postEqBandToggle[numPostEqBands];
    juce::ComboBox postEqBandShapeSelector[numPostEqBands];
    LongPressButton postEqBandResetButton[numPostEqBands];
    juce::Label postEqBandFreqLabel[numPostEqBands];
    WfsStandardSlider postEqBandFreqSlider[numPostEqBands];
    juce::Label postEqBandFreqValueLabel[numPostEqBands];
    juce::Label postEqBandGainLabel[numPostEqBands];
    WfsBasicDial postEqBandGainDial[numPostEqBands];
    juce::Label postEqBandGainValueLabel[numPostEqBands];
    juce::Label postEqBandQLabel[numPostEqBands];
    WfsBasicDial postEqBandQDial[numPostEqBands];
    juce::Label postEqBandQValueLabel[numPostEqBands];
    std::unique_ptr<EQDisplayComponent> postEqDisplay;

    // Post-Expander controls (below Post-EQ in Post-Processing tab)
    juce::Label postExpSectionLabel;
    juce::TextButton postExpBypassButton;
    juce::Label postExpThresholdLabel;
    WfsBasicDial postExpThresholdDial;
    juce::Label postExpThresholdValueLabel;
    juce::Label postExpRatioLabel;
    WfsBasicDial postExpRatioDial;
    juce::Label postExpRatioValueLabel;
    juce::Label postExpAttackLabel;
    WfsBasicDial postExpAttackDial;
    juce::Label postExpAttackValueLabel;
    juce::Label postExpReleaseLabel;
    WfsBasicDial postExpReleaseDial;
    juce::Label postExpReleaseValueLabel;

    // Reverb Return sub-tab
    juce::Label distanceAttenLabel;
    WfsBasicDial distanceAttenDial;
    juce::Label distanceAttenValueLabel;
    juce::Label distanceAttenUnitLabel;
    juce::Label commonAttenLabel;
    WfsBasicDial commonAttenDial;
    juce::Label commonAttenValueLabel;
    juce::Label commonAttenUnitLabel;
    juce::Label mutesLabel;
    juce::TextButton muteButtons[maxMuteButtons];
    juce::Label muteMacrosLabel;
    juce::ComboBox muteMacrosSelector;

    // Footer buttons
    LongPressButton storeButton;
    LongPressButton reloadButton;
    LongPressButton reloadBackupButton;
    LongPressButton importButton;
    LongPressButton exportButton;

    // No channels message
    juce::Label noChannelsLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbTab)
};
