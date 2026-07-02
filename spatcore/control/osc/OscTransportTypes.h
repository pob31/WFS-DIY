#pragma once

#include <juce_core/juce_core.h>

// App-agnostic OSC transport/attribution types, extracted verbatim from
// Source/Network/OSCProtocolTypes.h (Phase 4a of the spatcore extraction).
// App-dialect types (Protocol, TargetConfig, OSCPaths, LogEntry) stay in the
// app header, which re-exports these under WFSNetwork for existing call sites.
namespace spatcore::control::osc
{

/** Origin tag for parameter changes — identifies which actor caused a write.
    Set via OriginTagScope (RAII guard, below) before issuing a write; the
    parameter system's change-notification dispatch reads it and propagates
    it to listeners (Network Log, AI undo stack, etc.). */
enum class OriginTag
{
    None = 0,        // Default — no origin attributed (e.g. internal initialization)
    UI,              // User-driven via GUI controls
    MCP,             // AI client via MCP server
    OSC,             // Arbitrary external OSC client (NOT the tablet remote — see Remote)
    Tracking,        // Position tracking integration loop
    Snapshot,        // Snapshot recall / Reload Configuration
    LFO,             // LFO modulation writes
    Move,            // Programmed movement
    Automation,      // DAW host automation via plugin layer
    Hardware,        // Hardware controllers
    Remote           // Remote tablet dialect
};

/** Thread-local current origin tag. Read by listeners (e.g. loggers) when
    constructing log entries / change records; written via OriginTagScope.
    `inline` lets it live in a header without an ODR violation across TUs. */
inline thread_local OriginTag g_currentOriginTag = OriginTag::None;

/** Read the current thread's origin tag. */
inline OriginTag getCurrentOriginTag() noexcept { return g_currentOriginTag; }

/** RAII guard that sets the thread-local origin tag for its lifetime, then
    restores the previous value on destruction. Nesting is supported — inner
    scopes shadow outer scopes and the outer value resumes when the inner
    scope ends. Use at every external write entry point (OSC inbound handler,
    snapshot recall, MCP dispatcher, etc.). */
struct OriginTagScope
{
    OriginTag previous;
    explicit OriginTagScope (OriginTag tag) noexcept
        : previous (g_currentOriginTag) { g_currentOriginTag = tag; }
    ~OriginTagScope() noexcept { g_currentOriginTag = previous; }

    OriginTagScope (const OriginTagScope&) = delete;
    OriginTagScope& operator= (const OriginTagScope&) = delete;
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

/** Direction for remote-dialect delta commands */
enum class DeltaDirection
{
    Increment,
    Decrement
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

/** Maximum number of network targets (sizes the rate limiter's per-target state) */
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

} // namespace spatcore::control::osc
