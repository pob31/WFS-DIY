#pragma once

#include <juce_core/juce_core.h>

namespace spatcore::control::mcp
{

/** Core-facing logging interface for the MCP transport + dispatcher.

    The app implements this and decides where MCP traffic surfaces
    (WFS-DIY: MCPLogger forwards into the Network Log window's OSCLogger
    under Protocol::MCP with OriginTag::MCP). The core never sees the
    app's log-entry dialect — only these four calls.

    Threading: called from the SimpleWeb HTTP worker thread as well as
    the message thread; implementations must be thread-safe. */
class MCPLogSink
{
public:
    virtual ~MCPLogSink() = default;

    /** One-line lifecycle/info message — server start, port fallback, etc. */
    virtual void logInfo (const juce::String& message) = 0;

    /** Inbound JSON-RPC request from a client. `clientIP` is the connecting peer. */
    virtual void logRequest (const juce::String& method,
                             const juce::String& payloadAbbreviated,
                             const juce::String& clientIP,
                             int clientPort) = 0;

    /** Outbound response to a client. */
    virtual void logResponse (const juce::String& method,
                              const juce::String& payloadAbbreviated,
                              const juce::String& clientIP,
                              int clientPort) = 0;

    /** Error condition (transport failure, malformed JSON, etc.). */
    virtual void logError (const juce::String& message) = 0;
};

} // namespace spatcore::control::mcp
