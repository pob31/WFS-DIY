#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSValueTreeState.h"

namespace WFSNetwork
{

/**
 * OSCQueryServer
 *
 * HTTP server implementing the OSC Query protocol for parameter discovery.
 * Exposes WFS-DIY parameters as a browsable JSON namespace.
 *
 * Ref: https://github.com/Vidvox/OSCQueryProposal
 *
 * Supported:
 * - GET / returns the full namespace tree
 * - GET /wfs/input/0/positionX returns a specific node
 * - GET /path?HOST_INFO returns server metadata
 * - GET /path?VALUE|TYPE|RANGE|ACCESS|DESCRIPTION|CLIPMODE returns a single attribute
 * - HTTP 200/204/400/404 status codes per spec
 *
 * Not yet supported: WebSocket, LISTEN/IGNORE subscriptions.
 */
class OSCQueryServer : public juce::Thread,
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
    // Thread
    void run() override;

    // HTTP handling
    struct ParsedRequest { juce::String path; juce::String query; };
    ParsedRequest parseHttpRequest(const juce::String& request);
    void handleHttpRequest(juce::StreamingSocket& client);
    void sendHttpResponse(juce::StreamingSocket& client, int statusCode,
                          const juce::String& statusText, const juce::String& body);

    // JSON builders
    juce::String buildHostInfoJson();
    juce::DynamicObject* buildFullTree();
    juce::DynamicObject* buildInputChannelJson(int channelIndex);
    juce::DynamicObject* buildOutputChannelJson(int channelIndex);
    juce::DynamicObject* buildReverbChannelJson(int channelIndex);
    juce::DynamicObject* buildConfigJson();

    // Helpers
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

    // Attribute queries (?VALUE, ?RANGE, etc.)
    static juce::String extractAttribute(juce::DynamicObject* node, const juce::String& attr);

    // EQ params that take a band index as first argument
    static bool isEQParam(const juce::String& oscName);

    // ValueTree::Listener
    void valueTreePropertyChanged(juce::ValueTree& tree,
                                  const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&) override {}
    void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged(juce::ValueTree&) override {}

    // State
    WFSValueTreeState& state;
    std::unique_ptr<juce::StreamingSocket> serverSocket;
    std::atomic<bool> running { false };
    int oscPort = 0;
    int httpPort = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSCQueryServer)
};

} // namespace WFSNetwork
