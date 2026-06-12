#pragma once

/*
    MetalWfsBackend — native Metal execution of the WFS delay-and-sum kernel.

    JUCE-free, SDK-free. Owns the Metal objects, the persistent per-input
    delay rings (host-tracked positions, GPU-resident storage), and the
    prev->curr matrix tracking that gives zipper-free per-sample parameter
    ramps inside the kernel.

    Threading contract: single caller thread for processBlock() (the
    GpuAsyncPipeline pump). processBlock is synchronous (commit + wait) —
    deadline isolation comes from the pipeline above it, exactly as with the
    SDK backend. updateMatrices() may be called from any thread between
    blocks under the same informal single-word-float model used everywhere
    in the app (the backend snapshots into GPU buffers at launch time).

    Latency compensation: pipelineLatencyMs is PRE-SUBTRACTED from every
    pair's delay (clamped at 0) at launch time, so arrival times match the
    CPU path for all delays >= L.

    Measured on M4 Pro (Experiments/metal-wfs-spike): sync launch 0.13-0.17 ms
    mean across 8x16..64x128; correctness vs CPU reference ~1e-7.
*/

#include <cstdint>
#include <memory>
#include <string>

class MetalWfsBackend
{
public:
    MetalWfsBackend();
    ~MetalWfsBackend();

    MetalWfsBackend (const MetalWfsBackend&) = delete;
    MetalWfsBackend& operator= (const MetalWfsBackend&) = delete;

    /** Creates device objects and the persistent rings.
        maxDelaySeconds bounds the ring size (1.0 mirrors the CPU path).
        Returns false with getLastError() set on failure. */
    bool prepare (int numInputs,
                  int numOutputs,
                  int blockSize,
                  double sampleRate,
                  double pipelineLatencyMs,
                  double maxDelaySeconds = 1.0);

    /** Points the backend at the app's live matrices (ms delays, linear
        gains, input-major [in*numOutputs+out]). Read at every launch. */
    void setMatrixPointers (const float* delaysMs, const float* gains) noexcept;

    /** Processes one block synchronously on the GPU.
        inputs/outputs: arrays of channel pointers (numInputs / numOutputs),
        each blockSize samples. Returns false on device error. */
    bool processBlock (const float* const* inputs, float* const* outputs);

    /** Clears ring history and prev-matrix state (next launch ramps from the
        then-current matrices; history reads return silence). */
    void reset() noexcept;

    void release() noexcept;

    bool isReady() const noexcept { return ready; }
    const std::string& getLastError() const noexcept { return lastError; }
    double getLastLaunchMs() const noexcept { return lastLaunchMs; }

    /** Backend-identifying string for logs/UI. Part of the shared backend
        contract (mirrored by CudaWfsBackend) so NativeGpuWfsAlgorithm can
        forward it without knowing which backend is compiled in. */
    const std::string& getDeviceName() const noexcept { return deviceName; }

private:
    struct Impl;                 // Objective-C++ internals (Metal objects)
    std::unique_ptr<Impl> impl;

    bool ready { false };
    std::string lastError;
    std::string deviceName { "Apple Silicon (Metal)" };
    double lastLaunchMs { 0.0 };
};
