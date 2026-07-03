#pragma once

/*
    CudaWfsBackend - native CUDA execution of the WFS delay-and-sum kernel,
    the Windows/NVIDIA twin of MetalWfsBackend.

    JUCE-free, SDK-free. Owns the CUDA objects, the persistent per-input delay
    rings (direct + Floor-Reflection; host-tracked positions, device-resident
    storage), the per-pair HF shelf filter states, and the prev->curr matrix
    tracking that gives zipper-free per-sample parameter ramps inside the
    kernels. The kernels are compiled at prepare() time via NVRTC (mirroring
    the Metal backend's runtime MSL compile) and launched with the CUDA Driver
    API, so the build needs no .cu file and no nvcc step.

    DSP parity with the CPU InputBuffer reference: per-pair 800 Hz / Q 0.3 HF
    shelf on both taps (stepwise per launch, like the CPU's per-block
    setGainDb), Floor Reflections with per-input pre-filter chain (host-side,
    reusing WFSBiquadFilter verbatim) and live diffusion jitter. FR gains ramp
    per sample where the CPU steps them at 50 Hz - same accepted-divergence
    class as the base port's delay ramps.

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

namespace spatcore::gpu {

class CudaWfsBackend
{
public:
    explicit CudaWfsBackend (int deviceIndex = 0);
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

} // namespace spatcore::gpu

// Extraction-compat alias — app code migrates to qualified names later.
using spatcore::gpu::CudaWfsBackend;
