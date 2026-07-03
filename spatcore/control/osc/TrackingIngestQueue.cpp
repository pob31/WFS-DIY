#include "TrackingIngestQueue.h"

#include <utility>
#include <vector>

namespace spatcore::control::osc
{

TrackingIngestQueue::TrackingIngestQueue()
{
    // Drain at a fixed cadence on the MessageManager thread. This guarantees the
    // GUI gets idle slices between batches, which callAsync-on-demand draining
    // cannot (each call piles onto the MM queue and starves paint/input under
    // flood). See OSCIngestQueue for the same rationale.
    startTimer (kDrainIntervalMs);
}

TrackingIngestQueue::~TrackingIngestQueue()
{
    stopTimer();
}

void TrackingIngestQueue::setDrainIntervalMs (int ms)
{
    startTimer (juce::jmax (1, ms));
}

void TrackingIngestQueue::push (const TrackingUpdate& update)
{
    const juce::ScopedLock sl (lock);

    auto it = coalesced.find (update.key);
    if (it == coalesced.end())
    {
        if (coalesced.size() >= kCoalesceCap)
        {
            droppedTotal.fetch_add (1, std::memory_order_relaxed);
            return;
        }
        coalesced.emplace (update.key, update);
        return;
    }

    // Merge into the pending entry: newest-wins per half, so a position-only and
    // an orientation-only packet for the same key both survive to the next drain.
    TrackingUpdate& existing = it->second;
    if (update.hasPos)
    {
        existing.x       = update.x;
        existing.y       = update.y;
        existing.z       = update.z;
        existing.quality = update.quality;
        existing.hasPos  = true;
    }
    if (update.hasOri)
    {
        existing.rotation = update.rotation;
        existing.hasOri   = true;
    }
    coalescedTotal.fetch_add (1, std::memory_order_relaxed);
}

void TrackingIngestQueue::drainBatch()
{
    JUCE_ASSERT_MESSAGE_THREAD  // Timer callbacks run on the MessageManager thread

    // Move at most maxItemsPerTick items out under the lock, then apply outside the
    // lock so the receiver thread can keep pushing. Capping per-tick work prevents a
    // flood from draining for >16 ms and starving the GUI.
    const int cap = maxItemsPerTick.load();
    std::vector<TrackingUpdate> batch;
    batch.reserve (static_cast<size_t> (cap));

    {
        const juce::ScopedLock sl (lock);

        // Latest-value snapshots; order across keys doesn't matter.
        while (static_cast<int> (batch.size()) < cap && ! coalesced.empty())
        {
            auto it = coalesced.begin();
            batch.push_back (std::move (it->second));
            coalesced.erase (it);
        }
    }

    if (apply)
    {
        for (const auto& item : batch)
        {
            apply (item);
            dispatchedTotal.fetch_add (1, std::memory_order_relaxed);
        }
    }
}

} // namespace spatcore::control::osc
