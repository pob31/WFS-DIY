#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "../Localization/LocalizationManager.h"
#include "ColorScheme.h"
#include "WindowUtils.h"
#include "dials/WfsBasicDial.h"

/**
 * Set All Inputs Window
 * Allows applying parameter changes to ALL input channels at once.
 * Accessed via long-press on the "Set all Inputs..." button in InputsTab header.
 */

//==============================================================================
// Content Component
//==============================================================================
class SetAllInputsContent : public juce::Component,
                            public ColorScheme::Manager::Listener
{
public:
    SetAllInputsContent(WfsParameters& params)
        : parameters(params)
    {
        ColorScheme::Manager::getInstance().addListener(this);
        setupControls();
    }

    ~SetAllInputsContent() override
    {
        ColorScheme::Manager::getInstance().removeListener(this);
    }

    void colorSchemeChanged() override
    {
        applyTheme();
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(ColorScheme::get().background);

        // Red warning strip at top
        g.setColour(juce::Colour(0xFFCC3333));  // Red background
        g.fillRect(0, 0, getWidth(), 40);
    }

    void resized() override
    {
        const int rowHeight = 28;
        const int dialSize = 50;
        const int spacing = 10;
        const int sectionSpacing = 16;
        const int buttonWidth = 90;
        const int labelWidth = 140;
        const int margin = 20;
        const int buttonPairWidth = buttonWidth * 2 + spacing;  // 190px - width of ON+OFF buttons
        const int actionButtonWidth = buttonPairWidth;  // Match button pair width

        // Warning label (full width, centered within red strip)
        warningLabel.setBounds(0, 0, getWidth(), 40);

        // Content starts below the red strip
        auto bounds = getLocalBounds().withTrimmedTop(50).reduced(margin, 0).withTrimmedBottom(15);

        // === INPUT PROPERTIES SECTION ===
        // Minimal Latency - ON/OFF button pair
        auto row = bounds.removeFromTop(rowHeight);
        curvatureOnlyLabel.setBounds(row.removeFromLeft(labelWidth));
        curvatureOnlyOnButton.setBounds(row.removeFromLeft(buttonWidth));
        row.removeFromLeft(spacing);
        curvatureOnlyOffButton.setBounds(row.removeFromLeft(buttonWidth));
        bounds.removeFromTop(sectionSpacing);

        // === POSITION SECTION ===
        // Coordinate mode row
        row = bounds.removeFromTop(rowHeight);
        coordModeLabel.setBounds(row.removeFromLeft(labelWidth));
        coordModeSelector.setBounds(row.removeFromLeft(buttonPairWidth));
        bounds.removeFromTop(spacing);

        // Flip XYZ OFF action button (centered)
        flipXYZOffButton.setBounds(bounds.removeFromTop(rowHeight).withSizeKeepingCentre(actionButtonWidth, rowHeight));
        bounds.removeFromTop(spacing);

        // Constraint positions - ON/OFF button pair
        row = bounds.removeFromTop(rowHeight);
        constraintLabel.setBounds(row.removeFromLeft(labelWidth));
        constraintOnButton.setBounds(row.removeFromLeft(buttonWidth));
        row.removeFromLeft(spacing);
        constraintOffButton.setBounds(row.removeFromLeft(buttonWidth));
        bounds.removeFromTop(spacing);

        // Height factor dial row
        row = bounds.removeFromTop(dialSize + rowHeight);
        heightFactorLabel.setBounds(row.removeFromLeft(labelWidth).removeFromTop(rowHeight));
        auto dialArea = row.removeFromLeft(dialSize);
        heightFactorDial.setBounds(dialArea.withHeight(dialSize));
        int dialCenterX = dialArea.getX() + dialSize / 2;
        layoutDialValueUnit(heightFactorValueLabel, heightFactorUnitLabel, dialCenterX, dialArea.getY() + dialSize, rowHeight);
        bounds.removeFromTop(sectionSpacing);

        // === SOUND/ATTENUATION SECTION ===
        // Distance attenuation section label
        distAttenSectionLabel.setBounds(bounds.removeFromTop(rowHeight));
        bounds.removeFromTop(spacing / 2);

        // All Log / All 1/d buttons row (same size as ON/OFF buttons)
        row = bounds.removeFromTop(rowHeight);
        row.removeFromLeft(labelWidth);  // Indent to align with other button pairs
        int buttonPairStartX = row.getX();
        allLogButton.setBounds(row.removeFromLeft(buttonWidth));
        row.removeFromLeft(spacing);
        all1dButton.setBounds(row.removeFromLeft(buttonWidth));
        // Center of button pair for dial positioning below
        int buttonPairCenterX = buttonPairStartX + buttonWidth + spacing / 2;
        bounds.removeFromTop(spacing);

        // Dials row: common on left, dB/m OR ratio centered under buttons
        row = bounds.removeFromTop(dialSize + rowHeight * 2);

        // Common dial on left (first column)
        auto col = row.removeFromLeft(labelWidth);
        commonLabel.setBounds(col.removeFromTop(rowHeight));
        auto commonDialBounds = col.removeFromTop(dialSize);
        commonDial.setBounds(commonDialBounds.withSizeKeepingCentre(dialSize, dialSize));
        int commonCenterX = commonDialBounds.getX() + commonDialBounds.getWidth() / 2;
        layoutDialValueUnit(commonValueLabel, commonUnitLabel, commonCenterX, col.getY(), rowHeight);

        // dB/m and ratio dials centered under button pair
        auto remainingCol = row;
        auto labelBounds = remainingCol.removeFromTop(rowHeight);
        // Position labels centered under button pair
        dbmLabel.setBounds(labelBounds.withX(buttonPairCenterX - 40).withWidth(80));
        ratioLabel.setBounds(labelBounds.withX(buttonPairCenterX - 40).withWidth(80));
        auto dialBounds = remainingCol.removeFromTop(dialSize);
        auto dialRect = juce::Rectangle<int>(buttonPairCenterX - dialSize / 2, dialBounds.getY(), dialSize, dialSize);
        dbmDial.setBounds(dialRect);
        ratioDial.setBounds(dialRect);
        layoutDialValueUnit(dbmValueLabel, dbmUnitLabel, buttonPairCenterX, dialBounds.getY() + dialSize, rowHeight, 35, 40);
        layoutDialValueUnit(ratioValueLabel, ratioUnitLabel, buttonPairCenterX, dialBounds.getY() + dialSize, rowHeight, 35, 20);
        bounds.removeFromTop(spacing);

        // Reset directivity action (centered)
        resetDirectivityButton.setBounds(bounds.removeFromTop(rowHeight).withSizeKeepingCentre(actionButtonWidth, rowHeight));
        bounds.removeFromTop(sectionSpacing);

        // === LIVE SOURCE SECTION ===
        liveSourceOffButton.setBounds(bounds.removeFromTop(rowHeight).withSizeKeepingCentre(actionButtonWidth, rowHeight));
        bounds.removeFromTop(sectionSpacing);

        // === HACKOUSTICS SECTION ===
        // Floor Reflections - ON/OFF button pair
        row = bounds.removeFromTop(rowHeight);
        floorReflectionsLabel.setBounds(row.removeFromLeft(labelWidth));
        floorReflectionsOnButton.setBounds(row.removeFromLeft(buttonWidth));
        row.removeFromLeft(spacing);
        floorReflectionsOffButton.setBounds(row.removeFromLeft(buttonWidth));
        bounds.removeFromTop(sectionSpacing);

        // === LFO SECTION ===
        jitterLfoOffButton.setBounds(bounds.removeFromTop(rowHeight).withSizeKeepingCentre(actionButtonWidth, rowHeight));
        bounds.removeFromTop(sectionSpacing);

        // === MUTES SECTION ===
        // Mute macros row
        row = bounds.removeFromTop(rowHeight);
        muteMacrosLabel.setBounds(row.removeFromLeft(labelWidth));
        muteMacrosSelector.setBounds(row.removeFromLeft(buttonPairWidth));
        bounds.removeFromTop(spacing);

        // Sidelines - ON/OFF button pair
        row = bounds.removeFromTop(rowHeight);
        sidelinesLabel.setBounds(row.removeFromLeft(labelWidth));
        int sidelinesButtonStartX = row.getX();
        sidelinesOnButton.setBounds(row.removeFromLeft(buttonWidth));
        row.removeFromLeft(spacing);
        sidelinesOffButton.setBounds(row.removeFromLeft(buttonWidth));
        // Center of sidelines button pair for fringe dial
        int sidelinesButtonCenterX = sidelinesButtonStartX + buttonWidth + spacing / 2;
        bounds.removeFromTop(spacing);

        // Fringe dial row - centered under Sidelines ON/OFF buttons
        row = bounds.removeFromTop(dialSize + rowHeight);
        sidelinesFringeLabel.setBounds(row.removeFromLeft(labelWidth).removeFromTop(rowHeight));
        auto fringeDialRect = juce::Rectangle<int>(sidelinesButtonCenterX - dialSize / 2, row.getY(), dialSize, dialSize);
        sidelinesFringeDial.setBounds(fringeDialRect);
        layoutDialValueUnit(sidelinesFringeValueLabel, sidelinesFringeUnitLabel, sidelinesButtonCenterX, row.getY() + dialSize, rowHeight, 35, 20);
        bounds.removeFromTop(spacing);

        // Close button at bottom (centered)
        closeButton.setBounds(bounds.removeFromBottom(40).withSizeKeepingCentre(actionButtonWidth, 38));
    }

    std::function<void()> onCloseRequested;

private:
    WfsParameters& parameters;

    // Warning label
    juce::Label warningLabel;

    // Coordinate mode
    juce::Label coordModeLabel;
    juce::ComboBox coordModeSelector;

    // Minimal Latency - ON/OFF buttons
    juce::Label curvatureOnlyLabel;
    juce::TextButton curvatureOnlyOnButton;
    juce::TextButton curvatureOnlyOffButton;

    // Flip XYZ OFF
    juce::TextButton flipXYZOffButton;

    // Constraint positions - ON/OFF buttons
    juce::Label constraintLabel;
    juce::TextButton constraintOnButton;
    juce::TextButton constraintOffButton;

    // Height factor
    juce::Label heightFactorLabel;
    WfsBasicDial heightFactorDial;
    juce::Label heightFactorValueLabel;
    juce::Label heightFactorUnitLabel;

    // Distance attenuation section
    juce::Label distAttenSectionLabel;
    juce::TextButton allLogButton;
    juce::TextButton all1dButton;

    juce::Label dbmLabel;
    WfsBasicDial dbmDial;
    juce::Label dbmValueLabel;
    juce::Label dbmUnitLabel;

    juce::Label ratioLabel;
    WfsBasicDial ratioDial;
    juce::Label ratioValueLabel;
    juce::Label ratioUnitLabel;

    juce::Label commonLabel;
    WfsBasicDial commonDial;
    juce::Label commonValueLabel;
    juce::Label commonUnitLabel;

    // Reset directivity
    juce::TextButton resetDirectivityButton;

    // Mute macros
    juce::Label muteMacrosLabel;
    juce::ComboBox muteMacrosSelector;

    // Live Source OFF
    juce::TextButton liveSourceOffButton;

    // Sidelines - ON/OFF buttons
    juce::Label sidelinesLabel;
    juce::TextButton sidelinesOnButton;
    juce::TextButton sidelinesOffButton;
    juce::Label sidelinesFringeLabel;
    WfsBasicDial sidelinesFringeDial;
    juce::Label sidelinesFringeValueLabel;
    juce::Label sidelinesFringeUnitLabel;

    // Jitter & LFO OFF
    juce::TextButton jitterLfoOffButton;

    // Floor Reflections - ON/OFF buttons
    juce::Label floorReflectionsLabel;
    juce::TextButton floorReflectionsOnButton;
    juce::TextButton floorReflectionsOffButton;

    // Close button
    juce::TextButton closeButton;

    // Helper to layout dial value and unit labels (matching InputsTab pattern)
    // Places value and unit adjacent, centered as a pair under dial with overlap to reduce font padding gap
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

    void setupControls()
    {
        // Warning label
        addAndMakeVisible(warningLabel);
        warningLabel.setText(LOC("setAllInputs.warning"), juce::dontSendNotification);
        warningLabel.setJustificationType(juce::Justification::centred);
        warningLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));

        // Coordinate mode
        addAndMakeVisible(coordModeLabel);
        coordModeLabel.setText(LOC("setAllInputs.labels.coordinateMode"), juce::dontSendNotification);

        addAndMakeVisible(coordModeSelector);
        coordModeSelector.addItem(LOC("setAllInputs.coordinateModes.xyz"), 1);
        coordModeSelector.addItem(LOC("setAllInputs.coordinateModes.rThetaZ"), 2);
        coordModeSelector.addItem(LOC("setAllInputs.coordinateModes.rThetaPhi"), 3);
        coordModeSelector.setSelectedId(1, juce::dontSendNotification);
        coordModeSelector.onChange = [this]() {
            int mode = coordModeSelector.getSelectedId() - 1;
            applyToAllInputs(WFSParameterIDs::inputCoordinateMode, mode);
        };

        // Minimal Latency - ON/OFF buttons
        addAndMakeVisible(curvatureOnlyLabel);
        curvatureOnlyLabel.setText(LOC("setAllInputs.labels.minimalLatency"), juce::dontSendNotification);

        addAndMakeVisible(curvatureOnlyOnButton);
        curvatureOnlyOnButton.setButtonText(LOC("common.on"));
        curvatureOnlyOnButton.onClick = [this]() {
            applyToAllInputs(WFSParameterIDs::inputMinimalLatency, 1);
            showActionFeedback(curvatureOnlyOnButton);
        };

        addAndMakeVisible(curvatureOnlyOffButton);
        curvatureOnlyOffButton.setButtonText(LOC("common.off"));
        curvatureOnlyOffButton.onClick = [this]() {
            applyToAllInputs(WFSParameterIDs::inputMinimalLatency, 0);
            showActionFeedback(curvatureOnlyOffButton);
        };

        // Flip XYZ OFF
        addAndMakeVisible(flipXYZOffButton);
        flipXYZOffButton.setButtonText(LOC("setAllInputs.buttons.flipXyzOff"));
        flipXYZOffButton.onClick = [this]() {
            applyToAllInputs(WFSParameterIDs::inputFlipX, 0);
            applyToAllInputs(WFSParameterIDs::inputFlipY, 0);
            applyToAllInputs(WFSParameterIDs::inputFlipZ, 0);
            showActionFeedback(flipXYZOffButton);
        };

        // Constraint positions - ON/OFF buttons
        addAndMakeVisible(constraintLabel);
        constraintLabel.setText(LOC("setAllInputs.labels.constraintPositions"), juce::dontSendNotification);

        addAndMakeVisible(constraintOnButton);
        constraintOnButton.setButtonText(LOC("common.on"));
        constraintOnButton.onClick = [this]() {
            applyToAllInputs(WFSParameterIDs::inputConstraintX, 1);
            applyToAllInputs(WFSParameterIDs::inputConstraintY, 1);
            applyToAllInputs(WFSParameterIDs::inputConstraintZ, 1);
            applyToAllInputs(WFSParameterIDs::inputConstraintDistance, 1);
            showActionFeedback(constraintOnButton);
        };

        addAndMakeVisible(constraintOffButton);
        constraintOffButton.setButtonText(LOC("common.off"));
        constraintOffButton.onClick = [this]() {
            applyToAllInputs(WFSParameterIDs::inputConstraintX, 0);
            applyToAllInputs(WFSParameterIDs::inputConstraintY, 0);
            applyToAllInputs(WFSParameterIDs::inputConstraintZ, 0);
            applyToAllInputs(WFSParameterIDs::inputConstraintDistance, 0);
            showActionFeedback(constraintOffButton);
        };

        // Height factor
        addAndMakeVisible(heightFactorLabel);
        heightFactorLabel.setText(LOC("setAllInputs.labels.heightFactor"), juce::dontSendNotification);

        addAndMakeVisible(heightFactorDial);
        heightFactorDial.setValue(0.0f);  // Default 0%
        heightFactorDial.onValueChanged = [this](float v) {
            int percent = static_cast<int>(v * 100.0f);
            heightFactorValueLabel.setText(juce::String(percent), juce::dontSendNotification);
            applyToAllInputs(WFSParameterIDs::inputHeightFactor, percent);
        };

        addAndMakeVisible(heightFactorValueLabel);
        heightFactorValueLabel.setText("0", juce::dontSendNotification);

        addAndMakeVisible(heightFactorUnitLabel);
        heightFactorUnitLabel.setText("%", juce::dontSendNotification);

        // Distance attenuation section label
        addAndMakeVisible(distAttenSectionLabel);
        distAttenSectionLabel.setText(LOC("setAllInputs.labels.distanceAttenuation"), juce::dontSendNotification);
        distAttenSectionLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));

        // All Log button - sets param to 0, shows dB/m dial in popup
        addAndMakeVisible(allLogButton);
        allLogButton.setButtonText(LOC("setAllInputs.buttons.allLog"));
        allLogButton.onClick = [this]() {
            applyToAllInputs(WFSParameterIDs::inputAttenuationLaw, 0);
            // For popup: is1OverD=false means Log mode, show dB/m
            dbmLabel.setVisible(true);
            dbmDial.setVisible(true);
            dbmValueLabel.setVisible(true);
            dbmUnitLabel.setVisible(true);
            ratioLabel.setVisible(false);
            ratioDial.setVisible(false);
            ratioValueLabel.setVisible(false);
            ratioUnitLabel.setVisible(false);
            showActionFeedback(allLogButton);
        };

        // All 1/d button - sets param to 1, shows ratio dial in popup
        addAndMakeVisible(all1dButton);
        all1dButton.setButtonText(LOC("setAllInputs.buttons.all1d"));
        all1dButton.onClick = [this]() {
            applyToAllInputs(WFSParameterIDs::inputAttenuationLaw, 1);
            // For popup: is1OverD=true means 1/d mode, show ratio
            dbmLabel.setVisible(false);
            dbmDial.setVisible(false);
            dbmValueLabel.setVisible(false);
            dbmUnitLabel.setVisible(false);
            ratioLabel.setVisible(true);
            ratioDial.setVisible(true);
            ratioValueLabel.setVisible(true);
            ratioUnitLabel.setVisible(true);
            showActionFeedback(all1dButton);
        };

        // dB/m dial
        addAndMakeVisible(dbmLabel);
        dbmLabel.setText(LOC("setAllInputs.labels.dbPerMeter"), juce::dontSendNotification);
        dbmLabel.setJustificationType(juce::Justification::centred);

        addAndMakeVisible(dbmDial);
        // Range: -6 to 0, formula: dB = (v * 6.0) - 6.0
        dbmDial.setValue((WFSParameterDefaults::inputDistanceAttenuationDefault + 6.0f) / 6.0f);
        dbmDial.onValueChanged = [this](float v) {
            float dBm = (v * 6.0f) - 6.0f;
            dbmValueLabel.setText(juce::String(dBm, 1), juce::dontSendNotification);
            applyToAllInputs(WFSParameterIDs::inputDistanceAttenuation, dBm);
        };

        addAndMakeVisible(dbmValueLabel);
        dbmValueLabel.setText(juce::String(WFSParameterDefaults::inputDistanceAttenuationDefault, 1), juce::dontSendNotification);

        addAndMakeVisible(dbmUnitLabel);
        dbmUnitLabel.setText(LOC("units.decibelPerMeter"), juce::dontSendNotification);

        // ratio dial
        addAndMakeVisible(ratioLabel);
        ratioLabel.setText(LOC("setAllInputs.labels.ratio"), juce::dontSendNotification);
        ratioLabel.setJustificationType(juce::Justification::centred);

        addAndMakeVisible(ratioDial);
        // Range: 0.1 to 10.0, formula: ratio = pow(10, (v * 2) - 1)
        ratioDial.setValue(0.5f);  // Default 1.0x
        ratioDial.onValueChanged = [this](float v) {
            float ratio = std::pow(10.0f, (v * 2.0f) - 1.0f);
            ratioValueLabel.setText(juce::String(ratio, 2), juce::dontSendNotification);
            applyToAllInputs(WFSParameterIDs::inputDistanceRatio, ratio);
        };

        addAndMakeVisible(ratioValueLabel);
        ratioValueLabel.setText("1.00", juce::dontSendNotification);

        addAndMakeVisible(ratioUnitLabel);
        ratioUnitLabel.setText("x", juce::dontSendNotification);

        // Initially hide ratio dial (Log is default)
        ratioLabel.setVisible(false);
        ratioDial.setVisible(false);
        ratioValueLabel.setVisible(false);
        ratioUnitLabel.setVisible(false);

        // common dial
        addAndMakeVisible(commonLabel);
        commonLabel.setText(LOC("setAllInputs.labels.common"), juce::dontSendNotification);
        commonLabel.setJustificationType(juce::Justification::centred);

        addAndMakeVisible(commonDial);
        commonDial.setValue(1.0f);  // Default 100%
        commonDial.onValueChanged = [this](float v) {
            int percent = static_cast<int>(v * 100.0f);
            commonValueLabel.setText(juce::String(percent), juce::dontSendNotification);
            applyToAllInputs(WFSParameterIDs::inputCommonAtten, percent);
        };

        addAndMakeVisible(commonValueLabel);
        commonValueLabel.setText("100", juce::dontSendNotification);

        addAndMakeVisible(commonUnitLabel);
        commonUnitLabel.setText("%", juce::dontSendNotification);

        // Reset directivity (also resets Rotation, Tilt, HF Shelf)
        addAndMakeVisible(resetDirectivityButton);
        resetDirectivityButton.setButtonText(LOC("setAllInputs.buttons.resetDirectivity"));
        resetDirectivityButton.onClick = [this]() {
            applyToAllInputs(WFSParameterIDs::inputDirectivity, WFSParameterDefaults::inputDirectivityDefault);
            applyToAllInputs(WFSParameterIDs::inputRotation, WFSParameterDefaults::inputRotationDefault);
            applyToAllInputs(WFSParameterIDs::inputTilt, WFSParameterDefaults::inputTiltDefault);
            applyToAllInputs(WFSParameterIDs::inputHFshelf, WFSParameterDefaults::inputHFshelfDefault);
            showActionFeedback(resetDirectivityButton);
        };

        // Mute macros
        addAndMakeVisible(muteMacrosLabel);
        muteMacrosLabel.setText(LOC("setAllInputs.labels.muteMacros"), juce::dontSendNotification);

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
                applyMuteMacroToAllInputs(macroId);
            }
            muteMacrosSelector.setSelectedId(1, juce::dontSendNotification);
        };

        // Live Source OFF
        addAndMakeVisible(liveSourceOffButton);
        liveSourceOffButton.setButtonText(LOC("setAllInputs.buttons.turnOffLiveSource"));
        liveSourceOffButton.onClick = [this]() {
            applyToAllInputs(WFSParameterIDs::inputLSactive, 0);
            showActionFeedback(liveSourceOffButton);
        };

        // Sidelines - ON/OFF buttons
        addAndMakeVisible(sidelinesLabel);
        sidelinesLabel.setText(LOC("setAllInputs.labels.sidelines"), juce::dontSendNotification);

        addAndMakeVisible(sidelinesOnButton);
        sidelinesOnButton.setButtonText(LOC("common.on"));
        sidelinesOnButton.onClick = [this]() {
            applyToAllInputs(WFSParameterIDs::inputSidelinesActive, 1);
            showActionFeedback(sidelinesOnButton);
        };

        addAndMakeVisible(sidelinesOffButton);
        sidelinesOffButton.setButtonText(LOC("common.off"));
        sidelinesOffButton.onClick = [this]() {
            applyToAllInputs(WFSParameterIDs::inputSidelinesActive, 0);
            showActionFeedback(sidelinesOffButton);
        };

        addAndMakeVisible(sidelinesFringeLabel);
        sidelinesFringeLabel.setText(LOC("setAllInputs.labels.fringe"), juce::dontSendNotification);

        addAndMakeVisible(sidelinesFringeDial);
        // Range: 0.1 to 10m
        float defaultFringe = WFSParameterDefaults::inputSidelinesFringeDefault;
        float fringeNorm = (defaultFringe - WFSParameterDefaults::inputSidelinesFringeMin) /
                           (WFSParameterDefaults::inputSidelinesFringeMax - WFSParameterDefaults::inputSidelinesFringeMin);
        sidelinesFringeDial.setValue(fringeNorm);
        sidelinesFringeDial.onValueChanged = [this](float v) {
            float fringe = WFSParameterDefaults::inputSidelinesFringeMin +
                           v * (WFSParameterDefaults::inputSidelinesFringeMax - WFSParameterDefaults::inputSidelinesFringeMin);
            sidelinesFringeValueLabel.setText(juce::String(fringe, 2), juce::dontSendNotification);
            applyToAllInputs(WFSParameterIDs::inputSidelinesFringe, fringe);
        };

        addAndMakeVisible(sidelinesFringeValueLabel);
        sidelinesFringeValueLabel.setText(juce::String(defaultFringe, 2), juce::dontSendNotification);

        addAndMakeVisible(sidelinesFringeUnitLabel);
        sidelinesFringeUnitLabel.setText("m", juce::dontSendNotification);

        // Jitter & LFO OFF
        addAndMakeVisible(jitterLfoOffButton);
        jitterLfoOffButton.setButtonText(LOC("setAllInputs.buttons.turnOffJitterLfo"));
        jitterLfoOffButton.onClick = [this]() {
            applyToAllInputs(WFSParameterIDs::inputJitter, 0.0f);
            applyToAllInputs(WFSParameterIDs::inputLFOactive, 0);
            showActionFeedback(jitterLfoOffButton);
        };

        // Floor Reflections - ON/OFF buttons
        addAndMakeVisible(floorReflectionsLabel);
        floorReflectionsLabel.setText(LOC("setAllInputs.labels.floorReflections"), juce::dontSendNotification);

        addAndMakeVisible(floorReflectionsOnButton);
        floorReflectionsOnButton.setButtonText(LOC("common.on"));
        floorReflectionsOnButton.onClick = [this]() {
            applyToAllInputs(WFSParameterIDs::inputFRactive, 1);
            showActionFeedback(floorReflectionsOnButton);
        };

        addAndMakeVisible(floorReflectionsOffButton);
        floorReflectionsOffButton.setButtonText(LOC("common.off"));
        floorReflectionsOffButton.onClick = [this]() {
            applyToAllInputs(WFSParameterIDs::inputFRactive, 0);
            showActionFeedback(floorReflectionsOffButton);
        };

        // Close button
        addAndMakeVisible(closeButton);
        closeButton.setButtonText(LOC("setAllInputs.buttons.closeWindow"));
        closeButton.onClick = [this]() {
            if (onCloseRequested)
                onCloseRequested();
        };

        applyTheme();
    }

    void updateAttenLawVisibility(bool is1OverD)
    {
        // Show/hide dB/m vs ratio based on attenuation law
        dbmLabel.setVisible(!is1OverD);
        dbmDial.setVisible(!is1OverD);
        dbmValueLabel.setVisible(!is1OverD);
        dbmUnitLabel.setVisible(!is1OverD);

        ratioLabel.setVisible(is1OverD);
        ratioDial.setVisible(is1OverD);
        ratioValueLabel.setVisible(is1OverD);
        ratioUnitLabel.setVisible(is1OverD);
    }

    void applyTheme()
    {
        const auto& colors = ColorScheme::get();

        // Warning label - black bold on red strip
        warningLabel.setColour(juce::Label::textColourId, juce::Colours::black);

        // Labels
        auto setupLabel = [&colors](juce::Label& label) {
            label.setColour(juce::Label::textColourId, colors.textPrimary);
        };

        setupLabel(coordModeLabel);
        setupLabel(curvatureOnlyLabel);
        setupLabel(constraintLabel);
        setupLabel(heightFactorLabel);
        setupLabel(distAttenSectionLabel);
        setupLabel(dbmLabel);
        setupLabel(ratioLabel);
        setupLabel(commonLabel);
        setupLabel(muteMacrosLabel);
        setupLabel(sidelinesLabel);
        setupLabel(sidelinesFringeLabel);
        setupLabel(floorReflectionsLabel);

        // Value labels
        setupLabel(heightFactorValueLabel);
        setupLabel(dbmValueLabel);
        setupLabel(ratioValueLabel);
        setupLabel(commonValueLabel);
        setupLabel(sidelinesFringeValueLabel);

        // Unit labels - secondary color
        auto setupUnitLabel = [&colors](juce::Label& label) {
            label.setColour(juce::Label::textColourId, colors.textSecondary);
        };

        setupUnitLabel(heightFactorUnitLabel);
        setupUnitLabel(dbmUnitLabel);
        setupUnitLabel(ratioUnitLabel);
        setupUnitLabel(commonUnitLabel);
        setupUnitLabel(sidelinesFringeUnitLabel);

        // ON/OFF button pairs
        auto setupOnOffButtons = [&colors](juce::TextButton& onBtn, juce::TextButton& offBtn) {
            onBtn.setColour(juce::TextButton::buttonColourId, colors.buttonNormal);
            onBtn.setColour(juce::TextButton::textColourOffId, colors.textPrimary);
            offBtn.setColour(juce::TextButton::buttonColourId, colors.buttonNormal);
            offBtn.setColour(juce::TextButton::textColourOffId, colors.textPrimary);
        };

        setupOnOffButtons(curvatureOnlyOnButton, curvatureOnlyOffButton);
        setupOnOffButtons(constraintOnButton, constraintOffButton);
        setupOnOffButtons(sidelinesOnButton, sidelinesOffButton);
        setupOnOffButtons(floorReflectionsOnButton, floorReflectionsOffButton);

        // Action buttons
        auto setupActionButton = [&colors](juce::TextButton& btn) {
            btn.setColour(juce::TextButton::buttonColourId, colors.buttonNormal);
            btn.setColour(juce::TextButton::textColourOffId, colors.textPrimary);
        };

        setupActionButton(flipXYZOffButton);
        setupActionButton(allLogButton);
        setupActionButton(all1dButton);
        setupActionButton(resetDirectivityButton);
        setupActionButton(liveSourceOffButton);
        setupActionButton(jitterLfoOffButton);

        // Close button - orange-ish
        closeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF996633));
        closeButton.setColour(juce::TextButton::textColourOffId, colors.textPrimary);

        // Dials
        heightFactorDial.setColours(colors.sliderTrackBg, colors.accentBlue, colors.textPrimary);
        dbmDial.setColours(colors.sliderTrackBg, juce::Colour(0xFF9C27B0), colors.textPrimary);  // Purple
        ratioDial.setColours(colors.sliderTrackBg, juce::Colour(0xFF9C27B0), colors.textPrimary);  // Purple
        commonDial.setColours(colors.sliderTrackBg, juce::Colour(0xFF00ACC1), colors.textPrimary);  // Cyan
        sidelinesFringeDial.setColours(colors.sliderTrackBg, colors.accentGreen, colors.textPrimary);

        // ComboBoxes
        coordModeSelector.setColour(juce::ComboBox::backgroundColourId, colors.surfaceCard);
        coordModeSelector.setColour(juce::ComboBox::textColourId, colors.textPrimary);
        coordModeSelector.setColour(juce::ComboBox::outlineColourId, colors.buttonBorder);

        muteMacrosSelector.setColour(juce::ComboBox::backgroundColourId, colors.surfaceCard);
        muteMacrosSelector.setColour(juce::ComboBox::textColourId, colors.textPrimary);
        muteMacrosSelector.setColour(juce::ComboBox::outlineColourId, colors.buttonBorder);
    }

    void applyToAllInputs(const juce::Identifier& paramId, const juce::var& value)
    {
        int numInputs = parameters.getNumInputChannels();
        for (int i = 0; i < numInputs; ++i)
        {
            parameters.setInputParam(i, paramId.toString(), value);
        }
    }

    void applyMuteMacroToAllInputs(int macroId)
    {
        int numInputs = parameters.getNumInputChannels();
        int numOutputs = parameters.getNumOutputChannels();
        if (numOutputs <= 0) numOutputs = 64;

        for (int inputIdx = 0; inputIdx < numInputs; ++inputIdx)
        {
            // Get current mute states for this input (comma-separated "0,1,0,1,..." format)
            bool muteStates[64] = {false};
            juce::var mutesVar = parameters.getInputParam(inputIdx, WFSParameterIDs::inputMutes.toString());
            if (mutesVar.isString())
            {
                juce::String muteStr = mutesVar.toString();
                juce::StringArray muteValues;
                muteValues.addTokens(muteStr, ",", "");
                for (int i = 0; i < juce::jmin(64, muteValues.size()); ++i)
                    muteStates[i] = (muteValues[i] == "1");
            }

            switch (macroId)
            {
                case 2:  // MUTE ALL
                    for (int o = 0; o < numOutputs; ++o)
                        muteStates[o] = true;
                    break;
                case 3:  // UNMUTE ALL
                    for (int o = 0; o < 64; ++o)
                        muteStates[o] = false;
                    break;
                case 4:  // INVERT MUTES
                    for (int o = 0; o < numOutputs; ++o)
                        muteStates[o] = !muteStates[o];
                    break;
                case 5:  // MUTE ODD
                    for (int o = 0; o < numOutputs; o += 2)
                        muteStates[o] = true;
                    break;
                case 6:  // MUTE EVEN
                    for (int o = 1; o < numOutputs; o += 2)
                        muteStates[o] = true;
                    break;
                default:
                    // Array-based mutes (7-26)
                    if (macroId >= 7)
                    {
                        int arrayMacroIndex = macroId - 7;
                        int arrayNum = (arrayMacroIndex / 2) + 1;
                        bool isMute = (arrayMacroIndex % 2) == 0;

                        for (int o = 0; o < numOutputs; ++o)
                        {
                            int outputArray = static_cast<int>(parameters.getOutputParam(o, WFSParameterIDs::outputArray.toString()));
                            if (outputArray == arrayNum)
                                muteStates[o] = isMute;
                        }
                    }
                    break;
            }

            // Save as comma-separated string (same format as InputsTab)
            juce::StringArray muteValues;
            for (int i = 0; i < 64; ++i)
                muteValues.add(muteStates[i] ? "1" : "0");
            parameters.setInputParam(inputIdx, WFSParameterIDs::inputMutes.toString(), muteValues.joinIntoString(","));
        }
    }

    void showActionFeedback(juce::TextButton& button)
    {
        auto originalColor = button.findColour(juce::TextButton::buttonColourId);
        button.setColour(juce::TextButton::buttonColourId, ColorScheme::get().accentGreen);
        button.repaint();

        // Reset after 200ms
        juce::Timer::callAfterDelay(200, [&button, originalColor]() {
            if (button.isShowing())
            {
                button.setColour(juce::TextButton::buttonColourId, originalColor);
                button.repaint();
            }
        });
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SetAllInputsContent)
};

//==============================================================================
// Window Class
//==============================================================================
class SetAllInputsWindow : public juce::DocumentWindow,
                           public ColorScheme::Manager::Listener
{
public:
    SetAllInputsWindow(WfsParameters& params)
        : DocumentWindow(LOC("setAllInputs.windowTitle"),
                         ColorScheme::get().background,
                         DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(true);
        setResizable(false, false);

        content = std::make_unique<SetAllInputsContent>(params);
        content->onCloseRequested = [this]() { closeButtonPressed(); };
        content->setName(LOC("setAllInputs.windowTitle"));  // Accessible name for screen readers
        setContentOwned(content.release(), false);

        centreWithSize(370, 880);
        setVisible(true);
        WindowUtils::enableDarkTitleBar(this);

        ColorScheme::Manager::getInstance().addListener(this);
    }

    ~SetAllInputsWindow() override
    {
        ColorScheme::Manager::getInstance().removeListener(this);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }

    void colorSchemeChanged() override
    {
        setBackgroundColour(ColorScheme::get().background);
        repaint();
    }

private:
    std::unique_ptr<SetAllInputsContent> content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SetAllInputsWindow)
};
