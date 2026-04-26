#pragma once

#include <JuceHeader.h>
#include "MCPChangeRecords.h"

class WFSValueTreeState;

namespace WFSNetwork
{

/** Outcome of an undo or redo invocation. */
struct UndoResult
{
    bool success = false;
    juce::String operatorDescription;  // human-readable summary, surfaced to the AI
    juce::var beforeApplied;           // values that were displaced by this undo
    juce::var afterApplied;            // values that were written back into the tree
    juce::String errorCode;            // populated on failure: "no_history", "no_redo", "internal_error", "timeout"
    juce::String errorMessage;
    int recordsAffected = 0;           // 1 for the simple case; >1 once Block 2's dependency chasing lands

    static UndoResult ok (juce::String desc, juce::var before, juce::var after, int affected = 1)
    {
        return { true, std::move (desc), std::move (before), std::move (after), {}, {}, affected };
    }
    static UndoResult fail (juce::String code, juce::String msg)
    {
        return { false, {}, {}, {}, std::move (code), std::move (msg), 0 };
    }
};

/** Backend engine for AI-undo / AI-redo execution against the MCP change-
    record ring buffer. Phase 1 populated the buffer; Phase 5a (this class)
    makes the previously-stub `mcp.undo_last_ai_change` and
    `mcp.redo_last_undone_ai_change` tools actually move state.

    Threading: callers MUST already be on the JUCE message thread inside
    an OriginTagScope of MCP. The dispatcher's runOnMessageThread wrapper
    sets both up before invoking the undo-tool handler, so the standard
    MCP path is fine. The future Phase 5c keyboard-shortcut path is also
    fine since keyboard events arrive on the message thread; that path
    just needs to wrap its call site in OriginTagScope { MCP }.

    Doing a second message-thread hop here would deadlock — the message
    thread can't process the inner callAsync while waiting on its
    completion event.

    Phase 5b layers staleness detection and cross-actor notifications on
    top of this primitive; Phase 5c adds the toast overlay that calls
    `undoByIndex` (Block 2) for per-row clicks. */
class MCPUndoEngine
{
public:
    MCPUndoEngine (WFSValueTreeState& state, MCPChangeRecordBuffer& undoRing);
    ~MCPUndoEngine();

    /** Undo the newest record on the undo ring. Reverses its writes, then
        moves the record onto the internal redo ring. */
    UndoResult undoLast();

    /** Re-apply the newest record on the redo ring. Writes its `after_state`
        and moves the record back onto the undo ring. */
    UndoResult redoLast();

    /** Called by the dispatcher whenever a new state-modifying tool call
        lands. Standard undo/redo semantics: a fresh action invalidates any
        pending redo history. */
    void onNewStateModifyingRecord();

    /** Read-only count of records currently on the redo ring. Useful for
        the AI to know whether redo is available. */
    int redoableCount() const;

private:
    /** Apply a record's `before_state` to the tree (= reversal). */
    UndoResult applyReverse (const ChangeRecord& record);

    /** Apply a record's `after_state` to the tree (= re-apply / redo). */
    UndoResult applyForward (const ChangeRecord& record);

    /** Shared write logic; `payload` is either the before_state or
        after_state object, depending on direction. Caller is on the
        message thread. Returns an error string on failure (empty on
        success). */
    juce::String writePayloadHere (const ChangeRecord& record,
                                   const juce::var& payload);

    WFSValueTreeState& state;
    MCPChangeRecordBuffer& undoRing;
    std::unique_ptr<MCPChangeRecordBuffer> redoRing;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MCPUndoEngine)
};

} // namespace WFSNetwork
