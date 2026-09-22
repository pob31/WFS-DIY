#pragma once

#include <JuceHeader.h>
#include "EffectsTabContext.h"
#include "../ColorScheme.h"
#include "../ColorUtilities.h"
#include "../TriangleIndicator.h"
#include "../dials/WfsBasicDial.h"
#include "../dials/WfsDirectionalDial.h"
#include "../sliders/WfsStandardSlider.h"
#include "../sliders/WfsBidirectionalSlider.h"
#include "../sliders/WfsWidthExpansionSlider.h"
#include "../../Automation/AutomOtionProcessor.h"
#include "../../Parameters/WFSParameterIDs.h"
#include "../../Parameters/WFSParameterDefaults.h"
#include "../../Localization/LocalizationManager.h"

/**
    The Channel sub-tab: everything about one effects channel that is not its
    sends and not its chain. Four blocks, laid out in three columns plus a row:

      Channel   what the return costs and when it arrives
      Position  where the feed listens and where the return is heard
      Feed      the cone every source is fed through
      Return    the distance law, the per-output mutes and the per-ARRAY trims

    TWO POSITIONS, ONE CHANNEL. The feed listens at the base position and the
    return is heard at base + offset, which is why the offset has its own three
    boxes and is not folded into the position. AutomOtion then moves the RETURN
    by a further offset the engine adds and this panel never writes: an effect
    return always comes home, because the authored position is where the
    operator put that room in the show.

    LEVEL 3 IS DELIBERATELY SMALLER THAN LEVELS 1 AND 2. The sends grid is a
    true mixer; this is not. A return is a WFS render source, so its per-output
    gains are solved from geometry - the operator gets a per-output MUTE and a
    per-ARRAY trim on top of that solution, and nothing else, because an
    arbitrary per-output level would overwrite the spatialisation that makes
    the return localise where its marker sits.
*/
class EffectsChannelPanel : public juce::Component,
                            private juce::TextEditor::Listener
{
public:
    explicit EffectsChannelPanel (EffectsTabContext& context) : ctx (context)
    {
        setupChannelBlock();
        setupPositionBlock();
        setupFeedBlock();
        setupReturnBlock();
        setupAutomOtionBlock();
    }

    void setOtomoProcessor (AutomOtionProcessor* processor) { otomo = processor; }

    /** Called from the tab's loadChannelParameters, inside its loading scope. */
    void loadParameters()
    {
        using namespace WFSParameterIDs;
        namespace D = WFSParameterDefaults;

        setSliderFromDb (attenuationSlider, attenuationValue,
                         ctx.readFloat (effectAttenuation, D::effectAttenuationDefault),
                         D::effectAttenuationMin);

        const float latency = ctx.readFloat (effectDelayLatency, D::effectDelayLatencyDefault);
        delayLatencySlider.setValue (latency / D::effectDelayLatencyMax);
        delayLatencyValue.setText (juce::String (latency, 1) + " ms", juce::dontSendNotification);

        setToggle (minimalLatencyButton, ctx.readInt (effectMinimalLatency, D::effectMinimalLatencyDefault) != 0,
                   "effects.toggles.acousticPrecedence", "effects.toggles.minimalLatency");

        coordModeCombo.setSelectedId (ctx.readInt (effectCoordinateMode, 0) + 1, juce::dontSendNotification);
        loadPositionEditors();

        const int orientation = ctx.readInt (effectOrientation, D::effectOrientationDefault);
        const int angleOn  = ctx.readInt (effectAngleOn,  D::effectAngleOnDefault);
        const int angleOff = ctx.readInt (effectAngleOff, D::effectAngleOffDefault);

        directionalDial.setOrientation (static_cast<float> (orientation));
        directionalDial.setAngleOn (angleOn);
        directionalDial.setAngleOff (angleOff);
        orientationValue.setText (juce::String (orientation), juce::dontSendNotification);

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
                         D::effectHFdampingMin, D::effectHFdampingMax, " dB/m");
        setToggle (feedMiniLatencyButton, ctx.readInt (effectFeedMiniLatency, D::effectFeedMiniLatencyDefault) != 0,
                   "effects.toggles.disabled", "effects.toggles.enabled");
        setSliderLinear (distanceAttenPercentSlider, distanceAttenPercentValue,
                         static_cast<float> (ctx.readInt (effectDistanceAttenPercent, D::effectDistanceAttenPercentDefault)),
                         static_cast<float> (D::effectDistanceAttenPercentMin),
                         static_cast<float> (D::effectDistanceAttenPercentMax), " %");

        const bool inverseLaw = ctx.readInt (effectAttenuationLaw, D::effectAttenuationLawDefault) != 0;
        setToggle (attenuationLawButton, inverseLaw, "effects.toggles.lawLog", "effects.toggles.lawInverse");
        distanceAttenDial.setValue (ctx.readFloat (effectDistanceAttenuation, D::effectDistanceAttenuationDefault));
        distanceRatioDial.setValue (ctx.readFloat (effectDistanceRatio, D::effectDistanceRatioDefault));
        updateAttenuationLawVisibility();

        commonAttenDial.setValue (static_cast<float> (ctx.readInt (effectCommonAtten, D::effectCommonAttenDefault)));
        setSliderLinear (hfShelfSlider, hfShelfValue,
                         ctx.readFloat (effectHFshelf, D::effectHFshelfDefault),
                         D::effectHFshelfMin, D::effectHFshelfMax, " dB");
        setToggle (muteReverbSendsButton, ctx.readInt (effectMuteReverbSends, D::effectMuteReverbSendsDefault) != 0,
                   "effects.toggles.off", "effects.toggles.on");

        loadMuteStates();
        loadArrayAttens();
        loadAutomOtion();
    }

    /** The output count moved: the mute grid is one toggle per LIVE output and
        the array dials dim for arrays no output belongs to. */
    void refreshOutputDependentControls()
    {
        loadMuteStates();
        updateArrayAttenDimming();
        resized();
    }

    void updateOtomoLevelIndicators (float shortPeakDb, float rmsDb)
    {
        using namespace WFSParameterIDs;

        if (ctx.readInt (effectOtomoTrigger, 0) == 0)
            return;                        // manual: the indicators mean nothing

        if (shortPeakDb > ctx.readFloat (effectOtomoThreshold, -40.0f))
            otomoTriggerIndicator.setActive (true);

        if (rmsDb < ctx.readFloat (effectOtomoReset, -60.0f))
            otomoResetIndicator.setActive (true);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (ColorScheme::get().surfaceCard);
        for (const auto& r : blockBounds)
            g.fillRoundedRectangle (r.toFloat(), 4.0f);
    }

    void resized() override
    {
        layoutScale = static_cast<float> (getHeight()) / 640.0f;
        auto area = getLocalBounds().reduced (scaled (4));
        blockBounds.clear();

        auto top = area.removeFromTop (area.getHeight() * 3 / 5);
        area.removeFromTop (scaled (6));

        const int colWidth = (top.getWidth() - scaled (12)) / 3;
        auto col1 = top.removeFromLeft (colWidth);
        top.removeFromLeft (scaled (6));
        auto col2 = top.removeFromLeft (colWidth);
        top.removeFromLeft (scaled (6));
        auto col3 = top;

        blockBounds.push_back (col1);
        blockBounds.push_back (col2);
        blockBounds.push_back (col3);
        blockBounds.push_back (area);

        layoutChannelAndPosition (col1.reduced (scaled (6)));
        layoutFeed (col2.reduced (scaled (6)));
        layoutReturn (col3.reduced (scaled (6)));
        layoutAutomOtion (area.reduced (scaled (6)));
    }

private:
    //==========================================================================
    // Channel
    //==========================================================================

    void setupChannelBlock()
    {
        addHeader (channelHeader, "effects.sections.channel");

        addRow (attenuationLabel, "effects.labels.attenuation", attenuationValue);
        attenuationSlider.setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFF4A90D9));
        attenuationSlider.onGestureStart = [this] { ctx.beginGesture ("Effect Attenuation"); };
        attenuationSlider.onValueChanged = [this] (float v)
        {
            // The square law every dB fader in this app uses, so the bottom of
            // the travel is -92 dB rather than -inf.
            const float minLin = std::pow (10.0f, WFSParameterDefaults::effectAttenuationMin / 20.0f);
            const float dB = 20.0f * std::log10 (minLin + (1.0f - minLin) * v * v);
            attenuationValue.setText (juce::String (dB, 1) + " dB", juce::dontSendNotification);
            ctx.write (WFSParameterIDs::effectAttenuation, dB);
        };
        addAndMakeVisible (attenuationSlider);

        addRow (delayLatencyLabel, "effects.labels.delayLatency", delayLatencyValue);
        delayLatencySlider.setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFFD4A017));
        delayLatencySlider.onGestureStart = [this] { ctx.beginGesture ("Effect Delay/Latency"); };
        delayLatencySlider.onValueChanged = [this] (float v)
        {
            const float ms = v * WFSParameterDefaults::effectDelayLatencyMax;
            delayLatencyValue.setText (juce::String (ms, 1) + " ms", juce::dontSendNotification);
            ctx.write (WFSParameterIDs::effectDelayLatency, ms);
        };
        addAndMakeVisible (delayLatencySlider);

        addAndMakeVisible (minimalLatencyButton);
        minimalLatencyButton.onClick = [this]
        {
            const bool next = ! (ctx.readInt (WFSParameterIDs::effectMinimalLatency, 1) != 0);
            ctx.write (WFSParameterIDs::effectMinimalLatency, next ? 1 : 0);
            setToggle (minimalLatencyButton, next, "effects.toggles.acousticPrecedence", "effects.toggles.minimalLatency");
        };
    }

    //==========================================================================
    // Position
    //==========================================================================

    void setupPositionBlock()
    {
        addHeader (positionHeader, "effects.sections.position");

        addAndMakeVisible (coordModeCombo);
        coordModeCombo.addItem (LOC ("effects.coordModes.cartesian"), 1);
        coordModeCombo.addItem (LOC ("effects.coordModes.cylindrical"), 2);
        coordModeCombo.addItem (LOC ("effects.coordModes.spherical"), 3);
        coordModeCombo.onChange = [this]
        {
            ctx.write (WFSParameterIDs::effectCoordinateMode, coordModeCombo.getSelectedId() - 1);
            loadPositionEditors();
        };

        for (int axis = 0; axis < 3; ++axis)
        {
            addAndMakeVisible (positionLabels[axis]);
            positionLabels[axis].setJustificationType (juce::Justification::centredRight);

            auto& ed = positionEditors[axis];
            ed.setMultiLine (false);
            ed.setInputRestrictions (10, "-0123456789.");
            ed.addListener (this);
            addAndMakeVisible (ed);

            addAndMakeVisible (offsetLabels[axis]);
            offsetLabels[axis].setJustificationType (juce::Justification::centredRight);
            offsetLabels[axis].setText (LOC ("effects.labels.returnOffset") + " "
                                        + juce::String ("XYZ").substring (axis, axis + 1),
                                        juce::dontSendNotification);

            auto& oe = offsetEditors[axis];
            oe.setMultiLine (false);
            oe.setInputRestrictions (10, "-0123456789.");
            oe.addListener (this);
            addAndMakeVisible (oe);
        }
    }

    /** Cartesian writes XYZ; the other two modes write the polar triple, which
        the state converts, exactly as the input family does. */
    void loadPositionEditors()
    {
        using namespace WFSParameterIDs;

        const int mode = ctx.readInt (effectCoordinateMode, 0);
        static const char* cartesian[3] = { "X", "Y", "Z" };
        static const char* cylindrical[3] = { "r", "theta", "Z" };
        static const char* spherical[3] = { "r", "theta", "phi" };
        const char** names = mode == 1 ? cylindrical : (mode == 2 ? spherical : cartesian);

        const juce::Identifier* ids[3] = { &effectPositionX, &effectPositionY, &effectPositionZ };

        for (int axis = 0; axis < 3; ++axis)
        {
            positionLabels[axis].setText (juce::String (names[axis]), juce::dontSendNotification);
            positionEditors[axis].setText (juce::String (ctx.readFloat (*ids[axis], 0.0f), 2), false);
        }

        const juce::Identifier* offs[3] = { &effectReturnOffsetX, &effectReturnOffsetY, &effectReturnOffsetZ };
        for (int axis = 0; axis < 3; ++axis)
            offsetEditors[axis].setText (juce::String (ctx.readFloat (*offs[axis], 0.0f), 2), false);
    }

    //==========================================================================
    // Feed
    //==========================================================================

    void setupFeedBlock()
    {
        addHeader (feedHeader, "effects.sections.feed");

        // ONE DIAL FOR THE CONE, exactly as the reverb tab does it: the feed
        // cone of an effect is the same three parameters as a reverb node's,
        // so it gets the same control and the same colours rather than a
        // rotation dial with two unrelated sliders beside it. The sliders stay
        // as a numeric way in, and dial and slider write through each other.
        addAndMakeVisible (orientationLabel);
        orientationLabel.setText (LOC ("effects.labels.orientation"), juce::dontSendNotification);
        orientationLabel.setJustificationType (juce::Justification::centred);

        directionalDial.onGestureStart = [this] { ctx.beginGesture ("Effect Directional"); };
        directionalDial.onOrientationChanged = [this] (float angle)
        {
            orientationValue.setText (juce::String (static_cast<int> (angle)), juce::dontSendNotification);
            ctx.write (WFSParameterIDs::effectOrientation, juce::roundToInt (angle));
        };
        directionalDial.onAngleOnChanged = [this] (int degrees) { applyAngleOn (degrees, true); };
        directionalDial.onAngleOffChanged = [this] (int degrees) { applyAngleOff (degrees, true); };
        addAndMakeVisible (directionalDial);

        orientationValue.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (orientationValue);

        addRow (angleOnLabel, "effects.labels.angleOn", angleOnValue);
        angleOnSlider.setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFF4CAF50));  // Green to match dial
        angleOnSlider.onGestureStart = [this] { ctx.beginGesture ("Effect Angle On"); };
        angleOnSlider.onValueChanged = [this] (float v)
        {
            applyAngleOn (juce::roundToInt (denormalise (v, WFSParameterDefaults::effectAngleOnMin,
                                                            WFSParameterDefaults::effectAngleOnMax)), false);
        };
        addAndMakeVisible (angleOnSlider);

        addRow (angleOffLabel, "effects.labels.angleOff", angleOffValue);
        angleOffSlider.setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFFE53935));  // Red to match dial
        angleOffSlider.onGestureStart = [this] { ctx.beginGesture ("Effect Angle Off"); };
        angleOffSlider.onValueChanged = [this] (float v)
        {
            applyAngleOff (juce::roundToInt (denormalise (v, WFSParameterDefaults::effectAngleOffMin,
                                                             WFSParameterDefaults::effectAngleOffMax)), false);
        };
        addAndMakeVisible (angleOffSlider);

        addRow (pitchLabel, "effects.labels.pitch", pitchValue);
        pitchSlider.setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFF26A69A));
        pitchSlider.onGestureStart = [this] { ctx.beginGesture ("Effect Pitch"); };
        pitchSlider.onValueChanged = [this] (float v)
        {
            const int deg = juce::roundToInt (v * WFSParameterDefaults::effectPitchMax);
            pitchValue.setText (juce::String (deg) + juce::String::fromUTF8 ("\xc2\xb0"), juce::dontSendNotification);
            ctx.write (WFSParameterIDs::effectPitch, deg);
        };
        addAndMakeVisible (pitchSlider);

        addRow (hfDampingLabel, "effects.labels.hfDamping", hfDampingValue);
        hfDampingSlider.setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFFE07878));
        hfDampingSlider.onGestureStart = [this] { ctx.beginGesture ("Effect HF Damping"); };
        hfDampingSlider.onValueChanged = [this] (float v)
        {
            const float db = denormalise (v, WFSParameterDefaults::effectHFdampingMin,
                                             WFSParameterDefaults::effectHFdampingMax);
            hfDampingValue.setText (juce::String (db, 2) + " dB/m", juce::dontSendNotification);
            ctx.write (WFSParameterIDs::effectHFdamping, db);
        };
        addAndMakeVisible (hfDampingSlider);

        addAndMakeVisible (feedMiniLatencyButton);
        feedMiniLatencyButton.onClick = [this]
        {
            const bool next = ! (ctx.readInt (WFSParameterIDs::effectFeedMiniLatency, 1) != 0);
            ctx.write (WFSParameterIDs::effectFeedMiniLatency, next ? 1 : 0);
            setToggle (feedMiniLatencyButton, next, "effects.toggles.disabled", "effects.toggles.enabled");
        };

        addRow (distanceAttenPercentLabel, "effects.labels.distanceAttenPercent", distanceAttenPercentValue);
        distanceAttenPercentSlider.setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFF4A90D9));
        distanceAttenPercentSlider.onGestureStart = [this] { ctx.beginGesture ("Effect Distance Atten %"); };
        distanceAttenPercentSlider.onValueChanged = [this] (float v)
        {
            const int pct = juce::roundToInt (denormalise (v, WFSParameterDefaults::effectDistanceAttenPercentMin,
                                                              WFSParameterDefaults::effectDistanceAttenPercentMax));
            distanceAttenPercentValue.setText (juce::String (pct) + " %", juce::dontSendNotification);
            ctx.write (WFSParameterIDs::effectDistanceAttenPercent, pct);
        };
        addAndMakeVisible (distanceAttenPercentSlider);
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

    static juce::String degreeSign() { return juce::String::fromUTF8 ("\xc2\xb0"); }

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
    // Return
    //==========================================================================

    void setupReturnBlock()
    {
        addHeader (returnHeader, "effects.sections.returnSection");

        addAndMakeVisible (attenuationLawButton);
        attenuationLawButton.onClick = [this]
        {
            const bool next = ! (ctx.readInt (WFSParameterIDs::effectAttenuationLaw, 0) != 0);
            ctx.write (WFSParameterIDs::effectAttenuationLaw, next ? 1 : 0);
            setToggle (attenuationLawButton, next, "effects.toggles.lawLog", "effects.toggles.lawInverse");
            updateAttenuationLawVisibility();
        };

        setupDial (distanceAttenDial, distanceAttenLabel, distanceAttenValue,
                   "effects.labels.distanceAttenuation",
                   WFSParameterDefaults::effectDistanceAttenuationMin,
                   WFSParameterDefaults::effectDistanceAttenuationMax,
                   [this] (float v)
                   {
                       distanceAttenValue.setText (juce::String (v, 2) + " dB/m", juce::dontSendNotification);
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

        addRow (hfShelfLabel, "effects.labels.hfShelf", hfShelfValue);
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

        addAndMakeVisible (muteReverbSendsButton);
        muteReverbSendsButton.onClick = [this]
        {
            const bool next = ! (ctx.readInt (WFSParameterIDs::effectMuteReverbSends, 0) != 0);
            ctx.write (WFSParameterIDs::effectMuteReverbSends, next ? 1 : 0);
            setToggle (muteReverbSendsButton, next, "effects.toggles.off", "effects.toggles.on");
        };

        // The per-output mute grid, in a viewport for the same reason the input
        // one is: 128 outputs do not fit a column.
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

        addAndMakeVisible (muteMacroCombo);
        muteMacroCombo.addItem (LOC ("effects.muteMacros.select"), 1);
        muteMacroCombo.addItem (LOC ("effects.muteMacros.muteAll"), 2);
        muteMacroCombo.addItem (LOC ("effects.muteMacros.unmuteAll"), 3);
        muteMacroCombo.addItem (LOC ("effects.muteMacros.invert"), 4);
        muteMacroCombo.addItem (LOC ("effects.muteMacros.muteOdd"), 5);
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

        addHeader (arrayAttenHeader, "effects.sections.arrayAttenuation");

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
                case 1: state = true;                    break;   // mute all
                case 2: state = false;                   break;   // unmute all
                case 3: state = ! state;                 break;   // invert
                case 4: state = (i % 2) == 0;            break;   // mute odd (1-based)
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
    // AutomOtion
    //==========================================================================

    void setupAutomOtionBlock()
    {
        addHeader (otomoHeader, "effects.sections.automOtion");

        addAndMakeVisible (otomoCoordModeCombo);
        otomoCoordModeCombo.addItem (LOC ("effects.coordModes.cartesian"), 1);
        otomoCoordModeCombo.addItem (LOC ("effects.coordModes.cylindrical"), 2);
        otomoCoordModeCombo.addItem (LOC ("effects.coordModes.spherical"), 3);
        otomoCoordModeCombo.onChange = [this]
        {
            ctx.write (WFSParameterIDs::effectOtomoCoordinateMode, otomoCoordModeCombo.getSelectedId() - 1);
        };

        for (int axis = 0; axis < 3; ++axis)
        {
            addAndMakeVisible (otomoDestLabels[axis]);
            otomoDestLabels[axis].setJustificationType (juce::Justification::centredRight);
            otomoDestLabels[axis].setText (juce::String ("XYZ").substring (axis, axis + 1),
                                           juce::dontSendNotification);

            auto& ed = otomoDestEditors[axis];
            ed.setMultiLine (false);
            ed.setInputRestrictions (10, "-0123456789.");
            ed.addListener (this);
            addAndMakeVisible (ed);
        }

        addAndMakeVisible (otomoAbsRelButton);
        otomoAbsRelButton.onClick = [this]
        {
            const bool next = ! (ctx.readInt (WFSParameterIDs::effectOtomoAbsoluteRelative, 0) != 0);
            ctx.write (WFSParameterIDs::effectOtomoAbsoluteRelative, next ? 1 : 0);
            setToggle (otomoAbsRelButton, next, "effects.toggles.absolute", "effects.toggles.relative");
        };

        setupDial (otomoDurationDial, otomoDurationLabel, otomoDurationValue, "effects.labels.duration",
                   WFSParameterDefaults::effectOtomoDurationMin, WFSParameterDefaults::effectOtomoDurationMax,
                   [this] (float v)
                   {
                       otomoDurationValue.setText (juce::String (v, 1) + " s", juce::dontSendNotification);
                       ctx.write (WFSParameterIDs::effectOtomoDuration, v);
                   }, "Effect AutomOtion Duration");

        setupDial (otomoSpeedProfileDial, otomoSpeedProfileLabel, otomoSpeedProfileValue,
                   "effects.labels.speedProfile",
                   static_cast<float> (WFSParameterDefaults::effectOtomoSpeedProfileMin),
                   static_cast<float> (WFSParameterDefaults::effectOtomoSpeedProfileMax),
                   [this] (float v)
                   {
                       const int pct = juce::roundToInt (v);
                       otomoSpeedProfileValue.setText (juce::String (pct) + " %", juce::dontSendNotification);
                       ctx.write (WFSParameterIDs::effectOtomoSpeedProfile, pct);
                   }, "Effect AutomOtion Speed Profile");

        setupDial (otomoCurveDial, otomoCurveLabel, otomoCurveValue, "effects.labels.curve",
                   static_cast<float> (WFSParameterDefaults::effectOtomoCurveMin),
                   static_cast<float> (WFSParameterDefaults::effectOtomoCurveMax),
                   [this] (float v)
                   {
                       const int amount = juce::roundToInt (v);
                       otomoCurveValue.setText (juce::String (amount), juce::dontSendNotification);
                       ctx.write (WFSParameterIDs::effectOtomoCurve, amount);
                   }, "Effect AutomOtion Curve");
        otomoCurveDial.setBipolar (true);

        addAndMakeVisible (otomoTriggerButton);
        otomoTriggerButton.onClick = [this]
        {
            const bool next = ! (ctx.readInt (WFSParameterIDs::effectOtomoTrigger, 0) != 0);
            ctx.write (WFSParameterIDs::effectOtomoTrigger, next ? 1 : 0);
            setToggle (otomoTriggerButton, next, "effects.toggles.manual", "effects.toggles.audioTrigger");
        };

        setupDial (otomoThresholdDial, otomoThresholdLabel, otomoThresholdValue, "effects.labels.threshold",
                   WFSParameterDefaults::effectOtomoThresholdMin, WFSParameterDefaults::effectOtomoThresholdMax,
                   [this] (float v)
                   {
                       otomoThresholdValue.setText (juce::String (v, 1) + " dB", juce::dontSendNotification);
                       ctx.write (WFSParameterIDs::effectOtomoThreshold, v);
                   }, "Effect AutomOtion Threshold");

        setupDial (otomoResetDial, otomoResetLabel, otomoResetValue, "effects.labels.reset",
                   WFSParameterDefaults::effectOtomoResetMin, WFSParameterDefaults::effectOtomoResetMax,
                   [this] (float v)
                   {
                       otomoResetValue.setText (juce::String (v, 1) + " dB", juce::dontSendNotification);
                       ctx.write (WFSParameterIDs::effectOtomoReset, v);
                   }, "Effect AutomOtion Reset");

        addAndMakeVisible (otomoTriggerIndicator);
        addAndMakeVisible (otomoResetIndicator);

        // Start/Stop is the processor's, not the tree's: a movement is an act,
        // and the only tree property behind it is the pause latch.
        addAndMakeVisible (otomoStartButton);
        otomoStartButton.setButtonText (LOC ("effects.buttons.otomoStart"));
        otomoStartButton.onClick = [this]
        {
            if (otomo != nullptr)
                otomo->startMotion (ctx.slot());
        };

        addAndMakeVisible (otomoStopButton);
        otomoStopButton.setButtonText (LOC ("effects.buttons.otomoStop"));
        otomoStopButton.onClick = [this]
        {
            if (otomo != nullptr)
                otomo->stopMotion (ctx.slot());
        };

        addAndMakeVisible (otomoPauseButton);
        otomoPauseButton.onClick = [this]
        {
            const bool paused = ctx.readInt (WFSParameterIDs::effectOtomoPauseResume, 1) == 0;
            ctx.write (WFSParameterIDs::effectOtomoPauseResume, paused ? 1 : 0);
            setToggle (otomoPauseButton, ! paused, "effects.toggles.resume", "effects.toggles.paused");
        };
    }

    void loadAutomOtion()
    {
        using namespace WFSParameterIDs;
        namespace D = WFSParameterDefaults;

        otomoCoordModeCombo.setSelectedId (ctx.readInt (effectOtomoCoordinateMode, 0) + 1,
                                           juce::dontSendNotification);

        const juce::Identifier* dest[3] = { &effectOtomoX, &effectOtomoY, &effectOtomoZ };
        for (int axis = 0; axis < 3; ++axis)
            otomoDestEditors[axis].setText (juce::String (ctx.readFloat (*dest[axis], 0.0f), 2), false);

        setToggle (otomoAbsRelButton, ctx.readInt (effectOtomoAbsoluteRelative, 0) != 0,
                   "effects.toggles.absolute", "effects.toggles.relative");
        otomoDurationDial.setValue (ctx.readFloat (effectOtomoDuration, D::effectOtomoDurationDefault));
        otomoSpeedProfileDial.setValue (static_cast<float> (ctx.readInt (effectOtomoSpeedProfile,
                                                                         D::effectOtomoSpeedProfileDefault)));
        otomoCurveDial.setValue (static_cast<float> (ctx.readInt (effectOtomoCurve, D::effectOtomoCurveDefault)));
        setToggle (otomoTriggerButton, ctx.readInt (effectOtomoTrigger, 0) != 0,
                   "effects.toggles.manual", "effects.toggles.audioTrigger");
        otomoThresholdDial.setValue (ctx.readFloat (effectOtomoThreshold, D::effectOtomoThresholdDefault));
        otomoResetDial.setValue (ctx.readFloat (effectOtomoReset, D::effectOtomoResetDefault));
        setToggle (otomoPauseButton, ctx.readInt (effectOtomoPauseResume, 1) != 0,
                   "effects.toggles.resume", "effects.toggles.paused");
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
        const juce::Identifier* dest[3] = { &effectOtomoX, &effectOtomoY, &effectOtomoZ };

        for (int axis = 0; axis < 3; ++axis)
        {
            if (&editor == &positionEditors[axis])   { ctx.write (*pos[axis],  editor.getText().getFloatValue()); return; }
            if (&editor == &offsetEditors[axis])     { ctx.write (*off[axis],  editor.getText().getFloatValue()); return; }
            if (&editor == &otomoDestEditors[axis])  { ctx.write (*dest[axis], editor.getText().getFloatValue()); return; }
        }
    }

    //==========================================================================
    // Layout
    //==========================================================================

    void layoutChannelAndPosition (juce::Rectangle<int> area)
    {
        const int row = scaled (20);
        const int gap = scaled (4);

        channelHeader.setBounds (area.removeFromTop (row));
        area.removeFromTop (gap);

        layoutSliderRow (area, attenuationLabel, attenuationSlider, attenuationValue);
        layoutSliderRow (area, delayLatencyLabel, delayLatencySlider, delayLatencyValue);
        minimalLatencyButton.setBounds (area.removeFromTop (row));
        area.removeFromTop (gap * 2);

        positionHeader.setBounds (area.removeFromTop (row));
        area.removeFromTop (gap);
        coordModeCombo.setBounds (area.removeFromTop (row));
        area.removeFromTop (gap);

        for (int axis = 0; axis < 3; ++axis)
        {
            auto r = area.removeFromTop (row);
            positionLabels[axis].setBounds (r.removeFromLeft (scaled (60)));
            r.removeFromLeft (gap);
            positionEditors[axis].setBounds (r.removeFromLeft (scaled (70)));
            area.removeFromTop (gap / 2);
        }

        area.removeFromTop (gap);
        for (int axis = 0; axis < 3; ++axis)
        {
            auto r = area.removeFromTop (row);
            offsetLabels[axis].setBounds (r.removeFromLeft (scaled (90)));
            r.removeFromLeft (gap);
            offsetEditors[axis].setBounds (r.removeFromLeft (scaled (70)));
            area.removeFromTop (gap / 2);
        }
    }

    void layoutFeed (juce::Rectangle<int> area)
    {
        const int row = scaled (20);
        const int gap = scaled (4);

        feedHeader.setBounds (area.removeFromTop (row));
        area.removeFromTop (gap);

        const int dialSize = scaled (86);
        orientationLabel.setBounds (area.removeFromTop (scaled (14)));
        auto dialArea = area.removeFromTop (dialSize);
        directionalDial.setBounds (dialArea.withSizeKeepingCentre (dialSize, dialSize));
        orientationValue.setBounds (area.removeFromTop (scaled (14)));
        area.removeFromTop (gap);

        layoutSliderRow (area, angleOnLabel, angleOnSlider, angleOnValue);
        layoutSliderRow (area, angleOffLabel, angleOffSlider, angleOffValue);
        layoutSliderRow (area, pitchLabel, pitchSlider, pitchValue);
        layoutSliderRow (area, hfDampingLabel, hfDampingSlider, hfDampingValue);
        feedMiniLatencyButton.setBounds (area.removeFromTop (row));
        area.removeFromTop (gap);
        layoutSliderRow (area, distanceAttenPercentLabel, distanceAttenPercentSlider, distanceAttenPercentValue);
    }

    void layoutReturn (juce::Rectangle<int> area)
    {
        const int row = scaled (20);
        const int gap = scaled (4);

        returnHeader.setBounds (area.removeFromTop (row));
        area.removeFromTop (gap);
        attenuationLawButton.setBounds (area.removeFromTop (row));
        area.removeFromTop (gap);

        const int dialSize = scaled (46);
        auto dialRow = area.removeFromTop (dialSize + row * 2);
        const int third = dialRow.getWidth() / 3;
        layoutDial (dialRow.removeFromLeft (third), distanceAttenLabel, distanceAttenDial, distanceAttenValue, dialSize);
        layoutDial (dialRow.removeFromLeft (third), distanceRatioLabel, distanceRatioDial, distanceRatioValue, dialSize);
        // Only one of the two distance controls is visible at a time; they
        // share a slot so the block does not reflow when the law changes.
        distanceRatioLabel.setBounds (distanceAttenLabel.getBounds());
        distanceRatioDial.setBounds (distanceAttenDial.getBounds());
        distanceRatioValue.setBounds (distanceAttenValue.getBounds());
        layoutDial (dialRow, commonAttenLabel, commonAttenDial, commonAttenValue, dialSize);
        area.removeFromTop (gap);

        layoutSliderRow (area, hfShelfLabel, hfShelfSlider, hfShelfValue);
        muteReverbSendsButton.setBounds (area.removeFromTop (row));
        area.removeFromTop (gap);

        muteMacroCombo.setBounds (area.removeFromTop (row));
        area.removeFromTop (gap);

        // The grid: as many columns as fit, scrolled when they do not.
        const int numOutputs = juce::jlimit (1, maxMuteButtons, ctx.parameters.getNumOutputChannels());
        const int cell = scaled (20);
        const int perRow = juce::jmax (1, juce::jmin (numOutputs, area.getWidth() / cell));
        const int rows = (numOutputs + perRow - 1) / perRow;

        auto gridArea = area.removeFromTop (juce::jmin (area.getHeight() - scaled (60), rows * cell + scaled (4)));
        muteViewport.setBounds (gridArea);
        muteHolder.setSize (gridArea.getWidth() - scaled (14), rows * cell);

        for (int i = 0; i < numOutputs; ++i)
            muteButtons[i].setBounds ((i % perRow) * cell, (i / perRow) * cell, cell - 2, cell - 2);

        area.removeFromTop (gap);
        arrayAttenHeader.setBounds (area.removeFromTop (row));

        auto attenRow = area;
        const int dialW = juce::jmax (scaled (24), attenRow.getWidth() / 10);
        const int smallDial = juce::jmin (dialW - 2, scaled (34));
        for (int i = 0; i < 10; ++i)
        {
            auto cellArea = attenRow.removeFromLeft (dialW);
            arrayAttenLabels[i].setBounds (cellArea.removeFromTop (scaled (12)));
            arrayAttenDials[i].setBounds (cellArea.removeFromTop (smallDial)
                                                  .withSizeKeepingCentre (smallDial, smallDial));
            arrayAttenValues[i].setBounds (cellArea.removeFromTop (scaled (12)));
        }
    }

    void layoutAutomOtion (juce::Rectangle<int> area)
    {
        const int row = scaled (20);
        const int gap = scaled (4);

        otomoHeader.setBounds (area.removeFromTop (row));
        area.removeFromTop (gap);

        auto top = area.removeFromTop (row);
        otomoCoordModeCombo.setBounds (top.removeFromLeft (scaled (110)));
        top.removeFromLeft (gap);
        for (int axis = 0; axis < 3; ++axis)
        {
            otomoDestLabels[axis].setBounds (top.removeFromLeft (scaled (16)));
            top.removeFromLeft (gap / 2);
            otomoDestEditors[axis].setBounds (top.removeFromLeft (scaled (60)));
            top.removeFromLeft (gap);
        }
        otomoAbsRelButton.setBounds (top.removeFromLeft (scaled (90)));
        top.removeFromLeft (gap);
        otomoStartButton.setBounds (top.removeFromLeft (scaled (70)));
        top.removeFromLeft (gap);
        otomoStopButton.setBounds (top.removeFromLeft (scaled (70)));
        top.removeFromLeft (gap);
        otomoPauseButton.setBounds (top.removeFromLeft (scaled (90)));

        area.removeFromTop (gap);

        const int dialSize = scaled (46);
        auto dialRow = area.removeFromTop (dialSize + row * 2);
        const int slot = juce::jmax (scaled (60), dialRow.getWidth() / 6);
        layoutDial (dialRow.removeFromLeft (slot), otomoDurationLabel, otomoDurationDial, otomoDurationValue, dialSize);
        layoutDial (dialRow.removeFromLeft (slot), otomoSpeedProfileLabel, otomoSpeedProfileDial, otomoSpeedProfileValue, dialSize);
        layoutDial (dialRow.removeFromLeft (slot), otomoCurveLabel, otomoCurveDial, otomoCurveValue, dialSize);
        layoutDial (dialRow.removeFromLeft (slot), otomoThresholdLabel, otomoThresholdDial, otomoThresholdValue, dialSize);
        layoutDial (dialRow.removeFromLeft (slot), otomoResetLabel, otomoResetDial, otomoResetValue, dialSize);

        auto indicators = dialRow.removeFromLeft (slot);
        otomoTriggerButton.setBounds (indicators.removeFromTop (row));
        indicators.removeFromTop (gap);
        const int tri = scaled (14);
        auto triRow = indicators.removeFromTop (tri);
        otomoTriggerIndicator.setBounds (triRow.removeFromLeft (tri));
        triRow.removeFromLeft (gap);
        otomoResetIndicator.setBounds (triRow.removeFromLeft (tri));
    }

    void layoutSliderRow (juce::Rectangle<int>& area, juce::Label& label,
                          juce::Component& slider, juce::Label& value)
    {
        const int row = scaled (18);
        auto r = area.removeFromTop (row);
        label.setBounds (r.removeFromLeft (scaled (86)));
        value.setBounds (r.removeFromRight (scaled (60)));
        slider.setBounds (r.reduced (scaled (2), 0));
        area.removeFromTop (scaled (4));
    }

    void layoutDial (juce::Rectangle<int> area, juce::Label& label,
                     juce::Component& dial, juce::Label& value, int dialSize)
    {
        label.setBounds (area.removeFromTop (scaled (14)));
        dial.setBounds (area.removeFromTop (dialSize).withSizeKeepingCentre (dialSize, dialSize));
        value.setBounds (area.removeFromTop (scaled (14)));
    }

    //==========================================================================
    // Small helpers
    //==========================================================================

    void addHeader (juce::Label& label, const char* key)
    {
        label.setText (LOC (key), juce::dontSendNotification);
        label.setFont (juce::Font (juce::FontOptions (14.0f).withStyle ("Bold")));
        label.setColour (juce::Label::textColourId, ColorScheme::get().textSecondary);
        addAndMakeVisible (label);
    }

    void addRow (juce::Label& label, const char* key, juce::Label& value)
    {
        label.setText (LOC (key), juce::dontSendNotification);
        addAndMakeVisible (label);
        value.setJustificationType (juce::Justification::right);
        addAndMakeVisible (value);
    }

    void setupDial (WfsBasicDial& dial, juce::Label& label, juce::Label& value,
                    const char* key, float min, float max,
                    std::function<void (float)> onChanged, const juce::String& gestureName)
    {
        label.setText (LOC (key), juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (label);

        dial.setRange (min, max);
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
                          const juce::String& unit)
    {
        slider.setValue (normalise (v, min, max));
        value.setText (juce::String (v, unit == " %" ? 0 : 2) + unit, juce::dontSendNotification);
    }

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
    AutomOtionProcessor* otomo = nullptr;
    float layoutScale = 1.0f;
    std::vector<juce::Rectangle<int>> blockBounds;

    // Channel
    juce::Label channelHeader, attenuationLabel, attenuationValue, delayLatencyLabel, delayLatencyValue;
    WfsStandardSlider attenuationSlider;
    WfsBidirectionalSlider delayLatencySlider;
    juce::TextButton minimalLatencyButton;

    // Position
    juce::Label positionHeader;
    juce::ComboBox coordModeCombo;
    juce::Label positionLabels[3], offsetLabels[3];
    juce::TextEditor positionEditors[3], offsetEditors[3];

    // Feed
    juce::Label feedHeader, orientationLabel, orientationValue;
    WfsDirectionalDial directionalDial;
    juce::Label angleOnLabel, angleOnValue, angleOffLabel, angleOffValue, pitchLabel, pitchValue;
    WfsWidthExpansionSlider angleOnSlider, angleOffSlider;
    WfsBidirectionalSlider pitchSlider;
    juce::Label hfDampingLabel, hfDampingValue;
    WfsStandardSlider hfDampingSlider;
    juce::TextButton feedMiniLatencyButton;
    juce::Label distanceAttenPercentLabel, distanceAttenPercentValue;
    WfsStandardSlider distanceAttenPercentSlider;

    // Return
    juce::Label returnHeader;
    juce::TextButton attenuationLawButton;
    juce::Label distanceAttenLabel, distanceAttenValue, distanceRatioLabel, distanceRatioValue;
    WfsBasicDial distanceAttenDial, distanceRatioDial;
    juce::Label commonAttenLabel, commonAttenValue;
    WfsBasicDial commonAttenDial;
    juce::Label hfShelfLabel, hfShelfValue;
    WfsStandardSlider hfShelfSlider;
    juce::TextButton muteReverbSendsButton;
    juce::Viewport muteViewport;
    juce::Component muteHolder;
    juce::TextButton muteButtons[maxMuteButtons];
    juce::ComboBox muteMacroCombo;
    juce::Label arrayAttenHeader;
    juce::Label arrayAttenLabels[10], arrayAttenValues[10];
    WfsBasicDial arrayAttenDials[10];

    // AutomOtion
    juce::Label otomoHeader;
    juce::ComboBox otomoCoordModeCombo;
    juce::Label otomoDestLabels[3];
    juce::TextEditor otomoDestEditors[3];
    juce::TextButton otomoAbsRelButton, otomoTriggerButton, otomoPauseButton;
    juce::TextButton otomoStartButton, otomoStopButton;
    juce::Label otomoDurationLabel, otomoDurationValue, otomoSpeedProfileLabel, otomoSpeedProfileValue;
    juce::Label otomoCurveLabel, otomoCurveValue, otomoThresholdLabel, otomoThresholdValue;
    juce::Label otomoResetLabel, otomoResetValue;
    WfsBasicDial otomoDurationDial, otomoSpeedProfileDial, otomoCurveDial, otomoThresholdDial, otomoResetDial;
    TriangleIndicator otomoTriggerIndicator { TriangleIndicator::Up,   juce::Colour (0xFF4CAF50) };
    TriangleIndicator otomoResetIndicator   { TriangleIndicator::Down, juce::Colour (0xFF42A5F5) };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectsChannelPanel)
};
