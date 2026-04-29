#include "OSCIngestQueue.h"
#include "NetworkStringUtils.h"

#include <cstring>
#include <utility>

namespace WFSNetwork
{

OSCIngestQueue::OSCIngestQueue()
{
    // Drain at a fixed cadence on the MessageManager thread. This
    // guarantees the GUI gets idle slices between batches, which
    // callAsync-on-demand draining cannot (each call piles onto the MM
    // queue and starves paint/input under flood).
    startTimer (kDrainIntervalMs);
}

OSCIngestQueue::~OSCIngestQueue()
{
    stopTimer();
}

void OSCIngestQueue::timerCallback()
{
    drainBatch();
}

void OSCIngestQueue::setDrainIntervalMs (int ms)
{
    startTimer (juce::jmax (1, ms));
}

namespace
{
    // Round up to next 4-byte boundary (OSC alignment).
    inline int alignTo4 (int v) noexcept { return (v + 3) & ~3; }

    struct Prefix { const char* str; int len; bool perChannel; };

    // Coalesceable address prefixes. For per-channel prefixes we read
    // the first int32 arg as the channel id and key on
    // "<address>|<channel>"; for /wfs/config/ (no channel) we key on
    // the full address.
    constexpr Prefix kPrefixes[] = {
        { "/wfs/input/",   11, true  },
        { "/wfs/output/",  12, true  },
        { "/wfs/reverb/",  12, true  },
        { "/remoteInput/", 13, true  },
        { "/wfs/config/",  12, false }
    };
}

bool OSCIngestQueue::tryClassify (const char* data, int dataSize,
                                   juce::String& outKey)
{
    if (data == nullptr || dataSize <= 0 || data[0] != '/')
        return false;  // not a leading-/-message; bundles fall here too

    // Find end of address (first null within bounds).
    int addrEnd = 0;
    while (addrEnd < dataSize && data[addrEnd] != '\0')
        ++addrEnd;
    if (addrEnd == 0 || addrEnd >= dataSize)
        return false;

    // Match against the prefix set.
    const Prefix* matched = nullptr;
    for (const auto& p : kPrefixes)
    {
        if (addrEnd >= p.len && std::memcmp (data, p.str, (size_t) p.len) == 0)
        {
            matched = &p;
            break;
        }
    }
    if (matched == nullptr)
        return false;

    // OSCQuery short form like /wfs/input/7/positionX has a digit
    // immediately after the prefix. We don't try to canonicalise those
    // here — they go through the FIFO path. (Normalising would mean
    // re-emitting the standard form, which is more work than it saves.)
    if (matched->perChannel
        && matched->len < addrEnd
        && data[matched->len] >= '0' && data[matched->len] <= '9')
    {
        return false;
    }

    // For /wfs/config/* there is no channel id; key on the address.
    if (! matched->perChannel)
    {
        outKey = safeStringFromBytes (data, addrEnd);
        if (outKey.isEmpty())
            return false;  // address contained invalid UTF-8 — let FIFO handle it
        return true;
    }

    // Per-channel: read the first int32 arg.
    //   <address>\0... pad to 4 ... ",i...\0... pad to 4 ... <int32>
    const int typeTagStart = alignTo4 (addrEnd + 1);
    if (typeTagStart >= dataSize || data[typeTagStart] != ',')
        return false;

    // First arg type must be 'i' (int32) to extract the channel.
    if (typeTagStart + 1 >= dataSize || data[typeTagStart + 1] != 'i')
        return false;

    // Find end of type-tag string.
    int tagEnd = typeTagStart;
    while (tagEnd < dataSize && data[tagEnd] != '\0')
        ++tagEnd;
    if (tagEnd >= dataSize)
        return false;

    const int argStart = alignTo4 (tagEnd + 1);
    if (argStart + 4 > dataSize)
        return false;

    // Big-endian int32.
    const uint32_t bits = (static_cast<uint8_t> (data[argStart    ]) << 24)
                        | (static_cast<uint8_t> (data[argStart + 1]) << 16)
                        | (static_cast<uint8_t> (data[argStart + 2]) <<  8)
                        | (static_cast<uint8_t> (data[argStart + 3]));
    const int channelId = static_cast<int32_t> (bits);

    juce::String addr = safeStringFromBytes (data, addrEnd);
    if (addr.isEmpty())
        return false;
    outKey = addr + "|" + juce::String (channelId);
    return true;
}

void OSCIngestQueue::push (juce::MemoryBlock data,
                            juce::String senderIP,
                            int port,
                            ConnectionMode transport)
{
    juce::String coalesceKey;
    const bool coalesceable = tryClassify (
        static_cast<const char*> (data.getData()),
        static_cast<int> (data.getSize()),
        coalesceKey);

    {
        const juce::ScopedLock sl (lock);

        if (coalesceable)
        {
            if (coalesced.size() >= kCoalesceCap
                && coalesced.find (coalesceKey) == coalesced.end())
            {
                droppedTotal.fetch_add (1, std::memory_order_relaxed);
                ++droppedSinceLastReport;
                lastDropTransport = transport;
                return;
            }

            // Newest-wins. Replace any existing entry for this key.
            const auto inserted = coalesced.find (coalesceKey);
            if (inserted != coalesced.end())
                coalescedTotal.fetch_add (1, std::memory_order_relaxed);

            IngestItem item;
            item.data      = std::move (data);
            item.senderIP  = std::move (senderIP);
            item.port      = port;
            item.transport = transport;
            coalesced[std::move (coalesceKey)] = std::move (item);
        }
        else
        {
            if (fifo.size() >= kFifoCap)
            {
                droppedTotal.fetch_add (1, std::memory_order_relaxed);
                ++droppedSinceLastReport;
                lastDropTransport = transport;
                return;
            }

            IngestItem item;
            item.data      = std::move (data);
            item.senderIP  = std::move (senderIP);
            item.port      = port;
            item.transport = transport;
            fifo.push_back (std::move (item));
        }
    }

    // No scheduling — the Timer drains at a fixed cadence regardless
    // of arrival rate.
}

void OSCIngestQueue::drainBatch()
{
    // Move at most kMaxItemsPerTick items out of the queues into a
    // local batch under the lock, then dispatch outside the lock so
    // the receiver thread can keep pushing. Capping the per-tick work
    // is essential — without it, a flood drains for >16 ms and starves
    // the GUI.
    const int cap = maxItemsPerTick.load();
    std::vector<IngestItem> batch;
    batch.reserve (static_cast<size_t> (cap));

    uint64_t       dropsToReport = 0;
    ConnectionMode dropTransport = ConnectionMode::UDP;

    {
        const juce::ScopedLock sl (lock);

        // FIFO first (events / bundles are order-sensitive).
        while (static_cast<int> (batch.size()) < cap && ! fifo.empty())
        {
            batch.push_back (std::move (fifo.front()));
            fifo.pop_front();
        }

        // Then coalesced items (latest-value snapshots; order doesn't
        // matter — different keys are independent).
        while (static_cast<int> (batch.size()) < cap && ! coalesced.empty())
        {
            auto it = coalesced.begin();
            batch.push_back (std::move (it->second));
            coalesced.erase (it);
        }

        if (droppedSinceLastReport >= kDropLogEvery)
        {
            dropsToReport = droppedSinceLastReport;
            dropTransport = lastDropTransport;
            droppedSinceLastReport = 0;
        }
    }

    if (dispatch)
    {
        for (auto& item : batch)
        {
            dispatch (item.data, item.senderIP, item.port, item.transport);
            dispatchedTotal.fetch_add (1, std::memory_order_relaxed);
        }
    }

    if (dropsToReport > 0 && dropReport)
        dropReport (droppedTotal.load (std::memory_order_relaxed), dropTransport);
}

} // namespace WFSNetwork
