#include "OSCTCPReceiver.h"
#include "OSCParser.h"

namespace WFSNetwork
{

//==============================================================================
// OSCTCPReceiver
//==============================================================================

OSCTCPReceiver::OSCTCPReceiver()
    : Thread("OSCTCPReceiver")
{
}

OSCTCPReceiver::~OSCTCPReceiver()
{
    disconnect();
}

bool OSCTCPReceiver::connect(int port)
{
    if (connected.load())
        disconnect();

    serverSocket = std::make_unique<juce::StreamingSocket>();

    if (!serverSocket->createListener(port))
    {
        DBG("OSCTCPReceiver: Failed to create listener on port " << port);
        serverSocket.reset();
        return false;
    }

    portNumber = port;
    shouldStop = false;
    connected = true;
    startThread();

    DBG("OSCTCPReceiver: Listening on TCP port " << port);
    return true;
}

bool OSCTCPReceiver::disconnect()
{
    if (!connected.load())
        return true;

    shouldStop = true;

    // Stop accepting new connections
    if (serverSocket)
        serverSocket->close();

    // Stop and remove all client handlers
    {
        const juce::ScopedLock sl(clientsLock);
        for (auto& client : clients)
        {
            if (client)
                client->stop();
        }
        clients.clear();
    }

    stopThread(2000);

    serverSocket.reset();
    connected = false;
    portNumber = 0;

    DBG("OSCTCPReceiver: Disconnected");
    return true;
}

int OSCTCPReceiver::getClientCount() const
{
    const juce::ScopedLock sl(clientsLock);
    int count = 0;
    for (const auto& client : clients)
    {
        if (client && client->isActive())
            ++count;
    }
    return count;
}

void OSCTCPReceiver::addListener(Listener* listener)
{
    listeners.add(listener);
}

void OSCTCPReceiver::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

void OSCTCPReceiver::run()
{
    while (!threadShouldExit() && !shouldStop.load())
    {
        // Clean up inactive clients periodically
        removeInactiveClients();

        // Wait for incoming connections with timeout
        if (serverSocket->waitUntilReady(true, 100) == 1)
        {
            auto clientSocket = std::make_unique<juce::StreamingSocket>();

            if (serverSocket->waitUntilReady(true, 0) == 1)
            {
                // Get client IP from the connected socket
                juce::String clientIP;

                // Accept the connection
                auto* rawSocket = serverSocket->waitForNextConnection();
                if (rawSocket != nullptr)
                {
                    clientSocket.reset(rawSocket);
                    clientIP = clientSocket->getHostName();

                    // Check client limit
                    {
                        const juce::ScopedLock sl(clientsLock);
                        if (static_cast<int>(clients.size()) >= maxClients)
                        {
                            DBG("OSCTCPReceiver: Max clients reached, rejecting " << clientIP);
                            clientSocket->close();
                            continue;
                        }
                    }

                    DBG("OSCTCPReceiver: Client connected from " << clientIP);

                    // Create handler for this client
                    auto handler = std::make_unique<ClientHandler>(*this, std::move(clientSocket), clientIP);
                    handler->startThread();

                    const juce::ScopedLock sl(clientsLock);
                    clients.push_back(std::move(handler));
                }
            }
        }
    }
}

void OSCTCPReceiver::removeInactiveClients()
{
    const juce::ScopedLock sl(clientsLock);

    clients.erase(
        std::remove_if(clients.begin(), clients.end(),
            [](const std::unique_ptr<ClientHandler>& client) {
                return client == nullptr || !client->isActive();
            }),
        clients.end());
}

void OSCTCPReceiver::notifyMessage(const juce::OSCMessage& message,
                                   const juce::String& senderIP)
{
    // Must be called from the message thread
    listeners.call([&](Listener& l) { l.oscMessageReceived(message, senderIP); });
}

void OSCTCPReceiver::notifyBundle(const juce::OSCBundle& bundle,
                                  const juce::String& senderIP)
{
    // Must be called from the message thread
    listeners.call([&](Listener& l) { l.oscBundleReceived(bundle, senderIP); });
}

//==============================================================================
// ClientHandler
//==============================================================================

OSCTCPReceiver::ClientHandler::ClientHandler(OSCTCPReceiver& ownerRef,
                                             std::unique_ptr<juce::StreamingSocket> clientSocket,
                                             const juce::String& ip)
    : Thread("OSCTCPClient_" + ip)
    , owner(ownerRef)
    , socket(std::move(clientSocket))
    , clientIP(ip)
{
}

OSCTCPReceiver::ClientHandler::~ClientHandler()
{
    stop();
}

void OSCTCPReceiver::ClientHandler::stop()
{
    shouldStop = true;
    if (socket)
        socket->close();
    stopThread(2000);
    active = false;
}

void OSCTCPReceiver::ClientHandler::run()
{
    juce::HeapBlock<char> buffer(maxPacketSize);

    while (!threadShouldExit() && !shouldStop.load() && socket && socket->isConnected())
    {
        // Wait for data with timeout
        int ready = socket->waitUntilReady(true, 100);

        if (ready < 0)
        {
            // Error or disconnection
            DBG("OSCTCPReceiver: Client " << clientIP << " disconnected (error)");
            break;
        }

        if (ready == 0)
        {
            // Timeout - continue waiting
            continue;
        }

        // Read the 4-byte length prefix (big-endian)
        uint8_t lengthBytes[4];
        if (!readExactly(lengthBytes, 4))
        {
            DBG("OSCTCPReceiver: Client " << clientIP << " disconnected (read length failed)");
            break;
        }

        // Parse length as big-endian uint32
        uint32_t packetSize = (static_cast<uint32_t>(lengthBytes[0]) << 24) |
                              (static_cast<uint32_t>(lengthBytes[1]) << 16) |
                              (static_cast<uint32_t>(lengthBytes[2]) << 8) |
                              static_cast<uint32_t>(lengthBytes[3]);

        if (packetSize == 0 || packetSize > static_cast<uint32_t>(maxPacketSize))
        {
            DBG("OSCTCPReceiver: Invalid packet size " << packetSize << " from " << clientIP);
            break;
        }

        // Read the OSC packet data
        if (!readExactly(buffer.getData(), static_cast<int>(packetSize)))
        {
            DBG("OSCTCPReceiver: Client " << clientIP << " disconnected (read data failed)");
            break;
        }

        // Create a copy of the data for async processing
        juce::MemoryBlock data(buffer.getData(), packetSize);

        // Post to message thread for parsing
        juce::String ip = clientIP;
        juce::MessageManager::callAsync([this, data, ip]()
        {
            parseOSCData(data);
        });
    }

    active = false;
    DBG("OSCTCPReceiver: Client handler for " << clientIP << " stopped");
}

bool OSCTCPReceiver::ClientHandler::readExactly(void* buffer, int numBytes)
{
    char* dest = static_cast<char*>(buffer);
    int remaining = numBytes;

    while (remaining > 0 && !shouldStop.load())
    {
        int bytesRead = socket->read(dest, remaining, false);

        if (bytesRead < 0)
        {
            // Error
            return false;
        }

        if (bytesRead == 0)
        {
            // Check if socket is still ready
            if (socket->waitUntilReady(true, 100) <= 0)
                return false;
            continue;
        }

        dest += bytesRead;
        remaining -= bytesRead;
    }

    return remaining == 0;
}

void OSCTCPReceiver::ClientHandler::parseOSCData(const juce::MemoryBlock& data)
{
    try
    {
        const char* dataPtr = static_cast<const char*>(data.getData());
        int dataSize = static_cast<int>(data.getSize());
        int pos = 0;

        // Check if this is a bundle (starts with "#bundle")
        if (dataSize >= 8 && std::memcmp(dataPtr, "#bundle", 7) == 0)
        {
            auto bundle = OSCParser::parseBundle(dataPtr, dataSize, pos);
            owner.notifyBundle(bundle, clientIP);
        }
        else
        {
            // Try to parse as a single message
            auto message = OSCParser::parseMessage(dataPtr, dataSize, pos);
            owner.notifyMessage(message, clientIP);
        }
    }
    catch (const juce::OSCFormatError& e)
    {
        DBG("OSCTCPReceiver: Parse error from " << clientIP << ": " << e.description);
    }
}

} // namespace WFSNetwork
