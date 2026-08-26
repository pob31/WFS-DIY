#pragma once

#include <JuceHeader.h>

class WFSValueTreeState;

namespace spatcore::control::mcp { class MCPToolRegistry; }

namespace WFSNetwork
{

using spatcore::control::mcp::MCPToolRegistry;
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
    `nudge_tools[]` pass on top. This signature is stable for both.

    `onTopologyChanged` is invoked after a successful write to one of the channel
    COUNT parameters. Without it the tree changed and nothing re-prepared the
    renderer, routing matrices, patch rows or meters — the tool appeared to work
    and left the engine describing a channel list that no longer existed. The
    hand-written create/delete tools have always fired it; the generated count
    tools never did. Pass nullptr where there is nothing to notify. */
LoadStats loadGeneratedTools (MCPToolRegistry& registry,
                              WFSValueTreeState& state,
                              const juce::File& jsonPath,
                              MCPLogger& mcpLogger,
                              const std::function<void()>* onTopologyChanged = nullptr,
                              const std::function<void (int)>* onGradientMapChanged = nullptr);

} // namespace Tools::Generated
} // namespace WFSNetwork
