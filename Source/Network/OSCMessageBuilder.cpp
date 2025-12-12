#include "OSCMessageBuilder.h"

namespace WFSNetwork
{

//==============================================================================
// Static Mapping Tables
//==============================================================================

const std::map<juce::Identifier, OSCMessageBuilder::ParamMapping>& OSCMessageBuilder::getInputMappings()
{
    static const std::map<juce::Identifier, ParamMapping> mappings = {
        // Channel
        { WFSParameterIDs::inputName,             { "/wfs/input/name",             "/remoteOutput/name" } },
        { WFSParameterIDs::inputAttenuation,      { "/wfs/input/attenuation",      "/remoteOutput/attenuation" } },
        { WFSParameterIDs::inputDelayLatency,     { "/wfs/input/delayLatency",     "/remoteOutput/delayLatency" } },
        { WFSParameterIDs::inputMinimalLatency,   { "/wfs/input/minimalLatency",   "/remoteOutput/minimalLatency" } },

        // Position
        { WFSParameterIDs::inputPositionX,        { "/wfs/input/positionX",        "/remoteOutput/positionX" } },
        { WFSParameterIDs::inputPositionY,        { "/wfs/input/positionY",        "/remoteOutput/positionY" } },
        { WFSParameterIDs::inputPositionZ,        { "/wfs/input/positionZ",        "/remoteOutput/positionZ" } },
        { WFSParameterIDs::inputOffsetX,          { "/wfs/input/offsetX",          "/remoteOutput/offsetX" } },
        { WFSParameterIDs::inputOffsetY,          { "/wfs/input/offsetY",          "/remoteOutput/offsetY" } },
        { WFSParameterIDs::inputOffsetZ,          { "/wfs/input/offsetZ",          "/remoteOutput/offsetZ" } },
        { WFSParameterIDs::inputConstraintX,      { "/wfs/input/constraintX",      "/remoteOutput/constraintX" } },
        { WFSParameterIDs::inputConstraintY,      { "/wfs/input/constraintY",      "/remoteOutput/constraintY" } },
        { WFSParameterIDs::inputConstraintZ,      { "/wfs/input/constraintZ",      "/remoteOutput/constraintZ" } },
        { WFSParameterIDs::inputFlipX,            { "/wfs/input/flipX",            "/remoteOutput/flipX" } },
        { WFSParameterIDs::inputFlipY,            { "/wfs/input/flipY",            "/remoteOutput/flipY" } },
        { WFSParameterIDs::inputFlipZ,            { "/wfs/input/flipZ",            "/remoteOutput/flipZ" } },
        { WFSParameterIDs::inputCluster,          { "/wfs/input/cluster",          "/remoteOutput/cluster" } },
        { WFSParameterIDs::inputTrackingActive,   { "/wfs/input/trackingActive",   "/remoteOutput/trackingActive" } },
        { WFSParameterIDs::inputTrackingID,       { "/wfs/input/trackingID",       "/remoteOutput/trackingID" } },
        { WFSParameterIDs::inputTrackingSmooth,   { "/wfs/input/trackingSmooth",   "/remoteOutput/trackingSmooth" } },
        { WFSParameterIDs::inputMaxSpeedActive,   { "/wfs/input/maxSpeedActive",   "/remoteOutput/maxSpeedActive" } },
        { WFSParameterIDs::inputMaxSpeed,         { "/wfs/input/maxSpeed",         "/remoteOutput/maxSpeed" } },
        { WFSParameterIDs::inputHeightFactor,     { "/wfs/input/heightFactor",     "/remoteOutput/heightFactor" } },

        // Attenuation
        { WFSParameterIDs::inputAttenuationLaw,       { "/wfs/input/attenuationLaw",       "/remoteOutput/attenuationLaw" } },
        { WFSParameterIDs::inputDistanceAttenuation,  { "/wfs/input/distanceAttenuation",  "/remoteOutput/distanceAttenuation" } },
        { WFSParameterIDs::inputDistanceRatio,        { "/wfs/input/distanceRatio",        "/remoteOutput/distanceRatio" } },
        { WFSParameterIDs::inputCommonAtten,          { "/wfs/input/commonAtten",          "/remoteOutput/commonAtten" } },

        // Directivity
        { WFSParameterIDs::inputDirectivity,     { "/wfs/input/directivity",      "/remoteOutput/directivity" } },
        { WFSParameterIDs::inputRotation,        { "/wfs/input/rotation",         "/remoteOutput/rotation" } },
        { WFSParameterIDs::inputTilt,            { "/wfs/input/tilt",             "/remoteOutput/tilt" } },
        { WFSParameterIDs::inputHFshelf,         { "/wfs/input/HFshelf",          "/remoteOutput/HFshelf" } },

        // Live Source Tamer
        { WFSParameterIDs::inputLSactive,        { "/wfs/input/LSenable",         "/remoteOutput/LSenable" } },
        { WFSParameterIDs::inputLSradius,        { "/wfs/input/LSradius",         "/remoteOutput/LSradius" } },
        { WFSParameterIDs::inputLSshape,         { "/wfs/input/LSshape",          "/remoteOutput/LSshape" } },
        { WFSParameterIDs::inputLSattenuation,   { "/wfs/input/LSattenuation",    "/remoteOutput/LSattenuation" } },
        { WFSParameterIDs::inputLSpeakThreshold, { "/wfs/input/LSpeakThreshold",  "/remoteOutput/LSpeakThreshold" } },
        { WFSParameterIDs::inputLSpeakRatio,     { "/wfs/input/LSpeakRatio",      "/remoteOutput/LSpeakRatio" } },
        { WFSParameterIDs::inputLSslowThreshold, { "/wfs/input/LSslowThreshold",  "/remoteOutput/LSslowThreshold" } },
        { WFSParameterIDs::inputLSslowRatio,     { "/wfs/input/LSslowRatio",      "/remoteOutput/LSslowRatio" } },

        // Hackoustics (Floor Reflections)
        { WFSParameterIDs::inputFRactive,            { "/wfs/input/FRenable",             "/remoteOutput/FRenable" } },
        { WFSParameterIDs::inputFRattenuation,       { "/wfs/input/FRattenuation",        "/remoteOutput/FRattenuation" } },
        { WFSParameterIDs::inputFRlowCutActive,      { "/wfs/input/FRlowCutActive",       "/remoteOutput/FRlowCutActive" } },
        { WFSParameterIDs::inputFRlowCutFreq,        { "/wfs/input/FRlowCutFreq",         "/remoteOutput/FRlowCutFreq" } },
        { WFSParameterIDs::inputFRhighShelfActive,   { "/wfs/input/FRhighShelfActive",    "/remoteOutput/FRhighShelfActive" } },
        { WFSParameterIDs::inputFRhighShelfFreq,     { "/wfs/input/FRhighShelfFreq",      "/remoteOutput/FRhighShelfFreq" } },
        { WFSParameterIDs::inputFRhighShelfGain,     { "/wfs/input/FRhighShelfGain",      "/remoteOutput/FRhighShelfGain" } },
        { WFSParameterIDs::inputFRhighShelfSlope,    { "/wfs/input/FRhighShelfSlope",     "/remoteOutput/FRhighShelfSlope" } },
        { WFSParameterIDs::inputFRdiffusion,         { "/wfs/input/FRdiffusion",          "/remoteOutput/FRdiffusion" } },

        // Jitter
        { WFSParameterIDs::inputJitter,          { "/wfs/input/jitter",           "/remoteOutput/jitter" } },

        // LFO
        { WFSParameterIDs::inputLFOactive,       { "/wfs/input/LFOenable",        "/remoteOutput/LFOenable" } },
        { WFSParameterIDs::inputLFOperiod,       { "/wfs/input/LFOperiod",        "/remoteOutput/LFOperiod" } },
        { WFSParameterIDs::inputLFOphase,        { "/wfs/input/LFOphase",         "/remoteOutput/LFOphase" } },
        { WFSParameterIDs::inputLFOshapeX,       { "/wfs/input/LFOshapeX",        "/remoteOutput/LFOshapeX" } },
        { WFSParameterIDs::inputLFOshapeY,       { "/wfs/input/LFOshapeY",        "/remoteOutput/LFOshapeY" } },
        { WFSParameterIDs::inputLFOshapeZ,       { "/wfs/input/LFOshapeZ",        "/remoteOutput/LFOshapeZ" } },
        { WFSParameterIDs::inputLFOrateX,        { "/wfs/input/LFOrateX",         "/remoteOutput/LFOrateX" } },
        { WFSParameterIDs::inputLFOrateY,        { "/wfs/input/LFOrateY",         "/remoteOutput/LFOrateY" } },
        { WFSParameterIDs::inputLFOrateZ,        { "/wfs/input/LFOrateZ",         "/remoteOutput/LFOrateZ" } },
        { WFSParameterIDs::inputLFOamplitudeX,   { "/wfs/input/LFOamplitudeX",    "/remoteOutput/LFOamplitudeX" } },
        { WFSParameterIDs::inputLFOamplitudeY,   { "/wfs/input/LFOamplitudeY",    "/remoteOutput/LFOamplitudeY" } },
        { WFSParameterIDs::inputLFOamplitudeZ,   { "/wfs/input/LFOamplitudeZ",    "/remoteOutput/LFOamplitudeZ" } },
        { WFSParameterIDs::inputLFOphaseX,       { "/wfs/input/LFOphaseX",        "/remoteOutput/LFOphaseX" } },
        { WFSParameterIDs::inputLFOphaseY,       { "/wfs/input/LFOphaseY",        "/remoteOutput/LFOphaseY" } },
        { WFSParameterIDs::inputLFOphaseZ,       { "/wfs/input/LFOphaseZ",        "/remoteOutput/LFOphaseZ" } },
        { WFSParameterIDs::inputLFOgyrophone,    { "/wfs/input/LFOgyrophone",     "/remoteOutput/LFOgyrophone" } },

        // AutomOtion
        { WFSParameterIDs::inputOtomoX,                  { "/wfs/input/otomoX",              "/remoteOutput/otomoX" } },
        { WFSParameterIDs::inputOtomoY,                  { "/wfs/input/otomoY",              "/remoteOutput/otomoY" } },
        { WFSParameterIDs::inputOtomoZ,                  { "/wfs/input/otomoZ",              "/remoteOutput/otomoZ" } },
        { WFSParameterIDs::inputOtomoAbsoluteRelative,   { "/wfs/input/otomoAbsRel",         "/remoteOutput/otomoAbsRel" } },
        { WFSParameterIDs::inputOtomoStayReturn,         { "/wfs/input/otomoStayReturn",     "/remoteOutput/otomoStayReturn" } },
        { WFSParameterIDs::inputOtomoSpeedProfile,       { "/wfs/input/otomoSpeedProfile",   "/remoteOutput/otomoSpeedProfile" } },
        { WFSParameterIDs::inputOtomoTrigger,            { "/wfs/input/otomoTrigger",        "/remoteOutput/otomoTrigger" } },
        { WFSParameterIDs::inputOtomoThreshold,          { "/wfs/input/otomoThreshold",      "/remoteOutput/otomoThreshold" } },
        { WFSParameterIDs::inputOtomoReset,              { "/wfs/input/otomoReset",          "/remoteOutput/otomoReset" } },
        { WFSParameterIDs::inputOtomoPauseResume,        { "/wfs/input/otomoPauseResume",    "/remoteOutput/otomoPauseResume" } },

        // Mutes
        { WFSParameterIDs::inputMutes,           { "/wfs/input/mutes",            "/remoteOutput/mutes" } },
        { WFSParameterIDs::inputMuteMacro,       { "/wfs/input/muteMacro",        "/remoteOutput/muteMacro" } },

        // Reverb Sends
        { WFSParameterIDs::inputReverbSend,      { "/wfs/input/reverbSend",       "/remoteOutput/reverbSend" } },
    };

    return mappings;
}

const std::map<juce::Identifier, OSCMessageBuilder::ParamMapping>& OSCMessageBuilder::getOutputMappings()
{
    static const std::map<juce::Identifier, ParamMapping> mappings = {
        // Channel
        { WFSParameterIDs::outputName,               { "/wfs/output/name",              "/remoteOutput/output/name" } },
        { WFSParameterIDs::outputArray,              { "/wfs/output/array",             "/remoteOutput/output/array" } },
        { WFSParameterIDs::outputApplyToArray,       { "/wfs/output/applyToArray",      "/remoteOutput/output/applyToArray" } },
        { WFSParameterIDs::outputAttenuation,        { "/wfs/output/attenuation",       "/remoteOutput/output/attenuation" } },
        { WFSParameterIDs::outputDelayLatency,       { "/wfs/output/delayLatency",      "/remoteOutput/output/delayLatency" } },

        // Position
        { WFSParameterIDs::outputPositionX,          { "/wfs/output/positionX",         "/remoteOutput/output/positionX" } },
        { WFSParameterIDs::outputPositionY,          { "/wfs/output/positionY",         "/remoteOutput/output/positionY" } },
        { WFSParameterIDs::outputPositionZ,          { "/wfs/output/positionZ",         "/remoteOutput/output/positionZ" } },
        { WFSParameterIDs::outputOrientation,        { "/wfs/output/orientation",       "/remoteOutput/output/orientation" } },
        { WFSParameterIDs::outputAngleOn,            { "/wfs/output/angleOn",           "/remoteOutput/output/angleOn" } },
        { WFSParameterIDs::outputAngleOff,           { "/wfs/output/angleOff",          "/remoteOutput/output/angleOff" } },
        { WFSParameterIDs::outputPitch,              { "/wfs/output/pitch",             "/remoteOutput/output/pitch" } },
        { WFSParameterIDs::outputHFdamping,          { "/wfs/output/HFdamping",         "/remoteOutput/output/HFdamping" } },

        // Options
        { WFSParameterIDs::outputMiniLatencyEnable,      { "/wfs/output/miniLatencyEnable",   "/remoteOutput/output/miniLatencyEnable" } },
        { WFSParameterIDs::outputLSattenEnable,          { "/wfs/output/LSenable",            "/remoteOutput/output/LSenable" } },
        { WFSParameterIDs::outputDistanceAttenPercent,   { "/wfs/output/DistanceAttenPercent", "/remoteOutput/output/DistanceAttenPercent" } },
        { WFSParameterIDs::outputHparallax,              { "/wfs/output/Hparallax",           "/remoteOutput/output/Hparallax" } },
        { WFSParameterIDs::outputVparallax,              { "/wfs/output/Vparallax",           "/remoteOutput/output/Vparallax" } },

        // EQ
        { WFSParameterIDs::outputEQenabled,          { "/wfs/output/EQenabled",         "/remoteOutput/output/EQenabled" } },
    };

    return mappings;
}

//==============================================================================
// Message Building - Float Values
//==============================================================================

std::optional<juce::OSCMessage> OSCMessageBuilder::buildInputMessage(
    const juce::Identifier& paramId,
    int channelId,
    float value)
{
    const auto& mappings = getInputMappings();
    auto it = mappings.find(paramId);

    if (it == mappings.end())
        return std::nullopt;

    return buildMessage(it->second.oscPath, channelId, value);
}

std::optional<juce::OSCMessage> OSCMessageBuilder::buildOutputMessage(
    const juce::Identifier& paramId,
    int channelId,
    float value)
{
    const auto& mappings = getOutputMappings();
    auto it = mappings.find(paramId);

    if (it == mappings.end())
        return std::nullopt;

    return buildMessage(it->second.oscPath, channelId, value);
}

//==============================================================================
// Message Building - String Values
//==============================================================================

std::optional<juce::OSCMessage> OSCMessageBuilder::buildInputStringMessage(
    const juce::Identifier& paramId,
    int channelId,
    const juce::String& value)
{
    const auto& mappings = getInputMappings();
    auto it = mappings.find(paramId);

    if (it == mappings.end())
        return std::nullopt;

    return buildMessage(it->second.oscPath, channelId, value);
}

std::optional<juce::OSCMessage> OSCMessageBuilder::buildOutputStringMessage(
    const juce::Identifier& paramId,
    int channelId,
    const juce::String& value)
{
    const auto& mappings = getOutputMappings();
    auto it = mappings.find(paramId);

    if (it == mappings.end())
        return std::nullopt;

    return buildMessage(it->second.oscPath, channelId, value);
}

//==============================================================================
// REMOTE Protocol Messages
//==============================================================================

std::optional<juce::OSCMessage> OSCMessageBuilder::buildRemoteOutputMessage(
    const juce::Identifier& paramId,
    int channelId,
    float value)
{
    const auto& mappings = getInputMappings();
    auto it = mappings.find(paramId);

    if (it == mappings.end())
        return std::nullopt;

    return buildMessage(it->second.remotePath, channelId, value);
}

std::vector<juce::OSCMessage> OSCMessageBuilder::buildRemoteChannelDump(
    int channelId,
    const std::map<juce::Identifier, float>& paramValues)
{
    std::vector<juce::OSCMessage> messages;
    messages.reserve(paramValues.size());

    const auto& mappings = getInputMappings();

    for (const auto& [paramId, value] : paramValues)
    {
        auto it = mappings.find(paramId);
        if (it != mappings.end())
        {
            messages.push_back(buildMessage(it->second.remotePath, channelId, value));
        }
    }

    return messages;
}

//==============================================================================
// Path Queries
//==============================================================================

juce::String OSCMessageBuilder::getInputOSCPath(const juce::Identifier& paramId)
{
    const auto& mappings = getInputMappings();
    auto it = mappings.find(paramId);
    return (it != mappings.end()) ? it->second.oscPath : juce::String();
}

juce::String OSCMessageBuilder::getOutputOSCPath(const juce::Identifier& paramId)
{
    const auto& mappings = getOutputMappings();
    auto it = mappings.find(paramId);
    return (it != mappings.end()) ? it->second.oscPath : juce::String();
}

bool OSCMessageBuilder::isInputMapped(const juce::Identifier& paramId)
{
    return getInputMappings().find(paramId) != getInputMappings().end();
}

bool OSCMessageBuilder::isOutputMapped(const juce::Identifier& paramId)
{
    return getOutputMappings().find(paramId) != getOutputMappings().end();
}

//==============================================================================
// Bundle Building
//==============================================================================

juce::OSCBundle OSCMessageBuilder::createBundle(const std::vector<juce::OSCMessage>& messages)
{
    juce::OSCBundle bundle;

    for (const auto& msg : messages)
    {
        bundle.addElement(msg);
    }

    return bundle;
}

//==============================================================================
// Private Helpers
//==============================================================================

juce::OSCMessage OSCMessageBuilder::buildMessage(
    const juce::String& address,
    int channelId,
    float value)
{
    juce::OSCMessage msg(address);
    msg.addInt32(channelId);
    msg.addFloat32(value);
    return msg;
}

juce::OSCMessage OSCMessageBuilder::buildMessage(
    const juce::String& address,
    int channelId,
    const juce::String& value)
{
    juce::OSCMessage msg(address);
    msg.addInt32(channelId);
    msg.addString(value);
    return msg;
}

} // namespace WFSNetwork
