#include "MCPLogger.h"

namespace WFSNetwork
{

namespace
{
    constexpr int kAbbreviatedPayloadCap = 200;

    juce::String abbreviate (const juce::String& payload)
    {
        if (payload.length() <= kAbbreviatedPayloadCap)
            return payload;
        return payload.substring (0, kAbbreviatedPayloadCap) + " ...";
    }

    LogEntry makeEntry (const juce::String& direction,
                        const juce::String& method,
                        const juce::String& payloadAbbreviated,
                        const juce::String& peerIP,
                        int peerPort)
    {
        LogEntry entry;
        entry.timestamp = juce::Time::getCurrentTime();
        entry.direction = direction;
        entry.ipAddress = peerIP;
        entry.port = peerPort;
        entry.targetIndex = -1;
        entry.address = method;
        entry.arguments = abbreviate (payloadAbbreviated);
        entry.protocol = Protocol::MCP;
        entry.transport = ConnectionMode::TCP;  // Streamable HTTP runs over TCP
        entry.origin = OriginTag::MCP;
        return entry;
    }
}

void MCPLogger::logInfo (const juce::String& message)
{
    LogEntry entry;
    entry.timestamp = juce::Time::getCurrentTime();
    entry.direction = "--";
    entry.targetIndex = -1;
    entry.address = message;
    entry.protocol = Protocol::MCP;
    entry.origin = OriginTag::MCP;
    logger.logEntry (entry);
}

void MCPLogger::logRequest (const juce::String& method,
                            const juce::String& payloadAbbreviated,
                            const juce::String& clientIP,
                            int clientPort)
{
    logger.logEntry (makeEntry ("Rx", method, payloadAbbreviated, clientIP, clientPort));
}

void MCPLogger::logResponse (const juce::String& method,
                             const juce::String& payloadAbbreviated,
                             const juce::String& clientIP,
                             int clientPort)
{
    logger.logEntry (makeEntry ("Tx", method, payloadAbbreviated, clientIP, clientPort));
}

void MCPLogger::logError (const juce::String& message)
{
    LogEntry entry;
    entry.timestamp = juce::Time::getCurrentTime();
    entry.direction = "--";
    entry.targetIndex = -1;
    entry.address = "ERROR";
    entry.arguments = abbreviate (message);
    entry.protocol = Protocol::MCP;
    entry.origin = OriginTag::MCP;
    entry.isRejected = true;
    entry.rejectReason = message;
    logger.logEntry (entry);
}

} // namespace WFSNetwork
