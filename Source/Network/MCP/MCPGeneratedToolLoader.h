#pragma once

#include <JuceHeader.h>

class WFSValueTreeState;

namespace WFSNetwork
{

class MCPToolRegistry;
class MCPLogger;

namespace Tools::Generated
{

struct LoadStats
{
    int toolsLoaded      = 0;
    int nudgeToolsLoaded = 0;
    int skipped          = 0;
    juce::String errorMessage;  // empty on success
};

/** Load and register the auto-generated MCP tools from
    `Source/Network/MCP/generated_tools.json` (or wherever the build
    placed it).

    Failures (missing file, parse error, malformed entries) are logged
    via MCPLogger and reported in LoadStats; the function never throws.
    The MCP server keeps starting even if the file isn't there — only
    the auto-generated surface is missing in that case.

    Phase 2 Block 1 implements the `tools[]` pass; Block 2 will add the
    `nudge_tools[]` pass on top. This signature is stable for both. */
LoadStats loadGeneratedTools (MCPToolRegistry& registry,
                              WFSValueTreeState& state,
                              const juce::File& jsonPath,
                              MCPLogger& mcpLogger);

} // namespace Tools::Generated
} // namespace WFSNetwork
