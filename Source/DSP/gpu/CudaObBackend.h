#pragma once

/*
    CudaObBackend — native CUDA execution of the WFS OutputBuffer (scatter /
    write-time) algorithm. The Windows/NVIDIA twin of MetalObBackend; exposes the
    identical method surface so ObGpuBackend (the compile-time alias),
    GpuAsyncPipeline and NativeGpuOutputBufferAlgorithm stay backend-agnostic.

    JUCE-free. Owns the CUDA context/module, persistent per-output delay
    accumulators (direct + FR), per-pair HF shelf states, prev->curr matrix
    snapshots and -L latency compensation. Floor-Reflection host work lives in
    the shared WfsFrHostState.
*/

#include <cstdint>
#include <memory>
#include <string>

class CudaObBackend
{
public:
    CudaObBackend();
    ~CudaObBackend();

    CudaObBackend (const CudaObBackend&) = delete;
    CudaObBackend& operator= (const CudaObBackend&) = delete;

    bool prepare (int numInputs,
                  int numOutputs,
                  int blockSize,
                  double sampleRate,
                  double pipelineLatencyMs,
                  double maxDelaySeconds = 1.0);

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

    void reset() noexcept;
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
    std::string deviceName { "CUDA device" };
    double lastLaunchMs { 0.0 };
};
