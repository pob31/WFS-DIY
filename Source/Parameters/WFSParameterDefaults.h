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
    constexpr int maxOutputChannels    = 128;

    // Effects channels. The count is part of the render-source arithmetic
    // below: an effect return IS a WFS render source, appended after the input
    // slots and the derived stereo slices (spatcore::wfs::RenderSourceMap). The
    // mirrors of spatcore::effects::kMaxEffectChannels and kNumModuleSlots are
    // asserted in a .cpp (the WFSCalculationEngine.cpp pattern) rather than
    // here, so that spatcore/effects/EffectsTypes.h is not pulled into this very
    // widely included header.
    constexpr int maxEffectChannels     = 32;

    // Renderer source dimension. A stereo-pair input channel renders as 6 WFS
    // sources (its primary slot + 5 derived slices appended past the visible
    // inputs), and every effect channel adds one return source after those, so
    // the matrix row budget exceeds the input-channel budget twice over:
    //   maxInputRenderSources = 64 + 8 * 5 = 104   (inputs and their slices)
    //   maxRenderSources      = 104 + 32   = 136   (plus the effect returns)
    // These mirror spatcore::wfs::RenderSourceMap's kMaxInputRenderSources and
    // kMaxRenderSourceSlots - the SLOT count, i.e. the size of its descriptor
    // array, not its kMaxRenderSources alias, which still names the input-only
    // budget. WFSCalculationEngine.cpp static_asserts the two sets are equal.
    // Every per-source array in the app (the calculation engine's matrices, the
    // Live Source Tamer rows, the meter taps) is sized from maxRenderSources;
    // rows past the live count stay zero.
    constexpr int maxStereoChannels       = 8;
    constexpr int derivedSlicesPerStereo  = 5;
    constexpr int maxInputRenderSources   = maxInputChannels
                                          + maxStereoChannels * derivedSlicesPerStereo;  // 104
    constexpr int maxRenderSources        = maxInputRenderSources + maxEffectChannels;    // 136
    constexpr int maxReverbChannels    = 32;
    constexpr int maxNetworkTargets    = 6;
    constexpr int maxClusters          = 10;
    constexpr int numEQBands           = 6;
    constexpr int numReverbPreEQBands  = 4;

    constexpr int numEffectEQBands      = 6;   // <Band id="1".."6"> under FxEq1 / FxEq2
    constexpr int numEffectEqInstances  = 2;   // FxEq1, FxEq2
    constexpr int numEffectDynInstances = 2;   // FxDyn1, FxDyn2
    constexpr int numEffectDelayTaps    = 8;   // <Tap id="1".."8"> under FxDelay
    constexpr int numEffectModuleSlots  = 11;  // the eleven chain slots

    /** Wrap an LFO phase (degrees) into [-180, 180]. Values already in range
        pass through untouched, so both typed endpoints (-180 and 180) are
        preserved; out-of-range values wrap to the equivalent angle in
        [-180, 180) — e.g. 270 -> -90, 360 -> 0, -270 -> 90. */
    constexpr int wrapPhaseDegrees (int deg)
    {
        if (deg >= -180 && deg <= 180)
            return deg;
        return ((deg + 180) % 360 + 360) % 360 - 180;
    }

    /** Wrap a bearing (degrees) into the (-179, 180] range the axis and rotation
        parameters actually accept.

        wrapPhaseDegrees keeps both typed endpoints because an LFO phase of -180
        is a legitimate value; inputStereoAxisOffset's range starts at -179, so
        the same helper hands the ValueTree a -180 that gets clamped straight
        back to -179. A 1 degree decrement from -179 then never crosses into
        +180, it sticks, which is what a Space Mouse twist at the boundary felt
        like. -180 and +180 name the same bearing, so folding one onto the other
        loses nothing. */
    constexpr int wrapAxisDegrees (int deg)
    {
        const int wrapped = wrapPhaseDegrees (deg);
        return wrapped == -180 ? 180 : wrapped;
    }

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
    constexpr int effectChannelsDefault     = 0;   // a show that uses no effects is unchanged
    constexpr int algorithmDSPDefault       = 0;  // 0=Input Buffer CPU, 1=Output Buffer CPU
    constexpr int algorithmDSPMin           = 0;
    constexpr int algorithmDSPMax           = 1;
    constexpr int runDSPDefault             = 0;  // 0=stopped, 1=running

    //==========================================================================
    // Config > Binaural Section
    //==========================================================================

    constexpr bool binauralEnabledDefault           = false;  // Processing off by default
    constexpr int binauralSoloModeDefault           = 0;      // 0=Single, 1=Multi
    constexpr int binauralOutputChannelDefault      = -1;     // -1=disabled
    constexpr int binauralOutputChannelMin          = -1;
    constexpr int binauralOutputChannelMax          = 126;    // Max is numOutputs-2

    constexpr float binauralListenerDistanceDefault = 5.0f;   // meters from origin (0.0 for inward circle + circular stage)
    constexpr float binauralListenerDistanceMin     = 0.0f;
    constexpr float binauralListenerDistanceMax     = 10.0f;

    constexpr int binauralListenerAngleDefault      = 0;      // degrees (0=facing origin)
    constexpr int binauralListenerAngleMin          = -180;
    constexpr int binauralListenerAngleMax          = 180;

    constexpr float binauralAttenuationDefault      = 0.0f;   // dB
    constexpr float binauralAttenuationMin          = -40.0f;
    constexpr float binauralAttenuationMax          = 0.0f;

    constexpr float binauralDelayDefault            = 0.0f;   // ms
    constexpr float binauralDelayMin                = 0.0f;
    constexpr float binauralDelayMax                = 100.0f;

    constexpr int binauralRenderModeDefault         = 0;      // 0=ORTF legacy, 1=Structural HRTF, 2=SOFA
    constexpr int binauralRenderModeMin             = 0;
    constexpr int binauralRenderModeMax             = 2;

    constexpr float binauralHeadRadiusDefault       = 0.0875f; // meters (average adult head)
    constexpr float binauralHeadRadiusMin           = 0.06f;
    constexpr float binauralHeadRadiusMax           = 0.12f;

    constexpr float binauralListenerXDefault        = 0.0f;   // meters, lateral offset
    constexpr float binauralListenerXMin            = -10.0f;
    constexpr float binauralListenerXMax            = 10.0f;

    constexpr float binauralListenerHeightDefault   = 1.5f;   // meters (matches legacy hardcoded ear height)
    constexpr float binauralListenerHeightMin       = 0.5f;
    constexpr float binauralListenerHeightMax       = 3.0f;

    constexpr float binauralListenerYawDefault      = 0.0f;   // degrees, offset from facing-origin
    constexpr float binauralListenerYawMin          = -180.0f;
    constexpr float binauralListenerYawMax          = 180.0f;

    constexpr float binauralListenerPitchDefault    = 0.0f;
    constexpr float binauralListenerPitchMin        = -89.0f;
    constexpr float binauralListenerPitchMax        = 89.0f;

    constexpr float binauralListenerRollDefault     = 0.0f;
    constexpr float binauralListenerRollMin         = -90.0f;
    constexpr float binauralListenerRollMax         = 90.0f;

    constexpr float binauralReverbAttenuationDefault = 0.0f;  // dB, headphone reverb balance
    constexpr float binauralReverbAttenuationMin    = -40.0f;
    constexpr float binauralReverbAttenuationMax    = 12.0f;

    // Fixed binaural constants (not user-adjustable)
    constexpr float binauralSpeakerSpacing          = 0.20f;  // 20cm total (+-10cm from center)
    constexpr float binauralSpeakerAngle            = 45.0f;  // degrees from front-facing
    constexpr int binauralOnAngle                   = 70;     // degrees - full coverage zone
    constexpr int binauralOffAngle                  = 1;      // degrees - mute zone
    constexpr float binauralHFShelfPerMeter         = -0.3f;  // dB/m

    //==========================================================================
    // Config > Stage Section
    //==========================================================================

    constexpr int stageShapeDefault         = 0;       // 0=box, 1=cylinder, 2=dome
    constexpr int stageShapeMin             = 0;
    constexpr int stageShapeMax             = 2;

    constexpr float stageWidthDefault       = 20.0f;
    constexpr float stageWidthMin           = 0.0f;
    constexpr float stageWidthMax           = 100.0f;

    constexpr float stageDepthDefault       = 10.0f;
    constexpr float stageDepthMin           = 0.0f;
    constexpr float stageDepthMax           = 100.0f;

    constexpr float stageHeightDefault      = 8.0f;
    constexpr float stageHeightMin          = 0.0f;
    constexpr float stageHeightMax          = 100.0f;

    constexpr float stageDiameterDefault    = 20.0f;   // For cylinder/dome (meters)
    constexpr float stageDiameterMin        = 0.1f;    // Minimum 0.1m
    constexpr float stageDiameterMax        = 100.0f;

    constexpr float domeElevationDefault    = 180.0f;  // Hemisphere (degrees)
    constexpr float domeElevationMin        = 1.0f;    // Minimum 1 degree
    constexpr float domeElevationMax        = 360.0f;  // Full sphere max

    constexpr float originWidthDefault      = 0.0f;   // Center-referenced (0 = center)
    constexpr float originWidthMin          = -100.0f;
    constexpr float originWidthMax          = 200.0f;

    constexpr float originDepthDefault      = -5.0f;  // Downstage center (-stageDepth/2)
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

    // Native GPU async pipeline depth in blocks (added latency = depth x blockSize/sr,
    // pre-subtracted from WFS delays). Field data (M4 Pro, 2026-06): desktop-compositor
    // transients stall GPU dispatch by 3-5.3 ms; depth 4 @ 48 kHz/128 = 10.7 ms cushion
    // (~2x margin). Raw Metal recovers ~7x faster than the SDK prototype measured.
    constexpr int gpuPipelineDepthDefault   = 4;
    constexpr int gpuPipelineDepthMin       = 1;
    constexpr int gpuPipelineDepthMax       = 8;

    //==========================================================================
    // Config > UI Section
    //==========================================================================

    constexpr bool streamDeckEnabledDefault = true;  // Enabled by default (backward compatible)

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

    constexpr int   admCartMappingCount         = 4;
    constexpr int   admPolarMappingCount        = 4;
    constexpr int   admMappingCount             = 8;  // 4 cart + 4 polar

    // Cartesian mapping per-axis defaults
    constexpr int   admCartSignFlipDefault      = 0;
    constexpr float admCartCenterOffsetDefault  = 0.0f;
    constexpr float admCartCenterOffsetMin      = -50.0f;
    constexpr float admCartCenterOffsetMax      = 50.0f;
    constexpr float admCartBreakpointDefault    = 0.5f;
    constexpr float admCartBreakpointMin        = 0.01f;
    constexpr float admCartBreakpointMax        = 0.99f;
    constexpr float admCartWidthDefault         = 5.0f;   // meters
    constexpr float admCartWidthMin             = 0.1f;
    constexpr float admCartWidthMax             = 50.0f;

    // Polar mapping defaults
    constexpr float admPolarAzimuthOffsetDefault  = 0.0f;
    constexpr float admPolarAzimuthOffsetMin      = -180.0f;
    constexpr float admPolarAzimuthOffsetMax      = 180.0f;
    constexpr int   admPolarAzimuthFlipDefault    = 0;
    constexpr int   admPolarElevationFlipDefault  = 0;
    constexpr float admPolarDistMinDefault        = 0.0f;   // legacy
    constexpr float admPolarDistMinMin            = 0.0f;
    constexpr float admPolarDistMinMax            = 50.0f;
    constexpr float admPolarDistMaxDefault        = 10.0f;  // legacy
    constexpr float admPolarDistMaxMin            = 0.1f;
    constexpr float admPolarDistMaxMax            = 100.0f;
    constexpr float admPolarDistBreakpointDefault = 0.5f;
    constexpr float admPolarDistInnerDefault      = 5.0f;   // meters
    constexpr float admPolarDistOuterDefault      = 5.0f;   // meters
    constexpr float admPolarDistCenterDefault     = 0.0f;   // meters

    // Per-input ADM mapping assignment
    constexpr int   inputAdmMappingDefault        = -1;  // -1=none, 0-3=Cart, 4-7=Polar

    //==========================================================================
    // Config > Tracking Section
    //==========================================================================

    constexpr int trackingEnabledDefault    = 0;  // 0=OFF, 1=ON
    constexpr int trackingProtocolDefault   = 0;  // 0=DISABLED, 1=OSC, 2=PSN, 3=RTTrP, 4=MQTT
    constexpr int trackingProtocolMin       = 0;
    constexpr int trackingProtocolMax       = 4;
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

    constexpr int clusterReferenceModeDefault = 0;  // 0=First Input, 1=Barycenter, 2=Shared Position
    constexpr int clusterReferenceModeMin     = 0;
    constexpr int clusterReferenceModeMax     = 2;

    const juce::String clusterInputOrderDefault = "";  // empty = numeric order fallback

    constexpr int clusterInputsVisibleDefault = 1;  // 1=Show, 0=Hide (assigned inputs on Map)

    // Cluster > LFO Defaults
    constexpr int   clusterLFOactiveDefault         = 0;
    constexpr float clusterLFOperiodDefault         = 5.0f;
    constexpr float clusterLFOperiodMin             = 0.01f;
    constexpr float clusterLFOperiodMax             = 100.0f;
    constexpr int   clusterLFOphaseDefault          = 0;
    constexpr int   clusterLFOphaseMin              = -180;
    constexpr int   clusterLFOphaseMax              = 180;
    constexpr int   clusterLFOshapeDefault          = 0;       // Off
    constexpr float clusterLFOrateDefault           = 1.0f;
    constexpr float clusterLFOrateMin               = 0.01f;
    constexpr float clusterLFOrateMax               = 100.0f;
    constexpr float clusterLFOamplitudeXYZDefault   = 1.0f;    // meters
    constexpr float clusterLFOamplitudeXYZMin       = 0.0f;
    constexpr float clusterLFOamplitudeXYZMax       = 50.0f;
    constexpr int   clusterLFOamplitudeRotDefault   = 0;       // degrees
    constexpr int   clusterLFOamplitudeRotMin       = -360;
    constexpr int   clusterLFOamplitudeRotMax       = 360;
    constexpr float clusterLFOamplitudeScaleDefault = 1.0f;    // log center (no modulation)
    constexpr float clusterLFOamplitudeScaleMin     = 0.1f;
    constexpr float clusterLFOamplitudeScaleMax     = 10.0f;

    //==========================================================================
    // Cluster LFO Preset Defaults
    //==========================================================================
    constexpr int maxClusterLFOPresets = 16;

    //==========================================================================
    // Input Channel Defaults
    //==========================================================================

    // Input > Channel
    // Both default names are hard-coded English on purpose. They are persisted
    // into the ValueTree and into project files, so a localised default would
    // silently rewrite a stored name when the user switches language, and the
    // "is this name still a default?" test (resequenceDefaultInputNames) would
    // stop recognising names written under another language.

    /** Legacy default name (0-based index). Kept because stored sessions still
        carry "Input N" names that must be recognised as untouched defaults. */
    inline juce::String getDefaultInputName (int index) { return "Input " + juce::String (index + 1); }

    /** Default name of a channel of a given type. `ordinal` is 1-BASED and
        counts within that type only: monos 1,2,3... and stereos 1,2...
        independently, in display order. */
    inline juce::String getDefaultInputNameForType (bool stereo, int ordinal)
    {
        return (stereo ? "Stereo " : "Mono ") + juce::String (ordinal);
    }

    constexpr float inputAttenuationDefault     = 0.0f;
    constexpr float inputAttenuationMin         = -92.0f;
    constexpr float inputAttenuationMax         = 0.0f;

    // Per-channel marker colour: 24-bit RGB as a positive int, or -1 = auto (derive from the
    // channel number). See WFSParameterIDs::inputColour for why alpha is not stored. The
    // Default/Min/Max suffixes are load-bearing: OSCParameterBounds' BIND_I macro token-pastes
    // inputColourMin / inputColourMax onto the identifier name.
    constexpr int inputColourDefault            = -1;
    constexpr int inputColourMin                = -1;
    constexpr int inputColourMax                = 0xFFFFFF;   // 16777215

    // Per-channel input type values (Input node property `inputChannelType`).
    constexpr const char* inputChannelTypeMono   = "mono";
    constexpr const char* inputChannelTypeStereo = "stereo";

    // Stereo input count (config-level, System Config): the LAST
    // stereoInputChannels of the input list are stereo pairs, the rest mono.
    // A config-level split (not a per-channel type) so a stereo channel's two
    // patch columns are reserved from the start — no stealing a neighbouring
    // channel's hardware feed mid-project. Pre-stereo configs read as all-mono
    // (default 0). Changing it resizes the renderer: stopped-only, never in
    // snapshots (configuration, not show state).
    constexpr int stereoInputChannelsDefault    = 0;
    constexpr int stereoInputChannelsMin        = 0;
    constexpr int stereoInputChannelsMax        = maxStereoChannels;

    // Stereo image, per stereo pair. Width is the FULL left-to-right distance in
    // METRES, so each leg sits at half of it either side of the stored centre.
    // Absolute rather than a fraction of the array's extent: an extent-relative
    // reference is X-only, so it spreads ±R everywhere on a circular array and
    // does nothing at any setting on an array running along Y. 0 m collapses the
    // pair to a point source (the null-test condition).
    constexpr float inputStereoWidthDefault      = 4.0f;
    constexpr float inputStereoWidthMin          = 0.0f;
    constexpr float inputStereoWidthMax          = 50.0f;

    // Rotation applied to the automatic tangential axis, in degrees, positive
    // counter-clockwise viewed from above (the inputRotation convention).
    // 0 = automatic; ±180 is an explicit L/R swap.
    constexpr int inputStereoAxisOffsetDefault   = 0;
    constexpr int inputStereoAxisOffsetMin       = -179;
    constexpr int inputStereoAxisOffsetMax       = 180;

    // Drops the automatic tangential term entirely: with the lock on, the pair
    // spreads along the fixed world axis (house left/right) turned by
    // inputStereoAxisOffset, so the offset reads as an absolute bearing instead
    // of a rotation applied to something that moves with the source.
    constexpr int inputStereoAxisLockDefault     = 0;
    constexpr int inputStereoAxisLockMin         = 0;
    constexpr int inputStereoAxisLockMax         = 1;

    constexpr float inputDelayLatencyDefault    = 0.0f;
    constexpr float inputDelayLatencyMin        = -100.0f;
    constexpr float inputDelayLatencyMax        = 100.0f;

    constexpr int inputMinimalLatencyDefault    = 0;  // 0=Acoustic Precedence, 1=Minimal Latency

    // Input > Position (X/Y/Z bidirectional in stage coordinates)
    constexpr float inputPositionMin            = -50.0f;
    constexpr float inputPositionMax            = 50.0f;

    // Default working height for a new input: roughly a standing performer's
    // head. Absolute, not origin-relative — the same convention as
    // ReverbNodePlacement::kDefaultHeight, and why getDefaultInputPosition
    // ignores originH while it does subtract originW/originD from X/Y.
    constexpr float inputPositionZDefault       = 1.5f;

    constexpr float inputOffsetDefault          = 0.0f;
    constexpr float inputOffsetMin              = -50.0f;
    constexpr float inputOffsetMax              = 50.0f;

    constexpr int inputConstraintDefault        = 1;  // 0=Free, 1=Constrained
    constexpr int inputConstraintDistanceDefault        = 0;      // 0=OFF, 1=ON (for Cylindrical/Spherical modes)
    constexpr float inputConstraintDistanceMinDefault   = 0.0f;   // meters
    constexpr float inputConstraintDistanceMaxDefault   = 50.0f;  // meters
    constexpr int inputFlipDefault              = 0;  // 0=Normal, 1=Flipped
    constexpr int inputClusterDefault           = 0;  // 0=Single, 1-10=Cluster 1-10
    constexpr int inputClusterMin               = 0;
    constexpr int inputClusterMax               = 10;

    constexpr int inputTrackingActiveDefault    = 0;  // 0=OFF, 1=ON
    constexpr int inputTrackingIDMin            = 1;
    constexpr int inputTrackingIDMax            = 32;
    constexpr int inputTrackingSmoothDefault    = 100;
    constexpr int inputTrackingSmoothMin        = 0;
    constexpr int inputTrackingSmoothMax        = 100;

    constexpr int inputMaxSpeedActiveDefault    = 0;  // 0=OFF, 1=ON
    constexpr float inputMaxSpeedDefault        = 1.0f;
    constexpr float inputMaxSpeedMin            = 0.01f;
    constexpr float inputMaxSpeedMax            = 20.0f;

    constexpr int inputPathModeActiveDefault    = 0;  // 0=OFF, 1=ON

    constexpr int inputHeightFactorDefault      = 100;
    constexpr int inputHeightFactorMin          = 0;
    constexpr int inputHeightFactorMax          = 100;

    // Input > Coordinate Mode (0=Cartesian, 1=Cylindrical, 2=Spherical)
    constexpr int inputCoordinateModeDefault    = 0;
    constexpr int inputCoordinateModeMin        = 0;
    constexpr int inputCoordinateModeMax        = 2;

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

    constexpr int inputLSpeakEnableDefault       = 0;  // OFF by default

    constexpr float inputLSpeakThresholdDefault = -20.0f;
    constexpr float inputLSpeakThresholdMin     = -48.0f;
    constexpr float inputLSpeakThresholdMax     = 0.0f;

    constexpr float inputLSpeakRatioDefault     = 2.0f;
    constexpr float inputLSpeakRatioMin         = 1.0f;
    constexpr float inputLSpeakRatioMax         = 10.0f;

    constexpr int inputLSslowEnableDefault       = 0;  // OFF by default

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

    constexpr int inputFRdiffusionDefault       = 40;
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
    constexpr int inputLFOphaseMin              = -180;
    constexpr int inputLFOphaseMax              = 180;

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

    // Duration (how long the movement takes)
    constexpr float inputOtomoDurationDefault   = 5.0f;    // 5 seconds
    constexpr float inputOtomoDurationMin       = 0.1f;    // 100ms minimum
    constexpr float inputOtomoDurationMax       = 3600.0f; // 1 hour maximum

    // Curve (path bending perpendicular to direction of travel in XY plane)
    constexpr int inputOtomoCurveDefault        = 0;       // Straight path
    constexpr int inputOtomoCurveMin            = -100;    // Full left bend
    constexpr int inputOtomoCurveMax            = 100;     // Full right bend

    constexpr int inputOtomoTriggerDefault      = 0;  // 0=Manual, 1=Trigger
    constexpr float inputOtomoThresholdDefault  = -20.0f;
    constexpr float inputOtomoThresholdMin      = -92.0f;
    constexpr float inputOtomoThresholdMax      = 0.0f;

    constexpr float inputOtomoResetDefault      = -60.0f;
    constexpr float inputOtomoResetMin          = -92.0f;
    constexpr float inputOtomoResetMax          = 0.0f;

    constexpr int inputOtomoPauseResumeDefault  = 1;  // 0=Paused, 1=Resume

    // Input > AutomOtion (Polar coordinates)
    constexpr int inputOtomoCoordinateModeDefault = 0;  // 0=Cartesian, 1=Cylindrical, 2=Spherical

    constexpr float inputOtomoRDefault     = 0.0f;
    constexpr float inputOtomoRMin         = 0.0f;
    constexpr float inputOtomoRMax         = 50.0f;

    constexpr float inputOtomoThetaDefault = 0.0f;
    constexpr float inputOtomoThetaMin     = -3600.0f;  // 10 full rotations
    constexpr float inputOtomoThetaMax     = 3600.0f;

    constexpr float inputOtomoRsphDefault  = 0.0f;
    constexpr float inputOtomoRsphMin      = 0.0f;
    constexpr float inputOtomoRsphMax      = 50.0f;

    constexpr float inputOtomoPhiDefault   = 0.0f;
    constexpr float inputOtomoPhiMin       = -3600.0f;  // 10 full rotations
    constexpr float inputOtomoPhiMax       = 3600.0f;

    // Input > Sidelines (auto-mute at stage edges)
    constexpr int inputSidelinesActiveDefault    = 0;      // OFF by default
    constexpr float inputSidelinesFringeDefault  = 1.0f;   // 1 meter
    constexpr float inputSidelinesFringeMin      = 0.1f;   // 10cm minimum
    constexpr float inputSidelinesFringeMax      = 10.0f;  // 10m maximum

    // Input > Array Attenuation
    constexpr float inputArrayAttenDefault      = 0.0f;   // 0 dB (no attenuation)
    constexpr float inputArrayAttenMin          = -60.0f; // -60 dB
    constexpr float inputArrayAttenMax          = 0.0f;   // 0 dB

    //==========================================================================
    // Input > Gradient Maps Defaults
    //==========================================================================

    constexpr int maxGradientLayers             = 3;

    // Layer defaults
    constexpr int gmLayerEnabledDefault         = 0;       // OFF
    constexpr int gmLayerParamDefault           = 0;       // 0=Attenuation
    constexpr int gmLayerParamMin               = 0;
    constexpr int gmLayerParamMax               = 2;       // 0=Atten, 1=Height, 2=HF
    constexpr float gmLayerWhiteDefault         = 0.0f;    // No offset at white
    constexpr float gmLayerBlackDefault         = 0.0f;    // No offset at black
    constexpr float gmLayerCurveDefault         = 0.0f;    // Linear mapping
    constexpr float gmLayerCurveMin             = -1.0f;
    constexpr float gmLayerCurveMax             = 1.0f;
    constexpr int gmLayerVisibleDefault         = 1;       // Visible by default

    // Layer white/black ranges per parameter type
    constexpr float gmAttenWhiteMin             = -92.0f;  // dB
    constexpr float gmAttenWhiteMax             = 0.0f;
    constexpr float gmAttenBlackMin             = -92.0f;
    constexpr float gmAttenBlackMax             = 0.0f;

    constexpr float gmHeightWhiteMin            = -20.0f;  // meters
    constexpr float gmHeightWhiteMax            = 20.0f;
    constexpr float gmHeightBlackMin            = -20.0f;
    constexpr float gmHeightBlackMax            = 20.0f;

    constexpr float gmHFShelfWhiteMin           = -24.0f;  // dB
    constexpr float gmHFShelfWhiteMax           = 0.0f;
    constexpr float gmHFShelfBlackMin           = -24.0f;
    constexpr float gmHFShelfBlackMax           = 0.0f;

    // Shape defaults
    constexpr int gmShapeTypeDefault            = 0;       // Rectangle
    constexpr int gmShapeTypeMin                = 0;
    constexpr int gmShapeTypeMax                = 2;       // 0=Rect, 1=Ellipse, 2=Polygon
    constexpr float gmShapePosDefault           = 0.0f;    // meters
    constexpr float gmShapeRotationDefault      = 0.0f;    // degrees
    constexpr float gmShapeScaleXDefault        = 1.0f;    // 1m half-extent
    constexpr float gmShapeScaleYDefault        = 1.0f;
    constexpr int gmShapeFillTypeDefault        = 0;       // Uniform
    constexpr int gmShapeFillTypeMin            = 0;
    constexpr int gmShapeFillTypeMax            = 2;       // 0=Uniform, 1=Linear, 2=Radial
    constexpr float gmShapeFillValueDefault     = 1.0f;    // White (no effect)
    constexpr float gmShapeFillValueMin         = 0.0f;
    constexpr float gmShapeFillValueMax         = 1.0f;
    constexpr float gmShapeBlurDefault          = 0.0f;    // Sharp edges
    constexpr float gmShapeBlurMin              = 0.0f;
    constexpr float gmShapeBlurMax              = 5.0f;    // 5m max blur
    constexpr int gmShapeLockedDefault          = 0;       // Unlocked
    constexpr int gmShapeOrderDefault           = 0;
    constexpr int gmShapeEnabledDefault         = 1;       // Enabled by default

    // Bitmap rasterization
    constexpr float gmBitmapPixelsPerMeter      = 10.0f;   // 10 px/m resolution

    //==========================================================================
    // Output Channel Defaults
    //==========================================================================

    // Output > Channel
    inline juce::String getDefaultOutputName (int index) { return "Output " + juce::String (index + 1); }

    constexpr int outputArrayDefault            = 0;  // 0=Single, 1-10=Array 1-10
    constexpr int outputArrayMin                = 0;
    constexpr int outputArrayMax                = 10;

    constexpr int outputApplyToArrayDefault     = 1;  // 0=OFF, 1=ABSOLUTE, 2=RELATIVE

    constexpr float outputAttenuationDefault    = 0.0f;
    constexpr float outputAttenuationMin        = -92.0f;
    constexpr float outputAttenuationMax        = 0.0f;

    constexpr float outputDelayLatencyDefault   = 0.0f;
    constexpr float outputDelayLatencyMin       = -100.0f;
    constexpr float outputDelayLatencyMax       = 100.0f;

    // Output > Position
    constexpr float outputPositionDefault       = 0.0f;
    constexpr float outputPositionMin           = -50.0f;
    constexpr float outputPositionMax           = 50.0f;

    constexpr int outputOrientationDefault      = 0;  // Degrees
    constexpr int outputOrientationMin          = -180;
    constexpr int outputOrientationMax          = 180;

    constexpr int outputAngleOnDefault          = 86;  // Degrees
    constexpr int outputAngleOnMin              = 1;
    constexpr int outputAngleOnMax              = 180;

    constexpr int outputAngleOffDefault         = 90;  // Degrees
    constexpr int outputAngleOffMin             = 0;
    constexpr int outputAngleOffMax             = 179;

    constexpr int outputPitchDefault            = 0;  // Degrees
    constexpr int outputPitchMin                = -90;
    constexpr int outputPitchMax                = 90;

    constexpr float outputHFdampingDefault      = 0.0f;
    constexpr float outputHFdampingMin          = -6.0f;
    constexpr float outputHFdampingMax          = 0.0f;

    // Output > Coordinate Mode (0=Cartesian, 1=Cylindrical, 2=Spherical)
    constexpr int outputCoordinateModeDefault   = 0;
    constexpr int outputCoordinateModeMin       = 0;
    constexpr int outputCoordinateModeMax       = 2;

    // Output > Options
    constexpr int outputMiniLatencyEnableDefault = 1;    // 0=OFF, 1=ON
    constexpr int outputLSattenEnableDefault    = 1;     // 0=OFF, 1=ON
    constexpr int outputFRenableDefault         = 1;     // 0=OFF, 1=ON (Floor Reflections)

    constexpr int outputDistanceAttenPercentDefault = 100;
    constexpr int outputDistanceAttenPercentMin = 0;
    constexpr int outputDistanceAttenPercentMax = 200;

    // Parallax: separate ranges for horizontal vs vertical (CSV-canonical).
    constexpr float outputHparallaxDefault      = 0.0f;
    constexpr float outputHparallaxMin          = 0.0f;
    constexpr float outputHparallaxMax          = 50.0f;

    constexpr float outputVparallaxDefault      = 0.0f;
    constexpr float outputVparallaxMin          = -50.0f;
    constexpr float outputVparallaxMax          = 50.0f;

    // Output > EQ
    constexpr int outputEQenabledDefault        = 0;  // 0=OFF, 1=ON

    constexpr int eqShapeDefault                = 3;  // 0=Off, 1=LowCut, 2=LowShelf, 3=Peak, 4=BandPass, 5=HighShelf, 6=HighCut, 7=AllPass
    constexpr int eqShapeMin                    = 0;
    constexpr int eqShapeMax                    = 7;

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
    inline const int eqBandShapes[6] = { 0, 2, 3, 3, 5, 0 };  // Off(LowCut), LowShelf, Peak, Peak, HighShelf, Off(HighCut)
    inline const int eqBandComboDefaults[6] = { 1, 2, 3, 3, 5, 6 };  // LowCut, LowShelf, Peak, Peak, HighShelf, HighCut

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
     * Inputs are arranged in rows of up to 8, distributed within stage bounds.
     * Returns origin-relative coordinates (center-referenced for X/Y).
     */
    inline void getDefaultInputPosition (int index, int totalInputs,
                                         float stageWidth, float stageDepth, float /*stageHeight*/,
                                         float originW, float originD, float /*originH*/,
                                         float& x, float& y, float& z)
    {
        const int numCols = juce::jmin (8, totalInputs);
        const int numRows = (totalInputs + 7) / 8;
        const int col = index % numCols;
        const int row = index / numCols;

        // Calculate position as fraction across stage (0 to 1)
        float fracX = (col + 1) / (float)(numCols + 1);
        float fracY = (row + 1) / (float)(numRows + 1);

        // Stage bounds in origin-relative coordinates (center-referenced)
        float minX = -stageWidth / 2.0f - originW;
        float minY = -stageDepth / 2.0f - originD;

        // Position within stage bounds
        x = minX + stageWidth * fracX;
        y = minY + stageDepth * fracY;
        z = inputPositionZDefault;
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

    // Reverb > Coordinate Mode (0=Cartesian, 1=Cylindrical, 2=Spherical)
    constexpr int reverbCoordinateModeDefault   = 0;
    constexpr int reverbCoordinateModeMin       = 0;
    constexpr int reverbCoordinateModeMax       = 2;

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

    constexpr int reverbDistanceAttenEnableDefault = 100;
    constexpr int reverbDistanceAttenEnableMin   = 0;
    constexpr int reverbDistanceAttenEnableMax   = 200;

    // Reverb > Pre-Processing EQ (4 bands, per-channel)
    constexpr int reverbPreEQenableDefault       = 1;   // 0=EQ OFF, 1=EQ ON

    constexpr int reverbPreEQshapeDefault        = 0;   // 0=OFF, 1=LowCut, 2=LowShelf, 3=Peak/Notch, 4=HighShelf, 5=HighCut
    constexpr int reverbPreEQshapeMin            = 0;
    constexpr int reverbPreEQshapeMax            = 6;

    constexpr int reverbPreEQfreqDefault         = 1000;
    constexpr int reverbPreEQfreqMin             = 20;
    constexpr int reverbPreEQfreqMax             = 20000;

    constexpr float reverbPreEQgainDefault       = 0.0f;
    constexpr float reverbPreEQgainMin           = -24.0f;
    constexpr float reverbPreEQgainMax           = 24.0f;

    constexpr float reverbPreEQqDefault          = 0.7f;
    constexpr float reverbPreEQqMin              = 0.1f;
    constexpr float reverbPreEQqMax              = 20.0f;

    constexpr float reverbPreEQslopeDefault      = 0.7f;
    constexpr float reverbPreEQslopeMin          = 0.1f;
    constexpr float reverbPreEQslopeMax          = 20.0f;

    // Default EQ band frequencies for reverb pre-processing (4 bands)
    inline const int reverbPreEQBandFrequencies[4] = { 200, 800, 2000, 5000 };
    inline const int reverbPreEQBandShapes[4] = { 0, 3, 3, 4 };  // Off(BandPass), Peak, Peak, HighShelf
    inline const int reverbPreEQBandComboDefaults[4] = { 6, 3, 3, 4 };  // BandPass, Peak, Peak, HighShelf

    // Reverb > Return
    constexpr float reverbDistanceAttenuationDefault = -0.7f;
    constexpr float reverbDistanceAttenuationMin = -6.0f;
    constexpr float reverbDistanceAttenuationMax = 0.0f;

    constexpr int reverbCommonAttenDefault       = 100;
    constexpr int reverbCommonAttenMin           = 0;
    constexpr int reverbCommonAttenMax           = 100;

    constexpr int reverbMuteMacroDefault         = 0;   // 0=Mute Macro Select (no action)

    //==========================================================================
    // Reverb Algorithm Defaults (global, not per-channel)
    //==========================================================================

    constexpr int reverbAlgoTypeDefault            = 0;       // 0=SDN, 1=FDN, 2=IR
    constexpr int reverbAlgoTypeMin                = 0;
    constexpr int reverbAlgoTypeMax                = 2;

    constexpr float reverbRT60Default              = 1.5f;    // seconds
    constexpr float reverbRT60Min                  = 0.2f;
    constexpr float reverbRT60Max                  = 8.0f;

    constexpr float reverbRT60LowMultDefault       = 1.3f;    // multiplier
    constexpr float reverbRT60LowMultMin           = 0.1f;
    constexpr float reverbRT60LowMultMax           = 9.0f;

    constexpr float reverbRT60HighMultDefault      = 0.4f;    // multiplier
    constexpr float reverbRT60HighMultMin          = 0.1f;
    constexpr float reverbRT60HighMultMax          = 9.0f;

    constexpr float reverbCrossoverLowDefault      = 200.0f;  // Hz
    constexpr float reverbCrossoverLowMin          = 50.0f;
    constexpr float reverbCrossoverLowMax          = 500.0f;

    constexpr float reverbCrossoverHighDefault     = 2500.0f; // Hz
    constexpr float reverbCrossoverHighMin         = 1000.0f;
    constexpr float reverbCrossoverHighMax         = 10000.0f;

    constexpr float reverbDiffusionDefault         = 0.5f;
    constexpr float reverbDiffusionMin             = 0.0f;
    constexpr float reverbDiffusionMax             = 1.0f;

    constexpr float reverbSDNscaleDefault          = 1.0f;
    constexpr float reverbSDNscaleMin              = 0.5f;
    constexpr float reverbSDNscaleMax              = 4.0f;

    constexpr float reverbFDNsizeDefault           = 1.0f;
    constexpr float reverbFDNsizeMin               = 0.5f;
    constexpr float reverbFDNsizeMax               = 2.0f;

    constexpr float reverbIRtrimDefault            = 0.0f;    // ms
    constexpr float reverbIRtrimMin                = 0.0f;
    constexpr float reverbIRtrimMax                = 30000.0f; // 30s in ms (actual max is dynamic per file)

    constexpr float reverbIRlengthDefault          = 6.0f;    // seconds (full)
    constexpr float reverbIRlengthMin              = 0.1f;
    constexpr float reverbIRlengthMax              = 30.0f;   // 30s (actual max is dynamic per file)

    constexpr int reverbPerNodeIRDefault           = 0;       // 0=OFF, 1=ON

    constexpr int reverbIRGpuDefault               = 0;       // legacy 0=CPU/1=GPU (migrated to device id)
    constexpr int reverbFDNGpuDefault              = 0;       // legacy
    constexpr int reverbSDNGpuDefault              = 0;       // legacy
    constexpr const char* reverbIRGpuDeviceDefault  = "cpu";  // compute device id
    constexpr const char* reverbFDNGpuDeviceDefault = "cpu";
    constexpr const char* reverbSDNGpuDeviceDefault = "cpu";

    constexpr float reverbWetLevelDefault          = 0.0f;    // dB
    constexpr float reverbWetLevelMin              = -60.0f;  // effectively -inf
    constexpr float reverbWetLevelMax              = 12.0f;

    //==========================================================================
    // Reverb Post-Processing EQ Defaults (global, not per-channel)
    //==========================================================================

    constexpr int numReverbPostEQBands     = 4;

    constexpr int reverbPostEQenableDefault        = 1;       // 0=EQ OFF, 1=EQ ON

    constexpr int reverbPostEQshapeDefault         = 0;       // 0=OFF, 1=LowCut, 2=LowShelf, 3=Peak/Notch, 4=HighShelf, 5=HighCut
    constexpr int reverbPostEQshapeMin             = 0;
    constexpr int reverbPostEQshapeMax             = 6;

    constexpr int reverbPostEQfreqDefault          = 1000;
    constexpr int reverbPostEQfreqMin              = 20;
    constexpr int reverbPostEQfreqMax              = 20000;

    constexpr float reverbPostEQgainDefault        = 0.0f;
    constexpr float reverbPostEQgainMin            = -24.0f;
    constexpr float reverbPostEQgainMax            = 24.0f;

    constexpr float reverbPostEQqDefault           = 0.7f;
    constexpr float reverbPostEQqMin               = 0.1f;
    constexpr float reverbPostEQqMax               = 20.0f;

    constexpr float reverbPostEQslopeDefault       = 0.7f;
    constexpr float reverbPostEQslopeMin           = 0.1f;
    constexpr float reverbPostEQslopeMax           = 20.0f;

    // Default Post-EQ band frequencies (4 bands)
    inline const int reverbPostEQBandFrequencies[4] = { 200, 800, 2000, 5000 };
    inline const int reverbPostEQBandShapes[4] = { 0, 3, 3, 4 };  // Off(BandPass), Peak, Peak, HighShelf
    inline const int reverbPostEQBandComboDefaults[4] = { 6, 3, 3, 4 };  // BandPass, Peak, Peak, HighShelf

    //==========================================================================
    // Reverb Pre-Compressor Defaults (global, not per-channel)
    //==========================================================================

    constexpr int reverbPreCompBypassDefault       = 1;       // 1=bypassed (off), 0=active
    constexpr float reverbPreCompThresholdDefault   = -12.0f;  // dB
    constexpr float reverbPreCompThresholdMin       = -60.0f;
    constexpr float reverbPreCompThresholdMax       = 0.0f;
    constexpr float reverbPreCompRatioDefault       = 2.0f;    // :1
    constexpr float reverbPreCompRatioMin           = 1.0f;
    constexpr float reverbPreCompRatioMax           = 20.0f;
    constexpr float reverbPreCompAttackDefault      = 10.0f;   // ms
    constexpr float reverbPreCompAttackMin          = 0.1f;
    constexpr float reverbPreCompAttackMax          = 100.0f;
    constexpr float reverbPreCompReleaseDefault     = 100.0f;  // ms
    constexpr float reverbPreCompReleaseMin         = 10.0f;
    constexpr float reverbPreCompReleaseMax         = 1000.0f;

    //==========================================================================
    // Reverb Post-Expander Defaults (global, not per-channel)
    //==========================================================================

    constexpr int reverbPostExpBypassDefault        = 1;       // 1=bypassed (off), 0=active
    constexpr float reverbPostExpThresholdDefault    = -40.0f;  // dB
    constexpr float reverbPostExpThresholdMin        = -80.0f;
    constexpr float reverbPostExpThresholdMax        = -10.0f;
    constexpr float reverbPostExpRatioDefault        = 2.0f;    // 1:N
    constexpr float reverbPostExpRatioMin            = 1.0f;
    constexpr float reverbPostExpRatioMax            = 8.0f;
    constexpr float reverbPostExpAttackDefault       = 1.0f;    // ms
    constexpr float reverbPostExpAttackMin           = 0.1f;
    constexpr float reverbPostExpAttackMax           = 50.0f;
    constexpr float reverbPostExpReleaseDefault      = 200.0f;  // ms
    constexpr float reverbPostExpReleaseMin          = 50.0f;
    constexpr float reverbPostExpReleaseMax          = 2000.0f;

    //==========================================================================
    // Effects Channel Defaults
    //==========================================================================
    //
    // Phase 4, commit 1: DECLARATION ONLY - nothing reads these yet.
    //
    // The values are transcribed from spatcore/effects/EffectParams.h (the POD
    // structs the realtime side consumes, and the authority on WHICH parameters
    // exist and what they default to) and the ranges from
    // Documentation/effects-channels-plan.md sections 5.1-5.10 and 6.1 (the
    // authority on ranges, since the PODs carry none). Each POD field is the
    // identifier minus the "effect" prefix and minus its unit suffix, so this
    // block is checkable against EffectParams.h line by line.
    //
    // NAMING IS LOAD-BEARING: OSCParameterBounds.cpp's BIND_F/BIND_I macros
    // token-paste <identifier>Min / <identifier>Max, so a constant pair that is
    // not spelled exactly after its Identifier needs BIND_F_AS instead (the
    // reverbPosition precedent, used here for effectPosition*, effectReturnOffset*
    // and effectOtomoX/Y/Z). Plain 0/1 toggles get a Default only - BIND_BOOL
    // supplies their bounds.
    //
    // The static_asserts tying numEffectModuleSlots to
    // spatcore::effects::kNumModuleSlots and maxEffectChannels to
    // spatcore::effects::kMaxEffectChannels deliberately do NOT live here:
    // spatcore/effects/EffectsTypes.h must not be pulled into a header this
    // widely included. They go in a .cpp, the way WFSCalculationEngine.cpp does
    // for the render-source budget - and that is where they should go, NOT some
    // future file: WFSCalculationEngine.cpp:16 already includes
    // spatcore/wfs/RenderSourceMap.h, whose :108 defines kMaxEffectChannels = 32,
    // so maxEffectChannels can be pinned there with no new include at all, beside
    // the render-source asserts. BOTH ARE NOW THERE, at WFSCalculationEngine.cpp
    // (beside the render-source pair), so neither constant can drift from
    // spatcore silently. They were promised by the commit that wrote this note
    // and not delivered by it; a later audit caught the gap, which is the
    // argument for pinning a mirrored constant in the same commit that creates
    // it rather than in the next one.

    // Effect > Channel
    inline juce::String getDefaultEffectName (int index) { return "Effect " + juce::String (index + 1); }

    constexpr float effectAttenuationDefault      = 0.0f;    // dB
    constexpr float effectAttenuationMin          = -92.0f;
    constexpr float effectAttenuationMax          = 0.0f;

    constexpr float effectDelayLatencyDefault     = 0.0f;    // ms
    constexpr float effectDelayLatencyMin         = -100.0f;
    constexpr float effectDelayLatencyMax         = 100.0f;

    // NOTE THE SENSE AND THE DEFAULT. This flag sits on <Channel> like
    // inputMinimalLatency, but it does NOT inherit that constant's default:
    // inputMinimalLatencyDefault is 0 (:439, "0=Acoustic Precedence,
    // 1=Minimal Latency"), while plan 6.1 (:708) specifies 1 here, matching the
    // reverb feed flag reverbMiniLatencyEnableDefault (:927). An effect return
    // therefore ships with minimal latency ON where an input channel ships with
    // it OFF; that is deliberate, not a transcription of the input row.
    constexpr int effectMinimalLatencyDefault     = 1;       // 0=DISABLE, 1=ENABLE

    constexpr int effectLinkGroupDefault          = 0;       // 0=unlinked
    constexpr int effectLinkGroupMin              = 0;
    constexpr int effectLinkGroupMax              = 8;

    // Per-channel link mode, the mirror of outputApplyToArrayDefault (:749).
    // The constant below is only the FALLBACK: a channel is stamped with the
    // live effectsGlobalLinkMode at creation, and the global is nothing more
    // than "what a new channel gets" once this property exists (R5-5). Its
    // value matches outputApplyToArrayDefault so the two families read alike.
    constexpr int effectLinkModeDefault           = 1;       // 0=OFF, 1=ABSOLUTE, 2=RELATIVE
    constexpr int effectLinkModeMin               = 0;
    constexpr int effectLinkModeMax               = 2;

    constexpr int effectMuteDefault               = 0;
    constexpr int effectSoloDefault               = 0;

    // Effect > Position
    constexpr float effectPositionDefault         = 0.0f;    // metres; the real default is a placement
    constexpr float effectPositionMin             = -50.0f;
    constexpr float effectPositionMax             = 50.0f;

    constexpr float effectReturnOffsetDefault     = 0.0f;    // metres
    constexpr float effectReturnOffsetMin         = -50.0f;
    constexpr float effectReturnOffsetMax         = 50.0f;

    constexpr int effectCoordinateModeDefault     = 0;       // 0=Cartesian, 1=Cylindrical, 2=Spherical
    constexpr int effectCoordinateModeMin         = 0;
    constexpr int effectCoordinateModeMax         = 2;

    // Effect > Feed
    constexpr int effectOrientationDefault        = 0;       // degrees
    constexpr int effectOrientationMin            = -179;
    constexpr int effectOrientationMax            = 180;

    constexpr int effectAngleOnDefault            = 86;
    constexpr int effectAngleOnMin                = 1;
    constexpr int effectAngleOnMax                = 180;

    constexpr int effectAngleOffDefault           = 90;
    constexpr int effectAngleOffMin               = 0;
    constexpr int effectAngleOffMax               = 179;

    constexpr int effectPitchDefault              = 0;       // degrees
    constexpr int effectPitchMin                  = -90;
    constexpr int effectPitchMax                  = 90;

    constexpr float effectHFdampingDefault        = 0.0f;    // dB
    constexpr float effectHFdampingMin            = -6.0f;
    constexpr float effectHFdampingMax            = 0.0f;

    constexpr int effectFeedMiniLatencyDefault    = 1;       // 0=DISABLE, 1=ENABLE

    constexpr int effectDistanceAttenPercentDefault = 100;   // %
    constexpr int effectDistanceAttenPercentMin   = 0;
    constexpr int effectDistanceAttenPercentMax   = 200;

    // Effect > Return (ranges and defaults mirror the input channel set)
    constexpr int effectAttenuationLawDefault     = 0;       // 0=Log, 1=1/d

    constexpr float effectDistanceAttenuationDefault = -0.7f; // dB
    constexpr float effectDistanceAttenuationMin  = -6.0f;
    constexpr float effectDistanceAttenuationMax  = 0.0f;

    constexpr float effectDistanceRatioDefault    = 1.0f;
    constexpr float effectDistanceRatioMin        = 0.1f;
    constexpr float effectDistanceRatioMax        = 10.0f;

    constexpr int effectCommonAttenDefault        = 100;     // %
    constexpr int effectCommonAttenMin            = 0;
    constexpr int effectCommonAttenMax            = 100;

    constexpr float effectHFshelfDefault          = -6.0f;   // dB
    constexpr float effectHFshelfMin              = -24.0f;
    constexpr float effectHFshelfMax              = 0.0f;

    constexpr int effectMuteMacroDefault          = 0;       // 0=Mute Macro Select (no action)
    constexpr int effectMuteMacroMin              = 0;
    constexpr int effectMuteMacroMax              = 4;

    constexpr int effectMuteReverbSendsDefault    = 0;

    // Per-array trim of the return (R5-4), the mirror of inputArrayAtten* (:676).
    // Ten properties share one constant triple exactly as the input family does;
    // the array a given output belongs to is read from outputArrayAssignments at
    // matrix time, so nothing here is per-array.
    constexpr float effectArrayAttenDefault       = 0.0f;    // 0 dB (no attenuation)
    constexpr float effectArrayAttenMin           = -60.0f;
    constexpr float effectArrayAttenMax           = 0.0f;

    // effectMutes has no bounds entry on purpose: it is a packed CSV row, one
    // token per output, built at channel-creation time from the live output
    // count and refitted by setNumOutputChannels. reverbMutes is the precedent -
    // and having no bounds entry is exactly why a row needs its own clause in the
    // write interceptor: the generic numeric clamp only acts where getBounds
    // answers, so without one a bare number passes straight through and replaces
    // the whole row.

    // Effect > AutomOtion (the inputOtomo* set minus StayReturn; every value
    // matches its input constant, with one addition - effectOtomoCoordinateMode
    // carries a Min/Max pair where inputOtomoCoordinateMode (:641) has a Default
    // only, so BIND_I can bound it instead of a hand-written OSCQuery case)
    constexpr float effectOtomoDefault            = 0.0f;    // metres, X/Y/Z
    constexpr float effectOtomoMin                = -50.0f;
    constexpr float effectOtomoMax                = 50.0f;

    constexpr int effectOtomoAbsoluteRelativeDefault = 0;     // 0=Absolute, 1=Relative

    constexpr int effectOtomoSpeedProfileDefault  = 0;
    constexpr int effectOtomoSpeedProfileMin      = 0;
    constexpr int effectOtomoSpeedProfileMax      = 100;

    constexpr float effectOtomoDurationDefault    = 5.0f;    // seconds
    constexpr float effectOtomoDurationMin        = 0.1f;
    constexpr float effectOtomoDurationMax        = 3600.0f;

    constexpr int effectOtomoCurveDefault         = 0;       // straight path
    constexpr int effectOtomoCurveMin             = -100;
    constexpr int effectOtomoCurveMax             = 100;

    constexpr int effectOtomoTriggerDefault       = 0;       // 0=Manual, 1=Trigger

    constexpr float effectOtomoThresholdDefault   = -20.0f;  // dB
    constexpr float effectOtomoThresholdMin       = -92.0f;
    constexpr float effectOtomoThresholdMax       = 0.0f;

    constexpr float effectOtomoResetDefault       = -60.0f;  // dB
    constexpr float effectOtomoResetMin           = -92.0f;
    constexpr float effectOtomoResetMax           = 0.0f;

    constexpr int effectOtomoPauseResumeDefault   = 1;       // 0=Paused, 1=Resume

    constexpr int effectOtomoCoordinateModeDefault = 0;      // 0=Cartesian, 1=Cylindrical, 2=Spherical
    constexpr int effectOtomoCoordinateModeMin    = 0;
    constexpr int effectOtomoCoordinateModeMax    = 2;

    constexpr float effectOtomoRDefault           = 0.0f;    // cylindrical radius
    constexpr float effectOtomoRMin               = 0.0f;
    constexpr float effectOtomoRMax               = 50.0f;

    constexpr float effectOtomoThetaDefault       = 0.0f;    // azimuth, degrees
    constexpr float effectOtomoThetaMin           = -3600.0f;  // 10 full rotations
    constexpr float effectOtomoThetaMax           = 3600.0f;

    constexpr float effectOtomoRsphDefault        = 0.0f;    // spherical radius
    constexpr float effectOtomoRsphMin            = 0.0f;
    constexpr float effectOtomoRsphMax            = 50.0f;

    constexpr float effectOtomoPhiDefault         = 0.0f;    // elevation, degrees
    constexpr float effectOtomoPhiMin             = -3600.0f;  // 10 full rotations
    constexpr float effectOtomoPhiMax             = 3600.0f;

    // Effect > LFO. The input constants minus gyrophone; as for the inputs one
    // shape / rate / amplitude / phase constant covers the three axes.
    constexpr int   effectLFOactiveDefault        = 0;
    constexpr float effectLFOperiodDefault        = 5.0f;    // seconds
    constexpr float effectLFOperiodMin            = 0.01f;
    constexpr float effectLFOperiodMax            = 100.0f;
    constexpr int   effectLFOphaseDefault         = 0;       // degrees, canonical [-180, 180]
    constexpr int   effectLFOphaseMin             = -180;
    constexpr int   effectLFOphaseMax             = 180;
    constexpr int   effectLFOshapeDefault         = 0;       // 0=OFF,1=sine,2=square,3=sawtooth,4=triangle,5=keystone,6=log,7=exp,8=random
    constexpr int   effectLFOshapeMin             = 0;
    constexpr int   effectLFOshapeMax             = 8;
    constexpr float effectLFOrateDefault          = 1.0f;    // multiplier of the base period
    constexpr float effectLFOrateMin              = 0.01f;
    constexpr float effectLFOrateMax              = 100.0f;
    constexpr float effectLFOamplitudeDefault     = 1.0f;    // metres
    constexpr float effectLFOamplitudeMin         = 0.0f;
    constexpr float effectLFOamplitudeMax         = 50.0f;

    // Effect > Chain. The order string is a permutation of the eleven slot
    // tokens of spatcore::effects::kSlots, in their declared order - the
    // string form of spatcore::effects::kDefaultOrder.
    inline const juce::String effectChainOrderDefault = "dist,eq1,eq2,dyn1,dyn2,mod,phaser,trem,reverb,delay,crush";
    constexpr int effectChainBypassDefault        = 0;

    //--------------------------------------------------------------------------
    // Effect > FxDist (spatcore::effects::DistortionParams)
    //--------------------------------------------------------------------------

    constexpr int effectDistBypassDefault         = 1;       // every module is bypassed by default

    constexpr float effectDistDriveDefault        = 12.0f;   // dB
    constexpr float effectDistDriveMin            = 0.0f;
    constexpr float effectDistDriveMax            = 40.0f;

    constexpr float effectDistShapeDefault        = 0.5f;    // 0=hard clip +-0.8, 1=tanh
    constexpr float effectDistShapeMin            = 0.0f;
    constexpr float effectDistShapeMax            = 1.0f;

    constexpr float effectDistBiasDefault         = 0.0f;
    constexpr float effectDistBiasMin             = -0.5f;
    constexpr float effectDistBiasMax             = 0.5f;

    constexpr float effectDistPreLoShelfFreqDefault = 20.0f;   // Hz
    constexpr float effectDistPreLoShelfFreqMin   = 20.0f;
    constexpr float effectDistPreLoShelfFreqMax   = 2000.0f;

    constexpr float effectDistPreLoShelfGainDefault = 0.0f;    // dB
    constexpr float effectDistPreLoShelfGainMin   = -24.0f;
    constexpr float effectDistPreLoShelfGainMax   = 24.0f;

    constexpr float effectDistPreHiShelfFreqDefault = 20000.0f; // Hz
    constexpr float effectDistPreHiShelfFreqMin   = 1000.0f;
    constexpr float effectDistPreHiShelfFreqMax   = 20000.0f;

    constexpr float effectDistPreHiShelfGainDefault = 0.0f;    // dB
    constexpr float effectDistPreHiShelfGainMin   = -24.0f;
    constexpr float effectDistPreHiShelfGainMax   = 24.0f;

    constexpr float effectDistPostLoShelfFreqDefault = 20.0f;  // Hz
    constexpr float effectDistPostLoShelfFreqMin  = 20.0f;
    constexpr float effectDistPostLoShelfFreqMax  = 2000.0f;

    constexpr float effectDistPostLoShelfGainDefault = 0.0f;   // dB
    constexpr float effectDistPostLoShelfGainMin  = -24.0f;
    constexpr float effectDistPostLoShelfGainMax  = 24.0f;

    constexpr float effectDistPostHiShelfFreqDefault = 20000.0f; // Hz
    constexpr float effectDistPostHiShelfFreqMin  = 1000.0f;
    constexpr float effectDistPostHiShelfFreqMax  = 20000.0f;

    constexpr float effectDistPostHiShelfGainDefault = 0.0f;   // dB
    constexpr float effectDistPostHiShelfGainMin  = -24.0f;
    constexpr float effectDistPostHiShelfGainMax  = 24.0f;

    constexpr float effectDistOutputDefault       = -6.0f;   // dB
    constexpr float effectDistOutputMin           = -24.0f;
    constexpr float effectDistOutputMax           = 12.0f;

    constexpr float effectDistMixDefault          = 100.0f;  // wet %
    constexpr float effectDistMixMin              = 0.0f;
    constexpr float effectDistMixMax              = 100.0f;

    constexpr int effectDistOversampleDefault     = 0;       // 0=auto, 1=off, 2=2x, 3=4x
    constexpr int effectDistOversampleMin         = 0;
    constexpr int effectDistOversampleMax         = 3;

    //--------------------------------------------------------------------------
    // Effect > FxEq1 / FxEq2, 6 bands each (spatcore::effects::EqParams)
    //--------------------------------------------------------------------------

    constexpr int effectEQBypassDefault           = 1;

    // Shape ids are the OUTPUT EQ's (eqShapeDefault and friends), not the
    // reverb EQ's different numbering. The plan's range starts at 1, so the
    // output EQ's 0=Off is not offered on an effects band.
    // 0=Off .. 7=AllPass, the OUTPUT EQ's numbering (not the reverb EQ's, which
    // stops at 6). Plan 5.2 writes the range as 1..7, which would be the only
    // EQ in the app whose bands cannot be switched off one at a time: the
    // output EQ allows 0 (eqShapeMin), the reverb pre-EQ allows 0
    // (reverbPreEQshapeMin), and the effects EQ module itself documents 0 as a
    // true bypass and is tested at all-zero as its identity case. Taking 0.
    constexpr int effectEQshapeDefault            = 1;
    constexpr int effectEQshapeMin                = 0;
    constexpr int effectEQshapeMax                = 7;

    constexpr int effectEQfreqDefault             = 80;      // Hz
    constexpr int effectEQfreqMin                 = 20;
    constexpr int effectEQfreqMax                 = 20000;

    constexpr float effectEQgainDefault           = 0.0f;    // dB
    constexpr float effectEQgainMin               = -24.0f;
    constexpr float effectEQgainMax               = 24.0f;

    constexpr float effectEQqDefault              = 0.7f;
    constexpr float effectEQqMin                  = eqQMin;
    constexpr float effectEQqMax                  = eqQMax;

    constexpr float effectEQslopeDefault          = 0.7f;
    constexpr float effectEQslopeMin              = eqSlopeMin;
    constexpr float effectEQslopeMax              = eqSlopeMax;

    // Per-band defaults; gain, q and slope are uniform across the six bands
    // and use the scalars above.
    inline const int effectEQBandShapes[6] = { 1, 2, 3, 3, 5, 6 };  // LowCut, LowShelf, Peak, Peak, HighShelf, HighCut
    inline const int effectEQBandFrequencies[6] = { 80, 250, 1000, 4000, 8000, 12000 };

    //--------------------------------------------------------------------------
    // Effect > FxDyn1 / FxDyn2 (spatcore::effects::DynamicsParams)
    //--------------------------------------------------------------------------

    constexpr int effectDynBypassDefault          = 1;
    constexpr int effectDynDetectorDefault        = 0;       // 0=Peak, 1=RMS

    constexpr float effectDynLookaheadDefault     = 1.0f;    // ms, delays the AUDIO (reported latency)
    constexpr float effectDynLookaheadMin         = 0.0f;
    constexpr float effectDynLookaheadMax         = 5.0f;

    constexpr float effectDynMakeupDefault        = 0.0f;    // dB
    constexpr float effectDynMakeupMin            = -24.0f;
    constexpr float effectDynMakeupMax            = 24.0f;

    constexpr int effectDynAutoMakeupDefault      = 0;
    constexpr int effectDynCompOnDefault          = 1;

    constexpr float effectDynCompThresholdDefault = -20.0f;  // dB
    constexpr float effectDynCompThresholdMin     = -60.0f;
    constexpr float effectDynCompThresholdMax     = 0.0f;

    constexpr float effectDynCompRatioDefault     = 4.0f;    // :1, 100 = limiter
    constexpr float effectDynCompRatioMin         = 1.0f;
    constexpr float effectDynCompRatioMax         = 100.0f;

    constexpr float effectDynCompKneeDefault      = 0.0f;    // dB, 0 = the prototype's hard knee
    constexpr float effectDynCompKneeMin          = 0.0f;
    constexpr float effectDynCompKneeMax          = 24.0f;

    constexpr float effectDynCompAttackDefault    = 10.0f;   // ms
    constexpr float effectDynCompAttackMin        = 0.05f;
    constexpr float effectDynCompAttackMax        = 200.0f;

    constexpr float effectDynCompReleaseDefault   = 100.0f;  // ms
    constexpr float effectDynCompReleaseMin       = 5.0f;
    constexpr float effectDynCompReleaseMax       = 2000.0f;

    constexpr float effectDynCompDetectorDelayDefault = 0.0f; // ms, delays the DETECTOR (transient pass, no latency)
    constexpr float effectDynCompDetectorDelayMin = 0.0f;
    constexpr float effectDynCompDetectorDelayMax = 50.0f;

    constexpr float effectDynCompScLoCutDefault   = 20.0f;   // Hz
    constexpr float effectDynCompScLoCutMin       = 20.0f;
    constexpr float effectDynCompScLoCutMax       = 2000.0f;

    constexpr float effectDynCompScHiCutDefault   = 20000.0f; // Hz
    constexpr float effectDynCompScHiCutMin       = 1000.0f;
    constexpr float effectDynCompScHiCutMax       = 20000.0f;

    constexpr int effectDynExpOnDefault           = 0;

    constexpr float effectDynExpThresholdDefault  = -50.0f;  // dB
    constexpr float effectDynExpThresholdMin      = -90.0f;
    constexpr float effectDynExpThresholdMax      = 0.0f;

    constexpr float effectDynExpRatioDefault      = 2.0f;    // :1, 100 = gate
    constexpr float effectDynExpRatioMin          = 1.0f;
    constexpr float effectDynExpRatioMax          = 100.0f;

    constexpr float effectDynExpAttackDefault     = 10.0f;   // ms
    constexpr float effectDynExpAttackMin         = 0.05f;
    constexpr float effectDynExpAttackMax         = 200.0f;

    constexpr float effectDynExpReleaseDefault    = 100.0f;  // ms
    constexpr float effectDynExpReleaseMin        = 5.0f;
    constexpr float effectDynExpReleaseMax        = 2000.0f;

    constexpr float effectDynExpRangeDefault      = -60.0f;  // dB
    constexpr float effectDynExpRangeMin          = -80.0f;
    constexpr float effectDynExpRangeMax          = 0.0f;

    constexpr float effectDynExpHoldDefault       = 20.0f;   // ms
    constexpr float effectDynExpHoldMin           = 0.0f;
    constexpr float effectDynExpHoldMax           = 500.0f;

    constexpr float effectDynExpScLoCutDefault    = 20.0f;   // Hz
    constexpr float effectDynExpScLoCutMin        = 20.0f;
    constexpr float effectDynExpScLoCutMax        = 2000.0f;

    constexpr float effectDynExpScHiCutDefault    = 20000.0f; // Hz
    constexpr float effectDynExpScHiCutMin        = 1000.0f;
    constexpr float effectDynExpScHiCutMax        = 20000.0f;

    //--------------------------------------------------------------------------
    // Effect > FxMod, chorus / flanger (spatcore::effects::ModulationParams)
    //--------------------------------------------------------------------------

    constexpr int effectModBypassDefault          = 1;
    constexpr int effectModModeDefault            = 0;       // 0=Chorus, 1=Flanger

    constexpr float effectModRateDefault          = 0.8f;    // Hz
    constexpr float effectModRateMin              = 0.05f;
    constexpr float effectModRateMax              = 10.0f;

    constexpr float effectModDepthDefault         = 50.0f;   // % of the centre delay
    constexpr float effectModDepthMin             = 0.0f;
    constexpr float effectModDepthMax             = 100.0f;

    constexpr float effectModDelayDefault         = 15.0f;   // ms
    constexpr float effectModDelayMin             = 0.1f;
    constexpr float effectModDelayMax             = 30.0f;

    constexpr float effectModFeedbackDefault      = 0.0f;    // signed %
    constexpr float effectModFeedbackMin          = -95.0f;
    constexpr float effectModFeedbackMax          = 95.0f;

    constexpr int effectModVoicesDefault          = 2;
    constexpr int effectModVoicesMin              = 1;
    constexpr int effectModVoicesMax              = 3;

    constexpr int effectModShapeDefault           = 1;       // LFOWaveforms shape id
    constexpr int effectModShapeMin               = 1;
    constexpr int effectModShapeMax               = 8;

    constexpr float effectModPhaseDefault         = 0.0f;    // degrees
    constexpr float effectModPhaseMin             = 0.0f;
    constexpr float effectModPhaseMax             = 360.0f;

    constexpr float effectModLoCutDefault         = 20.0f;   // Hz
    constexpr float effectModLoCutMin             = 20.0f;
    constexpr float effectModLoCutMax             = 2000.0f;

    constexpr int effectModThroughZeroDefault     = 0;

    constexpr float effectModMixDefault           = 50.0f;   // wet %
    constexpr float effectModMixMin               = 0.0f;
    constexpr float effectModMixMax               = 100.0f;

    //--------------------------------------------------------------------------
    // Effect > FxPhaser (spatcore::effects::PhaserParams)
    //--------------------------------------------------------------------------

    constexpr int effectPhaserBypassDefault       = 1;

    // STAGES IS A SET, NOT AN INTERVAL. Plan 5.5 validates {4, 6, 8, 12}; the
    // Min/Max pair below is that set's HULL, which is all a bounds pair can
    // express. So 5, 7 and 9..11 pass the bounds table, OSC, MCP and OSCQuery,
    // and PhaserModule::validateStages (spatcore/effects/modules/PhaserModule.h
    // :318-325) then snaps them DOWN to the next member - it never rejects, so
    // a controller that sends 7 gets 6 and no error. Whatever binds this must
    // enforce set membership itself or accept that snap. Same deviation class
    // as effectReverbModelMax below.
    constexpr int effectPhaserStagesDefault       = 6;       // validated set: 4, 6, 8, 12
    constexpr int effectPhaserStagesMin           = 4;       // hull of the set, not the set
    constexpr int effectPhaserStagesMax           = 12;

    constexpr float effectPhaserCentreDefault     = 800.0f;  // Hz
    constexpr float effectPhaserCentreMin         = 100.0f;
    constexpr float effectPhaserCentreMax         = 5000.0f;

    constexpr float effectPhaserSpreadDefault     = 1.0f;    // octaves
    constexpr float effectPhaserSpreadMin         = 0.0f;
    constexpr float effectPhaserSpreadMax         = 3.0f;

    constexpr float effectPhaserRateDefault       = 0.3f;    // Hz
    constexpr float effectPhaserRateMin           = 0.02f;
    constexpr float effectPhaserRateMax           = 10.0f;

    constexpr float effectPhaserDepthDefault      = 2.0f;    // octaves
    constexpr float effectPhaserDepthMin          = 0.0f;
    constexpr float effectPhaserDepthMax          = 4.0f;

    constexpr int effectPhaserShapeDefault        = 1;       // LFOWaveforms shape id
    constexpr int effectPhaserShapeMin            = 1;
    constexpr int effectPhaserShapeMax            = 8;

    constexpr float effectPhaserFeedbackDefault   = 30.0f;   // signed %
    constexpr float effectPhaserFeedbackMin       = -95.0f;
    constexpr float effectPhaserFeedbackMax       = 95.0f;

    constexpr float effectPhaserMixDefault        = 50.0f;   // wet %
    constexpr float effectPhaserMixMin            = 0.0f;
    constexpr float effectPhaserMixMax            = 100.0f;

    //--------------------------------------------------------------------------
    // Effect > FxTrem (spatcore::effects::TremoloParams)
    //--------------------------------------------------------------------------

    constexpr int effectTremBypassDefault         = 1;

    constexpr float effectTremRateDefault         = 4.0f;    // Hz
    constexpr float effectTremRateMin             = 0.05f;
    constexpr float effectTremRateMax             = 20.0f;

    constexpr float effectTremDepthDefault        = 12.0f;   // dB (dB-linear modulation)
    constexpr float effectTremDepthMin            = 0.0f;
    constexpr float effectTremDepthMax            = 60.0f;

    constexpr float effectTremShapeDefault        = 0.0f;    // 0=sine, 1=triangle, continuous
    constexpr float effectTremShapeMin            = 0.0f;
    constexpr float effectTremShapeMax            = 1.0f;

    constexpr float effectTremMixDefault          = 100.0f;  // wet %
    constexpr float effectTremMixMin              = 0.0f;
    constexpr float effectTremMixMax              = 100.0f;

    //--------------------------------------------------------------------------
    // Effect > FxReverb (spatcore::effects::ReverbParams). Unrelated to the
    // <Reverbs> channel family: the two sets deliberately differ, e.g. the
    // crossover-high default is 4000 Hz here and 2500 Hz there.
    //--------------------------------------------------------------------------

    constexpr int effectReverbBypassDefault       = 1;

    // spatcore::effects::ReverbModel: 0 FDN, 1 Plate, 4 Modulated Hall, 5
    // Shimmer. 2 (SDN-style) and 3 (IR) are RESERVED ids: the bounds accept
    // them so a project that stored one still loads, and the engine runs the
    // FDN for them (resolveReverbModel) - the menu does not offer them.
    constexpr int effectReverbModelDefault        = 0;
    constexpr int effectReverbModelMin            = 0;
    constexpr int effectReverbModelMax            = 5;

    // spatcore::effects::ReverbType, append-only: 0-4 the shipped FDN rooms
    // (frozen), 5 Custom (no row), 6 Medium Hall - exactly the defaults above
    // and below, so a fresh channel is a preset it really holds - and 7-22
    // the model presets. The label says Preset; the identifier stays.
    constexpr int effectReverbTypeDefault         = 6;
    constexpr int effectReverbTypeMin             = 0;
    constexpr int effectReverbTypeMax             = 22;

    constexpr float effectReverbPredelayDefault   = 10.0f;   // ms
    constexpr float effectReverbPredelayMin       = 0.0f;
    constexpr float effectReverbPredelayMax       = 250.0f;

    constexpr float effectReverbRT60Default       = 1.5f;    // seconds
    constexpr float effectReverbRT60Min           = 0.2f;
    constexpr float effectReverbRT60Max           = 8.0f;

    constexpr float effectReverbRT60LowMultDefault = 1.3f;
    constexpr float effectReverbRT60LowMultMin    = 0.1f;
    constexpr float effectReverbRT60LowMultMax    = 9.0f;

    constexpr float effectReverbRT60HighMultDefault = 0.4f;
    constexpr float effectReverbRT60HighMultMin   = 0.1f;
    constexpr float effectReverbRT60HighMultMax   = 9.0f;

    constexpr float effectReverbCrossoverLowDefault = 200.0f; // Hz
    constexpr float effectReverbCrossoverLowMin   = 50.0f;
    constexpr float effectReverbCrossoverLowMax   = 500.0f;

    constexpr float effectReverbCrossoverHighDefault = 4000.0f; // Hz
    constexpr float effectReverbCrossoverHighMin  = 1000.0f;
    constexpr float effectReverbCrossoverHighMax  = 10000.0f;

    constexpr float effectReverbDiffusionDefault  = 0.5f;
    constexpr float effectReverbDiffusionMin      = 0.0f;
    constexpr float effectReverbDiffusionMax      = 1.0f;

    constexpr float effectReverbSizeDefault       = 1.0f;
    constexpr float effectReverbSizeMin           = 0.5f;
    constexpr float effectReverbSizeMax           = 2.0f;

    constexpr float effectReverbToneDefault       = 12000.0f; // Hz
    constexpr float effectReverbToneMin           = 1000.0f;
    constexpr float effectReverbToneMax           = 20000.0f;

    constexpr float effectReverbMixDefault        = 30.0f;   // wet %
    constexpr float effectReverbMixMin            = 0.0f;
    constexpr float effectReverbMixMax            = 100.0f;

    // Early reflections, in front of any model (spatcore ErProfile).
    constexpr int effectReverbERProfileDefault    = 0;       // Off
    constexpr int effectReverbERProfileMin        = 0;
    constexpr int effectReverbERProfileMax        = 4;       // Room, Chamber, Hall, Cathedral

    constexpr float effectReverbERLevelDefault    = -6.0f;   // dB against the dry
    constexpr float effectReverbERLevelMin        = -30.0f;
    constexpr float effectReverbERLevelMax        = 6.0f;

    // The tank's modulation: Plate, Modulated Hall and Shimmer (the FDN has none).
    constexpr float effectReverbModRateDefault    = 0.8f;    // Hz
    constexpr float effectReverbModRateMin        = 0.05f;
    constexpr float effectReverbModRateMax        = 5.0f;

    constexpr float effectReverbModDepthDefault   = 50.0f;   // %; 50 is the plate paper's excursion
    constexpr float effectReverbModDepthMin       = 0.0f;
    constexpr float effectReverbModDepthMax       = 100.0f;

    // Shimmer only (spatcore ShimmerInterval): 0 +12, 1 +7, 2 +7 & +12, 3 +19,
    // 4 +24, 5 +5, 6 -12, 7 -12 & +12.
    constexpr int effectReverbShimmerPitchDefault = 0;
    constexpr int effectReverbShimmerPitchMin     = 0;
    constexpr int effectReverbShimmerPitchMax     = 7;

    constexpr float effectReverbShimmerAmountDefault = 50.0f; // %
    constexpr float effectReverbShimmerAmountMin  = 0.0f;
    constexpr float effectReverbShimmerAmountMax  = 100.0f;

    //--------------------------------------------------------------------------
    // Effect > FxDelay, multitap (spatcore::effects::MultitapParams)
    //--------------------------------------------------------------------------

    constexpr int effectDelayBypassDefault        = 1;

    // The time maxima are the LARGEST the global cap allows
    // (effectsGlobalMaxDelaySecondsMax * 1000). The live maximum is the current
    // effectsGlobalMaxDelaySeconds, which is narrower whenever the cap is not 20 s.
    constexpr float effectDelayTimeDefault        = 375.0f;  // ms
    constexpr float effectDelayTimeMin            = 1.0f;
    constexpr float effectDelayTimeMax            = 20000.0f;

    constexpr int effectDelayTapsDefault          = 3;
    constexpr int effectDelayTapsMin              = 1;
    constexpr int effectDelayTapsMax              = 8;

    constexpr int effectDelayTapModeDefault       = 1;       // 0=Manual, 1=Pattern
    constexpr int effectDelayPatternDefault       = 0;       // 0=Equal, 1=Dotted, 2=Triplet, 3=Golden
    constexpr int effectDelayPatternMin           = 0;
    constexpr int effectDelayPatternMax           = 3;

    constexpr float effectDelayFeedbackDefault    = 30.0f;   // %
    constexpr float effectDelayFeedbackMin        = 0.0f;
    constexpr float effectDelayFeedbackMax        = 95.0f;

    constexpr int effectDelayFeedbackTapDefault   = 0;       // 0=last
    constexpr int effectDelayFeedbackTapMin       = 0;
    constexpr int effectDelayFeedbackTapMax       = 8;

    constexpr float effectDelayInLoCutDefault     = 20.0f;   // Hz
    constexpr float effectDelayInLoCutMin         = 20.0f;
    constexpr float effectDelayInLoCutMax         = 2000.0f;

    constexpr float effectDelayFbLoShelfFreqDefault = 200.0f; // Hz
    constexpr float effectDelayFbLoShelfFreqMin   = 20.0f;
    constexpr float effectDelayFbLoShelfFreqMax   = 2000.0f;

    constexpr float effectDelayFbLoShelfGainDefault = 0.0f;   // dB
    constexpr float effectDelayFbLoShelfGainMin   = -24.0f;
    constexpr float effectDelayFbLoShelfGainMax   = 24.0f;

    constexpr float effectDelayFbHiShelfFreqDefault = 4000.0f; // Hz
    constexpr float effectDelayFbHiShelfFreqMin   = 1000.0f;
    constexpr float effectDelayFbHiShelfFreqMax   = 20000.0f;

    constexpr float effectDelayFbHiShelfGainDefault = -3.0f;  // dB
    constexpr float effectDelayFbHiShelfGainMin   = -24.0f;
    constexpr float effectDelayFbHiShelfGainMax   = 24.0f;

    constexpr float effectDelayModRateDefault     = 0.1f;    // Hz
    constexpr float effectDelayModRateMin         = 0.02f;
    constexpr float effectDelayModRateMax         = 10.0f;

    constexpr float effectDelayModDepthDefault    = 0.0f;    // % of the delay time
    constexpr float effectDelayModDepthMin        = 0.0f;
    constexpr float effectDelayModDepthMax        = 50.0f;

    constexpr float effectDelayDiffusionDefault   = 0.0f;
    constexpr float effectDelayDiffusionMin       = 0.0f;
    constexpr float effectDelayDiffusionMax       = 1.0f;

    constexpr float effectDelayGlideDefault       = 200.0f;  // ms
    constexpr float effectDelayGlideMin           = 0.0f;
    constexpr float effectDelayGlideMax           = 2000.0f;

    constexpr float effectDelayMixDefault         = 35.0f;   // wet %
    constexpr float effectDelayMixMin             = 0.0f;
    constexpr float effectDelayMixMax             = 100.0f;

    // Per-tap, on the <Tap id="1".."8"> children
    constexpr float effectDelayTapTimeDefault     = 375.0f;  // ms, tap 1
    constexpr float effectDelayTapTimeMin         = 1.0f;
    constexpr float effectDelayTapTimeMax         = 20000.0f;

    constexpr float effectDelayTapLevelDefault    = 0.0f;    // dB, tap 1
    constexpr float effectDelayTapLevelMin        = -60.0f;
    constexpr float effectDelayTapLevelMax        = 0.0f;

    // Tap tables: k * 375 ms and -2 dB per tap, matching MultitapParams'
    // tapTimeMs[8] / tapLevelDb[8].
    inline const float effectDelayTapTimes[8]  = { 375.0f, 750.0f, 1125.0f, 1500.0f, 1875.0f, 2250.0f, 2625.0f, 3000.0f };
    inline const float effectDelayTapLevels[8] = { 0.0f, -2.0f, -4.0f, -6.0f, -8.0f, -10.0f, -12.0f, -14.0f };

    //--------------------------------------------------------------------------
    // Effect > FxCrush (spatcore::effects::BitcrusherParams)
    //--------------------------------------------------------------------------

    constexpr int effectCrushBypassDefault        = 1;

    constexpr float effectCrushBitsDefault        = 8.0f;    // fractional allowed
    constexpr float effectCrushBitsMin            = 1.0f;
    constexpr float effectCrushBitsMax            = 24.0f;

    constexpr float effectCrushRateDefault        = 12000.0f; // Hz, clamped to the device rate
    constexpr float effectCrushRateMin            = 100.0f;
    constexpr float effectCrushRateMax            = 96000.0f;

    constexpr int effectCrushFilterDefault        = 0;       // 0=hold (aliasing), 1=anti-aliased

    constexpr float effectCrushDitherDefault      = -96.0f;  // dB, -96 = off
    constexpr float effectCrushDitherMin          = -96.0f;
    constexpr float effectCrushDitherMax          = 0.0f;

    constexpr float effectCrushMixDefault         = 100.0f;  // wet %
    constexpr float effectCrushMixMin             = 0.0f;
    constexpr float effectCrushMixMax             = 100.0f;

    //--------------------------------------------------------------------------
    // Effect > Sends, per-CELL bounds
    //--------------------------------------------------------------------------
    // The four row properties (effectSendLevels / Ons / FxSendLevels / FxSendOns)
    // are unbound packed CSV strings, like effectMutes. These constants belong to
    // the CELL pseudo-identifiers, which no node carries: the OSC parser, the
    // ramper and the OSCQuery cell nodes validate one cell against them - and so
    // does the row normaliser, which clamps every token of a level row into
    // [Min, Max] and pads with Default. The Min/Max pair is therefore load-bearing
    // for stored state, not only for the wire.
    // A cell is gated by its On switch, so a level of 0 dB is unity and the row
    // starts silent because the switches start off.

    constexpr float effectSendLevelDefault        = 0.0f;    // dB
    constexpr float effectSendLevelMin            = -92.0f;
    constexpr float effectSendLevelMax            = 0.0f;

    constexpr int effectSendOnDefault             = 0;

    constexpr float effectFxSendLevelDefault      = 0.0f;    // dB
    constexpr float effectFxSendLevelMin          = -92.0f;
    constexpr float effectFxSendLevelMax          = 0.0f;

    constexpr int effectFxSendOnDefault           = 0;

    //==========================================================================
    // Effects Global Defaults (Config > EffectsGlobal, not per-channel)
    //==========================================================================

    constexpr int effectsMapVisibleDefault        = 1;       // Config > Master display toggle

    inline const juce::String effectsGlobalLinkNamesDefault =
        "Group 1,Group 2,Group 3,Group 4,Group 5,Group 6,Group 7,Group 8";

    constexpr int effectsGlobalLinkModeDefault    = 1;       // 0=off, 1=absolute, 2=relative
    constexpr int effectsGlobalLinkModeMin        = 0;
    constexpr int effectsGlobalLinkModeMax        = 2;

    constexpr int effectsGlobalFxFeedGeometricDefault = 1;   // 0=matrix only (delay 0), 1=geometric

    constexpr int effectsGlobalWorkerThreadsDefault = -1;    // -1=auto (the reverb-feed rule)
    constexpr int effectsGlobalWorkerThreadsMin   = -1;
    constexpr int effectsGlobalWorkerThreadsMax   = 4;

    constexpr int effectsGlobalReturnCushionDefault = 0;     // 0=auto, else blocks
    constexpr int effectsGlobalReturnCushionMin   = 0;
    constexpr int effectsGlobalReturnCushionMax   = 3;

    constexpr int effectsGlobalLoopGuardDefault   = 1;

    constexpr float effectsGlobalLoopGuardCeilingDefault = 6.0f;  // dBFS peak
    constexpr float effectsGlobalLoopGuardCeilingMin = 0.0f;
    constexpr float effectsGlobalLoopGuardCeilingMax = 24.0f;

    constexpr int effectsGlobalMaxDelaySecondsDefault = 5;   // sizes every delay module's buffer
    constexpr int effectsGlobalMaxDelaySecondsMin = 1;
    constexpr int effectsGlobalMaxDelaySecondsMax = 20;

    constexpr const char* effectsGlobalFeedGpuDeviceDefault = "cpu";  // compute device id

    //==========================================================================
    // Sampler Defaults
    //==========================================================================

    constexpr bool samplerEnabledDefault             = false;  // Disabled by default

    // Per-channel
    constexpr int inputSamplerActiveDefault           = 0;      // OFF
    constexpr int samplerMidiZoneQuadrantDefault      = 0;      // 0=full pad
    constexpr int inputSamplerActiveSetDefault       = 0;      // First set (0-based)

    // Grid
    constexpr int samplerGridRows                     = 6;
    constexpr int samplerGridCols                     = 6;
    // Sets are created by the operator and were previously unbounded. The only
    // ceiling stated anywhere was the OSCQuery range on inputSamplerActiveSet
    // (0-16); this makes that the actual limit, so every surface agrees.
    constexpr int maxSamplerSets       = 16;
    constexpr int samplerGridCells                    = 36;     // 6x6

    // Cell defaults
    constexpr float samplerCellInTimeDefault          = 0.0f;   // ms
    constexpr float samplerCellInTimeMin              = 0.0f;
    constexpr float samplerCellInTimeMax              = 5000.0f; // 5s
    constexpr float samplerCellOutTimeDefault         = 20.0f;  // ms (fade-out to avoid clicks)
    constexpr float samplerCellOutTimeMin             = 1.0f;
    constexpr float samplerCellOutTimeMax             = 5000.0f;
    constexpr float samplerCellOffsetDefault          = 0.0f;   // meters
    constexpr float samplerCellOffsetMin              = -50.0f;
    constexpr float samplerCellOffsetMax              = 50.0f;
    constexpr float samplerCellAttenuationDefault     = 0.0f;   // dB
    constexpr float samplerCellAttenuationMin         = -60.0f;
    constexpr float samplerCellAttenuationMax         = 0.0f;

    // Set defaults
    constexpr int samplerSetPlayModeDefault           = 0;      // 0=sequential
    constexpr float samplerSetPosDefault              = 0.0f;   // meters
    constexpr float samplerSetPosMin                  = -50.0f;
    constexpr float samplerSetPosMax                  = 50.0f;
    constexpr float samplerSetLevelDefault            = 0.0f;   // dB
    constexpr float samplerSetLevelMin                = -60.0f;
    constexpr float samplerSetLevelMax                = 0.0f;

    // Pressure mapping defaults
    constexpr int samplerSetPressEnabledDefault       = 0;      // OFF
    constexpr int samplerSetPressLevelEnabledDefault = 1;      // Level ON by default
    constexpr int samplerSetPressDirDefault           = 0;      // 0=positive
    constexpr float samplerSetPressCurveDefault       = 0.5f;   // linear
    constexpr float samplerSetPressCurveMin           = 0.0f;
    constexpr float samplerSetPressCurveMax           = 1.0f;
    constexpr int samplerSetPressXYEnabledDefault     = 1;      // ON by default
    constexpr float samplerSetPressXYScaleDefault     = 0.05f;  // m per update (same as joystick)
    constexpr float samplerSetPressXYScaleMin         = 0.01f;
    constexpr float samplerSetPressXYScaleMax         = 0.2f;

    //==========================================================================
    // Lightpad Defaults
    //==========================================================================

    constexpr int   lightpadSplitDefault         = 0;     // full (no split)
    constexpr float lightpadSensitivityDefault   = 0.05f; // meters per unit deflection
    constexpr float lightpadSensitivityMin       = 0.01f;
    constexpr float lightpadSensitivityMax       = 1.00f;
    constexpr int   lightpadZoneIdDefault        = -1;    // unassigned
    constexpr int   samplerControllerModeDefault = 0;    // 0=Off, 1=Lightpad, 2=Remote
    constexpr int   remotePadGridLayoutDefault   = 0;    // 0=3x2, 1=5x3

} // namespace WFSParameterDefaults
