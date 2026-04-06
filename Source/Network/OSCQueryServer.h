#pragma once

#include <JuceHeader.h>
#include <juce_simpleweb/juce_simpleweb.h>
#include "../Parameters/WFSValueTreeState.h"

namespace WFSNetwork
{

/**
 * OSCQueryServer
 *
 * HTTP + WebSocket server implementing the OSC Query protocol.
 * Uses juce_simpleweb (Ben Kuper) for HTTP and WebSocket on the same port.
 *
 * Ref: https://github.com/Vidvox/OSCQueryProposal
 *
 * HTTP (query):
 * - GET / returns the full namespace tree
 * - GET /wfs/input/0/positionX returns a specific node
 * - GET /path?HOST_INFO returns server metadata
 * - GET /path?VALUE|TYPE|RANGE|ACCESS|DESCRIPTION|CLIPMODE returns a single attribute
 *
 * WebSocket (subscription):
 * - LISTEN: client subscribes to value changes on a path
 * - IGNORE: client unsubscribes
 * - Server pushes binary OSC packets for subscribed parameters
 * - Server sends PATH_CHANGED/PATH_ADDED/PATH_REMOVED notifications
 */
class OSCQueryServer : public SimpleWebSocketServerBase::RequestHandler,
                       public SimpleWebSocketServerBase::Listener,
                       private juce::ValueTree::Listener
{
public:
    OSCQueryServer(WFSValueTreeState& state);
    ~OSCQueryServer() override;

    bool start(int oscPort, int httpPort);
    void stop();

    bool isRunning() const { return running.load(); }
    int getHttpPort() const { return httpPort; }
    int getOscPort() const { return oscPort; }

private:
    // --- HTTP Request Handler (SimpleWebSocketServerBase::RequestHandler) ---
    bool handleHTTPRequest(std::shared_ptr<HttpServer::Response> response,
                           std::shared_ptr<HttpServer::Request> request) override;

    // --- WebSocket Listener (SimpleWebSocketServerBase::Listener) ---
    void connectionOpened(const juce::String& id) override;
    void messageReceived(const juce::String& id, const juce::String& message) override;
    void connectionClosed(const juce::String& id, int status, const juce::String& reason) override;
    void connectionError(const juce::String& id, const juce::String& message) override;

    // --- HTTP Response Helpers ---
    void sendJsonResponse(std::shared_ptr<HttpServer::Response> response,
                          int statusCode, const juce::String& body);

    // --- JSON Builders (reused from previous implementation) ---
    juce::String buildHostInfoJson();
    juce::DynamicObject* buildFullTree();
    juce::DynamicObject* buildInputChannelJson(int channelIndex);
    juce::DynamicObject* buildOutputChannelJson(int channelIndex);
    juce::DynamicObject* buildReverbChannelJson(int channelIndex);
    juce::DynamicObject* buildConfigJson();

    static juce::DynamicObject* makeParamNode(const juce::String& fullPath,
                                               const juce::String& type,
                                               const juce::var& value,
                                               float rangeMin, float rangeMax,
                                               const juce::String& description);
    static juce::DynamicObject* makeContainerNode(const juce::String& fullPath,
                                                    const juce::String& description);
    static juce::String getOSCTypeTag(const juce::var& value);

    struct ParamRange { float min; float max; bool hasRange; };
    static ParamRange getParamRange(const juce::Identifier& paramId);
    static juce::String extractAttribute(juce::DynamicObject* node, const juce::String& attr);
    static bool isEQParam(const juce::String& oscName);

    // --- LISTEN/IGNORE Subscription Tracking ---
    void handleListenCommand(const juce::String& connectionId, const juce::String& path);
    void handleIgnoreCommand(const juce::String& connectionId, const juce::String& path);
    void removeAllSubscriptions(const juce::String& connectionId);

    // subscriptions: OSC path -> set of connection IDs
    std::map<juce::String, juce::StringArray> subscriptions;
    juce::CriticalSection subscriptionLock;

    // --- Value Change Push ---
    void pushValueChange(const juce::String& oscPath, const juce::var& value);

    // Reverse lookup: paramId -> OSC address name (built lazily)
    struct ReverseEntry { juce::String oscName; juce::String category; /* "input","output","reverb" */ };
    static const std::map<juce::Identifier, ReverseEntry>& getReverseMap();

    // Resolve a ValueTree property change to an OSC path
    juce::String resolveOSCPath(const juce::ValueTree& tree, const juce::Identifier& property) const;

    // --- ValueTree::Listener ---
    void valueTreePropertyChanged(juce::ValueTree& tree,
                                  const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;
    void valueTreeChildOrderChanged(juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged(juce::ValueTree&) override {}

    // --- State ---
    WFSValueTreeState& state;
    juce::ValueTree stateTree;  // stored to keep listener alive
    std::unique_ptr<SimpleWebSocketServer> wsServer;
    std::atomic<bool> running { false };
    int oscPort = 0;
    int httpPort = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSCQueryServer)
};

} // namespace WFSNetwork
