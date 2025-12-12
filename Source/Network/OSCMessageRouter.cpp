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
        { "flipX",            WFSParameterIDs::inputFlipX },
        { "flipY",            WFSParameterIDs::inputFlipY },
        { "flipZ",            WFSParameterIDs::inputFlipZ },
        { "cluster",          WFSParameterIDs::inputCluster },
        { "trackingActive",   WFSParameterIDs::inputTrackingActive },
        { "trackingID",       WFSParameterIDs::inputTrackingID },
        { "trackingSmooth",   WFSParameterIDs::inputTrackingSmooth },
        { "maxSpeedActive",   WFSParameterIDs::inputMaxSpeedActive },
        { "maxSpeed",         WFSParameterIDs::inputMaxSpeed },
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
        { "otomoX",              WFSParameterIDs::inputOtomoX },
        { "otomoY",              WFSParameterIDs::inputOtomoY },
        { "otomoZ",              WFSParameterIDs::inputOtomoZ },
        { "otomoAbsRel",         WFSParameterIDs::inputOtomoAbsoluteRelative },
        { "otomoStayReturn",     WFSParameterIDs::inputOtomoStayReturn },
        { "otomoSpeedProfile",   WFSParameterIDs::inputOtomoSpeedProfile },
        { "otomoTrigger",        WFSParameterIDs::inputOtomoTrigger },
        { "otomoThreshold",      WFSParameterIDs::inputOtomoThreshold },
        { "otomoReset",          WFSParameterIDs::inputOtomoReset },
        { "otomoPauseResume",    WFSParameterIDs::inputOtomoPauseResume },

        // Mutes
        { "mutes",           WFSParameterIDs::inputMutes },
        { "muteMacro",       WFSParameterIDs::inputMuteMacro },

        // Reverb
        { "reverbSend",      WFSParameterIDs::inputReverbSend },
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
        { "DistanceAttenPercent",   WFSParameterIDs::outputDistanceAttenPercent },
        { "Hparallax",              WFSParameterIDs::outputHparallax },
        { "Vparallax",              WFSParameterIDs::outputVparallax },

        // EQ
        { "EQenabled",          WFSParameterIDs::outputEQenabled },
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

bool OSCMessageRouter::isRemoteInputAddress(const juce::String& address)
{
    return address.startsWith("/remoteInput/");
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
    if (!result.paramId.isValid())
        return result;

    // Expected format: /wfs/input/{param} <channelID> <value>
    if (message.size() < 2)
        return result;

    result.channelId = extractInt(message[0]);

    // Determine value type based on argument
    if (message[1].isString())
        result.value = extractString(message[1]);
    else
        result.value = extractFloat(message[1]);

    result.valid = true;
    return result;
}

OSCMessageRouter::ParsedOutputMessage OSCMessageRouter::parseOutputMessage(const juce::OSCMessage& message)
{
    ParsedOutputMessage result;

    juce::String address = message.getAddressPattern().toString();

    if (!isOutputAddress(address))
        return result;

    result.paramId = getOutputParamId(address);
    if (!result.paramId.isValid())
        return result;

    // Expected format: /wfs/output/{param} <channelID> <value>
    if (message.size() < 2)
        return result;

    result.channelId = extractInt(message[0]);

    // Determine value type based on argument
    if (message[1].isString())
        result.value = extractString(message[1]);
    else
        result.value = extractFloat(message[1]);

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

    // Handle position deltas: /remoteInput/position{X,Y,Z} <ID> <"inc"/"dec"> <delta>
    if (paramName == "positionX" || paramName == "positionY" || paramName == "positionZ")
    {
        if (message.size() < 3)
            return result;

        result.type = ParsedRemoteInput::Type::PositionDelta;
        result.channelId = extractInt(message[0]);

        // Determine axis
        if (paramName == "positionX")
            result.axis = Axis::X;
        else if (paramName == "positionY")
            result.axis = Axis::Y;
        else
            result.axis = Axis::Z;

        // Parse direction
        juce::String dirStr = extractString(message[1]);
        result.direction = (dirStr.equalsIgnoreCase("inc"))
                               ? DeltaDirection::Increment
                               : DeltaDirection::Decrement;

        // Parse delta value
        result.deltaValue = extractFloat(message[2]);

        result.valid = true;
        return result;
    }

    return result;
}

} // namespace WFSNetwork
