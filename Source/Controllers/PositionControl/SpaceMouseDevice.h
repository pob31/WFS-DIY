#pragma once

/**
 * SpaceMouseDevice — HIDAPI driver for 3DConnexion SpaceMouse devices.
 *
 * Connects to any 3DConnexion device (VID 0x256F) and reads 6DOF input:
 *   - Translation X/Y/Z (Report ID 1, 3× int16 LE)
 *   - Rotation X/Y/Z   (Report ID 2, 3× int16 LE)
 *   - Buttons           (Report ID 3, bitmask)
 *
 * Values are normalized from the raw range (typ. -350..+350) to -1..+1.
 * Axes: 0=TransX, 1=TransY, 2=TransZ, 3=RotX, 4=RotY, 5=RotZ.
 * Buttons: 0 and 1 on the Compact model (left/right of the puck).
 *
 * Follows the same Thread + Timer + callAsync pattern as StreamDeckDevice.
 */

#define HID_API_NO_EXPORT_DEFINE
#include "hidapi/hidapi.h"

#if JUCE_WINDOWS
 #include <windows.h>
 #include <tlhelp32.h>
#endif

#include "ControllerDevice.h"

class SpaceMouseDevice : public ControllerDevice
{
public:
    //==========================================================================
    // Constants
    //==========================================================================

    static constexpr unsigned short VENDOR_ID_3DCONNEXION = 0x256F;

    static constexpr int NUM_AXES    = 6;   // TransX, TransY, TransZ, RotX, RotY, RotZ
    static constexpr int MAX_BUTTONS = 2;   // Compact has 2; others may have more

    static constexpr float RAW_MAX   = 350.0f;  // Typical max raw value
    static constexpr int   READ_TIMEOUT_MS = 20; // 50 Hz effective poll rate
    static constexpr int   REPORT_BUF_SIZE = 64;

    // Report IDs
    static constexpr uint8_t REPORT_TRANSLATION = 1;
    static constexpr uint8_t REPORT_ROTATION    = 2;
    static constexpr uint8_t REPORT_BUTTONS     = 3;

    //==========================================================================
    // Construction / Destruction
    //==========================================================================

    SpaceMouseDevice()
        : ControllerDevice ("SpaceMouseHID")
    {
    }

    ~SpaceMouseDevice() override
    {
        stopMonitoring();
        closeDevice();
    }

    //==========================================================================
    // ControllerDevice overrides — Device info
    //==========================================================================

    bool isConnected() const override           { return deviceHandle != nullptr; }
    juce::String getDeviceName() const override  { return connectedDeviceName; }
    int getDeviceId() const override             { return 1; } // Single SpaceMouse device ID

    int getNumAxes() const override    { return NUM_AXES; }
    int getNumButtons() const override { return MAX_BUTTONS; }

    juce::StringArray getAxisNames() const override
    {
        return { "TransX", "TransY", "TransZ", "RotX", "RotY", "RotZ" };
    }

    juce::StringArray getButtonNames() const override
    {
        return { "Left", "Right" };
    }

protected:
    //==========================================================================
    // ControllerDevice overrides — Connection
    //==========================================================================

    bool tryConnect() override
    {
        if (isConnected())
            return true;

        // Enumerate all 3DConnexion devices (any product ID)
        auto* devList = hid_enumerate (VENDOR_ID_3DCONNEXION, 0x0000);
        if (devList == nullptr)
        {
            DBG ("SpaceMouseHID: no 3DConnexion devices found");
            return false;
        }

        // Log all enumerated interfaces
        for (auto* cur = devList; cur != nullptr; cur = cur->next)
        {
            DBG ("SpaceMouseHID enumerate: PID=0x" + juce::String::toHexString (cur->product_id)
                 + " usage_page=0x" + juce::String::toHexString (cur->usage_page)
                 + " usage=0x" + juce::String::toHexString (cur->usage)
                 + " interface=" + juce::String (cur->interface_number)
                 + " product=" + juce::String (cur->product_string != nullptr ? cur->product_string : L"(null)"));
        }

        // Try each enumerated path until one opens successfully
        for (auto* cur = devList; cur != nullptr; cur = cur->next)
        {
            deviceHandle = hid_open_path (cur->path);
            if (deviceHandle != nullptr)
            {
                connectedDeviceName = (cur->product_string != nullptr)
                    ? juce::String (cur->product_string)
                    : juce::String ("SpaceMouse");

                hid_set_nonblocking (deviceHandle, 0);  // Blocking with timeout

                // Reset state
                std::memset (axisValues, 0, sizeof (axisValues));
                std::memset (buttonStates, 0, sizeof (buttonStates));

                DBG ("SpaceMouse connected: " + connectedDeviceName
                     + " (usage_page=0x" + juce::String::toHexString (cur->usage_page)
                     + " usage=0x" + juce::String::toHexString (cur->usage) + ")");

                setLED (true);

                hid_free_enumeration (devList);
                return true;
            }
        }

        DBG ("SpaceMouseHID: found devices but could not open any (driver conflict?)");
        hid_free_enumeration (devList);
        return false;
    }

    void disconnect() override
    {
        setLED (false);
        closeDevice();
    }

    /** Turn the SpaceMouse LED on or off (Report ID 0x04).
        Note: works on SpaceMouse Compact (USB). SpaceMouse Wireless
        accepts the command but the firmware ignores it — the ring LED
        on that model is purely firmware-controlled. */
    void setLED (bool on)
    {
        if (deviceHandle == nullptr)
            return;

        uint8_t report[2] = { 0x04, static_cast<uint8_t> (on ? 0x01 : 0x00) };
        hid_write (deviceHandle, report, 2);
    }

    bool poll() override
    {
        if (! isConnected())
            return false;

        uint8_t buffer[REPORT_BUF_SIZE];
        int bytesRead = hid_read_timeout (deviceHandle, buffer, REPORT_BUF_SIZE, READ_TIMEOUT_MS);

        if (bytesRead < 0)
        {
            // Device disconnected or error
            closeDevice();
            return false;
        }

        if (bytesRead == 0)
            return true;  // Timeout, no data — keep polling

        parseReport (buffer, bytesRead);
        return true;
    }

private:
    //==========================================================================
    // Report Parsing
    //==========================================================================

    void parseReport (const uint8_t* data, int length)
    {
        if (length < 1)
            return;

        uint8_t reportId = data[0];

        // Log unexpected or first-seen report IDs
        if (reportId != REPORT_TRANSLATION && reportId != REPORT_BUTTONS)
        {
            juce::String hex;
            for (int i = 0; i < juce::jmin (length, 14); ++i)
                hex += juce::String::toHexString (data[i]).paddedLeft ('0', 2) + " ";
            DBG ("SpaceMouseHID report ID=" + juce::String (reportId)
                 + " len=" + juce::String (length) + " data: " + hex.trim());
        }

        switch (reportId)
        {
            case REPORT_TRANSLATION:
                if (length >= 13)  // Combined report: 1 ID + 12 bytes (6× int16)
                {
                    parseAxes (data + 1, 0);  // Axes 0-2 (translation)
                    parseAxes (data + 7, 3);  // Axes 3-5 (rotation)
                }
                else if (length >= 7)  // Split report: 1 ID + 6 bytes (3× int16)
                {
                    parseAxes (data + 1, 0);  // Axes 0-2 only
                }
                break;

            case REPORT_ROTATION:
                if (length >= 7)
                    parseAxes (data + 1, 3);  // Axes 3-5 (split report mode)
                break;

            case REPORT_BUTTONS:
                if (length >= 2)
                    parseButtons (data + 1, length - 1);
                break;

            default:
                break;
        }
    }

    void parseAxes (const uint8_t* data, int axisOffset)
    {
        const int deviceId = getDeviceId();

        for (int i = 0; i < 3; ++i)
        {
            // Little-endian int16
            int16_t raw = static_cast<int16_t> (data[i * 2] | (data[i * 2 + 1] << 8));

            // Normalize to -1..+1
            float normalized = juce::jlimit (-1.0f, 1.0f, static_cast<float> (raw) / RAW_MAX);

            int axisIndex = axisOffset + i;

            if (std::abs (normalized - axisValues[axisIndex]) > 0.001f)
            {
                axisValues[axisIndex] = normalized;
                dispatchEvent ({ ControllerEvent::AxisMoved, deviceId, axisIndex, normalized, connectedDeviceName });
            }
        }
    }

    void parseButtons (const uint8_t* data, int length)
    {
        if (length < 1)
            return;

        const int deviceId = getDeviceId();
        uint8_t buttonBits = data[0];

        for (int i = 0; i < MAX_BUTTONS; ++i)
        {
            bool pressed = (buttonBits & (1 << i)) != 0;

            if (pressed != buttonStates[i])
            {
                buttonStates[i] = pressed;
                dispatchEvent ({
                    pressed ? ControllerEvent::ButtonPressed : ControllerEvent::ButtonReleased,
                    deviceId, i,
                    pressed ? 1.0f : 0.0f,
                    connectedDeviceName
                });
            }
        }
    }

    //==========================================================================
    // Cleanup
    //==========================================================================

    void closeDevice()
    {
        if (deviceHandle != nullptr)
        {
            hid_close (deviceHandle);
            deviceHandle = nullptr;
            DBG ("SpaceMouse disconnected");
        }
    }

    //==========================================================================
    // State
    //==========================================================================

    hid_device* deviceHandle = nullptr;
    juce::String connectedDeviceName;
    float axisValues[NUM_AXES] = {};
    bool  buttonStates[MAX_BUTTONS] = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpaceMouseDevice)

public:
    //==========================================================================
    // 3DxWare Driver Conflict Utilities (Windows + macOS)
    //==========================================================================

#if JUCE_WINDOWS
    /** Check if any 3DxWare driver processes are running. */
    static bool is3DxWareRunning()
    {
        return ! find3DxWareProcesses().isEmpty();
    }

    /** Kill all running 3DxWare driver processes.
        Returns true if all were successfully terminated. */
    static bool kill3DxWareProcesses()
    {
        auto processes = find3DxWareProcesses();
        bool allKilled = true;

        for (auto& name : processes)
        {
            juce::ChildProcess cp;
            if (cp.start ("taskkill /F /IM \"" + name + "\""))
            {
                cp.waitForProcessToFinish (3000);
                DBG ("Killed 3DxWare process: " + name);
            }
            else
            {
                DBG ("Failed to kill 3DxWare process: " + name);
                allKilled = false;
            }
        }

        return allKilled;
    }

private:
    static juce::StringArray find3DxWareProcesses()
    {
        juce::StringArray found;

        // Known 3DxWare process names
        static const char* processNames[] = {
            "3DxService.exe",
            "3DxWinCore64.exe",
            "3DxWinCore.exe",
            "3DxSmartFocus.exe",
            "3Dconnexion.exe"
        };

        HANDLE snapshot = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE)
            return found;

        PROCESSENTRY32 entry;
        entry.dwSize = sizeof (PROCESSENTRY32);

        if (Process32First (snapshot, &entry))
        {
            do
            {
                juce::String exeName (entry.szExeFile);
                for (auto& target : processNames)
                {
                    if (exeName.equalsIgnoreCase (target))
                    {
                        found.addIfNotAlreadyThere (exeName);
                        break;
                    }
                }
            }
            while (Process32Next (snapshot, &entry));
        }

        CloseHandle (snapshot);
        return found;
    }
#elif JUCE_MAC
    /** Check if any 3DxWare driver processes are running (macOS). */
    static bool is3DxWareRunning()
    {
        juce::ChildProcess cp;
        if (cp.start ("pgrep -x 3DconnexionHelper"))
        {
            cp.waitForProcessToFinish (2000);
            auto output = cp.readAllProcessOutput().trim();
            return output.isNotEmpty();
        }
        return false;
    }

    /** Kill 3DxWare driver processes and unload the launch agent (macOS).
        Returns true if successfully terminated. */
    static bool kill3DxWareProcesses()
    {
        bool success = true;

        // Unload the launch agent to prevent auto-restart
        {
            juce::ChildProcess cp;
            if (cp.start ("launchctl unload /Library/LaunchAgents/com.3dconnexion.3DconnexionHelper.plist"))
                cp.waitForProcessToFinish (3000);
            else
                success = false;
        }

        // Also try user-level agent
        {
            juce::ChildProcess cp;
            if (cp.start ("launchctl unload ~/Library/LaunchAgents/com.3dconnexion.3DconnexionHelper.plist"))
                cp.waitForProcessToFinish (3000);
        }

        // Kill any remaining processes
        {
            juce::ChildProcess cp;
            if (cp.start ("killall 3DconnexionHelper"))
            {
                cp.waitForProcessToFinish (3000);
                DBG ("Killed 3DconnexionHelper");
            }
        }

        return success;
    }
#else
    static bool is3DxWareRunning()   { return false; }
    static bool kill3DxWareProcesses() { return true; }
#endif
};
