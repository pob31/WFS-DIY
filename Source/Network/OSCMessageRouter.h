#pragma once

#include <JuceHeader.h>
#include "OSCProtocolTypes.h"
#include "../Parameters/WFSParameterIDs.h"

namespace WFSNetwork
{

/**
 * OSCMessageRouter
 *
 * Routes incoming OSC messages to parameter updates.
 * Parses OSC address patterns and extracts values.
 * Handles both standard OSC and REMOTE protocol messages.
 */
class OSCMessageRouter
{
public:
    //==========================================================================
    // Parsed Message Types
    //==========================================================================

    struct ParsedInputMessage
    {
        juce::Identifier paramId;
        int channelId = 0;
        juce::var value;
        float rampTimeSec = 0.0f;  // Optional 3rd OSC arg: transition time in seconds (0 = apply immediately)
        float rampTimeSecRequested = 0.0f; // As sent by the client, before the [0, 600] s clamp
        bool rampArgIgnored = false; // A trailing numeric arg was present but the param is not fade-capable
        int muteOutput = 0;          // inputMutes only: 1-based output of a single-output edit, 0 = whole list
        bool valid = false;
        juce::String invalidReason; // Set when valid==false because the value failed range gate.
    };

    struct ParsedOutputMessage
    {
        juce::Identifier paramId;
        int channelId = 0;
        int bandIndex = 0;  // For EQ parameters (1-6)
        juce::var value;
        bool valid = false;
        bool isEQparam = false;  // True for EQ parameters that need band index
        juce::String invalidReason;
    };

    struct ParsedReverbMessage
    {
        juce::Identifier paramId;
        int channelId = 0;
        int bandIndex = 0;  // For EQ parameters (1-4)
        juce::var value;
        bool valid = false;
        bool isEQparam = false;  // True for EQ parameters that need band index
        juce::String invalidReason;
    };

    /** One inbound /wfs/effect/ message.

        The nine effects GLOBALS are /wfs/config/effects/* and are parsed by
        parseConfigMessage like every other global; effectsMapVisible is the one
        the published contract keeps under this prefix, and it arrives here as
        Kind::Global.

        CLASSIFICATION IS BY IDENTIFIER, BEFORE ANY ARGUMENT IS READ. The
        effects family publishes EIGHT argument shapes over one prefix
        (Documentation/WFS-UI_effects.csv, column "OSC path"), and nothing in
        the argument list says which one arrived: `<ID> <instance> <value>` and
        `<ID> <value> <fadeSeconds>` are both "int, number, number" on the
        wire. The only thing that can tell them apart is the parameter NAME, so
        the parser resolves the address to an Identifier, looks its Kind up in
        ONE table (getEffectParamKind) and only then reads arguments.

        That is not a style preference. parseReverbMessage classified its EQ
        parameters with a startsWith on the address instead, the reverb keys
        turned out to be spelled differently from the test, and for months every
        standard-form pre-EQ write stored the BAND INDEX as the value - fixed on
        this branch in 76d5afa, and this struct exists so 174 effect parameters
        do not inherit the shape of that mistake.

        SUB-INDICES ARE 1-BASED ON THE WIRE, like <ID> itself and like the band
        argument of the reverb and MCP EQ paths. The accessors they feed
        (getEffectEQBand, getEffectDynSection, getEffectDelayTap) are 0-based,
        so the dispatch subtracts one in exactly one place per kind. */
    struct ParsedEffectMessage
    {
        enum class Kind
        {
            Unknown,      // not an effect address, or a name not in the map
            Scalar,       // /wfs/effect/<param> <ID> <value>            (122 rows)
            Instanced,    // /wfs/effect/<param> <ID> <instance> <value>  (24 rows)
            Band,         // /wfs/effect/<param> <ID> <inst> <band> <v>    (5 rows)
            Tap,          // /wfs/effect/<param> <ID> <tap> <value>        (2 rows)
            InputCell,    // /wfs/effect/<param> <ID> <input number> <v>   (2 rows)
            FxCell,       // /wfs/effect/<param> <ID> <src effect ID> <v>  (2 rows)
            Row,          // /wfs/effect/<param> <ID> "<csv>"              (6 rows)
            Global,       // /wfs/effect/mapVisible <value>, /wfs/config/effects/*
            Verb          // snapshot/clear/clearAll/selected/editOnMap: not a parameter
        };

        Kind kind = Kind::Unknown;
        juce::Identifier paramId;
        int channelId = 0;       // 1-based effect ID (dense: index = id - 1)
        int instanceIndex = 0;   // 1-based FxEq1/FxEq2 or FxDyn1/FxDyn2
        int bandIndex = 0;       // 1-based <Band> under the EQ instance
        int tapIndex = 0;        // 1-based <Tap> under <FxDelay>
        int cellIndex = 0;       // InputCell: input PERMANENT number. FxCell: source effect ID. Both 1-based.
        juce::var value;
        float rampTimeSec = 0.0f;          // parsed, NOT applied — see rampArgIgnored
        float rampTimeSecRequested = 0.0f; // as sent, before the [0, 600] s clamp
        bool rampArgIgnored = false;       // a ramp arg was present and the value was applied instantly
        bool valid = false;
        juce::String invalidReason;
        juce::String verb;       // Kind::Verb only: "snapshot/store", "clear", ...
    };

    struct ParsedRemoteInput
    {
        enum class Type {
            ChannelSelect,    // /remoteInput/inputNumber <ID> - request all params
            PositionDelta,    // Legacy: /remoteInput/positionX <ID> <inc/dec> <delta>
            ParameterSet,     // /remoteInput/<param> <ID> <value> - absolute set
            ParameterDelta,   // /remoteInput/<param> <ID> <inc/dec> <delta> - relative change
            PositionXY        // /remoteInput/positionXY <ID> <x> <y> - atomic XY position
        };

        Type type = Type::ChannelSelect;
        int channelId = 0;
        juce::Identifier paramId;                              // Which parameter (for ParameterSet/Delta)
        Axis axis = Axis::X;                                   // For legacy PositionDelta
        DeltaDirection direction = DeltaDirection::Increment;  // For delta types
        juce::var value;                                       // Value or delta amount
        float deltaValue = 0.0f;                               // Legacy: delta for PositionDelta
        float posX = 0.0f;                                     // For PositionXY: X coordinate
        float posY = 0.0f;                                     // For PositionXY: Y coordinate
        bool valid = false;
        juce::String invalidReason;
    };

    struct ParsedArrayAdjustMessage
    {
        juce::Identifier paramId;  // Output parameter to adjust
        int arrayId = 0;           // Array/cluster ID (1-based from remote)
        float valueChange = 0.0f;  // Delta to apply to parameter
        bool valid = false;
    };

    /** /arrayAdjust/mute <array # 1-10> <0/1> — absolute, unlike the other
        /arrayAdjust/ deltas; sets the session array mute (ArrayMuteState). */
    struct ParsedArrayMuteMessage
    {
        int arrayId = 0;
        bool muted = false;
        bool valid = false;
    };

    struct ParsedConfigMessage
    {
        juce::Identifier paramId;  // Config parameter identifier
        juce::var value;           // Parameter value (float or int)
        bool valid = false;
        juce::String invalidReason;
    };

    struct ParsedClusterMoveMessage
    {
        enum class Type {
            ClusterMove,      // /cluster/move <clusterId> <deltaX> <deltaY>
            BarycenterMove,   // /cluster/barycenter/move <clusterId> <deltaX> <deltaY>
            PositionXY        // /cluster/positionXY <clusterId> <stageX> <stageY>
        };

        Type type = Type::ClusterMove;
        int clusterId = 0;       // Cluster ID (1-10)
        float deltaX = 0.0f;     // X delta in meters
        float deltaY = 0.0f;     // Y delta in meters
        bool valid = false;
    };

    struct ParsedClusterScaleRotationMessage
    {
        enum class Type {
            Scale,      // /cluster/scale <clusterId> <factor>
            Rotation    // /cluster/rotation <clusterId> <angleDeg>
        };

        Type type = Type::Scale;
        int clusterId = 0;       // Cluster ID (1-10)
        float value = 0.0f;      // Scale factor or rotation angle (degrees)
        bool valid = false;
    };

    struct ParsedClusterCumulativeScaleRotationMessage
    {
        int clusterId = 0;           // Cluster ID (1-10)
        float cumulativeScale = 1.0f;    // Total scale from gesture start
        float cumulativeRotation = 0.0f; // Total rotation in degrees from gesture start
        bool valid = false;
    };

    struct ParsedADMOSCMessage
    {
        enum class Type { XYZ, AED, X, Y, Z, XY, Azim, Elev, Dist, Gain, Name };
        Type type = Type::XYZ;
        int objectId = 0;       // 1-based object number
        float v1 = 0.0f;       // x or azimuth or gain
        float v2 = 0.0f;       // y or elevation
        float v3 = 0.0f;       // z or distance
        juce::String stringValue;  // for Name type
        bool valid = false;

        bool isCartesian() const { return type == Type::XYZ || type == Type::X || type == Type::Y || type == Type::Z || type == Type::XY; }
        bool isPolar()     const { return type == Type::AED || type == Type::Azim || type == Type::Elev || type == Type::Dist; }
    };

    //==========================================================================
    // Message Routing
    //==========================================================================

    /**
     * Parse a standard OSC message.
     * Determines if it's an input, output, reverb, or config message and extracts data.
     */
    static ParsedInputMessage parseInputMessage(const juce::OSCMessage& message);
    static ParsedOutputMessage parseOutputMessage(const juce::OSCMessage& message);
    static ParsedReverbMessage parseReverbMessage(const juce::OSCMessage& message);
    static ParsedEffectMessage parseEffectMessage(const juce::OSCMessage& message);
    static ParsedConfigMessage parseConfigMessage(const juce::OSCMessage& message);

    /**
     * Parse a REMOTE protocol input message from Android app.
     * Handles /remoteInput/... addresses.
     */
    static ParsedRemoteInput parseRemoteInputMessage(const juce::OSCMessage& message);

    /**
     * Parse an array adjustment message from remote.
     * Handles /arrayAdjust/... addresses for bulk output parameter changes.
     */
    static ParsedArrayAdjustMessage parseArrayAdjustMessage(const juce::OSCMessage& message);

    /** True for /arrayAdjust/mute (checked before parseArrayAdjustMessage,
        which rejects it: the value is a state, not a delta). */
    static bool isArrayMuteAddress(const juce::String& address);
    static ParsedArrayMuteMessage parseArrayMuteMessage(const juce::OSCMessage& message);

    /**
     * Parse a cluster move message from remote.
     * Handles /cluster/move and /cluster/barycenter/move addresses.
     */
    static ParsedClusterMoveMessage parseClusterMoveMessage(const juce::OSCMessage& message);

    /**
     * Parse a cluster scale/rotation message from remote.
     * Handles /cluster/scale and /cluster/rotation addresses.
     */
    static ParsedClusterScaleRotationMessage parseClusterScaleRotationMessage(const juce::OSCMessage& message);

    /**
     * Parse a combined cumulative scale+rotation message from remote.
     * Handles /cluster/scaleRotation address.
     */
    static ParsedClusterCumulativeScaleRotationMessage parseClusterCumulativeScaleRotationMessage(const juce::OSCMessage& message);

    /**
     * Check if an address matches input, output, reverb, or config patterns.
     */
    static bool isInputAddress(const juce::String& address);
    static bool isOutputAddress(const juce::String& address);
    static bool isReverbAddress(const juce::String& address);
    static bool isEffectAddress(const juce::String& address);
    static bool isConfigAddress(const juce::String& address);
    static bool isRemoteInputAddress(const juce::String& address);
    static bool isArrayAdjustAddress(const juce::String& address);
    static bool isClusterMoveAddress(const juce::String& address);
    static bool isClusterScaleRotationAddress(const juce::String& address);
    static bool isClusterCumulativeScaleRotationAddress(const juce::String& address);
    static bool isClusterLFOAddress(const juce::String& address);
    static bool isADMOSCAddress(const juce::String& address);

    /** Parse an ADM-OSC message (/adm/obj/N/xyz, /adm/obj/N/aed, etc.) */
    static ParsedADMOSCMessage parseADMOSCMessage(const juce::OSCMessage& message);

    //==========================================================================
    // Address Pattern Matching
    //==========================================================================

    /**
     * Get the parameter identifier from an OSC address.
     * @param address The OSC address (e.g., "/wfs/input/attenuation")
     * @return The parameter ID or empty Identifier if not found
     */
    static juce::Identifier getInputParamId(const juce::String& address);
    static juce::Identifier getOutputParamId(const juce::String& address);
    static juce::Identifier getReverbParamId(const juce::String& address);
    static juce::Identifier getEffectParamId(const juce::String& address);
    static juce::Identifier getConfigParamId(const juce::String& address);

    /**
     * THE CLASSIFICATION TABLE. One lookup, keyed by Identifier, that says how
     * many arguments a message for this parameter carries and what they mean.
     * Every effect parameter has exactly one entry; an unknown Identifier
     * answers Kind::Unknown. See ParsedEffectMessage for why this is a table
     * and not a set of startsWith tests on the address.
     */
    static ParsedEffectMessage::Kind getEffectParamKind(const juce::Identifier& paramId);

    /**
     * True if the given input parameter accepts an optional 3rd OSC argument
     * specifying a transition time in seconds. See
     * Documentation/WFS-UI_input.csv, column "OSC path optional value".
     */
    static bool isInputParamRampCapable(const juce::Identifier& paramId);

    /**
     * The 98 effect parameters marked "extra value is transition time in
     * seconds" in Documentation/WFS-UI_effects.csv, column "OSC path optional
     * value".
     *
     * PARSED BUT NOT YET APPLIED. The ramper (OSCParameterRamper) is still
     * hard-wired to getInputParameter/setInputParameter at its three call
     * sites, so an effects ramp cannot run until that generalisation lands.
     * Until then a ramp argument on one of these is accepted, the value is
     * applied INSTANTLY and ParsedEffectMessage::rampArgIgnored is set so the
     * dispatch can say so once in the log - the same three-field contract
     * ParsedInputMessage already uses for a ramp arg on a non-ramp parameter.
     * The set is here now so that generalisation only has to connect it.
     */
    static bool isEffectParamRampCapable(const juce::Identifier& paramId);

    //==========================================================================
    // Value Extraction
    //==========================================================================

    /**
     * Extract a float value from OSC arguments.
     * Handles both float32 and int32 arguments.
     */
    static float extractFloat(const juce::OSCArgument& arg);

    /**
     * Extract an int value from OSC arguments.
     */
    static int extractInt(const juce::OSCArgument& arg);

    /**
     * Extract a string value from OSC arguments.
     */
    static juce::String extractString(const juce::OSCArgument& arg);

    /**
     * True when the string is a plain decimal number (optional sign, digits,
     * at most one dot — e.g. "10", "-3.5", "10000."). Used to accept numeric
     * values/fade times from clients that type OSC arguments as strings
     * (QLab custom messages are the motivating case).
     */
    static bool isNumericString(const juce::String& s);

    /**
     * Like extractFloat, but also accepts a string argument that is a plain
     * decimal number. Non-numeric strings (e.g. "inc"/"dec") return 0.
     */
    static float extractFloatLenient(const juce::OSCArgument& arg);

    /**
     * Extract the parameter portion from an OSC address.
     * e.g., "/wfs/input/attenuation" -> "attenuation"
     */
    static juce::String extractParamName(const juce::String& address);

    /**
     * Validate that every numeric argument in the message is finite.
     * Returns true if all floats are finite (or there are no float args).
     * Returns false on the first NaN/Inf encountered, writing a short
     * description into outReason (e.g. "non-finite float at arg 1 (NaN)").
     *
     * Used as a gate at the OSC entry path: messages carrying NaN/Inf
     * floats are rejected before they can corrupt the ValueTree (e.g.
     * NaN stage geometry → asserts in jlimit at every later constraint
     * application).
     */
    static bool hasOnlyFiniteFloats(const juce::OSCMessage& message,
                                    juce::String& outReason);

    /** Remote address map: paramName -> parameterID, for bulk state dump */
    static const std::map<juce::String, juce::Identifier>& getRemoteAddressMap();

    /** Address maps: oscParamName -> parameterID (used by OSCQuery for namespace discovery) */
    static const std::map<juce::String, juce::Identifier>& getInputAddressMap();

    /** Input names accepted on receive but not published by OSCQuery: the names
        this app itself sends (OSCMessageBuilder, QLab snapshot cues) where they
        differ from getInputAddressMap's. Consulted only after that map misses, so
        the published namespace and its reverse lookup stay as they are. */
    static const std::map<juce::String, juce::Identifier>& getInputInboundAliases();

    static const std::map<juce::String, juce::Identifier>& getOutputAddressMap();
    static const std::map<juce::String, juce::Identifier>& getReverbAddressMap();

    /** oscParamName -> parameterID for the 164 addressable /wfs/effect/ names.
        The nine effects GLOBALS are not here: they are /wfs/config/effects/*
        full paths and live in getConfigAddressMap, like every other global.
        The one exception is "mapVisible", which the published contract puts
        under /wfs/effect/ although effectsMapVisible is a Config property. */
    static const std::map<juce::String, juce::Identifier>& getEffectAddressMap();

    static const std::map<juce::String, juce::Identifier>& getConfigAddressMap();

private:
};

} // namespace WFSNetwork
