#pragma once

#include <JuceHeader.h>
#include <set>
#include "OSCProtocolTypes.h"

namespace WFSNetwork
{

/**
 * OSCLogger - Collects and stores OSC message logs for the Log Window UI.
 *
 * Uses a ring buffer to store recent log entries. Thread-safe for concurrent
 * logging from multiple sources and reading from the UI thread.
 */
class OSCLogger
{
public:
    //==========================================================================
    // Types
    //==========================================================================

    /** Callback when new entries are added */
    using LogCallback = std::function<void()>;

    //==========================================================================
    // Construction/Destruction
    //==========================================================================

    explicit OSCLogger(int maxEntries = 1000);
    ~OSCLogger() = default;

    //==========================================================================
    // Configuration
    //==========================================================================

    /** Enable or disable logging */
    void setEnabled(bool enabled) { isEnabled = enabled; }

    /** Check if logging is enabled */
    bool getEnabled() const { return isEnabled; }

    /** Set maximum number of entries to keep */
    void setMaxEntries(int max);

    /** Get maximum entries setting */
    int getMaxEntries() const { return maxEntries; }

    //==========================================================================
    // Logging
    //==========================================================================

    /** Log an incoming message (basic - for backward compatibility) */
    void logReceived(const juce::OSCMessage& message, Protocol protocol = Protocol::OSC);

    /** Log an incoming message with full network details */
    void logReceivedWithDetails(const juce::OSCMessage& message,
                                Protocol protocol,
                                const juce::String& senderIP,
                                int port,
                                ConnectionMode transport);

    /** Log an outgoing message (basic - for backward compatibility) */
    void logSent(int targetIndex, const juce::OSCMessage& message, Protocol protocol = Protocol::OSC);

    /** Log an outgoing message with full network details */
    void logSentWithDetails(int targetIndex,
                            const juce::OSCMessage& message,
                            Protocol protocol,
                            const juce::String& targetIP,
                            int port,
                            ConnectionMode transport);

    /** Log a rejected/filtered message */
    void logRejected(const juce::String& address,
                     const juce::String& senderIP,
                     int port,
                     ConnectionMode transport,
                     const juce::String& reason);

    /** Log a custom entry */
    void logEntry(const LogEntry& entry);

    /** Log a text message (for errors, status, etc.) */
    void logText(const juce::String& text);

    //==========================================================================
    // Reading
    //==========================================================================

    /** Get all current entries (thread-safe copy) */
    std::vector<LogEntry> getEntries() const;

    /** Get entries since a specific index */
    std::vector<LogEntry> getEntriesSince(size_t fromIndex) const;

    /** Get the current entry count */
    size_t getEntryCount() const;

    /** Get the total number of entries ever logged (for detecting new entries) */
    int64_t getTotalEntryCount() const { return totalEntryCount; }

    /** Clear all entries */
    void clear();

    //==========================================================================
    // Callbacks
    //==========================================================================

    /** Set callback for when new entries are added */
    void setLogCallback(LogCallback callback) { onNewEntry = std::move(callback); }

    //==========================================================================
    // Filtering
    //==========================================================================

    /** Filter options for getEntries */
    struct Filter
    {
        bool showRx = true;
        bool showTx = true;
        bool showUDP = true;
        bool showTCP = true;
        bool showRejected = false;     // When true, only show rejected messages
        int targetIndex = -1;          // -1 for all targets
        Protocol protocol = Protocol::Disabled;  // Disabled = show all protocols
        juce::String ipFilter;         // Empty = no filter, otherwise filter by IP
        juce::String addressFilter;    // Empty = no filter
        std::set<Protocol> enabledProtocols;  // Empty = show all, otherwise filter
        std::set<juce::String> enabledIPs;    // Empty = show all, otherwise filter
        bool hideHeartbeat = false;           // Hide remote heartbeat/ack messages
    };

    /** Get filtered entries */
    std::vector<LogEntry> getFilteredEntries(const Filter& filter) const;

    /** Get unique IP addresses seen in the log */
    std::set<juce::String> getUniqueIPs() const;

    /** Get unique protocols seen in the log */
    std::set<Protocol> getUniqueProtocols() const;

private:
    //==========================================================================
    // Private Members
    //==========================================================================

    std::vector<LogEntry> entries;
    int maxEntries;
    std::atomic<bool> isEnabled { false };
    std::atomic<int64_t> totalEntryCount { 0 };
    mutable juce::CriticalSection entriesLock;

    LogCallback onNewEntry;

    //==========================================================================
    // Private Methods
    //==========================================================================

    void addEntry(LogEntry entry);
    juce::String formatOSCArguments(const juce::OSCMessage& message) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSCLogger)
};

} // namespace WFSNetwork
