#pragma once

#include <JuceHeader.h>

/**
 * WFS Parameter Defaults and Ranges
 *
 * Default values, minimum/maximum ranges, and metadata for all WFS parameters.
 * Values extracted from CSV specification files.
 */
namespace WFSParameterDefaults
{
    //==========================================================================
    // System Constants
    //==========================================================================

    constexpr int maxInputChannels     = 64;
    constexpr int maxOutputChannels    = 64;
    constexpr int maxReverbChannels    = 16;
    constexpr int maxNetworkTargets    = 6;
    constexpr int maxClusters          = 10;
    constexpr int numEQBands           = 6;
    constexpr int numReverbEQBands     = 4;

    //==========================================================================
    // Config > Show Section
    //==========================================================================

    inline const juce::String showNameDefault       = "My Show";
    inline const juce::String showLocationDefault   = "";

    //==========================================================================
    // Config > I/O Section
    //==========================================================================

    constexpr int inputChannelsDefault      = 8;
    constexpr int outputChannelsDefault     = 16;
    constexpr int reverbChannelsDefault     = 0;
    constexpr int algorithmDSPDefault       = 0;  // 0=Input Buffer CPU, 1=Output Buffer CPU
    constexpr int algorithmDSPMin           = 0;
    constexpr int algorithmDSPMax           = 1;
    constexpr int runDSPDefault             = 0;  // 0=stopped, 1=running

    //==========================================================================
    // Config > Stage Section
    //==========================================================================

    constexpr float stageWidthDefault       = 20.0f;
    constexpr float stageWidthMin           = 0.0f;
    constexpr float stageWidthMax           = 100.0f;

    constexpr float stageDepthDefault       = 10.0f;
    constexpr float stageDepthMin           = 0.0f;
    constexpr float stageDepthMax           = 100.0f;

    constexpr float stageHeightDefault      = 8.0f;
    constexpr float stageHeightMin          = 0.0f;
    constexpr float stageHeightMax          = 100.0f;

    constexpr float originWidthDefault      = 10.0f;  // Half of stage width
    constexpr float originWidthMin          = -100.0f;
    constexpr float originWidthMax          = 200.0f;

    constexpr float originDepthDefault      = 0.0f;
    constexpr float originDepthMin          = -100.0f;
    constexpr float originDepthMax          = 200.0f;

    constexpr float originHeightDefault     = 0.0f;
    constexpr float originHeightMin         = -100.0f;
    constexpr float originHeightMax         = 200.0f;

    constexpr float speedOfSoundDefault     = 343.0f;
    constexpr float speedOfSoundMin         = 319.2f;
    constexpr float speedOfSoundMax         = 367.7f;

    constexpr float temperatureDefault      = 20.0f;
    constexpr float temperatureMin          = -20.0f;
    constexpr float temperatureMax          = 60.0f;

    // Speed of sound formula: speedOfSound = 331.3 + 0.606 * temperature
    inline float calculateSpeedOfSound (float temp) { return 331.3f + 0.606f * temp; }
    inline float calculateTemperature (float sos)   { return (sos - 331.3f) / 0.606f; }

    //==========================================================================
    // Config > Master Section
    //==========================================================================

    constexpr float masterLevelDefault      = 0.0f;
    constexpr float masterLevelMin          = -92.0f;
    constexpr float masterLevelMax          = 0.0f;

    constexpr float systemLatencyDefault    = 0.0f;
    constexpr float systemLatencyMin        = 0.0f;
    constexpr float systemLatencyMax        = 10.0f;

    constexpr float haasEffectDefault       = 0.0f;
    constexpr float haasEffectMin           = 0.0f;
    constexpr float haasEffectMax           = 10.0f;

    //==========================================================================
    // Config > Network Section
    //==========================================================================

    inline const juce::String networkCurrentIPDefault   = "127.0.0.1";
    constexpr int networkRxUDPportDefault               = 8000;
    constexpr int networkRxTCPportDefault               = 8001;
    inline const juce::String findDevicePasswordDefault = "";

    // Network Target Defaults
    inline const juce::String networkTSnameDefault  = "Target";
    constexpr int networkTSdataModeDefault          = 0;  // 0=UDP, 1=TCP
    inline const juce::String networkTSipDefault    = "127.0.0.1";
    constexpr int networkTSportDefault              = 9000;
    constexpr int networkTSrxEnableDefault          = 0;
    constexpr int networkTStxEnableDefault          = 0;
    constexpr int networkTSProtocolDefault          = 0;  // 0=DISABLED, 1=OSC, 2=REMOTE, 3=ADM-OSC

    //==========================================================================
    // Config > ADM-OSC Section
    //==========================================================================

    constexpr float admOscOffsetDefault     = 0.0f;
    constexpr float admOscOffsetMin         = -50.0f;
    constexpr float admOscOffsetMax         = 50.0f;

    constexpr float admOscScaleDefault      = 1.0f;
    constexpr float admOscScaleMin          = 0.01f;
    constexpr float admOscScaleMax          = 100.0f;

    constexpr int admOscFlipDefault         = 0;  // 0=OFF, 1=ON

    //==========================================================================
    // Config > Tracking Section
    //==========================================================================

    constexpr int trackingEnabledDefault    = 0;  // 0=OFF, 1=ON
    constexpr int trackingProtocolDefault   = 0;  // 0=DISABLED, 1=OSC, 2=PSN, 3=RTTrP
    constexpr int trackingProtocolMin       = 0;
    constexpr int trackingProtocolMax       = 3;
    constexpr int trackingPortDefault       = 7000;

    constexpr float trackingOffsetDefault   = 0.0f;
    constexpr float trackingOffsetMin       = -50.0f;
    constexpr float trackingOffsetMax       = 50.0f;

    constexpr float trackingScaleDefault    = 1.0f;
    constexpr float trackingScaleMin        = 0.01f;
    constexpr float trackingScaleMax        = 100.0f;

    constexpr int trackingFlipDefault       = 0;  // 0=OFF, 1=ON

    //==========================================================================
    // Config > Clusters Section
    //==========================================================================

    constexpr int clusterReferenceModeDefault = 0;  // 0=First Input, 1=Barycenter
    constexpr int clusterReferenceModeMin     = 0;
    constexpr int clusterReferenceModeMax     = 1;

    //==========================================================================
    // Input Channel Defaults
    //==========================================================================

    // Input > Channel
    inline juce::String getDefaultInputName (int index) { return "Input " + juce::String (index + 1); }

    constexpr float inputAttenuationDefault     = 0.0f;
    constexpr float inputAttenuationMin         = -92.0f;
    constexpr float inputAttenuationMax         = 0.0f;

    constexpr float inputDelayLatencyDefault    = 0.0f;
    constexpr float inputDelayLatencyMin        = -100.0f;
    constexpr float inputDelayLatencyMax        = 100.0f;

    constexpr int inputMinimalLatencyDefault    = 0;  // 0=Acoustic Precedence, 1=Minimal Latency

    // Input > Position
    constexpr float inputPositionMin            = 0.0f;
    constexpr float inputPositionMax            = 50.0f;

    constexpr float inputOffsetDefault          = 0.0f;
    constexpr float inputOffsetMin              = 0.0f;
    constexpr float inputOffsetMax              = 50.0f;

    constexpr int inputConstraintDefault        = 1;  // 0=Free, 1=Constrained
    constexpr int inputFlipDefault              = 0;  // 0=Normal, 1=Flipped
    constexpr int inputClusterDefault           = 0;  // 0=Single, 1-10=Cluster 1-10
    constexpr int inputClusterMin               = 0;
    constexpr int inputClusterMax               = 10;

    constexpr int inputTrackingActiveDefault    = 0;  // 0=OFF, 1=ON
    constexpr int inputTrackingIDMin            = 1;
    constexpr int inputTrackingIDMax            = 64;
    constexpr int inputTrackingSmoothDefault    = 100;
    constexpr int inputTrackingSmoothMin        = 0;
    constexpr int inputTrackingSmoothMax        = 100;

    constexpr int inputMaxSpeedActiveDefault    = 0;  // 0=OFF, 1=ON
    constexpr float inputMaxSpeedDefault        = 1.0f;
    constexpr float inputMaxSpeedMin            = 0.01f;
    constexpr float inputMaxSpeedMax            = 20.0f;

    constexpr int inputHeightFactorDefault      = 0;
    constexpr int inputHeightFactorMin          = 0;
    constexpr int inputHeightFactorMax          = 100;

    // Input > Attenuation
    constexpr int inputAttenuationLawDefault    = 0;  // 0=Log, 1=1/d
    constexpr float inputDistanceAttenuationDefault = -0.7f;
    constexpr float inputDistanceAttenuationMin = -6.0f;
    constexpr float inputDistanceAttenuationMax = 0.0f;

    constexpr float inputDistanceRatioDefault   = 1.0f;
    constexpr float inputDistanceRatioMin       = 0.1f;
    constexpr float inputDistanceRatioMax       = 10.0f;

    constexpr int inputCommonAttenDefault       = 100;
    constexpr int inputCommonAttenMin           = 0;
    constexpr int inputCommonAttenMax           = 100;

    // Input > Directivity
    constexpr int inputDirectivityDefault       = 360;
    constexpr int inputDirectivityMin           = 2;
    constexpr int inputDirectivityMax           = 360;

    constexpr int inputRotationDefault          = 0;
    constexpr int inputRotationMin              = -179;
    constexpr int inputRotationMax              = 180;

    constexpr int inputTiltDefault              = 0;
    constexpr int inputTiltMin                  = -90;
    constexpr int inputTiltMax                  = 90;

    constexpr float inputHFshelfDefault         = -6.0f;
    constexpr float inputHFshelfMin             = -24.0f;
    constexpr float inputHFshelfMax             = 0.0f;

    // Input > Live Source Tamer
    constexpr int inputLSactiveDefault          = 0;
    constexpr float inputLSradiusDefault        = 3.0f;
    constexpr float inputLSradiusMin            = 0.0f;
    constexpr float inputLSradiusMax            = 50.0f;

    constexpr int inputLSshapeDefault           = 0;  // 0=linear, 1=log, 2=square, 3=sine
    constexpr int inputLSshapeMin               = 0;
    constexpr int inputLSshapeMax               = 3;

    constexpr float inputLSattenuationDefault   = 0.0f;
    constexpr float inputLSattenuationMin       = -24.0f;
    constexpr float inputLSattenuationMax       = 0.0f;

    constexpr float inputLSpeakThresholdDefault = -20.0f;
    constexpr float inputLSpeakThresholdMin     = -48.0f;
    constexpr float inputLSpeakThresholdMax     = 0.0f;

    constexpr float inputLSpeakRatioDefault     = 2.0f;
    constexpr float inputLSpeakRatioMin         = 1.0f;
    constexpr float inputLSpeakRatioMax         = 10.0f;

    constexpr float inputLSslowThresholdDefault = -20.0f;
    constexpr float inputLSslowThresholdMin     = -48.0f;
    constexpr float inputLSslowThresholdMax     = 0.0f;

    constexpr float inputLSslowRatioDefault     = 2.0f;
    constexpr float inputLSslowRatioMin         = 1.0f;
    constexpr float inputLSslowRatioMax         = 10.0f;

    // Input > Hackoustics (Floor Reflections)
    constexpr int inputFRactiveDefault          = 0;
    constexpr float inputFRattenuationDefault   = -3.0f;
    constexpr float inputFRattenuationMin       = -60.0f;
    constexpr float inputFRattenuationMax       = 0.0f;

    constexpr int inputFRlowCutActiveDefault    = 1;
    constexpr int inputFRlowCutFreqDefault      = 100;
    constexpr int inputFRfreqMin                = 20;
    constexpr int inputFRfreqMax                = 20000;

    constexpr int inputFRhighShelfActiveDefault = 1;
    constexpr int inputFRhighShelfFreqDefault   = 3000;
    constexpr float inputFRhighShelfGainDefault = -2.0f;
    constexpr float inputFRhighShelfGainMin     = -24.0f;
    constexpr float inputFRhighShelfGainMax     = 0.0f;

    constexpr float inputFRhighShelfSlopeDefault = 0.4f;
    constexpr float inputFRhighShelfSlopeMin    = 0.1f;
    constexpr float inputFRhighShelfSlopeMax    = 0.9f;

    constexpr int inputFRdiffusionDefault       = 20;
    constexpr int inputFRdiffusionMin           = 0;
    constexpr int inputFRdiffusionMax           = 100;

    // Input > Jitter
    constexpr float inputJitterDefault          = 0.0f;
    constexpr float inputJitterMin              = 0.0f;
    constexpr float inputJitterMax              = 10.0f;

    // Input > LFO
    constexpr int inputLFOactiveDefault         = 0;
    constexpr float inputLFOperiodDefault       = 5.0f;
    constexpr float inputLFOperiodMin           = 0.01f;
    constexpr float inputLFOperiodMax           = 100.0f;

    constexpr int inputLFOphaseDefault          = 0;
    constexpr int inputLFOphaseMin              = 0;
    constexpr int inputLFOphaseMax              = 360;

    constexpr int inputLFOshapeDefault          = 0;  // 0=OFF, 1=sine, 2=square, 3=sawtooth, 4=triangle, 5=keystone, 6=log, 7=exp, 8=random
    constexpr int inputLFOshapeMin              = 0;
    constexpr int inputLFOshapeMax              = 8;

    constexpr float inputLFOrateDefault         = 1.0f;
    constexpr float inputLFOrateMin             = 0.01f;
    constexpr float inputLFOrateMax             = 100.0f;

    constexpr float inputLFOamplitudeDefault    = 1.0f;
    constexpr float inputLFOamplitudeMin        = 0.0f;
    constexpr float inputLFOamplitudeMax        = 50.0f;

    constexpr int inputLFOgyrophoneDefault      = 0;  // -1=Anti-Clockwise, 0=OFF, 1=Clockwise
    constexpr int inputLFOgyrophoneMin          = -1;
    constexpr int inputLFOgyrophoneMax          = 1;

    // Input > AutomOtion
    constexpr float inputOtomoDefault           = 0.0f;
    constexpr float inputOtomoMin               = -50.0f;
    constexpr float inputOtomoMax               = 50.0f;

    constexpr int inputOtomoAbsoluteRelativeDefault = 0;  // 0=Absolute, 1=Relative
    constexpr int inputOtomoStayReturnDefault   = 0;      // 0=Stay, 1=Return
    constexpr int inputOtomoSpeedProfileDefault = 0;
    constexpr int inputOtomoSpeedProfileMin     = 0;
    constexpr int inputOtomoSpeedProfileMax     = 100;

    constexpr int inputOtomoTriggerDefault      = 0;  // 0=Manual, 1=Trigger
    constexpr float inputOtomoThresholdDefault  = -20.0f;
    constexpr float inputOtomoThresholdMin      = -92.0f;
    constexpr float inputOtomoThresholdMax      = 0.0f;

    constexpr float inputOtomoResetDefault      = -60.0f;
    constexpr float inputOtomoResetMin          = -92.0f;
    constexpr float inputOtomoResetMax          = 0.0f;

    constexpr int inputOtomoPauseResumeDefault  = 1;  // 0=Paused, 1=Resume

    //==========================================================================
    // Output Channel Defaults
    //==========================================================================

    // Output > Channel
    inline juce::String getDefaultOutputName (int index) { return "Output " + juce::String (index + 1); }

    constexpr int outputArrayDefault            = 0;  // 0=Single, 1-10=Array 1-10
    constexpr int outputArrayMin                = 0;
    constexpr int outputArrayMax                = 10;

    constexpr int outputApplyToArrayDefault     = 0;  // 0=This speaker, 1=Apply to array

    constexpr float outputAttenuationDefault    = 0.0f;
    constexpr float outputAttenuationMin        = -92.0f;
    constexpr float outputAttenuationMax        = 12.0f;

    constexpr float outputDelayLatencyDefault   = 0.0f;
    constexpr float outputDelayLatencyMin       = -100.0f;
    constexpr float outputDelayLatencyMax       = 100.0f;

    // Output > Position
    constexpr float outputPositionDefault       = 0.0f;
    constexpr float outputPositionMin           = -100.0f;
    constexpr float outputPositionMax           = 100.0f;

    constexpr int outputOrientationDefault      = 0;  // Degrees
    constexpr int outputOrientationMin          = -180;
    constexpr int outputOrientationMax          = 180;

    constexpr int outputAngleOnDefault          = 86;  // Degrees
    constexpr int outputAngleOnMin              = 0;
    constexpr int outputAngleOnMax              = 90;

    constexpr int outputAngleOffDefault         = 90;  // Degrees
    constexpr int outputAngleOffMin             = 0;
    constexpr int outputAngleOffMax             = 180;

    constexpr int outputPitchDefault            = 0;  // Degrees
    constexpr int outputPitchMin                = -90;
    constexpr int outputPitchMax                = 90;

    constexpr float outputHFdampingDefault      = 0.0f;
    constexpr float outputHFdampingMin          = -24.0f;
    constexpr float outputHFdampingMax          = 0.0f;

    // Output > Options
    constexpr int outputMiniLatencyEnableDefault = 1;    // 0=OFF, 1=ON
    constexpr int outputLSattenEnableDefault    = 1;     // 0=OFF, 1=ON

    constexpr int outputDistanceAttenPercentDefault = 100;
    constexpr int outputDistanceAttenPercentMin = 0;
    constexpr int outputDistanceAttenPercentMax = 100;

    constexpr float outputParallaxDefault       = 0.0f;
    constexpr float outputParallaxMin           = -10.0f;
    constexpr float outputParallaxMax           = 10.0f;

    // Output > EQ
    constexpr int outputEQenabledDefault        = 0;  // 0=OFF, 1=ON

    constexpr int eqShapeDefault                = 3;  // 0=Off, 1=LowShelf, 2=LowCut, 3=Peak, 4=HighCut, 5=HighShelf
    constexpr int eqShapeMin                    = 0;
    constexpr int eqShapeMax                    = 5;

    constexpr float eqFrequencyDefault          = 1000.0f;
    constexpr float eqFrequencyMin              = 20.0f;
    constexpr float eqFrequencyMax              = 20000.0f;

    constexpr float eqGainDefault               = 0.0f;
    constexpr float eqGainMin                   = -24.0f;
    constexpr float eqGainMax                   = 24.0f;

    constexpr float eqQDefault                  = 0.7f;
    constexpr float eqQMin                      = 0.1f;
    constexpr float eqQMax                      = 10.0f;

    constexpr float eqSlopeDefault              = 0.7f;
    constexpr float eqSlopeMin                  = 0.1f;
    constexpr float eqSlopeMax                  = 1.0f;

    // Default EQ band settings (frequency per band)
    inline const float eqBandFrequencies[6] = { 80.0f, 250.0f, 1000.0f, 4000.0f, 8000.0f, 12000.0f };
    inline const int eqBandShapes[6] = { 1, 3, 3, 3, 5, 0 };  // LowShelf, Peak, Peak, Peak, HighShelf, Off

    //==========================================================================
    // Audio Patch Defaults
    //==========================================================================

    constexpr int driverModeDefault             = 0;
    inline const juce::String audioInterfaceDefault = "";

    constexpr int inputMatrixModeDefault        = 0;  // 0=Scroll, 1=Patch
    constexpr int outputMatrixModeDefault       = 0;  // 0=Scroll, 1=Patch, 2=Test

    constexpr int testToneDefault               = 0;  // 0=OFF, 1=Sine, 2=Pink Noise
    constexpr int sineFrequencyDefault          = 1000;
    constexpr int sineFrequencyMin              = 20;
    constexpr int sineFrequencyMax              = 20000;

    constexpr float testToneLevelDefault        = -92.0f;
    constexpr float testToneLevelMin            = -92.0f;
    constexpr float testToneLevelMax            = 0.0f;

    //==========================================================================
    // Helper Functions for Default Positions
    //==========================================================================

    /**
     * Calculate default input position distributed across the stage.
     * Inputs are arranged in rows of up to 8, centered on the stage.
     */
    inline void getDefaultInputPosition (int index, int totalInputs,
                                         float stageWidth, float stageDepth, float /*stageHeight*/,
                                         float /*originW*/, float /*originD*/, float /*originH*/,
                                         float& x, float& y, float& z)
    {
        const int numCols = juce::jmin (8, totalInputs);
        const int numRows = (totalInputs + 7) / 8;
        const int col = index % numCols;
        const int row = index / numCols;

        const float colSpacing = stageWidth / (float)(numCols + 1);
        const float rowSpacing = stageDepth / (float)(numRows + 1);

        x = colSpacing * (col + 1);
        y = rowSpacing * (row + 1);
        z = 0.0f;
    }

    //==========================================================================
    // Reverb Channel Defaults
    //==========================================================================

    // Reverb > Channel
    inline juce::String getDefaultReverbName (int index) { return "Reverb " + juce::String (index + 1); }

    constexpr float reverbAttenuationDefault     = 0.0f;
    constexpr float reverbAttenuationMin         = -92.0f;
    constexpr float reverbAttenuationMax         = 0.0f;

    constexpr float reverbDelayLatencyDefault    = 0.0f;
    constexpr float reverbDelayLatencyMin        = -100.0f;
    constexpr float reverbDelayLatencyMax        = 100.0f;

    // Reverb > Position
    constexpr float reverbPositionDefault        = 0.0f;
    constexpr float reverbPositionMin            = -50.0f;
    constexpr float reverbPositionMax            = 50.0f;

    constexpr float reverbReturnOffsetDefault    = 0.0f;
    constexpr float reverbReturnOffsetMin        = -50.0f;
    constexpr float reverbReturnOffsetMax        = 50.0f;

    // Reverb > Feed
    constexpr int reverbOrientationDefault       = 0;
    constexpr int reverbOrientationMin           = -179;
    constexpr int reverbOrientationMax           = 180;

    constexpr int reverbAngleOnDefault           = 86;
    constexpr int reverbAngleOnMin               = 1;
    constexpr int reverbAngleOnMax               = 180;

    constexpr int reverbAngleOffDefault          = 90;
    constexpr int reverbAngleOffMin              = 0;
    constexpr int reverbAngleOffMax              = 179;

    constexpr int reverbPitchDefault             = 0;
    constexpr int reverbPitchMin                 = -90;
    constexpr int reverbPitchMax                 = 90;

    constexpr float reverbHFdampingDefault       = 0.0f;
    constexpr float reverbHFdampingMin           = -6.0f;
    constexpr float reverbHFdampingMax           = 0.0f;

    constexpr int reverbMiniLatencyEnableDefault = 1;   // 0=DISABLE, 1=ENABLE
    constexpr int reverbLSenableDefault          = 1;   // 0=DISABLE, 1=ENABLE

    constexpr int reverbDistanceAttenEnableDefault = 100;
    constexpr int reverbDistanceAttenEnableMin   = 0;
    constexpr int reverbDistanceAttenEnableMax   = 200;

    // Reverb > EQ (4 bands)
    constexpr int reverbEQenableDefault          = 1;   // 0=EQ OFF, 1=EQ ON

    constexpr int reverbEQshapeDefault           = 0;   // 0=OFF, 1=LowCut, 2=LowShelf, 3=Peak/Notch, 4=HighShelf, 5=HighCut
    constexpr int reverbEQshapeMin               = 0;
    constexpr int reverbEQshapeMax               = 5;

    constexpr int reverbEQfreqDefault            = 1000;
    constexpr int reverbEQfreqMin                = 20;
    constexpr int reverbEQfreqMax                = 20000;

    constexpr float reverbEQgainDefault          = 0.0f;
    constexpr float reverbEQgainMin              = -24.0f;
    constexpr float reverbEQgainMax              = 24.0f;

    constexpr float reverbEQqDefault             = 0.7f;
    constexpr float reverbEQqMin                 = 0.1f;
    constexpr float reverbEQqMax                 = 20.0f;

    constexpr float reverbEQslopeDefault         = 0.7f;
    constexpr float reverbEQslopeMin             = 0.1f;
    constexpr float reverbEQslopeMax             = 20.0f;

    // Default EQ band frequencies for reverb (4 bands)
    inline const int reverbEQBandFrequencies[4] = { 200, 800, 2000, 5000 };
    inline const int reverbEQBandShapes[4] = { 0, 0, 0, 0 };  // All OFF by default

    // Reverb > Return
    constexpr float reverbDistanceAttenuationDefault = -0.7f;
    constexpr float reverbDistanceAttenuationMin = -6.0f;
    constexpr float reverbDistanceAttenuationMax = 0.0f;

    constexpr int reverbCommonAttenDefault       = 100;
    constexpr int reverbCommonAttenMin           = 0;
    constexpr int reverbCommonAttenMax           = 100;

    constexpr int reverbMuteMacroDefault         = 0;   // 0=Mute Macro Select (no action)

} // namespace WFSParameterDefaults
