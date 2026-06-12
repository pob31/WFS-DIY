#pragma once

/*
    CudaWfsBackend - native CUDA execution of the WFS delay-and-sum kernel,
    the Windows/NVIDIA twin of MetalWfsBackend.

    JUCE-free, SDK-free. Owns the CUDA objects, the persistent per-input delay
    ring (host-tracked positions, device-resident storage), and the prev->curr
    matrix tracking that gives zipper-free per-sample parameter ramps inside the
    kernel. The kernel is compiled at prepare() time via NVRTC (mirroring the
    Metal backend's runtime MSL compile) and launched with the CUDA Driver API,
    so the build needs no .cu file and no nvcc step.

    Threading contract: single caller thread for processBlock() (the
    GpuAsyncPipeline pump). processBlock is synchronous (async copies + launch
    on one stream, then cudaStreamSynchronize) - deadline isolation comes from
    the pipeline above it, exactly as with the Metal backend.

    Latency compensation: pipelineLatencyMs is PRE-SUBTRACTED from every pair's
    delay (clamped at 0) at launch time, so arrival times match the CPU path for
    all delays >= L.

    Method surface is identical to MetalWfsBackend so WfsGpuBackend can alias
    either at compile time.
*/

#include <cstdint>
#include <memory>
#include <string>

class CudaWfsBackend
{
public:
    CudaWfsBackend();
    ~CudaWfsBackend();

    CudaWfsBackend (const CudaWfsBackend&) = delete;
    CudaWfsBackend& operator= (const CudaWfsBackend&) = delete;

    /** Creates device objects, compiles the kernel, and the persistent ring.
        maxDelaySeconds bounds the ring size (1.0 mirrors the CPU path).
        Returns false with getLastError() set on failure (no CUDA device,
        NVRTC compile error, allocation failure, ...). */
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

    /** Backend-identifying string for logs/UI, e.g.
        "NVIDIA GeForce GTX 1650 (CUDA)". Set in prepare(). */
    const std::string& getDeviceName() const noexcept { return deviceName; }

private:
    struct Impl;                 // CUDA driver/runtime internals
    std::unique_ptr<Impl> impl;

    bool ready { false };
    std::string lastError;
    std::string deviceName;
    double lastLaunchMs { 0.0 };
};
