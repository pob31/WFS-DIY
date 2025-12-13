#include "OSCReceiverWithSenderIP.h"
#include "OSCParser.h"

namespace WFSNetwork
{

//==============================================================================
// OSCReceiverWithSenderIP
//==============================================================================

OSCReceiverWithSenderIP::OSCReceiverWithSenderIP()
    : Thread("OSCReceiverWithSenderIP")
{
}

OSCReceiverWithSenderIP::~OSCReceiverWithSenderIP()
{
    disconnect();
}

bool OSCReceiverWithSenderIP::connect(int port)
{
    if (connected.load())
        disconnect();

    socket = std::make_unique<juce::DatagramSocket>();

    if (!socket->bindToPort(port))
    {
        DBG("OSCReceiverWithSenderIP: Failed to bind to port " << port);
        socket.reset();
        return false;
    }

    portNumber = port;
    shouldStop = false;
    connected = true;
    startThread();

    DBG("OSCReceiverWithSenderIP: Listening on UDP port " << port);
    return true;
}

bool OSCReceiverWithSenderIP::disconnect()
{
    if (!connected.load())
        return true;

    shouldStop = true;

    if (socket)
        socket->shutdown();

    stopThread(2000);

    socket.reset();
    connected = false;
    portNumber = 0;

    DBG("OSCReceiverWithSenderIP: Disconnected");
    return true;
}

void OSCReceiverWithSenderIP::addListener(Listener* listener)
{
    listeners.add(listener);
}

void OSCReceiverWithSenderIP::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

void OSCReceiverWithSenderIP::run()
{
    juce::HeapBlock<char> buffer(bufferSize);

    while (!threadShouldExit() && !shouldStop.load())
    {
        // Wait for data with timeout (100ms) to allow thread exit checking
        int ready = socket->waitUntilReady(true, 100);

        if (ready < 0)
        {
            // Error occurred
            DBG("OSCReceiverWithSenderIP: Socket error");
            break;
        }

        if (ready == 0)
        {
            // Timeout - continue waiting
            continue;
        }

        // Data is available
        juce::String senderIP;
        int senderPort;

        int bytesRead = socket->read(buffer.getData(),
                                     bufferSize,
                                     false,
                                     senderIP,
                                     senderPort);

        if (bytesRead > 0)
        {
            // Create a copy of the data for async processing
            juce::MemoryBlock data(buffer.getData(), static_cast<size_t>(bytesRead));

            // Post to message thread for parsing and notification
            juce::MessageManager::callAsync([this, data, senderIP]()
            {
                parseOSCData(data, senderIP);
            });
        }
    }
}

void OSCReceiverWithSenderIP::parseOSCData(const juce::MemoryBlock& data,
                                           const juce::String& senderIP)
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
            notifyBundle(bundle, senderIP);
        }
        else
        {
            // Try to parse as a single message
            auto message = OSCParser::parseMessage(dataPtr, dataSize, pos);
            notifyMessage(message, senderIP);
        }
    }
    catch (const juce::OSCFormatError& e)
    {
        DBG("OSCReceiverWithSenderIP: Parse error from " << senderIP << ": " << e.description);
    }
}

void OSCReceiverWithSenderIP::notifyMessage(const juce::OSCMessage& message,
                                            const juce::String& senderIP)
{
    listeners.call([&](Listener& l) { l.oscMessageReceived(message, senderIP); });
}

void OSCReceiverWithSenderIP::notifyBundle(const juce::OSCBundle& bundle,
                                           const juce::String& senderIP)
{
    listeners.call([&](Listener& l) { l.oscBundleReceived(bundle, senderIP); });
}

} // namespace WFSNetwork
