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

        /** Fired when the server reports that a container's contents changed
            (OSCQuery PATH_CHANGED) — a channel added, removed, dragged to a new
            display position, or retyped mono<->stereo. The path is the container,
            e.g. "/wfs/input"; refetch it to see what it now holds. */
        using StructureCallback = std::function<void (const juce::String& /*oscPath*/)>;

        OscQueryClient();
        ~OscQueryClient() override;

        void setOscCallback (OscCallback cb);
        void setStructureCallback (StructureCallback cb);

        bool connect (const juce::String& host, int httpPort);
        void disconnect();

        bool listen (const juce::String& oscPath);
        bool ignore (const juce::String& oscPath);

        /** HTTP-GET `<oscPath>?VALUE` and if the server returns a numeric
            current value, fire the regular oscCallback with it. Useful to
            populate fresh state immediately after subscribe. */
        bool fetchCurrentValue (const juce::String& oscPath);

        /** HTTP-GET a whole container and return its parsed OSCQuery JSON —
            FULL_PATH/CONTENTS/VALUE and the rest. One request for a subtree, as
            against fetchCurrentValue's one request per leaf. */
        bool fetchNamespace (const juce::String& oscPath, juce::var& outJson);

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
        OscCallback       oscCallback;
        StructureCallback structureCallback;

        std::mutex lock;
        juce::String currentHost;
        int          currentHttpPort { 0 };
        juce::String cachedHostInfo;
        std::vector<juce::String> subscribedPaths;

        std::unique_ptr<SimpleWebSocketClient> ws;
    };
}
