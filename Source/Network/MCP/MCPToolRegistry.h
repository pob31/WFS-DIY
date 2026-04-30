#pragma once

#include <JuceHeader.h>
#include <functional>
#include "MCPChangeRecords.h"

namespace WFSNetwork
{

/** Tier-marker suffixes appended to a tool's description so the model
    can decide upfront whether a call needs operator confirmation. The
    `_meta.tier` field is also surfaced on each tool entry, but most MCP
    clients don't pass `_meta` to the model — only `name`, `description`,
    and `inputSchema`. Without baking the tier into the description, the
    model has to either probe the tool or call mcp_describe_parameters
    for every write it considers. ASCII-only on purpose (juce::String's
    `const char*` ctor asserts on non-ASCII bytes in debug). The same
    text is duplicated in tools/generate_mcp_tools.py — keep the two
    surfaces textually identical. */
inline constexpr const char* kTier2DescriptionSuffix =
    " [TIER 2: needs a confirm-token round trip OR an open Tier-2 "
    "auto-confirm / safety-gate window.]";

inline constexpr const char* kTier3DescriptionSuffix =
    " [TIER 3: destructive - refused unless the operator's safety gate "
    "is open; with the gate open, executes immediately.]";

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

    /** Phase 6 tier:
          1 = low-risk, executes immediately;
          2 = medium-risk, requires a confirmation token (two-step call);
          3 = destructive, requires confirmation AND an open safety gate.
        Auto-generated tools inherit their tier from generated_tools.json
        ("tier" field); hand-written tools should set this explicitly.
        Defaults to 1 to avoid silently gating tools whose author forgot
        to classify them. */
    int tier = 1;

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
