#pragma once

#include <JuceHeader.h>
#include "../Parameters/WFSValueTreeState.h"

namespace WFSNetwork
{

/**
 * OSCQueryServer
 *
 * Simple HTTP server implementing the OSC Query protocol for parameter discovery.
 * Exposes WFS-DIY parameters as a browsable JSON namespace.
 *
 * OSC Query Protocol:
 * - HTTP GET requests return JSON describing the parameter tree
 * - Path "/" returns the full namespace
 * - Path "/wfs/input/0/positionX" returns info about that specific parameter
 */
class OSCQueryServer : public juce::Thread,
                       private juce::ValueTree::Listener
{
public:
    OSCQueryServer(WFSValueTreeState& state);
    ~OSCQueryServer() override;

    /**
     * Start the OSC Query server.
     * @param oscPort UDP port for OSC messages (for advertising)
     * @param httpPort HTTP port for OSC Query discovery (e.g., 5005)
     * @return true if server started successfully
     */
    bool start(int oscPort, int httpPort);

    /**
     * Stop the OSC Query server.
     */
    void stop();

    /**
     * Check if the server is running.
     */
    bool isRunning() const { return running.load(); }

    /**
     * Get the HTTP port the server is listening on.
     */
    int getHttpPort() const { return httpPort; }

    /**
     * Get the OSC port for value messages.
     */
    int getOscPort() const { return oscPort; }

private:
    // Thread run method
    void run() override;

    // Handle a single HTTP request
    void handleHttpRequest(juce::StreamingSocket& client);

    // Parse HTTP request and extract path
    juce::String parseHttpPath(const juce::String& request);

    // Generate JSON response for a path
    juce::String generateJsonResponse(const juce::String& path);

    // Build JSON for root namespace
    juce::String buildRootJson();

    // Build JSON for a specific node
    juce::String buildNodeJson(const juce::String& path);

    // Helper to build input channel JSON
    juce::String buildInputChannelJson(int channelIndex);

    // Helper to build output channel JSON
    juce::String buildOutputChannelJson(int channelIndex);

    // ValueTree::Listener
    void valueTreePropertyChanged(juce::ValueTree& tree,
                                  const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&) override {}
    void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree&, int, int) override {}
    void valueTreeParentChanged(juce::ValueTree&) override {}

    // State reference
    WFSValueTreeState& state;

    // Server socket
    std::unique_ptr<juce::StreamingSocket> serverSocket;

    // Server state
    std::atomic<bool> running { false };
    int oscPort = 0;
    int httpPort = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSCQueryServer)
};

} // namespace WFSNetwork
