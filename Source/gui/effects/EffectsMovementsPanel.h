#pragma once

#include <JuceHeader.h>
#include "EffectsTabContext.h"
#include "../ColorScheme.h"
#include "../TriangleIndicator.h"
#include "../dials/WfsBasicDial.h"
#include "../../Automation/AutomOtionProcessor.h"
#include "../../Parameters/WFSParameterIDs.h"
#include "../../Parameters/WFSParameterDefaults.h"
#include "../../Localization/LocalizationManager.h"

/**
    The Movements sub-tab.

    WHERE MOVEMENT LIVES, on every family that has it. The Inputs tab keeps its
    LFO and its AutomOtion together under Movements rather than under the
    channel parameters, and an effects channel follows that: the Channel tab is
    what the return costs and where it sits, this tab is how it travels.

    ONLY AutomOtion FOR NOW. The effects schema has an <AutomOtion> node and no
    LFO parameters at all - there is no effectLFO* identifier to bind - so this
    panel holds one block where the input's holds two. The layout leaves the
    room an LFO block would take rather than centring AutomOtion in the space,
    so adding one later moves nothing that is already here.

    AN EFFECT RETURN ALWAYS COMES HOME. The family has no Stay/Return control
    because it has no such property: the processor publishes an OFFSET the
    calculation engine adds to the return position, and the authored position
    is never written. A movement that ended somewhere else would move the room
    itself, silently and for good.
*/
class EffectsMovementsPanel : public juce::Component,
                              private juce::TextEditor::Listener
{
public:
    explicit EffectsMovementsPanel (EffectsTabContext& context) : ctx (context)
    {
        addHeader (otomoHeader, "effects.sections.automOtion");

        addAndMakeVisible (coordModeCombo);
        coordModeCombo.addItem (LOC ("effects.coordModes.cartesian"), 1);
        coordModeCombo.addItem (LOC ("effects.coordModes.cylindrical"), 2);
        coordModeCombo.addItem (LOC ("effects.coordModes.spherical"), 3);
        coordModeCombo.onChange = [this]
        {
            ctx.write (WFSParameterIDs::effectOtomoCoordinateMode, coordModeCombo.getSelectedId() - 1);
        };

        addAndMakeVisible (destHeader);
        destHeader.setText (LOC ("effects.labels.destination"), juce::dontSendNotification);

        for (int axis = 0; axis < 3; ++axis)
        {
            addAndMakeVisible (destLabels[axis]);
            destLabels[axis].setJustificationType (juce::Justification::centredRight);
            destLabels[axis].setText (juce::String ("XYZ").substring (axis, axis + 1),
                                      juce::dontSendNotification);

            auto& ed = destEditors[axis];
            ed.setMultiLine (false);
            ed.setInputRestrictions (10, "-0123456789.");
            ed.addListener (this);
            addAndMakeVisible (ed);
        }

        addAndMakeVisible (absRelButton);
        absRelButton.onClick = [this]
        {
            const bool next = ! (ctx.readInt (WFSParameterIDs::effectOtomoAbsoluteRelative, 0) != 0);
            ctx.write (WFSParameterIDs::effectOtomoAbsoluteRelative, next ? 1 : 0);
            setToggle (absRelButton, next, "effects.toggles.absolute", "effects.toggles.relative");
        };

        setupDial (durationDial, durationLabel, durationValue, "effects.labels.duration",
                   WFSParameterDefaults::effectOtomoDurationMin, WFSParameterDefaults::effectOtomoDurationMax,
                   [this] (float v)
                   {
                       durationValue.setText (juce::String (v, 1) + " s", juce::dontSendNotification);
                       ctx.write (WFSParameterIDs::effectOtomoDuration, v);
                   }, "Effect AutomOtion Duration", juce::Colour (0xFFD4A017));

        setupDial (speedProfileDial, speedProfileLabel, speedProfileValue, "effects.labels.speedProfile",
                   static_cast<float> (WFSParameterDefaults::effectOtomoSpeedProfileMin),
                   static_cast<float> (WFSParameterDefaults::effectOtomoSpeedProfileMax),
                   [this] (float v)
                   {
                       const int pct = juce::roundToInt (v);
                       speedProfileValue.setText (juce::String (pct) + " %", juce::dontSendNotification);
                       ctx.write (WFSParameterIDs::effectOtomoSpeedProfile, pct);
                   }, "Effect AutomOtion Speed Profile", juce::Colour (0xFF26A69A));

        setupDial (curveDial, curveLabel, curveValue, "effects.labels.curve",
                   static_cast<float> (WFSParameterDefaults::effectOtomoCurveMin),
                   static_cast<float> (WFSParameterDefaults::effectOtomoCurveMax),
                   [this] (float v)
                   {
                       const int amount = juce::roundToInt (v);
                       curveValue.setText (juce::String (amount), juce::dontSendNotification);
                       ctx.write (WFSParameterIDs::effectOtomoCurve, amount);
                   }, "Effect AutomOtion Curve", juce::Colour (0xFF26A69A));
        curveDial.setBipolar (true);

        addAndMakeVisible (triggerButton);
        triggerButton.onClick = [this]
        {
            const bool next = ! (ctx.readInt (WFSParameterIDs::effectOtomoTrigger, 0) != 0);
            ctx.write (WFSParameterIDs::effectOtomoTrigger, next ? 1 : 0);
            setToggle (triggerButton, next, "effects.toggles.manual", "effects.toggles.audioTrigger");
            updateTriggerVisibility();
        };

        setupDial (thresholdDial, thresholdLabel, thresholdValue, "effects.labels.threshold",
                   WFSParameterDefaults::effectOtomoThresholdMin, WFSParameterDefaults::effectOtomoThresholdMax,
                   [this] (float v)
                   {
                       thresholdValue.setText (juce::String (v, 1) + " dB", juce::dontSendNotification);
                       ctx.write (WFSParameterIDs::effectOtomoThreshold, v);
                   }, "Effect AutomOtion Threshold", juce::Colour (0xFF4A90D9));

        setupDial (resetDial, resetLabel, resetValue, "effects.labels.reset",
                   WFSParameterDefaults::effectOtomoResetMin, WFSParameterDefaults::effectOtomoResetMax,
                   [this] (float v)
                   {
                       resetValue.setText (juce::String (v, 1) + " dB", juce::dontSendNotification);
                       ctx.write (WFSParameterIDs::effectOtomoReset, v);
                   }, "Effect AutomOtion Reset", juce::Colour (0xFF4A90D9));

        addAndMakeVisible (triggerIndicator);
        addAndMakeVisible (resetIndicator);

        // Start and Stop are the PROCESSOR's, not the tree's: a movement is an
        // act. The only tree property behind the block is the pause latch.
        addAndMakeVisible (startButton);
        startButton.setButtonText (LOC ("effects.buttons.otomoStart"));
        startButton.onClick = [this] { if (otomo != nullptr) otomo->startMotion (ctx.slot()); };

        addAndMakeVisible (stopButton);
        stopButton.setButtonText (LOC ("effects.buttons.otomoStop"));
        stopButton.onClick = [this] { if (otomo != nullptr) otomo->stopMotion (ctx.slot()); };

        addAndMakeVisible (pauseButton);
        pauseButton.onClick = [this]
        {
            const bool paused = ctx.readInt (WFSParameterIDs::effectOtomoPauseResume, 1) == 0;
            ctx.write (WFSParameterIDs::effectOtomoPauseResume, paused ? 1 : 0);
            setToggle (pauseButton, ! paused, "effects.toggles.resume", "effects.toggles.paused");
        };

        addAndMakeVisible (returnsHomeLabel);
        returnsHomeLabel.setText (LOC ("effects.labels.returnsHome"), juce::dontSendNotification);
        returnsHomeLabel.setColour (juce::Label::textColourId, ColorScheme::get().textDisabled);
        returnsHomeLabel.setJustificationType (juce::Justification::topLeft);
    }

    void setOtomoProcessor (AutomOtionProcessor* processor) { otomo = processor; }

    void loadParameters()
    {
        using namespace WFSParameterIDs;
        namespace D = WFSParameterDefaults;

        coordModeCombo.setSelectedId (ctx.readInt (effectOtomoCoordinateMode, 0) + 1,
                                      juce::dontSendNotification);

        const juce::Identifier* dest[3] = { &effectOtomoX, &effectOtomoY, &effectOtomoZ };
        for (int axis = 0; axis < 3; ++axis)
            destEditors[axis].setText (juce::String (ctx.readFloat (*dest[axis], 0.0f), 2), false);

        setToggle (absRelButton, ctx.readInt (effectOtomoAbsoluteRelative, 0) != 0,
                   "effects.toggles.absolute", "effects.toggles.relative");
        // The labels are set here as well as in the callbacks: a dial whose
        // loaded value equals its current one fires no callback, and a zero
        // default would otherwise leave its label blank on first show.
        const float duration = ctx.readFloat (effectOtomoDuration, D::effectOtomoDurationDefault);
        durationDial.setValue (duration);
        durationValue.setText (juce::String (duration, 1) + " s", juce::dontSendNotification);

        const int profile = ctx.readInt (effectOtomoSpeedProfile, D::effectOtomoSpeedProfileDefault);
        speedProfileDial.setValue (static_cast<float> (profile));
        speedProfileValue.setText (juce::String (profile) + " %", juce::dontSendNotification);

        const int curve = ctx.readInt (effectOtomoCurve, D::effectOtomoCurveDefault);
        curveDial.setValue (static_cast<float> (curve));
        curveValue.setText (juce::String (curve), juce::dontSendNotification);

        setToggle (triggerButton, ctx.readInt (effectOtomoTrigger, 0) != 0,
                   "effects.toggles.manual", "effects.toggles.audioTrigger");

        const float threshold = ctx.readFloat (effectOtomoThreshold, D::effectOtomoThresholdDefault);
        thresholdDial.setValue (threshold);
        thresholdValue.setText (juce::String (threshold, 1) + " dB", juce::dontSendNotification);

        const float reset = ctx.readFloat (effectOtomoReset, D::effectOtomoResetDefault);
        resetDial.setValue (reset);
        resetValue.setText (juce::String (reset, 1) + " dB", juce::dontSendNotification);
        setToggle (pauseButton, ctx.readInt (effectOtomoPauseResume, 1) != 0,
                   "effects.toggles.resume", "effects.toggles.paused");

        updateTriggerVisibility();
    }

    void updateLevelIndicators (float shortPeakDb, float rmsDb)
    {
        using namespace WFSParameterIDs;

        if (ctx.readInt (effectOtomoTrigger, 0) == 0)
            return;                        // manual: the indicators mean nothing

        if (shortPeakDb > ctx.readFloat (effectOtomoThreshold, -40.0f))
            triggerIndicator.setActive (true);

        if (rmsDb < ctx.readFloat (effectOtomoReset, -60.0f))
            resetIndicator.setActive (true);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (ColorScheme::get().surfaceCard);
        g.fillRoundedRectangle (blockBounds.toFloat(), 4.0f);
    }

    void resized() override
    {
        layoutScale = static_cast<float> (getHeight()) / 640.0f;

        auto area = getLocalBounds().reduced (scaled (4));

        // The AutomOtion block takes the left half. The right half is where an
        // LFO block would go if the effects schema ever gains one; leaving it
        // empty now means adding one later moves nothing.
        blockBounds = area.removeFromLeft (area.getWidth() / 2);

        auto inner = blockBounds.reduced (scaled (8));
        const int row = scaled (22);
        const int gap = scaled (6);

        otomoHeader.setBounds (inner.removeFromTop (row));
        inner.removeFromTop (gap);

        destHeader.setBounds (inner.removeFromTop (scaled (16)));
        auto destRow = inner.removeFromTop (row);
        coordModeCombo.setBounds (destRow.removeFromLeft (scaled (130)));
        destRow.removeFromLeft (gap);
        for (int axis = 0; axis < 3; ++axis)
        {
            destLabels[axis].setBounds (destRow.removeFromLeft (scaled (14)));
            destRow.removeFromLeft (gap / 2);
            destEditors[axis].setBounds (destRow.removeFromLeft (scaled (62)));
            destRow.removeFromLeft (gap);
        }
        inner.removeFromTop (gap);

        auto modeRow = inner.removeFromTop (row);
        absRelButton.setBounds (modeRow.removeFromLeft (scaled (100)));
        modeRow.removeFromLeft (gap);
        startButton.setBounds (modeRow.removeFromLeft (scaled (80)));
        modeRow.removeFromLeft (gap);
        stopButton.setBounds (modeRow.removeFromLeft (scaled (80)));
        modeRow.removeFromLeft (gap);
        pauseButton.setBounds (modeRow.removeFromLeft (scaled (100)));
        inner.removeFromTop (gap * 2);

        const int dialSize = scaled (54);
        auto dialRow = inner.removeFromTop (dialSize + scaled (30));
        const int slot = juce::jmax (scaled (70), dialRow.getWidth() / 3);
        layoutDial (dialRow.removeFromLeft (slot), durationLabel, durationDial, durationValue, dialSize);
        layoutDial (dialRow.removeFromLeft (slot), speedProfileLabel, speedProfileDial, speedProfileValue, dialSize);
        layoutDial (dialRow.removeFromLeft (slot), curveLabel, curveDial, curveValue, dialSize);
        inner.removeFromTop (gap * 2);

        auto trigRow = inner.removeFromTop (row);
        triggerButton.setBounds (trigRow.removeFromLeft (scaled (120)));
        trigRow.removeFromLeft (gap);
        const int tri = scaled (14);
        triggerIndicator.setBounds (trigRow.removeFromLeft (tri).withSizeKeepingCentre (tri, tri));
        trigRow.removeFromLeft (gap / 2);
        resetIndicator.setBounds (trigRow.removeFromLeft (tri).withSizeKeepingCentre (tri, tri));
        inner.removeFromTop (gap);

        auto trigDials = inner.removeFromTop (dialSize + scaled (30));
        layoutDial (trigDials.removeFromLeft (slot), thresholdLabel, thresholdDial, thresholdValue, dialSize);
        layoutDial (trigDials.removeFromLeft (slot), resetLabel, resetDial, resetValue, dialSize);
        inner.removeFromTop (gap * 2);

        returnsHomeLabel.setBounds (inner.removeFromTop (scaled (44)));
    }

private:
    void updateTriggerVisibility()
    {
        // The threshold and the rearm level only mean something in audio-trigger
        // mode, so in manual mode they dim rather than disappear.
        const bool audio = ctx.readInt (WFSParameterIDs::effectOtomoTrigger, 0) != 0;
        const float alpha = audio ? 1.0f : 0.35f;

        for (auto* c : { static_cast<juce::Component*> (&thresholdDial),
                         static_cast<juce::Component*> (&thresholdLabel),
                         static_cast<juce::Component*> (&thresholdValue),
                         static_cast<juce::Component*> (&resetDial),
                         static_cast<juce::Component*> (&resetLabel),
                         static_cast<juce::Component*> (&resetValue),
                         static_cast<juce::Component*> (&triggerIndicator),
                         static_cast<juce::Component*> (&resetIndicator) })
            c->setAlpha (alpha);
    }

    void textEditorReturnKeyPressed (juce::TextEditor& editor) override { commitEditor (editor); }
    void textEditorFocusLost (juce::TextEditor& editor) override        { commitEditor (editor); }

    void commitEditor (juce::TextEditor& editor)
    {
        if (ctx.isLoadingParameters)
            return;

        using namespace WFSParameterIDs;
        const juce::Identifier* dest[3] = { &effectOtomoX, &effectOtomoY, &effectOtomoZ };

        for (int axis = 0; axis < 3; ++axis)
            if (&editor == &destEditors[axis])
            {
                ctx.write (*dest[axis], editor.getText().getFloatValue());
                return;
            }
    }

    void addHeader (juce::Label& label, const char* key)
    {
        label.setText (LOC (key), juce::dontSendNotification);
        label.setFont (juce::Font (juce::FontOptions (15.0f).withStyle ("Bold")));
        label.setColour (juce::Label::textColourId, ColorScheme::get().textSecondary);
        addAndMakeVisible (label);
    }

    void setupDial (WfsBasicDial& dial, juce::Label& label, juce::Label& value,
                    const char* key, float min, float max,
                    std::function<void (float)> onChanged, const juce::String& gestureName,
                    juce::Colour colour)
    {
        label.setText (LOC (key), juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (label);

        dial.setRange (min, max);
        dial.setTrackColours (ColorScheme::get().sliderTrackBg, colour);
        dial.onGestureStart = [this, gestureName] { ctx.beginGesture (gestureName); };
        dial.onValueChanged = std::move (onChanged);
        addAndMakeVisible (dial);

        value.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (value);
    }

    void layoutDial (juce::Rectangle<int> area, juce::Label& label,
                     juce::Component& dial, juce::Label& value, int dialSize)
    {
        label.setBounds (area.removeFromTop (scaled (15)));
        dial.setBounds (area.removeFromTop (dialSize).withSizeKeepingCentre (dialSize, dialSize));
        value.setBounds (area.removeFromTop (scaled (15)));
    }

    void setToggle (juce::TextButton& button, bool state, const char* offKey, const char* onKey)
    {
        button.setButtonText (state ? LOC (onKey) : LOC (offKey));
        button.setColour (juce::TextButton::buttonColourId,
                          state ? juce::Colour (0xFF3A6EA5) : ColorScheme::get().buttonNormal);
    }

    int scaled (int ref) const
    {
        return juce::jmax (static_cast<int> (ref * 0.65f), static_cast<int> (ref * layoutScale));
    }

    EffectsTabContext& ctx;
    AutomOtionProcessor* otomo = nullptr;
    float layoutScale = 1.0f;
    juce::Rectangle<int> blockBounds;

    juce::Label otomoHeader, destHeader, returnsHomeLabel;
    juce::ComboBox coordModeCombo;
    juce::Label destLabels[3];
    juce::TextEditor destEditors[3];
    juce::TextButton absRelButton, triggerButton, pauseButton, startButton, stopButton;
    juce::Label durationLabel, durationValue, speedProfileLabel, speedProfileValue;
    juce::Label curveLabel, curveValue, thresholdLabel, thresholdValue, resetLabel, resetValue;
    WfsBasicDial durationDial, speedProfileDial, curveDial, thresholdDial, resetDial;
    TriangleIndicator triggerIndicator { TriangleIndicator::Up,   juce::Colour (0xFF4CAF50) };
    TriangleIndicator resetIndicator   { TriangleIndicator::Down, juce::Colour (0xFF42A5F5) };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectsMovementsPanel)
};
