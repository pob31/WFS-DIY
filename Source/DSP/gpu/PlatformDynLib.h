#pragma once
#if WFS_GPU_NATIVE && ! defined(__APPLE__)

/*
    PlatformDynLib — tiny cross-platform shim over the OS dynamic-loader, so the
    GPU device enumeration (GpuDeviceManager) and the vendor-plugin factory
    (GpuBackendFactory) share one code path on Linux and Windows.

        Linux:   dlopen / dlsym / readlink("/proc/self/exe")   (libfoo.so)
        Windows: LoadLibraryA / GetProcAddress / GetModuleFileNameA   (foo.dll)

    macOS does not use this (its GPU path is the in-process Metal backend), hence
    the !__APPLE__ guard. No CUDA/HIP SDK headers are pulled in here.
*/

#include <string>

#if defined(_WIN32)
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
#else
 #include <dlfcn.h>
 #include <climits>
 #include <unistd.h>
#endif

namespace wfsdyn
{
#if defined(_WIN32)

    using LibHandle = HMODULE;

    inline LibHandle openLib (const char* name) noexcept { return ::LoadLibraryA (name); }
    inline void*     sym (LibHandle h, const char* s) noexcept { return (void*) ::GetProcAddress (h, s); }

    /** Directory containing the running executable (no trailing separator), or "". */
    inline std::string exeDir()
    {
        char buf[MAX_PATH];
        const DWORD n = ::GetModuleFileNameA (nullptr, buf, (DWORD) sizeof (buf));
        if (n == 0 || n >= sizeof (buf)) return {};
        std::string p (buf, n);
        const auto slash = p.find_last_of ("\\/");
        return slash == std::string::npos ? std::string {} : p.substr (0, slash);
    }

    inline const char* libPrefix() noexcept { return ""; }      // foo.dll
    inline const char* libExt()    noexcept { return ".dll"; }

#else  // POSIX (Linux)

    using LibHandle = void*;

    inline LibHandle openLib (const char* name) noexcept { return ::dlopen (name, RTLD_NOW | RTLD_LOCAL); }
    inline void*     sym (LibHandle h, const char* s) noexcept { return ::dlsym (h, s); }

    inline std::string exeDir()
    {
        char buf[PATH_MAX];
        const ssize_t n = ::readlink ("/proc/self/exe", buf, sizeof (buf) - 1);
        if (n <= 0) return {};
        buf[n] = '\0';
        std::string p (buf);
        const auto slash = p.find_last_of ('/');
        return slash == std::string::npos ? std::string {} : p.substr (0, slash);
    }

    inline const char* libPrefix() noexcept { return "lib"; }   // libfoo.so
    inline const char* libExt()    noexcept { return ".so"; }

#endif

    /** Our per-vendor plugin file name, e.g. "libwfs_hip.so" / "wfs_cuda.dll". */
    inline std::string pluginName (const char* vendor)
    {
        return (std::string) libPrefix() + "wfs_" + vendor + libExt();
    }
}

#endif // WFS_GPU_NATIVE && !__APPLE__
