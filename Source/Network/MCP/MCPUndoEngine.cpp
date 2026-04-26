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
    // Phase 5b: subscribe to ValueTree property changes for staleness
    // detection. The state's addListener registers us against the root
    // tree, so we get callbacks for every nested property change too.
    state.addListener (this);
}

MCPUndoEngine::~MCPUndoEngine()
{
    state.removeListener (this);
}

void MCPUndoEngine::valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyChanged,
                                               const juce::Identifier& property)
{
    const auto origin = getCurrentOriginTag();
    const auto now = juce::Time::getCurrentTime();
    const auto newValue = treeWhosePropertyChanged.getProperty (property);
    const auto paramName = property.toString();

    // Update the last-writer cache (for staleness detection on undo).
    {
        const juce::ScopedLock sl (lastWriterLock);
        lastWriterByParamId[paramName] = LastWriter { origin, now, newValue };
    }

    // Phase 5b Block 3: cross-actor notifications. When a non-MCP write
    // lands on a parameter that an active AI record touched, accumulate a
    // notification so the next AI tool-call response carries it.
    // Skip MCP-origin writes (we caused them), and writes that don't touch
    // any parameter the AI has recorded.
    if (origin == OriginTag::MCP)
        return;

    // Find the most recent record (newest first) that mentions this param.
    // If found, capture the AI's value for the human-readable summary.
    juce::var aiValue;
    int channelId = 0;
    bool foundActiveRecord = false;
    {
        const auto records = undoRing.getRecent (-1);
        // Walk newest → oldest; the AI's most recent intent is what the
        // operator override is reacting to.
        for (auto it = records.rbegin(); it != records.rend(); ++it)
        {
            if (it->affectedParameters.contains (paramName))
            {
                if (auto* afterObj = it->afterState.isObject() ? it->afterState.getDynamicObject() : nullptr)
                    aiValue = afterObj->getProperty (juce::Identifier (paramName));
                if (! it->affectedGroups.empty())
                    channelId = it->affectedGroups.front().channelId;
                foundActiveRecord = true;
                break;
            }
        }
    }
    if (! foundActiveRecord)
        return;

    // Coincidence: operator dragged but landed on the AI's value. No
    // notification — the AI's last write is still effectively in place.
    if (newValue == aiValue)
        return;

    // Translate origin enum → human label.
    juce::String originStr;
    switch (origin)
    {
        case OriginTag::UI:         originStr = "UI";         break;
        case OriginTag::OSC:        originStr = "OSC";        break;
        case OriginTag::Tracking:   originStr = "Tracking";   break;
        case OriginTag::Snapshot:   originStr = "Snapshot";   break;
        case OriginTag::LFO:        originStr = "LFO";        break;
        case OriginTag::Move:       originStr = "Move";       break;
        case OriginTag::Automation: originStr = "Automation"; break;
        case OriginTag::None:       originStr = "Unknown";    break;
        default:                    originStr = "Unknown";    break;
    }

    auto note = std::make_unique<juce::DynamicObject>();
    note->setProperty ("type", "operator_override");
    note->setProperty ("parameter", paramName);
    if (channelId > 0)
        note->setProperty ("channel_id", channelId);
    note->setProperty ("ai_value", aiValue);
    note->setProperty ("current_value", newValue);
    note->setProperty ("since_origin", originStr);

    juce::String summary;
    summary << "Operator changed " << paramName;
    if (channelId > 0)
        summary << " for channel " << channelId;
    summary << " (you set " << aiValue.toString()
            << "; current " << newValue.toString() << ")"
            << " via " << originStr
            << " at " << now.toString (false, true);
    note->setProperty ("summary", summary);

    // De-dup: replace any existing pending notification for the same param.
    // Slider drags fire many property-changed events per second, but the AI
    // only needs the latest snapshot.
    {
        const juce::ScopedLock sl (notificationsLock);
        for (int i = pendingNotifications.size() - 1; i >= 0; --i)
        {
            if (auto* existingObj = pendingNotifications.getReference (i).getDynamicObject())
            {
                if (existingObj->getProperty ("parameter").toString() == paramName)
                {
                    pendingNotifications.remove (i);
                    break;
                }
            }
        }
        pendingNotifications.add (juce::var (note.release()));
    }
}

juce::Array<juce::var> MCPUndoEngine::drainPendingNotifications()
{
    const juce::ScopedLock sl (notificationsLock);
    juce::Array<juce::var> drained = std::move (pendingNotifications);
    pendingNotifications.clearQuick();
    return drained;
}

UndoResult MCPUndoEngine::checkStalenessOrEmpty (const ChangeRecord& record) const
{
    // No after-state means we can't compare current vs AI value.
    auto* afterObj = record.afterState.isObject() ? record.afterState.getDynamicObject() : nullptr;
    if (afterObj == nullptr)
        return UndoResult::fail ({}, {});  // empty errorCode → "no staleness, proceed"

    juce::Array<juce::var> drifts;

    {
        const juce::ScopedLock sl (lastWriterLock);

        for (const auto& paramName : record.affectedParameters)
        {
            auto it = lastWriterByParamId.find (paramName);
            if (it == lastWriterByParamId.end())
                continue;  // never seen a write for this param

            const LastWriter& lw = it->second;

            // The last writer was MCP, or it predates the record — either way,
            // not a drift caller cares about.
            if (lw.origin == OriginTag::MCP)
                continue;
            if (lw.when <= record.timestamp)
                continue;

            // Compare current value against AI's after_state. If they match,
            // the operator dragged but coincidentally landed on the AI's value
            // — not stale per the design doc.
            const juce::Identifier paramId (paramName);
            const juce::var aiValue = afterObj->getProperty (paramId);
            if (lw.value == aiValue)
                continue;

            // Drift confirmed.
            auto driftObj = std::make_unique<juce::DynamicObject>();
            driftObj->setProperty ("parameter",     paramName);
            driftObj->setProperty ("ai_value",      aiValue);
            driftObj->setProperty ("current_value", lw.value);

            // Translate origin enum → human string. LogEntry has the full
            // mapping; build a tiny inline switch here to avoid depending on
            // a LogEntry instance.
            juce::String originStr;
            switch (lw.origin)
            {
                case OriginTag::UI:         originStr = "UI";         break;
                case OriginTag::OSC:        originStr = "OSC";        break;
                case OriginTag::Tracking:   originStr = "Tracking";   break;
                case OriginTag::Snapshot:   originStr = "Snapshot";   break;
                case OriginTag::LFO:        originStr = "LFO";        break;
                case OriginTag::Move:       originStr = "Move";       break;
                case OriginTag::Automation: originStr = "Automation"; break;
                case OriginTag::None:       originStr = "Unknown";    break;
                default:                    originStr = "Unknown";    break;
            }
            driftObj->setProperty ("since_origin", originStr);

            // affected_groups[0].channelId surfaces the (1-based) channel for
            // human-readable error messages.
            if (! record.affectedGroups.empty())
                driftObj->setProperty ("channel_id", record.affectedGroups.front().channelId);

            drifts.add (juce::var (driftObj.release()));
        }
    }

    if (drifts.isEmpty())
        return UndoResult::fail ({}, {});  // empty errorCode → "no staleness, proceed"

    auto details = std::make_unique<juce::DynamicObject>();
    details->setProperty ("drifted_parameters", juce::var (drifts));

    return UndoResult::fail (
        "stale_target",
        "Cannot undo: " + juce::String (drifts.size())
            + " parameter(s) modified by non-MCP origin since this record. "
              "Operator should decide whether to restore the AI's value or keep the current one.",
        juce::var (details.release()));
}

//==============================================================================
UndoResult MCPUndoEngine::undoLast()
{
    ChangeRecord rec;
    if (! undoRing.popBack (rec))
        return UndoResult::fail ("no_history", "No AI-origin changes to undo.");

    // Phase 5b: refuse if a non-MCP origin drifted an affected parameter.
    UndoResult staleness = checkStalenessOrEmpty (rec);
    if (staleness.errorCode == "stale_target")
    {
        undoRing.push (std::move (rec));
        return staleness;
    }

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

    // Phase 5b: staleness check across the entire chain. If any record's
    // affected parameters drifted by a non-MCP origin, refuse the whole
    // batch and restore the chain to the undo ring.
    juce::Array<juce::var> allDrifts;
    for (const auto& rec : chain)
    {
        UndoResult st = checkStalenessOrEmpty (rec);
        if (st.errorCode == "stale_target")
        {
            if (auto* obj = st.details.isObject() ? st.details.getDynamicObject() : nullptr)
            {
                const auto driftsVar = obj->getProperty ("drifted_parameters");
                if (driftsVar.isArray())
                    for (const auto& d : *driftsVar.getArray())
                        allDrifts.add (d);
            }
        }
    }
    if (! allDrifts.isEmpty())
    {
        // Restore the chain (chronological order — oldest at front of `chain`
        // pushes to back of ring, but in our naming chain.back() IS the oldest,
        // so push back-to-front to land newest-on-top).
        for (auto it = chain.rbegin(); it != chain.rend(); ++it)
            undoRing.push (std::move (*it));

        auto details = std::make_unique<juce::DynamicObject>();
        details->setProperty ("drifted_parameters", juce::var (allDrifts));
        return UndoResult::fail (
            "stale_target",
            "Cannot undo batch: " + juce::String (allDrifts.size())
                + " parameter(s) modified by non-MCP origin since the targeted record(s).",
            juce::var (details.release()));
    }

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
