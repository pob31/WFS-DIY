#include "MCPServer.h"
#include "../OSCLogger.h"
#include "../../Parameters/WFSValueTreeState.h"
#include "../../Parameters/WFSFileManager.h"
#include "tools/SessionTools.h"
#include "tools/InputTools.h"
#include "tools/SnapshotTools.h"
#include "tools/UndoTools.h"

namespace WFSNetwork
{

MCPServer::MCPServer (WFSValueTreeState& state,
                      WFSFileManager& fileMgr,
                      OSCLogger& networkLogger)
    : valueTreeState (state),
      fileManager (fileMgr)
{
    mcpLogger     = std::make_unique<MCPLogger> (networkLogger);
    registry      = std::make_unique<MCPToolRegistry>();
    changeRecords = std::make_unique<MCPChangeRecordBuffer>();
    dispatcher    = std::make_unique<MCPDispatcher> (state, *registry, *changeRecords, *mcpLogger);
    transport     = std::make_unique<MCPTransport> (*mcpLogger);

    // Register the five hand-written Phase 1 tools. Phase 2 will register
    // the auto-generated tools on top of these.
    registry->registerTool (Tools::Session::describe (state));
    registry->registerTool (Tools::Input::describeSetName (state));
    registry->registerTool (Tools::Input::describeSetCartesian (state));
    registry->registerTool (Tools::Input::describeSetAttenuation (state));
    registry->registerTool (Tools::Snapshot::describe (fileMgr));

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
