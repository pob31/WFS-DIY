#pragma once

#include <JuceHeader.h>
#include "MCPLogger.h"
#include "MCPToolRegistry.h"

class WFSValueTreeState;

namespace WFSNetwork
{

/** JSON-RPC method dispatcher.

    Block 3 (current): returns a placeholder JSON-RPC error for every
    request. Just enough that POST /mcp → 200 OK with a valid JSON-RPC
    envelope, so that the transport can be smoke-tested end-to-end.

    Block 4 wires up:
      - initialize / initialized   (handshake + capability negotiation)
      - tools/list                 (drains MCPToolRegistry)
      - tools/call                 (validate → before-state → message-thread
                                    hop with OriginTagScope { MCP } → after-state
                                    → emit change record → respond)

    Threading: the transport calls handleRequest on its own worker
    thread. The dispatcher is responsible for hopping to the message
    thread for any state mutation (Block 4 implements that). Block 3
    is read-only at the transport boundary so threading is moot here. */
class MCPDispatcher
{
public:
    MCPDispatcher (WFSValueTreeState& state,
                   MCPToolRegistry& registry,
                   MCPLogger& mcpLogger);

    /** Receive a raw HTTP body, return a JSON-RPC envelope as a string.
        Always returns a syntactically valid JSON-RPC 2.0 envelope, even
        for malformed input — the transport layer should never have to
        synthesize a response itself. */
    juce::String handleRequest (const juce::String& body,
                                const juce::String& clientIP,
                                int clientPort);

private:
    juce::String makeJsonRpcError (const juce::var& id,
                                   int code,
                                   const juce::String& message,
                                   const juce::var& data = {}) const;

    WFSValueTreeState& state;
    MCPToolRegistry& registry;
    MCPLogger& mcpLogger;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MCPDispatcher)
};

} // namespace WFSNetwork
