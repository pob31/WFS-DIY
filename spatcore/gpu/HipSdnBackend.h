#pragma once

/*
    HipSdnBackend — native HIP/ROCm execution of the GPU SDN reverb (Windows /
    NVIDIA twin of MetalSdnBackend). Same method surface, hipRTC-compiled kernel,
    driven by the GpuAsyncPipelineT pump. One coupled Scattering Delay Network
    across all nodes, sample-exact with the synchronous CPU SDNAlgorithm — see
    CudaSdnKernels.h for the kernel and SdnHostConfig.h for the host math.

    setGeometry() stages the node positions and setParameters() the seven dynamic
    params (incl. sdnScale); the pump consumes both, recomputes the per-path
    delays/crossfade/decay/diffusion on the host and uploads them. The per-path
    delay lines + filter state live in device memory across launches.
*/

#include <cstdint>
#include <memory>
#include <string>

namespace spatcore::gpu {

class HipSdnBackend
{
public:
    explicit HipSdnBackend (int deviceIndex = 0);
    ~HipSdnBackend();

    HipSdnBackend (const HipSdnBackend&) = delete;
    HipSdnBackend& operator= (const HipSdnBackend&) = delete;

    bool prepare (int numNodes, int blockSize, double sampleRate);

    void setGeometry (const float* xyz, int count) noexcept;

    void setParameters (float rt60, float rt60LowMult, float rt60HighMult,
                        float crossoverLow, float crossoverHigh,
                        float diffusion, float sdnScale) noexcept;

    void requestReset() noexcept;

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
    std::string deviceName { "HIP" };
    double lastLaunchMs { 0.0 };
};

} // namespace spatcore::gpu

// Extraction-compat alias — app code migrates to qualified names later.
using spatcore::gpu::HipSdnBackend;
