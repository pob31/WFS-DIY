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
            return false;

        // Take the first available device
        deviceHandle = hid_open_path (devList->path);

        if (deviceHandle != nullptr)
        {
            // Store the device name
            if (devList->product_string != nullptr)
            {
                connectedDeviceName = juce::String (devList->product_string);
            }
            else
            {
                connectedDeviceName = "SpaceMouse";
            }

            hid_set_nonblocking (deviceHandle, 0);  // Blocking with timeout

            // Reset state
            std::memset (axisValues, 0, sizeof (axisValues));
            std::memset (buttonStates, 0, sizeof (buttonStates));

            DBG ("SpaceMouse connected: " + connectedDeviceName);
        }

        hid_free_enumeration (devList);
        return deviceHandle != nullptr;
    }

    void disconnect() override
    {
        closeDevice();
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

        switch (reportId)
        {
            case REPORT_TRANSLATION:
                if (length >= 7)  // 1 byte ID + 6 bytes (3× int16)
                    parseAxes (data + 1, 0);  // Axes 0-2
                break;

            case REPORT_ROTATION:
                if (length >= 7)
                    parseAxes (data + 1, 3);  // Axes 3-5
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
};
