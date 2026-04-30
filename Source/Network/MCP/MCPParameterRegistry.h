#pragma once

#include <JuceHeader.h>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace WFSNetwork
{

class MCPLogger;

/** Per-parameter descriptor surfaced through `mcp_describe_parameters`.

    The registry parses `Source/Network/MCP/generated_tools.json` once at
    server startup and exposes:
      - an O(1) whitelist of canonical parameter variable names (consumed
        by the wfs_set_parameter escape hatch to fail-loudly on unknown
        names with a did-you-mean suggestion);
      - a flat list of registry records that the describe-parameters tool
        can filter and return.

    Records are derived from the auto-generated manifest, so re-running
    `tools/generate_mcp_tools.py` and rebuilding the app is the single
    source of truth — no separate hand-maintained schema. */
struct ParameterRegistryRecord
{
    juce::String     variable;       // canonical name (from internal_variable)
    juce::String     toolName;       // generated tool that writes this param
    juce::String     scope;          // "global" / "input" / "output" / "reverb" / "cluster" / "eq_band"
    juce::String     type;           // JSON-Schema "integer" / "number" / "string"
    std::optional<double> minValue;
    std::optional<double> maxValue;
    juce::StringArray enumValues;    // empty if not an enum
    juce::var        defaultValue;   // void-var if no CSV default
    juce::String     description;    // straight from the tool's description
    juce::String     unit;           // best-effort extracted from description, or empty
    juce::String     oscPath;
    int              tier = 1;
    juce::String     groupKey;
    juce::String     csvSection;
    juce::StringArray synonyms;      // alias names that resolve to this canonical variable
};

class MCPParameterRegistry
{
public:
    /** Singleton accessor. The first call to `loadFromManifest` populates
        the records; subsequent reads are lock-free. */
    static MCPParameterRegistry& getInstance();

    /** Parse `generated_tools.json` at the given path and populate the
        registry. Idempotent — calling twice replaces the previous load.
        Failures (missing file, parse error) leave the registry empty and
        log a warning via the supplied MCPLogger.

        Threading: must be called from the JUCE message thread before any
        MCP tool handler runs (typically once during MCPServer construction,
        right before the loader registers tools). After return, all const
        accessors below are safe to call from any thread. */
    void loadFromManifest (const juce::File& jsonPath, MCPLogger& mcpLogger);

    /** Explicit reset — used by unit tests. Production code never calls this. */
    void clear();

    /** O(1) check used by wfs_set_parameter to fail loudly on unknown names.
        Accepts both canonical names and registered synonyms. */
    bool isKnown (const juce::String& variable) const noexcept;

    /** Translate a synonym (e.g. `stageOriginX`) to its canonical variable
        name (e.g. `originWidth`). Returns the input unchanged if no synonym
        is registered for it — callers should always feed the result of this
        through their own variable lookup. */
    juce::String canonicalize (const juce::String& variable) const;

    /** Top-N closest matches by Levenshtein distance ≤ maxDistance.
        Returns an empty array when nothing comes within the bound. */
    juce::StringArray suggestSimilar (const juce::String& variable,
                                       int maxResults = 3,
                                       int maxDistance = 3) const;

    /** Filtered view used by `mcp_describe_parameters`. All filters are
        AND-combined; an empty/blank value matches everything. */
    std::vector<ParameterRegistryRecord> filter (const juce::String& prefix,
                                                  const juce::String& scope,
                                                  const juce::String& groupKey) const;

    /** Total record count (includes records produced from nudge tools too,
        deduplicated by `variable`). */
    int size() const noexcept;

private:
    MCPParameterRegistry() = default;

    /** Wire the hard-coded synonyms (currently just the stage-origin
        aliases) into both knownVariables and the records' synonym arrays.
        Runs at the end of loadFromManifest so synonyms automatically
        appear in the registry view. */
    void installSynonyms();

    std::vector<ParameterRegistryRecord> records;
    std::unordered_set<juce::String>     knownVariables;
    /** Synonym → canonical map. Populated at load time from the
        hand-maintained list in installSynonyms(). */
    std::unordered_map<juce::String, juce::String> synonymToCanonical;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MCPParameterRegistry)
};

} // namespace WFSNetwork
