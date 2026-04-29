#pragma once

#include <JuceHeader.h>
#include <juce_simpleweb/juce_simpleweb.h>
#include "MCPLogger.h"

namespace WFSNetwork
{

/** HTTP transport for the MCP server.

    Implements MCP's Streamable HTTP transport at a single endpoint
    (`/mcp` by default). For Phase 1 this is request/response only —
    POST /mcp returns the JSON-RPC envelope produced by MCPDispatcher.
    GET /mcp would carry the SSE notification stream for server-pushed
    messages; that lands in a later phase. Until then, GET returns 405.

    Threading: SimpleWeb's HTTP server runs its io_context on its own
    thread (managed by SimpleWebSocketServerBase, which is a juce::Thread).
    handleHTTPRequest is invoked on that thread; the registered callback
    runs synchronously there and is expected to be quick. State mutation
    must be marshalled to the message thread by the callback itself
    (MCPDispatcher does this in Block 4). */
class MCPTransport : public SimpleWebSocketServerBase::RequestHandler,
                     public SimpleWebSocketServerBase::Listener
{
public:
    using HandlerCallback = std::function<juce::String (const juce::String& body,
                                                         const juce::String& clientIP,
                                                         int clientPort)>;

    explicit MCPTransport (MCPLogger& mcpLogger);
    ~MCPTransport() override;

    /** Start listening on the given port. Blocks for up to 1s waiting for
        the asynchronous bind to succeed (or fail) and returns true only
        on success. On failure the server is torn down and isRunning()
        will report false. The port may be 0 (let OS assign), but the
        MCP UX assumes a known port for client config. */
    bool start (int port, bool loopbackOnly);

    /** Stop accepting new connections and tear down the io_context. */
    void stop();

    bool isRunning() const noexcept     { return running.load(); }
    int  getBoundPort() const noexcept  { return boundPort; }

    /** Set the callback invoked on POST /mcp. Replaces any previously set
        handler. The callback must return a JSON-RPC envelope as a string;
        the transport wraps it as application/json with status 200. */
    void setRequestHandler (HandlerCallback callback);

private:
    bool handleHTTPRequest (std::shared_ptr<HttpServer::Response> response,
                            std::shared_ptr<HttpServer::Request> request) override;

    // SimpleWebSocketServerBase::Listener — flips initialised on bind result.
    void serverInitSuccess() override;
    void serverInitError (const juce::String& message) override;

    void writeJson (std::shared_ptr<HttpServer::Response> response,
                    SimpleWeb::StatusCode statusCode,
                    const juce::String& body,
                    const SimpleWeb::CaseInsensitiveMultimap& extraHeaders = {}) const;

    void writeMethodNotAllowed (std::shared_ptr<HttpServer::Response> response,
                                const juce::String& allowedMethods) const;

    static juce::String resolveClientIP (const std::shared_ptr<HttpServer::Request>& request);
    static int          resolveClientPort (const std::shared_ptr<HttpServer::Request>& request);

    MCPLogger& mcpLogger;
    HandlerCallback handler;
    juce::CriticalSection handlerLock;

    std::unique_ptr<SimpleWebSocketServer> server;
    std::atomic<bool> running { false };
    std::atomic<bool> initialized { false };
    juce::String initError;
    int boundPort = 0;
    bool loopbackOnlyMode = true;  // mirrors the start() argument; drives CORS

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MCPTransport)
};

} // namespace WFSNetwork
