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
     * Build an OSC message for a reverb parameter change.
     */
    static std::optional<juce::OSCMessage> buildReverbMessage(
        const juce::Identifier& paramId,
        int channelId,
        float value);

    /**
     * Build an OSC message for a reverb PRE-EQ BAND parameter change.
     *
     * The pre-EQ properties live on <Band id="n"> nodes, four per reverb
     * channel, so the two-argument form cannot say which band moved and a
     * receiver cannot mirror the state from it. This emits the three-argument
     * form the standard inbound parser reads - channel, band, value - with the
     * band 1-based, matching the channel number in the path, the OSCQuery
     * descriptor and the equivalent MCP tool.
     *
     * @param paramId   one of the reverbPreEQ* band properties
     * @param channelId 1-based reverb channel
     * @param bandIndex 1-based band
     * @param value     the new value
     * @return OSC message, or nullopt if the parameter is not a mapped
     *         pre-EQ band property
     */
    static std::optional<juce::OSCMessage> buildReverbBandMessage(
        const juce::Identifier& paramId,
        int channelId,
        int bandIndex,
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

    static std::optional<juce::OSCMessage> buildReverbStringMessage(
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
     * Build a REMOTE protocol output message with integer value.
     * Format: /remoteInput/{param} <channelID:i> <value:i>
     */
    static std::optional<juce::OSCMessage> buildRemoteOutputIntMessage(
        const juce::Identifier& paramId,
        int channelId,
        int value);

    /**
     * Build a REMOTE protocol output message with string value.
     * Format: /remoteInput/{param} <channelID> <value>
     */
    static std::optional<juce::OSCMessage> buildRemoteOutputStringMessage(
        const juce::Identifier& paramId,
        int channelId,
        const juce::String& value);

    /**
     * Build the REMOTE echo of one stored input value, typed for the tablet.
     * Int and double vars go out as ,ii / ,if. A string var on a parameter with
     * bounds is a number a load, a snapshot recall or an undo left as text: it goes
     * out typed by the parameter (,ii or ,if), never as ,is, which the tablet
     * stores as 0; text that is not a number is dropped. A string on a parameter
     * without bounds (a name, a mute list) still goes out as ,is.
     */
    static std::optional<juce::OSCMessage> buildRemoteEchoMessage(
        const juce::Identifier& paramId,
        int channelId,
        const juce::var& value);

    /**
     * Build all REMOTE protocol messages for a channel.
     * Used when Android app requests channel data.
     */
    static std::vector<juce::OSCMessage> buildRemoteChannelDump(
        int channelId,
        const std::map<juce::Identifier, float>& paramValues);

    /**
     * Build all REMOTE protocol messages for a channel (with separate int params).
     */
    static std::vector<juce::OSCMessage> buildRemoteChannelDump(
        int channelId,
        const std::map<juce::Identifier, float>& floatParamValues,
        const std::map<juce::Identifier, int>& intParamValues);

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
     * Get the OSC path for a reverb parameter.
     */
    static juce::String getReverbOSCPath(const juce::Identifier& paramId);

    /**
     * Check if a parameter ID is mapped for OSC.
     */
    static bool isInputMapped(const juce::Identifier& paramId);
    static bool isOutputMapped(const juce::Identifier& paramId);
    static bool isReverbMapped(const juce::Identifier& paramId);
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

    //==========================================================================
    // Mapping Tables (public for QLab cue builder access)
    //==========================================================================

    struct ParamMapping
    {
        juce::String oscPath;      // e.g., "/wfs/input/attenuation"
        juce::String remotePath;   // e.g., "/remoteOutput/attenuation"
    };

    static const std::map<juce::Identifier, ParamMapping>& getInputMappings();
    static const std::map<juce::Identifier, ParamMapping>& getOutputMappings();
    static const std::map<juce::Identifier, ParamMapping>& getReverbMappings();

    /** Every per-channel effect parameter's /wfs/effect/<name> address: the inverse
        of OSCMessageRouter::getEffectAddressMap (the effects CSV's "OSC path"
        column), minus the four send-cell pseudo-identifiers (a cell is not a
        stored value) and the global. No remote path: the tablet has no effects
        page. The QLab snapshot cues use it; the outbound echo (C8) is next. */
    static const std::map<juce::Identifier, ParamMapping>& getEffectMappings();
    static const std::map<juce::Identifier, juce::String>& getConfigMappings();

private:

    static juce::OSCMessage buildMessage(
        const juce::String& address,
        int channelId,
        float value);

    static juce::OSCMessage buildIntMessage(
        const juce::String& address,
        int channelId,
        int value);

    static juce::OSCMessage buildMessage(
        const juce::String& address,
        int channelId,
        const juce::String& value);
};

} // namespace WFSNetwork
