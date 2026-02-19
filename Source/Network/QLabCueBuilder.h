#pragma once

#include <JuceHeader.h>
#include "OSCProtocolTypes.h"
#include "OSCMessageBuilder.h"
#include "../Parameters/WFSParameterIDs.h"
#include "../Parameters/WFSFileManager.h"

namespace WFSNetwork
{

/**
 * Structured output from QLabCueBuilder.
 * Group and network cue messages are separated so that sendToQLab()
 * can query QLab for unique IDs between steps and construct move commands.
 */
struct QLabCueSequence
{
    std::vector<juce::OSCMessage> groupMessages;  // /new group, name, mode, number

    struct NetworkCue
    {
        std::vector<juce::OSCMessage> messages;   // /new network, number, patch, customString
        int movePosition = 0;                      // 1-based position in group
    };

    std::vector<NetworkCue> networkCues;
};

/**
 * QLabCueBuilder
 *
 * Builds a sequence of OSC messages that, when sent to QLab,
 * create a Group cue containing Network cues for each in-scope
 * parameter/channel from a WFS snapshot. Each Network cue sends
 * an OSC message back to the WFS processor to recall the parameter value.
 */
class QLabCueBuilder
{
public:
    /**
     * Build the structured cue sequence for a snapshot export to QLab.
     *
     * @param snapshotName     Display name for the QLab group cue
     * @param snapshotData     The <Inputs> ValueTree from the snapshot
     * @param scope            Extended scope filtering
     * @param numChannels      Total number of input channels
     * @param qlabPatchNumber  QLab network patch to assign to created cues
     * @return QLabCueSequence with group messages and per-cue messages
     */
    static QLabCueSequence buildSnapshotCues (
        const juce::String& snapshotName,
        const juce::ValueTree& snapshotData,
        const WFSFileManager::ExtendedSnapshotScope& scope,
        int numChannels,
        int qlabPatchNumber)
    {
        QLabCueSequence sequence;

        // 1. Create group cue
        sequence.groupMessages.push_back (juce::OSCMessage ("/new", juce::String ("group")));

        // 2. Name the group
        sequence.groupMessages.push_back (juce::OSCMessage ("/cue/selected/name",
            juce::String ("Snapshot " + snapshotName)));

        // 3. Set playlist mode (mode 6)
        sequence.groupMessages.push_back (juce::OSCMessage ("/cue/selected/mode", (int) 6));

        // 4. For each input channel in scope, create network cues
        int cueCounter = 0;

        for (int i = 0; i < snapshotData.getNumChildren(); ++i)
        {
            auto inputData = snapshotData.getChild (i);
            int channelId = static_cast<int> (inputData.getProperty (WFSParameterIDs::id, 0));
            int channelIndex = channelId - 1;

            if (channelIndex >= 0 && channelIndex < numChannels)
            {
                appendChannelCues (sequence.networkCues, inputData, channelIndex, channelId,
                                   scope, qlabPatchNumber, cueCounter);
            }
        }

        return sequence;
    }

    /** Get the count of network cues that would be created (for progress display) */
    static int countCues (
        const juce::ValueTree& snapshotData,
        const WFSFileManager::ExtendedSnapshotScope& scope,
        int numChannels)
    {
        int count = 0;
        const auto& inputMappings = OSCMessageBuilder::getInputMappings();

        for (int i = 0; i < snapshotData.getNumChildren(); ++i)
        {
            auto inputData = snapshotData.getChild (i);
            int channelIndex = static_cast<int> (inputData.getProperty (WFSParameterIDs::id, 0)) - 1;

            if (channelIndex < 0 || channelIndex >= numChannels)
                continue;

            for (int s = 0; s < inputData.getNumChildren(); ++s)
            {
                auto section = inputData.getChild (s);
                for (int p = 0; p < section.getNumProperties(); ++p)
                {
                    auto paramId = section.getPropertyName (p);
                    if (paramId == WFSParameterIDs::inputName)
                        continue;

                    if (inputMappings.find (paramId) != inputMappings.end()
                        && scope.isParameterIncluded (paramId, channelIndex))
                    {
                        ++count;
                    }
                }
            }
        }
        return count;
    }

private:
    /** Append QLab network cue entries for all in-scope parameters of one channel */
    static void appendChannelCues (
        std::vector<QLabCueSequence::NetworkCue>& networkCues,
        const juce::ValueTree& inputData,
        int channelIndex,
        int channelId,
        const WFSFileManager::ExtendedSnapshotScope& scope,
        int qlabPatchNumber,
        int& cueCounter)
    {
        const auto& inputMappings = OSCMessageBuilder::getInputMappings();

        for (int s = 0; s < inputData.getNumChildren(); ++s)
        {
            auto section = inputData.getChild (s);

            for (int p = 0; p < section.getNumProperties(); ++p)
            {
                auto paramId = section.getPropertyName (p);

                if (paramId == WFSParameterIDs::inputName)
                    continue;

                auto it = inputMappings.find (paramId);
                if (it == inputMappings.end())
                    continue;

                if (!scope.isParameterIncluded (paramId, channelIndex))
                    continue;

                auto oscPath = it->second.oscPath;
                auto value = section.getProperty (paramId);
                ++cueCounter;

                QLabCueSequence::NetworkCue cue;
                cue.movePosition = cueCounter;  // 1-based

                // a. Create network cue
                cue.messages.push_back (juce::OSCMessage ("/new", juce::String ("network")));

                // b. Set QLab network patch
                cue.messages.push_back (juce::OSCMessage ("/cue/selected/patch",
                    qlabPatchNumber));

                // d. Set customString (the OSC message QLab will send)
                cue.messages.push_back (juce::OSCMessage ("/cue/selected/customString",
                    formatCustomString (oscPath, channelId, value)));

                // e. Set descriptive cue name
                cue.messages.push_back (juce::OSCMessage ("/cue/selected/name",
                    formatCueName (paramId, channelId, value)));

                networkCues.push_back (std::move (cue));
            }
        }
    }

    /**
     * Format a parameter value as a QLab customString.
     * Format: "{oscPath} {channelId} {value}"
     * Float values always include a decimal point (so QLab sends float32).
     * Integer values are written without decimal (so QLab sends int32).
     */
    static juce::String formatCustomString (
        const juce::String& oscPath,
        int channelId,
        const juce::var& value)
    {
        juce::String result = oscPath + " " + juce::String (channelId) + " ";
        juce::String strVal = value.toString();

        // Values loaded from XML are always string vars — parse as number by content
        if (strVal.containsChar ('.'))
            result += juce::String (strVal.getDoubleValue(), 6);
        else
            result += juce::String (strVal.getIntValue());

        return result;
    }

    //==========================================================================
    // Parameter display metadata for QLab cue naming
    //==========================================================================

    struct ParamDisplayInfo
    {
        juce::String displayName;
        juce::String unit;
        bool isCompressionRatio;
    };

    static const std::map<juce::Identifier, ParamDisplayInfo>& getParamDisplayMap()
    {
        static const std::map<juce::Identifier, ParamDisplayInfo> map = {
            // Channel
            { WFSParameterIDs::inputAttenuation,          { "Attenuation",          "dB",  false } },
            { WFSParameterIDs::inputDelayLatency,         { "Delay",                "ms",  false } },
            { WFSParameterIDs::inputMinimalLatency,       { "Min Latency",          "",    false } },

            // Position
            { WFSParameterIDs::inputPositionX,            { "Position X",           "m",   false } },
            { WFSParameterIDs::inputPositionY,            { "Position Y",           "m",   false } },
            { WFSParameterIDs::inputPositionZ,            { "Position Z",           "m",   false } },
            { WFSParameterIDs::inputOffsetX,              { "Offset X",             "m",   false } },
            { WFSParameterIDs::inputOffsetY,              { "Offset Y",             "m",   false } },
            { WFSParameterIDs::inputOffsetZ,              { "Offset Z",             "m",   false } },
            { WFSParameterIDs::inputConstraintX,          { "Constraint X",         "",    false } },
            { WFSParameterIDs::inputConstraintY,          { "Constraint Y",         "",    false } },
            { WFSParameterIDs::inputConstraintZ,          { "Constraint Z",         "",    false } },
            { WFSParameterIDs::inputConstraintDistance,    { "Constraint Distance",  "",    false } },
            { WFSParameterIDs::inputConstraintDistanceMin, { "Constraint Dist Min",  "m",  false } },
            { WFSParameterIDs::inputConstraintDistanceMax, { "Constraint Dist Max",  "m",  false } },
            { WFSParameterIDs::inputFlipX,                { "Flip X",               "",    false } },
            { WFSParameterIDs::inputFlipY,                { "Flip Y",               "",    false } },
            { WFSParameterIDs::inputFlipZ,                { "Flip Z",               "",    false } },
            { WFSParameterIDs::inputCluster,              { "Cluster",              "",    false } },
            { WFSParameterIDs::inputTrackingActive,       { "Tracking Active",      "",    false } },
            { WFSParameterIDs::inputTrackingID,           { "Tracking ID",          "",    false } },
            { WFSParameterIDs::inputTrackingSmooth,       { "Tracking Smooth",      "",    false } },
            { WFSParameterIDs::inputMaxSpeedActive,       { "Max Speed Active",     "",    false } },
            { WFSParameterIDs::inputMaxSpeed,             { "Max Speed",            "m/s", false } },
            { WFSParameterIDs::inputPathModeActive,       { "Path Mode",            "",    false } },
            { WFSParameterIDs::inputHeightFactor,         { "Height Factor",        "",    false } },
            { WFSParameterIDs::inputCoordinateMode,       { "Coordinate Mode",      "",    false } },

            // Attenuation
            { WFSParameterIDs::inputAttenuationLaw,       { "Attenuation Law",      "",    false } },
            { WFSParameterIDs::inputDistanceAttenuation,  { "Distance Atten",       "dB",  false } },
            { WFSParameterIDs::inputDistanceRatio,        { "Distance Ratio",       "",    false } },
            { WFSParameterIDs::inputCommonAtten,          { "Common Atten",         "dB",  false } },

            // Directivity
            { WFSParameterIDs::inputDirectivity,          { "Directivity",          "",    false } },
            { WFSParameterIDs::inputRotation,             { "Rotation",             "deg", false } },
            { WFSParameterIDs::inputTilt,                 { "Tilt",                 "deg", false } },
            { WFSParameterIDs::inputHFshelf,              { "HF Shelf",             "dB",  false } },

            // Live Source Tamer
            { WFSParameterIDs::inputLSactive,             { "LS Active",            "",    false } },
            { WFSParameterIDs::inputLSradius,             { "LS Radius",            "m",   false } },
            { WFSParameterIDs::inputLSshape,              { "LS Shape",             "",    false } },
            { WFSParameterIDs::inputLSattenuation,        { "LS Attenuation",       "dB",  false } },
            { WFSParameterIDs::inputLSpeakThreshold,      { "LS Peak Threshold",    "dB",  false } },
            { WFSParameterIDs::inputLSpeakRatio,          { "LS Peak Ratio",        "",    true  } },
            { WFSParameterIDs::inputLSslowThreshold,      { "LS Slow Threshold",    "dB",  false } },
            { WFSParameterIDs::inputLSslowRatio,          { "LS Slow Ratio",        "",    true  } },

            // Hackoustics (Floor Reflections)
            { WFSParameterIDs::inputFRactive,             { "FR Active",            "",    false } },
            { WFSParameterIDs::inputFRattenuation,        { "FR Attenuation",       "dB",  false } },
            { WFSParameterIDs::inputFRlowCutActive,       { "FR Low Cut Active",    "",    false } },
            { WFSParameterIDs::inputFRlowCutFreq,         { "FR Low Cut Freq",      "Hz",  false } },
            { WFSParameterIDs::inputFRhighShelfActive,    { "FR High Shelf Active", "",    false } },
            { WFSParameterIDs::inputFRhighShelfFreq,      { "FR High Shelf Freq",   "Hz",  false } },
            { WFSParameterIDs::inputFRhighShelfGain,      { "FR High Shelf Gain",   "dB",  false } },
            { WFSParameterIDs::inputFRhighShelfSlope,     { "FR High Shelf Slope",  "",    false } },
            { WFSParameterIDs::inputFRdiffusion,          { "FR Diffusion",         "",    false } },

            // Jitter
            { WFSParameterIDs::inputJitter,               { "Jitter",               "",    false } },

            // LFO
            { WFSParameterIDs::inputLFOactive,            { "LFO Active",           "",    false } },
            { WFSParameterIDs::inputLFOperiod,            { "LFO Period",           "s",   false } },
            { WFSParameterIDs::inputLFOphase,             { "LFO Phase",            "deg", false } },
            { WFSParameterIDs::inputLFOshapeX,            { "LFO Shape X",          "",    false } },
            { WFSParameterIDs::inputLFOshapeY,            { "LFO Shape Y",          "",    false } },
            { WFSParameterIDs::inputLFOshapeZ,            { "LFO Shape Z",          "",    false } },
            { WFSParameterIDs::inputLFOrateX,             { "LFO Rate X",           "Hz",  false } },
            { WFSParameterIDs::inputLFOrateY,             { "LFO Rate Y",           "Hz",  false } },
            { WFSParameterIDs::inputLFOrateZ,             { "LFO Rate Z",           "Hz",  false } },
            { WFSParameterIDs::inputLFOamplitudeX,        { "LFO Amplitude X",      "m",   false } },
            { WFSParameterIDs::inputLFOamplitudeY,        { "LFO Amplitude Y",      "m",   false } },
            { WFSParameterIDs::inputLFOamplitudeZ,        { "LFO Amplitude Z",      "m",   false } },
            { WFSParameterIDs::inputLFOphaseX,            { "LFO Phase X",          "deg", false } },
            { WFSParameterIDs::inputLFOphaseY,            { "LFO Phase Y",          "deg", false } },
            { WFSParameterIDs::inputLFOphaseZ,            { "LFO Phase Z",          "deg", false } },
            { WFSParameterIDs::inputLFOgyrophone,         { "LFO Gyrophone",        "",    false } },

            // AutomOtion
            { WFSParameterIDs::inputOtomoX,               { "AutomOtion X",         "m",   false } },
            { WFSParameterIDs::inputOtomoY,               { "AutomOtion Y",         "m",   false } },
            { WFSParameterIDs::inputOtomoZ,               { "AutomOtion Z",         "m",   false } },
            { WFSParameterIDs::inputOtomoAbsoluteRelative, { "AutomOtion Abs/Rel",  "",    false } },
            { WFSParameterIDs::inputOtomoStayReturn,      { "AutomOtion Stay/Return", "",  false } },
            { WFSParameterIDs::inputOtomoSpeedProfile,    { "AutomOtion Speed",     "",    false } },
            { WFSParameterIDs::inputOtomoDuration,        { "AutomOtion Duration",  "s",   false } },
            { WFSParameterIDs::inputOtomoCurve,           { "AutomOtion Curve",     "",    false } },
            { WFSParameterIDs::inputOtomoTrigger,         { "AutomOtion Trigger",   "",    false } },
            { WFSParameterIDs::inputOtomoThreshold,       { "AutomOtion Threshold", "dB",  false } },
            { WFSParameterIDs::inputOtomoReset,           { "AutomOtion Reset",     "",    false } },
            { WFSParameterIDs::inputOtomoPauseResume,     { "AutomOtion Pause",     "",    false } },

            // Mutes
            { WFSParameterIDs::inputMutes,                { "Mutes",                "",    false } },
            { WFSParameterIDs::inputMuteMacro,            { "Mute Macro",           "",    false } },

            // Sidelines
            { WFSParameterIDs::inputSidelinesActive,      { "Sidelines Active",     "",    false } },
            { WFSParameterIDs::inputSidelinesFringe,      { "Sidelines Fringe",     "m",   false } },

            // Reverb Sends
            { WFSParameterIDs::inputReverbSend,           { "Reverb Send",          "dB",  false } },

            // Array Attenuation
            { WFSParameterIDs::inputArrayAtten1,          { "Array 1 Atten",        "dB",  false } },
            { WFSParameterIDs::inputArrayAtten2,          { "Array 2 Atten",        "dB",  false } },
            { WFSParameterIDs::inputArrayAtten3,          { "Array 3 Atten",        "dB",  false } },
            { WFSParameterIDs::inputArrayAtten4,          { "Array 4 Atten",        "dB",  false } },
            { WFSParameterIDs::inputArrayAtten5,          { "Array 5 Atten",        "dB",  false } },
            { WFSParameterIDs::inputArrayAtten6,          { "Array 6 Atten",        "dB",  false } },
            { WFSParameterIDs::inputArrayAtten7,          { "Array 7 Atten",        "dB",  false } },
            { WFSParameterIDs::inputArrayAtten8,          { "Array 8 Atten",        "dB",  false } },
            { WFSParameterIDs::inputArrayAtten9,          { "Array 9 Atten",        "dB",  false } },
            { WFSParameterIDs::inputArrayAtten10,         { "Array 10 Atten",       "dB",  false } },
        };

        return map;
    }

    /**
     * Format a descriptive QLab cue name.
     * Format: "Input <channelId> <paramName> <value><unit>"
     * Compression ratios are formatted as "1:<value>".
     */
    static juce::String formatCueName (
        const juce::Identifier& paramId,
        int channelId,
        const juce::var& value)
    {
        // Look up display info
        const auto& displayMap = getParamDisplayMap();
        auto it = displayMap.find (paramId);

        juce::String displayName;
        juce::String unit;
        bool isRatio = false;

        if (it != displayMap.end())
        {
            displayName = it->second.displayName;
            unit = it->second.unit;
            isRatio = it->second.isCompressionRatio;
        }
        else
        {
            // Derive name from parameter ID: strip "input" prefix, insert spaces before uppercase
            displayName = paramId.toString();
            if (displayName.startsWith ("input"))
                displayName = displayName.substring (5);

            // Insert spaces before uppercase letters
            juce::String spaced;
            for (int i = 0; i < displayName.length(); ++i)
            {
                auto ch = displayName[i];
                if (i > 0 && juce::CharacterFunctions::isUpperCase (ch))
                    spaced += ' ';
                spaced += ch;
            }
            displayName = spaced;
        }

        // Format value
        juce::String strVal = value.toString();
        juce::String formattedValue;

        if (isRatio)
        {
            formattedValue = "1:" + juce::String (strVal.getDoubleValue(), 1);
        }
        else if (strVal.containsChar ('.'))
        {
            formattedValue = juce::String (strVal.getDoubleValue(), 1);
            if (unit.isNotEmpty())
                formattedValue += " " + unit;
        }
        else
        {
            formattedValue = juce::String (strVal.getIntValue());
            if (unit.isNotEmpty())
                formattedValue += " " + unit;
        }

        return "Input " + juce::String (channelId) + " " + displayName + " " + formattedValue;
    }
};

} // namespace WFSNetwork
