#pragma once
#if WFS_GPU_NATIVE

/*
    GpuBackendFactory — runtime backend creation by DEVICE (Phase 3 Stage 3).

    Given a device id from GpuDeviceManager ("hip:0", "cuda:1", "cpu", ...),
    dlopens the matching CPU-safe vendor plugin (libwfs_<vendor>.so) and returns
    a backend through its interface. Returns nullptr for "cpu", an unknown id, or
    a vendor whose plugin/runtime is absent — the caller then uses the CPU path.

    The plugin .so must be findable by the loader (shipped next to the executable
    with an $ORIGIN rpath, or on LD_LIBRARY_PATH). Loaded plugins are cached and
    never dlclose()d while the process runs (their backends' vtables live there).

    The main app links NO GPU runtime; all vendor coupling lives in the plugins.
*/

#include "GpuBackendInterface.h"
#include "GpuDeviceManager.h"

#include <map>
#include <memory>
#include <string>

#if ! defined(__APPLE__)
 #include "PlatformDynLib.h"   // dlopen / LoadLibrary shim
#endif

namespace spatcore::gpu {

class GpuBackendFactory
{
public:
    static GpuBackendFactory& instance()
    {
        static GpuBackendFactory f;
        return f;
    }

    std::unique_ptr<IWfsBackend> makeWfs (const std::string& deviceId) { return wrap<IWfsBackend> (deviceId, "wfs_plugin_create_wfs"); }
    std::unique_ptr<IObBackend>  makeOb  (const std::string& deviceId) { return wrap<IObBackend>  (deviceId, "wfs_plugin_create_ob");  }
    std::unique_ptr<IIrBackend>  makeIr  (const std::string& deviceId) { return wrap<IIrBackend>  (deviceId, "wfs_plugin_create_ir");  }
    std::unique_ptr<IFdnBackend> makeFdn (const std::string& deviceId) { return wrap<IFdnBackend> (deviceId, "wfs_plugin_create_fdn"); }
    std::unique_ptr<ISdnBackend> makeSdn (const std::string& deviceId) { return wrap<ISdnBackend> (deviceId, "wfs_plugin_create_sdn"); }

private:
    GpuBackendFactory() = default;

    template <class T>
    std::unique_ptr<T> wrap (const std::string& deviceId, const char* createSym)
    {
        const GpuDevice* dev = GpuDeviceManager::instance().find (deviceId);
        if (dev == nullptr || dev->isCpu())
            return nullptr;                       // CPU / unknown -> caller uses CPU path

#if defined(__APPLE__)
        (void) createSym;
        return nullptr;                           // Metal is in-process on macOS (no plugin) — TODO
#else
        wfsdyn::LibHandle lib = loadVendor (dev->vendor);
        if (lib == nullptr)
            return nullptr;                       // plugin/runtime absent -> CPU fallback

        auto create = reinterpret_cast<T* (*) (int)> (wfsdyn::sym (lib, createSym));
        if (create == nullptr)
            return nullptr;

        return std::unique_ptr<T> (create (dev->index));
#endif
    }

#if ! defined(__APPLE__)
    wfsdyn::LibHandle loadVendor (GpuDevice::Vendor v)
    {
        std::string so;
        switch (v)
        {
            case GpuDevice::Vendor::HIP:  so = wfsdyn::pluginName ("hip");  break;
            case GpuDevice::Vendor::CUDA: so = wfsdyn::pluginName ("cuda"); break;
            default: return nullptr;
        }

        auto it = handles.find (so);
        if (it != handles.end())
            return it->second;                    // cached (incl. cached null on failure)

        // Prefer the plugin shipped next to the executable; fall back to the
        // loader search path (LD_LIBRARY_PATH/rpath on Linux, PATH/dir on Windows).
        wfsdyn::LibHandle h = nullptr;
        const std::string beside = wfsdyn::exeDir();
        if (! beside.empty())
            h = wfsdyn::openLib ((beside + "/" + so).c_str());
        if (h == nullptr)
            h = wfsdyn::openLib (so.c_str());

        handles[so] = h;
        return h;
    }

    std::map<std::string, wfsdyn::LibHandle> handles;
#endif
};

} // namespace spatcore::gpu

// Extraction-compat alias — app code migrates to qualified names later.
using spatcore::gpu::GpuBackendFactory;

#endif // WFS_GPU_NATIVE
