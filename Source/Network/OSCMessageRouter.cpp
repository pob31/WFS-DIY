#include "OSCMessageRouter.h"

namespace WFSNetwork
{

//==============================================================================
// Static Lookup Tables (Address -> Parameter ID)
//==============================================================================

const std::map<juce::String, juce::Identifier>& OSCMessageRouter::getInputAddressMap()
{
    static const std::map<juce::String, juce::Identifier> addressMap = {
        // Channel
        { "name",             WFSParameterIDs::inputName },
        { "attenuation",      WFSParameterIDs::inputAttenuation },
        { "delayLatency",     WFSParameterIDs::inputDelayLatency },
        { "minimalLatency",   WFSParameterIDs::inputMinimalLatency },

        // Position
        { "positionX",        WFSParameterIDs::inputPositionX },
        { "positionY",        WFSParameterIDs::inputPositionY },
        { "positionZ",        WFSParameterIDs::inputPositionZ },
        { "offsetX",          WFSParameterIDs::inputOffsetX },
        { "offsetY",          WFSParameterIDs::inputOffsetY },
        { "offsetZ",          WFSParameterIDs::inputOffsetZ },
        { "constraintX",      WFSParameterIDs::inputConstraintX },
        { "constraintY",      WFSParameterIDs::inputConstraintY },
        { "constraintZ",      WFSParameterIDs::inputConstraintZ },
        { "constraintDistance",    WFSParameterIDs::inputConstraintDistance },
        { "constraintDistanceMin", WFSParameterIDs::inputConstraintDistanceMin },
        { "constraintDistanceMax", WFSParameterIDs::inputConstraintDistanceMax },
        { "flipX",            WFSParameterIDs::inputFlipX },
        { "flipY",            WFSParameterIDs::inputFlipY },
        { "flipZ",            WFSParameterIDs::inputFlipZ },
        { "cluster",          WFSParameterIDs::inputCluster },
        { "trackingActive",   WFSParameterIDs::inputTrackingActive },
        { "trackingID",       WFSParameterIDs::inputTrackingID },
        { "trackingSmooth",   WFSParameterIDs::inputTrackingSmooth },
        { "maxSpeedActive",   WFSParameterIDs::inputMaxSpeedActive },
        { "maxSpeed",         WFSParameterIDs::inputMaxSpeed },
        { "pathModeActive",   WFSParameterIDs::inputPathModeActive },
        { "heightFactor",     WFSParameterIDs::inputHeightFactor },

        // Attenuation
        { "attenuationLaw",       WFSParameterIDs::inputAttenuationLaw },
        { "distanceAttenuation",  WFSParameterIDs::inputDistanceAttenuation },
        { "distanceRatio",        WFSParameterIDs::inputDistanceRatio },
        { "commonAtten",          WFSParameterIDs::inputCommonAtten },

        // Directivity
        { "directivity",     WFSParameterIDs::inputDirectivity },
        { "rotation",        WFSParameterIDs::inputRotation },
        { "tilt",            WFSParameterIDs::inputTilt },
        { "HFshelf",         WFSParameterIDs::inputHFshelf },

        // Live Source Tamer
        { "LSenable",         WFSParameterIDs::inputLSactive },
        { "LSradius",         WFSParameterIDs::inputLSradius },
        { "LSshape",          WFSParameterIDs::inputLSshape },
        { "LSattenuation",    WFSParameterIDs::inputLSattenuation },
        { "LSpeakThreshold",  WFSParameterIDs::inputLSpeakThreshold },
        { "LSpeakRatio",      WFSParameterIDs::inputLSpeakRatio },
        { "LSslowThreshold",  WFSParameterIDs::inputLSslowThreshold },
        { "LSslowRatio",      WFSParameterIDs::inputLSslowRatio },

        // Hackoustics (Floor Reflections)
        { "FRenable",             WFSParameterIDs::inputFRactive },
        { "FRattenuation",        WFSParameterIDs::inputFRattenuation },
        { "FRlowCutActive",       WFSParameterIDs::inputFRlowCutActive },
        { "FRlowCutFreq",         WFSParameterIDs::inputFRlowCutFreq },
        { "FRhighShelfActive",    WFSParameterIDs::inputFRhighShelfActive },
        { "FRhighShelfFreq",      WFSParameterIDs::inputFRhighShelfFreq },
        { "FRhighShelfGain",      WFSParameterIDs::inputFRhighShelfGain },
        { "FRhighShelfSlope",     WFSParameterIDs::inputFRhighShelfSlope },
        { "FRdiffusion",          WFSParameterIDs::inputFRdiffusion },
        { "muteReverbSends",      WFSParameterIDs::inputMuteReverbSends },

        // Jitter
        { "jitter",          WFSParameterIDs::inputJitter },

        // LFO
        { "LFOenable",        WFSParameterIDs::inputLFOactive },
        { "LFOperiod",        WFSParameterIDs::inputLFOperiod },
        { "LFOphase",         WFSParameterIDs::inputLFOphase },
        { "LFOshapeX",        WFSParameterIDs::inputLFOshapeX },
        { "LFOshapeY",        WFSParameterIDs::inputLFOshapeY },
        { "LFOshapeZ",        WFSParameterIDs::inputLFOshapeZ },
        { "LFOrateX",         WFSParameterIDs::inputLFOrateX },
        { "LFOrateY",         WFSParameterIDs::inputLFOrateY },
        { "LFOrateZ",         WFSParameterIDs::inputLFOrateZ },
        { "LFOamplitudeX",    WFSParameterIDs::inputLFOamplitudeX },
        { "LFOamplitudeY",    WFSParameterIDs::inputLFOamplitudeY },
        { "LFOamplitudeZ",    WFSParameterIDs::inputLFOamplitudeZ },
        { "LFOphaseX",        WFSParameterIDs::inputLFOphaseX },
        { "LFOphaseY",        WFSParameterIDs::inputLFOphaseY },
        { "LFOphaseZ",        WFSParameterIDs::inputLFOphaseZ },
        { "LFOgyrophone",     WFSParameterIDs::inputLFOgyrophone },

        // AutomOtion
        { "otomoX",                  WFSParameterIDs::inputOtomoX },
        { "otomoY",                  WFSParameterIDs::inputOtomoY },
        { "otomoZ",                  WFSParameterIDs::inputOtomoZ },
        { "otomoAbsoluteRelative",   WFSParameterIDs::inputOtomoAbsoluteRelative },
        { "otomoStayReturn",         WFSParameterIDs::inputOtomoStayReturn },
        { "otomoDuration",           WFSParameterIDs::inputOtomoDuration },
        { "otomoCurve",              WFSParameterIDs::inputOtomoCurve },
        { "otomoSpeed",              WFSParameterIDs::inputOtomoSpeedProfile },
        { "otomoTrigger",            WFSParameterIDs::inputOtomoTrigger },
        { "otomoTriggerThreshold",   WFSParameterIDs::inputOtomoThreshold },
        { "otomoTriggerReset",       WFSParameterIDs::inputOtomoReset },
        { "otomoPauseResume",        WFSParameterIDs::inputOtomoPauseResume },

        // AutomOtion (Polar coordinates)
        { "otomoCoordinateMode",     WFSParameterIDs::inputOtomoCoordinateMode },
        { "otomoR",                  WFSParameterIDs::inputOtomoR },
        { "otomoTheta",              WFSParameterIDs::inputOtomoTheta },
        { "otomoRsph",               WFSParameterIDs::inputOtomoRsph },
        { "otomoPhi",                WFSParameterIDs::inputOtomoPhi },

        // Mutes
        { "mutes",           WFSParameterIDs::inputMutes },
        { "muteMacro",       WFSParameterIDs::inputMuteMacro },

        // Sidelines
        { "sidelinesEnable", WFSParameterIDs::inputSidelinesActive },
        { "sidelinesFringe", WFSParameterIDs::inputSidelinesFringe },

        // Reverb
        { "reverbSend",      WFSParameterIDs::inputReverbSend },

        // Sampler
        { "samplerActive",   WFSParameterIDs::inputSamplerActive },
        { "samplerSet",      WFSParameterIDs::inputSamplerActiveSet },

        // Gradient Map layer enable — per-layer addresses: <channelID> <0|1>
        { "gmLayer0Enabled", WFSParameterIDs::gmLayer0Enabled },
        { "gmLayer1Enabled", WFSParameterIDs::gmLayer1Enabled },
        { "gmLayer2Enabled", WFSParameterIDs::gmLayer2Enabled },
    };

    return addressMap;
}

const std::map<juce::String, juce::Identifier>& OSCMessageRouter::getOutputAddressMap()
{
    static const std::map<juce::String, juce::Identifier> addressMap = {
        // Channel
        { "name",               WFSParameterIDs::outputName },
        { "array",              WFSParameterIDs::outputArray },
        { "applyToArray",       WFSParameterIDs::outputApplyToArray },
        { "attenuation",        WFSParameterIDs::outputAttenuation },
        { "delayLatency",       WFSParameterIDs::outputDelayLatency },

        // Position
        { "positionX",          WFSParameterIDs::outputPositionX },
        { "positionY",          WFSParameterIDs::outputPositionY },
        { "positionZ",          WFSParameterIDs::outputPositionZ },
        { "orientation",        WFSParameterIDs::outputOrientation },
        { "angleOn",            WFSParameterIDs::outputAngleOn },
        { "angleOff",           WFSParameterIDs::outputAngleOff },
        { "pitch",              WFSParameterIDs::outputPitch },
        { "HFdamping",          WFSParameterIDs::outputHFdamping },

        // Options
        { "miniLatencyEnable",      WFSParameterIDs::outputMiniLatencyEnable },
        { "LSenable",               WFSParameterIDs::outputLSattenEnable },
        { "FRenable",               WFSParameterIDs::outputFRenable },
        { "DistanceAttenPercent",   WFSParameterIDs::outputDistanceAttenPercent },
        { "Hparallax",              WFSParameterIDs::outputHparallax },
        { "Vparallax",              WFSParameterIDs::outputVparallax },

        // EQ
        { "EQenable",           WFSParameterIDs::outputEQenabled },
        { "EQshape",            WFSParameterIDs::eqShape },
        { "EQfreq",             WFSParameterIDs::eqFrequency },
        { "EQgain",             WFSParameterIDs::eqGain },
        { "EQq",                WFSParameterIDs::eqQ },
        { "EQslope",            WFSParameterIDs::eqSlope },
    };

    return addressMap;
}

const std::map<juce::String, juce::Identifier>& OSCMessageRouter::getReverbAddressMap()
{
    static const std::map<juce::String, juce::Identifier> addressMap = {
        // Channel
        { "name",               WFSParameterIDs::reverbName },
        { "attenuation",        WFSParameterIDs::reverbAttenuation },
        { "delayLatency",       WFSParameterIDs::reverbDelayLatency },

        // Position
        { "positionX",          WFSParameterIDs::reverbPositionX },
        { "positionY",          WFSParameterIDs::reverbPositionY },
        { "positionZ",          WFSParameterIDs::reverbPositionZ },
        { "returnOffsetX",      WFSParameterIDs::reverbReturnOffsetX },
        { "returnOffsetY",      WFSParameterIDs::reverbReturnOffsetY },
        { "returnOffsetZ",      WFSParameterIDs::reverbReturnOffsetZ },

        // Feed
        { "orientation",        WFSParameterIDs::reverbOrientation },
        { "angleOn",            WFSParameterIDs::reverbAngleOn },
        { "angleOff",           WFSParameterIDs::reverbAngleOff },
        { "pitch",              WFSParameterIDs::reverbPitch },
        { "HFdamping",          WFSParameterIDs::reverbHFdamping },
        { "miniLatencyEnable",  WFSParameterIDs::reverbMiniLatencyEnable },
        { "LSenable",           WFSParameterIDs::reverbLSenable },
        { "DistanceAttenPercent", WFSParameterIDs::reverbDistanceAttenEnable },

        // Pre-Processing EQ
        { "preEQenable",        WFSParameterIDs::reverbPreEQenable },
        { "preEQshape",         WFSParameterIDs::reverbPreEQshape },
        { "preEQfreq",          WFSParameterIDs::reverbPreEQfreq },
        { "preEQgain",          WFSParameterIDs::reverbPreEQgain },
        { "preEQq",             WFSParameterIDs::reverbPreEQq },
        { "preEQslope",         WFSParameterIDs::reverbPreEQslope },

        // Return
        { "distanceAttenuation", WFSParameterIDs::reverbDistanceAttenuation },
        { "commonAtten",        WFSParameterIDs::reverbCommonAtten },
        { "mutes",              WFSParameterIDs::reverbMutes },
        { "muteMacro",          WFSParameterIDs::reverbMuteMacro },
    };

    return addressMap;
}

const std::map<juce::String, juce::Identifier>& OSCMessageRouter::getRemoteAddressMap()
{
    // Remote protocol address names -> parameter IDs
    // Used for /remoteInput/* addresses from Android app
    static const std::map<juce::String, juce::Identifier> addressMap = {
        // Channel
        { "inputName",        WFSParameterIDs::inputName },
        { "attenuation",      WFSParameterIDs::inputAttenuation },
        { "delayLatency",     WFSParameterIDs::inputDelayLatency },
        { "minimalLatency",   WFSParameterIDs::inputMinimalLatency },

        // Position
        { "positionX",        WFSParameterIDs::inputPositionX },
        { "positionY",        WFSParameterIDs::inputPositionY },
        { "positionZ",        WFSParameterIDs::inputPositionZ },
        { "offsetX",          WFSParameterIDs::inputOffsetX },
        { "offsetY",          WFSParameterIDs::inputOffsetY },
        { "offsetZ",          WFSParameterIDs::inputOffsetZ },
        { "cluster",          WFSParameterIDs::inputCluster },
        { "maxSpeedActive",   WFSParameterIDs::inputMaxSpeedActive },
        { "maxSpeed",         WFSParameterIDs::inputMaxSpeed },
        { "pathModeActive",   WFSParameterIDs::inputPathModeActive },
        { "heightFactor",     WFSParameterIDs::inputHeightFactor },

        // Attenuation
        { "attenuationLaw",       WFSParameterIDs::inputAttenuationLaw },
        { "distanceAttenuation",  WFSParameterIDs::inputDistanceAttenuation },
        { "distanceRatio",        WFSParameterIDs::inputDistanceRatio },
        { "commonAtten",          WFSParameterIDs::inputCommonAtten },

        // Directivity
        { "directivity",      WFSParameterIDs::inputDirectivity },
        { "rotation",         WFSParameterIDs::inputRotation },
        { "tilt",             WFSParameterIDs::inputTilt },
        { "HFshelf",          WFSParameterIDs::inputHFshelf },

        // Live Source Tamer
        { "liveSourceActive",         WFSParameterIDs::inputLSactive },
        { "liveSourceRadius",         WFSParameterIDs::inputLSradius },
        { "liveSourceShape",          WFSParameterIDs::inputLSshape },
        { "liveSourceAttenuation",    WFSParameterIDs::inputLSattenuation },
        { "liveSourcePeakEnable",     WFSParameterIDs::inputLSpeakEnable },
        { "liveSourcePeakThreshold",  WFSParameterIDs::inputLSpeakThreshold },
        { "liveSourcePeakRatio",      WFSParameterIDs::inputLSpeakRatio },
        { "liveSourceSlowEnable",     WFSParameterIDs::inputLSslowEnable },
        { "liveSourceSlowThreshold",  WFSParameterIDs::inputLSslowThreshold },
        { "liveSourceSlowRatio",      WFSParameterIDs::inputLSslowRatio },

        // Hackoustics (Floor Reflections)
        { "FRactive",             WFSParameterIDs::inputFRactive },
        { "FRattenuation",        WFSParameterIDs::inputFRattenuation },
        { "FRlowCutActive",       WFSParameterIDs::inputFRlowCutActive },
        { "FRlowCutFreq",         WFSParameterIDs::inputFRlowCutFreq },
        { "FRhighShelfActive",    WFSParameterIDs::inputFRhighShelfActive },
        { "FRhighShelfFreq",      WFSParameterIDs::inputFRhighShelfFreq },
        { "FRhighShelfGain",      WFSParameterIDs::inputFRhighShelfGain },
        { "FRhighShelfSlope",     WFSParameterIDs::inputFRhighShelfSlope },
        { "FRdiffusion",          WFSParameterIDs::inputFRdiffusion },

        // Jitter
        { "jitter",               WFSParameterIDs::inputJitter },

        // LFO
        { "LFOactive",        WFSParameterIDs::inputLFOactive },
        { "LFOperiod",        WFSParameterIDs::inputLFOperiod },
        { "LFOphase",         WFSParameterIDs::inputLFOphase },
        { "LFOshapeX",        WFSParameterIDs::inputLFOshapeX },
        { "LFOshapeY",        WFSParameterIDs::inputLFOshapeY },
        { "LFOshapeZ",        WFSParameterIDs::inputLFOshapeZ },
        { "LFOrateX",         WFSParameterIDs::inputLFOrateX },
        { "LFOrateY",         WFSParameterIDs::inputLFOrateY },
        { "LFOrateZ",         WFSParameterIDs::inputLFOrateZ },
        { "LFOamplitudeX",    WFSParameterIDs::inputLFOamplitudeX },
        { "LFOamplitudeY",    WFSParameterIDs::inputLFOamplitudeY },
        { "LFOamplitudeZ",    WFSParameterIDs::inputLFOamplitudeZ },
        { "LFOphaseX",        WFSParameterIDs::inputLFOphaseX },
        { "LFOphaseY",        WFSParameterIDs::inputLFOphaseY },
        { "LFOphaseZ",        WFSParameterIDs::inputLFOphaseZ },
        { "LFOgyrophone",     WFSParameterIDs::inputLFOgyrophone },

        // Tracking
        { "trackingActive",   WFSParameterIDs::inputTrackingActive },
        { "trackingID",       WFSParameterIDs::inputTrackingID },
        { "trackingSmooth",   WFSParameterIDs::inputTrackingSmooth },

        // Sidelines
        { "sidelinesActive",  WFSParameterIDs::inputSidelinesActive },
        { "sidelinesFringe",  WFSParameterIDs::inputSidelinesFringe },
    };

    return addressMap;
}

const std::map<juce::String, juce::Identifier>& OSCMessageRouter::getConfigAddressMap()
{
    // Config parameter addresses -> parameter IDs
    // Note: These use full paths (not just the param name) because config paths have subpaths
    static const std::map<juce::String, juce::Identifier> addressMap = {
        // Stage parameters
        { OSCPaths::CONFIG_STAGE_SHAPE,          WFSParameterIDs::stageShape },
        { OSCPaths::CONFIG_STAGE_WIDTH,          WFSParameterIDs::stageWidth },
        { OSCPaths::CONFIG_STAGE_DEPTH,          WFSParameterIDs::stageDepth },
        { OSCPaths::CONFIG_STAGE_HEIGHT,         WFSParameterIDs::stageHeight },
        { OSCPaths::CONFIG_STAGE_DIAMETER,       WFSParameterIDs::stageDiameter },
        { OSCPaths::CONFIG_STAGE_DOME_ELEVATION, WFSParameterIDs::domeElevation },
        { OSCPaths::CONFIG_STAGE_ORIGIN_X,       WFSParameterIDs::originWidth },
        { OSCPaths::CONFIG_STAGE_ORIGIN_Y,       WFSParameterIDs::originDepth },
        { OSCPaths::CONFIG_STAGE_ORIGIN_Z,       WFSParameterIDs::originHeight },

        // Reverb Algorithm parameters (global)
        { OSCPaths::CONFIG_REVERB_ALGO_TYPE,      WFSParameterIDs::reverbAlgoType },
        { OSCPaths::CONFIG_REVERB_RT60,            WFSParameterIDs::reverbRT60 },
        { OSCPaths::CONFIG_REVERB_RT60_LOW_MULT,   WFSParameterIDs::reverbRT60LowMult },
        { OSCPaths::CONFIG_REVERB_RT60_HIGH_MULT,  WFSParameterIDs::reverbRT60HighMult },
        { OSCPaths::CONFIG_REVERB_CROSSOVER_LOW,   WFSParameterIDs::reverbCrossoverLow },
        { OSCPaths::CONFIG_REVERB_CROSSOVER_HIGH,  WFSParameterIDs::reverbCrossoverHigh },
        { OSCPaths::CONFIG_REVERB_DIFFUSION,       WFSParameterIDs::reverbDiffusion },
        { OSCPaths::CONFIG_REVERB_SDN_SCALE,       WFSParameterIDs::reverbSDNscale },
        { OSCPaths::CONFIG_REVERB_FDN_SIZE,        WFSParameterIDs::reverbFDNsize },
        { OSCPaths::CONFIG_REVERB_IR_TRIM,         WFSParameterIDs::reverbIRtrim },
        { OSCPaths::CONFIG_REVERB_IR_LENGTH,       WFSParameterIDs::reverbIRlength },
        { OSCPaths::CONFIG_REVERB_PER_NODE_IR,     WFSParameterIDs::reverbPerNodeIR },
        { OSCPaths::CONFIG_REVERB_WET_LEVEL,       WFSParameterIDs::reverbWetLevel },

        // Reverb Pre-Compressor parameters (global)
        { OSCPaths::CONFIG_REVERB_PRE_COMP_BYPASS,     WFSParameterIDs::reverbPreCompBypass },
        { OSCPaths::CONFIG_REVERB_PRE_COMP_THRESHOLD,  WFSParameterIDs::reverbPreCompThreshold },
        { OSCPaths::CONFIG_REVERB_PRE_COMP_RATIO,      WFSParameterIDs::reverbPreCompRatio },
        { OSCPaths::CONFIG_REVERB_PRE_COMP_ATTACK,     WFSParameterIDs::reverbPreCompAttack },
        { OSCPaths::CONFIG_REVERB_PRE_COMP_RELEASE,    WFSParameterIDs::reverbPreCompRelease },

        // Reverb Post-Processing EQ parameters (global)
        { OSCPaths::CONFIG_REVERB_POST_EQ_ENABLE,  WFSParameterIDs::reverbPostEQenable },
        { OSCPaths::CONFIG_REVERB_POST_EQ_SHAPE,   WFSParameterIDs::reverbPostEQshape },
        { OSCPaths::CONFIG_REVERB_POST_EQ_FREQ,    WFSParameterIDs::reverbPostEQfreq },
        { OSCPaths::CONFIG_REVERB_POST_EQ_GAIN,    WFSParameterIDs::reverbPostEQgain },
        { OSCPaths::CONFIG_REVERB_POST_EQ_Q,       WFSParameterIDs::reverbPostEQq },
        { OSCPaths::CONFIG_REVERB_POST_EQ_SLOPE,   WFSParameterIDs::reverbPostEQslope },

        // Reverb Post-Expander parameters (global)
        { OSCPaths::CONFIG_REVERB_POST_EXP_BYPASS,     WFSParameterIDs::reverbPostExpBypass },
        { OSCPaths::CONFIG_REVERB_POST_EXP_THRESHOLD,  WFSParameterIDs::reverbPostExpThreshold },
        { OSCPaths::CONFIG_REVERB_POST_EXP_RATIO,      WFSParameterIDs::reverbPostExpRatio },
        { OSCPaths::CONFIG_REVERB_POST_EXP_ATTACK,     WFSParameterIDs::reverbPostExpAttack },
        { OSCPaths::CONFIG_REVERB_POST_EXP_RELEASE,    WFSParameterIDs::reverbPostExpRelease },
    };

    return addressMap;
}

//==============================================================================
// Address Pattern Matching
//==============================================================================

bool OSCMessageRouter::isInputAddress(const juce::String& address)
{
    return address.startsWith("/wfs/input/");
}

bool OSCMessageRouter::isOutputAddress(const juce::String& address)
{
    return address.startsWith("/wfs/output/");
}

bool OSCMessageRouter::isReverbAddress(const juce::String& address)
{
    return address.startsWith("/wfs/reverb/");
}

bool OSCMessageRouter::isConfigAddress(const juce::String& address)
{
    return address.startsWith("/wfs/config/");
}

bool OSCMessageRouter::isRemoteInputAddress(const juce::String& address)
{
    return address.startsWith("/remoteInput/");
}

bool OSCMessageRouter::isArrayAdjustAddress(const juce::String& address)
{
    return address.startsWith("/arrayAdjust/");
}

bool OSCMessageRouter::isClusterMoveAddress(const juce::String& address)
{
    return address == "/cluster/move" || address == "/cluster/barycenter/move"
        || address == "/cluster/positionXY";
}

bool OSCMessageRouter::isClusterScaleRotationAddress(const juce::String& address)
{
    return address == "/cluster/scale" || address == "/cluster/rotation";
}

bool OSCMessageRouter::isClusterCumulativeScaleRotationAddress(const juce::String& address)
{
    return address == "/cluster/scaleRotation";
}

bool OSCMessageRouter::isClusterLFOAddress(const juce::String& address)
{
    return address.startsWith("/wfs/cluster/lfo");
}

juce::String OSCMessageRouter::extractParamName(const juce::String& address)
{
    // Extract the last part of the path
    // e.g., "/wfs/input/attenuation" -> "attenuation"
    int lastSlash = address.lastIndexOf("/");
    if (lastSlash >= 0)
        return address.substring(lastSlash + 1);
    return address;
}

juce::Identifier OSCMessageRouter::getInputParamId(const juce::String& address)
{
    juce::String paramName = extractParamName(address);
    const auto& addressMap = getInputAddressMap();

    auto it = addressMap.find(paramName);
    if (it != addressMap.end())
        return it->second;

    return {};
}

juce::Identifier OSCMessageRouter::getOutputParamId(const juce::String& address)
{
    juce::String paramName = extractParamName(address);
    const auto& addressMap = getOutputAddressMap();

    auto it = addressMap.find(paramName);
    if (it != addressMap.end())
        return it->second;

    return {};
}

juce::Identifier OSCMessageRouter::getReverbParamId(const juce::String& address)
{
    juce::String paramName = extractParamName(address);
    const auto& addressMap = getReverbAddressMap();

    auto it = addressMap.find(paramName);
    if (it != addressMap.end())
        return it->second;

    return {};
}

juce::Identifier OSCMessageRouter::getConfigParamId(const juce::String& address)
{
    // Config addresses use full paths, not just the param name
    const auto& addressMap = getConfigAddressMap();

    auto it = addressMap.find(address);
    if (it != addressMap.end())
        return it->second;

    return {};
}

//==============================================================================
// Value Extraction
//==============================================================================

float OSCMessageRouter::extractFloat(const juce::OSCArgument& arg)
{
    if (arg.isFloat32())
        return arg.getFloat32();
    if (arg.isInt32())
        return static_cast<float>(arg.getInt32());
    return 0.0f;
}

int OSCMessageRouter::extractInt(const juce::OSCArgument& arg)
{
    if (arg.isInt32())
        return arg.getInt32();
    if (arg.isFloat32())
        return static_cast<int>(arg.getFloat32());
    return 0;
}

juce::String OSCMessageRouter::extractString(const juce::OSCArgument& arg)
{
    if (arg.isString())
        return arg.getString();
    return {};
}

//==============================================================================
// Message Parsing
//==============================================================================

OSCMessageRouter::ParsedInputMessage OSCMessageRouter::parseInputMessage(const juce::OSCMessage& message)
{
    ParsedInputMessage result;

    juce::String address = message.getAddressPattern().toString();

    if (!isInputAddress(address))
        return result;

    result.paramId = getInputParamId(address);

    if (result.paramId.isValid())
    {
        // Standard format: /wfs/input/{param} <channelID> <value>
        if (message.size() < 2)
            return result;

        result.channelId = extractInt(message[0]);

        if (message[1].isString())
            result.value = extractString(message[1]);
        else
            result.value = extractFloat(message[1]);

        result.valid = true;
    }
    else
    {
        // Try OSCQuery format: /wfs/input/{channelID}/{param} <value>
        // e.g. /wfs/input/1/attenuation -5.0
        juce::String suffix = address.fromFirstOccurrenceOf("/wfs/input/", false, true);
        int slashIdx = suffix.indexOf("/");
        if (slashIdx > 0)
        {
            juce::String channelStr = suffix.substring(0, slashIdx);
            juce::String paramName = suffix.substring(slashIdx + 1);

            if (channelStr.containsOnly("0123456789") && paramName.isNotEmpty())
            {
                const auto& addrMap = getInputAddressMap();
                auto it = addrMap.find(paramName);
                if (it != addrMap.end())
                {
                    result.paramId = it->second;
                    result.channelId = channelStr.getIntValue();

                    if (message.size() >= 1)
                    {
                        if (message[0].isString())
                            result.value = extractString(message[0]);
                        else
                            result.value = extractFloat(message[0]);

                        result.valid = true;
                    }
                }
            }
        }
    }

    return result;
}

OSCMessageRouter::ParsedOutputMessage OSCMessageRouter::parseOutputMessage(const juce::OSCMessage& message)
{
    ParsedOutputMessage result;

    juce::String address = message.getAddressPattern().toString();

    if (!isOutputAddress(address))
        return result;

    result.paramId = getOutputParamId(address);

    if (result.paramId.isValid())
    {
        juce::String paramName = extractParamName(address);

        bool isEQParam = paramName.startsWith("EQ") && paramName != "EQenable";

        if (isEQParam)
        {
            if (message.size() < 3)
                return result;

            result.isEQparam = true;
            result.channelId = extractInt(message[0]);
            result.bandIndex = extractInt(message[1]);

            if (message[2].isString())
                result.value = extractString(message[2]);
            else
                result.value = extractFloat(message[2]);
        }
        else
        {
            if (message.size() < 2)
                return result;

            result.channelId = extractInt(message[0]);

            if (message[1].isString())
                result.value = extractString(message[1]);
            else
                result.value = extractFloat(message[1]);
        }

        result.valid = true;
    }
    else
    {
        // Try OSCQuery format: /wfs/output/{channelID}/{param} <value>
        juce::String suffix = address.fromFirstOccurrenceOf("/wfs/output/", false, true);
        int slashIdx = suffix.indexOf("/");
        if (slashIdx > 0)
        {
            juce::String channelStr = suffix.substring(0, slashIdx);
            juce::String paramName = suffix.substring(slashIdx + 1);

            if (channelStr.containsOnly("0123456789") && paramName.isNotEmpty())
            {
                const auto& addrMap = getOutputAddressMap();
                auto it = addrMap.find(paramName);
                if (it != addrMap.end())
                {
                    result.paramId = it->second;
                    result.channelId = channelStr.getIntValue();

                    bool isEQParam = paramName.startsWith("EQ") && paramName != "EQenable";
                    if (isEQParam)
                    {
                        // OSCQuery EQ format: /wfs/output/N/EQfreq <bandIndex> <value>
                        if (message.size() >= 2)
                        {
                            result.isEQparam = true;
                            result.bandIndex = extractInt(message[0]);
                            if (message[1].isString())
                                result.value = extractString(message[1]);
                            else
                                result.value = extractFloat(message[1]);
                            result.valid = true;
                        }
                    }
                    else
                    {
                        if (message.size() >= 1)
                        {
                            if (message[0].isString())
                                result.value = extractString(message[0]);
                            else
                                result.value = extractFloat(message[0]);
                            result.valid = true;
                        }
                    }
                }
            }
        }
    }

    return result;
}

OSCMessageRouter::ParsedReverbMessage OSCMessageRouter::parseReverbMessage(const juce::OSCMessage& message)
{
    ParsedReverbMessage result;

    juce::String address = message.getAddressPattern().toString();

    if (!isReverbAddress(address))
        return result;

    result.paramId = getReverbParamId(address);

    if (result.paramId.isValid())
    {
        juce::String paramName = extractParamName(address);

        bool isEQParam = paramName.startsWith("EQ") && paramName != "EQenable";

        if (isEQParam)
        {
            if (message.size() < 3)
                return result;

            result.isEQparam = true;
            result.channelId = extractInt(message[0]);
            result.bandIndex = extractInt(message[1]);

            if (message[2].isString())
                result.value = extractString(message[2]);
            else
                result.value = extractFloat(message[2]);
        }
        else
        {
            if (message.size() < 2)
                return result;

            result.channelId = extractInt(message[0]);

            if (message[1].isString())
                result.value = extractString(message[1]);
            else
                result.value = extractFloat(message[1]);
        }

        result.valid = true;
    }
    else
    {
        // Try OSCQuery format: /wfs/reverb/{channelID}/{param} <value>
        juce::String suffix = address.fromFirstOccurrenceOf("/wfs/reverb/", false, true);
        int slashIdx = suffix.indexOf("/");
        if (slashIdx > 0)
        {
            juce::String channelStr = suffix.substring(0, slashIdx);
            juce::String paramName = suffix.substring(slashIdx + 1);

            if (channelStr.containsOnly("0123456789") && paramName.isNotEmpty())
            {
                const auto& addrMap = getReverbAddressMap();
                auto it = addrMap.find(paramName);
                if (it != addrMap.end())
                {
                    result.paramId = it->second;
                    result.channelId = channelStr.getIntValue();

                    bool isEQParam = paramName.startsWith("preEQ") && paramName != "preEQenable";
                    if (isEQParam)
                    {
                        if (message.size() >= 2)
                        {
                            result.isEQparam = true;
                            result.bandIndex = extractInt(message[0]);
                            if (message[1].isString())
                                result.value = extractString(message[1]);
                            else
                                result.value = extractFloat(message[1]);
                            result.valid = true;
                        }
                    }
                    else
                    {
                        if (message.size() >= 1)
                        {
                            if (message[0].isString())
                                result.value = extractString(message[0]);
                            else
                                result.value = extractFloat(message[0]);
                            result.valid = true;
                        }
                    }
                }
            }
        }
    }

    return result;
}

OSCMessageRouter::ParsedConfigMessage OSCMessageRouter::parseConfigMessage(const juce::OSCMessage& message)
{
    ParsedConfigMessage result;

    juce::String address = message.getAddressPattern().toString();

    if (!isConfigAddress(address))
        return result;

    result.paramId = getConfigParamId(address);
    if (!result.paramId.isValid())
        return result;

    // Config messages have format: /wfs/config/... <value>
    // No channel ID - just a single value
    if (message.size() < 1)
        return result;

    // Determine value type based on argument
    if (message[0].isInt32())
        result.value = extractInt(message[0]);
    else if (message[0].isFloat32())
        result.value = extractFloat(message[0]);
    else if (message[0].isString())
        result.value = extractString(message[0]);
    else
        return result;

    result.valid = true;
    return result;
}

OSCMessageRouter::ParsedRemoteInput OSCMessageRouter::parseRemoteInputMessage(const juce::OSCMessage& message)
{
    ParsedRemoteInput result;

    juce::String address = message.getAddressPattern().toString();

    if (!isRemoteInputAddress(address))
        return result;

    juce::String paramName = extractParamName(address);

    // Handle channel selection: /remoteInput/inputNumber <ID>
    if (paramName == "inputNumber")
    {
        if (message.size() < 1)
            return result;

        result.type = ParsedRemoteInput::Type::ChannelSelect;
        result.channelId = extractInt(message[0]);
        result.valid = true;
        return result;
    }

    // Handle combined XY position: /remoteInput/positionXY <ID> <posX> <posY>
    if (paramName == "positionXY")
    {
        // Need 3 args: inputId (int), posX (float), posY (float)
        if (message.size() < 3)
            return result;

        result.type = ParsedRemoteInput::Type::PositionXY;
        result.channelId = extractInt(message[0]);
        result.posX = extractFloat(message[1]);
        result.posY = extractFloat(message[2]);
        result.valid = true;
        return result;
    }

    // Check if this is a known Remote parameter
    const auto& remoteMap = getRemoteAddressMap();
    auto it = remoteMap.find(paramName);
    if (it == remoteMap.end())
        return result;  // Unknown parameter

    // Need at least 2 args: <channelID> <value> or <channelID> <inc/dec>
    if (message.size() < 2)
        return result;

    result.paramId = it->second;
    result.channelId = extractInt(message[0]);

    // Check if second argument is "inc" or "dec" for delta mode
    if (message[1].isString())
    {
        juce::String directive = extractString(message[1]);
        if (directive.equalsIgnoreCase("inc") || directive.equalsIgnoreCase("dec"))
        {
            // Delta mode: /remoteInput/<param> <ID> <inc/dec> <value>
            result.type = ParsedRemoteInput::Type::ParameterDelta;
            result.direction = directive.equalsIgnoreCase("inc")
                ? DeltaDirection::Increment : DeltaDirection::Decrement;

            if (message.size() >= 3)
                result.value = extractFloat(message[2]);
            else
                result.value = 1.0f;  // Default delta of 1

            // Legacy compatibility: also set axis and deltaValue for position params
            if (paramName == "positionX" || paramName == "offsetX")
                result.axis = Axis::X;
            else if (paramName == "positionY" || paramName == "offsetY")
                result.axis = Axis::Y;
            else if (paramName == "positionZ" || paramName == "offsetZ")
                result.axis = Axis::Z;
            result.deltaValue = static_cast<float>(result.value);

            result.valid = true;
            return result;
        }

        // String value (e.g., inputName): /remoteInput/inputName <ID> <name>
        result.type = ParsedRemoteInput::Type::ParameterSet;
        result.value = directive;
        result.valid = true;
        return result;
    }

    // Absolute numeric value: /remoteInput/<param> <ID> <value>
    result.type = ParsedRemoteInput::Type::ParameterSet;
    if (message[1].isInt32())
        result.value = extractInt(message[1]);
    else
        result.value = extractFloat(message[1]);
    result.valid = true;
    return result;
}

OSCMessageRouter::ParsedArrayAdjustMessage OSCMessageRouter::parseArrayAdjustMessage(const juce::OSCMessage& message)
{
    ParsedArrayAdjustMessage result;

    juce::String address = message.getAddressPattern().toString();

    if (!isArrayAdjustAddress(address))
        return result;

    // Need exactly 2 args: <array #> <value change>
    if (message.size() < 2)
        return result;

    juce::String paramName = extractParamName(address);

    // Map array adjust addresses to output parameter IDs
    if (paramName == "delayLatency")
        result.paramId = WFSParameterIDs::outputDelayLatency;
    else if (paramName == "attenuation")
        result.paramId = WFSParameterIDs::outputAttenuation;
    else if (paramName == "Hparallax")
        result.paramId = WFSParameterIDs::outputHparallax;
    else if (paramName == "Vparallax")
        result.paramId = WFSParameterIDs::outputVparallax;
    else
        return result;  // Unknown parameter

    result.arrayId = extractInt(message[0]);
    result.valueChange = extractFloat(message[1]);
    result.valid = true;

    return result;
}

OSCMessageRouter::ParsedClusterMoveMessage OSCMessageRouter::parseClusterMoveMessage(const juce::OSCMessage& message)
{
    ParsedClusterMoveMessage result;

    juce::String address = message.getAddressPattern().toString();

    if (!isClusterMoveAddress(address))
        return result;

    // Both message types require 3 arguments: clusterId (int), deltaX (float), deltaY (float)
    if (message.size() < 3)
        return result;

    // Determine type from address
    if (address == "/cluster/move")
        result.type = ParsedClusterMoveMessage::Type::ClusterMove;
    else if (address == "/cluster/barycenter/move")
        result.type = ParsedClusterMoveMessage::Type::BarycenterMove;
    else if (address == "/cluster/positionXY")
        result.type = ParsedClusterMoveMessage::Type::PositionXY;
    else
        return result;

    result.clusterId = extractInt(message[0]);
    result.deltaX = extractFloat(message[1]);
    result.deltaY = extractFloat(message[2]);

    // Validate cluster ID (1-10)
    if (result.clusterId < 1 || result.clusterId > 10)
        return result;

    result.valid = true;
    return result;
}

OSCMessageRouter::ParsedClusterScaleRotationMessage OSCMessageRouter::parseClusterScaleRotationMessage(const juce::OSCMessage& message)
{
    ParsedClusterScaleRotationMessage result;

    juce::String address = message.getAddressPattern().toString();

    if (!isClusterScaleRotationAddress(address))
        return result;

    // Both message types require 2 arguments: clusterId (int), value (float)
    if (message.size() < 2)
        return result;

    // Determine type from address
    if (address == "/cluster/scale")
        result.type = ParsedClusterScaleRotationMessage::Type::Scale;
    else if (address == "/cluster/rotation")
        result.type = ParsedClusterScaleRotationMessage::Type::Rotation;
    else
        return result;

    result.clusterId = extractInt(message[0]);
    result.value = extractFloat(message[1]);

    // Validate cluster ID (1-10)
    if (result.clusterId < 1 || result.clusterId > 10)
        return result;

    result.valid = true;
    return result;
}

OSCMessageRouter::ParsedClusterCumulativeScaleRotationMessage
OSCMessageRouter::parseClusterCumulativeScaleRotationMessage(const juce::OSCMessage& message)
{
    ParsedClusterCumulativeScaleRotationMessage result;

    juce::String address = message.getAddressPattern().toString();
    if (!isClusterCumulativeScaleRotationAddress(address))
        return result;

    // Requires 3 arguments: clusterId (int), cumulativeScale (float), cumulativeRotation (float)
    if (message.size() < 3)
        return result;

    result.clusterId = extractInt(message[0]);
    result.cumulativeScale = extractFloat(message[1]);
    result.cumulativeRotation = extractFloat(message[2]);

    if (result.clusterId < 1 || result.clusterId > 10)
        return result;

    result.valid = true;
    return result;
}

//==============================================================================
// ADM-OSC Message Parsing
//==============================================================================

bool OSCMessageRouter::isADMOSCAddress(const juce::String& address)
{
    return address.startsWith("/adm/obj/");
}

OSCMessageRouter::ParsedADMOSCMessage OSCMessageRouter::parseADMOSCMessage(const juce::OSCMessage& message)
{
    ParsedADMOSCMessage result;
    juce::String address = message.getAddressPattern().toString();

    if (!address.startsWith("/adm/obj/"))
        return result;

    // Extract object ID: /adm/obj/N/...
    juce::String rest = address.substring(9);  // after "/adm/obj/"
    int slashPos = rest.indexOf("/");
    if (slashPos < 0)
        return result;

    result.objectId = rest.substring(0, slashPos).getIntValue();
    if (result.objectId <= 0)
        return result;

    juce::String param = rest.substring(slashPos + 1);  // after the second slash

    if (param == "xyz" && message.size() >= 3)
    {
        result.type  = ParsedADMOSCMessage::Type::XYZ;
        result.v1    = extractFloat(message[0]);
        result.v2    = extractFloat(message[1]);
        result.v3    = extractFloat(message[2]);
        result.valid = true;
    }
    else if (param == "aed" && message.size() >= 3)
    {
        result.type  = ParsedADMOSCMessage::Type::AED;
        result.v1    = extractFloat(message[0]);  // azimuth
        result.v2    = extractFloat(message[1]);  // elevation
        result.v3    = extractFloat(message[2]);  // distance
        result.valid = true;
    }
    else if (param == "x" && message.size() >= 1)
    {
        result.type  = ParsedADMOSCMessage::Type::X;
        result.v1    = extractFloat(message[0]);
        result.valid = true;
    }
    else if (param == "y" && message.size() >= 1)
    {
        result.type  = ParsedADMOSCMessage::Type::Y;
        result.v1    = extractFloat(message[0]);
        result.valid = true;
    }
    else if (param == "z" && message.size() >= 1)
    {
        result.type  = ParsedADMOSCMessage::Type::Z;
        result.v1    = extractFloat(message[0]);
        result.valid = true;
    }
    else if (param == "xy" && message.size() >= 2)
    {
        result.type  = ParsedADMOSCMessage::Type::XY;
        result.v1    = extractFloat(message[0]);
        result.v2    = extractFloat(message[1]);
        result.valid = true;
    }
    else if (param == "azim" && message.size() >= 1)
    {
        result.type  = ParsedADMOSCMessage::Type::Azim;
        result.v1    = extractFloat(message[0]);
        result.valid = true;
    }
    else if (param == "elev" && message.size() >= 1)
    {
        result.type  = ParsedADMOSCMessage::Type::Elev;
        result.v1    = extractFloat(message[0]);
        result.valid = true;
    }
    else if (param == "dist" && message.size() >= 1)
    {
        result.type  = ParsedADMOSCMessage::Type::Dist;
        result.v1    = extractFloat(message[0]);
        result.valid = true;
    }
    else if (param == "gain" && message.size() >= 1)
    {
        result.type  = ParsedADMOSCMessage::Type::Gain;
        result.v1    = extractFloat(message[0]);
        result.valid = true;
    }
    else if (param == "name" && message.size() >= 1 && message[0].isString())
    {
        result.type        = ParsedADMOSCMessage::Type::Name;
        result.stringValue = message[0].getString();
        result.valid       = true;
    }

    return result;
}

} // namespace WFSNetwork
