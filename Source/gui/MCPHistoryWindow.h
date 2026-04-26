#pragma once

#include <JuceHeader.h>
#include <vector>
#include "../Network/MCP/MCPUndoEngine.h"
#include "../Network/MCP/MCPChangeRecords.h"
#include "ColorScheme.h"

/** Persistent navigator over the AI change-record ring + redo stack.

    Phase 5c shipped a transient toast (MCPUndoOverlay) for "what just
    happened?". This window is the persistent counterpart for "what has
    the AI done across this rehearsal?" and "let me step backward and
    forward through the history". Both surfaces read the same buffer.

    Layout (top to bottom):
      - Header: title + record count
      - Scrollable table (newest first):
          timestamp | tool name | description | status | batch | action
        × on applied rows triggers targeted undo with group dependency
        chasing; ↺ on the topmost undone row triggers single-step redo.
      - Cursor divider between applied (above) and undone-redoable (below)
      - Footer: ⏮ Step Back / position indicator / Step Forward ⏭

    Polls both rings at 2 Hz — the buffer is 100-cap, render is cheap,
    and 500 ms latency is invisible at the speed an AI conversation
    moves. */
class MCPHistoryWindow : public juce::DocumentWindow,
                         public ColorScheme::Manager::Listener
{
public:
    MCPHistoryWindow (WFSNetwork::MCPUndoEngine& engine,
                      WFSNetwork::MCPChangeRecordBuffer& undoRing);
    ~MCPHistoryWindow() override;

    void closeButtonPressed() override;
    void colorSchemeChanged() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MCPHistoryWindow)
};

//==============================================================================
// Internal table component — renders rows + handles mouse interactions.
//==============================================================================
class MCPHistoryTableComponent : public juce::Component,
                                  private juce::Timer
{
public:
    MCPHistoryTableComponent (WFSNetwork::MCPUndoEngine& engine,
                              WFSNetwork::MCPChangeRecordBuffer& undoRing);
    ~MCPHistoryTableComponent() override;

    void paint (juce::Graphics& g) override;
    void mouseMove (const juce::MouseEvent& event) override;
    void mouseExit (const juce::MouseEvent& event) override;
    void mouseDown (const juce::MouseEvent& event) override;

    /** Number of rows currently rendered (applied + undone). */
    int getNumRows() const noexcept { return static_cast<int> (rows.size()); }

    /** Number of rows above the cursor (= currently-applied records). */
    int getUndoableCount() const noexcept { return undoableCount; }

    /** Number of rows below the cursor (= currently-redoable records). */
    int getRedoableCount() const noexcept { return getNumRows() - undoableCount; }

    /** Resync immediately (called by the parent on demand, e.g. after
        clicking Step Back / Forward in the footer so the row jumps without
        waiting for the next 500 ms tick). */
    void refreshNow();

    static constexpr int kRowHeight = 28;

private:
    void timerCallback() override;
    void rebuildRows();
    int rowIndexAtY (int y) const;
    juce::Rectangle<int> actionButtonBoundsForRow (int rowIdx) const;

    /** One displayable row — either an applied (undo-ring) record or an
        undone (redo-stack) record. Row order is newest applied first,
        then cursor, then newest-undone first. */
    struct DisplayRow
    {
        WFSNetwork::ChangeRecord record;
        bool isApplied;        // true = sits above the cursor
        int bufferIndex;       // index back into the source ring (undoRing or redoRing snapshot)
    };

    WFSNetwork::MCPUndoEngine& engine;
    WFSNetwork::MCPChangeRecordBuffer& undoRing;

    std::vector<DisplayRow> rows;
    int undoableCount = 0;
    int hoveredRow = -1;
    bool hoveredAction = false;
};
