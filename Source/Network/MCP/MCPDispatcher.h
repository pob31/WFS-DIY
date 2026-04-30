#pragma once

#include <JuceHeader.h>
#include <atomic>
#include "MCPLogger.h"
#include "MCPToolRegistry.h"
#include "MCPChangeRecords.h"
#include "MCPUndoEngine.h"
#include "MCPResourceRegistry.h"
#include "MCPPromptRegistry.h"
#include "MCPTierEnforcement.h"

class WFSValueTreeState;

namespace WFSNetwork
{

/** JSON-RPC method dispatcher.

    Implements the MCP-over-JSON-RPC handshake and routes tools/list +
    tools/call to MCPToolRegistry. State-modifying tool calls are
    marshalled to the JUCE message thread (inside an OriginTagScope set
    to MCP) so all the existing thread-safety guarantees of the parameter
    system continue to hold. The dispatcher then captures before/after
    state and emits a ChangeRecord into the ring buffer for Phase 5 undo.

    Threading: handleRequest is called on the SimpleWeb HTTP worker
    thread. It blocks until the message-thread invocation of the tool
    handler completes (or a timeout fires). Tool handlers always see
    OriginTag::MCP and can read/write the ValueTree directly. */
class MCPDispatcher
{
public:
    MCPDispatcher (WFSValueTreeState& state,
                   MCPToolRegistry& registry,
                   MCPChangeRecordBuffer& ringBuffer,
                   MCPUndoEngine& undoEngine,
                   MCPResourceRegistry& resourceRegistry,
                   MCPPromptRegistry& promptRegistry,
                   MCPTierEnforcement& tierEnforcement,
                   MCPLogger& mcpLogger);

    /** Receive a raw HTTP body, return a JSON-RPC envelope as a string.
        Always returns a syntactically valid JSON-RPC 2.0 envelope, even
        for malformed input — the transport layer should never have to
        synthesize a response itself. */
    juce::String handleRequest (const juce::String& body,
                                const juce::String& clientIP,
                                int clientPort);

    /** Block until the message-thread tool execution completes. Tunable
        if a workflow legitimately needs longer (snapshot-load can be
        seconds on big projects). Default 5s. */
    void setToolExecutionTimeoutMs (int ms) noexcept { toolTimeoutMs = juce::jmax (250, ms); }

private:
    // ---- JSON-RPC method handlers ---------------------------------------
    juce::String handleInitialize    (const juce::var& id, const juce::var& params);
    juce::String handleInitialized   ();    // notification — returns "" (no body)
    juce::String handleToolsList     (const juce::var& id);
    juce::String handleToolsCall     (const juce::var& id, const juce::var& params);
    juce::String handleResourcesList (const juce::var& id);
    juce::String handleResourcesRead (const juce::var& id, const juce::var& params);
    juce::String handlePromptsList   (const juce::var& id);
    juce::String handlePromptsGet    (const juce::var& id, const juce::var& params);

    // ---- Helpers --------------------------------------------------------
    juce::String makeJsonRpcResult (const juce::var& id, const juce::var& result) const;
    juce::String makeJsonRpcError  (const juce::var& id,
                                    int code,
                                    const juce::String& message,
                                    const juce::var& data = {}) const;

    /** Serialize a juce::var to a compact JSON string (no pretty print). */
    static juce::String toCompactJson (const juce::var& v);

    /** Run the handler on the JUCE message thread, populate `result` and
        (optionally) `record`. Returns false on timeout. */
    bool runOnMessageThread (const ToolDescriptor& tool,
                             const juce::var& args,
                             ChangeRecord* record,
                             ToolResult& outResult);

    WFSValueTreeState& state;
    MCPToolRegistry& registry;
    MCPChangeRecordBuffer& ringBuffer;
    MCPUndoEngine& undoEngine;
    MCPResourceRegistry& resources;
    MCPPromptRegistry& prompts;
    MCPTierEnforcement& tierEnforcement;
    MCPLogger& mcpLogger;

    std::atomic<bool> initialized { false };  // flips true after `initialize` succeeds
    int toolTimeoutMs = 5000;

    // MCP spec revision we negotiate with. Static text — bumped when the
    // wire-level changes adopted by the project shift to a newer revision.
    static constexpr const char* kProtocolVersion = "2024-11-05";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MCPDispatcher)
};

} // namespace WFSNetwork
