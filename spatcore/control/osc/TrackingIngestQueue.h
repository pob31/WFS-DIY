#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <unordered_map>

namespace spatcore::control::osc
{

/**
 * TrackingUpdate
 *
 * A fully-decoded, protocol-agnostic tracking sample. Producers (the PSN/RTTrP/
 * MQTT receiver threads) fill one of these — position and/or orientation, with the
 * offset/scale/flip transform already applied — and hand it to a TrackingIngestQueue.
 * No JUCE ValueTree or String work happens on the producer side.
 */
struct TrackingUpdate
{
    // Coalesce key: the tracking ID (PSN/RTTrP) or the resolved input slot (MQTT).
    int   key = 0;

    // Position (offset -> scale -> flip already applied on the producer thread).
    float x = 0.0f, y = 0.0f, z = 0.0f;
    bool  hasPos = false;

    // Orientation (yaw, degrees). MQTT never sets this.
    float rotation = 0.0f;
    bool  hasOri   = false;

    // Filter quality hint (MQTT carries it; PSN/RTTrP leave it at 1.0).
    float quality = 1.0f;
};

/**
 * TrackingIngestQueue
 *
 * Sits between a self-threaded tracking receiver (PSN/RTTrP/MQTT) and the
 * MessageManager thread that owns the ValueTree. It exists to fix the RT-safety
 * violation where the receiver threads wrote the ValueTree — and thereby fired
 * WFSCalculationEngine::valueTreePropertyChanged and ~40 other listeners — off the
 * message thread (control-plane-map §3.6, open-questions-control Q7 Violation B).
 *
 * This mirrors OSCIngestQueue, the in-repo template for the same problem on the OSC
 * path, but carries a fully-decoded TrackingUpdate instead of raw wire bytes:
 *
 *  - push(...) is called on the receiver's network thread. It coalesces by `key`
 *    (newest-wins), MERGING position and orientation so a pos-only and an ori-only
 *    packet for the same tag both survive one drain tick. Returns immediately.
 *  - A private juce::Timer drains on the MessageManager thread at a fixed 60 Hz
 *    cadence and invokes the injected apply callback per item. Coalescing means a
 *    100 Hz position stream collapses to one ValueTree write per key per tick, so
 *    the GUI never starves (the failure mode a per-packet callAsync would cause).
 *
 * The apply callback — and therefore all ValueTree access, listener firing, and the
 * stateful TrackingPositionFilter — runs only on the message thread.
 */
class TrackingIngestQueue : private juce::Timer
{
public:
    /** Applied per drained item, on the MessageManager thread only. */
    using ApplyFn = std::function<void (const TrackingUpdate&)>;

    TrackingIngestQueue();
    ~TrackingIngestQueue() override;

    void setApply (ApplyFn fn) { apply = std::move (fn); }

    /** Network-thread entry point. Coalesce-merges into the pending set under the
     *  lock and returns; the Timer does the draining. */
    void push (const TrackingUpdate& update);

    /** Counters for diagnostics. */
    uint64_t getDroppedTotal()    const noexcept { return droppedTotal.load(); }
    uint64_t getCoalescedTotal()  const noexcept { return coalescedTotal.load(); }
    uint64_t getDispatchedTotal() const noexcept { return dispatchedTotal.load(); }

    /** Tunables (at-runtime). */
    void setMaxItemsPerTick (int n) { maxItemsPerTick.store (juce::jmax (1, n)); }
    void setDrainIntervalMs (int ms);
    int  getMaxItemsPerTick() const { return maxItemsPerTick.load(); }

private:
    void timerCallback() override { drainBatch(); }
    void drainBatch();

    juce::CriticalSection                     lock;
    std::unordered_map<int, TrackingUpdate>   coalesced;   // newest-wins per key, pos/ori merged

    std::atomic<uint64_t> droppedTotal    { 0 };
    std::atomic<uint64_t> coalescedTotal  { 0 };
    std::atomic<uint64_t> dispatchedTotal { 0 };
    std::atomic<int>      maxItemsPerTick { 32 };

    ApplyFn apply;

    // Drain cadence: 60 Hz gives the GUI ~16 ms idle slices between batches — the
    // same trade OSCIngestQueue makes; well below perceptible tracking latency.
    static constexpr int    kDrainIntervalMs = 16;

    // Distinct-key cap: a hard ceiling on pending trackers. Far above any real
    // tracker count; guards against an adversarial flood of unique ids.
    static constexpr size_t kCoalesceCap     = 1024;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackingIngestQueue)
};

} // namespace spatcore::control::osc
