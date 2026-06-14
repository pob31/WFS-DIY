#pragma once
#if WFS_GPU_NATIVE

/*
    GpuDeviceManager — runtime enumeration of available compute devices across
    vendors (Phase 3 Stage 2). CPU is always present as the fallback; GPU
    devices are discovered by dlopen-ing each vendor runtime and querying it.

    dlopen-based on purpose: the app does NOT hard-link the vendor runtimes, so
    the binary launches on a CPU-only or single-vendor machine, and a runtime
    that is absent simply yields no devices for that vendor (graceful). The
    function pointer signatures are declared locally, so this header pulls in
    NO CUDA/HIP/Metal SDK headers.

    Device id strings are stable ("cpu", "hip:0", "cuda:1", "metal:0") and are
    what the per-role ValueTree params (Stage 4) store and the factory
    (Stage 3) dispatches on.
*/

#include <string>
#include <vector>
#include <cstring>

#if ! defined(__APPLE__)
 #include <dlfcn.h>
#endif

struct GpuDevice
{
    enum class Vendor { CPU, HIP, CUDA, Metal };

    Vendor      vendor { Vendor::CPU };
    int         index  { -1 };          // ordinal within the vendor runtime (-1 for CPU)
    std::string name;                   // human-readable, for the UI
    std::string id;                     // stable selector, e.g. "hip:0"

    bool isCpu() const noexcept { return vendor == Vendor::CPU; }

    static const char* vendorLabel (Vendor v) noexcept
    {
        switch (v)
        {
            case Vendor::HIP:   return "HIP";
            case Vendor::CUDA:  return "CUDA";
            case Vendor::Metal: return "Metal";
            default:            return "CPU";
        }
    }
};

namespace wfsgpu_detail
{
#if ! defined(__APPLE__)
    // Minimal dlsym'd signatures (vendor C ABI; all return 0 on success).
    using FnInit    = int (*) (unsigned);
    using FnCount   = int (*) (int*);
    using FnDevGet  = int (*) (int*, int);
    using FnDevName = int (*) (char*, int, int);

    inline void enumerateVia (std::vector<GpuDevice>& out, GpuDevice::Vendor vendor,
                              const char* soA, const char* soB,
                              const char* initSym, const char* countSym,
                              const char* devGetSym, const char* devNameSym,
                              const char* idPrefix, const char* fallbackName)
    {
        void* lib = dlopen (soA, RTLD_LAZY | RTLD_LOCAL);
        if (lib == nullptr && soB != nullptr) lib = dlopen (soB, RTLD_LAZY | RTLD_LOCAL);
        if (lib == nullptr) return;                       // runtime absent -> no devices

        auto init    = initSym    ? (FnInit)    dlsym (lib, initSym)    : nullptr;
        auto count   =              (FnCount)   dlsym (lib, countSym);
        auto devGet  = devGetSym  ? (FnDevGet)  dlsym (lib, devGetSym)  : nullptr;
        auto devName = devNameSym ? (FnDevName) dlsym (lib, devNameSym) : nullptr;
        if (count == nullptr) return;

        if (init != nullptr) init (0);

        int n = 0;
        if (count (&n) != 0 || n <= 0) return;

        for (int i = 0; i < n; ++i)
        {
            int dev = i;
            if (devGet != nullptr) devGet (&dev, i);

            char buf[256]; std::memset (buf, 0, sizeof (buf));
            std::string nm = (std::string) fallbackName + " " + std::to_string (i);
            if (devName != nullptr && devName (buf, (int) sizeof (buf) - 1, dev) == 0 && buf[0] != '\0')
                nm = buf;

            out.push_back ({ vendor, i, nm, (std::string) idPrefix + std::to_string (i) });
        }
        // Deliberately keep the lib resident (do not dlclose).
    }
#endif
}

class GpuDeviceManager
{
public:
    static GpuDeviceManager& instance()
    {
        static GpuDeviceManager m;
        return m;
    }

    void refresh()
    {
        devs.clear();
        devs.push_back ({ GpuDevice::Vendor::CPU, -1, "CPU", "cpu" });

#if defined(__APPLE__)
        // Metal enumeration lives in a .mm (Obj-C) on macOS — added with the
        // Metal device path; for now the macOS build reports the default device.
        devs.push_back ({ GpuDevice::Vendor::Metal, 0, "Metal (default)", "metal:0" });
#else
        // AMD / HIP: runtime lib carries both count + name symbols.
        wfsgpu_detail::enumerateVia (devs, GpuDevice::Vendor::HIP,
                                     "libamdhip64.so.6", "libamdhip64.so",
                                     "hipInit", "hipGetDeviceCount",
                                     "hipDeviceGet", "hipDeviceGetName",
                                     "hip:", "AMD GPU");
        // NVIDIA / CUDA: enumerate via the DRIVER lib (only present with an
        // installed NVIDIA driver), so a toolkit-only box reports no CUDA GPU.
        wfsgpu_detail::enumerateVia (devs, GpuDevice::Vendor::CUDA,
                                     "libcuda.so.1", "libcuda.so",
                                     "cuInit", "cuDeviceGetCount",
                                     "cuDeviceGet", "cuDeviceGetName",
                                     "cuda:", "NVIDIA GPU");
#endif
    }

    const std::vector<GpuDevice>& devices() const noexcept { return devs; }

    const GpuDevice* find (const std::string& deviceId) const noexcept
    {
        for (const auto& d : devs)
            if (d.id == deviceId) return &d;
        return nullptr;
    }

    bool hasGpu() const noexcept
    {
        for (const auto& d : devs) if (! d.isCpu()) return true;
        return false;
    }

private:
    GpuDeviceManager() { refresh(); }
    std::vector<GpuDevice> devs;
};

#endif // WFS_GPU_NATIVE
