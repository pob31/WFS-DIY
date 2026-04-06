#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "../Sampler/SamplerData.h"
#include "../Localization/LocalizationManager.h"
#include "../AppSettings.h"
#include "ColorScheme.h"
#include "sliders/WfsStandardSlider.h"
#include "sliders/WfsRangeSlider.h"
#include "buttons/LongPressButton.h"
#include "LightpadZoneOverlay.h"
#include "ColumnFocusTraverser.h"
#include "sliders/WfsBidirectionalSlider.h"
#include "StatusBar.h"
#include "HelpCard.h"

/**
 * Sampler subtab for InputsTab.
 * Left 67%: 6x6 sample grid with cell selection.
 * Right 33%: parameter panel (cell props, set management, pressure mappings).
 */
class SamplerSubTab : public juce::Component,
                      public ColorScheme::Manager::Listener,
                      public juce::Label::Listener,
                      public juce::ValueTree::Listener,
                      private juce::Timer
{
public:
    /** Callback fired when sampler data changes (wired in MainComponent) */
    std::function<void()> onSamplerDataChanged;
    std::function<bool()> isQLabAvailable;
    std::function<void(int channelId, int setNumber, const juce::String& setName)> onQLabSetCueRequested;

    /** Callback to trigger preview playback: (channelIndex, cellIndex, noteOn) */
    std::function<void (int, int, bool)> onPreviewCell;

    /** Query callback: returns the cell index currently playing on a channel (-1 if none) */
    std::function<int (int channelIndex)> getPlayingCellIndex;

    SamplerSubTab (WfsParameters& params)
        : parameters (params),
          clearCellButton (800),
          deleteSetButton (800)
    {
        setOpaque (true);
        ColorScheme::Manager::getInstance().addListener (this);

        // ── Grid area (painted in paint(), mouse handled in mouseDown/mouseDrag) ──

        // ── Cell property controls ──
        cellNameEditor.setJustification (juce::Justification::centredLeft);
        cellNameEditor.onReturnKey = [this] { onCellNameChanged(); cellNameEditor.giveAwayKeyboardFocus(); };
        cellNameEditor.onFocusLost = [this] { onCellNameChanged(); };
        cellNameEditor.onEscapeKey = [this] {
            if (! selectedCells.empty()) {
                int idx = selectedCells[0];
                if (idx >= 0 && idx < static_cast<int> (cells.size()))
                    cellNameEditor.setText (cells[static_cast<size_t> (idx)].name, false);
            }
            cellNameEditor.giveAwayKeyboardFocus();
        };
        addChildComponent (cellNameEditor);

        loadCellButton.setButtonText (LOC ("sampler.cell.load"));
        loadCellButton.onClick = [this] { onLoadCell(); };
        addChildComponent (loadCellButton);

        clearCellButton.setButtonText (LOC ("sampler.cell.clear"));
        clearCellButton.onLongPress = [this] { onClearCell(); };
        addChildComponent (clearCellButton);

        previewButton.setButtonText (LOC ("sampler.cell.preview"));
        previewButton.onClick = [this] { onPreviewToggle(); };
        addChildComponent (previewButton);

        inOutRangeSlider = std::make_unique<WfsRangeSlider> (
            WFSParameterDefaults::samplerCellInTimeMin,
            WFSParameterDefaults::samplerCellOutTimeMax);
        inOutRangeSlider->setTrackColours (ColorScheme::get().sliderTrackBg,
                                            juce::Colour (0xFFD4A017)); // Yellow — matches delay bar
        inOutRangeSlider->setShowValues (true);
        inOutRangeSlider->setValueFormatter ([] (float v) { return juce::String ((int) v); });
        inOutRangeSlider->onValuesChanged = [this] (float minV, float maxV)
        {
            onCellParamChanged (WFSParameterIDs::samplerCellInTime, minV);
            onCellParamChanged (WFSParameterIDs::samplerCellOutTime, maxV);
        };
        addChildComponent (*inOutRangeSlider);

        // Offset labels (editable number boxes on one row)
        {
            const char* placeholders[3] = { "X", "Y", "Z" };
            for (int i = 0; i < 3; ++i)
            {
                cellOffsetEditors[i].setJustification (juce::Justification::centred);
                cellOffsetEditors[i].setInputRestrictions (8, "-.0123456789");
                cellOffsetEditors[i].setTextToShowWhenEmpty (placeholders[i], juce::Colours::grey);
                cellOffsetEditors[i].onReturnKey = [this, i] { onOffsetEditorChanged (i); cellOffsetEditors[i].giveAwayKeyboardFocus(); };
                cellOffsetEditors[i].onFocusLost = [this, i] { onOffsetEditorChanged (i); };
                cellOffsetEditors[i].onEscapeKey = [this, i] {
                    if (! selectedCells.empty()) {
                        int idx = selectedCells[0];
                        if (idx >= 0 && idx < static_cast<int> (cells.size())) {
                            const float vals[3] = { cells[static_cast<size_t> (idx)].offsetX,
                                                     cells[static_cast<size_t> (idx)].offsetY,
                                                     cells[static_cast<size_t> (idx)].offsetZ };
                            cellOffsetEditors[i].setText (juce::String (vals[i], 1), false);
                        }
                    }
                    cellOffsetEditors[i].giveAwayKeyboardFocus();
                };
                addChildComponent (cellOffsetEditors[i]);
            }
        }

        cellAttenSlider = std::make_unique<WfsStandardSlider> (
            WFSParameterDefaults::samplerCellAttenuationMin,
            WFSParameterDefaults::samplerCellAttenuationMax);
        cellAttenSlider->setLabel (LOC ("sampler.cell.attenuation"));
        cellAttenSlider->setInlineMode (true);
        cellAttenSlider->setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFF4A90D9));
        cellAttenSlider->setValueToString ([] (float v) { return juce::String (v, 1) + " dB"; });
        cellAttenSlider->onValueChanged = [this] (float v) { onCellParamChanged (WFSParameterIDs::samplerCellAttenuation, v); };
        addChildComponent (*cellAttenSlider);

        // ── Set management controls ──
        setSelector.onChange = [this] { onSetSelectorChanged(); };
        addAndMakeVisible (setSelector);

        addSetButton.setButtonText ("+");
        addSetButton.onClick = [this] { onAddSet(); };
        addAndMakeVisible (addSetButton);

        deleteSetButton.setButtonText ("-");
        deleteSetButton.onLongPress = [this] { onDeleteSet(); };
        addAndMakeVisible (deleteSetButton);

        renameSetButton.setButtonText (LOC ("sampler.set.rename"));
        renameSetButton.onClick = [this] { onRenameSet(); };
        addAndMakeVisible (renameSetButton);

        qlabSetButton.setButtonText ("Q");
        qlabSetButton.onClick = [this] {
            if (onQLabSetCueRequested && activeSetIndex >= 0 && activeSetIndex < static_cast<int> (sets.size()))
            {
                juce::String name = sets[static_cast<size_t> (activeSetIndex)].name;
                if (name.isEmpty())
                    name = LOC ("sampler.set.default") + " " + juce::String (activeSetIndex + 1);
                onQLabSetCueRequested (currentChannel + 1, activeSetIndex + 1, name);
            }
        };
        addChildComponent (qlabSetButton);

        setNameEditor.setJustification (juce::Justification::centredLeft);
        setNameEditor.onReturnKey = [this] { onSetNameChanged(); setNameEditor.giveAwayKeyboardFocus(); };
        setNameEditor.onFocusLost = [this] { onSetNameChanged(); };
        setNameEditor.onEscapeKey = [this] {
            if (activeSetIndex >= 0 && activeSetIndex < static_cast<int> (sets.size()))
                setNameEditor.setText (sets[static_cast<size_t> (activeSetIndex)].name, false);
            setNameEditor.giveAwayKeyboardFocus();
        };
        addChildComponent (setNameEditor);

        playModeButton.setButtonText (LOC ("sampler.set.sequential"));
        playModeButton.onClick = [this] { onTogglePlayMode(); };
        addAndMakeVisible (playModeButton);

        // ── Set position number boxes (X, Y, Z on one row) ──
        {
            const char* placeholders[3] = { "X", "Y", "Z" };
            for (int i = 0; i < 3; ++i)
            {
                setPosEditors[i].setJustification (juce::Justification::centred);
                setPosEditors[i].setInputRestrictions (8, "-.0123456789");
                setPosEditors[i].setTextToShowWhenEmpty (placeholders[i], juce::Colours::grey);
                setPosEditors[i].onReturnKey = [this, i] { onSetPosEditorChanged (i); setPosEditors[i].giveAwayKeyboardFocus(); };
                setPosEditors[i].onFocusLost = [this, i] { onSetPosEditorChanged (i); };
                setPosEditors[i].onEscapeKey = [this, i] {
                    if (activeSetIndex >= 0 && activeSetIndex < static_cast<int> (sets.size())) {
                        const auto& set = sets[static_cast<size_t> (activeSetIndex)];
                        const float vals[3] = { set.posX, set.posY, set.posZ };
                        setPosEditors[i].setText (juce::String (vals[i], 1), false);
                    }
                    setPosEditors[i].giveAwayKeyboardFocus();
                };
                addAndMakeVisible (setPosEditors[i]);
            }
        }

        setLevelSlider = std::make_unique<WfsStandardSlider> (
            WFSParameterDefaults::samplerSetLevelMin,
            WFSParameterDefaults::samplerSetLevelMax);
        setLevelSlider->setLabel (LOC ("sampler.set.level"));
        setLevelSlider->setInlineMode (true);
        setLevelSlider->setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFF4A90D9));
        setLevelSlider->setValueToString ([] (float v) { return juce::String (v, 1) + " dB"; });
        setLevelSlider->onValueChanged = [this] (float v) { onSetParamChanged (WFSParameterIDs::samplerSetLevel, v); };
        addAndMakeVisible (*setLevelSlider);

        // ── Pressure mapping controls ──
        setupPressureRow (pressLevelEnable, pressLevelDirBtn, pressLevelCurveSlider, pressLevelCurveValueLabel,
                          WFSParameterIDs::samplerSetPressLevelEnabled,
                          WFSParameterIDs::samplerSetPressLevelDir,
                          WFSParameterIDs::samplerSetPressLevelCurve,
                          LOC ("sampler.press.level"), true);

        setupPressureRow (pressZEnable, pressZDirBtn, pressZCurveSlider, pressZCurveValueLabel,
                          WFSParameterIDs::samplerSetPressZEnabled,
                          WFSParameterIDs::samplerSetPressZDir,
                          WFSParameterIDs::samplerSetPressZCurve,
                          LOC ("sampler.press.height"), false);

        setupPressureRow (pressHFEnable, pressHFDirBtn, pressHFCurveSlider, pressHFCurveValueLabel,
                          WFSParameterIDs::samplerSetPressHFEnabled,
                          WFSParameterIDs::samplerSetPressHFDir,
                          WFSParameterIDs::samplerSetPressHFCurve,
                          LOC ("sampler.press.hf"), false);

        pressXYEnable.setButtonText (LOC ("sampler.press.xy"));
        pressXYEnable.setToggleState (true, juce::dontSendNotification);  // ON by default
        pressXYEnable.onClick = [this]
        {
            if (! isLoadingData)
                saveCurrentSetProperty (WFSParameterIDs::samplerSetPressXYEnabled,
                                        pressXYEnable.getToggleState() ? 1 : 0);
            float alpha = pressXYEnable.getToggleState() ? 1.0f : 0.4f;
            if (pressXYScaleSlider) pressXYScaleSlider->setAlpha (alpha);
            pressXYScaleValueLabel.setAlpha (alpha);
        };
        addAndMakeVisible (pressXYEnable);

        pressXYScaleSlider = std::make_unique<WfsStandardSlider> (
            WFSParameterDefaults::samplerSetPressXYScaleMin,
            WFSParameterDefaults::samplerSetPressXYScaleMax);
        pressXYScaleSlider->setTrackColours (ColorScheme::get().sliderTrackBg, juce::Colour (0xFFFF6D00));
        pressXYScaleSlider->onValueChanged = [this] (float v)
        {
            if (! isLoadingData)
            {
                pressXYScaleValueLabel.setText (juce::String (v * 100.0f, 1), juce::dontSendNotification);
                saveCurrentSetProperty (WFSParameterIDs::samplerSetPressXYScale, v);
            }
        };
        pressXYScaleSlider->setValue (WFSParameterDefaults::samplerSetPressXYScaleDefault);
        addAndMakeVisible (*pressXYScaleSlider);

        pressXYScaleValueLabel.setText ("5.0", juce::dontSendNotification);
        pressXYScaleValueLabel.setJustificationType (juce::Justification::centred);
        pressXYScaleValueLabel.setEditable (true, false);
        pressXYScaleValueLabel.addListener (this);
        addAndMakeVisible (pressXYScaleValueLabel);

        // ── Lightpad Zone Selector Button ──
        lightpadZoneButton.setButtonText (LOC ("sampler.lightpadZone.none"));
        lightpadZoneButton.onClick = [this]
        {
            if (controllerMode == 1)
                showLightpadZoneOverlay();
            else if (controllerMode == 2)
                showRemotePadZoneOverlay();
        };
        lightpadZoneButton.setVisible (false);  // Shown when lightpad is enabled
        addChildComponent (lightpadZoneButton);

        // ── Copy / Paste ──
        copyButton.setButtonText (LOC ("sampler.buttons.copy"));
        copyButton.onClick = [this] { onCopy(); };
        addAndMakeVisible (copyButton);

        pasteButton.setButtonText (LOC ("sampler.buttons.paste"));
        pasteButton.onClick = [this] { onPaste(); };
        pasteButton.setEnabled (false);
        addAndMakeVisible (pasteButton);

        // ── Import / Export ──
        importButton.setButtonText (LOC ("sampler.buttons.import"));
        importButton.onClick = [this] { onImport(); };
        addAndMakeVisible (importButton);

        exportButton.setButtonText (LOC ("sampler.buttons.export"));
        exportButton.onClick = [this] { onExport(); };
        addAndMakeVisible (exportButton);


        // Sampler help card
        addAndMakeVisible(samplerHelpButton);
        addChildComponent(samplerHelpCard);
        samplerHelpCard.setContent(LOC("help.sampler.title"), LOC("help.sampler.body"));
        samplerHelpButton.setCard(&samplerHelpCard);

        setFocusContainerType (FocusContainerType::keyboardFocusContainer);
    }

    ~SamplerSubTab() override
    {
        stopTimer();
        if (samplerTree.isValid())
            samplerTree.removeListener (this);
        ColorScheme::Manager::getInstance().removeListener (this);
    }

    std::unique_ptr<juce::ComponentTraverser> createKeyboardFocusTraverser() override
    {
        return std::make_unique<ColumnCircuitTraverser> (std::vector<std::vector<juce::Component*>> {
            // Cell properties: Offset X / Y / Z
            { &cellOffsetEditors[0], &cellOffsetEditors[1], &cellOffsetEditors[2] },
            // Set management: Pos X / Y / Z
            { &setPosEditors[0], &setPosEditors[1], &setPosEditors[2] }
        });
    }

    // ==================== LAYOUT ====================

    void resized() override
    {
        // Use 760 as reference (sub-tab content area height at 1080p)
        layoutScale = static_cast<float> (getHeight()) / 760.0f;
        auto bounds = getLocalBounds();

        // Right panel: 33%
        int panelWidth = juce::jmax (220, bounds.getWidth() * 33 / 100);
        panelArea = bounds.removeFromRight (panelWidth);

        // Grid area: remaining 67%
        gridArea = bounds;

        // Help button — top-right of grid area
        {
            const int btnSize = scaled (22);
            samplerHelpButton.setBounds(gridArea.getRight() - btnSize - scaled (8), gridArea.getY() + scaled (8), btnSize, btnSize);
            // Help card — centered over the grid tiles
            int cardW = juce::jmin(gridArea.getWidth() - 40, 700);
            int cardH = samplerHelpCard.getIdealHeight(cardW);
            int cardX = gridArea.getX() + (gridArea.getWidth() - cardW) / 2;
            int cardY = gridArea.getY() + 40;
            samplerHelpCard.setBounds(cardX, cardY, cardW, cardH);
        }

        layoutPanel();
        WfsLookAndFeel::scaleTextEditorFonts (*this, layoutScale);
    }

    // ==================== PAINT ====================

    void paint (juce::Graphics& g) override
    {
        auto& cs = ColorScheme::get();

        // Background
        g.fillAll (cs.background);

        // ── Paint grid ──
        paintGrid (g);

        // ── Paint panel background ──
        g.setColour (cs.surfaceCard);
        g.fillRect (panelArea);

        // Panel separator line
        g.setColour (cs.chromeDivider);
        g.drawVerticalLine (panelArea.getX(), static_cast<float> (panelArea.getY()),
                            static_cast<float> (panelArea.getBottom()));

        // Section separator lines
        for (int sepY : sectionSeparatorYs)
        {
            g.setColour (cs.chromeDivider);
            g.drawHorizontalLine (sepY, static_cast<float> (panelArea.getX() + 4),
                                  static_cast<float> (panelArea.getRight() - 4));
        }

        // Section labels
        g.setColour (cs.textSecondary);
        g.setFont (juce::FontOptions (juce::jmax (10.0f, 14.0f * layoutScale)));
        for (auto& [label, yPos] : sectionLabels)
            g.drawText (label, panelArea.getX() + 8, yPos, panelArea.getWidth() - 16, scaled (20),
                        juce::Justification::centredLeft);

        // Control inline labels (e.g. "In/Out (ms)", "Offset (m)", "Position (m)")
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.setFont (juce::FontOptions (juce::jmax (10.0f, 14.0f * layoutScale)));
        for (auto& cl : controlLabels)
            g.drawText (cl.text, cl.x, cl.y, cl.w, cl.h, juce::Justification::centredLeft);

        // Guide text when no cell is selected
        if (selectedCells.empty())
        {
            g.setColour (cs.textSecondary.withAlpha (0.4f));
            g.setFont (juce::FontOptions (juce::jmax (10.0f, 12.0f * WfsLookAndFeel::uiScale)));
            int guideTopTrim = lightpadZoneButton.isVisible() ? 60 : 20;
            auto guideArea = panelArea.withTrimmedTop (guideTopTrim).reduced (8);
            g.drawFittedText (LOC ("sampler.guide"), guideArea,
                              juce::Justification::centredTop, 4);
        }
    }

    // ==================== MOUSE HANDLING ====================

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! gridArea.contains (e.getPosition()))
            return;

        int cellIdx = hitTestGrid (e.getPosition());

        // Click on grid padding/margin (no cell hit) → ignore
        if (cellIdx < 0)
            return;

        // Ctrl+Click → toggle cell in/out of active set
        if (e.mods.isCtrlDown() && activeSetIndex >= 0
            && activeSetIndex < static_cast<int> (sets.size()))
        {
            auto& set = sets[static_cast<size_t> (activeSetIndex)];
            auto it = std::find (set.cellIndices.begin(), set.cellIndices.end(), cellIdx);
            if (it != set.cellIndices.end())
                set.cellIndices.erase (it);
            else
                set.cellIndices.push_back (cellIdx);
            saveSetToValueTree (activeSetIndex);
            notifyDataChanged();
            repaint();
            return;
        }

        if (e.mods.isShiftDown())
        {
            // Toggle selection
            auto it = std::find (selectedCells.begin(), selectedCells.end(), cellIdx);
            if (it != selectedCells.end())
                selectedCells.erase (it);
            else
                selectedCells.push_back (cellIdx);
        }
        else
        {
            // Single select
            if (std::find (selectedCells.begin(), selectedCells.end(), cellIdx) == selectedCells.end())
            {
                selectedCells.clear();
                selectedCells.push_back (cellIdx);
            }
        }

        updateCellPropertyPanel();
        updateCopyPasteButtons();
        repaint();
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        if (! gridArea.contains (e.getPosition()))
            return;

        int cellIdx = hitTestGrid (e.getPosition());
        if (cellIdx >= 0)
        {
            selectedCells.clear();
            selectedCells.push_back (cellIdx);
            updateCellPropertyPanel();
            onLoadCell();
        }
    }

    // ==================== PUBLIC API ====================

    void setStatusBar (StatusBar* bar)
    {
        statusBar = bar;
        setupHelpText();
    }

    /** Set the current input channel (0-based) */
    void setCurrentChannel (int channel)
    {
        if (currentChannel != channel)
        {
            stopPreview();
            currentChannel = channel;
            selectedCells.clear();
            loadFromValueTree();
        }
    }

    int getCurrentChannel() const noexcept { return currentChannel; }
    int getActiveSetIndex() const noexcept { return activeSetIndex; }

    /** Reload data from ValueTree (call when channel changes or external edit occurs) */
    void loadFromValueTree()
    {
        isLoadingData = true;

        if (samplerTree.isValid())
            samplerTree.removeListener (this);

        samplerTree = parameters.getValueTreeState().getInputSamplerSection (currentChannel);

        if (samplerTree.isValid())
            samplerTree.addListener (this);

        // Load cells
        cells.clear();
        cells.resize (WFSParameterDefaults::samplerGridCells);

        if (samplerTree.isValid())
        {
            for (int i = 0; i < samplerTree.getNumChildren(); ++i)
            {
                auto child = samplerTree.getChild (i);
                if (child.hasType (WFSParameterIDs::SamplerCell))
                {
                    int id = child.getProperty ("id", -1);
                    if (id >= 0 && id < WFSParameterDefaults::samplerGridCells)
                        cells[static_cast<size_t> (id)].loadFromValueTree (child);
                }
            }

            // Load sets
            sets.clear();
            for (int i = 0; i < samplerTree.getNumChildren(); ++i)
            {
                auto child = samplerTree.getChild (i);
                if (child.hasType (WFSParameterIDs::SamplerSet))
                {
                    SamplerData::SamplerSet set;
                    set.loadFromValueTree (child);
                    sets.push_back (set);
                }
            }
        }

        // Update set selector
        setSelector.clear (juce::dontSendNotification);
        for (int i = 0; i < static_cast<int> (sets.size()); ++i)
        {
            juce::String name = sets[static_cast<size_t> (i)].name;
            if (name.isEmpty())
                name = LOC ("sampler.set.default") + " " + juce::String (i + 1);
            setSelector.addItem (juce::String (i + 1) + ". " + name, i + 1);
        }

        if (! sets.empty())
        {
            // Read persisted active set from ValueTree
            int storedSet = samplerTree.isValid()
                ? static_cast<int> (samplerTree.getProperty (WFSParameterIDs::inputSamplerActiveSet, 0))
                : 0;
            activeSetIndex = juce::jlimit (0, static_cast<int> (sets.size()) - 1, storedSet);
            setSelector.setSelectedId (activeSetIndex + 1, juce::dontSendNotification);
        }
        else
        {
            activeSetIndex = -1;
        }

        updateSetPropertyPanel();
        updateCellPropertyPanel();
        updateCopyPasteButtons();

        // Load lightpad zone assignment for current channel
        if (lightpadZoneButton.isVisible())
        {
            int zoneId = static_cast<int> (parameters.getInputParam (
                currentChannel, WFSParameterIDs::lightpadZoneId.toString()));
            currentLightpadZoneId = zoneId;
            updateLightpadZoneButtonText();
        }

        isLoadingData = false;
        repaint();
    }

    // ColorScheme::Manager::Listener
    void colorSchemeChanged() override { repaint(); }

    // ValueTree::Listener — react to external changes (e.g. OSC set selection)
    void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override
    {
        if (isLoadingData) return;
        if (tree == samplerTree && property == WFSParameterIDs::inputSamplerActiveSet)
        {
            int newIdx = static_cast<int> (tree.getProperty (property));
            if (newIdx >= 0 && newIdx < static_cast<int> (sets.size()) && newIdx != activeSetIndex)
            {
                activeSetIndex = newIdx;
                setSelector.setSelectedId (activeSetIndex + 1, juce::dontSendNotification);
                updateSetPropertyPanel();
                repaint();
            }
        }
    }

    void visibilityChanged() override
    {
        if (isVisible())
            startPlaybackMonitor();
        else
            stopPlaybackMonitor();
    }

    // Timer for visual feedback on playing cells and auto-stop preview
    void timerCallback() override
    {
        int playingCell = -1;
        if (getPlayingCellIndex)
            playingCell = getPlayingCellIndex (currentChannel);

        // Auto-stop preview when playback ends
        if (previewingCellIndex >= 0 && playingCell < 0 && playEndTime == 0)
        {
            previewingCellIndex = -1;
            previewButton.setButtonText (LOC ("sampler.cell.preview"));
        }

        // Update visual feedback with hold timer
        if (playingCell >= 0)
        {
            // Currently playing — show highlight immediately
            if (lastPlayingCell != playingCell)
            {
                lastPlayingCell = playingCell;
                repaint();
            }
            playEndTime = 0;
        }
        else if (lastPlayingCell >= 0)
        {
            // Playback just stopped — hold highlight briefly
            if (playEndTime == 0)
                playEndTime = juce::Time::currentTimeMillis();

            if (juce::Time::currentTimeMillis() - playEndTime > 150)
            {
                lastPlayingCell = -1;
                playEndTime = 0;
                repaint();
            }
        }
    }

    void startPlaybackMonitor()  { if (! isTimerRunning()) startTimer (50); }
    void stopPlaybackMonitor()   { stopTimer(); lastPlayingCell = -1; playEndTime = 0; }

private:
    // ==================== GRID PAINTING ====================

    void paintGrid (juce::Graphics& g)
    {
        auto& cs = ColorScheme::get();
        const int rows = WFSParameterDefaults::samplerGridRows;
        const int cols = WFSParameterDefaults::samplerGridCols;
        const int pad = 3;

        auto area = gridArea.reduced (8);
        float cellW = static_cast<float> (area.getWidth() - (cols - 1) * pad) / static_cast<float> (cols);
        float cellH = static_cast<float> (area.getHeight() - (rows - 1) * pad) / static_cast<float> (rows);

        // Make cells square, use the smaller dimension
        float cellSize = juce::jmin (cellW, cellH);

        // Center the grid
        float totalW = cellSize * cols + static_cast<float> ((cols - 1) * pad);
        float totalH = cellSize * rows + static_cast<float> ((rows - 1) * pad);
        float startX = static_cast<float> (area.getX()) + (static_cast<float> (area.getWidth()) - totalW) * 0.5f;
        float startY = static_cast<float> (area.getY()) + (static_cast<float> (area.getHeight()) - totalH) * 0.5f;

        // Determine which cells are in the active set
        std::set<int> setCellIndices;
        if (activeSetIndex >= 0 && activeSetIndex < static_cast<int> (sets.size()))
        {
            for (int idx : sets[static_cast<size_t> (activeSetIndex)].cellIndices)
                setCellIndices.insert (idx);
        }

        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                int cellIdx = row * cols + col;
                float x = startX + static_cast<float> (col) * (cellSize + static_cast<float> (pad));
                float y = startY + static_cast<float> (row) * (cellSize + static_cast<float> (pad));
                auto cellRect = juce::Rectangle<float> (x, y, cellSize, cellSize);

                bool isSelected = std::find (selectedCells.begin(), selectedCells.end(), cellIdx) != selectedCells.end();
                bool isInSet = setCellIndices.count (cellIdx) > 0;
                bool hasAudio = cellIdx < static_cast<int> (cells.size()) && ! cells[static_cast<size_t> (cellIdx)].isEmpty();
                bool isPlaying = (cellIdx == lastPlayingCell);

                // Cell background
                if (isPlaying)
                    g.setColour (juce::Colour (0xFFFF6D00).withAlpha (0.5f));  // Orange glow
                else if (isSelected)
                    g.setColour (cs.accentBlue.withAlpha (0.4f));
                else if (isInSet)
                    g.setColour (cs.accentGreen.withAlpha (0.2f));
                else
                    g.setColour (cs.surfaceCard);

                g.fillRoundedRectangle (cellRect, 3.0f);

                // Cell border
                if (isPlaying)
                    g.setColour (juce::Colour (0xFFFF6D00));  // Orange border
                else if (isSelected)
                    g.setColour (cs.accentBlue);
                else if (isInSet)
                    g.setColour (cs.accentGreen.withAlpha (0.6f));
                else
                    g.setColour (cs.chromeDivider);

                g.drawRoundedRectangle (cellRect, 3.0f, (isSelected || isPlaying) ? 2.0f : 1.0f);

                // Cell content
                if (hasAudio)
                {
                    // Show name or cell number
                    auto& cell = cells[static_cast<size_t> (cellIdx)];
                    juce::String displayName = cell.name.isNotEmpty()
                        ? cell.name
                        : juce::String (cellIdx + 1);

                    g.setColour (cs.textPrimary);
                    g.setFont (juce::FontOptions (juce::jmin (cellSize * 0.22f, 12.0f)));
                    g.drawText (displayName, cellRect.reduced (2), juce::Justification::centred);

                    // Small audio indicator dot
                    g.setColour (cs.accentBlue.withAlpha (0.7f));
                    g.fillEllipse (x + cellSize - 8.0f, y + 2.0f, 5.0f, 5.0f);
                }
                else
                {
                    // Empty cell number
                    g.setColour (cs.textSecondary.withAlpha (0.55f));
                    g.setFont (juce::FontOptions (juce::jmin (cellSize * 0.4f, 22.0f)));
                    g.drawText (juce::String (cellIdx + 1), cellRect, juce::Justification::centred);
                }

                // Sequence number badge for cells in the active set
                if (isInSet && activeSetIndex >= 0 && activeSetIndex < static_cast<int> (sets.size()))
                {
                    auto& setIndices = sets[static_cast<size_t> (activeSetIndex)].cellIndices;
                    auto it = std::find (setIndices.begin(), setIndices.end(), cellIdx);
                    if (it != setIndices.end())
                    {
                        int seqNum = static_cast<int> (std::distance (setIndices.begin(), it)) + 1;
                        float badgeW = juce::jmin (18.0f, cellSize * 0.3f);
                        float badgeH = juce::jmin (14.0f, cellSize * 0.25f);
                        g.setColour (cs.accentGreen);
                        g.fillRoundedRectangle (x + 2.0f, y + 2.0f, badgeW, badgeH, 3.0f);
                        g.setColour (cs.textPrimary);
                        g.setFont (juce::FontOptions (juce::jmin (10.0f, badgeH - 2.0f)));
                        g.drawText (juce::String (seqNum),
                                    static_cast<int> (x + 2.0f), static_cast<int> (y + 2.0f),
                                    static_cast<int> (badgeW), static_cast<int> (badgeH),
                                    juce::Justification::centred);
                    }
                }
            }
        }

        // Help hint text at bottom-left of grid (multi-line, larger font)
        g.setColour (cs.textSecondary.withAlpha (0.35f));
        g.setFont (juce::FontOptions (juce::jmax (14.0f, 20.0f * layoutScale)));
        auto hintText = LOC ("sampler.grid.help");
        float lineH = g.getCurrentFont().getHeight() * 1.3f;
        int numHintLines = 4;
        int hintH = (int)(numHintLines * lineH);
        g.drawFittedText (hintText,
                          gridArea.getX() + 8, gridArea.getBottom() - hintH - 8,
                          gridArea.getWidth() / 3, hintH,
                          juce::Justification::bottomLeft, numHintLines);
    }

    int hitTestGrid (juce::Point<int> pos) const
    {
        const int rows = WFSParameterDefaults::samplerGridRows;
        const int cols = WFSParameterDefaults::samplerGridCols;
        const int pad = 3;

        auto area = gridArea.reduced (8);
        float cellW = static_cast<float> (area.getWidth() - (cols - 1) * pad) / static_cast<float> (cols);
        float cellH = static_cast<float> (area.getHeight() - (rows - 1) * pad) / static_cast<float> (rows);
        float cellSize = juce::jmin (cellW, cellH);

        float totalW = cellSize * cols + static_cast<float> ((cols - 1) * pad);
        float totalH = cellSize * rows + static_cast<float> ((rows - 1) * pad);
        float startX = static_cast<float> (area.getX()) + (static_cast<float> (area.getWidth()) - totalW) * 0.5f;
        float startY = static_cast<float> (area.getY()) + (static_cast<float> (area.getHeight()) - totalH) * 0.5f;

        float relX = static_cast<float> (pos.x) - startX;
        float relY = static_cast<float> (pos.y) - startY;

        if (relX < 0 || relY < 0) return -1;

        int col = static_cast<int> (relX / (cellSize + static_cast<float> (pad)));
        int row = static_cast<int> (relY / (cellSize + static_cast<float> (pad)));

        if (col < 0 || col >= cols || row < 0 || row >= rows) return -1;

        // Check we're actually within the cell (not in the padding)
        float cellX = static_cast<float> (col) * (cellSize + static_cast<float> (pad));
        float cellY = static_cast<float> (row) * (cellSize + static_cast<float> (pad));
        if (relX > cellX + cellSize || relY > cellY + cellSize) return -1;

        return row * cols + col;
    }

    // ==================== PANEL LAYOUT ====================

    void layoutPanel()
    {
        const int rowH = scaled (30);
        const int pad = scaled (5);
        const int sectionGap = scaled (8);
        int x0 = panelArea.getX() + 6;
        int contentW = panelArea.getWidth() - 12;

        sectionSeparatorYs.clear();
        sectionLabels.clear();
        controlLabels.clear();

        int y = panelArea.getY() + 4;

        // ── Lightpad Zone Selector ──
        if (lightpadZoneButton.isVisible())
        {
            int labelW = contentW * 40 / 100;
            controlLabels.push_back ({ LOC ("sampler.labels.lightpadZone"), x0, y, labelW, rowH });
            lightpadZoneButton.setBounds (x0 + labelW + pad, y, contentW - labelW - pad, rowH);
            y += rowH + pad + sectionGap;
            sectionSeparatorYs.push_back (y);
            y += sectionGap;
        }

        // ── Section: Cell Properties ──
        sectionLabels.push_back ({ LOC ("sampler.section.cell"), y });
        y += scaled (20);

        cellNameEditor.setBounds (x0, y, contentW, rowH);
        y += rowH + pad;

        int btnW = (contentW - pad * 2) / 3;
        loadCellButton.setBounds (x0, y, btnW, rowH);
        clearCellButton.setBounds (x0 + btnW + pad, y, btnW, rowH);
        previewButton.setBounds (x0 + 2 * (btnW + pad), y, contentW - 2 * (btnW + pad), rowH);
        y += rowH + pad;

        // In/Out range slider with label
        {
            int labelW = contentW * 30 / 100;
            int sliderW = contentW - labelW - pad;
            controlLabels.push_back ({ LOC ("sampler.cell.inOut"), x0, y, labelW, rowH });
            if (inOutRangeSlider) inOutRangeSlider->setBounds (x0 + labelW + pad, y, sliderW, rowH);
            y += rowH + pad;
        }

        // Offset X/Y/Z number boxes with label
        {
            int labelW = contentW * 30 / 100;
            int boxAreaW = contentW - labelW - pad;
            int boxW = (boxAreaW - pad * 2) / 3;
            controlLabels.push_back ({ LOC ("sampler.cell.offset"), x0, y, labelW, rowH });
            for (int i = 0; i < 3; ++i)
                cellOffsetEditors[i].setBounds (x0 + labelW + pad + i * (boxW + pad), y, boxW, rowH);
            y += rowH + pad;
        }

        if (cellAttenSlider)   { cellAttenSlider->setBounds (x0, y, contentW, rowH);   y += rowH + pad; }

        // ── Separator ──
        y += sectionGap;
        sectionSeparatorYs.push_back (y);
        y += sectionGap;

        // ── Section: Set Management ──
        sectionLabels.push_back ({ LOC ("sampler.section.set"), y });
        y += scaled (20);

        // Row 1: [selector] [+] [-] [Q]
        int sqBtn = rowH;  // Square buttons
        int qlabBtnW = (isQLabAvailable && isQLabAvailable()) ? sqBtn + pad : 0;
        int selectorW = contentW - 2 * (sqBtn + pad) - qlabBtnW;
        setSelector.setBounds (x0, y, juce::jmax (60, selectorW), rowH);
        int btnX = x0 + juce::jmax (60, selectorW) + pad;
        addSetButton.setBounds (btnX, y, sqBtn, rowH);
        btnX += sqBtn + pad;
        deleteSetButton.setBounds (btnX, y, sqBtn, rowH);
        btnX += sqBtn + pad;
        qlabSetButton.setBounds (btnX, y, sqBtn, rowH);
        qlabSetButton.setVisible (isQLabAvailable && isQLabAvailable());
        y += rowH + pad;

        // Row 2: [Rename 50%] [Sequential 50%]
        int halfW3 = (contentW - pad) / 2;
        renameSetButton.setBounds (x0, y, halfW3, rowH);
        playModeButton.setBounds (x0 + halfW3 + pad, y, contentW - halfW3 - pad, rowH);
        y += rowH + pad;

        setNameEditor.setBounds (x0, y, contentW, rowH);
        y += rowH + pad;

        // Set position X/Y/Z on one row with label
        {
            int labelW = contentW * 30 / 100;
            int boxAreaW = contentW - labelW - pad;
            int boxW = (boxAreaW - pad * 2) / 3;
            controlLabels.push_back ({ LOC ("sampler.set.pos"), x0, y, labelW, rowH });
            for (int i = 0; i < 3; ++i)
                setPosEditors[i].setBounds (x0 + labelW + pad + i * (boxW + pad), y, boxW, rowH);
            y += rowH + pad;
        }

        if (setLevelSlider) { setLevelSlider->setBounds (x0, y, contentW, rowH); y += rowH + pad; }

        // ── Separator ──
        y += sectionGap;
        sectionSeparatorYs.push_back (y);
        y += sectionGap;

        // ── Section: Pressure Mappings ──
        sectionLabels.push_back ({ LOC ("sampler.section.pressure"), y });
        y += scaled (20);

        layoutPressureRow (y, pressLevelEnable, pressLevelDirBtn, pressLevelCurveSlider, pressLevelCurveValueLabel, x0, contentW, rowH, pad);
        layoutPressureRow (y, pressZEnable, pressZDirBtn, pressZCurveSlider, pressZCurveValueLabel, x0, contentW, rowH, pad);
        layoutPressureRow (y, pressHFEnable, pressHFDirBtn, pressHFCurveSlider, pressHFCurveValueLabel, x0, contentW, rowH, pad);

        int halfW2 = (contentW - pad) / 2;
        int xyValueW = 45;
        pressXYEnable.setBounds (x0, y, halfW2, rowH);
        if (pressXYScaleSlider) pressXYScaleSlider->setBounds (x0 + halfW2 + pad, y, halfW2 - xyValueW - pad, rowH);
        pressXYScaleValueLabel.setBounds (x0 + contentW - xyValueW, y, xyValueW, rowH);
        y += rowH + pad;

        // ── Section: Copy/Paste & Import/Export — anchored at bottom ──
        {
            int halfW = (contentW - pad) / 2;
            int ay = panelArea.getBottom() - pad - rowH;
            importButton.setBounds (x0, ay, halfW, rowH);
            exportButton.setBounds (x0 + halfW + pad, ay, halfW, rowH);
            ay -= rowH + pad;
            copyButton.setBounds (x0, ay, halfW, rowH);
            pasteButton.setBounds (x0 + halfW + pad, ay, halfW, rowH);
            ay -= scaled (16);
            sectionLabels.push_back ({ LOC ("sampler.section.actions"), ay });
            ay -= sectionGap;
            sectionSeparatorYs.push_back (ay);
        }
    }

    void layoutPressureRow (int& y, juce::ToggleButton& enable,
                            juce::TextButton& dirBtn, WfsBidirectionalSlider& curveSlider,
                            juce::Label& valueLabel,
                            int x0, int contentW, int rowH, int pad)
    {
        int enableW = contentW * 35 / 100;
        int dirW = 30;
        int valueW = 45;
        int sliderW = contentW - enableW - dirW - valueW - pad * 3;
        enable.setBounds (x0, y, enableW, rowH);
        dirBtn.setBounds (x0 + enableW + pad, y, dirW, rowH);
        curveSlider.setBounds (x0 + enableW + pad + dirW + pad, y, sliderW, rowH);
        valueLabel.setBounds (x0 + contentW - valueW, y, valueW, rowH);
        y += rowH + pad;
    }

    // ==================== CELL PROPERTY PANEL ====================

    void setCellPropertiesVisible (bool v)
    {
        cellNameEditor.setVisible (v);
        loadCellButton.setVisible (v);
        clearCellButton.setVisible (v);
        previewButton.setVisible (v);
        if (inOutRangeSlider) inOutRangeSlider->setVisible (v);
        for (int i = 0; i < 3; ++i)
            cellOffsetEditors[i].setVisible (v);
        if (cellAttenSlider)   cellAttenSlider->setVisible (v);
    }

    void updateCellPropertyPanel()
    {
        bool hasSel = ! selectedCells.empty();
        setCellPropertiesVisible (hasSel);

        if (! hasSel) return;

        isLoadingData = true;

        int idx = selectedCells[0];
        if (idx >= 0 && idx < static_cast<int> (cells.size()))
        {
            auto& cell = cells[static_cast<size_t> (idx)];
            cellNameEditor.setText (cell.name, false);
            if (inOutRangeSlider) inOutRangeSlider->setValues (cell.inTime, cell.outTime);
            cellOffsetEditors[0].setText (juce::String (cell.offsetX, 1), false);
            cellOffsetEditors[1].setText (juce::String (cell.offsetY, 1), false);
            cellOffsetEditors[2].setText (juce::String (cell.offsetZ, 1), false);
            if (cellAttenSlider)   cellAttenSlider->setValue (cell.attenuation);
        }

        isLoadingData = false;
    }

    // ==================== SET PROPERTY PANEL ====================

    void updateSetPropertyPanel()
    {
        bool hasSet = activeSetIndex >= 0 && activeSetIndex < static_cast<int> (sets.size());
        setNameEditor.setVisible (false);  // Only visible during rename

        if (! hasSet)
        {
            playModeButton.setEnabled (false);
            return;
        }

        isLoadingData = true;

        auto& set = sets[static_cast<size_t> (activeSetIndex)];
        playModeButton.setEnabled (true);
        playModeButton.setButtonText (set.playMode == 0
            ? LOC ("sampler.set.sequential")
            : LOC ("sampler.set.roundRobin"));

        setPosEditors[0].setText (juce::String (set.posX, 1), false);
        setPosEditors[1].setText (juce::String (set.posY, 1), false);
        setPosEditors[2].setText (juce::String (set.posZ, 1), false);
        if (setLevelSlider) setLevelSlider->setValue (set.level);

        // Pressure mappings
        pressLevelEnable.setToggleState (set.pressLevel.enabled, juce::dontSendNotification);
        pressLevelDirBtn.setButtonText (set.pressLevel.direction == 0 ? "+" : "-");
        pressLevelCurveSlider.setValue (set.pressLevel.curve * 2.0f - 1.0f);
        pressLevelCurveSlider.repaint();
        pressLevelCurveValueLabel.setText (juce::String (set.pressLevel.curve, 2), juce::dontSendNotification);
        { float a = set.pressLevel.enabled ? 1.0f : 0.4f; pressLevelDirBtn.setAlpha (a); pressLevelCurveSlider.setAlpha (a); pressLevelCurveValueLabel.setAlpha (a); }

        pressZEnable.setToggleState (set.pressZ.enabled, juce::dontSendNotification);
        pressZDirBtn.setButtonText (set.pressZ.direction == 0 ? "+" : "-");
        pressZCurveSlider.setValue (set.pressZ.curve * 2.0f - 1.0f);
        pressZCurveSlider.repaint();
        pressZCurveValueLabel.setText (juce::String (set.pressZ.curve, 2), juce::dontSendNotification);
        { float a = set.pressZ.enabled ? 1.0f : 0.4f; pressZDirBtn.setAlpha (a); pressZCurveSlider.setAlpha (a); pressZCurveValueLabel.setAlpha (a); }

        pressHFEnable.setToggleState (set.pressHF.enabled, juce::dontSendNotification);
        pressHFDirBtn.setButtonText (set.pressHF.direction == 0 ? "+" : "-");
        pressHFCurveSlider.setValue (set.pressHF.curve * 2.0f - 1.0f);
        pressHFCurveSlider.repaint();
        pressHFCurveValueLabel.setText (juce::String (set.pressHF.curve, 2), juce::dontSendNotification);
        { float a = set.pressHF.enabled ? 1.0f : 0.4f; pressHFDirBtn.setAlpha (a); pressHFCurveSlider.setAlpha (a); pressHFCurveValueLabel.setAlpha (a); }

        pressXYEnable.setToggleState (set.pressXYEnabled, juce::dontSendNotification);
        if (pressXYScaleSlider) pressXYScaleSlider->setValue (set.pressXYScale);
        pressXYScaleValueLabel.setText (juce::String (set.pressXYScale * 100.0f, 1), juce::dontSendNotification);
        { float a = set.pressXYEnabled ? 1.0f : 0.4f; if (pressXYScaleSlider) pressXYScaleSlider->setAlpha (a); pressXYScaleValueLabel.setAlpha (a); }

        isLoadingData = false;
    }

    // ==================== CELL CALLBACKS ====================

    void onCellNameChanged()
    {
        if (isLoadingData || selectedCells.empty()) return;
        int idx = selectedCells[0];
        if (idx < 0 || idx >= static_cast<int> (cells.size())) return;

        cells[static_cast<size_t> (idx)].name = cellNameEditor.getText();
        saveCellProperty (idx, WFSParameterIDs::samplerCellName, cellNameEditor.getText());
        repaint();
    }

    void onOffsetEditorChanged (int axis)
    {
        if (isLoadingData || selectedCells.empty()) return;
        float val = cellOffsetEditors[axis].getText().getFloatValue();
        val = juce::jlimit (WFSParameterDefaults::samplerCellOffsetMin,
                            WFSParameterDefaults::samplerCellOffsetMax, val);
        cellOffsetEditors[axis].setText (juce::String (val, 1), false);

        static const juce::Identifier ids[3] = {
            WFSParameterIDs::samplerCellOffsetX,
            WFSParameterIDs::samplerCellOffsetY,
            WFSParameterIDs::samplerCellOffsetZ
        };
        onCellParamChanged (ids[axis], val);
    }

    void onSetPosEditorChanged (int axis)
    {
        if (isLoadingData || activeSetIndex < 0 || activeSetIndex >= static_cast<int> (sets.size())) return;
        float val = setPosEditors[axis].getText().getFloatValue();
        val = juce::jlimit (WFSParameterDefaults::samplerSetPosMin,
                            WFSParameterDefaults::samplerSetPosMax, val);
        setPosEditors[axis].setText (juce::String (val, 1), false);

        static const juce::Identifier ids[3] = {
            WFSParameterIDs::samplerSetPosX,
            WFSParameterIDs::samplerSetPosY,
            WFSParameterIDs::samplerSetPosZ
        };
        onSetParamChanged (ids[axis], val);
    }

    void onCellParamChanged (const juce::Identifier& paramId, float value)
    {
        if (isLoadingData || selectedCells.empty()) return;

        for (int idx : selectedCells)
        {
            if (idx < 0 || idx >= static_cast<int> (cells.size())) continue;

            auto& cell = cells[static_cast<size_t> (idx)];
            if (paramId == WFSParameterIDs::samplerCellInTime)       cell.inTime = value;
            else if (paramId == WFSParameterIDs::samplerCellOutTime) cell.outTime = value;
            else if (paramId == WFSParameterIDs::samplerCellOffsetX) cell.offsetX = value;
            else if (paramId == WFSParameterIDs::samplerCellOffsetY) cell.offsetY = value;
            else if (paramId == WFSParameterIDs::samplerCellOffsetZ) cell.offsetZ = value;
            else if (paramId == WFSParameterIDs::samplerCellAttenuation) cell.attenuation = value;

            saveCellProperty (idx, paramId, value);
        }
    }

    void onLoadCell()
    {
        if (selectedCells.empty()) return;

        // Snapshot selection — an async reload (e.g. from ValueTree listener)
        // could clear selectedCells before the file-chooser callback runs.
        auto savedSelection = selectedCells;

        auto lastFolder = AppSettings::getLastFolder ("lastSampleFolder");
        fileChooser = std::make_unique<juce::FileChooser> (
            LOC ("sampler.cell.loadTitle"), lastFolder, "*.wav;*.aiff;*.flac;*.ogg");

        fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, savedSelection] (const juce::FileChooser& fc)
            {
                auto result = fc.getResult();
                if (! result.existsAsFile()) return;

                AppSettings::setLastFolder ("lastSampleFolder", result.getParentDirectory());

                // Import to project samples/ folder
                auto& fm = parameters.getFileManager();
                if (! fm.hasValidProjectFolder())
                {
                    // No project folder — use file in place (store absolute path)
                    juce::String relPath = result.getFullPathName();
                    for (int idx : savedSelection)
                    {
                        if (idx < 0 || idx >= static_cast<int> (cells.size())) continue;
                        auto& cell = cells[static_cast<size_t> (idx)];
                        cell.relativeFilePath = relPath;
                        if (cell.name.isEmpty())
                            cell.name = result.getFileNameWithoutExtension();
                        saveCellProperty (idx, WFSParameterIDs::samplerCellFile, relPath);
                        saveCellProperty (idx, WFSParameterIDs::samplerCellName, cell.name);
                    }
                    if (selectedCells.empty() && ! savedSelection.empty())
                        selectedCells = savedSelection;
                    updateCellPropertyPanel();
                    notifyDataChanged();
                    repaint();
                    return;
                }

                auto samplesFolder = fm.getSamplesFolder();
                if (! samplesFolder.isDirectory())
                    samplesFolder.createDirectory();

                juce::String relPath = result.getFileName();

                // Copy file if not already in samples folder
                if (result.getParentDirectory() != samplesFolder)
                {
                    auto dest = samplesFolder.getChildFile (result.getFileName());
                    if (! dest.existsAsFile())
                        result.copyFileTo (dest);
                    else if (dest != result)
                    {
                        // Name collision — append counter
                        auto base = result.getFileNameWithoutExtension();
                        auto ext = result.getFileExtension();
                        int counter = 1;
                        do { dest = samplesFolder.getChildFile (base + "_" + juce::String (counter++) + ext); }
                        while (dest.existsAsFile() && counter < 1000);
                        result.copyFileTo (dest);
                        relPath = dest.getFileName();
                    }
                }

                // Apply to all cells that were selected when Load was clicked
                for (int idx : savedSelection)
                {
                    if (idx < 0 || idx >= static_cast<int> (cells.size())) continue;

                    auto& cell = cells[static_cast<size_t> (idx)];
                    cell.relativeFilePath = relPath;
                    if (cell.name.isEmpty())
                        cell.name = result.getFileNameWithoutExtension();

                    saveCellProperty (idx, WFSParameterIDs::samplerCellFile, relPath);
                    saveCellProperty (idx, WFSParameterIDs::samplerCellName, cell.name);
                }

                // Restore selection if it was cleared during the dialog
                if (selectedCells.empty() && ! savedSelection.empty())
                    selectedCells = savedSelection;

                updateCellPropertyPanel();
                notifyDataChanged();
                repaint();
            });
    }

    void onPreviewToggle()
    {
        if (selectedCells.empty() || ! onPreviewCell) return;

        int idx = selectedCells[0];
        if (idx < 0 || idx >= static_cast<int> (cells.size())) return;

        // Toggle preview state
        if (previewingCellIndex == idx)
        {
            // Stop preview
            onPreviewCell (currentChannel, previewingCellIndex, false);
            previewingCellIndex = -1;
            previewButton.setButtonText (LOC ("sampler.cell.preview"));
        }
        else
        {
            // Stop any previous preview
            if (previewingCellIndex >= 0)
                onPreviewCell (currentChannel, previewingCellIndex, false);

            // Start new preview
            previewingCellIndex = idx;
            onPreviewCell (currentChannel, idx, true);
            previewButton.setButtonText (LOC ("sampler.cell.previewStop"));
            startPlaybackMonitor();
        }
    }

    void stopPreview()
    {
        if (previewingCellIndex >= 0 && onPreviewCell)
        {
            onPreviewCell (currentChannel, previewingCellIndex, false);
            previewingCellIndex = -1;
            previewButton.setButtonText (LOC ("sampler.cell.preview"));
        }
    }

    void onClearCell()
    {
        if (selectedCells.empty()) return;

        for (int idx : selectedCells)
        {
            if (idx < 0 || idx >= static_cast<int> (cells.size())) continue;

            auto& cell = cells[static_cast<size_t> (idx)];
            cell = SamplerData::SampleCell();  // Reset to defaults

            // Clear in ValueTree
            saveCellProperty (idx, WFSParameterIDs::samplerCellName, juce::String());
            saveCellProperty (idx, WFSParameterIDs::samplerCellFile, juce::String());
            saveCellProperty (idx, WFSParameterIDs::samplerCellInTime, WFSParameterDefaults::samplerCellInTimeDefault);
            saveCellProperty (idx, WFSParameterIDs::samplerCellOutTime, WFSParameterDefaults::samplerCellOutTimeDefault);
            saveCellProperty (idx, WFSParameterIDs::samplerCellOffsetX, WFSParameterDefaults::samplerCellOffsetDefault);
            saveCellProperty (idx, WFSParameterIDs::samplerCellOffsetY, WFSParameterDefaults::samplerCellOffsetDefault);
            saveCellProperty (idx, WFSParameterIDs::samplerCellOffsetZ, WFSParameterDefaults::samplerCellOffsetDefault);
            saveCellProperty (idx, WFSParameterIDs::samplerCellAttenuation, WFSParameterDefaults::samplerCellAttenuationDefault);
        }

        updateCellPropertyPanel();
        notifyDataChanged();
        repaint();
    }

    // ==================== SET CALLBACKS ====================

    void onSetSelectorChanged()
    {
        if (isLoadingData) return;
        int newIdx = setSelector.getSelectedId() - 1;
        if (newIdx >= 0 && newIdx < static_cast<int> (sets.size()))
        {
            activeSetIndex = newIdx;
            if (samplerTree.isValid())
                samplerTree.setProperty (WFSParameterIDs::inputSamplerActiveSet, newIdx, nullptr);
            updateSetPropertyPanel();
            notifyDataChanged();
            repaint();
        }
    }

    void onAddSet()
    {
        SamplerData::SamplerSet newSet;
        newSet.name = LOC ("sampler.set.default") + " " + juce::String (static_cast<int> (sets.size()) + 1);

        // If cells are selected, assign them to the new set; otherwise start empty
        if (! selectedCells.empty())
            newSet.cellIndices.assign (selectedCells.begin(), selectedCells.end());

        sets.push_back (newSet);
        activeSetIndex = static_cast<int> (sets.size()) - 1;

        // Clear grid selection so it's obvious this is a fresh set
        selectedCells.clear();

        // Save to ValueTree
        saveSetToValueTree (activeSetIndex);
        loadFromValueTree();
        notifyDataChanged();
    }

    void onDeleteSet()
    {
        if (activeSetIndex < 0 || activeSetIndex >= static_cast<int> (sets.size()))
            return;

        // Remove from ValueTree
        if (samplerTree.isValid())
        {
            int setCount = 0;
            for (int i = 0; i < samplerTree.getNumChildren(); ++i)
            {
                if (samplerTree.getChild (i).hasType (WFSParameterIDs::SamplerSet))
                {
                    if (setCount == activeSetIndex)
                    {
                        samplerTree.removeChild (i, nullptr);
                        break;
                    }
                    setCount++;
                }
            }
        }

        sets.erase (sets.begin() + activeSetIndex);
        if (activeSetIndex >= static_cast<int> (sets.size()))
            activeSetIndex = static_cast<int> (sets.size()) - 1;

        loadFromValueTree();
        notifyDataChanged();
    }

    void onRenameSet()
    {
        if (activeSetIndex < 0) return;
        setNameEditor.setVisible (true);
        setNameEditor.setText (sets[static_cast<size_t> (activeSetIndex)].name);
        setNameEditor.grabKeyboardFocus();
    }

    void onSetNameChanged()
    {
        if (activeSetIndex < 0 || activeSetIndex >= static_cast<int> (sets.size())) return;
        sets[static_cast<size_t> (activeSetIndex)].name = setNameEditor.getText();
        setNameEditor.setVisible (false);
        saveSetToValueTree (activeSetIndex);
        loadFromValueTree();
        notifyDataChanged();
    }

    void onTogglePlayMode()
    {
        if (isLoadingData || activeSetIndex < 0 || activeSetIndex >= static_cast<int> (sets.size())) return;
        auto& set = sets[static_cast<size_t> (activeSetIndex)];
        set.playMode = (set.playMode == 0) ? 1 : 0;
        saveCurrentSetProperty (WFSParameterIDs::samplerSetPlayMode, set.playMode);
        updateSetPropertyPanel();
    }

    void onSetParamChanged (const juce::Identifier& paramId, float value)
    {
        if (isLoadingData || activeSetIndex < 0 || activeSetIndex >= static_cast<int> (sets.size())) return;
        auto& set = sets[static_cast<size_t> (activeSetIndex)];

        if (paramId == WFSParameterIDs::samplerSetPosX)       set.posX = value;
        else if (paramId == WFSParameterIDs::samplerSetPosY)  set.posY = value;
        else if (paramId == WFSParameterIDs::samplerSetPosZ)  set.posZ = value;
        else if (paramId == WFSParameterIDs::samplerSetLevel) set.level = value;

        saveCurrentSetProperty (paramId, value);
    }

    // ==================== LABEL EDITING ====================

    void labelTextChanged (juce::Label* label) override
    {
        if (isLoadingData) return;
        float value = label->getText().retainCharacters ("-0123456789.").getFloatValue();

        auto handleCurveLabel = [&] (juce::Label& lbl, WfsBidirectionalSlider& slider,
                                      const juce::Identifier& curveId)
        {
            float curve = juce::jlimit (0.0f, 1.0f, value);
            slider.setValue (curve * 2.0f - 1.0f);
            lbl.setText (juce::String (curve, 2), juce::dontSendNotification);
            saveCurrentSetProperty (curveId, curve);
        };

        if (label == &pressLevelCurveValueLabel)
            handleCurveLabel (pressLevelCurveValueLabel, pressLevelCurveSlider,
                              WFSParameterIDs::samplerSetPressLevelCurve);
        else if (label == &pressZCurveValueLabel)
            handleCurveLabel (pressZCurveValueLabel, pressZCurveSlider,
                              WFSParameterIDs::samplerSetPressZCurve);
        else if (label == &pressHFCurveValueLabel)
            handleCurveLabel (pressHFCurveValueLabel, pressHFCurveSlider,
                              WFSParameterIDs::samplerSetPressHFCurve);
        else if (label == &pressXYScaleValueLabel)
        {
            float cm = juce::jlimit (WFSParameterDefaults::samplerSetPressXYScaleMin * 100.0f,
                                     WFSParameterDefaults::samplerSetPressXYScaleMax * 100.0f, value);
            float meters = cm / 100.0f;
            if (pressXYScaleSlider) pressXYScaleSlider->setValue (meters);
            pressXYScaleValueLabel.setText (juce::String (cm, 1), juce::dontSendNotification);
            saveCurrentSetProperty (WFSParameterIDs::samplerSetPressXYScale, meters);
        }
    }

    // ==================== PRESSURE CALLBACKS ====================

    void setupPressureRow (juce::ToggleButton& enable, juce::TextButton& dirBtn,
                           WfsBidirectionalSlider& curveSlider, juce::Label& valueLabel,
                           const juce::Identifier& enableId,
                           const juce::Identifier& dirId,
                           const juce::Identifier& curveId,
                           const juce::String& label,
                           bool defaultEnabled)
    {
        enable.setButtonText (label);
        enable.setToggleState (defaultEnabled, juce::dontSendNotification);
        enable.onClick = [this, &enable, &dirBtn, &curveSlider, &valueLabel, enableId]
        {
            if (! isLoadingData)
                saveCurrentSetProperty (enableId, enable.getToggleState() ? 1 : 0);
            float alpha = enable.getToggleState() ? 1.0f : 0.4f;
            dirBtn.setAlpha (alpha);
            curveSlider.setAlpha (alpha);
            valueLabel.setAlpha (alpha);
        };
        addAndMakeVisible (enable);

        dirBtn.setButtonText ("+");
        dirBtn.onClick = [this, &dirBtn, dirId]
        {
            if (isLoadingData) return;
            bool isPositive = dirBtn.getButtonText() == "+";
            dirBtn.setButtonText (isPositive ? "-" : "+");
            saveCurrentSetProperty (dirId, isPositive ? 1 : 0);
        };
        addAndMakeVisible (dirBtn);

        curveSlider.onValueChanged = [this, &valueLabel, curveId] (float v)
        {
            if (! isLoadingData)
            {
                float curve = (v + 1.0f) / 2.0f;
                valueLabel.setText (juce::String (curve, 2), juce::dontSendNotification);
                saveCurrentSetProperty (curveId, curve);
            }
        };
        curveSlider.setValue (0.0f);  // Center = 0.5 stored = linear default
        addAndMakeVisible (curveSlider);

        valueLabel.setText ("0.50", juce::dontSendNotification);
        valueLabel.setJustificationType (juce::Justification::centred);
        valueLabel.setEditable (true, false);
        valueLabel.addListener (this);
        addAndMakeVisible (valueLabel);

        // Apply initial dimming based on default enabled state
        float alpha = defaultEnabled ? 1.0f : 0.4f;
        dirBtn.setAlpha (alpha);
        curveSlider.setAlpha (alpha);
        valueLabel.setAlpha (alpha);
    }

    // ==================== COPY / PASTE ====================

    static inline SamplerData::SampleCell clipboardCell;
    static inline SamplerData::SamplerSet clipboardSet;
    static inline bool hasClipboardCell = false;
    static inline bool hasClipboardSet = false;

    void onCopy()
    {
        if (! selectedCells.empty())
        {
            int idx = selectedCells[0];
            if (idx >= 0 && idx < static_cast<int> (cells.size()))
            {
                clipboardCell = cells[static_cast<size_t> (idx)];
                hasClipboardCell = true;
                hasClipboardSet = false;
            }
        }
        else if (activeSetIndex >= 0 && activeSetIndex < static_cast<int> (sets.size()))
        {
            clipboardSet = sets[static_cast<size_t> (activeSetIndex)];
            hasClipboardSet = true;
            hasClipboardCell = false;
        }
        updateCopyPasteButtons();
    }

    void onPaste()
    {
        if (hasClipboardCell && ! selectedCells.empty())
        {
            for (int idx : selectedCells)
            {
                if (idx < 0 || idx >= static_cast<int> (cells.size())) continue;

                auto& cell = cells[static_cast<size_t> (idx)];
                // Copy properties but keep audio unloaded (file path copied)
                cell.name = clipboardCell.name;
                cell.relativeFilePath = clipboardCell.relativeFilePath;
                cell.inTime = clipboardCell.inTime;
                cell.outTime = clipboardCell.outTime;
                cell.offsetX = clipboardCell.offsetX;
                cell.offsetY = clipboardCell.offsetY;
                cell.offsetZ = clipboardCell.offsetZ;
                cell.attenuation = clipboardCell.attenuation;

                saveCellToValueTree (idx);
            }
            updateCellPropertyPanel();
            notifyDataChanged();
            repaint();
        }
        else if (hasClipboardSet)
        {
            auto newSet = clipboardSet;
            newSet.name = newSet.name + " " + LOC ("sampler.set.copy");
            sets.push_back (newSet);
            activeSetIndex = static_cast<int> (sets.size()) - 1;
            saveSetToValueTree (activeSetIndex);
            loadFromValueTree();
            notifyDataChanged();
        }
    }

    void updateCopyPasteButtons()
    {
        bool cellMode = ! selectedCells.empty();
        copyButton.setButtonText (cellMode ? LOC ("sampler.buttons.copyCell")
                                           : LOC ("sampler.buttons.copySet"));
        pasteButton.setEnabled (cellMode ? hasClipboardCell : hasClipboardSet);
        pasteButton.setButtonText (hasClipboardCell ? LOC ("sampler.buttons.pasteCell")
                                                    : LOC ("sampler.buttons.pasteSet"));
    }

    // ==================== IMPORT / EXPORT ====================

    void onImport()
    {
        auto lastFolder = AppSettings::getLastFolder ("lastSampleFolder");
        fileChooser = std::make_unique<juce::FileChooser> (
            LOC ("sampler.importTitle"), lastFolder, "*.xml");

        fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                auto result = fc.getResult();
                if (! result.existsAsFile()) return;

                AppSettings::setLastFolder ("lastSampleFolder", result.getParentDirectory());

                auto xml = juce::XmlDocument::parse (result);
                if (xml == nullptr) return;

                auto importTree = juce::ValueTree::fromXml (*xml);
                if (! importTree.isValid() || ! importTree.hasType (WFSParameterIDs::Sampler))
                    return;

                // Replace current sampler tree
                if (samplerTree.isValid())
                {
                    auto parent = samplerTree.getParent();
                    int idx = parent.indexOf (samplerTree);
                    parent.removeChild (idx, nullptr);
                    parent.addChild (importTree, idx, nullptr);
                }

                loadFromValueTree();
                notifyDataChanged();
            });
    }

    void onExport()
    {
        if (! samplerTree.isValid()) return;

        auto lastFolder = AppSettings::getLastFolder ("lastSampleFolder");
        fileChooser = std::make_unique<juce::FileChooser> (
            LOC ("sampler.exportTitle"), lastFolder.getChildFile ("sampler_config.xml"), "*.xml");

        fileChooser->launchAsync (juce::FileBrowserComponent::saveMode,
            [this] (const juce::FileChooser& fc)
            {
                auto result = fc.getResult();
                if (result == juce::File()) return;

                AppSettings::setLastFolder ("lastSampleFolder", result.getParentDirectory());

                auto xml = samplerTree.createXml();
                if (xml != nullptr)
                    xml->writeTo (result);
            });
    }

    // ==================== VALUETREE SAVE HELPERS ====================

    void saveCellProperty (int cellIndex, const juce::Identifier& propId, const juce::var& value)
    {
        if (! samplerTree.isValid()) return;

        for (int i = 0; i < samplerTree.getNumChildren(); ++i)
        {
            auto child = samplerTree.getChild (i);
            if (child.hasType (WFSParameterIDs::SamplerCell)
                && static_cast<int> (child.getProperty ("id", -1)) == cellIndex)
            {
                child.setProperty (propId, value, nullptr);
                return;
            }
        }
    }

    void saveCellToValueTree (int cellIndex)
    {
        if (cellIndex < 0 || cellIndex >= static_cast<int> (cells.size())) return;
        auto& cell = cells[static_cast<size_t> (cellIndex)];

        saveCellProperty (cellIndex, WFSParameterIDs::samplerCellName, cell.name);
        saveCellProperty (cellIndex, WFSParameterIDs::samplerCellFile, cell.relativeFilePath);
        saveCellProperty (cellIndex, WFSParameterIDs::samplerCellInTime, cell.inTime);
        saveCellProperty (cellIndex, WFSParameterIDs::samplerCellOutTime, cell.outTime);
        saveCellProperty (cellIndex, WFSParameterIDs::samplerCellOffsetX, cell.offsetX);
        saveCellProperty (cellIndex, WFSParameterIDs::samplerCellOffsetY, cell.offsetY);
        saveCellProperty (cellIndex, WFSParameterIDs::samplerCellOffsetZ, cell.offsetZ);
        saveCellProperty (cellIndex, WFSParameterIDs::samplerCellAttenuation, cell.attenuation);
    }

    void saveCurrentSetProperty (const juce::Identifier& propId, const juce::var& value)
    {
        if (! samplerTree.isValid() || activeSetIndex < 0) return;

        int setCount = 0;
        for (int i = 0; i < samplerTree.getNumChildren(); ++i)
        {
            auto child = samplerTree.getChild (i);
            if (child.hasType (WFSParameterIDs::SamplerSet))
            {
                if (setCount == activeSetIndex)
                {
                    child.setProperty (propId, value, nullptr);
                    notifyDataChanged();
                    return;
                }
                setCount++;
            }
        }
    }

    void saveSetToValueTree (int setIndex)
    {
        if (! samplerTree.isValid() || setIndex < 0 || setIndex >= static_cast<int> (sets.size()))
            return;

        auto& set = sets[static_cast<size_t> (setIndex)];

        // Build a ValueTree for this set
        juce::ValueTree setTree (WFSParameterIDs::SamplerSet);
        setTree.setProperty ("id", setIndex, nullptr);
        setTree.setProperty (WFSParameterIDs::samplerSetName, set.name, nullptr);
        setTree.setProperty (WFSParameterIDs::samplerSetPlayMode, set.playMode, nullptr);
        setTree.setProperty (WFSParameterIDs::samplerSetCells,
                             SamplerData::SamplerSet::cellIndicesToString (set.cellIndices), nullptr);
        setTree.setProperty (WFSParameterIDs::samplerSetPosX, set.posX, nullptr);
        setTree.setProperty (WFSParameterIDs::samplerSetPosY, set.posY, nullptr);
        setTree.setProperty (WFSParameterIDs::samplerSetPosZ, set.posZ, nullptr);
        setTree.setProperty (WFSParameterIDs::samplerSetLevel, set.level, nullptr);

        setTree.setProperty (WFSParameterIDs::samplerSetPressLevelEnabled, set.pressLevel.enabled ? 1 : 0, nullptr);
        setTree.setProperty (WFSParameterIDs::samplerSetPressLevelDir, set.pressLevel.direction, nullptr);
        setTree.setProperty (WFSParameterIDs::samplerSetPressLevelCurve, set.pressLevel.curve, nullptr);

        setTree.setProperty (WFSParameterIDs::samplerSetPressZEnabled, set.pressZ.enabled ? 1 : 0, nullptr);
        setTree.setProperty (WFSParameterIDs::samplerSetPressZDir, set.pressZ.direction, nullptr);
        setTree.setProperty (WFSParameterIDs::samplerSetPressZCurve, set.pressZ.curve, nullptr);

        setTree.setProperty (WFSParameterIDs::samplerSetPressHFEnabled, set.pressHF.enabled ? 1 : 0, nullptr);
        setTree.setProperty (WFSParameterIDs::samplerSetPressHFDir, set.pressHF.direction, nullptr);
        setTree.setProperty (WFSParameterIDs::samplerSetPressHFCurve, set.pressHF.curve, nullptr);

        setTree.setProperty (WFSParameterIDs::samplerSetPressXYEnabled, set.pressXYEnabled ? 1 : 0, nullptr);
        setTree.setProperty (WFSParameterIDs::samplerSetPressXYScale, set.pressXYScale, nullptr);

        // Find and replace existing set, or append
        int setCount = 0;
        for (int i = 0; i < samplerTree.getNumChildren(); ++i)
        {
            auto child = samplerTree.getChild (i);
            if (child.hasType (WFSParameterIDs::SamplerSet))
            {
                if (setCount == setIndex)
                {
                    samplerTree.removeChild (i, nullptr);
                    samplerTree.addChild (setTree, i, nullptr);
                    return;
                }
                setCount++;
            }
        }

        // Not found — append
        samplerTree.appendChild (setTree, nullptr);
    }

    void notifyDataChanged()
    {
        if (onSamplerDataChanged)
            onSamplerDataChanged();
    }

    // ==================== MEMBER DATA ====================

    WfsParameters& parameters;
    int currentChannel = -1;
    bool isLoadingData = false;

    // ValueTree reference
    juce::ValueTree samplerTree;

    // Local data copies
    std::vector<SamplerData::SampleCell> cells;
    std::vector<SamplerData::SamplerSet> sets;

    // Selection
    std::vector<int> selectedCells;
    int activeSetIndex = 0;
    int previewingCellIndex = -1;
    int lastPlayingCell = -1;
    juce::int64 playEndTime = 0;

    HelpCardButton samplerHelpButton;
    HelpCard samplerHelpCard;

    // Layout scaling
    float layoutScale = 1.0f;
    int scaled (int ref) const { return juce::jmax (static_cast<int> (ref * 0.65f), static_cast<int> (ref * layoutScale)); }

    // Layout areas
    juce::Rectangle<int> gridArea;
    juce::Rectangle<int> panelArea;
    std::vector<int> sectionSeparatorYs;
    std::vector<std::pair<juce::String, int>> sectionLabels;

    struct ControlLabel { juce::String text; int x, y, w, h; };
    std::vector<ControlLabel> controlLabels;

    // ── Cell property controls ──
    juce::TextEditor cellNameEditor;
    juce::TextButton loadCellButton;
    LongPressButton clearCellButton;
    juce::TextButton previewButton;
    std::unique_ptr<WfsRangeSlider> inOutRangeSlider;
    juce::TextEditor cellOffsetEditors[3];  // X, Y, Z
    std::unique_ptr<WfsStandardSlider> cellAttenSlider;

    // ── Set management controls ──
    juce::ComboBox setSelector;
    juce::TextButton addSetButton;
    LongPressButton deleteSetButton;
    juce::TextButton renameSetButton;
    juce::TextEditor setNameEditor;
    juce::TextButton playModeButton;
    juce::TextButton qlabSetButton;

    // ── Set position/level ──
    juce::TextEditor setPosEditors[3];  // X, Y, Z
    std::unique_ptr<WfsStandardSlider> setLevelSlider;

    // ── Pressure mapping controls ──
    juce::ToggleButton pressLevelEnable;
    juce::TextButton pressLevelDirBtn;
    WfsBidirectionalSlider pressLevelCurveSlider;
    juce::Label pressLevelCurveValueLabel;

    juce::ToggleButton pressZEnable;
    juce::TextButton pressZDirBtn;
    WfsBidirectionalSlider pressZCurveSlider;
    juce::Label pressZCurveValueLabel;

    juce::ToggleButton pressHFEnable;
    juce::TextButton pressHFDirBtn;
    WfsBidirectionalSlider pressHFCurveSlider;
    juce::Label pressHFCurveValueLabel;

    juce::ToggleButton pressXYEnable;
    std::unique_ptr<WfsStandardSlider> pressXYScaleSlider;
    juce::Label pressXYScaleValueLabel;

    // ── Copy/Paste & Import/Export ──
    juce::TextButton copyButton;
    juce::TextButton pasteButton;
    juce::TextButton importButton;
    juce::TextButton exportButton;

    // ── Zone Selector (Lightpad or Remote) ──
    juce::TextButton lightpadZoneButton;
    int currentLightpadZoneId = -1;  // zone assigned to current input
    int controllerMode = 0;          // 0=Off, 1=Lightpad, 2=Remote
    int remotePadCols = 3;
    int remotePadRows = 2;

    // File chooser (must persist during async operation)
    std::unique_ptr<juce::FileChooser> fileChooser;

public:
    // Lightpad zone query/callbacks — wired from MainComponent via InputsTab
    struct LightpadZoneQuery
    {
        std::function<std::vector<std::pair<int, juce::String>>()> getAllZones;
        std::function<std::set<int>()> getAssignedZoneIds;
        std::function<std::map<int, int>()> getAssignedZones;  // zoneId → inputIndex
        std::function<void (bool)> showZoneNumbers;
        std::function<std::vector<PadLayoutInfo>()> getPadLayouts;
    };

    LightpadZoneQuery lightpadZoneQuery;
    std::function<void (int inputIndex, int zoneId)> onLightpadZoneChanged;

    void setLightpadEnabled (bool enabled)
    {
        // Legacy method — now use setControllerMode instead
        setControllerMode (enabled ? 1 : 0);
    }

    /** Set the active controller mode: 0=Off, 1=Lightpad, 2=Remote */
    void setControllerMode (int mode)
    {
        controllerMode = mode;
        lightpadZoneButton.setVisible (mode == 1 || mode == 2);
        if (mode == 2)
            updateRemoteZoneButtonText();
        else if (mode == 1)
            updateLightpadZoneButtonText();
        resized();
    }

    /** Set remote pad grid size for zone picker */
    void setRemotePadGridSize (int cols, int rows)
    {
        remotePadCols = cols;
        remotePadRows = rows;
    }

    void updateLightpadZoneButtonText()
    {
        if (currentLightpadZoneId < 0)
        {
            lightpadZoneButton.setButtonText (LOC ("sampler.lightpadZone.none"));
            lightpadZoneButton.setColour (juce::TextButton::buttonColourId,
                                           juce::Colour (0xFF3A3A3A));
        }
        else
        {
            // Get zone display name and pad colour
            auto info = decodeZoneId (currentLightpadZoneId);
            bool split[3] = { false, false, false };

            // Try to get split state from pad layouts
            if (lightpadZoneQuery.getPadLayouts)
            {
                auto pads = lightpadZoneQuery.getPadLayouts();
                for (auto& pl : pads)
                    if (pl.padIndex >= 0 && pl.padIndex < 3)
                        split[pl.padIndex] = pl.isSplit;
            }

            lightpadZoneButton.setButtonText (
                getZoneDisplayName (currentLightpadZoneId, split));
            lightpadZoneButton.setColour (juce::TextButton::buttonColourId,
                                           LightpadColours::getPadColour (info.padIndex).darker (0.3f));
        }
    }

    void updateRemoteZoneButtonText()
    {
        if (currentLightpadZoneId < 0)
        {
            lightpadZoneButton.setButtonText (LOC ("sampler.lightpadZone.none"));
            lightpadZoneButton.setColour (juce::TextButton::buttonColourId,
                                           juce::Colour (0xFF3A3A3A));
        }
        else
        {
            juce::String padName = LOC ("sampler.zone.remotePad")
                .replace ("{num}", juce::String (currentLightpadZoneId + 1));
            float hue = std::fmod (static_cast<float> (currentChannel) * 0.13f, 1.0f);
            auto colour = juce::Colour::fromHSV (hue, 0.7f, 0.9f, 1.0f);
            lightpadZoneButton.setButtonText (padName);
            lightpadZoneButton.setColour (juce::TextButton::buttonColourId, colour.darker (0.3f));
        }
    }

    void showRemotePadZoneOverlay()
    {
        auto* parent = getTopLevelComponent();
        if (parent == nullptr) return;

        int cols = remotePadCols;
        int rows = remotePadRows;

        // Get assigned zones
        std::map<int, int> assignedMap;  // zoneId → inputChannel
        if (lightpadZoneQuery.getAssignedZones)
            assignedMap = lightpadZoneQuery.getAssignedZones();

        class RemotePadOverlay : public juce::Component
        {
        public:
            RemotePadOverlay (int cols_, int rows_, int currentZone, int currentCh,
                              const std::map<int, int>& assigned,
                              std::function<void (int)> onSelect,
                              std::function<void()> onDismiss)
                : gridCols (cols_), gridRows (rows_), selectedZone (currentZone),
                  currentChannel (currentCh), assignedZones (assigned),
                  selectCallback (std::move (onSelect)), dismissCallback (std::move (onDismiss))
            {
            }

            void paint (juce::Graphics& g) override
            {
                g.fillAll (juce::Colour (0xCC000000));

                int gridW = gridCols * 60 + (gridCols - 1) * 4 + 40;
                int gridH = gridRows * 50 + (gridRows - 1) * 4 + 60;
                gridArea = getLocalBounds().withSizeKeepingCentre (gridW, gridH);

                g.setColour (juce::Colour (0xFF2A2A2A));
                g.fillRoundedRectangle (gridArea.toFloat(), 8.0f);
                g.setColour (juce::Colour (0xFF555555));
                g.drawRoundedRectangle (gridArea.toFloat(), 8.0f, 1.0f);

                g.setColour (juce::Colours::white);
                g.setFont (juce::FontOptions (13.0f));
                g.drawText (LOC ("sampler.zone.selectRemotePad"),
                            gridArea.removeFromTop (24), juce::Justification::centred);

                auto content = gridArea.reduced (16, 8);
                int pad = 4;
                float cellW = static_cast<float> (content.getWidth() - (gridCols - 1) * pad) / gridCols;
                float cellH = static_cast<float> (content.getHeight() - (gridRows - 1) * pad) / gridRows;

                cellBounds.clear();
                for (int r = 0; r < gridRows; ++r)
                {
                    for (int c = 0; c < gridCols; ++c)
                    {
                        int zoneId = r * gridCols + c;
                        float cx = content.getX() + c * (cellW + pad);
                        float cy = content.getY() + r * (cellH + pad);
                        auto cell = juce::Rectangle<float> (cx, cy, cellW, cellH);
                        cellBounds.push_back ({ zoneId, cell });

                        bool isMine = (zoneId == selectedZone);
                        auto it = assignedZones.find (zoneId);
                        bool isAssignedOther = (it != assignedZones.end() && ! isMine);
                        int assignedInput = (it != assignedZones.end()) ? it->second : -1;

                        // Background
                        if (isMine)
                        {
                            float hue = std::fmod (static_cast<float> (currentChannel) * 0.13f, 1.0f);
                            g.setColour (juce::Colour::fromHSV (hue, 0.7f, 0.9f, 1.0f).withAlpha (0.6f));
                        }
                        else if (isAssignedOther)
                        {
                            float hue = std::fmod (static_cast<float> (assignedInput) * 0.13f, 1.0f);
                            g.setColour (juce::Colour::fromHSV (hue, 0.4f, 0.4f, 0.4f));
                        }
                        else
                            g.setColour (juce::Colour (0xFF444444));

                        g.fillRoundedRectangle (cell, 3.0f);

                        // Border
                        g.setColour (isMine ? juce::Colours::white : juce::Colour (0xFF666666));
                        g.drawRoundedRectangle (cell, 3.0f, isMine ? 2.0f : 1.0f);

                        // Number
                        g.setColour (isAssignedOther ? juce::Colours::white.withAlpha (0.3f) : juce::Colours::white);
                        g.setFont (juce::FontOptions (juce::jmin (cellW * 0.35f, 14.0f)));
                        g.drawText (juce::String (zoneId + 1), cell.reduced (2),
                                    juce::Justification::centred);

                        // Input label
                        if (assignedInput >= 0)
                        {
                            g.setFont (juce::FontOptions (juce::jmin (cellW * 0.2f, 9.0f)));
                            g.drawText ("In " + juce::String (assignedInput + 1),
                                        cell.reduced (2).removeFromBottom (cellH * 0.3f),
                                        juce::Justification::centred);
                        }
                    }
                }
            }

            void mouseDown (const juce::MouseEvent& e) override
            {
                for (auto& [zoneId, rect] : cellBounds)
                {
                    if (rect.contains (e.getPosition().toFloat()))
                    {
                        // Toggle: if already selected, unassign
                        int newZone = (zoneId == selectedZone) ? -1 : zoneId;
                        if (selectCallback) selectCallback (newZone);
                        if (dismissCallback) dismissCallback();
                        return;
                    }
                }
                // Click outside grid → dismiss
                if (dismissCallback) dismissCallback();
            }

        private:
            int gridCols, gridRows, selectedZone, currentChannel;
            std::map<int, int> assignedZones;
            std::function<void (int)> selectCallback;
            std::function<void()> dismissCallback;
            juce::Rectangle<int> gridArea;
            std::vector<std::pair<int, juce::Rectangle<float>>> cellBounds;
        };

        juce::Component::SafePointer<SamplerSubTab> safeThis (this);

        auto removeOverlay = [parent] ()
        {
            for (int i = parent->getNumChildComponents() - 1; i >= 0; --i)
            {
                auto* child = parent->getChildComponent (i);
                if (dynamic_cast<RemotePadOverlay*> (child))
                {
                    parent->removeChildComponent (child);
                    delete child;
                    break;
                }
            }
        };

        auto* overlay = new RemotePadOverlay (cols, rows, currentLightpadZoneId, currentChannel,
            assignedMap,
            [safeThis] (int selectedZoneId)
            {
                if (! safeThis) return;
                safeThis->currentLightpadZoneId = selectedZoneId;
                safeThis->updateRemoteZoneButtonText();
                safeThis->saveInputParam (WFSParameterIDs::lightpadZoneId, selectedZoneId);
                if (safeThis->onLightpadZoneChanged)
                    safeThis->onLightpadZoneChanged (safeThis->currentChannel, selectedZoneId);
            },
            removeOverlay);

        overlay->setBounds (parent->getLocalBounds());
        parent->addAndMakeVisible (overlay);
    }

    void showLightpadZoneOverlay()
    {
        auto* parent = getTopLevelComponent();
        if (parent == nullptr) return;

        // Gather data from queries
        auto allZones = lightpadZoneQuery.getAllZones
                        ? lightpadZoneQuery.getAllZones()
                        : std::vector<std::pair<int, juce::String>>{};

        auto padLayouts = lightpadZoneQuery.getPadLayouts
                          ? lightpadZoneQuery.getPadLayouts()
                          : std::vector<PadLayoutInfo>{};

        // Build assigned map: zoneId → inputIndex
        std::map<int, int> assignedMap;
        if (lightpadZoneQuery.getAssignedZones)
            assignedMap = lightpadZoneQuery.getAssignedZones();

        int inputIdx = currentChannel;

        juce::Component::SafePointer<juce::Component> safeParent = parent;
        juce::Component::SafePointer<SamplerSubTab> safeThis = this;

        auto removeOverlay = [safeParent, safeThis] (int selectedZoneId)
        {
            juce::MessageManager::callAsync ([safeThis, safeParent, selectedZoneId]()
            {
                if (safeParent == nullptr) return;

                auto* p = safeParent.getComponent();
                for (int i = p->getNumChildComponents() - 1; i >= 0; --i)
                {
                    auto* child = p->getChildComponent (i);
                    if (dynamic_cast<LightpadZoneOverlay*> (child) ||
                        dynamic_cast<LightpadZoneBackdrop*> (child))
                    {
                        p->removeChildComponent (child);
                        delete child;
                    }
                }

                if (safeThis != nullptr)
                {
                    safeThis->currentLightpadZoneId = selectedZoneId;
                    safeThis->updateLightpadZoneButtonText();

                    // Save and notify
                    safeThis->saveInputParam (WFSParameterIDs::lightpadZoneId, selectedZoneId);
                    if (safeThis->onLightpadZoneChanged)
                        safeThis->onLightpadZoneChanged (safeThis->currentChannel, selectedZoneId);
                }
            });
        };

        // Backdrop
        auto backdrop = std::make_unique<LightpadZoneBackdrop> (
            [removeOverlay, cur = currentLightpadZoneId]() { removeOverlay (cur); });
        backdrop->setBounds (parent->getLocalBounds());
        parent->addAndMakeVisible (backdrop.release());

        // Overlay
        auto overlay = std::make_unique<LightpadZoneOverlay> (
            padLayouts, allZones, assignedMap, inputIdx,
            currentLightpadZoneId, removeOverlay);

        auto requiredSize = overlay->getRequiredSize();
        auto btnBounds = parent->getLocalArea (&lightpadZoneButton,
                                                lightpadZoneButton.getLocalBounds());

        int popupX = btnBounds.getX();
        int popupY = btnBounds.getBottom() + 4;

        auto parentBounds = parent->getLocalBounds();
        if (popupX + requiredSize.x > parentBounds.getRight())
            popupX = parentBounds.getRight() - requiredSize.x;
        if (popupX < 0) popupX = 0;
        if (popupY + requiredSize.y > parentBounds.getBottom())
            popupY = btnBounds.getY() - requiredSize.y - 4;
        if (popupY < 0) popupY = 0;

        overlay->setBounds (popupX, popupY, requiredSize.x, requiredSize.y);
        parent->addAndMakeVisible (overlay.release());
    }

private:
    void saveInputParam (const juce::Identifier& paramId, const juce::var& value)
    {
        if (currentChannel < 0) return;
        parameters.setInputParam (currentChannel, paramId.toString(), value);
    }

    // ==================== STATUS BAR HELP ====================

    StatusBar* statusBar = nullptr;
    std::unordered_map<juce::Component*, juce::String> helpTextMap;

    void setupHelpText()
    {
        helpTextMap.clear();

        helpTextMap[&loadCellButton]         = LOC ("sampler.tooltips.load");
        helpTextMap[&clearCellButton]        = LOC ("sampler.tooltips.clear");
        helpTextMap[&previewButton]          = LOC ("sampler.tooltips.preview");
        if (inOutRangeSlider)
            helpTextMap[inOutRangeSlider.get()] = LOC ("sampler.tooltips.inOut");
        for (int i = 0; i < 3; ++i)
            helpTextMap[&cellOffsetEditors[i]] = LOC ("sampler.tooltips.offset");
        if (cellAttenSlider)
            helpTextMap[cellAttenSlider.get()] = LOC ("sampler.tooltips.attenuation");

        helpTextMap[&addSetButton]           = LOC ("sampler.tooltips.addSet");
        helpTextMap[&deleteSetButton]        = LOC ("sampler.tooltips.deleteSet");
        helpTextMap[&renameSetButton]        = LOC ("sampler.tooltips.renameSet");
        helpTextMap[&playModeButton]         = LOC ("sampler.tooltips.playMode");
        for (int i = 0; i < 3; ++i)
            helpTextMap[&setPosEditors[i]]   = LOC ("sampler.tooltips.setPos");
        if (setLevelSlider)
            helpTextMap[setLevelSlider.get()] = LOC ("sampler.tooltips.setLevel");

        helpTextMap[&pressLevelEnable]           = LOC ("sampler.tooltips.pressLevel");
        helpTextMap[&pressLevelDirBtn]           = LOC ("sampler.tooltips.pressDir");
        helpTextMap[&pressLevelCurveSlider]      = LOC ("sampler.tooltips.pressCurve");
        helpTextMap[&pressLevelCurveValueLabel]  = LOC ("sampler.tooltips.pressCurve");
        helpTextMap[&pressZEnable]               = LOC ("sampler.tooltips.pressHeight");
        helpTextMap[&pressZDirBtn]               = LOC ("sampler.tooltips.pressDir");
        helpTextMap[&pressZCurveSlider]          = LOC ("sampler.tooltips.pressCurve");
        helpTextMap[&pressZCurveValueLabel]      = LOC ("sampler.tooltips.pressCurve");
        helpTextMap[&pressHFEnable]              = LOC ("sampler.tooltips.pressHF");
        helpTextMap[&pressHFDirBtn]              = LOC ("sampler.tooltips.pressDir");
        helpTextMap[&pressHFCurveSlider]         = LOC ("sampler.tooltips.pressCurve");
        helpTextMap[&pressHFCurveValueLabel]     = LOC ("sampler.tooltips.pressCurve");
        helpTextMap[&pressXYEnable]              = LOC ("sampler.tooltips.pressXY");
        if (pressXYScaleSlider)
            helpTextMap[pressXYScaleSlider.get()] = LOC ("sampler.tooltips.pressXYScale");
        helpTextMap[&pressXYScaleValueLabel]     = LOC ("sampler.tooltips.pressXYScale");

        helpTextMap[&copyButton]             = LOC ("sampler.tooltips.copy");
        helpTextMap[&pasteButton]            = LOC ("sampler.tooltips.paste");
        helpTextMap[&importButton]           = LOC ("sampler.tooltips.import");
        helpTextMap[&exportButton]           = LOC ("sampler.tooltips.export");
        helpTextMap[&samplerHelpButton]     = LOC ("help.sampler.title");

        for (auto& [comp, text] : helpTextMap)
            comp->addMouseListener (this, true);
    }

    void mouseEnter (const juce::MouseEvent& e) override
    {
        if (statusBar == nullptr) return;
        juce::Component* comp = e.eventComponent;
        while (comp != nullptr)
        {
            auto it = helpTextMap.find (comp);
            if (it != helpTextMap.end())
            {
                statusBar->setHelpText (it->second);
                return;
            }
            comp = comp->getParentComponent();
        }
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        if (statusBar != nullptr)
            statusBar->clearText();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerSubTab)
};
