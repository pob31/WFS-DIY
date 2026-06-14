#pragma once

/*
    WfsGpuBackend — compile-time selection of the native GPU WFS backend.

    The Metal, CUDA, and HIP backends never coexist in a single binary (Apple -> Metal, AMD/Linux -> HIP, else -> CUDA) and they expose the identical method
    surface (prepare / setMatrixPointers / processBlock / reset / release /
    isReady / getLastError / getLastLaunchMs / getDeviceName). A plain type
    alias is therefore enough to let GpuAsyncPipeline and NativeGpuWfsAlgorithm
    stay backend-agnostic with zero #if scatter.
*/

#if defined(__APPLE__)
  #include "MetalWfsBackend.h"
  using WfsGpuBackend = MetalWfsBackend;
#elif defined(WFS_GPU_HIP)
  #include "HipWfsBackend.h"
  using WfsGpuBackend = HipWfsBackend;
#else
  #include "CudaWfsBackend.h"
  using WfsGpuBackend = CudaWfsBackend;
#endif

#if WFS_GPU_NATIVE
#include "GpuBackendInterface.h"
#include <memory>

// Thin adapter wrapping the compile-time-selected concrete WFS backend behind
// IWfsBackend, so the algorithm wrapper can hold it by interface pointer
// (runtime-polymorphic). The concrete backend is untouched.
template <class B>
class WfsBackendAdapter final : public IWfsBackend
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

/** Runtime-polymorphic entry point. On the plugin build (WFS_GPU_PLUGINS, Linux)
    it dlopens the per-vendor plugin for the device id; otherwise it returns the
    compile-time-selected backend (macOS Metal / Windows CUDA / Linux hard-link). */
#if defined(WFS_GPU_PLUGINS) && ! defined(__APPLE__)
 #include "GpuBackendFactory.h"
 inline std::unique_ptr<IWfsBackend> makeWfsBackend (const std::string& deviceId)
 {
     return GpuBackendFactory::instance().makeWfs (deviceId);
 }
#else
 inline std::unique_ptr<IWfsBackend> makeWfsBackend (const std::string& deviceId = {})
 {
     (void) deviceId;
     return std::make_unique<WfsBackendAdapter<WfsGpuBackend>>();
 }
#endif
#endif // WFS_GPU_NATIVE
