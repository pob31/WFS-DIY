#pragma once

/*
    MetalFdnBackend — native Metal execution of the GPU FDN reverb.

    JUCE-free, SDK-free, structured like the other native backends (pImpl,
    MSL kernel string compiled at prepare(), synchronous commit+wait
    processBlock driven by the GpuAsyncPipelineT pump). One 16-line Feedback
    Delay Network per node, sample-exact with the CPU FDNAlgorithm — see
    MetalFdnKernels.h for the kernel and FdnHostConfig.h for the host math.

    Parameters are applied stepwise (the CPU FDN does not ramp): setParameters()
    stages the six dynamic params from the engine thread; the pump consumes
    them at the next launch (recomputes the decay/diffusion coefficients and
    uploads them). fdnSize is captured at prepare() only, matching the CPU
    (its delay lengths are never resized for a runtime fdnSize change).
*/

#include <cstdint>
#include <memory>
#include <string>

class MetalFdnBackend
{
public:
    explicit MetalFdnBackend (int deviceIndex = 0);
    ~MetalFdnBackend();

    MetalFdnBackend (const MetalFdnBackend&) = delete;
    MetalFdnBackend& operator= (const MetalFdnBackend&) = delete;

    /** Creates device objects, computes the static per-node config (from
        fdnSize) and allocates the persistent state. Returns false with
        getLastError() set on failure. */
    bool prepare (int numNodes,
                  int blockSize,
                  double sampleRate,
                  float fdnSize);

    /** Stages the six dynamic FDN parameters for the pump thread (stepwise,
        consumed at the next launch). Callable from the engine thread. */
    void setParameters (float rt60, float rt60LowMult, float rt60HighMult,
                        float crossoverLow, float crossoverHigh, float diffusion) noexcept;

    /** Clears all delay/filter state at the next launch. Any thread. */
    void requestReset() noexcept;

    /** Processes one block synchronously on the GPU (pump thread).
        inputs/outputs: numNodes channel pointers, blockSize samples each. */
    bool processBlock (const float* const* inputs, float* const* outputs);

    void release() noexcept;

    bool isReady() const noexcept { return ready; }
    const std::string& getLastError() const noexcept { return lastError; }
    double getLastLaunchMs() const noexcept { return lastLaunchMs; }
    const std::string& getDeviceName() const noexcept { return deviceName; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl;

    bool ready { false };
    std::string lastError;
    std::string deviceName { "Apple Silicon (Metal)" };
    double lastLaunchMs { 0.0 };
};
