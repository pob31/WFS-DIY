#pragma once

#include <JuceHeader.h>

namespace WFSNetwork
{

//==============================================================================
// Protocol Types
//==============================================================================

/** OSC Protocol types matching NetworkTab UI */
enum class Protocol
{
    Disabled = 0,
    OSC = 1,
    Remote = 2,
    ADMOSC = 3,
    OSCQuery = 4,   // OSC Query protocol
    PSN = 5,        // PosiStageNet tracking protocol
    RTTrP = 6,      // RTTrP tracking protocol
    QLab = 7        // QLab cue writing protocol
};

/** Connection mode (transport layer) */
enum class ConnectionMode
{
    UDP = 0,
    TCP = 1
};

/** Connection status for UI display */
enum class ConnectionStatus
{
    Disconnected,
    Connecting,
    Connected,
    Error
};

/** Axis for position/offset operations */
enum class Axis
{
    X,
    Y,
    Z
};

/** Direction for REMOTE protocol delta commands */
enum class DeltaDirection
{
    Increment,
    Decrement
};

//==============================================================================
// Configuration Structures
//==============================================================================

/** Configuration for a single network target */
struct TargetConfig
{
    juce::String name;
    juce::String ipAddress = "127.0.0.1";
    int port = 9000;
    Protocol protocol = Protocol::Disabled;
    ConnectionMode mode = ConnectionMode::UDP;
    bool rxEnabled = false;
    bool txEnabled = false;
    int qlabPatchNumber = 1;     // QLab network patch number for created cues

    bool isValid() const
    {
        return protocol != Protocol::Disabled
            && !ipAddress.isEmpty()
            && port > 0 && port <= 65535;
    }

    bool isActive() const
    {
        if (protocol == Protocol::QLab)
            return true;  // QLab targets are always active (no Rx/Tx concept)
        return protocol != Protocol::Disabled && (rxEnabled || txEnabled);
    }
};

/** Global network configuration */
struct GlobalConfig
{
    int udpReceivePort = 8000;
    int tcpReceivePort = 8001;
    juce::String networkInterface;
    bool ipFilteringEnabled = false;
    juce::StringArray allowedIPs;
};

//==============================================================================
// Constants
//==============================================================================

/** Maximum number of network targets */
constexpr int MAX_TARGETS = 6;

/** Rate limiting: maximum messages per second */
constexpr int MAX_RATE_HZ = 50;

/** Rate limiting: minimum interval between messages in milliseconds */
constexpr int MIN_INTERVAL_MS = 20;

/** Default UDP receive port */
constexpr int DEFAULT_UDP_PORT = 8000;

/** Default TCP receive port */
constexpr int DEFAULT_TCP_PORT = 8001;

/** Default target transmit port */
constexpr int DEFAULT_TX_PORT = 9000;

/** Default QLab OSC command port */
constexpr int DEFAULT_QLAB_PORT = 53000;

/** Default QLab OSC reply port */
constexpr int DEFAULT_QLAB_REPLY_PORT = 53001;

//==============================================================================
// OSC Address Patterns
//==============================================================================

namespace OSCPaths
{
    // Standard WFS OSC paths
    constexpr const char* INPUT_PREFIX = "/wfs/input/";
    constexpr const char* OUTPUT_PREFIX = "/wfs/output/";
    constexpr const char* CONFIG_PREFIX = "/wfs/config/";

    // Config/Stage paths (global parameters, no channel ID)
    constexpr const char* CONFIG_STAGE_SHAPE = "/wfs/config/stage/shape";
    constexpr const char* CONFIG_STAGE_WIDTH = "/wfs/config/stage/width";
    constexpr const char* CONFIG_STAGE_DEPTH = "/wfs/config/stage/depth";
    constexpr const char* CONFIG_STAGE_HEIGHT = "/wfs/config/stage/height";
    constexpr const char* CONFIG_STAGE_DIAMETER = "/wfs/config/stage/diameter";
    constexpr const char* CONFIG_STAGE_DOME_ELEVATION = "/wfs/config/stage/domeElevation";
    constexpr const char* CONFIG_STAGE_ORIGIN_X = "/wfs/config/stage/originX";
    constexpr const char* CONFIG_STAGE_ORIGIN_Y = "/wfs/config/stage/originY";
    constexpr const char* CONFIG_STAGE_ORIGIN_Z = "/wfs/config/stage/originZ";

    // Config/Reverb Algorithm paths (global parameters, no channel ID)
    constexpr const char* CONFIG_REVERB_ALGO_TYPE      = "/wfs/config/reverb/algoType";
    constexpr const char* CONFIG_REVERB_RT60            = "/wfs/config/reverb/rt60";
    constexpr const char* CONFIG_REVERB_RT60_LOW_MULT   = "/wfs/config/reverb/rt60LowMult";
    constexpr const char* CONFIG_REVERB_RT60_HIGH_MULT  = "/wfs/config/reverb/rt60HighMult";
    constexpr const char* CONFIG_REVERB_CROSSOVER_LOW   = "/wfs/config/reverb/crossoverLow";
    constexpr const char* CONFIG_REVERB_CROSSOVER_HIGH  = "/wfs/config/reverb/crossoverHigh";
    constexpr const char* CONFIG_REVERB_DIFFUSION       = "/wfs/config/reverb/diffusion";
    constexpr const char* CONFIG_REVERB_SDN_SCALE       = "/wfs/config/reverb/sdnScale";
    constexpr const char* CONFIG_REVERB_FDN_SIZE        = "/wfs/config/reverb/fdnSize";
    constexpr const char* CONFIG_REVERB_IR_TRIM         = "/wfs/config/reverb/irTrim";
    constexpr const char* CONFIG_REVERB_IR_LENGTH       = "/wfs/config/reverb/irLength";
    constexpr const char* CONFIG_REVERB_PER_NODE_IR     = "/wfs/config/reverb/perNodeIR";
    constexpr const char* CONFIG_REVERB_WET_LEVEL       = "/wfs/config/reverb/wetLevel";

    // Config/Reverb Pre-Compressor paths (global parameters, no channel ID)
    constexpr const char* CONFIG_REVERB_PRE_COMP_BYPASS    = "/wfs/config/reverb/preCompBypass";
    constexpr const char* CONFIG_REVERB_PRE_COMP_THRESHOLD = "/wfs/config/reverb/preCompThreshold";
    constexpr const char* CONFIG_REVERB_PRE_COMP_RATIO     = "/wfs/config/reverb/preCompRatio";
    constexpr const char* CONFIG_REVERB_PRE_COMP_ATTACK    = "/wfs/config/reverb/preCompAttack";
    constexpr const char* CONFIG_REVERB_PRE_COMP_RELEASE   = "/wfs/config/reverb/preCompRelease";

    // Config/Reverb Post-Processing EQ paths (global parameters, no channel ID)
    constexpr const char* CONFIG_REVERB_POST_EQ_ENABLE = "/wfs/config/reverb/postEQenable";
    constexpr const char* CONFIG_REVERB_POST_EQ_SHAPE  = "/wfs/config/reverb/postEQshape";
    constexpr const char* CONFIG_REVERB_POST_EQ_FREQ   = "/wfs/config/reverb/postEQfreq";
    constexpr const char* CONFIG_REVERB_POST_EQ_GAIN   = "/wfs/config/reverb/postEQgain";
    constexpr const char* CONFIG_REVERB_POST_EQ_Q      = "/wfs/config/reverb/postEQq";
    constexpr const char* CONFIG_REVERB_POST_EQ_SLOPE  = "/wfs/config/reverb/postEQslope";

    // Config/Reverb Post-Expander paths (global parameters, no channel ID)
    constexpr const char* CONFIG_REVERB_POST_EXP_BYPASS    = "/wfs/config/reverb/postExpBypass";
    constexpr const char* CONFIG_REVERB_POST_EXP_THRESHOLD = "/wfs/config/reverb/postExpThreshold";
    constexpr const char* CONFIG_REVERB_POST_EXP_RATIO     = "/wfs/config/reverb/postExpRatio";
    constexpr const char* CONFIG_REVERB_POST_EXP_ATTACK    = "/wfs/config/reverb/postExpAttack";
    constexpr const char* CONFIG_REVERB_POST_EXP_RELEASE   = "/wfs/config/reverb/postExpRelease";

    // REMOTE protocol paths
    constexpr const char* REMOTE_INPUT_PREFIX = "/remoteInput/";
    constexpr const char* REMOTE_OUTPUT_PREFIX = "/remoteOutput/";

    // REMOTE specific commands
    constexpr const char* REMOTE_INPUT_NUMBER = "/remoteInput/inputNumber";
    constexpr const char* REMOTE_POSITION_X = "/remoteInput/positionX";
    constexpr const char* REMOTE_POSITION_Y = "/remoteInput/positionY";
    constexpr const char* REMOTE_POSITION_Z = "/remoteInput/positionZ";
}

//==============================================================================
// Logging Entry
//==============================================================================

/** Log entry for network messages */
struct LogEntry
{
    juce::Time timestamp;
    juce::String direction;      // "Rx" or "Tx"
    juce::String ipAddress;      // Sender IP (for Rx) or Target IP (for Tx)
    int port = 0;                // Port number
    int targetIndex = -1;        // -1 for incoming on global port
    juce::String address;        // OSC address pattern
    juce::String arguments;      // Formatted arguments
    Protocol protocol = Protocol::OSC;
    ConnectionMode transport = ConnectionMode::UDP;  // UDP or TCP
    bool isRejected = false;     // True if message was filtered/rejected
    juce::String rejectReason;   // Why message was rejected (if applicable)

    juce::String toString() const
    {
        return timestamp.formatted("%H:%M:%S.") + juce::String(timestamp.getMilliseconds()).paddedLeft('0', 3)
             + " [" + direction + "] "
             + address + " " + arguments;
    }

    /** Get protocol as display string */
    juce::String getProtocolString() const
    {
        switch (protocol)
        {
            case Protocol::Disabled: return "Disabled";
            case Protocol::OSC:      return "OSC";
            case Protocol::Remote:   return "Remote";
            case Protocol::ADMOSC:   return "ADM-OSC";
            case Protocol::OSCQuery: return "OSCQuery";
            case Protocol::PSN:      return "PSN";
            case Protocol::RTTrP:    return "RTTrP";
            case Protocol::QLab:     return "QLab";
            default:                 return "Unknown";
        }
    }

    /** Get transport as display string */
    juce::String getTransportString() const
    {
        return transport == ConnectionMode::TCP ? "TCP" : "UDP";
    }
};

} // namespace WFSNetwork
