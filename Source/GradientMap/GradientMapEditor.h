#pragma once

#include <JuceHeader.h>
#include "GradientMapData.h"
#include "GradientMapEvaluator.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"
#include "../gui/ColorScheme.h"
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

            layerVisibleBtn[i].setClickingTogglesState (true);
            layerVisibleBtn[i].setToggleState (true, juce::dontSendNotification);
            layerVisibleBtn[i].onClick = [this] { repaint(); };
            addAndMakeVisible (layerVisibleBtn[i]);
        }

        layerEnableBtn[0].setToggleState (true, juce::dontSendNotification);

        // Layer enable (power) toggles
        for (int i = 0; i < 3; ++i)
        {
            layerEnabledBtn[i].setClickingTogglesState (true);
            layerEnabledBtn[i].setToggleState (true, juce::dontSendNotification);
            layerEnabledBtn[i].onClick = [this, i] { onLayerEnabledToggle (i); };
            addAndMakeVisible (layerEnabledBtn[i]);
        }

        whiteValueLabel.setText (LOC ("inputs.gradientMap.labels.white"), juce::dontSendNotification);
        whiteValueLabel.setJustificationType (juce::Justification::centredRight);
        blackValueLabel.setText (LOC ("inputs.gradientMap.labels.black"), juce::dontSendNotification);
        blackValueLabel.setJustificationType (juce::Justification::centredRight);
        curveLabel.setText (LOC ("inputs.gradientMap.labels.curve"), juce::dontSendNotification);
        curveLabel.setJustificationType (juce::Justification::centredRight);

        whiteValueEditor.setJustification (juce::Justification::centred);
        blackValueEditor.setJustification (juce::Justification::centred);
        whiteValueEditor.onReturnKey = [this] { onLayerPropertyChanged(); };
        blackValueEditor.onReturnKey = [this] { onLayerPropertyChanged(); };
        whiteValueEditor.onFocusLost = [this] { onLayerPropertyChanged(); };
        blackValueEditor.onFocusLost = [this] { onLayerPropertyChanged(); };

        curveSlider.setRange (-1.0, 1.0, 0.01);
        curveSlider.setValue (0.0, juce::dontSendNotification);
        curveSlider.setSliderStyle (juce::Slider::LinearHorizontal);
        curveSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
        curveSlider.onValueChange = [this] { onLayerPropertyChanged(); };

        mappingHintLabel.setJustificationType (juce::Justification::centred);
        mappingHintLabel.setFont (juce::FontOptions (11.0f));
        mappingHintLabel.setColour (juce::Label::textColourId, juce::Colour (0xFF909090));

        heightWarningLabel.setJustificationType (juce::Justification::centred);
        heightWarningLabel.setFont (juce::FontOptions (11.0f));
        heightWarningLabel.setColour (juce::Label::textColourId, juce::Colour (0xFFDD8844));
        heightWarningLabel.setText (LOC ("inputs.gradientMap.warnings.heightRatioZero"), juce::dontSendNotification);
        heightWarningLabel.setVisible (false);

        addAndMakeVisible (whiteValueLabel);
        addAndMakeVisible (whiteValueEditor);
        addAndMakeVisible (blackValueLabel);
        addAndMakeVisible (blackValueEditor);
        addAndMakeVisible (curveLabel);
        addAndMakeVisible (curveSlider);
        addAndMakeVisible (mappingHintLabel);
        addAndMakeVisible (heightWarningLabel);

        // Shape property controls
        shapeFillLabel.setText (LOC ("inputs.gradientMap.labels.fill"), juce::dontSendNotification);
        shapeFillLabel.setJustificationType (juce::Justification::centredRight);
        shapeFillValueSlider.setRange (0.0, 1.0, 0.01);
        shapeFillValueSlider.setValue (1.0, juce::dontSendNotification);
        shapeFillValueSlider.setSliderStyle (juce::Slider::LinearHorizontal);
        shapeFillValueSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
        shapeFillValueSlider.onValueChange = [this] { onShapePropertyChanged(); };

        shapeBlurLabel.setText (LOC ("inputs.gradientMap.labels.blur"), juce::dontSendNotification);
        shapeBlurLabel.setJustificationType (juce::Justification::centredRight);
        shapeBlurSlider.setRange (0.0, 5.0, 0.01);
        shapeBlurSlider.setValue (0.0, juce::dontSendNotification);
        shapeBlurSlider.setSliderStyle (juce::Slider::LinearHorizontal);
        shapeBlurSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
        shapeBlurSlider.onValueChange = [this] { onShapePropertyChanged(); };

        shapeEnableBtn.setButtonText (LOC ("inputs.gradientMap.buttons.enable"));
        shapeEnableBtn.setClickingTogglesState (true);
        shapeEnableBtn.setToggleState (true, juce::dontSendNotification);
        shapeEnableBtn.onClick = [this] { onShapePropertyChanged(); };

        shapeLockBtn.setButtonText (LOC ("inputs.gradientMap.buttons.lock"));
        shapeLockBtn.setClickingTogglesState (true);
        shapeLockBtn.onClick = [this] { onShapePropertyChanged(); };

        shapeDeleteBtn.setButtonText (LOC ("inputs.gradientMap.buttons.delete"));
        shapeDeleteBtn.onClick = [this] { deleteSelectedShapes(); };

        pivotToggleBtn.setButtonText (LOC ("inputs.gradientMap.buttons.pivotCenter"));
        pivotToggleBtn.onClick = [this]
        {
            pivotAtOrigin = ! pivotAtOrigin;
            pivotToggleBtn.setButtonText (pivotAtOrigin ? LOC ("inputs.gradientMap.buttons.pivotOrigin")
                                                        : LOC ("inputs.gradientMap.buttons.pivotCenter"));
            repaint();
        };

        addAndMakeVisible (shapeFillLabel);
        addAndMakeVisible (shapeFillValueSlider);
        addAndMakeVisible (shapeBlurLabel);
        addAndMakeVisible (shapeBlurSlider);
        addAndMakeVisible (shapeEnableBtn);
        addAndMakeVisible (shapeLockBtn);
        addAndMakeVisible (shapeDeleteBtn);
        addAndMakeVisible (pivotToggleBtn);

        // Gradient value sliders (shown for linear/radial fills)
        gradValue1Label.setText (LOC ("inputs.gradientMap.labels.start"), juce::dontSendNotification);
        gradValue1Label.setJustificationType (juce::Justification::centredRight);
        gradValue2Label.setText (LOC ("inputs.gradientMap.labels.end"), juce::dontSendNotification);
        gradValue2Label.setJustificationType (juce::Justification::centredRight);

        gradValue1Slider.setRange (0.0, 1.0, 0.01);
        gradValue1Slider.setValue (1.0, juce::dontSendNotification);
        gradValue1Slider.setSliderStyle (juce::Slider::LinearHorizontal);
        gradValue1Slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
        gradValue1Slider.onValueChange = [this] { onGradientValueChanged(); };

        gradValue2Slider.setRange (0.0, 1.0, 0.01);
        gradValue2Slider.setValue (0.0, juce::dontSendNotification);
        gradValue2Slider.setSliderStyle (juce::Slider::LinearHorizontal);
        gradValue2Slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
        gradValue2Slider.onValueChange = [this] { onGradientValueChanged(); };

        addAndMakeVisible (gradValue1Label);
        addAndMakeVisible (gradValue1Slider);
        addAndMakeVisible (gradValue2Label);
        addAndMakeVisible (gradValue2Slider);

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
        resetView();
        repaint();
    }

    /** Set the current input's height ratio (0-1) so the editor can warn when it's zero */
    void setHeightRatio (float ratio)
    {
        heightRatio = ratio;
        updateHeightWarning();
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

        drawGrid (g, canvasBounds);

        // Fill stage area with "zero effect" color (black in dark themes, white in light)
        drawStageBackground (g, canvasBounds);

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
        g.setFont (13.0f);
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

        // Grey preview swatches at the left edge of the label area
        auto drawSwatch = [&] (const juce::Component& label, float greyVal)
        {
            if (! label.isVisible()) return;
            auto lb = label.getBounds();
            int swX = lb.getX();
            int swY = lb.getCentreY() - 7;
            g.setColour (juce::Colour::greyLevel (displayGrey (greyVal)));
            g.fillRect (swX, swY, 14, 14);
            g.setColour (ColorScheme::get().chromeDivider);
            g.drawRect (swX, swY, 14, 14, 1);
        };

        drawSwatch (shapeFillLabel, static_cast<float> (shapeFillValueSlider.getValue()));
        drawSwatch (gradValue1Label, static_cast<float> (gradValue1Slider.getValue()));
        drawSwatch (gradValue2Label, static_cast<float> (gradValue2Slider.getValue()));
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // Property panel (right 21%)
        int panelWidth = juce::jmax (200, bounds.getWidth() * 21 / 100);
        auto panelBounds = bounds.removeFromRight (panelWidth);

        int rowH = 26, labelW = 60, pad = 4;
        int eyeBtnW = 24;

        // Layer selector row (horizontal, spanning full width)
        auto layerRow = panelBounds.removeFromTop (30);
        {
            int ly = layerRow.getY() + 3;
            int lBtnH = 24;
            int powerBtnW = 20;
            int groupW = (panelWidth - 8) / 3;

            for (int i = 0; i < 3; ++i)
            {
                int lx = layerRow.getX() + 4 + i * groupW;
                layerEnabledBtn[i].setBounds (lx, ly, powerBtnW, lBtnH);
                layerVisibleBtn[i].setBounds (lx + powerBtnW, ly, eyeBtnW, lBtnH);
                layerEnableBtn[i].setBounds (lx + powerBtnW + eyeBtnW, ly, groupW - powerBtnW - eyeBtnW - 2, lBtnH);
            }
        }

        // Separator 1
        separatorY1 = panelBounds.getY();
        panelBounds.removeFromTop (6);

        // Layer header + params
        panelBounds.removeFromTop (22);  // Colored header text (painted)
        layerHeaderY = panelBounds.getY() - 22;

        auto row = panelBounds.removeFromTop (rowH);
        whiteValueLabel.setBounds (row.removeFromLeft (labelW));
        whiteValueEditor.setBounds (row.reduced (pad, 2));

        row = panelBounds.removeFromTop (rowH);
        blackValueLabel.setBounds (row.removeFromLeft (labelW));
        blackValueEditor.setBounds (row.reduced (pad, 2));

        row = panelBounds.removeFromTop (rowH);
        curveLabel.setBounds (row.removeFromLeft (labelW));
        curveSlider.setBounds (row.reduced (pad, 2));

        // Mapping hint label
        mappingHintLabel.setBounds (panelBounds.removeFromTop (18).reduced (2, 0));

        // Height ratio warning (only visible on Height layer when ratio is 0%)
        heightWarningLabel.setBounds (panelBounds.removeFromTop (16).reduced (2, 0));

        // Separator 2
        separatorY2 = panelBounds.getY() + 2;
        panelBounds.removeFromTop (8);

        // Shape properties
        row = panelBounds.removeFromTop (rowH);
        shapeFillLabel.setBounds (row.removeFromLeft (labelW));
        shapeFillValueSlider.setBounds (row.reduced (pad, 2));

        row = panelBounds.removeFromTop (rowH);
        gradValue1Label.setBounds (row.removeFromLeft (labelW));
        gradValue1Slider.setBounds (row.reduced (pad, 2));

        row = panelBounds.removeFromTop (rowH);
        gradValue2Label.setBounds (row.removeFromLeft (labelW));
        gradValue2Slider.setBounds (row.reduced (pad, 2));

        row = panelBounds.removeFromTop (rowH);
        shapeBlurLabel.setBounds (row.removeFromLeft (labelW));
        shapeBlurSlider.setBounds (row.reduced (pad, 2));

        row = panelBounds.removeFromTop (rowH);
        shapeEnableBtn.setBounds (row.removeFromLeft (42).reduced (pad, 2));
        pivotToggleBtn.setBounds (row.removeFromLeft (90).reduced (pad, 2));
        shapeLockBtn.setBounds (row.removeFromLeft (50).reduced (pad, 2));
        shapeDeleteBtn.setBounds (row.removeFromLeft (55).reduced (pad, 2));

        // Tool buttons at bottom of panel (3 rows)
        int toolBtnH = 26, toolPad = 3;
        auto toolArea = panelBounds.removeFromBottom (toolBtnH * 3 + toolPad * 4);

        // Separator before tools
        separatorY3 = toolArea.getY();
        toolArea.removeFromTop (toolPad + 2);

        auto toolRow1 = toolArea.removeFromTop (toolBtnH);
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
    }

    //==========================================================================
    // Mouse Interaction
    //==========================================================================

    void mouseDown (const juce::MouseEvent& e) override
    {
        auto canvasBounds = getCanvasBounds();
        if (! canvasBounds.contains (e.getPosition()))
            return;

        if (e.mods.isRightButtonDown())
        {
            isPanning = true;
            panStart = viewOffset;
            return;
        }

        auto stagePos = screenToStage (e.getPosition(), canvasBounds);

        if (currentTool == Tool::Select)
        {
            // Check scale/rotate handles on already-selected shapes first
            if (selectedShapeIndices.size() == 1)
            {
                int selIdx = selectedShapeIndices[0];
                if (selIdx >= 0 && selIdx < static_cast<int> (currentLayerData.shapes.size()))
                {
                    auto& shape = currentLayerData.shapes[static_cast<size_t> (selIdx)];

                    // Rotation handle hit-test
                    juce::Point<float> rotHandlePos;
                    if (getRotationHandlePos (shape, canvasBounds, rotHandlePos)
                        && e.getPosition().toFloat().getDistanceFrom (rotHandlePos) < 8.0f)
                    {
                        isRotating = true;
                        rotateShapeIdx = selIdx;
                        shapeStartRotation = shape.rotation;

                        auto pivot = pivotAtOrigin ? juce::Point<float> (0.0f, 0.0f)
                                                   : juce::Point<float> (shape.posX, shape.posY);
                        rotateStartAngle = std::atan2 (stagePos.x - pivot.x, -(stagePos.y - pivot.y));
                        repaint();
                        return;
                    }

                    // Scale corner handle hit-test
                    int cornerIdx = hitTestScaleHandle (e.getPosition().toFloat(), shape, canvasBounds);
                    if (cornerIdx >= 0)
                    {
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
        auto canvasBounds = getCanvasBounds();

        if (isPanning)
        {
            viewOffset.x = panStart.x + static_cast<float> (e.getDistanceFromDragStartX());
            viewOffset.y = panStart.y + static_cast<float> (e.getDistanceFromDragStartY());
            repaint();
            return;
        }

        // Rotation dragging
        if (isRotating && rotateShapeIdx >= 0
            && rotateShapeIdx < static_cast<int> (currentLayerData.shapes.size()))
        {
            auto stagePos = screenToStage (e.getPosition(), canvasBounds);
            auto& shape = currentLayerData.shapes[static_cast<size_t> (rotateShapeIdx)];
            auto pivot = pivotAtOrigin ? juce::Point<float> (0.0f, 0.0f)
                                       : juce::Point<float> (shape.posX, shape.posY);
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
            auto pivot = pivotAtOrigin ? juce::Point<float> (0.0f, 0.0f)
                                       : juce::Point<float> (shape.posX, shape.posY);

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
        auto canvasBounds = getCanvasBounds();

        if (isPanning)
        {
            isPanning = false;
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
                currentLayerData.shapes.push_back (newShape);
                selectedShapeIndices.clear();
                selectedShapeIndices.push_back (static_cast<int> (currentLayerData.shapes.size()) - 1);
                updateShapePropertyPanel();
                saveCurrentLayerToValueTree();
            }

            repaint();
        }
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
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
        if (getCanvasBounds().contains (e.getPosition()))
        {
            float zoomFactor = (wheel.deltaY > 0) ? 1.15f : (1.0f / 1.15f);
            viewScale = juce::jlimit (5.0f, 500.0f, viewScale * zoomFactor);
            repaint();
        }
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
        {
            deleteSelectedShapes();
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
        void paintButton (juce::Graphics& g, bool isHighlighted, bool isDown) override
        {
            auto b = getLocalBounds().toFloat().reduced (3.0f, 5.0f);
            float cx = b.getCentreX(), cy = b.getCentreY();
            float hw = b.getWidth() * 0.48f, hh = b.getHeight() * 0.38f;

            bool open = getToggleState();
            float alpha = open ? 1.0f : 0.35f;
            if (isHighlighted) alpha = juce::jmin (1.0f, alpha + 0.15f);
            (void) isDown;

            g.setColour (juce::Colours::white.withAlpha (alpha));

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
        void paintButton (juce::Graphics& g, bool isHighlighted, bool isDown) override
        {
            auto b = getLocalBounds().toFloat().reduced (3.0f, 5.0f);
            float cx = b.getCentreX(), cy = b.getCentreY();
            float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.4f;

            bool on = getToggleState();
            float alpha = on ? 1.0f : 0.3f;
            if (isHighlighted) alpha = juce::jmin (1.0f, alpha + 0.15f);
            (void) isDown;

            auto col = on ? juce::Colours::limegreen.withAlpha (alpha)
                          : juce::Colours::white.withAlpha (alpha);
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
    juce::Slider curveSlider;

    juce::Label mappingHintLabel;
    juce::Label heightWarningLabel;

    // Property panel UI — Shape
    juce::Label shapeFillLabel, shapeBlurLabel;
    juce::Slider shapeFillValueSlider, shapeBlurSlider;
    juce::TextButton shapeEnableBtn, shapeLockBtn, shapeDeleteBtn;

    // Gradient value sliders
    juce::Label gradValue1Label, gradValue2Label;
    juce::Slider gradValue1Slider, gradValue2Slider;

    // Pivot toggle
    juce::TextButton pivotToggleBtn;

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
    bool pivotAtOrigin = false;

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
            { draggingGradientHandle = 0; return true; }
            if (screenPos.toFloat().getDistanceFrom (p2Screen.toFloat()) < hitRadius)
            { draggingGradientHandle = 1; return true; }
        }
        else if (shape.fillType == GradientMap::FillType::RadialGradient)
        {
            auto centerScreen = stageToScreen (shapeLocalToStage ({ shape.radialGradient.cx, shape.radialGradient.cy }, shape), canvas);
            auto edgeLocal = juce::Point<float> (shape.radialGradient.cx + shape.radialGradient.radius, shape.radialGradient.cy);
            auto edgeScreen = stageToScreen (shapeLocalToStage (edgeLocal, shape), canvas);

            if (screenPos.toFloat().getDistanceFrom (centerScreen.toFloat()) < hitRadius)
            { draggingGradientHandle = 0; return true; }
            if (screenPos.toFloat().getDistanceFrom (edgeScreen.toFloat()) < hitRadius)
            { draggingGradientHandle = 1; return true; }
        }

        return false;
    }

    //==========================================================================
    // Drawing
    //==========================================================================

    void drawGrid (juce::Graphics& g, const juce::Rectangle<int>& canvas) const
    {
        g.setColour (juce::Colour (0xFF333333));

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

            // Draw fill based on fill type (using displayGrey for theme-correct rendering)
            if (shape.fillType == GradientMap::FillType::LinearGradient)
            {
                auto p1Stage = shapeLocalToStage ({ shape.linearGradient.x1, shape.linearGradient.y1 }, shape);
                auto p2Stage = shapeLocalToStage ({ shape.linearGradient.x2, shape.linearGradient.y2 }, shape);
                auto p1Scr = stageToScreen (p1Stage, canvas);
                auto p2Scr = stageToScreen (p2Stage, canvas);

                juce::ColourGradient grad (
                    juce::Colour::greyLevel (displayGrey (shape.linearGradient.value1)).withAlpha (shapeAlpha),
                    static_cast<float> (p1Scr.x), static_cast<float> (p1Scr.y),
                    juce::Colour::greyLevel (displayGrey (shape.linearGradient.value2)).withAlpha (shapeAlpha),
                    static_cast<float> (p2Scr.x), static_cast<float> (p2Scr.y), false);
                g.setGradientFill (grad);
            }
            else if (shape.fillType == GradientMap::FillType::RadialGradient)
            {
                auto centerStage = shapeLocalToStage ({ shape.radialGradient.cx, shape.radialGradient.cy }, shape);
                auto edgeStage = shapeLocalToStage ({ shape.radialGradient.cx + shape.radialGradient.radius, shape.radialGradient.cy }, shape);
                auto cScr = stageToScreen (centerStage, canvas);
                auto eScr = stageToScreen (edgeStage, canvas);
                float rPx = cScr.toFloat().getDistanceFrom (eScr.toFloat());

                juce::ColourGradient grad (
                    juce::Colour::greyLevel (displayGrey (shape.radialGradient.centerValue)).withAlpha (shapeAlpha),
                    static_cast<float> (cScr.x), static_cast<float> (cScr.y),
                    juce::Colour::greyLevel (displayGrey (shape.radialGradient.edgeValue)).withAlpha (shapeAlpha),
                    static_cast<float> (cScr.x) + rPx, static_cast<float> (cScr.y), true);
                g.setGradientFill (grad);
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

            if (isSelected)
            {
                g.setColour (getLayerColour (layerIdx).withAlpha (shapeAlpha));
                g.strokePath (path, juce::PathStrokeType (2.5f));
            }
            else
            {
                g.setColour (getLayerColour (layerIdx).withAlpha (shapeAlpha * 0.6f));
                g.strokePath (path, juce::PathStrokeType (1.0f));
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
    }

    void drawGradientHandles (juce::Graphics& g, const juce::Rectangle<int>& canvas) const
    {
        if (gradientEditShapeIdx < 0 || gradientEditShapeIdx >= static_cast<int> (currentLayerData.shapes.size()))
            return;

        bool isGradientTool = (currentTool == Tool::LinearGradient || currentTool == Tool::RadialGradient);
        if (! isGradientTool)
            return;

        const auto& shape = currentLayerData.shapes[static_cast<size_t> (gradientEditShapeIdx)];
        constexpr float handleR = 6.0f;

        if (shape.fillType == GradientMap::FillType::LinearGradient)
        {
            auto p1 = stageToScreen (shapeLocalToStage ({ shape.linearGradient.x1, shape.linearGradient.y1 }, shape), canvas).toFloat();
            auto p2 = stageToScreen (shapeLocalToStage ({ shape.linearGradient.x2, shape.linearGradient.y2 }, shape), canvas).toFloat();

            // Connecting line
            g.setColour (juce::Colours::white.withAlpha (0.6f));
            float dashLengths[] = { 4.0f, 3.0f };
            g.drawDashedLine (juce::Line<float> (p1, p2), dashLengths, 2, 1.2f);

            // Handle 1 (start)
            g.setColour (juce::Colour::greyLevel (displayGrey (shape.linearGradient.value1)));
            g.fillEllipse (p1.x - handleR, p1.y - handleR, handleR * 2, handleR * 2);
            g.setColour (juce::Colours::white);
            g.drawEllipse (p1.x - handleR, p1.y - handleR, handleR * 2, handleR * 2, 1.5f);

            // Handle 2 (end)
            g.setColour (juce::Colour::greyLevel (displayGrey (shape.linearGradient.value2)));
            g.fillEllipse (p2.x - handleR, p2.y - handleR, handleR * 2, handleR * 2);
            g.setColour (juce::Colours::white);
            g.drawEllipse (p2.x - handleR, p2.y - handleR, handleR * 2, handleR * 2, 1.5f);
        }
        else if (shape.fillType == GradientMap::FillType::RadialGradient)
        {
            auto center = stageToScreen (shapeLocalToStage ({ shape.radialGradient.cx, shape.radialGradient.cy }, shape), canvas).toFloat();
            auto edge = stageToScreen (shapeLocalToStage ({ shape.radialGradient.cx + shape.radialGradient.radius, shape.radialGradient.cy }, shape), canvas).toFloat();

            // Connecting line
            g.setColour (juce::Colours::white.withAlpha (0.6f));
            g.drawLine (juce::Line<float> (center, edge), 1.2f);

            // Center handle
            g.setColour (juce::Colour::greyLevel (displayGrey (shape.radialGradient.centerValue)));
            g.fillEllipse (center.x - handleR, center.y - handleR, handleR * 2, handleR * 2);
            g.setColour (juce::Colours::white);
            g.drawEllipse (center.x - handleR, center.y - handleR, handleR * 2, handleR * 2, 1.5f);

            // Edge handle
            g.setColour (juce::Colour::greyLevel (displayGrey (shape.radialGradient.edgeValue)));
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

        // Highlight active layer button
        for (int i = 0; i < 3; ++i)
            layerEnableBtn[i].setToggleState (i == activeLayer, juce::dontSendNotification);

        // Auto-show layer if hidden
        if (! layerVisibleBtn[activeLayer].getToggleState())
            layerVisibleBtn[activeLayer].setToggleState (true, juce::dontSendNotification);

        selectedShapeIndices.clear();
        loadActiveLayer();
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
        curveSlider.setValue (currentLayerData.curve, juce::dontSendNotification);
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
        if (selectedShapeIndices.size() == 1)
        {
            int idx = selectedShapeIndices[0];
            if (idx >= 0 && idx < static_cast<int> (currentLayerData.shapes.size()))
            {
                auto& shape = currentLayerData.shapes[static_cast<size_t> (idx)];
                shapeFillValueSlider.setValue (shape.fillValue, juce::dontSendNotification);
                shapeBlurSlider.setValue (shape.blur, juce::dontSendNotification);
                shapeLockBtn.setToggleState (shape.locked, juce::dontSendNotification);
                shapeEnableBtn.setToggleState (shape.enabled, juce::dontSendNotification);

                // Populate gradient sliders
                if (shape.fillType == GradientMap::FillType::LinearGradient)
                {
                    gradValue1Slider.setValue (shape.linearGradient.value1, juce::dontSendNotification);
                    gradValue2Slider.setValue (shape.linearGradient.value2, juce::dontSendNotification);
                    gradValue1Label.setText (LOC ("inputs.gradientMap.labels.start"), juce::dontSendNotification);
                    gradValue2Label.setText (LOC ("inputs.gradientMap.labels.end"), juce::dontSendNotification);
                }
                else if (shape.fillType == GradientMap::FillType::RadialGradient)
                {
                    gradValue1Slider.setValue (shape.radialGradient.centerValue, juce::dontSendNotification);
                    gradValue2Slider.setValue (shape.radialGradient.edgeValue, juce::dontSendNotification);
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
        shapeBlurLabel.setVisible (v);
        shapeBlurSlider.setVisible (v);
        shapeEnableBtn.setVisible (v);
        shapeLockBtn.setVisible (v);
        shapeDeleteBtn.setVisible (v);
        pivotToggleBtn.setVisible (v);

        // Fill controls are shown/hidden separately by updateShapeFillPanelVisibility
        if (! v)
        {
            shapeFillLabel.setVisible (false);
            shapeFillValueSlider.setVisible (false);
            gradValue1Label.setVisible (false);
            gradValue1Slider.setVisible (false);
            gradValue2Label.setVisible (false);
            gradValue2Slider.setVisible (false);
        }
        else
        {
            updateShapeFillPanelVisibility();
        }
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
        gradValue1Label.setVisible (isGradient);
        gradValue1Slider.setVisible (isGradient);
        gradValue2Label.setVisible (isGradient);
        gradValue2Slider.setVisible (isGradient);
    }

    void onLayerPropertyChanged()
    {
        if (isLoadingData)
            return;

        // Layer param is fixed: 0=Attenuation, 1=Height, 2=HF Shelf (matches activeLayer)
        currentLayerData.param = static_cast<GradientMap::TargetParam> (activeLayer);
        currentLayerData.whiteValue = whiteValueEditor.getText().getFloatValue();
        currentLayerData.blackValue = blackValueEditor.getText().getFloatValue();
        currentLayerData.curve = static_cast<float> (curveSlider.getValue());
        currentLayerData.enabled = true;  // Enable layer when properties are edited

        saveCurrentLayerToValueTree();
        repaint();
    }

    void onShapePropertyChanged()
    {
        if (isLoadingData || selectedShapeIndices.empty())
            return;

        for (int idx : selectedShapeIndices)
        {
            if (idx >= 0 && idx < static_cast<int> (currentLayerData.shapes.size()))
            {
                auto& shape = currentLayerData.shapes[static_cast<size_t> (idx)];
                shape.fillValue = static_cast<float> (shapeFillValueSlider.getValue());
                shape.blur = static_cast<float> (shapeBlurSlider.getValue());
                shape.locked = shapeLockBtn.getToggleState();
                shape.enabled = shapeEnableBtn.getToggleState();
            }
        }

        // Keep fill tool value in sync with panel slider
        currentFillValue = static_cast<float> (shapeFillValueSlider.getValue());

        saveCurrentLayerToValueTree();
        repaint();
    }

    void onGradientValueChanged()
    {
        if (isLoadingData || selectedShapeIndices.size() != 1)
            return;

        int idx = selectedShapeIndices[0];
        if (idx < 0 || idx >= static_cast<int> (currentLayerData.shapes.size()))
            return;

        auto& shape = currentLayerData.shapes[static_cast<size_t> (idx)];

        if (shape.fillType == GradientMap::FillType::LinearGradient)
        {
            shape.linearGradient.value1 = static_cast<float> (gradValue1Slider.getValue());
            shape.linearGradient.value2 = static_cast<float> (gradValue2Slider.getValue());
        }
        else if (shape.fillType == GradientMap::FillType::RadialGradient)
        {
            shape.radialGradient.centerValue = static_cast<float> (gradValue1Slider.getValue());
            shape.radialGradient.edgeValue   = static_cast<float> (gradValue2Slider.getValue());
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GradientMapEditor)
};
