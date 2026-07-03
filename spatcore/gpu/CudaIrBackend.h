#pragma once

/*
    CudaIrBackend — Windows/NVIDIA twin of MetalIrBackend.

    Identical method surface and host-side behaviour (the shared work lives
    in IrConvHostState); only the device plumbing differs: NVRTC compiles
    the CudaIrKernels.h string at prepare() time, buffers are device
    allocations fed via pinned staging, and processBlock is a synchronous
    upload -> ir_fdl_mac -> download on a private stream (the
    GpuAsyncPipelineT pump above provides the deadline isolation).

    See MetalIrBackend.h for the full algorithm/threading contract notes.
*/

#include <cstdint>
#include <memory>
#include <string>

namespace spatcore::gpu {

class CudaIrBackend
{
public:
    explicit CudaIrBackend (int deviceIndex = 0);
    ~CudaIrBackend();

    CudaIrBackend (const CudaIrBackend&) = delete;
    CudaIrBackend& operator= (const CudaIrBackend&) = delete;

    bool prepare (int numNodes,
                  int blockSize,
                  double sampleRate,
                  int maxIrSamples);

    void stageIr (const float* monoIr, int numSamples);
    void requestReset() noexcept;

    bool processBlock (const float* const* inputs, float* const* outputs);

    void release() noexcept;

    bool isReady() const noexcept { return ready; }
    const std::string& getLastError() const noexcept { return lastError; }
    double getLastLaunchMs() const noexcept { return lastLaunchMs; }
    const std::string& getDeviceName() const noexcept { return deviceName; }

    int getSegmentsLoaded() const noexcept;
    int getSegmentsTotal() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;

    bool ready { false };
    std::string lastError;
    std::string deviceName { "NVIDIA (CUDA)" };
    double lastLaunchMs { 0.0 };
};

} // namespace spatcore::gpu

// Extraction-compat alias — app code migrates to qualified names later.
using spatcore::gpu::CudaIrBackend;
