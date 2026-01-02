#pragma once

#include <JuceHeader.h>
#include "OSCProtocolTypes.h"
#include "../Parameters/WFSParameterIDs.h"

namespace WFSNetwork
{

/**
 * OSCMessageBuilder
 *
 * Converts parameter changes to OSC messages.
 * Maps WFSParameterIDs to OSC address patterns.
 * Values are in real-world units (dB, meters, degrees, ms).
 */
class OSCMessageBuilder
{
public:
    //==========================================================================
    // Message Building
    //==========================================================================

    /**
     * Build an OSC message for an input parameter change.
     * @param paramId The parameter identifier (e.g., inputAttenuation)
     * @param channelId 1-based channel number
     * @param value The parameter value in real-world units
     * @return OSC message or nullopt if parameter not mapped
     */
    static std::optional<juce::OSCMessage> buildInputMessage(
        const juce::Identifier& paramId,
        int channelId,
        float value);

    /**
     * Build an OSC message for an output parameter change.
     */
    static std::optional<juce::OSCMessage> buildOutputMessage(
        const juce::Identifier& paramId,
        int channelId,
        float value);

    /**
     * Build an OSC message for a config parameter change.
     * Config parameters are global (no channel ID).
     * @param paramId The parameter identifier (e.g., stageShape)
     * @param value The parameter value
     * @return OSC message or nullopt if parameter not mapped
     */
    static std::optional<juce::OSCMessage> buildConfigMessage(
        const juce::Identifier& paramId,
        float value);

    /**
     * Build an OSC message for a config integer parameter.
     */
    static std::optional<juce::OSCMessage> buildConfigMessage(
        const juce::Identifier& paramId,
        int value);

    /**
     * Build an OSC message for a string parameter (e.g., name).
     */
    static std::optional<juce::OSCMessage> buildInputStringMessage(
        const juce::Identifier& paramId,
        int channelId,
        const juce::String& value);

    static std::optional<juce::OSCMessage> buildOutputStringMessage(
        const juce::Identifier& paramId,
        int channelId,
        const juce::String& value);

    //==========================================================================
    // REMOTE Protocol Messages
    //==========================================================================

    /**
     * Build a REMOTE protocol output message.
     * Format: /remoteOutput/{param} <channelID> <value>
     */
    static std::optional<juce::OSCMessage> buildRemoteOutputMessage(
        const juce::Identifier& paramId,
        int channelId,
        float value);

    /**
     * Build all REMOTE protocol messages for a channel.
     * Used when Android app requests channel data.
     */
    static std::vector<juce::OSCMessage> buildRemoteChannelDump(
        int channelId,
        const std::map<juce::Identifier, float>& paramValues);

    //==========================================================================
    // Path Queries
    //==========================================================================

    /**
     * Get the OSC path for an input parameter.
     * @return Path string or empty if not mapped
     */
    static juce::String getInputOSCPath(const juce::Identifier& paramId);

    /**
     * Get the OSC path for an output parameter.
     */
    static juce::String getOutputOSCPath(const juce::Identifier& paramId);

    /**
     * Get the OSC path for a config parameter.
     */
    static juce::String getConfigOSCPath(const juce::Identifier& paramId);

    /**
     * Check if a parameter ID is mapped for OSC.
     */
    static bool isInputMapped(const juce::Identifier& paramId);
    static bool isOutputMapped(const juce::Identifier& paramId);
    static bool isConfigMapped(const juce::Identifier& paramId);

    //==========================================================================
    // Bundle Building
    //==========================================================================

    /**
     * Create an OSC bundle from multiple messages.
     */
    static juce::OSCBundle createBundle(const std::vector<juce::OSCMessage>& messages);

    //==========================================================================
    // Direct Message Building (for stage config, etc.)
    //==========================================================================

    /**
     * Build a config float message with custom address.
     * @param address The OSC address (e.g., "/stage/width")
     * @param value The float value
     */
    static juce::OSCMessage buildConfigFloatMessage(
        const juce::String& address,
        float value);

    /**
     * Build a config integer message with custom address.
     * @param address The OSC address (e.g., "/stage/shape")
     * @param value The integer value
     */
    static juce::OSCMessage buildConfigIntMessage(
        const juce::String& address,
        int value);

private:
    //==========================================================================
    // Internal Mapping
    //==========================================================================

    struct ParamMapping
    {
        juce::String oscPath;      // e.g., "/wfs/input/attenuation"
        juce::String remotePath;   // e.g., "/remoteOutput/attenuation"
    };

    static const std::map<juce::Identifier, ParamMapping>& getInputMappings();
    static const std::map<juce::Identifier, ParamMapping>& getOutputMappings();
    static const std::map<juce::Identifier, juce::String>& getConfigMappings();

    static juce::OSCMessage buildMessage(
        const juce::String& address,
        int channelId,
        float value);

    static juce::OSCMessage buildMessage(
        const juce::String& address,
        int channelId,
        const juce::String& value);
};

} // namespace WFSNetwork
