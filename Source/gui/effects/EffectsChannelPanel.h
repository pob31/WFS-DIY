#pragma once

#include <JuceHeader.h>
#include "EffectsTabContext.h"
#include "../ColorScheme.h"
#include "../ColorUtilities.h"
#include "../buttons/LongPressButton.h"
#include "../dials/WfsBasicDial.h"
#include "../dials/WfsDirectionalDial.h"
#include "../sliders/WfsStandardSlider.h"
#include "../sliders/WfsBidirectionalSlider.h"
#include "../sliders/WfsWidthExpansionSlider.h"
#include "../../Parameters/WFSParameterIDs.h"
#include "../../Parameters/WFSParameterDefaults.h"
#include "../../Localization/LocalizationManager.h"

/**
    The Channel Parameters sub-tab: what the return costs, where it sits and
    how it is heard. Three columns, laid out to the Reverb tab's geometry
    (ReverbTab::layoutChannelParametersTab) so the two families read alike:

      column 1   attenuation, delay/latency, coordinates, position + return offset
      column 2   "Effect Feed": the cone dial with its three sliders, HF damping,
                 distance attenuation %, the feed's minimal-latency switch
      column 3   "Effect Return": the two attenuation dials, the HF shelf, the
                 mute macro and the per-output mute grid, the per-ARRAY trims

    TWO POSITIONS, ONE CHANNEL. The feed listens at the base position and the
    return is heard at base + offset, which is why the offset has its own
    boxes. AutomOtion moves the RETURN by a further offset the engine adds and
    this panel never writes; it lives on the Movements tab.

    LEVEL 3 IS DELIBERATELY SMALLER THAN LEVELS 1 AND 2. A return is a WFS
    render source, so its per-output gains are solved from geometry. The
    operator gets a per-output MUTE and a per-ARRAY trim on top of that
    solution and nothing else, because an arbitrary per-output level would
    overwrite the spatialisation that makes the return localise where its
    marker sits.
*/
class EffectsChannelPanel : public juce::Component,
                            private juce::TextEditor::Listener
{
public:
    explicit EffectsChannelPanel (EffectsTabContext& context) : ctx (context)
    {
        setupLinkRow();
        setupColumn1();
        setupFeedColumn();
        setupReturnColumn();
    }

    /** Called from the tab's loadChannelParameters, inside its loading scope. */
    /** The group names changed on the Settings tab: rebuild the combo's
        items, keeping the selection. */
    void refreshLinkGroupNames()
    {
        const int selected = linkGroupCombo.getSelectedId();
        linkGroupCombo.clear (juce::dontSendNotification);
        linkGroupCombo.addItem (LOC ("effects.link.unlinked"), 1);
        for (int g = 1; g <= WFSParameterDefaults::effectLinkGroupMax; ++g)
            linkGroupCombo.addItem (groupName (g), g + 1);
        linkGroupCombo.setSelectedId (selected, juce::dontSendNotification);
    }

    void loadParameters()
    {
        using namespace WFSParameterIDs;
        namespace D = WFSParameterDefaults;

        const int group = juce::jlimit (0, D::effectLinkGroupMax, ctx.readInt (effectLinkGroup, 0));
        linkGroupCombo.setSelectedId (group + 1, juce::dontSendNotification);
        const int mode = juce::jlimit (D::effectLinkModeMin, D::effectLinkModeMax,
                                       ctx.readInt (effectLinkMode, D::effectLinkModeDefault));
        linkModeCombo.setSelectedId (mode + 1, juce::dontSendNotification);
        updateLinkModeEnabled();

        setSliderFromDb (attenuationSlider, attenuationValue,
                         ctx.readFloat (effectAttenuation, D::effectAttenuationDefault),
                         D::effectAttenuationMin);

        const float latency = ctx.readFloat (effectDelayLatency, D::effectDelayLatencyDefault);
        delayLatencySlider.setValue (latency / D::effectDelayLatencyMax);
        delayLatencyValue.setText (latencyText (latency), juce::dontSendNotification);

        setLatencyButton (minimalLatencyButton,
                          ctx.readInt (effectMinimalLatency, D::effectMinimalLatencyDefault) != 0);

        coordModeCombo.setSelectedId (ctx.readInt (effectCoordinateMode, 0) + 1, juce::dontSendNotification);
        loadPositionEditors();

        const int orientation = ctx.readInt (effectOrientation, D::effectOrientationDefault);
        const int angleOn  = ctx.readInt (effectAngleOn,  D::effectAngleOnDefault);
        const int angleOff = ctx.readInt (effectAngleOff, D::effectAngleOffDefault);

        directionalDial.setOrientation (static_cast<float> (orientation));
        directionalDial.setAngleOn (angleOn);
        directionalDial.setAngleOff (angleOff);
        orientationValue.setText (juce::String (orientation) + " " + degreeSign(), juce::dontSendNotification);

        angleOnSlider.setValue (normalise (static_cast<float> (angleOn),
                                           static_cast<float> (D::effectAngleOnMin),
                                           static_cast<float> (D::effectAngleOnMax)));
        angleOffSlider.setValue (normalise (static_cast<float> (angleOff),
                                            static_cast<float> (D::effectAngleOffMin),
                                            static_cast<float> (D::effectAngleOffMax)));
        pitchSlider.setValue (static_cast<float> (ctx.readInt (effectPitch, D::effectPitchDefault))
                              / static_cast<float> (D::effectPitchMax));
        updateAngleLabels();

        setSliderLinear (hfDampingSlider, hfDampingValue,
                         ctx.readFloat (effectHFdamping, D::effectHFdampingDefault),
                         D::effectHFdampingMin, D::effectHFdampingMax, " dB/m", 1);
        setSliderLinear (distanceAttenPercentSlider, distanceAttenPercentValue,
                         static_cast<float> (ctx.readInt (effectDistanceAttenPercent, D::effectDistanceAttenPercentDefault)),
                         static_cast<float> (D::effectDistanceAttenPercentMin),
                         static_cast<float> (D::effectDistanceAttenPercentMax), "%", 0);
        setLatencyButton (feedMiniLatencyButton,
                          ctx.readInt (effectFeedMiniLatency, D::effectFeedMiniLatencyDefault) != 0);

        const bool inverseLaw = ctx.readInt (effectAttenuationLaw, D::effectAttenuationLawDefault) != 0;
        setToggle (attenuationLawButton, inverseLaw, "effects.toggles.lawLog", "effects.toggles.lawInverse");
        // Labels set explicitly, not only from the dial callbacks: a dial
        // whose loaded value equals its current one fires no callback.
        const float distAtten = ctx.readFloat (effectDistanceAttenuation, D::effectDistanceAttenuationDefault);
        distanceAttenDial.setValue (distAtten);
        distanceAttenValue.setText (juce::String (distAtten, 1) + " dB/m", juce::dontSendNotification);

        const float distRatio = ctx.readFloat (effectDistanceRatio, D::effectDistanceRatioDefault);
        distanceRatioDial.setValue (distRatio);
        distanceRatioValue.setText (juce::String (distRatio, 2) + " x", juce::dontSendNotification);
        updateAttenuationLawVisibility();

        const int common = ctx.readInt (effectCommonAtten, D::effectCommonAttenDefault);
        commonAttenDial.setValue (static_cast<float> (common));
        commonAttenValue.setText (juce::String (common) + " %", juce::dontSendNotification);
        setSliderLinear (hfShelfSlider, hfShelfValue,
                         ctx.readFloat (effectHFshelf, D::effectHFshelfDefault),
                         D::effectHFshelfMin, D::effectHFshelfMax, " dB", 1);
        setToggle (muteReverbSendsButton, ctx.readInt (effectMuteReverbSends, D::effectMuteReverbSendsDefault) != 0,
                   "effects.toggles.reverbSendsUnmuted", "effects.toggles.reverbSendsMuted");

        loadMuteStates();
        loadArrayAttens();
    }

    /** The output count moved: the mute grid is one toggle per LIVE output and
        the array dials dim for arrays no output belongs to. */
    void refreshOutputDependentControls()
    {
        loadMuteStates();
        updateArrayAttenDimming();
        resized();
    }

    void paint (juce::Graphics& g) override
    {
        // The Reverb tab's column dividers, not cards: two thin lines and
        // otherwise the tab background.
        g.setColour (ColorScheme::get().chromeDivider);
        const float topY = static_cast<float> (contentTop);
        const float botY = static_cast<float> (getHeight());
        g.drawVerticalLine (columnDividerX1, topY, botY);
        g.drawVerticalLine (columnDividerX2, topY, botY);
    }

    void resized() override
    {
        layoutScale = static_cast<float> (getHeight()) / 780.0f;

        auto area = getLocalBounds().reduced (scaled (10), scaled (10));

        const int rowHeight    = scaled (30);
        const int sliderHeight = scaled (40);
        const int spacing      = scaled (10);
        const int labelWidth   = scaled (115);
        const int valueWidth   = scaled (60);
        const int editorWidth  = scaled (70);
        const int unitWidth    = scaled (25);
        const int dialSize     = juce::jmax (60, static_cast<int> (100.0f * layoutScale));
        const int titleHeight  = scaled (25);

        // Top row: the link-group controls (full width, above the 3 columns),
        // where the reverb tab keeps "Apply to all nodes".
        {
            auto topRow = area.removeFromTop (rowHeight);
            linkGroupCombo.setBounds (topRow.removeFromLeft (scaled (160)));
            topRow.removeFromLeft (spacing);
            linkModeCombo.setBounds (topRow.removeFromLeft (scaled (150)));
            topRow.removeFromLeft (spacing);
            groupMuteButton.setBounds (topRow.removeFromLeft (scaled (130)));
        }
        area.removeFromTop (spacing);
        contentTop = area.getY();

        const int colWidth = area.getWidth() / 3;
        auto col1 = area.removeFromLeft (colWidth).reduced (scaled (5), 0);
        columnDividerX1 = area.getX();
        auto col2 = area.removeFromLeft (colWidth).reduced (scaled (5), 0);
        columnDividerX2 = area.getX();
        auto col3 = area.reduced (scaled (5), 0);

        // =====================================================================
        // Column 1: channel + position
        // =====================================================================
        auto row = col1.removeFromTop (rowHeight);
        attenuationLabel.setBounds (row.removeFromLeft (labelWidth));
        attenuationValue.setBounds (row.removeFromRight (valueWidth));
        col1.removeFromTop (scaled (3));
        attenuationSlider.setBounds (col1.removeFromTop (sliderHeight));
        col1.removeFromTop (spacing);

        row = col1.removeFromTop (rowHeight);
        delayLatencyLabel.setBounds (row.removeFromLeft (labelWidth));
        delayLatencyValue.setBounds (row.removeFromRight (scaled (130)));
        col1.removeFromTop (scaled (3));
        delayLatencySlider.setBounds (col1.removeFromTop (sliderHeight));
        col1.removeFromTop (spacing);

        minimalLatencyButton.setBounds (col1.removeFromTop (rowHeight));
        col1.removeFromTop (spacing);

        auto coordRow = col1.removeFromTop (rowHeight);
        coordModeLabel.setBounds (coordRow.removeFromLeft (scaled (50)));
        coordModeCombo.setBounds (coordRow.removeFromLeft (scaled (80)));
        col1.removeFromTop (spacing);

        for (int i = 0; i < 3; ++i)
        {
            row = col1.removeFromTop (rowHeight);
            positionLabels[i].setBounds (row.removeFromLeft (labelWidth));
            positionEditors[i].setBounds (row.removeFromLeft (editorWidth));
            row.removeFromLeft (scaled (3));
            positionUnits[i].setBounds (row.removeFromLeft (unitWidth));
            row.removeFromLeft (scaled (25));
            offsetLabels[i].setBounds (row.removeFromLeft (labelWidth));
            offsetEditors[i].setBounds (row.removeFromLeft (editorWidth));
            row.removeFromLeft (scaled (3));
            offsetUnits[i].setBounds (row.removeFromLeft (unitWidth));
            col1.removeFromTop (scaled (5));
        }

        // =====================================================================
        // Column 2: Effect Feed
        // =====================================================================
        feedTitle.setBounds (col2.removeFromTop (titleHeight));
        col2.removeFromTop (spacing);

        {
            const int ddDialSize = scaled (90);
            const int ddDialMargin = scaled (20);
            const int sliderGroupHeight = 3 * (rowHeight + scaled (3) + sliderHeight) + 2 * spacing;
            const int dialGroupHeight = rowHeight + ddDialSize + rowHeight;
            const int dialTopOffset = (sliderGroupHeight - dialGroupHeight) / 2;

            auto col2Full = col2;

            auto dialColumn = col2.removeFromRight (ddDialSize + ddDialMargin);
            dialColumn.removeFromTop (juce::jmax (0, dialTopOffset));
            orientationLabel.setBounds (dialColumn.removeFromTop (rowHeight));
            directionalDial.setBounds (dialColumn.removeFromTop (ddDialSize).withSizeKeepingCentre (ddDialSize, ddDialSize));
            orientationValue.setBounds (dialColumn.removeFromTop (rowHeight));

            row = col2.removeFromTop (rowHeight);
            angleOnLabel.setBounds (row.removeFromLeft (labelWidth));
            angleOnValue.setBounds (row.removeFromRight (valueWidth));
            col2.removeFromTop (scaled (3));
            angleOnSlider.setBounds (col2.removeFromTop (sliderHeight));
            col2.removeFromTop (spacing);

            row = col2.removeFromTop (rowHeight);
            angleOffLabel.setBounds (row.removeFromLeft (labelWidth));
            angleOffValue.setBounds (row.removeFromRight (valueWidth));
            col2.removeFromTop (scaled (3));
            angleOffSlider.setBounds (col2.removeFromTop (sliderHeight));
            col2.removeFromTop (spacing);

            row = col2.removeFromTop (rowHeight);
            pitchLabel.setBounds (row.removeFromLeft (labelWidth));
            pitchValue.setBounds (row.removeFromRight (valueWidth));
            col2.removeFromTop (scaled (3));
            pitchSlider.setBounds (col2.removeFromTop (sliderHeight));
            col2.removeFromTop (spacing);

            col2 = col2Full;
            col2.removeFromTop (titleHeight + spacing + sliderGroupHeight + spacing);
        }

        row = col2.removeFromTop (rowHeight);
        hfDampingLabel.setBounds (row.removeFromLeft (labelWidth));
        hfDampingValue.setBounds (row.removeFromRight (valueWidth));
        col2.removeFromTop (scaled (3));
        hfDampingSlider.setBounds (col2.removeFromTop (sliderHeight));
        col2.removeFromTop (spacing);

        row = col2.removeFromTop (rowHeight);
        distanceAttenPercentLabel.setBounds (row.removeFromLeft (labelWidth));
        distanceAttenPercentValue.setBounds (row.removeFromRight (valueWidth));
        col2.removeFromTop (scaled (3));
        distanceAttenPercentSlider.setBounds (col2.removeFromTop (sliderHeight));
        col2.removeFromTop (spacing);

        feedMiniLatencyButton.setBounds (col2.removeFromTop (rowHeight));

        // =====================================================================
        // Column 3: Effect Return
        // =====================================================================
        returnTitle.setBounds (col3.removeFromTop (titleHeight));
        col3.removeFromTop (spacing);

        attenuationLawButton.setBounds (col3.removeFromTop (rowHeight).removeFromLeft (scaled (200)));
        col3.removeFromTop (spacing);

        {
            const int halfColWidth = col3.getWidth() / 2;
            auto dialsRow = col3.removeFromTop (dialSize + rowHeight * 2 + spacing);

            auto leftArea = dialsRow.removeFromLeft (halfColWidth);
            distanceAttenLabel.setBounds (leftArea.removeFromTop (rowHeight));
            distanceAttenDial.setBounds (leftArea.removeFromTop (dialSize).withSizeKeepingCentre (dialSize, dialSize));
            distanceAttenValue.setBounds (leftArea.removeFromTop (rowHeight));
            // The ratio dial shares the slot: only one of the two is visible,
            // so the column does not reflow when the law changes.
            distanceRatioLabel.setBounds (distanceAttenLabel.getBounds());
            distanceRatioDial.setBounds (distanceAttenDial.getBounds());
            distanceRatioValue.setBounds (distanceAttenValue.getBounds());

            auto rightArea = dialsRow;
            commonAttenLabel.setBounds (rightArea.removeFromTop (rowHeight));
            commonAttenDial.setBounds (rightArea.removeFromTop (dialSize).withSizeKeepingCentre (dialSize, dialSize));
            commonAttenValue.setBounds (rightArea.removeFromTop (rowHeight));
        }
        col3.removeFromTop (spacing);

        row = col3.removeFromTop (rowHeight);
        hfShelfLabel.setBounds (row.removeFromLeft (labelWidth));
        hfShelfValue.setBounds (row.removeFromRight (valueWidth));
        col3.removeFromTop (scaled (3));
        hfShelfSlider.setBounds (col3.removeFromTop (sliderHeight));
        col3.removeFromTop (spacing);

        muteMacroLabel.setBounds (col3.removeFromTop (rowHeight));
        muteMacroCombo.setBounds (col3.removeFromTop (rowHeight));
        col3.removeFromTop (spacing);

        outputMutesLabel.setBounds (col3.removeFromTop (rowHeight));
        layoutMuteGrid (col3);
        col3.removeFromTop (spacing);

        muteReverbSendsButton.setBounds (col3.removeFromTop (rowHeight));
        col3.removeFromTop (spacing);

        arrayAttenLabel.setBounds (col3.removeFromTop (rowHeight));
        {
            auto attenRow = col3.removeFromTop (scaled (14) + scaled (40) + scaled (14));
            const int slot = juce::jmax (scaled (28), attenRow.getWidth() / 10);
            // Not `small`: <windows.h> defines it as a macro for `char`.
            const int dialPx = juce::jmin (slot - 2, scaled (40));
            for (int i = 0; i < 10; ++i)
            {
                auto cell = attenRow.removeFromLeft (slot);
                arrayAttenLabels[i].setBounds (cell.removeFromTop (scaled (14)));
                arrayAttenDials[i].setBounds (cell.removeFromTop (dialPx).withSizeKeepingCentre (dialPx, dialPx));
                arrayAttenValues[i].setBounds (cell.removeFromTop (scaled (14)));
            }
        }
    }

private:
    //==========================================================================
    // The link row
    //==========================================================================

    void setupLinkRow()
    {
        addAndMakeVisible (linkGroupCombo);
        linkGroupCombo.addItem (LOC ("effects.link.unlinked"), 1);
        for (int g = 1; g <= WFSParameterDefaults::effectLinkGroupMax; ++g)
            linkGroupCombo.addItem (groupName (g), g + 1);
        linkGroupCombo.onChange = [this]
        {
            ctx.write (WFSParameterIDs::effectLinkGroup, linkGroupCombo.getSelectedId() - 1);
            updateLinkModeEnabled();
        };

        addAndMakeVisible (linkModeCombo);
        linkModeCombo.addItem (LOC ("effects.link.modeOff"), 1);
        linkModeCombo.addItem (LOC ("effects.link.modeAbsolute"), 2);
        linkModeCombo.addItem (LOC ("effects.link.modeRelative"), 3);
        linkModeCombo.onChange = [this]
        {
            ctx.write (WFSParameterIDs::effectLinkMode, linkModeCombo.getSelectedId() - 1);
        };

        // A group mute WRITES every member once and leaves each one
        // independently editable afterwards (R5-2). It is not a link.
        addAndMakeVisible (groupMuteButton);
        groupMuteButton.setButtonText (LOC ("effects.buttons.groupMute"));
        groupMuteButton.onLongPress = [this]
        {
            const int group = ctx.readInt (WFSParameterIDs::effectLinkGroup, 0);
            if (group == 0)
            {
                ctx.showStatusMessage (LOC ("effects.messages.groupMuteNeedsGroup"));
                return;
            }

            // Not a self-write: the tab must reload so its header Mute button
            // shows what the group mute just did to this channel.
            groupMuted = ! groupMuted;
            ctx.parameters.getValueTreeState().setEffectGroupMute (group, groupMuted);
        };

        ctx.helpTextMap[&linkGroupCombo]  = LOC ("effects.help.linkGroup");
        ctx.helpTextMap[&linkModeCombo]   = LOC ("effects.help.linkMode");
        ctx.helpTextMap[&groupMuteButton] = LOC ("effects.help.groupMute");
    }

    void updateLinkModeEnabled()
    {
        // A mode with no group to act on is a control that does nothing, so it
        // greys out exactly as Apply-to-Array does for a Single output.
        linkModeCombo.setEnabled (linkGroupCombo.getSelectedId() > 1);
    }

    juce::String groupName (int group) const
    {
        juce::StringArray tokens;
        tokens.addTokens (ctx.parameters.getConfigParam ("effectsGlobalLinkNames").toString(), ",", "");
        const auto name = tokens[group - 1].trim();
        return name.isNotEmpty() ? name : LOC ("effects.link.group") + " " + juce::String (group);
    }

    //==========================================================================
    // Column 1
    //==========================================================================

    void setupColumn1()
    {
        addLabel (attenuationLabel, "effects.labels.attenuation");
        addValue (attenuationValue, "0.0 dB");
        attenuationSlider.setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFF4A90D9));
        attenuationSlider.onGestureStart = [this] { ctx.beginGesture ("Effect Attenuation"); };
        attenuationSlider.onValueChanged = [this] (float v)
        {
            const float minLin = std::pow (10.0f, WFSParameterDefaults::effectAttenuationMin / 20.0f);
            const float dB = 20.0f * std::log10 (minLin + (1.0f - minLin) * v * v);
            attenuationValue.setText (juce::String (dB, 1) + " dB", juce::dontSendNotification);
            ctx.write (WFSParameterIDs::effectAttenuation, dB);
        };
        addAndMakeVisible (attenuationSlider);

        addLabel (delayLatencyLabel, "effects.labels.delayLatency");
        addValue (delayLatencyValue, latencyText (0.0f));
        delayLatencySlider.setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFFD4A017));
        delayLatencySlider.onGestureStart = [this] { ctx.beginGesture ("Effect Delay/Latency"); };
        delayLatencySlider.onValueChanged = [this] (float v)
        {
            const float ms = v * WFSParameterDefaults::effectDelayLatencyMax;
            delayLatencyValue.setText (latencyText (ms), juce::dontSendNotification);
            ctx.write (WFSParameterIDs::effectDelayLatency, ms);
        };
        addAndMakeVisible (delayLatencySlider);

        addAndMakeVisible (minimalLatencyButton);
        minimalLatencyButton.setColour (juce::TextButton::textColourOnId, juce::Colours::black);
        minimalLatencyButton.onClick = [this]
        {
            const bool next = ! (ctx.readInt (WFSParameterIDs::effectMinimalLatency, 1) != 0);
            ctx.write (WFSParameterIDs::effectMinimalLatency, next ? 1 : 0);
            setLatencyButton (minimalLatencyButton, next);
        };

        addLabel (coordModeLabel, "effects.labels.coordinates");
        addAndMakeVisible (coordModeCombo);
        coordModeCombo.addItem (LOC ("effects.coordModes.xyz"), 1);
        coordModeCombo.addItem (juce::String (juce::CharPointer_UTF8 ("r \xce\xb8 Z")), 2);
        coordModeCombo.addItem (juce::String (juce::CharPointer_UTF8 ("r \xce\xb8 \xcf\x86")), 3);
        coordModeCombo.onChange = [this]
        {
            ctx.write (WFSParameterIDs::effectCoordinateMode, coordModeCombo.getSelectedId() - 1);
            loadPositionEditors();
        };

        static const char* posKeys[3] = { "effects.labels.positionX", "effects.labels.positionY", "effects.labels.positionZ" };
        static const char* offKeys[3] = { "effects.labels.returnOffsetX", "effects.labels.returnOffsetY", "effects.labels.returnOffsetZ" };

        for (int axis = 0; axis < 3; ++axis)
        {
            addLabel (positionLabels[axis], posKeys[axis]);
            setupNumberBox (positionEditors[axis]);
            addUnit (positionUnits[axis], "m");

            addLabel (offsetLabels[axis], offKeys[axis]);
            setupNumberBox (offsetEditors[axis]);
            addUnit (offsetUnits[axis], "m");
        }
    }

    /** Cartesian shows X/Y/Z; the polar modes relabel the boxes the way the
        input and reverb tabs do. The stored triple is always Cartesian. */
    void loadPositionEditors()
    {
        using namespace WFSParameterIDs;

        const int mode = ctx.readInt (effectCoordinateMode, 0);
        static const char* cartesian[3] = { "effects.labels.positionX", "effects.labels.positionY", "effects.labels.positionZ" };
        static const char* cylindrical[3] = { "effects.labels.positionR", "effects.labels.positionTheta", "effects.labels.positionZ" };
        static const char* spherical[3] = { "effects.labels.positionR", "effects.labels.positionTheta", "effects.labels.positionPhi" };
        const char** keys = mode == 1 ? cylindrical : (mode == 2 ? spherical : cartesian);
        static const char* unitsCyl[3] = { "m", "\xc2\xb0", "m" };
        static const char* unitsSph[3] = { "m", "\xc2\xb0", "\xc2\xb0" };
        static const char* unitsCart[3] = { "m", "m", "m" };
        const char** units = mode == 1 ? unitsCyl : (mode == 2 ? unitsSph : unitsCart);

        const juce::Identifier* ids[3] = { &effectPositionX, &effectPositionY, &effectPositionZ };
        for (int axis = 0; axis < 3; ++axis)
        {
            positionLabels[axis].setText (LOC (keys[axis]), juce::dontSendNotification);
            positionUnits[axis].setText (juce::String::fromUTF8 (units[axis]), juce::dontSendNotification);
            positionEditors[axis].setText (juce::String (ctx.readFloat (*ids[axis], 0.0f), 2), false);
        }

        const juce::Identifier* offs[3] = { &effectReturnOffsetX, &effectReturnOffsetY, &effectReturnOffsetZ };
        for (int axis = 0; axis < 3; ++axis)
            offsetEditors[axis].setText (juce::String (ctx.readFloat (*offs[axis], 0.0f), 2), false);
    }

    //==========================================================================
    // Column 2: Effect Feed
    //==========================================================================

    void setupFeedColumn()
    {
        addTitle (feedTitle, "effects.sections.effectFeed");

        // ONE DIAL FOR THE CONE, exactly as the reverb tab does it: the feed
        // cone of an effect is the same three parameters as a reverb node's,
        // so it gets the same control and the same colours. The sliders stay
        // as a numeric way in, and dial and slider write through each other.
        addLabel (orientationLabel, "effects.labels.orientation");
        orientationLabel.setJustificationType (juce::Justification::centred);

        directionalDial.onGestureStart = [this] { ctx.beginGesture ("Effect Directional"); };
        directionalDial.onOrientationChanged = [this] (float angle)
        {
            orientationValue.setText (juce::String (static_cast<int> (angle)) + " " + degreeSign(), juce::dontSendNotification);
            ctx.write (WFSParameterIDs::effectOrientation, juce::roundToInt (angle));
        };
        directionalDial.onAngleOnChanged = [this] (int degrees) { applyAngleOn (degrees, true); };
        directionalDial.onAngleOffChanged = [this] (int degrees) { applyAngleOff (degrees, true); };
        addAndMakeVisible (directionalDial);

        addValue (orientationValue, "0 " + degreeSign());
        orientationValue.setJustificationType (juce::Justification::centred);

        addLabel (angleOnLabel, "effects.labels.angleOn");
        addValue (angleOnValue, "86" + degreeSign());
        angleOnSlider.setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFF4CAF50));  // Green to match dial
        angleOnSlider.onGestureStart = [this] { ctx.beginGesture ("Effect Angle On"); };
        angleOnSlider.onValueChanged = [this] (float v)
        {
            applyAngleOn (juce::roundToInt (denormalise (v, WFSParameterDefaults::effectAngleOnMin,
                                                            WFSParameterDefaults::effectAngleOnMax)), false);
        };
        addAndMakeVisible (angleOnSlider);

        addLabel (angleOffLabel, "effects.labels.angleOff");
        addValue (angleOffValue, "90" + degreeSign());
        angleOffSlider.setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFFE53935));  // Red to match dial
        angleOffSlider.onGestureStart = [this] { ctx.beginGesture ("Effect Angle Off"); };
        angleOffSlider.onValueChanged = [this] (float v)
        {
            applyAngleOff (juce::roundToInt (denormalise (v, WFSParameterDefaults::effectAngleOffMin,
                                                             WFSParameterDefaults::effectAngleOffMax)), false);
        };
        addAndMakeVisible (angleOffSlider);

        addLabel (pitchLabel, "effects.labels.pitch");
        addValue (pitchValue, "0" + degreeSign());
        pitchSlider.setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFF26A69A));
        pitchSlider.onGestureStart = [this] { ctx.beginGesture ("Effect Pitch"); };
        pitchSlider.onValueChanged = [this] (float v)
        {
            const int deg = juce::roundToInt (v * WFSParameterDefaults::effectPitchMax);
            pitchValue.setText (juce::String (deg) + degreeSign(), juce::dontSendNotification);
            ctx.write (WFSParameterIDs::effectPitch, deg);
        };
        addAndMakeVisible (pitchSlider);

        addLabel (hfDampingLabel, "effects.labels.hfDamping");
        addValue (hfDampingValue, "0.0 dB/m");
        hfDampingSlider.setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFFE07878));
        hfDampingSlider.onGestureStart = [this] { ctx.beginGesture ("Effect HF Damping"); };
        hfDampingSlider.onValueChanged = [this] (float v)
        {
            const float db = denormalise (v, WFSParameterDefaults::effectHFdampingMin,
                                             WFSParameterDefaults::effectHFdampingMax);
            hfDampingValue.setText (juce::String (db, 1) + " dB/m", juce::dontSendNotification);
            ctx.write (WFSParameterIDs::effectHFdamping, db);
        };
        addAndMakeVisible (hfDampingSlider);

        addLabel (distanceAttenPercentLabel, "effects.labels.distanceAttenPercent");
        addValue (distanceAttenPercentValue, "100%");
        distanceAttenPercentSlider.setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFF4A90D9));
        distanceAttenPercentSlider.onGestureStart = [this] { ctx.beginGesture ("Effect Distance Atten %"); };
        distanceAttenPercentSlider.onValueChanged = [this] (float v)
        {
            const int pct = juce::roundToInt (denormalise (v, WFSParameterDefaults::effectDistanceAttenPercentMin,
                                                              WFSParameterDefaults::effectDistanceAttenPercentMax));
            distanceAttenPercentValue.setText (juce::String (pct) + "%", juce::dontSendNotification);
            ctx.write (WFSParameterIDs::effectDistanceAttenPercent, pct);
        };
        addAndMakeVisible (distanceAttenPercentSlider);

        addAndMakeVisible (feedMiniLatencyButton);
        feedMiniLatencyButton.setColour (juce::TextButton::textColourOnId, juce::Colours::black);
        feedMiniLatencyButton.onClick = [this]
        {
            const bool next = ! (ctx.readInt (WFSParameterIDs::effectFeedMiniLatency, 1) != 0);
            ctx.write (WFSParameterIDs::effectFeedMiniLatency, next ? 1 : 0);
            setLatencyButton (feedMiniLatencyButton, next);
        };
    }

    /** angleOn and angleOff share one cone, so the pair is constrained to
        180 degrees and either one pushes the other out of the way - the same
        rule, enforced from both sides, that the reverb tab applies. */
    void applyAngleOn (int degrees, bool fromDial)
    {
        namespace D = WFSParameterDefaults;
        degrees = juce::jlimit (D::effectAngleOnMin, D::effectAngleOnMax, degrees);

        if (! fromDial)
            directionalDial.setAngleOn (degrees);

        angleOnSlider.setValue (normalise (static_cast<float> (degrees),
                                           static_cast<float> (D::effectAngleOnMin),
                                           static_cast<float> (D::effectAngleOnMax)));
        angleOnValue.setText (juce::String (degrees) + degreeSign(), juce::dontSendNotification);
        ctx.write (WFSParameterIDs::effectAngleOn, degrees);

        const int angleOff = directionalDial.getAngleOff();
        if (degrees + angleOff > 180)
            applyAngleOff (180 - degrees, false);
    }

    void applyAngleOff (int degrees, bool fromDial)
    {
        namespace D = WFSParameterDefaults;
        degrees = juce::jlimit (D::effectAngleOffMin, D::effectAngleOffMax, degrees);

        if (! fromDial)
            directionalDial.setAngleOff (degrees);

        angleOffSlider.setValue (normalise (static_cast<float> (degrees),
                                            static_cast<float> (D::effectAngleOffMin),
                                            static_cast<float> (D::effectAngleOffMax)));
        angleOffValue.setText (juce::String (degrees) + degreeSign(), juce::dontSendNotification);
        ctx.write (WFSParameterIDs::effectAngleOff, degrees);

        const int angleOn = directionalDial.getAngleOn();
        if (angleOn + degrees > 180)
            applyAngleOn (180 - degrees, false);
    }

    void updateAngleLabels()
    {
        angleOnValue.setText (juce::String (ctx.readInt (WFSParameterIDs::effectAngleOn, 86)) + degreeSign(),
                              juce::dontSendNotification);
        angleOffValue.setText (juce::String (ctx.readInt (WFSParameterIDs::effectAngleOff, 90)) + degreeSign(),
                               juce::dontSendNotification);
        pitchValue.setText (juce::String (ctx.readInt (WFSParameterIDs::effectPitch, 0)) + degreeSign(),
                            juce::dontSendNotification);
    }

    //==========================================================================
    // Column 3: Effect Return
    //==========================================================================

    void setupReturnColumn()
    {
        addTitle (returnTitle, "effects.sections.effectReturn");

        addAndMakeVisible (attenuationLawButton);
        attenuationLawButton.onClick = [this]
        {
            const bool next = ! (ctx.readInt (WFSParameterIDs::effectAttenuationLaw, 0) != 0);
            ctx.write (WFSParameterIDs::effectAttenuationLaw, next ? 1 : 0);
            setToggle (attenuationLawButton, next, "effects.toggles.lawLog", "effects.toggles.lawInverse");
            updateAttenuationLawVisibility();
        };

        setupDial (distanceAttenDial, distanceAttenLabel, distanceAttenValue,
                   "effects.labels.distanceAtten",
                   WFSParameterDefaults::effectDistanceAttenuationMin,
                   WFSParameterDefaults::effectDistanceAttenuationMax,
                   [this] (float v)
                   {
                       distanceAttenValue.setText (juce::String (v, 1) + " dB/m", juce::dontSendNotification);
                       ctx.write (WFSParameterIDs::effectDistanceAttenuation, v);
                   }, "Effect Distance Attenuation");

        setupDial (distanceRatioDial, distanceRatioLabel, distanceRatioValue,
                   "effects.labels.distanceRatio",
                   WFSParameterDefaults::effectDistanceRatioMin,
                   WFSParameterDefaults::effectDistanceRatioMax,
                   [this] (float v)
                   {
                       distanceRatioValue.setText (juce::String (v, 2) + " x", juce::dontSendNotification);
                       ctx.write (WFSParameterIDs::effectDistanceRatio, v);
                   }, "Effect Distance Ratio");

        setupDial (commonAttenDial, commonAttenLabel, commonAttenValue,
                   "effects.labels.commonAtten",
                   static_cast<float> (WFSParameterDefaults::effectCommonAttenMin),
                   static_cast<float> (WFSParameterDefaults::effectCommonAttenMax),
                   [this] (float v)
                   {
                       const int pct = juce::roundToInt (v);
                       commonAttenValue.setText (juce::String (pct) + " %", juce::dontSendNotification);
                       ctx.write (WFSParameterIDs::effectCommonAtten, pct);
                   }, "Effect Common Attenuation");

        addLabel (hfShelfLabel, "effects.labels.hfShelf");
        addValue (hfShelfValue, "-6.0 dB");
        hfShelfSlider.setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFFE07878));
        hfShelfSlider.onGestureStart = [this] { ctx.beginGesture ("Effect HF Shelf"); };
        hfShelfSlider.onValueChanged = [this] (float v)
        {
            const float db = denormalise (v, WFSParameterDefaults::effectHFshelfMin,
                                             WFSParameterDefaults::effectHFshelfMax);
            hfShelfValue.setText (juce::String (db, 1) + " dB", juce::dontSendNotification);
            ctx.write (WFSParameterIDs::effectHFshelf, db);
        };
        addAndMakeVisible (hfShelfSlider);

        // The macro combo and the grid, the reverb tab's way: a label, a
        // full-width selector that snaps back to its placeholder, and square
        // toggles in a viewport for rigs wider than one row.
        addLabel (muteMacroLabel, "effects.labels.muteMacro");
        addAndMakeVisible (muteMacroCombo);
        muteMacroCombo.addItem (LOC ("effects.muteMacros.select"),    1);
        muteMacroCombo.addItem (LOC ("effects.muteMacros.muteAll"),   2);
        muteMacroCombo.addItem (LOC ("effects.muteMacros.unmuteAll"), 3);
        muteMacroCombo.addItem (LOC ("effects.muteMacros.invert"),    4);
        muteMacroCombo.addItem (LOC ("effects.muteMacros.muteOdd"),   5);
        muteMacroCombo.addItem (LOC ("effects.muteMacros.muteEven"),  6);
        muteMacroCombo.onChange = [this]
        {
            const int macro = muteMacroCombo.getSelectedId() - 1;
            if (macro > 0)
            {
                applyMuteMacro (macro);
                saveMuteStates();
            }
            muteMacroCombo.setSelectedId (1, juce::dontSendNotification);
        };
        muteMacroCombo.setSelectedId (1, juce::dontSendNotification);

        addLabel (outputMutesLabel, "effects.labels.outputMutes");
        muteViewport.setViewedComponent (&muteHolder, false);
        muteViewport.setScrollBarsShown (true, false);
        addAndMakeVisible (muteViewport);

        for (int i = 0; i < maxMuteButtons; ++i)
        {
            auto& b = muteButtons[i];
            b.setButtonText (juce::String (i + 1));
            b.setClickingTogglesState (true);
            b.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xFFFF5722));
            b.onClick = [this] { saveMuteStates(); };
            muteHolder.addAndMakeVisible (b);
        }

        addAndMakeVisible (muteReverbSendsButton);
        muteReverbSendsButton.onClick = [this]
        {
            const bool next = ! (ctx.readInt (WFSParameterIDs::effectMuteReverbSends, 0) != 0);
            ctx.write (WFSParameterIDs::effectMuteReverbSends, next ? 1 : 0);
            setToggle (muteReverbSendsButton, next, "effects.toggles.reverbSendsUnmuted", "effects.toggles.reverbSendsMuted");
        };

        addLabel (arrayAttenLabel, "effects.labels.arrayAttenuation");

        for (int i = 0; i < 10; ++i)
        {
            const auto arrayColour = WfsColorUtilities::getArrayColor (i + 1);

            arrayAttenLabels[i].setText (LOC ("effects.labels.arrayPrefix") + " " + juce::String (i + 1),
                                         juce::dontSendNotification);
            arrayAttenLabels[i].setJustificationType (juce::Justification::centred);
            addAndMakeVisible (arrayAttenLabels[i]);

            auto& dial = arrayAttenDials[i];
            dial.setColours (juce::Colours::black, arrayColour, juce::Colours::grey);
            dial.setTrackColours (ColorScheme::get().buttonBorder, arrayColour);
            dial.onGestureStart = [this, i]
            {
                ctx.beginGesture ("Effect Array " + juce::String (i + 1) + " Attenuation");
            };
            dial.onValueChanged = [this, i] (float v)
            {
                // The same square law the input array trims use, so the two
                // families read alike at the same dial position.
                constexpr float minLinear = 0.001f;        // -60 dB
                const float linear = minLinear + v * v * (1.0f - minLinear);
                const float db = 20.0f * std::log10 (linear);
                arrayAttenValues[i].setText (juce::String (db, 1) + " dB", juce::dontSendNotification);
                ctx.write (WFSValueTreeState::getEffectArrayAttenId (i), db);
            };
            addAndMakeVisible (dial);

            arrayAttenValues[i].setText ("0.0 dB", juce::dontSendNotification);
            arrayAttenValues[i].setJustificationType (juce::Justification::centred);
            addAndMakeVisible (arrayAttenValues[i]);
        }
    }

    void updateAttenuationLawVisibility()
    {
        const bool inverse = ctx.readInt (WFSParameterIDs::effectAttenuationLaw, 0) != 0;
        distanceAttenDial.setVisible (! inverse);
        distanceAttenLabel.setVisible (! inverse);
        distanceAttenValue.setVisible (! inverse);
        distanceRatioDial.setVisible (inverse);
        distanceRatioLabel.setVisible (inverse);
        distanceRatioValue.setVisible (inverse);
    }

    void layoutMuteGrid (juce::Rectangle<int>& col3)
    {
        const int numOutputs = juce::jlimit (1, maxMuteButtons, ctx.parameters.getNumOutputChannels());
        const int perRow = juce::jmin (numOutputs, 16);
        const int rows = (numOutputs + perRow - 1) / perRow;
        const int muteSpacing = scaled (4);
        const int btnInset = scaled (2);

        // Reserve what the controls below the grid need, then let the grid
        // take the rest of the column, scrolling when the rig is taller.
        const int reservedBelow = scaled (30) * 3 + scaled (10) * 3 + scaled (68);
        const int availH = juce::jmax (scaled (40), col3.getHeight() - reservedBelow);
        const int sbThick = muteViewport.getScrollBarThickness();

        auto gridMetrics = [&] (int holderW, int& btnSize, int& btnH, int& gridH)
        {
            btnSize = (holderW - muteSpacing * (perRow - 1)) / perRow;
            const int squareH = juce::jmax (1, btnSize - btnInset * 2);
            const int maxH    = juce::jmax (squareH, static_cast<int> (btnSize * 1.8f));
            const int fillH   = juce::jmax (1, availH / rows - muteSpacing);
            btnH  = juce::jlimit (squareH, maxH, fillH);
            gridH = rows * (btnH + muteSpacing);
        };

        int btnSize = 0, btnH = 0, gridH = 0;
        gridMetrics (col3.getWidth(), btnSize, btnH, gridH);
        const bool needsScroll = gridH > availH;
        const int holderW = col3.getWidth() - (needsScroll ? sbThick : 0);
        if (needsScroll)
            gridMetrics (holderW, btnSize, btnH, gridH);

        muteViewport.setBounds (col3.removeFromTop (juce::jmin (gridH, availH)));
        muteHolder.setSize (holderW, gridH);

        for (int i = 0; i < maxMuteButtons; ++i)
        {
            const bool live = i < numOutputs;
            muteButtons[i].setVisible (live);
            if (live)
                muteButtons[i].setBounds ((i % perRow) * (btnSize + muteSpacing),
                                          (i / perRow) * (btnH + muteSpacing),
                                          btnSize, btnH);
        }
    }

    void loadMuteStates()
    {
        const int numOutputs = juce::jlimit (0, maxMuteButtons, ctx.parameters.getNumOutputChannels());
        auto section = ctx.parameters.getValueTreeState().getEffectReturnSection (ctx.slot());

        juce::StringArray tokens;
        if (section.isValid())
            tokens.addTokens (section.getProperty (WFSParameterIDs::effectMutes).toString(), ",", "");

        for (int i = 0; i < maxMuteButtons; ++i)
        {
            muteButtons[i].setVisible (i < numOutputs);
            muteButtons[i].setToggleState (i < tokens.size() && tokens[i].getIntValue() != 0,
                                           juce::dontSendNotification);
        }
    }

    void saveMuteStates()
    {
        if (ctx.isLoadingParameters)
            return;

        // Only the LIVE outputs are written: a hidden button has no output to
        // speak for, and writing it would invent a row wider than the rig.
        const int numOutputs = juce::jlimit (1, maxMuteButtons, ctx.parameters.getNumOutputChannels());
        juce::StringArray row;
        for (int i = 0; i < numOutputs; ++i)
            row.add (muteButtons[i].getToggleState() ? "1" : "0");

        ctx.write (WFSParameterIDs::effectMutes, row.joinIntoString (","));
    }

    void applyMuteMacro (int macro)
    {
        const int numOutputs = juce::jlimit (1, maxMuteButtons, ctx.parameters.getNumOutputChannels());

        for (int i = 0; i < numOutputs; ++i)
        {
            bool state = muteButtons[i].getToggleState();
            switch (macro)
            {
                case 1: state = true;           break;   // mute all
                case 2: state = false;          break;   // unmute all
                case 3: state = ! state;        break;   // invert
                case 4: state = (i % 2) == 0;   break;   // mute odd (1-based)
                case 5: state = (i % 2) == 1;   break;   // mute even (1-based)
                default: break;
            }
            muteButtons[i].setToggleState (state, juce::dontSendNotification);
        }
    }

    void loadArrayAttens()
    {
        for (int i = 0; i < 10; ++i)
        {
            const float db = ctx.readFloat (WFSValueTreeState::getEffectArrayAttenId (i), 0.0f);
            constexpr float minLinear = 0.001f;
            const float linear = std::pow (10.0f, db / 20.0f);
            const float v = std::sqrt (juce::jmax (0.0f, (linear - minLinear) / (1.0f - minLinear)));
            arrayAttenDials[i].setValue (juce::jlimit (0.0f, 1.0f, v));
            arrayAttenValues[i].setText (juce::String (db, 1) + " dB", juce::dontSendNotification);
        }

        updateArrayAttenDimming();
    }

    void updateArrayAttenDimming()
    {
        // An array no output belongs to has nothing to trim, so its dial dims
        // rather than disappearing - the rig can gain that array later.
        std::array<bool, 10> used {};
        const int numOutputs = ctx.parameters.getNumOutputChannels();
        for (int o = 0; o < numOutputs; ++o)
        {
            const int arrayNum = static_cast<int> (ctx.parameters.getOutputParam (o, "outputArray"));
            if (arrayNum >= 1 && arrayNum <= 10)
                used[static_cast<size_t> (arrayNum - 1)] = true;
        }

        for (int i = 0; i < 10; ++i)
        {
            const float alpha = used[static_cast<size_t> (i)] ? 1.0f : 0.3f;
            arrayAttenDials[i].setAlpha (alpha);
            arrayAttenLabels[i].setAlpha (alpha);
            arrayAttenValues[i].setAlpha (alpha);
        }
    }

    //==========================================================================
    // Text editors
    //==========================================================================

    void textEditorReturnKeyPressed (juce::TextEditor& editor) override { commitEditor (editor); }
    void textEditorFocusLost (juce::TextEditor& editor) override        { commitEditor (editor); }

    void commitEditor (juce::TextEditor& editor)
    {
        if (ctx.isLoadingParameters)
            return;

        using namespace WFSParameterIDs;
        const juce::Identifier* pos[3] = { &effectPositionX, &effectPositionY, &effectPositionZ };
        const juce::Identifier* off[3] = { &effectReturnOffsetX, &effectReturnOffsetY, &effectReturnOffsetZ };

        for (int axis = 0; axis < 3; ++axis)
        {
            if (&editor == &positionEditors[axis]) { ctx.write (*pos[axis], editor.getText().getFloatValue()); return; }
            if (&editor == &offsetEditors[axis])   { ctx.write (*off[axis], editor.getText().getFloatValue()); return; }
        }
    }

    //==========================================================================
    // Small helpers
    //==========================================================================

    void addTitle (juce::Label& label, const char* key)
    {
        label.setText (LOC (key), juce::dontSendNotification);
        label.setFont (juce::FontOptions().withHeight (18.0f).withStyle ("Bold"));
        addAndMakeVisible (label);
    }

    void addLabel (juce::Label& label, const char* key)
    {
        label.setText (LOC (key), juce::dontSendNotification);
        addAndMakeVisible (label);
    }

    void addValue (juce::Label& value, const juce::String& initial)
    {
        value.setText (initial, juce::dontSendNotification);
        value.setJustificationType (juce::Justification::right);
        addAndMakeVisible (value);
    }

    void addUnit (juce::Label& unit, const char* text)
    {
        unit.setText (juce::String::fromUTF8 (text), juce::dontSendNotification);
        unit.setJustificationType (juce::Justification::left);
        unit.setColour (juce::Label::textColourId, ColorScheme::get().textSecondary);
        addAndMakeVisible (unit);
    }

    void setupNumberBox (juce::TextEditor& editor)
    {
        editor.setMultiLine (false);
        editor.setInputRestrictions (10, "-0123456789.");
        editor.addListener (this);
        addAndMakeVisible (editor);
    }

    void setupDial (WfsBasicDial& dial, juce::Label& label, juce::Label& value,
                    const char* key, float min, float max,
                    std::function<void (float)> onChanged, const juce::String& gestureName)
    {
        label.setText (LOC (key), juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (label);

        dial.setRange (min, max);
        dial.setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFF4A90D9));
        dial.onGestureStart = [this, gestureName] { ctx.beginGesture (gestureName); };
        dial.onValueChanged = std::move (onChanged);
        addAndMakeVisible (dial);

        value.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (value);
    }

    void setToggle (juce::TextButton& button, bool state, const char* offKey, const char* onKey)
    {
        button.setButtonText (state ? LOC (onKey) : LOC (offKey));
        button.setColour (juce::TextButton::buttonColourId,
                          state ? juce::Colour (0xFF3A6EA5) : ColorScheme::get().buttonNormal);
    }

    /** The reverb tab's minimal-latency switch: gold with black text when
        allowed, track-grey when excluded. */
    void setLatencyButton (juce::TextButton& button, bool allowed)
    {
        button.setToggleState (allowed, juce::dontSendNotification);
        button.setButtonText (allowed ? LOC ("effects.toggles.minLatencyOn")
                                      : LOC ("effects.toggles.minLatencyOff"));
        const auto colour = allowed ? juce::Colour (0xFFD4A017) : ColorScheme::get().sliderTrackBg;
        button.setColour (juce::TextButton::buttonColourId, colour);
        button.setColour (juce::TextButton::buttonOnColourId, colour);
    }

    static juce::String latencyText (float ms)
    {
        // The reverb tab names which of the two it is: a negative trim is
        // latency taken away, a positive one is delay added.
        return (ms < 0.0f ? LOC ("effects.labels.latencyPrefix") : LOC ("effects.labels.delayPrefix"))
             + " " + juce::String (std::abs (ms), 1) + " ms";
    }

    void setSliderFromDb (WfsStandardSlider& slider, juce::Label& value, float db, float minDb)
    {
        const float minLin = std::pow (10.0f, minDb / 20.0f);
        const float lin = std::pow (10.0f, db / 20.0f);
        slider.setValue (juce::jlimit (0.0f, 1.0f,
                                       std::sqrt (juce::jmax (0.0f, (lin - minLin) / (1.0f - minLin)))));
        value.setText (juce::String (db, 1) + " dB", juce::dontSendNotification);
    }

    template <typename SliderType>
    void setSliderLinear (SliderType& slider, juce::Label& value, float v, float min, float max,
                          const juce::String& unit, int decimals)
    {
        slider.setValue (normalise (v, min, max));
        value.setText (juce::String (v, decimals) + unit, juce::dontSendNotification);
    }

    static juce::String degreeSign() { return juce::String::fromUTF8 ("\xc2\xb0"); }

    static float normalise (float v, float min, float max)
    {
        return max > min ? juce::jlimit (0.0f, 1.0f, (v - min) / (max - min)) : 0.0f;
    }

    static float denormalise (float v, float min, float max)
    {
        return min + juce::jlimit (0.0f, 1.0f, v) * (max - min);
    }

    int scaled (int ref) const
    {
        return juce::jmax (static_cast<int> (ref * 0.65f), static_cast<int> (ref * layoutScale));
    }

    //==========================================================================
    static constexpr int maxMuteButtons = WFSParameterDefaults::maxOutputChannels;

    EffectsTabContext& ctx;
    float layoutScale = 1.0f;
    int contentTop = 0;
    int columnDividerX1 = 0;
    int columnDividerX2 = 0;
    bool groupMuted = false;

    // The link row
    juce::ComboBox linkGroupCombo, linkModeCombo;
    LongPressButton groupMuteButton { 800 };

    // Column 1
    juce::Label attenuationLabel, attenuationValue, delayLatencyLabel, delayLatencyValue;
    WfsStandardSlider attenuationSlider;
    WfsBidirectionalSlider delayLatencySlider;
    juce::TextButton minimalLatencyButton;
    juce::Label coordModeLabel;
    juce::ComboBox coordModeCombo;
    juce::Label positionLabels[3], positionUnits[3], offsetLabels[3], offsetUnits[3];
    juce::TextEditor positionEditors[3], offsetEditors[3];

    // Column 2: Effect Feed
    juce::Label feedTitle, orientationLabel, orientationValue;
    WfsDirectionalDial directionalDial;
    juce::Label angleOnLabel, angleOnValue, angleOffLabel, angleOffValue, pitchLabel, pitchValue;
    WfsWidthExpansionSlider angleOnSlider, angleOffSlider;
    WfsBidirectionalSlider pitchSlider;
    juce::Label hfDampingLabel, hfDampingValue;
    WfsStandardSlider hfDampingSlider;
    juce::Label distanceAttenPercentLabel, distanceAttenPercentValue;
    WfsStandardSlider distanceAttenPercentSlider;
    juce::TextButton feedMiniLatencyButton;

    // Column 3: Effect Return
    juce::Label returnTitle;
    juce::TextButton attenuationLawButton;
    juce::Label distanceAttenLabel, distanceAttenValue, distanceRatioLabel, distanceRatioValue;
    WfsBasicDial distanceAttenDial, distanceRatioDial;
    juce::Label commonAttenLabel, commonAttenValue;
    WfsBasicDial commonAttenDial;
    juce::Label hfShelfLabel, hfShelfValue;
    WfsStandardSlider hfShelfSlider;
    juce::Label muteMacroLabel, outputMutesLabel;
    juce::ComboBox muteMacroCombo;
    juce::Viewport muteViewport;
    juce::Component muteHolder;
    juce::TextButton muteButtons[maxMuteButtons];
    juce::TextButton muteReverbSendsButton;
    juce::Label arrayAttenLabel;
    juce::Label arrayAttenLabels[10], arrayAttenValues[10];
    WfsBasicDial arrayAttenDials[10];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectsChannelPanel)
};
