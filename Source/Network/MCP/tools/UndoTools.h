#pragma once

#include <JuceHeader.h>
#include "../MCPToolRegistry.h"
#include "../MCPChangeRecords.h"

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

    inline juce::var pendingPhase5Result()
    {
        auto obj = std::make_unique<juce::DynamicObject>();
        obj->setProperty ("status", "not_yet_implemented");
        obj->setProperty ("phase",  "Pending Phase 5 (AI undo surfacing). The change-record "
                                    "ring buffer is populated as of Phase 1, but reversal "
                                    "logic, staleness detection, and the cross-actor "
                                    "notification side-channel are not yet wired up.");
        return juce::var (obj.release());
    }
}

//==============================================================================
// mcp.undo_last_ai_change — stub until Phase 5
//==============================================================================

inline ToolDescriptor describeUndo()
{
    ToolDescriptor d;
    d.name        = "mcp.undo_last_ai_change";
    d.description = "Undo the most recent AI-initiated state change. (Phase 1 stub: "
                    "registered now so clients can depend on it from day one; the real "
                    "implementation lands in Phase 5 alongside the growing-toast overlay "
                    "and Cmd/Ctrl-Alt-Z keyboard shortcut.)";
    d.inputSchema   = detail::emptyObjectSchema();
    d.modifiesState = false;  // stub doesn't actually mutate; Phase 5 flips this
    d.handler = [] (const juce::var&, ChangeRecord*) -> ToolResult
    {
        return ToolResult::ok (detail::pendingPhase5Result());
    };
    return d;
}

//==============================================================================
// mcp.redo_last_undone_ai_change — stub until Phase 5
//==============================================================================

inline ToolDescriptor describeRedo()
{
    ToolDescriptor d;
    d.name        = "mcp.redo_last_undone_ai_change";
    d.description = "Redo the most recently undone AI-initiated change. (Phase 1 stub; "
                    "real implementation in Phase 5.)";
    d.inputSchema   = detail::emptyObjectSchema();
    d.modifiesState = false;
    d.handler = [] (const juce::var&, ChangeRecord*) -> ToolResult
    {
        return ToolResult::ok (detail::pendingPhase5Result());
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
