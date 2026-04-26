#pragma once

#include <JuceHeader.h>
#include "MCPLogger.h"

namespace WFSNetwork
{

/** Phase 7 — startup OSCQuery cross-check.

    The MCP tool surface is generated from `WFS-UI_*.csv`; the live OSC
    routing is hand-coded in `OSCMessageRouter`; the OSCQuery server
    advertises whatever `OSCMessageRouter` knows about. These three are
    independently maintained, so it's easy for the CSV to declare an OSC
    path the live router doesn't actually handle (rename in code without
    CSV update, CSV typo, channel-cap refactor leaving stale templates).
    The AI calls succeed at the JSON-RPC level but the resulting OSC
    packet goes nowhere — silent failure that's hard to diagnose.

    This auditor runs once at MCP server startup on a background
    `juce::Thread`. It fetches the OSCQuery JSON tree at
    `http://127.0.0.1:<httpPort>/`, walks `CONTENTS` recursively into a
    set of full paths, then walks `generated_tools.json` and reports any
    tool whose `internal_osc_path` (or `internal_osc_path_template`
    stripped of `{...}`) is missing from the live tree. Drift is logged
    via `MCPLogger` so it surfaces in the Network Log under
    Protocol::MCP. Capped at 20 error lines to avoid log spam. */
class MCPOSCQueryAuditor : private juce::Thread
{
public:
    MCPOSCQueryAuditor (MCPLogger& logger,
                        juce::File generatedToolsJson,
                        juce::String oscQueryUrl);

    ~MCPOSCQueryAuditor() override;

    /** Kick the audit. Returns immediately; results land in the
        Network Log within a few seconds. Safe to call multiple times
        — re-entry while the previous run is in flight is a no-op. */
    void runAudit();

private:
    void run() override;

    static void collectPaths (const juce::var& node,
                              const juce::String& prefix,
                              std::set<juce::String>& out);

    static juce::String stripPlaceholder (const juce::String& templatePath);

    MCPLogger&    mcpLogger;
    juce::File    generatedToolsJson;
    juce::String  oscQueryUrl;

    static constexpr int kStartupDelayMs        = 1000;  // give OSCQuery time to come up
    static constexpr int kHttpTimeoutMs         = 5000;
    static constexpr int kMaxDriftLogsPerRun    = 20;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MCPOSCQueryAuditor)
};

} // namespace WFSNetwork
