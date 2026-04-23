#pragma once

#include <atomic>
#include <mutex>
#include <juce_core/juce_core.h>
#include <juce_osc/juce_osc.h>

namespace wfs::plugin
{
    class OscTransport
    {
    public:
        OscTransport();
        ~OscTransport();

        bool connect (const juce::String& host, int udpPort);
        void disconnect();
        bool isConnected() const { return connected.load(); }

        bool sendInt    (const juce::String& oscPath, int channelId, int value);
        bool sendFloat  (const juce::String& oscPath, int channelId, float value);
        bool sendFloat  (const juce::String& oscPath, int channelId, float value, float rampSeconds);

        /** Three-float payload with no leading channel-ID arg (for ADM-OSC, where
            the channel is embedded in the address path). */
        bool sendFloats3 (const juce::String& oscPath, float v1, float v2, float v3);

    private:
        std::mutex lock;
        juce::OSCSender sender;
        std::atomic<bool> connected { false };
    };
}
