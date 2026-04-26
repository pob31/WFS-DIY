#pragma once

#include <JuceHeader.h>
#include <functional>

namespace WFSNetwork
{

class WFSValueTreeStateRef;  // forward — actual ref lives in MCPDispatcher

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

/** Per-tool descriptor + handler. The handler receives validated arguments
    as a juce::var (parsed from JSON) and returns a ToolResult. Block 5
    populates real handlers; in Block 3 the registry is empty. */
struct ToolDescriptor
{
    juce::String name;
    juce::String description;
    juce::var parametersSchema;  // JSON-Schema object, exposed via tools/list
    std::function<ToolResult (const juce::var&)> handler;
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
