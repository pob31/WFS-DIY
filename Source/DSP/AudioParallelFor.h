#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

//==============================================================================
/**
    Lightweight fork-join thread pool for parallelising per-node DSP.

    Usage:
        AudioParallelFor pool;
        pool.prepare (3);                       // 3 workers + calling thread = 4 cores
        pool.parallelFor (numNodes, [&](int n) {
            processNode (n);
        });
        pool.shutdown();                        // joins all workers

    Workers sleep on a condition variable between dispatches.
    Work distribution uses an atomic counter (implicit work-stealing).
    The calling thread participates, then waits for all items to complete.
    Falls back to sequential if count <= 1 or no workers are prepared.
*/
class AudioParallelFor
{
public:
    AudioParallelFor() = default;

    ~AudioParallelFor()
    {
        shutdown();
    }

    //==========================================================================
    /** Create persistent worker threads. Safe to call multiple times (shuts down first). */
    void prepare (int numWorkers)
    {
        shutdown();

        if (numWorkers <= 0)
            return;

        running.store (true, std::memory_order_release);
        numActiveWorkers = numWorkers;

        workers.reserve (static_cast<size_t> (numWorkers));
        for (int i = 0; i < numWorkers; ++i)
            workers.emplace_back ([this] { workerLoop(); });
    }

    /** Join all workers. Safe to call multiple times. */
    void shutdown()
    {
        if (workers.empty())
            return;

        running.store (false, std::memory_order_release);

        {
            std::lock_guard<std::mutex> lock (dispatchMutex);
            dispatch.store (true, std::memory_order_release);
        }
        dispatchCV.notify_all();

        for (auto& w : workers)
        {
            if (w.joinable())
                w.join();
        }

        workers.clear();
        numActiveWorkers = 0;
    }

    //==========================================================================
    /**
        Distribute func(0), func(1), ... func(count-1) across workers + calling thread.
        Blocks until all items are complete.
    */
    template <typename Func>
    void parallelFor (int count, Func&& func)
    {
        if (count <= 0)
            return;

        // Sequential fallback: no workers or only 1 item
        if (workers.empty() || count == 1)
        {
            for (int i = 0; i < count; ++i)
                func (i);
            return;
        }

        // Set up the work
        currentFunc = [&func](int i) { func (i); };
        totalItems.store (count, std::memory_order_relaxed);
        nextItem.store (0, std::memory_order_relaxed);
        doneCount.store (0, std::memory_order_release);

        // Wake workers
        {
            std::lock_guard<std::mutex> lock (dispatchMutex);
            dispatch.store (true, std::memory_order_release);
        }
        dispatchCV.notify_all();

        // Calling thread participates
        executeItems();

        // Wait for all items to complete
        while (doneCount.load (std::memory_order_acquire) < count)
        {
            // Spin briefly, then yield
            for (int spin = 0; spin < 64; ++spin)
            {
                if (doneCount.load (std::memory_order_acquire) >= count)
                    break;
            }
            std::this_thread::yield();
        }

        // Reset dispatch flag so workers go back to sleep
        dispatch.store (false, std::memory_order_release);
        currentFunc = nullptr;
    }

    /** Get number of active workers (not counting the calling thread). */
    int getNumWorkers() const { return numActiveWorkers; }

private:
    //==========================================================================
    void workerLoop()
    {
        while (running.load (std::memory_order_acquire))
        {
            // Wait for work dispatch
            {
                std::unique_lock<std::mutex> lock (dispatchMutex);
                dispatchCV.wait (lock, [this] {
                    return dispatch.load (std::memory_order_acquire);
                });
            }

            if (! running.load (std::memory_order_acquire))
                break;

            executeItems();

            // Wait until dispatch is cleared (main thread has collected results)
            while (dispatch.load (std::memory_order_acquire)
                   && running.load (std::memory_order_acquire))
            {
                std::this_thread::yield();
            }
        }
    }

    void executeItems()
    {
        int total = totalItems.load (std::memory_order_relaxed);

        while (true)
        {
            int idx = nextItem.fetch_add (1, std::memory_order_relaxed);
            if (idx >= total)
                break;

            if (currentFunc)
                currentFunc (idx);

            doneCount.fetch_add (1, std::memory_order_release);
        }
    }

    //==========================================================================
    // Worker threads
    std::vector<std::thread> workers;
    int numActiveWorkers = 0;
    std::atomic<bool> running { false };

    // Dispatch signalling
    std::mutex dispatchMutex;
    std::condition_variable dispatchCV;
    std::atomic<bool> dispatch { false };

    // Work items
    std::function<void (int)> currentFunc;
    std::atomic<int> totalItems { 0 };
    std::atomic<int> nextItem { 0 };
    std::atomic<int> doneCount { 0 };
};
