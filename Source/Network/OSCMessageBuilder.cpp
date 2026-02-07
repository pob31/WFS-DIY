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
        { WFSParameterIDs::inputName,             { "/wfs/input/name",             "/remoteInput/inputName" } },
        { WFSParameterIDs::inputAttenuation,      { "/wfs/input/attenuation",      "/remoteInput/attenuation" } },
        { WFSParameterIDs::inputDelayLatency,     { "/wfs/input/delayLatency",     "/remoteInput/delayLatency" } },
        { WFSParameterIDs::inputMinimalLatency,   { "/wfs/input/minimalLatency",   "/remoteInput/minimalLatency" } },

        // Position
        { WFSParameterIDs::inputPositionX,        { "/wfs/input/positionX",        "/remoteInput/positionX" } },
        { WFSParameterIDs::inputPositionY,        { "/wfs/input/positionY",        "/remoteInput/positionY" } },
        { WFSParameterIDs::inputPositionZ,        { "/wfs/input/positionZ",        "/remoteInput/positionZ" } },
        { WFSParameterIDs::inputOffsetX,          { "/wfs/input/offsetX",          "/remoteInput/offsetX" } },
        { WFSParameterIDs::inputOffsetY,          { "/wfs/input/offsetY",          "/remoteInput/offsetY" } },
        { WFSParameterIDs::inputOffsetZ,          { "/wfs/input/offsetZ",          "/remoteInput/offsetZ" } },
        { WFSParameterIDs::inputConstraintX,      { "/wfs/input/constraintX",      "/remoteInput/constraintX" } },
        { WFSParameterIDs::inputConstraintY,      { "/wfs/input/constraintY",      "/remoteInput/constraintY" } },
        { WFSParameterIDs::inputConstraintZ,      { "/wfs/input/constraintZ",      "/remoteInput/constraintZ" } },
        { WFSParameterIDs::inputConstraintDistance,    { "/wfs/input/constraintDistance",    "/remoteInput/constraintDistance" } },
        { WFSParameterIDs::inputConstraintDistanceMin, { "/wfs/input/constraintDistanceMin", "/remoteInput/constraintDistanceMin" } },
        { WFSParameterIDs::inputConstraintDistanceMax, { "/wfs/input/constraintDistanceMax", "/remoteInput/constraintDistanceMax" } },
        { WFSParameterIDs::inputFlipX,            { "/wfs/input/flipX",            "/remoteInput/flipX" } },
        { WFSParameterIDs::inputFlipY,            { "/wfs/input/flipY",            "/remoteInput/flipY" } },
        { WFSParameterIDs::inputFlipZ,            { "/wfs/input/flipZ",            "/remoteInput/flipZ" } },
        { WFSParameterIDs::inputCluster,          { "/wfs/input/cluster",          "/remoteInput/cluster" } },
        { WFSParameterIDs::inputTrackingActive,   { "/wfs/input/trackingActive",   "/remoteInput/trackingActive" } },
        { WFSParameterIDs::inputTrackingID,       { "/wfs/input/trackingID",       "/remoteInput/trackingID" } },
        { WFSParameterIDs::inputTrackingSmooth,   { "/wfs/input/trackingSmooth",   "/remoteInput/trackingSmooth" } },
        { WFSParameterIDs::inputMaxSpeedActive,   { "/wfs/input/maxSpeedActive",   "/remoteInput/maxSpeedActive" } },
        { WFSParameterIDs::inputMaxSpeed,         { "/wfs/input/maxSpeed",         "/remoteInput/maxSpeed" } },
        { WFSParameterIDs::inputPathModeActive,   { "/wfs/input/pathModeActive",   "/remoteInput/pathModeActive" } },
        { WFSParameterIDs::inputHeightFactor,     { "/wfs/input/heightFactor",     "/remoteInput/heightFactor" } },
        { WFSParameterIDs::inputCoordinateMode,   { "/wfs/input/coordinateMode",   "/remoteInput/coordinateMode" } },

        // Attenuation
        { WFSParameterIDs::inputAttenuationLaw,       { "/wfs/input/attenuationLaw",       "/remoteInput/attenuationLaw" } },
        { WFSParameterIDs::inputDistanceAttenuation,  { "/wfs/input/distanceAttenuation",  "/remoteInput/distanceAttenuation" } },
        { WFSParameterIDs::inputDistanceRatio,        { "/wfs/input/distanceRatio",        "/remoteInput/distanceRatio" } },
        { WFSParameterIDs::inputCommonAtten,          { "/wfs/input/commonAtten",          "/remoteInput/commonAtten" } },

        // Directivity
        { WFSParameterIDs::inputDirectivity,     { "/wfs/input/directivity",      "/remoteInput/directivity" } },
        { WFSParameterIDs::inputRotation,        { "/wfs/input/rotation",         "/remoteInput/rotation" } },
        { WFSParameterIDs::inputTilt,            { "/wfs/input/tilt",             "/remoteInput/tilt" } },
        { WFSParameterIDs::inputHFshelf,         { "/wfs/input/HFshelf",          "/remoteInput/HFshelf" } },

        // Live Source Tamer
        { WFSParameterIDs::inputLSactive,        { "/wfs/input/LSactive",         "/remoteInput/liveSourceActive" } },
        { WFSParameterIDs::inputLSradius,        { "/wfs/input/LSradius",         "/remoteInput/liveSourceRadius" } },
        { WFSParameterIDs::inputLSshape,         { "/wfs/input/LSshape",          "/remoteInput/liveSourceShape" } },
        { WFSParameterIDs::inputLSattenuation,   { "/wfs/input/LSattenuation",    "/remoteInput/liveSourceAttenuation" } },
        { WFSParameterIDs::inputLSpeakThreshold, { "/wfs/input/LSpeakThreshold",  "/remoteInput/liveSourcePeakThreshold" } },
        { WFSParameterIDs::inputLSpeakRatio,     { "/wfs/input/LSpeakRatio",      "/remoteInput/liveSourcePeakRatio" } },
        { WFSParameterIDs::inputLSslowThreshold, { "/wfs/input/LSslowThreshold",  "/remoteInput/liveSourceSlowThreshold" } },
        { WFSParameterIDs::inputLSslowRatio,     { "/wfs/input/LSslowRatio",      "/remoteInput/liveSourceSlowRatio" } },

        // Hackoustics (Floor Reflections)
        { WFSParameterIDs::inputFRactive,            { "/wfs/input/FRactive",             "/remoteInput/FRactive" } },
        { WFSParameterIDs::inputFRattenuation,       { "/wfs/input/FRattenuation",        "/remoteInput/FRattenuation" } },
        { WFSParameterIDs::inputFRlowCutActive,      { "/wfs/input/FRlowCutActive",       "/remoteInput/FRlowCutActive" } },
        { WFSParameterIDs::inputFRlowCutFreq,        { "/wfs/input/FRlowCutFreq",         "/remoteInput/FRlowCutFreq" } },
        { WFSParameterIDs::inputFRhighShelfActive,   { "/wfs/input/FRhighShelfActive",    "/remoteInput/FRhighShelfActive" } },
        { WFSParameterIDs::inputFRhighShelfFreq,     { "/wfs/input/FRhighShelfFreq",      "/remoteInput/FRhighShelfFreq" } },
        { WFSParameterIDs::inputFRhighShelfGain,     { "/wfs/input/FRhighShelfGain",      "/remoteInput/FRhighShelfGain" } },
        { WFSParameterIDs::inputFRhighShelfSlope,    { "/wfs/input/FRhighShelfSlope",     "/remoteInput/FRhighShelfSlope" } },
        { WFSParameterIDs::inputFRdiffusion,         { "/wfs/input/FRdiffusion",          "/remoteInput/FRdiffusion" } },

        // Jitter
        { WFSParameterIDs::inputJitter,          { "/wfs/input/jitter",           "/remoteInput/jitter" } },

        // LFO
        { WFSParameterIDs::inputLFOactive,       { "/wfs/input/LFOactive",        "/remoteInput/LFOactive" } },
        { WFSParameterIDs::inputLFOperiod,       { "/wfs/input/LFOperiod",        "/remoteInput/LFOperiod" } },
        { WFSParameterIDs::inputLFOphase,        { "/wfs/input/LFOphase",         "/remoteInput/LFOphase" } },
        { WFSParameterIDs::inputLFOshapeX,       { "/wfs/input/LFOshapeX",        "/remoteInput/LFOshapeX" } },
        { WFSParameterIDs::inputLFOshapeY,       { "/wfs/input/LFOshapeY",        "/remoteInput/LFOshapeY" } },
        { WFSParameterIDs::inputLFOshapeZ,       { "/wfs/input/LFOshapeZ",        "/remoteInput/LFOshapeZ" } },
        { WFSParameterIDs::inputLFOrateX,        { "/wfs/input/LFOrateX",         "/remoteInput/LFOrateX" } },
        { WFSParameterIDs::inputLFOrateY,        { "/wfs/input/LFOrateY",         "/remoteInput/LFOrateY" } },
        { WFSParameterIDs::inputLFOrateZ,        { "/wfs/input/LFOrateZ",         "/remoteInput/LFOrateZ" } },
        { WFSParameterIDs::inputLFOamplitudeX,   { "/wfs/input/LFOamplitudeX",    "/remoteInput/LFOamplitudeX" } },
        { WFSParameterIDs::inputLFOamplitudeY,   { "/wfs/input/LFOamplitudeY",    "/remoteInput/LFOamplitudeY" } },
        { WFSParameterIDs::inputLFOamplitudeZ,   { "/wfs/input/LFOamplitudeZ",    "/remoteInput/LFOamplitudeZ" } },
        { WFSParameterIDs::inputLFOphaseX,       { "/wfs/input/LFOphaseX",        "/remoteInput/LFOphaseX" } },
        { WFSParameterIDs::inputLFOphaseY,       { "/wfs/input/LFOphaseY",        "/remoteInput/LFOphaseY" } },
        { WFSParameterIDs::inputLFOphaseZ,       { "/wfs/input/LFOphaseZ",        "/remoteInput/LFOphaseZ" } },
        { WFSParameterIDs::inputLFOgyrophone,    { "/wfs/input/LFOgyrophone",     "/remoteInput/LFOgyrophone" } },

        // AutomOtion
        { WFSParameterIDs::inputOtomoX,                  { "/wfs/input/otomoX",                  "" } },
        { WFSParameterIDs::inputOtomoY,                  { "/wfs/input/otomoY",                  "" } },
        { WFSParameterIDs::inputOtomoZ,                  { "/wfs/input/otomoZ",                  "" } },
        { WFSParameterIDs::inputOtomoAbsoluteRelative,   { "/wfs/input/otomoAbsoluteRelative",   "" } },
        { WFSParameterIDs::inputOtomoStayReturn,         { "/wfs/input/otomoStayReturn",         "" } },
        { WFSParameterIDs::inputOtomoDuration,           { "/wfs/input/otomoDuration",           "" } },
        { WFSParameterIDs::inputOtomoCurve,              { "/wfs/input/otomoCurve",              "" } },
        { WFSParameterIDs::inputOtomoSpeedProfile,       { "/wfs/input/otomoSpeed",              "" } },
        { WFSParameterIDs::inputOtomoTrigger,            { "/wfs/input/otomoTrigger",            "" } },
        { WFSParameterIDs::inputOtomoThreshold,          { "/wfs/input/otomoTriggerThreshold",   "" } },
        { WFSParameterIDs::inputOtomoReset,              { "/wfs/input/otomoTriggerReset",       "" } },
        { WFSParameterIDs::inputOtomoPauseResume,        { "/wfs/input/otomoPauseResume",        "" } },

        // Mutes
        { WFSParameterIDs::inputMutes,           { "/wfs/input/mutes",            "/remoteInput/mutes" } },
        { WFSParameterIDs::inputMuteMacro,       { "/wfs/input/muteMacro",        "/remoteInput/muteMacro" } },

        // Sidelines
        { WFSParameterIDs::inputSidelinesActive, { "/wfs/input/sidelinesEnable",  "/remoteInput/sidelinesActive" } },
        { WFSParameterIDs::inputSidelinesFringe, { "/wfs/input/sidelinesFringe",  "/remoteInput/sidelinesFringe" } },

        // Reverb Sends
        { WFSParameterIDs::inputReverbSend,      { "/wfs/input/reverbSend",       "/remoteInput/reverbSend" } },
    };

    return mappings;
}

const std::map<juce::Identifier, juce::String>& OSCMessageBuilder::getConfigMappings()
{
    static const std::map<juce::Identifier, juce::String> mappings = {
        // Stage parameters
        { WFSParameterIDs::stageShape,        OSCPaths::CONFIG_STAGE_SHAPE },
        { WFSParameterIDs::stageWidth,        OSCPaths::CONFIG_STAGE_WIDTH },
        { WFSParameterIDs::stageDepth,        OSCPaths::CONFIG_STAGE_DEPTH },
        { WFSParameterIDs::stageHeight,       OSCPaths::CONFIG_STAGE_HEIGHT },
        { WFSParameterIDs::stageDiameter,     OSCPaths::CONFIG_STAGE_DIAMETER },
        { WFSParameterIDs::domeElevation,     OSCPaths::CONFIG_STAGE_DOME_ELEVATION },
        { WFSParameterIDs::originWidth,       OSCPaths::CONFIG_STAGE_ORIGIN_X },
        { WFSParameterIDs::originDepth,       OSCPaths::CONFIG_STAGE_ORIGIN_Y },
        { WFSParameterIDs::originHeight,      OSCPaths::CONFIG_STAGE_ORIGIN_Z },
    };

    return mappings;
}

const std::map<juce::Identifier, OSCMessageBuilder::ParamMapping>& OSCMessageBuilder::getOutputMappings()
{
    static const std::map<juce::Identifier, ParamMapping> mappings = {
        // Channel
        { WFSParameterIDs::outputName,               { "/wfs/output/name",              "/remoteInput/output/name" } },
        { WFSParameterIDs::outputArray,              { "/wfs/output/array",             "/remoteInput/output/array" } },
        { WFSParameterIDs::outputApplyToArray,       { "/wfs/output/applyToArray",      "/remoteInput/output/applyToArray" } },
        { WFSParameterIDs::outputAttenuation,        { "/wfs/output/attenuation",       "/remoteInput/output/attenuation" } },
        { WFSParameterIDs::outputDelayLatency,       { "/wfs/output/delayLatency",      "/remoteInput/output/delayLatency" } },

        // Position
        { WFSParameterIDs::outputPositionX,          { "/wfs/output/positionX",         "/remoteInput/output/positionX" } },
        { WFSParameterIDs::outputPositionY,          { "/wfs/output/positionY",         "/remoteInput/output/positionY" } },
        { WFSParameterIDs::outputPositionZ,          { "/wfs/output/positionZ",         "/remoteInput/output/positionZ" } },
        { WFSParameterIDs::outputCoordinateMode,     { "/wfs/output/coordinateMode",    "/remoteInput/output/coordinateMode" } },
        { WFSParameterIDs::outputOrientation,        { "/wfs/output/orientation",       "/remoteInput/output/orientation" } },
        { WFSParameterIDs::outputAngleOn,            { "/wfs/output/angleOn",           "/remoteInput/output/angleOn" } },
        { WFSParameterIDs::outputAngleOff,           { "/wfs/output/angleOff",          "/remoteInput/output/angleOff" } },
        { WFSParameterIDs::outputPitch,              { "/wfs/output/pitch",             "/remoteInput/output/pitch" } },
        { WFSParameterIDs::outputHFdamping,          { "/wfs/output/HFdamping",         "/remoteInput/output/HFdamping" } },

        // Options
        { WFSParameterIDs::outputMiniLatencyEnable,      { "/wfs/output/miniLatencyEnable",   "/remoteInput/output/miniLatencyEnable" } },
        { WFSParameterIDs::outputLSattenEnable,          { "/wfs/output/LSenable",            "/remoteInput/output/LSenable" } },
        { WFSParameterIDs::outputFRenable,               { "/wfs/output/FRenable",            "/remoteInput/output/FRenable" } },
        { WFSParameterIDs::outputDistanceAttenPercent,   { "/wfs/output/DistanceAttenPercent", "/remoteInput/output/DistanceAttenPercent" } },
        { WFSParameterIDs::outputHparallax,              { "/wfs/output/Hparallax",           "/remoteInput/output/Hparallax" } },
        { WFSParameterIDs::outputVparallax,              { "/wfs/output/Vparallax",           "/remoteInput/output/Vparallax" } },

        // EQ
        { WFSParameterIDs::outputEQenabled,          { "/wfs/output/EQenable",          "/remoteInput/output/EQenable" } },
        { WFSParameterIDs::eqShape,                  { "/wfs/output/EQshape",           "/remoteInput/output/EQshape" } },
        { WFSParameterIDs::eqFrequency,              { "/wfs/output/EQfreq",            "/remoteInput/output/EQfreq" } },
        { WFSParameterIDs::eqGain,                   { "/wfs/output/EQgain",            "/remoteInput/output/EQgain" } },
        { WFSParameterIDs::eqQ,                      { "/wfs/output/EQq",               "/remoteInput/output/EQq" } },
        { WFSParameterIDs::eqSlope,                  { "/wfs/output/EQslope",           "/remoteInput/output/EQslope" } },
    };

    return mappings;
}

const std::map<juce::Identifier, OSCMessageBuilder::ParamMapping>& OSCMessageBuilder::getReverbMappings()
{
    static const std::map<juce::Identifier, ParamMapping> mappings = {
        // Channel
        { WFSParameterIDs::reverbName,               { "/wfs/reverb/name",              "/remoteInput/reverb/name" } },
        { WFSParameterIDs::reverbAttenuation,        { "/wfs/reverb/attenuation",       "/remoteInput/reverb/attenuation" } },
        { WFSParameterIDs::reverbDelayLatency,       { "/wfs/reverb/delayLatency",      "/remoteInput/reverb/delayLatency" } },

        // Position
        { WFSParameterIDs::reverbPositionX,          { "/wfs/reverb/positionX",         "/remoteInput/reverb/positionX" } },
        { WFSParameterIDs::reverbPositionY,          { "/wfs/reverb/positionY",         "/remoteInput/reverb/positionY" } },
        { WFSParameterIDs::reverbPositionZ,          { "/wfs/reverb/positionZ",         "/remoteInput/reverb/positionZ" } },
        { WFSParameterIDs::reverbReturnOffsetX,      { "/wfs/reverb/returnOffsetX",     "/remoteInput/reverb/returnOffsetX" } },
        { WFSParameterIDs::reverbReturnOffsetY,      { "/wfs/reverb/returnOffsetY",     "/remoteInput/reverb/returnOffsetY" } },
        { WFSParameterIDs::reverbReturnOffsetZ,      { "/wfs/reverb/returnOffsetZ",     "/remoteInput/reverb/returnOffsetZ" } },

        // Feed
        { WFSParameterIDs::reverbOrientation,        { "/wfs/reverb/orientation",       "/remoteInput/reverb/orientation" } },
        { WFSParameterIDs::reverbAngleOn,            { "/wfs/reverb/angleOn",           "/remoteInput/reverb/angleOn" } },
        { WFSParameterIDs::reverbAngleOff,           { "/wfs/reverb/angleOff",          "/remoteInput/reverb/angleOff" } },
        { WFSParameterIDs::reverbPitch,              { "/wfs/reverb/pitch",             "/remoteInput/reverb/pitch" } },
        { WFSParameterIDs::reverbHFdamping,          { "/wfs/reverb/HFdamping",         "/remoteInput/reverb/HFdamping" } },
        { WFSParameterIDs::reverbMiniLatencyEnable,  { "/wfs/reverb/miniLatencyEnable", "/remoteInput/reverb/miniLatencyEnable" } },
        { WFSParameterIDs::reverbLSenable,           { "/wfs/reverb/LSenable",          "/remoteInput/reverb/LSenable" } },
        { WFSParameterIDs::reverbDistanceAttenEnable, { "/wfs/reverb/DistanceAttenPercent", "/remoteInput/reverb/DistanceAttenPercent" } },

        // EQ
        { WFSParameterIDs::reverbEQenable,           { "/wfs/reverb/EQenable",          "/remoteInput/reverb/EQenable" } },
        { WFSParameterIDs::reverbEQshape,            { "/wfs/reverb/EQshape",           "/remoteInput/reverb/EQshape" } },
        { WFSParameterIDs::reverbEQfreq,             { "/wfs/reverb/EQfreq",            "/remoteInput/reverb/EQfreq" } },
        { WFSParameterIDs::reverbEQgain,             { "/wfs/reverb/EQgain",            "/remoteInput/reverb/EQgain" } },
        { WFSParameterIDs::reverbEQq,                { "/wfs/reverb/EQq",               "/remoteInput/reverb/EQq" } },
        { WFSParameterIDs::reverbEQslope,            { "/wfs/reverb/EQslope",           "/remoteInput/reverb/EQslope" } },

        // Return
        { WFSParameterIDs::reverbDistanceAttenuation, { "/wfs/reverb/distanceAttenuation", "/remoteInput/reverb/distanceAttenuation" } },
        { WFSParameterIDs::reverbCommonAtten,        { "/wfs/reverb/commonAtten",       "/remoteInput/reverb/commonAtten" } },
        { WFSParameterIDs::reverbMutes,              { "/wfs/reverb/mutes",             "/remoteInput/reverb/mutes" } },
        { WFSParameterIDs::reverbMuteMacro,          { "/wfs/reverb/muteMacro",         "/remoteInput/reverb/muteMacro" } },
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

std::optional<juce::OSCMessage> OSCMessageBuilder::buildReverbMessage(
    const juce::Identifier& paramId,
    int channelId,
    float value)
{
    const auto& mappings = getReverbMappings();
    auto it = mappings.find(paramId);

    if (it == mappings.end())
        return std::nullopt;

    return buildMessage(it->second.oscPath, channelId, value);
}

//==============================================================================
// Message Building - Config Values (no channel ID)
//==============================================================================

std::optional<juce::OSCMessage> OSCMessageBuilder::buildConfigMessage(
    const juce::Identifier& paramId,
    float value)
{
    const auto& mappings = getConfigMappings();
    auto it = mappings.find(paramId);

    if (it == mappings.end())
        return std::nullopt;

    return buildConfigFloatMessage(it->second, value);
}

std::optional<juce::OSCMessage> OSCMessageBuilder::buildConfigMessage(
    const juce::Identifier& paramId,
    int value)
{
    const auto& mappings = getConfigMappings();
    auto it = mappings.find(paramId);

    if (it == mappings.end())
        return std::nullopt;

    return buildConfigIntMessage(it->second, value);
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

std::optional<juce::OSCMessage> OSCMessageBuilder::buildReverbStringMessage(
    const juce::Identifier& paramId,
    int channelId,
    const juce::String& value)
{
    const auto& mappings = getReverbMappings();
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

std::optional<juce::OSCMessage> OSCMessageBuilder::buildRemoteOutputIntMessage(
    const juce::Identifier& paramId,
    int channelId,
    int value)
{
    const auto& mappings = getInputMappings();
    auto it = mappings.find(paramId);

    if (it == mappings.end())
        return std::nullopt;

    return buildIntMessage(it->second.remotePath, channelId, value);
}

std::optional<juce::OSCMessage> OSCMessageBuilder::buildRemoteOutputStringMessage(
    const juce::Identifier& paramId,
    int channelId,
    const juce::String& value)
{
    const auto& mappings = getInputMappings();
    auto it = mappings.find(paramId);

    if (it == mappings.end())
        return std::nullopt;

    juce::OSCMessage msg(juce::OSCAddressPattern(it->second.remotePath));
    msg.addInt32(channelId);
    msg.addString(value);
    return msg;
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

std::vector<juce::OSCMessage> OSCMessageBuilder::buildRemoteChannelDump(
    int channelId,
    const std::map<juce::Identifier, float>& floatParamValues,
    const std::map<juce::Identifier, int>& intParamValues)
{
    std::vector<juce::OSCMessage> messages;
    messages.reserve(floatParamValues.size() + intParamValues.size());

    const auto& mappings = getInputMappings();

    for (const auto& [paramId, value] : floatParamValues)
    {
        auto it = mappings.find(paramId);
        if (it != mappings.end())
        {
            messages.push_back(buildMessage(it->second.remotePath, channelId, value));
        }
    }

    for (const auto& [paramId, value] : intParamValues)
    {
        auto it = mappings.find(paramId);
        if (it != mappings.end())
        {
            messages.push_back(buildIntMessage(it->second.remotePath, channelId, value));
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

juce::String OSCMessageBuilder::getConfigOSCPath(const juce::Identifier& paramId)
{
    const auto& mappings = getConfigMappings();
    auto it = mappings.find(paramId);
    return (it != mappings.end()) ? it->second : juce::String();
}

juce::String OSCMessageBuilder::getReverbOSCPath(const juce::Identifier& paramId)
{
    const auto& mappings = getReverbMappings();
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

bool OSCMessageBuilder::isReverbMapped(const juce::Identifier& paramId)
{
    return getReverbMappings().find(paramId) != getReverbMappings().end();
}

bool OSCMessageBuilder::isConfigMapped(const juce::Identifier& paramId)
{
    return getConfigMappings().find(paramId) != getConfigMappings().end();
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

juce::OSCMessage OSCMessageBuilder::buildIntMessage(
    const juce::String& address,
    int channelId,
    int value)
{
    juce::OSCMessage msg(address);
    msg.addInt32(channelId);
    msg.addInt32(value);
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

juce::OSCMessage OSCMessageBuilder::buildConfigFloatMessage(
    const juce::String& address,
    float value)
{
    juce::OSCMessage msg(address);
    msg.addFloat32(value);
    return msg;
}

juce::OSCMessage OSCMessageBuilder::buildConfigIntMessage(
    const juce::String& address,
    int value)
{
    juce::OSCMessage msg(address);
    msg.addInt32(value);
    return msg;
}

} // namespace WFSNetwork
