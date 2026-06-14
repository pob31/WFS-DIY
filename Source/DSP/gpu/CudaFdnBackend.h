#pragma once

/*
    CudaFdnBackend — Windows/NVIDIA twin of MetalFdnBackend.

    Identical method surface and host-side behaviour (the shared host math
    lives in FdnHostConfig); only the device plumbing differs: NVRTC compiles
    the CudaFdnKernels.h string at prepare(), buffers are device allocations
    fed via pinned staging, and processBlock is a synchronous upload ->
    fdn_process -> download on a private stream (the GpuAsyncPipelineT pump
    above provides the deadline isolation).

    See MetalFdnBackend.h for the full algorithm/threading contract.
*/

#include <cstdint>
#include <memory>
#include <string>

class CudaFdnBackend
{
public:
    CudaFdnBackend();
    ~CudaFdnBackend();

    CudaFdnBackend (const CudaFdnBackend&) = delete;
    CudaFdnBackend& operator= (const CudaFdnBackend&) = delete;

    bool prepare (int numNodes, int blockSize, double sampleRate, float fdnSize);

    void setParameters (float rt60, float rt60LowMult, float rt60HighMult,
                        float crossoverLow, float crossoverHigh, float diffusion) noexcept;

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
    std::string deviceName { "NVIDIA (CUDA)" };
    double lastLaunchMs { 0.0 };
};
