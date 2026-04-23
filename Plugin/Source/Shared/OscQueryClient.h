#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>
#include <juce_core/juce_core.h>
#include <juce_simpleweb/juce_simpleweb.h>

namespace wfs::plugin
{
    class OscQueryClient  : private SimpleWebSocketClientBase::Listener
    {
    public:
        enum class State { Idle, Connecting, Handshaking, Ready, Error };

        using OscCallback = std::function<void (const juce::String& /*oscPath*/,
                                                float /*value*/)>;

        OscQueryClient();
        ~OscQueryClient() override;

        void setOscCallback (OscCallback cb);

        bool connect (const juce::String& host, int httpPort);
        void disconnect();

        bool listen (const juce::String& oscPath);
        bool ignore (const juce::String& oscPath);

        /** HTTP-GET `<oscPath>?VALUE` and if the server returns a numeric
            current value, fire the regular oscCallback with it. Useful to
            populate fresh state immediately after subscribe. */
        bool fetchCurrentValue (const juce::String& oscPath);

        State         getState() const        { return state.load(); }
        juce::String  getLastHostInfo() const;

    private:
        void connectionOpened() override;
        void messageReceived (const juce::String&) override;
        void dataReceived    (const juce::MemoryBlock&) override;
        void connectionClosed (int status, const juce::String& reason) override;
        void connectionError  (const juce::String& message) override;

        bool httpGet (const juce::String& pathAndQuery, juce::String& outBody);
        void sendCommand (const juce::String& command, const juce::String& path);
        bool decodeOscPacket (const juce::MemoryBlock& data,
                              juce::String& outPath,
                              float& outValue);

        void setState (State s);

        std::atomic<State> state { State::Idle };
        OscCallback   oscCallback;

        std::mutex lock;
        juce::String currentHost;
        int          currentHttpPort { 0 };
        juce::String cachedHostInfo;
        std::vector<juce::String> subscribedPaths;

        std::unique_ptr<SimpleWebSocketClient> ws;
    };
}
