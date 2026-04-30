#pragma once

#include <JuceHeader.h>
#include "MCPLogger.h"
#include "MCPToolRegistry.h"
#include "MCPChangeRecords.h"
#include "MCPUndoEngine.h"
#include "MCPResourceRegistry.h"
#include "MCPPromptRegistry.h"
#include "MCPTierEnforcement.h"
#include "MCPOSCQueryAuditor.h"
#include "MCPDispatcher.h"
#include "MCPTransport.h"

class WFSValueTreeState;
class WFSFileManager;

namespace WFSNetwork
{

class OSCLogger;

/** Top-level MCP server: owns the transport, dispatcher, tool registry,
    and logger; surfaces a small lifecycle API to the rest of the app.

    Construction is cheap (no threads, no sockets). Call start() to bind
    a port and begin accepting connections; stop() drains and tears down.

    Phase 1 wiring:
      Phase 1 Block 3 — this file: constructs all five sub-components,
      wires Transport→Dispatcher, registers no tools.
      Phase 1 Block 4 — fleshes out Dispatcher with real JSON-RPC.
      Phase 1 Block 5 — populates the tool registry.
      Phase 1 Block 6 — MainComponent owns one of these and a
      NetworkTab UI controls start/stop/port. */
class MCPServer
{
public:
    static constexpr int kDefaultPort = 7400;

    MCPServer (WFSValueTreeState& state,
               WFSFileManager& fileManager,
               OSCLogger& networkLogger,
               const juce::File& generatedToolsJson,
               const juce::File& knowledgeResourcesDir);
    ~MCPServer();

    /** Start listening on the given port. Returns true if the listener
        was launched. Loopback-only by default; pass false to bind on the
        active network interface (required for remote AI clients). */
    bool start (int port = kDefaultPort, bool loopbackOnly = true);

    /** Stop the listener and drain in-flight requests. */
    void stop();

    bool isRunning() const noexcept    { return transport != nullptr && transport->isRunning(); }
    int  getBoundPort() const noexcept { return transport != nullptr ? transport->getBoundPort() : 0; }

    /** Tool registry — exposed so Phase 1 Block 5 can register the
        hand-written tools, and Phase 2 the auto-generated ones. */
    MCPToolRegistry& getToolRegistry() noexcept { return *registry; }

    /** Ring buffer of recent state-modifying tool calls. Phase 5 surfaces
        these via the AI-undo overlay + keyboard shortcuts; Phase 1 just
        populates it. Read-only tool calls do NOT produce records. */
    MCPChangeRecordBuffer& getChangeRecords() noexcept { return *changeRecords; }

    /** Knowledge-resource catalog (Phase 3). Populated at startup;
        consulted by the dispatcher's resources/list and resources/read
        handlers. */
    MCPResourceRegistry& getResources() noexcept { return *resourceRegistry; }

    /** Workflow-prompt catalog (Phase 4). Hand-curated set of multi-step
        templates surfaced via prompts/list and prompts/get. */
    MCPPromptRegistry& getPrompts() noexcept { return *promptRegistry; }

    /** Backend undo/redo engine (Phase 5a). Drives mcp.undo_last_ai_change
        and mcp.redo_last_undone_ai_change. Phase 5c will also call into
        this directly for the keyboard-shortcut path. */
    MCPUndoEngine& getUndoEngine() noexcept { return *undoEngine; }

    /** Phase 6 — tier enforcement. Owns confirmation tokens, the safety
        gate state, and the dry-run flag. The Network tab UI binds to
        this to show + control gate / dry-run. */
    MCPTierEnforcement& getTierEnforcement() noexcept { return *tierEnforcement; }

    /** Phase 7 — kick the OSCQuery startup cross-check. Caller passes the
        URL (typically `http://127.0.0.1:<httpPort>/`) where OSCQuery is
        bound; the auditor walks `generated_tools.json` and logs any tool
        whose declared OSC path is missing from the live tree. Safe to
        call multiple times — re-runs replace the previous audit. Pass an
        empty URL to disable / cancel. */
    void runOSCQueryAudit (const juce::String& url);

private:
    WFSValueTreeState& valueTreeState;
    WFSFileManager& fileManager;
    std::unique_ptr<MCPLogger> mcpLogger;
    std::unique_ptr<MCPToolRegistry> registry;
    std::unique_ptr<MCPChangeRecordBuffer> changeRecords;
    std::unique_ptr<MCPUndoEngine> undoEngine;
    std::unique_ptr<MCPResourceRegistry> resourceRegistry;
    std::unique_ptr<MCPPromptRegistry> promptRegistry;
    std::unique_ptr<MCPTierEnforcement> tierEnforcement;
    std::unique_ptr<MCPDispatcher> dispatcher;
    std::unique_ptr<MCPTransport> transport;
    std::unique_ptr<MCPOSCQueryAuditor> oscQueryAuditor;
    juce::File generatedToolsJsonPath;  // Phase 7: remembered for the auditor

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MCPServer)
};

} // namespace WFSNetwork
