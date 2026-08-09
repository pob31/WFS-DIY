/*
    Standalone smoke host for the wfs_headtrack plugin.

    Loads the plugin exactly the way the app does — dlopen, resolve by name,
    check the ABI version — and prints the pose stream. Links no OpenCV, so a
    successful run also proves the plugin is self-contained.

    This is the tool that validates the angle SIGNS (FaceTracker.h kSign*):
      shake "no"  → yaw   positive when you turn to YOUR right
      nod  "yes"  → pitch positive when you look UP
      tilt        → roll  positive when your RIGHT ear drops

    Usage:  test-headtrack-plugin [seconds] [cameraIndex]
    Exit codes (distinct, so CI can tell failures apart):
      0 ok · 2 library not found · 3 missing symbol · 4 ABI mismatch
      5 create failed · 6 start failed · 7 no poses received
*/

#include "HeadTrackPluginApi.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <thread>

#if defined(_WIN32)
  #include <windows.h>
  #define LIB_NAME "wfs_headtrack.dll"
  static void* openLib (const char* p)              { return (void*) LoadLibraryA (p); }
  static void* symbol  (void* h, const char* n)     { return (void*) GetProcAddress ((HMODULE) h, n); }
#else
  #include <dlfcn.h>
  #if defined(__APPLE__)
    #define LIB_NAME "libwfs_headtrack.dylib"
  #else
    #define LIB_NAME "libwfs_headtrack.so"
  #endif
  static void* openLib (const char* p)              { return dlopen (p, RTLD_NOW | RTLD_LOCAL); }
  static void* symbol  (void* h, const char* n)     { return dlsym (h, n); }
#endif

namespace
{
    std::atomic<int> poseCount { 0 };
    std::atomic<int> faceCount { 0 };

    void onPose (const WfsHeadtrackPose* pose, void*)
    {
        const int n = poseCount.fetch_add (1) + 1;
        if (pose->faceDetected)
            faceCount.fetch_add (1);

        // ~5 Hz of console output; the callback is in the latency path.
        if (n % 6 != 0)
            return;

        constexpr double kRadToDeg = 57.2957795;
        if (pose->faceDetected)
            std::printf ("\ryaw %+7.1f  pitch %+7.1f  roll %+7.1f   conf %.2f   n=%d   ",
                         pose->yawRad * kRadToDeg, pose->pitchRad * kRadToDeg,
                         pose->rollRad * kRadToDeg, pose->confidence, n);
        else
            std::printf ("\r(no face)                                        n=%d   ", n);
        std::fflush (stdout);
    }
}

int main (int argc, char** argv)
{
    const int seconds     = argc > 1 ? std::atoi (argv[1]) : 10;
    const int cameraIndex = argc > 2 ? std::atoi (argv[2]) : 0;

    void* lib = openLib (LIB_NAME);
    if (lib == nullptr)
    {
        std::fprintf (stderr, "FAIL: cannot load %s (is it beside this executable?)\n", LIB_NAME);
        return 2;
    }

    auto abiFn     = (int32_t (*)(void))              symbol (lib, "wfs_headtrack_abi_version");
    auto nameFn    = (const char* (*)(void))          symbol (lib, "wfs_headtrack_name");
    auto createFn  = (void* (*)(const WfsHeadtrackConfig*)) symbol (lib, "wfs_headtrack_create");
    auto startFn   = (int32_t (*)(void*, WfsHeadtrackPoseCb, void*)) symbol (lib, "wfs_headtrack_start");
    auto stopFn    = (void (*)(void*))                symbol (lib, "wfs_headtrack_stop");
    auto destroyFn = (void (*)(void*))                symbol (lib, "wfs_headtrack_destroy");
    auto errorFn   = (const char* (*)(void*))         symbol (lib, "wfs_headtrack_last_error");

    if (! abiFn || ! nameFn || ! createFn || ! startFn || ! stopFn || ! destroyFn || ! errorFn)
    {
        std::fprintf (stderr, "FAIL: missing export (are the symbols marked WFS_HEADTRACK_API?)\n");
        return 3;
    }

    const int32_t abi = abiFn();
    if (abi != WFS_HEADTRACK_ABI)
    {
        std::fprintf (stderr, "FAIL: ABI mismatch — plugin %d, host %d\n", abi, WFS_HEADTRACK_ABI);
        return 4;
    }
    std::printf ("plugin: %s (ABI %d)\n", nameFn(), abi);

    WfsHeadtrackConfig config {};
    config.structSize = sizeof (config);
    config.cameraIndex = cameraIndex;

    void* handle = createFn (&config);
    if (handle == nullptr)
    {
        std::fprintf (stderr, "FAIL: create — %s\n", errorFn (nullptr));
        return 5;
    }

    if (! startFn (handle, &onPose, nullptr))
    {
        std::fprintf (stderr, "FAIL: start — %s\n", errorFn (handle));
        destroyFn (handle);
        return 6;
    }

    std::printf ("running %d s — shake your head (yaw+ = your right), nod (pitch+ = up),\n"
                 "tilt (roll+ = right ear down)\n", seconds);
    std::this_thread::sleep_for (std::chrono::seconds (seconds));

    stopFn (handle);
    const int poses = poseCount.load();
    const int faces = faceCount.load();
    destroyFn (handle);

    std::printf ("\n%d poses in %d s (%.1f/s), %d with a face\n",
                 poses, seconds, poses / static_cast<double> (seconds), faces);

    if (poses == 0)
    {
        std::fprintf (stderr, "FAIL: no poses received\n");
        return 7;
    }

    std::printf ("OK\n");
    return 0;
}
