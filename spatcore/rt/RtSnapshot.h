#pragma once

#include <juce_core/juce_core.h>
#include <type_traits>

namespace spatcore::rt
{

/**
    Message-thread → realtime-thread parameter hand-off primitive.

    Generalizes the pattern shipped in BinauralCalculationEngine::RtParams
    (the 2026-07 RT-safety fix): the message thread builds a trivially
    copyable POD snapshot of everything the realtime side needs, publishes
    it under a brief SpinLock, and the realtime thread copies it out once
    per block. The realtime side never touches a ValueTree, never allocates,
    and never blocks on the message thread beyond the few-instruction copy.

    The contract (core-boundary-proposal-control.md §3.3):
      1. T is a trivially copyable POD — enforced below. No Strings, no
         vectors, no pointers into message-thread-owned storage.
      2. publish() is called from ONE non-realtime thread only (normally the
         message thread), after building the snapshot locally outside the lock.
      3. acquire() is called from realtime threads; it is allocation-free and
         lock-bounded by a POD copy.
      4. Values must be pre-cooked at publish time (dB → linear, string →
         index, positions → floats) so the realtime side does no conversion
         that could fault or allocate.

    Publish-before-enable ordering is the caller's job, exactly as in the
    binaural exemplar: publish the first snapshot before the consumer starts
    reading, so a worker never observes a default-constructed T unless T's
    defaults are themselves safe (make them safe anyway).
*/
template <typename T>
class RtSnapshot
{
    static_assert (std::is_trivially_copyable_v<T>,
                   "RtSnapshot<T>: T must be trivially copyable — POD-only hand-off");

public:
    RtSnapshot() = default;

    /** Message thread: publish a freshly built snapshot. Build `next` as a
        local outside the lock; this call is just the guarded copy. */
    void publish (const T& next) noexcept
    {
        const juce::SpinLock::ScopedLockType lock (spinLock);
        value = next;
    }

    /** Realtime thread: copy out the current snapshot (once per block).
        Allocation-free; the lock is held only for the POD copy. */
    T acquire() const noexcept
    {
        const juce::SpinLock::ScopedLockType lock (spinLock);
        return value;
    }

private:
    mutable juce::SpinLock spinLock;
    T value {};
};

} // namespace spatcore::rt
