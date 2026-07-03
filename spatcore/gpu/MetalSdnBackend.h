#pragma once

/*
    MetalSdnBackend — native Metal execution of the GPU SDN reverb.

    JUCE-free, SDK-free, structured like the other native backends (pImpl, MSL
    kernel string compiled at prepare(), synchronous commit+wait processBlock
    driven by the GpuAsyncPipelineT pump). One coupled Scattering Delay Network
    across all nodes, sample-exact with the synchronous CPU SDNAlgorithm — see
    MetalSdnKernels.h for the kernel and SdnHostConfig.h for the host math.

    Unlike the FDN backend, the SDN is geometry-driven: setGeometry() stages the
    node positions and setParameters() stages the seven dynamic params (incl.
    sdnScale, which feeds the inter-node delays). The pump consumes both at the
    next launch, recomputing the per-path delays (with a ~10 ms crossfade on any
    changed length), 3-band decay gains and diffusion, then uploads them. The
    per-path delay lines + filter state persist in device buffers across launches.
*/

#include <cstdint>
#include <memory>
#include <string>

namespace spatcore::gpu {

class MetalSdnBackend
{
public:
    explicit MetalSdnBackend (int deviceIndex = 0);
    ~MetalSdnBackend();

    MetalSdnBackend (const MetalSdnBackend&) = delete;
    MetalSdnBackend& operator= (const MetalSdnBackend&) = delete;

    /** Creates device objects, computes the static per-node config and allocates
        the persistent state. Returns false with getLastError() set on failure. */
    bool prepare (int numNodes, int blockSize, double sampleRate);

    /** Stages the node positions (count * 3 floats: x,y,z per node) for the pump
        thread. Callable from the engine thread. */
    void setGeometry (const float* xyz, int count) noexcept;

    /** Stages the seven dynamic SDN parameters for the pump thread (stepwise,
        consumed at the next launch). Callable from the engine thread. */
    void setParameters (float rt60, float rt60LowMult, float rt60HighMult,
                        float crossoverLow, float crossoverHigh,
                        float diffusion, float sdnScale) noexcept;

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

} // namespace spatcore::gpu

// Extraction-compat alias — app code migrates to qualified names later.
using spatcore::gpu::MetalSdnBackend;
