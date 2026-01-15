#pragma once

#include <JuceHeader.h>
#include "ColorScheme.h"
#include "../WfsParameters.h"
#include "../DSP/TestSignalGenerator.h"

/**
 * PatchMatrixComponent
 *
 * Custom scrollable table for audio channel patching.
 * Displays WFS channels (rows) × Hardware interface channels (columns).
 * Supports three modes: Scrolling, Patching, and Testing (output only).
 *
 * Mode behaviors:
 * - Scrolling: Any mouse drag scrolls viewport, no patching
 * - Patching: Left mouse drag patches channels, right mouse/3-finger drag scrolls
 * - Testing: Click output to play test signal (output patch only)
 *
 * Enforces 1:1 mapping constraint: each WFS channel maps to at most one hardware
 * channel, and vice versa.
 */
class PatchMatrixComponent : public juce::Component,
                              public juce::ScrollBar::Listener,
                              public juce::ValueTree::Listener
{
public:
    enum class Mode
    {
        Scrolling,
        Patching,
        Testing  // Output patch only
    };

    struct PatchPoint
    {
        int wfsChannel;      // Row index (0-based)
        int hardwareChannel; // Column index (0-based)

        bool operator==(const PatchPoint& other) const
        {
            return wfsChannel == other.wfsChannel &&
                   hardwareChannel == other.hardwareChannel;
        }
    };

    /**
     * Constructor
     * @param valueTreeState  Reference to WFSValueTreeState
     * @param isInputPatch    true for input patch, false for output patch
     * @param testSignalGen   Pointer to test signal generator (for output patch testing mode)
     */
    PatchMatrixComponent(WFSValueTreeState& valueTreeState,
                         bool isInputPatch,
                         TestSignalGenerator* testSignalGen = nullptr);

    ~PatchMatrixComponent() override;

    /**
     * Set the current mode
     */
    void setMode(Mode newMode);

    /**
     * Get the current mode
     */
    Mode getMode() const { return currentMode; }

    /**
     * Load patches from ValueTree
     */
    void loadPatchesFromValueTree();

    /**
     * Save patches to ValueTree
     */
    void savePatchesToValueTree();

    /**
     * Clear all patches (for Unpatch All button)
     */
    void clearAllPatches();

    /**
     * Check if a WFS channel is patched to any hardware channel
     */
    bool isWFSChannelPatched(int wfsChannel) const;

    /**
     * Get the hardware channel a WFS channel is patched to (-1 if not patched)
     */
    int getHardwareChannelForWFS(int wfsChannel) const;

    /**
     * Callback when processing state changes (stops test signals)
     */
    void setProcessingStateChanged(bool isProcessing);

    /**
     * Clear the active test channel highlighting (called when hold is disabled)
     */
    void clearActiveTestChannel();

    /**
     * Callback for when test signal is auto-configured (so UI can sync)
     */
    std::function<void()> onTestSignalConfigured;

    /**
     * Callback for status bar messages (e.g., "Choose a Test Signal to Enable Testing")
     */
    std::function<void(const juce::String&)> onStatusMessage;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    // Mouse handling
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& event,
                        const juce::MouseWheelDetails& wheel) override;

    // Keyboard navigation for accessibility
    bool keyPressed(const juce::KeyPress& key) override;
    bool keyStateChanged(bool isKeyDown) override;
    void focusGained(FocusChangeType cause) override;
    void focusLost(FocusChangeType cause) override;

    // ScrollBar::Listener
    void scrollBarMoved(juce::ScrollBar* bar, double newRangeStart) override;

    // ValueTree::Listener (for channel count changes)
    void valueTreePropertyChanged(juce::ValueTree& tree,
                                   const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;
    void valueTreeChildOrderChanged(juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged(juce::ValueTree&) override {}

private:
    // Visual constants
    static constexpr int cellWidth = 40;
    static constexpr int cellHeight = 30;
    static constexpr int headerHeight = 50;
    static constexpr int rowHeaderWidth = 120;
    static constexpr int scrollBarThickness = 16;

    // Data
    WFSValueTreeState& parameters;
    juce::ValueTree patchTree;  // Reference to InputPatch or OutputPatch
    juce::ValueTree channelsTree;  // Reference to Inputs or Outputs tree
    std::vector<PatchPoint> patches;  // Active 1:1 mappings
    Mode currentMode = Mode::Scrolling;
    bool isInputPatch;
    TestSignalGenerator* testSignalGenerator;

    // Binaural channel tracking (output patch only)
    juce::ValueTree binauralTree;    // Reference to Binaural section
    int binauralFirstChannel = -1;   // First channel of binaural pair (1-based, -1 = disabled)

    // Scrolling
    juce::ScrollBar horizontalScroll{false};
    juce::ScrollBar verticalScroll{true};
    int scrollOffsetX = 0;
    int scrollOffsetY = 0;
    int maxScrollX = 0;
    int maxScrollY = 0;

    // Mouse/touch handling
    juce::Point<int> dragStartPos;
    juce::Point<int> scrollStartOffset;
    bool isDraggingToScroll = false;
    int touchFingerCount = 0;
    int scrollDragSourceIndex = -1;  // Track which touch source initiated scroll

    // Patching drag state
    struct PatchDragState
    {
        juce::Point<int> startCell{-1, -1};
        juce::Point<int> currentCell{-1, -1};
        std::vector<PatchPoint> previewPatches;
        bool isActive = false;
    };
    PatchDragState patchDragState;

    // Hover state
    juce::Point<int> hoveredCell{-1, -1};

    // Keyboard navigation state
    juce::Point<int> selectedCell{-1, -1};  // Currently selected cell for keyboard navigation
    bool keyboardNavigationActive = false;   // True when using arrow keys

    // Active test channel (for highlighting)
    int activeTestHardwareChannel = -1;
    bool spacebarTestActive = false;  // True when spacebar started test without hold

    // Channel dimensions
    int numWFSChannels = 0;
    int numHardwareChannels = 0;

    // Helper methods
    void updateScrollBars();
    void updateChannelCounts();
    juce::Point<int> getCellAtPosition(juce::Point<float> pos) const;
    bool isCellVisible(int row, int col) const;
    juce::Rectangle<int> getCellBounds(int row, int col) const;

    // Binaural channel helpers (output patch only)
    void updateBinauralChannels();
    bool isChannelUsedByBinaural(int hardwareChannel) const;  // 0-based
    void drawHeadphonesIcon(juce::Graphics& g, juce::Rectangle<float> bounds);

    // Drawing methods
    void drawHeader(juce::Graphics& g);
    void drawRowHeaders(juce::Graphics& g);
    void drawCells(juce::Graphics& g);
    void drawCell(juce::Graphics& g, int row, int col, juce::Rectangle<int> bounds);

    // Patching logic
    void startPatchOperation(juce::Point<int> cell);
    void updatePatchDrag(juce::Point<int> currentCell);
    void commitPatchOperation();
    void cancelPatchOperation();

    bool isPatchActive(int wfsChannel, int hwChannel) const;
    bool isValidPatch(int wfsChannel, int hwChannel) const;
    juce::Colour getCellColor(int wfsChannel) const;

    // Testing mode (output patch only)
    void handleTestClick(int hardwareChannel);

    // Keyboard navigation helpers
    void scrollToMakeVisible(juce::Point<int> cell);
    void announceSelectedCell();
    void handleCellActivation(juce::Point<int> cell);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchMatrixComponent)
};
