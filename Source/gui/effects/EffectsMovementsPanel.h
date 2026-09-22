#pragma once

#include <JuceHeader.h>
#include "EffectsTabContext.h"
#include "../ColorScheme.h"
#include "../TriangleIndicator.h"
#include "../buttons/TransportButtons.h"
#include "../dials/WfsBasicDial.h"
#include "../dials/WfsRotationDial.h"
#include "../dials/WfsLFOIndicators.h"
#include "../sliders/WfsStandardSlider.h"
#include "../sliders/WfsBidirectionalSlider.h"
#include "../../Automation/AutomOtionProcessor.h"
#include "../../Helpers/CoordinateConverter.h"
#include "../../Accessibility/TTSManager.h"
#include "../../Parameters/WFSParameterIDs.h"
#include "../../Parameters/WFSParameterDefaults.h"
#include "../../Localization/LocalizationManager.h"

/**
    The Movements sub-tab: LFO on the left, AutomOtion on the right, laid out
    as InputsTab::layoutMovementsTab lays out the same two blocks, so the two
    tabs read alike - same header row of toggle / period / phase / progress,
    same three axis rows, same AutomOtion rows of destination, dials, trigger
    and transport.

    BOTH MOVEMENTS ARE OFFSETS. The processors publish offsets the calculation
    engine adds to the return position (and to each other); the authored
    position is never written. That is why there is no Stay/Return here: a
    movement that ended somewhere else would move the room itself, silently
    and for good. And there is no gyrophone and no jitter: an effect return is
    an omnidirectional render source with no brightness cone to rotate, and
    the jitter is a property of the input family.
*/
class EffectsMovementsPanel : public juce::Component,
                              private juce::TextEditor::Listener
{
public:
    explicit EffectsMovementsPanel (EffectsTabContext& context) : ctx (context)
    {
        setupLfo();
        setupOtomo();
    }

    void setOtomoProcessor (AutomOtionProcessor* processor) { otomo = processor; }

    void loadParameters()
    {
        loadLfo();
        loadOtomo();
    }

    /** 50 Hz: the two trigger indicators, for the channel shown. */
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

    /** 50 Hz: the progress dial and the three output bars, for the channel shown. */
    void updateLFOIndicators (float progress, bool active, float nx, float ny, float nz)
    {
        lfoProgressDial.setProgress (progress);
        lfoProgressDial.setActive (active);
        axes[0].outSlider.setValue (nx);
        axes[1].outSlider.setValue (ny);
        axes[2].outSlider.setValue (nz);
    }

    void paint (juce::Graphics& g) override
    {
        if (columnDividerX > 0)
        {
            g.setColour (ColorScheme::get().chromeDivider);
            g.drawVerticalLine (columnDividerX, static_cast<float> (contentTop),
                                static_cast<float> (getHeight() - contentTop));
        }
    }

    void resized() override
    {
        // The Inputs tab's Movements geometry, constant for constant
        layoutScale = static_cast<float> (getHeight()) / 780.0f;

        auto area = getLocalBounds().reduced (scaled (10), scaled (6));
        contentTop = area.getY();

        const int rowHeight = scaled (28);
        const int sliderHeight = scaled (40);
        const int spacing = scaled (4);
        const int labelWidth = scaled (65);
        const int valueWidth = scaled (55);
        const int dialSize = juce::jmax (40, static_cast<int> (65.0f * layoutScale));
        const int buttonWidth = scaled (95);
        const int transportButtonSize = scaled (35);
        const int colPad = scaled (10);

        auto col1 = area.removeFromLeft (area.getWidth() / 2).reduced (colPad, 0);
        columnDividerX = area.getX();
        auto col2 = area.reduced (colPad, 0);

        // ===================================================================
        // Column 1: LFO
        // ===================================================================

        // Header row: toggle, period dial, phase dial, progress dial. Four
        // sections where the input has five - there is no gyrophone here.
        const int headerDialSize = dialSize;
        const int headerLabelHeight = scaled (16);
        const int headerValueHeight = scaled (16);
        const int headerRowHeight = headerLabelHeight + headerDialSize + headerValueHeight;
        auto headerRow = col1.removeFromTop (headerRowHeight);

        const int toggleWidth = scaled (90);
        const int dialBlockWidth = headerDialSize + 15;
        const int headerSpacing = juce::jmax (spacing, (headerRow.getWidth() - toggleWidth - 3 * dialBlockWidth) / 4);
        const int uiCenterY = headerLabelHeight + headerDialSize / 2;

        auto toggleArea = headerRow.removeFromLeft (toggleWidth);
        lfoActiveButton.setBounds (toggleArea.getX(), headerRow.getY() + uiCenterY - rowHeight / 2,
                                   toggleWidth, rowHeight);
        headerRow.removeFromLeft (headerSpacing);

        auto periodArea = headerRow.removeFromLeft (dialBlockWidth);
        lfoPeriodLabel.setBounds (periodArea.removeFromTop (headerLabelHeight));
        auto periodDialBounds = periodArea.removeFromTop (headerDialSize);
        lfoPeriodDial.setBounds (periodDialBounds.withSizeKeepingCentre (headerDialSize, headerDialSize));
        layoutDialValueUnit (lfoPeriodValue, lfoPeriodUnit, periodDialBounds.getCentreX(),
                             periodArea.getY(), periodArea.getHeight(), scaled (55), scaled (25));
        headerRow.removeFromLeft (headerSpacing);

        auto phaseArea = headerRow.removeFromLeft (dialBlockWidth);
        lfoPhaseLabel.setBounds (phaseArea.removeFromTop (headerLabelHeight));
        auto phaseDialBounds = phaseArea.removeFromTop (headerDialSize);
        lfoPhaseDial.setBounds (phaseDialBounds.withSizeKeepingCentre (headerDialSize, headerDialSize));
        layoutDialValueUnit (lfoPhaseValue, lfoPhaseUnit, phaseDialBounds.getCentreX(),
                             phaseArea.getY(), phaseArea.getHeight(), scaled (35), scaled (20));
        headerRow.removeFromLeft (headerSpacing);

        auto progressArea = headerRow.removeFromLeft (dialBlockWidth);
        progressArea.removeFromTop (headerLabelHeight);
        lfoProgressDial.setBounds (progressArea.removeFromTop (headerDialSize)
                                       .withSizeKeepingCentre (headerDialSize, headerDialSize));

        col1.removeFromTop (spacing);

        // Axis rows. One row is:
        //   [Shape label] [Shape combo]                 [Phase: label]
        //   [Amp label]   [=== slider ===] [value]      [phase dial spanning]
        //                 [=== output viz ===]
        //   [Rate label]  [=== slider ===] [value]
        const int vizHeight    = scaled (16);
        const int vizPad       = scaled (2);
        const int sliderLabelW = scaled (75);
        const int phaseValueH  = scaled (16);
        const int sliderStackH = sliderHeight * 2 + vizHeight + vizPad * 2;
        const int phaseDialSize = sliderStackH;
        const int axisRowHeight = rowHeight + spacing + sliderStackH + phaseValueH;
        const int axisRowSpacing = scaled (8);

        for (int axis = 0; axis < 3; ++axis)
        {
            auto& a = axes[axis];
            auto block = col1.removeFromTop (axisRowHeight);

            auto line1 = block.removeFromTop (rowHeight);
            a.shapeLabel.setBounds (line1.removeFromLeft (labelWidth));
            a.shapeSelector.setBounds (line1.removeFromLeft (juce::jmin (scaled (120), line1.getWidth())));
            a.phaseLabel.setBounds (line1.removeFromRight (phaseDialSize + scaled (4)));

            block.removeFromTop (spacing);

            auto phaseColumn = block.removeFromRight (phaseDialSize + scaled (4));
            phaseColumn.removeFromLeft (scaled (4));
            a.phaseDial.setBounds (phaseColumn.removeFromTop (phaseDialSize)
                                       .withSizeKeepingCentre (phaseDialSize, phaseDialSize));
            a.phaseValue.setBounds (phaseColumn.removeFromTop (phaseValueH)
                                        .withSizeKeepingCentre (phaseDialSize / 2, phaseValueH));

            auto line2 = block.removeFromTop (sliderHeight);
            a.ampLabel.setBounds (line2.removeFromLeft (sliderLabelW));
            a.ampValue.setBounds (line2.removeFromRight (valueWidth).withSizeKeepingCentre (valueWidth, scaled (24)));
            a.ampSlider.setBounds (line2);

            block.removeFromTop (vizPad);

            auto vizLine = block.removeFromTop (vizHeight);
            vizLine.removeFromLeft (sliderLabelW);
            vizLine.removeFromRight (valueWidth);
            a.outSlider.setBounds (vizLine);

            block.removeFromTop (vizPad);

            auto line3 = block.removeFromTop (sliderHeight);
            a.rateLabel.setBounds (line3.removeFromLeft (sliderLabelW));
            a.rateValue.setBounds (line3.removeFromRight (valueWidth).withSizeKeepingCentre (valueWidth, scaled (24)));
            a.rateSlider.setBounds (line3);

            if (axis < 2)
                col1.removeFromTop (axisRowSpacing);
        }

        // ===================================================================
        // Column 2: AutomOtion
        // ===================================================================
        const int otomoRowSpacing = spacing * 5;

        auto row = col2.removeFromTop (rowHeight + 4);
        otomoTitleLabel.setFont (juce::FontOptions (juce::jmax (11.0f, 16.0f * layoutScale)).withStyle ("Bold"));
        otomoTitleLabel.setBounds (row);
        col2.removeFromTop (otomoRowSpacing);

        // Row 1: [Coord ▼] [X: [__] m] [Y: [__] m] [Z: [__] m] [Absolute]
        // (the input's row ends with Stay/Return; this family has none)
        const int otomoSelectorWidth = scaled (90);
        const int otomoToggleWidth   = scaled (80);
        const int compactLabelWidth  = scaled (24);
        const int compactEditorWidth = scaled (55);
        const int compactUnitWidth   = scaled (22);
        const int row1FixedWidth = otomoSelectorWidth
                                 + (compactLabelWidth + compactEditorWidth + compactUnitWidth) * 3
                                 + otomoToggleWidth;
        const int row1Gap = juce::jmax (spacing, (col2.getWidth() - row1FixedWidth) / 5);

        row = col2.removeFromTop (rowHeight);
        coordModeSelector.setBounds (row.removeFromLeft (otomoSelectorWidth));
        row.removeFromLeft (row1Gap);
        for (int axis = 0; axis < 3; ++axis)
        {
            destLabels[axis].setBounds (row.removeFromLeft (compactLabelWidth));
            destEditors[axis].setBounds (row.removeFromLeft (compactEditorWidth));
            destUnits[axis].setBounds (row.removeFromLeft (compactUnitWidth));
            row.removeFromLeft (row1Gap);
        }
        absRelButton.setBounds (row.removeFromLeft (otomoToggleWidth));
        col2.removeFromTop (otomoRowSpacing);

        // Row 2: Duration, Curve (Cartesian only), Speed Profile
        const int otomoDialWidth = dialSize + 30;
        const int row2Gap = juce::jmax (0, (col2.getWidth() - otomoDialWidth * 3) / 4);

        auto dials1 = col2.removeFromTop (dialSize + rowHeight * 2 - 5);
        dials1.removeFromLeft (row2Gap);

        auto durArea = dials1.removeFromLeft (otomoDialWidth);
        durationLabel.setBounds (durArea.removeFromTop (rowHeight));
        durationDial.setBounds (durArea.removeFromTop (dialSize).withSizeKeepingCentre (dialSize, dialSize));
        durationValue.setBounds (durArea.removeFromTop (rowHeight));
        dials1.removeFromLeft (row2Gap);

        auto curveArea = dials1.removeFromLeft (otomoDialWidth);
        curveLabel.setBounds (curveArea.removeFromTop (rowHeight));
        auto curveDialBounds = curveArea.removeFromTop (dialSize);
        curveDial.setBounds (curveDialBounds.withSizeKeepingCentre (dialSize, dialSize));
        layoutDialValueUnit (curveValue, curveUnit, curveDialBounds.getCentreX(),
                             curveArea.getY(), rowHeight, scaled (30), scaled (25));
        dials1.removeFromLeft (row2Gap);

        auto speedArea = dials1.removeFromLeft (otomoDialWidth);
        speedProfileLabel.setBounds (speedArea.removeFromTop (rowHeight));
        auto speedDialBounds = speedArea.removeFromTop (dialSize);
        speedProfileDial.setBounds (speedDialBounds.withSizeKeepingCentre (dialSize, dialSize));
        layoutDialValueUnit (speedProfileValue, speedProfileUnit, speedDialBounds.getCentreX(),
                             speedArea.getY(), rowHeight, scaled (30), scaled (25));
        col2.removeFromTop (otomoRowSpacing);

        // Row 3: [Manual/Triggered] [Threshold dial ▲] [▼ Reset dial]
        const int triggerDialSize = scaled (50);
        const int triggerDialWidth = triggerDialSize + 30;
        const int row3Gap = juce::jmax (0, (col2.getWidth() - buttonWidth - triggerDialWidth * 2) / 4);

        auto triggerRow = col2.removeFromTop (triggerDialSize + rowHeight * 2 - 5);
        triggerRow.removeFromLeft (row3Gap);

        auto trigBtnArea = triggerRow.removeFromLeft (buttonWidth);
        triggerButton.setBounds (trigBtnArea.getX(),
                                 triggerRow.getY() + (triggerRow.getHeight() - rowHeight) / 2,
                                 buttonWidth, rowHeight);
        triggerRow.removeFromLeft (row3Gap);

        const int tri = scaled (12);

        auto threshArea = triggerRow.removeFromLeft (triggerDialWidth);
        thresholdLabel.setBounds (threshArea.removeFromTop (rowHeight));
        auto threshDialBounds = threshArea.removeFromTop (triggerDialSize);
        auto threshRect = threshDialBounds.withSizeKeepingCentre (triggerDialSize, triggerDialSize);
        thresholdDial.setBounds (threshRect);
        layoutDialValueUnit (thresholdValue, thresholdUnit, threshDialBounds.getCentreX(),
                             threshArea.getY(), rowHeight, scaled (42), scaled (30));
        triggerIndicator.setBounds (threshRect.getRight() + scaled (2), threshRect.getCentreY() - tri / 2, tri, tri);
        triggerRow.removeFromLeft (row3Gap);

        auto resetArea = triggerRow.removeFromLeft (triggerDialWidth);
        resetLabel.setBounds (resetArea.removeFromTop (rowHeight));
        auto resetDialBounds = resetArea.removeFromTop (triggerDialSize);
        auto resetRect = resetDialBounds.withSizeKeepingCentre (triggerDialSize, triggerDialSize);
        resetDial.setBounds (resetRect);
        layoutDialValueUnit (resetValue, resetUnit, resetDialBounds.getCentreX(),
                             resetArea.getY(), rowHeight, scaled (42), scaled (30));
        resetIndicator.setBounds (resetRect.getX() - tri - scaled (2), resetRect.getCentreY() - tri / 2, tri, tri);
        col2.removeFromTop (otomoRowSpacing);

        // Row 4: transport, spread across the column
        const int row4Gap = juce::jmax (spacing, (col2.getWidth() - transportButtonSize * 3 - buttonWidth * 2) / 6);
        row = col2.removeFromTop (transportButtonSize);
        row.removeFromLeft (row4Gap);
        startButton.setBounds (row.removeFromLeft (transportButtonSize));
        row.removeFromLeft (row4Gap);
        pauseButton.setBounds (row.removeFromLeft (transportButtonSize));
        row.removeFromLeft (row4Gap);
        stopButton.setBounds (row.removeFromLeft (transportButtonSize));
        row.removeFromLeft (row4Gap);
        stopAllButton.setBounds (row.removeFromLeft (buttonWidth));
        row.removeFromLeft (row4Gap);
        pauseResumeAllButton.setBounds (row.removeFromLeft (buttonWidth));
        col2.removeFromTop (otomoRowSpacing);

        // The note, where the Inputs tab keeps its help cards
        returnsHomeLabel.setBounds (col2.removeFromTop (scaled (44)));
    }

private:
    //==========================================================================
    // LFO
    //==========================================================================

    struct AxisControls
    {
        juce::Label shapeLabel, ampLabel, ampValue, rateLabel, rateValue, phaseLabel, phaseValue;
        juce::ComboBox shapeSelector;
        WfsStandardSlider ampSlider;
        WfsBidirectionalSlider rateSlider;
        WfsRotationDial phaseDial;
        WfsLFOOutputSlider outSlider;
    };

    static const juce::Identifier& shapeId (int axis)
    {
        using namespace WFSParameterIDs;
        return axis == 0 ? effectLFOshapeX : axis == 1 ? effectLFOshapeY : effectLFOshapeZ;
    }
    static const juce::Identifier& rateId (int axis)
    {
        using namespace WFSParameterIDs;
        return axis == 0 ? effectLFOrateX : axis == 1 ? effectLFOrateY : effectLFOrateZ;
    }
    static const juce::Identifier& amplitudeId (int axis)
    {
        using namespace WFSParameterIDs;
        return axis == 0 ? effectLFOamplitudeX : axis == 1 ? effectLFOamplitudeY : effectLFOamplitudeZ;
    }
    static const juce::Identifier& axisPhaseId (int axis)
    {
        using namespace WFSParameterIDs;
        return axis == 0 ? effectLFOphaseX : axis == 1 ? effectLFOphaseY : effectLFOphaseZ;
    }

    void setupLfo()
    {
        using namespace WFSParameterIDs;

        addAndMakeVisible (lfoActiveButton);
        lfoActiveButton.setButtonText (LOC ("effects.toggles.lfoOff"));
        lfoActiveButton.setClickingTogglesState (true);
        lfoActiveButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xFF26A69A));
        lfoActiveButton.onClick = [this]
        {
            const bool on = lfoActiveButton.getToggleState();
            lfoActiveButton.setButtonText (on ? LOC ("effects.toggles.lfoOn") : LOC ("effects.toggles.lfoOff"));
            ctx.write (effectLFOactive, on ? 1 : 0);
            updateLfoAlpha();
        };

        // Period dial (0.01-100 s): period = pow(10, sqrt(v)*4 - 2)
        addLabel (lfoPeriodLabel, LOC ("effects.labels.period"), juce::Justification::centred);
        lfoPeriodDial.setColours (juce::Colours::black, juce::Colour (0xFFD4A017), juce::Colours::grey);
        lfoPeriodDial.setTrackColours (juce::Colour (0xFF2D2D2D), juce::Colour (0xFFD4A017));
        lfoPeriodDial.onGestureStart = [this] { ctx.beginGesture ("Effect LFO Period"); };
        lfoPeriodDial.onValueChanged = [this] (float v)
        {
            const float period = std::pow (10.0f, std::sqrt (v) * 4.0f - 2.0f);
            lfoPeriodValue.setText (formatLfoPeriod (period), juce::dontSendNotification);
            ctx.write (effectLFOperiod, period);
        };
        addAndMakeVisible (lfoPeriodDial);
        addLabel (lfoPeriodValue, "5.00", juce::Justification::right);
        addLabel (lfoPeriodUnit, "s", juce::Justification::left);
        lfoPeriodUnit.setMinimumHorizontalScale (1.0f);

        // Global phase (-180..180)
        addLabel (lfoPhaseLabel, LOC ("effects.labels.phase"), juce::Justification::centred);
        lfoPhaseDial.setColours (juce::Colours::black, juce::Colour (0xFFE6B422), juce::Colours::grey);
        lfoPhaseDial.onGestureStart = [this] { ctx.beginGesture ("Effect LFO Phase"); };
        lfoPhaseDial.onAngleChanged = [this] (float angle)
        {
            const int degrees = static_cast<int> (angle);
            lfoPhaseValue.setText (juce::String (degrees), juce::dontSendNotification);
            ctx.write (effectLFOphase, degrees);
        };
        addAndMakeVisible (lfoPhaseDial);
        addLabel (lfoPhaseValue, "0", juce::Justification::right);
        addLabel (lfoPhaseUnit, juce::String::fromUTF8 ("\xc2\xb0"), juce::Justification::left);
        lfoPhaseUnit.setMinimumHorizontalScale (1.0f);

        // Progress (read-only)
        addAndMakeVisible (lfoProgressDial);
        lfoProgressDial.setColours (juce::Colours::black, juce::Colour (0xFF808080));

        static const char* const shapeKeys[] = { "off", "sine", "square", "sawtooth", "triangle",
                                                 "keystone", "log", "exp", "random" };
        static const char* const axisNames[] = { "X", "Y", "Z" };

        for (int axis = 0; axis < 3; ++axis)
        {
            auto& a = axes[axis];
            const juce::String ax (axisNames[axis]);

            addLabel (a.shapeLabel, LOC ("effects.labels.shape" + ax), juce::Justification::centredLeft);
            addAndMakeVisible (a.shapeSelector);
            for (int i = 0; i < 9; ++i)
                a.shapeSelector.addItem (LOC (juce::String ("effects.lfo.shapes.") + shapeKeys[i]), i + 1);
            a.shapeSelector.setSelectedId (1, juce::dontSendNotification);
            a.shapeSelector.onChange = [this, axis, ax]
            {
                auto& sel = axes[axis].shapeSelector;
                ctx.write (shapeId (axis), sel.getSelectedId() - 1);
                updateLfoAlpha();
                TTSManager::getInstance().announceValueChange ("Effect LFO Shape " + ax, sel.getText());
            };

            // Rate (0.01-100x, bidirectional): rate = pow(10, v*2), centre 1.0x
            addLabel (a.rateLabel, LOC ("effects.labels.rate" + ax), juce::Justification::centredLeft);
            a.rateSlider.setTrackColours (juce::Colour (0xFF1E1E1E), juce::Colour (0xFFD4A017));
            a.rateSlider.onGestureStart = [this, ax] { ctx.beginGesture ("Effect LFO Rate " + ax); };
            a.rateSlider.onValueChanged = [this, axis] (float v)
            {
                const float rate = std::pow (10.0f, v * 2.0f);
                axes[axis].rateValue.setText (juce::String (rate, 2) + "x", juce::dontSendNotification);
                ctx.write (rateId (axis), rate);
            };
            addAndMakeVisible (a.rateSlider);
            addLabel (a.rateValue, "1.00x", juce::Justification::right);

            // Amplitude (0-50 m)
            addLabel (a.ampLabel, LOC ("effects.labels.amplitude" + ax), juce::Justification::centredLeft);
            a.ampSlider.setTrackColours (juce::Colour (0xFF1E1E1E), juce::Colour (0xFF26A69A));
            a.ampSlider.onGestureStart = [this, ax] { ctx.beginGesture ("Effect LFO Amplitude " + ax); };
            a.ampSlider.onValueChanged = [this, axis] (float v)
            {
                const float amp = v * 50.0f;
                axes[axis].ampValue.setText (juce::String (amp, 1) + " m", juce::dontSendNotification);
                ctx.write (amplitudeId (axis), amp);
            };
            addAndMakeVisible (a.ampSlider);
            addLabel (a.ampValue, "1.0 m", juce::Justification::right);

            // Per-axis phase (-180..180)
            addLabel (a.phaseLabel, LOC ("effects.labels.phase" + ax), juce::Justification::centred);
            a.phaseDial.setColours (juce::Colour (0xFF3A3A3A), juce::Colour (0xFFE6B422), juce::Colours::white);
            a.phaseDial.onGestureStart = [this, ax] { ctx.beginGesture ("Effect LFO Phase " + ax); };
            a.phaseDial.onAngleChanged = [this, axis] (float angle)
            {
                const int degrees = static_cast<int> (std::round (angle));
                axes[axis].phaseValue.setText (juce::String (degrees) + juce::String::charToString (0x00B0),
                                               juce::dontSendNotification);
                ctx.write (axisPhaseId (axis), degrees);
            };
            addAndMakeVisible (a.phaseDial);
            addLabel (a.phaseValue, juce::String ("0") + juce::String::charToString (0x00B0), juce::Justification::centred);
            a.phaseValue.setFont (juce::FontOptions().withHeight (juce::jmax (11.0f, 15.0f * layoutScale)));

            // Output (read-only)
            addAndMakeVisible (a.outSlider);
            a.outSlider.setTrackColour (juce::Colour (0xFF808080));

            ctx.helpTextMap[&a.shapeSelector] = LOC ("effects.help.lfoShape" + ax);
            ctx.helpTextMap[&a.rateSlider]    = LOC ("effects.help.lfoRate" + ax);
            ctx.helpTextMap[&a.ampSlider]     = LOC ("effects.help.lfoAmplitude" + ax);
            ctx.helpTextMap[&a.phaseDial]     = LOC ("effects.help.lfoPhase" + ax);
        }

        ctx.helpTextMap[&lfoActiveButton] = LOC ("effects.help.lfoActive");
        ctx.helpTextMap[&lfoPeriodDial]   = LOC ("effects.help.lfoPeriod");
        ctx.helpTextMap[&lfoPhaseDial]    = LOC ("effects.help.lfoPhase");
    }

    void loadLfo()
    {
        using namespace WFSParameterIDs;
        namespace D = WFSParameterDefaults;

        const bool active = ctx.readInt (effectLFOactive, 0) != 0;
        lfoActiveButton.setToggleState (active, juce::dontSendNotification);
        lfoActiveButton.setButtonText (active ? LOC ("effects.toggles.lfoOn") : LOC ("effects.toggles.lfoOff"));

        // period = pow(10, sqrt(v)*4 - 2)  =>  v = ((log10(period) + 2) / 4)^2
        const float period = juce::jlimit (D::effectLFOperiodMin, D::effectLFOperiodMax,
                                           ctx.readFloat (effectLFOperiod, D::effectLFOperiodDefault));
        lfoPeriodDial.setValue (juce::jlimit (0.0f, 1.0f, std::pow ((std::log10 (period) + 2.0f) / 4.0f, 2.0f)));
        lfoPeriodValue.setText (formatLfoPeriod (period), juce::dontSendNotification);

        const int phase = D::wrapPhaseDegrees (ctx.readInt (effectLFOphase, 0));
        lfoPhaseDial.setAngle (static_cast<float> (phase));
        lfoPhaseValue.setText (juce::String (phase), juce::dontSendNotification);

        for (int axis = 0; axis < 3; ++axis)
        {
            auto& a = axes[axis];

            a.shapeSelector.setSelectedId (juce::jlimit (0, 8, ctx.readInt (shapeId (axis), 0)) + 1,
                                           juce::dontSendNotification);

            // rate = pow(10, v*2)  =>  v = log10(rate) / 2
            const float rate = juce::jlimit (D::effectLFOrateMin, D::effectLFOrateMax,
                                             ctx.readFloat (rateId (axis), D::effectLFOrateDefault));
            a.rateSlider.setValue (juce::jlimit (-1.0f, 1.0f, std::log10 (rate) / 2.0f));
            a.rateValue.setText (juce::String (rate, 2) + "x", juce::dontSendNotification);

            const float amp = juce::jlimit (D::effectLFOamplitudeMin, D::effectLFOamplitudeMax,
                                            ctx.readFloat (amplitudeId (axis), D::effectLFOamplitudeDefault));
            a.ampSlider.setValue (amp / 50.0f);
            a.ampValue.setText (juce::String (amp, 1) + " m", juce::dontSendNotification);

            const int axisPhase = D::wrapPhaseDegrees (ctx.readInt (axisPhaseId (axis), 0));
            a.phaseDial.setAngle (static_cast<float> (axisPhase));
            a.phaseValue.setText (juce::String (axisPhase) + juce::String::charToString (0x00B0),
                                  juce::dontSendNotification);
        }

        updateLfoAlpha();
    }

    void updateLfoAlpha()
    {
        // Dim the whole block while the LFO is off, and an axis whose shape is
        // OFF (combo id 1) on top of that - InputsTab::updateLfoAlpha.
        const bool enabled = lfoActiveButton.getToggleState();
        const float mainAlpha = enabled ? 1.0f : 0.5f;

        for (auto* c : { static_cast<juce::Component*> (&lfoPeriodLabel),
                         static_cast<juce::Component*> (&lfoPeriodDial),
                         static_cast<juce::Component*> (&lfoPeriodValue),
                         static_cast<juce::Component*> (&lfoPeriodUnit),
                         static_cast<juce::Component*> (&lfoPhaseLabel),
                         static_cast<juce::Component*> (&lfoPhaseDial),
                         static_cast<juce::Component*> (&lfoPhaseValue),
                         static_cast<juce::Component*> (&lfoPhaseUnit),
                         static_cast<juce::Component*> (&lfoProgressDial) })
            c->setAlpha (mainAlpha);

        for (auto& a : axes)
        {
            const float alpha = (enabled && a.shapeSelector.getSelectedId() != 1) ? 1.0f : 0.5f;
            for (auto* c : { static_cast<juce::Component*> (&a.shapeLabel),
                             static_cast<juce::Component*> (&a.shapeSelector),
                             static_cast<juce::Component*> (&a.ampLabel),
                             static_cast<juce::Component*> (&a.ampSlider),
                             static_cast<juce::Component*> (&a.ampValue),
                             static_cast<juce::Component*> (&a.rateLabel),
                             static_cast<juce::Component*> (&a.rateSlider),
                             static_cast<juce::Component*> (&a.rateValue),
                             static_cast<juce::Component*> (&a.phaseLabel),
                             static_cast<juce::Component*> (&a.phaseDial),
                             static_cast<juce::Component*> (&a.phaseValue),
                             static_cast<juce::Component*> (&a.outSlider) })
                c->setAlpha (alpha);
        }
    }

    static juce::String formatLfoPeriod (float seconds)
    {
        if (seconds >= 99.95f) return juce::String (juce::roundToInt (seconds));
        if (seconds >= 9.995f) return juce::String (seconds, 1);
        return juce::String (seconds, 2);
    }

    //==========================================================================
    // AutomOtion
    //==========================================================================

    void setupOtomo()
    {
        using namespace WFSParameterIDs;

        addAndMakeVisible (otomoTitleLabel);
        otomoTitleLabel.setText (LOC ("effects.sections.automOtion"), juce::dontSendNotification);
        otomoTitleLabel.setJustificationType (juce::Justification::centredLeft);

        addAndMakeVisible (coordModeSelector);
        coordModeSelector.addItem (LOC ("effects.coordModes.xyz"), 1);
        coordModeSelector.addItem (juce::String (juce::CharPointer_UTF8 ("r \xce\xb8 Z")), 2);
        coordModeSelector.addItem (juce::String (juce::CharPointer_UTF8 ("r \xce\xb8 \xcf\x86")), 3);
        coordModeSelector.setSelectedId (1, juce::dontSendNotification);
        coordModeSelector.onChange = [this]
        {
            ctx.write (effectOtomoCoordinateMode, coordModeSelector.getSelectedId() - 1);
            updateOtomoLabelsAndValues();
            updateOtomoDestinationEditors();
            updateOtomoCurveVisibility();
            resized();
        };

        for (int axis = 0; axis < 3; ++axis)
        {
            addLabel (destLabels[axis], "", juce::Justification::centredRight);
            auto& ed = destEditors[axis];
            ed.setMultiLine (false);
            ed.setInputRestrictions (10, "-0123456789.");
            ed.setJustification (juce::Justification::centredRight);
            ed.setText ("0.00", false);
            ed.addListener (this);
            addAndMakeVisible (ed);
            addLabel (destUnits[axis], "", juce::Justification::centredLeft);
        }

        addAndMakeVisible (absRelButton);
        absRelButton.setButtonText (LOC ("effects.toggles.absolute"));
        absRelButton.setClickingTogglesState (true);
        absRelButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xFF26A69A));
        absRelButton.onClick = [this]
        {
            const bool relative = absRelButton.getToggleState();
            absRelButton.setButtonText (relative ? LOC ("effects.toggles.relative") : LOC ("effects.toggles.absolute"));
            ctx.write (effectOtomoAbsoluteRelative, relative ? 1 : 0);
        };

        // Duration (0.1-3600 s, log): duration = pow(10, sqrt(v)*3.556 - 1)
        addLabel (durationLabel, LOC ("effects.labels.duration"), juce::Justification::centred);
        durationDial.setColours (juce::Colours::black, juce::Colour (0xFFD4A017), juce::Colours::grey);
        durationDial.setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFFD4A017));
        durationDial.onGestureStart = [this] { ctx.beginGesture ("Effect AutomOtion Duration"); };
        durationDial.onValueChanged = [this] (float v)
        {
            const float duration = juce::jlimit (0.1f, 3600.0f, std::pow (10.0f, std::sqrt (v) * 3.556f - 1.0f));
            durationValue.setText (formatDuration (duration), juce::dontSendNotification);
            ctx.write (effectOtomoDuration, duration);
        };
        addAndMakeVisible (durationDial);
        addLabel (durationValue, "5.00 s", juce::Justification::centred);

        // Curve (-100..+100, bipolar)
        addLabel (curveLabel, LOC ("effects.labels.curve"), juce::Justification::centred);
        curveDial.setColours (juce::Colours::black, juce::Colour (0xFF26A69A), juce::Colours::grey);
        curveDial.setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFF26A69A));
        curveDial.setBipolar (true);
        curveDial.onGestureStart = [this] { ctx.beginGesture ("Effect AutomOtion Curve"); };
        curveDial.onValueChanged = [this] (float v)
        {
            const int curve = static_cast<int> ((v * 200.0f) - 100.0f);
            curveValue.setText (juce::String (curve), juce::dontSendNotification);
            ctx.write (effectOtomoCurve, curve);
        };
        addAndMakeVisible (curveDial);
        addLabel (curveValue, "0", juce::Justification::right);
        addLabel (curveUnit, "%", juce::Justification::left);
        curveUnit.setMinimumHorizontalScale (1.0f);

        // Speed profile (0-100 %)
        addLabel (speedProfileLabel, LOC ("effects.labels.speedProfile"), juce::Justification::centred);
        speedProfileDial.setColours (juce::Colours::black, juce::Colour (0xFF00ACC1), juce::Colours::grey);
        speedProfileDial.setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFF00ACC1));
        speedProfileDial.onGestureStart = [this] { ctx.beginGesture ("Effect AutomOtion Speed Profile"); };
        speedProfileDial.onValueChanged = [this] (float v)
        {
            const int percent = static_cast<int> (v * 100.0f);
            speedProfileValue.setText (juce::String (percent), juce::dontSendNotification);
            ctx.write (effectOtomoSpeedProfile, percent);
        };
        addAndMakeVisible (speedProfileDial);
        addLabel (speedProfileValue, "0", juce::Justification::right);
        addLabel (speedProfileUnit, "%", juce::Justification::left);
        speedProfileUnit.setMinimumHorizontalScale (1.0f);

        // Trigger (Manual / Triggered)
        addAndMakeVisible (triggerButton);
        triggerButton.setButtonText (LOC ("effects.toggles.manual"));
        triggerButton.setClickingTogglesState (true);
        triggerButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xFF2E7D32));
        triggerButton.onClick = [this]
        {
            const bool triggered = triggerButton.getToggleState();
            triggerButton.setButtonText (triggered ? LOC ("effects.toggles.triggered") : LOC ("effects.toggles.manual"));
            ctx.write (effectOtomoTrigger, triggered ? 1 : 0);
            updateOtomoTriggerAppearance();
        };

        // Threshold and Reset (-92..0 dB, squared law)
        addLabel (thresholdLabel, LOC ("effects.labels.threshold"), juce::Justification::centred);
        thresholdDial.setColours (juce::Colours::black, juce::Colour (0xFF2E7D32), juce::Colours::grey);
        thresholdDial.setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFF2E7D32));
        thresholdDial.onGestureStart = [this] { ctx.beginGesture ("Effect AutomOtion Threshold"); };
        thresholdDial.onValueChanged = [this] (float v)
        {
            const float dB = levelFromDial (v);
            thresholdValue.setText (juce::String (dB, 1), juce::dontSendNotification);
            ctx.write (effectOtomoThreshold, dB);
        };
        addAndMakeVisible (thresholdDial);
        addLabel (thresholdValue, "-20.0", juce::Justification::right);
        addLabel (thresholdUnit, "dB", juce::Justification::left);
        thresholdUnit.setMinimumHorizontalScale (1.0f);

        addLabel (resetLabel, LOC ("effects.labels.reset"), juce::Justification::centred);
        resetDial.setColours (juce::Colours::black, juce::Colour (0xFF2E7D32), juce::Colours::grey);
        resetDial.setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFF2E7D32));
        resetDial.onGestureStart = [this] { ctx.beginGesture ("Effect AutomOtion Reset"); };
        resetDial.onValueChanged = [this] (float v)
        {
            const float dB = levelFromDial (v);
            resetValue.setText (juce::String (dB, 1), juce::dontSendNotification);
            ctx.write (effectOtomoReset, dB);
        };
        addAndMakeVisible (resetDial);
        addLabel (resetValue, "-60.0", juce::Justification::right);
        addLabel (resetUnit, "dB", juce::Justification::left);
        resetUnit.setMinimumHorizontalScale (1.0f);

        addAndMakeVisible (triggerIndicator);
        addAndMakeVisible (resetIndicator);

        // Transport. Start and Stop are the PROCESSOR's, not the tree's: a
        // movement is an act. The only tree property behind the row is the
        // pause latch.
        addAndMakeVisible (startButton);
        startButton.onClick = [this] { if (otomo != nullptr) otomo->startMotion (ctx.slot()); };

        addAndMakeVisible (stopButton);
        stopButton.onClick = [this] { if (otomo != nullptr) otomo->stopMotion (ctx.slot()); };

        addAndMakeVisible (pauseButton);
        pauseButton.setClickingTogglesState (true);
        pauseButton.onClick = [this]
        {
            const bool paused = pauseButton.getToggleState();
            if (otomo != nullptr)
            {
                if (paused) otomo->pauseMotion (ctx.slot());
                else        otomo->resumeMotion (ctx.slot());
            }
            ctx.write (effectOtomoPauseResume, paused ? 0 : 1);
        };

        addAndMakeVisible (stopAllButton);
        stopAllButton.setButtonText (LOC ("effects.buttons.stopAll"));
        stopAllButton.onClick = [this] { if (otomo != nullptr) otomo->stopAllMotion(); };

        addAndMakeVisible (pauseResumeAllButton);
        pauseResumeAllButton.setButtonText (LOC ("effects.buttons.pauseAll"));
        pauseResumeAllButton.setClickingTogglesState (true);
        pauseResumeAllButton.onClick = [this]
        {
            if (otomo == nullptr)
                return;

            if (pauseResumeAllButton.getToggleState())
            {
                otomo->pauseAllMotion();
                pauseResumeAllButton.setButtonText (LOC ("effects.buttons.resumeAll"));
            }
            else
            {
                otomo->resumeAllMotion();
                pauseResumeAllButton.setButtonText (LOC ("effects.buttons.pauseAll"));
            }
        };

        addAndMakeVisible (returnsHomeLabel);
        returnsHomeLabel.setText (LOC ("effects.labels.returnsHome"), juce::dontSendNotification);
        returnsHomeLabel.setColour (juce::Label::textColourId, ColorScheme::get().textDisabled);
        returnsHomeLabel.setJustificationType (juce::Justification::topLeft);

        ctx.helpTextMap[&coordModeSelector]    = LOC ("effects.help.otomoCoordMode");
        ctx.helpTextMap[&absRelButton]         = LOC ("effects.help.otomoAbsRel");
        ctx.helpTextMap[&durationDial]         = LOC ("effects.help.otomoDuration");
        ctx.helpTextMap[&curveDial]            = LOC ("effects.help.otomoCurve");
        ctx.helpTextMap[&speedProfileDial]     = LOC ("effects.help.otomoSpeedProfile");
        ctx.helpTextMap[&triggerButton]        = LOC ("effects.help.otomoTrigger");
        ctx.helpTextMap[&thresholdDial]        = LOC ("effects.help.otomoThreshold");
        ctx.helpTextMap[&resetDial]            = LOC ("effects.help.otomoReset");
        ctx.helpTextMap[&startButton]          = LOC ("effects.help.otomoStart");
        ctx.helpTextMap[&stopButton]           = LOC ("effects.help.otomoStop");
        ctx.helpTextMap[&pauseButton]          = LOC ("effects.help.otomoPause");
        ctx.helpTextMap[&stopAllButton]        = LOC ("effects.help.otomoStopAll");
        ctx.helpTextMap[&pauseResumeAllButton] = LOC ("effects.help.otomoPauseResumeAll");

        updateOtomoLabelsAndValues();
    }

    void loadOtomo()
    {
        using namespace WFSParameterIDs;
        namespace D = WFSParameterDefaults;

        coordModeSelector.setSelectedId (juce::jlimit (0, 2, ctx.readInt (effectOtomoCoordinateMode, 0)) + 1,
                                         juce::dontSendNotification);
        updateOtomoLabelsAndValues();
        updateOtomoDestinationEditors();

        const bool relative = ctx.readInt (effectOtomoAbsoluteRelative, 0) != 0;
        absRelButton.setToggleState (relative, juce::dontSendNotification);
        absRelButton.setButtonText (relative ? LOC ("effects.toggles.relative") : LOC ("effects.toggles.absolute"));

        // duration = pow(10, sqrt(v)*3.556 - 1)  =>  v = ((log10(d) + 1) / 3.556)^2
        const float duration = juce::jlimit (0.1f, 3600.0f, ctx.readFloat (effectOtomoDuration, D::effectOtomoDurationDefault));
        durationDial.setValue (juce::jlimit (0.0f, 1.0f, std::pow ((std::log10 (duration) + 1.0f) / 3.556f, 2.0f)));
        durationValue.setText (formatDuration (duration), juce::dontSendNotification);

        const int curve = juce::jlimit (-100, 100, ctx.readInt (effectOtomoCurve, D::effectOtomoCurveDefault));
        curveDial.setValue ((curve + 100) / 200.0f);
        curveValue.setText (juce::String (curve), juce::dontSendNotification);

        const int profile = juce::jlimit (0, 100, ctx.readInt (effectOtomoSpeedProfile, D::effectOtomoSpeedProfileDefault));
        speedProfileDial.setValue (profile / 100.0f);
        speedProfileValue.setText (juce::String (profile), juce::dontSendNotification);

        const bool triggered = ctx.readInt (effectOtomoTrigger, 0) != 0;
        triggerButton.setToggleState (triggered, juce::dontSendNotification);
        triggerButton.setButtonText (triggered ? LOC ("effects.toggles.triggered") : LOC ("effects.toggles.manual"));

        const float threshold = ctx.readFloat (effectOtomoThreshold, D::effectOtomoThresholdDefault);
        thresholdDial.setValue (dialFromLevel (threshold));
        thresholdValue.setText (juce::String (threshold, 1), juce::dontSendNotification);

        const float reset = ctx.readFloat (effectOtomoReset, D::effectOtomoResetDefault);
        resetDial.setValue (dialFromLevel (reset));
        resetValue.setText (juce::String (reset, 1), juce::dontSendNotification);

        pauseButton.setToggleState (ctx.readInt (effectOtomoPauseResume, 1) == 0, juce::dontSendNotification);

        updateOtomoTriggerAppearance();
        updateOtomoCurveVisibility();
    }

    void updateOtomoLabelsAndValues()
    {
        const auto mode = static_cast<WFSCoordinates::Mode> (coordModeSelector.getSelectedId() - 1);

        juce::String short1, short2, short3;
        WFSCoordinates::getShortLabels (mode, short1, short2, short3);
        destLabels[0].setText (short1, juce::dontSendNotification);
        destLabels[1].setText (short2, juce::dontSendNotification);
        destLabels[2].setText (short3, juce::dontSendNotification);

        juce::String label1, label2, label3, unit1, unit2, unit3;
        WFSCoordinates::getCoordinateLabels (mode, label1, label2, label3, unit1, unit2, unit3);
        destUnits[0].setText (unit1, juce::dontSendNotification);
        destUnits[1].setText (unit2, juce::dontSendNotification);
        destUnits[2].setText (unit3, juce::dontSendNotification);

        const juce::String names[3] = { label1.trimCharactersAtEnd (":"), label2.trimCharactersAtEnd (":"), label3.trimCharactersAtEnd (":") };
        const juce::String units[3] = { unit1, unit2, unit3 };
        for (int axis = 0; axis < 3; ++axis)
            ctx.helpTextMap[&destEditors[axis]] = LOC ("effects.help.otomoDest")
                                                      .replace ("{name}", names[axis])
                                                      .replace ("{unit}", units[axis]);
    }

    /** The three destination ids the current coordinate mode edits. */
    void destinationIds (const juce::Identifier* (&ids)[3], int (&decimals)[3]) const
    {
        using namespace WFSParameterIDs;
        const int mode = coordModeSelector.getSelectedId() - 1;

        if (mode == 1)      { ids[0] = &effectOtomoR;    ids[1] = &effectOtomoTheta; ids[2] = &effectOtomoZ;   decimals[0] = 2; decimals[1] = 1; decimals[2] = 2; }
        else if (mode == 2) { ids[0] = &effectOtomoRsph; ids[1] = &effectOtomoTheta; ids[2] = &effectOtomoPhi; decimals[0] = 2; decimals[1] = 1; decimals[2] = 1; }
        else                { ids[0] = &effectOtomoX;    ids[1] = &effectOtomoY;     ids[2] = &effectOtomoZ;   decimals[0] = 2; decimals[1] = 2; decimals[2] = 2; }
    }

    void updateOtomoDestinationEditors()
    {
        const juce::Identifier* ids[3];
        int decimals[3];
        destinationIds (ids, decimals);

        for (int axis = 0; axis < 3; ++axis)
            destEditors[axis].setText (juce::String (ctx.readFloat (*ids[axis], 0.0f), decimals[axis]), false);
    }

    void updateOtomoTriggerAppearance()
    {
        // The threshold and the rearm level only mean something in trigger
        // mode, so in manual mode they dim rather than disappear.
        const float alpha = triggerButton.getToggleState() ? 1.0f : 0.4f;

        for (auto* c : { static_cast<juce::Component*> (&thresholdLabel),
                         static_cast<juce::Component*> (&thresholdDial),
                         static_cast<juce::Component*> (&thresholdValue),
                         static_cast<juce::Component*> (&thresholdUnit),
                         static_cast<juce::Component*> (&resetLabel),
                         static_cast<juce::Component*> (&resetDial),
                         static_cast<juce::Component*> (&resetValue),
                         static_cast<juce::Component*> (&resetUnit),
                         static_cast<juce::Component*> (&triggerIndicator),
                         static_cast<juce::Component*> (&resetIndicator) })
            c->setAlpha (alpha);
    }

    void updateOtomoCurveVisibility()
    {
        // A bent path only means something between two Cartesian points
        const bool cartesian = coordModeSelector.getSelectedId() == 1;
        curveLabel.setVisible (cartesian);
        curveDial.setVisible (cartesian);
        curveValue.setVisible (cartesian);
        curveUnit.setVisible (cartesian);
    }

    void textEditorReturnKeyPressed (juce::TextEditor& editor) override { commitEditor (editor); }
    void textEditorFocusLost (juce::TextEditor& editor) override        { commitEditor (editor); }

    void commitEditor (juce::TextEditor& editor)
    {
        if (ctx.isLoadingParameters)
            return;

        const juce::Identifier* ids[3];
        int decimals[3];
        destinationIds (ids, decimals);

        for (int axis = 0; axis < 3; ++axis)
            if (&editor == &destEditors[axis])
            {
                ctx.beginGesture ("Effect AutomOtion Destination");
                ctx.write (*ids[axis], editor.getText().getFloatValue());
                return;
            }
    }

    static juce::String formatDuration (float duration)
    {
        if (duration < 10.0f)   return juce::String (duration, 2) + " s";
        if (duration < 60.0f)   return juce::String (duration, 1) + " s";
        if (duration < 3600.0f) return juce::String (static_cast<int> (duration / 60)) + "m "
                                     + juce::String (static_cast<int> (duration) % 60) + "s";
        return "1h";
    }

    // The trigger dials' law: dB = 20*log10(10^(-92/20) + (1 - 10^(-92/20)) * v^2)
    static float levelFromDial (float v)
    {
        const float floor = std::pow (10.0f, -92.0f / 20.0f);
        return 20.0f * std::log10 (floor + (1.0f - floor) * v * v);
    }

    static float dialFromLevel (float dB)
    {
        const float floor = std::pow (10.0f, -92.0f / 20.0f);
        const float lin = std::pow (10.0f, juce::jlimit (-92.0f, 0.0f, dB) / 20.0f);
        return std::sqrt (juce::jlimit (0.0f, 1.0f, (lin - floor) / (1.0f - floor)));
    }

    //==========================================================================
    // Helpers
    //==========================================================================

    void addLabel (juce::Label& label, const juce::String& text, juce::Justification just)
    {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (just);
        addAndMakeVisible (label);
    }

    void layoutDialValueUnit (juce::Label& valueLabel, juce::Label& unitLabel,
                              int dialCenterX, int y, int height, int valueWidth, int unitWidth)
    {
        const int overlap = scaled (7);
        const int totalWidth = valueWidth + unitWidth - overlap;
        const int startX = dialCenterX - totalWidth / 2;
        valueLabel.setBounds (startX, y, valueWidth, height);
        valueLabel.setJustificationType (juce::Justification::right);
        unitLabel.setBounds (startX + valueWidth - overlap, y, unitWidth, height);
        unitLabel.setJustificationType (juce::Justification::left);
    }

    int scaled (int ref) const
    {
        return juce::jmax (static_cast<int> (ref * 0.65f), static_cast<int> (ref * layoutScale));
    }

    //==========================================================================
    EffectsTabContext& ctx;
    AutomOtionProcessor* otomo = nullptr;
    float layoutScale = 1.0f;
    int columnDividerX = 0;
    int contentTop = 0;

    // LFO
    juce::TextButton lfoActiveButton;
    juce::Label lfoPeriodLabel, lfoPeriodValue, lfoPeriodUnit;
    WfsBasicDial lfoPeriodDial;
    juce::Label lfoPhaseLabel, lfoPhaseValue, lfoPhaseUnit;
    WfsRotationDial lfoPhaseDial;
    WfsLFOProgressDial lfoProgressDial;
    AxisControls axes[3];

    // AutomOtion
    juce::Label otomoTitleLabel, returnsHomeLabel;
    juce::ComboBox coordModeSelector;
    juce::Label destLabels[3], destUnits[3];
    juce::TextEditor destEditors[3];
    juce::TextButton absRelButton, triggerButton;
    juce::Label durationLabel, durationValue;
    juce::Label curveLabel, curveValue, curveUnit;
    juce::Label speedProfileLabel, speedProfileValue, speedProfileUnit;
    juce::Label thresholdLabel, thresholdValue, thresholdUnit;
    juce::Label resetLabel, resetValue, resetUnit;
    WfsBasicDial durationDial, curveDial, speedProfileDial, thresholdDial, resetDial;
    TriangleIndicator triggerIndicator { TriangleIndicator::Up,   juce::Colour (0xFF4CAF50) };
    TriangleIndicator resetIndicator   { TriangleIndicator::Down, juce::Colour (0xFF42A5F5) };
    PlayButton startButton;
    StopButton stopButton;
    PauseButton pauseButton;
    juce::TextButton stopAllButton, pauseResumeAllButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectsMovementsPanel)
};
