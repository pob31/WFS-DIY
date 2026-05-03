#pragma once

#include <JuceHeader.h>

namespace WFSTouch {

/**
    Persistent mapping between a physical touchscreen and a JUCE display.

    Keyed by udev sysfs path (e.g. "/sys/devices/.../input47") so the mapping
    survives replug to a different USB port and reboots — /dev/input/eventN
    can renumber, sysfs path cannot.

    Auto-mapping (v1) uses ABS_MT_POSITION_{X,Y} min/max from the kernel,
    transformed into the chosen display's userArea. Swap/flip toggles
    handle landscape/portrait/upside-down panels without a calibration UI.
*/
struct TouchDeviceMapping
{
    juce::String sysPath;        // udev syspath; stable identifier
    int          displayIndex = -1;  // -1 = disabled; otherwise index into Desktop::Display list
    bool         swapXY = false;
    bool         flipX  = false;
    bool         flipY  = false;

    bool isEnabled() const noexcept { return displayIndex >= 0; }

    juce::var toVar() const
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("sysPath",     sysPath);
        obj->setProperty ("displayIndex", displayIndex);
        obj->setProperty ("swapXY",      swapXY);
        obj->setProperty ("flipX",       flipX);
        obj->setProperty ("flipY",       flipY);
        return juce::var (obj);
    }

    static TouchDeviceMapping fromVar (const juce::var& v)
    {
        TouchDeviceMapping m;
        if (auto* obj = v.getDynamicObject())
        {
            m.sysPath      = obj->getProperty ("sysPath").toString();
            m.displayIndex = (int) obj->getProperty ("displayIndex");
            m.swapXY       = (bool) obj->getProperty ("swapXY");
            m.flipX        = (bool) obj->getProperty ("flipX");
            m.flipY        = (bool) obj->getProperty ("flipY");
        }
        return m;
    }
};

} // namespace WFSTouch
