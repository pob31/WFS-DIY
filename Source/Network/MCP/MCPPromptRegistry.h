#pragma once

#include <JuceHeader.h>

namespace WFSNetwork
{

/** One named argument a prompt accepts at invocation time. The MCP spec
    surfaces these in `prompts/list` so clients can render an input form
    or auto-fill from conversation context. */
struct PromptArgument
{
    juce::String name;         // "reference_source_name"
    juce::String description;  // human-readable hint, shown to the operator
    bool required = false;     // dispatcher rejects prompts/get without this arg if true
};

/** One entry in the workflow-prompt catalog. The `templateBody` is a
    static, single-message system prompt verbatim from
    Documentation/MCP/prompts/MCP_PROMPTS.md; the dispatcher prepends a
    short "Invoked with: ..." preamble when arguments are passed so the
    AI sees both the recipe and the parameterization. */
struct PromptEntry
{
    juce::String name;            // "system_tuning_workflow"
    juce::String description;     // 1-line summary for the prompts/list response
    std::vector<PromptArgument> arguments;
    juce::String templateBody;    // raw template text, no placeholder interpolation in v1
};

/** Catalog of MCP workflow prompts, hard-coded to match the 6 entries in
    Documentation/MCP/prompts/MCP_PROMPTS.md §Prompt catalog. Stable per
    the spec ("Resist the temptation to add lots of prompts") so an inline
    catalog is preferred over runtime file IO. */
class MCPPromptRegistry
{
public:
    MCPPromptRegistry();

    /** Snapshot for prompts/list. */
    const std::vector<PromptEntry>& all() const noexcept { return entries; }

    /** Look up by name. Returns nullptr if unknown. */
    const PromptEntry* findByName (const juce::String& name) const;

    int size() const noexcept { return static_cast<int> (entries.size()); }

private:
    std::vector<PromptEntry> entries;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MCPPromptRegistry)
};

} // namespace WFSNetwork
