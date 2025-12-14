#pragma once

#include <JuceHeader.h>
#include "../WfsParameters.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSParameterDefaults.h"

/**
 * Map Tab Component
 * Interactive 2D spatial mapping and visualization for WFS sources, speakers, and clusters.
 */
class MapTab : public juce::Component,
               private juce::ValueTree::Listener
{
public:
    MapTab(WfsParameters& params)
        : parameters(params),
          inputsTree(params.getInputTree()),
          outputsTree(params.getOutputTree()),
          reverbsTree(params.getReverbTree()),
          configTree(params.getConfigTree())
    {
        // Add ValueTree listeners
        inputsTree.addListener(this);
        outputsTree.addListener(this);
        reverbsTree.addListener(this);
        configTree.addListener(this);

        // Initialize view to center on stage
        resetView();
    }

    ~MapTab() override
    {
        inputsTree.removeListener(this);
        outputsTree.removeListener(this);
        reverbsTree.removeListener(this);
        configTree.removeListener(this);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xFF1E1E1E));

        drawGrid(g);
        drawStageBounds(g);
        drawOriginMarker(g);
        drawOutputs(g);
        drawReverbs(g);
        drawClusters(g);
        drawInputs(g);

        // Draw zoom level indicator
        g.setColour(juce::Colours::grey);
        g.setFont(12.0f);
        g.drawText("Zoom: " + juce::String(viewScale, 1) + " px/m",
                   10, getHeight() - 25, 150, 20, juce::Justification::left);
    }

    void resized() override
    {
        // Reset view offset to center when resized
        if (viewOffset.isOrigin())
            resetView();
    }

    //==========================================================================
    // Mouse Handling
    //==========================================================================

    void mouseDown(const juce::MouseEvent& e) override
    {
        dragStartPos = e.position;
        dragStartOffset = viewOffset;

        // Check for input hit
        int hitInput = getInputAtPosition(e.position);
        if (hitInput >= 0 && !e.mods.isMiddleButtonDown())
        {
            selectedInput = hitInput;
            isDraggingInput = true;
            inputDragStartStagePos = getInputPosition(hitInput);
            repaint();
            return;
        }

        // No input hit - prepare for panning
        selectedInput = -1;
        isDraggingInput = false;
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (isDraggingInput && selectedInput >= 0)
        {
            // Move input source
            auto stagePos = screenToStage(e.position);

            // Clamp to stage bounds
            float stageW = getStageWidth();
            float stageD = getStageDepth();
            stagePos.x = juce::jlimit(0.0f, stageW, stagePos.x);
            stagePos.y = juce::jlimit(0.0f, stageD, stagePos.y);

            parameters.setInputParam(selectedInput, "inputPositionX", stagePos.x);
            parameters.setInputParam(selectedInput, "inputPositionY", stagePos.y);
            repaint();
        }
        else
        {
            // Pan view
            auto delta = e.position - dragStartPos;
            viewOffset = dragStartOffset + delta;
            repaint();
        }
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        juce::ignoreUnused(e);
        isDraggingInput = false;
    }

    void mouseDoubleClick(const juce::MouseEvent& e) override
    {
        juce::ignoreUnused(e);
        // Double-click to reset view
        resetView();
        repaint();
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
    {
        // Zoom centered on mouse position
        auto mouseStagePos = screenToStage(e.position);

        float zoomFactor = 1.0f + wheel.deltaY * 0.1f;
        float newScale = viewScale * zoomFactor;
        viewScale = juce::jlimit(5.0f, 500.0f, newScale);

        // Adjust offset to keep mouse position stable
        auto newScreenPos = stageToScreen(mouseStagePos);
        viewOffset += e.position - newScreenPos;

        repaint();
    }

private:
    WfsParameters& parameters;
    juce::ValueTree inputsTree;
    juce::ValueTree outputsTree;
    juce::ValueTree reverbsTree;
    juce::ValueTree configTree;

    // View state
    float viewScale = 30.0f;  // pixels per meter
    juce::Point<float> viewOffset { 0.0f, 0.0f };

    // Interaction state
    int selectedInput = -1;
    bool isDraggingInput = false;
    juce::Point<float> dragStartPos;
    juce::Point<float> dragStartOffset;
    juce::Point<float> inputDragStartStagePos;

    // Constants
    static constexpr float markerRadius = 14.0f;
    static constexpr float innerRadiusRatio = 0.6f;

    //==========================================================================
    // Coordinate Conversion
    //==========================================================================

    juce::Point<float> stageToScreen(juce::Point<float> stagePos) const
    {
        float originW = getOriginWidth();
        float originD = getOriginDepth();

        // Convert stage coordinates to screen coordinates
        // Stage Y increases upward, screen Y increases downward
        float screenX = getWidth() / 2.0f + (stagePos.x - originW) * viewScale + viewOffset.x;
        float screenY = getHeight() / 2.0f - (stagePos.y - originD) * viewScale + viewOffset.y;

        return { screenX, screenY };
    }

    juce::Point<float> screenToStage(juce::Point<float> screenPos) const
    {
        float originW = getOriginWidth();
        float originD = getOriginDepth();

        // Convert screen coordinates to stage coordinates
        float stageX = (screenPos.x - getWidth() / 2.0f - viewOffset.x) / viewScale + originW;
        float stageY = (getHeight() / 2.0f + viewOffset.y - screenPos.y) / viewScale + originD;

        return { stageX, stageY };
    }

    //==========================================================================
    // Stage Dimension Helpers
    //==========================================================================

    float getStageWidth() const
    {
        auto stageTree = configTree.getChildWithName(WFSParameterIDs::Stage);
        if (stageTree.isValid())
            return static_cast<float>(stageTree.getProperty(WFSParameterIDs::stageWidth,
                                      WFSParameterDefaults::stageWidthDefault));
        return WFSParameterDefaults::stageWidthDefault;
    }

    float getStageDepth() const
    {
        auto stageTree = configTree.getChildWithName(WFSParameterIDs::Stage);
        if (stageTree.isValid())
            return static_cast<float>(stageTree.getProperty(WFSParameterIDs::stageDepth,
                                      WFSParameterDefaults::stageDepthDefault));
        return WFSParameterDefaults::stageDepthDefault;
    }

    float getOriginWidth() const
    {
        auto stageTree = configTree.getChildWithName(WFSParameterIDs::Stage);
        if (stageTree.isValid())
            return static_cast<float>(stageTree.getProperty(WFSParameterIDs::originWidth,
                                      WFSParameterDefaults::originWidthDefault));
        return WFSParameterDefaults::originWidthDefault;
    }

    float getOriginDepth() const
    {
        auto stageTree = configTree.getChildWithName(WFSParameterIDs::Stage);
        if (stageTree.isValid())
            return static_cast<float>(stageTree.getProperty(WFSParameterIDs::originDepth,
                                      WFSParameterDefaults::originDepthDefault));
        return WFSParameterDefaults::originDepthDefault;
    }

    void resetView()
    {
        // Reset view to show entire stage centered
        float stageW = getStageWidth();
        float stageD = getStageDepth();

        // Calculate scale to fit stage in view with some padding
        float scaleW = (getWidth() > 0) ? (getWidth() * 0.8f) / stageW : 30.0f;
        float scaleD = (getHeight() > 0) ? (getHeight() * 0.8f) / stageD : 30.0f;
        viewScale = juce::jmin(scaleW, scaleD);
        viewScale = juce::jlimit(5.0f, 500.0f, viewScale);

        viewOffset = { 0.0f, 0.0f };
    }

    //==========================================================================
    // Color Helpers (matching Android WFS Control app)
    //==========================================================================

    static juce::Colour getMarkerColor(int id, bool isClusterMarker = false)
    {
        // HSL-based colors matching Android app
        // For inputs: hue = (id * 360 / 32) % 360, saturation = 0.9, lightness = 0.6
        // For clusters: hue = (id * 360 / 10) % 360, saturation = 0.7, lightness = 0.7
        int totalMarkers = isClusterMarker ? 10 : 32;
        float hue = std::fmod((id * 360.0f / totalMarkers), 360.0f) / 360.0f;
        float saturation = isClusterMarker ? 0.7f : 0.9f;
        float lightness = isClusterMarker ? 0.7f : 0.6f;
        return juce::Colour::fromHSL(hue, saturation, lightness, 1.0f);
    }

    static juce::Colour getArrayColor(int arrayNumber)
    {
        return getMarkerColor(arrayNumber, true);
    }

    //==========================================================================
    // Drawing Methods
    //==========================================================================

    void drawGrid(juce::Graphics& g)
    {
        g.setColour(juce::Colours::darkgrey);

        // Calculate visible stage area
        auto topLeft = screenToStage({ 0.0f, 0.0f });
        auto bottomRight = screenToStage({ static_cast<float>(getWidth()),
                                           static_cast<float>(getHeight()) });

        // Draw vertical lines (every 1 meter)
        int startX = static_cast<int>(std::floor(juce::jmin(topLeft.x, bottomRight.x)));
        int endX = static_cast<int>(std::ceil(juce::jmax(topLeft.x, bottomRight.x)));

        for (int x = startX; x <= endX; ++x)
        {
            auto screenX = stageToScreen({ static_cast<float>(x), 0.0f }).x;
            g.drawVerticalLine(static_cast<int>(screenX), 0.0f, static_cast<float>(getHeight()));
        }

        // Draw horizontal lines (every 1 meter)
        int startY = static_cast<int>(std::floor(juce::jmin(topLeft.y, bottomRight.y)));
        int endY = static_cast<int>(std::ceil(juce::jmax(topLeft.y, bottomRight.y)));

        for (int y = startY; y <= endY; ++y)
        {
            auto screenY = stageToScreen({ 0.0f, static_cast<float>(y) }).y;
            g.drawHorizontalLine(static_cast<int>(screenY), 0.0f, static_cast<float>(getWidth()));
        }
    }

    void drawStageBounds(juce::Graphics& g)
    {
        float stageW = getStageWidth();
        float stageD = getStageDepth();

        auto topLeft = stageToScreen({ 0.0f, stageD });
        auto bottomRight = stageToScreen({ stageW, 0.0f });

        juce::Rectangle<float> stageRect(topLeft.x, topLeft.y,
                                         bottomRight.x - topLeft.x,
                                         bottomRight.y - topLeft.y);

        g.setColour(juce::Colours::white);
        g.drawRect(stageRect, 2.0f);
    }

    void drawOriginMarker(juce::Graphics& g)
    {
        // Draw origin marker at (originWidth, originDepth) which is the (0,0) point
        float originW = getOriginWidth();
        float originD = getOriginDepth();

        auto originScreen = stageToScreen({ originW, originD });

        // Draw crosshairs
        float crosshairLength = 20.0f;
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.drawLine(originScreen.x - crosshairLength, originScreen.y,
                   originScreen.x + crosshairLength, originScreen.y, 2.0f);
        g.drawLine(originScreen.x, originScreen.y - crosshairLength,
                   originScreen.x, originScreen.y + crosshairLength, 2.0f);

        // Draw small circle at origin
        g.drawEllipse(originScreen.x - 5.0f, originScreen.y - 5.0f, 10.0f, 10.0f, 2.0f);
    }

    void drawOutputs(juce::Graphics& g)
    {
        int numOutputs = parameters.getNumOutputChannels();
        float originW = getOriginWidth();
        float originD = getOriginDepth();

        for (int i = 0; i < numOutputs; ++i)
        {
            // Check visibility - individual or array-based
            int array = static_cast<int>(parameters.getOutputParam(i, "outputArray"));
            bool visible = true;

            if (array == 0)
            {
                // Single speaker - check individual visibility
                // For backwards compatibility, treat unset (void) as visible
                auto val = parameters.getOutputParam(i, "outputMapVisible");
                visible = val.isVoid() || static_cast<int>(val) != 0;
            }
            else
            {
                // Part of array - check array visibility
                auto val = parameters.getOutputParam(i, "outputArrayMapVisible");
                visible = val.isVoid() || static_cast<int>(val) != 0;
            }

            if (!visible)
                continue;

            // Output positions are relative to origin, convert to absolute stage coordinates
            float relX = static_cast<float>(parameters.getOutputParam(i, "outputPositionX"));
            float relY = static_cast<float>(parameters.getOutputParam(i, "outputPositionY"));
            float posX = originW + relX;
            float posY = originD + relY;
            int orientation = static_cast<int>(parameters.getOutputParam(i, "outputOrientation"));

            auto screenPos = stageToScreen({ posX, posY });

            // Get array color
            juce::Colour outputColor = getArrayColor(array);

            // Draw speaker wedge shape showing orientation
            juce::Path wedge;
            float wedgeSize = 10.0f;

            // Create wedge pointing in orientation direction
            // Orientation is in degrees, 0 = forward (positive Y in stage coords)
            float angleRad = juce::degreesToRadians(static_cast<float>(orientation) - 90.0f);

            // Wedge points
            float tipX = screenPos.x + std::cos(angleRad) * wedgeSize * 1.5f;
            float tipY = screenPos.y + std::sin(angleRad) * wedgeSize * 1.5f;

            float leftAngle = angleRad + juce::MathConstants<float>::pi * 0.75f;
            float rightAngle = angleRad - juce::MathConstants<float>::pi * 0.75f;

            float leftX = screenPos.x + std::cos(leftAngle) * wedgeSize;
            float leftY = screenPos.y + std::sin(leftAngle) * wedgeSize;
            float rightX = screenPos.x + std::cos(rightAngle) * wedgeSize;
            float rightY = screenPos.y + std::sin(rightAngle) * wedgeSize;

            wedge.startNewSubPath(tipX, tipY);
            wedge.lineTo(leftX, leftY);
            wedge.lineTo(rightX, rightY);
            wedge.closeSubPath();

            g.setColour(outputColor);
            g.fillPath(wedge);
            g.setColour(juce::Colours::white);
            g.strokePath(wedge, juce::PathStrokeType(1.0f));

            // Draw channel number
            g.setFont(10.0f);
            g.setColour(juce::Colours::white);
            g.drawText(juce::String(i + 1),
                       static_cast<int>(screenPos.x) - 8, static_cast<int>(screenPos.y) - 5,
                       16, 10, juce::Justification::centred);
        }
    }

    void drawReverbs(juce::Graphics& g)
    {
        // Check global reverb visibility toggle
        auto reverbsVisibleVar = parameters.getConfigParam("reverbsMapVisible");
        bool reverbsVisible = reverbsVisibleVar.isVoid() || static_cast<int>(reverbsVisibleVar) != 0;
        if (!reverbsVisible)
            return;

        int numReverbs = parameters.getNumReverbChannels();
        float originW = getOriginWidth();
        float originD = getOriginDepth();

        for (int i = 0; i < numReverbs; ++i)
        {
            // Reverb positions are relative to origin, convert to absolute stage coordinates
            float relX = static_cast<float>(parameters.getReverbParam(i, "reverbPositionX"));
            float relY = static_cast<float>(parameters.getReverbParam(i, "reverbPositionY"));
            float posX = originW + relX;
            float posY = originD + relY;

            auto screenPos = stageToScreen({ posX, posY });

            // Draw diamond shape for reverb
            juce::Path diamond;
            float size = 10.0f;
            diamond.startNewSubPath(screenPos.x, screenPos.y - size);
            diamond.lineTo(screenPos.x + size, screenPos.y);
            diamond.lineTo(screenPos.x, screenPos.y + size);
            diamond.lineTo(screenPos.x - size, screenPos.y);
            diamond.closeSubPath();

            g.setColour(juce::Colour(0xFF9C27B0).withAlpha(0.6f));  // Purple, semi-transparent
            g.fillPath(diamond);
            g.setColour(juce::Colour(0xFF9C27B0));
            g.strokePath(diamond, juce::PathStrokeType(1.5f));

            // Draw channel number
            g.setFont(9.0f);
            g.setColour(juce::Colours::white);
            g.drawText("R" + juce::String(i + 1),
                       static_cast<int>(screenPos.x) - 10, static_cast<int>(screenPos.y) - 5,
                       20, 10, juce::Justification::centred);
        }
    }

    void drawClusters(juce::Graphics& g)
    {
        int numInputs = parameters.getNumInputChannels();

        // For each cluster (1-10), collect positions and draw boundary
        for (int cluster = 1; cluster <= 10; ++cluster)
        {
            std::vector<juce::Point<float>> clusterPositions;

            for (int i = 0; i < numInputs; ++i)
            {
                // Skip hidden inputs
                auto visibleVar = parameters.getInputParam(i, "inputMapVisible");
                bool visible = visibleVar.isVoid() || static_cast<int>(visibleVar) != 0;
                if (!visible)
                    continue;

                int inputCluster = static_cast<int>(parameters.getInputParam(i, "inputCluster"));
                if (inputCluster == cluster)
                {
                    float posX = static_cast<float>(parameters.getInputParam(i, "inputPositionX"));
                    float posY = static_cast<float>(parameters.getInputParam(i, "inputPositionY"));
                    clusterPositions.push_back(stageToScreen({ posX, posY }));
                }
            }

            if (clusterPositions.size() >= 2)
            {
                // Draw bounding box around cluster
                float minX = clusterPositions[0].x;
                float maxX = clusterPositions[0].x;
                float minY = clusterPositions[0].y;
                float maxY = clusterPositions[0].y;

                for (const auto& pos : clusterPositions)
                {
                    minX = juce::jmin(minX, pos.x);
                    maxX = juce::jmax(maxX, pos.x);
                    minY = juce::jmin(minY, pos.y);
                    maxY = juce::jmax(maxY, pos.y);
                }

                // Add padding around markers
                float padding = markerRadius + 5.0f;
                juce::Rectangle<float> clusterBounds(minX - padding, minY - padding,
                                                     maxX - minX + padding * 2,
                                                     maxY - minY + padding * 2);

                g.setColour(getMarkerColor(cluster, true).withAlpha(0.2f));
                g.fillRoundedRectangle(clusterBounds, 8.0f);
                g.setColour(getMarkerColor(cluster, true).withAlpha(0.5f));
                g.drawRoundedRectangle(clusterBounds, 8.0f, 1.5f);
            }
        }
    }

    void drawInputs(juce::Graphics& g)
    {
        int numInputs = parameters.getNumInputChannels();

        // Draw inputs in reverse order so lower indices are on top
        for (int i = numInputs - 1; i >= 0; --i)
        {
            // Check visibility
            auto visibleVar = parameters.getInputParam(i, "inputMapVisible");
            bool visible = visibleVar.isVoid() || static_cast<int>(visibleVar) != 0;
            if (!visible)
                continue;

            drawInputMarker(g, i, i == selectedInput);
        }
    }

    void drawInputMarker(juce::Graphics& g, int inputIndex, bool isSelected)
    {
        float posX = static_cast<float>(parameters.getInputParam(inputIndex, "inputPositionX"));
        float posY = static_cast<float>(parameters.getInputParam(inputIndex, "inputPositionY"));
        float posZ = static_cast<float>(parameters.getInputParam(inputIndex, "inputPositionZ"));

        float offsetX = static_cast<float>(parameters.getInputParam(inputIndex, "inputOffsetX"));
        float offsetY = static_cast<float>(parameters.getInputParam(inputIndex, "inputOffsetY"));

        int lsActive = static_cast<int>(parameters.getInputParam(inputIndex, "inputLSactive"));
        float lsRadius = static_cast<float>(parameters.getInputParam(inputIndex, "inputLSradius"));

        auto screenPos = stageToScreen({ posX, posY });

        // Check if input is locked on map
        auto lockedVar = parameters.getInputParam(inputIndex, "inputMapLocked");
        bool isLocked = !lockedVar.isVoid() && static_cast<int>(lockedVar) != 0;
        bool isBeingDragged = isSelected && isDraggingInput;

        // Draw LS radius if active
        if (lsActive != 0)
        {
            float radiusPixels = lsRadius * viewScale;
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.fillEllipse(screenPos.x - radiusPixels, screenPos.y - radiusPixels,
                          radiusPixels * 2, radiusPixels * 2);
            g.setColour(juce::Colours::white.withAlpha(0.3f));
            g.drawEllipse(screenPos.x - radiusPixels, screenPos.y - radiusPixels,
                          radiusPixels * 2, radiusPixels * 2, 1.0f);
        }

        // Draw composite position ring (position + offset) if offset is non-zero
        if (std::abs(offsetX) > 0.01f || std::abs(offsetY) > 0.01f)
        {
            auto compositePos = stageToScreen({ posX + offsetX, posY + offsetY });
            g.setColour(juce::Colours::white.withAlpha(0.5f));
            g.drawEllipse(compositePos.x - markerRadius - 3, compositePos.y - markerRadius - 3,
                          (markerRadius + 3) * 2, (markerRadius + 3) * 2, 1.0f);
        }

        // Determine colors (matching Android app style)
        juce::Colour outerColor;
        juce::Colour labelColor;

        if (isLocked)
        {
            outerColor = juce::Colours::lightgrey;
            labelColor = juce::Colours::red;
        }
        else if (isBeingDragged)
        {
            outerColor = juce::Colours::white;
            labelColor = juce::Colours::white;
        }
        else
        {
            outerColor = getMarkerColor(inputIndex + 1);  // 1-based for color
            labelColor = juce::Colours::white;
        }

        // Draw outer circle
        g.setColour(outerColor);
        g.fillEllipse(screenPos.x - markerRadius, screenPos.y - markerRadius,
                      markerRadius * 2, markerRadius * 2);

        // Draw inner black circle
        float innerRadius = markerRadius * innerRadiusRatio;
        g.setColour(juce::Colours::black);
        g.fillEllipse(screenPos.x - innerRadius, screenPos.y - innerRadius,
                      innerRadius * 2, innerRadius * 2);

        // Draw channel number
        g.setColour(labelColor);
        g.setFont(juce::FontOptions().withHeight(12.0f).withStyle("Bold"));
        g.drawText(juce::String(inputIndex + 1),
                   static_cast<int>(screenPos.x) - static_cast<int>(markerRadius),
                   static_cast<int>(screenPos.y) - 6,
                   static_cast<int>(markerRadius * 2), 12,
                   juce::Justification::centred);

        // Draw height indicator triangle if Z != 0
        if (std::abs(posZ) > 0.01f)
        {
            juce::Path triangle;
            float triSize = 5.0f;
            float triOffset = markerRadius + 4.0f;

            if (posZ > 0)  // Above ground
            {
                triangle.addTriangle(screenPos.x, screenPos.y - triOffset - triSize,
                                     screenPos.x - triSize, screenPos.y - triOffset,
                                     screenPos.x + triSize, screenPos.y - triOffset);
            }
            else  // Below ground
            {
                triangle.addTriangle(screenPos.x, screenPos.y + triOffset + triSize,
                                     screenPos.x - triSize, screenPos.y + triOffset,
                                     screenPos.x + triSize, screenPos.y + triOffset);
            }

            g.setColour(juce::Colours::white);
            g.fillPath(triangle);
        }

        // Selection highlight
        if (isSelected)
        {
            g.setColour(juce::Colours::yellow);
            g.drawEllipse(screenPos.x - markerRadius - 3, screenPos.y - markerRadius - 3,
                          (markerRadius + 3) * 2, (markerRadius + 3) * 2, 2.0f);
        }

        // Show coordinates when dragging
        if (isBeingDragged)
        {
            juce::String coordText = "(" + juce::String(posX, 1) + ", " + juce::String(posY, 1) + ")";
            g.setColour(juce::Colours::yellow);
            g.setFont(10.0f);

            // Position text to the right or left depending on screen position
            if (screenPos.x < getWidth() / 2)
                g.drawText(coordText, static_cast<int>(screenPos.x + markerRadius + 5),
                           static_cast<int>(screenPos.y) - 5, 80, 12, juce::Justification::left);
            else
                g.drawText(coordText, static_cast<int>(screenPos.x - markerRadius - 85),
                           static_cast<int>(screenPos.y) - 5, 80, 12, juce::Justification::right);
        }
    }

    //==========================================================================
    // Hit Testing
    //==========================================================================

    int getInputAtPosition(juce::Point<float> screenPos) const
    {
        int numInputs = parameters.getNumInputChannels();
        float pickupRadius = markerRadius * 1.25f;  // Slightly larger for easier pickup

        // Check in reverse order (top-most first)
        for (int i = 0; i < numInputs; ++i)
        {
            // Skip hidden inputs
            auto visibleVar = parameters.getInputParam(i, "inputMapVisible");
            bool visible = visibleVar.isVoid() || static_cast<int>(visibleVar) != 0;
            if (!visible)
                continue;

            // Skip locked inputs (can't be dragged)
            auto lockedVar = parameters.getInputParam(i, "inputMapLocked");
            bool isLocked = !lockedVar.isVoid() && static_cast<int>(lockedVar) != 0;
            if (isLocked)
                continue;

            float posX = static_cast<float>(parameters.getInputParam(i, "inputPositionX"));
            float posY = static_cast<float>(parameters.getInputParam(i, "inputPositionY"));

            auto markerScreenPos = stageToScreen({ posX, posY });
            float distance = screenPos.getDistanceFrom(markerScreenPos);

            if (distance <= pickupRadius)
                return i;
        }

        return -1;  // No hit
    }

    juce::Point<float> getInputPosition(int inputIndex) const
    {
        float posX = static_cast<float>(parameters.getInputParam(inputIndex, "inputPositionX"));
        float posY = static_cast<float>(parameters.getInputParam(inputIndex, "inputPositionY"));
        return { posX, posY };
    }

    //==========================================================================
    // ValueTree::Listener
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                  const juce::Identifier& property) override
    {
        juce::ignoreUnused(treeWhosePropertyHasChanged, property);

        // Repaint on any property change
        juce::MessageManager::callAsync([this]() {
            repaint();
        });
    }

    void valueTreeChildAdded(juce::ValueTree& parentTree, juce::ValueTree& child) override
    {
        juce::ignoreUnused(parentTree, child);
        juce::MessageManager::callAsync([this]() {
            repaint();
        });
    }

    void valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& child, int index) override
    {
        juce::ignoreUnused(parentTree, child, index);
        juce::MessageManager::callAsync([this]() {
            selectedInput = -1;  // Clear selection if channel removed
            repaint();
        });
    }

    void valueTreeChildOrderChanged(juce::ValueTree& parentTree, int oldIndex, int newIndex) override
    {
        juce::ignoreUnused(parentTree, oldIndex, newIndex);
    }

    void valueTreeParentChanged(juce::ValueTree& tree) override
    {
        juce::ignoreUnused(tree);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MapTab)
};
