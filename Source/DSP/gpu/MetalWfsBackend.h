#pragma once

/*
    MetalWfsBackend — native Metal execution of the WFS delay-and-sum kernels.

    JUCE-free, SDK-free. Owns the Metal objects, the persistent per-input
    delay rings (direct + Floor-Reflection; host-tracked positions,
    GPU-resident storage), the per-pair HF shelf filter states, and the
    prev->curr matrix tracking that gives zipper-free per-sample parameter
    ramps inside the kernels.

    DSP parity with the CPU InputBuffer reference: per-pair 800 Hz / Q 0.3 HF
    shelf on both taps (stepwise per launch, like the CPU's per-block
    setGainDb), Floor Reflections with per-input pre-filter chain (host-side,
    reusing WFSBiquadFilter verbatim) and live diffusion jitter. FR gains ramp
    per sample where the CPU steps them at 50 Hz.

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
    explicit MetalWfsBackend (int deviceIndex = 0);
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

    /** Points the backend at the app's live matrices (input-major
        [in*numOutputs+out]). Read at every launch. delaysMs in ms, gains
        linear; hfAttenDb / frHfAttenDb in dB (negative); frDelaysMs is the
        EXTRA delay of the reflected path relative to direct; frLevels are
        ABSOLUTE linear gains. The four FR/HF pointers may be null (features
        silent). */
    void setMatrixPointers (const float* delaysMs, const float* gains,
                            const float* hfAttenDb = nullptr,
                            const float* frDelaysMs = nullptr,
                            const float* frLevels = nullptr,
                            const float* frHfAttenDb = nullptr) noexcept;

    /** Per-input Floor-Reflection pre-filter parameters (50 Hz timer thread).
        Mirrors InputBufferProcessor's setters. */
    void setFRFilterParams (int inputIndex,
                            bool lowCutActive, float lowCutFreq,
                            bool highShelfActive, float highShelfFreq,
                            float highShelfGain, float highShelfSlope) noexcept;

    /** Per-input FR diffusion amount (0-100%); max jitter 5 ms at 100%. */
    void setFRDiffusion (int inputIndex, float diffusionPercent) noexcept;

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
