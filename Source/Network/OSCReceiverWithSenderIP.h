#pragma once

#include <JuceHeader.h>

namespace WFSNetwork
{

/**
 * OSCReceiverWithSenderIP
 *
 * Custom OSC receiver that captures sender IP address.
 * JUCE's built-in OSCReceiver doesn't expose sender information,
 * so we use DatagramSocket directly to get sender IP for filtering.
 */
class OSCReceiverWithSenderIP : private juce::Thread
{
public:
    /**
     * Listener interface with sender IP information.
     */
    struct Listener
    {
        virtual ~Listener() = default;

        /** Called when an OSC message is received. */
        virtual void oscMessageReceived(const juce::OSCMessage& message,
                                        const juce::String& senderIP) = 0;

        /** Called when an OSC bundle is received. */
        virtual void oscBundleReceived(const juce::OSCBundle& bundle,
                                       const juce::String& senderIP) = 0;
    };

    OSCReceiverWithSenderIP();
    ~OSCReceiverWithSenderIP() override;

    /**
     * Start listening on the specified UDP port.
     * @param portNumber The port to listen on.
     * @return true if successfully bound to the port.
     */
    bool connect(int portNumber);

    /**
     * Stop listening and close the socket.
     * @return true if successfully stopped.
     */
    bool disconnect();

    /**
     * Check if the receiver is currently listening.
     */
    bool isConnected() const { return connected.load(); }

    /**
     * Get the port number currently being listened on.
     */
    int getPortNumber() const { return portNumber; }

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
    void parseOSCData(const juce::MemoryBlock& data, const juce::String& senderIP);
    void notifyMessage(const juce::OSCMessage& message, const juce::String& senderIP);
    void notifyBundle(const juce::OSCBundle& bundle, const juce::String& senderIP);

    std::unique_ptr<juce::DatagramSocket> socket;
    juce::ListenerList<Listener> listeners;
    std::atomic<bool> connected { false };
    std::atomic<bool> shouldStop { false };
    int portNumber = 0;

    static constexpr int bufferSize = 65536;  // Max UDP packet size

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSCReceiverWithSenderIP)
};

} // namespace WFSNetwork
