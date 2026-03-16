#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "ColorScheme.h"
#include "ColorUtilities.h"
#include "WfsJoystickComponent.h"
#include "sliders/WfsAutoCenterSlider.h"
#include "sliders/WfsStandardSlider.h"
#include "sliders/WfsBidirectionalSlider.h"
#include "dials/WfsEndlessDial.h"
#include "dials/WfsBasicDial.h"
#include "dials/WfsRotationDial.h"
#include "dials/WfsLFOIndicators.h"
#include "StatusBar.h"
#include "../Localization/LocalizationManager.h"
#include "../DSP/ClusterLFOProcessor.h"
#include "../AppSettings.h"
#include "buttons/LongPressButton.h"
#include "ColumnFocusTraverser.h"

//==============================================================================
/**
 * Custom button for cluster selection with bold text
 */
class ClusterButton : public juce::TextButton
{
public:
    ClusterButton (const juce::String& text) : juce::TextButton (text) {}

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto& lf = getLookAndFeel();
        lf.drawButtonBackground (g, *this, findColour (getToggleState() ? buttonOnColourId : buttonColourId),
                                 shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

        // Draw text with bold font
        g.setFont (juce::FontOptions (15.0f).withStyle ("Bold"));
        g.setColour (findColour (getToggleState() ? textColourOnId : textColourOffId));
        g.drawText (getButtonText(), getLocalBounds(), juce::Justification::centred);
    }
};

//==============================================================================
// Forward declaration for row component
class ClustersTab;

/**
 * Custom row component for the cluster input list with a drag handle
 * for manual reordering.
 */
class ClusterInputRowComponent : public juce::Component
{
public:
    ClusterInputRowComponent (ClustersTab& ownerTab) : owner (ownerTab) {}

    void update (int row, int idx, bool tracked, bool first, bool selected, int refMode,
                 const juce::String& name, bool beingDragged)
    {
        rowNumber = row;
        inputIdx  = idx;
        isTracked = tracked;
        isFirst   = first;
        isSelected = selected;
        refModeId  = refMode;
        inputName  = name;
        isDragged  = beingDragged;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        int handleW = 20;

        // Background
        if (isSelected)
            g.fillAll (ColorScheme::get().listSelection);
        else if (isTracked)
            g.fillAll (ColorScheme::get().listSelection.interpolatedWith (juce::Colour (0xFFFF9800), 0.3f));
        else if (isFirst && refModeId == 1)
            g.fillAll (ColorScheme::get().listSelection.interpolatedWith (juce::Colour (0xFF00FF00), 0.2f));
        else
            g.fillAll (ColorScheme::get().surfaceCard);

        // Draw drag handle (three horizontal lines) — hidden for locked tracked input at slot 0
        bool locked = isTracked && rowNumber == 0;
        if (!locked)
        {
            auto handleArea = bounds.removeFromLeft (handleW);
            g.setColour (ColorScheme::get().textSecondary.withAlpha (0.6f));
            int cy = handleArea.getCentreY();
            int cx = handleArea.getCentreX();
            for (int line = -1; line <= 1; ++line)
            {
                float y = static_cast<float> (cy + line * 4);
                g.drawLine (static_cast<float> (cx - 5), y,
                            static_cast<float> (cx + 5), y, 1.0f);
            }
        }
        else
        {
            bounds.removeFromLeft (handleW);
        }

        // Text
        g.setColour (isTracked ? juce::Colour (0xFFFF9800) : ColorScheme::get().textPrimary);
        juce::String text = LOC ("clusters.labels.inputPrefix") + " " + juce::String (inputIdx + 1);
        juce::String defaultName = LOC ("clusters.labels.inputPrefix") + " " + juce::String (inputIdx + 1);
        if (inputName.isNotEmpty() && inputName != defaultName)
            text += " - " + inputName;
        if (isTracked)
            text += " " + LOC ("clusters.status.trackedMarker");
        g.drawText (text, bounds.reduced (5, 0), juce::Justification::centredLeft);

        // Highlight border when this row is being dragged
        if (isDragged)
        {
            g.setColour (ColorScheme::get().textPrimary.withAlpha (0.4f));
            g.drawRect (getLocalBounds(), 1);
        }
    }

    // Mouse methods — defined after ClustersTab is complete
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp   (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;

    int getRowNumber() const noexcept { return rowNumber; }
    int getInputIdx()  const noexcept { return inputIdx; }

private:
    bool isLocked() const { return isTracked && rowNumber == 0; }

    ClustersTab& owner;
    int rowNumber  = 0;
    int inputIdx   = 0;
    bool isTracked = false;
    bool isFirst   = false;
    bool isSelected = false;
    int refModeId  = 1;
    juce::String inputName;
    bool isDragged = false;
};

//==============================================================================
/**
 * Clusters Tab Component
 * Management of input clusters with position, rotation, scale, attenuation,
 * and LFO controls for X, Y, Z, Rotation, and Scale modulation.
 */
class ClustersTab : public juce::Component,
                    public juce::ListBoxModel,
                    private juce::Timer,
                    private juce::ValueTree::Listener,
                    private juce::Label::Listener
{
public:
    ClustersTab(WfsParameters& params)
        : parameters(params),
          inputsTree(params.getInputTree()),
          configTree(params.getConfigTree()),
          clusterLFOProcessor(params.getValueTreeState())
    {
        // Add listeners
        inputsTree.addListener(this);
        configTree.addListener(this);

        // ==================== CLUSTER SELECTOR BAR ====================
        for (int i = 0; i < 10; ++i)
        {
            auto* btn = new ClusterButton(juce::String(i + 1));
            btn->setClickingTogglesState(true);
            btn->setRadioGroupId(1001);
            // Use cluster colors matching Map tab
            juce::Colour clusterColor = WfsColorUtilities::getMarkerColor(i + 1, true);
            btn->setColour(juce::TextButton::buttonColourId, clusterColor.darker(0.6f));  // Darker when not selected
            btn->setColour(juce::TextButton::buttonOnColourId, clusterColor);  // Full color when selected
            btn->setColour(juce::TextButton::textColourOffId, juce::Colours::black);
            btn->setColour(juce::TextButton::textColourOnId, juce::Colours::black);
            btn->onClick = [this, i]() {
                selectCluster(i + 1);
            };
            addAndMakeVisible(btn);
            clusterButtons.add(btn);
        }

        // ==================== ASSIGNED INPUTS PANEL ====================
        addAndMakeVisible(assignedInputsLabel);
        assignedInputsLabel.setText(LOC("clusters.labels.assignedInputs"), juce::dontSendNotification);
        assignedInputsLabel.setFont(juce::FontOptions().withHeight(16.0f).withStyle("Bold"));

        addAndMakeVisible(inputsList);
        inputsList.setModel(this);
        inputsList.setColour(juce::ListBox::backgroundColourId, ColorScheme::get().backgroundAlt);
        inputsList.setRowHeight(24);

        // ==================== CENTER PANEL - REFERENCE + CONTROLS ====================
        // Reference mode selector
        addAndMakeVisible(referenceModeLabel);
        referenceModeLabel.setText(LOC("clusters.labels.reference"), juce::dontSendNotification);

        addAndMakeVisible(referenceModeSelector);
        referenceModeSelector.addItem(LOC("clusters.referenceMode.firstInput"), 1);
        referenceModeSelector.addItem(LOC("clusters.referenceMode.barycenter"), 2);
        referenceModeSelector.setSelectedId(1, juce::dontSendNotification);
        referenceModeSelector.onChange = [this]() {
            if (selectedCluster > 0)
            {
                int mode = referenceModeSelector.getSelectedId() - 1;
                parameters.getValueTreeState().setClusterParameter(selectedCluster,
                    WFSParameterIDs::clusterReferenceMode, mode);
            }
        };

        // Reference position display
        addAndMakeVisible(refPosLabel);
        refPosLabel.setText(LOC("clusters.labels.posPrefix"), juce::dontSendNotification);
        refPosLabel.setColour(juce::Label::textColourId, ColorScheme::get().textSecondary);

        addAndMakeVisible(refPosXLabel);
        refPosXLabel.setColour(juce::Label::textColourId, ColorScheme::get().textSecondary);

        addAndMakeVisible(refPosYLabel);
        refPosYLabel.setColour(juce::Label::textColourId, ColorScheme::get().textSecondary);

        addAndMakeVisible(refPosZLabel);
        refPosZLabel.setColour(juce::Label::textColourId, ColorScheme::get().textSecondary);

        // Status label (tracking info)
        addAndMakeVisible(statusLabel);
        statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFFF9800));
        statusLabel.setFont(juce::FontOptions().withHeight(14.0f));

        // Controls label
        addAndMakeVisible(controlsLabel);
        controlsLabel.setText(LOC("clusters.labels.controls"), juce::dontSendNotification);
        controlsLabel.setFont(juce::FontOptions().withHeight(16.0f).withStyle("Bold"));

        // Position joystick label
        addAndMakeVisible(positionLabel);
        positionLabel.setText(LOC("clusters.labels.position"), juce::dontSendNotification);
        positionLabel.setFont(juce::FontOptions().withHeight(14.0f));
        positionLabel.setJustificationType(juce::Justification::centred);

        // Position joystick
        addAndMakeVisible(positionJoystick);
        positionJoystick.setOuterColour(juce::Colour(0xFF3A3A3A));
        positionJoystick.setThumbColour(juce::Colour(0xFF4CAF50));
        positionJoystick.setReportingIntervalHz(50.0);

        // Z slider label
        addAndMakeVisible(zSliderLabel);
        zSliderLabel.setText(LOC("clusters.labels.z"), juce::dontSendNotification);
        zSliderLabel.setFont(juce::FontOptions().withHeight(14.0f));
        zSliderLabel.setJustificationType(juce::Justification::centred);

        // Z slider
        zSlider.setTrackColours(juce::Colour(0xFF3A3A3A), juce::Colour(0xFF4CAF50));
        zSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction("Cluster Param");
        };
        addAndMakeVisible(zSlider);

        // Attenuation slider label
        addAndMakeVisible(attenuationLabel);
        attenuationLabel.setText(LOC("clusters.labels.attenuation"), juce::dontSendNotification);
        attenuationLabel.setFont(juce::FontOptions().withHeight(14.0f));
        attenuationLabel.setJustificationType(juce::Justification::centred);

        // Attenuation slider
        attenuationSlider.setTrackColours(juce::Colour(0xFF3A3A3A), juce::Colour(0xFFFF5722));
        attenuationSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction("Cluster Param");
        };
        addAndMakeVisible(attenuationSlider);

        // Rotation dial label
        addAndMakeVisible(rotationLabel);
        rotationLabel.setText(LOC("clusters.labels.rotation"), juce::dontSendNotification);
        rotationLabel.setFont(juce::FontOptions().withHeight(14.0f));
        rotationLabel.setJustificationType(juce::Justification::centred);

        // Rotation dial
        addAndMakeVisible(rotationDial);
        rotationDial.setColours(juce::Colour(0xFF3A3A3A), juce::Colour(0xFF2196F3), juce::Colours::white);
        rotationDial.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction("Cluster Param");
        };

        // Scale joystick label
        addAndMakeVisible(scaleLabel);
        scaleLabel.setText(LOC("clusters.labels.scale"), juce::dontSendNotification);
        scaleLabel.setFont(juce::FontOptions().withHeight(14.0f));
        scaleLabel.setJustificationType(juce::Justification::centred);

        // Scale slider (uniform, auto-centers)
        addAndMakeVisible(scaleSlider);
        scaleSlider.setTrackColours(juce::Colour(0xFF3A3A3A), juce::Colour(0xFF9C27B0));
        scaleSlider.onGestureStart = [this]() {
            parameters.getValueTreeState().beginUndoTransaction("Cluster Param");
        };

        // Plane selector (placed inside rotation dial center)
        planeLabel.setVisible(false);

        addAndMakeVisible(planeSelector);
        planeSelector.addItem(LOC("clusters.planes.xy"), 1);
        planeSelector.addItem(LOC("clusters.planes.xz"), 2);
        planeSelector.addItem(LOC("clusters.planes.yz"), 3);
        planeSelector.setSelectedId(1, juce::dontSendNotification);
        planeSelector.onChange = [this]() {
            currentPlane = static_cast<Plane>(planeSelector.getSelectedId() - 1);
        };

        // ==================== LFO PANEL ====================
        setupLFOPanel();

        // Initialise LFO tab navigation circuits
        lfoCircuits = {
            // Global: Period & Phase
            { &lfoPeriodValueLabel, &lfoPhaseValueLabel },
            // X axis: Amplitude → Phase → Rate
            { &lfoRows[0].amplitudeValueLabel, &lfoRows[0].phaseValueLabel, &lfoRows[0].rateValueLabel },
            // Y axis: Amplitude → Phase → Rate
            { &lfoRows[1].amplitudeValueLabel, &lfoRows[1].phaseValueLabel, &lfoRows[1].rateValueLabel },
            // Z axis: Amplitude → Phase → Rate
            { &lfoRows[2].amplitudeValueLabel, &lfoRows[2].phaseValueLabel, &lfoRows[2].rateValueLabel },
            // Rot axis: Amplitude → Phase → Rate
            { &lfoRows[3].amplitudeValueLabel, &lfoRows[3].phaseValueLabel, &lfoRows[3].rateValueLabel },
            // Scale axis: Amplitude → Phase → Rate
            { &lfoRows[4].amplitudeValueLabel, &lfoRows[4].phaseValueLabel, &lfoRows[4].rateValueLabel },
        };
        circuitTabHandler.circuits = &lfoCircuits;

        // ==================== LFO PRESET TILES ====================
        for (int i = 0; i < WFSParameterDefaults::maxClusterLFOPresets; ++i)
        {
            auto* tile = new LFOPresetTile (*this, i);
            addAndMakeVisible (tile);
            presetTiles.add (tile);
        }

        addAndMakeVisible (stopAllLFOButton);
        stopAllLFOButton.setButtonText (LOC ("clusters.presets.stopAll"));
        stopAllLFOButton.setBaseColour (juce::Colour (0xFFB71C1C));
        stopAllLFOButton.onLongPress = [this]() { stopAllClusterLFOs(); };

        addAndMakeVisible (exportPresetsButton);
        exportPresetsButton.setButtonText (LOC ("clusters.presets.export"));
        exportPresetsButton.onClick = [this]() { exportLFOPresets(); };

        addAndMakeVisible (importPresetsButton);
        importPresetsButton.setButtonText (LOC ("clusters.presets.import"));
        importPresetsButton.onClick = [this]() { importLFOPresets(); };

        // Start with first cluster selected
        selectCluster(1);
        updateClusterButtonStates();

        // Start timer at 50Hz
        startTimer(20);
    }

    ~ClustersTab() override
    {
        stopTimer();
        inputsTree.removeListener(this);
        configTree.removeListener(this);
    }

    void setStatusBar(StatusBar* bar)
    {
        statusBar = bar;
        setupHelpTextMappings();
    }

    std::unique_ptr<juce::ComponentTraverser> createKeyboardFocusTraverser() override
    {
        return std::make_unique<ColumnCircuitTraverser>(lfoCircuits);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(ColorScheme::get().background);

        float topY = 50.0f * layoutScale;
        float botY = static_cast<float>(getHeight()) - 10.0f * layoutScale;

        g.setColour(ColorScheme::get().chromeDivider);
        g.drawVerticalLine(dividerX1, topY, botY);
        g.drawVerticalLine(dividerX2, topY, botY);
    }

    void resized() override
    {
        layoutScale = static_cast<float>(getHeight()) / 932.0f;
        auto bounds = getLocalBounds().reduced(scaled(10));

        // ==================== CLUSTER SELECTOR BAR (full width) ====================
        auto selectorArea = bounds.removeFromTop(scaled(40));
        int buttonWidth = selectorArea.getWidth() / 10;
        for (int i = 0; i < 10; ++i)
        {
            clusterButtons[i]->setBounds(selectorArea.removeFromLeft(buttonWidth).reduced(scaled(2)));
        }

        bounds.removeFromTop(scaled(10));

        // Split into columns: narrow left (input list), wider center + right
        int totalWidth = bounds.getWidth();
        int leftW = totalWidth / 5;  // ~20% for the input list (60% of original 1/3)
        int colPad = scaled(10);
        auto leftPanel  = bounds.removeFromLeft(leftW).reduced(colPad, 0);
        dividerX1 = bounds.getX();
        auto centerPanel = bounds.removeFromLeft((bounds.getWidth()) / 2).reduced(colPad, 0);
        dividerX2 = bounds.getX();
        auto rightPanel  = bounds.reduced(colPad, 0);

        // ==================== LEFT PANEL - ASSIGNED INPUTS ====================
        assignedInputsLabel.setBounds(leftPanel.removeFromTop(scaled(20)));
        leftPanel.removeFromTop(scaled(5));
        inputsList.setBounds(leftPanel);

        // ==================== CENTER PANEL - REFERENCE + CONTROLS ====================
        // Row 1: Reference mode + position display + status (single compact row)
        auto infoRow = centerPanel.removeFromTop(scaled(24));
        referenceModeLabel.setBounds(infoRow.removeFromLeft(scaled(70)));
        referenceModeSelector.setBounds(infoRow.removeFromLeft(scaled(120)));
        infoRow.removeFromLeft(scaled(8));
        refPosXLabel.setBounds(infoRow.removeFromLeft(scaled(60)));
        refPosYLabel.setBounds(infoRow.removeFromLeft(scaled(60)));
        refPosZLabel.setBounds(infoRow.removeFromLeft(scaled(60)));
        statusLabel.setBounds(infoRow);
        refPosLabel.setBounds(juce::Rectangle<int>()); // hidden — axis letters in X/Y/Z labels
        controlsLabel.setBounds(juce::Rectangle<int>()); // hidden — controls are self-evident

        centerPanel.removeFromTop(scaled(16));

        // Row 2: All controls packed horizontally
        {
            // Labels row
            auto labelRow = centerPanel.removeFromTop(scaled(18));
            int ctrlWidth = centerPanel.getWidth();

            centerPanel.removeFromTop(scaled(2));

            // Controls row — use remaining height
            auto ctrlRow = centerPanel.removeFromTop(juce::jmin(scaled(160), centerPanel.getHeight()));

            // Layout: pos(joy) | gap | Z(slider) | gap | att(slider) | gap | rot(dial) | gap | scale(slider)
            int sliderW = scaled(43);
            int sliderPad = scaled(10);

            // Fixed: 3 sliders + 4 gaps
            int fixedW = sliderW * 3 + sliderPad * 4;
            int flexW = ctrlWidth - fixedW;
            // 2 round controls (pos + rot)
            int joySize = juce::jmin(ctrlRow.getHeight(), flexW / 2);
            int rotW = joySize;

            // Distribute leftover into slider gaps
            int leftover = flexW - joySize * 2;
            if (leftover > 0)
                sliderPad += leftover / 4;

            // Labels
            auto labelCopy = labelRow;
            positionLabel.setBounds(labelCopy.removeFromLeft(joySize));
            labelCopy.removeFromLeft(sliderPad);
            zSliderLabel.setBounds(labelCopy.removeFromLeft(sliderW));
            labelCopy.removeFromLeft(sliderPad);
            attenuationLabel.setBounds(labelCopy.removeFromLeft(sliderW));
            labelCopy.removeFromLeft(sliderPad);
            rotationLabel.setBounds(labelCopy.removeFromLeft(rotW));
            labelCopy.removeFromLeft(sliderPad);
            scaleLabel.setBounds(labelCopy.removeFromLeft(sliderW));

            // Controls
            auto joyArea = ctrlRow.removeFromLeft(joySize);
            positionJoystick.setBounds(joyArea.withSizeKeepingCentre(joySize, joySize).reduced(scaled(3)));
            ctrlRow.removeFromLeft(sliderPad);

            auto zArea = ctrlRow.removeFromLeft(sliderW);
            zSlider.setBounds(zArea.reduced(scaled(2)));
            ctrlRow.removeFromLeft(sliderPad);

            auto attenArea = ctrlRow.removeFromLeft(sliderW);
            attenuationSlider.setBounds(attenArea.reduced(scaled(2)));
            ctrlRow.removeFromLeft(sliderPad);

            auto rotArea = ctrlRow.removeFromLeft(rotW);
            auto dialBounds = rotArea.withSizeKeepingCentre(joySize, joySize).reduced(scaled(3));
            rotationDial.setBounds(dialBounds);

            // Plane selector — centered inside the rotation dial ring
            int comboW = scaled(60);
            int comboH = scaled(24);
            planeSelector.setBounds(
                dialBounds.getCentreX() - comboW / 2,
                dialBounds.getCentreY() - comboH / 2,
                comboW, comboH);
            ctrlRow.removeFromLeft(sliderPad);

            auto scaleArea = ctrlRow.removeFromLeft(sliderW);
            scaleSlider.setBounds(scaleArea.reduced(scaled(2)));
        }

        // ==================== CENTER PANEL - LFO PRESET GRID ====================
        centerPanel.removeFromTop (scaled (10));

        // Button row at bottom of center column
        auto presetButtonRow = centerPanel.removeFromBottom (scaled (28));
        centerPanel.removeFromBottom (scaled (4));

        // 4x4 grid fills remaining center area
        if (centerPanel.getHeight() > 0)
        {
            int cols = 4;
            int rows = 4;
            int tileW = centerPanel.getWidth() / cols;
            int tileH = centerPanel.getHeight() / rows;
            int pad = scaled (2);

            for (int r = 0; r < rows; ++r)
            {
                for (int c = 0; c < cols; ++c)
                {
                    int idx = r * cols + c;
                    presetTiles[idx]->setBounds (
                        juce::Rectangle<int> (
                            centerPanel.getX() + c * tileW,
                            centerPanel.getY() + r * tileH,
                            tileW, tileH
                        ).reduced (pad));
                }
            }
        }

        // Button row: [Stop All] [Export] [Import]
        {
            int btnW = presetButtonRow.getWidth() / 3;
            stopAllLFOButton.setBounds (presetButtonRow.removeFromLeft (btnW).reduced (scaled (2), 0));
            exportPresetsButton.setBounds (presetButtonRow.removeFromLeft (btnW).reduced (scaled (2), 0));
            importPresetsButton.setBounds (presetButtonRow.reduced (scaled (2), 0));
        }

        // ==================== RIGHT PANEL - LFO ====================
        layoutLFOPanel(rightPanel);

        WfsLookAndFeel::scaleTextEditorFonts(*this, layoutScale);
    }

    // ListBoxModel implementation
    int getNumRows() override
    {
        return static_cast<int>(assignedInputs.size());
    }

    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override
    {
        // Handled by ClusterInputRowComponent via refreshComponentForRow
        juce::ignoreUnused(rowNumber, g, width, height, rowIsSelected);
    }

    juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected,
                                             juce::Component* existingComponentToUpdate) override
    {
        if (rowNumber < 0 || rowNumber >= static_cast<int>(assignedInputs.size()))
        {
            delete existingComponentToUpdate;
            return nullptr;
        }

        auto* row = dynamic_cast<ClusterInputRowComponent*>(existingComponentToUpdate);
        if (row == nullptr)
        {
            delete existingComponentToUpdate;
            row = new ClusterInputRowComponent(*this);
        }

        int idx = assignedInputs[static_cast<size_t>(rowNumber)];
        bool tracked = isInputFullyTracked(idx);
        bool first   = (rowNumber == 0);
        int refMode  = referenceModeSelector.getSelectedId();
        juce::String name = parameters.getInputParam(idx, "inputName").toString();
        bool beingDragged = (draggingInputIdx == idx);
        row->update(rowNumber, idx, tracked, first, isRowSelected, refMode, name, beingDragged);

        return row;
    }

    void listBoxItemClicked(int row, const juce::MouseEvent&) override
    {
        juce::ignoreUnused(row);
    }

    // Public methods for keyboard navigation
    void setSelectedCluster(int clusterIndex)
    {
        if (clusterIndex >= 1 && clusterIndex <= 10)
            selectCluster(clusterIndex);
    }

    int getSelectedCluster() const noexcept { return selectedCluster; }

    std::function<void (int)> onClusterSelected;

    void setHighlightedPresetTile (int index)
    {
        highlightedPresetTile = index;
        for (int i = 0; i < presetTiles.size(); ++i)
            presetTiles[i]->repaint();
    }

    // Stream Deck access to preset / LFO control methods
    void sdStorePreset (int idx)             { storePreset (idx); }
    void sdRecallPreset (int idx)            { recallPreset (idx); }
    void sdActivateCurrentClusterLFO()       { activateCurrentClusterLFO(); }
    void sdStopAllClusterLFOs()              { stopAllClusterLFOs(); }

    bool isAnyClusterLFOActive() const
    {
        for (int c = 1; c <= 10; ++c)
            if (clusterLFOProcessor.isActive(c))
                return true;
        return false;
    }

    void getClusterLFOOffset (int inputIdx, float& x, float& y, float& z) const
    {
        if (inputIdx >= 0 && inputIdx < static_cast<int> (clusterLFOInputOffsets.size()))
        {
            x = clusterLFOInputOffsets[static_cast<size_t> (inputIdx)][0];
            y = clusterLFOInputOffsets[static_cast<size_t> (inputIdx)][1];
            z = clusterLFOInputOffsets[static_cast<size_t> (inputIdx)][2];
        }
        else { x = y = z = 0.0f; }
    }

    void selectNextCluster()
    {
        int next = selectedCluster + 1;
        if (next > 10)
            next = 1;
        selectCluster(next);
    }

    void selectPreviousCluster()
    {
        int prev = selectedCluster - 1;
        if (prev < 1)
            prev = 10;
        selectCluster(prev);
    }

private:
    enum class Plane { XY = 0, XZ = 1, YZ = 2 };

    WfsParameters& parameters;
    juce::ValueTree inputsTree;
    juce::ValueTree configTree;

    int selectedCluster = 1;
    Plane currentPlane = Plane::XY;
    float previousDialAngle = 0.0f;
    float layoutScale = 1.0f;
    bool isLoadingParameters = false;

    /** Scale a reference pixel value by layoutScale with a 65% minimum floor */
    int scaled(int ref) const { return juce::jmax(static_cast<int>(ref * 0.65f), static_cast<int>(ref * layoutScale)); }

    std::vector<int> assignedInputs;
    int draggingInputIdx = -1;  // channel index being dragged, -1 = not dragging
    int dividerX1 = 0, dividerX2 = 0;  // column separator X positions

    // Cluster selector buttons
    juce::OwnedArray<ClusterButton> clusterButtons;

    // Assigned inputs panel
    juce::Label assignedInputsLabel;
    juce::ListBox inputsList { {}, this };

    // Reference + status
    juce::Label referenceModeLabel;
    juce::ComboBox referenceModeSelector;
    juce::Label refPosLabel, refPosXLabel, refPosYLabel, refPosZLabel;
    juce::Label statusLabel;

    // Controls panel
    juce::Label controlsLabel;
    juce::Label positionLabel;
    WfsJoystickComponent positionJoystick;
    juce::Label zSliderLabel;
    WfsAutoCenterSlider zSlider { WfsAutoCenterSlider::Orientation::vertical };
    juce::Label attenuationLabel;
    WfsAutoCenterSlider attenuationSlider { WfsAutoCenterSlider::Orientation::vertical };
    juce::Label rotationLabel;
    WfsEndlessDial rotationDial;
    juce::Label scaleLabel;
    WfsAutoCenterSlider scaleSlider { WfsAutoCenterSlider::Orientation::vertical };
    juce::Label planeLabel;
    juce::ComboBox planeSelector;

    // StatusBar
    StatusBar* statusBar = nullptr;
    std::map<juce::Component*, juce::String> helpTextMap;
    std::map<juce::Component*, juce::String> oscMethodMap;

    // ==================== LFO PRESET TILES ====================

    class LFOPresetTile : public juce::Component,
                          public juce::Label::Listener
    {
    public:
        LFOPresetTile (ClustersTab& o, int index) : owner (o), presetIndex (index)
        {
            nameLabel.setFont (juce::FontOptions().withHeight (22.0f));
            nameLabel.setJustificationType (juce::Justification::centredLeft);
            nameLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.8f));
            nameLabel.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
            nameLabel.setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
            nameLabel.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xFF2A2A2A));
            nameLabel.setColour (juce::TextEditor::textColourId, juce::Colours::white);
            nameLabel.setEditable (false, true, false); // single-click = no edit, double = no, triple = no; edit on slow click
            nameLabel.addListener (this);
            addAndMakeVisible (nameLabel);
            updateFromValueTree();
        }

        void paint (juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();
            float cornerRadius = 4.0f;

            // Background
            auto bgColour = populated ? juce::Colour (0xFF2E4A3E) : ColorScheme::get().surfaceCard;
            if (isMouseOver (false))
                bgColour = bgColour.brighter (0.15f);
            g.setColour (bgColour);
            g.fillRoundedRectangle (bounds, cornerRadius);

            // Border
            g.setColour (ColorScheme::get().chromeDivider);
            g.drawRoundedRectangle (bounds.reduced (0.5f), cornerRadius, 1.0f);

            // Stream Deck highlight
            if (presetIndex == owner.highlightedPresetTile)
            {
                g.setColour (juce::Colour (0xFF6A5ACD).withAlpha (0.6f));
                g.drawRoundedRectangle (bounds.reduced (1.5f), cornerRadius, 2.5f);
            }

            // Number in top-left
            float fontSize = juce::jmax (12.0f, bounds.getHeight() * 0.22f);
            g.setFont (juce::FontOptions (fontSize).withStyle ("Bold"));
            g.setColour (ColorScheme::get().textSecondary);
            g.drawText (juce::String (presetIndex + 1), bounds.reduced (4.0f, 2.0f),
                        juce::Justification::topLeft);

            // Active axes indicators at bottom-right
            if (populated && activeAxes.isNotEmpty())
            {
                float axisFontSize = juce::jmax (8.0f, bounds.getHeight() * 0.16f);
                g.setFont (juce::FontOptions (axisFontSize));

                // Draw each axis letter with its own colour
                float xPos = bounds.getRight() - 4.0f;
                float yPos = bounds.getBottom() - axisFontSize - 2.0f;
                for (int i = activeAxes.length() - 1; i >= 0; --i)
                {
                    auto ch = activeAxes.substring (i, i + 1);
                    float charW = axisFontSize * 0.7f;
                    xPos -= charW;

                    if (ch == "R")      g.setColour (juce::Colour (0xFF42A5F5)); // blue
                    else if (ch == "S") g.setColour (juce::Colour (0xFFAB47BC)); // purple
                    else                g.setColour (juce::Colour (0xFF66BB6A)); // green for XYZ

                    g.drawText (ch, juce::Rectangle<float> (xPos, yPos, charW, axisFontSize),
                                juce::Justification::centred);
                }
            }
        }

        void resized() override
        {
            auto bounds = getLocalBounds();
            // Name label at top, same height as the number text
            int headerH = juce::jmax (16, bounds.getHeight() / 4);
            int numWidth = juce::jmax (16, bounds.getWidth() / 4);
            nameLabel.setBounds (bounds.removeFromTop (headerH).withTrimmedLeft (numWidth).reduced (2, 0));
            nameLabel.setFont (juce::FontOptions().withHeight (juce::jmax (11.0f, headerH * 0.7f)));
        }

        void mouseDown (const juce::MouseEvent& e) override
        {
            // Middle-click or right-click (2-finger tap on trackpad) → STORE
            if (e.mods.isMiddleButtonDown() || e.mods.isPopupMenu())
            {
                owner.storePreset (presetIndex);
                return;
            }

            // Left single click
            if (e.mods.isLeftButtonDown())
            {
                if (populated)
                    owner.recallPreset (presetIndex);
            }
        }

        void mouseDoubleClick (const juce::MouseEvent& e) override
        {
            if (e.mods.isLeftButtonDown() && populated)
            {
                owner.recallPreset (presetIndex);
                owner.activateCurrentClusterLFO();
            }
        }

        void mouseEnter (const juce::MouseEvent&) override { repaint(); }
        void mouseExit  (const juce::MouseEvent&) override { repaint(); }

        void labelTextChanged (juce::Label* label) override
        {
            if (label == &nameLabel)
            {
                auto preset = owner.parameters.getValueTreeState().ensureClusterLFOPreset (presetIndex);
                if (preset.isValid())
                    preset.setProperty (WFSParameterIDs::clusterLFOPresetName, nameLabel.getText(), nullptr);
            }
        }

        void updateFromValueTree()
        {
            auto preset = owner.parameters.getValueTreeState().ensureClusterLFOPreset (presetIndex);
            if (! preset.isValid()) { populated = false; repaint(); return; }

            using namespace WFSParameterIDs;
            const juce::Identifier shapeIds[] = { clusterLFOshapeX, clusterLFOshapeY, clusterLFOshapeZ,
                                                  clusterLFOshapeRot, clusterLFOshapeScale };
            const char axisChars[] = { 'X', 'Y', 'Z', 'R', 'S' };

            populated = false;
            activeAxes.clear();
            for (int i = 0; i < 5; ++i)
            {
                int shape = static_cast<int> (preset.getProperty (shapeIds[i], 0));
                if (shape != 0) // 0 = Off
                {
                    populated = true;
                    activeAxes += axisChars[i];
                }
            }

            juce::String presetName = preset.getProperty (clusterLFOPresetName, "").toString();
            nameLabel.setText (presetName.isEmpty() ? "---" : presetName, juce::dontSendNotification);
            nameLabel.setAlpha (populated ? 1.0f : 0.4f);
            repaint();
        }

        bool isPopulated() const { return populated; }

    private:
        ClustersTab& owner;
        int presetIndex;
        juce::Label nameLabel;
        bool populated = false;
        juce::String activeAxes;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LFOPresetTile)
    };

    juce::OwnedArray<LFOPresetTile> presetTiles;
    LongPressButton stopAllLFOButton { 500 }; // 500ms long press to prevent accidental stop
    juce::TextButton exportPresetsButton;
    juce::TextButton importPresetsButton;
    std::shared_ptr<juce::FileChooser> presetFileChooser;
    int highlightedPresetTile = -1;

    // ==================== LFO PANEL ====================
    ClusterLFOProcessor clusterLFOProcessor;
    std::vector<std::array<float, 3>> clusterLFOInputOffsets;  // per-input [x, y, z] offsets

    juce::Label lfoSectionLabel;
    juce::TextButton lfoActiveButton;

    // LFO global controls
    juce::Label lfoPeriodLabel;
    WfsBasicDial lfoPeriodDial;
    juce::Label lfoPeriodValueLabel;

    juce::Label lfoPhaseLabel;
    WfsRotationDial lfoPhaseDial;
    juce::Label lfoPhaseValueLabel;

    WfsLFOProgressDial lfoProgressDial;

    // Per-axis LFO rows: X, Y, Z, Rot, Scale
    struct LFOAxisRow
    {
        juce::Label axisLabel;
        juce::ComboBox shapeSelector;
        juce::Label amplitudeLabel;
        // Amplitude: WfsStandardSlider for XYZ, WfsBidirectionalSlider for Rot/Scale
        std::unique_ptr<juce::Component> amplitudeSlider;
        juce::Label amplitudeValueLabel;
        juce::Label rateLabel;
        WfsBidirectionalSlider rateSlider;
        juce::Label rateValueLabel;
        juce::Label phaseLabel;
        WfsRotationDial phaseDial;
        juce::Label phaseValueLabel;
        WfsLFOOutputSlider outputSlider;
    };
    LFOAxisRow lfoRows[5]; // 0=X, 1=Y, 2=Z, 3=Rot, 4=Scale

    //==========================================================================
    // LFO Panel Setup
    //==========================================================================

    void setupLFOPanel()
    {
        using namespace WFSParameterIDs;
        using namespace WFSParameterDefaults;

        // Section label
        addAndMakeVisible(lfoSectionLabel);
        lfoSectionLabel.setText(LOC("clusters.lfo.labels.section"), juce::dontSendNotification);
        lfoSectionLabel.setFont(juce::FontOptions().withHeight(16.0f).withStyle("Bold"));

        // Active button
        addAndMakeVisible(lfoActiveButton);
        lfoActiveButton.setButtonText(LOC("clusters.toggles.lfoOff"));
        lfoActiveButton.setClickingTogglesState(true);
        lfoActiveButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF3A3A3A));
        lfoActiveButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF26A69A));
        lfoActiveButton.onClick = [this]() {
            if (isLoadingParameters) return;
            bool active = lfoActiveButton.getToggleState();
            lfoActiveButton.setButtonText(active ? LOC("clusters.toggles.lfoOn") : LOC("clusters.toggles.lfoOff"));
            saveClusterLFOParam(clusterLFOactive, active ? 1 : 0);
            updateLFOAlpha();
        };

        // Period dial
        addAndMakeVisible(lfoPeriodLabel);
        lfoPeriodLabel.setText(LOC("clusters.lfo.labels.period"), juce::dontSendNotification);
        lfoPeriodLabel.setFont(juce::FontOptions().withHeight(15.0f));
        lfoPeriodLabel.setJustificationType(juce::Justification::centred);

        addAndMakeVisible(lfoPeriodDial);
        lfoPeriodDial.setColours(juce::Colour(0xFF3A3A3A), juce::Colour(0xFFD4A017), juce::Colours::white);
        lfoPeriodDial.onValueChanged = [this](float val) {
            if (isLoadingParameters) return;
            // Log scale: 0->1 maps to 0.01->100s
            float periodSec = 0.01f * std::pow(10000.0f, val);
            lfoPeriodValueLabel.setText(juce::String(periodSec, 2) + " s", juce::dontSendNotification);
            saveClusterLFOParam(WFSParameterIDs::clusterLFOperiod, periodSec);
        };

        addAndMakeVisible(lfoPeriodValueLabel);
        lfoPeriodValueLabel.setText("5.00 s", juce::dontSendNotification);
        lfoPeriodValueLabel.setFont(juce::FontOptions().withHeight(15.0f));
        lfoPeriodValueLabel.setJustificationType(juce::Justification::centred);
        setupEditableValueLabel(lfoPeriodValueLabel);

        // Phase dial
        addAndMakeVisible(lfoPhaseLabel);
        lfoPhaseLabel.setText(LOC("clusters.lfo.labels.phase"), juce::dontSendNotification);
        lfoPhaseLabel.setFont(juce::FontOptions().withHeight(15.0f));
        lfoPhaseLabel.setJustificationType(juce::Justification::centred);

        addAndMakeVisible(lfoPhaseDial);
        lfoPhaseDial.setColours(juce::Colour(0xFF3A3A3A), juce::Colour(0xFFE6B422), juce::Colours::white);
        lfoPhaseDial.onAngleChanged = [this](float angle) {
            if (isLoadingParameters) return;
            int deg = static_cast<int>(std::round(angle));
            lfoPhaseValueLabel.setText(juce::String(deg) + juce::String::charToString(0x00B0), juce::dontSendNotification);
            saveClusterLFOParam(WFSParameterIDs::clusterLFOphase, deg);
        };

        addAndMakeVisible(lfoPhaseValueLabel);
        lfoPhaseValueLabel.setText(juce::String("0") + juce::String::charToString(0x00B0), juce::dontSendNotification);
        lfoPhaseValueLabel.setFont(juce::FontOptions().withHeight(15.0f));
        lfoPhaseValueLabel.setJustificationType(juce::Justification::centred);
        setupEditableValueLabel(lfoPhaseValueLabel);

        // Progress dial
        addAndMakeVisible(lfoProgressDial);

        // LFO shape names (reuse from inputs)
        juce::StringArray shapeNames;
        shapeNames.add(LOC("inputs.lfo.shapes.off"));
        shapeNames.add(LOC("inputs.lfo.shapes.sine"));
        shapeNames.add(LOC("inputs.lfo.shapes.square"));
        shapeNames.add(LOC("inputs.lfo.shapes.sawtooth"));
        shapeNames.add(LOC("inputs.lfo.shapes.triangle"));
        shapeNames.add(LOC("inputs.lfo.shapes.keystone"));
        shapeNames.add(LOC("inputs.lfo.shapes.log"));
        shapeNames.add(LOC("inputs.lfo.shapes.exp"));
        shapeNames.add(LOC("inputs.lfo.shapes.random"));

        // Axis names and parameter IDs
        struct AxisInfo {
            const char* locKey;
            juce::Identifier shapeId, rateId, amplitudeId, phaseId;
            bool isRotation;
            bool isScale;
        };

        const AxisInfo axisInfos[5] = {
            { "clusters.lfo.labels.x",     clusterLFOshapeX,     clusterLFOrateX,     clusterLFOamplitudeX,     clusterLFOphaseX,     false, false },
            { "clusters.lfo.labels.y",     clusterLFOshapeY,     clusterLFOrateY,     clusterLFOamplitudeY,     clusterLFOphaseY,     false, false },
            { "clusters.lfo.labels.z",     clusterLFOshapeZ,     clusterLFOrateZ,     clusterLFOamplitudeZ,     clusterLFOphaseZ,     false, false },
            { "clusters.lfo.labels.rot",   clusterLFOshapeRot,   clusterLFOrateRot,   clusterLFOamplitudeRot,   clusterLFOphaseRot,   true,  false },
            { "clusters.lfo.labels.scaleLfo", clusterLFOshapeScale, clusterLFOrateScale, clusterLFOamplitudeScale, clusterLFOphaseScale, false, true  },
        };

        for (int a = 0; a < 5; ++a)
        {
            auto& row = lfoRows[a];
            auto& info = axisInfos[a];

            // Axis label
            addAndMakeVisible(row.axisLabel);
            row.axisLabel.setText(LOC(info.locKey), juce::dontSendNotification);
            row.axisLabel.setFont(juce::FontOptions().withHeight(16.0f).withStyle("Bold"));

            // Shape selector
            addAndMakeVisible(row.shapeSelector);
            for (int s = 0; s < shapeNames.size(); ++s)
                row.shapeSelector.addItem(shapeNames[s], s + 1);
            row.shapeSelector.setSelectedId(1, juce::dontSendNotification);

            const int axisIdx = a;
            row.shapeSelector.onChange = [this, axisIdx]() {
                if (isLoadingParameters) return;
                int shape = lfoRows[axisIdx].shapeSelector.getSelectedId() - 1;
                saveClusterLFOParam(getAxisShapeId(axisIdx), shape);
                updateLFOAlpha();
            };

            // Amplitude slider
            addAndMakeVisible(row.amplitudeLabel);
            if (info.isRotation)
                row.amplitudeLabel.setText(LOC("clusters.lfo.labels.angle"), juce::dontSendNotification);
            else if (info.isScale)
                row.amplitudeLabel.setText(LOC("clusters.lfo.labels.ratio"), juce::dontSendNotification);
            else
                row.amplitudeLabel.setText(LOC("clusters.lfo.labels.amplitude"), juce::dontSendNotification);
            row.amplitudeLabel.setFont(juce::FontOptions().withHeight(juce::jmax(11.0f, 15.0f * layoutScale)));

            if (info.isRotation)
            {
                // Rotation amplitude: bidirectional -360..+360 degrees
                auto* slider = new WfsBidirectionalSlider();
                slider->setTrackColours(ColorScheme::get().sliderTrackBg, juce::Colour(0xFF2196F3));
                slider->onValueChanged = [this, axisIdx](float val) {
                    if (isLoadingParameters) return;
                    int deg = static_cast<int>(std::round(val * 360.0f));
                    lfoRows[axisIdx].amplitudeValueLabel.setText(juce::String(deg) + juce::String::charToString(0x00B0), juce::dontSendNotification);
                    saveClusterLFOParam(getAxisAmplitudeId(axisIdx), deg);
                };
                row.amplitudeSlider.reset(slider);
            }
            else if (info.isScale)
            {
                // Scale amplitude: bidirectional, log mapping 0.1x..10x
                auto* slider = new WfsBidirectionalSlider();
                slider->setTrackColours(ColorScheme::get().sliderTrackBg, juce::Colour(0xFF9C27B0));
                slider->onValueChanged = [this, axisIdx](float val) {
                    if (isLoadingParameters) return;
                    // slider -1..+1 -> amplitude 0.1..10 via pow(10, val)
                    float ampScale = std::pow(10.0f, val);
                    lfoRows[axisIdx].amplitudeValueLabel.setText(juce::String(ampScale, 2) + "x", juce::dontSendNotification);
                    saveClusterLFOParam(getAxisAmplitudeId(axisIdx), ampScale);
                };
                row.amplitudeSlider.reset(slider);
            }
            else
            {
                // XYZ amplitude: standard slider 0..50m
                auto* slider = new WfsStandardSlider();
                slider->setTrackColours(ColorScheme::get().sliderTrackBg, juce::Colour(0xFF00BCD4));
                slider->onValueChanged = [this, axisIdx](float val) {
                    if (isLoadingParameters) return;
                    float meters = val * 50.0f;
                    lfoRows[axisIdx].amplitudeValueLabel.setText(juce::String(meters, 1) + " m", juce::dontSendNotification);
                    saveClusterLFOParam(getAxisAmplitudeId(axisIdx), meters);
                };
                row.amplitudeSlider.reset(slider);
            }
            addAndMakeVisible(row.amplitudeSlider.get());

            addAndMakeVisible(row.amplitudeValueLabel);
            row.amplitudeValueLabel.setFont(juce::FontOptions().withHeight(juce::jmax(11.0f, 15.0f * layoutScale)));
            row.amplitudeValueLabel.setJustificationType(juce::Justification::centredRight);
            setupEditableValueLabel(row.amplitudeValueLabel);

            // Rate slider (bidirectional, log scale 0.01..100x)
            addAndMakeVisible(row.rateLabel);
            row.rateLabel.setText(LOC("clusters.lfo.labels.rate"), juce::dontSendNotification);
            row.rateLabel.setFont(juce::FontOptions().withHeight(juce::jmax(11.0f, 15.0f * layoutScale)));

            addAndMakeVisible(row.rateSlider);
            row.rateSlider.setTrackColours(ColorScheme::get().sliderTrackBg, juce::Colour(0xFFD4A017));
            row.rateSlider.onValueChanged = [this, axisIdx](float val) {
                if (isLoadingParameters) return;
                float rate = std::pow(100.0f, val); // -1->0.01, 0->1, +1->100
                lfoRows[axisIdx].rateValueLabel.setText(juce::String(rate, 2) + "x", juce::dontSendNotification);
                saveClusterLFOParam(getAxisRateId(axisIdx), rate);
            };

            addAndMakeVisible(row.rateValueLabel);
            row.rateValueLabel.setText("1.00x", juce::dontSendNotification);
            row.rateValueLabel.setFont(juce::FontOptions().withHeight(juce::jmax(11.0f, 15.0f * layoutScale)));
            row.rateValueLabel.setJustificationType(juce::Justification::centredRight);
            setupEditableValueLabel(row.rateValueLabel);

            // Phase label + dial
            addAndMakeVisible(row.phaseLabel);
            row.phaseLabel.setText(LOC("clusters.lfo.labels.phase"), juce::dontSendNotification);
            row.phaseLabel.setFont(juce::FontOptions().withHeight(juce::jmax(11.0f, 15.0f * layoutScale)));
            row.phaseLabel.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(row.phaseDial);
            row.phaseDial.setColours(juce::Colour(0xFF3A3A3A), juce::Colour(0xFFE6B422), juce::Colours::white);
            row.phaseDial.onAngleChanged = [this, axisIdx](float angle) {
                if (isLoadingParameters) return;
                int deg = static_cast<int>(std::round(angle));
                lfoRows[axisIdx].phaseValueLabel.setText(juce::String(deg) + juce::String::charToString(0x00B0), juce::dontSendNotification);
                saveClusterLFOParam(getAxisPhaseId(axisIdx), deg);
            };

            addAndMakeVisible(row.phaseValueLabel);
            row.phaseValueLabel.setText(juce::String("0") + juce::String::charToString(0x00B0), juce::dontSendNotification);
            row.phaseValueLabel.setFont(juce::FontOptions().withHeight(juce::jmax(11.0f, 15.0f * layoutScale)));
            row.phaseValueLabel.setJustificationType(juce::Justification::centred);
            setupEditableValueLabel(row.phaseValueLabel);

            // Output indicator (read-only)
            addAndMakeVisible(row.outputSlider);
            row.outputSlider.setTrackColour(juce::Colour(0xFF808080));  // Mid grey (display-only)
        }
    }

    //==========================================================================
    // LFO Panel Layout
    //==========================================================================

    void layoutLFOPanel(juce::Rectangle<int> area)
    {
        // Sizing constants
        const int rowHeight    = scaled(24);     // shape selector row
        const int sliderHeight = scaled(40);     // amplitude & rate sliders (thicker, matches InputsTab)
        const int vizHeight    = scaled(16);     // output viz indicator (thin, read-only)
        const int spacing      = scaled(4);      // inter-axis gap
        const int vizPad       = scaled(2);      // padding above/below viz
        const int labelWidth   = scaled(50);     // axis label ("X", "Y", etc.)
        const int valueWidth   = scaled(55);     // value labels (wider for larger font)
        const int selectorWidth = scaled(100);   // waveform combobox
        const int sliderLabelW = scaled(75);     // "Amplitude:", "Rate:" labels (wider for 15pt)
        const int phaseValueH  = scaled(16);     // phase value label height
        const int dialSize = juce::jmax(40, static_cast<int>(65.0f * layoutScale));

        // ==================== HEADER ====================
        // [LFO + ON/OFF] gap [progress] gap [Period dial] gap [Phase dial]
        // Equal gaps between all 4 element groups
        const int headerLabelH = scaled(16);
        const int headerValueH = scaled(14);
        auto headerArea = area.removeFromTop(headerLabelH + dialSize + headerValueH);
        {
            int lfoLabelW = scaled(35);
            int buttonW = scaled(80);
            int innerGap = scaled(4); // small gap between LFO label and button
            int leftGroupW = lfoLabelW + innerGap + buttonW;

            // 4 elements, 3 equal gaps
            int fixedW = leftGroupW + dialSize + dialSize + dialSize; // left group + progress + period + phase
            int gap = juce::jmax(scaled(6), (headerArea.getWidth() - fixedW) / 3);

            // [LFO label + button] — vertically centered
            auto leftGroup = headerArea.removeFromLeft(leftGroupW);
            {
                auto lblArea = leftGroup.removeFromLeft(lfoLabelW);
                lfoSectionLabel.setBounds(lblArea.withSizeKeepingCentre(lfoLabelW, rowHeight));
                leftGroup.removeFromLeft(innerGap);
                lfoActiveButton.setBounds(leftGroup.withSizeKeepingCentre(buttonW, rowHeight));
            }
            headerArea.removeFromLeft(gap);

            // [Progress dial] — vertically centered
            auto progressArea = headerArea.removeFromLeft(dialSize);
            lfoProgressDial.setBounds(progressArea.withSizeKeepingCentre(dialSize, dialSize));
            headerArea.removeFromLeft(gap);

            // [Period dial block] — label on top, dial in middle, value below
            auto periodBlock = headerArea.removeFromLeft(dialSize);
            {
                lfoPeriodLabel.setBounds(periodBlock.removeFromTop(headerLabelH));
                auto dialArea = periodBlock.removeFromTop(dialSize);
                lfoPeriodDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
                lfoPeriodValueLabel.setBounds(periodBlock.removeFromTop(headerValueH));
            }
            headerArea.removeFromLeft(gap);

            // [Phase dial block]
            auto phaseBlock = headerArea.removeFromLeft(dialSize);
            {
                lfoPhaseLabel.setBounds(phaseBlock.removeFromTop(headerLabelH));
                auto dialArea = phaseBlock.removeFromTop(dialSize);
                lfoPhaseDial.setBounds(dialArea.withSizeKeepingCentre(dialSize, dialSize));
                lfoPhaseValueLabel.setBounds(phaseBlock.removeFromTop(headerValueH));
            }
        }

        area.removeFromTop(spacing * 2);

        // ==================== PER-AXIS ROWS (5 axes) ====================
        // Calculate available height per axis
        int totalAxisHeight = area.getHeight() - spacing * 4; // 4 gaps between 5 axes
        int axisBlockHeight = totalAxisHeight / 5;

        // Phase dial spans all 3 slider rows: amp + vizPad + viz + vizPad + rate
        int sliderStackH = sliderHeight * 2 + vizHeight + vizPad * 2;
        int phaseDialSize = sliderStackH;

        for (int a = 0; a < 5; ++a)
        {
            auto& row = lfoRows[a];
            auto axisArea = area.removeFromTop(axisBlockHeight);

            // Line 1: [Axis label] [Shape ComboBox] ... [Phase label on right]
            auto line1 = axisArea.removeFromTop(rowHeight);
            row.axisLabel.setBounds(line1.removeFromLeft(labelWidth));
            row.shapeSelector.setBounds(line1.removeFromLeft(juce::jmin(selectorWidth, line1.getWidth())));
            row.phaseLabel.setBounds(line1.removeFromRight(phaseDialSize + scaled(4)));

            axisArea.removeFromTop(spacing);

            // Right column: phase dial + value (spans all 3 slider rows)
            auto phaseColumn = axisArea.removeFromRight(phaseDialSize + scaled(4));
            phaseColumn.removeFromLeft(scaled(4));
            row.phaseDial.setBounds(phaseColumn.removeFromTop(phaseDialSize)
                                    .withSizeKeepingCentre(phaseDialSize, phaseDialSize));
            auto phaseValArea = phaseColumn.removeFromTop(phaseValueH);
            row.phaseValueLabel.setBounds(phaseValArea.withSizeKeepingCentre(phaseDialSize / 2, phaseValueH));

            // Row 2: Amplitude slider
            auto line2 = axisArea.removeFromTop(sliderHeight);
            row.amplitudeLabel.setBounds(line2.removeFromLeft(sliderLabelW));
            auto ampValArea = line2.removeFromRight(valueWidth);
            row.amplitudeValueLabel.setBounds(ampValArea.withSizeKeepingCentre(valueWidth, scaled(24)));
            row.amplitudeSlider->setBounds(line2);

            axisArea.removeFromTop(vizPad);

            // Viz: Output indicator (between amplitude and rate)
            auto vizLine = axisArea.removeFromTop(vizHeight);
            vizLine.removeFromLeft(sliderLabelW);
            vizLine.removeFromRight(valueWidth);
            row.outputSlider.setBounds(vizLine);

            axisArea.removeFromTop(vizPad);

            // Row 3: Rate slider
            auto line3 = axisArea.removeFromTop(sliderHeight);
            row.rateLabel.setBounds(line3.removeFromLeft(sliderLabelW));
            auto rateValArea = line3.removeFromRight(valueWidth);
            row.rateValueLabel.setBounds(rateValArea.withSizeKeepingCentre(valueWidth, scaled(24)));
            row.rateSlider.setBounds(line3);

            if (a < 4)
                area.removeFromTop(spacing);
        }
    }

    //==========================================================================
    // LFO Parameter Helpers
    //==========================================================================

    juce::Identifier getAxisShapeId(int axisIdx) const
    {
        using namespace WFSParameterIDs;
        const juce::Identifier ids[] = { clusterLFOshapeX, clusterLFOshapeY, clusterLFOshapeZ, clusterLFOshapeRot, clusterLFOshapeScale };
        return ids[axisIdx];
    }

    juce::Identifier getAxisRateId(int axisIdx) const
    {
        using namespace WFSParameterIDs;
        const juce::Identifier ids[] = { clusterLFOrateX, clusterLFOrateY, clusterLFOrateZ, clusterLFOrateRot, clusterLFOrateScale };
        return ids[axisIdx];
    }

    juce::Identifier getAxisAmplitudeId(int axisIdx) const
    {
        using namespace WFSParameterIDs;
        const juce::Identifier ids[] = { clusterLFOamplitudeX, clusterLFOamplitudeY, clusterLFOamplitudeZ, clusterLFOamplitudeRot, clusterLFOamplitudeScale };
        return ids[axisIdx];
    }

    juce::Identifier getAxisPhaseId(int axisIdx) const
    {
        using namespace WFSParameterIDs;
        const juce::Identifier ids[] = { clusterLFOphaseX, clusterLFOphaseY, clusterLFOphaseZ, clusterLFOphaseRot, clusterLFOphaseScale };
        return ids[axisIdx];
    }

    void saveClusterLFOParam(const juce::Identifier& paramId, const juce::var& value)
    {
        if (isLoadingParameters || selectedCluster < 1) return;
        auto lfoSection = parameters.getValueTreeState().getClusterLFOSection(selectedCluster);
        if (lfoSection.isValid())
            lfoSection.setProperty(paramId, value, nullptr);
    }

    //==========================================================================
    // LFO Preset Tile Operations
    //==========================================================================

    /** List of LFO properties to copy between presets and cluster LFO sections */
    static constexpr int numLFOPresetProps = 22;

    const juce::Identifier& getLFOPresetPropId (int i) const
    {
        using namespace WFSParameterIDs;
        static const juce::Identifier ids[] = {
            clusterLFOperiod, clusterLFOphase,
            clusterLFOshapeX, clusterLFOshapeY, clusterLFOshapeZ, clusterLFOshapeRot, clusterLFOshapeScale,
            clusterLFOrateX, clusterLFOrateY, clusterLFOrateZ, clusterLFOrateRot, clusterLFOrateScale,
            clusterLFOamplitudeX, clusterLFOamplitudeY, clusterLFOamplitudeZ, clusterLFOamplitudeRot, clusterLFOamplitudeScale,
            clusterLFOphaseX, clusterLFOphaseY, clusterLFOphaseZ, clusterLFOphaseRot, clusterLFOphaseScale
        };
        return ids[i];
    }

    void storePreset (int presetIndex)
    {
        if (selectedCluster < 1) return;
        auto lfoSection = parameters.getValueTreeState().getClusterLFOSection (selectedCluster);
        if (! lfoSection.isValid()) return;

        auto preset = parameters.getValueTreeState().ensureClusterLFOPreset (presetIndex);
        if (! preset.isValid()) return;

        for (int i = 0; i < numLFOPresetProps; ++i)
            preset.setProperty (getLFOPresetPropId (i), lfoSection.getProperty (getLFOPresetPropId (i)), nullptr);

        presetTiles[presetIndex]->updateFromValueTree();

        if (statusBar != nullptr)
            statusBar->showTemporaryMessage (
                LOC ("clusters.presets.stored").replace ("{n}", juce::String (presetIndex + 1)), 2000);
    }

    void recallPreset (int presetIndex)
    {
        if (selectedCluster < 1) return;
        auto preset = parameters.getValueTreeState().ensureClusterLFOPreset (presetIndex);
        if (! preset.isValid() || ! presetTiles[presetIndex]->isPopulated()) return;

        auto lfoSection = parameters.getValueTreeState().getClusterLFOSection (selectedCluster);
        if (! lfoSection.isValid()) return;

        isLoadingParameters = true;

        // Apply non-shape properties first (amplitude, rate, phase, period)
        // so the fade dip on shape change uses the new amplitudes
        for (int i = 0; i < numLFOPresetProps; ++i)
        {
            auto& id = getLFOPresetPropId (i);
            // Skip shape IDs in first pass
            if (id == WFSParameterIDs::clusterLFOshapeX || id == WFSParameterIDs::clusterLFOshapeY ||
                id == WFSParameterIDs::clusterLFOshapeZ || id == WFSParameterIDs::clusterLFOshapeRot ||
                id == WFSParameterIDs::clusterLFOshapeScale)
                continue;
            lfoSection.setProperty (id, preset.getProperty (id), nullptr);
        }

        // Now apply shapes (triggers fade dip if shape changed)
        lfoSection.setProperty (WFSParameterIDs::clusterLFOshapeX,     preset.getProperty (WFSParameterIDs::clusterLFOshapeX), nullptr);
        lfoSection.setProperty (WFSParameterIDs::clusterLFOshapeY,     preset.getProperty (WFSParameterIDs::clusterLFOshapeY), nullptr);
        lfoSection.setProperty (WFSParameterIDs::clusterLFOshapeZ,     preset.getProperty (WFSParameterIDs::clusterLFOshapeZ), nullptr);
        lfoSection.setProperty (WFSParameterIDs::clusterLFOshapeRot,   preset.getProperty (WFSParameterIDs::clusterLFOshapeRot), nullptr);
        lfoSection.setProperty (WFSParameterIDs::clusterLFOshapeScale, preset.getProperty (WFSParameterIDs::clusterLFOshapeScale), nullptr);

        isLoadingParameters = false;
        loadClusterLFOParameters();

        if (statusBar != nullptr)
            statusBar->showTemporaryMessage (
                LOC ("clusters.presets.recalled").replace ("{n}", juce::String (presetIndex + 1)), 2000);
    }

    void activateCurrentClusterLFO()
    {
        if (selectedCluster < 1) return;
        auto lfoSection = parameters.getValueTreeState().getClusterLFOSection (selectedCluster);
        if (lfoSection.isValid())
            lfoSection.setProperty (WFSParameterIDs::clusterLFOactive, 1, nullptr);
        lfoActiveButton.setToggleState (true, juce::dontSendNotification);
        lfoActiveButton.setButtonText (LOC ("clusters.toggles.lfoOn"));
        updateLFOAlpha();
    }

    void stopAllClusterLFOs()
    {
        for (int c = 1; c <= 10; ++c)
        {
            auto lfoSection = parameters.getValueTreeState().getClusterLFOSection (c);
            if (lfoSection.isValid())
                lfoSection.setProperty (WFSParameterIDs::clusterLFOactive, 0, nullptr);
        }
        loadClusterLFOParameters();
    }

    void exportLFOPresets()
    {
        presetFileChooser = std::make_shared<juce::FileChooser> (
            LOC ("clusters.presets.exportDialog"),
            AppSettings::getLastFolder ("lastXmlFolder"), "*.xml");
        int chooserFlags = juce::FileBrowserComponent::saveMode
                         | juce::FileBrowserComponent::canSelectFiles;

        presetFileChooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& fc) {
            auto result = fc.getResult();
            if (result != juce::File{})
            {
                auto file = result.hasFileExtension (".xml") ? result : result.withFileExtension (".xml");
                AppSettings::setLastFolder ("lastXmlFolder", file.getParentDirectory());
                if (parameters.getFileManager().exportClusterLFOPresets (file))
                {
                    if (statusBar != nullptr)
                        statusBar->showTemporaryMessage (LOC ("clusters.presets.exported"), 2000);
                }
            }
        });
    }

    void importLFOPresets()
    {
        presetFileChooser = std::make_shared<juce::FileChooser> (
            LOC ("clusters.presets.importDialog"),
            AppSettings::getLastFolder ("lastXmlFolder"), "*.xml");
        int chooserFlags = juce::FileBrowserComponent::openMode
                         | juce::FileBrowserComponent::canSelectFiles;

        presetFileChooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& fc) {
            auto result = fc.getResult();
            if (result.existsAsFile())
            {
                AppSettings::setLastFolder ("lastXmlFolder", result.getParentDirectory());
                if (parameters.getFileManager().importClusterLFOPresets (result))
                {
                    for (auto* tile : presetTiles)
                        tile->updateFromValueTree();
                    if (statusBar != nullptr)
                        statusBar->showTemporaryMessage (LOC ("clusters.presets.imported"), 2000);
                }
            }
        });
    }

    void loadClusterLFOParameters()
    {
        isLoadingParameters = true;

        auto lfoSection = parameters.getValueTreeState().getClusterLFOSection(selectedCluster);
        if (!lfoSection.isValid()) { isLoadingParameters = false; return; }

        using namespace WFSParameterIDs;
        using namespace WFSParameterDefaults;

        // Active button
        bool active = static_cast<int>(lfoSection.getProperty(clusterLFOactive, 0)) != 0;
        lfoActiveButton.setToggleState(active, juce::dontSendNotification);
        lfoActiveButton.setButtonText(active ? LOC("clusters.toggles.lfoOn") : LOC("clusters.toggles.lfoOff"));

        // Period: seconds -> slider 0..1 (log scale)
        float periodSec = static_cast<float>(lfoSection.getProperty(clusterLFOperiod, clusterLFOperiodDefault));
        float periodSlider = std::log10(periodSec / 0.01f) / std::log10(10000.0f);
        lfoPeriodDial.setValue(juce::jlimit(0.0f, 1.0f, periodSlider));
        lfoPeriodValueLabel.setText(juce::String(periodSec, 2) + " s", juce::dontSendNotification);

        // Global phase
        int globalPhase = static_cast<int>(lfoSection.getProperty(clusterLFOphase, 0));
        lfoPhaseDial.setAngle(static_cast<float>(globalPhase));
        lfoPhaseValueLabel.setText(juce::String(globalPhase) + juce::String::charToString(0x00B0), juce::dontSendNotification);

        // Per-axis parameters
        const juce::Identifier shapeIds[]  = { clusterLFOshapeX, clusterLFOshapeY, clusterLFOshapeZ, clusterLFOshapeRot, clusterLFOshapeScale };
        const juce::Identifier rateIds[]   = { clusterLFOrateX, clusterLFOrateY, clusterLFOrateZ, clusterLFOrateRot, clusterLFOrateScale };
        const juce::Identifier ampIds[]    = { clusterLFOamplitudeX, clusterLFOamplitudeY, clusterLFOamplitudeZ, clusterLFOamplitudeRot, clusterLFOamplitudeScale };
        const juce::Identifier phaseIds[]  = { clusterLFOphaseX, clusterLFOphaseY, clusterLFOphaseZ, clusterLFOphaseRot, clusterLFOphaseScale };

        for (int a = 0; a < 5; ++a)
        {
            auto& row = lfoRows[a];

            // Shape
            int shape = static_cast<int>(lfoSection.getProperty(shapeIds[a], 0));
            row.shapeSelector.setSelectedId(shape + 1, juce::dontSendNotification);

            // Rate: real value -> slider (-1..+1, log scale: pow(100, slider) = rate)
            float rate = static_cast<float>(lfoSection.getProperty(rateIds[a], clusterLFOrateDefault));
            float rateSlider = std::log10(rate) / 2.0f; // log10(100)=2
            row.rateSlider.setValue(juce::jlimit(-1.0f, 1.0f, rateSlider));
            row.rateValueLabel.setText(juce::String(rate, 2) + "x", juce::dontSendNotification);

            // Amplitude
            if (a < 3) // XYZ
            {
                float ampMeters = static_cast<float>(lfoSection.getProperty(ampIds[a], clusterLFOamplitudeXYZDefault));
                if (auto* slider = dynamic_cast<WfsStandardSlider*>(row.amplitudeSlider.get()))
                    slider->setValue(ampMeters / 50.0f);
                row.amplitudeValueLabel.setText(juce::String(ampMeters, 1) + " m", juce::dontSendNotification);
            }
            else if (a == 3) // Rotation
            {
                int ampDeg = static_cast<int>(lfoSection.getProperty(ampIds[a], clusterLFOamplitudeRotDefault));
                if (auto* slider = dynamic_cast<WfsBidirectionalSlider*>(row.amplitudeSlider.get()))
                    slider->setValue(static_cast<float>(ampDeg) / 360.0f);
                row.amplitudeValueLabel.setText(juce::String(ampDeg) + juce::String::charToString(0x00B0), juce::dontSendNotification);
            }
            else // Scale
            {
                float ampScale = static_cast<float>(lfoSection.getProperty(ampIds[a], clusterLFOamplitudeScaleDefault));
                if (auto* slider = dynamic_cast<WfsBidirectionalSlider*>(row.amplitudeSlider.get()))
                    slider->setValue(std::log10(ampScale)); // 0.1->-1, 1->0, 10->+1
                row.amplitudeValueLabel.setText(juce::String(ampScale, 2) + "x", juce::dontSendNotification);
            }

            // Phase
            int phaseDeg = static_cast<int>(lfoSection.getProperty(phaseIds[a], 0));
            row.phaseDial.setAngle(static_cast<float>(phaseDeg));
            row.phaseValueLabel.setText(juce::String(phaseDeg) + juce::String::charToString(0x00B0), juce::dontSendNotification);
        }

        updateLFOAlpha();
        isLoadingParameters = false;
    }

    void updateLFOAlpha()
    {
        bool active = lfoActiveButton.getToggleState();
        float mainAlpha = active ? 1.0f : 0.5f;

        // Top LFO controls
        lfoSectionLabel.setAlpha(mainAlpha);
        lfoPeriodLabel.setAlpha(mainAlpha);
        lfoPeriodDial.setAlpha(mainAlpha);
        lfoPeriodValueLabel.setAlpha(mainAlpha);
        lfoPhaseLabel.setAlpha(mainAlpha);
        lfoPhaseDial.setAlpha(mainAlpha);
        lfoPhaseValueLabel.setAlpha(mainAlpha);
        lfoProgressDial.setAlpha(mainAlpha);

        // Per-axis controls
        for (int a = 0; a < 5; ++a)
        {
            bool axisActive = active && (lfoRows[a].shapeSelector.getSelectedId() > 1);
            float axisAlpha = axisActive ? 1.0f : 0.5f;

            lfoRows[a].axisLabel.setAlpha(mainAlpha);
            lfoRows[a].shapeSelector.setAlpha(mainAlpha);
            lfoRows[a].amplitudeLabel.setAlpha(axisAlpha);
            lfoRows[a].amplitudeSlider->setAlpha(axisAlpha);
            lfoRows[a].amplitudeValueLabel.setAlpha(axisAlpha);
            lfoRows[a].rateLabel.setAlpha(axisAlpha);
            lfoRows[a].rateSlider.setAlpha(axisAlpha);
            lfoRows[a].rateValueLabel.setAlpha(axisAlpha);
            lfoRows[a].phaseLabel.setAlpha(axisAlpha);
            lfoRows[a].phaseDial.setAlpha(axisAlpha);
            lfoRows[a].phaseValueLabel.setAlpha(axisAlpha);
            lfoRows[a].outputSlider.setAlpha(axisAlpha);
        }
    }

    void updateLFOIndicators()
    {
        if (selectedCluster < 1) return;

        float progress = clusterLFOProcessor.getRampProgress(selectedCluster);
        lfoProgressDial.setProgress(progress);

        for (int a = 0; a < 5; ++a)
        {
            float normalized = 0.0f;
            switch (a)
            {
                case 0: normalized = clusterLFOProcessor.getNormalizedX(selectedCluster); break;
                case 1: normalized = clusterLFOProcessor.getNormalizedY(selectedCluster); break;
                case 2: normalized = clusterLFOProcessor.getNormalizedZ(selectedCluster); break;
                case 3: normalized = clusterLFOProcessor.getNormalizedRot(selectedCluster); break;
                case 4: normalized = clusterLFOProcessor.getNormalizedScale(selectedCluster); break;
            }
            lfoRows[a].outputSlider.setValue(normalized);
        }
    }

    //==========================================================================
    // Editable Value Labels
    //==========================================================================

    void setupEditableValueLabel(juce::Label& label)
    {
        label.setEditable(true, false);
        label.addListener(this);
    }

    void editorShown (juce::Label* label, juce::TextEditor& editor) override
    {
        for (auto& col : lfoCircuits)
            if (std::find (col.begin(), col.end(), static_cast<juce::Component*>(label)) != col.end())
            {
                editor.addKeyListener (&circuitTabHandler);
                break;
            }
    }

    void labelTextChanged(juce::Label* label) override
    {
        if (isLoadingParameters) return;

        // Extract numeric value from text (strips units)
        float val = label->getText().retainCharacters("0123456789.-").getFloatValue();

        // Period value label
        if (label == &lfoPeriodValueLabel)
        {
            float period = juce::jlimit(0.01f, 100.0f, val);
            // Inverse of: period = 0.01 * pow(10000, v) → v = log10(period/0.01) / log10(10000)
            float v = std::log10(period / 0.01f) / 4.0f;
            lfoPeriodDial.setValue(juce::jlimit(0.0f, 1.0f, v));
            return;
        }

        // Phase value label (global)
        if (label == &lfoPhaseValueLabel)
        {
            int deg = juce::jlimit(0, 360, static_cast<int>(std::round(val)));
            lfoPhaseDial.setAngle(static_cast<float>(deg));
            return;
        }

        // Per-axis value labels
        for (int a = 0; a < 5; ++a)
        {
            auto& row = lfoRows[a];

            if (label == &row.amplitudeValueLabel)
            {
                if (a == 3) // Rotation: deg → bidirectional slider -1..1
                {
                    float deg = juce::jlimit(-360.0f, 360.0f, val);
                    auto* slider = dynamic_cast<WfsBidirectionalSlider*>(row.amplitudeSlider.get());
                    if (slider) slider->setValue(deg / 360.0f);
                }
                else if (a == 4) // Scale: ratio → bidirectional slider via log10
                {
                    float ratio = juce::jlimit(0.1f, 10.0f, val);
                    auto* slider = dynamic_cast<WfsBidirectionalSlider*>(row.amplitudeSlider.get());
                    if (slider) slider->setValue(std::log10(ratio));
                }
                else // XYZ: metres → standard slider 0..1
                {
                    float meters = juce::jlimit(0.0f, 50.0f, val);
                    auto* slider = dynamic_cast<WfsStandardSlider*>(row.amplitudeSlider.get());
                    if (slider) slider->setValue(meters / 50.0f);
                }
                return;
            }

            if (label == &row.rateValueLabel)
            {
                float rate = juce::jlimit(0.01f, 100.0f, val);
                // Inverse of: rate = pow(100, v) → v = log10(rate) / log10(100) = log10(rate) / 2
                row.rateSlider.setValue(std::log10(rate) / 2.0f);
                return;
            }

            if (label == &row.phaseValueLabel)
            {
                int deg = juce::jlimit(0, 360, static_cast<int>(std::round(val)));
                row.phaseDial.setAngle(static_cast<float>(deg));
                return;
            }
        }
    }

    //==========================================================================
    // LFO Offset Computation (for all clusters)
    // Computes per-input offsets without writing to ValueTree.
    // MainComponent reads these offsets and feeds them to the calculation engine.
    //==========================================================================

    void processAllClusterLFOs()
    {
        clusterLFOProcessor.process(0.02f);

        int numInputs = parameters.getNumInputChannels();

        // Resize and zero all offsets
        if (static_cast<int> (clusterLFOInputOffsets.size()) != numInputs)
            clusterLFOInputOffsets.resize (static_cast<size_t> (numInputs), { 0.0f, 0.0f, 0.0f });
        for (auto& o : clusterLFOInputOffsets)
            o = { 0.0f, 0.0f, 0.0f };

        for (int c = 1; c <= 10; ++c)
        {
            if (!clusterLFOProcessor.isActive(c))
                continue;

            // Read absolute offsets (not incremental deltas)
            float oX = clusterLFOProcessor.getOffsetX(c);
            float oY = clusterLFOProcessor.getOffsetY(c);
            float oZ = clusterLFOProcessor.getOffsetZ(c);
            float oRot = clusterLFOProcessor.getOffsetRotDeg(c);
            float oScale = clusterLFOProcessor.getOffsetScale(c);

            bool hasTranslation = (oX != 0.0f || oY != 0.0f || oZ != 0.0f);
            bool hasRotation = (oRot != 0.0f);
            bool hasScale = (oScale != 1.0f);

            if (!hasTranslation && !hasRotation && !hasScale)
                continue;

            // Gather inputs for this cluster
            std::vector<int> clusterInputs;
            for (int i = 0; i < numInputs; ++i)
            {
                int cl = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
                if (cl == c)
                    clusterInputs.push_back(i);
            }

            if (clusterInputs.empty())
                continue;

            // Calculate reference point from BASE positions (stable, not LFO-modified)
            float refX = 0.0f, refY = 0.0f, refZ = 0.0f;
            if (hasRotation || hasScale)
            {
                bool foundRef = false;
                for (int idx : clusterInputs)
                {
                    if (isInputFullyTracked(idx))
                    {
                        auto [px, py, pz] = getInputPosition(idx);
                        refX = px; refY = py; refZ = pz;
                        foundRef = true;
                        break;
                    }
                }
                if (!foundRef)
                {
                    int mode = static_cast<int>(parameters.getValueTreeState().getClusterParameter(
                        c, WFSParameterIDs::clusterReferenceMode));
                    if (mode == 0) // First input
                    {
                        auto [px, py, pz] = getInputPosition(clusterInputs[0]);
                        refX = px; refY = py; refZ = pz;
                    }
                    else // Barycenter
                    {
                        float sx = 0, sy = 0, sz = 0;
                        for (int idx : clusterInputs)
                        {
                            auto [px, py, pz] = getInputPosition(idx);
                            sx += px; sy += py; sz += pz;
                        }
                        float n = static_cast<float>(clusterInputs.size());
                        refX = sx / n; refY = sy / n; refZ = sz / n;
                    }
                }
            }

            // Compute per-input offsets (no ValueTree writes)
            for (int inputIdx : clusterInputs)
            {
                auto [baseX, baseY, baseZ] = getInputPosition(inputIdx);
                float px = baseX, py = baseY, pz = baseZ;

                // Rotation around reference (XY plane, absolute angle)
                if (hasRotation)
                {
                    float rad = juce::degreesToRadians(oRot);
                    float cosA = std::cos(rad);
                    float sinA = std::sin(rad);
                    float relX = px - refX;
                    float relY = py - refY;
                    px = refX + relX * cosA - relY * sinA;
                    py = refY + relX * sinA + relY * cosA;
                }

                // Scale around reference (absolute factor)
                if (hasScale)
                {
                    px = refX + (px - refX) * oScale;
                    py = refY + (py - refY) * oScale;
                    pz = refZ + (pz - refZ) * oScale;
                }

                // Translation (absolute offset)
                px += oX;
                py += oY;
                pz += oZ;

                clusterLFOInputOffsets[static_cast<size_t> (inputIdx)] = { px - baseX, py - baseY, pz - baseZ };
            }
        }
    }

    //==========================================================================
    // StatusBar Help Text
    //==========================================================================

    void setupHelpTextMappings()
    {
        helpTextMap.clear();
        oscMethodMap.clear();

        // Center column controls
        helpTextMap[&referenceModeSelector] = LOC("clusters.help.referenceMode");
        helpTextMap[&positionJoystick]      = LOC("clusters.help.positionJoystick");
        helpTextMap[&zSlider]               = LOC("clusters.help.zSlider");
        helpTextMap[&attenuationSlider]     = LOC("clusters.help.attenuationSlider");
        helpTextMap[&rotationDial]          = LOC("clusters.help.rotationDial");
        helpTextMap[&scaleSlider]            = LOC("clusters.help.scaleJoystick");
        helpTextMap[&planeSelector]         = LOC("clusters.help.planeSelector");

        // LFO controls
        helpTextMap[&lfoActiveButton]    = LOC("clusters.help.lfoActiveButton");
        helpTextMap[&lfoPeriodDial]      = LOC("clusters.help.lfoPeriodDial");
        helpTextMap[&lfoPhaseDial]       = LOC("clusters.help.lfoPhaseDial");

        oscMethodMap[&lfoActiveButton]   = LOC("clusters.osc.lfoActive");
        oscMethodMap[&lfoPeriodDial]     = LOC("clusters.osc.lfoPeriod");
        oscMethodMap[&lfoPhaseDial]      = LOC("clusters.osc.lfoPhase");

        const char* helpShapeKeys[]  = { "clusters.help.lfoShapeXSelector", "clusters.help.lfoShapeYSelector", "clusters.help.lfoShapeZSelector", "clusters.help.lfoShapeRotSelector", "clusters.help.lfoShapeScaleSelector" };
        const char* helpRateKeys[]   = { "clusters.help.lfoRateXSlider", "clusters.help.lfoRateYSlider", "clusters.help.lfoRateZSlider", "clusters.help.lfoRateRotSlider", "clusters.help.lfoRateScaleSlider" };
        const char* helpAmpKeys[]    = { "clusters.help.lfoAmplitudeXSlider", "clusters.help.lfoAmplitudeYSlider", "clusters.help.lfoAmplitudeZSlider", "clusters.help.lfoAmplitudeRotSlider", "clusters.help.lfoAmplitudeScaleSlider" };
        const char* helpPhaseKeys[]  = { "clusters.help.lfoPhaseXDial", "clusters.help.lfoPhaseYDial", "clusters.help.lfoPhaseZDial", "clusters.help.lfoPhaseRotDial", "clusters.help.lfoPhaseScaleDial" };

        const char* oscShapeKeys[]   = { "clusters.osc.lfoShapeX", "clusters.osc.lfoShapeY", "clusters.osc.lfoShapeZ", "clusters.osc.lfoShapeRot", "clusters.osc.lfoShapeScale" };
        const char* oscRateKeys[]    = { "clusters.osc.lfoRateX", "clusters.osc.lfoRateY", "clusters.osc.lfoRateZ", "clusters.osc.lfoRateRot", "clusters.osc.lfoRateScale" };
        const char* oscAmpKeys[]     = { "clusters.osc.lfoAmplitudeX", "clusters.osc.lfoAmplitudeY", "clusters.osc.lfoAmplitudeZ", "clusters.osc.lfoAmplitudeRot", "clusters.osc.lfoAmplitudeScale" };
        const char* oscPhaseKeys[]   = { "clusters.osc.lfoPhaseX", "clusters.osc.lfoPhaseY", "clusters.osc.lfoPhaseZ", "clusters.osc.lfoPhaseRot", "clusters.osc.lfoPhaseScale" };

        for (int a = 0; a < 5; ++a)
        {
            helpTextMap[&lfoRows[a].shapeSelector]      = LOC(helpShapeKeys[a]);
            helpTextMap[&lfoRows[a].rateSlider]          = LOC(helpRateKeys[a]);
            helpTextMap[lfoRows[a].amplitudeSlider.get()] = LOC(helpAmpKeys[a]);
            helpTextMap[&lfoRows[a].phaseDial]           = LOC(helpPhaseKeys[a]);

            oscMethodMap[&lfoRows[a].shapeSelector]      = LOC(oscShapeKeys[a]);
            oscMethodMap[&lfoRows[a].rateSlider]          = LOC(oscRateKeys[a]);
            oscMethodMap[lfoRows[a].amplitudeSlider.get()] = LOC(oscAmpKeys[a]);
            oscMethodMap[&lfoRows[a].phaseDial]           = LOC(oscPhaseKeys[a]);
        }

        // Preset tile controls
        for (int i = 0; i < presetTiles.size(); ++i)
            helpTextMap[presetTiles[i]] = LOC ("clusters.help.presetTile");
        helpTextMap[&stopAllLFOButton]    = LOC ("clusters.help.stopAllLFO");
        helpTextMap[&exportPresetsButton] = LOC ("clusters.help.exportPresets");
        helpTextMap[&importPresetsButton] = LOC ("clusters.help.importPresets");

        // Register mouse listeners
        for (auto& pair : helpTextMap)
        {
            bool wantsEventsFromChildren = (dynamic_cast<juce::ComboBox*>(pair.first) != nullptr);
            pair.first->addMouseListener(this, wantsEventsFromChildren);
        }
    }

    void mouseEnter(const juce::MouseEvent& event) override
    {
        if (statusBar == nullptr) return;

        juce::Component* component = event.eventComponent;
        while (component != nullptr)
        {
            if (helpTextMap.find(component) != helpTextMap.end())
            {
                statusBar->setHelpText(helpTextMap[component]);
                if (oscMethodMap.find(component) != oscMethodMap.end())
                    statusBar->setOscMethod(oscMethodMap[component]);
                return;
            }
            component = component->getParentComponent();
        }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (statusBar != nullptr)
            statusBar->clearText();
    }

    //==========================================================================
    // Cluster Selection and State
    //==========================================================================

    void selectCluster(int clusterIndex)
    {
        selectedCluster = clusterIndex;

        // Update toggle state
        for (int i = 0; i < 10; ++i)
            clusterButtons[i]->setToggleState(i + 1 == clusterIndex, juce::dontSendNotification);

        // Load reference mode
        int mode = static_cast<int>(parameters.getValueTreeState().getClusterParameter(
            clusterIndex, WFSParameterIDs::clusterReferenceMode));
        referenceModeSelector.setSelectedId(mode + 1, juce::dontSendNotification);

        // Reset rotation dial
        rotationDial.setAngle(0.0f);
        previousDialAngle = 0.0f;

        // Update assigned inputs list
        updateAssignedInputsList();
        updateReferencePositionDisplay();
        updateStatusLabel();

        // Load LFO parameters for this cluster
        loadClusterLFOParameters();

        // Notify Stream Deck of cluster change
        if (onClusterSelected)
            onClusterSelected (selectedCluster);
    }

    void updateClusterButtonStates()
    {
        int numInputs = parameters.getNumInputChannels();

        for (int c = 1; c <= 10; ++c)
        {
            int inputCount = 0;
            for (int i = 0; i < numInputs; ++i)
            {
                int cluster = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
                if (cluster == c)
                    inputCount++;
            }

            auto* btn = clusterButtons[c - 1];
            if (inputCount > 0)
            {
                btn->setButtonText(juce::String(c) + " (" + juce::String(inputCount) + ")");
                btn->setColour(juce::TextButton::textColourOffId, juce::Colours::black);
            }
            else
            {
                btn->setButtonText(juce::String(c));
                btn->setColour(juce::TextButton::textColourOffId, juce::Colours::black.withAlpha(0.5f));
            }
        }
    }

    void updateAssignedInputsList()
    {
        // Collect current cluster members and detect tracked input
        std::vector<int> currentMembers;
        int trackedInputIdx = -1;
        int numInputs = parameters.getNumInputChannels();

        for (int i = 0; i < numInputs; ++i)
        {
            int cluster = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
            if (cluster == selectedCluster)
            {
                currentMembers.push_back(i);
                if (isInputFullyTracked(i))
                    trackedInputIdx = i;
            }
        }

        // Read stored order
        juce::String storedOrder = parameters.getValueTreeState().getClusterParameter(
            selectedCluster, WFSParameterIDs::clusterInputOrder).toString();

        assignedInputs.clear();

        if (storedOrder.isNotEmpty())
        {
            // Build ordered list from stored order, keeping only current members
            juce::StringArray tokens;
            tokens.addTokens(storedOrder, ",", "");
            for (auto& tok : tokens)
            {
                int ch = tok.getIntValue();
                if (std::find(currentMembers.begin(), currentMembers.end(), ch) != currentMembers.end())
                    assignedInputs.push_back(ch);
            }

            // Append any new members not in stored order
            for (int ch : currentMembers)
            {
                if (std::find(assignedInputs.begin(), assignedInputs.end(), ch) == assignedInputs.end())
                    assignedInputs.push_back(ch);
            }
        }
        else
        {
            // No stored order — use numeric order
            assignedInputs = currentMembers;
        }

        // Force tracked input to position 0
        if (trackedInputIdx >= 0)
        {
            auto it = std::find(assignedInputs.begin(), assignedInputs.end(), trackedInputIdx);
            if (it != assignedInputs.end() && it != assignedInputs.begin())
            {
                assignedInputs.erase(it);
                assignedInputs.insert(assignedInputs.begin(), trackedInputIdx);
            }
        }

        inputsList.updateContent();
        inputsList.repaint();
    }

    //==========================================================================
    // Input Order Persistence & Drag Reorder
    //==========================================================================

    void saveInputOrder()
    {
        juce::String order;
        for (int i = 0; i < static_cast<int>(assignedInputs.size()); ++i)
        {
            if (i > 0) order += ",";
            order += juce::String(assignedInputs[static_cast<size_t>(i)]);
        }
        parameters.getValueTreeState().setClusterParameter(
            selectedCluster, WFSParameterIDs::clusterInputOrder, order);
    }

    void moveInputRow (int fromRow, int toRow)
    {
        int numRows = static_cast<int>(assignedInputs.size());
        if (fromRow < 0 || fromRow >= numRows || toRow < 0 || toRow >= numRows || fromRow == toRow)
            return;

        // Prevent moving above a locked tracked input at slot 0
        bool slot0Locked = !assignedInputs.empty() && isInputFullyTracked(assignedInputs[0]);
        if (slot0Locked)
        {
            if (fromRow == 0) return;        // Can't move the tracked input
            if (toRow == 0) toRow = 1;        // Can't place above it
        }
        if (fromRow == toRow) return;

        // Move the element
        int inputIdx = assignedInputs[static_cast<size_t>(fromRow)];
        assignedInputs.erase(assignedInputs.begin() + fromRow);
        assignedInputs.insert(assignedInputs.begin() + toRow, inputIdx);

        // Update dragging tracking
        draggingInputIdx = inputIdx;

        inputsList.updateContent();
        inputsList.repaint();
        updateReferencePositionDisplay();
    }

    void startDrag (int inputIdx)
    {
        draggingInputIdx = inputIdx;
    }

    void handleDragMove (float listLocalY)
    {
        if (draggingInputIdx < 0) return;

        int rowH = inputsList.getRowHeight();
        if (rowH <= 0) return;

        // Account for scroll position
        int targetRow = static_cast<int>(listLocalY + inputsList.getVerticalPosition()) / rowH;
        targetRow = juce::jlimit(0, static_cast<int>(assignedInputs.size()) - 1, targetRow);

        // Find current row of the dragged input
        int currentRow = -1;
        for (int i = 0; i < static_cast<int>(assignedInputs.size()); ++i)
        {
            if (assignedInputs[static_cast<size_t>(i)] == draggingInputIdx)
            {
                currentRow = i;
                break;
            }
        }

        if (currentRow >= 0 && currentRow != targetRow)
            moveInputRow(currentRow, targetRow);
    }

    void endDrag()
    {
        if (draggingInputIdx >= 0)
        {
            draggingInputIdx = -1;
            saveInputOrder();
        }
    }

    void updateReferencePositionDisplay()
    {
        auto [x, y, z] = calculateReferencePoint();
        refPosXLabel.setText(LOC("clusters.labels.x") + " " + juce::String(x, 2), juce::dontSendNotification);
        refPosYLabel.setText(LOC("clusters.labels.y") + " " + juce::String(y, 2), juce::dontSendNotification);
        refPosZLabel.setText(LOC("clusters.labels.z") + " " + juce::String(z, 2), juce::dontSendNotification);
    }

    void updateStatusLabel()
    {
        for (int inputIdx : assignedInputs)
        {
            if (isInputFullyTracked(inputIdx))
            {
                statusLabel.setText(LOC("clusters.status.tracking").replace("{num}", juce::String(inputIdx + 1)),
                                    juce::dontSendNotification);
                return;
            }
        }

        if (assignedInputs.empty())
            statusLabel.setText(LOC("clusters.status.noInputs"), juce::dontSendNotification);
        else
            statusLabel.setText("", juce::dontSendNotification);
    }

    //==========================================================================
    // Tracking Check
    //==========================================================================

    bool isInputFullyTracked(int inputIdx) const
    {
        int globalTracking = static_cast<int>(parameters.getConfigParam("trackingEnabled"));
        int protocolEnabled = static_cast<int>(parameters.getConfigParam("trackingProtocol"));
        int localTracking = static_cast<int>(parameters.getInputParam(inputIdx, "inputTrackingActive"));

        return (globalTracking != 0) && (protocolEnabled != 0) && (localTracking != 0);
    }

    //==========================================================================
    // Reference Point Calculation
    //==========================================================================

    std::tuple<float, float, float> calculateReferencePoint()
    {
        if (assignedInputs.empty())
            return { 0.0f, 0.0f, 0.0f };

        for (int inputIdx : assignedInputs)
        {
            if (isInputFullyTracked(inputIdx))
                return getInputPosition(inputIdx);
        }

        int mode = referenceModeSelector.getSelectedId() - 1;

        if (mode == 0)
        {
            return getInputPosition(assignedInputs[0]);
        }
        else
        {
            return calculateBarycenter();
        }
    }

    std::tuple<float, float, float> calculateBarycenter()
    {
        if (assignedInputs.empty())
            return { 0.0f, 0.0f, 0.0f };

        float sumX = 0, sumY = 0, sumZ = 0;
        for (int inputIdx : assignedInputs)
        {
            auto [x, y, z] = getInputPosition(inputIdx);
            sumX += x;
            sumY += y;
            sumZ += z;
        }

        float n = static_cast<float>(assignedInputs.size());
        return { sumX / n, sumY / n, sumZ / n };
    }

    std::tuple<float, float, float> getInputPosition(int inputIdx) const
    {
        float x = static_cast<float>(parameters.getInputParam(inputIdx, "inputPositionX"));
        float y = static_cast<float>(parameters.getInputParam(inputIdx, "inputPositionY"));
        float z = static_cast<float>(parameters.getInputParam(inputIdx, "inputPositionZ"));
        return { x, y, z };
    }

    void setInputPosition(int inputIdx, float x, float y, float z)
    {
        parameters.setInputParam(inputIdx, "inputPositionX", x);
        parameters.setInputParam(inputIdx, "inputPositionY", y);
        parameters.setInputParam(inputIdx, "inputPositionZ", z);
    }

    std::tuple<float, float, float> getInputOffset(int inputIdx) const
    {
        float x = static_cast<float>(parameters.getInputParam(inputIdx, "inputOffsetX"));
        float y = static_cast<float>(parameters.getInputParam(inputIdx, "inputOffsetY"));
        float z = static_cast<float>(parameters.getInputParam(inputIdx, "inputOffsetZ"));
        return { x, y, z };
    }

    void setInputOffset(int inputIdx, float x, float y, float z)
    {
        parameters.setInputParam(inputIdx, "inputOffsetX", x);
        parameters.setInputParam(inputIdx, "inputOffsetY", y);
        parameters.setInputParam(inputIdx, "inputOffsetZ", z);
    }

    //==========================================================================
    // Distance Constraint Helpers (for Cylindrical/Spherical modes)
    //==========================================================================

    float calculateDistanceFromOrigin(float x, float y, float z, int coordMode) const
    {
        if (coordMode == 1)
            return std::sqrt(x * x + y * y);
        else if (coordMode == 2)
            return std::sqrt(x * x + y * y + z * z);
        return 0.0f;
    }

    void applyDistanceConstraint(float& x, float& y, float& z, int coordMode, float minDist, float maxDist)
    {
        float currentDist = calculateDistanceFromOrigin(x, y, z, coordMode);

        if (currentDist < 0.0001f)
            currentDist = 0.0001f;

        float targetDist = juce::jlimit(minDist, maxDist, currentDist);

        if (!juce::approximatelyEqual(currentDist, targetDist))
        {
            float scale = targetDist / currentDist;
            if (coordMode == 1)
            {
                x *= scale;
                y *= scale;
            }
            else if (coordMode == 2)
            {
                x *= scale;
                y *= scale;
                z *= scale;
            }
        }
    }

    //==========================================================================
    // Stage Bounds Helpers
    //==========================================================================

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

    //==========================================================================
    // Transformation Algorithms
    //==========================================================================

    void applyPositionDelta(float dx, float dy, float dz)
    {
        if (assignedInputs.empty())
            return;

        int trackedIdx = -1;
        for (int inputIdx : assignedInputs)
        {
            if (isInputFullyTracked(inputIdx))
            {
                trackedIdx = inputIdx;
                break;
            }
        }

        if (trackedIdx >= 0)
        {
            auto [ox, oy, oz] = getInputOffset(trackedIdx);
            auto [px, py, pz] = getInputPosition(trackedIdx);
            float newOx = ox + dx;
            float newOy = oy + dy;
            float newOz = oz + dz;

            bool constrainX = static_cast<int>(parameters.getInputParam(trackedIdx, "inputConstraintX")) != 0;
            bool constrainY = static_cast<int>(parameters.getInputParam(trackedIdx, "inputConstraintY")) != 0;
            bool constrainZ = static_cast<int>(parameters.getInputParam(trackedIdx, "inputConstraintZ")) != 0;

            if (constrainX)
            {
                float totalX = juce::jlimit(getStageMinX(), getStageMaxX(), px + newOx);
                newOx = totalX - px;
            }
            if (constrainY)
            {
                float totalY = juce::jlimit(getStageMinY(), getStageMaxY(), py + newOy);
                newOy = totalY - py;
            }
            if (constrainZ)
            {
                float totalZ = juce::jlimit(getStageMinZ(), getStageMaxZ(), pz + newOz);
                newOz = totalZ - pz;
            }

            int coordMode = static_cast<int>(parameters.getInputParam(trackedIdx, "inputCoordinateMode"));
            if (coordMode == 1 || coordMode == 2)
            {
                int constraintDist = static_cast<int>(parameters.getInputParam(trackedIdx, "inputConstraintDistance"));
                if (constraintDist != 0)
                {
                    float minDist = static_cast<float>(parameters.getInputParam(trackedIdx, "inputConstraintDistanceMin"));
                    float maxDist = static_cast<float>(parameters.getInputParam(trackedIdx, "inputConstraintDistanceMax"));
                    float totalX = px + newOx;
                    float totalY = py + newOy;
                    float totalZ = pz + newOz;
                    applyDistanceConstraint(totalX, totalY, totalZ, coordMode, minDist, maxDist);
                    newOx = totalX - px;
                    newOy = totalY - py;
                    newOz = totalZ - pz;
                }
            }

            setInputOffset(trackedIdx, newOx, newOy, newOz);
        }
        else
        {
            for (int inputIdx : assignedInputs)
            {
                auto [px, py, pz] = getInputPosition(inputIdx);
                float newX = px + dx;
                float newY = py + dy;
                float newZ = pz + dz;

                bool constrainX = static_cast<int>(parameters.getInputParam(inputIdx, "inputConstraintX")) != 0;
                bool constrainY = static_cast<int>(parameters.getInputParam(inputIdx, "inputConstraintY")) != 0;
                bool constrainZ = static_cast<int>(parameters.getInputParam(inputIdx, "inputConstraintZ")) != 0;

                if (constrainX)
                    newX = juce::jlimit(getStageMinX(), getStageMaxX(), newX);
                if (constrainY)
                    newY = juce::jlimit(getStageMinY(), getStageMaxY(), newY);
                if (constrainZ)
                    newZ = juce::jlimit(getStageMinZ(), getStageMaxZ(), newZ);

                int coordMode = static_cast<int>(parameters.getInputParam(inputIdx, "inputCoordinateMode"));
                if (coordMode == 1 || coordMode == 2)
                {
                    int constraintDist = static_cast<int>(parameters.getInputParam(inputIdx, "inputConstraintDistance"));
                    if (constraintDist != 0)
                    {
                        float minDist = static_cast<float>(parameters.getInputParam(inputIdx, "inputConstraintDistanceMin"));
                        float maxDist = static_cast<float>(parameters.getInputParam(inputIdx, "inputConstraintDistanceMax"));
                        applyDistanceConstraint(newX, newY, newZ, coordMode, minDist, maxDist);
                    }
                }

                setInputPosition(inputIdx, newX, newY, newZ);
            }
        }
    }

    void applyAttenuationDelta(float deltaDB)
    {
        for (int inputIdx : assignedInputs)
        {
            float current = static_cast<float>(parameters.getInputParam(inputIdx, "inputAttenuation"));
            float newAtten = juce::jlimit(-92.0f, 0.0f, current + deltaDB);
            parameters.setInputParam(inputIdx, "inputAttenuation", newAtten);
        }
    }

    void applyRotationDelta(float angleDeg)
    {
        if (assignedInputs.empty())
            return;

        auto [refX, refY, refZ] = calculateReferencePoint();

        float rad = juce::degreesToRadians(angleDeg);
        float cosA = std::cos(rad);
        float sinA = std::sin(rad);

        for (int inputIdx : assignedInputs)
        {
            auto [px, py, pz] = getInputPosition(inputIdx);
            float newX = px, newY = py, newZ = pz;

            switch (currentPlane)
            {
                case Plane::XY:
                    newX = refX + (px - refX) * cosA - (py - refY) * sinA;
                    newY = refY + (px - refX) * sinA + (py - refY) * cosA;
                    break;
                case Plane::XZ:
                    newX = refX + (px - refX) * cosA - (pz - refZ) * sinA;
                    newZ = refZ + (px - refX) * sinA + (pz - refZ) * cosA;
                    break;
                case Plane::YZ:
                    newY = refY + (py - refY) * cosA - (pz - refZ) * sinA;
                    newZ = refZ + (py - refY) * sinA + (pz - refZ) * cosA;
                    break;
            }

            setInputPosition(inputIdx, newX, newY, newZ);
        }
    }

    void applyScaleDelta(float scaleX, float scaleY)
    {
        if (assignedInputs.empty())
            return;

        auto [refX, refY, refZ] = calculateReferencePoint();

        for (int inputIdx : assignedInputs)
        {
            auto [px, py, pz] = getInputPosition(inputIdx);
            float newX = px, newY = py, newZ = pz;

            switch (currentPlane)
            {
                case Plane::XY:
                    newX = refX + (px - refX) * scaleX;
                    newY = refY + (py - refY) * scaleY;
                    break;
                case Plane::XZ:
                    newX = refX + (px - refX) * scaleX;
                    newZ = refZ + (pz - refZ) * scaleY;
                    break;
                case Plane::YZ:
                    newY = refY + (py - refY) * scaleX;
                    newZ = refZ + (pz - refZ) * scaleY;
                    break;
            }

            setInputPosition(inputIdx, newX, newY, newZ);
        }
    }

    //==========================================================================
    // Timer Callback (50Hz)
    //==========================================================================

    void timerCallback() override
    {
        // Process cluster LFOs for ALL clusters (even when no inputs selected)
        processAllClusterLFOs();

        // Update LFO UI indicators for the currently selected cluster
        updateLFOIndicators();

        if (selectedCluster < 1 || assignedInputs.empty())
            return;

        // Position joystick (auto-centers, gives -1..1 values)
        auto [jx, jy] = positionJoystick.getCurrentPosition();
        if (jx != 0.0f || jy != 0.0f)
            applyPositionDelta(jx * 0.05f, jy * 0.05f, 0.0f);

        // Z slider (auto-centers)
        float zVal = zSlider.getValue();
        if (zVal != 0.0f)
            applyPositionDelta(0.0f, 0.0f, zVal * 0.05f);

        // Attenuation slider (auto-centers)
        float attenVal = attenuationSlider.getValue();
        if (attenVal != 0.0f)
            applyAttenuationDelta(attenVal * 0.5f);

        // Rotation dial (1:1, calculate delta from previous)
        float currentAngle = rotationDial.getAngle();
        float angleDelta = currentAngle - previousDialAngle;
        if (angleDelta > 180.0f) angleDelta -= 360.0f;
        if (angleDelta < -180.0f) angleDelta += 360.0f;
        if (angleDelta != 0.0f)
            applyRotationDelta(angleDelta);
        previousDialAngle = currentAngle;

        // Scale slider (auto-centers, gives -1..1 → uniform scale)
        float sv = scaleSlider.getValue();
        if (sv != 0.0f)
        {
            float sc = 1.0f + sv * 0.02f;
            applyScaleDelta(sc, sc);
        }

        // Update reference position display periodically
        updateReferencePositionDisplay();
    }

    //==========================================================================
    // ValueTree::Listener
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                  const juce::Identifier& property) override
    {
        juce::ignoreUnused(treeWhosePropertyHasChanged);

        if (property == WFSParameterIDs::inputCluster)
        {
            juce::MessageManager::callAsync([this]() {
                updateClusterButtonStates();
                updateAssignedInputsList();
                updateStatusLabel();
            });
        }
        else if (property == WFSParameterIDs::inputTrackingActive ||
                 property == WFSParameterIDs::trackingEnabled ||
                 property == WFSParameterIDs::trackingProtocol)
        {
            juce::MessageManager::callAsync([this]() {
                updateAssignedInputsList();
                updateStatusLabel();
            });
        }
        else if (property == WFSParameterIDs::inputName)
        {
            juce::MessageManager::callAsync([this]() {
                inputsList.updateContent();
                inputsList.repaint();
            });
        }
    }

    void valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&) override
    {
        juce::MessageManager::callAsync([this]() {
            updateClusterButtonStates();
            updateAssignedInputsList();
        });
    }

    void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) override
    {
        juce::MessageManager::callAsync([this]() {
            updateClusterButtonStates();
            updateAssignedInputsList();
        });
    }

    void valueTreeChildOrderChanged(juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged(juce::ValueTree&) override {}

    // Tab navigation circuits for LFO controls
    std::vector<std::vector<juce::Component*>> lfoCircuits;

    struct CircuitTabHandler : public juce::KeyListener
    {
        std::vector<std::vector<juce::Component*>>* circuits = nullptr;

        bool keyPressed (const juce::KeyPress& key, juce::Component* originatingComponent) override
        {
            if (circuits == nullptr || ! key.isKeyCode (juce::KeyPress::tabKey))
                return false;

            auto* label = dynamic_cast<juce::Label*> (originatingComponent->getParentComponent());
            if (label == nullptr)
                return false;

            std::vector<juce::Component*>* col = nullptr;
            int idx = -1;
            for (auto& c : *circuits)
            {
                auto it = std::find (c.begin(), c.end(), static_cast<juce::Component*> (label));
                if (it != c.end())
                {
                    col = &c;
                    idx = (int) std::distance (c.begin(), it);
                    break;
                }
            }
            if (col == nullptr)
                return false;

            bool forward = ! key.getModifiers().isShiftDown();
            int n = (int) col->size();
            int nextIdx = forward ? (idx + 1) % n : (idx + n - 1) % n;

            for (int j = 0; j < n - 1; ++j)
            {
                if ((*col)[(size_t) nextIdx]->isVisible()
                    && (*col)[(size_t) nextIdx]->isEnabled())
                    break;
                nextIdx = forward ? (nextIdx + 1) % n : (nextIdx + n - 1) % n;
            }

            auto* next = (*col)[(size_t) nextIdx];
            label->hideEditor (false);

            if (auto* nextLabel = dynamic_cast<juce::Label*> (next))
                nextLabel->showEditor();
            else
                next->grabKeyboardFocus();

            return true;
        }
    } circuitTabHandler;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClustersTab)

    friend class ClusterInputRowComponent;
};

//==============================================================================
// ClusterInputRowComponent mouse method implementations
// (deferred because they need ClustersTab to be complete)
//==============================================================================

inline void ClusterInputRowComponent::mouseDown (const juce::MouseEvent& e)
{
    if (e.position.x < 20.0f && !isLocked())
    {
        owner.startDrag(inputIdx);
    }
}

inline void ClusterInputRowComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (owner.draggingInputIdx < 0) return;

    // Convert to list-local coordinates
    auto posInList = e.getEventRelativeTo(&owner.inputsList).position;
    owner.handleDragMove(posInList.y);
}

inline void ClusterInputRowComponent::mouseUp (const juce::MouseEvent&)
{
    owner.endDrag();
}

inline void ClusterInputRowComponent::mouseMove (const juce::MouseEvent& e)
{
    if (e.position.x < 20.0f && !isLocked())
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    else
        setMouseCursor(juce::MouseCursor::NormalCursor);
}
