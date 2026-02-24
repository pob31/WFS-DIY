#include "PatchMatrixComponent.h"
#include "../Parameters/WFSParameterIDs.h"
#include "ColorUtilities.h"
#include "../Accessibility/TTSManager.h"
#include "../Localization/LocalizationManager.h"
#include <cmath>

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

    // For output patch, listen to binaural channel changes
    if (!isInputPatch)
    {
        auto configTree = parameters.getState().getChildWithName(WFSParameterIDs::Config);
        binauralTree = configTree.getChildWithName(WFSParameterIDs::Binaural);
        binauralTree.addListener(this);
        updateBinauralChannels();
    }

    // Add scrollbars
    addAndMakeVisible(horizontalScroll);
    addAndMakeVisible(verticalScroll);

    horizontalScroll.addListener(this);
    verticalScroll.addListener(this);

    // Disable mouse activity effects
    setRepaintsOnMouseActivity(false);
    setMouseClickGrabsKeyboardFocus(true);  // Allow clicking to grab focus for keyboard navigation
    setWantsKeyboardFocus(true);  // Enable keyboard navigation

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
    if (binauralTree.isValid())
        binauralTree.removeListener(this);
    horizontalScroll.removeListener(this);
    verticalScroll.removeListener(this);
}

void PatchMatrixComponent::setMode(Mode newMode)
{
    if (currentMode != newMode)
    {
        // Cancel any ongoing operations
        cancelPatchOperation();

        // Stop test audio when leaving testing mode (but keep settings for quick re-testing)
        if (currentMode == Mode::Testing && testSignalGenerator)
        {
            testSignalGenerator->setOutputChannel(-1);
            activeTestHardwareChannel = -1;
            spacebarTestActive = false;
        }

        currentMode = newMode;
        repaint();

        // TTS: Announce mode change and provide hints
        juce::String modeAnnouncement;
        if (newMode == Mode::Patching)
        {
            modeAnnouncement = isInputPatch ? "Input patching mode" : "Output patching mode";
            modeAnnouncement += ". Drag to create patches.";
        }
        else if (newMode == Mode::Testing && !isInputPatch)
        {
            modeAnnouncement = "Output testing mode. Click a patched cell to test.";

            // Hint about test signal settings if not configured
            if (testSignalGenerator)
            {
                auto signalType = testSignalGenerator->getSignalType();
                float levelDb = testSignalGenerator->getLevelDb();

                if (signalType == TestSignalGenerator::SignalType::Off)
                    modeAnnouncement += " Warning: Test signal is OFF. Select a signal type.";
                else if (levelDb <= -92.0f)
                    modeAnnouncement += " Warning: Test level is very low.";
            }
        }
        else if (newMode == Mode::Scrolling)
        {
            modeAnnouncement = "Scrolling mode.";
        }

        if (modeAnnouncement.isNotEmpty())
            TTSManager::getInstance().announceImmediate(modeAnnouncement,
                juce::AccessibilityHandler::AnnouncementPriority::medium);
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
        spacebarTestActive = false;
    }
}

void PatchMatrixComponent::clearActiveTestChannel()
{
    if (testSignalGenerator)
    {
        testSignalGenerator->setOutputChannel(-1);
    }
    activeTestHardwareChannel = -1;
    spacebarTestActive = false;
    repaint();
}

void PatchMatrixComponent::paint(juce::Graphics& g)
{
    g.fillAll(ColorScheme::get().background);

    // Draw title label row (Audio Interface Input/Output)
    {
        g.setColour(ColorScheme::get().backgroundAlt);
        g.fillRect(0, 0, getWidth(), titleHeight);

        g.setColour(ColorScheme::get().textSecondary);
        g.setFont(juce::FontOptions(juce::jmax(9.0f, 12.0f * WfsLookAndFeel::uiScale)).withStyle("Bold"));

        g.drawText(isInputPatch ? LOC("audioPatch.labels.interfaceInput")
                                : LOC("audioPatch.labels.interfaceOutput"),
                   rowHeaderWidth + 5, 0,
                   getWidth() - rowHeaderWidth - scrollBarThickness - 10, titleHeight,
                   juce::Justification::centredLeft, true);

        g.setColour(ColorScheme::get().chromeDivider);
        g.drawHorizontalLine(titleHeight - 1, 0.0f, static_cast<float>(getWidth()));
    }

    // Draw "Processor Inputs/Outputs" label in top-left corner cell (above row headers)
    {
        g.setColour(ColorScheme::get().textSecondary);
        g.setFont(juce::FontOptions(juce::jmax(9.0f, 12.0f * WfsLookAndFeel::uiScale)).withStyle("Bold"));

        g.drawText(isInputPatch ? LOC("audioPatch.labels.processorInputs")
                                : LOC("audioPatch.labels.processorOutputs"),
                   5, titleHeight, rowHeaderWidth - 10, headerHeight,
                   juce::Justification::bottomLeft, true);
    }

    // Draw cells, clipped to the content area to prevent overlap with headers
    {
        juce::Graphics::ScopedSaveState saveState(g);
        g.reduceClipRegion(rowHeaderWidth, contentTop,
                           getWidth() - rowHeaderWidth - scrollBarThickness,
                           getHeight() - contentTop - scrollBarThickness);
        drawCells(g);
    }

    // Draw column header, clipped to exclude row header area
    {
        juce::Graphics::ScopedSaveState saveState(g);
        g.reduceClipRegion(rowHeaderWidth, titleHeight,
                           getWidth() - rowHeaderWidth - scrollBarThickness,
                           headerHeight);
        drawHeader(g);
    }

    // Draw row headers (no clipping needed, they're at fixed position)
    drawRowHeaders(g);
}

void PatchMatrixComponent::resized()
{
    updateScaledSizes();
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

        // TTS: Announce cell info for accessibility (in patching or testing mode)
        if (hoveredCell.x >= 0 && hoveredCell.y >= 0 &&
            (currentMode == Mode::Patching || currentMode == Mode::Testing))
        {
            int wfsChannel = hoveredCell.y;  // Row = WFS channel
            int hwChannel = hoveredCell.x;   // Column = hardware channel

            // Get WFS channel name
            juce::String channelName;
            juce::String channelType = isInputPatch ? "Input" : "Output";

            if (isInputPatch)
            {
                auto inputsTree = parameters.getState().getChildWithName(WFSParameterIDs::Inputs);
                if (wfsChannel < inputsTree.getNumChildren())
                {
                    auto inputTree = inputsTree.getChild(wfsChannel);
                    auto channelTree = inputTree.getChildWithName(WFSParameterIDs::Channel);
                    if (channelTree.isValid())
                        channelName = channelTree.getProperty(WFSParameterIDs::inputName).toString();
                }
            }
            else
            {
                auto outputsTree = parameters.getState().getChildWithName(WFSParameterIDs::Outputs);
                if (wfsChannel < outputsTree.getNumChildren())
                {
                    auto outputTree = outputsTree.getChild(wfsChannel);
                    auto channelTree = outputTree.getChildWithName(WFSParameterIDs::Channel);
                    if (channelTree.isValid())
                        channelName = channelTree.getProperty(WFSParameterIDs::outputName).toString();
                }
            }

            // Build announcement based on mode
            juce::String announcement;

            if (currentMode == Mode::Testing)
            {
                // Testing mode: announce what would be tested
                if (isPatchActive(wfsChannel, hwChannel))
                {
                    announcement = "Test " + channelType + " " + juce::String(wfsChannel + 1);
                    if (channelName.isNotEmpty())
                        announcement += ", " + channelName;
                    announcement += " on audio interface channel " + juce::String(hwChannel + 1);
                }
                else
                {
                    // Not patched - can't test
                    announcement = channelType + " " + juce::String(wfsChannel + 1) +
                                   " not patched to channel " + juce::String(hwChannel + 1);
                }
            }
            else if (currentMode == Mode::Patching)
            {
                // Patching mode: announce current status and action
                int existingHwChannel = getHardwareChannelForWFS(wfsChannel);

                if (existingHwChannel == hwChannel)
                {
                    // Already patched here
                    announcement = channelType + " " + juce::String(wfsChannel + 1);
                    if (channelName.isNotEmpty())
                        announcement += ", " + channelName;
                    announcement += " patched to audio interface channel " + juce::String(hwChannel + 1);
                }
                else if (existingHwChannel >= 0)
                {
                    // Patched elsewhere
                    announcement = channelType + " " + juce::String(wfsChannel + 1);
                    if (channelName.isNotEmpty())
                        announcement += ", " + channelName;
                    announcement += " currently patched to channel " + juce::String(existingHwChannel + 1);
                }
                else
                {
                    // Not patched - prompt to patch
                    announcement = "Patch " + channelType + " " + juce::String(wfsChannel + 1);
                    if (channelName.isNotEmpty())
                        announcement += ", " + channelName;
                    announcement += " to audio interface channel " + juce::String(hwChannel + 1);
                }
            }

            if (announcement.isNotEmpty())
                TTSManager::getInstance().announceDebounced(announcement);
        }
    }
}

void PatchMatrixComponent::mouseExit(const juce::MouseEvent&)
{
    hoveredCell = {-1, -1};
    TTSManager::getInstance().cancelDebouncedAnnouncement();
    repaint();
}

void PatchMatrixComponent::mouseWheelMove(const juce::MouseEvent& event,
                                          const juce::MouseWheelDetails& wheel)
{
    // Native horizontal scrolling (trackpad two-finger swipe left/right)
    if (wheel.deltaX != 0.0f)
    {
        scrollOffsetX = juce::jlimit(0, maxScrollX,
            scrollOffsetX - static_cast<int>(wheel.deltaX * cellWidth * 3));
    }

    // Vertical scrolling, or Shift+wheel for horizontal
    if (wheel.deltaY != 0.0f)
    {
        if (event.mods.isShiftDown())
        {
            // Shift+vertical wheel = horizontal scroll (for regular mouse wheels)
            scrollOffsetX = juce::jlimit(0, maxScrollX,
                scrollOffsetX - static_cast<int>(wheel.deltaY * cellWidth * 3));
        }
        else
        {
            // Normal vertical scroll
            scrollOffsetY = juce::jlimit(0, maxScrollY,
                scrollOffsetY - static_cast<int>(wheel.deltaY * cellHeight * 3));
        }
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
    juce::ignoreUnused(tree);
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

    // Handle binaural channel changes (output patch only)
    if (property == WFSParameterIDs::binauralOutputChannel)
    {
        updateBinauralChannels();
        repaint();
    }
}

void PatchMatrixComponent::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    juce::ignoreUnused(child);
    // Check if this is an input/output channel being added
    if (parent == channelsTree)
    {
        updateChannelCounts();
        repaint();
    }
}

void PatchMatrixComponent::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    juce::ignoreUnused(child, index);
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
    int visibleHeight = getHeight() - contentTop - scrollBarThickness;

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
    if (pos.x >= rowHeaderWidth && pos.y >= titleHeight && pos.y < contentTop)
    {
        col = static_cast<int>((pos.x - rowHeaderWidth + scrollOffsetX) / cellWidth);
        if (col >= 0 && col < numHardwareChannels)
            return {col, -1};  // Header cell (col, row=-1)
    }

    // Check if in row header area (WFS channels)
    if (pos.x < rowHeaderWidth && pos.y >= contentTop)
    {
        row = static_cast<int>((pos.y - contentTop + scrollOffsetY) / cellHeight);
        if (row >= 0 && row < numWFSChannels)
            return {-1, row};  // Row header (col=-1, row)
    }

    // Check if in cell area
    if (pos.x >= rowHeaderWidth && pos.y >= contentTop)
    {
        col = static_cast<int>((pos.x - rowHeaderWidth + scrollOffsetX) / cellWidth);
        row = static_cast<int>((pos.y - contentTop + scrollOffsetY) / cellHeight);

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
    int visibleHeight = getHeight() - contentTop - scrollBarThickness;

    return cellX >= -cellWidth && cellX < visibleWidth &&
           cellY >= -cellHeight && cellY < visibleHeight;
}

juce::Rectangle<int> PatchMatrixComponent::getCellBounds(int row, int col) const
{
    int x = rowHeaderWidth + col * cellWidth - scrollOffsetX;
    int y = contentTop + row * cellHeight - scrollOffsetY;

    return juce::Rectangle<int>(x, y, cellWidth, cellHeight);
}

//==============================================================================
// Drawing Methods

void PatchMatrixComponent::drawHeader(juce::Graphics& g)
{
    // Draw header background
    g.setColour(ColorScheme::get().backgroundAlt);
    g.fillRect(rowHeaderWidth, titleHeight, getWidth() - rowHeaderWidth, headerHeight);

    // Draw hardware channel numbers
    int visibleCols = (getWidth() - rowHeaderWidth) / cellWidth + 2;
    int firstCol = scrollOffsetX / cellWidth;

    g.setColour(ColorScheme::get().textPrimary);
    g.setFont(juce::jmax(10.0f, 14.0f * WfsLookAndFeel::uiScale));

    for (int c = 0; c < visibleCols; ++c)
    {
        int col = firstCol + c;
        if (col >= numHardwareChannels)
            break;

        int x = rowHeaderWidth + c * cellWidth - (scrollOffsetX % cellWidth);

        // Draw binaural indicator (output patch only)
        if (!isInputPatch && isChannelUsedByBinaural(col))
        {
            g.setColour(ColorScheme::get().textDisabled.withAlpha(0.25f));
            g.fillRect(x, titleHeight, cellWidth, headerHeight);

            // Draw channel number in top half
            g.setColour(ColorScheme::get().textDisabled);
            g.drawText(juce::String(col + 1), x, titleHeight, cellWidth, headerHeight / 2,
                       juce::Justification::centred);

            // Draw headphones icon in bottom half
            drawHeadphonesIcon(g, juce::Rectangle<float>(
                static_cast<float>(x), static_cast<float>(titleHeight) + static_cast<float>(headerHeight) * 0.45f,
                static_cast<float>(cellWidth), static_cast<float>(headerHeight) * 0.5f));

            // Draw grid line
            g.setColour(ColorScheme::get().chromeDivider);
            g.drawVerticalLine(x, static_cast<float>(titleHeight), static_cast<float>(contentTop));
            continue;  // Skip normal rendering for this column
        }

        // Highlight if this is the active test channel (output patch only)
        if (!isInputPatch && currentMode == Mode::Testing && col == activeTestHardwareChannel)
        {
            g.setColour(juce::Colours::green.withAlpha(0.3f));
            g.fillRect(x, titleHeight, cellWidth, headerHeight);
        }
        // Highlight if hovered
        else if (hoveredCell.x == col && currentMode == Mode::Testing)
        {
            g.setColour(ColorScheme::get().textPrimary.withAlpha(0.1f));
            g.fillRect(x, titleHeight, cellWidth, headerHeight);
        }

        g.setColour(ColorScheme::get().textPrimary);
        g.drawText(juce::String(col + 1), x, titleHeight, cellWidth, headerHeight,
                   juce::Justification::centred);

        // Draw grid line
        g.setColour(ColorScheme::get().chromeDivider);
        g.drawVerticalLine(x, static_cast<float>(titleHeight), static_cast<float>(contentTop));
    }
}

void PatchMatrixComponent::drawRowHeaders(juce::Graphics& g)
{
    // Draw row header background
    g.setColour(ColorScheme::get().backgroundAlt);
    g.fillRect(0, contentTop, rowHeaderWidth, getHeight() - contentTop);

    // Draw WFS channel labels
    int visibleRows = (getHeight() - contentTop) / cellHeight + 2;
    int firstRow = scrollOffsetY / cellHeight;

    g.setFont(juce::jmax(8.0f, 12.0f * WfsLookAndFeel::uiScale));

    for (int r = 0; r < visibleRows; ++r)
    {
        int row = firstRow + r;
        if (row >= numWFSChannels)
            break;

        int y = contentTop + r * cellHeight - (scrollOffsetY % cellHeight);

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
            g.setColour(ColorScheme::get().textPrimary);
        }

        g.drawText(label, 5, y, rowHeaderWidth - 10, cellHeight,
                   juce::Justification::centredLeft);

        // Draw grid line
        g.setColour(ColorScheme::get().chromeDivider);
        g.drawHorizontalLine(y, 0, static_cast<float>(rowHeaderWidth));
    }
}

void PatchMatrixComponent::drawCells(juce::Graphics& g)
{
    int visibleCols = (getWidth() - rowHeaderWidth) / cellWidth + 2;
    int visibleRows = (getHeight() - contentTop) / cellHeight + 2;
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
    // Grey out binaural-reserved channels (output patch only)
    if (isChannelUsedByBinaural(col))
    {
        // Greyed background
        g.setColour(ColorScheme::get().textDisabled.withAlpha(0.15f));
        g.fillRect(bounds);

        // Diagonal stripes
        g.setColour(ColorScheme::get().textDisabled.withAlpha(0.2f));
        for (int i = -bounds.getHeight(); i < bounds.getWidth(); i += 8)
            g.drawLine(static_cast<float>(bounds.getX() + i), static_cast<float>(bounds.getBottom()),
                       static_cast<float>(bounds.getX() + i + bounds.getHeight()), static_cast<float>(bounds.getY()), 1.0f);

        // Grid border and return
        g.setColour(ColorScheme::get().chromeDivider);
        g.drawRect(bounds, 1);
        return;
    }

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
        g.setFont(juce::jmax(10.0f, 14.0f * WfsLookAndFeel::uiScale));
        g.drawText(juce::String(col + 1), bounds, juce::Justification::centred);
    }
    else
    {
        g.setColour(ColorScheme::get().surfaceCard);
        g.fillRect(bounds);
    }

    // Hover highlight (in patching mode only)
    if (currentMode == Mode::Patching &&
        hoveredCell.x == col && hoveredCell.y == row)
    {
        g.setColour(ColorScheme::get().textPrimary.withAlpha(0.1f));
        g.fillRect(bounds);
    }

    // Active test highlight (in testing mode) - color based on signal type
    if (!isInputPatch && currentMode == Mode::Testing &&
        isPatched && col == activeTestHardwareChannel && testSignalGenerator)
    {
        auto signalType = testSignalGenerator->getSignalType();

        switch (signalType)
        {
            case TestSignalGenerator::SignalType::PinkNoise:
                // Pink color for pink noise
                g.setColour(juce::Colour(0xFFFF69B4));  // Hot pink
                g.fillRect(bounds);
                break;

            case TestSignalGenerator::SignalType::Tone:
            {
                // Map frequency (20-20000 Hz) to hue (log scale)
                // Purple (0.8) for low frequencies, red (0) for high frequencies
                // Matches the frequency slider color
                float freq = testSignalGenerator->getFrequency();
                float logFreq = std::log10(juce::jlimit(20.0f, 20000.0f, freq));
                float minLog = std::log10(20.0f);
                float maxLog = std::log10(20000.0f);
                float t = (logFreq - minLog) / (maxLog - minLog);  // 0 = low freq, 1 = high freq
                float hue = 0.8f * (1.0f - t);  // Purple (0.8) at low, red (0) at high
                g.setColour(juce::Colour::fromHSV(hue, 0.8f, 0.9f, 1.0f));
                g.fillRect(bounds);
                break;
            }

            case TestSignalGenerator::SignalType::Sweep:
            {
                // Rainbow gradient for sweep (purple/low freq to red/high freq)
                // Draw pixel-by-pixel gradient horizontally
                for (int x = bounds.getX(); x < bounds.getRight(); ++x)
                {
                    float t = static_cast<float>(x - bounds.getX()) / static_cast<float>(bounds.getWidth());
                    // Purple (0.8) on left to red (0.0) on right
                    float hue = 0.8f * (1.0f - t);
                    g.setColour(juce::Colour::fromHSV(hue, 0.8f, 0.9f, 1.0f));
                    g.drawVerticalLine(x, static_cast<float>(bounds.getY()), static_cast<float>(bounds.getBottom()));
                }
                break;
            }

            case TestSignalGenerator::SignalType::DiracPulse:
                // White square for pulse
                g.setColour(juce::Colours::white);
                g.fillRect(bounds);
                break;

            default:
                // Fallback green for unknown types
                g.setColour(juce::Colours::green);
                g.fillRect(bounds);
                break;
        }

        // Draw bold black channel number on top of all test signal colors
        g.setColour(juce::Colours::black);
        g.setFont(juce::FontOptions(juce::jmax(10.0f, 14.0f * WfsLookAndFeel::uiScale)).withStyle("Bold"));
        g.drawText(juce::String(col + 1), bounds, juce::Justification::centred);
    }

    // Keyboard selection highlight (bold contrasting border)
    if (keyboardNavigationActive && selectedCell.x == col && selectedCell.y == row)
    {
        // Draw thick white border for high visibility
        g.setColour(juce::Colours::white);
        g.drawRect(bounds.reduced(1), 2);
        // Draw inner black border for contrast on light backgrounds
        g.setColour(juce::Colours::black);
        g.drawRect(bounds.reduced(3), 1);
    }

    // Draw grid lines
    g.setColour(ColorScheme::get().chromeDivider);
    g.drawRect(bounds, 1);
}

//==============================================================================
// Binaural Channel Helpers

void PatchMatrixComponent::updateBinauralChannels()
{
    binauralFirstChannel = binauralTree.isValid()
        ? (int)binauralTree.getProperty(WFSParameterIDs::binauralOutputChannel, -1)
        : -1;
}

bool PatchMatrixComponent::isChannelUsedByBinaural(int hwChannel) const
{
    if (isInputPatch || binauralFirstChannel < 0)
        return false;
    // binauralFirstChannel is 1-based, hwChannel is 0-based
    return hwChannel == (binauralFirstChannel - 1) || hwChannel == binauralFirstChannel;
}

void PatchMatrixComponent::drawHeadphonesIcon(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    float size = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.7f;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();

    juce::Path headphones;

    // Headband arc
    float r = size * 0.4f;
    headphones.addCentredArc(cx, cy - size * 0.05f, r, r * 0.7f,
                             0.0f, -juce::MathConstants<float>::pi, 0.0f, true);

    // Left ear cup
    float cupW = size * 0.28f, cupH = size * 0.38f;
    headphones.addRoundedRectangle(cx - r - cupW * 0.5f, cy - cupH * 0.3f,
                                   cupW, cupH, cupW * 0.35f);

    // Right ear cup
    headphones.addRoundedRectangle(cx + r - cupW * 0.5f, cy - cupH * 0.3f,
                                   cupW, cupH, cupW * 0.35f);

    g.setColour(ColorScheme::get().textSecondary);
    g.strokePath(headphones, juce::PathStrokeType(1.5f));
}

//==============================================================================
// Patching Logic

void PatchMatrixComponent::startPatchOperation(juce::Point<int> cell)
{
    // Block patching to binaural-reserved channels
    if (isChannelUsedByBinaural(cell.x))
        return;

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
    // Only allow 45° diagonals to prevent duplicate row/col patches
    int deltaRow = currentCell.y - patchDragState.startCell.y;
    int deltaCol = currentCell.x - patchDragState.startCell.x;

    // Use the minimum extent to constrain to 45° angles
    int steps = juce::jmin(std::abs(deltaRow), std::abs(deltaCol));

    // If one delta is zero, allow straight horizontal or vertical lines
    if (deltaRow == 0 || deltaCol == 0)
        steps = juce::jmax(std::abs(deltaRow), std::abs(deltaCol));

    // Direction increments: -1, 0, or +1
    int rowIncrement = (deltaRow > 0) ? 1 : (deltaRow < 0) ? -1 : 0;
    int colIncrement = (deltaCol > 0) ? 1 : (deltaCol < 0) ? -1 : 0;

    patchDragState.previewPatches.clear();

    for (int i = 0; i <= steps; ++i)
    {
        int row = patchDragState.startCell.y + (i * rowIncrement);
        int col = patchDragState.startCell.x + (i * colIncrement);

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

    // Block testing when signal type is Off - no visual feedback, just status message
    if (testSignalGenerator->getSignalType() == TestSignalGenerator::SignalType::Off)
    {
        if (onStatusMessage)
            onStatusMessage(LOC("audioPatch.messages.chooseTestSignal"));
        return;
    }

    // Toggle behavior: if clicking on already-active channel, stop the test signal
    if (hardwareChannel == activeTestHardwareChannel)
    {
        testSignalGenerator->setOutputChannel(-1);
        activeTestHardwareChannel = -1;
        repaint();

        // TTS: Announce test stopped
        TTSManager::getInstance().announceImmediate("Test stopped",
            juce::AccessibilityHandler::AnnouncementPriority::medium);
        return;
    }

    // Set test signal to this channel
    testSignalGenerator->setOutputChannel(hardwareChannel);

    // Track active channel for highlighting
    activeTestHardwareChannel = hardwareChannel;
    repaint();

    // TTS: Announce test started
    juce::String announcement = "Testing audio interface channel " + juce::String(hardwareChannel + 1);
    TTSManager::getInstance().announceImmediate(announcement,
        juce::AccessibilityHandler::AnnouncementPriority::medium);
}

// ============================================================================
// Keyboard Navigation for Accessibility
// ============================================================================

bool PatchMatrixComponent::keyPressed(const juce::KeyPress& key)
{
    bool handled = false;
    juce::Point<int> newCell = selectedCell;

    // Check if this is an arrow key
    bool isArrowKey = (key == juce::KeyPress::leftKey || key == juce::KeyPress::rightKey ||
                       key == juce::KeyPress::upKey || key == juce::KeyPress::downKey);

    // Arrow key navigation:
    // - First press (not yet navigating): go to top-left corner (0,0)
    // - Subsequent presses: move one cell at a time
    if (isArrowKey)
    {
        if (!keyboardNavigationActive || selectedCell.x < 0 || selectedCell.y < 0)
        {
            // First press - go to top-left corner
            newCell = {0, 0};
            handled = true;
        }
        else
        {
            // Already navigating - move one cell at a time
            if (key == juce::KeyPress::leftKey)
            {
                newCell.x = juce::jmax(0, selectedCell.x - 1);
                handled = true;
            }
            else if (key == juce::KeyPress::rightKey)
            {
                newCell.x = juce::jmin(numHardwareChannels - 1, selectedCell.x + 1);
                handled = true;
            }
            else if (key == juce::KeyPress::upKey)
            {
                newCell.y = juce::jmax(0, selectedCell.y - 1);
                handled = true;
            }
            else if (key == juce::KeyPress::downKey)
            {
                newCell.y = juce::jmin(numWFSChannels - 1, selectedCell.y + 1);
                handled = true;
            }
        }
    }
    // Space bar or Enter activates the cell (patch/unpatch or test) - only in Patch/Test mode
    else if ((key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey) &&
             currentMode != Mode::Scrolling && keyboardNavigationActive)
    {
        // Special handling for spacebar in Testing mode without Hold
        if (key == juce::KeyPress::spaceKey && currentMode == Mode::Testing &&
            !isInputPatch && testSignalGenerator && !testSignalGenerator->isHoldEnabled())
        {
            // Hold is OFF: start test on press, will stop on release
            int hwChannel = selectedCell.x;
            int wfsChannel = selectedCell.y;

            if (isPatchActive(wfsChannel, hwChannel))
            {
                // Block if signal type is Off
                if (testSignalGenerator->getSignalType() == TestSignalGenerator::SignalType::Off)
                {
                    if (onStatusMessage)
                        onStatusMessage(LOC("audioPatch.messages.chooseTestSignal"));
                    return true;
                }

                // Start test signal
                testSignalGenerator->setOutputChannel(hwChannel);
                activeTestHardwareChannel = hwChannel;
                spacebarTestActive = true;
                repaint();

                juce::String announcement = "Testing audio interface channel " + juce::String(hwChannel + 1);
                TTSManager::getInstance().announceImmediate(announcement,
                    juce::AccessibilityHandler::AnnouncementPriority::medium);
            }
            else
            {
                TTSManager::getInstance().announceImmediate("Cannot test - not patched here",
                    juce::AccessibilityHandler::AnnouncementPriority::medium);
            }
            return true;
        }

        // Normal activation (Patching mode, or Testing with Hold ON, or Enter key)
        handleCellActivation(selectedCell);
        return true;
    }
    // Page navigation - move by 10 cells
    else if (key == juce::KeyPress::pageUpKey)
    {
        newCell.y = juce::jmax(0, selectedCell.y - 10);
        handled = true;
    }
    else if (key == juce::KeyPress::pageDownKey)
    {
        newCell.y = juce::jmin(numWFSChannels - 1, selectedCell.y + 10);
        handled = true;
    }
    else if (key == juce::KeyPress::homeKey)
    {
        newCell = {0, 0};
        handled = true;
    }
    else if (key == juce::KeyPress::endKey)
    {
        newCell = {numHardwareChannels - 1, numWFSChannels - 1};
        handled = true;
    }

    // Update selection if moved
    if (handled && newCell != selectedCell)
    {
        selectedCell = newCell;
        keyboardNavigationActive = true;
        scrollToMakeVisible(selectedCell);
        announceSelectedCell();
        repaint();
    }

    return handled;
}

bool PatchMatrixComponent::keyStateChanged(bool isKeyDown)
{
    // Handle spacebar release to stop test signal when Hold is off
    if (!isKeyDown && spacebarTestActive)
    {
        // Check if spacebar is released
        if (!juce::KeyPress::isKeyCurrentlyDown(juce::KeyPress::spaceKey))
        {
            // Stop test signal
            if (testSignalGenerator)
                testSignalGenerator->setOutputChannel(-1);

            activeTestHardwareChannel = -1;
            spacebarTestActive = false;
            repaint();

            TTSManager::getInstance().announceImmediate("Test stopped",
                juce::AccessibilityHandler::AnnouncementPriority::medium);

            return true;
        }
    }

    return false;
}

void PatchMatrixComponent::focusGained(FocusChangeType)
{
    // Announce mode when gaining focus
    juce::String modeStr;
    if (currentMode == Mode::Patching)
    {
        modeStr = isInputPatch ? "Input patch matrix" : "Output patch matrix";
        modeStr += ". Use arrows to navigate, space to patch.";
    }
    else if (currentMode == Mode::Testing)
    {
        modeStr = "Output test matrix";
        modeStr += ". Use arrows to navigate, space to test.";
    }
    else
    {
        modeStr = "Patch matrix, scroll mode";
        modeStr += ". Use arrows to navigate.";
    }

    TTSManager::getInstance().announceImmediate(modeStr,
        juce::AccessibilityHandler::AnnouncementPriority::medium);

    // Don't activate keyboard navigation until they actually press an arrow
    repaint();
}

void PatchMatrixComponent::focusLost(FocusChangeType)
{
    // Keep keyboardNavigationActive and selectedCell so navigation resumes
    // from the same position when focus returns (e.g., after clicking a mode button)
    repaint();
}

void PatchMatrixComponent::scrollToMakeVisible(juce::Point<int> cell)
{
    if (cell.x < 0 || cell.y < 0)
        return;

    auto matrixArea = getLocalBounds();
    matrixArea.removeFromTop(contentTop);
    matrixArea.removeFromLeft(rowHeaderWidth);
    matrixArea.removeFromRight(scrollBarThickness);
    matrixArea.removeFromBottom(scrollBarThickness);

    int visibleCols = matrixArea.getWidth() / cellWidth;
    int visibleRows = matrixArea.getHeight() / cellHeight;

    // Calculate which cell indices are currently visible
    int firstVisibleCol = scrollOffsetX / cellWidth;
    int lastVisibleCol = firstVisibleCol + visibleCols - 1;
    int firstVisibleRow = scrollOffsetY / cellHeight;
    int lastVisibleRow = firstVisibleRow + visibleRows - 1;

    // Scroll horizontally if needed
    if (cell.x < firstVisibleCol)
        scrollOffsetX = cell.x * cellWidth;
    else if (cell.x > lastVisibleCol)
        scrollOffsetX = (cell.x - visibleCols + 1) * cellWidth;

    // Scroll vertically if needed
    if (cell.y < firstVisibleRow)
        scrollOffsetY = cell.y * cellHeight;
    else if (cell.y > lastVisibleRow)
        scrollOffsetY = (cell.y - visibleRows + 1) * cellHeight;

    // Clamp scroll positions
    scrollOffsetX = juce::jlimit(0, maxScrollX, scrollOffsetX);
    scrollOffsetY = juce::jlimit(0, maxScrollY, scrollOffsetY);

    updateScrollBars();
}

void PatchMatrixComponent::announceSelectedCell()
{
    if (selectedCell.x < 0 || selectedCell.y < 0)
        return;

    int wfsChannel = selectedCell.y;
    int hwChannel = selectedCell.x;

    // Get WFS channel name
    juce::String channelName;
    juce::String channelType = isInputPatch ? "Input" : "Output";

    if (isInputPatch)
    {
        auto inputsTree = parameters.getState().getChildWithName(WFSParameterIDs::Inputs);
        if (wfsChannel < inputsTree.getNumChildren())
        {
            auto inputTree = inputsTree.getChild(wfsChannel);
            auto channelTree = inputTree.getChildWithName(WFSParameterIDs::Channel);
            if (channelTree.isValid())
                channelName = channelTree.getProperty(WFSParameterIDs::inputName).toString();
        }
    }
    else
    {
        auto outputsTree = parameters.getState().getChildWithName(WFSParameterIDs::Outputs);
        if (wfsChannel < outputsTree.getNumChildren())
        {
            auto outputTree = outputsTree.getChild(wfsChannel);
            auto channelTree = outputTree.getChildWithName(WFSParameterIDs::Channel);
            if (channelTree.isValid())
                channelName = channelTree.getProperty(WFSParameterIDs::outputName).toString();
        }
    }

    // Build announcement
    juce::String announcement = channelType + " " + juce::String(wfsChannel + 1);
    if (channelName.isNotEmpty())
        announcement += ", " + channelName;
    announcement += ", interface channel " + juce::String(hwChannel + 1);

    // Add patch status
    if (isPatchActive(wfsChannel, hwChannel))
        announcement += ", patched";
    else
    {
        int existingHw = getHardwareChannelForWFS(wfsChannel);
        if (existingHw >= 0)
            announcement += ", patched elsewhere to " + juce::String(existingHw + 1);
        else
            announcement += ", not patched";
    }

    TTSManager::getInstance().announceImmediate(announcement,
        juce::AccessibilityHandler::AnnouncementPriority::medium);
}

void PatchMatrixComponent::handleCellActivation(juce::Point<int> cell)
{
    if (cell.x < 0 || cell.y < 0)
        return;

    int wfsChannel = cell.y;
    int hwChannel = cell.x;

    if (currentMode == Mode::Testing && !isInputPatch)
    {
        // Testing mode: test if patched
        if (isPatchActive(wfsChannel, hwChannel))
        {
            handleTestClick(hwChannel);
        }
        else
        {
            TTSManager::getInstance().announceImmediate("Cannot test - not patched here",
                juce::AccessibilityHandler::AnnouncementPriority::medium);
        }
    }
    else if (currentMode == Mode::Patching)
    {
        // Patching mode: toggle patch
        if (isPatchActive(wfsChannel, hwChannel))
        {
            // Remove this patch
            patches.erase(std::remove_if(patches.begin(), patches.end(),
                [wfsChannel, hwChannel](const PatchPoint& p) {
                    return p.wfsChannel == wfsChannel && p.hardwareChannel == hwChannel;
                }), patches.end());
            savePatchesToValueTree();
            repaint();

            juce::String channelType = isInputPatch ? "Input" : "Output";
            TTSManager::getInstance().announceImmediate(
                channelType + " " + juce::String(wfsChannel + 1) + " unpatched",
                juce::AccessibilityHandler::AnnouncementPriority::medium);
        }
        else
        {
            // Check if this WFS channel is already patched elsewhere
            int existingHw = getHardwareChannelForWFS(wfsChannel);
            if (existingHw >= 0)
            {
                // Remove existing patch first
                patches.erase(std::remove_if(patches.begin(), patches.end(),
                    [wfsChannel](const PatchPoint& p) {
                        return p.wfsChannel == wfsChannel;
                    }), patches.end());
            }

            // Check if another WFS channel is patched to this hardware channel
            for (auto it = patches.begin(); it != patches.end();)
            {
                if (it->hardwareChannel == hwChannel)
                    it = patches.erase(it);
                else
                    ++it;
            }

            // Add new patch
            patches.push_back({wfsChannel, hwChannel});
            savePatchesToValueTree();
            repaint();

            juce::String channelType = isInputPatch ? "Input" : "Output";
            TTSManager::getInstance().announceImmediate(
                channelType + " " + juce::String(wfsChannel + 1) + " patched to interface channel " + juce::String(hwChannel + 1),
                juce::AccessibilityHandler::AnnouncementPriority::medium);
        }
    }
}
