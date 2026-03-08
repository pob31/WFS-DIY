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

/**
 * Sampler subtab for InputsTab.
 * Left 67%: 6x6 sample grid with cell selection.
 * Right 33%: parameter panel (cell props, set management, pressure mappings).
 */
class SamplerSubTab : public juce::Component,
                      public ColorScheme::Manager::Listener
{
public:
    /** Callback fired when sampler data changes (wired in MainComponent) */
    std::function<void()> onSamplerDataChanged;

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
        cellNameEditor.onReturnKey = [this] { onCellNameChanged(); };
        addChildComponent (cellNameEditor);

        loadCellButton.setButtonText (LOC ("sampler.cell.load"));
        loadCellButton.onClick = [this] { onLoadCell(); };
        addChildComponent (loadCellButton);

        clearCellButton.setButtonText (LOC ("sampler.cell.clear"));
        clearCellButton.onLongPress = [this] { onClearCell(); };
        addChildComponent (clearCellButton);

        previewButton.setButtonText (LOC ("sampler.cell.preview"));
        previewButton.onClick = [this] { /* Phase 7 */ };
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
                cellOffsetEditors[i].onReturnKey = [this, i] { onOffsetEditorChanged (i); };
                cellOffsetEditors[i].onFocusLost = [this, i] { onOffsetEditorChanged (i); };
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

        setNameEditor.setJustification (juce::Justification::centredLeft);
        setNameEditor.onReturnKey = [this] { onSetNameChanged(); };
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
                setPosEditors[i].onReturnKey = [this, i] { onSetPosEditorChanged (i); };
                setPosEditors[i].onFocusLost = [this, i] { onSetPosEditorChanged (i); };
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
        setupPressureRow (pressLevelEnable, pressLevelDirBtn, pressLevelCurveSlider,
                          WFSParameterIDs::samplerSetPressLevelEnabled,
                          WFSParameterIDs::samplerSetPressLevelDir,
                          WFSParameterIDs::samplerSetPressLevelCurve,
                          LOC ("sampler.press.level"));

        setupPressureRow (pressZEnable, pressZDirBtn, pressZCurveSlider,
                          WFSParameterIDs::samplerSetPressZEnabled,
                          WFSParameterIDs::samplerSetPressZDir,
                          WFSParameterIDs::samplerSetPressZCurve,
                          LOC ("sampler.press.height"));

        setupPressureRow (pressHFEnable, pressHFDirBtn, pressHFCurveSlider,
                          WFSParameterIDs::samplerSetPressHFEnabled,
                          WFSParameterIDs::samplerSetPressHFDir,
                          WFSParameterIDs::samplerSetPressHFCurve,
                          LOC ("sampler.press.hf"));

        pressXYEnable.setButtonText (LOC ("sampler.press.xy"));
        pressXYEnable.onClick = [this]
        {
            if (isLoadingData) return;
            saveCurrentSetProperty (WFSParameterIDs::samplerSetPressXYEnabled,
                                    pressXYEnable.getToggleState() ? 1 : 0);
        };
        addAndMakeVisible (pressXYEnable);

        pressXYScaleSlider = std::make_unique<WfsStandardSlider> (
            WFSParameterDefaults::samplerSetPressXYScaleMin,
            WFSParameterDefaults::samplerSetPressXYScaleMax);
        pressXYScaleSlider->setLabel (LOC ("sampler.press.xyScale"));
        pressXYScaleSlider->setInlineMode (true);
        pressXYScaleSlider->setValueToString ([] (float v) { return juce::String (v * 100.0f, 0); });
        pressXYScaleSlider->onValueChanged = [this] (float v)
        {
            if (! isLoadingData)
                saveCurrentSetProperty (WFSParameterIDs::samplerSetPressXYScale, v);
        };
        addAndMakeVisible (*pressXYScaleSlider);

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
    }

    ~SamplerSubTab() override
    {
        ColorScheme::Manager::getInstance().removeListener (this);
    }

    // ==================== LAYOUT ====================

    void resized() override
    {
        auto bounds = getLocalBounds();

        // Right panel: 33%
        int panelWidth = juce::jmax (220, bounds.getWidth() * 33 / 100);
        panelArea = bounds.removeFromRight (panelWidth);

        // Grid area: remaining 67%
        gridArea = bounds;

        layoutPanel();
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
        g.setFont (juce::FontOptions (11.0f));
        for (auto& [label, yPos] : sectionLabels)
            g.drawText (label, panelArea.getX() + 8, yPos, panelArea.getWidth() - 16, 16,
                        juce::Justification::centredLeft);

        // Control inline labels (e.g. "In/Out (ms)", "Offset (m)", "Pos (m)")
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.setFont (juce::FontOptions (10.0f));
        for (auto& cl : controlLabels)
            g.drawText (cl.text, cl.x, cl.y, cl.w, cl.h, juce::Justification::centredLeft);
    }

    // ==================== MOUSE HANDLING ====================

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! gridArea.contains (e.getPosition()))
            return;

        int cellIdx = hitTestGrid (e.getPosition());

        // Click on grid background → deselect all
        if (cellIdx < 0)
        {
            selectedCells.clear();
            updateCellPropertyPanel();
            updateCopyPasteButtons();
            repaint();
            return;
        }

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

    /** Set the current input channel (0-based) */
    void setCurrentChannel (int channel)
    {
        if (currentChannel != channel)
        {
            currentChannel = channel;
            selectedCells.clear();
            loadFromValueTree();
        }
    }

    int getCurrentChannel() const noexcept { return currentChannel; }

    /** Reload data from ValueTree (call when channel changes or external edit occurs) */
    void loadFromValueTree()
    {
        isLoadingData = true;

        samplerTree = parameters.getValueTreeState().getInputSamplerSection (currentChannel);

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
            setSelector.addItem (name, i + 1);
        }

        if (! sets.empty())
        {
            activeSetIndex = juce::jlimit (0, static_cast<int> (sets.size()) - 1, activeSetIndex);
            setSelector.setSelectedId (activeSetIndex + 1, juce::dontSendNotification);
        }
        else
        {
            activeSetIndex = -1;
        }

        updateSetPropertyPanel();
        updateCellPropertyPanel();
        updateCopyPasteButtons();

        isLoadingData = false;
        repaint();
    }

    // ColorScheme::Manager::Listener
    void colorSchemeChanged() override { repaint(); }

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

                // Cell background
                if (isSelected)
                    g.setColour (cs.accentBlue.withAlpha (0.4f));
                else if (isInSet)
                    g.setColour (cs.accentGreen.withAlpha (0.2f));
                else
                    g.setColour (cs.surfaceCard);

                g.fillRoundedRectangle (cellRect, 3.0f);

                // Cell border
                if (isSelected)
                    g.setColour (cs.accentBlue);
                else if (isInSet)
                    g.setColour (cs.accentGreen.withAlpha (0.6f));
                else
                    g.setColour (cs.chromeDivider);

                g.drawRoundedRectangle (cellRect, 3.0f, isSelected ? 2.0f : 1.0f);

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
                    g.setFont (juce::FontOptions (juce::jmin (cellSize * 0.2f, 11.0f)));
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

        // Help hint text at bottom of grid
        g.setColour (cs.textSecondary.withAlpha (0.35f));
        g.setFont (juce::FontOptions (10.0f));
        g.drawText (LOC ("sampler.grid.help"),
                    gridArea.getX() + 8, gridArea.getBottom() - 16,
                    gridArea.getWidth() - 16, 14,
                    juce::Justification::centredLeft);
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
        const int rowH = 26;
        const int pad = 3;
        const int sectionGap = 5;
        int x0 = panelArea.getX() + 6;
        int contentW = panelArea.getWidth() - 12;

        sectionSeparatorYs.clear();
        sectionLabels.clear();
        controlLabels.clear();

        int y = panelArea.getY() + 4;

        // ── Section: Cell Properties ──
        sectionLabels.push_back ({ LOC ("sampler.section.cell"), y });
        y += 16;

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
        y += 16;

        // Row 1: [selector] [+] [-]
        int selectorW = contentW - 2 * (24 + pad);
        setSelector.setBounds (x0, y, juce::jmax (60, selectorW), rowH);
        addSetButton.setBounds (x0 + juce::jmax (60, selectorW) + pad, y, 24, rowH);
        deleteSetButton.setBounds (x0 + juce::jmax (60, selectorW) + pad + 24 + pad, y, 24, rowH);
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
        y += 16;

        layoutPressureRow (y, pressLevelEnable, pressLevelDirBtn, pressLevelCurveSlider, x0, contentW, rowH, pad);
        layoutPressureRow (y, pressZEnable, pressZDirBtn, pressZCurveSlider, x0, contentW, rowH, pad);
        layoutPressureRow (y, pressHFEnable, pressHFDirBtn, pressHFCurveSlider, x0, contentW, rowH, pad);

        int halfW2 = (contentW - pad) / 2;
        pressXYEnable.setBounds (x0, y, halfW2, rowH);
        if (pressXYScaleSlider) pressXYScaleSlider->setBounds (x0 + halfW2 + pad, y, halfW2, rowH);
        y += rowH + pad;

        // ── Separator ──
        y += sectionGap;
        sectionSeparatorYs.push_back (y);
        y += sectionGap;

        // ── Section: Copy/Paste & Import/Export ──
        sectionLabels.push_back ({ LOC ("sampler.section.actions"), y });
        y += 16;

        int halfW = (contentW - pad) / 2;
        copyButton.setBounds (x0, y, halfW, rowH);
        pasteButton.setBounds (x0 + halfW + pad, y, halfW, rowH);
        y += rowH + pad;

        importButton.setBounds (x0, y, halfW, rowH);
        exportButton.setBounds (x0 + halfW + pad, y, halfW, rowH);
    }

    void layoutPressureRow (int& y, juce::ToggleButton& enable,
                            juce::TextButton& dirBtn, WfsStandardSlider& curveSlider,
                            int x0, int contentW, int rowH, int pad)
    {
        int enableW = contentW * 40 / 100;
        int dirW = 30;
        int sliderW = contentW - enableW - dirW - pad * 2;
        enable.setBounds (x0, y, enableW, rowH);
        dirBtn.setBounds (x0 + enableW + pad, y, dirW, rowH);
        curveSlider.setBounds (x0 + enableW + pad + dirW + pad, y, sliderW, rowH);
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
        pressLevelCurveSlider.setValue (set.pressLevel.curve);

        pressZEnable.setToggleState (set.pressZ.enabled, juce::dontSendNotification);
        pressZDirBtn.setButtonText (set.pressZ.direction == 0 ? "+" : "-");
        pressZCurveSlider.setValue (set.pressZ.curve);

        pressHFEnable.setToggleState (set.pressHF.enabled, juce::dontSendNotification);
        pressHFDirBtn.setButtonText (set.pressHF.direction == 0 ? "+" : "-");
        pressHFCurveSlider.setValue (set.pressHF.curve);

        pressXYEnable.setToggleState (set.pressXYEnabled, juce::dontSendNotification);
        if (pressXYScaleSlider) pressXYScaleSlider->setValue (set.pressXYScale);

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

        auto lastFolder = AppSettings::getLastFolder ("lastSampleFolder");
        fileChooser = std::make_unique<juce::FileChooser> (
            LOC ("sampler.cell.loadTitle"), lastFolder, "*.wav;*.aiff;*.flac;*.ogg");

        fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                auto result = fc.getResult();
                if (! result.existsAsFile()) return;

                AppSettings::setLastFolder ("lastSampleFolder", result.getParentDirectory());

                // Import to project samples/ folder
                auto& fm = parameters.getFileManager();
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

                // Apply to all selected cells
                for (int idx : selectedCells)
                {
                    if (idx < 0 || idx >= static_cast<int> (cells.size())) continue;

                    auto& cell = cells[static_cast<size_t> (idx)];
                    cell.relativeFilePath = relPath;
                    if (cell.name.isEmpty())
                        cell.name = result.getFileNameWithoutExtension();

                    saveCellProperty (idx, WFSParameterIDs::samplerCellFile, relPath);
                    saveCellProperty (idx, WFSParameterIDs::samplerCellName, cell.name);
                }

                updateCellPropertyPanel();
                notifyDataChanged();
                repaint();
            });
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
            updateSetPropertyPanel();
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

    // ==================== PRESSURE CALLBACKS ====================

    void setupPressureRow (juce::ToggleButton& enable, juce::TextButton& dirBtn,
                           WfsStandardSlider& curveSlider,
                           const juce::Identifier& enableId,
                           const juce::Identifier& dirId,
                           const juce::Identifier& curveId,
                           const juce::String& label)
    {
        enable.setButtonText (label);
        enable.onClick = [this, &enable, enableId]
        {
            if (! isLoadingData)
                saveCurrentSetProperty (enableId, enable.getToggleState() ? 1 : 0);
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

        curveSlider.onValueChanged = [this, curveId] (float v)
        {
            if (! isLoadingData)
                saveCurrentSetProperty (curveId, v);
        };
        addAndMakeVisible (curveSlider);
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
    int currentChannel = 0;
    bool isLoadingData = false;

    // ValueTree reference
    juce::ValueTree samplerTree;

    // Local data copies
    std::vector<SamplerData::SampleCell> cells;
    std::vector<SamplerData::SamplerSet> sets;

    // Selection
    std::vector<int> selectedCells;
    int activeSetIndex = 0;

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

    // ── Set position/level ──
    juce::TextEditor setPosEditors[3];  // X, Y, Z
    std::unique_ptr<WfsStandardSlider> setLevelSlider;

    // ── Pressure mapping controls ──
    juce::ToggleButton pressLevelEnable;
    juce::TextButton pressLevelDirBtn;
    WfsStandardSlider pressLevelCurveSlider { WFSParameterDefaults::samplerSetPressCurveMin,
                                               WFSParameterDefaults::samplerSetPressCurveMax };

    juce::ToggleButton pressZEnable;
    juce::TextButton pressZDirBtn;
    WfsStandardSlider pressZCurveSlider { WFSParameterDefaults::samplerSetPressCurveMin,
                                           WFSParameterDefaults::samplerSetPressCurveMax };

    juce::ToggleButton pressHFEnable;
    juce::TextButton pressHFDirBtn;
    WfsStandardSlider pressHFCurveSlider { WFSParameterDefaults::samplerSetPressCurveMin,
                                            WFSParameterDefaults::samplerSetPressCurveMax };

    juce::ToggleButton pressXYEnable;
    std::unique_ptr<WfsStandardSlider> pressXYScaleSlider;

    // ── Copy/Paste & Import/Export ──
    juce::TextButton copyButton;
    juce::TextButton pasteButton;
    juce::TextButton importButton;
    juce::TextButton exportButton;

    // File chooser (must persist during async operation)
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerSubTab)
};
