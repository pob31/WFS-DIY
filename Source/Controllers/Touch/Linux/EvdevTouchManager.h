#pragma once

#if defined (__linux__)

#include <JuceHeader.h>

#include "EvdevTouchDevice.h"
#include "../TouchDeviceMapping.h"

#include <atomic>
#include <map>
#include <memory>
#include <thread>
#include <vector>

struct udev;
struct udev_monitor;

namespace WFSTouch {

/**
    Top-level multi-touchscreen orchestrator (Linux only).

    - Discovers touchscreens via libudev (subsystem=input, ID_INPUT_TOUCHSCREEN=1)
      and watches for hotplug events on a dedicated thread.
    - Owns one EvdevTouchDevice per detected touchscreen; each runs its own
      blocking-read worker.
    - Allocates stable JUCE source indices per finger across all devices.
    - On each device snapshot, transforms raw axis values to screen pixels via
      the user's chosen Display, then routes to the right top-level window via
      Desktop::findComponentAt() at touch-down time. Subsequent move/up events
      stay attached to that window's ComponentPeer (drag-capture semantics).
    - Persists per-device mappings to the supplied PropertiesFile keyed by
      udev sysfs path so config survives replug and reboot.

    Lifetime: owned by MainComponent on Linux only; never instantiated on
    macOS or Windows.
*/
class EvdevTouchManager
{
public:
    EvdevTouchManager();
    ~EvdevTouchManager();

    EvdevTouchManager (const EvdevTouchManager&) = delete;
    EvdevTouchManager& operator= (const EvdevTouchManager&) = delete;

    /** Snapshot of one detected touchscreen as exposed to the settings UI. */
    struct DeviceInfo
    {
        juce::String sysPath;
        juce::String devNode;          // /dev/input/eventN
        juce::String displayName;       // vendor + model
        EvdevTouchDevice::AxisRange xRange;
        EvdevTouchDevice::AxisRange yRange;
        bool         isOpen   = false;  // device file successfully opened + grabbed
        bool         hasError = false;
        juce::String errorMessage;
        TouchDeviceMapping mapping;
    };

    /** Returns the currently-detected devices in deterministic order. */
    std::vector<DeviceInfo> getDetectedDevices() const;

    /** Updates the mapping for a sysPath; (re)opens or closes the device as needed. */
    void setMapping (const juce::String& sysPath, const TouchDeviceMapping& mapping);

    /** Listener invoked on the message thread when the device list or mappings change.
        Returns an opaque token used to remove the listener. */
    using ChangeCallback = std::function<void()>;
    int addChangeListener (ChangeCallback cb);
    void removeChangeListener (int token);

private:
    struct Entry
    {
        DeviceInfo info;
        std::unique_ptr<EvdevTouchDevice> device;  // non-null while opened/grabbed
    };

    // udev hotplug monitor thread loop
    void runUdevMonitor();

    // Initial enumeration of touchscreens at startup
    void scanInitial();

    // Library-thread → message-thread bouncers
    void scheduleRediscover();
    void rediscoverOnMessageThread();

    // Snapshot received from a device worker thread; bounce to message thread
    void onSnapshot (const juce::String& sysPath, EvdevTouchDevice::Snapshot snap);
    void dispatchSnapshot (const juce::String& sysPath, EvdevTouchDevice::Snapshot snap);

    // Touch index allocation (message thread only)
    int  acquireTouchIndex (const juce::String& sysPath, int slot, int trackingId);
    void releaseTouchIndex (const juce::String& sysPath, int slot);
    int  lookupTouchIndex  (const juce::String& sysPath, int slot) const;

    // Mapping <-> persistence
    void loadMappingsFromSettings();
    void saveMappingsToSettings();
    void applyMapping (Entry& e);

    // Helpers
    static juce::String readSysfsAttribute (const juce::String& sysPath, const juce::String& attr);
    static juce::String describeDevice (const juce::String& sysPath);

    std::unique_ptr<juce::PropertiesFile> settings;

    // Owned on the message thread
    std::vector<Entry>        entries;
    std::map<juce::String, TouchDeviceMapping> savedMappings;  // sysPath -> mapping

    // Touch index state. Key = (sysPath, slot). Indices 0..15 reserved for touch.
    static constexpr int kMaxTouchIndex = 15;
    struct FingerKey
    {
        juce::String sysPath;
        int slot = 0;
        bool operator< (const FingerKey& o) const noexcept
        {
            return sysPath == o.sysPath ? slot < o.slot : sysPath < o.sysPath;
        }
    };
    struct FingerState
    {
        int  touchIndex = -1;
        int  trackingId = -1;
        juce::WeakReference<juce::Component> topLevel;  // peer derived on dispatch
        bool inContact = false;
    };
    std::map<FingerKey, FingerState> fingers;
    std::array<bool, kMaxTouchIndex + 1> touchIndexInUse{};

    // udev background
    udev*         udevContext = nullptr;
    udev_monitor* udevMon     = nullptr;
    int           udevWakePipe[2] = { -1, -1 };
    std::atomic<bool> monitorRunning { false };
    std::thread       monitorThread;

    int nextListenerToken = 0;
    std::map<int, ChangeCallback> listeners;
    void fireChange();

    // Coalesce hotplug events
    std::atomic<bool> rediscoverPending { false };

    JUCE_DECLARE_WEAK_REFERENCEABLE (EvdevTouchManager)
};

} // namespace WFSTouch

#endif // JUCE_LINUX
