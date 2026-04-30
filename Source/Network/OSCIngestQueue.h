#pragma once

#include <JuceHeader.h>
#include "OSCProtocolTypes.h"

#include <atomic>
#include <deque>
#include <functional>
#include <unordered_map>

namespace WFSNetwork
{

/**
 * OSCIngestQueue
 *
 * Sits between the OSC receiver threads (UDP and TCP) and the
 * MessageManager dispatch path. Two responsibilities:
 *
 *  1. Coalesce hot per-(address, channel) parameter updates so that a
 *     flood of `/wfs/input/positionX 7 ...` messages collapses to a
 *     single ValueTree write per drain tick instead of a separate
 *     callAsync per UDP datagram.
 *
 *  2. Bound the queue depth for non-coalesceable messages (bundles,
 *     cluster gestures, ADM, handshake) so that an adversarial or
 *     misbehaving client cannot starve the GUI thread.
 *
 * Both protect against the failure mode where each UDP packet creates
 * a `juce::MessageManager::callAsync` lambda holding a MemoryBlock +
 * String, the queue grows unboundedly under flood, and the GUI freezes.
 *
 * Thread model:
 *  - `push(...)` is called on receiver threads (UDP socket thread,
 *    each TCP client thread).
 *  - The first `push` after a drain schedules exactly one callAsync.
 *  - The drain runs on the MessageManager thread, swaps the maps under
 *    lock, then iterates and calls the dispatch callback per item.
 */
class OSCIngestQueue : private juce::Timer
{
public:
    using DispatchFn = std::function<void (const juce::MemoryBlock& data,
                                            const juce::String& senderIP,
                                            int port,
                                            ConnectionMode transport)>;

    /** Logging helper for the periodic "queue full" rejected entry.
     *  Called on the MessageManager thread. */
    using DropReportFn = std::function<void (uint64_t totalDropped,
                                              ConnectionMode transport)>;

    OSCIngestQueue();
    ~OSCIngestQueue();

    void setDispatch (DispatchFn fn)         { dispatch  = std::move (fn); }
    void setDropReport (DropReportFn fn)     { dropReport = std::move (fn); }

    /** Receiver-thread entry point. Takes the bytes from the wire and
     *  classifies them: per-channel parameter updates go into the
     *  coalesce map (newest-wins per key), everything else goes into
     *  the bounded FIFO. Either path is dropped if the corresponding
     *  cap is exceeded. */
    void push (juce::MemoryBlock data,
               juce::String senderIP,
               int port,
               ConnectionMode transport);

    /** Counters for diagnostics. */
    uint64_t getDroppedTotal()      const noexcept { return droppedTotal.load(); }
    uint64_t getCoalescedTotal()    const noexcept { return coalescedTotal.load(); }
    uint64_t getDispatchedTotal()   const noexcept { return dispatchedTotal.load(); }

    /** Tunables (at-runtime). The defaults are conservative for debug
     *  builds; release builds can typically handle higher caps. */
    void setMaxItemsPerTick (int n)    { maxItemsPerTick.store (juce::jmax (1, n)); }
    void setDrainIntervalMs (int ms);
    int  getMaxItemsPerTick() const    { return maxItemsPerTick.load(); }

private:
    struct IngestItem
    {
        juce::MemoryBlock data;
        juce::String      senderIP;
        int               port = 0;
        ConnectionMode    transport = ConnectionMode::UDP;
    };

    /** Cheap address + first-int32 extraction directly off the
     *  datagram bytes. Returns true if the datagram should be
     *  coalesced; outKey is set to the coalesce key. Returns false
     *  for unparseable, bundles, or non-coalesceable addresses, in
     *  which case the caller routes to the FIFO. */
    static bool tryClassify (const char* data, int dataSize,
                              juce::String& outKey);

    void timerCallback() override;
    void drainBatch();

    juce::CriticalSection                            lock;
    std::unordered_map<juce::String, IngestItem>     coalesced;
    std::deque<IngestItem>                           fifo;

    std::atomic<uint64_t> droppedTotal     { 0 };
    std::atomic<uint64_t> coalescedTotal   { 0 };
    std::atomic<uint64_t> dispatchedTotal  { 0 };
    uint64_t              droppedSinceLastReport = 0;
    ConnectionMode        lastDropTransport      = ConnectionMode::UDP;

    DispatchFn   dispatch;
    DropReportFn dropReport;
    std::atomic<int> maxItemsPerTick { 32 };

    // Caps tuned for WFS-DIY's parameter cardinality:
    //  - ~50 user-visible per-channel parameters x 64 channels = 3200
    //    distinct keys. 4096 leaves headroom.
    //  - 256 FIFO slots cover bundle/handshake/cluster traffic for
    //    ~1 s at the rates we've observed in normal use.
    static constexpr size_t   kCoalesceCap    = 4096;
    static constexpr size_t   kFifoCap        = 256;
    static constexpr uint64_t kDropLogEvery   = 1000;

    // Drain cadence: 60 Hz gives the GUI thread ~16 ms idle slices
    // between batches. At realistic OSC traffic rates this is well
    // below human-perceptible parameter latency.
    static constexpr int kDrainIntervalMs = 16;

    // Default hard cap on items processed per Timer tick. Initialised
    // into the `maxItemsPerTick` atomic above. Tunable at runtime via
    // `setMaxItemsPerTick`. Default of 8 (× 60 Hz ≈ 480 dispatches/s)
    // is conservative for debug builds on modest hardware; release
    // builds can typically run higher.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OSCIngestQueue)
};

} // namespace WFSNetwork
