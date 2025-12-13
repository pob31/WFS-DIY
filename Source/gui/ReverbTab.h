#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "ChannelSelector.h"
#include "SliderUIComponents.h"
#include "DialUIComponents.h"
#include "StatusBar.h"

/**
 * Reverb Tab Component
 * Configuration for reverb channels with 6 sub-tabs
 */
class ReverbTab : public juce::Component,
                  private juce::TextEditor::Listener,
                  private juce::ChangeListener,
                  private juce::Label::Listener,
                  private juce::ValueTree::Listener
{
public:
    ReverbTab (WfsParameters& params)
        : parameters (params),
          reverbsTree (params.getReverbTree()),
          configTree (params.getConfigTree()),
          ioTree (params.getConfigTree().getChildWithName (WFSParameterIDs::IO))
    {
        reverbsTree.addListener (this);
        configTree.addListener (this);
        if (ioTree.isValid())
            ioTree.addListener (this);

        setupHeader();
        setupSubTabs();
        setupReverbSubTab();
        setupPositionSubTab();
        setupReverbFeedSubTab();
        setupEQSubTab();
        setupAlgorithmSubTab();
        setupReverbReturnSubTab();
        setupFooter();
        setupHelpText();
        setupOscMethods();
        setupMouseListeners();

        // Setup "no channels" message
        noChannelsLabel.setText ("No reverb channels configured.\n\nSet the number of Reverb Channels in System Config.",
                                 juce::dontSendNotification);
        noChannelsLabel.setJustificationType (juce::Justification::centred);
        noChannelsLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
        addChildComponent (noChannelsLabel);  // Hidden by default

        int numReverbs = parameters.getNumReverbChannels();
        channelSelector.setNumChannels (numReverbs > 0 ? numReverbs : 1);

        if (numReverbs > 0)
            loadChannelParameters (1);

        updateVisibility();
    }

    ~ReverbTab() override
    {
        reverbsTree.removeListener (this);
        configTree.removeListener (this);
        if (ioTree.isValid())
            ioTree.removeListener (this);
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
        g.fillAll (juce::Colour (0xFF1E1E1E));
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        const int padding = 10;

        // Footer (always visible for Import functionality)
        auto footerArea = bounds.removeFromBottom (footerHeight).reduced (padding, padding);
        layoutFooter (footerArea);

        // Position the "no channels" message in the center of remaining space
        noChannelsLabel.setBounds (bounds.reduced (40));

        // Header
        auto headerArea = bounds.removeFromTop (headerHeight).reduced (padding, padding);
        layoutHeader (headerArea);

        // Sub-tabs area
        auto contentArea = bounds.reduced (padding, 0);
        auto tabBarArea = contentArea.removeFromTop (32);
        subTabBar.setBounds (tabBarArea);

        subTabContentArea = contentArea.reduced (0, padding);
        layoutCurrentSubTab();
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
        nameLabel.setText ("Name:", juce::dontSendNotification);
        nameLabel.setColour (juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible (nameEditor);
        nameEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xFF2D2D2D));
        nameEditor.setColour (juce::TextEditor::textColourId, juce::Colours::white);
        nameEditor.setColour (juce::TextEditor::outlineColourId, juce::Colour (0xFF3D3D3D));
        nameEditor.addListener (this);
    }

    void setupSubTabs()
    {
        addAndMakeVisible (subTabBar);
        subTabBar.addTab ("Reverb", juce::Colour (0xFF2A2A2A), -1);
        subTabBar.addTab ("Position", juce::Colour (0xFF2A2A2A), -1);
        subTabBar.addTab ("Reverb Feed", juce::Colour (0xFF2A2A2A), -1);
        subTabBar.addTab ("EQ", juce::Colour (0xFF2A2A2A), -1);
        subTabBar.addTab ("Algorithm", juce::Colour (0xFF2A2A2A), -1);
        subTabBar.addTab ("Reverb Return", juce::Colour (0xFF2A2A2A), -1);
        subTabBar.addChangeListener (static_cast<juce::ChangeListener*> (this));
    }

    void setupReverbSubTab()
    {
        // Attenuation
        addAndMakeVisible (attenuationLabel);
        attenuationLabel.setText ("Attenuation:", juce::dontSendNotification);
        attenuationLabel.setColour (juce::Label::textColourId, juce::Colours::white);

        attenuationSlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFFFF5722));
        attenuationSlider.onValueChanged = [this] (float v)
        {
            float dB = 20.0f * std::log10 (std::pow (10.0f, -92.0f / 20.0f) +
                       ((1.0f - std::pow (10.0f, -92.0f / 20.0f)) * v * v));
            attenuationValueLabel.setText (juce::String (dB, 1) + " dB", juce::dontSendNotification);
            saveReverbParam (WFSParameterIDs::reverbAttenuation, dB);
        };
        addAndMakeVisible (attenuationSlider);

        addAndMakeVisible (attenuationValueLabel);
        attenuationValueLabel.setText ("0.0 dB", juce::dontSendNotification);
        attenuationValueLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel (attenuationValueLabel);

        // Delay/Latency
        addAndMakeVisible (delayLatencyLabel);
        delayLatencyLabel.setText ("Delay/Latency:", juce::dontSendNotification);
        delayLatencyLabel.setColour (juce::Label::textColourId, juce::Colours::white);

        delayLatencySlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFF4CAF50));
        delayLatencySlider.onValueChanged = [this] (float v)
        {
            float ms = v * 200.0f - 100.0f;  // -100 to +100 ms
            delayLatencyValueLabel.setText (juce::String (ms, 1) + " ms", juce::dontSendNotification);
            saveReverbParam (WFSParameterIDs::reverbDelayLatency, ms);
        };
        addAndMakeVisible (delayLatencySlider);

        addAndMakeVisible (delayLatencyValueLabel);
        delayLatencyValueLabel.setText ("0.0 ms", juce::dontSendNotification);
        delayLatencyValueLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        setupEditableValueLabel (delayLatencyValueLabel);
    }

    void setupPositionSubTab()
    {
        // Position X/Y/Z
        const char* posLabels[] = { "Position X:", "Position Y:", "Position Z:" };
        juce::Label* posLabelPtrs[] = { &posXLabel, &posYLabel, &posZLabel };
        juce::TextEditor* posEditorPtrs[] = { &posXEditor, &posYEditor, &posZEditor };
        juce::Label* posUnitPtrs[] = { &posXUnitLabel, &posYUnitLabel, &posZUnitLabel };

        for (int i = 0; i < 3; ++i)
        {
            addAndMakeVisible (*posLabelPtrs[i]);
            posLabelPtrs[i]->setText (posLabels[i], juce::dontSendNotification);
            posLabelPtrs[i]->setColour (juce::Label::textColourId, juce::Colours::white);

            addAndMakeVisible (*posEditorPtrs[i]);
            posEditorPtrs[i]->setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xFF2D2D2D));
            posEditorPtrs[i]->setColour (juce::TextEditor::textColourId, juce::Colours::white);
            posEditorPtrs[i]->setInputRestrictions (10, "-0123456789.");
            posEditorPtrs[i]->addListener (this);

            addAndMakeVisible (*posUnitPtrs[i]);
            posUnitPtrs[i]->setText ("m", juce::dontSendNotification);
            posUnitPtrs[i]->setColour (juce::Label::textColourId, juce::Colours::grey);
        }

        // Return Offset X/Y/Z
        const char* offsetLabels[] = { "Return Offset X:", "Return Offset Y:", "Return Offset Z:" };
        juce::Label* offsetLabelPtrs[] = { &returnOffsetXLabel, &returnOffsetYLabel, &returnOffsetZLabel };
        juce::TextEditor* offsetEditorPtrs[] = { &returnOffsetXEditor, &returnOffsetYEditor, &returnOffsetZEditor };
        juce::Label* offsetUnitPtrs[] = { &returnOffsetXUnitLabel, &returnOffsetYUnitLabel, &returnOffsetZUnitLabel };

        for (int i = 0; i < 3; ++i)
        {
            addAndMakeVisible (*offsetLabelPtrs[i]);
            offsetLabelPtrs[i]->setText (offsetLabels[i], juce::dontSendNotification);
            offsetLabelPtrs[i]->setColour (juce::Label::textColourId, juce::Colours::white);

            addAndMakeVisible (*offsetEditorPtrs[i]);
            offsetEditorPtrs[i]->setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xFF2D2D2D));
            offsetEditorPtrs[i]->setColour (juce::TextEditor::textColourId, juce::Colours::white);
            offsetEditorPtrs[i]->setInputRestrictions (10, "-0123456789.");
            offsetEditorPtrs[i]->addListener (this);

            addAndMakeVisible (*offsetUnitPtrs[i]);
            offsetUnitPtrs[i]->setText ("m", juce::dontSendNotification);
            offsetUnitPtrs[i]->setColour (juce::Label::textColourId, juce::Colours::grey);
        }
    }

    void setupReverbFeedSubTab()
    {
        // Orientation dial
        addAndMakeVisible (orientationLabel);
        orientationLabel.setText ("Orientation:", juce::dontSendNotification);
        orientationLabel.setColour (juce::Label::textColourId, juce::Colours::white);

        orientationDial.onAngleChanged = [this] (float v)
        {
            int degrees = static_cast<int> (v);
            orientationValueLabel.setText (juce::String (degrees) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
            saveReverbParam (WFSParameterIDs::reverbOrientation, degrees);
        };
        addAndMakeVisible (orientationDial);

        addAndMakeVisible (orientationValueLabel);
        orientationValueLabel.setText (juce::String ("0") + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
        orientationValueLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        orientationValueLabel.setJustificationType (juce::Justification::centred);

        // Angle On slider
        addAndMakeVisible (angleOnLabel);
        angleOnLabel.setText ("Angle On:", juce::dontSendNotification);
        angleOnLabel.setColour (juce::Label::textColourId, juce::Colours::white);

        angleOnSlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFF2196F3));
        angleOnSlider.onValueChanged = [this] (float v)
        {
            int degrees = static_cast<int> (v * 179.0f + 1.0f);  // 1-180
            angleOnValueLabel.setText (juce::String (degrees) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
            saveReverbParam (WFSParameterIDs::reverbAngleOn, degrees);
        };
        addAndMakeVisible (angleOnSlider);

        addAndMakeVisible (angleOnValueLabel);
        angleOnValueLabel.setText ("86" + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
        angleOnValueLabel.setColour (juce::Label::textColourId, juce::Colours::white);

        // Angle Off slider
        addAndMakeVisible (angleOffLabel);
        angleOffLabel.setText ("Angle Off:", juce::dontSendNotification);
        angleOffLabel.setColour (juce::Label::textColourId, juce::Colours::white);

        angleOffSlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFF9C27B0));
        angleOffSlider.onValueChanged = [this] (float v)
        {
            int degrees = static_cast<int> (v * 179.0f);  // 0-179
            angleOffValueLabel.setText (juce::String (degrees) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
            saveReverbParam (WFSParameterIDs::reverbAngleOff, degrees);
        };
        addAndMakeVisible (angleOffSlider);

        addAndMakeVisible (angleOffValueLabel);
        angleOffValueLabel.setText ("90" + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
        angleOffValueLabel.setColour (juce::Label::textColourId, juce::Colours::white);

        // Pitch slider
        addAndMakeVisible (pitchLabel);
        pitchLabel.setText ("Pitch:", juce::dontSendNotification);
        pitchLabel.setColour (juce::Label::textColourId, juce::Colours::white);

        pitchSlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFF00BCD4));
        pitchSlider.onValueChanged = [this] (float v)
        {
            int degrees = static_cast<int> (v * 180.0f - 90.0f);  // -90 to +90
            pitchValueLabel.setText (juce::String (degrees) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
            saveReverbParam (WFSParameterIDs::reverbPitch, degrees);
        };
        addAndMakeVisible (pitchSlider);

        addAndMakeVisible (pitchValueLabel);
        pitchValueLabel.setText ("0" + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
        pitchValueLabel.setColour (juce::Label::textColourId, juce::Colours::white);

        // HF Damping slider
        addAndMakeVisible (hfDampingLabel);
        hfDampingLabel.setText ("HF Damping:", juce::dontSendNotification);
        hfDampingLabel.setColour (juce::Label::textColourId, juce::Colours::white);

        hfDampingSlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFFFF9800));
        hfDampingSlider.onValueChanged = [this] (float v)
        {
            float dB = v * 6.0f - 6.0f;  // -6 to 0 dB/m
            hfDampingValueLabel.setText (juce::String (dB, 1) + " dB/m", juce::dontSendNotification);
            saveReverbParam (WFSParameterIDs::reverbHFdamping, dB);
        };
        addAndMakeVisible (hfDampingSlider);

        addAndMakeVisible (hfDampingValueLabel);
        hfDampingValueLabel.setText ("0.0 dB/m", juce::dontSendNotification);
        hfDampingValueLabel.setColour (juce::Label::textColourId, juce::Colours::white);

        // Toggle buttons
        addAndMakeVisible (miniLatencyEnableButton);
        miniLatencyEnableButton.setButtonText ("MINIMAL LATENCY");
        miniLatencyEnableButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF2D2D2D));
        miniLatencyEnableButton.onClick = [this]
        {
            bool enabled = !miniLatencyEnableButton.getToggleState();
            miniLatencyEnableButton.setToggleState (enabled, juce::dontSendNotification);
            miniLatencyEnableButton.setButtonText (enabled ? "ENABLE" : "DISABLE");
            saveReverbParam (WFSParameterIDs::reverbMiniLatencyEnable, enabled ? 1 : 0);
        };

        addAndMakeVisible (lsEnableButton);
        lsEnableButton.setButtonText ("LIVE SOURCE ATTEN");
        lsEnableButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF2D2D2D));
        lsEnableButton.onClick = [this]
        {
            bool enabled = !lsEnableButton.getToggleState();
            lsEnableButton.setToggleState (enabled, juce::dontSendNotification);
            lsEnableButton.setButtonText (enabled ? "LS ENABLE" : "LS DISABLE");
            saveReverbParam (WFSParameterIDs::reverbLSenable, enabled ? 1 : 0);
        };

        // Distance Attenuation Enable slider
        addAndMakeVisible (distanceAttenEnableLabel);
        distanceAttenEnableLabel.setText ("Distance Atten %:", juce::dontSendNotification);
        distanceAttenEnableLabel.setColour (juce::Label::textColourId, juce::Colours::white);

        distanceAttenEnableSlider.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFF4CAF50));
        distanceAttenEnableSlider.onValueChanged = [this] (float v)
        {
            int percent = static_cast<int> (v * 200.0f);  // 0-200%
            distanceAttenEnableValueLabel.setText (juce::String (percent) + "%", juce::dontSendNotification);
            saveReverbParam (WFSParameterIDs::reverbDistanceAttenEnable, percent);
        };
        addAndMakeVisible (distanceAttenEnableSlider);

        addAndMakeVisible (distanceAttenEnableValueLabel);
        distanceAttenEnableValueLabel.setText ("100%", juce::dontSendNotification);
        distanceAttenEnableValueLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    }

    void setupEQSubTab()
    {
        // EQ Enable button
        addAndMakeVisible (eqEnableButton);
        eqEnableButton.setButtonText ("EQ ON");
        eqEnableButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF4CAF50));
        eqEnableButton.onClick = [this]
        {
            bool enabled = !eqEnableButton.getToggleState();
            eqEnableButton.setToggleState (enabled, juce::dontSendNotification);
            eqEnableButton.setButtonText (enabled ? "EQ ON" : "EQ OFF");
            eqEnableButton.setColour (juce::TextButton::buttonColourId,
                                      enabled ? juce::Colour (0xFF4CAF50) : juce::Colour (0xFF2D2D2D));
            saveReverbParam (WFSParameterIDs::reverbEQenable, enabled ? 1 : 0);
        };

        // 4 EQ bands
        for (int i = 0; i < numEqBands; ++i)
        {
            // Band label
            addAndMakeVisible (eqBandLabel[i]);
            eqBandLabel[i].setText ("Band " + juce::String (i + 1), juce::dontSendNotification);
            eqBandLabel[i].setColour (juce::Label::textColourId, juce::Colours::white);
            eqBandLabel[i].setJustificationType (juce::Justification::centred);

            // Shape selector (no band-pass)
            addAndMakeVisible (eqBandShapeSelector[i]);
            eqBandShapeSelector[i].addItem ("OFF", 1);
            eqBandShapeSelector[i].addItem ("Low Cut", 2);
            eqBandShapeSelector[i].addItem ("Low Shelf", 3);
            eqBandShapeSelector[i].addItem ("Peak/Notch", 4);
            eqBandShapeSelector[i].addItem ("High Shelf", 5);
            eqBandShapeSelector[i].addItem ("High Cut", 6);
            eqBandShapeSelector[i].setSelectedId (1);
            eqBandShapeSelector[i].onChange = [this, i]
            {
                int shape = eqBandShapeSelector[i].getSelectedId() - 1;
                saveEQBandParam (i, WFSParameterIDs::reverbEQshape, shape);
            };

            // Frequency slider
            addAndMakeVisible (eqBandFreqLabel[i]);
            eqBandFreqLabel[i].setText ("Freq:", juce::dontSendNotification);
            eqBandFreqLabel[i].setColour (juce::Label::textColourId, juce::Colours::grey);

            eqBandFreqSlider[i].setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFF2196F3));
            eqBandFreqSlider[i].onValueChanged = [this, i] (float v)
            {
                int freq = static_cast<int> (20.0f * std::pow (10.0f, 3.0f * v));
                eqBandFreqValueLabel[i].setText (formatFrequency (freq), juce::dontSendNotification);
                saveEQBandParam (i, WFSParameterIDs::reverbEQfreq, freq);
            };
            addAndMakeVisible (eqBandFreqSlider[i]);

            addAndMakeVisible (eqBandFreqValueLabel[i]);
            eqBandFreqValueLabel[i].setText ("1000 Hz", juce::dontSendNotification);
            eqBandFreqValueLabel[i].setColour (juce::Label::textColourId, juce::Colours::white);

            // Gain dial
            addAndMakeVisible (eqBandGainLabel[i]);
            eqBandGainLabel[i].setText ("Gain", juce::dontSendNotification);
            eqBandGainLabel[i].setColour (juce::Label::textColourId, juce::Colours::grey);
            eqBandGainLabel[i].setJustificationType (juce::Justification::centred);

            eqBandGainDial[i].onValueChanged = [this, i] (float v)
            {
                float gain = v * 48.0f - 24.0f;  // -24 to +24 dB
                eqBandGainValueLabel[i].setText (juce::String (gain, 1) + " dB", juce::dontSendNotification);
                saveEQBandParam (i, WFSParameterIDs::reverbEQgain, gain);
            };
            addAndMakeVisible (eqBandGainDial[i]);

            addAndMakeVisible (eqBandGainValueLabel[i]);
            eqBandGainValueLabel[i].setText ("0.0 dB", juce::dontSendNotification);
            eqBandGainValueLabel[i].setColour (juce::Label::textColourId, juce::Colours::white);
            eqBandGainValueLabel[i].setJustificationType (juce::Justification::centred);

            // Q dial
            addAndMakeVisible (eqBandQLabel[i]);
            eqBandQLabel[i].setText ("Q", juce::dontSendNotification);
            eqBandQLabel[i].setColour (juce::Label::textColourId, juce::Colours::grey);
            eqBandQLabel[i].setJustificationType (juce::Justification::centred);

            eqBandQDial[i].onValueChanged = [this, i] (float v)
            {
                float q = 0.1f + 0.21f * (std::pow (100.0f, v) - 1.0f);  // 0.1-20.0
                eqBandQValueLabel[i].setText (juce::String (q, 2), juce::dontSendNotification);
                saveEQBandParam (i, WFSParameterIDs::reverbEQq, q);
            };
            addAndMakeVisible (eqBandQDial[i]);

            addAndMakeVisible (eqBandQValueLabel[i]);
            eqBandQValueLabel[i].setText ("0.70", juce::dontSendNotification);
            eqBandQValueLabel[i].setColour (juce::Label::textColourId, juce::Colours::white);
            eqBandQValueLabel[i].setJustificationType (juce::Justification::centred);

            // Slope dial
            addAndMakeVisible (eqBandSlopeLabel[i]);
            eqBandSlopeLabel[i].setText ("Slope", juce::dontSendNotification);
            eqBandSlopeLabel[i].setColour (juce::Label::textColourId, juce::Colours::grey);
            eqBandSlopeLabel[i].setJustificationType (juce::Justification::centred);

            eqBandSlopeDial[i].onValueChanged = [this, i] (float v)
            {
                float slope = 0.1f + 0.21f * (std::pow (100.0f, v) - 1.0f);  // 0.1-20.0
                eqBandSlopeValueLabel[i].setText (juce::String (slope, 2), juce::dontSendNotification);
                saveEQBandParam (i, WFSParameterIDs::reverbEQslope, slope);
            };
            addAndMakeVisible (eqBandSlopeDial[i]);

            addAndMakeVisible (eqBandSlopeValueLabel[i]);
            eqBandSlopeValueLabel[i].setText ("0.70", juce::dontSendNotification);
            eqBandSlopeValueLabel[i].setColour (juce::Label::textColourId, juce::Colours::white);
            eqBandSlopeValueLabel[i].setJustificationType (juce::Justification::centred);
        }
    }

    void setupAlgorithmSubTab()
    {
        addAndMakeVisible (algorithmPlaceholderLabel);
        algorithmPlaceholderLabel.setText ("Coming Soon", juce::dontSendNotification);
        algorithmPlaceholderLabel.setFont (juce::FontOptions().withHeight (32.0f).withStyle ("Bold"));
        algorithmPlaceholderLabel.setColour (juce::Label::textColourId, juce::Colour (0xFF666666));
        algorithmPlaceholderLabel.setJustificationType (juce::Justification::centred);
    }

    void setupReverbReturnSubTab()
    {
        // Distance Attenuation dial
        addAndMakeVisible (distanceAttenLabel);
        distanceAttenLabel.setText ("Distance Atten:", juce::dontSendNotification);
        distanceAttenLabel.setColour (juce::Label::textColourId, juce::Colours::white);

        distanceAttenDial.onValueChanged = [this] (float v)
        {
            float dB = v * 6.0f - 6.0f;  // -6 to 0 dB/m
            distanceAttenValueLabel.setText (juce::String (dB, 1) + " dB/m", juce::dontSendNotification);
            saveReverbParam (WFSParameterIDs::reverbDistanceAttenuation, dB);
        };
        addAndMakeVisible (distanceAttenDial);

        addAndMakeVisible (distanceAttenValueLabel);
        distanceAttenValueLabel.setText ("-0.7 dB/m", juce::dontSendNotification);
        distanceAttenValueLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        distanceAttenValueLabel.setJustificationType (juce::Justification::centred);

        // Common Attenuation dial
        addAndMakeVisible (commonAttenLabel);
        commonAttenLabel.setText ("Common Atten:", juce::dontSendNotification);
        commonAttenLabel.setColour (juce::Label::textColourId, juce::Colours::white);

        commonAttenDial.onValueChanged = [this] (float v)
        {
            int percent = static_cast<int> (v * 100.0f);  // 0-100%
            commonAttenValueLabel.setText (juce::String (percent) + "%", juce::dontSendNotification);
            saveReverbParam (WFSParameterIDs::reverbCommonAtten, percent);
        };
        addAndMakeVisible (commonAttenDial);

        addAndMakeVisible (commonAttenValueLabel);
        commonAttenValueLabel.setText ("100%", juce::dontSendNotification);
        commonAttenValueLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        commonAttenValueLabel.setJustificationType (juce::Justification::centred);

        // Mute buttons
        addAndMakeVisible (mutesLabel);
        mutesLabel.setText ("Output Mutes:", juce::dontSendNotification);
        mutesLabel.setColour (juce::Label::textColourId, juce::Colours::white);

        for (int i = 0; i < maxMuteButtons; ++i)
        {
            muteButtons[i].setButtonText (juce::String (i + 1));
            muteButtons[i].setColour (juce::ToggleButton::textColourId, juce::Colours::white);
            muteButtons[i].onClick = [this, i]
            {
                saveMuteStates();
            };
            addAndMakeVisible (muteButtons[i]);
        }

        // Mute Macro selector
        addAndMakeVisible (muteMacrosLabel);
        muteMacrosLabel.setText ("Mute Macro:", juce::dontSendNotification);
        muteMacrosLabel.setColour (juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible (muteMacrosSelector);
        muteMacrosSelector.addItem ("Mute Macro Select", 1);
        muteMacrosSelector.addItem ("MUTE ALL", 2);
        muteMacrosSelector.addItem ("UNMUTE ALL", 3);
        muteMacrosSelector.addItem ("INVERT MUTES", 4);
        muteMacrosSelector.addItem ("MUTE ODD", 5);
        muteMacrosSelector.addItem ("MUTE EVEN", 6);
        for (int arr = 1; arr <= 10; ++arr)
        {
            muteMacrosSelector.addItem ("MUTE ARRAY " + juce::String (arr), 6 + (arr - 1) * 2 + 1);
            muteMacrosSelector.addItem ("UNMUTE ARRAY " + juce::String (arr), 6 + (arr - 1) * 2 + 2);
        }
        muteMacrosSelector.setSelectedId (1);
        muteMacrosSelector.onChange = [this]
        {
            int macroId = muteMacrosSelector.getSelectedId();
            if (macroId > 1)
            {
                applyMuteMacro (macroId);
                muteMacrosSelector.setSelectedId (1, juce::dontSendNotification);
            }
        };
    }

    void setupFooter()
    {
        addAndMakeVisible (storeButton);
        storeButton.setButtonText ("Store");
        storeButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF2D2D2D));
        storeButton.onClick = [this] { storeReverbConfiguration(); };

        addAndMakeVisible (reloadButton);
        reloadButton.setButtonText ("Reload");
        reloadButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF2D2D2D));
        reloadButton.onClick = [this] { reloadReverbConfiguration(); };

        addAndMakeVisible (reloadBackupButton);
        reloadBackupButton.setButtonText ("Reload Backup");
        reloadBackupButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF2D2D2D));
        reloadBackupButton.onClick = [this] { reloadReverbConfigBackup(); };

        addAndMakeVisible (importButton);
        importButton.setButtonText ("Import");
        importButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF2D2D2D));
        importButton.onClick = [this] { importReverbConfiguration(); };

        addAndMakeVisible (exportButton);
        exportButton.setButtonText ("Export");
        exportButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF2D2D2D));
        exportButton.onClick = [this] { exportReverbConfiguration(); };
    }

    void setupEditableValueLabel (juce::Label& label)
    {
        label.setEditable (true, false);
        label.addListener (this);
    }

    void setupHelpText()
    {
        helpTextMap[&channelSelector] = "Reverb Channel Number and Selection.";
        helpTextMap[&nameEditor] = "Displayed Reverb Channel Name (editable).";
        helpTextMap[&attenuationSlider] = "Reverb channel attenuation (-92 to 0 dB).";
        helpTextMap[&delayLatencySlider] = "Reverb delay/latency compensation (-100 to +100 ms).";
        helpTextMap[&orientationDial] = "Reverb orientation angle (-179 to +180 degrees).";
        helpTextMap[&angleOnSlider] = "Angle at which amplification starts (1-180 degrees).";
        helpTextMap[&angleOffSlider] = "Angle at which no amplification occurs (0-179 degrees).";
        helpTextMap[&pitchSlider] = "Vertical orientation of reverb (-90 to +90 degrees).";
        helpTextMap[&hfDampingSlider] = "High frequency loss per meter (-6.0 to 0.0 dB/m).";
        helpTextMap[&distanceAttenEnableSlider] = "Distance attenuation percentage (0-200%).";
        helpTextMap[&eqEnableButton] = "Enable or disable EQ processing for this reverb.";
        helpTextMap[&distanceAttenDial] = "Distance attenuation for reverb return (-6.0 to 0.0 dB/m).";
        helpTextMap[&commonAttenDial] = "Common attenuation percentage (0-100%).";
        helpTextMap[&muteMacrosSelector] = "Quick mute operations for output channels.";
    }

    void setupOscMethods()
    {
        oscMethodMap[&channelSelector] = "/wfs/reverb/selected <ID>";
        oscMethodMap[&nameEditor] = "/wfs/reverb/name <ID> <value>";
        oscMethodMap[&attenuationSlider] = "/wfs/reverb/attenuation <ID> <value>";
        oscMethodMap[&delayLatencySlider] = "/wfs/reverb/delayLatency <ID> <value>";
        oscMethodMap[&orientationDial] = "/wfs/reverb/orientation <ID> <value>";
        oscMethodMap[&angleOnSlider] = "/wfs/reverb/angleOn <ID> <value>";
        oscMethodMap[&angleOffSlider] = "/wfs/reverb/angleOff <ID> <value>";
        oscMethodMap[&pitchSlider] = "/wfs/reverb/pitch <ID> <value>";
        oscMethodMap[&hfDampingSlider] = "/wfs/reverb/HFdamping <ID> <value>";
        oscMethodMap[&distanceAttenDial] = "/wfs/reverb/distanceAttenuation <ID> <value>";
        oscMethodMap[&commonAttenDial] = "/wfs/reverb/commonAtten <ID> <value>";
    }

    void setupMouseListeners()
    {
        for (auto& pair : helpTextMap)
            pair.first->addMouseListener (this, false);
    }

    //==========================================================================
    // Layout Methods
    //==========================================================================

    void layoutHeader (juce::Rectangle<int> area)
    {
        auto selectorArea = area.removeFromLeft (200);
        channelSelector.setBounds (selectorArea);

        area.removeFromLeft (20);

        nameLabel.setBounds (area.removeFromLeft (50));
        area.removeFromLeft (5);
        nameEditor.setBounds (area.removeFromLeft (200));
    }

    void layoutFooter (juce::Rectangle<int> area)
    {
        const int buttonWidth = (area.getWidth() - 40) / 5;
        const int spacing = 10;

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

        setReverbVisible (false);
        setPositionVisible (false);
        setReverbFeedVisible (false);
        setEQVisible (false);
        setAlgorithmVisible (false);
        setReverbReturnVisible (false);

        switch (tabIndex)
        {
            case 0: setReverbVisible (true); layoutReverbSubTab(); break;
            case 1: setPositionVisible (true); layoutPositionSubTab(); break;
            case 2: setReverbFeedVisible (true); layoutReverbFeedSubTab(); break;
            case 3: setEQVisible (true); layoutEQSubTab(); break;
            case 4: setAlgorithmVisible (true); layoutAlgorithmSubTab(); break;
            case 5: setReverbReturnVisible (true); layoutReverbReturnSubTab(); break;
        }
    }

    void layoutReverbSubTab()
    {
        auto area = subTabContentArea;
        const int rowHeight = 30;
        const int sliderHeight = 40;
        const int spacing = 15;
        const int labelWidth = 120;
        const int valueWidth = 80;

        auto leftCol = area.removeFromLeft (area.getWidth() / 2).reduced (10, 0);

        // Attenuation
        auto row = leftCol.removeFromTop (rowHeight);
        attenuationLabel.setBounds (row.removeFromLeft (labelWidth));
        attenuationValueLabel.setBounds (row.removeFromRight (valueWidth));
        leftCol.removeFromTop (5);
        attenuationSlider.setBounds (leftCol.removeFromTop (sliderHeight));
        leftCol.removeFromTop (spacing);

        // Delay/Latency
        row = leftCol.removeFromTop (rowHeight);
        delayLatencyLabel.setBounds (row.removeFromLeft (labelWidth));
        delayLatencyValueLabel.setBounds (row.removeFromRight (valueWidth));
        leftCol.removeFromTop (5);
        delayLatencySlider.setBounds (leftCol.removeFromTop (sliderHeight));
    }

    void layoutPositionSubTab()
    {
        auto area = subTabContentArea;
        const int rowHeight = 30;
        const int spacing = 10;
        const int labelWidth = 140;
        const int editorWidth = 80;
        const int unitWidth = 30;

        auto leftCol = area.removeFromLeft (area.getWidth() / 2).reduced (10, 0);
        auto rightCol = area.reduced (10, 0);

        // Position section (left)
        juce::Label* posLabelPtrs[] = { &posXLabel, &posYLabel, &posZLabel };
        juce::TextEditor* posEditorPtrs[] = { &posXEditor, &posYEditor, &posZEditor };
        juce::Label* posUnitPtrs[] = { &posXUnitLabel, &posYUnitLabel, &posZUnitLabel };

        for (int i = 0; i < 3; ++i)
        {
            auto row = leftCol.removeFromTop (rowHeight);
            posLabelPtrs[i]->setBounds (row.removeFromLeft (labelWidth));
            posEditorPtrs[i]->setBounds (row.removeFromLeft (editorWidth));
            row.removeFromLeft (5);
            posUnitPtrs[i]->setBounds (row.removeFromLeft (unitWidth));
            leftCol.removeFromTop (spacing);
        }

        // Return Offset section (right)
        juce::Label* offsetLabelPtrs[] = { &returnOffsetXLabel, &returnOffsetYLabel, &returnOffsetZLabel };
        juce::TextEditor* offsetEditorPtrs[] = { &returnOffsetXEditor, &returnOffsetYEditor, &returnOffsetZEditor };
        juce::Label* offsetUnitPtrs[] = { &returnOffsetXUnitLabel, &returnOffsetYUnitLabel, &returnOffsetZUnitLabel };

        for (int i = 0; i < 3; ++i)
        {
            auto row = rightCol.removeFromTop (rowHeight);
            offsetLabelPtrs[i]->setBounds (row.removeFromLeft (labelWidth));
            offsetEditorPtrs[i]->setBounds (row.removeFromLeft (editorWidth));
            row.removeFromLeft (5);
            offsetUnitPtrs[i]->setBounds (row.removeFromLeft (unitWidth));
            rightCol.removeFromTop (spacing);
        }
    }

    void layoutReverbFeedSubTab()
    {
        auto area = subTabContentArea;
        const int rowHeight = 30;
        const int sliderHeight = 40;
        const int spacing = 10;
        const int labelWidth = 140;
        const int valueWidth = 80;
        const int dialSize = 80;

        auto leftCol = area.removeFromLeft (area.getWidth() / 3).reduced (5, 0);
        auto middleCol = area.removeFromLeft (area.getWidth() / 2).reduced (5, 0);
        auto rightCol = area.reduced (5, 0);

        // Left column: Orientation dial
        orientationLabel.setBounds (leftCol.removeFromTop (rowHeight));
        auto dialArea = leftCol.removeFromTop (dialSize + 20);
        orientationDial.setBounds (dialArea.removeFromLeft (dialSize).withSizeKeepingCentre (dialSize, dialSize));
        orientationValueLabel.setBounds (leftCol.removeFromTop (rowHeight));

        // Left column: Buttons
        leftCol.removeFromTop (spacing * 2);
        miniLatencyEnableButton.setBounds (leftCol.removeFromTop (rowHeight));
        leftCol.removeFromTop (spacing);
        lsEnableButton.setBounds (leftCol.removeFromTop (rowHeight));

        // Middle column: Angle sliders
        auto row = middleCol.removeFromTop (rowHeight);
        angleOnLabel.setBounds (row.removeFromLeft (labelWidth));
        angleOnValueLabel.setBounds (row.removeFromRight (valueWidth));
        middleCol.removeFromTop (5);
        angleOnSlider.setBounds (middleCol.removeFromTop (sliderHeight));
        middleCol.removeFromTop (spacing);

        row = middleCol.removeFromTop (rowHeight);
        angleOffLabel.setBounds (row.removeFromLeft (labelWidth));
        angleOffValueLabel.setBounds (row.removeFromRight (valueWidth));
        middleCol.removeFromTop (5);
        angleOffSlider.setBounds (middleCol.removeFromTop (sliderHeight));
        middleCol.removeFromTop (spacing);

        row = middleCol.removeFromTop (rowHeight);
        pitchLabel.setBounds (row.removeFromLeft (labelWidth));
        pitchValueLabel.setBounds (row.removeFromRight (valueWidth));
        middleCol.removeFromTop (5);
        pitchSlider.setBounds (middleCol.removeFromTop (sliderHeight));

        // Right column: HF Damping and Distance Atten
        row = rightCol.removeFromTop (rowHeight);
        hfDampingLabel.setBounds (row.removeFromLeft (labelWidth));
        hfDampingValueLabel.setBounds (row.removeFromRight (valueWidth));
        rightCol.removeFromTop (5);
        hfDampingSlider.setBounds (rightCol.removeFromTop (sliderHeight));
        rightCol.removeFromTop (spacing);

        row = rightCol.removeFromTop (rowHeight);
        distanceAttenEnableLabel.setBounds (row.removeFromLeft (labelWidth));
        distanceAttenEnableValueLabel.setBounds (row.removeFromRight (valueWidth));
        rightCol.removeFromTop (5);
        distanceAttenEnableSlider.setBounds (rightCol.removeFromTop (sliderHeight));
    }

    void layoutEQSubTab()
    {
        auto area = subTabContentArea;
        const int buttonHeight = 30;
        const int bandWidth = (area.getWidth() - 40) / numEqBands;
        const int dialSize = 60;
        const int sliderHeight = 35;
        const int labelHeight = 20;
        const int spacing = 5;

        // EQ Enable button at top
        eqEnableButton.setBounds (area.removeFromTop (buttonHeight).withWidth (100));
        area.removeFromTop (spacing * 2);

        // Layout bands horizontally
        for (int i = 0; i < numEqBands; ++i)
        {
            auto bandArea = area.removeFromLeft (bandWidth).reduced (5, 0);

            eqBandLabel[i].setBounds (bandArea.removeFromTop (labelHeight));
            eqBandShapeSelector[i].setBounds (bandArea.removeFromTop (buttonHeight));
            bandArea.removeFromTop (spacing);

            // Frequency slider
            eqBandFreqLabel[i].setBounds (bandArea.removeFromTop (labelHeight));
            eqBandFreqSlider[i].setBounds (bandArea.removeFromTop (sliderHeight));
            eqBandFreqValueLabel[i].setBounds (bandArea.removeFromTop (labelHeight));
            bandArea.removeFromTop (spacing);

            // Gain, Q, Slope dials in a row
            auto dialRow = bandArea.removeFromTop (dialSize + labelHeight * 2);
            int dialSpacing = (dialRow.getWidth() - dialSize * 3) / 4;

            auto gainArea = dialRow.removeFromLeft (dialSize + dialSpacing).reduced (dialSpacing / 2, 0);
            eqBandGainLabel[i].setBounds (gainArea.removeFromTop (labelHeight));
            eqBandGainDial[i].setBounds (gainArea.removeFromTop (dialSize).withSizeKeepingCentre (dialSize, dialSize));
            eqBandGainValueLabel[i].setBounds (gainArea.removeFromTop (labelHeight));

            auto qArea = dialRow.removeFromLeft (dialSize + dialSpacing).reduced (dialSpacing / 2, 0);
            eqBandQLabel[i].setBounds (qArea.removeFromTop (labelHeight));
            eqBandQDial[i].setBounds (qArea.removeFromTop (dialSize).withSizeKeepingCentre (dialSize, dialSize));
            eqBandQValueLabel[i].setBounds (qArea.removeFromTop (labelHeight));

            auto slopeArea = dialRow.removeFromLeft (dialSize + dialSpacing).reduced (dialSpacing / 2, 0);
            eqBandSlopeLabel[i].setBounds (slopeArea.removeFromTop (labelHeight));
            eqBandSlopeDial[i].setBounds (slopeArea.removeFromTop (dialSize).withSizeKeepingCentre (dialSize, dialSize));
            eqBandSlopeValueLabel[i].setBounds (slopeArea.removeFromTop (labelHeight));
        }
    }

    void layoutAlgorithmSubTab()
    {
        algorithmPlaceholderLabel.setBounds (subTabContentArea);
    }

    void layoutReverbReturnSubTab()
    {
        auto area = subTabContentArea;
        const int dialSize = 80;
        const int labelHeight = 25;
        const int spacing = 10;

        auto topRow = area.removeFromTop (dialSize + labelHeight * 2 + spacing);

        // Distance Attenuation dial
        auto dialArea = topRow.removeFromLeft (150).reduced (10, 0);
        distanceAttenLabel.setBounds (dialArea.removeFromTop (labelHeight));
        distanceAttenDial.setBounds (dialArea.removeFromTop (dialSize).withSizeKeepingCentre (dialSize, dialSize));
        distanceAttenValueLabel.setBounds (dialArea.removeFromTop (labelHeight));

        // Common Attenuation dial
        dialArea = topRow.removeFromLeft (150).reduced (10, 0);
        commonAttenLabel.setBounds (dialArea.removeFromTop (labelHeight));
        commonAttenDial.setBounds (dialArea.removeFromTop (dialSize).withSizeKeepingCentre (dialSize, dialSize));
        commonAttenValueLabel.setBounds (dialArea.removeFromTop (labelHeight));

        // Mute Macro selector
        topRow.removeFromLeft (20);
        auto macroArea = topRow.removeFromLeft (200);
        muteMacrosLabel.setBounds (macroArea.removeFromTop (labelHeight));
        muteMacrosSelector.setBounds (macroArea.removeFromTop (30));

        // Mutes section
        area.removeFromTop (spacing);
        mutesLabel.setBounds (area.removeFromTop (labelHeight));
        area.removeFromTop (spacing);

        // Layout mute buttons in a grid
        int numOutputs = parameters.getNumOutputChannels();
        if (numOutputs <= 0) numOutputs = 16;
        int numColumns = juce::jmin (8, numOutputs);
        const int buttonSize = 40;
        const int buttonSpacing = 5;

        for (int i = 0; i < maxMuteButtons; ++i)
        {
            if (i < numOutputs)
            {
                int col = i % numColumns;
                int row = i / numColumns;
                int x = col * (buttonSize + buttonSpacing);
                int y = row * (buttonSize + buttonSpacing);
                muteButtons[i].setBounds (area.getX() + x, area.getY() + y, buttonSize, buttonSize);
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

    void setReverbVisible (bool visible)
    {
        attenuationLabel.setVisible (visible);
        attenuationSlider.setVisible (visible);
        attenuationValueLabel.setVisible (visible);
        delayLatencyLabel.setVisible (visible);
        delayLatencySlider.setVisible (visible);
        delayLatencyValueLabel.setVisible (visible);
    }

    void setPositionVisible (bool visible)
    {
        posXLabel.setVisible (visible); posYLabel.setVisible (visible); posZLabel.setVisible (visible);
        posXEditor.setVisible (visible); posYEditor.setVisible (visible); posZEditor.setVisible (visible);
        posXUnitLabel.setVisible (visible); posYUnitLabel.setVisible (visible); posZUnitLabel.setVisible (visible);
        returnOffsetXLabel.setVisible (visible); returnOffsetYLabel.setVisible (visible); returnOffsetZLabel.setVisible (visible);
        returnOffsetXEditor.setVisible (visible); returnOffsetYEditor.setVisible (visible); returnOffsetZEditor.setVisible (visible);
        returnOffsetXUnitLabel.setVisible (visible); returnOffsetYUnitLabel.setVisible (visible); returnOffsetZUnitLabel.setVisible (visible);
    }

    void setReverbFeedVisible (bool visible)
    {
        orientationLabel.setVisible (visible);
        orientationDial.setVisible (visible);
        orientationValueLabel.setVisible (visible);
        angleOnLabel.setVisible (visible); angleOnSlider.setVisible (visible); angleOnValueLabel.setVisible (visible);
        angleOffLabel.setVisible (visible); angleOffSlider.setVisible (visible); angleOffValueLabel.setVisible (visible);
        pitchLabel.setVisible (visible); pitchSlider.setVisible (visible); pitchValueLabel.setVisible (visible);
        hfDampingLabel.setVisible (visible); hfDampingSlider.setVisible (visible); hfDampingValueLabel.setVisible (visible);
        miniLatencyEnableButton.setVisible (visible);
        lsEnableButton.setVisible (visible);
        distanceAttenEnableLabel.setVisible (visible);
        distanceAttenEnableSlider.setVisible (visible);
        distanceAttenEnableValueLabel.setVisible (visible);
    }

    void setEQVisible (bool visible)
    {
        eqEnableButton.setVisible (visible);
        for (int i = 0; i < numEqBands; ++i)
        {
            eqBandLabel[i].setVisible (visible);
            eqBandShapeSelector[i].setVisible (visible);
            eqBandFreqLabel[i].setVisible (visible);
            eqBandFreqSlider[i].setVisible (visible);
            eqBandFreqValueLabel[i].setVisible (visible);
            eqBandGainLabel[i].setVisible (visible);
            eqBandGainDial[i].setVisible (visible);
            eqBandGainValueLabel[i].setVisible (visible);
            eqBandQLabel[i].setVisible (visible);
            eqBandQDial[i].setVisible (visible);
            eqBandQValueLabel[i].setVisible (visible);
            eqBandSlopeLabel[i].setVisible (visible);
            eqBandSlopeDial[i].setVisible (visible);
            eqBandSlopeValueLabel[i].setVisible (visible);
        }
    }

    void setAlgorithmVisible (bool visible)
    {
        algorithmPlaceholderLabel.setVisible (visible);
    }

    void setReverbReturnVisible (bool visible)
    {
        distanceAttenLabel.setVisible (visible);
        distanceAttenDial.setVisible (visible);
        distanceAttenValueLabel.setVisible (visible);
        commonAttenLabel.setVisible (visible);
        commonAttenDial.setVisible (visible);
        commonAttenValueLabel.setVisible (visible);
        mutesLabel.setVisible (visible);
        muteMacrosLabel.setVisible (visible);
        muteMacrosSelector.setVisible (visible);
        for (int i = 0; i < maxMuteButtons; ++i)
            muteButtons[i].setVisible (visible && i < parameters.getNumOutputChannels());
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
        delayLatencySlider.setValue ((delayMs + 100.0f) / 200.0f);
        delayLatencyValueLabel.setText (juce::String (delayMs, 1) + " ms", juce::dontSendNotification);

        // Position
        posXEditor.setText (juce::String (getFloatParam (WFSParameterIDs::reverbPositionX, 0.0f), 2), juce::dontSendNotification);
        posYEditor.setText (juce::String (getFloatParam (WFSParameterIDs::reverbPositionY, 0.0f), 2), juce::dontSendNotification);
        posZEditor.setText (juce::String (getFloatParam (WFSParameterIDs::reverbPositionZ, 0.0f), 2), juce::dontSendNotification);

        // Return Offset
        returnOffsetXEditor.setText (juce::String (getFloatParam (WFSParameterIDs::reverbReturnOffsetX, 0.0f), 2), juce::dontSendNotification);
        returnOffsetYEditor.setText (juce::String (getFloatParam (WFSParameterIDs::reverbReturnOffsetY, 0.0f), 2), juce::dontSendNotification);
        returnOffsetZEditor.setText (juce::String (getFloatParam (WFSParameterIDs::reverbReturnOffsetZ, 0.0f), 2), juce::dontSendNotification);

        // Reverb Feed
        int orientation = getIntParam (WFSParameterIDs::reverbOrientation, 0);
        orientationDial.setAngle (static_cast<float> (orientation));
        orientationValueLabel.setText (juce::String (orientation) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);

        int angleOn = getIntParam (WFSParameterIDs::reverbAngleOn, 86);
        angleOnSlider.setValue ((angleOn - 1.0f) / 179.0f);
        angleOnValueLabel.setText (juce::String (angleOn) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);

        int angleOff = getIntParam (WFSParameterIDs::reverbAngleOff, 90);
        angleOffSlider.setValue (angleOff / 179.0f);
        angleOffValueLabel.setText (juce::String (angleOff) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);

        int pitch = getIntParam (WFSParameterIDs::reverbPitch, 0);
        pitchSlider.setValue ((pitch + 90.0f) / 180.0f);
        pitchValueLabel.setText (juce::String (pitch) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);

        float hfDamping = getFloatParam (WFSParameterIDs::reverbHFdamping, 0.0f);
        hfDampingSlider.setValue ((hfDamping + 6.0f) / 6.0f);
        hfDampingValueLabel.setText (juce::String (hfDamping, 1) + " dB/m", juce::dontSendNotification);

        int miniLatency = getIntParam (WFSParameterIDs::reverbMiniLatencyEnable, 1);
        miniLatencyEnableButton.setToggleState (miniLatency != 0, juce::dontSendNotification);
        miniLatencyEnableButton.setButtonText (miniLatency != 0 ? "ENABLE" : "DISABLE");

        int lsEnable = getIntParam (WFSParameterIDs::reverbLSenable, 1);
        lsEnableButton.setToggleState (lsEnable != 0, juce::dontSendNotification);
        lsEnableButton.setButtonText (lsEnable != 0 ? "LS ENABLE" : "LS DISABLE");

        int distanceAttenEnable = getIntParam (WFSParameterIDs::reverbDistanceAttenEnable, 100);
        distanceAttenEnableSlider.setValue (distanceAttenEnable / 200.0f);
        distanceAttenEnableValueLabel.setText (juce::String (distanceAttenEnable) + "%", juce::dontSendNotification);

        // EQ
        int eqEnabled = getIntParam (WFSParameterIDs::reverbEQenable, 1);
        eqEnableButton.setToggleState (eqEnabled != 0, juce::dontSendNotification);
        eqEnableButton.setButtonText (eqEnabled != 0 ? "EQ ON" : "EQ OFF");
        eqEnableButton.setColour (juce::TextButton::buttonColourId,
                                  eqEnabled != 0 ? juce::Colour (0xFF4CAF50) : juce::Colour (0xFF2D2D2D));

        loadEQBandParameters();

        // Reverb Return
        float distanceAtten = getFloatParam (WFSParameterIDs::reverbDistanceAttenuation, -0.7f);
        distanceAttenDial.setValue ((distanceAtten + 6.0f) / 6.0f);
        distanceAttenValueLabel.setText (juce::String (distanceAtten, 1) + " dB/m", juce::dontSendNotification);

        int commonAtten = getIntParam (WFSParameterIDs::reverbCommonAtten, 100);
        commonAttenDial.setValue (commonAtten / 100.0f);
        commonAttenValueLabel.setText (juce::String (commonAtten) + "%", juce::dontSendNotification);

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

            int shape = band.getProperty (WFSParameterIDs::reverbEQshape, 0);
            eqBandShapeSelector[i].setSelectedId (shape + 1, juce::dontSendNotification);

            int freq = band.getProperty (WFSParameterIDs::reverbEQfreq, 1000);
            float freqSlider = std::log10 (freq / 20.0f) / 3.0f;
            eqBandFreqSlider[i].setValue (juce::jlimit (0.0f, 1.0f, freqSlider));
            eqBandFreqValueLabel[i].setText (formatFrequency (freq), juce::dontSendNotification);

            float gain = band.getProperty (WFSParameterIDs::reverbEQgain, 0.0f);
            eqBandGainDial[i].setValue ((gain + 24.0f) / 48.0f);
            eqBandGainValueLabel[i].setText (juce::String (gain, 1) + " dB", juce::dontSendNotification);

            float q = band.getProperty (WFSParameterIDs::reverbEQq, 0.7f);
            float qSlider = std::log ((q - 0.1f) / 0.21f + 1.0f) / std::log (100.0f);
            eqBandQDial[i].setValue (juce::jlimit (0.0f, 1.0f, qSlider));
            eqBandQValueLabel[i].setText (juce::String (q, 2), juce::dontSendNotification);

            float slope = band.getProperty (WFSParameterIDs::reverbEQslope, 0.7f);
            float slopeSlider = std::log ((slope - 0.1f) / 0.21f + 1.0f) / std::log (100.0f);
            eqBandSlopeDial[i].setValue (juce::jlimit (0.0f, 1.0f, slopeSlider));
            eqBandSlopeValueLabel[i].setText (juce::String (slope, 2), juce::dontSendNotification);
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
        parameters.setReverbParam (currentChannel - 1, paramId.toString(), value);
    }

    void saveEQBandParam (int bandIndex, const juce::Identifier& paramId, const juce::var& value)
    {
        if (isLoadingParameters) return;

        auto& vts = parameters.getValueTreeState();
        auto band = vts.getReverbEQBand (currentChannel - 1, bandIndex);
        if (band.isValid())
            band.setProperty (paramId, value, vts.getUndoManager());
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
                    // TODO: Implement array-based muting based on output array assignments
                    // int arrayIndex = (macroId - 7) / 2;
                    // bool mute = ((macroId - 7) % 2) == 0;
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
            showStatusMessage ("Please select a project folder in System Config first.");
            return;
        }
        if (fileManager.saveReverbConfig())
            showStatusMessage ("Reverb configuration saved.");
        else
            showStatusMessage ("Error: " + fileManager.getLastError());
    }

    void reloadReverbConfiguration()
    {
        auto& fileManager = parameters.getFileManager();
        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage ("Please select a project folder in System Config first.");
            return;
        }
        if (fileManager.loadReverbConfig())
        {
            loadChannelParameters (currentChannel);
            showStatusMessage ("Reverb configuration loaded.");
        }
        else
            showStatusMessage ("Error: " + fileManager.getLastError());
    }

    void reloadReverbConfigBackup()
    {
        auto& fileManager = parameters.getFileManager();
        if (!fileManager.hasValidProjectFolder())
        {
            showStatusMessage ("Please select a project folder in System Config first.");
            return;
        }
        if (fileManager.loadReverbConfigBackup (0))
        {
            loadChannelParameters (currentChannel);
            showStatusMessage ("Reverb configuration loaded from backup.");
        }
        else
            showStatusMessage ("Error: " + fileManager.getLastError());
    }

    void importReverbConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser> ("Import Reverb Configuration",
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
                        showStatusMessage ("Reverb configuration imported.");
                    }
                    else
                        showStatusMessage ("Error: " + fileManager.getLastError());
                }
            });
    }

    void exportReverbConfiguration()
    {
        auto chooser = std::make_shared<juce::FileChooser> ("Export Reverb Configuration",
            juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("reverbs.xml"), "*.xml");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser] (const juce::FileChooser& fc)
            {
                auto result = fc.getResult();
                if (result != juce::File())
                {
                    auto& fileManager = parameters.getFileManager();
                    if (fileManager.exportReverbConfig (result))
                        showStatusMessage ("Reverb configuration exported.");
                    else
                        showStatusMessage ("Error: " + fileManager.getLastError());
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
    }

    void textEditorReturnKeyPressed (juce::TextEditor& editor) override
    {
        textEditorFocusLost (editor);
    }

    void textEditorFocusLost (juce::TextEditor& editor) override
    {
        if (isLoadingParameters) return;

        if (&editor == &nameEditor)
            saveReverbParam (WFSParameterIDs::reverbName, nameEditor.getText());
        else if (&editor == &posXEditor)
            saveReverbParam (WFSParameterIDs::reverbPositionX, editor.getText().getFloatValue());
        else if (&editor == &posYEditor)
            saveReverbParam (WFSParameterIDs::reverbPositionY, editor.getText().getFloatValue());
        else if (&editor == &posZEditor)
            saveReverbParam (WFSParameterIDs::reverbPositionZ, editor.getText().getFloatValue());
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
        }
        else if (label == &delayLatencyValueLabel)
        {
            float ms = juce::jlimit (-100.0f, 100.0f, value);
            delayLatencySlider.setValue ((ms + 100.0f) / 200.0f);
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
            }
            updateVisibility();
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

        auto* component = event.eventComponent;
        if (helpTextMap.find (component) != helpTextMap.end())
            statusBar->setHelpText (helpTextMap[component]);
        if (oscMethodMap.find (component) != oscMethodMap.end())
            statusBar->setOscMethod (oscMethodMap[component]);
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        if (statusBar != nullptr)
        {
            statusBar->setHelpText ("");
            statusBar->setOscMethod ("");
        }
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
        orientationDial.setVisible (hasChannels);
        orientationValueLabel.setVisible (hasChannels);
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
        for (int i = 0; i < numEqBands; ++i)
        {
            eqBandLabel[i].setVisible (hasChannels);
            eqBandShapeSelector[i].setVisible (hasChannels);
            eqBandFreqLabel[i].setVisible (hasChannels);
            eqBandFreqSlider[i].setVisible (hasChannels);
            eqBandFreqValueLabel[i].setVisible (hasChannels);
            eqBandGainLabel[i].setVisible (hasChannels);
            eqBandGainDial[i].setVisible (hasChannels);
            eqBandGainValueLabel[i].setVisible (hasChannels);
            eqBandQLabel[i].setVisible (hasChannels);
            eqBandQDial[i].setVisible (hasChannels);
            eqBandQValueLabel[i].setVisible (hasChannels);
            eqBandSlopeLabel[i].setVisible (hasChannels);
            eqBandSlopeDial[i].setVisible (hasChannels);
            eqBandSlopeValueLabel[i].setVisible (hasChannels);
        }

        // Algorithm sub-tab
        algorithmPlaceholderLabel.setVisible (hasChannels);

        // Reverb Return sub-tab
        distanceAttenLabel.setVisible (hasChannels);
        distanceAttenDial.setVisible (hasChannels);
        distanceAttenValueLabel.setVisible (hasChannels);
        commonAttenLabel.setVisible (hasChannels);
        commonAttenDial.setVisible (hasChannels);
        commonAttenValueLabel.setVisible (hasChannels);
        mutesLabel.setVisible (hasChannels);
        for (int i = 0; i < maxMuteButtons; ++i)
            muteButtons[i].setVisible (hasChannels && i < parameters.getNumOutputChannels());
        muteMacrosLabel.setVisible (hasChannels);
        muteMacrosSelector.setVisible (hasChannels);

        // Footer buttons remain visible (for Import functionality)
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

    static constexpr int headerHeight = 60;
    static constexpr int footerHeight = 50;
    static constexpr int numEqBands = 4;
    static constexpr int maxMuteButtons = 64;
    juce::Rectangle<int> subTabContentArea;

    // Header
    ChannelSelectorButton channelSelector { "Reverb" };
    juce::Label nameLabel;
    juce::TextEditor nameEditor;

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
    juce::Label posXLabel, posYLabel, posZLabel;
    juce::TextEditor posXEditor, posYEditor, posZEditor;
    juce::Label posXUnitLabel, posYUnitLabel, posZUnitLabel;
    juce::Label returnOffsetXLabel, returnOffsetYLabel, returnOffsetZLabel;
    juce::TextEditor returnOffsetXEditor, returnOffsetYEditor, returnOffsetZEditor;
    juce::Label returnOffsetXUnitLabel, returnOffsetYUnitLabel, returnOffsetZUnitLabel;

    // Reverb Feed sub-tab
    juce::Label orientationLabel;
    WfsEndlessDial orientationDial;
    juce::Label orientationValueLabel;
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
    WfsStandardSlider distanceAttenEnableSlider;
    juce::Label distanceAttenEnableValueLabel;

    // EQ sub-tab
    juce::TextButton eqEnableButton;
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
    juce::Label eqBandSlopeLabel[numEqBands];
    WfsBasicDial eqBandSlopeDial[numEqBands];
    juce::Label eqBandSlopeValueLabel[numEqBands];

    // Algorithm sub-tab (placeholder)
    juce::Label algorithmPlaceholderLabel;

    // Reverb Return sub-tab
    juce::Label distanceAttenLabel;
    WfsBasicDial distanceAttenDial;
    juce::Label distanceAttenValueLabel;
    juce::Label commonAttenLabel;
    WfsBasicDial commonAttenDial;
    juce::Label commonAttenValueLabel;
    juce::Label mutesLabel;
    juce::ToggleButton muteButtons[maxMuteButtons];
    juce::Label muteMacrosLabel;
    juce::ComboBox muteMacrosSelector;

    // Footer buttons
    juce::TextButton storeButton;
    juce::TextButton reloadButton;
    juce::TextButton reloadBackupButton;
    juce::TextButton importButton;
    juce::TextButton exportButton;

    // No channels message
    juce::Label noChannelsLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbTab)
};
