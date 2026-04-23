#include "OscTransport.h"

namespace wfs::plugin
{
    OscTransport::OscTransport() = default;

    OscTransport::~OscTransport()
    {
        disconnect();
    }

    bool OscTransport::connect (const juce::String& host, int udpPort)
    {
        std::lock_guard<std::mutex> sl (lock);
        if (connected.load())
            sender.disconnect();

        const bool ok = sender.connect (host, udpPort);
        connected.store (ok);
        return ok;
    }

    void OscTransport::disconnect()
    {
        std::lock_guard<std::mutex> sl (lock);
        if (connected.load())
        {
            sender.disconnect();
            connected.store (false);
        }
    }

    bool OscTransport::sendInt (const juce::String& oscPath, int channelId, int value)
    {
        std::lock_guard<std::mutex> sl (lock);
        if (! connected.load())
            return false;
        return sender.send (juce::OSCAddressPattern (oscPath), channelId, value);
    }

    bool OscTransport::sendFloat (const juce::String& oscPath, int channelId, float value)
    {
        std::lock_guard<std::mutex> sl (lock);
        if (! connected.load())
            return false;
        return sender.send (juce::OSCAddressPattern (oscPath), channelId, value);
    }

    bool OscTransport::sendFloat (const juce::String& oscPath, int channelId, float value, float rampSeconds)
    {
        std::lock_guard<std::mutex> sl (lock);
        if (! connected.load())
            return false;
        return sender.send (juce::OSCAddressPattern (oscPath), channelId, value, rampSeconds);
    }

    bool OscTransport::sendFloats3 (const juce::String& oscPath, float v1, float v2, float v3)
    {
        std::lock_guard<std::mutex> sl (lock);
        if (! connected.load())
            return false;
        return sender.send (juce::OSCAddressPattern (oscPath), v1, v2, v3);
    }
}
