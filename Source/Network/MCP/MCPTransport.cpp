#include "MCPTransport.h"

namespace WFSNetwork
{

namespace
{
    constexpr const char* kEndpointPath = "/mcp";

    SimpleWeb::CaseInsensitiveMultimap defaultHeaders (bool loopbackOnly)
    {
        SimpleWeb::CaseInsensitiveMultimap h;
        h.emplace ("Content-Type", "application/json");
        // Loopback-bound: a wildcard `*` is acceptable because the socket
        // itself is bound to 127.0.0.1, so only same-machine origins can
        // reach this endpoint at all. LAN-bound: restrict to "null" to
        // refuse browser CORS preflight from arbitrary LAN pages — the
        // real auth story is still TODO (no token model yet), so the
        // tightest CORS posture is the only safety we have.
        h.emplace ("Access-Control-Allow-Origin", loopbackOnly ? "*" : "null");
        h.emplace ("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        h.emplace ("Access-Control-Allow-Headers", "Content-Type, Authorization");
        return h;
    }
}

MCPTransport::MCPTransport (MCPLogger& l) : mcpLogger (l) {}

MCPTransport::~MCPTransport()
{
    stop();
}

bool MCPTransport::start (int port, bool loopbackOnly)
{
    if (running.load())
        stop();

    server = std::make_unique<SimpleWebSocketServer>();
    server->addHTTPRequestHandler (this);
    server->addWebSocketListener (this);  // catches serverInitError / serverInitSuccess

    const juce::String localAddress = loopbackOnly ? juce::String ("127.0.0.1") : juce::String();

    initSucceeded.store (false);
    initFailed.store    (false);
    {
        const juce::ScopedLock sl (initErrorLock);
        initError.clear();
    }

    // SimpleWebSocketServerBase::start spawns the listener thread; bind
    // happens asynchronously inside that thread and signals back via the
    // Listener callbacks (serverInitSuccess / serverInitError). We poll
    // briefly so callers get a deterministic running/not-running state.
    server->start (port, /*wsSuffix*/ "", localAddress, /*allowAddressReuse*/ false);

    constexpr int kInitTimeoutMs = 1000;
    const auto deadline = juce::Time::getMillisecondCounter() + kInitTimeoutMs;
    while (! initSucceeded.load()
           && ! initFailed.load()
           && juce::Time::getMillisecondCounter() < deadline)
    {
        juce::Thread::sleep (10);
    }

    if (! initSucceeded.load())
    {
        juce::String reason;
        {
            const juce::ScopedLock sl (initErrorLock);
            reason = initError.isNotEmpty() ? initError : juce::String ("timed out waiting for bind");
        }
        mcpLogger.logError ("MCP server failed to start on port " + juce::String (port)
                            + ": " + reason);
        server->removeHTTPRequestHandler (this);
        server->removeWebSocketListener (this);
        server->stop();
        server.reset();
        return false;
    }

    boundPort = port;
    loopbackOnlyMode = loopbackOnly;
    running = true;

    mcpLogger.logInfo ("MCP server listening on "
                       + (loopbackOnly ? juce::String ("127.0.0.1:") : juce::String ("0.0.0.0:"))
                       + juce::String (port) + kEndpointPath);
    return true;
}

void MCPTransport::serverInitSuccess()
{
    // Called from the SimpleWeb IO thread. Just flip the atomic — the
    // main start() loop is polling it.
    initSucceeded.store (true);
}

void MCPTransport::serverInitError (const juce::String& message)
{
    // Called from the SimpleWeb IO thread. Stash the message under a
    // lock (juce::String is ref-counted and NOT thread-safe under
    // concurrent read/write), then flip the atomic so start() sees the
    // failure and tears the server down.
    {
        const juce::ScopedLock sl (initErrorLock);
        initError = message.isNotEmpty() ? message : juce::String ("unknown bind error");
    }
    initFailed.store (true);
}

void MCPTransport::stop()
{
    if (! running.load())
        return;

    running = false;

    if (server != nullptr)
    {
        server->removeHTTPRequestHandler (this);
        server->removeWebSocketListener (this);
        server->stop();
        server.reset();
    }

    boundPort = 0;
    initSucceeded.store (false);
    initFailed.store    (false);
    {
        const juce::ScopedLock sl (initErrorLock);
        initError.clear();
    }
    mcpLogger.logInfo ("MCP server stopped");
}

void MCPTransport::setRequestHandler (HandlerCallback cb)
{
    const juce::ScopedLock sl (handlerLock);
    handler = std::move (cb);
}

bool MCPTransport::handleHTTPRequest (std::shared_ptr<HttpServer::Response> response,
                                       std::shared_ptr<HttpServer::Request> request)
{
    juce::String path   = juce::String (request->path);
    juce::String method = juce::String (request->method);

    // Normalize trailing slash so "/mcp" and "/mcp/" both match.
    if (path.length() > 1 && path.endsWithChar ('/'))
        path = path.dropLastCharacters (1);

    if (path != kEndpointPath)
    {
        // Let other handlers (or the default 404) take care of unknown paths.
        return false;
    }

    if (method == "OPTIONS")
    {
        // CORS preflight — answer with empty body and the Allow* headers from
        // defaultHeaders(). SimpleWeb routes OPTIONS through default_resource
        // since benkuper/juce_simpleweb#5 merged.
        writeJson (response, SimpleWeb::StatusCode::success_no_content, juce::String());
        return true;
    }

    if (method == "GET")
    {
        // Streamable-HTTP server-push (SSE) lands in a later phase. For Phase 1
        // we expose request/response only, so GET is explicitly disallowed.
        writeMethodNotAllowed (response, "POST, OPTIONS");
        return true;
    }

    if (method != "POST")
    {
        writeMethodNotAllowed (response, "POST, OPTIONS");
        return true;
    }

    // POST /mcp — read body, hand to dispatcher, return its JSON-RPC envelope.
    juce::String body = juce::String (request->content.string());
    juce::String clientIP = resolveClientIP (request);
    int clientPort = resolveClientPort (request);

    HandlerCallback cb;
    {
        const juce::ScopedLock sl (handlerLock);
        cb = handler;
    }

    if (! cb)
    {
        // Server is up but the dispatcher hasn't been wired yet (Phase 1 Block 3
        // can hit this path during integration). Return a structured 503 so the
        // client knows to retry later rather than treating it as a hard failure.
        const juce::String err =
            R"({"jsonrpc":"2.0","id":null,"error":{"code":-32603,)"
            R"("message":"MCP dispatcher not initialized"}})";
        writeJson (response, SimpleWeb::StatusCode::server_error_service_unavailable, err);
        return true;
    }

    juce::String responseBody;
    try
    {
        responseBody = cb (body, clientIP, clientPort);
    }
    catch (const std::exception& e)
    {
        mcpLogger.logError (juce::String ("Dispatcher threw: ") + e.what());
        const juce::String err =
            R"({"jsonrpc":"2.0","id":null,"error":{"code":-32603,)"
            R"("message":"Internal server error"}})";
        writeJson (response, SimpleWeb::StatusCode::server_error_internal_server_error, err);
        return true;
    }

    writeJson (response, SimpleWeb::StatusCode::success_ok, responseBody);
    return true;
}

void MCPTransport::writeJson (std::shared_ptr<HttpServer::Response> response,
                              SimpleWeb::StatusCode statusCode,
                              const juce::String& body,
                              const SimpleWeb::CaseInsensitiveMultimap& extraHeaders) const
{
    auto headers = defaultHeaders (loopbackOnlyMode);
    for (const auto& kv : extraHeaders)
        headers.emplace (kv.first, kv.second);

    response->write (statusCode, body.toStdString(), headers);
}

void MCPTransport::writeMethodNotAllowed (std::shared_ptr<HttpServer::Response> response,
                                          const juce::String& allowedMethods) const
{
    SimpleWeb::CaseInsensitiveMultimap h;
    h.emplace ("Allow", allowedMethods.toStdString());
    const juce::String body = R"({"error":"method_not_allowed"})";
    writeJson (response, SimpleWeb::StatusCode::client_error_method_not_allowed, body, h);
}

juce::String MCPTransport::resolveClientIP (const std::shared_ptr<HttpServer::Request>& request)
{
    try
    {
        auto endpoint = request->remote_endpoint();
        return juce::String (endpoint.address().to_string());
    }
    catch (...)
    {
        return juce::String();
    }
}

int MCPTransport::resolveClientPort (const std::shared_ptr<HttpServer::Request>& request)
{
    try
    {
        return static_cast<int> (request->remote_endpoint().port());
    }
    catch (...)
    {
        return 0;
    }
}

} // namespace WFSNetwork
