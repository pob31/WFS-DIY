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
    ADMOSC = 3
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

    bool isValid() const
    {
        return protocol != Protocol::Disabled
            && !ipAddress.isEmpty()
            && port > 0 && port <= 65535;
    }

    bool isActive() const
    {
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

//==============================================================================
// OSC Address Patterns
//==============================================================================

namespace OSCPaths
{
    // Standard WFS OSC paths
    constexpr const char* INPUT_PREFIX = "/wfs/input/";
    constexpr const char* OUTPUT_PREFIX = "/wfs/output/";
    constexpr const char* CONFIG_PREFIX = "/wfs/config/";

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
    juce::String direction;  // "Rx" or "Tx"
    int targetIndex = -1;    // -1 for incoming on global port
    juce::String address;
    juce::String arguments;
    Protocol protocol = Protocol::OSC;

    juce::String toString() const
    {
        return timestamp.formatted("%H:%M:%S.") + juce::String(timestamp.getMilliseconds()).paddedLeft('0', 3)
             + " [" + direction + "] "
             + address + " " + arguments;
    }
};

} // namespace WFSNetwork
