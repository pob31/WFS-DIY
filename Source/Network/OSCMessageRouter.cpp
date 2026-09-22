#include "OSCMessageRouter.h"
#include "OSCParameterBounds.h"
#include "../Parameters/WFSParameterDefaults.h"

#include <cmath>
#include <set>

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
        { "colour",           WFSParameterIDs::inputColour },
        { "attenuation",      WFSParameterIDs::inputAttenuation },
        { "delayLatency",     WFSParameterIDs::inputDelayLatency },
        { "minimalLatency",   WFSParameterIDs::inputMinimalLatency },
        { "stereoWidth",      WFSParameterIDs::inputStereoWidth },
        { "stereoAxisOffset", WFSParameterIDs::inputStereoAxisOffset },
        { "stereoAxisLock",   WFSParameterIDs::inputStereoAxisLock },

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

        // Array Attenuation (per-input send level to each output array)
        { "arrayAtten1",     WFSParameterIDs::inputArrayAtten1 },
        { "arrayAtten2",     WFSParameterIDs::inputArrayAtten2 },
        { "arrayAtten3",     WFSParameterIDs::inputArrayAtten3 },
        { "arrayAtten4",     WFSParameterIDs::inputArrayAtten4 },
        { "arrayAtten5",     WFSParameterIDs::inputArrayAtten5 },
        { "arrayAtten6",     WFSParameterIDs::inputArrayAtten6 },
        { "arrayAtten7",     WFSParameterIDs::inputArrayAtten7 },
        { "arrayAtten8",     WFSParameterIDs::inputArrayAtten8 },
        { "arrayAtten9",     WFSParameterIDs::inputArrayAtten9 },
        { "arrayAtten10",    WFSParameterIDs::inputArrayAtten10 },

        // Sidelines
        { "sidelinesEnable", WFSParameterIDs::inputSidelinesActive },
        { "sidelinesFringe", WFSParameterIDs::inputSidelinesFringe },

        // Reverb

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

const std::map<juce::String, juce::Identifier>& OSCMessageRouter::getInputInboundAliases()
{
    // The names OSCMessageBuilder sends and the parameter CSV documents, where
    // the receive map above settled on different ones. QLab snapshot cues are
    // built from the sending names, so without these their Live Source, floor
    // reflection, LFO and coordinate mode cues were dropped without a word.
    static const std::map<juce::String, juce::Identifier> aliases = {
        { "LSactive",       WFSParameterIDs::inputLSactive },
        { "LSpeakEnable",   WFSParameterIDs::inputLSpeakEnable },
        { "LSslowEnable",   WFSParameterIDs::inputLSslowEnable },
        { "FRactive",       WFSParameterIDs::inputFRactive },
        { "LFOactive",      WFSParameterIDs::inputLFOactive },
        { "coordinateMode", WFSParameterIDs::inputCoordinateMode },
    };

    return aliases;
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

const std::map<juce::String, juce::Identifier>& OSCMessageRouter::getEffectAddressMap()
{
    // 190 names, transcribed from the "OSC path" column of
    // Documentation/WFS-UI_effects.csv. That column is the published contract:
    // the map is the transcription, not the design.
    //
    // FOUR OF THESE NAMES ARE STRICT PREFIXES OF OTHERS - "sendLevel" of
    // "sendLevels", "sendOn" of "sendOns", "fxSendLevel"/"fxSendOn" likewise -
    // and the parameter identifiers behind them collide the same way
    // (effectDist/effectDistance*, effectDelay/effectDelayLatency,
    // effectMute/effectMutes). Nothing here may ever be resolved with a
    // startsWith: this map is keyed for EXACT lookup and every classification
    // below compares Identifiers with ==.
    static const std::map<juce::String, juce::Identifier> addressMap = {
        // Header
        { "mapVisible",             WFSParameterIDs::effectsMapVisible },

        // Channel
        { "name",                   WFSParameterIDs::effectName },
        { "attenuation",            WFSParameterIDs::effectAttenuation },
        { "delayLatency",           WFSParameterIDs::effectDelayLatency },
        { "minimalLatency",         WFSParameterIDs::effectMinimalLatency },
        { "linkGroup",              WFSParameterIDs::effectLinkGroup },
        { "linkMode",               WFSParameterIDs::effectLinkMode },
        { "mute",                   WFSParameterIDs::effectMute },
        { "solo",                   WFSParameterIDs::effectSolo },

        // Position
        { "coordinateMode",         WFSParameterIDs::effectCoordinateMode },
        { "positionX",              WFSParameterIDs::effectPositionX },
        { "positionY",              WFSParameterIDs::effectPositionY },
        { "positionZ",              WFSParameterIDs::effectPositionZ },
        { "returnOffsetX",          WFSParameterIDs::effectReturnOffsetX },
        { "returnOffsetY",          WFSParameterIDs::effectReturnOffsetY },
        { "returnOffsetZ",          WFSParameterIDs::effectReturnOffsetZ },

        // Feed
        { "orientation",            WFSParameterIDs::effectOrientation },
        { "angleOn",                WFSParameterIDs::effectAngleOn },
        { "angleOff",               WFSParameterIDs::effectAngleOff },
        { "pitch",                  WFSParameterIDs::effectPitch },
        { "HFdamping",              WFSParameterIDs::effectHFdamping },
        { "feedMiniLatency",        WFSParameterIDs::effectFeedMiniLatency },
        { "distanceAttenPercent",   WFSParameterIDs::effectDistanceAttenPercent },

        // Return
        { "attenuationLaw",         WFSParameterIDs::effectAttenuationLaw },
        { "distanceAttenuation",    WFSParameterIDs::effectDistanceAttenuation },
        { "distanceRatio",          WFSParameterIDs::effectDistanceRatio },
        { "commonAtten",            WFSParameterIDs::effectCommonAtten },
        { "HFshelf",                WFSParameterIDs::effectHFshelf },
        { "mutes",                  WFSParameterIDs::effectMutes },
        { "muteMacro",              WFSParameterIDs::effectMuteMacro },
        { "muteReverbSends",        WFSParameterIDs::effectMuteReverbSends },
        { "arrayAtten1",            WFSParameterIDs::effectArrayAtten1 },
        { "arrayAtten2",            WFSParameterIDs::effectArrayAtten2 },
        { "arrayAtten3",            WFSParameterIDs::effectArrayAtten3 },
        { "arrayAtten4",            WFSParameterIDs::effectArrayAtten4 },
        { "arrayAtten5",            WFSParameterIDs::effectArrayAtten5 },
        { "arrayAtten6",            WFSParameterIDs::effectArrayAtten6 },
        { "arrayAtten7",            WFSParameterIDs::effectArrayAtten7 },
        { "arrayAtten8",            WFSParameterIDs::effectArrayAtten8 },
        { "arrayAtten9",            WFSParameterIDs::effectArrayAtten9 },
        { "arrayAtten10",           WFSParameterIDs::effectArrayAtten10 },

        // AutomOtion
        { "otomoX",                 WFSParameterIDs::effectOtomoX },
        { "otomoY",                 WFSParameterIDs::effectOtomoY },
        { "otomoZ",                 WFSParameterIDs::effectOtomoZ },
        { "otomoCoordinateMode",    WFSParameterIDs::effectOtomoCoordinateMode },
        { "otomoR",                 WFSParameterIDs::effectOtomoR },
        { "otomoTheta",             WFSParameterIDs::effectOtomoTheta },
        { "otomoRsph",              WFSParameterIDs::effectOtomoRsph },
        { "otomoPhi",               WFSParameterIDs::effectOtomoPhi },
        { "otomoAbsoluteRelative",  WFSParameterIDs::effectOtomoAbsoluteRelative },
        { "otomoSpeedProfile",      WFSParameterIDs::effectOtomoSpeedProfile },
        { "otomoDuration",          WFSParameterIDs::effectOtomoDuration },
        { "otomoCurve",             WFSParameterIDs::effectOtomoCurve },
        { "otomoTrigger",           WFSParameterIDs::effectOtomoTrigger },
        { "otomoThreshold",         WFSParameterIDs::effectOtomoThreshold },
        { "otomoReset",             WFSParameterIDs::effectOtomoReset },
        { "otomoPauseResume",       WFSParameterIDs::effectOtomoPauseResume },

        // LFO
        { "LFOactive",              WFSParameterIDs::effectLFOactive },
        { "LFOperiod",              WFSParameterIDs::effectLFOperiod },
        { "LFOphase",               WFSParameterIDs::effectLFOphase },
        { "LFOshapeX",              WFSParameterIDs::effectLFOshapeX },
        { "LFOshapeY",              WFSParameterIDs::effectLFOshapeY },
        { "LFOshapeZ",              WFSParameterIDs::effectLFOshapeZ },
        { "LFOrateX",               WFSParameterIDs::effectLFOrateX },
        { "LFOrateY",               WFSParameterIDs::effectLFOrateY },
        { "LFOrateZ",               WFSParameterIDs::effectLFOrateZ },
        { "LFOamplitudeX",          WFSParameterIDs::effectLFOamplitudeX },
        { "LFOamplitudeY",          WFSParameterIDs::effectLFOamplitudeY },
        { "LFOamplitudeZ",          WFSParameterIDs::effectLFOamplitudeZ },
        { "LFOphaseX",              WFSParameterIDs::effectLFOphaseX },
        { "LFOphaseY",              WFSParameterIDs::effectLFOphaseY },
        { "LFOphaseZ",              WFSParameterIDs::effectLFOphaseZ },

        // Chain
        { "chainOrder",             WFSParameterIDs::effectChainOrder },
        { "chainBypass",            WFSParameterIDs::effectChainBypass },

        // FxDist
        { "distBypass",             WFSParameterIDs::effectDistBypass },
        { "distDrive",              WFSParameterIDs::effectDistDrive },
        { "distShape",              WFSParameterIDs::effectDistShape },
        { "distBias",               WFSParameterIDs::effectDistBias },
        { "distPreLoShelfFreq",     WFSParameterIDs::effectDistPreLoShelfFreq },
        { "distPreLoShelfGain",     WFSParameterIDs::effectDistPreLoShelfGain },
        { "distPreHiShelfFreq",     WFSParameterIDs::effectDistPreHiShelfFreq },
        { "distPreHiShelfGain",     WFSParameterIDs::effectDistPreHiShelfGain },
        { "distPostLoShelfFreq",    WFSParameterIDs::effectDistPostLoShelfFreq },
        { "distPostLoShelfGain",    WFSParameterIDs::effectDistPostLoShelfGain },
        { "distPostHiShelfFreq",    WFSParameterIDs::effectDistPostHiShelfFreq },
        { "distPostHiShelfGain",    WFSParameterIDs::effectDistPostHiShelfGain },
        { "distOutput",             WFSParameterIDs::effectDistOutput },
        { "distMix",                WFSParameterIDs::effectDistMix },
        { "distOversample",         WFSParameterIDs::effectDistOversample },

        // FxEQ
        { "EQBypass",               WFSParameterIDs::effectEQBypass },
        { "EQshape",                WFSParameterIDs::effectEQshape },
        { "EQfreq",                 WFSParameterIDs::effectEQfreq },
        { "EQgain",                 WFSParameterIDs::effectEQgain },
        { "EQq",                    WFSParameterIDs::effectEQq },
        { "EQslope",                WFSParameterIDs::effectEQslope },

        // FxDyn
        { "dynBypass",              WFSParameterIDs::effectDynBypass },
        { "dynDetector",            WFSParameterIDs::effectDynDetector },
        { "dynLookahead",           WFSParameterIDs::effectDynLookahead },
        { "dynMakeup",              WFSParameterIDs::effectDynMakeup },
        { "dynAutoMakeup",          WFSParameterIDs::effectDynAutoMakeup },
        { "dynCompOn",              WFSParameterIDs::effectDynCompOn },
        { "dynCompThreshold",       WFSParameterIDs::effectDynCompThreshold },
        { "dynCompRatio",           WFSParameterIDs::effectDynCompRatio },
        { "dynCompKnee",            WFSParameterIDs::effectDynCompKnee },
        { "dynCompAttack",          WFSParameterIDs::effectDynCompAttack },
        { "dynCompRelease",         WFSParameterIDs::effectDynCompRelease },
        { "dynCompDetectorDelay",   WFSParameterIDs::effectDynCompDetectorDelay },
        { "dynCompScLoCut",         WFSParameterIDs::effectDynCompScLoCut },
        { "dynCompScHiCut",         WFSParameterIDs::effectDynCompScHiCut },
        { "dynExpOn",               WFSParameterIDs::effectDynExpOn },
        { "dynExpThreshold",        WFSParameterIDs::effectDynExpThreshold },
        { "dynExpRatio",            WFSParameterIDs::effectDynExpRatio },
        { "dynExpAttack",           WFSParameterIDs::effectDynExpAttack },
        { "dynExpRelease",          WFSParameterIDs::effectDynExpRelease },
        { "dynExpRange",            WFSParameterIDs::effectDynExpRange },
        { "dynExpHold",             WFSParameterIDs::effectDynExpHold },
        { "dynExpScLoCut",          WFSParameterIDs::effectDynExpScLoCut },
        { "dynExpScHiCut",          WFSParameterIDs::effectDynExpScHiCut },

        // FxMod
        { "modBypass",              WFSParameterIDs::effectModBypass },
        { "modMode",                WFSParameterIDs::effectModMode },
        { "modRate",                WFSParameterIDs::effectModRate },
        { "modDepth",               WFSParameterIDs::effectModDepth },
        { "modDelay",               WFSParameterIDs::effectModDelay },
        { "modFeedback",            WFSParameterIDs::effectModFeedback },
        { "modVoices",              WFSParameterIDs::effectModVoices },
        { "modShape",               WFSParameterIDs::effectModShape },
        { "modPhase",               WFSParameterIDs::effectModPhase },
        { "modLoCut",               WFSParameterIDs::effectModLoCut },
        { "modThroughZero",         WFSParameterIDs::effectModThroughZero },
        { "modMix",                 WFSParameterIDs::effectModMix },

        // FxPhaser
        { "phaserBypass",           WFSParameterIDs::effectPhaserBypass },
        { "phaserStages",           WFSParameterIDs::effectPhaserStages },
        { "phaserCentre",           WFSParameterIDs::effectPhaserCentre },
        { "phaserSpread",           WFSParameterIDs::effectPhaserSpread },
        { "phaserRate",             WFSParameterIDs::effectPhaserRate },
        { "phaserDepth",            WFSParameterIDs::effectPhaserDepth },
        { "phaserShape",            WFSParameterIDs::effectPhaserShape },
        { "phaserFeedback",         WFSParameterIDs::effectPhaserFeedback },
        { "phaserMix",              WFSParameterIDs::effectPhaserMix },

        // FxTrem
        { "tremBypass",             WFSParameterIDs::effectTremBypass },
        { "tremRate",               WFSParameterIDs::effectTremRate },
        { "tremDepth",              WFSParameterIDs::effectTremDepth },
        { "tremShape",              WFSParameterIDs::effectTremShape },
        { "tremMix",                WFSParameterIDs::effectTremMix },

        // FxReverb
        { "reverbBypass",           WFSParameterIDs::effectReverbBypass },
        { "reverbModel",            WFSParameterIDs::effectReverbModel },
        { "reverbType",             WFSParameterIDs::effectReverbType },
        { "reverbPredelay",         WFSParameterIDs::effectReverbPredelay },
        { "reverbRT60",             WFSParameterIDs::effectReverbRT60 },
        { "reverbRT60LowMult",      WFSParameterIDs::effectReverbRT60LowMult },
        { "reverbRT60HighMult",     WFSParameterIDs::effectReverbRT60HighMult },
        { "reverbCrossoverLow",     WFSParameterIDs::effectReverbCrossoverLow },
        { "reverbCrossoverHigh",    WFSParameterIDs::effectReverbCrossoverHigh },
        { "reverbDiffusion",        WFSParameterIDs::effectReverbDiffusion },
        { "reverbSize",             WFSParameterIDs::effectReverbSize },
        { "reverbTone",             WFSParameterIDs::effectReverbTone },
        { "reverbMix",              WFSParameterIDs::effectReverbMix },

        // FxDelay
        { "delayBypass",            WFSParameterIDs::effectDelayBypass },
        { "delayTime",              WFSParameterIDs::effectDelayTime },
        { "delayTaps",              WFSParameterIDs::effectDelayTaps },
        { "delayTapMode",           WFSParameterIDs::effectDelayTapMode },
        { "delayPattern",           WFSParameterIDs::effectDelayPattern },
        { "delayFeedback",          WFSParameterIDs::effectDelayFeedback },
        { "delayFeedbackTap",       WFSParameterIDs::effectDelayFeedbackTap },
        { "delayInLoCut",           WFSParameterIDs::effectDelayInLoCut },
        { "delayFbLoShelfFreq",     WFSParameterIDs::effectDelayFbLoShelfFreq },
        { "delayFbLoShelfGain",     WFSParameterIDs::effectDelayFbLoShelfGain },
        { "delayFbHiShelfFreq",     WFSParameterIDs::effectDelayFbHiShelfFreq },
        { "delayFbHiShelfGain",     WFSParameterIDs::effectDelayFbHiShelfGain },
        { "delayModRate",           WFSParameterIDs::effectDelayModRate },
        { "delayModDepth",          WFSParameterIDs::effectDelayModDepth },
        { "delayDiffusion",         WFSParameterIDs::effectDelayDiffusion },
        { "delayGlide",             WFSParameterIDs::effectDelayGlide },
        { "delayMix",               WFSParameterIDs::effectDelayMix },
        { "delayTapTime",           WFSParameterIDs::effectDelayTapTime },
        { "delayTapLevel",          WFSParameterIDs::effectDelayTapLevel },

        // FxCrush
        { "crushBypass",            WFSParameterIDs::effectCrushBypass },
        { "crushBits",              WFSParameterIDs::effectCrushBits },
        { "crushRate",              WFSParameterIDs::effectCrushRate },
        { "crushFilter",            WFSParameterIDs::effectCrushFilter },
        { "crushDither",            WFSParameterIDs::effectCrushDither },
        { "crushMix",               WFSParameterIDs::effectCrushMix },

        // Sends
        { "sendLevels",             WFSParameterIDs::effectSendLevels },
        { "sendOns",                WFSParameterIDs::effectSendOns },
        { "fxSendLevels",           WFSParameterIDs::effectFxSendLevels },
        { "fxSendOns",              WFSParameterIDs::effectFxSendOns },
        { "sendLevel",              WFSParameterIDs::effectSendLevel },
        { "sendOn",                 WFSParameterIDs::effectSendOn },
        { "fxSendLevel",            WFSParameterIDs::effectFxSendLevel },
        { "fxSendOn",               WFSParameterIDs::effectFxSendOn },
    };

    return addressMap;
}

OSCMessageRouter::ParsedEffectMessage::Kind
OSCMessageRouter::getEffectParamKind (const juce::Identifier& paramId)
{
    using Kind = ParsedEffectMessage::Kind;

    // THE ONE TABLE. Each set holds the identifiers of one argument shape; a
    // parameter that is in none of them is an ordinary per-channel scalar.
    // Membership is by Identifier equality - never by a name prefix - and the
    // sets are built once.
    //
    // The row counts in the comments are the census of the CSV's "OSC path"
    // column and are checked by the control replay, not by the compiler: if a
    // parameter is added to the CSV and forgotten here it silently becomes a
    // Scalar, which is exactly the failure the eight-shape table exists to
    // prevent. Count before you trust.

    // 24 rows: <ID> <instance> <value>. The doubled module node types.
    static const std::set<juce::Identifier> instanced = {
        WFSParameterIDs::effectEQBypass,
        WFSParameterIDs::effectDynBypass,
        WFSParameterIDs::effectDynDetector,
        WFSParameterIDs::effectDynLookahead,
        WFSParameterIDs::effectDynMakeup,
        WFSParameterIDs::effectDynAutoMakeup,
        WFSParameterIDs::effectDynCompOn,
        WFSParameterIDs::effectDynCompThreshold,
        WFSParameterIDs::effectDynCompRatio,
        WFSParameterIDs::effectDynCompKnee,
        WFSParameterIDs::effectDynCompAttack,
        WFSParameterIDs::effectDynCompRelease,
        WFSParameterIDs::effectDynCompDetectorDelay,
        WFSParameterIDs::effectDynCompScLoCut,
        WFSParameterIDs::effectDynCompScHiCut,
        WFSParameterIDs::effectDynExpOn,
        WFSParameterIDs::effectDynExpThreshold,
        WFSParameterIDs::effectDynExpRatio,
        WFSParameterIDs::effectDynExpAttack,
        WFSParameterIDs::effectDynExpRelease,
        WFSParameterIDs::effectDynExpRange,
        WFSParameterIDs::effectDynExpHold,
        WFSParameterIDs::effectDynExpScLoCut,
        WFSParameterIDs::effectDynExpScHiCut,
    };

    // 5 rows: <ID> <instance> <band> <value>. Two indices, on <Band> nodes
    // under FxEq1 / FxEq2.
    static const std::set<juce::Identifier> band = {
        WFSParameterIDs::effectEQshape,
        WFSParameterIDs::effectEQfreq,
        WFSParameterIDs::effectEQgain,
        WFSParameterIDs::effectEQq,
        WFSParameterIDs::effectEQslope,
    };

    // 2 rows: <ID> <tap> <value>. <Tap> nodes under the single <FxDelay>.
    static const std::set<juce::Identifier> tap = {
        WFSParameterIDs::effectDelayTapTime,
        WFSParameterIDs::effectDelayTapLevel,
    };

    // 2 + 2 rows: one CELL of a send row. The column is an input PERMANENT
    // number for the first pair and a DENSE effect index for the second, which
    // is why they are two kinds and not one.
    static const std::set<juce::Identifier> inputCell = {
        WFSParameterIDs::effectSendLevel,
        WFSParameterIDs::effectSendOn,
    };
    static const std::set<juce::Identifier> fxCell = {
        WFSParameterIDs::effectFxSendLevel,
        WFSParameterIDs::effectFxSendOn,
    };

    // 6 rows: <ID> "<csv>". A packed row is ONE string for the whole row. A
    // bare number is never a row, and the parser refuses one rather than
    // letting it reach the write interceptor.
    static const std::set<juce::Identifier> row = {
        WFSParameterIDs::effectMutes,
        WFSParameterIDs::effectChainOrder,
        WFSParameterIDs::effectSendLevels,
        WFSParameterIDs::effectSendOns,
        WFSParameterIDs::effectFxSendLevels,
        WFSParameterIDs::effectFxSendOns,
    };

    // 10 rows: <value>, no channel. Nine are /wfs/config/effects/*; the tenth
    // is effectsMapVisible, which the contract addresses under /wfs/effect/
    // although the property lives in Config.
    static const std::set<juce::Identifier> global = {
        WFSParameterIDs::effectsMapVisible,
        WFSParameterIDs::effectChannels,
        WFSParameterIDs::effectsGlobalLinkNames,
        WFSParameterIDs::effectsGlobalLinkMode,
        WFSParameterIDs::effectsGlobalFxFeedGeometric,
        WFSParameterIDs::effectsGlobalWorkerThreads,
        WFSParameterIDs::effectsGlobalReturnCushion,
        WFSParameterIDs::effectsGlobalLoopGuard,
        WFSParameterIDs::effectsGlobalLoopGuardCeiling,
        WFSParameterIDs::effectsGlobalMaxDelaySeconds,
        WFSParameterIDs::effectsGlobalFeedGpuDevice,
    };

    if (! paramId.isValid())                       return Kind::Unknown;
    if (instanced.count (paramId) != 0)            return Kind::Instanced;
    if (band.count (paramId) != 0)                 return Kind::Band;
    if (tap.count (paramId) != 0)                  return Kind::Tap;
    if (inputCell.count (paramId) != 0)            return Kind::InputCell;
    if (fxCell.count (paramId) != 0)               return Kind::FxCell;
    if (row.count (paramId) != 0)                  return Kind::Row;
    if (global.count (paramId) != 0)               return Kind::Global;
    return Kind::Scalar;
}

bool OSCMessageRouter::isEffectParamRampCapable (const juce::Identifier& paramId)
{
    // Mirrors the "OSC path optional value" column of
    // Documentation/WFS-UI_effects.csv: every row reading "extra value is
    // transition time in seconds". 119 entries.
    //
    // NOT WIRED TO THE RAMPER YET - see the declaration. A ramp argument on one
    // of these is parsed, the value is applied instantly, and rampArgIgnored is
    // raised so the dispatch logs the difference once instead of leaving a
    // client believing in a fade that never ran.
    static const std::set<juce::Identifier> rampCapable = {
        WFSParameterIDs::effectAttenuation,
        WFSParameterIDs::effectDelayLatency,
        WFSParameterIDs::effectPositionX,
        WFSParameterIDs::effectPositionY,
        WFSParameterIDs::effectPositionZ,
        WFSParameterIDs::effectReturnOffsetX,
        WFSParameterIDs::effectReturnOffsetY,
        WFSParameterIDs::effectReturnOffsetZ,
        WFSParameterIDs::effectOrientation,
        WFSParameterIDs::effectAngleOn,
        WFSParameterIDs::effectAngleOff,
        WFSParameterIDs::effectPitch,
        WFSParameterIDs::effectHFdamping,
        WFSParameterIDs::effectDistanceAttenPercent,
        WFSParameterIDs::effectDistanceAttenuation,
        WFSParameterIDs::effectDistanceRatio,
        WFSParameterIDs::effectCommonAtten,
        WFSParameterIDs::effectHFshelf,
        WFSParameterIDs::effectArrayAtten1,
        WFSParameterIDs::effectArrayAtten2,
        WFSParameterIDs::effectArrayAtten3,
        WFSParameterIDs::effectArrayAtten4,
        WFSParameterIDs::effectArrayAtten5,
        WFSParameterIDs::effectArrayAtten6,
        WFSParameterIDs::effectArrayAtten7,
        WFSParameterIDs::effectArrayAtten8,
        WFSParameterIDs::effectArrayAtten9,
        WFSParameterIDs::effectArrayAtten10,
        WFSParameterIDs::effectLFOperiod,
        WFSParameterIDs::effectLFOphase,
        WFSParameterIDs::effectLFOrateX,
        WFSParameterIDs::effectLFOrateY,
        WFSParameterIDs::effectLFOrateZ,
        WFSParameterIDs::effectLFOamplitudeX,
        WFSParameterIDs::effectLFOamplitudeY,
        WFSParameterIDs::effectLFOamplitudeZ,
        WFSParameterIDs::effectLFOphaseX,
        WFSParameterIDs::effectLFOphaseY,
        WFSParameterIDs::effectLFOphaseZ,
        WFSParameterIDs::effectDistDrive,
        WFSParameterIDs::effectDistShape,
        WFSParameterIDs::effectDistBias,
        WFSParameterIDs::effectDistPreLoShelfFreq,
        WFSParameterIDs::effectDistPreLoShelfGain,
        WFSParameterIDs::effectDistPreHiShelfFreq,
        WFSParameterIDs::effectDistPreHiShelfGain,
        WFSParameterIDs::effectDistPostLoShelfFreq,
        WFSParameterIDs::effectDistPostLoShelfGain,
        WFSParameterIDs::effectDistPostHiShelfFreq,
        WFSParameterIDs::effectDistPostHiShelfGain,
        WFSParameterIDs::effectDistOutput,
        WFSParameterIDs::effectDistMix,
        WFSParameterIDs::effectEQfreq,
        WFSParameterIDs::effectEQgain,
        WFSParameterIDs::effectEQq,
        WFSParameterIDs::effectEQslope,
        WFSParameterIDs::effectDynMakeup,
        WFSParameterIDs::effectDynCompThreshold,
        WFSParameterIDs::effectDynCompRatio,
        WFSParameterIDs::effectDynCompKnee,
        WFSParameterIDs::effectDynCompAttack,
        WFSParameterIDs::effectDynCompRelease,
        WFSParameterIDs::effectDynCompDetectorDelay,
        WFSParameterIDs::effectDynCompScLoCut,
        WFSParameterIDs::effectDynCompScHiCut,
        WFSParameterIDs::effectDynExpThreshold,
        WFSParameterIDs::effectDynExpRatio,
        WFSParameterIDs::effectDynExpAttack,
        WFSParameterIDs::effectDynExpRelease,
        WFSParameterIDs::effectDynExpRange,
        WFSParameterIDs::effectDynExpHold,
        WFSParameterIDs::effectDynExpScLoCut,
        WFSParameterIDs::effectDynExpScHiCut,
        WFSParameterIDs::effectModRate,
        WFSParameterIDs::effectModDepth,
        WFSParameterIDs::effectModDelay,
        WFSParameterIDs::effectModFeedback,
        WFSParameterIDs::effectModPhase,
        WFSParameterIDs::effectModLoCut,
        WFSParameterIDs::effectModMix,
        WFSParameterIDs::effectPhaserCentre,
        WFSParameterIDs::effectPhaserSpread,
        WFSParameterIDs::effectPhaserRate,
        WFSParameterIDs::effectPhaserDepth,
        WFSParameterIDs::effectPhaserFeedback,
        WFSParameterIDs::effectPhaserMix,
        WFSParameterIDs::effectTremRate,
        WFSParameterIDs::effectTremDepth,
        WFSParameterIDs::effectTremShape,
        WFSParameterIDs::effectTremMix,
        WFSParameterIDs::effectReverbPredelay,
        WFSParameterIDs::effectReverbRT60,
        WFSParameterIDs::effectReverbRT60LowMult,
        WFSParameterIDs::effectReverbRT60HighMult,
        WFSParameterIDs::effectReverbCrossoverLow,
        WFSParameterIDs::effectReverbCrossoverHigh,
        WFSParameterIDs::effectReverbDiffusion,
        WFSParameterIDs::effectReverbTone,
        WFSParameterIDs::effectReverbMix,
        WFSParameterIDs::effectDelayTime,
        WFSParameterIDs::effectDelayFeedback,
        WFSParameterIDs::effectDelayInLoCut,
        WFSParameterIDs::effectDelayFbLoShelfFreq,
        WFSParameterIDs::effectDelayFbLoShelfGain,
        WFSParameterIDs::effectDelayFbHiShelfFreq,
        WFSParameterIDs::effectDelayFbHiShelfGain,
        WFSParameterIDs::effectDelayModRate,
        WFSParameterIDs::effectDelayModDepth,
        WFSParameterIDs::effectDelayDiffusion,
        WFSParameterIDs::effectDelayGlide,
        WFSParameterIDs::effectDelayMix,
        WFSParameterIDs::effectDelayTapTime,
        WFSParameterIDs::effectDelayTapLevel,
        WFSParameterIDs::effectCrushBits,
        WFSParameterIDs::effectCrushRate,
        WFSParameterIDs::effectCrushDither,
        WFSParameterIDs::effectCrushMix,
        WFSParameterIDs::effectSendLevel,
        WFSParameterIDs::effectFxSendLevel,
    };

    return rampCapable.find (paramId) != rampCapable.end();
}

const std::map<juce::String, juce::Identifier>& OSCMessageRouter::getRemoteAddressMap()
{
    // Remote protocol address names -> parameter IDs
    // Used for /remoteInput/* addresses from Android app
    static const std::map<juce::String, juce::Identifier> addressMap = {
        // Channel
        { "inputName",        WFSParameterIDs::inputName },
        { "inputColour",      WFSParameterIDs::inputColour },
        { "attenuation",      WFSParameterIDs::inputAttenuation },
        { "delayLatency",     WFSParameterIDs::inputDelayLatency },
        { "minimalLatency",   WFSParameterIDs::inputMinimalLatency },
        { "stereoWidth",      WFSParameterIDs::inputStereoWidth },
        { "stereoAxisOffset", WFSParameterIDs::inputStereoAxisOffset },
        { "stereoAxisLock",   WFSParameterIDs::inputStereoAxisLock },

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

        // Array Attenuation (per-input send level to each output array)
        { "arrayAtten1",      WFSParameterIDs::inputArrayAtten1 },
        { "arrayAtten2",      WFSParameterIDs::inputArrayAtten2 },
        { "arrayAtten3",      WFSParameterIDs::inputArrayAtten3 },
        { "arrayAtten4",      WFSParameterIDs::inputArrayAtten4 },
        { "arrayAtten5",      WFSParameterIDs::inputArrayAtten5 },
        { "arrayAtten6",      WFSParameterIDs::inputArrayAtten6 },
        { "arrayAtten7",      WFSParameterIDs::inputArrayAtten7 },
        { "arrayAtten8",      WFSParameterIDs::inputArrayAtten8 },
        { "arrayAtten9",      WFSParameterIDs::inputArrayAtten9 },
        { "arrayAtten10",     WFSParameterIDs::inputArrayAtten10 },

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

        // Effects globals (Config > EffectsGlobal). Every one of these names
        // starts with "effect", which is why getParameterScope tests
        // "effectsGlobal" BEFORE the per-channel "effect" prefix and names
        // effectChannels and effectsMapVisible individually: without that the
        // prefix would route them at a channel node that does not carry them
        // and the write would be dropped with no error at all.
        { OSCPaths::CONFIG_EFFECTS_LINK_NAMES,         WFSParameterIDs::effectsGlobalLinkNames },
        { OSCPaths::CONFIG_EFFECTS_LINK_MODE,          WFSParameterIDs::effectsGlobalLinkMode },
        { OSCPaths::CONFIG_EFFECTS_FX_FEED_GEOMETRIC,  WFSParameterIDs::effectsGlobalFxFeedGeometric },
        { OSCPaths::CONFIG_EFFECTS_WORKER_THREADS,     WFSParameterIDs::effectsGlobalWorkerThreads },
        { OSCPaths::CONFIG_EFFECTS_RETURN_CUSHION,     WFSParameterIDs::effectsGlobalReturnCushion },
        { OSCPaths::CONFIG_EFFECTS_LOOP_GUARD,         WFSParameterIDs::effectsGlobalLoopGuard },
        { OSCPaths::CONFIG_EFFECTS_LOOP_GUARD_CEILING, WFSParameterIDs::effectsGlobalLoopGuardCeiling },
        { OSCPaths::CONFIG_EFFECTS_MAX_DELAY_SECONDS,  WFSParameterIDs::effectsGlobalMaxDelaySeconds },
        { OSCPaths::CONFIG_EFFECTS_FEED_GPU_DEVICE,    WFSParameterIDs::effectsGlobalFeedGpuDevice },

        // THE FIRST CHANNEL COUNT EVER ADDRESSABLE OVER OSC. No other family
        // publishes one, and this one is accepted ONLY while processing is
        // stopped - the refusal lives in OSCManager's config branch, because a
        // count change tears down the shared rings and would stop a running
        // show from a wrong cue. Its path is one segment deep so
        // buildConfigJson's group split skips it and it stays out of the
        // published OSCQuery namespace.
        { OSCPaths::CONFIG_EFFECT_CHANNELS,            WFSParameterIDs::effectChannels },
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
    return address.startsWith(OSCPaths::REVERB_PREFIX);
}

bool OSCMessageRouter::isEffectAddress(const juce::String& address)
{
    return address.startsWith(OSCPaths::EFFECT_PREFIX);
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

    const auto& aliases = getInputInboundAliases();
    auto alias = aliases.find(paramName);
    if (alias != aliases.end())
        return alias->second;

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

juce::Identifier OSCMessageRouter::getEffectParamId(const juce::String& address)
{
    // EXACT lookup of the last path segment, like the other per-channel
    // families. Never a prefix test: "sendLevel" is a prefix of "sendLevels"
    // and they are a cell and a whole row.
    juce::String paramName = extractParamName(address);
    const auto& addressMap = getEffectAddressMap();

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

bool OSCMessageRouter::isNumericString(const juce::String& s)
{
    const juce::String t = s.trim();
    if (t.isEmpty())
        return false;

    int digits = 0, dots = 0;
    for (int i = 0; i < t.length(); ++i)
    {
        const auto c = t[i];
        if (c == '+' || c == '-')
        {
            if (i != 0)
                return false;
        }
        else if (c == '.')
        {
            if (++dots > 1)
                return false;
        }
        else if (c >= '0' && c <= '9')
        {
            ++digits;
        }
        else
        {
            return false;
        }
    }
    return digits > 0;
}

float OSCMessageRouter::extractFloatLenient(const juce::OSCArgument& arg)
{
    if (arg.isString())
    {
        const juce::String s = arg.getString();
        return isNumericString(s) ? s.trim().getFloatValue() : 0.0f;
    }
    return extractFloat(arg);
}

bool OSCMessageRouter::hasOnlyFiniteFloats(const juce::OSCMessage& message,
                                           juce::String& outReason)
{
    for (int i = 0; i < message.size(); ++i)
    {
        if (message[i].isFloat32())
        {
            const float v = message[i].getFloat32();
            if (! std::isfinite(v))
            {
                const char* kind = std::isnan(v) ? "NaN" : "Inf";
                outReason = "non-finite float at arg " + juce::String(i)
                          + " (" + kind + ")";
                return false;
            }
        }
    }
    return true;
}

namespace
{
    // Range-gate a parsed value against the documented bounds for paramId.
    // String values pass through unchanged; numeric values that fall
    // outside [min, max] cause this helper to write a rejection reason
    // and return false. Parameters with no entry in the bounds table
    // pass through as well.
    static bool valueWithinBounds (const juce::Identifier& paramId,
                                   const juce::var& value,
                                   juce::String& outReason)
    {
        if (value.isString())
            return true;
        auto b = WFSNetwork::getBounds (paramId);
        if (! b.has_value())
            return true;
        const double d = static_cast<double> (value);
        if (d < b->min || d > b->max)
        {
            outReason = WFSNetwork::formatOutOfRangeReason (paramId, d);
            return false;
        }
        return true;
    }

    // Clamp the optional ramp-time arg to a sane window [0, 600] s. We
    // clamp rather than reject so a bad ramp doesn't drop the value.
    static float clampRampSeconds (float v)
    {
        return juce::jlimit (0.0f, 600.0f, v);
    }

    // inputMutes is ONE list for the whole input, so its arguments do not take
    // the <value> [fade] shape of the other input parameters. After the channel,
    // starting at argument `first`:
    //   "<list>"      a non-numeric string such as "0,1,0,0": sets every output
    //   <out> <0|1>   two numbers, typed or numeric strings (QLab): one output
    // A lone number is refused. Stored as the list, it unmuted every output but
    // the first, which is how QLab cues and scalar OSC writes lost the mutes.
    static void parseMuteArguments (const juce::OSCMessage& message, int first,
                                     OSCMessageRouter::ParsedInputMessage& result)
    {
        auto isNumber = [&message] (int i)
        {
            return message[i].isInt32() || message[i].isFloat32()
                || (message[i].isString() && OSCMessageRouter::isNumericString (message[i].getString()));
        };

        const int count = message.size() - first;

        if (count == 1 && message[first].isString() && ! isNumber (first))
        {
            result.value = message[first].getString();
            result.valid = true;
            return;
        }

        if (count == 2 && isNumber (first) && isNumber (first + 1))
        {
            const float output = OSCMessageRouter::extractFloatLenient (message[first]);
            const float state  = OSCMessageRouter::extractFloatLenient (message[first + 1]);
            const int maxOutputs = WFSParameterDefaults::maxOutputChannels;

            if (output != std::floor (output) || output < 1.0f || output > static_cast<float> (maxOutputs))
            {
                result.invalidReason = "inputMutes: output " + juce::String (output)
                                     + " is not a whole number from 1 to " + juce::String (maxOutputs);
                return;
            }
            if (state != 0.0f && state != 1.0f)
            {
                result.invalidReason = "inputMutes: state " + juce::String (state) + " is not 0 or 1";
                return;
            }

            result.muteOutput = static_cast<int> (output);
            result.value = static_cast<int> (state);
            result.valid = true;
            return;
        }

        result.invalidReason = "inputMutes takes <output> <0|1>, or the full list \"0,1,...\" "
                               "(one 0 or 1 per output)";
    }
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

    // Check for OSCQuery format first: /wfs/input/{channelID}/{param} <value> [rampTimeSec]
    // e.g. /wfs/input/1/attenuation -5.0
    juce::String suffix = address.fromFirstOccurrenceOf("/wfs/input/", false, true);
    int slashIdx = suffix.indexOf("/");
    if (slashIdx > 0)
    {
        juce::String firstSeg = suffix.substring(0, slashIdx);
        juce::String paramName = suffix.substring(slashIdx + 1);

        if (firstSeg.containsOnly("0123456789") && paramName.isNotEmpty())
        {
            juce::Identifier paramId;
            const auto& addrMap = getInputAddressMap();
            const auto& aliases = getInputInboundAliases();
            if (auto it = addrMap.find(paramName); it != addrMap.end())
                paramId = it->second;
            else if (auto alias = aliases.find(paramName); alias != aliases.end())
                paramId = alias->second;

            if (paramId.isValid() && message.size() >= 1)
            {
                result.paramId = paramId;
                result.channelId = firstSeg.getIntValue();

                if (result.paramId == WFSParameterIDs::inputMutes)
                {
                    parseMuteArguments (message, 0, result);
                    return result;
                }

                // Numeric strings are coerced to floats — QLab custom messages
                // may type every argument as a string. inputName legitimately
                // takes arbitrary text and is exempt.
                if (message[0].isString()
                    && (result.paramId == WFSParameterIDs::inputName
                        || ! isNumericString (message[0].getString())))
                    result.value = extractString(message[0]);
                else
                    result.value = extractFloatLenient(message[0]);

                // Optional ramp time: /wfs/input/{channelID}/{param} <value> <rampTimeSec>
                if (message.size() >= 2)
                {
                    const bool fadeIsNumeric = message[1].isFloat32() || message[1].isInt32()
                        || (message[1].isString() && isNumericString (message[1].getString()));

                    if (fadeIsNumeric && isInputParamRampCapable(result.paramId))
                    {
                        result.rampTimeSecRequested = extractFloatLenient (message[1]);
                        result.rampTimeSec = clampRampSeconds (result.rampTimeSecRequested);
                    }
                    else if (fadeIsNumeric)
                        result.rampArgIgnored = true;
                }

                if (! valueWithinBounds (result.paramId, result.value, result.invalidReason))
                    return result;

                result.valid = true;
                return result;
            }
        }
    }

    // Standard format: /wfs/input/{param} <channelID> <value> [rampTimeSec]
    result.paramId = getInputParamId(address);
    if (result.paramId.isValid() && message.size() >= 2)
    {
        result.channelId = extractInt(message[0]);

        if (result.paramId == WFSParameterIDs::inputMutes)
        {
            parseMuteArguments (message, 1, result);
            return result;
        }

        // Numeric strings are coerced to floats — QLab custom messages may
        // type every argument as a string. inputName legitimately takes
        // arbitrary text and is exempt (as are the "inc"/"dec" directives,
        // which are non-numeric and therefore stay strings).
        if (message[1].isString()
            && (result.paramId == WFSParameterIDs::inputName
                || ! isNumericString (message[1].getString())))
            result.value = extractString(message[1]);
        else
            result.value = extractFloatLenient(message[1]);

        // Optional ramp time argument — only accepted for parameters listed as
        // ramp-capable in Documentation/WFS-UI_input.csv.
        if (message.size() >= 3)
        {
            const bool fadeIsNumeric = message[2].isFloat32() || message[2].isInt32()
                || (message[2].isString() && isNumericString (message[2].getString()));

            if (fadeIsNumeric && isInputParamRampCapable(result.paramId))
            {
                result.rampTimeSecRequested = extractFloatLenient (message[2]);
                result.rampTimeSec = clampRampSeconds (result.rampTimeSecRequested);
            }
            else if (fadeIsNumeric)
                result.rampArgIgnored = true;
        }

        if (! valueWithinBounds (result.paramId, result.value, result.invalidReason))
            return result;

        result.valid = true;
    }

    return result;
}

bool OSCMessageRouter::isInputParamRampCapable(const juce::Identifier& paramId)
{
    // Whitelist mirrors the "OSC path optional value" column in
    // Documentation/WFS-UI_input.csv: every row with
    // "extra value is transition time in seconds".
    static const std::set<juce::Identifier> rampCapable = {
        WFSParameterIDs::inputAttenuation,
        WFSParameterIDs::inputCommonAtten,
        WFSParameterIDs::inputArrayAtten1,
        WFSParameterIDs::inputArrayAtten2,
        WFSParameterIDs::inputArrayAtten3,
        WFSParameterIDs::inputArrayAtten4,
        WFSParameterIDs::inputArrayAtten5,
        WFSParameterIDs::inputArrayAtten6,
        WFSParameterIDs::inputArrayAtten7,
        WFSParameterIDs::inputArrayAtten8,
        WFSParameterIDs::inputArrayAtten9,
        WFSParameterIDs::inputArrayAtten10,
        WFSParameterIDs::inputDelayLatency,
        // The stereo image is a geometric quantity like a position, so a cue
        // that widens or rotates a pair wants the same transition time an
        // OSC-driven move gets. The axis ramps linearly through its own
        // wrap, exactly as inputRotation above already does: a cue crossing
        // ±180 sweeps the long way round rather than the short one, which is
        // why a cue that needs the short arc should be split in two.
        WFSParameterIDs::inputStereoWidth,
        WFSParameterIDs::inputStereoAxisOffset,
        WFSParameterIDs::inputPositionX,
        WFSParameterIDs::inputPositionY,
        WFSParameterIDs::inputPositionZ,
        WFSParameterIDs::inputOffsetX,
        WFSParameterIDs::inputOffsetY,
        WFSParameterIDs::inputOffsetZ,
        WFSParameterIDs::inputMaxSpeed,
        WFSParameterIDs::inputHeightFactor,
        WFSParameterIDs::inputDistanceAttenuation,
        WFSParameterIDs::inputDistanceRatio,
        WFSParameterIDs::inputDirectivity,
        WFSParameterIDs::inputRotation,
        WFSParameterIDs::inputTilt,
        WFSParameterIDs::inputHFshelf,
        WFSParameterIDs::inputLSradius,
        WFSParameterIDs::inputLSattenuation,
        WFSParameterIDs::inputLSpeakThreshold,
        WFSParameterIDs::inputLSpeakRatio,
        WFSParameterIDs::inputLSslowThreshold,
        WFSParameterIDs::inputLSslowRatio,
        WFSParameterIDs::inputFRattenuation,
        WFSParameterIDs::inputFRlowCutFreq,
        WFSParameterIDs::inputFRhighShelfFreq,
        WFSParameterIDs::inputFRhighShelfGain,
        WFSParameterIDs::inputFRhighShelfSlope,
        WFSParameterIDs::inputFRdiffusion,
        WFSParameterIDs::inputJitter,
        WFSParameterIDs::inputLFOperiod,
        WFSParameterIDs::inputLFOphase,
        WFSParameterIDs::inputLFOrateX,
        WFSParameterIDs::inputLFOrateY,
        WFSParameterIDs::inputLFOrateZ,
        WFSParameterIDs::inputLFOamplitudeX,
        WFSParameterIDs::inputLFOamplitudeY,
        WFSParameterIDs::inputLFOamplitudeZ,
        WFSParameterIDs::inputLFOphaseX,
        WFSParameterIDs::inputLFOphaseY,
        WFSParameterIDs::inputLFOphaseZ,
    };

    return rampCapable.find (paramId) != rampCapable.end();
}

OSCMessageRouter::ParsedOutputMessage OSCMessageRouter::parseOutputMessage(const juce::OSCMessage& message)
{
    ParsedOutputMessage result;

    juce::String address = message.getAddressPattern().toString();

    if (!isOutputAddress(address))
        return result;

    // Check OSCQuery format first: /wfs/output/{channelID}/{param} <value>
    juce::String suffix = address.fromFirstOccurrenceOf("/wfs/output/", false, true);
    int slashIdx = suffix.indexOf("/");
    if (slashIdx > 0)
    {
        juce::String firstSeg = suffix.substring(0, slashIdx);
        juce::String paramName = suffix.substring(slashIdx + 1);

        if (firstSeg.containsOnly("0123456789") && paramName.isNotEmpty())
        {
            const auto& addrMap = getOutputAddressMap();
            auto it = addrMap.find(paramName);
            if (it != addrMap.end())
            {
                result.paramId = it->second;
                result.channelId = firstSeg.getIntValue();

                bool isEQ = paramName.startsWith("EQ") && paramName != "EQenable";
                if (isEQ && message.size() >= 2)
                {
                    result.isEQparam = true;
                    result.bandIndex = extractInt(message[0]);
                    if (message[1].isString())
                        result.value = extractString(message[1]);
                    else
                        result.value = extractFloat(message[1]);
                    if (! valueWithinBounds (result.paramId, result.value, result.invalidReason))
                        return result;
                    result.valid = true;
                }
                else if (!isEQ && message.size() >= 1)
                {
                    if (message[0].isString())
                        result.value = extractString(message[0]);
                    else
                        result.value = extractFloat(message[0]);
                    if (! valueWithinBounds (result.paramId, result.value, result.invalidReason))
                        return result;
                    result.valid = true;
                }
                return result;
            }
        }
    }

    // Standard format: /wfs/output/{param} <channelID> <value>
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

        if (! valueWithinBounds (result.paramId, result.value, result.invalidReason))
            return result;

        result.valid = true;
    }

    return result;
}

OSCMessageRouter::ParsedReverbMessage OSCMessageRouter::parseReverbMessage(const juce::OSCMessage& message)
{
    ParsedReverbMessage result;

    juce::String address = message.getAddressPattern().toString();

    if (!isReverbAddress(address))
        return result;

    // Check OSCQuery format first: /wfs/reverb/{channelID}/{param} <value>
    juce::String suffix = address.fromFirstOccurrenceOf(OSCPaths::REVERB_PREFIX, false, true);
    int slashIdx = suffix.indexOf("/");
    if (slashIdx > 0)
    {
        juce::String firstSeg = suffix.substring(0, slashIdx);
        juce::String paramName = suffix.substring(slashIdx + 1);

        if (firstSeg.containsOnly("0123456789") && paramName.isNotEmpty())
        {
            const auto& addrMap = getReverbAddressMap();
            auto it = addrMap.find(paramName);
            if (it != addrMap.end())
            {
                result.paramId = it->second;
                result.channelId = firstSeg.getIntValue();

                bool isEQ = paramName.startsWith("preEQ") && paramName != "preEQenable";
                if (isEQ && message.size() >= 2)
                {
                    result.isEQparam = true;
                    result.bandIndex = extractInt(message[0]);
                    if (message[1].isString())
                        result.value = extractString(message[1]);
                    else
                        result.value = extractFloat(message[1]);
                    if (! valueWithinBounds (result.paramId, result.value, result.invalidReason))
                        return result;
                    result.valid = true;
                }
                else if (!isEQ && message.size() >= 1)
                {
                    if (message[0].isString())
                        result.value = extractString(message[0]);
                    else
                        result.value = extractFloat(message[0]);
                    if (! valueWithinBounds (result.paramId, result.value, result.invalidReason))
                        return result;
                    result.valid = true;
                }
                return result;
            }
        }
    }

    // Standard format: /wfs/reverb/{param} <channelID> <value>
    //                   /wfs/reverb/{preEQparam} <channelID> <band> <value>
    result.paramId = getReverbParamId(address);
    if (result.paramId.isValid())
    {
        juce::String paramName = extractParamName(address);

        // The reverb map spells its EQ entries preEQgain/preEQfreq/... - this
        // test used to read startsWith("EQ"), copied from parseOutputMessage
        // where the keys really are EQgain/EQfreq. No reverb name has ever
        // matched it, so every standard-form pre-EQ write took the two-argument
        // branch below and stored the BAND INDEX as the value. Keep it in step
        // with the OSCQuery arm above, which tests preEQ correctly.
        bool isEQParam = paramName.startsWith("preEQ") && paramName != "preEQenable";

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

        if (! valueWithinBounds (result.paramId, result.value, result.invalidReason))
            return result;

        result.valid = true;
    }

    return result;
}

namespace
{
    // Every argument that is an INDEX rather than a value goes through this:
    // an effect id, an instance, a band, a tap, a send column. QLab types its
    // unquoted custom-message arguments as strings, so "2" must read as 2 the
    // way the input family already accepts "2" as a value (extractFloatLenient
    // + isNumericString). extractInt alone answers 0 for a string, which would
    // turn every QLab-sent index into "band 0" and reject it.
    int extractIndexLenient (const juce::OSCArgument& arg)
    {
        return juce::roundToInt (OSCMessageRouter::extractFloatLenient (arg));
    }

    bool argIsNumber (const juce::OSCMessage& message, int i)
    {
        return message[i].isInt32() || message[i].isFloat32()
            || (message[i].isString() && OSCMessageRouter::isNumericString (message[i].getString()));
    }

    // The one effect parameter that legitimately carries free text, plus the
    // two effects globals that do. Everything else coerces a numeric string.
    bool effectParamTakesText (const juce::Identifier& paramId)
    {
        return paramId == WFSParameterIDs::effectName
            || paramId == WFSParameterIDs::effectsGlobalLinkNames
            || paramId == WFSParameterIDs::effectsGlobalFeedGpuDevice;
    }

    // How many instances the identifier's node type has. effectEQBypass sits on
    // FxEq1/FxEq2; every effectDyn* sits on FxDyn1/FxDyn2. Both are 2 today and
    // the constants are read rather than assumed, because "they happen to be
    // equal" is not a reason for one bound to stand for the other.
    int effectInstanceCountFor (const juce::Identifier& paramId)
    {
        return paramId == WFSParameterIDs::effectEQBypass
                   ? WFSParameterDefaults::numEffectEqInstances
                   : WFSParameterDefaults::numEffectDynInstances;
    }

    juce::String indexOutOfRange (const juce::String& what, int got, int lo, int hi)
    {
        return what + " " + juce::String (got) + " is not in [" + juce::String (lo)
             + ", " + juce::String (hi) + "]";
    }
}

OSCMessageRouter::ParsedEffectMessage OSCMessageRouter::parseEffectMessage(const juce::OSCMessage& message)
{
    using Kind = ParsedEffectMessage::Kind;

    ParsedEffectMessage result;

    const juce::String address = message.getAddressPattern().toString();

    // /wfs/effect/ ONLY. The nine effects globals and the channel count are
    // addressed under /wfs/config/ like every other global and are parsed by
    // parseConfigMessage, which matches the FULL path against
    // getConfigAddressMap - they need no shape table, because a global is
    // always <value>. The one global that reaches this parser is
    // effectsMapVisible, which the published contract keeps under the channel
    // prefix although the property lives in Config; getEffectParamKind answers
    // Kind::Global for it and the channel arm below is skipped.
    if (! isEffectAddress (address))
        return result;

    //--------------------------------------------------------------------------
    // VERBS, before anything else. They are actions with no identifier and no
    // value, so letting them fall through to the parameter lookup would report
    // them as an unknown parameter - which tells an operator nothing about why
    // nothing happened.
    //--------------------------------------------------------------------------
    {
        const juce::String suffix = address.fromFirstOccurrenceOf (OSCPaths::EFFECT_PREFIX, false, true);

        if (suffix == "clear" || suffix == "clearAll"
            || suffix == "selected" || suffix == "editOnMap")
        {
            // Received by OSCManager (the Effects tab, the Map and the engine
            // behind three callbacks). No reason: nothing is refused here.
            result.kind = Kind::Verb;
            result.verb = suffix;
            return result;
        }

        if (suffix == "snapshot/store" || suffix == "snapshot/load")
        {
            result.kind = Kind::Verb;
            result.verb = suffix;
            // Retired, not pending (plan revision 8): one snapshot file carries
            // the inputs AND the effects, so /wfs/input/snapshot/load|store
            // already recall and store the effects. Kept recognised so a client
            // that learnt this address hears where to go instead of silence.
            result.invalidReason = juce::String (OSCPaths::EFFECT_PREFIX) + suffix
                                 + " is retired: one snapshot carries the inputs and the effects,"
                                   " so use /wfs/input/snapshot/" + suffix.fromFirstOccurrenceOf ("/", false, false)
                                 + " \"<name>\". The message was understood and nothing was changed.";
            return result;
        }
    }

    //--------------------------------------------------------------------------
    // IDENTIFY, THEN CLASSIFY, THEN READ ARGUMENTS - in that order and never
    // any other. argBase is the index of the first argument the SHAPE owns:
    // the OSCQuery short form spends the channel on the address, the standard
    // form spends message[0] on it, and a global spends neither.
    //--------------------------------------------------------------------------
    int argBase = 0;
    bool channelFromAddress = false;

    {
        const juce::String suffix = address.fromFirstOccurrenceOf (OSCPaths::EFFECT_PREFIX, false, true);
        const int slashIdx = suffix.indexOf ("/");

        if (slashIdx > 0)
        {
            const juce::String firstSeg  = suffix.substring (0, slashIdx);
            const juce::String paramName = suffix.substring (slashIdx + 1);

            if (firstSeg.containsOnly ("0123456789") && paramName.isNotEmpty())
            {
                const auto& addrMap = getEffectAddressMap();
                if (auto it = addrMap.find (paramName); it != addrMap.end())
                {
                    result.paramId = it->second;
                    result.channelId = firstSeg.getIntValue();
                    channelFromAddress = true;
                }
            }
        }

        if (! result.paramId.isValid())
            result.paramId = getEffectParamId (address);
    }

    if (! result.paramId.isValid())
    {
        // AN UNKNOWN NAME IS STILL AN ANSWER. isEffectAddress is a bare
        // startsWith, so a misspelling under this prefix is captured here and
        // can never reach an unknown-address fallthrough anywhere else: if this
        // return stays silent, `/wfs/effect/attenuatino 1 -9.5` is the same
        // experience as a cable that came out. Kind stays Unknown; only the
        // reason is filled, so the dispatch reports it and changes nothing.
        result.invalidReason = address + " is not an effect parameter. The"
                                         " effects family publishes its names under "
                             + juce::String (OSCPaths::EFFECT_PREFIX)
                             + " - check the spelling. Nothing was changed.";
        return result;
    }

    result.kind = getEffectParamKind (result.paramId);

    //--------------------------------------------------------------------------
    // The channel, for every shape that has one.
    //--------------------------------------------------------------------------
    if (result.kind == Kind::Global)
    {
        if (channelFromAddress)
        {
            result.invalidReason = result.paramId.toString()
                                 + " is a global: it takes <value> and no effect id";
            return result;
        }
    }
    else
    {
        if (! channelFromAddress)
        {
            if (message.size() < 1)
            {
                // The standard form spends message[0] on the effect id, so a
                // message with no arguments at all names a channel it did not
                // send. Said out loud for the same reason as above.
                result.invalidReason = result.paramId.toString()
                                     + " takes <ID> and then its arguments, and this message"
                                       " carried none. Nothing was changed.";
                return result;
            }
            result.channelId = extractIndexLenient (message[0]);
            argBase = 1;
        }

        if (result.channelId < 1 || result.channelId > WFSParameterDefaults::maxEffectChannels)
        {
            result.invalidReason = indexOutOfRange ("effect id", result.channelId,
                                                    1, WFSParameterDefaults::maxEffectChannels);
            return result;
        }
    }

    //--------------------------------------------------------------------------
    // THE SUB-INDICES. Read here, before the value, because the value POSITION
    // depends on how many of them this shape has - which is exactly the thing
    // an argument-first parser cannot know.
    //--------------------------------------------------------------------------
    int valueArg = argBase;

    switch (result.kind)
    {
        case Kind::Instanced:
        {
            const int maxInstance = effectInstanceCountFor (result.paramId);
            if (message.size() < argBase + 2)
            {
                result.invalidReason = result.paramId.toString()
                                     + " takes <ID> <instance 1.." + juce::String (maxInstance)
                                     + "> <value>";
                return result;
            }
            result.instanceIndex = extractIndexLenient (message[argBase]);
            if (result.instanceIndex < 1 || result.instanceIndex > maxInstance)
            {
                result.invalidReason = indexOutOfRange ("instance", result.instanceIndex, 1, maxInstance);
                return result;
            }
            valueArg = argBase + 1;
            break;
        }

        case Kind::Band:
        {
            const int maxInstance = WFSParameterDefaults::numEffectEqInstances;
            const int maxBand     = WFSParameterDefaults::numEffectEQBands;
            if (message.size() < argBase + 3)
            {
                result.invalidReason = result.paramId.toString()
                                     + " takes <ID> <instance 1.." + juce::String (maxInstance)
                                     + "> <band 1.." + juce::String (maxBand) + "> <value>";
                return result;
            }
            result.instanceIndex = extractIndexLenient (message[argBase]);
            result.bandIndex     = extractIndexLenient (message[argBase + 1]);
            if (result.instanceIndex < 1 || result.instanceIndex > maxInstance)
            {
                result.invalidReason = indexOutOfRange ("EQ instance", result.instanceIndex, 1, maxInstance);
                return result;
            }
            if (result.bandIndex < 1 || result.bandIndex > maxBand)
            {
                result.invalidReason = indexOutOfRange ("EQ band", result.bandIndex, 1, maxBand);
                return result;
            }
            valueArg = argBase + 2;
            break;
        }

        case Kind::Tap:
        {
            const int maxTap = WFSParameterDefaults::numEffectDelayTaps;
            if (message.size() < argBase + 2)
            {
                result.invalidReason = result.paramId.toString()
                                     + " takes <ID> <tap 1.." + juce::String (maxTap) + "> <value>";
                return result;
            }
            result.tapIndex = extractIndexLenient (message[argBase]);
            if (result.tapIndex < 1 || result.tapIndex > maxTap)
            {
                result.invalidReason = indexOutOfRange ("delay tap", result.tapIndex, 1, maxTap);
                return result;
            }
            valueArg = argBase + 1;
            break;
        }

        case Kind::InputCell:
        case Kind::FxCell:
        {
            // The two cell keyings differ, and the difference is the whole
            // reason they are two kinds: an effectSend* column is an input
            // PERMANENT NUMBER and an effectFxSend* column is a dense effect
            // id. The rows are stamped at those fixed widths and never resized,
            // so the bound is the row width, not the live channel count.
            const bool isInputKeyed = (result.kind == Kind::InputCell);
            const int  maxCell = isInputKeyed ? WFSParameterDefaults::maxInputChannels
                                              : WFSParameterDefaults::maxEffectChannels;
            const juce::String what = isInputKeyed ? "input number" : "source effect id";

            if (message.size() < argBase + 2)
            {
                result.invalidReason = result.paramId.toString()
                                     + " takes <ID> <" + what + " 1.." + juce::String (maxCell)
                                     + "> <value>";
                return result;
            }
            result.cellIndex = extractIndexLenient (message[argBase]);
            if (result.cellIndex < 1 || result.cellIndex > maxCell)
            {
                result.invalidReason = indexOutOfRange (what, result.cellIndex, 1, maxCell);
                return result;
            }
            valueArg = argBase + 1;
            break;
        }

        case Kind::Row:
        {
            // A PACKED ROW IS ONE STRING, AND A BARE NUMBER IS NEVER ONE.
            // Modelled on parseMuteArguments: accept exactly one non-numeric
            // string and refuse everything else with a reason, rather than
            // handing a scalar to the write interceptor to reject silently.
            // A row set to "0" is a routing wiped to one column.
            const int count = message.size() - argBase;
            if (count == 1 && message[argBase].isString() && ! argIsNumber (message, argBase))
            {
                result.value = message[argBase].getString();
                result.valid = true;
                return result;
            }

            result.invalidReason = result.paramId.toString()
                                 + " is a packed row: it takes <ID> \"<comma-separated row>\","
                                   " one token per column. The stored row was kept.";
            return result;
        }

        case Kind::Scalar:
        case Kind::Global:
            valueArg = argBase;
            break;

        case Kind::Verb:
        case Kind::Unknown:
        default:
            return result;
    }

    if (message.size() < valueArg + 1)
    {
        // The shape was understood and the value was not sent. Every other arm
        // of the switch above already says what it wanted; the Scalar and Global
        // arms reach here instead, so this is where they say it.
        result.invalidReason = result.paramId.toString()
                             + (result.kind == Kind::Global
                                    ? " is a global: it takes <value>, and none was sent."
                                    : " takes <ID> <value> [fade], and no value was sent.")
                             + " Nothing was changed.";
        return result;
    }

    //--------------------------------------------------------------------------
    // THE VALUE, at the position the shape put it.
    //
    // A NON-NUMERIC STRING AT A NUMERIC PARAMETER IS REFUSED, not stored. The
    // range gate waves strings through by design (a row, a name and a chain
    // permutation are all strings), so if the parser accepted "loud" as a value
    // the write interceptor would store it and every later reader would get 0
    // out of it - a -92 dB send reading as unity. Only the three text-taking
    // parameters may carry free text; everything else coerces a numeric string
    // the way QLab needs and rejects anything else with a reason.
    //--------------------------------------------------------------------------
    if (effectParamTakesText (result.paramId))
    {
        result.value = message[valueArg].isString() ? extractString (message[valueArg])
                                                    : juce::var (extractFloatLenient (message[valueArg]));
    }
    else if (message[valueArg].isString() && ! isNumericString (message[valueArg].getString()))
    {
        result.invalidReason = result.paramId.toString() + " takes a number, not \""
                             + message[valueArg].getString() + "\"";
        return result;
    }
    else
    {
        result.value = extractFloatLenient (message[valueArg]);
    }

    //--------------------------------------------------------------------------
    // The optional ramp time, PARSED AND NOT APPLIED. See
    // isEffectParamRampCapable: the ramper is still input-only, so the value is
    // applied instantly and rampArgIgnored records that the fade did not run.
    // The three fields mirror ParsedInputMessage exactly, so generalising the
    // ramper is a wiring job here and not a redesign.
    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    // AN ARGUMENT THE SHAPE CANNOT SPEND IS A WRONG SHAPE, NOT A SPARE.
    //
    // Six per-channel shapes share this one prefix, so a client that picks the
    // wrong one sends a well-formed message that means something else: the
    // parser reads `/wfs/effect/distanceRatio 7 2 5.0` as "effect 7, value 2,
    // fade 5" and stores 2, when the sender plainly meant the 5.0. Nothing in
    // the argument list can tell those apart - but the ARITY can, and counting
    // is free. After the value the shape owns at most one more argument, the
    // optional transition time, and only for the 98 parameters
    // isEffectParamRampCapable lists. Anything beyond that is refused with the
    // shape spelled out, instead of being stored as a value nobody sent.
    //--------------------------------------------------------------------------
    if (message.size() > valueArg + 2)
    {
        result.invalidReason = result.paramId.toString() + " was sent "
                             + juce::String (message.size()) + " arguments and this shape takes "
                             + juce::String (valueArg + (isEffectParamRampCapable (result.paramId) ? 2 : 1))
                             + " at most. The stored value was kept - check that the address"
                               " matches the shape you are sending.";
        return result;
    }

    if (message.size() == valueArg + 2)
    {
        if (! isEffectParamRampCapable (result.paramId))
        {
            // The trailing number is NOT a fade here, so the only readings left
            // are "one argument too many" or "the wrong address" - and under the
            // second one the value already read is an index, not a value. Both
            // are refusals; swallowing it reported a value the sender never sent.
            result.invalidReason = result.paramId.toString()
                                 + " takes no transition time, so the trailing "
                                 + juce::String (extractFloatLenient (message[valueArg + 1]), 3)
                                 + " cannot be one. The stored value was kept - if you meant it"
                                   " as the value, check the shape this address takes.";
            return result;
        }

        if (! argIsNumber (message, valueArg + 1))
        {
            result.invalidReason = result.paramId.toString()
                                 + ": the transition time must be a number, not \""
                                 + extractString (message[valueArg + 1])
                                 + "\". The stored value was kept.";
            return result;
        }

        result.rampTimeSecRequested = extractFloatLenient (message[valueArg + 1]);
        result.rampTimeSec = clampRampSeconds (result.rampTimeSecRequested);
        result.rampArgIgnored = true;
    }

    if (! valueWithinBounds (result.paramId, result.value, result.invalidReason))
        return result;

    // AN INT-TYPED PARAMETER IS STORED AS AN INT. extractFloatLenient answers a
    // float for everything, so a bypass switch arriving as int32 0 would be
    // written as "0.0" - a cosmetic difference from what the GUI writes, and a
    // real one to anything reading the tree with an isInt()-guarded accessor
    // after a load has turned every property into a string. The bounds table
    // already knows which parameters are integers, so it is the one that says
    // so here; a parameter with no entry keeps whatever type it arrived as.
    // Rounded AFTER the range gate, so a value outside the range is rejected
    // rather than rounded into it.
    if (result.value.isDouble())
        if (auto b = WFSNetwork::getBounds (result.paramId); b.has_value() && b->isInt)
            result.value = juce::roundToInt (static_cast<double> (result.value));

    result.valid = true;
    return result;
}

namespace
{
    // THE ONLY TWO CONFIG PARAMETERS THAT CARRY FREE TEXT. Of the 48 addresses
    // in getConfigAddressMap, 46 are numbers - 45 of them carry a bounds entry
    // and the 46th is effectChannels - and exactly these two name things:
    // effectsGlobalLinkNames is a name list and effectsGlobalFeedGpuDevice is a
    // device string. Note that stageShape and reverbAlgoType are NOT here: they
    // read as words in the GUI but travel as integer enums (BIND_I), so a word
    // sent at either of them is a typo and not a value.
    //
    // The effects counterpart is effectParamTakesText, which answers the same
    // question for the /wfs/effect/ family and names effectName as its third.
    bool configParamTakesText (const juce::Identifier& paramId)
    {
        return paramId == WFSParameterIDs::effectsGlobalLinkNames
            || paramId == WFSParameterIDs::effectsGlobalFeedGpuDevice;
    }
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

    //--------------------------------------------------------------------------
    // A NON-NUMERIC STRING AT A NUMERIC CONFIG PARAMETER IS REFUSED, not stored.
    //
    // This is the same rule parseEffectMessage applies to the 190 addressable
    // effect names, and it has to be applied HERE as well because the ten
    // addresses this commit added to getConfigAddressMap do not go through that
    // parser. valueWithinBounds waves every string through by design (a name and
    // a device string are legitimately strings), so without this gate a typo
    // reaches the tree verbatim: `/wfs/config/effects/workerThreads "many"` used
    // to persist the word, and `/wfs/config/effectChannels "seven"` used to read
    // back as static_cast<int> of a String == 0 and DELETE EVERY EFFECT CHANNEL,
    // silently and with no undo. Only the two config parameters that name things
    // may carry free text; everything else coerces a numeric string the way QLab
    // needs - QLab types its unquoted arguments as strings, which is why "3" must
    // still work - and rejects anything else with a reason.
    //--------------------------------------------------------------------------
    if (message[0].isInt32())
        result.value = extractInt(message[0]);
    else if (message[0].isFloat32())
        result.value = extractFloat(message[0]);
    else if (message[0].isString())
    {
        const juce::String s = message[0].getString();

        if (configParamTakesText (result.paramId))
        {
            result.value = s;
        }
        else if (isNumericString (s))
        {
            result.value = s.trim().getFloatValue();
        }
        else
        {
            result.invalidReason = result.paramId.toString() + " takes a number, not \""
                                 + s + "\"";
            return result;
        }
    }
    else
        return result;

    if (! valueWithinBounds (result.paramId, result.value, result.invalidReason))
        return result;

    //--------------------------------------------------------------------------
    // THE CHANNEL COUNT IS A WHOLE NUMBER, AND IT IS THE ONE PARAMETER HERE WITH
    // NO BOUNDS ENTRY. Its absence from the table is deliberate (a structural
    // edit is not a clamped value), but that also means valueWithinBounds waves
    // it through, so the range and the integer-ness are checked here instead.
    // setNumEffectChannels would jlimit 0..32 silently, which turns "set 40" into
    // "delete eight channels" and "set 2.5" into "delete one"; both are refused.
    //--------------------------------------------------------------------------
    if (result.paramId == WFSParameterIDs::effectChannels)
    {
        const double d = static_cast<double> (result.value);
        if (d != std::floor (d) || d < 0.0
            || d > static_cast<double> (WFSParameterDefaults::maxEffectChannels))
        {
            result.invalidReason =
                "effectChannels takes a whole number from 0 to "
                + juce::String (WFSParameterDefaults::maxEffectChannels)
                + ", not " + juce::String (d, 3)
                + ". The effect count was not changed.";
            return result;
        }
        result.value = static_cast<int> (d);
    }

    // AN INT-TYPED PARAMETER IS STORED AS AN INT, for the same reason
    // parseEffectMessage rounds: a numeric string coerces to a float here, and
    // "3.0" written at an integer property reads back as a default through every
    // isInt()-guarded accessor once a load has turned the property into a string.
    if (result.value.isDouble())
        if (auto b = WFSNetwork::getBounds (result.paramId); b.has_value() && b->isInt)
            result.value = juce::roundToInt (static_cast<double> (result.value));

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

        if (auto pb = WFSNetwork::getBounds (WFSParameterIDs::inputPositionX); pb.has_value())
        {
            const double x = static_cast<double> (result.posX);
            const double y = static_cast<double> (result.posY);
            if (x < pb->min || x > pb->max || y < pb->min || y > pb->max)
            {
                result.invalidReason =
                    "out of range: positionXY (" + juce::String (x, 3) + ", "
                    + juce::String (y, 3) + ") not in ["
                    + juce::String (pb->min, 3) + ", " + juce::String (pb->max, 3) + "]";
                return result;
            }
        }

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

    if (! valueWithinBounds (result.paramId, result.value, result.invalidReason))
        return result;

    result.valid = true;
    return result;
}

bool OSCMessageRouter::isArrayMuteAddress(const juce::String& address)
{
    return address == "/arrayAdjust/mute";
}

OSCMessageRouter::ParsedArrayMuteMessage OSCMessageRouter::parseArrayMuteMessage(const juce::OSCMessage& message)
{
    ParsedArrayMuteMessage result;

    if (! isArrayMuteAddress(message.getAddressPattern().toString()) || message.size() < 2)
        return result;

    // Numbers only (int or float); a string argument is not a state.
    if (! (message[0].isInt32() || message[0].isFloat32())
        || ! (message[1].isInt32() || message[1].isFloat32()))
        return result;

    result.arrayId = extractInt(message[0]);
    if (result.arrayId < 1 || result.arrayId > WFSParameterDefaults::outputArrayMax)
        return result;

    result.muted = extractFloat(message[1]) >= 0.5f;
    result.valid = true;
    return result;
}

OSCMessageRouter::ParsedArrayAdjustMessage OSCMessageRouter::parseArrayAdjustMessage(const juce::OSCMessage& message)
{
    ParsedArrayAdjustMessage result;

    juce::String address = message.getAddressPattern().toString();

    if (!isArrayAdjustAddress(address))
        return result;

    // Need at least 2 args: <array #> <value change> or <array #> <inc/dec> [<value>]
    if (message.size() < 2)
        return result;

    juce::String paramName = extractParamName(address);

    // Map array adjust addresses to output parameter IDs
    if (paramName == "delayLatency")
        result.paramId = WFSParameterIDs::outputDelayLatency;
    else if (paramName == "attenuation" || paramName == "level")
        result.paramId = WFSParameterIDs::outputAttenuation;
    else if (paramName == "Hparallax")
        result.paramId = WFSParameterIDs::outputHparallax;
    else if (paramName == "Vparallax")
        result.paramId = WFSParameterIDs::outputVparallax;
    else
        return result;  // Unknown parameter

    result.arrayId = extractInt(message[0]);

    // Delta mode: /arrayAdjust/<param> <array #> <inc/dec> <value>
    if (message[1].isString())
    {
        juce::String directive = extractString(message[1]);
        if (! directive.equalsIgnoreCase("inc") && ! directive.equalsIgnoreCase("dec"))
            return result;  // Unknown directive

        float magnitude = (message.size() >= 3) ? extractFloat(message[2]) : 1.0f;
        result.valueChange = directive.equalsIgnoreCase("inc") ? magnitude : -magnitude;
        result.valid = true;
        return result;
    }

    // Plain numeric delta: /arrayAdjust/<param> <array #> <value change>
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
