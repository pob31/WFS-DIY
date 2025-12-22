#pragma once

#include <JuceHeader.h>

/**
 * WFS Parameter Identifiers
 *
 * All ValueTree property identifiers for the WFS Processor application.
 * These are organized by section matching the CSV specification files.
 */
namespace WFSParameterIDs
{
    //==========================================================================
    // ValueTree Type Identifiers
    //==========================================================================

    const juce::Identifier WFSProcessor      ("WFSProcessor");
    const juce::Identifier Config            ("Config");
    const juce::Identifier Show              ("Show");
    const juce::Identifier IO                ("IO");
    const juce::Identifier Stage             ("Stage");
    const juce::Identifier Master            ("Master");
    const juce::Identifier Network           ("Network");
    const juce::Identifier NetworkTarget     ("Target");
    const juce::Identifier ADMOSC            ("ADMOSC");
    const juce::Identifier Tracking          ("Tracking");
    const juce::Identifier Inputs            ("Inputs");
    const juce::Identifier Input             ("Input");
    const juce::Identifier Channel           ("Channel");
    const juce::Identifier Position          ("Position");
    const juce::Identifier Attenuation       ("Attenuation");
    const juce::Identifier Directivity       ("Directivity");
    const juce::Identifier LiveSourceTamer   ("LiveSourceTamer");
    const juce::Identifier Hackoustics       ("Hackoustics");
    const juce::Identifier LFO               ("LFO");
    const juce::Identifier AutomOtion        ("AutomOtion");
    const juce::Identifier Mutes             ("Mutes");
    const juce::Identifier Outputs           ("Outputs");
    const juce::Identifier Output            ("Output");
    const juce::Identifier Options           ("Options");
    const juce::Identifier EQ                ("EQ");
    const juce::Identifier Band              ("Band");
    const juce::Identifier AudioPatch        ("AudioPatch");
    const juce::Identifier InputPatch        ("InputPatch");
    const juce::Identifier OutputPatch       ("OutputPatch");

    //==========================================================================
    // Common Property Identifiers
    //==========================================================================

    const juce::Identifier id                ("id");
    const juce::Identifier name              ("name");
    const juce::Identifier enabled           ("enabled");
    const juce::Identifier count             ("count");
    const juce::Identifier version           ("version");
    const juce::Identifier rows              ("rows");
    const juce::Identifier cols              ("cols");

    //==========================================================================
    // Config > Show Section
    //==========================================================================

    const juce::Identifier showName          ("showName");
    const juce::Identifier showLocation      ("showLocation");

    //==========================================================================
    // Config > I/O Section
    //==========================================================================

    const juce::Identifier inputChannels     ("inputChannels");
    const juce::Identifier outputChannels    ("outputChannels");
    const juce::Identifier reverbChannels    ("reverbChannels");
    const juce::Identifier algorithmDSP      ("algorithmDSP");
    const juce::Identifier runDSP            ("runDSP");

    //==========================================================================
    // Config > Stage Section
    //==========================================================================

    const juce::Identifier stageWidth        ("stageWidth");
    const juce::Identifier stageDepth        ("stageDepth");
    const juce::Identifier stageHeight       ("stageHeight");
    const juce::Identifier originWidth       ("originWidth");
    const juce::Identifier originDepth       ("originDepth");
    const juce::Identifier originHeight      ("originHeight");
    const juce::Identifier speedOfSound      ("speedOfSound");
    const juce::Identifier temperature       ("temperature");

    //==========================================================================
    // Config > Master Section
    //==========================================================================

    const juce::Identifier masterLevel       ("masterLevel");
    const juce::Identifier systemLatency     ("systemLatency");
    const juce::Identifier haasEffect        ("haasEffect");

    //==========================================================================
    // Config > Network Section
    //==========================================================================

    const juce::Identifier networkInterface  ("networkInterface");
    const juce::Identifier networkCurrentIP  ("networkCurrentIP");
    const juce::Identifier networkRxUDPport  ("networkRxUDPport");
    const juce::Identifier networkRxTCPport  ("networkRxTCPport");
    const juce::Identifier findDevicePassword ("findDevicePassword");

    // Network Target Properties
    const juce::Identifier networkTSname     ("networkTSname");
    const juce::Identifier networkTSdataMode ("networkTSdataMode");
    const juce::Identifier networkTSip       ("networkTSip");
    const juce::Identifier networkTSport     ("networkTSport");
    const juce::Identifier networkTSrxEnable ("networkTSrxEnable");
    const juce::Identifier networkTStxEnable ("networkTStxEnable");
    const juce::Identifier networkTSProtocol ("networkTSProtocol");

    // OSC Source Filtering
    const juce::Identifier networkOscSourceFilter ("networkOscSourceFilter");

    // OSC Query
    const juce::Identifier networkOscQueryEnabled ("networkOscQueryEnabled");
    const juce::Identifier networkOscQueryPort    ("networkOscQueryPort");

    //==========================================================================
    // Config > ADM-OSC Section
    //==========================================================================

    const juce::Identifier admOscOffsetX     ("admOscOffsetX");
    const juce::Identifier admOscOffsetY     ("admOscOffsetY");
    const juce::Identifier admOscOffsetZ     ("admOscOffsetZ");
    const juce::Identifier admOscScaleX      ("admOscScaleX");
    const juce::Identifier admOscScaleY      ("admOscScaleY");
    const juce::Identifier admOscScaleZ      ("admOscScaleZ");
    const juce::Identifier admOscFlipX       ("admOscFlipX");
    const juce::Identifier admOscFlipY       ("admOscFlipY");
    const juce::Identifier admOscFlipZ       ("admOscFlipZ");

    //==========================================================================
    // Config > Tracking Section
    //==========================================================================

    const juce::Identifier trackingEnabled   ("trackingEnabled");
    const juce::Identifier trackingProtocol  ("trackingProtocol");
    const juce::Identifier trackingPort      ("trackingPort");
    const juce::Identifier trackingOffsetX   ("trackingOffsetX");
    const juce::Identifier trackingOffsetY   ("trackingOffsetY");
    const juce::Identifier trackingOffsetZ   ("trackingOffsetZ");
    const juce::Identifier trackingScaleX    ("trackingScaleX");
    const juce::Identifier trackingScaleY    ("trackingScaleY");
    const juce::Identifier trackingScaleZ    ("trackingScaleZ");
    const juce::Identifier trackingFlipX     ("trackingFlipX");
    const juce::Identifier trackingFlipY     ("trackingFlipY");
    const juce::Identifier trackingFlipZ     ("trackingFlipZ");

    //==========================================================================
    // Config > Clusters Section
    //==========================================================================

    const juce::Identifier Clusters          ("Clusters");
    const juce::Identifier Cluster           ("Cluster");
    const juce::Identifier clusterReferenceMode ("clusterReferenceMode");

    //==========================================================================
    // Input Channel Parameters
    //==========================================================================

    // Input > Channel
    const juce::Identifier inputName             ("inputName");
    const juce::Identifier inputAttenuation      ("inputAttenuation");
    const juce::Identifier inputDelayLatency     ("inputDelayLatency");
    const juce::Identifier inputMinimalLatency   ("inputMinimalLatency");

    // Input > Position
    const juce::Identifier inputPositionX        ("inputPositionX");
    const juce::Identifier inputPositionY        ("inputPositionY");
    const juce::Identifier inputPositionZ        ("inputPositionZ");
    const juce::Identifier inputOffsetX          ("inputOffsetX");
    const juce::Identifier inputOffsetY          ("inputOffsetY");
    const juce::Identifier inputOffsetZ          ("inputOffsetZ");
    const juce::Identifier inputConstraintX      ("inputConstraintX");
    const juce::Identifier inputConstraintY      ("inputConstraintY");
    const juce::Identifier inputConstraintZ      ("inputConstraintZ");
    const juce::Identifier inputFlipX            ("inputFlipX");
    const juce::Identifier inputFlipY            ("inputFlipY");
    const juce::Identifier inputFlipZ            ("inputFlipZ");
    const juce::Identifier inputCluster          ("inputCluster");
    const juce::Identifier inputTrackingActive   ("inputTrackingActive");
    const juce::Identifier inputTrackingID       ("inputTrackingID");
    const juce::Identifier inputTrackingSmooth   ("inputTrackingSmooth");
    const juce::Identifier inputMaxSpeedActive   ("inputMaxSpeedActive");
    const juce::Identifier inputMaxSpeed         ("inputMaxSpeed");
    const juce::Identifier inputHeightFactor     ("inputHeightFactor");

    // Input > Attenuation
    const juce::Identifier inputAttenuationLaw       ("inputAttenuationLaw");
    const juce::Identifier inputDistanceAttenuation  ("inputDistanceAttenuation");
    const juce::Identifier inputDistanceRatio        ("inputDistanceRatio");
    const juce::Identifier inputCommonAtten          ("inputCommonAtten");

    // Input > Directivity
    const juce::Identifier inputDirectivity      ("inputDirectivity");
    const juce::Identifier inputRotation         ("inputRotation");
    const juce::Identifier inputTilt             ("inputTilt");
    const juce::Identifier inputHFshelf          ("inputHFshelf");

    // Input > Live Source Tamer
    const juce::Identifier inputLSactive         ("inputLSactive");
    const juce::Identifier inputLSradius         ("inputLSradius");
    const juce::Identifier inputLSshape          ("inputLSshape");
    const juce::Identifier inputLSattenuation    ("inputLSattenuation");
    const juce::Identifier inputLSpeakThreshold  ("inputLSpeakThreshold");
    const juce::Identifier inputLSpeakRatio      ("inputLSpeakRatio");
    const juce::Identifier inputLSslowThreshold  ("inputLSslowThreshold");
    const juce::Identifier inputLSslowRatio      ("inputLSslowRatio");

    // Input > Hackoustics (Floor Reflections)
    const juce::Identifier inputFRactive             ("inputFRactive");
    const juce::Identifier inputFRattenuation        ("inputFRattenuation");
    const juce::Identifier inputFRlowCutActive       ("inputFRlowCutActive");
    const juce::Identifier inputFRlowCutFreq         ("inputFRlowCutFreq");
    const juce::Identifier inputFRhighShelfActive    ("inputFRhighShelfActive");
    const juce::Identifier inputFRhighShelfFreq      ("inputFRhighShelfFreq");
    const juce::Identifier inputFRhighShelfGain      ("inputFRhighShelfGain");
    const juce::Identifier inputFRhighShelfSlope     ("inputFRhighShelfSlope");
    const juce::Identifier inputFRdiffusion          ("inputFRdiffusion");
    const juce::Identifier inputMuteReverbSends      ("inputMuteReverbSends");

    // Input > Jitter
    const juce::Identifier inputJitter           ("inputJitter");

    // Input > LFO
    const juce::Identifier inputLFOactive        ("inputLFOactive");
    const juce::Identifier inputLFOperiod        ("inputLFOperiod");
    const juce::Identifier inputLFOphase         ("inputLFOphase");
    const juce::Identifier inputLFOshapeX        ("inputLFOshapeX");
    const juce::Identifier inputLFOshapeY        ("inputLFOshapeY");
    const juce::Identifier inputLFOshapeZ        ("inputLFOshapeZ");
    const juce::Identifier inputLFOrateX         ("inputLFOrateX");
    const juce::Identifier inputLFOrateY         ("inputLFOrateY");
    const juce::Identifier inputLFOrateZ         ("inputLFOrateZ");
    const juce::Identifier inputLFOamplitudeX    ("inputLFOamplitudeX");
    const juce::Identifier inputLFOamplitudeY    ("inputLFOamplitudeY");
    const juce::Identifier inputLFOamplitudeZ    ("inputLFOamplitudeZ");
    const juce::Identifier inputLFOphaseX        ("inputLFOphaseX");
    const juce::Identifier inputLFOphaseY        ("inputLFOphaseY");
    const juce::Identifier inputLFOphaseZ        ("inputLFOphaseZ");
    const juce::Identifier inputLFOgyrophone     ("inputLFOgyrophone");

    // Input > AutomOtion
    const juce::Identifier inputOtomoX               ("inputOtomoX");
    const juce::Identifier inputOtomoY               ("inputOtomoY");
    const juce::Identifier inputOtomoZ               ("inputOtomoZ");
    const juce::Identifier inputOtomoAbsoluteRelative ("inputOtomoAbsoluteRelative");
    const juce::Identifier inputOtomoStayReturn      ("inputOtomoStayReturn");
    const juce::Identifier inputOtomoSpeedProfile    ("inputOtomoSpeedProfile");
    const juce::Identifier inputOtomoTrigger         ("inputOtomoTrigger");
    const juce::Identifier inputOtomoThreshold       ("inputOtomoThreshold");
    const juce::Identifier inputOtomoReset           ("inputOtomoReset");
    const juce::Identifier inputOtomoPauseResume     ("inputOtomoPauseResume");

    // Input > Mutes
    const juce::Identifier inputMutes            ("inputMutes");
    const juce::Identifier inputMuteMacro        ("inputMuteMacro");

    // Input > Array Attenuation (per-array level control, 0 dB default, -60 to 0 dB range)
    const juce::Identifier inputArrayAtten1      ("inputArrayAtten1");
    const juce::Identifier inputArrayAtten2      ("inputArrayAtten2");
    const juce::Identifier inputArrayAtten3      ("inputArrayAtten3");
    const juce::Identifier inputArrayAtten4      ("inputArrayAtten4");
    const juce::Identifier inputArrayAtten5      ("inputArrayAtten5");
    const juce::Identifier inputArrayAtten6      ("inputArrayAtten6");
    const juce::Identifier inputArrayAtten7      ("inputArrayAtten7");
    const juce::Identifier inputArrayAtten8      ("inputArrayAtten8");
    const juce::Identifier inputArrayAtten9      ("inputArrayAtten9");
    const juce::Identifier inputArrayAtten10     ("inputArrayAtten10");

    // Input > Map Display
    const juce::Identifier inputMapLocked        ("inputMapLocked");
    const juce::Identifier inputMapVisible       ("inputMapVisible");

    //==========================================================================
    // Output Channel Parameters
    //==========================================================================

    // Output > Channel
    const juce::Identifier outputName            ("outputName");
    const juce::Identifier outputArray           ("outputArray");
    const juce::Identifier outputApplyToArray    ("outputApplyToArray");
    const juce::Identifier outputAttenuation     ("outputAttenuation");
    const juce::Identifier outputDelayLatency    ("outputDelayLatency");

    // Output > Position
    const juce::Identifier outputPositionX       ("outputPositionX");
    const juce::Identifier outputPositionY       ("outputPositionY");
    const juce::Identifier outputPositionZ       ("outputPositionZ");
    const juce::Identifier outputOrientation     ("outputOrientation");
    const juce::Identifier outputAngleOn         ("outputAngleOn");
    const juce::Identifier outputAngleOff        ("outputAngleOff");
    const juce::Identifier outputPitch           ("outputPitch");
    const juce::Identifier outputHFdamping       ("outputHFdamping");

    // Output > Options
    const juce::Identifier outputMiniLatencyEnable   ("outputMiniLatencyEnable");
    const juce::Identifier outputLSattenEnable       ("outputLSattenEnable");
    const juce::Identifier outputFRenable            ("outputFRenable");
    const juce::Identifier outputDistanceAttenPercent ("outputDistanceAttenPercent");
    const juce::Identifier outputHparallax           ("outputHparallax");
    const juce::Identifier outputVparallax           ("outputVparallax");

    // Output > EQ
    const juce::Identifier outputEQenabled       ("outputEQenabled");
    const juce::Identifier eqShape               ("eqShape");
    const juce::Identifier eqFrequency           ("eqFrequency");
    const juce::Identifier eqGain                ("eqGain");
    const juce::Identifier eqQ                   ("eqQ");
    const juce::Identifier eqSlope               ("eqSlope");

    // Output > Map Display
    const juce::Identifier outputMapVisible      ("outputMapVisible");
    const juce::Identifier outputArrayMapVisible ("outputArrayMapVisible");

    //==========================================================================
    // Audio Patch Parameters
    //==========================================================================

    const juce::Identifier driverMode            ("driverMode");
    const juce::Identifier audioInterface        ("audioInterface");
    const juce::Identifier inputMatrixMode       ("inputMatrixMode");
    const juce::Identifier outputMatrixMode      ("outputMatrixMode");
    const juce::Identifier testTone              ("testTone");
    const juce::Identifier sineFrequency         ("sineFrequency");
    const juce::Identifier testToneLevel         ("testToneLevel");
    const juce::Identifier patchData             ("patchData");

    //==========================================================================
    // Reverb Parameters
    //==========================================================================

    // Input reverb send (array of sends per input)
    const juce::Identifier inputReverbSend       ("inputReverbSend");

    // Reverb Section Identifiers
    const juce::Identifier Reverbs               ("Reverbs");
    const juce::Identifier Reverb                ("Reverb");
    const juce::Identifier Feed                  ("Feed");
    const juce::Identifier ReverbReturn          ("Return");

    // Reverb > Channel
    const juce::Identifier reverbName            ("reverbName");
    const juce::Identifier reverbAttenuation     ("reverbAttenuation");
    const juce::Identifier reverbDelayLatency    ("reverbDelayLatency");

    // Reverb > Position
    const juce::Identifier reverbPositionX       ("reverbPositionX");
    const juce::Identifier reverbPositionY       ("reverbPositionY");
    const juce::Identifier reverbPositionZ       ("reverbPositionZ");
    const juce::Identifier reverbReturnOffsetX   ("reverbReturnOffsetX");
    const juce::Identifier reverbReturnOffsetY   ("reverbReturnOffsetY");
    const juce::Identifier reverbReturnOffsetZ   ("reverbReturnOffsetZ");

    // Reverb > Feed
    const juce::Identifier reverbOrientation     ("reverbOrientation");
    const juce::Identifier reverbAngleOn         ("reverbAngleOn");
    const juce::Identifier reverbAngleOff        ("reverbAngleOff");
    const juce::Identifier reverbPitch           ("reverbPitch");
    const juce::Identifier reverbHFdamping       ("reverbHFdamping");
    const juce::Identifier reverbMiniLatencyEnable ("reverbMiniLatencyEnable");
    const juce::Identifier reverbLSenable        ("reverbLSenable");
    const juce::Identifier reverbDistanceAttenEnable ("reverbDistanceAttenEnable");

    // Reverb > EQ (4 bands)
    const juce::Identifier reverbEQenable        ("reverbEQenable");
    const juce::Identifier reverbEQshape         ("reverbEQshape");
    const juce::Identifier reverbEQfreq          ("reverbEQfreq");
    const juce::Identifier reverbEQgain          ("reverbEQgain");
    const juce::Identifier reverbEQq             ("reverbEQq");
    const juce::Identifier reverbEQslope         ("reverbEQslope");

    // Reverb > Return
    const juce::Identifier reverbDistanceAttenuation ("reverbDistanceAttenuation");
    const juce::Identifier reverbCommonAtten     ("reverbCommonAtten");
    const juce::Identifier reverbMutes           ("reverbMutes");
    const juce::Identifier reverbMuteMacro       ("reverbMuteMacro");

    // Reverb > Map Display (global toggle in Config section)
    const juce::Identifier reverbsMapVisible     ("reverbsMapVisible");

} // namespace WFSParameterIDs
