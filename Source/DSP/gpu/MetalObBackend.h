#pragma once

/*
    MetalObBackend — native Metal execution of the WFS OutputBuffer (scatter /
    write-time) algorithm. The SCATTER twin of MetalWfsBackend.

    JUCE-free, SDK-free. Owns the Metal objects, the persistent per-output delay
    accumulators (direct + Floor-Reflection; the GPU-resident twin of the CPU
    OutputBufferProcessor's per-output delayBuffer/frDelayBuffer), the per-pair
    HF shelf filter states, and the prev->curr matrix tracking that gives
    zipper-free per-sample parameter ramps inside the kernels.

    The host-side processBlock mirrors MetalWfsBackend.mm exactly (matrix
    snapshot with -L compensation, prev->curr ramp continuity, persistent device
    state, host-tracked accumulator-head advance). Floor-Reflection host work
    (per-input pre-filter, diffusion jitter, FR matrix snapshot) lives in the
    SHARED WfsFrHostState - identical to the WFS path, because applying the LTI
    FR shelf chain before vs. after the delay is equivalent.

    Two kernels per launch: ob_filter (per-pair 800 Hz air shelf -> filtered
    staging) then ob_scatter (one single writer per output scatters the filtered
    contributions forward into the per-output accumulator; no atomics).

    Method surface matches MetalWfsBackend so WfsGpuBackend/ObGpuBackend,
    GpuAsyncPipeline and the algorithm wrappers stay backend-agnostic.

    Threading contract identical to MetalWfsBackend: single pump-thread caller
    for processBlock(); FR setters may come from the 50 Hz timer thread.
*/

#include <cstdint>
#include <memory>
#include <string>

class MetalObBackend
{
public:
    MetalObBackend();
    ~MetalObBackend();

    MetalObBackend (const MetalObBackend&) = delete;
    MetalObBackend& operator= (const MetalObBackend&) = delete;

    /** Creates device objects and the persistent per-output accumulators.
        maxDelaySeconds bounds the accumulator length (1.0 mirrors the CPU path).
        Returns false with getLastError() set on failure. */
    bool prepare (int numInputs,
                  int numOutputs,
                  int blockSize,
                  double sampleRate,
                  double pipelineLatencyMs,
                  double maxDelaySeconds = 1.0);

    /** Points the backend at the app's live matrices (input-major
        [in*numOutputs+out]). Same semantics as MetalWfsBackend. */
    void setMatrixPointers (const float* delaysMs, const float* gains,
                            const float* hfAttenDb = nullptr,
                            const float* frDelaysMs = nullptr,
                            const float* frLevels = nullptr,
                            const float* frHfAttenDb = nullptr) noexcept;

    void setFRFilterParams (int inputIndex,
                            bool lowCutActive, float lowCutFreq,
                            bool highShelfActive, float highShelfFreq,
                            float highShelfGain, float highShelfSlope) noexcept;

    void setFRDiffusion (int inputIndex, float diffusionPercent) noexcept;

    bool processBlock (const float* const* inputs, float* const* outputs);

    /** Clears the delay accumulators and prev-matrix state. */
    void reset() noexcept;

    void release() noexcept;

    bool isReady() const noexcept { return ready; }
    const std::string& getLastError() const noexcept { return lastError; }
    double getLastLaunchMs() const noexcept { return lastLaunchMs; }
    const std::string& getDeviceName() const noexcept { return deviceName; }

private:
    struct Impl;                 // Objective-C++ internals (Metal objects)
    std::unique_ptr<Impl> impl;

    bool ready { false };
    std::string lastError;
    std::string deviceName { "Apple Silicon (Metal)" };
    double lastLaunchMs { 0.0 };
};
