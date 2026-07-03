#pragma once

#include <JuceHeader.h>
#include "../OSCLogger.h"
#include "../OSCProtocolTypes.h"
#include "MCPCompat.h"

namespace WFSNetwork
{

/** Thin shim over OSCLogger that surfaces MCP traffic in the existing
    Network Log window under Protocol::MCP. Every entry written through
    this class carries OriginTag::MCP so it round-trips through the same
    Origin column the rest of Phase 1 populates.

    Lifetime: owned by MCPServer; the underlying OSCLogger lives on the
    main app for the lifetime of the process.

    Implements the core's MCPLogSink so the spatcore transport/dispatcher
    can log without knowing the app's LogEntry/Protocol dialect. */
class MCPLogger : public MCPLogSink
{
public:
    explicit MCPLogger (OSCLogger& underlyingLogger) : logger (underlyingLogger) {}

    /** One-line lifecycle/info message — server start, port fallback, etc. */
    void logInfo (const juce::String& message) override;

    /** Inbound JSON-RPC request from a client. `clientIP` is the connecting peer. */
    void logRequest (const juce::String& method,
                     const juce::String& payloadAbbreviated,
                     const juce::String& clientIP,
                     int clientPort) override;

    /** Outbound response to a client. */
    void logResponse (const juce::String& method,
                      const juce::String& payloadAbbreviated,
                      const juce::String& clientIP,
                      int clientPort) override;

    /** Error condition (transport failure, malformed JSON, etc.). */
    void logError (const juce::String& message) override;

private:
    OSCLogger& logger;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MCPLogger)
};

} // namespace WFSNetwork
