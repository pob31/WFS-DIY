#include "OSCConnection.h"
#include "OSCSerializer.h"

namespace WFSNetwork
{

//==============================================================================
// Construction/Destruction
//==============================================================================

OSCConnection::OSCConnection(int index)
    : juce::Thread("OSCConnection_" + juce::String(index)),
      targetIndex(index)
{
    jassert(index >= 0 && index < MAX_TARGETS);
}

OSCConnection::~OSCConnection()
{
    // Stop any pending connection thread
    stopThread(2500);
    disconnect();
}

//==============================================================================
// Configuration
//==============================================================================

void OSCConnection::configure(const TargetConfig& newConfig)
{
    // Stop background thread before acquiring lock to avoid deadlock
    // (run() also acquires sendLock)
    connectionPending = false;
    stopThread(3000);

    const juce::ScopedLock sl(sendLock);

    bool needsReconnect = (config.ipAddress != newConfig.ipAddress
                        || config.port != newConfig.port
                        || config.mode != newConfig.mode);

    config = newConfig;

    bool hasConnection = (sender != nullptr) || (tcpSocket != nullptr);

    if (needsReconnect && hasConnection)
    {
        // Reconnect with new settings — thread already stopped above
        destroySender();
        disconnectTCP();
        setStatus(ConnectionStatus::Disconnected);

        if (config.isActive() && config.txEnabled)
            connect();
    }
    else if (config.isActive() && config.txEnabled && !hasConnection)
    {
        // Start connection if now active
        connect();
    }
    else if (!config.isActive() || !config.txEnabled)
    {
        // Disconnect if no longer active
        destroySender();
        disconnectTCP();
        setStatus(ConnectionStatus::Disconnected);
    }
}

//==============================================================================
// Connection Control
//==============================================================================

bool OSCConnection::connect()
{
    // Stop any existing background thread before acquiring lock to avoid
    // deadlock (run() also acquires sendLock for TCP connect)
    connectionPending = false;
    stopThread(3000);

    const juce::ScopedLock sl(sendLock);

    if (!config.isValid())
    {
        setStatus(ConnectionStatus::Error);
        return false;
    }

    setStatus(ConnectionStatus::Connecting);

    if (!createSender())
    {
        setStatus(ConnectionStatus::Error);
        return false;
    }

    // For UDP, we're "connected" as soon as the sender is created
    // For TCP, we need to actually establish a connection (non-blocking)
    if (config.mode == ConnectionMode::UDP)
    {
        if (sender->connect(config.ipAddress, config.port))
        {
            setStatus(ConnectionStatus::Connected);
            resetStats();
            DBG("OSCConnection[" << targetIndex << "]: Connected to "
                << config.ipAddress << ":" << config.port << " (UDP)");
            return true;
        }

        setStatus(ConnectionStatus::Error);
        DBG("OSCConnection[" << targetIndex << "]: Failed to connect to "
            << config.ipAddress << ":" << config.port << " (UDP)");
        return false;
    }
    else // TCP - non-blocking connection in background thread
    {
        // Start async connection (thread already stopped above)
        connectionPending = true;
        startThread();

        // Return true to indicate connection attempt started
        // Final result will come via onStatusChanged callback
        return true;
    }
}

void OSCConnection::disconnect()
{
    // Stop any pending connection attempt
    connectionPending = false;
    stopThread(3000);

    const juce::ScopedLock sl(sendLock);

    destroySender();
    disconnectTCP();
    setStatus(ConnectionStatus::Disconnected);
}

bool OSCConnection::isConnected() const
{
    const juce::ScopedLock sl(sendLock);

    if (config.mode == ConnectionMode::TCP)
        return status == ConnectionStatus::Connected &&
               tcpSocket != nullptr &&
               tcpSocket->isConnected();
    else
        return status == ConnectionStatus::Connected && sender != nullptr;
}

juce::String OSCConnection::getStatusString() const
{
    switch (status)
    {
        case ConnectionStatus::Disconnected: return "Disconnected";
        case ConnectionStatus::Connecting:   return "Connecting...";
        case ConnectionStatus::Connected:    return "Connected";
        case ConnectionStatus::Error:        return "Error";
        default:                             return "Unknown";
    }
}

//==============================================================================
// Message Sending
//==============================================================================

bool OSCConnection::send(const juce::OSCMessage& message)
{
    const juce::ScopedLock sl(sendLock);

    if (!isConnected() || !config.txEnabled)
        return false;

    bool success = false;

    if (config.mode == ConnectionMode::TCP)
    {
        auto data = OSCSerializer::serializeMessage(message);
        success = sendWithLengthPrefix(data);
    }
    else
    {
        success = sender->send(message);
    }

    if (success)
    {
        ++messagesSent;
        return true;
    }
    else
    {
        ++sendErrors;

        // Mark as disconnected on TCP failure to allow reconnection
        if (config.mode == ConnectionMode::TCP)
            setStatus(ConnectionStatus::Disconnected);

        return false;
    }
}

bool OSCConnection::send(const juce::OSCBundle& bundle)
{
    const juce::ScopedLock sl(sendLock);

    if (!isConnected() || !config.txEnabled)
        return false;

    bool success = false;

    if (config.mode == ConnectionMode::TCP)
    {
        auto data = OSCSerializer::serializeBundle(bundle);
        success = sendWithLengthPrefix(data);
    }
    else
    {
        success = sender->send(bundle);
    }

    if (success)
    {
        messagesSent += bundle.size();
        return true;
    }
    else
    {
        ++sendErrors;
        DBG("OSCConnection[" << targetIndex << "]: Send error for bundle");

        // Mark as disconnected on TCP failure to allow reconnection
        if (config.mode == ConnectionMode::TCP)
            setStatus(ConnectionStatus::Disconnected);

        return false;
    }
}

//==============================================================================
// Statistics
//==============================================================================

void OSCConnection::resetStats()
{
    messagesSent = 0;
    sendErrors = 0;
}

//==============================================================================
// Private Methods
//==============================================================================

void OSCConnection::setStatus(ConnectionStatus newStatus)
{
    if (status != newStatus)
    {
        status = newStatus;
        // Could notify listeners here if needed
    }
}

bool OSCConnection::createSender()
{
    if (sender == nullptr)
    {
        sender = std::make_unique<juce::OSCSender>();
    }
    return sender != nullptr;
}

void OSCConnection::destroySender()
{
    if (sender != nullptr)
    {
        sender->disconnect();
        sender.reset();
    }
}

//==============================================================================
// TCP-specific Methods
//==============================================================================

bool OSCConnection::connectTCPSync()
{
    tcpSocket = std::make_unique<juce::StreamingSocket>();

    if (!tcpSocket->connect(config.ipAddress, config.port, 2000)) // 2 second timeout
    {
        DBG("OSCConnection[" << targetIndex
            << "]: TCP connection failed to " << config.ipAddress << ":" << config.port);
        tcpSocket.reset();
        return false;
    }

    return true;
}

void OSCConnection::run()
{
    // Background thread for TCP connection
    if (!connectionPending)
        return;

    // Copy config values we need (to avoid holding lock during connect)
    juce::String targetIP;
    int targetPort;
    {
        const juce::ScopedLock sl(sendLock);
        targetIP = config.ipAddress;
        targetPort = config.port;
    }

    DBG("OSCConnection[" << targetIndex << "]: Async TCP connecting to "
        << targetIP << ":" << targetPort);

    // Attempt TCP connection (blocking, but in background thread)
    bool success = false;
    {
        const juce::ScopedLock sl(sendLock);
        if (connectionPending && !threadShouldExit())
            success = connectTCPSync();
    }

    // Check if we were cancelled
    if (!connectionPending || threadShouldExit())
        return;

    connectionPending = false;

    // Update status and notify via message thread
    juce::MessageManager::callAsync([this, success, targetIP, targetPort]()
    {
        if (success)
        {
            setStatus(ConnectionStatus::Connected);
            resetStats();
            DBG("OSCConnection[" << targetIndex << "]: Connected to "
                << targetIP << ":" << targetPort << " (TCP)");
        }
        else
        {
            setStatus(ConnectionStatus::Error);
            DBG("OSCConnection[" << targetIndex << "]: Failed to connect to "
                << targetIP << ":" << targetPort << " (TCP)");
        }

        if (onStatusChanged)
            onStatusChanged(status);
    });
}

void OSCConnection::disconnectTCP()
{
    if (tcpSocket != nullptr)
    {
        tcpSocket->close();
        tcpSocket.reset();
    }
}

bool OSCConnection::sendWithLengthPrefix(const juce::MemoryBlock& oscData)
{
    if (tcpSocket == nullptr || !tcpSocket->isConnected())
        return false;

    // Create buffer with 4-byte length prefix + OSC data
    juce::MemoryOutputStream out;

    // Write length as 4-byte big-endian (matches OSCTCPReceiver expectation)
    uint32_t length = static_cast<uint32_t>(oscData.getSize());
    out.writeByte(static_cast<char>((length >> 24) & 0xFF));
    out.writeByte(static_cast<char>((length >> 16) & 0xFF));
    out.writeByte(static_cast<char>((length >> 8) & 0xFF));
    out.writeByte(static_cast<char>(length & 0xFF));

    // Write OSC data
    out.write(oscData.getData(), oscData.getSize());

    // Send everything
    const auto& block = out.getMemoryBlock();
    int written = tcpSocket->write(block.getData(), static_cast<int>(block.getSize()));

    return written == static_cast<int>(block.getSize());
}

} // namespace WFSNetwork
