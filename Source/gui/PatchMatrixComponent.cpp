#include "PatchMatrixComponent.h"
#include "../Parameters/WFSParameterIDs.h"
#include "ColorUtilities.h"

PatchMatrixComponent::PatchMatrixComponent(WFSValueTreeState& valueTreeState,
                                           bool inputPatch,
                                           TestSignalGenerator* testSignalGen)
    : parameters(valueTreeState),
      isInputPatch(inputPatch),
      testSignalGenerator(testSignalGen)
{
    // Get reference to appropriate patch tree
    auto audioPatchTree = parameters.getState().getChildWithName(WFSParameterIDs::AudioPatch);
    if (isInputPatch)
        patchTree = audioPatchTree.getChildWithName(WFSParameterIDs::InputPatch);
    else
        patchTree = audioPatchTree.getChildWithName(WFSParameterIDs::OutputPatch);

    // Get reference to Inputs or Outputs tree (to listen for channel count changes)
    if (isInputPatch)
        channelsTree = parameters.getState().getChildWithName(WFSParameterIDs::Inputs);
    else
        channelsTree = parameters.getState().getChildWithName(WFSParameterIDs::Outputs);

    // Listen for channel count changes
    patchTree.addListener(this);
    channelsTree.addListener(this);  // Listen for input/output additions/removals

    // Add scrollbars
    addAndMakeVisible(horizontalScroll);
    addAndMakeVisible(verticalScroll);

    horizontalScroll.addListener(this);
    verticalScroll.addListener(this);

    // Disable mouse activity effects
    setRepaintsOnMouseActivity(false);
    setMouseClickGrabsKeyboardFocus(false);
    setWantsKeyboardFocus(false);

    // Enable double buffering for smooth scrolling
    setBufferedToImage(true);
    setOpaque(true);  // Component is fully opaque, enables rendering optimizations

    // Load initial state
    updateChannelCounts();
    loadPatchesFromValueTree();
}

PatchMatrixComponent::~PatchMatrixComponent()
{
    patchTree.removeListener(this);
    channelsTree.removeListener(this);
    horizontalScroll.removeListener(this);
    verticalScroll.removeListener(this);
}

void PatchMatrixComponent::setMode(Mode newMode)
{
    if (currentMode != newMode)
    {
        // Cancel any ongoing operations
        cancelPatchOperation();

        // Stop test signals when leaving testing mode
        if (currentMode == Mode::Testing && testSignalGenerator)
        {
            testSignalGenerator->reset();
            activeTestHardwareChannel = -1;
        }

        currentMode = newMode;
        repaint();
    }
}

void PatchMatrixComponent::loadPatchesFromValueTree()
{
    patches.clear();

    if (!patchTree.isValid())
        return;

    juce::String patchDataStr = patchTree.getProperty(WFSParameterIDs::patchData).toString();
    juce::StringArray rows = juce::StringArray::fromTokens(patchDataStr, ";", "");

    for (int r = 0; r < rows.size() && r < numWFSChannels; ++r)
    {
        juce::StringArray cols = juce::StringArray::fromTokens(rows[r], ",", "");
        for (int c = 0; c < cols.size() && c < numHardwareChannels; ++c)
        {
            if (cols[c].getIntValue() == 1)
            {
                patches.push_back({r, c});
            }
        }
    }

    repaint();
}

void PatchMatrixComponent::savePatchesToValueTree()
{
    if (!patchTree.isValid())
        return;

    // Create 2D matrix
    std::vector<std::vector<int>> matrix(numWFSChannels, std::vector<int>(numHardwareChannels, 0));

    for (const auto& patch : patches)
    {
        if (patch.wfsChannel < numWFSChannels && patch.hardwareChannel < numHardwareChannels)
        {
            matrix[patch.wfsChannel][patch.hardwareChannel] = 1;
        }
    }

    // Convert to string format
    juce::StringArray rowStrings;
    for (const auto& row : matrix)
    {
        juce::StringArray colStrings;
        for (int val : row)
        {
            colStrings.add(juce::String(val));
        }
        rowStrings.add(colStrings.joinIntoString(","));
    }

    juce::String patchDataStr = rowStrings.joinIntoString(";");
    patchTree.setProperty(WFSParameterIDs::patchData, patchDataStr, nullptr);
}

void PatchMatrixComponent::clearAllPatches()
{
    DBG("PatchMatrixComponent::clearAllPatches() - patches.size() before = " + juce::String(patches.size()));
    patches.clear();
    DBG("PatchMatrixComponent::clearAllPatches() - patches.size() after = " + juce::String(patches.size()));
    savePatchesToValueTree();
    DBG("PatchMatrixComponent::clearAllPatches() - saved to ValueTree");
    repaint();
}

bool PatchMatrixComponent::isWFSChannelPatched(int wfsChannel) const
{
    for (const auto& patch : patches)
    {
        if (patch.wfsChannel == wfsChannel)
            return true;
    }
    return false;
}

int PatchMatrixComponent::getHardwareChannelForWFS(int wfsChannel) const
{
    for (const auto& patch : patches)
    {
        if (patch.wfsChannel == wfsChannel)
            return patch.hardwareChannel;
    }
    return -1;
}

void PatchMatrixComponent::setProcessingStateChanged(bool isProcessing)
{
    // Stop test signals when WFS processing starts
    if (isProcessing && testSignalGenerator)
    {
        testSignalGenerator->reset();
        activeTestHardwareChannel = -1;
    }
}

void PatchMatrixComponent::clearActiveTestChannel()
{
    if (testSignalGenerator)
    {
        testSignalGenerator->setOutputChannel(-1);
    }
    activeTestHardwareChannel = -1;
    repaint();
}

void PatchMatrixComponent::paint(juce::Graphics& g)
{
    g.fillAll(backgroundColour);

    // Draw cells, clipped to the content area to prevent overlap with headers
    {
        juce::Graphics::ScopedSaveState saveState(g);
        g.reduceClipRegion(rowHeaderWidth, headerHeight,
                           getWidth() - rowHeaderWidth - scrollBarThickness,
                           getHeight() - headerHeight - scrollBarThickness);
        drawCells(g);
    }

    // Draw column header, clipped to exclude row header area
    {
        juce::Graphics::ScopedSaveState saveState(g);
        g.reduceClipRegion(rowHeaderWidth, 0,
                           getWidth() - rowHeaderWidth - scrollBarThickness,
                           headerHeight);
        drawHeader(g);
    }

    // Draw row headers (no clipping needed, they're at fixed position)
    drawRowHeaders(g);
}

void PatchMatrixComponent::resized()
{
    updateScrollBars();

    // Position scrollbars
    auto bounds = getLocalBounds();

    auto bottomBar = bounds.removeFromBottom(scrollBarThickness);
    horizontalScroll.setBounds(bottomBar.removeFromLeft(bounds.getWidth() - scrollBarThickness));

    auto rightBar = bounds.removeFromRight(scrollBarThickness);
    verticalScroll.setBounds(rightBar.removeFromTop(bounds.getHeight()));
}

void PatchMatrixComponent::mouseDown(const juce::MouseEvent& e)
{
    if (e.source.isTouch())
        touchFingerCount++;

    // Determine if this is a scroll gesture
    // In Patching/Testing modes: 2+ fingers = scroll, 1 finger = action
    bool isScrollGesture = currentMode == Mode::Scrolling ||
                          e.mods.isRightButtonDown() ||
                          touchFingerCount >= 2;

    if (isScrollGesture)
    {
        dragStartPos = e.position.toInt();
        scrollStartOffset = juce::Point<int>(scrollOffsetX, scrollOffsetY);
        isDraggingToScroll = true;
        scrollDragSourceIndex = e.source.getIndex();  // Track which source initiated scroll
        return;
    }

    // Patching or testing mode with left click
    if (e.mods.isLeftButtonDown())
    {
        auto cell = getCellAtPosition(e.position);

        if (currentMode == Mode::Patching && cell.x >= 0 && cell.y >= 0)
        {
            startPatchOperation(cell);
        }
        else if (currentMode == Mode::Testing)
        {
            // In testing mode, clicking:
            // - Column header (cell.y == -1): play test on that hardware channel
            // - Row header (cell.x == -1): play test on patched hardware channel for that WFS channel
            // - Cell: play test on that hardware channel

            int targetChannel = -1;

            if (cell.y == -1 && cell.x >= 0)
            {
                // Clicked on column header - use hardware channel directly
                targetChannel = cell.x;
            }
            else if (cell.x == -1 && cell.y >= 0)
            {
                // Clicked on row header - use patched hardware channel for this WFS channel
                targetChannel = getHardwareChannelForWFS(cell.y);
            }
            else if (cell.x >= 0 && cell.y >= 0)
            {
                // Clicked on cell - use hardware channel
                targetChannel = cell.x;
            }

            if (targetChannel >= 0)
            {
                handleTestClick(targetChannel);
            }
        }
    }
}

void PatchMatrixComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (isDraggingToScroll)
    {
        // Only respond to the touch source that initiated the scroll (prevents jumping)
        if (e.source.isTouch() && e.source.getIndex() != scrollDragSourceIndex)
            return;

        // Scroll viewport
        auto delta = e.position.toInt() - dragStartPos;
        scrollOffsetX = juce::jlimit(0, maxScrollX, scrollStartOffset.x - delta.x);
        scrollOffsetY = juce::jlimit(0, maxScrollY, scrollStartOffset.y - delta.y);

        updateScrollBars();
        repaint();
        return;
    }

    if (currentMode == Mode::Patching && patchDragState.isActive)
    {
        // Update patch drag
        auto cell = getCellAtPosition(e.position);
        updatePatchDrag(cell);
    }
}

void PatchMatrixComponent::mouseUp(const juce::MouseEvent& e)
{
    if (e.source.isTouch())
        touchFingerCount = juce::jmax(0, touchFingerCount - 1);

    if (isDraggingToScroll)
    {
        isDraggingToScroll = false;
        scrollDragSourceIndex = -1;  // Reset source tracking
        return;
    }

    if (currentMode == Mode::Patching && patchDragState.isActive)
    {
        commitPatchOperation();
    }

    // In testing mode, stop test signal on release unless hold is enabled
    if (currentMode == Mode::Testing && testSignalGenerator)
    {
        if (!testSignalGenerator->isHoldEnabled())
        {
            testSignalGenerator->setOutputChannel(-1);
            activeTestHardwareChannel = -1;
            repaint();
        }
    }
}

void PatchMatrixComponent::mouseMove(const juce::MouseEvent& e)
{
    auto newHoveredCell = getCellAtPosition(e.position);

    if (newHoveredCell != hoveredCell)
    {
        hoveredCell = newHoveredCell;
        repaint();
    }
}

void PatchMatrixComponent::mouseExit(const juce::MouseEvent&)
{
    hoveredCell = {-1, -1};
    repaint();
}

void PatchMatrixComponent::mouseWheelMove(const juce::MouseEvent& event,
                                          const juce::MouseWheelDetails& wheel)
{
    // Vertical scroll by default, horizontal with shift
    if (event.mods.isShiftDown())
    {
        scrollOffsetX = juce::jlimit(0, maxScrollX,
                                    scrollOffsetX - static_cast<int>(wheel.deltaX * cellWidth * 3));
    }
    else
    {
        scrollOffsetY = juce::jlimit(0, maxScrollY,
                                    scrollOffsetY - static_cast<int>(wheel.deltaY * cellHeight * 3));
    }

    updateScrollBars();
    repaint();
}

void PatchMatrixComponent::scrollBarMoved(juce::ScrollBar* bar, double newRangeStart)
{
    if (bar == &horizontalScroll)
    {
        scrollOffsetX = static_cast<int>(newRangeStart);
    }
    else if (bar == &verticalScroll)
    {
        scrollOffsetY = static_cast<int>(newRangeStart);
    }

    repaint();
}

void PatchMatrixComponent::valueTreePropertyChanged(juce::ValueTree& tree,
                                                    const juce::Identifier& property)
{
    if (property == WFSParameterIDs::rows || property == WFSParameterIDs::cols)
    {
        updateChannelCounts();

        // Remove invalid patches (beyond new bounds)
        patches.erase(
            std::remove_if(patches.begin(), patches.end(),
                [this](const PatchPoint& p) {
                    return p.wfsChannel >= numWFSChannels ||
                           p.hardwareChannel >= numHardwareChannels;
                }),
            patches.end()
        );

        savePatchesToValueTree();
        updateScrollBars();
        repaint();
    }
}

void PatchMatrixComponent::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    // Check if this is an input/output channel being added
    if (parent == channelsTree)
    {
        updateChannelCounts();
        repaint();
    }
}

void PatchMatrixComponent::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    // Check if this is an input/output channel being removed
    if (parent == channelsTree)
    {
        updateChannelCounts();

        // Remove invalid patches (beyond new bounds)
        patches.erase(
            std::remove_if(patches.begin(), patches.end(),
                [this](const PatchPoint& p) {
                    return p.wfsChannel >= numWFSChannels ||
                           p.hardwareChannel >= numHardwareChannels;
                }),
            patches.end()
        );

        savePatchesToValueTree();
        repaint();
    }
}

//==============================================================================
// Helper Methods

void PatchMatrixComponent::updateScrollBars()
{
    int visibleWidth = getWidth() - rowHeaderWidth - scrollBarThickness;
    int visibleHeight = getHeight() - headerHeight - scrollBarThickness;

    int totalWidth = numHardwareChannels * cellWidth;
    int totalHeight = numWFSChannels * cellHeight;

    maxScrollX = juce::jmax(0, totalWidth - visibleWidth);
    maxScrollY = juce::jmax(0, totalHeight - visibleHeight);

    scrollOffsetX = juce::jlimit(0, maxScrollX, scrollOffsetX);
    scrollOffsetY = juce::jlimit(0, maxScrollY, scrollOffsetY);

    horizontalScroll.setRangeLimits(0, totalWidth);
    horizontalScroll.setCurrentRange(scrollOffsetX, visibleWidth);
    horizontalScroll.setVisible(totalWidth > visibleWidth);

    verticalScroll.setRangeLimits(0, totalHeight);
    verticalScroll.setCurrentRange(scrollOffsetY, visibleHeight);
    verticalScroll.setVisible(totalHeight > visibleHeight);
}

void PatchMatrixComponent::updateChannelCounts()
{
    // Get WFS channel count from actual configured inputs/outputs
    if (isInputPatch)
        numWFSChannels = parameters.getNumInputChannels();
    else
        numWFSChannels = parameters.getNumOutputChannels();

    // Hardware channels: use max (64) or read from patch tree if specified
    if (patchTree.isValid())
    {
        numHardwareChannels = patchTree.getProperty(WFSParameterIDs::cols, 64);
    }
    else
    {
        numHardwareChannels = 64;  // Maximum hardware channels
    }

    // Update scrollbars for new dimensions
    updateScrollBars();
}

juce::Point<int> PatchMatrixComponent::getCellAtPosition(juce::Point<float> pos) const
{
    int col = -1;
    int row = -1;

    // Check if in header area (hardware channels)
    if (pos.x >= rowHeaderWidth && pos.y < headerHeight)
    {
        col = static_cast<int>((pos.x - rowHeaderWidth + scrollOffsetX) / cellWidth);
        if (col >= 0 && col < numHardwareChannels)
            return {col, -1};  // Header cell (col, row=-1)
    }

    // Check if in row header area (WFS channels)
    if (pos.x < rowHeaderWidth && pos.y >= headerHeight)
    {
        row = static_cast<int>((pos.y - headerHeight + scrollOffsetY) / cellHeight);
        if (row >= 0 && row < numWFSChannels)
            return {-1, row};  // Row header (col=-1, row)
    }

    // Check if in cell area
    if (pos.x >= rowHeaderWidth && pos.y >= headerHeight)
    {
        col = static_cast<int>((pos.x - rowHeaderWidth + scrollOffsetX) / cellWidth);
        row = static_cast<int>((pos.y - headerHeight + scrollOffsetY) / cellHeight);

        if (col >= 0 && col < numHardwareChannels && row >= 0 && row < numWFSChannels)
            return {col, row};
    }

    return {-1, -1};  // Not in a valid cell
}

bool PatchMatrixComponent::isCellVisible(int row, int col) const
{
    int cellX = col * cellWidth - scrollOffsetX;
    int cellY = row * cellHeight - scrollOffsetY;

    int visibleWidth = getWidth() - rowHeaderWidth - scrollBarThickness;
    int visibleHeight = getHeight() - headerHeight - scrollBarThickness;

    return cellX >= -cellWidth && cellX < visibleWidth &&
           cellY >= -cellHeight && cellY < visibleHeight;
}

juce::Rectangle<int> PatchMatrixComponent::getCellBounds(int row, int col) const
{
    int x = rowHeaderWidth + col * cellWidth - scrollOffsetX;
    int y = headerHeight + row * cellHeight - scrollOffsetY;

    return juce::Rectangle<int>(x, y, cellWidth, cellHeight);
}

//==============================================================================
// Drawing Methods

void PatchMatrixComponent::drawHeader(juce::Graphics& g)
{
    // Draw header background
    g.setColour(headerColour);
    g.fillRect(rowHeaderWidth, 0, getWidth() - rowHeaderWidth, headerHeight);

    // Draw hardware channel numbers
    int visibleCols = (getWidth() - rowHeaderWidth) / cellWidth + 2;
    int firstCol = scrollOffsetX / cellWidth;

    g.setColour(juce::Colours::white);
    g.setFont(14.0f);

    for (int c = 0; c < visibleCols; ++c)
    {
        int col = firstCol + c;
        if (col >= numHardwareChannels)
            break;

        int x = rowHeaderWidth + c * cellWidth - (scrollOffsetX % cellWidth);

        // Highlight if this is the active test channel (output patch only)
        if (!isInputPatch && currentMode == Mode::Testing && col == activeTestHardwareChannel)
        {
            g.setColour(juce::Colours::green.withAlpha(0.3f));
            g.fillRect(x, 0, cellWidth, headerHeight);
        }
        // Highlight if hovered
        else if (hoveredCell.x == col && currentMode == Mode::Testing)
        {
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.fillRect(x, 0, cellWidth, headerHeight);
        }

        g.setColour(juce::Colours::white);
        g.drawText(juce::String(col + 1), x, 0, cellWidth, headerHeight,
                   juce::Justification::centred);

        // Draw grid line
        g.setColour(gridLineColour);
        g.drawVerticalLine(x, 0, static_cast<float>(headerHeight));
    }
}

void PatchMatrixComponent::drawRowHeaders(juce::Graphics& g)
{
    // Draw row header background
    g.setColour(headerColour);
    g.fillRect(0, headerHeight, rowHeaderWidth, getHeight() - headerHeight);

    // Draw WFS channel labels
    int visibleRows = (getHeight() - headerHeight) / cellHeight + 2;
    int firstRow = scrollOffsetY / cellHeight;

    g.setFont(12.0f);

    for (int r = 0; r < visibleRows; ++r)
    {
        int row = firstRow + r;
        if (row >= numWFSChannels)
            break;

        int y = headerHeight + r * cellHeight - (scrollOffsetY % cellHeight);

        // Get channel name
        juce::String channelName;
        if (isInputPatch)
        {
            // Get input by index (0-based) - each child has type "Input"
            auto inputsTree = parameters.getState().getChildWithName(WFSParameterIDs::Inputs);
            if (row < inputsTree.getNumChildren())
            {
                auto inputTree = inputsTree.getChild(row);
                auto channelTree = inputTree.getChildWithName(WFSParameterIDs::Channel);
                if (channelTree.isValid())
                    channelName = channelTree.getProperty(WFSParameterIDs::inputName).toString();
            }
        }
        else
        {
            // Get output by index (0-based) - each child has type "Output"
            auto outputsTree = parameters.getState().getChildWithName(WFSParameterIDs::Outputs);
            if (row < outputsTree.getNumChildren())
            {
                auto outputTree = outputsTree.getChild(row);
                auto channelTree = outputTree.getChildWithName(WFSParameterIDs::Channel);
                if (channelTree.isValid())
                    channelName = channelTree.getProperty(WFSParameterIDs::outputName).toString();
            }
        }

        juce::String label = juce::String(row + 1) + " " + channelName;

        // Check if this channel is patched
        bool isPatched = isWFSChannelPatched(row);
        int hwChannel = isPatched ? getHardwareChannelForWFS(row) : -1;

        // Highlight background if active test channel (output patch only)
        if (!isInputPatch && currentMode == Mode::Testing &&
            activeTestHardwareChannel >= 0 && hwChannel == activeTestHardwareChannel)
        {
            g.setColour(juce::Colours::green.withAlpha(0.3f));
            g.fillRect(0, y, rowHeaderWidth, cellHeight);
        }

        // Set text color - orange for unpatched, white for patched
        if (!isPatched)
        {
            g.setColour(juce::Colours::orange);
        }
        else
        {
            g.setColour(juce::Colours::white);
        }

        g.drawText(label, 5, y, rowHeaderWidth - 10, cellHeight,
                   juce::Justification::centredLeft);

        // Draw grid line
        g.setColour(gridLineColour);
        g.drawHorizontalLine(y, 0, static_cast<float>(rowHeaderWidth));
    }
}

void PatchMatrixComponent::drawCells(juce::Graphics& g)
{
    int visibleCols = (getWidth() - rowHeaderWidth) / cellWidth + 2;
    int visibleRows = (getHeight() - headerHeight) / cellHeight + 2;
    int firstCol = scrollOffsetX / cellWidth;
    int firstRow = scrollOffsetY / cellHeight;

    for (int r = 0; r < visibleRows; ++r)
    {
        for (int c = 0; c < visibleCols; ++c)
        {
            int row = firstRow + r;
            int col = firstCol + c;

            if (row >= numWFSChannels || col >= numHardwareChannels)
                continue;

            auto bounds = getCellBounds(row, col);
            drawCell(g, row, col, bounds);
        }
    }
}

void PatchMatrixComponent::drawCell(juce::Graphics& g, int row, int col,
                                    juce::Rectangle<int> bounds)
{
    bool isPatched = isPatchActive(row, col);
    bool isPreview = false;

    // Check if in preview (during drag)
    if (patchDragState.isActive)
    {
        for (const auto& previewPatch : patchDragState.previewPatches)
        {
            if (previewPatch.wfsChannel == row && previewPatch.hardwareChannel == col)
            {
                isPreview = true;
                break;
            }
        }
    }

    if (isPatched || isPreview)
    {
        // Use color based on channel
        juce::Colour cellColor = getCellColor(row);

        if (isPreview)
            cellColor = cellColor.withAlpha(0.6f);

        g.setColour(cellColor);
        g.fillRect(bounds);

        // Draw hardware channel number
        g.setColour(WfsColorUtilities::getContrastingTextColor(cellColor));
        g.setFont(14.0f);
        g.drawText(juce::String(col + 1), bounds, juce::Justification::centred);
    }
    else
    {
        g.setColour(emptyCellColour);
        g.fillRect(bounds);
    }

    // Hover highlight (in patching mode only)
    if (currentMode == Mode::Patching &&
        hoveredCell.x == col && hoveredCell.y == row)
    {
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.fillRect(bounds);
    }

    // Active test highlight (in testing mode) - highlight the patch that's being tested
    if (!isInputPatch && currentMode == Mode::Testing &&
        isPatched && col == activeTestHardwareChannel)
    {
        g.setColour(juce::Colours::green.withAlpha(0.5f));
        g.drawRect(bounds, 3);  // Thick border for emphasis
    }

    // Draw grid lines
    g.setColour(gridLineColour);
    g.drawRect(bounds, 1);
}

//==============================================================================
// Patching Logic

void PatchMatrixComponent::startPatchOperation(juce::Point<int> cell)
{
    patchDragState.startCell = cell;
    patchDragState.currentCell = cell;
    patchDragState.isActive = true;

    // Check if clicking on existing patch to remove it
    if (isPatchActive(cell.y, cell.x))
    {
        // Single click to remove
        patches.erase(
            std::remove_if(patches.begin(), patches.end(),
                [cell](const PatchPoint& p) {
                    return p.wfsChannel == cell.y && p.hardwareChannel == cell.x;
                }),
            patches.end()
        );

        patchDragState.isActive = false;
        savePatchesToValueTree();
        repaint();
    }
    else
    {
        // Start drag for new patch
        updatePatchDrag(cell);
    }
}

void PatchMatrixComponent::updatePatchDrag(juce::Point<int> currentCell)
{
    if (!patchDragState.isActive || currentCell.x < 0 || currentCell.y < 0)
        return;

    patchDragState.currentCell = currentCell;

    // Calculate diagonal patches from start to current
    int deltaRow = currentCell.y - patchDragState.startCell.y;
    int deltaCol = currentCell.x - patchDragState.startCell.x;

    int steps = juce::jmax(std::abs(deltaRow), std::abs(deltaCol));

    patchDragState.previewPatches.clear();

    for (int i = 0; i <= steps; ++i)
    {
        int row = patchDragState.startCell.y;
        int col = patchDragState.startCell.x;

        if (steps > 0)
        {
            row += (deltaRow * i) / steps;
            col += (deltaCol * i) / steps;
        }

        if (row >= 0 && row < numWFSChannels &&
            col >= 0 && col < numHardwareChannels &&
            isValidPatch(row, col))
        {
            patchDragState.previewPatches.push_back({row, col});
        }
    }

    repaint();
}

void PatchMatrixComponent::commitPatchOperation()
{
    if (!patchDragState.isActive)
        return;

    // Remove conflicts and add new patches
    for (const auto& newPatch : patchDragState.previewPatches)
    {
        // Remove any existing patches that conflict
        patches.erase(
            std::remove_if(patches.begin(), patches.end(),
                [newPatch](const PatchPoint& p) {
                    return p.wfsChannel == newPatch.wfsChannel ||
                           p.hardwareChannel == newPatch.hardwareChannel;
                }),
            patches.end()
        );

        // Add new patch
        patches.push_back(newPatch);
    }

    savePatchesToValueTree();
    cancelPatchOperation();
}

void PatchMatrixComponent::cancelPatchOperation()
{
    patchDragState.isActive = false;
    patchDragState.startCell = {-1, -1};
    patchDragState.currentCell = {-1, -1};
    patchDragState.previewPatches.clear();
    repaint();
}

bool PatchMatrixComponent::isPatchActive(int wfsChannel, int hwChannel) const
{
    for (const auto& patch : patches)
    {
        if (patch.wfsChannel == wfsChannel && patch.hardwareChannel == hwChannel)
            return true;
    }
    return false;
}

bool PatchMatrixComponent::isValidPatch(int wfsChannel, int hwChannel) const
{
    // Check 1:1 constraint
    for (const auto& patch : patches)
    {
        if (patch.wfsChannel == wfsChannel && patch.hardwareChannel != hwChannel)
            return false;  // WFS channel already mapped elsewhere
        if (patch.hardwareChannel == hwChannel && patch.wfsChannel != wfsChannel)
            return false;  // Hardware channel already mapped elsewhere
    }
    return true;
}

juce::Colour PatchMatrixComponent::getCellColor(int wfsChannel) const
{
    if (isInputPatch)
    {
        return WfsColorUtilities::getInputColor(wfsChannel + 1);
    }
    else
    {
        // For outputs, use array color
        auto outputTree = parameters.getState().getChildWithName(WFSParameterIDs::Outputs)
                            .getChildWithName(WFSParameterIDs::Output + juce::String(wfsChannel + 1));
        if (outputTree.isValid())
        {
            auto channelTree = outputTree.getChildWithName(WFSParameterIDs::Channel);
            int arrayNum = channelTree.getProperty(WFSParameterIDs::outputArray);
            return WfsColorUtilities::getArrayColor(arrayNum);
        }
    }

    return juce::Colours::grey;
}

void PatchMatrixComponent::handleTestClick(int hardwareChannel)
{
    if (!testSignalGenerator || hardwareChannel < 0 || hardwareChannel >= numHardwareChannels)
        return;

    // Toggle behavior: if clicking on already-active channel, stop the test signal
    if (hardwareChannel == activeTestHardwareChannel)
    {
        testSignalGenerator->setOutputChannel(-1);
        activeTestHardwareChannel = -1;
        repaint();
        return;
    }

    // Set test signal to this channel
    // User must manually select signal type and level from control panel for safety
    testSignalGenerator->setOutputChannel(hardwareChannel);

    // Track active channel for highlighting
    activeTestHardwareChannel = hardwareChannel;
    repaint();
}
