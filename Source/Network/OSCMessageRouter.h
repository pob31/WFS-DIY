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
        juce::var value;
        bool valid = false;
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
        enum class Type { ChannelSelect, PositionDelta };

        Type type = Type::ChannelSelect;
        int channelId = 0;
        Axis axis = Axis::X;
        DeltaDirection direction = DeltaDirection::Increment;
        float deltaValue = 0.0f;
        bool valid = false;
    };

    //==========================================================================
    // Message Routing
    //==========================================================================

    /**
     * Parse a standard OSC message.
     * Determines if it's an input, output, or reverb message and extracts data.
     */
    static ParsedInputMessage parseInputMessage(const juce::OSCMessage& message);
    static ParsedOutputMessage parseOutputMessage(const juce::OSCMessage& message);
    static ParsedReverbMessage parseReverbMessage(const juce::OSCMessage& message);

    /**
     * Parse a REMOTE protocol input message from Android app.
     * Handles /remoteInput/* addresses.
     */
    static ParsedRemoteInput parseRemoteInputMessage(const juce::OSCMessage& message);

    /**
     * Check if an address matches input, output, or reverb patterns.
     */
    static bool isInputAddress(const juce::String& address);
    static bool isOutputAddress(const juce::String& address);
    static bool isReverbAddress(const juce::String& address);
    static bool isRemoteInputAddress(const juce::String& address);

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

private:
    //==========================================================================
    // Internal Lookup Tables
    //==========================================================================

    static const std::map<juce::String, juce::Identifier>& getInputAddressMap();
    static const std::map<juce::String, juce::Identifier>& getOutputAddressMap();
    static const std::map<juce::String, juce::Identifier>& getReverbAddressMap();

    /**
     * Extract the parameter portion from an OSC address.
     * e.g., "/wfs/input/attenuation" -> "attenuation"
     */
    static juce::String extractParamName(const juce::String& address);
};

} // namespace WFSNetwork
