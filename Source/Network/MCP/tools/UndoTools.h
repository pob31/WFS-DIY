#pragma once

#include <JuceHeader.h>
#include "../MCPToolRegistry.h"
#include "../MCPChangeRecords.h"
#include "../MCPUndoEngine.h"

namespace WFSNetwork::Tools::Undo
{

namespace detail
{
    inline juce::var changeRecordToVar (const ChangeRecord& record)
    {
        auto obj = std::make_unique<juce::DynamicObject>();
        obj->setProperty ("timestamp_iso",
                          record.timestamp.toISO8601 (true));
        obj->setProperty ("tool_name",            record.toolName);
        obj->setProperty ("arguments",            record.arguments);
        obj->setProperty ("operator_description", record.operatorDescription);

        juce::Array<juce::var> params;
        for (const auto& p : record.affectedParameters)
            params.add (juce::var (p));
        obj->setProperty ("affected_parameters", juce::var (params));

        juce::Array<juce::var> groups;
        for (const auto& g : record.affectedGroups)
        {
            auto entry = std::make_unique<juce::DynamicObject>();
            entry->setProperty ("channel_id", g.channelId);
            entry->setProperty ("group_name", g.groupName);
            groups.add (juce::var (entry.release()));
        }
        obj->setProperty ("affected_groups", juce::var (groups));

        obj->setProperty ("before_state", record.beforeState);
        obj->setProperty ("after_state",  record.afterState);
        obj->setProperty ("origin",       juce::String ("MCP"));
        obj->setProperty ("is_self_corrected", record.isSelfCorrected);
        return juce::var (obj.release());
    }

    inline juce::var emptyObjectSchema()
    {
        auto schema = std::make_unique<juce::DynamicObject>();
        schema->setProperty ("type", "object");
        schema->setProperty ("properties", juce::var (new juce::DynamicObject()));
        schema->setProperty ("additionalProperties", false);
        return juce::var (schema.release());
    }

    /** Translate an UndoResult from MCPUndoEngine into the ToolResult shape
        the MCP dispatcher expects. Phase 5b: when the engine refuses with
        `stale_target`, surface the staleness details in the success
        envelope (with `is_stale: true`) instead of raising a generic tool
        error. The AI gets to read the drift report and ask the operator. */
    inline ToolResult toolResultFromUndo (const UndoResult& outcome)
    {
        if (outcome.success)
        {
            auto obj = std::make_unique<juce::DynamicObject>();
            obj->setProperty ("operator_description", outcome.operatorDescription);
            obj->setProperty ("before_applied",       outcome.beforeApplied);
            obj->setProperty ("after_applied",        outcome.afterApplied);
            obj->setProperty ("records_affected",     outcome.recordsAffected);
            return ToolResult::ok (juce::var (obj.release()));
        }

        // Stale-target: return as a structured ok result with is_stale flag,
        // so the AI can surface the drift to the operator instead of treating
        // it as a hard tool error.
        if (outcome.errorCode == "stale_target")
        {
            auto obj = std::make_unique<juce::DynamicObject>();
            obj->setProperty ("is_stale",         true);
            obj->setProperty ("refusal_reason",   outcome.errorMessage);
            obj->setProperty ("staleness",        outcome.details);
            return ToolResult::ok (juce::var (obj.release()));
        }

        return ToolResult::error (outcome.errorCode, outcome.errorMessage);
    }
}

//==============================================================================
// mcp.undo_last_ai_change — Phase 5a real implementation
//==============================================================================

inline juce::var buildUndoSchema()
{
    auto idx = std::make_unique<juce::DynamicObject>();
    idx->setProperty ("type", "integer");
    idx->setProperty ("minimum", 0);
    idx->setProperty ("description",
        "Optional. Index into the undo ring buffer (0 = oldest, size-1 = newest). "
        "Omit to undo the most recent record. When set, performs targeted undo: "
        "the target record AND any later records whose affected_groups intersect "
        "are reversed together as a batch. Use mcp_get_ai_change_history to read "
        "the buffer first if you need to find a specific record.");

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("record_index", juce::var (idx.release()));

    auto schema = std::make_unique<juce::DynamicObject>();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (props.release()));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema.release());
}

inline ToolDescriptor describeUndo (MCPUndoEngine& engine)
{
    ToolDescriptor d;
    d.name        = "mcp_undo_last_ai_change";
    d.description = "Undo a previous AI-initiated state change. With no arguments, "
                    "reverses only the most recent state-modifying tool call. With "
                    "`record_index`, performs targeted undo: reverses the record at "
                    "that index AND any later records whose affected_groups intersect, "
                    "as a single batch. Read-only tool calls do NOT generate undo "
                    "records. Returns no_history if nothing to undo, or invalid_index "
                    "for an out-of-range record_index.";
    d.inputSchema   = buildUndoSchema();
    d.modifiesState = false;  // The undo engine handles its own change-record bookkeeping
                              // (it pushes the undone record onto the redo ring); the
                              // dispatcher should NOT also push a generic record for
                              // this call. Keep modifiesState=false to skip that.
    d.handler = [&engine] (const juce::var& args, ChangeRecord*) -> ToolResult
    {
        // Optional record_index argument routes between undoLast (no arg) and
        // undoByIndex (targeted with dependency chasing).
        if (args.isObject())
        {
            auto* obj = args.getDynamicObject();
            if (obj != nullptr && obj->hasProperty ("record_index"))
            {
                const int idx = static_cast<int> (obj->getProperty ("record_index"));
                return detail::toolResultFromUndo (engine.undoByIndex (idx));
            }
        }
        return detail::toolResultFromUndo (engine.undoLast());
    };
    return d;
}

//==============================================================================
// mcp.redo_last_undone_ai_change — Phase 5a real implementation
//==============================================================================

inline ToolDescriptor describeRedo (MCPUndoEngine& engine)
{
    ToolDescriptor d;
    d.name        = "mcp_redo_last_undone_ai_change";
    d.description = "Re-apply the most recently undone AI change. The redo stack is "
                    "cleared whenever a new state-modifying tool call lands (standard "
                    "undo/redo semantics), so redo is only available immediately after "
                    "an undo. Returns no_redo if nothing to redo.";
    d.inputSchema   = detail::emptyObjectSchema();
    d.modifiesState = false;  // same rationale as describeUndo
    d.handler = [&engine] (const juce::var&, ChangeRecord*) -> ToolResult
    {
        return detail::toolResultFromUndo (engine.redoLast());
    };
    return d;
}

//==============================================================================
// mcp.get_ai_change_history — fully implemented (read-only over the ring buffer)
//==============================================================================

inline juce::var buildHistorySchema()
{
    auto limit = std::make_unique<juce::DynamicObject>();
    limit->setProperty ("type", "integer");
    limit->setProperty ("minimum", 1);
    limit->setProperty ("maximum", 100);
    limit->setProperty ("description", "Max number of records to return (newest last). Omit for all.");

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("limit", juce::var (limit.release()));

    auto schema = std::make_unique<juce::DynamicObject>();
    schema->setProperty ("type", "object");
    schema->setProperty ("properties", juce::var (props.release()));
    schema->setProperty ("additionalProperties", false);
    return juce::var (schema.release());
}

inline ToolDescriptor describeGetHistory (MCPChangeRecordBuffer& buffer)
{
    ToolDescriptor d;
    d.name        = "mcp_get_ai_change_history";
    d.description = "Read the AI change-record ring buffer — every state-modifying tool "
                    "call this MCP server has executed in the last 100 entries. Useful "
                    "for explaining 'what did I just do?' in voice flows. Read-only.";
    d.inputSchema   = buildHistorySchema();
    d.modifiesState = false;
    d.handler = [&buffer] (const juce::var& args, ChangeRecord*) -> ToolResult
    {
        int limit = -1;
        if (args.isObject())
        {
            auto* obj = args.getDynamicObject();
            if (obj != nullptr && obj->hasProperty ("limit"))
                limit = static_cast<int> (obj->getProperty ("limit"));
        }

        auto records = buffer.getRecent (limit);

        juce::Array<juce::var> serialized;
        for (const auto& r : records)
            serialized.add (detail::changeRecordToVar (r));

        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("records", juce::var (serialized));
        result->setProperty ("count", serialized.size());
        result->setProperty ("buffer_capacity", buffer.capacity());
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

} // namespace WFSNetwork::Tools::Undo
