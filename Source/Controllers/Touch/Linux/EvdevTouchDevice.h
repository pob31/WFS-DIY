#pragma once

#if defined (__linux__)

#include <JuceHeader.h>

#include <array>
#include <atomic>
#include <functional>

namespace WFSTouch {

/**
    Owns a single /dev/input/eventN file descriptor, reads Multitouch
    protocol B events on a worker thread, and reports per-finger snapshots
    to the message thread on each SYN_REPORT.

    EVIOCGRAB takes exclusive ownership while the device is enabled — the
    Wayland compositor (and any other client) stops receiving events from
    this physical screen, which is what prevents the synthesized mouse
    stream from doubling up our injected touch events.

    Up to 16 simultaneous slots are tracked; ABS_MT_TRACKING_ID == -1
    marks a slot as released. Slot indices come straight from the kernel.
*/
class EvdevTouchDevice
{
public:
    static constexpr int kMaxSlots = 16;

    struct FingerState
    {
        int  trackingId = -1;     // kernel tracking id; -1 = inactive
        int  rawX = 0;
        int  rawY = 0;
        bool active = false;      // currently in contact
        bool pendingDown = false; // down event not yet dispatched
        bool pendingUp   = false; // up event not yet dispatched
        bool dirty = false;       // position changed since last dispatch
    };

    /** Per-axis range pulled from EVIOCGABS at open time. */
    struct AxisRange
    {
        int min = 0;
        int max = 1;
        bool isValid() const noexcept { return max > min; }
    };

    /** Snapshot delivered to the message thread on each SYN_REPORT. */
    struct Snapshot
    {
        std::array<FingerState, kMaxSlots> slots{};
    };

    using SnapshotCallback = std::function<void (const Snapshot&)>;

    EvdevTouchDevice (const juce::String& devNodePath,
                      const juce::String& sysPath,
                      const juce::String& displayName);
    ~EvdevTouchDevice();

    EvdevTouchDevice (const EvdevTouchDevice&) = delete;
    EvdevTouchDevice& operator= (const EvdevTouchDevice&) = delete;

    /** Returns true once the fd is open and the worker thread is running. */
    bool start (SnapshotCallback onSnapshot);

    /** Stops the worker, releases the EVIOCGRAB and closes the fd. */
    void stop();

    bool isRunning() const noexcept { return running.load (std::memory_order_acquire); }

    juce::String getDevNodePath()  const { return devNode; }
    juce::String getSysPath()      const { return sysPath; }
    juce::String getDisplayName()  const { return displayName; }
    AxisRange    getXRange()       const { return xRange; }
    AxisRange    getYRange()       const { return yRange; }
    juce::String getLastErrorMessage() const { return lastError; }

private:
    void runReadLoop();

    juce::String devNode;
    juce::String sysPath;
    juce::String displayName;
    juce::String lastError;

    AxisRange xRange;
    AxisRange yRange;

    int fd = -1;
    int wakePipe[2] = { -1, -1 };  // self-pipe to break the worker out of poll()

    std::atomic<bool> running { false };
    std::thread       worker;

    SnapshotCallback snapshotCallback;
};

} // namespace WFSTouch

#endif // JUCE_LINUX
