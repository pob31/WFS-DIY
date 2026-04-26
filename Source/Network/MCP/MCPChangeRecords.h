#pragma once

#include <JuceHeader.h>
#include <deque>
#include "../OSCProtocolTypes.h"

namespace WFSNetwork
{

/** Identifies a parameter group as "(channel_id, group_name)".
    For per-channel parameters channelId is 1-based; for global parameters
    use channelId == 0. The group_name is the CSV Section column,
    normalized to drop coordinate-mode suffixes. Used by Phase 5 undo to
    chase dependency chains across records. */
struct AffectedGroup
{
    int channelId = 0;
    juce::String groupName;

    bool operator== (const AffectedGroup& other) const noexcept
    {
        return channelId == other.channelId && groupName == other.groupName;
    }
};

/** One entry in the AI change-record ring buffer.

    Tier-1-success and tier-2-confirmed-execution tool calls produce a
    record. Read-only tool calls (session.get_state, snapshot.list, etc.)
    do NOT — there is nothing to undo. Block 4 captures the envelope and
    timing; Block 5's hand-written tools populate the affected fields;
    Phase 5's undo system reads the buffer back. */
struct ChangeRecord
{
    juce::Time timestamp;
    juce::String toolName;
    juce::var arguments;                       // JSON object the AI passed in
    juce::String operatorDescription;          // human-readable, surfaced in toast
    juce::StringArray affectedParameters;      // parameter variable names
    std::vector<AffectedGroup> affectedGroups; // (channel_id, group_name)
    juce::var beforeState;                     // JSON object: paramName → value
    juce::var afterState;                      // JSON object: paramName → value
    OriginTag origin = OriginTag::MCP;         // always MCP for these records

    /** Set by MCPUndoEngine::undoByIndex when a record is part of a multi-
        record reversal chain (target + dependents). All chain members
        share the same id. 0 means "solo" — undone individually, redone
        individually. Lets MCPUndoEngine::redoLast re-apply a chain
        atomically by popping all consecutive records with the same id. */
    int redoBatchId = 0;

    /** Phase 5b Block 4: set true when a non-MCP origin writes a value
        DIFFERENT from this record's `afterState` to one of its
        affected_parameters. Tells Phase 5c's toast overlay to drop the
        row from the active-undo affordance — the operator has already
        overridden the AI's effect, so offering to "undo" would just
        reverse the operator's correction. The record stays in the
        queryable history (mcp.get_ai_change_history still returns it);
        only its toast presence goes away. */
    bool isSelfCorrected = false;
};

/** Thread-safe ring buffer of AI change records. Capacity is fixed at
    construction; pushing past capacity drops the oldest entry. Phase 1
    populates this; Phase 5 surfaces the undo overlay + keyboard
    shortcuts on top. The size cap (100 by default) tracks the spec. */
class MCPChangeRecordBuffer
{
public:
    static constexpr int kDefaultCapacity = 100;

    explicit MCPChangeRecordBuffer (int capacity = kDefaultCapacity);

    /** Append a record, dropping the oldest entry if the buffer is full. */
    void push (ChangeRecord record);

    /** Pop the most-recent record into `out`. Returns false if the buffer is
        empty. Used by the undo engine to drain a record onto the redo stack. */
    bool popBack (ChangeRecord& out);

    /** Copy the record at the given index (0 = oldest, size-1 = newest) into
        `out`. Returns false if the index is out of range. */
    bool peekAt (int index, ChangeRecord& out) const;

    /** Remove the record at the given index, copying it into `out`. Used by
        targeted-undo dependency chasing (Phase 5a Block 2). Returns false if
        the index is out of range. */
    bool removeAt (int index, ChangeRecord& out);

    /** Snapshot of the last `n` records (most recent last). Pass -1 for all. */
    std::vector<ChangeRecord> getRecent (int n = -1) const;

    /** Phase 5b Block 4: mutate matching records' isSelfCorrected flag
        in-place under lock. Used by the engine's ValueTree listener when
        a non-MCP origin writes a different value than the AI's last
        write to a parameter the record touched. Returns the number of
        records mutated. */
    int markMatchingAsSelfCorrected (std::function<bool (const ChangeRecord&)> predicate);

    /** Current count of stored records. */
    int size() const noexcept;

    int capacity() const noexcept { return cap; }

    /** Remove every record. Used by Phase 5 redo-stack reset on new actions. */
    void clear();

private:
    int cap;
    std::deque<ChangeRecord> records;
    mutable juce::CriticalSection lock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MCPChangeRecordBuffer)
};

} // namespace WFSNetwork
