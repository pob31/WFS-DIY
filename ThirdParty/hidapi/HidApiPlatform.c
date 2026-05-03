/**
 * HIDAPI Platform Wrapper
 *
 * Includes the correct platform-specific HIDAPI implementation based on
 * the target OS. This avoids needing per-platform file settings in Projucer.
 */

// Prevent HIDAPI from using dllexport (we're embedding, not building a DLL)
#define HID_API_NO_EXPORT_DEFINE

#if defined(_WIN32)
    #include "windows/hid.c"
#elif defined(__APPLE__)
    #include "mac/hid.c"
#elif defined(__linux__)
    #include "linux/hid.c"
#else
    #error "Unsupported platform for HIDAPI"
#endif
