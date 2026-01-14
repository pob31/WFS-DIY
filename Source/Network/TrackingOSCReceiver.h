#pragma once

#include <JuceHeader.h>
#include "OSCReceiverWithSenderIP.h"
#include "../Parameters/WFSValueTreeState.h"
#include "../Parameters/WFSParameterIDs.h"

namespace WFSNetwork
{

/**
 * TrackingPathPattern
 *
 * Parses and matches OSC path patterns with placeholders.
 * Supports: <ID>, <x>, <y>, <z> placeholders in any order.
 * Example: "/wfs/tracking <ID> <x> <y> <z>"
 */
struct TrackingPathPattern
{
    juce::String baseAddress;      // The OSC address part (before arguments)
    int idArgIndex = -1;           // Which argument contains ID (-1 if not in args)
    int xArgIndex = -1;            // Which argument contains X
    int yArgIndex = -1;            // Which argument contains Y
    int zArgIndex = -1;            // Which argument contains Z
    int minRequiredArgs = 0;       // Minimum args needed (ID is required)

    /**
     * Parse a pattern string like "/wfs/tracking <ID> <x> <y> <z>"
     * @return true if pattern is valid (has at least address and ID)
     */
    bool parse(const juce::String& pattern)
    {
        baseAddress = juce::String();
        idArgIndex = xArgIndex = yArgIndex = zArgIndex = -1;
        minRequiredArgs = 0;

        if (pattern.isEmpty() || !pattern.startsWith("/"))
            return false;

        // Split pattern into tokens
        juce::StringArray tokens;
        tokens.addTokens(pattern, " ", "");
        tokens.removeEmptyStrings();

        if (tokens.isEmpty())
            return false;

        // First token is the OSC address
        baseAddress = tokens[0];

        // Remaining tokens are argument placeholders
        int argIndex = 0;
        for (int i = 1; i < tokens.size(); ++i)
        {
            juce::String token = tokens[i].toLowerCase();

            if (token == "<id>")
            {
                idArgIndex = argIndex;
            }
            else if (token == "<x>")
            {
                xArgIndex = argIndex;
            }
            else if (token == "<y>")
            {
                yArgIndex = argIndex;
            }
            else if (token == "<z>")
            {
                zArgIndex = argIndex;
            }
            // Skip unknown placeholders but still count them as arguments
            ++argIndex;
        }

        // ID is required
        if (idArgIndex < 0)
            return false;

        // Minimum args = ID position + 1
        minRequiredArgs = idArgIndex + 1;

        return true;
    }

    /**
     * Check if an OSC message matches this pattern.
     */
    bool matches(const juce::OSCMessage& msg) const
    {
        if (baseAddress.isEmpty())
            return false;

        // Check address matches
        if (msg.getAddressPattern().toString() != baseAddress)
            return false;

        // Check we have at least enough arguments for ID
        if (msg.size() < minRequiredArgs)
            return false;

        return true;
    }

    /**
     * Extract tracking ID from message (returns -1 if invalid)
     */
    int extractID(const juce::OSCMessage& msg) const
    {
        if (idArgIndex < 0 || idArgIndex >= msg.size())
            return -1;

        const auto& arg = msg[idArgIndex];
        if (arg.isInt32())
            return arg.getInt32();
        if (arg.isFloat32())
            return static_cast<int>(arg.getFloat32());

        return -1;
    }

    /**
     * Extract X coordinate from message.
     * @param hasValue Set to true if value was present in message
     */
    float extractX(const juce::OSCMessage& msg, bool& hasValue) const
    {
        hasValue = false;
        if (xArgIndex < 0 || xArgIndex >= msg.size())
            return 0.0f;

        const auto& arg = msg[xArgIndex];
        hasValue = true;
        if (arg.isFloat32())
            return arg.getFloat32();
        if (arg.isInt32())
            return static_cast<float>(arg.getInt32());

        hasValue = false;
        return 0.0f;
    }

    /**
     * Extract Y coordinate from message.
     */
    float extractY(const juce::OSCMessage& msg, bool& hasValue) const
    {
        hasValue = false;
        if (yArgIndex < 0 || yArgIndex >= msg.size())
            return 0.0f;

        const auto& arg = msg[yArgIndex];
        hasValue = true;
        if (arg.isFloat32())
            return arg.getFloat32();
        if (arg.isInt32())
            return static_cast<float>(arg.getInt32());

        hasValue = false;
        return 0.0f;
    }

    /**
     * Extract Z coordinate from message.
     */
    float extractZ(const juce::OSCMessage& msg, bool& hasValue) const
    {
        hasValue = false;
        if (zArgIndex < 0 || zArgIndex >= msg.size())
            return 0.0f;

        const auto& arg = msg[zArgIndex];
        hasValue = true;
        if (arg.isFloat32())
            return arg.getFloat32();
        if (arg.isInt32())
            return static_cast<float>(arg.getInt32());

        hasValue = false;
        return 0.0f;
    }
};

/**
 * TrackingOSCReceiver
 *
 * Dedicated OSC receiver for tracking data.
 * Listens on a separate port for tracking messages,
 * applies transformations (offset, scale, flip),
 * and routes to inputs with matching tracking IDs.
 */
class TrackingOSCReceiver : private OSCReceiverWithSenderIP::Listener
{
public:
    /**
     * Constructor.
     * @param valueTreeState Reference to parameter state for routing to inputs
     */
    explicit TrackingOSCReceiver(WFSValueTreeState& valueTreeState);
    ~TrackingOSCReceiver();

    /**
     * Start listening for tracking OSC messages.
     * @param port UDP port to listen on
     * @param pathPattern OSC path pattern with placeholders
     * @return true if started successfully
     */
    bool start(int port, const juce::String& pathPattern);

    /**
     * Stop listening.
     */
    void stop();

    /**
     * Check if receiver is active.
     */
    bool isActive() const { return receiver != nullptr && receiver->isConnected(); }

    /**
     * Update transformation parameters.
     * Called when offset/scale/flip values change.
     */
    void setTransformations(float offsetX, float offsetY, float offsetZ,
                            float scaleX, float scaleY, float scaleZ,
                            bool flipX, bool flipY, bool flipZ);

    /**
     * Update just the path pattern (while running).
     * @return true if pattern is valid
     */
    bool setPathPattern(const juce::String& pathPattern);

    /**
     * Get statistics.
     */
    struct Statistics
    {
        int messagesReceived = 0;
        int messagesMatched = 0;
        int messagesRouted = 0;
    };
    Statistics getStatistics() const;
    void resetStatistics();

private:
    // OSCReceiverWithSenderIP::Listener
    void oscMessageReceived(const juce::OSCMessage& message,
                            const juce::String& senderIP) override;
    void oscBundleReceived(const juce::OSCBundle& bundle,
                           const juce::String& senderIP) override;

    void processTrackingMessage(const juce::OSCMessage& message);
    void routeToInputs(int trackingId, float x, float y, float z,
                       bool hasX, bool hasY, bool hasZ);

    WFSValueTreeState& state;
    std::unique_ptr<OSCReceiverWithSenderIP> receiver;
    TrackingPathPattern pattern;

    // Transformation parameters
    float offsetX = 0.0f, offsetY = 0.0f, offsetZ = 0.0f;
    float scaleX = 1.0f, scaleY = 1.0f, scaleZ = 1.0f;
    bool flipX = false, flipY = false, flipZ = false;

    // Statistics
    std::atomic<int> messagesReceived { 0 };
    std::atomic<int> messagesMatched { 0 };
    std::atomic<int> messagesRouted { 0 };

    // Thread safety for pattern updates
    juce::CriticalSection patternLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackingOSCReceiver)
};

} // namespace WFSNetwork
