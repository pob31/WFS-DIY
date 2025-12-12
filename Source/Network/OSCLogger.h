#pragma once

#include <JuceHeader.h>
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

    /** Log an incoming message */
    void logReceived(const juce::OSCMessage& message, Protocol protocol = Protocol::OSC);

    /** Log an outgoing message */
    void logSent(int targetIndex, const juce::OSCMessage& message, Protocol protocol = Protocol::OSC);

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
        int targetIndex = -1;  // -1 for all targets
        Protocol protocol = Protocol::Disabled;  // Disabled = show all protocols
        juce::String addressFilter;  // Empty = no filter
    };

    /** Get filtered entries */
    std::vector<LogEntry> getFilteredEntries(const Filter& filter) const;

private:
    //==========================================================================
    // Private Members
    //==========================================================================

    std::vector<LogEntry> entries;
    int maxEntries;
    std::atomic<bool> isEnabled { true };
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
