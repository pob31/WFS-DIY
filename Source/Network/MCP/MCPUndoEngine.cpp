#include "MCPUndoEngine.h"
#include "../OSCProtocolTypes.h"
#include "../../Parameters/WFSValueTreeState.h"

namespace WFSNetwork
{

namespace
{
    /** Resolve the 0-based ValueTree channel index from a record's first
        affected_groups entry. Records produced by the Phase 1 hand-written
        tools and the Phase 2 generic dispatcher always carry exactly one
        entry; the channelId is 1-based for per-channel tools and 0 for
        globals. Returns -1 (= global) when no group or channelId is 0. */
    int resolveChannelIndex (const ChangeRecord& record)
    {
        if (record.affectedGroups.empty())
            return -1;
        const int displayId = record.affectedGroups.front().channelId;
        return displayId > 0 ? displayId - 1 : -1;
    }

    /** Detect EQ-band records by group name. The Phase 2 generic dispatcher
        emits group_name = "EQ" for output-EQ tools; the band index is in
        the original tool arguments under the `band` key (1-based). */
    bool isEqBandRecord (const ChangeRecord& record)
    {
        if (record.affectedGroups.empty())
            return false;
        return record.affectedGroups.front().groupName == "EQ";
    }

    int extractBandIndex (const ChangeRecord& record)
    {
        if (! record.arguments.isObject())
            return -1;
        auto* obj = record.arguments.getDynamicObject();
        if (obj == nullptr || ! obj->hasProperty ("band"))
            return -1;
        return static_cast<int> (obj->getProperty ("band")) - 1;
    }

    /** True if any (channel_id, group_name) pair appears in both records.
        Phase 5a's dependency rule: a later record "depends on" an earlier
        one when they share a parameter group on the same channel — so
        undoing the earlier one rolls back the entire trajectory for that
        group. */
    bool groupsIntersect (const ChangeRecord& a, const ChangeRecord& b)
    {
        for (const auto& ga : a.affectedGroups)
            for (const auto& gb : b.affectedGroups)
                if (ga.channelId == gb.channelId && ga.groupName == gb.groupName)
                    return true;
        return false;
    }
}

//==============================================================================
MCPUndoEngine::MCPUndoEngine (WFSValueTreeState& s, MCPChangeRecordBuffer& undo)
    : state (s),
      undoRing (undo),
      redoRing (std::make_unique<MCPChangeRecordBuffer> (undo.capacity()))
{
}

MCPUndoEngine::~MCPUndoEngine() = default;

//==============================================================================
UndoResult MCPUndoEngine::undoLast()
{
    ChangeRecord rec;
    if (! undoRing.popBack (rec))
        return UndoResult::fail ("no_history", "No AI-origin changes to undo.");

    UndoResult outcome = applyReverse (rec);
    if (! outcome.success)
    {
        // Restore the record so the caller can retry — undo failure shouldn't
        // silently lose the entry.
        undoRing.push (rec);
        return outcome;
    }

    // Move the (now-reversed) record onto the redo ring so a subsequent
    // mcp.redo_last_undone_ai_change can re-apply it.
    redoRing->push (std::move (rec));
    return outcome;
}

juce::Array<int> MCPUndoEngine::previewUndo (int recordIndex) const
{
    juce::Array<int> indices;

    const int total = undoRing.size();
    if (recordIndex < 0 || recordIndex >= total)
        return indices;

    ChangeRecord target;
    if (! undoRing.peekAt (recordIndex, target))
        return indices;

    indices.add (recordIndex);
    for (int i = recordIndex + 1; i < total; ++i)
    {
        ChangeRecord later;
        if (undoRing.peekAt (i, later) && groupsIntersect (target, later))
            indices.add (i);
    }

    return indices;
}

UndoResult MCPUndoEngine::undoByIndex (int recordIndex)
{
    const auto preview = previewUndo (recordIndex);
    if (preview.isEmpty())
        return UndoResult::fail ("invalid_index",
                                 "Record index " + juce::String (recordIndex) + " out of range.");

    // Pop the chain. Walk preview indices in REVERSE so removeAt's index
    // remains valid as we shrink the buffer from the right.
    std::vector<ChangeRecord> chain;
    chain.reserve (static_cast<size_t> (preview.size()));
    for (int i = preview.size() - 1; i >= 0; --i)
    {
        ChangeRecord rec;
        if (! undoRing.removeAt (preview[i], rec))
            return UndoResult::fail ("internal_error", "Buffer changed during undo");
        chain.push_back (std::move (rec));
    }
    // chain[0] is the newest of the preview (because we walked indices in reverse);
    // chain.back() is the target (oldest of the preview).

    const int batchId = (chain.size() > 1) ? nextBatchId++ : 0;

    // Reverse in reverse-chronological order (newest first). Stamp the batch
    // id so redoLast can later re-apply atomically.
    juce::String combinedDesc;
    for (auto& rec : chain)
    {
        rec.redoBatchId = batchId;
        const auto err = writePayloadHere (rec, rec.beforeState);
        if (err.isNotEmpty())
        {
            // Partial failure: push everything reversed-so-far back, including
            // this record, so the buffer is in a consistent state. The state
            // tree itself may be partially undone — that's an honest report.
            for (auto& back : chain)
                undoRing.push (std::move (back));
            return UndoResult::fail ("internal_error", err);
        }
        if (combinedDesc.isNotEmpty())
            combinedDesc << " | ";
        combinedDesc << "Undid: " << rec.operatorDescription;
    }

    // Push to the redo ring in chronological order (oldest first), so the
    // newest of the batch ends up at the back where redoLast will see it
    // first.
    for (auto it = chain.rbegin(); it != chain.rend(); ++it)
        redoRing->push (std::move (*it));

    return UndoResult::ok (combinedDesc, juce::var(), juce::var(), preview.size());
}

UndoResult MCPUndoEngine::redoLast()
{
    ChangeRecord rec;
    if (! redoRing->popBack (rec))
        return UndoResult::fail ("no_redo", "No undone AI changes available to redo.");

    // Solo record (no batch) — same path as before.
    if (rec.redoBatchId == 0)
    {
        UndoResult outcome = applyForward (rec);
        if (! outcome.success)
        {
            redoRing->push (rec);
            return outcome;
        }
        undoRing.push (std::move (rec));
        return outcome;
    }

    // Batch: pop all consecutive members of the same batch from the back
    // of the redo ring.
    const int batchId = rec.redoBatchId;
    std::vector<ChangeRecord> batch;
    batch.push_back (std::move (rec));

    while (true)
    {
        ChangeRecord peek;
        if (! redoRing->peekAt (redoRing->size() - 1, peek))
            break;
        if (peek.redoBatchId != batchId)
            break;
        if (! redoRing->popBack (peek))
            break;
        batch.push_back (std::move (peek));
    }

    // batch[0] is newest (we popped from the back first); apply in
    // chronological order (oldest first) by walking in reverse.
    juce::String combinedDesc;
    for (auto it = batch.rbegin(); it != batch.rend(); ++it)
    {
        const auto err = writePayloadHere (*it, it->afterState);
        if (err.isNotEmpty())
        {
            // Restore everything we touched and bail out.
            for (auto& back : batch)
                redoRing->push (std::move (back));
            return UndoResult::fail ("internal_error", err);
        }
        if (combinedDesc.isNotEmpty())
            combinedDesc << " | ";
        combinedDesc << "Redid: " << it->operatorDescription;
    }

    // Push back to the undo ring in chronological order — oldest-first iteration
    // means the newest ends up at the back.
    for (auto it = batch.rbegin(); it != batch.rend(); ++it)
        undoRing.push (std::move (*it));

    return UndoResult::ok (combinedDesc, juce::var(), juce::var(), static_cast<int> (batch.size()));
}

void MCPUndoEngine::onNewStateModifyingRecord()
{
    redoRing->clear();
}

int MCPUndoEngine::redoableCount() const
{
    return redoRing->size();
}

//==============================================================================
UndoResult MCPUndoEngine::applyReverse (const ChangeRecord& record)
{
    const auto err = writePayloadHere (record, record.beforeState);
    if (err.isNotEmpty())
        return UndoResult::fail ("internal_error", err);

    juce::String desc = "Undid: " + record.operatorDescription;
    return UndoResult::ok (std::move (desc), record.afterState, record.beforeState, 1);
}

UndoResult MCPUndoEngine::applyForward (const ChangeRecord& record)
{
    const auto err = writePayloadHere (record, record.afterState);
    if (err.isNotEmpty())
        return UndoResult::fail ("internal_error", err);

    juce::String desc = "Redid: " + record.operatorDescription;
    return UndoResult::ok (std::move (desc), record.beforeState, record.afterState, 1);
}

//==============================================================================
juce::String MCPUndoEngine::writePayloadHere (const ChangeRecord& record,
                                              const juce::var& payload)
{
    // Caller guarantees we're on the JUCE message thread inside an
    // OriginTagScope of MCP. Doing a second callAsync here would deadlock
    // — the dispatcher's tool-execution wait blocks the message thread,
    // and the inner lambda can never run.
    jassert (juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto* payloadObj = payload.isObject() ? payload.getDynamicObject() : nullptr;
    if (payloadObj == nullptr)
        return "Record's payload (before/after state) is not an object";

    const int channelIndex = resolveChannelIndex (record);
    const bool isEqBand = isEqBandRecord (record);
    const int bandIndex = isEqBand ? extractBandIndex (record) : -1;

    try
    {
        const auto& props = payloadObj->getProperties();
        for (int i = 0; i < props.size(); ++i)
        {
            const juce::Identifier paramId = props.getName (i);
            const juce::var value = props.getValueAt (i);

            if (isEqBand && bandIndex >= 0 && channelIndex >= 0)
                state.setOutputEQBandParameterWithArrayPropagation (channelIndex, bandIndex, paramId, value);
            else
                state.setParameter (paramId, value, channelIndex);
        }
    }
    catch (const std::exception& e)
    {
        return juce::String ("Reversal threw: ") + e.what();
    }
    catch (...)
    {
        return "Reversal threw unknown exception";
    }

    return {};
}

} // namespace WFSNetwork
