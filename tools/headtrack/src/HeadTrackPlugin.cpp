/*
    wfs_headtrack — webcam head-tracking plugin for WFS-DIY.

    Implements the C ABI in spatcore/binaural/plugin/HeadTrackPluginApi.h: the
    app dlopens this library, and OpenCV (camera capture, the YuNet detector,
    the PnP solve) stays entirely on this side of the boundary.

    Threading: start() spawns ONE inference thread that owns the FaceTracker
    and invokes the app's callback — the ABI promises a single producer for the
    lifetime of a start()/stop() pair, which is what lets the app publish into
    its lock-free snapshot without synchronisation. Capture runs its own reader
    thread inside Capture (latest-frame mailbox, see Capture.h).

    Everything crossing the ABI is POD; the handle is opaque and only ever
    freed by wfs_headtrack_destroy.
*/

#include "../../../spatcore/binaural/plugin/HeadTrackPluginApi.h"
#include "Capture.h"
#include "FaceTracker.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <new>
#include <string>
#include <thread>

#if defined(_WIN32)
  #include <windows.h>
#else
  #include <dlfcn.h>
#endif

namespace
{

/** Resolve the model next to this shared library, so the plugin stays
    relocatable and needs no configuration. */
std::string modelPathBesideThisLibrary()
{
    const char* kModelName = "face_detection_yunet_2023mar.onnx";

#if defined(_WIN32)
    HMODULE self = nullptr;
    if (GetModuleHandleExA (GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
                                | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCSTR> (&modelPathBesideThisLibrary), &self))
    {
        char path[MAX_PATH] = {};
        if (GetModuleFileNameA (self, path, MAX_PATH) > 0)
        {
            std::string dir (path);
            const auto slash = dir.find_last_of ("\\/");
            if (slash != std::string::npos)
                return dir.substr (0, slash + 1) + kModelName;
        }
    }
#else
    Dl_info info {};
    if (dladdr (reinterpret_cast<void*> (&modelPathBesideThisLibrary), &info) != 0
        && info.dli_fname != nullptr)
    {
        std::string dir (info.dli_fname);
        const auto slash = dir.find_last_of ('/');
        if (slash != std::string::npos)
            return dir.substr (0, slash + 1) + kModelName;
    }
#endif

    return kModelName;   // last resort: current working directory
}

struct Instance
{
    wfs::headtrack::Capture capture;
    wfs::headtrack::FaceTracker tracker;

    std::thread worker;
    std::atomic<bool> running { false };

    WfsHeadtrackPoseCb callback = nullptr;
    void* userData = nullptr;

    int cameraIndex = 0, width = 640, height = 480, fps = 60;
    std::string lastError;

    void inferenceLoop()
    {
        cv::Mat frame;
        uint64_t seen = 0;
        const auto epoch = std::chrono::steady_clock::now();

        while (running.load (std::memory_order_acquire))
        {
            if (! capture.waitForFrame (frame, seen))
                break;

            const auto pose = tracker.process (frame);

            WfsHeadtrackPose out {};
            out.structSize = static_cast<uint32_t> (sizeof (WfsHeadtrackPose));
            out.timestampMs = std::chrono::duration<double, std::milli> (
                                  std::chrono::steady_clock::now() - epoch).count();
            out.yawRad = pose.yawRad;
            out.pitchRad = pose.pitchRad;
            out.rollRad = pose.rollRad;
            out.confidence = pose.confidence;
            out.faceDetected = pose.valid ? 1 : 0;

            // Deliver every frame, face or not: the "no face" packets are what
            // tell the app the tracker is alive but has nothing to report,
            // which is different from the tracker having died.
            if (auto cb = callback)
                cb (&out, userData);
        }
    }
};

std::string globalError = "";

} // namespace

//==============================================================================

int32_t wfs_headtrack_abi_version (void)
{
    return WFS_HEADTRACK_ABI;
}

const char* wfs_headtrack_name (void)
{
    return "Webcam (YuNet)";
}

void* wfs_headtrack_create (const WfsHeadtrackConfig* config)
{
    auto* instance = new (std::nothrow) Instance();
    if (instance == nullptr)
    {
        globalError = "Out of memory creating head tracker";
        return nullptr;
    }

    if (config != nullptr && config->structSize >= sizeof (WfsHeadtrackConfig))
    {
        instance->cameraIndex = config->cameraIndex;
        if (config->width  > 0) instance->width  = config->width;
        if (config->height > 0) instance->height = config->height;
        if (config->fps    > 0) instance->fps    = config->fps;
    }

    return instance;
}

int32_t wfs_headtrack_start (void* handle, WfsHeadtrackPoseCb cb, void* userData)
{
    auto* instance = static_cast<Instance*> (handle);
    if (instance == nullptr || cb == nullptr)
    {
        globalError = "wfs_headtrack_start called with a null handle or callback";
        return 0;
    }

    if (instance->running.load (std::memory_order_acquire))
        return 1;   // idempotent

    if (! instance->tracker.load (modelPathBesideThisLibrary(), instance->width, instance->height))
    {
        instance->lastError = instance->tracker.getLastError();
        return 0;
    }

    if (! instance->capture.start (instance->cameraIndex, instance->width,
                                   instance->height, instance->fps))
    {
        instance->lastError = instance->capture.getLastError();
        return 0;
    }

    instance->callback = cb;
    instance->userData = userData;
    instance->running.store (true, std::memory_order_release);
    instance->worker = std::thread ([instance] { instance->inferenceLoop(); });
    return 1;
}

void wfs_headtrack_stop (void* handle)
{
    auto* instance = static_cast<Instance*> (handle);
    if (instance == nullptr)
        return;

    instance->running.store (false, std::memory_order_release);
    instance->capture.stop();          // unblocks waitForFrame
    if (instance->worker.joinable())
        instance->worker.join();       // no callback can be in flight after this

    instance->callback = nullptr;
    instance->userData = nullptr;
}

void wfs_headtrack_destroy (void* handle)
{
    auto* instance = static_cast<Instance*> (handle);
    if (instance == nullptr)
        return;

    wfs_headtrack_stop (handle);
    delete instance;
}

const char* wfs_headtrack_last_error (void* handle)
{
    if (auto* instance = static_cast<Instance*> (handle))
        return instance->lastError.c_str();
    return globalError.c_str();
}
