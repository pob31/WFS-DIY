#pragma once

#include <JuceHeader.h>
#include <functional>
#include "MCPChangeRecords.h"

namespace WFSNetwork
{

/** Outcome of a tool invocation, before serialization to JSON-RPC envelope. */
struct ToolResult
{
    bool success = false;
    juce::var value;        // tool-specific payload on success
    juce::String errorCode; // populated when success == false
    juce::String errorMessage;

    static ToolResult ok (juce::var v = {})           { return { true, std::move (v), {}, {} }; }
    static ToolResult error (juce::String code, juce::String msg)
    {
        return { false, {}, std::move (code), std::move (msg) };
    }
};

/** Per-tool descriptor + handler. Handlers always run on the JUCE message
    thread (the dispatcher hops there inside an OriginTagScope of MCP).

    State-modifying tools should set `modifiesState = true` and populate
    the optional `record` parameter passed to the handler — at minimum
    `operatorDescription`, `affectedParameters`, `affectedGroups`,
    `beforeState`, `afterState`. Read-only tools can ignore `record`. */
struct ToolDescriptor
{
    juce::String name;
    juce::String description;
    juce::var inputSchema;       // JSON-Schema object, exposed via tools/list
    bool modifiesState = false;  // drives change-record capture in dispatcher

    std::function<ToolResult (const juce::var& args, ChangeRecord* record)> handler;
};

/** Registry of tools the MCP server exposes. Block 5 fills this with the
    five hand-written Phase 1 tools and the three undo stubs; Phase 2 layers
    auto-generated tools on top. */
class MCPToolRegistry
{
public:
    MCPToolRegistry() = default;

    void registerTool (ToolDescriptor descriptor);

    /** Returns nullptr if no tool with that name is registered. */
    const ToolDescriptor* find (const juce::String& name) const;

    /** Snapshot of all registered descriptors (for tools/list responses). */
    const std::vector<ToolDescriptor>& all() const noexcept { return tools; }

    int size() const noexcept { return static_cast<int> (tools.size()); }

private:
    std::vector<ToolDescriptor> tools;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MCPToolRegistry)
};

} // namespace WFSNetwork
