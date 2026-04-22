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

    private:
        std::mutex lock;
        juce::OSCSender sender;
        std::atomic<bool> connected { false };
    };
}
