#include "MCPDispatcher.h"
#include "../../Parameters/WFSValueTreeState.h"

namespace WFSNetwork
{

namespace
{
    // JSON-RPC 2.0 standard error codes.
    constexpr int kParseError     = -32700;
    constexpr int kInvalidRequest = -32600;
    constexpr int kMethodNotFound = -32601;
    constexpr int kInvalidParams  = -32602;
    constexpr int kInternalError  = -32603;

    juce::DynamicObject* asObject (const juce::var& v)
    {
        return v.isObject() ? v.getDynamicObject() : nullptr;
    }
}

//==============================================================================
MCPDispatcher::MCPDispatcher (WFSValueTreeState& s,
                              MCPToolRegistry& r,
                              MCPChangeRecordBuffer& buf,
                              MCPUndoEngine& undo,
                              MCPResourceRegistry& res,
                              MCPPromptRegistry& prm,
                              MCPLogger& l)
    : state (s), registry (r), ringBuffer (buf), undoEngine (undo),
      resources (res), prompts (prm), mcpLogger (l)
{
}

juce::String MCPDispatcher::handleRequest (const juce::String& body,
                                           const juce::String& clientIP,
                                           int clientPort)
{
    // Parse the body as JSON. Anything that isn't an object → -32700.
    juce::var parsed = juce::JSON::fromString (body);
    auto* obj = asObject (parsed);
    if (obj == nullptr)
    {
        mcpLogger.logError ("Parse error: not a JSON object - " + body.substring (0, 80));
        return makeJsonRpcError ({}, kParseError, "Parse error: body is not a JSON object");
    }

    juce::var id = obj->getProperty ("id");
    juce::String method = obj->getProperty ("method").toString();
    juce::var params = obj->getProperty ("params");

    if (method.isEmpty())
    {
        mcpLogger.logError ("Invalid request: missing 'method' field");
        return makeJsonRpcError (id, kInvalidRequest, "Missing 'method' field");
    }

    mcpLogger.logRequest (method, toCompactJson (params), clientIP, clientPort);

    juce::String response;

    if      (method == "initialize")                response = handleInitialize (id, params);
    else if (method == "notifications/initialized") response = handleInitialized();
    else if (method == "tools/list")                response = handleToolsList (id);
    else if (method == "tools/call")                response = handleToolsCall (id, params);
    else if (method == "resources/list")            response = handleResourcesList (id);
    else if (method == "resources/read")            response = handleResourcesRead (id, params);
    else if (method == "prompts/list")              response = handlePromptsList (id);
    else if (method == "prompts/get")               response = handlePromptsGet (id, params);
    else
    {
        response = makeJsonRpcError (id, kMethodNotFound,
                                     "Method not found: " + method);
    }

    if (response.isNotEmpty())
        mcpLogger.logResponse (method, response, clientIP, clientPort);

    return response;
}

//==============================================================================
// initialize / initialized
//==============================================================================

juce::String MCPDispatcher::handleInitialize (const juce::var& id, const juce::var& /*params*/)
{
    // The client's protocolVersion in params is informational for Phase 1;
    // we always advertise our supported revision and let MCP's negotiation
    // logic on the client side accept or reject.
    auto serverInfo = std::make_unique<juce::DynamicObject>();
    serverInfo->setProperty ("name",    "WFS-DIY");
    serverInfo->setProperty ("version", juce::String (ProjectInfo::versionString));

    auto toolsCap = std::make_unique<juce::DynamicObject>();
    toolsCap->setProperty ("listChanged", false);

    auto resourcesCap = std::make_unique<juce::DynamicObject>();
    resourcesCap->setProperty ("subscribe",   false);  // No live subscription in v1
    resourcesCap->setProperty ("listChanged", false);  // Static catalog bundled with app

    auto promptsCap = std::make_unique<juce::DynamicObject>();
    promptsCap->setProperty ("listChanged", false);  // Static catalog bundled with app

    auto capabilities = std::make_unique<juce::DynamicObject>();
    capabilities->setProperty ("tools",     juce::var (toolsCap.release()));
    capabilities->setProperty ("resources", juce::var (resourcesCap.release()));
    capabilities->setProperty ("prompts",   juce::var (promptsCap.release()));

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("protocolVersion", juce::String (kProtocolVersion));
    result->setProperty ("capabilities",    juce::var (capabilities.release()));
    result->setProperty ("serverInfo",      juce::var (serverInfo.release()));

    initialized = true;
    return makeJsonRpcResult (id, juce::var (result.release()));
}

juce::String MCPDispatcher::handleInitialized()
{
    // Notification — no response body, JSON-RPC says return nothing. The
    // transport already sent 200 OK; an empty string here means we just
    // close out. (Streamable HTTP allows an empty body on notifications.)
    return juce::String();
}

//==============================================================================
// tools/list
//==============================================================================

juce::String MCPDispatcher::handleToolsList (const juce::var& id)
{
    juce::Array<juce::var> tools;
    for (const auto& descriptor : registry.all())
    {
        auto entry = std::make_unique<juce::DynamicObject>();
        entry->setProperty ("name",        descriptor.name);
        entry->setProperty ("description", descriptor.description);
        entry->setProperty ("inputSchema", descriptor.inputSchema);
        tools.add (juce::var (entry.release()));
    }

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("tools", juce::var (tools));

    return makeJsonRpcResult (id, juce::var (result.release()));
}

//==============================================================================
// tools/call
//==============================================================================

juce::String MCPDispatcher::handleToolsCall (const juce::var& id, const juce::var& params)
{
    auto* paramsObj = asObject (params);
    if (paramsObj == nullptr)
        return makeJsonRpcError (id, kInvalidParams, "tools/call requires params object");

    juce::String name = paramsObj->getProperty ("name").toString();
    if (name.isEmpty())
        return makeJsonRpcError (id, kInvalidParams, "tools/call requires 'name'");

    juce::var args = paramsObj->getProperty ("arguments");
    if (! args.isObject())
    {
        // Default to empty object if caller omitted arguments.
        args = juce::var (new juce::DynamicObject());
    }

    const ToolDescriptor* tool = registry.find (name);
    if (tool == nullptr)
        return makeJsonRpcError (id, kMethodNotFound, "Tool not found: " + name);

    // Set up change record envelope for state-modifying tools. Read-only
    // tools pass nullptr to the handler.
    ChangeRecord record;
    record.timestamp = juce::Time::getCurrentTime();
    record.toolName  = name;
    record.arguments = args;

    ChangeRecord* recordPtr = tool->modifiesState ? &record : nullptr;

    ToolResult result;
    if (! runOnMessageThread (*tool, args, recordPtr, result))
    {
        return makeJsonRpcError (id, kInternalError,
                                 "Tool execution timed out: " + name);
    }

    if (recordPtr != nullptr && result.success)
    {
        ringBuffer.push (record);
        // Standard undo/redo: a fresh state-modifying tool call invalidates
        // any pending redo history. Phase 5a wires this up; the undo engine
        // owns the redo ring.
        undoEngine.onNewStateModifyingRecord();
    }

    if (! result.success)
    {
        // Map handler-level error to a tools/call result with isError=true.
        auto contentItem = std::make_unique<juce::DynamicObject>();
        contentItem->setProperty ("type", "text");
        contentItem->setProperty ("text",
            "Tool error [" + result.errorCode + "]: " + result.errorMessage);

        juce::Array<juce::var> content;
        content.add (juce::var (contentItem.release()));

        auto callResult = std::make_unique<juce::DynamicObject>();
        callResult->setProperty ("content", juce::var (content));
        callResult->setProperty ("isError", true);

        // Phase 5b Block 3: drain pending cross-actor notifications onto
        // the response envelope. Errors get them too — the AI may need to
        // see the override even when its own tool failed.
        const auto notifs = undoEngine.drainPendingNotifications();
        if (! notifs.isEmpty())
            callResult->setProperty ("notifications", juce::var (notifs));

        return makeJsonRpcResult (id, juce::var (callResult.release()));
    }

    // Wrap the tool's value in MCP's content envelope. Compact JSON keeps
    // the AI-facing surface easy to scan.
    auto contentItem = std::make_unique<juce::DynamicObject>();
    contentItem->setProperty ("type", "text");
    contentItem->setProperty ("text", toCompactJson (result.value));

    juce::Array<juce::var> content;
    content.add (juce::var (contentItem.release()));

    auto callResult = std::make_unique<juce::DynamicObject>();
    callResult->setProperty ("content", juce::var (content));
    callResult->setProperty ("isError", false);

    // Phase 5b Block 3: cross-actor notifications side-channel.
    const auto notifs = undoEngine.drainPendingNotifications();
    if (! notifs.isEmpty())
        callResult->setProperty ("notifications", juce::var (notifs));

    return makeJsonRpcResult (id, juce::var (callResult.release()));
}

//==============================================================================
// resources/list
//==============================================================================

juce::String MCPDispatcher::handleResourcesList (const juce::var& id)
{
    juce::Array<juce::var> arr;
    for (const auto& entry : resources.all())
    {
        auto item = std::make_unique<juce::DynamicObject>();
        item->setProperty ("uri",         entry.uri);
        item->setProperty ("name",        entry.name);
        item->setProperty ("description", entry.description);
        item->setProperty ("mimeType",    entry.mimeType);
        arr.add (juce::var (item.release()));
    }

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("resources", juce::var (arr));
    return makeJsonRpcResult (id, juce::var (result.release()));
}

//==============================================================================
// resources/read
//==============================================================================

juce::String MCPDispatcher::handleResourcesRead (const juce::var& id, const juce::var& params)
{
    auto* paramsObj = asObject (params);
    if (paramsObj == nullptr)
        return makeJsonRpcError (id, kInvalidParams, "resources/read requires params object");

    const juce::String uri = paramsObj->getProperty ("uri").toString();
    if (uri.isEmpty())
        return makeJsonRpcError (id, kInvalidParams, "resources/read requires 'uri'");

    const ResourceEntry* entry = resources.findByURI (uri);
    if (entry == nullptr)
        return makeJsonRpcError (id, kInvalidParams, "Unknown resource URI: " + uri);

    if (! entry->file.existsAsFile())
        return makeJsonRpcError (id, kInternalError,
                                 "Resource file missing on disk: " + entry->file.getFullPathName(),
                                 juce::var (entry->uri));

    const juce::String body = entry->file.loadFileAsString();

    auto contentItem = std::make_unique<juce::DynamicObject>();
    contentItem->setProperty ("uri",      entry->uri);
    contentItem->setProperty ("mimeType", entry->mimeType);
    contentItem->setProperty ("text",     body);

    juce::Array<juce::var> contents;
    contents.add (juce::var (contentItem.release()));

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("contents", juce::var (contents));
    return makeJsonRpcResult (id, juce::var (result.release()));
}

//==============================================================================
// prompts/list
//==============================================================================

juce::String MCPDispatcher::handlePromptsList (const juce::var& id)
{
    juce::Array<juce::var> arr;
    for (const auto& entry : prompts.all())
    {
        juce::Array<juce::var> argsArr;
        for (const auto& a : entry.arguments)
        {
            auto argObj = std::make_unique<juce::DynamicObject>();
            argObj->setProperty ("name",        a.name);
            argObj->setProperty ("description", a.description);
            argObj->setProperty ("required",    a.required);
            argsArr.add (juce::var (argObj.release()));
        }

        auto item = std::make_unique<juce::DynamicObject>();
        item->setProperty ("name",        entry.name);
        item->setProperty ("description", entry.description);
        item->setProperty ("arguments",   juce::var (argsArr));
        arr.add (juce::var (item.release()));
    }

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("prompts", juce::var (arr));
    return makeJsonRpcResult (id, juce::var (result.release()));
}

//==============================================================================
// prompts/get
//==============================================================================

juce::String MCPDispatcher::handlePromptsGet (const juce::var& id, const juce::var& params)
{
    auto* paramsObj = asObject (params);
    if (paramsObj == nullptr)
        return makeJsonRpcError (id, kInvalidParams, "prompts/get requires params object");

    const juce::String name = paramsObj->getProperty ("name").toString();
    if (name.isEmpty())
        return makeJsonRpcError (id, kInvalidParams, "prompts/get requires 'name'");

    const PromptEntry* entry = prompts.findByName (name);
    if (entry == nullptr)
        return makeJsonRpcError (id, kInvalidParams, "Unknown prompt: " + name);

    // Validate required arguments. The MCP spec leaves enforcement to the
    // server; we surface specific arg names so the AI client can ask the
    // operator for the missing piece rather than retry blindly.
    const juce::var argsVar = paramsObj->getProperty ("arguments");
    auto* argsObj = asObject (argsVar);

    juce::StringArray providedArgPairs;  // for the preamble
    for (const auto& a : entry->arguments)
    {
        const bool present = (argsObj != nullptr) && argsObj->hasProperty (a.name);
        if (a.required && ! present)
        {
            return makeJsonRpcError (id, kInvalidParams,
                                     "prompts/get for '" + name + "' missing required arg: " + a.name,
                                     juce::var (a.name));
        }
        if (present)
        {
            const juce::String value = argsObj->getProperty (a.name).toString();
            providedArgPairs.add (a.name + "=\"" + value + "\"");
        }
    }

    // Build a small preamble showing what arguments the client passed in,
    // followed by the static template body. None of the v1 templates use
    // {{placeholder}} interpolation; the AI handles the args contextually
    // based on the descriptions in the template.
    juce::String text;
    if (! providedArgPairs.isEmpty())
        text << "[Invoked with: " << providedArgPairs.joinIntoString (", ") << "]\n\n";
    text << entry->templateBody;

    auto contentItem = std::make_unique<juce::DynamicObject>();
    contentItem->setProperty ("type", "text");
    contentItem->setProperty ("text", text);

    auto message = std::make_unique<juce::DynamicObject>();
    message->setProperty ("role",    "user");
    message->setProperty ("content", juce::var (contentItem.release()));

    juce::Array<juce::var> messages;
    messages.add (juce::var (message.release()));

    auto result = std::make_unique<juce::DynamicObject>();
    result->setProperty ("description", entry->description);
    result->setProperty ("messages", juce::var (messages));
    return makeJsonRpcResult (id, juce::var (result.release()));
}

//==============================================================================
// Message-thread hop
//==============================================================================

bool MCPDispatcher::runOnMessageThread (const ToolDescriptor& tool,
                                        const juce::var& args,
                                        ChangeRecord* record,
                                        ToolResult& outResult)
{
    // SimpleWeb worker thread → JUCE message thread, then block on the
    // result. WaitableEvent + reference captures are safe because this
    // function does not return until either the lambda runs or wait()
    // times out. The lambda captures by reference; it never outlives us.
    juce::WaitableEvent done;

    juce::MessageManager::callAsync ([&tool, &args, record, &outResult, &done]()
    {
        OriginTagScope originScope { OriginTag::MCP };
        try
        {
            outResult = tool.handler (args, record);
        }
        catch (const std::exception& e)
        {
            outResult = ToolResult::error ("internal_error",
                                           juce::String ("Handler threw: ") + e.what());
        }
        catch (...)
        {
            outResult = ToolResult::error ("internal_error", "Handler threw unknown exception");
        }
        done.signal();
    });

    return done.wait (toolTimeoutMs);
}

//==============================================================================
// Envelope builders
//==============================================================================

juce::String MCPDispatcher::makeJsonRpcResult (const juce::var& id, const juce::var& result) const
{
    auto envelope = std::make_unique<juce::DynamicObject>();
    envelope->setProperty ("jsonrpc", "2.0");
    envelope->setProperty ("id", id);
    envelope->setProperty ("result", result);
    return juce::JSON::toString (juce::var (envelope.release()), true);
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

juce::String MCPDispatcher::toCompactJson (const juce::var& v)
{
    return juce::JSON::toString (v, /*allOnOneLine*/ true);
}

} // namespace WFSNetwork
