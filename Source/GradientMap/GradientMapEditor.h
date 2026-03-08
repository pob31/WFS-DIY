#pragma once

#include <JuceHeader.h>
#include "GradientMapData.h"
#include "GradientMapEvaluator.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "../gui/ColorScheme.h"
#include "../gui/StatusBar.h"
#include "../gui/sliders/WfsStandardSlider.h"
#include "../gui/sliders/WfsBidirectionalSlider.h"
#include "../Localization/LocalizationManager.h"

/**
 * Gradient Map Editor
 *
 * Full visual editor for drawing greyscale gradient map shapes on a stage map.
 * Layout: Toolbar (top) | Canvas (left 70%) | Property Panel (right 30%)
 *
 * Features:
 * - Draw rectangles, ellipses, polygons
 * - Select, move, resize, rotate shapes
 * - Multi-selection with rubber-band or Shift+click
 * - Uniform, linear, and radial gradient fills
 * - Edge blur per shape
 * - 3 layers with enable/hide toggles
 * - Stage map with grid, bounds, and origin marker
 */
class GradientMapEditor : public juce::Component,
                          private juce::ValueTree::Listener
{
public:
    //==========================================================================
    // Tool Modes
    //==========================================================================

    enum class Tool { Select, DrawRect, DrawEllipse, DrawPolygon, Fill, LinearGradient, RadialGradient };

    //==========================================================================
    // Layer Colors (matching input visualisation bars)
    //==========================================================================

    static constexpr juce::uint32 layerColours[3] = {
        0xFF4A90D9,  // Attenuation — blue
        0xFF5BAE5B,  // Height — muted green
        0xFFE07878   // HF Shelf — pink/coral
    };

    static inline const char* layerNames[3] = { "Attenuation", "Height", "HF Shelf" };

    static juce::Colour getLayerColour (int layerIdx)
    {
        return juce::Colour (layerColours[juce::jlimit (0, 2, layerIdx)]);
    }

    //==========================================================================
    // Construction
    //==========================================================================

    GradientMapEditor()
    {
        setOpaque (true);
        setWantsKeyboardFocus (true);

        // Toolbar buttons
        selectToolBtn.setButtonText (LOC ("inputs.gradientMap.tools.select"));
        rectToolBtn.setButtonText (LOC ("inputs.gradientMap.tools.rect"));
        ellipseToolBtn.setButtonText (LOC ("inputs.gradientMap.tools.ellipse"));
        polygonToolBtn.setButtonText (LOC ("inputs.gradientMap.tools.polygon"));

        selectToolBtn.setRadioGroupId (1001);
        rectToolBtn.setRadioGroupId (1001);
        ellipseToolBtn.setRadioGroupId (1001);
        polygonToolBtn.setRadioGroupId (1001);

        selectToolBtn.setToggleState (true, juce::dontSendNotification);
        selectToolBtn.setClickingTogglesState (true);
        rectToolBtn.setClickingTogglesState (true);
        ellipseToolBtn.setClickingTogglesState (true);
        polygonToolBtn.setClickingTogglesState (true);

        selectToolBtn.onClick = [this] { currentTool = Tool::Select; };
        rectToolBtn.onClick = [this] { currentTool = Tool::DrawRect; };
        ellipseToolBtn.onClick = [this] { currentTool = Tool::DrawEllipse; };
        polygonToolBtn.onClick = [this] { currentTool = Tool::DrawPolygon; };

        addAndMakeVisible (selectToolBtn);
        addAndMakeVisible (rectToolBtn);
        addAndMakeVisible (ellipseToolBtn);
        addAndMakeVisible (polygonToolBtn);

        // Fill tools
        fillToolBtn.setButtonText (LOC ("inputs.gradientMap.tools.fill"));
        linearGradToolBtn.setButtonText (LOC ("inputs.gradientMap.tools.linGrad"));
        radialGradToolBtn.setButtonText (LOC ("inputs.gradientMap.tools.radGrad"));

        fillToolBtn.setRadioGroupId (1001);
        linearGradToolBtn.setRadioGroupId (1001);
        radialGradToolBtn.setRadioGroupId (1001);

        fillToolBtn.setClickingTogglesState (true);
        linearGradToolBtn.setClickingTogglesState (true);
        radialGradToolBtn.setClickingTogglesState (true);

        fillToolBtn.onClick = [this] { currentTool = Tool::Fill; updateShapeFillPanelVisibility(); };
        linearGradToolBtn.onClick = [this] { currentTool = Tool::LinearGradient; updateShapeFillPanelVisibility(); };
        radialGradToolBtn.onClick = [this] { currentTool = Tool::RadialGradient; updateShapeFillPanelVisibility(); };

        addAndMakeVisible (fillToolBtn);
        addAndMakeVisible (linearGradToolBtn);
        addAndMakeVisible (radialGradToolBtn);

        // Layer buttons — permanently assigned parameters, colored by type
        const char* layerLocKeys[3] = {
            "inputs.gradientMap.layers.attenuation",
            "inputs.gradientMap.layers.height",
            "inputs.gradientMap.layers.hfShelf"
        };
        for (int i = 0; i < 3; ++i)
        {
            layerEnableBtn[i].setButtonText (LOC (layerLocKeys[i]));
            layerEnableBtn[i].setClickingTogglesState (true);
            layerEnableBtn[i].setToggleState (false, juce::dontSendNotification);
            layerEnableBtn[i].setColour (juce::TextButton::buttonOnColourId, getLayerColour (i));
            layerEnableBtn[i].onClick = [this, i] { setActiveLayer (i); };
            addAndMakeVisible (layerEnableBtn[i]);

            layerVisibleBtn[i].layerColour = getLayerColour (i);
            layerVisibleBtn[i].setClickingTogglesState (true);
            layerVisibleBtn[i].setToggleState (true, juce::dontSendNotification);
            layerVisibleBtn[i].onClick = [this] { repaint(); };
            addAndMakeVisible (layerVisibleBtn[i]);
        }

        layerEnableBtn[0].setToggleState (true, juce::dontSendNotification);

        // Layer enable (power) toggles
        for (int i = 0; i < 3; ++i)
        {
            layerEnabledBtn[i].layerColour = getLayerColour (i);
            layerEnabledBtn[i].setClickingTogglesState (true);
            layerEnabledBtn[i].setToggleState (true, juce::dontSendNotification);
            layerEnabledBtn[i].onClick = [this, i] { onLayerEnabledToggle (i); };
            addAndMakeVisible (layerEnabledBtn[i]);
        }

        auto labelFont = juce::FontOptions (juce::jmax (10.0f, 15.0f * WfsLookAndFeel::uiScale));
        whiteValueLabel.setText (LOC ("inputs.gradientMap.labels.white"), juce::dontSendNotification);
        whiteValueLabel.setJustificationType (juce::Justification::centredRight);
        whiteValueLabel.setFont (labelFont);
        blackValueLabel.setText (LOC ("inputs.gradientMap.labels.black"), juce::dontSendNotification);
        blackValueLabel.setJustificationType (juce::Justification::centredRight);
        blackValueLabel.setFont (labelFont);
        curveLabel.setText (LOC ("inputs.gradientMap.labels.curve"), juce::dontSendNotification);
        curveLabel.setJustificationType (juce::Justification::centredRight);
        curveLabel.setFont (labelFont);

        whiteValueEditor.setJustification (juce::Justification::centred);
        blackValueEditor.setJustification (juce::Justification::centred);
        whiteValueEditor.onReturnKey = [this] { onLayerPropertyChanged(); };
        blackValueEditor.onReturnKey = [this] { onLayerPropertyChanged(); };
        whiteValueEditor.onFocusLost = [this] { onLayerPropertyChanged(); };
        blackValueEditor.onFocusLost = [this] { onLayerPropertyChanged(); };

        curveSlider.setTrackColours (juce::Colour::fromRGB (30, 30, 30), getLayerColour (0));
        curveSlider.onValueChanged = [this] (float)
        {
            if (! isLoadingData)
            {
                curveValueEditor.setText (juce::String (curveSlider.getValue(), 2), false);
                onLayerPropertyChanged();
            }
        };
        curveValueEditor.setJustification (juce::Justification::centred);
        curveValueEditor.onReturnKey = [this]
        {
            curveSlider.setValue (curveValueEditor.getText().getFloatValue());
            onLayerPropertyChanged();
        };
        curveValueEditor.onFocusLost = curveValueEditor.onReturnKey;

        auto hintFont = juce::FontOptions (juce::jmax (10.0f, 13.0f * WfsLookAndFeel::uiScale));
        mappingHintLabel.setJustificationType (juce::Justification::centred);
        mappingHintLabel.setFont (hintFont);
        mappingHintLabel.setColour (juce::Label::textColourId, juce::Colour (0xFF909090));

        heightWarningLabel.setJustificationType (juce::Justification::centred);
        heightWarningLabel.setFont (hintFont);
        heightWarningLabel.setColour (juce::Label::textColourId, juce::Colour (0xFFDD8844));
        heightWarningLabel.setText (LOC ("inputs.gradientMap.warnings.heightRatioZero"), juce::dontSendNotification);
        heightWarningLabel.setVisible (false);

        addAndMakeVisible (whiteValueLabel);
        addAndMakeVisible (whiteValueEditor);
        addAndMakeVisible (blackValueLabel);
        addAndMakeVisible (blackValueEditor);
        addAndMakeVisible (curveLabel);
        addAndMakeVisible (curveSlider);
        addAndMakeVisible (curveValueEditor);
        addAndMakeVisible (mappingHintLabel);
        addAndMakeVisible (heightWarningLabel);

        // Shape property controls
        shapeNameLabel.setText (LOC ("inputs.gradientMap.labels.name"), juce::dontSendNotification);
        shapeNameLabel.setJustificationType (juce::Justification::centredRight);
        shapeNameLabel.setFont (labelFont);
        shapeNameEditor.onReturnKey = [this]
        {
            if (! selectedShapeIndices.empty())
            {
                auto idx = selectedShapeIndices[0];
                if (idx >= 0 && idx < static_cast<int> (currentLayerData.shapes.size()))
                {
                    currentLayerData.shapes[static_cast<size_t> (idx)].name = shapeNameEditor.getText();
                    saveCurrentLayerToValueTree();
                    repaint();
                }
            }
        };
        shapeNameEditor.onFocusLost = shapeNameEditor.onReturnKey;

        shapeFillLabel.setText (LOC ("inputs.gradientMap.labels.fill"), juce::dontSendNotification);
        shapeFillLabel.setJustificationType (juce::Justification::centredRight);
        shapeFillLabel.setFont (labelFont);
        shapeFillValueSlider.setValue (1.0f);
        shapeFillValueSlider.onValueChanged = [this] (float v)
        {
            if (! isLoadingData)
            {
                shapeFillValueEditor.setText (juce::String (v, 2), false);
                onShapePropertyChanged();
            }
        };
        shapeFillValueEditor.setJustification (juce::Justification::centred);
        shapeFillValueEditor.onReturnKey = [this]
        {
            shapeFillValueSlider.setValue (shapeFillValueEditor.getText().getFloatValue());
            onShapePropertyChanged();
        };
        shapeFillValueEditor.onFocusLost = shapeFillValueEditor.onReturnKey;

        shapeBlurLabel.setText (LOC ("inputs.gradientMap.labels.blur"), juce::dontSendNotification);
        shapeBlurLabel.setJustificationType (juce::Justification::centredRight);
        shapeBlurLabel.setFont (labelFont);
        shapeBlurSlider.onGestureStart = [this]
        {
            if (! isLoadingData)
                pushUndo();
        };
        shapeBlurSlider.onValueChanged = [this] (float v)
        {
            if (! isLoadingData)
            {
                shapeBlurEditor.setText (juce::String (v, 2), false);
                // Update shape data for visual feedback but skip expensive rasterization
                for (int idx : selectedShapeIndices)
                    if (idx >= 0 && idx < static_cast<int> (currentLayerData.shapes.size()))
                        currentLayerData.shapes[static_cast<size_t> (idx)].blur = v;
                repaint();
            }
        };
        shapeBlurSlider.onGestureEnd = [this]
        {
            if (! isLoadingData)
                saveCurrentLayerToValueTree();  // Single rasterize on release
        };
        shapeBlurEditor.setJustification (juce::Justification::centred);
        shapeBlurEditor.onReturnKey = [this]
        {
            shapeBlurSlider.setValue (shapeBlurEditor.getText().getFloatValue());
            onShapePropertyChanged();
        };
        shapeBlurEditor.onFocusLost = shapeBlurEditor.onReturnKey;

        shapeEnableBtn.setButtonText (LOC ("inputs.gradientMap.buttons.enable"));
        shapeEnableBtn.setClickingTogglesState (true);
        shapeEnableBtn.setToggleState (true, juce::dontSendNotification);
        shapeEnableBtn.onClick = [this] { onShapePropertyChanged(); };

        shapeLockBtn.setButtonText (LOC ("inputs.gradientMap.buttons.lock"));
        shapeLockBtn.setClickingTogglesState (true);
        shapeLockBtn.onClick = [this] { onShapePropertyChanged(); };

        shapeDeleteBtn.setButtonText (LOC ("inputs.gradientMap.buttons.delete"));
        shapeDeleteBtn.onClick = [this] { deleteSelectedShapes(); };

        addAndMakeVisible (shapeNameLabel);
        addAndMakeVisible (shapeNameEditor);
        addAndMakeVisible (shapeFillLabel);
        addAndMakeVisible (shapeFillValueSlider);
        addAndMakeVisible (shapeFillValueEditor);
        addAndMakeVisible (shapeBlurLabel);
        addAndMakeVisible (shapeBlurSlider);
        addAndMakeVisible (shapeBlurEditor);
        addAndMakeVisible (shapeEnableBtn);
        addAndMakeVisible (shapeLockBtn);
        addAndMakeVisible (shapeDeleteBtn);


        // Copy/paste buttons
        copyBtn.setButtonText (LOC ("inputs.gradientMap.buttons.copyLayer"));
        copyBtn.onClick = [this] { onCopy(); };
        pasteBtn.setButtonText (LOC ("inputs.gradientMap.buttons.pasteLayer"));
        pasteBtn.onClick = [this] { onPaste(); };
        pasteBtn.setEnabled (false);
        addAndMakeVisible (copyBtn);
        addAndMakeVisible (pasteBtn);

        // Gradient value sliders (shown for linear/radial fills)
        gradValue1Label.setText (LOC ("inputs.gradientMap.labels.start"), juce::dontSendNotification);
        gradValue1Label.setJustificationType (juce::Justification::centredRight);
        gradValue1Label.setFont (labelFont);
        gradValue2Label.setText (LOC ("inputs.gradientMap.labels.end"), juce::dontSendNotification);
        gradValue2Label.setJustificationType (juce::Justification::centredRight);
        gradValue2Label.setFont (labelFont);

        gradValue1Slider.setValue (1.0f);
        gradValue1Slider.onValueChanged = [this] (float v)
        {
            if (! isLoadingData)
            {
                gradValue1Editor.setText (juce::String (v, 2), false);
                onGradientValueChanged();
            }
        };
        gradValue1Editor.setJustification (juce::Justification::centred);
        gradValue1Editor.onReturnKey = [this]
        {
            gradValue1Slider.setValue (gradValue1Editor.getText().getFloatValue());
            onGradientValueChanged();
        };
        gradValue1Editor.onFocusLost = gradValue1Editor.onReturnKey;

        gradValue2Slider.onValueChanged = [this] (float v)
        {
            if (! isLoadingData)
            {
                gradValue2Editor.setText (juce::String (v, 2), false);
                onGradientValueChanged();
            }
        };
        gradValue2Editor.setJustification (juce::Justification::centred);
        gradValue2Editor.onReturnKey = [this]
        {
            gradValue2Slider.setValue (gradValue2Editor.getText().getFloatValue());
            onGradientValueChanged();
        };
        gradValue2Editor.onFocusLost = gradValue2Editor.onReturnKey;

        addAndMakeVisible (gradValue1Label);
        addAndMakeVisible (gradValue1Slider);
        addAndMakeVisible (gradValue1Editor);
        addAndMakeVisible (gradValue2Label);
        addAndMakeVisible (gradValue2Slider);
        addAndMakeVisible (gradValue2Editor);

        // Greyscale sliders: invert display in light theme so dark = full effect
        bool lightTheme = isLightTheme();
        shapeFillValueSlider.setInvertForLightTheme (lightTheme);
        gradValue1Slider.setInvertForLightTheme (lightTheme);
        gradValue2Slider.setInvertForLightTheme (lightTheme);

        setShapePropertiesVisible (false);
    }

    ~GradientMapEditor() override
    {
        if (gradientMapsTree.isValid())
            gradientMapsTree.removeListener (this);
    }

    //==========================================================================
    // Configuration
    //==========================================================================

    /** Set the ValueTree to edit (call when channel changes) */
    void setGradientMapsTree (const juce::ValueTree& gmTree)
    {
        if (gradientMapsTree.isValid())
            gradientMapsTree.removeListener (this);

        gradientMapsTree = gmTree;

        if (gradientMapsTree.isValid())
            gradientMapsTree.addListener (this);

        loadFromValueTree();
        repaint();
    }

    /** Set stage configuration for coordinate transforms */
    void setStageConfig (int shape, float width, float depth, float diameter,
                         float originW, float originD)
    {
        stageShape = shape;
        stageWidth = width;
        stageDepth = depth;
        stageDiameter = diameter;
        originWidth = originW;
        originDepth = originD;
        recalculateStageBounds();
        hasInitialView = false;
        resetView();
        repaint();
    }

    /** Set the current input's height ratio (0-1) so the editor can warn when it's zero */
    void setHeightRatio (float ratio)
    {
        heightRatio = ratio;
        updateHeightWarning();
    }

    /** Set the status bar for help text display */
    void setStatusBar (StatusBar* bar)
    {
        statusBar = bar;
        setupHelpText();
    }

    /** Callback when gradient maps change (for rasterization trigger) */
    std::function<void()> onGradientMapsChanged;

    //==========================================================================
    // Component Overrides
    //==========================================================================

    void paint (juce::Graphics& g) override
    {
        g.fillAll (ColorScheme::get().background);

        auto canvasBounds = getCanvasBounds();

        // Clip to canvas area
        g.saveState();
        g.reduceClipRegion (canvasBounds);

        // Fill stage area with "zero effect" color (black in dark themes, white in light)
        drawStageBackground (g, canvasBounds);

        // Grid drawn AFTER background so it's visible on the stage
        drawGrid (g, canvasBounds);

        drawStageBounds (g, canvasBounds);
        drawOriginMarker (g, canvasBounds);

        // Draw shapes for each visible layer
        for (int layerIdx = 0; layerIdx < 3; ++layerIdx)
        {
            if (! layerVisibleBtn[layerIdx].getToggleState())
                continue;

            bool layerEnabled = layerEnabledBtn[layerIdx].getToggleState();
            float alpha = (layerIdx == activeLayer) ? 1.0f : 0.3f;
            if (! layerEnabled)
                alpha *= 0.2f;
            drawLayerShapes (g, canvasBounds, layerIdx, alpha);
        }

        // Draw selection handles
        drawSelectionHandles (g, canvasBounds);

        // Draw gradient control handles
        drawGradientHandles (g, canvasBounds);

        // Draw rubber-band
        if (isRubberBanding)
        {
            g.setColour (juce::Colours::dodgerblue.withAlpha (0.3f));
            g.fillRect (rubberBandRect);
            g.setColour (juce::Colours::dodgerblue);
            g.drawRect (rubberBandRect, 1);
        }

        // Draw shape being created
        if (isCreating)
        {
            g.setColour (juce::Colours::white.withAlpha (0.5f));
            g.drawRect (juce::Rectangle<float> (createStartScreen.toFloat(), createCurrentScreen.toFloat()), 1.0f);
        }

        // Draw polygon being built — vertices, edges, and hint text
        if (isBuildingPolygon && ! polygonVertices.empty())
        {
            auto layerCol = getLayerColour (activeLayer);
            const float dotR = 4.0f;

            // Draw edges between placed vertices
            g.setColour (layerCol.withAlpha (0.6f));
            for (size_t vi = 1; vi < polygonVertices.size(); ++vi)
            {
                auto a = stageToScreen (polygonVertices[vi - 1], canvasBounds).toFloat();
                auto b = stageToScreen (polygonVertices[vi], canvasBounds).toFloat();
                g.drawLine (a.x, a.y, b.x, b.y, 1.5f);
            }

            // Draw vertex dots
            g.setColour (layerCol);
            for (auto& v : polygonVertices)
            {
                auto scr = stageToScreen (v, canvasBounds).toFloat();
                g.fillEllipse (scr.x - dotR, scr.y - dotR, dotR * 2, dotR * 2);
            }

            // Hint text on canvas
            g.setColour (juce::Colours::white.withAlpha (0.7f));
            g.setFont (juce::jmax (10.0f, 13.0f * WfsLookAndFeel::uiScale));
            g.drawText (LOC ("inputs.gradientMap.hints.polygonClose"),
                        canvasBounds.reduced (8).removeFromBottom (24),
                        juce::Justification::centredBottom, false);
        }

        g.restoreState();

        // Property panel background
        auto panelBounds = getPropertyPanelBounds();
        g.setColour (ColorScheme::get().background.brighter (0.05f));
        g.fillRect (panelBounds);

        int panelLeft = panelBounds.getX();
        int panelRight = panelBounds.getRight();

        // Separator lines
        g.setColour (ColorScheme::get().chromeDivider);
        g.drawHorizontalLine (separatorY1, static_cast<float> (panelLeft + 4), static_cast<float> (panelRight - 4));
        g.drawHorizontalLine (separatorY2, static_cast<float> (panelLeft + 4), static_cast<float> (panelRight - 4));
        g.drawHorizontalLine (separatorY3, static_cast<float> (panelLeft + 4), static_cast<float> (panelRight - 4));

        // Layer header — colored by active layer
        g.setColour (getLayerColour (activeLayer));
        {
            const char* headerKeys[3] = {
                "inputs.gradientMap.header.attenuationLayer",
                "inputs.gradientMap.header.heightLayer",
                "inputs.gradientMap.header.hfShelfLayer"
            };
            g.drawText (LOC (headerKeys[juce::jlimit (0, 2, activeLayer)]),
                        panelLeft + 4, layerHeaderY, panelRight - panelLeft - 8, 20,
                        juce::Justification::centred);
        }

    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // Property panel (right 21%)
        int panelWidth = juce::jmax (200, bounds.getWidth() * 21 / 100);
        auto panelBounds = bounds.removeFromRight (panelWidth);

        int rowH = 28, labelW = 60, pad = 4;
        int eyeBtnW = 24;
        int powerBtnW = 20;

        // Layer selector — Row 1: selection buttons, Row 2: power + eye centered
        auto layerRow1 = panelBounds.removeFromTop (36);
        auto layerRow2 = panelBounds.removeFromTop (22);
        {
            int groupW = (panelWidth - 8) / 3;
            for (int i = 0; i < 3; ++i)
            {
                int lx = layerRow1.getX() + 4 + i * groupW;
                layerEnableBtn[i].setBounds (lx, layerRow1.getY() + 2, groupW - 2, 32);

                int toggleW = powerBtnW + eyeBtnW;
                int toggleX = lx + (groupW - toggleW) / 2;
                layerEnabledBtn[i].setBounds (toggleX, layerRow2.getY() + 1, powerBtnW, 20);
                layerVisibleBtn[i].setBounds (toggleX + powerBtnW, layerRow2.getY() + 1, eyeBtnW, 20);
            }
        }

        // Separator 1
        separatorY1 = panelBounds.getY();
        panelBounds.removeFromTop (6);

        // Layer header + params
        panelBounds.removeFromTop (22);  // Colored header text (painted)
        layerHeaderY = panelBounds.getY() - 22;

        int editorW = 50;

        auto row = panelBounds.removeFromTop (rowH);
        whiteValueLabel.setBounds (row.removeFromLeft (labelW));
        whiteValueEditor.setBounds (row.reduced (pad, 2));

        row = panelBounds.removeFromTop (rowH);
        blackValueLabel.setBounds (row.removeFromLeft (labelW));
        blackValueEditor.setBounds (row.reduced (pad, 2));

        row = panelBounds.removeFromTop (rowH);
        curveLabel.setBounds (row.removeFromLeft (labelW));
        curveValueEditor.setBounds (row.removeFromRight (editorW).reduced (0, 2));
        curveSlider.setBounds (row.reduced (pad, 2));

        // Mapping hint label
        mappingHintLabel.setBounds (panelBounds.removeFromTop (20).reduced (2, 0));

        // Height ratio warning (only visible on Height layer when ratio is 0%)
        heightWarningLabel.setBounds (panelBounds.removeFromTop (18).reduced (2, 0));

        // Separator 2
        separatorY2 = panelBounds.getY() + 2;
        panelBounds.removeFromTop (8);

        // Shape properties
        row = panelBounds.removeFromTop (rowH);
        shapeNameLabel.setBounds (row.removeFromLeft (labelW));
        shapeNameEditor.setBounds (row.reduced (pad, 2));

        row = panelBounds.removeFromTop (rowH);
        shapeFillLabel.setBounds (row.removeFromLeft (labelW));
        shapeFillValueEditor.setBounds (row.removeFromRight (editorW).reduced (0, 2));
        shapeFillValueSlider.setBounds (row.reduced (pad, 2));

        row = panelBounds.removeFromTop (rowH);
        gradValue1Label.setBounds (row.removeFromLeft (labelW));
        gradValue1Editor.setBounds (row.removeFromRight (editorW).reduced (0, 2));
        gradValue1Slider.setBounds (row.reduced (pad, 2));

        row = panelBounds.removeFromTop (rowH);
        gradValue2Label.setBounds (row.removeFromLeft (labelW));
        gradValue2Editor.setBounds (row.removeFromRight (editorW).reduced (0, 2));
        gradValue2Slider.setBounds (row.reduced (pad, 2));

        row = panelBounds.removeFromTop (rowH);
        shapeBlurLabel.setBounds (row.removeFromLeft (labelW));
        shapeBlurEditor.setBounds (row.removeFromRight (editorW).reduced (0, 2));
        shapeBlurSlider.setBounds (row.reduced (pad, 2));

        row = panelBounds.removeFromTop (rowH);
        shapeEnableBtn.setBounds (row.removeFromLeft (42).reduced (pad, 2));
        shapeLockBtn.setBounds (row.removeFromLeft (50).reduced (pad, 2));
        shapeDeleteBtn.setBounds (row.removeFromLeft (55).reduced (pad, 2));

        // Tool buttons at bottom of panel (4 rows: copy/paste, select, shapes, fills)
        int toolBtnH = 32, toolPad = 6;
        auto toolArea = panelBounds.removeFromBottom (toolBtnH * 4 + toolPad * 5 + 2);

        // Separator before tools
        separatorY3 = toolArea.getY();
        toolArea.removeFromTop (toolPad + 2);

        // Copy/paste row (above select tool)
        auto toolRow0 = toolArea.removeFromTop (toolBtnH + toolPad).withTrimmedTop (toolPad);
        copyBtn.setBounds (toolRow0.removeFromLeft (toolRow0.getWidth() / 2).reduced (toolPad, 0));
        pasteBtn.setBounds (toolRow0.reduced (toolPad, 0));

        auto toolRow1 = toolArea.removeFromTop (toolBtnH + toolPad).withTrimmedTop (toolPad);
        auto toolRow2 = toolArea.removeFromTop (toolBtnH + toolPad).withTrimmedTop (toolPad);
        auto toolRow3 = toolArea.removeFromTop (toolBtnH + toolPad).withTrimmedTop (toolPad);

        selectToolBtn.setBounds (toolRow1.reduced (toolPad, 0));

        int col3W = (toolRow2.getWidth() - toolPad * 4) / 3;
        int tx = toolRow2.getX() + toolPad;
        rectToolBtn.setBounds (tx, toolRow2.getY(), col3W, toolBtnH);
        ellipseToolBtn.setBounds (tx + col3W + toolPad, toolRow2.getY(), col3W, toolBtnH);
        polygonToolBtn.setBounds (tx + (col3W + toolPad) * 2, toolRow2.getY(), col3W, toolBtnH);

        tx = toolRow3.getX() + toolPad;
        fillToolBtn.setBounds (tx, toolRow3.getY(), col3W, toolBtnH);
        linearGradToolBtn.setBounds (tx + col3W + toolPad, toolRow3.getY(), col3W, toolBtnH);
        radialGradToolBtn.setBounds (tx + (col3W + toolPad) * 2, toolRow3.getY(), col3W, toolBtnH);

        if (! hasInitialView)
        {
            resetView();
            hasInitialView = true;
        }
    }

    //==========================================================================
    // Mouse Interaction
    //==========================================================================

    void mouseDown (const juce::MouseEvent& e) override
    {
        // Ignore events forwarded from child components (via addMouseListener for help text)
        if (e.eventComponent != this)
            return;

        auto canvasBounds = getCanvasBounds();
        if (! canvasBounds.contains (e.getPosition()))
        {
            isRubberBanding = false;
            return;
        }

        if (e.mods.isMiddleButtonDown())
        {
            resetView();
            repaint();
            return;
        }

        if (e.mods.isRightButtonDown())
        {
            isPanning = true;
            panStart = viewOffset;
            return;
        }

        auto stagePos = screenToStage (e.getPosition(), canvasBounds);

        if (currentTool == Tool::Select)
        {
            // Origin handle hit-test (works with multi-selection)
            juce::Point<float> originHandlePos;
            if (! selectedShapeIndices.empty()
                && getOriginHandlePos (canvasBounds, originHandlePos)
                && e.getPosition().toFloat().getDistanceFrom (originHandlePos) < 10.0f)
            {
                pushUndo();
                isDraggingOriginHandle = true;
                auto centroid = getSelectionCentroid();
                originHandleStartAngle = std::atan2 (centroid.x, -centroid.y);
                originHandleStartDist = std::hypot (centroid.x, centroid.y);

                originDragStarts.clear();
                for (int idx : selectedShapeIndices)
                {
                    if (idx >= 0 && idx < static_cast<int> (currentLayerData.shapes.size()))
                    {
                        auto& s = currentLayerData.shapes[static_cast<size_t> (idx)];
                        OriginDragStart ds;
                        ds.posX = s.posX;  ds.posY = s.posY;
                        ds.scaleX = s.scaleX;  ds.scaleY = s.scaleY;
                        ds.angleFromOrigin = std::atan2 (s.posX, -s.posY);
                        ds.distFromOrigin = std::hypot (s.posX, s.posY);
                        originDragStarts.push_back (ds);
                    }
                }
                repaint();
                return;
            }

            // Gradient handle drag (works in Select mode too)
            if (tryStartGradientHandleDrag (e.getPosition(), canvasBounds))
                return;

            // Check scale/rotate handles on already-selected shapes first
            if (selectedShapeIndices.size() == 1)
            {
                int selIdx = selectedShapeIndices[0];
                if (selIdx >= 0 && selIdx < static_cast<int> (currentLayerData.shapes.size()))
                {
                    auto& shape = currentLayerData.shapes[static_cast<size_t> (selIdx)];

                    // Rotation handle hit-test (skip if locked)
                    juce::Point<float> rotHandlePos;
                    if (! shape.locked
                        && getRotationHandlePos (shape, canvasBounds, rotHandlePos)
                        && e.getPosition().toFloat().getDistanceFrom (rotHandlePos) < 8.0f)
                    {
                        pushUndo();
                        isRotating = true;
                        rotateShapeIdx = selIdx;
                        shapeStartRotation = shape.rotation;

                        auto pivot = juce::Point<float> (shape.posX, shape.posY);
                        rotateStartAngle = std::atan2 (stagePos.x - pivot.x, -(stagePos.y - pivot.y));
                        repaint();
                        return;
                    }

                    // Scale corner handle hit-test (skip if locked)
                    int cornerIdx = shape.locked ? -1 : hitTestScaleHandle (e.getPosition().toFloat(), shape, canvasBounds);
                    if (cornerIdx >= 0)
                    {
                        pushUndo();
                        draggingScaleHandle = cornerIdx;
                        scaleHandleShapeIdx = selIdx;
                        shapeStartScaleX = shape.scaleX;
                        shapeStartScaleY = shape.scaleY;
                        dragStartStage = stagePos;
                        repaint();
                        return;
                    }
                }
            }

            int hitIdx = hitTestShape (stagePos);

            if (hitIdx >= 0)
            {
                if (e.mods.isShiftDown())
                {
                    // Toggle selection
                    auto it = std::find (selectedShapeIndices.begin(), selectedShapeIndices.end(), hitIdx);
                    if (it != selectedShapeIndices.end())
                        selectedShapeIndices.erase (it);
                    else
                        selectedShapeIndices.push_back (hitIdx);
                }
                else if (std::find (selectedShapeIndices.begin(), selectedShapeIndices.end(), hitIdx)
                         == selectedShapeIndices.end())
                {
                    selectedShapeIndices.clear();
                    selectedShapeIndices.push_back (hitIdx);
                }

                pushUndo();
                isDragging = true;
                dragStartStage = stagePos;
                updateShapePropertyPanel();
            }
            else
            {
                selectedShapeIndices.clear();
                isRubberBanding = true;
                rubberBandStart = e.getPosition();
                rubberBandRect = {};
                setShapePropertiesVisible (false);
            }
        }
        else if (currentTool == Tool::Fill)
        {
            int hitIdx = hitTestShape (stagePos);
            if (hitIdx >= 0)
            {
                pushUndo();
                auto& shape = currentLayerData.shapes[static_cast<size_t> (hitIdx)];
                shape.fillType = GradientMap::FillType::Uniform;
                shape.fillValue = currentFillValue;

                selectedShapeIndices.clear();
                selectedShapeIndices.push_back (hitIdx);
                updateShapePropertyPanel();
                saveCurrentLayerToValueTree();
            }
        }
        else if (currentTool == Tool::LinearGradient)
        {
            // Check if clicking a gradient handle first
            if (tryStartGradientHandleDrag (e.getPosition(), canvasBounds))
                return;

            int hitIdx = hitTestShape (stagePos);
            if (hitIdx >= 0)
            {
                pushUndo();
                auto& shape = currentLayerData.shapes[static_cast<size_t> (hitIdx)];
                if (shape.fillType != GradientMap::FillType::LinearGradient)
                {
                    shape.fillType = GradientMap::FillType::LinearGradient;
                    // Default: horizontal gradient, grey-to-grey (not black-to-white)
                    shape.linearGradient = { -1.0f, 0.0f, 0.8f, 1.0f, 0.0f, 0.2f };
                }

                selectedShapeIndices.clear();
                selectedShapeIndices.push_back (hitIdx);
                gradientEditShapeIdx = hitIdx;
                updateShapePropertyPanel();
                saveCurrentLayerToValueTree();
            }
        }
        else if (currentTool == Tool::RadialGradient)
        {
            if (tryStartGradientHandleDrag (e.getPosition(), canvasBounds))
                return;

            int hitIdx = hitTestShape (stagePos);
            if (hitIdx >= 0)
            {
                pushUndo();
                auto& shape = currentLayerData.shapes[static_cast<size_t> (hitIdx)];
                if (shape.fillType != GradientMap::FillType::RadialGradient)
                {
                    shape.fillType = GradientMap::FillType::RadialGradient;
                    shape.radialGradient = { 0.0f, 0.0f, 0.8f, 1.0f, 0.2f };
                }

                selectedShapeIndices.clear();
                selectedShapeIndices.push_back (hitIdx);
                gradientEditShapeIdx = hitIdx;
                updateShapePropertyPanel();
                saveCurrentLayerToValueTree();
            }
        }
        else if (currentTool == Tool::DrawRect || currentTool == Tool::DrawEllipse)
        {
            isCreating = true;
            createStartScreen = e.getPosition();
            createCurrentScreen = e.getPosition();
            createStartStage = stagePos;
        }
        else if (currentTool == Tool::DrawPolygon)
        {
            if (! isBuildingPolygon)
            {
                isBuildingPolygon = true;
                polygonVertices.clear();
            }
            polygonVertices.push_back (stagePos);
        }

        repaint();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (e.eventComponent != this)
            return;

        auto canvasBounds = getCanvasBounds();

        if (isPanning)
        {
            viewOffset.x = panStart.x + static_cast<float> (e.getDistanceFromDragStartX());
            viewOffset.y = panStart.y + static_cast<float> (e.getDistanceFromDragStartY());
            repaint();
            return;
        }

        // Origin handle dragging (rotate position around origin + scale shape size)
        if (isDraggingOriginHandle)
        {
            auto stagePos = screenToStage (e.getPosition(), canvasBounds);
            float currentAngle = std::atan2 (stagePos.x, -stagePos.y);
            float currentDist = std::hypot (stagePos.x, stagePos.y);

            float deltaAngle = currentAngle - originHandleStartAngle;
            float scaleFactor = (originHandleStartDist > 0.01f)
                                ? currentDist / originHandleStartDist : 1.0f;
            scaleFactor = juce::jlimit (0.1f, 10.0f, scaleFactor);

            if (e.mods.isShiftDown())
                deltaAngle = std::round (deltaAngle / juce::degreesToRadians (15.0f))
                             * juce::degreesToRadians (15.0f);

            for (size_t i = 0; i < originDragStarts.size() && i < selectedShapeIndices.size(); ++i)
            {
                auto& shape = currentLayerData.shapes[static_cast<size_t> (selectedShapeIndices[i])];
                auto& ds = originDragStarts[i];

                float newAngle = ds.angleFromOrigin + deltaAngle;
                shape.posX = ds.distFromOrigin * std::sin (newAngle);
                shape.posY = -ds.distFromOrigin * std::cos (newAngle);
                shape.scaleX = juce::jmax (0.05f, ds.scaleX * scaleFactor);
                shape.scaleY = juce::jmax (0.05f, ds.scaleY * scaleFactor);
            }
            repaint();
            return;
        }

        // Rotation dragging
        if (isRotating && rotateShapeIdx >= 0
            && rotateShapeIdx < static_cast<int> (currentLayerData.shapes.size()))
        {
            auto stagePos = screenToStage (e.getPosition(), canvasBounds);
            auto& shape = currentLayerData.shapes[static_cast<size_t> (rotateShapeIdx)];
            auto pivot = juce::Point<float> (shape.posX, shape.posY);
            float currentAngle = std::atan2 (stagePos.x - pivot.x, -(stagePos.y - pivot.y));
            float deltaAngle = juce::radiansToDegrees (currentAngle - rotateStartAngle);

            float newRotation = shapeStartRotation + deltaAngle;
            if (e.mods.isShiftDown())
                newRotation = std::round (newRotation / 15.0f) * 15.0f;

            shape.rotation = newRotation;
            repaint();
            return;
        }

        // Scale handle dragging
        if (draggingScaleHandle >= 0 && scaleHandleShapeIdx >= 0
            && scaleHandleShapeIdx < static_cast<int> (currentLayerData.shapes.size()))
        {
            auto stagePos = screenToStage (e.getPosition(), canvasBounds);
            auto& shape = currentLayerData.shapes[static_cast<size_t> (scaleHandleShapeIdx)];
            auto pivot = juce::Point<float> (shape.posX, shape.posY);

            // Convert mouse position to shape-local coords (unscaled, unrotated)
            auto rotInverse = juce::AffineTransform::rotation (juce::degreesToRadians (shape.rotation)).inverted();
            auto relStage = stagePos - pivot;
            auto rotatedRel = relStage.transformedBy (rotInverse);

            float newScaleX = juce::jmax (0.05f, std::abs (rotatedRel.x));
            float newScaleY = juce::jmax (0.05f, std::abs (rotatedRel.y));

            if (e.mods.isShiftDown())
            {
                float uniform = juce::jmax (newScaleX, newScaleY);
                newScaleX = uniform;
                newScaleY = uniform;
            }

            shape.scaleX = newScaleX;
            shape.scaleY = newScaleY;
            repaint();
            return;
        }

        if (draggingGradientHandle >= 0 && gradientEditShapeIdx >= 0
            && gradientEditShapeIdx < static_cast<int> (currentLayerData.shapes.size()))
        {
            auto stagePos = screenToStage (e.getPosition(), canvasBounds);
            auto& shape = currentLayerData.shapes[static_cast<size_t> (gradientEditShapeIdx)];
            auto localPos = stageToShapeLocal (stagePos, shape);

            if (shape.fillType == GradientMap::FillType::LinearGradient)
            {
                if (draggingGradientHandle == 0)
                { shape.linearGradient.x1 = localPos.x; shape.linearGradient.y1 = localPos.y; }
                else
                { shape.linearGradient.x2 = localPos.x; shape.linearGradient.y2 = localPos.y; }
            }
            else if (shape.fillType == GradientMap::FillType::RadialGradient)
            {
                if (draggingGradientHandle == 0)
                { shape.radialGradient.cx = localPos.x; shape.radialGradient.cy = localPos.y; }
                else
                {
                    float dx = localPos.x - shape.radialGradient.cx;
                    float dy = localPos.y - shape.radialGradient.cy;
                    shape.radialGradient.radius = std::sqrt (dx * dx + dy * dy);
                }
            }
            repaint();
            return;
        }

        if (isDragging && ! selectedShapeIndices.empty())
        {
            auto stagePos = screenToStage (e.getPosition(), canvasBounds);
            float dx = stagePos.x - dragStartStage.x;
            float dy = stagePos.y - dragStartStage.y;

            for (int idx : selectedShapeIndices)
            {
                if (idx >= 0 && idx < static_cast<int> (currentLayerData.shapes.size()))
                {
                    auto& shape = currentLayerData.shapes[static_cast<size_t> (idx)];
                    if (! shape.locked)
                    {
                        shape.posX += dx;
                        shape.posY += dy;
                    }
                }
            }

            dragStartStage = stagePos;
            repaint();
        }

        if (isRubberBanding)
        {
            rubberBandRect = juce::Rectangle<int> (rubberBandStart, e.getPosition());
            repaint();
        }

        if (isCreating)
        {
            createCurrentScreen = e.getPosition();
            repaint();
        }
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (e.eventComponent != this)
            return;

        auto canvasBounds = getCanvasBounds();

        if (isPanning)
        {
            isPanning = false;
            return;
        }

        if (isDraggingOriginHandle)
        {
            isDraggingOriginHandle = false;
            originDragStarts.clear();
            saveCurrentLayerToValueTree();
            return;
        }

        if (isRotating)
        {
            isRotating = false;
            rotateShapeIdx = -1;
            saveCurrentLayerToValueTree();
            return;
        }

        if (draggingScaleHandle >= 0)
        {
            draggingScaleHandle = -1;
            scaleHandleShapeIdx = -1;
            saveCurrentLayerToValueTree();
            return;
        }

        if (draggingGradientHandle >= 0)
        {
            draggingGradientHandle = -1;
            saveCurrentLayerToValueTree();
            return;
        }

        if (isDragging)
        {
            isDragging = false;
            saveCurrentLayerToValueTree();
        }

        if (isRubberBanding)
        {
            isRubberBanding = false;
            // Select shapes inside rubber band
            selectedShapeIndices.clear();
            for (int i = 0; i < static_cast<int> (currentLayerData.shapes.size()); ++i)
            {
                auto& shape = currentLayerData.shapes[static_cast<size_t> (i)];
                auto screenPos = stageToScreen ({ shape.posX, shape.posY }, canvasBounds);
                if (rubberBandRect.contains (screenPos))
                    selectedShapeIndices.push_back (i);
            }
            updateShapePropertyPanel();
            repaint();
        }

        if (isCreating)
        {
            isCreating = false;
            auto stageEnd = screenToStage (e.getPosition(), canvasBounds);

            GradientMap::Shape newShape;
            newShape.type = (currentTool == Tool::DrawRect) ? GradientMap::ShapeType::Rectangle
                                                             : GradientMap::ShapeType::Ellipse;
            newShape.posX = (createStartStage.x + stageEnd.x) * 0.5f;
            newShape.posY = (createStartStage.y + stageEnd.y) * 0.5f;
            newShape.scaleX = std::abs (stageEnd.x - createStartStage.x) * 0.5f;
            newShape.scaleY = std::abs (stageEnd.y - createStartStage.y) * 0.5f;
            newShape.fillValue = 1.0f;
            newShape.order = static_cast<int> (currentLayerData.shapes.size());

            if (newShape.scaleX > 0.05f && newShape.scaleY > 0.05f)
            {
                pushUndo();
                currentLayerData.shapes.push_back (newShape);
                selectedShapeIndices.clear();
                selectedShapeIndices.push_back (static_cast<int> (currentLayerData.shapes.size()) - 1);
                updateShapePropertyPanel();
                saveCurrentLayerToValueTree();
            }

            repaint();
        }

        // Unconditional reset — prevents stale rubber band if mouseUp was missed
        isRubberBanding = false;
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        if (e.eventComponent != this)
            return;

        if (isBuildingPolygon && polygonVertices.size() >= 3)
        {
            GradientMap::Shape newShape;
            newShape.type = GradientMap::ShapeType::Polygon;

            // Calculate centroid
            float cx = 0, cy = 0;
            for (auto& v : polygonVertices) { cx += v.x; cy += v.y; }
            cx /= static_cast<float> (polygonVertices.size());
            cy /= static_cast<float> (polygonVertices.size());

            newShape.posX = cx;
            newShape.posY = cy;
            newShape.scaleX = 1.0f;
            newShape.scaleY = 1.0f;

            // Store vertices relative to centroid
            for (auto& v : polygonVertices)
                newShape.vertices.push_back ({ v.x - cx, v.y - cy });

            newShape.fillValue = 1.0f;
            newShape.order = static_cast<int> (currentLayerData.shapes.size());

            pushUndo();
            currentLayerData.shapes.push_back (newShape);
            selectedShapeIndices.clear();
            selectedShapeIndices.push_back (static_cast<int> (currentLayerData.shapes.size()) - 1);
            updateShapePropertyPanel();
            saveCurrentLayerToValueTree();

            isBuildingPolygon = false;
            polygonVertices.clear();
            repaint();
        }
        else
        {
            juce::ignoreUnused (e);
        }
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
    {
        if (e.eventComponent != this)
            return;

        if (getCanvasBounds().contains (e.getPosition()))
        {
            float zoomFactor = (wheel.deltaY > 0) ? 1.15f : (1.0f / 1.15f);
            viewScale = juce::jlimit (5.0f, 500.0f, viewScale * zoomFactor);
            repaint();
        }
    }

    void mouseEnter (const juce::MouseEvent& e) override
    {
        if (statusBar != nullptr)
        {
            auto* comp = e.eventComponent;
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
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        if (statusBar != nullptr)
            statusBar->clearText();
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
        {
            deleteSelectedShapes();
            return true;
        }
        if (key == juce::KeyPress ('z', juce::ModifierKeys::ctrlModifier, 0))
        {
            undo();
            return true;
        }
        if (key == juce::KeyPress ('z', juce::ModifierKeys::ctrlModifier
                                        | juce::ModifierKeys::shiftModifier, 0))
        {
            redo();
            return true;
        }
        if (key == juce::KeyPress ('y', juce::ModifierKeys::ctrlModifier, 0))
        {
            redo();
            return true;
        }
        return false;
    }

private:
    //==========================================================================
    // Data
    //==========================================================================

    juce::ValueTree gradientMapsTree;
    GradientMap::Layer currentLayerData;
    int activeLayer = 0;
    Tool currentTool = Tool::Select;

    // Stage config
    int stageShape = 0;
    float stageWidth = 20.0f, stageDepth = 10.0f, stageDiameter = 20.0f;
    float originWidth = 0.0f, originDepth = -5.0f;
    float stageMinX = -10.0f, stageMaxX = 10.0f;
    float stageMinY = -10.0f, stageMaxY = 0.0f;

    // View state
    float viewScale = 30.0f;        // Pixels per meter
    juce::Point<float> viewOffset;  // Screen offset from center

    // Interaction state
    std::vector<int> selectedShapeIndices;
    bool isDragging = false;
    bool isPanning = false;
    bool isRubberBanding = false;
    bool isCreating = false;
    bool isBuildingPolygon = false;
    juce::Point<float> dragStartStage;
    juce::Point<float> panStart;
    juce::Point<int> rubberBandStart;
    juce::Rectangle<int> rubberBandRect;
    juce::Point<int> createStartScreen, createCurrentScreen;
    juce::Point<float> createStartStage;
    std::vector<juce::Point<float>> polygonVertices;

    bool isLoadingData = false;
    bool hasInitialView = false;
    float heightRatio = 1.0f;

    //==========================================================================
    // Toolbar UI
    //==========================================================================

    juce::TextButton selectToolBtn, rectToolBtn, ellipseToolBtn, polygonToolBtn;
    juce::TextButton fillToolBtn, linearGradToolBtn, radialGradToolBtn;
    juce::TextButton layerEnableBtn[3];

    /** Small toggle button that paints an eye icon instead of text */
    class EyeButton : public juce::ToggleButton
    {
    public:
        juce::Colour layerColour { juce::Colours::white };

        void paintButton (juce::Graphics& g, bool isHighlighted, bool isDown) override
        {
            auto b = getLocalBounds().toFloat().reduced (3.0f, 5.0f);
            float cx = b.getCentreX(), cy = b.getCentreY();
            float hw = b.getWidth() * 0.48f, hh = b.getHeight() * 0.38f;

            bool open = getToggleState();
            float alpha = open ? 1.0f : 0.35f;
            if (isHighlighted) alpha = juce::jmin (1.0f, alpha + 0.15f);
            (void) isDown;

            g.setColour (layerColour.withAlpha (alpha));

            // Eye outline (two arcs)
            juce::Path eye;
            eye.startNewSubPath (cx - hw, cy);
            eye.cubicTo (cx - hw * 0.5f, cy - hh, cx + hw * 0.5f, cy - hh, cx + hw, cy);
            eye.cubicTo (cx + hw * 0.5f, cy + hh, cx - hw * 0.5f, cy + hh, cx - hw, cy);
            eye.closeSubPath();
            g.strokePath (eye, juce::PathStrokeType (1.2f));

            // Pupil
            if (open)
            {
                float r = hh * 0.45f;
                g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
            }
            else
            {
                // Slash through eye when hidden
                g.drawLine (cx - hw * 0.7f, cy + hh * 0.8f,
                            cx + hw * 0.7f, cy - hh * 0.8f, 1.5f);
            }
        }
    };
    EyeButton layerVisibleBtn[3];

    /** Small toggle button that paints a power icon */
    class PowerButton : public juce::ToggleButton
    {
    public:
        juce::Colour layerColour { juce::Colours::white };

        void paintButton (juce::Graphics& g, bool isHighlighted, bool isDown) override
        {
            auto b = getLocalBounds().toFloat().reduced (3.0f, 5.0f);
            float cx = b.getCentreX(), cy = b.getCentreY();
            float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.4f;

            bool on = getToggleState();
            float alpha = on ? 1.0f : 0.3f;
            if (isHighlighted) alpha = juce::jmin (1.0f, alpha + 0.15f);
            (void) isDown;

            auto col = on ? layerColour.withAlpha (alpha)
                          : layerColour.withAlpha (alpha * 0.5f);
            g.setColour (col);

            // Arc (open at top)
            juce::Path arc;
            float startAngle = juce::degreesToRadians (50.0f);
            float endAngle   = juce::degreesToRadians (310.0f);
            arc.addArc (cx - r, cy - r, r * 2.0f, r * 2.0f, startAngle, endAngle, true);
            g.strokePath (arc, juce::PathStrokeType (1.5f));

            // Vertical line at top
            g.drawLine (cx, cy - r * 1.1f, cx, cy - r * 0.1f, 1.5f);
        }
    };
    PowerButton layerEnabledBtn[3];

    // Property panel UI — Layer
    juce::Label whiteValueLabel, blackValueLabel, curveLabel;
    juce::TextEditor whiteValueEditor, blackValueEditor;
    WfsBidirectionalSlider curveSlider;
    juce::TextEditor curveValueEditor;

    juce::Label mappingHintLabel;
    juce::Label heightWarningLabel;

    // Property panel UI — Shape
    juce::Label shapeNameLabel, shapeFillLabel, shapeBlurLabel;
    juce::TextEditor shapeNameEditor;
    WfsGreyscaleSlider shapeFillValueSlider;
    juce::TextEditor shapeFillValueEditor;
    WfsStandardSlider shapeBlurSlider { 0.0f, 5.0f };
    juce::TextEditor shapeBlurEditor;
    juce::TextButton shapeEnableBtn, shapeLockBtn, shapeDeleteBtn;

    // Gradient value sliders
    juce::Label gradValue1Label, gradValue2Label;
    WfsGreyscaleSlider gradValue1Slider, gradValue2Slider;
    juce::TextEditor gradValue1Editor, gradValue2Editor;

    // Copy/paste
    juce::TextButton copyBtn, pasteBtn;

    // Separator line Y positions (set in resized, drawn in paint)
    int separatorY1 = 0, separatorY2 = 0, separatorY3 = 0, layerHeaderY = 0;

    // Fill tool state
    float currentFillValue = 1.0f;
    int draggingGradientHandle = -1;   // -1=none, 0=start/center, 1=end/edge
    int gradientEditShapeIdx = -1;     // Shape whose gradient handles are shown

    // Scale/Rotate state
    int draggingScaleHandle = -1;      // -1=none, 0-3=corner index
    int scaleHandleShapeIdx = -1;
    bool isRotating = false;
    int rotateShapeIdx = -1;
    float rotateStartAngle = 0.0f;
    float shapeStartRotation = 0.0f;
    float shapeStartScaleX = 0.0f, shapeStartScaleY = 0.0f;
    // Origin-based handle state
    bool isDraggingOriginHandle = false;
    float originHandleStartAngle = 0.0f;
    float originHandleStartDist = 0.0f;
    struct OriginDragStart {
        float posX, posY, scaleX, scaleY;
        float angleFromOrigin, distFromOrigin;
    };
    std::vector<OriginDragStart> originDragStarts;

    // Undo/Redo — layer-level snapshots
    std::vector<GradientMap::Layer> undoStack;
    std::vector<GradientMap::Layer> redoStack;
    static constexpr int maxUndoLevels = 50;

    void pushUndo()
    {
        undoStack.push_back (currentLayerData);
        if (static_cast<int> (undoStack.size()) > maxUndoLevels)
            undoStack.erase (undoStack.begin());
        redoStack.clear();
    }

    void undo()
    {
        if (undoStack.empty()) return;
        redoStack.push_back (currentLayerData);
        currentLayerData = undoStack.back();
        undoStack.pop_back();
        selectedShapeIndices.clear();
        saveCurrentLayerToValueTree();
        updateShapePropertyPanel();
        repaint();
    }

    void redo()
    {
        if (redoStack.empty()) return;
        undoStack.push_back (currentLayerData);
        currentLayerData = redoStack.back();
        redoStack.pop_back();
        selectedShapeIndices.clear();
        saveCurrentLayerToValueTree();
        updateShapePropertyPanel();
        repaint();
    }

    //==========================================================================
    // Layer Value Bounds
    //==========================================================================

    struct LayerValueBounds { float min, max, defaultWhite, defaultBlack; };

    static LayerValueBounds getLayerBounds (int layer)
    {
        switch (layer)
        {
            case 0:  return { -92.0f,  0.0f, -92.0f,  0.0f };  // Attenuation (dB)
            case 1:  return { -50.0f, 50.0f,   0.0f,  0.0f };  // Height (m)
            case 2:  return { -24.0f,  0.0f, -24.0f,  0.0f };  // HF Shelf (dB)
            default: return { -92.0f,  0.0f,   0.0f,  0.0f };
        }
    }

    //==========================================================================
    // Theme Helpers
    //==========================================================================

    bool isLightTheme() const
    {
        return ColorScheme::Manager::getInstance().getCurrentTheme() == ColorScheme::Theme::Light;
    }

    /** Map a greyscale value for display: inverted in light theme so dark = full effect */
    float displayGrey (float v) const
    {
        return isLightTheme() ? (1.0f - v) : v;
    }

    //==========================================================================
    // Coordinate Transforms (matching MapTab pattern)
    //==========================================================================

    juce::Point<int> stageToScreen (juce::Point<float> stage, const juce::Rectangle<int>& canvas) const
    {
        float cx = canvas.getCentreX() + stage.x * viewScale + viewOffset.x;
        float cy = canvas.getCentreY() - stage.y * viewScale + viewOffset.y;
        return { juce::roundToInt (cx), juce::roundToInt (cy) };
    }

    juce::Point<float> screenToStage (juce::Point<int> screen, const juce::Rectangle<int>& canvas) const
    {
        float sx = (static_cast<float> (screen.x) - canvas.getCentreX() - viewOffset.x) / viewScale;
        float sy = (canvas.getCentreY() + viewOffset.y - static_cast<float> (screen.y)) / viewScale;
        return { sx, sy };
    }

    juce::Rectangle<int> getCanvasBounds() const
    {
        auto b = getLocalBounds();
        int panelWidth = juce::jmax (200, b.getWidth() * 21 / 100);
        b.removeFromRight (panelWidth);
        return b;
    }

    juce::Rectangle<int> getPropertyPanelBounds() const
    {
        auto b = getLocalBounds();
        int panelWidth = juce::jmax (200, b.getWidth() * 21 / 100);
        return b.removeFromRight (panelWidth);
    }

    //==========================================================================
    // Stage Bounds
    //==========================================================================

    void recalculateStageBounds()
    {
        if (stageShape == 0)  // Box
        {
            float halfW = stageWidth * 0.5f;
            float halfD = stageDepth * 0.5f;
            stageMinX = -halfW - originWidth;
            stageMaxX =  halfW - originWidth;
            stageMinY = -halfD - originDepth;
            stageMaxY =  halfD - originDepth;
        }
        else  // Cylinder / Dome
        {
            float half = stageDiameter * 0.5f;
            stageMinX = -half - originWidth;
            stageMaxX =  half - originWidth;
            stageMinY = -half - originDepth;
            stageMaxY =  half - originDepth;
        }
    }

    void resetView()
    {
        auto canvas = getCanvasBounds();
        if (canvas.isEmpty()) return;

        float stageW = stageMaxX - stageMinX;
        float stageH = stageMaxY - stageMinY;
        if (stageW <= 0 || stageH <= 0) return;

        float scaleW = canvas.getWidth() * 0.8f / stageW;
        float scaleH = canvas.getHeight() * 0.8f / stageH;
        viewScale = juce::jmin (scaleW, scaleH);

        viewOffset.x = originWidth * viewScale;
        viewOffset.y = -originDepth * viewScale;
    }

    //==========================================================================
    // Shape-Local Coordinate Helpers
    //==========================================================================

    juce::Point<float> stageToShapeLocal (juce::Point<float> stagePos, const GradientMap::Shape& shape) const
    {
        auto transform = juce::AffineTransform::scale (shape.scaleX, shape.scaleY)
                            .rotated (juce::degreesToRadians (shape.rotation))
                            .translated (shape.posX, shape.posY);
        return stagePos.transformedBy (transform.inverted());
    }

    juce::Point<float> shapeLocalToStage (juce::Point<float> localPos, const GradientMap::Shape& shape) const
    {
        auto transform = juce::AffineTransform::scale (shape.scaleX, shape.scaleY)
                            .rotated (juce::degreesToRadians (shape.rotation))
                            .translated (shape.posX, shape.posY);
        return localPos.transformedBy (transform);
    }

    //==========================================================================
    // Selection Handle Helpers (corners + rotation handle positions)
    //==========================================================================

    /** Get the 4 transformed corner positions in screen coords for a shape */
    void getCornerScreenPositions (const GradientMap::Shape& shape, const juce::Rectangle<int>& canvas,
                                   juce::Point<float> outCorners[4]) const
    {
        juce::Point<float> localCorners[4] = { {-1,-1}, {1,-1}, {-1,1}, {1,1} };
        for (int i = 0; i < 4; ++i)
            outCorners[i] = stageToScreen (shapeLocalToStage (localCorners[i], shape), canvas).toFloat();
    }

    /** Get the rotation handle screen position for a shape. Returns false if degenerate. */
    bool getRotationHandlePos (const GradientMap::Shape& shape, const juce::Rectangle<int>& canvas,
                               juce::Point<float>& outPos) const
    {
        auto topCenter = stageToScreen (shapeLocalToStage ({ 0.0f, -1.0f }, shape), canvas).toFloat();
        auto shapeCenter = stageToScreen ({ shape.posX, shape.posY }, canvas).toFloat();
        auto dir = topCenter - shapeCenter;
        float dirLen = dir.getDistanceFromOrigin();
        if (dirLen < 0.01f) return false;
        outPos = topCenter + dir * (20.0f / dirLen);
        return true;
    }

    /** Hit-test scale corner handles. Returns corner index 0-3 or -1. */
    int hitTestScaleHandle (juce::Point<float> screenPos, const GradientMap::Shape& shape,
                            const juce::Rectangle<int>& canvas) const
    {
        juce::Point<float> corners[4];
        getCornerScreenPositions (shape, canvas, corners);
        for (int i = 0; i < 4; ++i)
            if (screenPos.getDistanceFrom (corners[i]) < 8.0f)
                return i;
        return -1;
    }

    //==========================================================================
    // Origin Handle Helpers
    //==========================================================================

    juce::Point<float> getSelectionCentroid() const
    {
        juce::Point<float> c { 0.0f, 0.0f };
        int n = 0;
        for (int idx : selectedShapeIndices)
        {
            if (idx >= 0 && idx < static_cast<int> (currentLayerData.shapes.size()))
            {
                const auto& s = currentLayerData.shapes[static_cast<size_t> (idx)];
                c += { s.posX, s.posY };
                ++n;
            }
        }
        return n > 0 ? c / static_cast<float> (n) : c;
    }

    /** Returns the screen position of the origin handle, or nullopt if shapes are too close to origin */
    bool getOriginHandlePos (const juce::Rectangle<int>& canvas, juce::Point<float>& outPos) const
    {
        if (selectedShapeIndices.empty()) return false;
        auto centroid = getSelectionCentroid();
        auto originScreen = stageToScreen ({ 0.0f, 0.0f }, canvas).toFloat();
        auto centroidScreen = stageToScreen (centroid, canvas).toFloat();
        auto dir = centroidScreen - originScreen;
        float dirLen = dir.getDistanceFromOrigin();
        if (dirLen < 5.0f) return false;
        outPos = centroidScreen + dir * (25.0f / dirLen);
        return true;
    }

    //==========================================================================
    // Gradient Handle Hit-Testing & Dragging
    //==========================================================================

    bool tryStartGradientHandleDrag (juce::Point<int> screenPos, const juce::Rectangle<int>& canvas)
    {
        if (gradientEditShapeIdx < 0 || gradientEditShapeIdx >= static_cast<int> (currentLayerData.shapes.size()))
            return false;

        const auto& shape = currentLayerData.shapes[static_cast<size_t> (gradientEditShapeIdx)];
        constexpr float hitRadius = 8.0f;

        if (shape.fillType == GradientMap::FillType::LinearGradient)
        {
            auto p1Screen = stageToScreen (shapeLocalToStage ({ shape.linearGradient.x1, shape.linearGradient.y1 }, shape), canvas);
            auto p2Screen = stageToScreen (shapeLocalToStage ({ shape.linearGradient.x2, shape.linearGradient.y2 }, shape), canvas);

            if (screenPos.toFloat().getDistanceFrom (p1Screen.toFloat()) < hitRadius)
            { pushUndo(); draggingGradientHandle = 0; return true; }
            if (screenPos.toFloat().getDistanceFrom (p2Screen.toFloat()) < hitRadius)
            { pushUndo(); draggingGradientHandle = 1; return true; }
        }
        else if (shape.fillType == GradientMap::FillType::RadialGradient)
        {
            auto centerScreen = stageToScreen (shapeLocalToStage ({ shape.radialGradient.cx, shape.radialGradient.cy }, shape), canvas);
            auto edgeLocal = juce::Point<float> (shape.radialGradient.cx + shape.radialGradient.radius, shape.radialGradient.cy);
            auto edgeScreen = stageToScreen (shapeLocalToStage (edgeLocal, shape), canvas);

            if (screenPos.toFloat().getDistanceFrom (centerScreen.toFloat()) < hitRadius)
            { pushUndo(); draggingGradientHandle = 0; return true; }
            if (screenPos.toFloat().getDistanceFrom (edgeScreen.toFloat()) < hitRadius)
            { pushUndo(); draggingGradientHandle = 1; return true; }
        }

        return false;
    }

    //==========================================================================
    // Drawing
    //==========================================================================

    void drawGrid (juce::Graphics& g, const juce::Rectangle<int>& canvas) const
    {
        g.setColour (isLightTheme() ? juce::Colour (0xFFE0E0E0) : juce::Colour (0xFF222222));

        float left = screenToStage ({ canvas.getX(), 0 }, canvas).x;
        float right = screenToStage ({ canvas.getRight(), 0 }, canvas).x;
        float bottom = screenToStage ({ 0, canvas.getBottom() }, canvas).y;
        float top = screenToStage ({ 0, canvas.getY() }, canvas).y;

        for (float x = std::floor (left); x <= std::ceil (right); x += 1.0f)
        {
            auto p = stageToScreen ({ x, 0 }, canvas);
            g.drawVerticalLine (p.x, static_cast<float> (canvas.getY()), static_cast<float> (canvas.getBottom()));
        }

        for (float y = std::floor (bottom); y <= std::ceil (top); y += 1.0f)
        {
            auto p = stageToScreen ({ 0, y }, canvas);
            g.drawHorizontalLine (p.y, static_cast<float> (canvas.getX()), static_cast<float> (canvas.getRight()));
        }
    }

    void drawStageBackground (juce::Graphics& g, const juce::Rectangle<int>& canvas) const
    {
        // "Zero effect" = black on dark/OLED, white on light
        g.setColour (isLightTheme() ? juce::Colour (0xFFFFFFFF) : juce::Colour (0xFF000000));

        if (stageShape == 0)
        {
            auto topLeft = stageToScreen ({ stageMinX, stageMaxY }, canvas);
            auto bottomRight = stageToScreen ({ stageMaxX, stageMinY }, canvas);
            g.fillRect (juce::Rectangle<int> (topLeft, bottomRight));
        }
        else
        {
            auto center = stageToScreen ({ -originWidth, -originDepth }, canvas);
            float radiusPx = (stageDiameter * 0.5f) * viewScale;
            g.fillEllipse (center.x - radiusPx, center.y - radiusPx,
                           radiusPx * 2, radiusPx * 2);
        }
    }

    void drawStageBounds (juce::Graphics& g, const juce::Rectangle<int>& canvas) const
    {
        g.setColour (juce::Colour (0xFFAAAAAA));

        if (stageShape == 0)
        {
            auto topLeft = stageToScreen ({ stageMinX, stageMaxY }, canvas);
            auto bottomRight = stageToScreen ({ stageMaxX, stageMinY }, canvas);
            g.drawRect (juce::Rectangle<int> (topLeft, bottomRight).toFloat(), 2.0f);
        }
        else
        {
            auto center = stageToScreen ({ -originWidth, -originDepth }, canvas);
            float radiusPx = (stageDiameter * 0.5f) * viewScale;
            g.drawEllipse (center.x - radiusPx, center.y - radiusPx,
                           radiusPx * 2, radiusPx * 2, 2.0f);
        }
    }

    void drawOriginMarker (juce::Graphics& g, const juce::Rectangle<int>& canvas) const
    {
        auto origin = stageToScreen ({ 0.0f, 0.0f }, canvas);
        g.setColour (ColorScheme::get().textPrimary.withAlpha (0.7f));
        g.drawLine (origin.x - 20.0f, static_cast<float> (origin.y),
                    origin.x + 20.0f, static_cast<float> (origin.y), 1.0f);
        g.drawLine (static_cast<float> (origin.x), origin.y - 20.0f,
                    static_cast<float> (origin.x), origin.y + 20.0f, 1.0f);
        g.drawEllipse (origin.x - 5.0f, origin.y - 5.0f, 10.0f, 10.0f, 1.0f);
    }

    void drawLayerShapes (juce::Graphics& g, const juce::Rectangle<int>& canvas,
                          int layerIdx, float alpha) const
    {
        const auto& layerData = (layerIdx == activeLayer) ? currentLayerData : getLayerDataFromTree (layerIdx);

        for (int i = 0; i < static_cast<int> (layerData.shapes.size()); ++i)
        {
            const auto& shape = layerData.shapes[static_cast<size_t> (i)];
            float shapeAlpha = shape.enabled ? alpha : alpha * 0.2f;
            juce::Path path = shape.getPath();

            // Stage → screen transform
            float cx = static_cast<float> (canvas.getCentreX()) + viewOffset.x;
            float cy = static_cast<float> (canvas.getCentreY()) + viewOffset.y;

            auto stageToScreenTransform = juce::AffineTransform::scale (viewScale, -viewScale)
                                              .translated (cx, cy);

            path.applyTransform (stageToScreenTransform);

            // Skip degenerate paths — avoids Direct2D initialiseBitmapData assertion
            auto pathBounds = path.getBounds();
            if (pathBounds.isEmpty() || pathBounds.getWidth() < 0.5f || pathBounds.getHeight() < 0.5f)
                continue;

            // Draw fill based on fill type (using displayGrey for theme-correct rendering)
            if (shape.fillType == GradientMap::FillType::LinearGradient)
            {
                auto p1Stage = shapeLocalToStage ({ shape.linearGradient.x1, shape.linearGradient.y1 }, shape);
                auto p2Stage = shapeLocalToStage ({ shape.linearGradient.x2, shape.linearGradient.y2 }, shape);
                auto p1Scr = stageToScreen (p1Stage, canvas);
                auto p2Scr = stageToScreen (p2Stage, canvas);

                auto c1Lin = juce::Colour::greyLevel (displayGrey (shape.linearGradient.value1)).withAlpha (shapeAlpha);
                auto c2Lin = juce::Colour::greyLevel (displayGrey (shape.linearGradient.value2)).withAlpha (shapeAlpha);
                float linDist = p1Scr.toFloat().getDistanceFrom (p2Scr.toFloat());
                if (linDist < 1.0f)
                {
                    g.setColour (c1Lin.interpolatedWith (c2Lin, 0.5f));
                }
                else
                {
                    juce::ColourGradient grad (c1Lin, static_cast<float> (p1Scr.x), static_cast<float> (p1Scr.y),
                                               c2Lin, static_cast<float> (p2Scr.x), static_cast<float> (p2Scr.y), false);
                    g.setGradientFill (grad);
                }
            }
            else if (shape.fillType == GradientMap::FillType::RadialGradient)
            {
                auto centerStage = shapeLocalToStage ({ shape.radialGradient.cx, shape.radialGradient.cy }, shape);
                auto edgeStage = shapeLocalToStage ({ shape.radialGradient.cx + shape.radialGradient.radius, shape.radialGradient.cy }, shape);
                auto cScr = stageToScreen (centerStage, canvas);
                auto eScr = stageToScreen (edgeStage, canvas);
                float rPx = cScr.toFloat().getDistanceFrom (eScr.toFloat());

                auto cCenter = juce::Colour::greyLevel (displayGrey (shape.radialGradient.centerValue)).withAlpha (shapeAlpha);
                auto cEdge   = juce::Colour::greyLevel (displayGrey (shape.radialGradient.edgeValue)).withAlpha (shapeAlpha);
                if (rPx < 1.0f)
                {
                    g.setColour (cCenter.interpolatedWith (cEdge, 0.5f));
                }
                else
                {
                    juce::ColourGradient grad (cCenter, static_cast<float> (cScr.x), static_cast<float> (cScr.y),
                                               cEdge, static_cast<float> (cScr.x) + rPx, static_cast<float> (cScr.y), true);
                    g.setGradientFill (grad);
                }
            }
            else
            {
                g.setColour (juce::Colour::greyLevel (displayGrey (shape.fillValue)).withAlpha (shapeAlpha));
            }
            g.fillPath (path);

            // Draw outline — tinted with layer color
            bool isSelected = (layerIdx == activeLayer) &&
                              std::find (selectedShapeIndices.begin(), selectedShapeIndices.end(), i)
                                  != selectedShapeIndices.end();

            // Outline always visible — boost alpha for non-active layers
            float outlineAlpha = juce::jmax (0.4f, shapeAlpha);
            if (isSelected)
            {
                g.setColour (getLayerColour (layerIdx).withAlpha (outlineAlpha));
                g.strokePath (path, juce::PathStrokeType (2.5f));
            }
            else
            {
                g.setColour (getLayerColour (layerIdx).withAlpha (outlineAlpha * 0.7f));
                g.strokePath (path, juce::PathStrokeType (1.0f));
            }
        }

        // Draw shape names on active layer
        if (layerIdx == activeLayer)
        {
            g.setColour (getLayerColour (layerIdx).withAlpha (0.9f));
            g.setFont (12.0f);
            for (const auto& shape : layerData.shapes)
            {
                if (shape.name.isNotEmpty())
                {
                    auto center = stageToScreen ({ shape.posX, shape.posY }, canvas);
                    g.drawText (shape.name,
                                center.x - 60, center.y - 22, 120, 16,
                                juce::Justification::centred, true);
                }
            }
        }
    }

    void drawSelectionHandles (juce::Graphics& g, const juce::Rectangle<int>& canvas) const
    {
        if (selectedShapeIndices.empty())
            return;

        g.setColour (getLayerColour (activeLayer));
        const float handleSize = 6.0f;

        for (int idx : selectedShapeIndices)
        {
            if (idx < 0 || idx >= static_cast<int> (currentLayerData.shapes.size()))
                continue;

            const auto& shape = currentLayerData.shapes[static_cast<size_t> (idx)];

            // Transform 4 unit-square corners through shape's full transform
            juce::Point<float> localCorners[4] = { {-1,-1}, {1,-1}, {-1,1}, {1,1} };
            for (auto& c : localCorners)
            {
                auto screenP = stageToScreen (shapeLocalToStage (c, shape), canvas).toFloat();
                g.fillRect (screenP.x - handleSize * 0.5f, screenP.y - handleSize * 0.5f, handleSize, handleSize);
            }

            // Rotation handle: above top-center of shape
            auto topCenter = stageToScreen (shapeLocalToStage ({ 0.0f, -1.0f }, shape), canvas).toFloat();
            auto shapeCenter = stageToScreen ({ shape.posX, shape.posY }, canvas).toFloat();
            auto dir = topCenter - shapeCenter;
            float dirLen = dir.getDistanceFromOrigin();
            if (dirLen > 0.01f)
            {
                auto rotHandlePos = topCenter + dir * (20.0f / dirLen);
                g.drawLine (topCenter.x, topCenter.y, rotHandlePos.x, rotHandlePos.y, 1.0f);
                g.drawEllipse (rotHandlePos.x - 4.0f, rotHandlePos.y - 4.0f, 8.0f, 8.0f, 1.5f);
            }
        }

        // Origin handle — yellow line from origin to centroid, diamond beyond
        juce::Point<float> originHandlePos;
        if (getOriginHandlePos (canvas, originHandlePos))
        {
            auto originScreen = stageToScreen ({ 0.0f, 0.0f }, canvas).toFloat();
            auto centroidScreen = stageToScreen (getSelectionCentroid(), canvas).toFloat();

            g.setColour (getLayerColour (activeLayer).withAlpha (0.5f));
            g.drawLine (originScreen.x, originScreen.y, centroidScreen.x, centroidScreen.y, 1.0f);

            g.setColour (getLayerColour (activeLayer));
            juce::Path diamond;
            diamond.startNewSubPath (originHandlePos.x, originHandlePos.y - 5.0f);
            diamond.lineTo (originHandlePos.x + 5.0f, originHandlePos.y);
            diamond.lineTo (originHandlePos.x, originHandlePos.y + 5.0f);
            diamond.lineTo (originHandlePos.x - 5.0f, originHandlePos.y);
            diamond.closeSubPath();
            g.fillPath (diamond);
        }
    }

    void drawGradientHandles (juce::Graphics& g, const juce::Rectangle<int>& canvas) const
    {
        if (gradientEditShapeIdx < 0 || gradientEditShapeIdx >= static_cast<int> (currentLayerData.shapes.size()))
            return;

        const auto& shape = currentLayerData.shapes[static_cast<size_t> (gradientEditShapeIdx)];
        constexpr float handleR = 6.0f;
        auto handleCol = getLayerColour (activeLayer);

        if (shape.fillType == GradientMap::FillType::LinearGradient)
        {
            auto p1 = stageToScreen (shapeLocalToStage ({ shape.linearGradient.x1, shape.linearGradient.y1 }, shape), canvas).toFloat();
            auto p2 = stageToScreen (shapeLocalToStage ({ shape.linearGradient.x2, shape.linearGradient.y2 }, shape), canvas).toFloat();

            // Connecting line
            g.setColour (handleCol.withAlpha (0.6f));
            float dashLengths[] = { 4.0f, 3.0f };
            g.drawDashedLine (juce::Line<float> (p1, p2), dashLengths, 2, 1.2f);

            // Handle 1 (start)
            g.setColour (handleCol);
            g.fillEllipse (p1.x - handleR, p1.y - handleR, handleR * 2, handleR * 2);
            g.setColour (juce::Colours::white);
            g.drawEllipse (p1.x - handleR, p1.y - handleR, handleR * 2, handleR * 2, 1.5f);

            // Handle 2 (end)
            g.setColour (handleCol);
            g.fillEllipse (p2.x - handleR, p2.y - handleR, handleR * 2, handleR * 2);
            g.setColour (juce::Colours::white);
            g.drawEllipse (p2.x - handleR, p2.y - handleR, handleR * 2, handleR * 2, 1.5f);
        }
        else if (shape.fillType == GradientMap::FillType::RadialGradient)
        {
            auto center = stageToScreen (shapeLocalToStage ({ shape.radialGradient.cx, shape.radialGradient.cy }, shape), canvas).toFloat();
            auto edge = stageToScreen (shapeLocalToStage ({ shape.radialGradient.cx + shape.radialGradient.radius, shape.radialGradient.cy }, shape), canvas).toFloat();

            // Connecting line
            g.setColour (handleCol.withAlpha (0.6f));
            g.drawLine (juce::Line<float> (center, edge), 1.2f);

            // Center handle
            g.setColour (handleCol);
            g.fillEllipse (center.x - handleR, center.y - handleR, handleR * 2, handleR * 2);
            g.setColour (juce::Colours::white);
            g.drawEllipse (center.x - handleR, center.y - handleR, handleR * 2, handleR * 2, 1.5f);

            // Edge handle
            g.setColour (handleCol);
            g.fillEllipse (edge.x - handleR, edge.y - handleR, handleR * 2, handleR * 2);
            g.setColour (juce::Colours::white);
            g.drawEllipse (edge.x - handleR, edge.y - handleR, handleR * 2, handleR * 2, 1.5f);
        }
    }

    //==========================================================================
    // Hit Testing
    //==========================================================================

    int hitTestShape (juce::Point<float> stagePos) const
    {
        // Test in reverse order (topmost shape first)
        for (int i = static_cast<int> (currentLayerData.shapes.size()) - 1; i >= 0; --i)
        {
            const auto& shape = currentLayerData.shapes[static_cast<size_t> (i)];
            juce::Path path = shape.getPath();
            if (path.contains (stagePos.x, stagePos.y))
                return i;
        }
        return -1;
    }

    //==========================================================================
    // Layer Management
    //==========================================================================

    void setActiveLayer (int layerIdx)
    {
        if (layerIdx == activeLayer)
            return;

        // Save current layer back to ValueTree
        saveCurrentLayerToValueTree();

        activeLayer = layerIdx;
        undoStack.clear();
        redoStack.clear();

        // Update curve slider color to match active layer
        curveSlider.setTrackColours (juce::Colour::fromRGB (30, 30, 30), getLayerColour (activeLayer));

        // Highlight active layer button
        for (int i = 0; i < 3; ++i)
            layerEnableBtn[i].setToggleState (i == activeLayer, juce::dontSendNotification);

        // Auto-show layer if hidden
        if (! layerVisibleBtn[activeLayer].getToggleState())
            layerVisibleBtn[activeLayer].setToggleState (true, juce::dontSendNotification);

        selectedShapeIndices.clear();
        loadActiveLayer();

        // Apply default white value for fresh layers
        auto bounds = getLayerBounds (activeLayer);
        if (currentLayerData.whiteValue == 0.0f && currentLayerData.blackValue == 0.0f
            && bounds.defaultWhite != 0.0f)
        {
            currentLayerData.whiteValue = bounds.defaultWhite;
        }

        updateLayerPropertyPanel();
        setShapePropertiesVisible (false);
        repaint();
    }

    void onLayerEnabledToggle (int layerIdx)
    {
        bool enabled = layerEnabledBtn[layerIdx].getToggleState();

        if (layerIdx == activeLayer)
        {
            currentLayerData.enabled = enabled;
            saveCurrentLayerToValueTree();
        }
        else
        {
            if (gradientMapsTree.isValid() && layerIdx < gradientMapsTree.getNumChildren())
            {
                auto layerTree = gradientMapsTree.getChild (layerIdx);
                layerTree.setProperty (WFSParameterIDs::gmLayerEnabled, enabled ? 1 : 0, nullptr);
                if (onGradientMapsChanged)
                    onGradientMapsChanged();
            }
        }
        repaint();
    }

    void loadFromValueTree()
    {
        isLoadingData = true;
        loadActiveLayer();
        updateLayerPropertyPanel();
        syncLayerEnabledButtons();
        isLoadingData = false;
    }

    void syncLayerEnabledButtons()
    {
        if (! gradientMapsTree.isValid())
            return;
        for (int i = 0; i < 3 && i < gradientMapsTree.getNumChildren(); ++i)
        {
            auto layerTree = gradientMapsTree.getChild (i);
            bool enabled = static_cast<int> (layerTree.getProperty (WFSParameterIDs::gmLayerEnabled, 1)) != 0;
            layerEnabledBtn[i].setToggleState (enabled, juce::dontSendNotification);
        }
    }

    void loadActiveLayer()
    {
        if (! gradientMapsTree.isValid() || activeLayer >= gradientMapsTree.getNumChildren())
        {
            currentLayerData = {};
            return;
        }

        auto layerTree = gradientMapsTree.getChild (activeLayer);
        currentLayerData = GradientMap::Layer::fromValueTree (layerTree);
    }

    GradientMap::Layer getLayerDataFromTree (int layerIdx) const
    {
        if (! gradientMapsTree.isValid() || layerIdx >= gradientMapsTree.getNumChildren())
            return {};
        return GradientMap::Layer::fromValueTree (gradientMapsTree.getChild (layerIdx));
    }

    //==========================================================================
    // Saving to ValueTree
    //==========================================================================

    void saveCurrentLayerToValueTree()
    {
        if (! gradientMapsTree.isValid() || activeLayer >= gradientMapsTree.getNumChildren())
            return;

        isLoadingData = true;

        auto layerTree = gradientMapsTree.getChild (activeLayer);

        // Update layer properties
        using namespace WFSParameterIDs;
        layerTree.setProperty (gmLayerEnabled,  currentLayerData.enabled ? 1 : 0, nullptr);
        layerTree.setProperty (gmLayerParam,    static_cast<int> (currentLayerData.param), nullptr);
        layerTree.setProperty (gmLayerWhite,    currentLayerData.whiteValue, nullptr);
        layerTree.setProperty (gmLayerBlack,    currentLayerData.blackValue, nullptr);
        layerTree.setProperty (gmLayerCurve,    currentLayerData.curve, nullptr);
        layerTree.setProperty (gmLayerVisible,  currentLayerData.visible ? 1 : 0, nullptr);

        // Replace all shape children
        while (layerTree.getNumChildren() > 0)
            layerTree.removeChild (0, nullptr);

        for (const auto& shape : currentLayerData.shapes)
            layerTree.appendChild (shape.toValueTree(), nullptr);

        isLoadingData = false;

        if (onGradientMapsChanged)
            onGradientMapsChanged();
    }

    //==========================================================================
    // Shape Operations
    //==========================================================================

    void deleteSelectedShapes()
    {
        if (selectedShapeIndices.empty())
            return;

        pushUndo();

        // Sort descending to delete from end first
        std::sort (selectedShapeIndices.begin(), selectedShapeIndices.end(), std::greater<int>());

        for (int idx : selectedShapeIndices)
        {
            if (idx >= 0 && idx < static_cast<int> (currentLayerData.shapes.size()))
                currentLayerData.shapes.erase (currentLayerData.shapes.begin() + idx);
        }

        selectedShapeIndices.clear();
        setShapePropertiesVisible (false);
        saveCurrentLayerToValueTree();
        repaint();
    }

    //==========================================================================
    // Property Panel Updates
    //==========================================================================

    void updateLayerPropertyPanel()
    {
        whiteValueEditor.setText (juce::String (currentLayerData.whiteValue, 2), false);
        blackValueEditor.setText (juce::String (currentLayerData.blackValue, 2), false);
        curveSlider.setValue (currentLayerData.curve);
        curveValueEditor.setText (juce::String (currentLayerData.curve, 2), false);
        updateMappingHint();
    }

    void updateMappingHint()
    {
        // Use LOC keys: dark* for light theme (inverted), white* for dark/OLED themes
        const char* darkHintKeys[3] = {
            "inputs.gradientMap.hints.darkMaxAtten",
            "inputs.gradientMap.hints.darkMaxHeight",
            "inputs.gradientMap.hints.darkMaxHF"
        };
        const char* whiteHintKeys[3] = {
            "inputs.gradientMap.hints.whiteMaxAtten",
            "inputs.gradientMap.hints.whiteMaxHeight",
            "inputs.gradientMap.hints.whiteMaxHF"
        };

        int idx = juce::jlimit (0, 2, activeLayer);
        const char* key = isLightTheme() ? darkHintKeys[idx] : whiteHintKeys[idx];
        mappingHintLabel.setText (LOC (key), juce::dontSendNotification);
        updateHeightWarning();
    }

    void updateHeightWarning()
    {
        // Show warning only on the Height layer (index 1) when heightRatio is zero
        bool show = (activeLayer == 1) && (heightRatio < 0.001f);
        heightWarningLabel.setVisible (show);
    }

    void updateShapePropertyPanel()
    {
        juce::ScopedValueSetter<bool> loadGuard (isLoadingData, true);

        if (selectedShapeIndices.size() == 1)
        {
            int idx = selectedShapeIndices[0];
            if (idx >= 0 && idx < static_cast<int> (currentLayerData.shapes.size()))
            {
                auto& shape = currentLayerData.shapes[static_cast<size_t> (idx)];
                shapeNameEditor.setText (shape.name, false);
                shapeFillValueSlider.setValue (shape.fillValue);
                shapeFillValueEditor.setText (juce::String (shape.fillValue, 2), false);
                shapeBlurSlider.setValue (shape.blur);
                shapeBlurEditor.setText (juce::String (shape.blur, 2), false);
                shapeLockBtn.setToggleState (shape.locked, juce::dontSendNotification);
                shapeEnableBtn.setToggleState (shape.enabled, juce::dontSendNotification);

                // Populate gradient sliders
                if (shape.fillType == GradientMap::FillType::LinearGradient)
                {
                    gradValue1Slider.setValue (shape.linearGradient.value1);
                    gradValue1Editor.setText (juce::String (shape.linearGradient.value1, 2), false);
                    gradValue2Slider.setValue (shape.linearGradient.value2);
                    gradValue2Editor.setText (juce::String (shape.linearGradient.value2, 2), false);
                    gradValue1Label.setText (LOC ("inputs.gradientMap.labels.start"), juce::dontSendNotification);
                    gradValue2Label.setText (LOC ("inputs.gradientMap.labels.end"), juce::dontSendNotification);
                }
                else if (shape.fillType == GradientMap::FillType::RadialGradient)
                {
                    gradValue1Slider.setValue (shape.radialGradient.centerValue);
                    gradValue1Editor.setText (juce::String (shape.radialGradient.centerValue, 2), false);
                    gradValue2Slider.setValue (shape.radialGradient.edgeValue);
                    gradValue2Editor.setText (juce::String (shape.radialGradient.edgeValue, 2), false);
                    gradValue1Label.setText (LOC ("inputs.gradientMap.labels.center"), juce::dontSendNotification);
                    gradValue2Label.setText (LOC ("inputs.gradientMap.labels.edge"), juce::dontSendNotification);
                }

                gradientEditShapeIdx = idx;
                setShapePropertiesVisible (true);
                updateShapeFillPanelVisibility();
                return;
            }
        }

        if (selectedShapeIndices.empty())
            gradientEditShapeIdx = -1;

        setShapePropertiesVisible (! selectedShapeIndices.empty());
    }

    void setShapePropertiesVisible (bool v)
    {
        shapeNameLabel.setVisible (v);
        shapeNameEditor.setVisible (v);
        shapeBlurLabel.setVisible (v);
        shapeBlurSlider.setVisible (v);
        shapeBlurEditor.setVisible (v);
        shapeEnableBtn.setVisible (v);
        shapeLockBtn.setVisible (v);
        shapeDeleteBtn.setVisible (v);

        // Fill controls are shown/hidden separately by updateShapeFillPanelVisibility
        if (! v)
        {
            shapeFillLabel.setVisible (false);
            shapeFillValueSlider.setVisible (false);
            shapeFillValueEditor.setVisible (false);
            gradValue1Label.setVisible (false);
            gradValue1Slider.setVisible (false);
            gradValue1Editor.setVisible (false);
            gradValue2Label.setVisible (false);
            gradValue2Slider.setVisible (false);
            gradValue2Editor.setVisible (false);
        }
        else
        {
            updateShapeFillPanelVisibility();
        }

        updateCopyPasteButtons();
    }

    void updateShapeFillPanelVisibility()
    {
        GradientMap::FillType ft = GradientMap::FillType::Uniform;

        if (selectedShapeIndices.size() == 1)
        {
            int idx = selectedShapeIndices[0];
            if (idx >= 0 && idx < static_cast<int> (currentLayerData.shapes.size()))
                ft = currentLayerData.shapes[static_cast<size_t> (idx)].fillType;
        }

        bool isUniform = (ft == GradientMap::FillType::Uniform);
        bool isGradient = (ft == GradientMap::FillType::LinearGradient || ft == GradientMap::FillType::RadialGradient);

        shapeFillLabel.setVisible (isUniform);
        shapeFillValueSlider.setVisible (isUniform);
        shapeFillValueEditor.setVisible (isUniform);
        gradValue1Label.setVisible (isGradient);
        gradValue1Slider.setVisible (isGradient);
        gradValue1Editor.setVisible (isGradient);
        gradValue2Label.setVisible (isGradient);
        gradValue2Slider.setVisible (isGradient);
        gradValue2Editor.setVisible (isGradient);
    }

    void onLayerPropertyChanged()
    {
        if (isLoadingData)
            return;

        pushUndo();

        // Layer param is fixed: 0=Attenuation, 1=Height, 2=HF Shelf (matches activeLayer)
        currentLayerData.param = static_cast<GradientMap::TargetParam> (activeLayer);

        auto bounds = getLayerBounds (activeLayer);
        currentLayerData.whiteValue = juce::jlimit (bounds.min, bounds.max,
                                                     whiteValueEditor.getText().getFloatValue());
        currentLayerData.blackValue = juce::jlimit (bounds.min, bounds.max,
                                                     blackValueEditor.getText().getFloatValue());
        whiteValueEditor.setText (juce::String (currentLayerData.whiteValue, 2), false);
        blackValueEditor.setText (juce::String (currentLayerData.blackValue, 2), false);

        currentLayerData.curve = curveSlider.getValue();
        currentLayerData.enabled = true;  // Enable layer when properties are edited

        saveCurrentLayerToValueTree();
        repaint();
    }

    void onShapePropertyChanged()
    {
        if (isLoadingData || selectedShapeIndices.empty())
            return;

        pushUndo();

        for (int idx : selectedShapeIndices)
        {
            if (idx >= 0 && idx < static_cast<int> (currentLayerData.shapes.size()))
            {
                auto& shape = currentLayerData.shapes[static_cast<size_t> (idx)];
                shape.fillValue = shapeFillValueSlider.getValue();
                shape.locked = shapeLockBtn.getToggleState();
                shape.enabled = shapeEnableBtn.getToggleState();
            }
        }

        // Keep fill tool value in sync with panel slider
        currentFillValue = shapeFillValueSlider.getValue();

        saveCurrentLayerToValueTree();
        repaint();
    }

    void onGradientValueChanged()
    {
        if (isLoadingData || selectedShapeIndices.size() != 1)
            return;

        pushUndo();

        int idx = selectedShapeIndices[0];
        if (idx < 0 || idx >= static_cast<int> (currentLayerData.shapes.size()))
            return;

        auto& shape = currentLayerData.shapes[static_cast<size_t> (idx)];

        if (shape.fillType == GradientMap::FillType::LinearGradient)
        {
            shape.linearGradient.value1 = gradValue1Slider.getValue();
            shape.linearGradient.value2 = gradValue2Slider.getValue();
        }
        else if (shape.fillType == GradientMap::FillType::RadialGradient)
        {
            shape.radialGradient.centerValue = gradValue1Slider.getValue();
            shape.radialGradient.edgeValue   = gradValue2Slider.getValue();
        }

        saveCurrentLayerToValueTree();
        repaint();
    }

    //==========================================================================
    // ValueTree::Listener
    //==========================================================================

    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override
    {
        if (! isLoadingData)
        {
            loadActiveLayer();
            updateLayerPropertyPanel();
            repaint();
        }
    }

    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override
    {
        if (! isLoadingData)
        {
            loadActiveLayer();
            repaint();
        }
    }

    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override
    {
        if (! isLoadingData)
        {
            loadActiveLayer();
            repaint();
        }
    }

    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged (juce::ValueTree&) override {}

    //==========================================================================
    // Copy / Paste
    //==========================================================================

    static inline GradientMap::Shape clipboardShape;
    static inline GradientMap::Layer clipboardLayer;
    static inline bool hasClipboardShape = false;
    static inline bool hasClipboardLayer = false;

    void onCopy()
    {
        if (! selectedShapeIndices.empty())
        {
            int idx = selectedShapeIndices[0];
            if (idx >= 0 && idx < static_cast<int> (currentLayerData.shapes.size()))
            {
                clipboardShape = currentLayerData.shapes[static_cast<size_t> (idx)];
                hasClipboardShape = true;
                hasClipboardLayer = false;
            }
        }
        else
        {
            clipboardLayer = currentLayerData;
            hasClipboardLayer = true;
            hasClipboardShape = false;
        }
        updateCopyPasteButtons();
    }

    void onPaste()
    {
        if (hasClipboardShape)
        {
            pushUndo();
            auto newShape = clipboardShape;
            newShape.posX += 0.5f;
            newShape.posY += 0.5f;
            newShape.order = static_cast<int> (currentLayerData.shapes.size());
            currentLayerData.shapes.push_back (newShape);
            selectedShapeIndices.clear();
            selectedShapeIndices.push_back (static_cast<int> (currentLayerData.shapes.size()) - 1);
            updateShapePropertyPanel();
            saveCurrentLayerToValueTree();
        }
        else if (hasClipboardLayer)
        {
            currentLayerData.shapes = clipboardLayer.shapes;
            currentLayerData.whiteValue = clipboardLayer.whiteValue;
            currentLayerData.blackValue = clipboardLayer.blackValue;
            currentLayerData.curve = clipboardLayer.curve;
            selectedShapeIndices.clear();
            updateLayerPropertyPanel();
            setShapePropertiesVisible (false);
            saveCurrentLayerToValueTree();
        }
        repaint();
    }

    void updateCopyPasteButtons()
    {
        bool shapeMode = ! selectedShapeIndices.empty();
        copyBtn.setButtonText (shapeMode ? LOC ("inputs.gradientMap.buttons.copyShape")
                                         : LOC ("inputs.gradientMap.buttons.copyLayer"));

        // Paste always shows what's on clipboard, enabled if anything is copied
        if (hasClipboardShape)
            pasteBtn.setButtonText (LOC ("inputs.gradientMap.buttons.pasteShape"));
        else if (hasClipboardLayer)
            pasteBtn.setButtonText (LOC ("inputs.gradientMap.buttons.pasteLayer"));
        else
            pasteBtn.setButtonText (LOC ("inputs.gradientMap.buttons.pasteShape"));
        pasteBtn.setEnabled (hasClipboardShape || hasClipboardLayer);
    }

    //==========================================================================
    // Status Bar Help Text
    //==========================================================================

    StatusBar* statusBar = nullptr;
    std::unordered_map<juce::Component*, juce::String> helpTextMap;

    void setupHelpText()
    {
        for (int i = 0; i < 3; ++i)
        {
            helpTextMap[&layerEnableBtn[i]]  = LOC ("inputs.gradientMap.help.layerSelect");
            helpTextMap[&layerEnabledBtn[i]] = LOC ("inputs.gradientMap.help.layerEnable");
            helpTextMap[&layerVisibleBtn[i]] = LOC ("inputs.gradientMap.help.layerVisible");
        }
        helpTextMap[&whiteValueEditor]     = LOC ("inputs.gradientMap.help.whiteValue");
        helpTextMap[&blackValueEditor]     = LOC ("inputs.gradientMap.help.blackValue");
        helpTextMap[&curveSlider]          = LOC ("inputs.gradientMap.help.curve");
        helpTextMap[&curveValueEditor]     = LOC ("inputs.gradientMap.help.curve");
        helpTextMap[&shapeFillValueSlider] = LOC ("inputs.gradientMap.help.fillValue");
        helpTextMap[&shapeFillValueEditor] = LOC ("inputs.gradientMap.help.fillValue");
        helpTextMap[&shapeBlurSlider]      = LOC ("inputs.gradientMap.help.blur");
        helpTextMap[&shapeEnableBtn]       = LOC ("inputs.gradientMap.help.shapeEnable");
        helpTextMap[&shapeLockBtn]         = LOC ("inputs.gradientMap.help.shapeLock");
        helpTextMap[&shapeDeleteBtn]       = LOC ("inputs.gradientMap.help.shapeDelete");
        helpTextMap[&selectToolBtn]        = LOC ("inputs.gradientMap.help.selectTool");
        helpTextMap[&rectToolBtn]          = LOC ("inputs.gradientMap.help.rectTool");
        helpTextMap[&ellipseToolBtn]       = LOC ("inputs.gradientMap.help.ellipseTool");
        helpTextMap[&polygonToolBtn]       = LOC ("inputs.gradientMap.help.polygonTool");
        helpTextMap[&fillToolBtn]          = LOC ("inputs.gradientMap.help.fillTool");
        helpTextMap[&linearGradToolBtn]    = LOC ("inputs.gradientMap.help.linGradTool");
        helpTextMap[&radialGradToolBtn]    = LOC ("inputs.gradientMap.help.radGradTool");
        helpTextMap[&copyBtn]              = LOC ("inputs.gradientMap.help.copy");
        helpTextMap[&pasteBtn]             = LOC ("inputs.gradientMap.help.paste");


        for (auto& [comp, text] : helpTextMap)
            comp->addMouseListener (this, true);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GradientMapEditor)
};
