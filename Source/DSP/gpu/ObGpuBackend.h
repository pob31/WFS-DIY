#pragma once

/*
    ObGpuBackend — compile-time selection of the native GPU OutputBuffer
    (scatter / write-time) backend. The scatter twin of WfsGpuBackend.

    The Metal, CUDA, and HIP backends never coexist in a single binary (Apple -> Metal, AMD/Linux -> HIP, else -> CUDA) and expose the identical method surface
    (prepare / setMatrixPointers / setFRFilterParams / setFRDiffusion /
    processBlock / reset / release / isReady / getLastError / getLastLaunchMs /
    getDeviceName). A plain type alias lets GpuAsyncPipeline and
    NativeGpuOutputBufferAlgorithm stay backend-agnostic with zero #if scatter.
*/

#if defined(__APPLE__)
  #include "MetalObBackend.h"
  using ObGpuBackend = MetalObBackend;
#elif defined(WFS_GPU_HIP)
  #include "HipObBackend.h"
  using ObGpuBackend = HipObBackend;
#else
  #include "CudaObBackend.h"
  using ObGpuBackend = CudaObBackend;
#endif

#if WFS_GPU_NATIVE
#include "GpuBackendInterface.h"
#include <memory>

// Thin adapter wrapping the compile-time-selected concrete OutputBuffer backend
// behind IObBackend (runtime-polymorphic). The concrete backend is untouched.
template <class B>
class ObBackendAdapter final : public IObBackend
{
public:
    bool prepare (int ni, int no, int bs, double sr, double lat, double maxDel) override
        { return impl.prepare (ni, no, bs, sr, lat, maxDel); }
    void setMatrixPointers (const float* d, const float* g, const float* hf,
                            const float* fd, const float* fl, const float* fh) noexcept override
        { impl.setMatrixPointers (d, g, hf, fd, fl, fh); }
    void setFRFilterParams (int i, bool lca, float lcf, bool hsa, float hsf,
                            float hsg, float hss) noexcept override
        { impl.setFRFilterParams (i, lca, lcf, hsa, hsf, hsg, hss); }
    void setFRDiffusion (int i, float p) noexcept override { impl.setFRDiffusion (i, p); }
    bool processBlock (const float* const* in, float* const* out) override { return impl.processBlock (in, out); }
    void reset() noexcept override { impl.reset(); }
    void release() noexcept override { impl.release(); }
    bool isReady() const noexcept override { return impl.isReady(); }
    const std::string& getLastError() const noexcept override { return impl.getLastError(); }
    double getLastLaunchMs() const noexcept override { return impl.getLastLaunchMs(); }
    const std::string& getDeviceName() const noexcept override { return impl.getDeviceName(); }
private:
    B impl;
};

inline std::unique_ptr<IObBackend> makeObBackend()
{
    return std::make_unique<ObBackendAdapter<ObGpuBackend>>();
}
#endif // WFS_GPU_NATIVE
