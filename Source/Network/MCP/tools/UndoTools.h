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
        the MCP dispatcher expects. */
    inline ToolResult toolResultFromUndo (const UndoResult& outcome)
    {
        if (! outcome.success)
            return ToolResult::error (outcome.errorCode, outcome.errorMessage);

        auto obj = std::make_unique<juce::DynamicObject>();
        obj->setProperty ("operator_description", outcome.operatorDescription);
        obj->setProperty ("before_applied",       outcome.beforeApplied);
        obj->setProperty ("after_applied",        outcome.afterApplied);
        obj->setProperty ("records_affected",     outcome.recordsAffected);
        return ToolResult::ok (juce::var (obj.release()));
    }
}

//==============================================================================
// mcp.undo_last_ai_change — Phase 5a real implementation
//==============================================================================

inline ToolDescriptor describeUndo (MCPUndoEngine& engine)
{
    ToolDescriptor d;
    d.name        = "mcp.undo_last_ai_change";
    d.description = "Undo the most recent AI-initiated state change. Reverses the "
                    "writes the corresponding tool call performed and moves the record "
                    "onto the redo stack. Read-only tool calls (session.get_state, "
                    "snapshot.list, etc.) do NOT generate undo records, so this only "
                    "reverses state mutations. Returns no_history if nothing to undo.";
    d.inputSchema   = detail::emptyObjectSchema();
    // Block 2 will extend the schema with an optional `record_index` for
    // targeted undo. For Block 1, no args = "undo the last record".
    d.modifiesState = false;  // The undo engine handles its own change-record bookkeeping
                              // (it pushes the undone record onto the redo ring); the
                              // dispatcher should NOT also push a generic record for
                              // this call. Keep modifiesState=false to skip that.
    d.handler = [&engine] (const juce::var&, ChangeRecord*) -> ToolResult
    {
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
    d.name        = "mcp.redo_last_undone_ai_change";
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
    d.name        = "mcp.get_ai_change_history";
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
