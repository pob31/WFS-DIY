#pragma once

#include <JuceHeader.h>

namespace WFSNetwork
{

/** One entry in the WFS knowledge-base resource catalog. Each entry maps an
    `wfs://knowledge/<topic>` URI to a markdown file plus the metadata the
    MCP `resources/list` response needs (name + description) so the AI can
    decide whether the resource is relevant before fetching it. */
struct ResourceEntry
{
    juce::String uri;          // "wfs://knowledge/psychoacoustics"
    juce::String name;         // "How humans localize sound"
    juce::String description;  // 1–3 sentence hook for the AI
    juce::String mimeType;     // "text/markdown" for everything in v1
    juce::File   file;         // resolved at construction by joining resourcesDir + relative filename
};

/** Catalog of MCP knowledge resources, hard-coded to match the table in
    Documentation/MCP/specs/MCP_RESOURCES.md §Resource catalog. Constructed
    once at MCPServer startup; the registry is then consulted by
    MCPDispatcher's resources/list + resources/read handlers.

    Build-time: the markdown files live at Documentation/MCP/resources/.
    Runtime: a postbuild step copies them next to the executable (under
    MCP/resources/), and MainComponent's path-resolver finds the directory
    via the same fallback chain it uses for generated_tools.json and lang/. */
class MCPResourceRegistry
{
public:
    /** Build the catalog. `resourcesDir` is expected to contain
        `knowledge_*.md` files; the constructor resolves each entry's
        `file` lazily by joining the directory with the canonical filename
        per entry. Missing files are NOT pruned at construction — they are
        reported as a structured `internal_error` at read time so a stale
        deployment surfaces visibly rather than silently shrinking the
        resources/list response. */
    explicit MCPResourceRegistry (const juce::File& resourcesDir);

    /** Snapshot for resources/list. */
    const std::vector<ResourceEntry>& all() const noexcept { return entries; }

    /** Look up by URI. Returns nullptr if the URI isn't in the catalog. */
    const ResourceEntry* findByURI (const juce::String& uri) const;

    int size() const noexcept { return static_cast<int> (entries.size()); }

private:
    std::vector<ResourceEntry> entries;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MCPResourceRegistry)
};

} // namespace WFSNetwork
