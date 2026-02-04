#pragma once

#include <JuceHeader.h>
#include "OSCProtocolTypes.h"
#include "../Parameters/WFSParameterIDs.h"

namespace WFSNetwork
{

/**
 * OSCMessageRouter
 *
 * Routes incoming OSC messages to parameter updates.
 * Parses OSC address patterns and extracts values.
 * Handles both standard OSC and REMOTE protocol messages.
 */
class OSCMessageRouter
{
public:
    //==========================================================================
    // Parsed Message Types
    //==========================================================================

    struct ParsedInputMessage
    {
        juce::Identifier paramId;
        int channelId = 0;
        juce::var value;
        bool valid = false;
    };

    struct ParsedOutputMessage
    {
        juce::Identifier paramId;
        int channelId = 0;
        int bandIndex = 0;  // For EQ parameters (1-6)
        juce::var value;
        bool valid = false;
        bool isEQparam = false;  // True for EQ parameters that need band index
    };

    struct ParsedReverbMessage
    {
        juce::Identifier paramId;
        int channelId = 0;
        int bandIndex = 0;  // For EQ parameters (1-4)
        juce::var value;
        bool valid = false;
        bool isEQparam = false;  // True for EQ parameters that need band index
    };

    struct ParsedRemoteInput
    {
        enum class Type {
            ChannelSelect,    // /remoteInput/inputNumber <ID> - request all params
            PositionDelta,    // Legacy: /remoteInput/positionX <ID> <inc/dec> <delta>
            ParameterSet,     // /remoteInput/<param> <ID> <value> - absolute set
            ParameterDelta,   // /remoteInput/<param> <ID> <inc/dec> <delta> - relative change
            PositionXY        // /remoteInput/positionXY <ID> <x> <y> - atomic XY position
        };

        Type type = Type::ChannelSelect;
        int channelId = 0;
        juce::Identifier paramId;                              // Which parameter (for ParameterSet/Delta)
        Axis axis = Axis::X;                                   // For legacy PositionDelta
        DeltaDirection direction = DeltaDirection::Increment;  // For delta types
        juce::var value;                                       // Value or delta amount
        float deltaValue = 0.0f;                               // Legacy: delta for PositionDelta
        float posX = 0.0f;                                     // For PositionXY: X coordinate
        float posY = 0.0f;                                     // For PositionXY: Y coordinate
        bool valid = false;
    };

    struct ParsedArrayAdjustMessage
    {
        juce::Identifier paramId;  // Output parameter to adjust
        int arrayId = 0;           // Array/cluster ID (1-based from remote)
        float valueChange = 0.0f;  // Delta to apply to parameter
        bool valid = false;
    };

    struct ParsedConfigMessage
    {
        juce::Identifier paramId;  // Config parameter identifier
        juce::var value;           // Parameter value (float or int)
        bool valid = false;
    };

    struct ParsedClusterMoveMessage
    {
        enum class Type {
            ClusterMove,      // /cluster/move <clusterId> <deltaX> <deltaY>
            BarycenterMove    // /cluster/barycenter/move <clusterId> <deltaX> <deltaY>
        };

        Type type = Type::ClusterMove;
        int clusterId = 0;       // Cluster ID (1-10)
        float deltaX = 0.0f;     // X delta in meters
        float deltaY = 0.0f;     // Y delta in meters
        bool valid = false;
    };

    //==========================================================================
    // Message Routing
    //==========================================================================

    /**
     * Parse a standard OSC message.
     * Determines if it's an input, output, reverb, or config message and extracts data.
     */
    static ParsedInputMessage parseInputMessage(const juce::OSCMessage& message);
    static ParsedOutputMessage parseOutputMessage(const juce::OSCMessage& message);
    static ParsedReverbMessage parseReverbMessage(const juce::OSCMessage& message);
    static ParsedConfigMessage parseConfigMessage(const juce::OSCMessage& message);

    /**
     * Parse a REMOTE protocol input message from Android app.
     * Handles /remoteInput/* addresses.
     */
    static ParsedRemoteInput parseRemoteInputMessage(const juce::OSCMessage& message);

    /**
     * Parse an array adjustment message from remote.
     * Handles /arrayAdjust/* addresses for bulk output parameter changes.
     */
    static ParsedArrayAdjustMessage parseArrayAdjustMessage(const juce::OSCMessage& message);

    /**
     * Parse a cluster move message from remote.
     * Handles /cluster/move and /cluster/barycenter/move addresses.
     */
    static ParsedClusterMoveMessage parseClusterMoveMessage(const juce::OSCMessage& message);

    /**
     * Check if an address matches input, output, reverb, or config patterns.
     */
    static bool isInputAddress(const juce::String& address);
    static bool isOutputAddress(const juce::String& address);
    static bool isReverbAddress(const juce::String& address);
    static bool isConfigAddress(const juce::String& address);
    static bool isRemoteInputAddress(const juce::String& address);
    static bool isArrayAdjustAddress(const juce::String& address);
    static bool isClusterMoveAddress(const juce::String& address);

    //==========================================================================
    // Address Pattern Matching
    //==========================================================================

    /**
     * Get the parameter identifier from an OSC address.
     * @param address The OSC address (e.g., "/wfs/input/attenuation")
     * @return The parameter ID or empty Identifier if not found
     */
    static juce::Identifier getInputParamId(const juce::String& address);
    static juce::Identifier getOutputParamId(const juce::String& address);
    static juce::Identifier getReverbParamId(const juce::String& address);
    static juce::Identifier getConfigParamId(const juce::String& address);

    //==========================================================================
    // Value Extraction
    //==========================================================================

    /**
     * Extract a float value from OSC arguments.
     * Handles both float32 and int32 arguments.
     */
    static float extractFloat(const juce::OSCArgument& arg);

    /**
     * Extract an int value from OSC arguments.
     */
    static int extractInt(const juce::OSCArgument& arg);

    /**
     * Extract a string value from OSC arguments.
     */
    static juce::String extractString(const juce::OSCArgument& arg);

    /**
     * Extract the parameter portion from an OSC address.
     * e.g., "/wfs/input/attenuation" -> "attenuation"
     */
    static juce::String extractParamName(const juce::String& address);

private:
    //==========================================================================
    // Internal Lookup Tables
    //==========================================================================

    static const std::map<juce::String, juce::Identifier>& getInputAddressMap();
    static const std::map<juce::String, juce::Identifier>& getOutputAddressMap();
    static const std::map<juce::String, juce::Identifier>& getReverbAddressMap();
    static const std::map<juce::String, juce::Identifier>& getRemoteAddressMap();
    static const std::map<juce::String, juce::Identifier>& getConfigAddressMap();
};

} // namespace WFSNetwork
