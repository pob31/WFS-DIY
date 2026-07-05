#pragma once

/*
    GpuHostWorkPool — a JUCE-free fork-join thread pool for parallelising the
    per-block HOST prep the GPU backends do before each kernel launch (input
    packing, per-input FR filtering / jitter, matrix snapshots, IR FFTs).

    Why not reuse rt/AudioParallelFor.h? It transitively pulls JUCE (WorkgroupToken,
    AudioWorkgroupCoordinator) and the vendor plugin DLLs cannot include JUCE
    (finding F1). This is the same proven fork-join skeleton re-expressed with
    only the standard library: std::thread workers + a mutex/CV dispatch + an
    atomic item counter. The macOS audio-workgroup nicety is intentionally
    dropped (flagged in the plan; the Metal path can add it later).

    Ownership / lifecycle (plan section 3):
      - ONE pool per backend Impl (Wfs / Ob / Ir). Pools are NOT shared across
        concurrently-pumping backends (single dispatch state, F3).
      - Created in the backend prepare() (engine-setup thread).
      - Used ONLY from the single pump thread (single-caller contract).
      - shutdown()/joined in the backend release() BEFORE any CUDA/HIP teardown
        and before the plugin DLL could unload.
      - Workers NEVER touch GPU APIs — every parallelFor lambda operates on host
        memory only, so there are no per-worker GPU-context concerns.

    Determinism (plan section 4): parallelFor distributes items dynamically, but
    every item writes only item-indexed state and every item's FP sequence is a
    pure function of (item index, block inputs, per-item persistent state) — no
    cross-item host accumulation. Results are therefore bit-identical for ANY
    worker count / scheduling. The M3 gate proves this in executable form:
    WFS_GPU_HOST_WORKERS=0 vs =3 must yield byte-identical GPU baselines.
*/

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include "../rt/RtThreadPriority.h"

namespace spatcore::gpu {

class GpuHostWorkPool
{
public:
    GpuHostWorkPool() = default;
    ~GpuHostWorkPool() { shutdown(); }

    GpuHostWorkPool (const GpuHostWorkPool&) = delete;
    GpuHostWorkPool& operator= (const GpuHostWorkPool&) = delete;

    //==========================================================================
    /** Spawn persistent worker threads. Safe to call repeatedly (shuts down
        first). numWorkers == 0 => no threads: parallelFor runs fully sequential
        on the caller (the WFS_GPU_HOST_WORKERS=0 kill switch / determinism
        cross-check). periodMs/computationMs feed RtThreadPriority (macOS P-core
        placement); Windows/Linux ignore them. */
    void prepare (int numWorkers, double periodMs = 0.0, double computationMs = 0.0)
    {
        shutdown();

        if (numWorkers <= 0)
        {
            numActiveWorkers = 0;
            return;
        }

        realtimePeriodMs      = periodMs;
        realtimeComputationMs = computationMs;

        running.store (true, std::memory_order_release);
        numActiveWorkers = numWorkers;

        workers.reserve (static_cast<size_t> (numWorkers));
        for (int i = 0; i < numWorkers; ++i)
            workers.emplace_back ([this] { workerLoop(); });
    }

    /** Join all workers. Safe to call multiple times / when never prepared. */
    void shutdown()
    {
        if (workers.empty())
        {
            numActiveWorkers = 0;
            return;
        }

        running.store (false, std::memory_order_release);

        {
            std::lock_guard<std::mutex> lock (dispatchMutex);
            dispatch.store (true, std::memory_order_release);
        }
        dispatchCV.notify_all();

        for (auto& w : workers)
            if (w.joinable())
                w.join();

        workers.clear();
        numActiveWorkers = 0;
    }

    //==========================================================================
    /** Run func(0), func(1), ... func(count-1) across the workers + the calling
        thread, blocking until all items complete. Sequential fallback when no
        workers are prepared or count <= 1 (identical call order to a plain
        for-loop). Single-caller contract: only the pump thread calls this. */
    template <typename Func>
    void parallelFor (int count, Func&& func)
    {
        if (count <= 0)
            return;

        // Sequential fallback: no workers or a single item. Same order as the
        // original loop -> bit-identical to the pre-M3 sequential code.
        if (workers.empty() || count == 1)
        {
            for (int i = 0; i < count; ++i)
                func (i);
            return;
        }

        // Publish the work item. currentFunc holds a type-erased view of func;
        // it is only alive for the duration of this call (workers are parked
        // again before we return), so capturing by reference is safe.
        currentFunc = [&func] (int i) { func (i); };
        totalItems.store (count, std::memory_order_relaxed);
        nextItem.store (0, std::memory_order_relaxed);
        doneCount.store (0, std::memory_order_release);

        // Wake the workers.
        {
            std::lock_guard<std::mutex> lock (dispatchMutex);
            dispatch.store (true, std::memory_order_release);
        }
        dispatchCV.notify_all();

        // The calling (pump) thread participates.
        executeItems();

        // Wait for every item to complete (CV, no spin).
        {
            std::unique_lock<std::mutex> lock (doneMutex);
            doneCV.wait (lock, [this, count] {
                return doneCount.load (std::memory_order_acquire) >= count;
            });
        }

        // Park the workers again (clear dispatch).
        {
            std::lock_guard<std::mutex> lock (dispatchMutex);
            dispatch.store (false, std::memory_order_release);
        }
        dispatchCV.notify_all();

        currentFunc = nullptr;
    }

    /** Worker threads spawned (not counting the calling thread). */
    int getNumWorkers() const noexcept { return numActiveWorkers; }

private:
    //==========================================================================
    void workerLoop()
    {
        // Self-elevate to the audio scheduling class (macOS P-core placement /
        // Windows MMCSS "Pro Audio" / Linux SCHED_FIFO). Host-only work, never
        // GPU APIs.
        if (realtimePeriodMs > 0.0)
            spatcore::rt::setCurrentThreadAudioPriority (realtimePeriodMs, realtimeComputationMs);

        while (running.load (std::memory_order_acquire))
        {
            {
                std::unique_lock<std::mutex> lock (dispatchMutex);
                dispatchCV.wait (lock, [this] {
                    return dispatch.load (std::memory_order_acquire);
                });
            }

            if (! running.load (std::memory_order_acquire))
                break;

            executeItems();

            // Wait until the caller clears dispatch (has collected all items).
            {
                std::unique_lock<std::mutex> lock (dispatchMutex);
                dispatchCV.wait (lock, [this] {
                    return ! dispatch.load (std::memory_order_acquire)
                        || ! running.load (std::memory_order_acquire);
                });
            }
        }
    }

    void executeItems()
    {
        const int total = totalItems.load (std::memory_order_relaxed);

        while (true)
        {
            const int idx = nextItem.fetch_add (1, std::memory_order_relaxed);
            if (idx >= total)
                break;

            if (currentFunc)
                currentFunc (idx);

            if (doneCount.fetch_add (1, std::memory_order_release) + 1 >= total)
            {
                std::lock_guard<std::mutex> lock (doneMutex);
                doneCV.notify_one();
            }
        }
    }

    //==========================================================================
    std::vector<std::thread> workers;
    int numActiveWorkers = 0;
    std::atomic<bool> running { false };

    // Realtime scheduling hints (macOS P-core placement); 0 = disabled.
    double realtimePeriodMs = 0.0;
    double realtimeComputationMs = 0.0;

    // Dispatch signalling (caller -> workers).
    std::mutex dispatchMutex;
    std::condition_variable dispatchCV;
    std::atomic<bool> dispatch { false };

    // Completion signalling (workers -> caller).
    std::mutex doneMutex;
    std::condition_variable doneCV;

    // Work items.
    std::function<void (int)> currentFunc;
    std::atomic<int> totalItems { 0 };
    std::atomic<int> nextItem { 0 };
    std::atomic<int> doneCount { 0 };
};

//==============================================================================
/** Resolve the host worker count: the WFS_GPU_HOST_WORKERS env override if set
    (0 = sequential kill switch + determinism cross-check), else autoDefault.
    Read once per backend prepare(). Clamped to a sane [0, 32]. */
inline int hostWorkerCountFromEnv (int autoDefault) noexcept
{
    if (const char* e = std::getenv ("WFS_GPU_HOST_WORKERS"))
    {
        char* end = nullptr;
        const long v = std::strtol (e, &end, 10);
        if (end != e && v >= 0)
            return (int) (v < 32 ? v : 32);
    }
    return autoDefault;
}

} // namespace spatcore::gpu
