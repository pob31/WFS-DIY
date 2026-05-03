#pragma once

#include <JuceHeader.h>

#include "TouchDeviceMapping.h"

#include <vector>
#include <functional>

namespace WFSTouch {

#if ! defined (__linux__)

/**
    No-op TouchManager for macOS/Windows.

    Lets MainComponent and SystemConfigTab include the same header on every
    platform. On macOS and Windows the OS already delivers per-finger touch
    events through JUCE's normal MouseEvent pipeline, so no extra plumbing
    is needed here.
*/
class EvdevTouchManager
{
public:
    struct DeviceInfo {};
    using ChangeCallback = std::function<void()>;

    EvdevTouchManager() {}
    std::vector<DeviceInfo> getDetectedDevices() const { return {}; }
    void setMapping (const juce::String&, const TouchDeviceMapping&) {}
    int  addChangeListener (ChangeCallback) { return 0; }
    void removeChangeListener (int) {}
};

#endif

} // namespace WFSTouch
