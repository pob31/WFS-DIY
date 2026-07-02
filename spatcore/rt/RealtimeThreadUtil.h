#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#if JUCE_MAC
 #include <mach/mach.h>
 #include <mach/thread_policy.h>
 #include <pthread.h>
#endif

//==============================================================================
/**
    Upgrades the *calling* thread to a realtime time-constraint scheduling policy
    on macOS, so the Apple Silicon scheduler places it on a performance (P) core
    instead of an efficiency (E) core.

    This mirrors JUCE's own Thread::startRealtimeThread implementation
    (modules/juce_core/native/juce_SharedCode_posix.h, tryToUpgradeCurrentThreadToRealtime),
    and is intended for raw std::thread workers that cannot use the juce::Thread API
    (e.g. the AudioParallelFor fork-join pool). juce::Thread-based workers should use
    Thread::startRealtimeThread(RealtimeOptions) instead.

    @param periodMs       Expected time between wake-ups (one audio block), in milliseconds.
    @param computationMs  Expected processing time per wake-up, in milliseconds.
                          Clamped to <= periodMs.

    On non-macOS platforms this is a no-op (the std::thread pool keeps its current
    behaviour; Windows realtime scheduling is handled process-wide in Main.cpp).
*/
inline bool setCurrentThreadRealtimeAudio (double periodMs, double computationMs)
{
   #if JUCE_MAC
    if (periodMs <= 0.0)
        return false;

    if (computationMs <= 0.0 || computationMs > periodMs)
        computationMs = periodMs;

    thread_time_constraint_policy_data_t policy;
    policy.period      = (uint32_t) juce::Time::secondsToHighResolutionTicks (periodMs      / 1000.0);
    policy.computation = (uint32_t) juce::Time::secondsToHighResolutionTicks (computationMs / 1000.0);
    policy.constraint  = (uint32_t) juce::Time::secondsToHighResolutionTicks (periodMs      / 1000.0);
    policy.preemptible = true;

    const auto result = thread_policy_set (pthread_mach_thread_np (pthread_self()),
                                           THREAD_TIME_CONSTRAINT_POLICY,
                                           (thread_policy_t) &policy,
                                           THREAD_TIME_CONSTRAINT_POLICY_COUNT);

    // Mirror JUCE: if the requested computation budget is too high, retry smaller.
    if (result == KERN_INVALID_ARGUMENT && computationMs > 50.0)
        return setCurrentThreadRealtimeAudio (periodMs, 50.0);

    return result == KERN_SUCCESS;
   #else
    juce::ignoreUnused (periodMs, computationMs);
    return false;
   #endif
}
