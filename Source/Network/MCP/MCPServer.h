#pragma once

#include <JuceHeader.h>
#include "MCPLogger.h"
#include "MCPToolRegistry.h"
#include "MCPDispatcher.h"
#include "MCPTransport.h"

class WFSValueTreeState;

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

    MCPServer (WFSValueTreeState& state, OSCLogger& networkLogger);
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

private:
    WFSValueTreeState& valueTreeState;
    std::unique_ptr<MCPLogger> mcpLogger;
    std::unique_ptr<MCPToolRegistry> registry;
    std::unique_ptr<MCPDispatcher> dispatcher;
    std::unique_ptr<MCPTransport> transport;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MCPServer)
};

} // namespace WFSNetwork
