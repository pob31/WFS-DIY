#pragma once
#if WFS_GPU_NATIVE

/*
    GpuBackendInterface — abstract backend interfaces for RUNTIME-polymorphic
    GPU backend selection (Phase 3 of the GPU build-out).

    Today the backend is chosen at COMPILE time by the *GpuBackend.h aliases
    (Apple->Metal / WFS_GPU_HIP->Hip / else->Cuda). To select a backend (and
    eventually a device/vendor) at RUNTIME, the algorithm wrappers hold a
    pointer to one of these interfaces instead of a concrete value member, and
    a factory (makeXBackend(), in each *GpuBackend.h) returns the right
    implementation wrapped in a thin adapter.

    The adapters wrap the EXISTING concrete backends unchanged (no inheritance
    added to the Metal/CUDA/HIP classes), so this is purely additive — the
    concrete code, kernels and host state are all untouched.

    IGpuBackend is the common pump-facing + lifecycle surface (the only methods
    GpuAsyncPipeline needs, plus release/getDeviceName); each algorithm extends
    it with its own prepare()/setup methods.
*/

#include <string>

/** Parses the per-vendor device ordinal out of a GpuDeviceManager id string
    ("cuda:1" -> 1, "metal:0" -> 0, "cpu" / "" / malformed -> 0). The factory
    already resolves the ordinal on the plugin path; this is for the in-process
    (macOS / non-plugin) path that only has the id string. Non-throwing. */
inline int deviceIndexFromId (const std::string& id) noexcept
{
    const std::string::size_type colon = id.find_last_of (':');
    if (colon == std::string::npos || colon + 1 >= id.size())
        return 0;

    int value = 0;
    for (std::string::size_type i = colon + 1; i < id.size(); ++i)
    {
        const char c = id[i];
        if (c < '0' || c > '9')
            return 0;                         // malformed -> default device
        value = value * 10 + (c - '0');
    }
    return value;
}

struct IGpuBackend
{
    virtual ~IGpuBackend() = default;

    // Pump-facing (called on the GpuAsyncPipeline pump thread):
    virtual bool processBlock (const float* const* inputs, float* const* outputs) = 0;
    virtual bool isReady() const noexcept = 0;
    virtual const std::string& getLastError() const noexcept = 0;
    virtual double getLastLaunchMs() const noexcept = 0;

    // Lifecycle / status (called by the algorithm wrapper):
    virtual void release() noexcept = 0;
    virtual const std::string& getDeviceName() const noexcept = 0;
};

// WFS delay-and-sum (gather) backend.
struct IWfsBackend : IGpuBackend
{
    virtual bool prepare (int numInputs, int numOutputs, int blockSize,
                          double sampleRate, double pipelineLatencyMs,
                          double maxDelaySeconds) = 0;
    virtual void setMatrixPointers (const float* delaysMs, const float* gains,
                                    const float* hfAttenDb, const float* frDelaysMs,
                                    const float* frLevels, const float* frHfAttenDb) noexcept = 0;
    virtual void setFRFilterParams (int inputIndex, bool lowCutActive, float lowCutFreq,
                                    bool highShelfActive, float highShelfFreq,
                                    float highShelfGain, float highShelfSlope) noexcept = 0;
    virtual void setFRDiffusion (int inputIndex, float diffusionPercent) noexcept = 0;
    virtual void reset() noexcept = 0;
};

// OutputBuffer (scatter) backend — same surface as WFS.
struct IObBackend : IGpuBackend
{
    virtual bool prepare (int numInputs, int numOutputs, int blockSize,
                          double sampleRate, double pipelineLatencyMs,
                          double maxDelaySeconds) = 0;
    virtual void setMatrixPointers (const float* delaysMs, const float* gains,
                                    const float* hfAttenDb, const float* frDelaysMs,
                                    const float* frLevels, const float* frHfAttenDb) noexcept = 0;
    virtual void setFRFilterParams (int inputIndex, bool lowCutActive, float lowCutFreq,
                                    bool highShelfActive, float highShelfFreq,
                                    float highShelfGain, float highShelfSlope) noexcept = 0;
    virtual void setFRDiffusion (int inputIndex, float diffusionPercent) noexcept = 0;
    virtual void reset() noexcept = 0;
};

// IR convolution reverb backend.
struct IIrBackend : IGpuBackend
{
    virtual bool prepare (int numNodes, int blockSize, double sampleRate, int maxIrSamples) = 0;
    virtual void stageIr (const float* monoIr, int numSamples) = 0;
    virtual void requestReset() noexcept = 0;
    virtual int getSegmentsLoaded() const noexcept = 0;
    virtual int getSegmentsTotal() const noexcept = 0;
};

// FDN reverb backend.
struct IFdnBackend : IGpuBackend
{
    virtual bool prepare (int numNodes, int blockSize, double sampleRate, float fdnSize) = 0;
    virtual void setParameters (float rt60, float rt60LowMult, float rt60HighMult,
                                float crossoverLow, float crossoverHigh, float diffusion) noexcept = 0;
    virtual void requestReset() noexcept = 0;
};

// SDN reverb backend.
struct ISdnBackend : IGpuBackend
{
    virtual bool prepare (int numNodes, int blockSize, double sampleRate) = 0;
    virtual void setGeometry (const float* xyz, int count) noexcept = 0;
    virtual void setParameters (float rt60, float rt60LowMult, float rt60HighMult,
                                float crossoverLow, float crossoverHigh,
                                float diffusion, float sdnScale) noexcept = 0;
    virtual void requestReset() noexcept = 0;
};

#endif // WFS_GPU_NATIVE
