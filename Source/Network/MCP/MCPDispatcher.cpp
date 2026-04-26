#include "MCPDispatcher.h"

namespace WFSNetwork
{

namespace
{
    // JSON-RPC 2.0 standard error codes.
    constexpr int kParseError     = -32700;
    constexpr int kInvalidRequest = -32600;
    constexpr int kMethodNotFound = -32601;
    constexpr int kInternalError  = -32603;

    juce::String varToJson (const juce::var& v)
    {
        return juce::JSON::toString (v, true);
    }
}

MCPDispatcher::MCPDispatcher (WFSValueTreeState& s,
                              MCPToolRegistry& r,
                              MCPLogger& l)
    : state (s), registry (r), mcpLogger (l)
{
}

juce::String MCPDispatcher::handleRequest (const juce::String& body,
                                           const juce::String& clientIP,
                                           int clientPort)
{
    // Parse the body as JSON. Anything that isn't a JSON object → -32700.
    juce::var parsed = juce::JSON::fromString (body);
    if (! parsed.isObject())
    {
        mcpLogger.logError ("Parse error: not a JSON object — " + body.substring (0, 80));
        return makeJsonRpcError ({}, kParseError, "Parse error: body is not a JSON object");
    }

    auto* obj = parsed.getDynamicObject();
    juce::var id = obj->getProperty ("id");
    juce::String method = obj->getProperty ("method").toString();
    juce::var params = obj->getProperty ("params");

    if (method.isEmpty())
    {
        mcpLogger.logError ("Invalid request: missing 'method' field");
        return makeJsonRpcError (id, kInvalidRequest, "Missing 'method' field");
    }

    mcpLogger.logRequest (method, varToJson (params), clientIP, clientPort);

    // Block 3 placeholder: every method returns "not implemented".
    // Block 4 replaces this body with real initialize / tools/list / tools/call dispatch.
    juce::String response = makeJsonRpcError (
        id,
        kMethodNotFound,
        "MCP server is in skeleton mode (Phase 1 Block 3) — method '" + method + "' not yet implemented",
        juce::var (juce::String ("pending_block_4")));

    mcpLogger.logResponse (method, response, clientIP, clientPort);
    return response;
}

juce::String MCPDispatcher::makeJsonRpcError (const juce::var& id,
                                              int code,
                                              const juce::String& message,
                                              const juce::var& data) const
{
    auto envelope = std::make_unique<juce::DynamicObject>();
    envelope->setProperty ("jsonrpc", "2.0");
    envelope->setProperty ("id", id);

    auto error = std::make_unique<juce::DynamicObject>();
    error->setProperty ("code", code);
    error->setProperty ("message", message);
    if (! data.isVoid())
        error->setProperty ("data", data);

    envelope->setProperty ("error", juce::var (error.release()));

    return juce::JSON::toString (juce::var (envelope.release()), true);
}

} // namespace WFSNetwork
