#pragma once

#include <JuceHeader.h>
#include "OSCReceiverWithSenderIP.h"

namespace WFSNetwork
{

/**
 * OSCTCPReceiver
 *
 * TCP server for receiving OSC messages with sender IP information.
 * Uses length-prefix framing (4-byte big-endian size before each OSC packet).
 * Accepts multiple simultaneous client connections.
 */
class OSCTCPReceiver : private juce::Thread
{
public:
    /**
     * Use the same Listener interface as the UDP receiver for consistency.
     */
    using Listener = OSCReceiverWithSenderIP::Listener;

    OSCTCPReceiver();
    ~OSCTCPReceiver() override;

    /**
     * Start the TCP server on the specified port.
     * @param portNumber The port to listen on.
     * @return true if successfully bound to the port.
     */
    bool connect(int portNumber);

    /**
     * Stop the server and close all connections.
     * @return true if successfully stopped.
     */
    bool disconnect();

    /**
     * Check if the server is currently listening.
     */
    bool isConnected() const { return connected.load(); }

    /**
     * Get the port number currently being listened on.
     */
    int getPortNumber() const { return portNumber; }

    /**
     * Get the number of currently connected clients.
     */
    int getClientCount() const;

    /**
     * Add a listener to receive OSC messages.
     */
    void addListener(Listener* listener);

    /**
     * Remove a previously added listener.
     */
    void removeListener(Listener* listener);

private:
    void run() override;

    /**
     * Handle a connected client in its own thread.
     */
    class ClientHandler : public juce::Thread
    {
    public:
        ClientHandler(OSCTCPReceiver& owner,
                      std::unique_ptr<juce::StreamingSocket> clientSocket,
                      const juce::String& clientIP);
        ~ClientHandler() override;

        void run() override;
        void stop();
        bool isActive() const { return active.load(); }
        juce::String getClientIP() const { return clientIP; }

    private:
        bool readExactly(void* buffer, int numBytes);
        void parseOSCData(const juce::MemoryBlock& data);

        OSCTCPReceiver& owner;
        std::unique_ptr<juce::StreamingSocket> socket;
        juce::String clientIP;
        std::atomic<bool> active { true };
        std::atomic<bool> shouldStop { false };
    };

    void removeInactiveClients();
    void notifyMessage(const juce::OSCMessage& message, const juce::String& senderIP);
    void notifyBundle(const juce::OSCBundle& bundle, const juce::String& senderIP);

    std::unique_ptr<juce::StreamingSocket> serverSocket;
    std::vector<std::unique_ptr<ClientHandler>> clients;
    juce::CriticalSection clientsLock;

    juce::ListenerList<Listener> listeners;
    std::atomic<bool> connected { false };
    std::atomic<bool> shouldStop { false };
    int portNumber = 0;

    static constexpr int maxPacketSize = 65536;
    static constexpr int maxClients = 16;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSCTCPReceiver)
};

} // namespace WFSNetwork
