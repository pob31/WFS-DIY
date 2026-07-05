#pragma once

#include "RtThreadPriority.h"

namespace spatcore::rt {

//==============================================================================
/**
    Upgrades the *calling* thread to a realtime audio scheduling class.

    This is now a thin forwarder to RtThreadPriority.h's
    setCurrentThreadAudioPriority(), the single JUCE-free implementation of
    "elevate the current thread for audio work". It exists so the app's
    juce::Thread-free std::thread pools (AudioParallelFor's ReverbEngine CPU
    pool) keep their historical call site.

    Behaviour by platform now lives in RtThreadPriority.h:
      - Windows: MMCSS "Pro Audio" (dyn-loaded avrt) + AVRT_PRIORITY_HIGH,
                 fallback THREAD_PRIORITY_HIGHEST. (Previously a no-op here —
                 this is the fix that finally elevates the CPU reverb pool
                 workers on Windows.)
      - macOS:   mach time-constraint policy (P-core placement), JUCE-free.
      - Linux:   SCHED_FIFO best-effort.

    @param periodMs       Expected time between wake-ups (one audio block), ms.
    @param computationMs  Expected processing time per wake-up, ms (<= periodMs).

    Timing-only; never affects computed values.
*/
inline bool setCurrentThreadRealtimeAudio (double periodMs, double computationMs)
{
    return setCurrentThreadAudioPriority (periodMs, computationMs);
}

} // namespace spatcore::rt

// Extraction-compat aliases — app code migrates to qualified names later.
using spatcore::rt::setCurrentThreadRealtimeAudio;
