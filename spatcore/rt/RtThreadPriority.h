#pragma once

/*
    RtThreadPriority.h — the single JUCE-free implementation of "elevate the
    CURRENT thread for realtime audio work", using raw OS calls.

    One entry point, three platform strategies:

      - Windows: MMCSS "Pro Audio" via a RUNTIME-loaded avrt.dll
        (LoadLibraryW + GetProcAddress — deliberately NO avrt import-lib on any
        link line, so the plugin DLLs and the app never gain a hard avrt
        dependency). AvSetMmThreadCharacteristicsW(L"Pro Audio") registers the
        thread with the class the ASIO/JUCE device threads use (~prio 23-26,
        above the DWM's compositor bursts) without stacking another
        TIME_CRITICAL(15) thread, then AvSetMmThreadPriority(h, HIGH). The task
        handle is stored per-thread and reverted automatically at thread exit.
        If avrt is unavailable, fall back to SetThreadPriority(HIGHEST).

      - macOS: the mach time-constraint policy (so the AS scheduler places the
        thread on a performance core), re-expressed JUCE-free — mach_timebase_info
        arithmetic instead of juce::Time::secondsToHighResolutionTicks.

      - Linux: best-effort pthread_setschedparam(SCHED_FIFO); graceful fallback
        (returns false) when the process lacks the RLIMIT_RTPRIO / capability.

    This is purely a SCHEDULING hint: it never changes any computed value, so
    every bit-exact render/baseline is unaffected. RealtimeThreadUtil.h forwards
    to setCurrentThreadAudioPriority() here (which is also what finally gives the
    ReverbEngine CPU pool workers priority elevation on Windows — previously a
    no-op there).
*/

#include <algorithm>
#include <cstdint>
#include <thread>
#include <vector>

#if defined(_WIN32)
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
#elif defined(__APPLE__)
 #include <mach/mach.h>
 #include <mach/mach_time.h>
 #include <mach/thread_policy.h>
 #include <pthread.h>
#elif defined(__linux__)
 #include <pthread.h>
 #include <sched.h>
#endif

namespace spatcore::rt {

#if defined(_WIN32)
namespace detail {

// AVRT_PRIORITY enum value from avrt.h (LOW=-1, NORMAL=0, HIGH=1, CRITICAL=2).
// Declared locally so we need neither <avrt.h> nor the avrt.lib import — the
// functions are resolved at RUNTIME via GetProcAddress.
enum { kAvrtPriorityHigh = 1 };

using AvSetMmThreadCharacteristicsW_fn   = HANDLE (WINAPI*) (LPCWSTR, LPDWORD);
using AvSetMmThreadPriority_fn           = BOOL   (WINAPI*) (HANDLE, int);
using AvRevertMmThreadCharacteristics_fn = BOOL   (WINAPI*) (HANDLE);

// Process-lifetime holder for the runtime-loaded avrt entry points.
struct AvrtApi
{
    HMODULE lib = nullptr;
    AvSetMmThreadCharacteristicsW_fn   setCharacteristics = nullptr;
    AvSetMmThreadPriority_fn           setPriority        = nullptr;
    AvRevertMmThreadCharacteristics_fn revert             = nullptr;
    bool ok = false;

    AvrtApi()
    {
        lib = ::LoadLibraryW (L"avrt.dll");
        if (lib != nullptr)
        {
            setCharacteristics = reinterpret_cast<AvSetMmThreadCharacteristicsW_fn> (
                reinterpret_cast<void*> (::GetProcAddress (lib, "AvSetMmThreadCharacteristicsW")));
            setPriority = reinterpret_cast<AvSetMmThreadPriority_fn> (
                reinterpret_cast<void*> (::GetProcAddress (lib, "AvSetMmThreadPriority")));
            revert = reinterpret_cast<AvRevertMmThreadCharacteristics_fn> (
                reinterpret_cast<void*> (::GetProcAddress (lib, "AvRevertMmThreadCharacteristics")));
            ok = (setCharacteristics != nullptr && setPriority != nullptr);
        }
    }
    // Deliberately never FreeLibrary: this is a process-lifetime singleton and
    // worker threads may still need `revert` at their own exit.
};

inline const AvrtApi& avrtApi()
{
    static const AvrtApi api;   // C++11 thread-safe function-local static init
    return api;
}

// Per-thread MMCSS task handle; reverted automatically when the thread exits
// (the plan's "store the task handle to revert on thread exit").
struct MmcssThreadTask
{
    HANDLE handle = nullptr;
    ~MmcssThreadTask()
    {
        if (handle != nullptr)
        {
            const auto& api = avrtApi();
            if (api.revert != nullptr)
                api.revert (handle);
            handle = nullptr;
        }
    }
};

} // namespace detail
#endif // _WIN32

//==============================================================================
/** Elevates the CALLING thread to an audio-realtime scheduling class.

    @param periodMs       Expected wake-up interval (one audio block), ms.
                          Consumed by the macOS time-constraint policy; ignored
                          by the Windows/Linux strategies.
    @param computationMs  Expected processing time per wake-up, ms (clamped to
                          <= periodMs). macOS only.

    @returns true if a realtime elevation took effect; false on graceful
             fallback (e.g. avrt missing -> plain HIGHEST) or an unsupported
             platform. Purely a scheduling hint — never affects values. */
inline bool setCurrentThreadAudioPriority (double periodMs, double computationMs) noexcept
{
#if defined(_WIN32)
    (void) periodMs; (void) computationMs;

    const auto& api = detail::avrtApi();
    if (api.ok)
    {
        static thread_local detail::MmcssThreadTask task;
        if (task.handle != nullptr)
            return true;   // this thread is already registered with MMCSS

        DWORD taskIndex = 0;   // must be 0 on the first call for a task name
        HANDLE h = api.setCharacteristics (L"Pro Audio", &taskIndex);
        if (h != nullptr)
        {
            task.handle = h;
            api.setPriority (h, detail::kAvrtPriorityHigh);
            return true;
        }
    }

    // Fallback: a plain priority bump (also the path when avrt.dll is absent).
    ::SetThreadPriority (::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    return false;

#elif defined(__APPLE__)
    if (periodMs <= 0.0)
        return false;
    if (computationMs <= 0.0 || computationMs > periodMs)
        computationMs = periodMs;

    mach_timebase_info_data_t tb {};
    if (mach_timebase_info (&tb) != KERN_SUCCESS || tb.numer == 0)
        return false;

    // ms -> mach absolute-time ticks: ns * (denom / numer).
    auto msToAbs = [&tb] (double ms) -> uint32_t
    {
        const double ticks = (ms * 1.0e6) * (double) tb.denom / (double) tb.numer;
        return (uint32_t) ticks;
    };

    thread_time_constraint_policy_data_t policy;
    policy.period      = msToAbs (periodMs);
    policy.computation = msToAbs (computationMs);
    policy.constraint  = msToAbs (periodMs);
    policy.preemptible = 1;

    const kern_return_t r = thread_policy_set (pthread_mach_thread_np (pthread_self()),
                                               THREAD_TIME_CONSTRAINT_POLICY,
                                               (thread_policy_t) &policy,
                                               THREAD_TIME_CONSTRAINT_POLICY_COUNT);

    // Mirror JUCE: if the computation budget is rejected, retry smaller.
    if (r == KERN_INVALID_ARGUMENT && computationMs > 50.0)
        return setCurrentThreadAudioPriority (periodMs, 50.0);

    return r == KERN_SUCCESS;

#elif defined(__linux__)
    (void) periodMs; (void) computationMs;

    sched_param sp {};
    const int lo = sched_get_priority_min (SCHED_FIFO);
    const int hi = sched_get_priority_max (SCHED_FIFO);
    sp.sched_priority = (lo >= 0 && hi >= lo) ? (lo + hi) / 2 : 1;
    return pthread_setschedparam (pthread_self(), SCHED_FIFO, &sp) == 0;

#else
    (void) periodMs; (void) computationMs;
    return false;
#endif
}

//==============================================================================
/** Best-effort PHYSICAL (not logical/SMT) core count, used to auto-size the
    host worker pools. Falls back to hardware_concurrency()/2 (SMT assumption)
    when the OS topology query is unavailable. Always >= 1. */
inline int physicalCoreCount() noexcept
{
#if defined(_WIN32)
    DWORD len = 0;
    ::GetLogicalProcessorInformationEx (RelationProcessorCore, nullptr, &len);
    if (len > 0)
    {
        std::vector<unsigned char> buf (len);
        if (::GetLogicalProcessorInformationEx (RelationProcessorCore,
                reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX> (buf.data()),
                &len))
        {
            int cores = 0;
            unsigned char* p   = buf.data();
            unsigned char* end = buf.data() + len;
            while (p + sizeof (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX) <= end + sizeof (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX))
            {
                auto* info = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX> (p);
                if (info->Size == 0)
                    break;
                if (info->Relationship == RelationProcessorCore)
                    ++cores;
                p += info->Size;
                if (p >= end)
                    break;
            }
            if (cores > 0)
                return cores;
        }
    }
#endif
    const unsigned int logical = std::thread::hardware_concurrency();
    if (logical == 0)
        return 1;
    return (int) std::max (1u, logical / 2u);
}

} // namespace spatcore::rt
