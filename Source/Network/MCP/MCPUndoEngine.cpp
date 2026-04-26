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

UndoResult MCPUndoEngine::redoLast()
{
    ChangeRecord rec;
    if (! redoRing->popBack (rec))
        return UndoResult::fail ("no_redo", "No undone AI changes available to redo.");

    UndoResult outcome = applyForward (rec);
    if (! outcome.success)
    {
        redoRing->push (rec);
        return outcome;
    }

    undoRing.push (std::move (rec));
    return outcome;
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
