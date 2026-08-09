#pragma once

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

namespace wfs::headtrack
{

/**
    Camera capture with a latest-frame mailbox.

    The whole point is latency: cv::VideoCapture::read() blocks, and any queue
    between capture and inference turns into accumulated delay the moment
    inference is slower than the frame rate. So the reader thread does nothing
    but overwrite a single slot, and the consumer always gets the newest frame —
    frames the consumer was too slow for are simply dropped, never queued.
*/
class Capture
{
public:
    ~Capture() { stop(); }

    /** Open the device. Returns false and fills lastError on failure. */
    bool start (int cameraIndex, int width, int height, int fps)
    {
        stop();

        // On Windows, DirectShow opens faster and buffers less than MSMF;
        // fall back to the platform default if it refuses the device.
#if defined(_WIN32)
        if (! cap.open (cameraIndex, cv::CAP_DSHOW))
            cap.open (cameraIndex);
#else
        cap.open (cameraIndex);
#endif
        if (! cap.isOpened())
        {
            lastError = "Could not open camera index " + std::to_string (cameraIndex)
                      + " (in use by another application, or no camera present)";
            return false;
        }

        // MJPG is what lets most UVC cameras reach 60 fps at 640x480 — raw
        // YUY2 is usually capped at 30. Order matters: fourcc before size.
        cap.set (cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc ('M', 'J', 'P', 'G'));
        cap.set (cv::CAP_PROP_FRAME_WIDTH,  width);
        cap.set (cv::CAP_PROP_FRAME_HEIGHT, height);
        cap.set (cv::CAP_PROP_FPS,          fps);
        cap.set (cv::CAP_PROP_BUFFERSIZE,   1);   // best-effort; the mailbox is the real defence

        cv::Mat probe;
        if (! cap.read (probe) || probe.empty())
        {
            lastError = "Camera opened but delivered no frames";
            cap.release();
            return false;
        }

        grantedWidth  = probe.cols;
        grantedHeight = probe.rows;
        grantedFps    = cap.get (cv::CAP_PROP_FPS);

        running.store (true, std::memory_order_release);
        {
            std::lock_guard<std::mutex> lock (mutex);
            probe.copyTo (latest);
            ++sequence;
        }
        reader = std::thread ([this] { readLoop(); });
        return true;
    }

    void stop()
    {
        if (! running.exchange (false, std::memory_order_acq_rel))
        {
            if (reader.joinable()) reader.join();
            return;
        }
        condition.notify_all();
        if (reader.joinable())
            reader.join();
        cap.release();
    }

    /**
        Block until a frame newer than `lastSeen` is available, then copy it
        out and update `lastSeen`. Returns false when capture stopped.
    */
    bool waitForFrame (cv::Mat& out, uint64_t& lastSeen)
    {
        std::unique_lock<std::mutex> lock (mutex);
        condition.wait (lock, [this, &lastSeen]
        {
            return ! running.load (std::memory_order_acquire) || sequence != lastSeen;
        });

        if (! running.load (std::memory_order_acquire))
            return false;

        latest.copyTo (out);
        lastSeen = sequence;
        return true;
    }

    const std::string& getLastError()  const { return lastError; }
    int    getGrantedWidth()           const { return grantedWidth; }
    int    getGrantedHeight()          const { return grantedHeight; }
    double getGrantedFps()             const { return grantedFps; }

private:
    void readLoop()
    {
        cv::Mat frame;
        while (running.load (std::memory_order_acquire))
        {
            if (! cap.read (frame) || frame.empty())
            {
                // Transient hiccup (USB re-enumeration, sleep/wake): back off
                // briefly rather than spinning. A permanently dead device just
                // stops producing, and the app's staleness timeout notices.
                std::this_thread::sleep_for (std::chrono::milliseconds (5));
                continue;
            }

            {
                std::lock_guard<std::mutex> lock (mutex);
                frame.copyTo (latest);   // overwrite: never queue
                ++sequence;
            }
            condition.notify_one();
        }
    }

    cv::VideoCapture cap;
    std::thread reader;
    std::atomic<bool> running { false };

    std::mutex mutex;
    std::condition_variable condition;
    cv::Mat latest;
    uint64_t sequence = 0;

    std::string lastError;
    int grantedWidth = 0, grantedHeight = 0;
    double grantedFps = 0.0;
};

} // namespace wfs::headtrack
