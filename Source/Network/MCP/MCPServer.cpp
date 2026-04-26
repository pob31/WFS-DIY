#include "MCPServer.h"
#include "../OSCLogger.h"
#include "../../Parameters/WFSValueTreeState.h"

namespace WFSNetwork
{

MCPServer::MCPServer (WFSValueTreeState& state, OSCLogger& networkLogger)
    : valueTreeState (state)
{
    mcpLogger     = std::make_unique<MCPLogger> (networkLogger);
    registry      = std::make_unique<MCPToolRegistry>();
    changeRecords = std::make_unique<MCPChangeRecordBuffer>();
    dispatcher    = std::make_unique<MCPDispatcher> (state, *registry, *changeRecords, *mcpLogger);
    transport     = std::make_unique<MCPTransport> (*mcpLogger);

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
