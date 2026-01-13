#include "OSCConnection.h"
#include "OSCSerializer.h"

namespace WFSNetwork
{

//==============================================================================
// Construction/Destruction
//==============================================================================

OSCConnection::OSCConnection(int index)
    : targetIndex(index)
{
    jassert(index >= 0 && index < MAX_TARGETS);
}

OSCConnection::~OSCConnection()
{
    disconnect();
}

//==============================================================================
// Configuration
//==============================================================================

void OSCConnection::configure(const TargetConfig& newConfig)
{
    const juce::ScopedLock sl(sendLock);

    bool needsReconnect = (config.ipAddress != newConfig.ipAddress
                        || config.port != newConfig.port
                        || config.mode != newConfig.mode);

    config = newConfig;

    bool hasConnection = (sender != nullptr) || (tcpSocket != nullptr);

    if (needsReconnect && hasConnection)
    {
        // Reconnect with new settings
        disconnect();
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
        disconnect();
    }
}

//==============================================================================
// Connection Control
//==============================================================================

bool OSCConnection::connect()
{
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
    // For TCP, we need to actually establish a connection
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
    }
    else // TCP
    {
        if (connectTCP())
        {
            setStatus(ConnectionStatus::Connected);
            resetStats();
            DBG("OSCConnection[" << targetIndex << "]: Connected to "
                << config.ipAddress << ":" << config.port << " (TCP)");
            return true;
        }
    }

    setStatus(ConnectionStatus::Error);
    DBG("OSCConnection[" << targetIndex << "]: Failed to connect to "
        << config.ipAddress << ":" << config.port);
    return false;
}

void OSCConnection::disconnect()
{
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
    {
        DBG("OSCConnection[" << targetIndex << "]: Cannot send - connected=" << (isConnected() ? "yes" : "no")
            << " txEnabled=" << (config.txEnabled ? "yes" : "no")
            << " status=" << getStatusString());
        return false;
    }

    DBG("OSCConnection[" << targetIndex << "]: Sending " << message.getAddressPattern().toString()
        << " to " << config.ipAddress << ":" << config.port);

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
        DBG("OSCConnection[" << targetIndex << "]: Send SUCCESS");
        return true;
    }
    else
    {
        ++sendErrors;
        DBG("OSCConnection[" << targetIndex << "]: Send FAILED for " << message.getAddressPattern().toString());

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

bool OSCConnection::connectTCP()
{
    tcpSocket = std::make_unique<juce::StreamingSocket>();

    if (!tcpSocket->connect(config.ipAddress, config.port, 5000)) // 5 second timeout
    {
        DBG("OSCConnection[" << targetIndex
            << "]: TCP connection failed to " << config.ipAddress << ":" << config.port);
        tcpSocket.reset();
        return false;
    }

    return true;
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
