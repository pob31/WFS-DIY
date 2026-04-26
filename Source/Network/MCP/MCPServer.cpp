#include "MCPServer.h"
#include "../OSCLogger.h"
#include "../../Parameters/WFSValueTreeState.h"
#include "../../Parameters/WFSFileManager.h"
#include "MCPGeneratedToolLoader.h"
#include "tools/SessionTools.h"
#include "tools/InputTools.h"
#include "tools/SnapshotTools.h"
#include "tools/UndoTools.h"
#include "tools/SetParameterTool.h"

namespace WFSNetwork
{

MCPServer::MCPServer (WFSValueTreeState& state,
                      WFSFileManager& fileMgr,
                      OSCLogger& networkLogger,
                      const juce::File& generatedToolsJson,
                      const juce::File& knowledgeResourcesDir)
    : valueTreeState (state),
      fileManager (fileMgr)
{
    mcpLogger        = std::make_unique<MCPLogger> (networkLogger);
    registry         = std::make_unique<MCPToolRegistry>();
    changeRecords    = std::make_unique<MCPChangeRecordBuffer>();
    resourceRegistry = std::make_unique<MCPResourceRegistry> (knowledgeResourcesDir);
    dispatcher       = std::make_unique<MCPDispatcher> (state, *registry, *changeRecords,
                                                        *resourceRegistry, *mcpLogger);
    transport        = std::make_unique<MCPTransport> (*mcpLogger);

    mcpLogger->logInfo ("Loaded " + juce::String (resourceRegistry->size())
                        + " knowledge resources from " + knowledgeResourcesDir.getFullPathName());

    // Phase 2 — register the auto-generated tool surface FIRST. The
    // hand-written tools registered below silently overwrite by name,
    // so when a Phase-1 hand-written tool collides with a generated one
    // (input.set_attenuation today), the hand-written version wins
    // (preserves its richer operator description).
    Tools::Generated::loadGeneratedTools (*registry, state, generatedToolsJson, *mcpLogger);

    // Phase 1 hand-written tools — registered after the generated set so
    // that any name collision keeps the hand-written variant.
    registry->registerTool (Tools::Session::describe (state));
    registry->registerTool (Tools::Input::describeSetName (state));
    registry->registerTool (Tools::Input::describeSetCartesian (state));
    registry->registerTool (Tools::Input::describeSetAttenuation (state));
    registry->registerTool (Tools::Snapshot::describe (fileMgr));

    // Phase 2 Block 3 — generic escape-hatch tool. Lets the AI hit a
    // parameter the auto-generated surface didn't cover. Trusted-caller
    // semantics: no clamping, exact variable names required.
    registry->registerTool (Tools::SetParameter::describe (state));

    // Undo / redo tools — stubs until Phase 5; mcp.get_ai_change_history is
    // a real read-only implementation over the ring buffer.
    registry->registerTool (Tools::Undo::describeUndo());
    registry->registerTool (Tools::Undo::describeRedo());
    registry->registerTool (Tools::Undo::describeGetHistory (*changeRecords));

    // Wire the transport's POST /mcp callback to the dispatcher.
    transport->setRequestHandler ([disp = dispatcher.get()] (const juce::String& body,
                                                             const juce::String& clientIP,
                                                             int clientPort)
    {
        return disp->handleRequest (body, clientIP, clientPort);
    });
}

MCPServer::~MCPServer()
{
    stop();
}

bool MCPServer::start (int port, bool loopbackOnly)
{
    if (isRunning())
        return true;

    return transport->start (port, loopbackOnly);
}

void MCPServer::stop()
{
    if (transport != nullptr)
        transport->stop();
}

} // namespace WFSNetwork
