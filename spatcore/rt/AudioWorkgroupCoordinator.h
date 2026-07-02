#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

namespace spatcore::rt {

//==============================================================================
/**
    Shares the current audio device's realtime workgroup with the DSP worker
    threads and lets each worker (re)join it when it changes (e.g. on a device
    or sample-rate switch).

    Why: per-thread realtime time-constraint policy makes the worker threads
    P-core eligible, but the kernel treats each as an independent deadline and
    tends to pack them onto a single P-cluster. Joining them all to the CoreAudio
    device's os_workgroup tells the kernel they are one workload sharing the audio
    I/O deadline, so it schedules them coherently across the performance cores
    (and independent of which app is frontmost).

    Realtime-safe: a worker hits a lock-free fast path (one atomic load) every
    block; the CriticalSection is only taken on the rare generation change.

    Cross-platform: on non-macOS platforms juce::AudioWorkgroup is an empty handle
    and join() is a no-op, so this compiles and runs everywhere with zero effect.

    Usage:
        // owner (message/audio-setup thread), whenever the device may have changed:
        coordinator.set (device->getWorkgroup());

        // each worker thread:
        void run() override {
            juce::WorkgroupToken token;   // created & destroyed on THIS thread
            uint32_t seenGen = 0;
            while (! threadShouldExit()) {
                coordinator->joinIfChanged (token, seenGen);
                // ... per-block work ...
            }
        }
*/
class AudioWorkgroupCoordinator
{
public:
    /** Publish the current device workgroup. Call from the audio-setup path
        (e.g. MainComponent::prepareToPlay) whenever it may have changed. */
    void set (juce::AudioWorkgroup newWorkgroup)
    {
        const juce::ScopedLock sl (lock);
        current = std::move (newWorkgroup);
        generation.fetch_add (1, std::memory_order_release);
    }

    /** Called by a worker thread, passing its OWN token and a per-thread
        "seen generation" counter. Joins (or rejoins) only when the workgroup
        has changed since the last call. The token must live on, and be
        destroyed on, the calling thread. */
    void joinIfChanged (juce::WorkgroupToken& token, uint32_t& seenGeneration)
    {
        const uint32_t g = generation.load (std::memory_order_acquire);
        if (g == seenGeneration)
            return;                         // fast path: one atomic load, no lock

        juce::AudioWorkgroup wg;
        {
            const juce::ScopedLock sl (lock);
            wg = current;
        }

        if (wg)
            wg.join (token);                // join / rejoin the new workgroup
        else
            token.reset();                  // device has no workgroup: leave any previous one

        seenGeneration = g;
    }

private:
    juce::CriticalSection lock;
    juce::AudioWorkgroup current;
    std::atomic<uint32_t> generation { 0 };
};

} // namespace spatcore::rt

// Extraction-compat aliases — app code migrates to qualified names later.
using spatcore::rt::AudioWorkgroupCoordinator;
