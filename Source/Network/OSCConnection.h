#pragma once

#include <JuceHeader.h>
#include "OSCProtocolTypes.h"

namespace WFSNetwork
{

/**
 * OSCConnection - Manages a single OSC connection to a network target.
 *
 * Wraps juce::OSCSender for outgoing messages and tracks connection state.
 * Each instance represents one of the 6 configurable network targets.
 */
class OSCConnection
{
public:
    //==========================================================================
    // Construction/Destruction
    //==========================================================================

    explicit OSCConnection(int targetIndex);
    ~OSCConnection();

    //==========================================================================
    // Configuration
    //==========================================================================

    /** Configure the connection with target settings */
    void configure(const TargetConfig& config);

    /** Get the current configuration */
    const TargetConfig& getConfig() const { return config; }

    /** Get the target index (0-5) */
    int getTargetIndex() const { return targetIndex; }

    //==========================================================================
    // Connection Control
    //==========================================================================

    /** Connect to the target (for TCP) or prepare sender (for UDP) */
    bool connect();

    /** Disconnect from the target */
    void disconnect();

    /** Check if connected/ready to send */
    bool isConnected() const;

    /** Get current connection status */
    ConnectionStatus getStatus() const { return status; }

    /** Get status as string for display */
    juce::String getStatusString() const;

    //==========================================================================
    // Message Sending
    //==========================================================================

    /** Send a single OSC message. Returns true if sent successfully. */
    bool send(const juce::OSCMessage& message);

    /** Send an OSC bundle. Returns true if sent successfully. */
    bool send(const juce::OSCBundle& bundle);

    //==========================================================================
    // Properties
    //==========================================================================

    juce::String getTargetIP() const { return config.ipAddress; }
    int getTargetPort() const { return config.port; }
    Protocol getProtocol() const { return config.protocol; }
    ConnectionMode getConnectionMode() const { return config.mode; }
    bool isRxEnabled() const { return config.rxEnabled; }
    bool isTxEnabled() const { return config.txEnabled; }
    bool isActive() const { return config.isActive(); }

    //==========================================================================
    // Statistics
    //==========================================================================

    /** Get number of messages sent since connection */
    int64_t getMessagesSent() const { return messagesSent; }

    /** Get number of send errors since connection */
    int64_t getSendErrors() const { return sendErrors; }

    /** Reset statistics */
    void resetStats();

private:
    //==========================================================================
    // Private Members
    //==========================================================================

    int targetIndex;
    TargetConfig config;
    std::unique_ptr<juce::OSCSender> sender;          // For UDP
    std::unique_ptr<juce::StreamingSocket> tcpSocket; // For TCP
    ConnectionStatus status = ConnectionStatus::Disconnected;
    juce::CriticalSection sendLock;

    // Statistics
    std::atomic<int64_t> messagesSent { 0 };
    std::atomic<int64_t> sendErrors { 0 };

    //==========================================================================
    // Private Methods
    //==========================================================================

    void setStatus(ConnectionStatus newStatus);
    bool createSender();
    void destroySender();

    // TCP-specific methods
    bool connectTCP();
    void disconnectTCP();
    bool sendWithLengthPrefix(const juce::MemoryBlock& oscData);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSCConnection)
};

} // namespace WFSNetwork
