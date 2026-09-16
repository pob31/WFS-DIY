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

    // <InputSnapshot> root attributes: MIDI note trigger binding.
    // On the ROOT, not inside <ExtendedScope>, so the binding index can be built
    // with an outer-element-only XML parse and so scope templates (which share
    // the scope serializer) never carry a note. Absent = unbound.
    const juce::Identifier midiChannel       ("midiChannel");   // 1..16
    const juce::Identifier midiNote          ("midiNote");      // 0..127

    //==========================================================================
    // Config > Show Section
    //==========================================================================

    const juce::Identifier showName          ("showName");
    const juce::Identifier showLocation      ("showLocation");
    const juce::Identifier autoPreselectDirty ("autoPreselectDirty");
    const juce::Identifier writeToQLab         ("writeToQLab");
    const juce::Identifier writeSnapshotLoadCue("writeSnapshotLoadCue");

    //==========================================================================
    // Config > I/O Section
    //==========================================================================

    const juce::Identifier inputChannels     ("inputChannels");
    const juce::Identifier stereoInputChannels ("stereoInputChannels");  // how many of the LAST input channels are stereo pairs
    const juce::Identifier outputChannels    ("outputChannels");
    const juce::Identifier reverbChannels    ("reverbChannels");
    const juce::Identifier effectChannels    ("effectChannels");
    // effectChannels is a CONFIG property despite the "effect" prefix: it lives
    // in <Config><IO>, not on an <Effect>, so getParameterScope needs a BY-NAME
    // exception here ahead of its "effect" prefix test (the reverbChannels
    // precedent). Without it the prefix test would send the write into the
    // per-channel Effect branch, which carries no such property, and the write
    // would be dropped with no error.
    const juce::Identifier algorithmDSP      ("algorithmDSP");
    const juce::Identifier algorithmDeviceId ("algorithmDeviceId");   // compute device for the GPU algorithm paths ("cpu"/"hip:0"/...)
    const juce::Identifier runDSP            ("runDSP");

    // One-way ownership latch for the input channel numbers.
    // false/absent = fresh session: numbers track display order — every
    // structural edit renumbers the whole list dense 1..N, so a channel dragged
    // to display position 2 reads "#2". true = user-owned: numbers are
    // permanent (creation appends highest + 1, deletion retires a number and
    // leaves a gap). Latched by anything that makes a number observable or
    // depended-upon: any project/config load, opening the input patch window or
    // the level meter window, selecting the Inputs or Map tab, storing or
    // recalling an input snapshot, an incoming OSC/ADM/Remote message
    // addressing an input by number, a Remote client handshake, sending QLab
    // cues, and any MCP tool addressing an input by number. Persists with the
    // session; its absence in a file written by an older build is harmless,
    // because every load latches.
    const juce::Identifier channelNumbersUserOwned ("channelNumbersUserOwned");

    // Input channel inventory — a FILE artifact only, written into <IO> by the
    // system-config save and consumed by the load. It never lives in the runtime
    // tree: the <Input> nodes are the single source of truth, and a live copy
    // could only desync from them.
    //
    // inputChannels is a SUM, and the mono/stereo split plus the display order
    // live solely in each <Input>'s inputChannelType and child position — which
    // are written to inputs.xml, not system.xml. Without this a system config
    // reloaded on its own rebuilds every channel as mono (setNumInputChannels
    // appends default-mono channels), and the positional patch rows then land on
    // the wrong channels: a stereo row's two hardware columns end up on a mono
    // channel. Two counts would not do — mono and stereo interleave freely, so
    // the arrangement has to be recorded per channel, not tallied.
    //
    // Deliberately NOT stamped by createIOSection: ensureCompleteSchema
    // back-fills template content into loaded trees, so a templated default
    // would write an empty inventory onto a legacy file and make absence
    // ambiguous. Absent = "legacy file, fall back to the sum". Same rule, and
    // the same hazard, as channelNumbersUserOwned above.
    const juce::Identifier InputChannelList  ("InputChannelList");  // <IO> child, display order
    const juce::Identifier Ch                ("Ch");                // one per live channel
    // Short attribute names, deliberately long C++ names: this header is pulled
    // in under `using namespace WFSParameterIDs`, where a bare `n` or `type`
    // would shadow (and be shadowed by) ordinary locals — WFSValueTreeState.cpp
    // alone has a `for (int n = 1; ...)` loop.
    const juce::Identifier chNumber          ("n");                 // permanent channel number
    const juce::Identifier chType            ("type");              // "mono" / "stereo"

    // Hardware-input FINGERPRINT on an <Input> node in inputs.xml and on a
    // snapshot's <Input> entry: the 1-based hardware inputs the channel's patch
    // row held when the file was written, e.g. "15" or "15,16".
    //
    // A guard, never a source. Nothing repatches from it: the patch lives in
    // system.xml's <AudioPatch> and is applied from there alone. It exists so a
    // file written under one configuration can be RECOGNISED as such before it
    // is applied to another - inputs.xml and snapshots otherwise carry no
    // channel<->hardware relation at all, so a stale one loaded by number had
    // nothing to trip over. Keyed by number (it sits on the <Input>), so it is
    // immune to the position problem that defeats everything positional. It is
    // stamped into the saved COPY and evicted from the live tree after a load,
    // like InputChannelList; a live copy could only go stale on the next
    // re-patch.
    // Named hwInputsFingerprint in C++ (the attribute is still "hwInputs"): this
    // header is pulled in under `using namespace WFSParameterIDs`, where a bare
    // hwInputs shadows updateHardwareChannelCount's parameter of that name.
    const juce::Identifier hwInputsFingerprint ("hwInputs");

    //==========================================================================
    // Config > Binaural Section
    //==========================================================================

    const juce::Identifier Binaural              ("Binaural");
    const juce::Identifier binauralEnabled       ("binauralEnabled");       // bool: processing active
    const juce::Identifier binauralSoloMode      ("binauralSoloMode");      // 0=Single, 1=Multi
    const juce::Identifier binauralOutputChannel ("binauralOutputChannel"); // First output channel (-1=disabled)
    const juce::Identifier binauralListenerDistance ("binauralListenerDistance"); // meters
    const juce::Identifier binauralListenerAngle ("binauralListenerAngle"); // degrees
    const juce::Identifier binauralAttenuation   ("binauralAttenuation");   // dB
    const juce::Identifier binauralDelay         ("binauralDelay");         // ms
    const juce::Identifier inputSoloStates       ("inputSoloStates");       // comma-separated 0/1
    const juce::Identifier binauralRenderMode    ("binauralRenderMode");    // 0=ORTF legacy, 1=Structural HRTF, 2=SOFA
    const juce::Identifier binauralSofaFile      ("binauralSofaFile");      // relative to <project>/sofa, ""=built-in set
    const juce::Identifier binauralHeadRadius    ("binauralHeadRadius");    // meters (structural model personalization)
    const juce::Identifier binauralListenerX     ("binauralListenerX");     // meters, lateral offset from axial placement
    const juce::Identifier binauralListenerHeight ("binauralListenerHeight"); // meters (ear height, new modes only)
    const juce::Identifier binauralListenerYaw   ("binauralListenerYaw");   // degrees, offset from facing-origin
    const juce::Identifier binauralListenerPitch ("binauralListenerPitch"); // degrees
    const juce::Identifier binauralListenerRoll  ("binauralListenerRoll");  // degrees
    const juce::Identifier binauralHeadTrackerSource ("binauralHeadTrackerSource"); // "manual" or stable device id
    const juce::Identifier binauralReverbAttenuation ("binauralReverbAttenuation"); // dB, headphone reverb balance

    //==========================================================================
    // Config > Stage Section
    //==========================================================================

    const juce::Identifier stageShape        ("stageShape");       // 0=box, 1=cylinder, 2=dome

    // One-way ownership latch for channel positions (inputs/outputs/reverbs).
    // false/absent = engine-owned: stage or count changes silently re-run the
    // initial placement. true = user-owned: the user has opened the Map tab or
    // manually edited a position, and nothing repositions automatically again.
    // Persists with the session, so a saved show stays user-owned on reload.
    const juce::Identifier positionsUserOwned ("positionsUserOwned");
    const juce::Identifier stageWidth        ("stageWidth");
    const juce::Identifier stageDepth        ("stageDepth");
    const juce::Identifier stageHeight       ("stageHeight");
    const juce::Identifier stageDiameter     ("stageDiameter");    // For cylinder/dome
    const juce::Identifier domeElevation     ("domeElevation");    // Elevation angle in degrees
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
    const juce::Identifier gpuPipelineDepth  ("gpuPipelineDepth"); // Native GPU async pipeline depth (blocks)

    //==========================================================================
    // Config > UI Section
    //==========================================================================

    const juce::Identifier UI                ("UI");
    const juce::Identifier colorScheme       ("colorScheme");  // 0=Default, 1=OLED Black, 2=Light
    const juce::Identifier streamDeckEnabled ("streamDeckEnabled");

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
    const juce::Identifier networkTSqlabPatch ("networkTSqlabPatch");

    // OSC Source Filtering
    const juce::Identifier networkOscSourceFilter ("networkOscSourceFilter");

    // OSC Query
    const juce::Identifier networkOscQueryEnabled ("networkOscQueryEnabled");
    const juce::Identifier networkOscQueryPort    ("networkOscQueryPort");

    //==========================================================================
    // Config > ADM-OSC Section
    //==========================================================================

    // ValueTree node types for ADM-OSC mappings
    const juce::Identifier ADMCartMapping    ("ADMCartMapping");
    const juce::Identifier ADMPolarMapping   ("ADMPolarMapping");
    const juce::Identifier ADMCartAxis       ("ADMCartAxis");

    // Cartesian mapping per-axis params (stored in ADMCartAxis children, 3 per mapping)
    const juce::Identifier admCartAxisId         ("admCartAxisId");         // int: 0=X, 1=Y, 2=Z (which internal axis)
    const juce::Identifier admCartAxisSwap       ("admCartAxisSwap");       // int: 0=X, 1=Y, 2=Z (which ADM axis feeds this)
    const juce::Identifier admCartSignFlip       ("admCartSignFlip");       // int: 0/1
    const juce::Identifier admCartCenterOffset   ("admCartCenterOffset");   // float: meters, where norm 0.0 maps
    const juce::Identifier admCartBreakpoint     ("admCartBreakpoint");     // float: 0.01-0.99, piecewise linear break
    const juce::Identifier admCartPosInnerWidth  ("admCartPosInnerWidth");  // float: meters, center→breakpoint (positive)
    const juce::Identifier admCartPosOuterWidth  ("admCartPosOuterWidth");  // float: meters, breakpoint→+1 (positive)
    const juce::Identifier admCartNegInnerWidth  ("admCartNegInnerWidth");  // float: meters, center→breakpoint (negative)
    const juce::Identifier admCartNegOuterWidth  ("admCartNegOuterWidth");  // float: meters, breakpoint→-1 (negative)

    // Polar mapping params (stored on ADMPolarMapping node)
    const juce::Identifier admPolarAzimuthOffset ("admPolarAzimuthOffset"); // float: degrees (-180 to 180)
    const juce::Identifier admPolarAzimuthFlip   ("admPolarAzimuthFlip");   // int: 0/1
    const juce::Identifier admPolarElevationFlip ("admPolarElevationFlip"); // int: 0/1
    const juce::Identifier admPolarDistMin       ("admPolarDistMin");       // float: legacy, kept for migration
    const juce::Identifier admPolarDistMax       ("admPolarDistMax");       // float: legacy, kept for migration
    const juce::Identifier admPolarDistBreakpoint ("admPolarDistBreakpoint"); // float: 0.01-0.99
    const juce::Identifier admPolarDistInner      ("admPolarDistInner");     // float: meters (center to breakpoint)
    const juce::Identifier admPolarDistOuter      ("admPolarDistOuter");     // float: meters (breakpoint to max)
    const juce::Identifier admPolarDistCenter     ("admPolarDistCenter");    // float: meters (offset)

    // Legacy ADM-OSC params (kept for migration detection only)
    const juce::Identifier admOscOffsetX     ("admOscOffsetX");
    const juce::Identifier admOscScaleX      ("admOscScaleX");
    const juce::Identifier admOscFlipX       ("admOscFlipX");

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
    const juce::Identifier trackingOscPath      ("trackingOscPath");
    const juce::Identifier trackingPsnInterface ("trackingPsnInterface");
    const juce::Identifier trackingMqttHost     ("trackingMqttHost");
    const juce::Identifier trackingMqttTopic    ("trackingMqttTopic");
    const juce::Identifier trackingMqttJsonX    ("trackingMqttJsonX");
    const juce::Identifier trackingMqttJsonY    ("trackingMqttJsonY");
    const juce::Identifier trackingMqttJsonZ    ("trackingMqttJsonZ");
    const juce::Identifier trackingMqttJsonQ    ("trackingMqttJsonQ");
    const juce::Identifier trackingMqttTagIds   ("trackingMqttTagIds");

    //==========================================================================
    // Config > Clusters Section
    //==========================================================================

    const juce::Identifier Clusters          ("Clusters");
    const juce::Identifier Cluster           ("Cluster");
    const juce::Identifier clusterReferenceMode ("clusterReferenceMode");
    const juce::Identifier clusterInputOrder    ("clusterInputOrder");

    // How the clusterInputOrder CSVs in THIS FILE are keyed. Written on the
    // <Clusters> node by the system-config save, read by the load, and never
    // present in the runtime tree.
    //
    // In memory clusterInputOrder is a CSV of 0-based SLOT indices, which is the
    // right key for everything that consumes it live. On disk it is not: slots
    // are defined by inputs.xml while the CSV is persisted in system.xml, so a
    // load that reconciles the channel list shifts the slot space out from under
    // a CSV that was written against the file's. The load-time remaps then
    // half-corrupted it — deletions shifted tokens, the display-order restore
    // (a raw moveChild) did not — and the first token picks the pivot for cluster
    // drag and rotate in "First Input" mode, so the cluster silently pivoted
    // around the wrong source.
    //
    // Absent = a file written before this existed = the CSVs are slots. Those
    // still load correctly, because the live slot space at flush time equals the
    // file's; they are simply protected from the intermediate remaps and are
    // rewritten as numbers on the next save.
    //
    // Deliberately NOT written by createClustersSection: ensureCompleteSchema
    // back-fills template content into loaded trees, so a templated value would
    // be stamped onto legacy files and destroy the discriminator. Same rule, and
    // the same hazard, as channelNumbersUserOwned and InputChannelList. Do not
    // try to discriminate on "does a token equal 0" instead — slot 0 is a
    // legitimate legacy token, and "3,1,2" is valid under both readings.
    const juce::Identifier inputOrderKey        ("inputOrderKey");
    const juce::String     inputOrderKeyNumber  ("number");
    const juce::Identifier clusterInputsVisible ("clusterInputsVisible");

    // Config > Clusters > Cluster > LFO
    const juce::Identifier ClusterLFO              ("ClusterLFO");
    const juce::Identifier clusterLFOactive        ("clusterLFOactive");
    const juce::Identifier clusterLFOperiod        ("clusterLFOperiod");
    const juce::Identifier clusterLFOphase         ("clusterLFOphase");

    const juce::Identifier clusterLFOshapeX        ("clusterLFOshapeX");
    const juce::Identifier clusterLFOshapeY        ("clusterLFOshapeY");
    const juce::Identifier clusterLFOshapeZ        ("clusterLFOshapeZ");
    const juce::Identifier clusterLFOshapeRot      ("clusterLFOshapeRot");
    const juce::Identifier clusterLFOshapeScale    ("clusterLFOshapeScale");

    const juce::Identifier clusterLFOrateX         ("clusterLFOrateX");
    const juce::Identifier clusterLFOrateY         ("clusterLFOrateY");
    const juce::Identifier clusterLFOrateZ         ("clusterLFOrateZ");
    const juce::Identifier clusterLFOrateRot       ("clusterLFOrateRot");
    const juce::Identifier clusterLFOrateScale     ("clusterLFOrateScale");

    const juce::Identifier clusterLFOamplitudeX    ("clusterLFOamplitudeX");
    const juce::Identifier clusterLFOamplitudeY    ("clusterLFOamplitudeY");
    const juce::Identifier clusterLFOamplitudeZ    ("clusterLFOamplitudeZ");
    const juce::Identifier clusterLFOamplitudeRot  ("clusterLFOamplitudeRot");
    const juce::Identifier clusterLFOamplitudeScale("clusterLFOamplitudeScale");

    const juce::Identifier clusterLFOphaseX        ("clusterLFOphaseX");
    const juce::Identifier clusterLFOphaseY        ("clusterLFOphaseY");
    const juce::Identifier clusterLFOphaseZ        ("clusterLFOphaseZ");
    const juce::Identifier clusterLFOphaseRot      ("clusterLFOphaseRot");
    const juce::Identifier clusterLFOphaseScale    ("clusterLFOphaseScale");

    //==========================================================================
    // Config > ClusterLFOPresets (global, shared by all clusters)
    //==========================================================================
    const juce::Identifier ClusterLFOPresets    ("ClusterLFOPresets");
    const juce::Identifier ClusterLFOPreset     ("ClusterLFOPreset");
    const juce::Identifier clusterLFOPresetName ("clusterLFOPresetName");

    //==========================================================================
    // Input Channel Parameters
    //==========================================================================

    // Input node (structural property on <Input> itself, beside `id`):
    // "mono" | "stereo"; absent = mono. Source of truth for a channel's type
    // under the stable-number model. While the legacy count-based API still
    // exists it is kept in sync with the tail split (stereoInputChannels).
    const juce::Identifier inputChannelType      ("inputChannelType");

    // Input > Channel
    const juce::Identifier inputName             ("inputName");
    // 24-bit RGB (0x000000..0xFFFFFF) as a POSITIVE int, or -1 meaning "auto" — derive the
    // colour from the channel number as WfsColorUtilities::getInputColor always has.
    // Not a full ARGB on purpose: fromXml types every property as a string and WFSVar::toInt
    // resolves it through String::getIntValue(), a SIGNED 32-bit parse, so 0xFF...... would
    // not survive a save. Alpha is always opaque for a channel colour, so 24 bits is enough.
    const juce::Identifier inputColour           ("inputColour");
    const juce::Identifier inputSolo             ("inputSolo");             // 0/1, binaural solo — per channel so it travels with the node (reorder/delete safe); replaces the Binaural inputSoloStates csv
    const juce::Identifier inputStereoWidth      ("inputStereoWidth");      // metres, stereo pairs only — the FULL left-to-right distance, so each leg sits at half of it. Absolute: no speaker position is read, so it means the same thing on a bar, a circle or a Y-running array
    // Degrees, stereo pairs only: a rotation APPLIED TO the automatic tangential
    // axis, so 0 means automatic rather than "spread along +X". Positive
    // counter-clockwise viewed from above, the same convention as inputRotation;
    // ±180 is an explicit L/R swap. Applied in the world frame — inputFlipX/Y
    // already mirror the anchor and the automatic axis follows it, so mirroring
    // this offset too would undo the flip it is riding on.
    const juce::Identifier inputStereoAxisOffset ("inputStereoAxisOffset");
    // 0/1, stereo pairs only: drops the automatic tangential term and spreads along
    // the fixed world axis instead, so inputStereoAxisOffset above stops being a
    // rotation applied to a moving reference and becomes an absolute bearing
    // (0 = house left/right). The automatic axis is derived from the origin->anchor
    // direction, which is exactly what an operator wants gone when the image has to
    // hold still while the source walks.
    const juce::Identifier inputStereoAxisLock   ("inputStereoAxisLock");
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
    const juce::Identifier inputConstraintDistance      ("inputConstraintDistance");     // 0=OFF, 1=ON (for Cylindrical/Spherical modes)
    const juce::Identifier inputConstraintDistanceMin   ("inputConstraintDistanceMin");  // meters (0-50)
    const juce::Identifier inputConstraintDistanceMax   ("inputConstraintDistanceMax");  // meters (0-50)
    const juce::Identifier inputFlipX            ("inputFlipX");
    const juce::Identifier inputFlipY            ("inputFlipY");
    const juce::Identifier inputFlipZ            ("inputFlipZ");
    const juce::Identifier inputCluster          ("inputCluster");
    const juce::Identifier inputTrackingActive   ("inputTrackingActive");
    const juce::Identifier inputTrackingID       ("inputTrackingID");
    const juce::Identifier inputTrackingSmooth   ("inputTrackingSmooth");
    const juce::Identifier inputMaxSpeedActive   ("inputMaxSpeedActive");
    const juce::Identifier inputMaxSpeed         ("inputMaxSpeed");
    const juce::Identifier inputPathModeActive   ("inputPathModeActive");   // 0=OFF, 1=ON
    const juce::Identifier inputHeightFactor     ("inputHeightFactor");
    const juce::Identifier inputCoordinateMode   ("inputCoordinateMode");  // 0=Cartesian, 1=Cylindrical, 2=Spherical
    const juce::Identifier inputAdmMapping       ("inputAdmMapping");      // -1=none, 0-3=Cart mapping, 4-7=Polar mapping

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
    const juce::Identifier inputLSpeakEnable     ("inputLSpeakEnable");
    const juce::Identifier inputLSpeakThreshold  ("inputLSpeakThreshold");
    const juce::Identifier inputLSpeakRatio      ("inputLSpeakRatio");
    const juce::Identifier inputLSslowEnable     ("inputLSslowEnable");
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
    const juce::Identifier inputOtomoDuration        ("inputOtomoDuration");
    const juce::Identifier inputOtomoCurve           ("inputOtomoCurve");
    const juce::Identifier inputOtomoTrigger         ("inputOtomoTrigger");
    const juce::Identifier inputOtomoThreshold       ("inputOtomoThreshold");
    const juce::Identifier inputOtomoReset           ("inputOtomoReset");
    const juce::Identifier inputOtomoPauseResume     ("inputOtomoPauseResume");

    // Input > AutomOtion (Polar coordinates)
    const juce::Identifier inputOtomoCoordinateMode  ("inputOtomoCoordinateMode");  // 0=Cartesian, 1=Cylindrical, 2=Spherical
    const juce::Identifier inputOtomoR               ("inputOtomoR");               // Cylindrical radius
    const juce::Identifier inputOtomoTheta           ("inputOtomoTheta");           // Azimuth angle (shared cyl/sph)
    const juce::Identifier inputOtomoRsph            ("inputOtomoRsph");            // Spherical radius
    const juce::Identifier inputOtomoPhi             ("inputOtomoPhi");             // Elevation angle

    // Input > Mutes
    const juce::Identifier inputMutes            ("inputMutes");
    const juce::Identifier inputMuteMacro        ("inputMuteMacro");

    // Input > Sidelines (auto-mute at stage edges)
    const juce::Identifier inputSidelinesActive  ("inputSidelinesActive");  // 0=OFF, 1=ON
    const juce::Identifier inputSidelinesFringe  ("inputSidelinesFringe");  // Fringe size in meters

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
    const juce::Identifier inputHiddenByCluster  ("inputHiddenByCluster");

    //==========================================================================
    // Input > Gradient Maps
    //==========================================================================

    const juce::Identifier GradientMaps          ("GradientMaps");
    const juce::Identifier GradientLayer         ("GradientLayer");
    const juce::Identifier GradientShape         ("GradientShape");

    // Layer properties
    const juce::Identifier gmLayerEnabled        ("gmLayerEnabled");        // 0=OFF, 1=ON
    const juce::Identifier gmLayer0Enabled       ("gmLayer0Enabled");       // OSC alias for layer 0
    const juce::Identifier gmLayer1Enabled       ("gmLayer1Enabled");       // OSC alias for layer 1
    const juce::Identifier gmLayer2Enabled       ("gmLayer2Enabled");       // OSC alias for layer 2
    const juce::Identifier gmLayerParam          ("gmLayerParam");          // 0=Attenuation, 1=Height, 2=HF Shelf
    const juce::Identifier gmLayerWhite          ("gmLayerWhite");          // Value at white (float, unit depends on param)
    const juce::Identifier gmLayerBlack          ("gmLayerBlack");          // Value at black (float, unit depends on param)
    const juce::Identifier gmLayerCurve          ("gmLayerCurve");          // -1.0 to +1.0 (midpoint shift)
    const juce::Identifier gmLayerVisible        ("gmLayerVisible");        // 0=hidden, 1=visible (editor only)

    // Shape properties
    const juce::Identifier gmShapeType           ("gmShapeType");           // 0=Rectangle, 1=Ellipse, 2=Polygon
    const juce::Identifier gmShapePosX           ("gmShapePosX");           // Center X in stage coords (meters)
    const juce::Identifier gmShapePosY           ("gmShapePosY");           // Center Y in stage coords (meters)
    const juce::Identifier gmShapeRotation       ("gmShapeRotation");       // Degrees
    const juce::Identifier gmShapeScaleX         ("gmShapeScaleX");         // Width in meters (half-extent)
    const juce::Identifier gmShapeScaleY         ("gmShapeScaleY");         // Height in meters (half-extent)
    const juce::Identifier gmShapeVertices       ("gmShapeVertices");       // Semicolon-separated x,y pairs for polygons
    const juce::Identifier gmShapeFillType       ("gmShapeFillType");       // 0=Uniform, 1=Linear Gradient, 2=Radial Gradient
    const juce::Identifier gmShapeFillValue      ("gmShapeFillValue");      // Uniform fill greyscale 0.0-1.0
    const juce::Identifier gmShapeFillParams     ("gmShapeFillParams");     // Gradient control points (string)
    const juce::Identifier gmShapeBlur           ("gmShapeBlur");           // Edge blur in meters (0=sharp)
    const juce::Identifier gmShapeLocked         ("gmShapeLocked");         // 0=unlocked, 1=locked
    const juce::Identifier gmShapeOrder          ("gmShapeOrder");          // Z-order for painter's algorithm
    const juce::Identifier gmShapeEnabled        ("gmShapeEnabled");        // 0=disabled, 1=enabled
    const juce::Identifier gmShapeName           ("gmShapeName");           // Optional display name

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
    const juce::Identifier outputCoordinateMode  ("outputCoordinateMode");  // 0=Cartesian, 1=Cylindrical, 2=Spherical

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
    const juce::Identifier activeHardwareInputs  ("activeHardwareInputs");
    const juce::Identifier activeHardwareOutputs ("activeHardwareOutputs");

    //==========================================================================
    // Reverb Parameters
    //==========================================================================

    // Input reverb send (array of sends per input)

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
    const juce::Identifier reverbCoordinateMode  ("reverbCoordinateMode");  // 0=Cartesian, 1=Cylindrical, 2=Spherical

    // Reverb > Feed
    const juce::Identifier reverbOrientation     ("reverbOrientation");
    const juce::Identifier reverbAngleOn         ("reverbAngleOn");
    const juce::Identifier reverbAngleOff        ("reverbAngleOff");
    const juce::Identifier reverbPitch           ("reverbPitch");
    const juce::Identifier reverbHFdamping       ("reverbHFdamping");
    const juce::Identifier reverbMiniLatencyEnable ("reverbMiniLatencyEnable");
    const juce::Identifier reverbDistanceAttenEnable ("reverbDistanceAttenEnable");

    // Reverb > Pre-Processing EQ (4 bands, per-channel)
    const juce::Identifier reverbPreEQenable     ("reverbPreEQenable");
    const juce::Identifier reverbPreEQshape      ("reverbPreEQshape");
    const juce::Identifier reverbPreEQfreq       ("reverbPreEQfreq");
    const juce::Identifier reverbPreEQgain       ("reverbPreEQgain");
    const juce::Identifier reverbPreEQq          ("reverbPreEQq");
    const juce::Identifier reverbPreEQslope      ("reverbPreEQslope");

    // Reverb > Return
    const juce::Identifier reverbDistanceAttenuation ("reverbDistanceAttenuation");
    const juce::Identifier reverbCommonAtten     ("reverbCommonAtten");
    const juce::Identifier reverbMutes           ("reverbMutes");
    const juce::Identifier reverbMuteMacro       ("reverbMuteMacro");

    // Reverb > Map Display (global toggle in Config section)
    const juce::Identifier reverbsMapVisible     ("reverbsMapVisible");
    const juce::Identifier effectsMapVisible     ("effectsMapVisible");
    // Same story as effectChannels: a Config/Master display toggle whose name
    // starts with "effect", so it too needs a by-name Config exception before
    // the prefix test in getParameterScope.

    // Reverb > Algorithm (global, stored as child of Reverbs node)
    const juce::Identifier ReverbAlgorithm       ("ReverbAlgorithm");
    const juce::Identifier reverbAlgoType        ("reverbAlgoType");       // 0=SDN, 1=FDN, 2=IR
    const juce::Identifier reverbRT60            ("reverbRT60");           // seconds
    const juce::Identifier reverbRT60LowMult     ("reverbRT60LowMult");   // multiplier
    const juce::Identifier reverbRT60HighMult    ("reverbRT60HighMult");  // multiplier
    const juce::Identifier reverbCrossoverLow    ("reverbCrossoverLow");  // Hz
    const juce::Identifier reverbCrossoverHigh   ("reverbCrossoverHigh"); // Hz
    const juce::Identifier reverbDiffusion       ("reverbDiffusion");     // 0.0-1.0
    const juce::Identifier reverbSDNscale        ("reverbSDNscale");      // SDN inter-node delay multiplier
    const juce::Identifier reverbFDNsize         ("reverbFDNsize");       // FDN delay line multiplier
    const juce::Identifier reverbIRfile          ("reverbIRfile");        // file path string
    const juce::Identifier reverbIRtrim          ("reverbIRtrim");        // ms
    const juce::Identifier reverbIRlength        ("reverbIRlength");      // seconds
    const juce::Identifier reverbPerNodeIR       ("reverbPerNodeIR");     // 0/1
    const juce::Identifier reverbIRGpu           ("reverbIRGpu");         // legacy 0=CPU/1=GPU (migrated to reverbIRGpuDevice)
    const juce::Identifier reverbFDNGpu          ("reverbFDNGpu");        // legacy 0=CPU/1=GPU (migrated to reverbFDNGpuDevice)
    const juce::Identifier reverbSDNGpu          ("reverbSDNGpu");        // legacy 0=CPU/1=GPU (migrated to reverbSDNGpuDevice)
    const juce::Identifier reverbIRGpuDevice     ("reverbIRGpuDevice");   // compute device id ("cpu"/"hip:0"/...)
    const juce::Identifier reverbFDNGpuDevice    ("reverbFDNGpuDevice");  // compute device id
    const juce::Identifier reverbSDNGpuDevice    ("reverbSDNGpuDevice");  // compute device id
    const juce::Identifier reverbWetLevel        ("reverbWetLevel");      // dB

    // Reverb > Pre-Processing Compressor (global, stored as child of Reverbs node)
    const juce::Identifier ReverbPreComp         ("ReverbPreComp");
    const juce::Identifier reverbPreCompBypass   ("reverbPreCompBypass");     // 0=active, 1=bypassed
    const juce::Identifier reverbPreCompThreshold("reverbPreCompThreshold");  // dB
    const juce::Identifier reverbPreCompRatio    ("reverbPreCompRatio");      // ratio :1
    const juce::Identifier reverbPreCompAttack   ("reverbPreCompAttack");     // ms
    const juce::Identifier reverbPreCompRelease  ("reverbPreCompRelease");    // ms

    // Reverb > Post-Processing EQ (global, stored as child of Reverbs node)
    const juce::Identifier ReverbPostEQ          ("ReverbPostEQ");
    const juce::Identifier PostEQBand            ("PostEQBand");
    const juce::Identifier reverbPostEQenable    ("reverbPostEQenable");
    const juce::Identifier reverbPostEQshape     ("reverbPostEQshape");
    const juce::Identifier reverbPostEQfreq      ("reverbPostEQfreq");
    const juce::Identifier reverbPostEQgain      ("reverbPostEQgain");
    const juce::Identifier reverbPostEQq         ("reverbPostEQq");
    const juce::Identifier reverbPostEQslope     ("reverbPostEQslope");

    // Reverb > Post-Processing Expander (global, stored as child of Reverbs node)
    const juce::Identifier ReverbPostExp         ("ReverbPostExp");
    const juce::Identifier reverbPostExpBypass   ("reverbPostExpBypass");     // 0=active, 1=bypassed
    const juce::Identifier reverbPostExpThreshold("reverbPostExpThreshold");  // dB
    const juce::Identifier reverbPostExpRatio    ("reverbPostExpRatio");      // ratio 1:N
    const juce::Identifier reverbPostExpAttack   ("reverbPostExpAttack");     // ms
    const juce::Identifier reverbPostExpRelease  ("reverbPostExpRelease");    // ms

    //==========================================================================
    // Effects Channel Parameters
    //==========================================================================
    //
    // Phase 4, commit 1: DECLARATION ONLY. Nothing builds these nodes, reads
    // these properties or routes them yet - the tree builders, accessors, scope
    // routing, bounds table and persistence land in later commits.
    //
    // Schema (Documentation/effects-channels-plan.md section 6.1, with the
    // module-node reshape described below):
    //
    //   <Effects count="N" effectPositionsUserOwned="0">   top-level, ONLY <Effect> children
    //     <Effect id="1">                                  dense: id = index + 1
    //       <Channel/> <Position/> <Feed/> <Return/> <AutomOtion/> <Chain/>
    //       <FxDist/>
    //       <FxEq1><Band id="1".."6"/></FxEq1>  <FxEq2><Band id="1".."6"/></FxEq2>
    //       <FxDyn1/> <FxDyn2/> <FxMod/> <FxPhaser/> <FxTrem/> <FxReverb/>
    //       <FxDelay><Tap id="1".."8"/></FxDelay>
    //       <FxCrush/>
    //       <Sends/>
    //     </Effect>
    //   </Effects>
    //
    // REUSED type identifiers - do NOT redeclare: Channel, Position and
    // AutomOtion (the input section names, :29/:30/:36), Feed and ReverbReturn
    // (declared in the reverb block above; ReverbReturn's C++ name is
    // disambiguated but its XML tag is plain "Return", which is the tag the
    // effects <Return> section uses too), and Band (:42, shared with the output
    // EQ). Only the names that do not already exist are declared here.
    //
    // ELEVEN MODULE NODE TYPES, NOT INSTANCED ONES. The plan drafted
    // <FxEQ id="1">/<FxEQ id="2"> and <FxDyn id="1">/<FxDyn id="2">; that is
    // dropped. Identical property names on two sibling nodes break the one-node
    // rule the snapshot layer depends on (Documentation/CLAUDE.md:1905-1914 -
    // hasProperty is the discriminator), make every first-hit generic search
    // (the setReverbParameter-shaped walk, getTreeForParameter) silently address
    // instance 1 while reporting success, and put two node types in one id
    // namespace, which backfillFromTemplate mishandles by appending a duplicate
    // on every load. Instead each of spatcore's eleven chain SLOTS
    // (spatcore/effects/EffectsTypes.h:54-67, tokens dist, eq1, eq2, dyn1, dyn2,
    // mod, phaser, trem, reverb, delay, crush) gets its own id-less node type,
    // so the tree is a 1:1 transcription of the engine's slot table and
    // getChildWithName resolves an instance without a sub-index.
    //
    // The PROPERTY names stay singular - effectEQgain, effectDynCompThreshold -
    // because the node now carries the instance. CONSEQUENCE: a property name
    // appears on TWO node types (FxEq1/FxEq2, FxDyn1/FxDyn2), so anything that
    // maps a property name to a node must take the instance as well; the wire
    // keeps its instance argument and simply routes to a different node.

    // PREFIX COLLISIONS - A LATER BINDING COMMIT MUST MATCH EXACTLY OR TEST
    // LONGEST-FIRST. The app routes sub-trees with literal startsWith tests on
    // the internal variable name (Source/Network/MCP/MCPGeneratedToolLoader.cpp
    // :862-908 for samplerCell/samplerSet/networkTS/admCart/admPolar/gmShape/
    // gmLayer, and :457-458 for reverbPostEQ/reverbPreEQ), and plan 7.3/7.4
    // specifies that same shape for effects (effectSend, effectFxSend, effectEQ,
    // effectDyn, effectDelayTap). Four families below are strict prefixes of
    // each other, so a naive startsWith binds to the WRONG node and reports
    // success - the exact failure this declaration block exists to prevent:
    //
    //   effectDist   (FxDist module)  swallows effectDistanceAttenPercent (Feed),
    //                                 effectDistanceAttenuation and
    //                                 effectDistanceRatio (Return)
    //   effectDelay  (FxDelay module) swallows effectDelayLatency (Channel)
    //   effectSend   (Sends cells)    swallows the row identifiers
    //                                 effectSendLevels / effectSendOns, which
    //                                 stay string setters
    //   effectMute   (Channel)        is a prefix of effectMutes,
    //                                 effectMuteMacro and effectMuteReverbSends
    //
    // All eight names are plan-faithful and must NOT be renamed to dodge this.
    // Order the tests longest-first, or compare with == against the Identifier.

    // Effects Section Identifiers (node types)
    const juce::Identifier Effects               ("Effects");
    const juce::Identifier Effect                ("Effect");
    const juce::Identifier Chain                 ("Chain");
    const juce::Identifier Sends                 ("Sends");
    const juce::Identifier Tap                   ("Tap");           // <FxDelay> child, id="1".."8"
    const juce::Identifier EffectsGlobal         ("EffectsGlobal"); // Config child, not an <Effects> sibling

    // The eleven chain slots, named for the tokens in spatcore::effects::kSlots
    const juce::Identifier FxDist                ("FxDist");
    const juce::Identifier FxEq1                 ("FxEq1");
    const juce::Identifier FxEq2                 ("FxEq2");
    const juce::Identifier FxDyn1                ("FxDyn1");
    const juce::Identifier FxDyn2                ("FxDyn2");
    const juce::Identifier FxMod                 ("FxMod");
    const juce::Identifier FxPhaser              ("FxPhaser");
    const juce::Identifier FxTrem                ("FxTrem");
    const juce::Identifier FxReverb              ("FxReverb");
    const juce::Identifier FxDelay               ("FxDelay");
    const juce::Identifier FxCrush               ("FxCrush");

    // Effects container property (on <Effects>, not per-channel): the layout
    // ownership latch, twin of the reverb position latch.
    const juce::Identifier effectPositionsUserOwned ("effectPositionsUserOwned");

    // Effect > Channel
    const juce::Identifier effectName            ("effectName");
    const juce::Identifier effectAttenuation     ("effectAttenuation");
    const juce::Identifier effectDelayLatency    ("effectDelayLatency");
    const juce::Identifier effectMinimalLatency  ("effectMinimalLatency");
    const juce::Identifier effectLinkGroup       ("effectLinkGroup");       // 0=unlinked, 1..8
    const juce::Identifier effectMute            ("effectMute");
    const juce::Identifier effectSolo            ("effectSolo");

    // Effect > Position
    const juce::Identifier effectPositionX       ("effectPositionX");
    const juce::Identifier effectPositionY       ("effectPositionY");
    const juce::Identifier effectPositionZ       ("effectPositionZ");
    const juce::Identifier effectCoordinateMode  ("effectCoordinateMode");  // 0=Cartesian, 1=Cylindrical, 2=Spherical
    const juce::Identifier effectReturnOffsetX   ("effectReturnOffsetX");
    const juce::Identifier effectReturnOffsetY   ("effectReturnOffsetY");
    const juce::Identifier effectReturnOffsetZ   ("effectReturnOffsetZ");

    // Effect > Feed (node type Feed, shared with the reverb channels)
    const juce::Identifier effectOrientation     ("effectOrientation");
    const juce::Identifier effectAngleOn         ("effectAngleOn");
    const juce::Identifier effectAngleOff        ("effectAngleOff");
    const juce::Identifier effectPitch           ("effectPitch");
    const juce::Identifier effectHFdamping       ("effectHFdamping");
    const juce::Identifier effectFeedMiniLatency ("effectFeedMiniLatency");
    const juce::Identifier effectDistanceAttenPercent ("effectDistanceAttenPercent");

    // Effect > Return (node type ReverbReturn, whose XML tag is "Return")
    const juce::Identifier effectAttenuationLaw  ("effectAttenuationLaw");      // 0=Log, 1=1/d
    const juce::Identifier effectDistanceAttenuation ("effectDistanceAttenuation");
    const juce::Identifier effectDistanceRatio   ("effectDistanceRatio");
    const juce::Identifier effectCommonAtten     ("effectCommonAtten");
    const juce::Identifier effectHFshelf         ("effectHFshelf");
    const juce::Identifier effectMutes           ("effectMutes");              // packed CSV, one token per output
    const juce::Identifier effectMuteMacro       ("effectMuteMacro");
    const juce::Identifier effectMuteReverbSends ("effectMuteReverbSends");

    // Effect > AutomOtion (the input set minus StayReturn - an effect return
    // always returns; ranges mirror the inputOtomo* set, with the single
    // addition noted at the AutomOtion defaults block)
    const juce::Identifier effectOtomoX          ("effectOtomoX");
    const juce::Identifier effectOtomoY          ("effectOtomoY");
    const juce::Identifier effectOtomoZ          ("effectOtomoZ");
    const juce::Identifier effectOtomoAbsoluteRelative ("effectOtomoAbsoluteRelative");
    const juce::Identifier effectOtomoSpeedProfile ("effectOtomoSpeedProfile");
    const juce::Identifier effectOtomoDuration   ("effectOtomoDuration");
    const juce::Identifier effectOtomoCurve      ("effectOtomoCurve");
    const juce::Identifier effectOtomoTrigger    ("effectOtomoTrigger");
    const juce::Identifier effectOtomoThreshold  ("effectOtomoThreshold");
    const juce::Identifier effectOtomoReset      ("effectOtomoReset");
    const juce::Identifier effectOtomoPauseResume ("effectOtomoPauseResume");
    const juce::Identifier effectOtomoCoordinateMode ("effectOtomoCoordinateMode");  // 0=Cartesian, 1=Cylindrical, 2=Spherical
    const juce::Identifier effectOtomoR          ("effectOtomoR");             // Cylindrical radius
    const juce::Identifier effectOtomoTheta      ("effectOtomoTheta");         // Azimuth angle (shared cyl/sph)
    const juce::Identifier effectOtomoRsph       ("effectOtomoRsph");          // Spherical radius
    const juce::Identifier effectOtomoPhi        ("effectOtomoPhi");           // Elevation angle

    // Effect > Chain
    const juce::Identifier effectChainOrder      ("effectChainOrder");         // permutation of the 11 slot tokens
    const juce::Identifier effectChainBypass     ("effectChainBypass");

    // Effect > FxDist
    const juce::Identifier effectDistBypass      ("effectDistBypass");
    const juce::Identifier effectDistDrive       ("effectDistDrive");          // dB
    const juce::Identifier effectDistShape       ("effectDistShape");          // 0=hard clip, 1=tanh, continuous
    const juce::Identifier effectDistBias        ("effectDistBias");           // asymmetry -> even harmonics
    const juce::Identifier effectDistPreLoShelfFreq ("effectDistPreLoShelfFreq");
    const juce::Identifier effectDistPreLoShelfGain ("effectDistPreLoShelfGain");
    const juce::Identifier effectDistPreHiShelfFreq ("effectDistPreHiShelfFreq");
    const juce::Identifier effectDistPreHiShelfGain ("effectDistPreHiShelfGain");
    const juce::Identifier effectDistPostLoShelfFreq ("effectDistPostLoShelfFreq");
    const juce::Identifier effectDistPostLoShelfGain ("effectDistPostLoShelfGain");
    const juce::Identifier effectDistPostHiShelfFreq ("effectDistPostHiShelfFreq");
    const juce::Identifier effectDistPostHiShelfGain ("effectDistPostHiShelfGain");
    const juce::Identifier effectDistOutput      ("effectDistOutput");         // dB
    const juce::Identifier effectDistMix         ("effectDistMix");            // wet %
    const juce::Identifier effectDistOversample  ("effectDistOversample");     // 0=auto, 1=off, 2=2x, 3=4x

    // Effect > FxEq1 / FxEq2 - the bypass lives on the module node, the five
    // band properties on its six <Band id="1".."6"> children. Both instances
    // carry the same property names on different node types (see the header
    // note): the node IS the instance.
    const juce::Identifier effectEQBypass        ("effectEQBypass");
    const juce::Identifier effectEQshape         ("effectEQshape");            // output-EQ shape ids 1..7
    const juce::Identifier effectEQfreq          ("effectEQfreq");             // Hz
    const juce::Identifier effectEQgain          ("effectEQgain");             // dB
    const juce::Identifier effectEQq             ("effectEQq");
    const juce::Identifier effectEQslope         ("effectEQslope");

    // Effect > FxDyn1 / FxDyn2 (a compressor stage followed by an expander stage)
    const juce::Identifier effectDynBypass       ("effectDynBypass");
    const juce::Identifier effectDynDetector     ("effectDynDetector");        // 0=Peak, 1=RMS
    const juce::Identifier effectDynLookahead    ("effectDynLookahead");       // ms, delays the AUDIO (adds latency)
    const juce::Identifier effectDynMakeup       ("effectDynMakeup");          // dB
    const juce::Identifier effectDynAutoMakeup   ("effectDynAutoMakeup");
    const juce::Identifier effectDynCompOn       ("effectDynCompOn");
    const juce::Identifier effectDynCompThreshold ("effectDynCompThreshold");  // dB
    const juce::Identifier effectDynCompRatio    ("effectDynCompRatio");       // :1, 100 = limiter
    const juce::Identifier effectDynCompKnee     ("effectDynCompKnee");        // dB
    const juce::Identifier effectDynCompAttack   ("effectDynCompAttack");      // ms
    const juce::Identifier effectDynCompRelease  ("effectDynCompRelease");     // ms
    const juce::Identifier effectDynCompDetectorDelay ("effectDynCompDetectorDelay"); // ms, delays the DETECTOR (transient pass, no latency)
    const juce::Identifier effectDynCompScLoCut  ("effectDynCompScLoCut");     // Hz
    const juce::Identifier effectDynCompScHiCut  ("effectDynCompScHiCut");     // Hz
    const juce::Identifier effectDynExpOn        ("effectDynExpOn");
    const juce::Identifier effectDynExpThreshold ("effectDynExpThreshold");    // dB
    const juce::Identifier effectDynExpRatio     ("effectDynExpRatio");        // :1, 100 = gate
    const juce::Identifier effectDynExpAttack    ("effectDynExpAttack");       // ms
    const juce::Identifier effectDynExpRelease   ("effectDynExpRelease");      // ms
    const juce::Identifier effectDynExpRange     ("effectDynExpRange");        // dB
    const juce::Identifier effectDynExpHold      ("effectDynExpHold");         // ms
    const juce::Identifier effectDynExpScLoCut   ("effectDynExpScLoCut");      // Hz
    const juce::Identifier effectDynExpScHiCut   ("effectDynExpScHiCut");      // Hz

    // Effect > FxMod (chorus / flanger)
    const juce::Identifier effectModBypass       ("effectModBypass");
    const juce::Identifier effectModMode         ("effectModMode");            // 0=Chorus, 1=Flanger
    const juce::Identifier effectModRate         ("effectModRate");            // Hz
    const juce::Identifier effectModDepth        ("effectModDepth");           // % of the centre delay
    const juce::Identifier effectModDelay        ("effectModDelay");           // ms
    const juce::Identifier effectModFeedback     ("effectModFeedback");        // signed %
    const juce::Identifier effectModVoices       ("effectModVoices");
    const juce::Identifier effectModShape        ("effectModShape");           // LFOWaveforms shape id
    const juce::Identifier effectModPhase        ("effectModPhase");           // degrees, spreads linked channels
    const juce::Identifier effectModLoCut        ("effectModLoCut");           // Hz
    const juce::Identifier effectModThroughZero  ("effectModThroughZero");
    const juce::Identifier effectModMix          ("effectModMix");             // wet %

    // Effect > FxPhaser
    const juce::Identifier effectPhaserBypass    ("effectPhaserBypass");
    const juce::Identifier effectPhaserStages    ("effectPhaserStages");       // validated: 4, 6, 8 or 12
    const juce::Identifier effectPhaserCentre    ("effectPhaserCentre");       // Hz
    const juce::Identifier effectPhaserSpread    ("effectPhaserSpread");       // octaves
    const juce::Identifier effectPhaserRate      ("effectPhaserRate");         // Hz
    const juce::Identifier effectPhaserDepth     ("effectPhaserDepth");        // octaves
    const juce::Identifier effectPhaserShape     ("effectPhaserShape");        // LFOWaveforms shape id
    const juce::Identifier effectPhaserFeedback  ("effectPhaserFeedback");     // signed %
    const juce::Identifier effectPhaserMix       ("effectPhaserMix");          // wet %

    // Effect > FxTrem
    const juce::Identifier effectTremBypass      ("effectTremBypass");
    const juce::Identifier effectTremRate        ("effectTremRate");           // Hz
    const juce::Identifier effectTremDepth       ("effectTremDepth");          // dB (dB-linear modulation)
    const juce::Identifier effectTremShape       ("effectTremShape");          // 0=sine .. 1=triangle, continuous
    const juce::Identifier effectTremMix         ("effectTremMix");            // wet %

    // Effect > FxReverb (the per-chain reverb module; unrelated to the <Reverbs> family)
    const juce::Identifier effectReverbBypass    ("effectReverbBypass");
    const juce::Identifier effectReverbModel     ("effectReverbModel");        // 0=FDN (v1); 1=Plate, 2=SDN, 3=IR later
    const juce::Identifier effectReverbType      ("effectReverbType");         // preset within the model
    const juce::Identifier effectReverbPredelay  ("effectReverbPredelay");     // ms
    const juce::Identifier effectReverbRT60      ("effectReverbRT60");         // seconds
    const juce::Identifier effectReverbRT60LowMult ("effectReverbRT60LowMult");
    const juce::Identifier effectReverbRT60HighMult ("effectReverbRT60HighMult");
    const juce::Identifier effectReverbCrossoverLow ("effectReverbCrossoverLow");   // Hz
    const juce::Identifier effectReverbCrossoverHigh ("effectReverbCrossoverHigh"); // Hz
    const juce::Identifier effectReverbDiffusion ("effectReverbDiffusion");
    const juce::Identifier effectReverbSize      ("effectReverbSize");
    const juce::Identifier effectReverbTone      ("effectReverbTone");         // Hz
    const juce::Identifier effectReverbMix       ("effectReverbMix");          // wet %

    // Effect > FxDelay (multitap) - the two per-tap properties live on its
    // eight <Tap id="1".."8"> children, everything else on the module node.
    const juce::Identifier effectDelayBypass     ("effectDelayBypass");
    const juce::Identifier effectDelayTime       ("effectDelayTime");          // ms
    const juce::Identifier effectDelayTaps       ("effectDelayTaps");
    const juce::Identifier effectDelayTapMode    ("effectDelayTapMode");       // 0=Manual, 1=Pattern
    const juce::Identifier effectDelayPattern    ("effectDelayPattern");       // 0=Equal, 1=Dotted, 2=Triplet, 3=Golden
    const juce::Identifier effectDelayFeedback   ("effectDelayFeedback");      // %
    const juce::Identifier effectDelayFeedbackTap ("effectDelayFeedbackTap");  // 0=last, 1..8
    const juce::Identifier effectDelayInLoCut    ("effectDelayInLoCut");       // Hz
    const juce::Identifier effectDelayFbLoShelfFreq ("effectDelayFbLoShelfFreq");
    const juce::Identifier effectDelayFbLoShelfGain ("effectDelayFbLoShelfGain");
    const juce::Identifier effectDelayFbHiShelfFreq ("effectDelayFbHiShelfFreq");
    const juce::Identifier effectDelayFbHiShelfGain ("effectDelayFbHiShelfGain");
    const juce::Identifier effectDelayModRate    ("effectDelayModRate");       // Hz
    const juce::Identifier effectDelayModDepth   ("effectDelayModDepth");      // % of the delay time
    const juce::Identifier effectDelayDiffusion  ("effectDelayDiffusion");
    const juce::Identifier effectDelayGlide      ("effectDelayGlide");         // ms
    const juce::Identifier effectDelayMix        ("effectDelayMix");           // wet %
    const juce::Identifier effectDelayTapTime    ("effectDelayTapTime");       // ms,  per <Tap>
    const juce::Identifier effectDelayTapLevel   ("effectDelayTapLevel");      // dB,  per <Tap>

    // Effect > FxCrush (bitcrusher / downsampler)
    const juce::Identifier effectCrushBypass     ("effectCrushBypass");
    const juce::Identifier effectCrushBits       ("effectCrushBits");          // fractional allowed
    const juce::Identifier effectCrushRate       ("effectCrushRate");          // Hz, clamped to the device rate
    const juce::Identifier effectCrushFilter     ("effectCrushFilter");        // 0=hold (aliasing), 1=anti-aliased
    const juce::Identifier effectCrushDither     ("effectCrushDither");        // dB, -96 = off
    const juce::Identifier effectCrushMix        ("effectCrushMix");           // wet %

    // Effect > Sends - four packed CSV rows on one <Sends> node. effectSend*
    // are 64 wide and keyed by input PERMANENT NUMBER; effectFxSend* are 32
    // wide and keyed by dense effect index, with the diagonal forced off.
    //
    // DECLARED HERE, NOT STAMPED BY ANY BUILDER, because createEffectSendsSection
    // refuses to stamp a CSV whose width it cannot yet maintain and the rows
    // arrive at runtime instead.
    //
    // Eight names in this family are declared and never stamped; these four and
    // the four cell pseudo-identifiers below. The difference is the whole point:
    // a NODE WILL CARRY these four, so stripObsoleteEffectProperties must exempt
    // them by name or the load path deletes an operator's send routing with no
    // undo - while no node ever carries a cell identifier, so exempting one there
    // would only protect a property that is already a bug. Rename or retire one
    // of these four and its entry in that hook must move in the same commit.
    const juce::Identifier effectSendLevels      ("effectSendLevels");
    const juce::Identifier effectSendOns         ("effectSendOns");
    const juce::Identifier effectFxSendLevels    ("effectFxSendLevels");
    const juce::Identifier effectFxSendOns       ("effectFxSendOns");

    // Sends CELL pseudo-identifiers. NO node ever carries these: they exist so
    // the OSC parser, the ramper and the OSCQuery cell nodes have something to
    // validate a single cell against, while the generic parameter path finds no
    // tree for them and therefore refuses a write instead of overwriting a whole
    // row with a scalar. Never stamp one onto a node.
    const juce::Identifier effectSendLevel       ("effectSendLevel");
    const juce::Identifier effectSendOn          ("effectSendOn");
    const juce::Identifier effectFxSendLevel     ("effectFxSendLevel");
    const juce::Identifier effectFxSendOn        ("effectFxSendOn");

    // Config > EffectsGlobal (global, stored as a child of <Config> - NOT a
    // sibling of the <Effect> channels). Every name below starts with "effect",
    // so each one needs a BY-NAME exception ahead of getParameterScope's prefix
    // test, exactly like reverbsMapVisible; without it the prefix test routes
    // them into the Effect branch, which finds no channel node and drops the
    // write in silence.
    const juce::Identifier effectsGlobalLinkNames ("effectsGlobalLinkNames");        // CSV, 8 group names
    const juce::Identifier effectsGlobalLinkMode  ("effectsGlobalLinkMode");         // 0=off, 1=absolute, 2=relative
    const juce::Identifier effectsGlobalFxFeedGeometric ("effectsGlobalFxFeedGeometric");
    const juce::Identifier effectsGlobalWorkerThreads ("effectsGlobalWorkerThreads"); // -1=auto, 0..4 fixed
    const juce::Identifier effectsGlobalReturnCushion ("effectsGlobalReturnCushion"); // 0=auto, 1..3 blocks
    const juce::Identifier effectsGlobalLoopGuard ("effectsGlobalLoopGuard");
    const juce::Identifier effectsGlobalLoopGuardCeiling ("effectsGlobalLoopGuardCeiling"); // dBFS peak
    const juce::Identifier effectsGlobalMaxDelaySeconds ("effectsGlobalMaxDelaySeconds"); // sizes every delay module's buffer
    const juce::Identifier effectsGlobalFeedGpuDevice ("effectsGlobalFeedGpuDevice"); // compute device id ("cpu"/"hip:0"/...)

    //==========================================================================
    // Sampler Parameters
    //==========================================================================

    // ValueTree Type Identifiers
    const juce::Identifier Sampler               ("Sampler");
    const juce::Identifier SamplerCell           ("SamplerCell");
    const juce::Identifier SamplerSet            ("SamplerSet");

    // Snapshot scope section for ADM-OSC
    const juce::Identifier ADMMapping            ("ADMMapping");

    // Config > UI (master toggle)
    const juce::Identifier samplerEnabled        ("samplerEnabled");          // bool: global enable
    const juce::Identifier samplerBlockSerial    ("samplerBlockSerial");      // string: BLOCKS device ID

    // Per-channel (stored in Channel section)
    const juce::Identifier inputSamplerActive    ("inputSamplerActive");      // 0=OFF, 1=ON
    const juce::Identifier samplerMidiZoneQuadrant ("samplerMidiZoneQuadrant"); // 0=full, 1-4=quadrant
    const juce::Identifier inputSamplerActiveSet ("inputSamplerActiveSet");    // 0-based index of active set

    // SamplerCell properties (per cell in 6x6 grid)
    const juce::Identifier samplerCellName       ("samplerCellName");         // display name
    const juce::Identifier samplerCellFile       ("samplerCellFile");         // relative path in samples/
    const juce::Identifier samplerCellInTime     ("samplerCellInTime");       // ms (fade-in / start offset)
    const juce::Identifier samplerCellOutTime    ("samplerCellOutTime");      // ms (fade-out)
    const juce::Identifier samplerCellOffsetX    ("samplerCellOffsetX");      // position offset meters
    const juce::Identifier samplerCellOffsetY    ("samplerCellOffsetY");
    const juce::Identifier samplerCellOffsetZ    ("samplerCellOffsetZ");
    const juce::Identifier samplerCellAttenuation("samplerCellAttenuation");  // dB (0 = no change)

    // SamplerSet properties (dynamic, user creates/deletes)
    const juce::Identifier samplerSetName        ("samplerSetName");
    const juce::Identifier samplerSetPlayMode    ("samplerSetPlayMode");      // 0=sequential, 1=round-robin
    const juce::Identifier samplerSetCells       ("samplerSetCells");         // comma-separated cell indices
    const juce::Identifier samplerSetPosX        ("samplerSetPosX");          // absolute base position
    const juce::Identifier samplerSetPosY        ("samplerSetPosY");
    const juce::Identifier samplerSetPosZ        ("samplerSetPosZ");
    const juce::Identifier samplerSetLevel       ("samplerSetLevel");         // dB

    // SamplerSet > Pressure mappings (per set)
    const juce::Identifier samplerSetPressLevelEnabled  ("samplerSetPressLevelEnabled");  // 0/1
    const juce::Identifier samplerSetPressLevelDir      ("samplerSetPressLevelDir");      // 0=positive, 1=negative
    const juce::Identifier samplerSetPressLevelCurve    ("samplerSetPressLevelCurve");    // 0.0-1.0
    const juce::Identifier samplerSetPressZEnabled      ("samplerSetPressZEnabled");
    const juce::Identifier samplerSetPressZDir          ("samplerSetPressZDir");
    const juce::Identifier samplerSetPressZCurve        ("samplerSetPressZCurve");
    const juce::Identifier samplerSetPressHFEnabled     ("samplerSetPressHFEnabled");
    const juce::Identifier samplerSetPressHFDir         ("samplerSetPressHFDir");
    const juce::Identifier samplerSetPressHFCurve       ("samplerSetPressHFCurve");
    const juce::Identifier samplerSetPressXYEnabled     ("samplerSetPressXYEnabled");     // 0/1 (XY joystick)
    const juce::Identifier samplerSetPressXYScale       ("samplerSetPressXYScale");       // m per update

    //==========================================================================
    // Lightpad Parameters
    //==========================================================================

    // Config > UI (global settings)
    const juce::Identifier lightpadPad0Split     ("lightpadPad0Split");       // int: 0=full, 1=split into 4
    const juce::Identifier lightpadPad1Split     ("lightpadPad1Split");
    const juce::Identifier lightpadPad2Split     ("lightpadPad2Split");
    const juce::Identifier lightpadPad0DeviceId  ("lightpadPad0DeviceId");    // string: Block UID
    const juce::Identifier lightpadPad1DeviceId  ("lightpadPad1DeviceId");
    const juce::Identifier lightpadPad2DeviceId  ("lightpadPad2DeviceId");
    const juce::Identifier lightpadSensitivity   ("lightpadSensitivity");     // float: m per unit deflection

    // Sampler controller mode (stored in Config > UI)
    const juce::Identifier samplerControllerMode ("SamplerControllerMode");  // int: 0=Off, 1=Lightpad, 2=Remote
    const juce::Identifier remotePadGridLayout   ("RemotePadGridLayout");   // int: 0=3x2, 1=5x3

    // Per-channel (stored in Channel section)
    const juce::Identifier lightpadZoneId        ("lightpadZoneId");          // int: -1=unassigned, 0-14=zone

} // namespace WFSParameterIDs
