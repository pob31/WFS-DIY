#pragma once

#include <JuceHeader.h>
#include <set>
#include "../Network/MCP/MCPUndoEngine.h"
#include "../Network/MCP/MCPChangeRecords.h"
#include "ColorScheme.h"

/** Growing-toast overlay for AI-undo records.

    Top-right corner of the main window, non-modal. Each AI tool call that
    modifies state pushes a row showing the operator-friendly description.
    Rows have per-row × buttons (targeted undo with group-based dependency
    chasing — see MCPUndoEngine::undoByIndex). Fade out after 15 s of
    display. Self-corrected records (operator already manually overrode
    the AI's value — flag set by MCPUndoEngine's listener in Phase 5b
    Block 4) are skipped from the visible list. The toast hides itself
    when no rows are visible and shows itself when a new record arrives.

    Implementation polls the change-record buffer at 5 Hz; pull is simpler
    than wiring a listener API onto MCPChangeRecordBuffer and the ~200 ms
    delay is invisible at the speed an AI conversation moves. */
class MCPUndoOverlay : public juce::Component,
                      private juce::Timer,
                      public ColorScheme::Manager::Listener
{
public:
    MCPUndoOverlay (WFSNetwork::MCPUndoEngine& engine,
                    WFSNetwork::MCPChangeRecordBuffer& buffer);
    ~MCPUndoOverlay() override;

    /** Called by MainComponent in resized() to position the overlay in
        the parent's top-right corner. The overlay sizes itself based on
        the number of visible rows. */
    void positionInParent (juce::Rectangle<int> parentBounds);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseMove (const juce::MouseEvent& event) override;
    void mouseExit (const juce::MouseEvent& event) override;

    void colorSchemeChanged() override { repaint(); }

private:
    void timerCallback() override;

    /** One displayed row in the overlay. Carries a copy of the underlying
        ChangeRecord plus display-side state (fade, hover). */
    struct DisplayedRow
    {
        WFSNetwork::ChangeRecord record;
        juce::Time displayCreatedAt;       // when this row first appeared in the toast
        bool fadingOut = false;            // Block 3 — fade after 15 s
        float opacity = 1.0f;
    };

    /** Re-syncs `rows` against the current undoRing snapshot. Adds new
        rows for records that appeared since last poll, drops rows whose
        backing record is gone (undone, or self-corrected). */
    void syncWithBuffer();

    /** Find the row index (in `rows`) for a given mouse position, or -1. */
    int rowIndexAtY (int y) const;

    /** Indices in `rows` that the × button at `rowIdx` would also reverse,
        per group-based dependency chasing. Used for hover-preview
        highlighting (Block 4). For Block 1 returns just {rowIdx}. */
    juce::Array<int> previewForRow (int rowIdx) const;

    /** Convert a row index in our `rows` list to the corresponding
        ring-buffer index for MCPUndoEngine::undoByIndex. */
    int bufferIndexForRow (int rowIdx) const;

    juce::Rectangle<int> getCloseButtonBounds() const;
    juce::Rectangle<int> getRowBounds (int rowIdx) const;
    juce::Rectangle<int> getRowDeleteButtonBounds (int rowIdx) const;

    WFSNetwork::MCPUndoEngine& engine;
    WFSNetwork::MCPChangeRecordBuffer& buffer;

    std::vector<DisplayedRow> rows;
    // Timestamps (ms) of records the user has dismissed — aged out, container
    // close, or per-row ×. syncWithBuffer skips these so a still-in-the-ring
    // record doesn't get re-added on the next 200 ms tick.
    std::set<juce::int64> dismissedTimestamps;
    int hoveredRow = -1;
    bool hoveredRowDeleteButton = false;
    bool hoveredCloseButton = false;
    juce::Array<int> hoverPreviewRows;  // rows that would also be reversed if × clicked

    // Layout constants — multiplied by WfsLookAndFeel::uiScale at use-site
    // so the toast resizes together with the rest of the app's UI.
    static int rowHeight();
    static int headerHeight();
    static int toastWidth();
    static int paddingFromCorner();
    static int rowDeleteButtonSize();
    static constexpr int kMaxVisibleRows = 10;
    static constexpr int kRowLifetimeMs = 15000;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MCPUndoOverlay)
};
