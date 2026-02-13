#include "OSCLogger.h"

namespace WFSNetwork
{

//==============================================================================
// Construction
//==============================================================================

OSCLogger::OSCLogger(int max)
    : maxEntries(max)
{
    entries.reserve(static_cast<size_t>(maxEntries));
}

//==============================================================================
// Configuration
//==============================================================================

void OSCLogger::setMaxEntries(int max)
{
    const juce::ScopedLock sl(entriesLock);

    maxEntries = juce::jmax(100, max);

    // Trim if needed
    while (entries.size() > static_cast<size_t>(maxEntries))
    {
        entries.erase(entries.begin());
    }
}

//==============================================================================
// Logging
//==============================================================================

void OSCLogger::logReceived(const juce::OSCMessage& message, Protocol protocol)
{
    if (!isEnabled)
        return;

    LogEntry entry;
    entry.timestamp = juce::Time::getCurrentTime();
    entry.direction = "Rx";
    entry.targetIndex = -1;  // Incoming messages don't have a target
    entry.address = message.getAddressPattern().toString();
    entry.arguments = formatOSCArguments(message);
    entry.protocol = protocol;

    addEntry(std::move(entry));
}

void OSCLogger::logReceivedWithDetails(const juce::OSCMessage& message,
                                       Protocol protocol,
                                       const juce::String& senderIP,
                                       int port,
                                       ConnectionMode transport)
{
    if (!isEnabled)
        return;

    LogEntry entry;
    entry.timestamp = juce::Time::getCurrentTime();
    entry.direction = "Rx";
    entry.ipAddress = senderIP;
    entry.port = port;
    entry.targetIndex = -1;
    entry.address = message.getAddressPattern().toString();
    entry.arguments = formatOSCArguments(message);
    entry.protocol = protocol;
    entry.transport = transport;

    addEntry(std::move(entry));
}

void OSCLogger::logSent(int targetIndex, const juce::OSCMessage& message, Protocol protocol)
{
    if (!isEnabled)
        return;

    LogEntry entry;
    entry.timestamp = juce::Time::getCurrentTime();
    entry.direction = "Tx";
    entry.targetIndex = targetIndex;
    entry.address = message.getAddressPattern().toString();
    entry.arguments = formatOSCArguments(message);
    entry.protocol = protocol;

    addEntry(std::move(entry));
}

void OSCLogger::logSentWithDetails(int targetIndex,
                                   const juce::OSCMessage& message,
                                   Protocol protocol,
                                   const juce::String& targetIP,
                                   int port,
                                   ConnectionMode transport)
{
    if (!isEnabled)
        return;

    LogEntry entry;
    entry.timestamp = juce::Time::getCurrentTime();
    entry.direction = "Tx";
    entry.ipAddress = targetIP;
    entry.port = port;
    entry.targetIndex = targetIndex;
    entry.address = message.getAddressPattern().toString();
    entry.arguments = formatOSCArguments(message);
    entry.protocol = protocol;
    entry.transport = transport;

    addEntry(std::move(entry));
}

void OSCLogger::logRejected(const juce::String& address,
                            const juce::String& senderIP,
                            int port,
                            ConnectionMode transport,
                            const juce::String& reason)
{
    if (!isEnabled)
        return;

    LogEntry entry;
    entry.timestamp = juce::Time::getCurrentTime();
    entry.direction = "Rx";
    entry.ipAddress = senderIP;
    entry.port = port;
    entry.targetIndex = -1;
    entry.address = address;
    entry.arguments = "";
    entry.protocol = Protocol::Disabled;
    entry.transport = transport;
    entry.isRejected = true;
    entry.rejectReason = reason;

    addEntry(std::move(entry));
}

void OSCLogger::logEntry(const LogEntry& entry)
{
    if (!isEnabled)
        return;

    addEntry(entry);
}

void OSCLogger::logText(const juce::String& text)
{
    if (!isEnabled)
        return;

    LogEntry entry;
    entry.timestamp = juce::Time::getCurrentTime();
    entry.direction = "--";
    entry.targetIndex = -1;
    entry.address = text;
    entry.arguments = "";
    entry.protocol = Protocol::Disabled;

    addEntry(std::move(entry));
}

//==============================================================================
// Reading
//==============================================================================

std::vector<LogEntry> OSCLogger::getEntries() const
{
    const juce::ScopedLock sl(entriesLock);
    return entries;
}

std::vector<LogEntry> OSCLogger::getEntriesSince(size_t fromIndex) const
{
    const juce::ScopedLock sl(entriesLock);

    if (fromIndex >= entries.size())
        return {};

    return std::vector<LogEntry>(entries.begin() + static_cast<std::ptrdiff_t>(fromIndex), entries.end());
}

size_t OSCLogger::getEntryCount() const
{
    const juce::ScopedLock sl(entriesLock);
    return entries.size();
}

void OSCLogger::clear()
{
    const juce::ScopedLock sl(entriesLock);
    entries.clear();
    // Don't reset totalEntryCount - it's used for change detection
}

//==============================================================================
// Filtering
//==============================================================================

std::vector<LogEntry> OSCLogger::getFilteredEntries(const Filter& filter) const
{
    const juce::ScopedLock sl(entriesLock);

    std::vector<LogEntry> result;
    result.reserve(entries.size());

    for (const auto& entry : entries)
    {
        // Rejected mode filter - only show rejected when in rejected mode
        if (filter.showRejected)
        {
            if (!entry.isRejected)
                continue;
        }
        else
        {
            // In normal mode, don't show rejected messages
            if (entry.isRejected)
                continue;
        }

        // Direction filter
        if (entry.direction == "Rx" && !filter.showRx)
            continue;
        if (entry.direction == "Tx" && !filter.showTx)
            continue;

        // Transport filter (UDP/TCP)
        if (entry.transport == ConnectionMode::UDP && !filter.showUDP)
            continue;
        if (entry.transport == ConnectionMode::TCP && !filter.showTCP)
            continue;

        // Target filter
        if (filter.targetIndex >= 0 && entry.targetIndex != filter.targetIndex)
            continue;

        // Protocol filter (single protocol)
        if (filter.protocol != Protocol::Disabled && entry.protocol != filter.protocol)
            continue;

        // Protocol set filter (multiple protocols)
        if (!filter.enabledProtocols.empty()
            && filter.enabledProtocols.find(entry.protocol) == filter.enabledProtocols.end())
            continue;

        // IP filter (single IP)
        if (filter.ipFilter.isNotEmpty() && entry.ipAddress != filter.ipFilter)
            continue;

        // IP set filter (multiple IPs)
        if (!filter.enabledIPs.empty()
            && filter.enabledIPs.find(entry.ipAddress) == filter.enabledIPs.end())
            continue;

        // Address filter
        if (filter.addressFilter.isNotEmpty()
            && !entry.address.containsIgnoreCase(filter.addressFilter))
            continue;

        // Heartbeat/ACK filter
        if (filter.hideHeartbeat
            && (entry.address == "/remote/ping"
                || entry.address == "/remote/pong"
                || entry.address == "/remote/heartbeat"
                || entry.address == "/remote/heartbeatAck"))
            continue;

        result.push_back(entry);
    }

    return result;
}

std::set<juce::String> OSCLogger::getUniqueIPs() const
{
    const juce::ScopedLock sl(entriesLock);

    std::set<juce::String> ips;
    for (const auto& entry : entries)
    {
        if (entry.ipAddress.isNotEmpty())
            ips.insert(entry.ipAddress);
    }
    return ips;
}

std::set<Protocol> OSCLogger::getUniqueProtocols() const
{
    const juce::ScopedLock sl(entriesLock);

    std::set<Protocol> protocols;
    for (const auto& entry : entries)
    {
        if (entry.protocol != Protocol::Disabled)
            protocols.insert(entry.protocol);
    }
    return protocols;
}

//==============================================================================
// Private Methods
//==============================================================================

void OSCLogger::addEntry(LogEntry entry)
{
    {
        const juce::ScopedLock sl(entriesLock);

        // Remove oldest entry if at capacity
        if (entries.size() >= static_cast<size_t>(maxEntries))
        {
            entries.erase(entries.begin());
        }

        entries.push_back(std::move(entry));
        ++totalEntryCount;
    }

    // Notify callback (outside of lock)
    if (onNewEntry)
        onNewEntry();
}

juce::String OSCLogger::formatOSCArguments(const juce::OSCMessage& message) const
{
    juce::StringArray args;

    for (const auto& arg : message)
    {
        if (arg.isFloat32())
            args.add(juce::String(arg.getFloat32(), 3));
        else if (arg.isInt32())
            args.add(juce::String(arg.getInt32()));
        else if (arg.isString())
            args.add("\"" + arg.getString() + "\"");
        else if (arg.isBlob())
            args.add("[blob:" + juce::String(static_cast<int>(arg.getBlob().getSize())) + " bytes]");
        else if (arg.isColour())
            args.add("[colour]");
        else
            args.add("[?]");
    }

    return args.joinIntoString(" ");
}

} // namespace WFSNetwork
