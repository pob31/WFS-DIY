#pragma once

/**
 * StreamDeckDevice — Low-level USB HID driver for Elgato Stream Deck+
 *
 * Handles direct HID communication: opening/closing the device, sending
 * button/LCD images, receiving button presses, dial rotations, and touch events.
 * Runs a background thread for reading HID input reports and dispatches events
 * to the GUI thread via juce::MessageManager::callAsync.
 *
 * Usage:
 *   StreamDeckDevice device;
 *   device.onButtonPressed = [](int btn) { ... };
 *   device.onDialRotated = [](int dial, int dir) { ... };
 *   device.startMonitoring();  // begins hotplug detection + connection
 */

// Prevent HIDAPI from exporting symbols (we're embedding, not building a DLL)
#define HID_API_NO_EXPORT_DEFINE
#include "hidapi/hidapi.h"

#include <JuceHeader.h>

class StreamDeckDevice : private juce::Thread,
                         private juce::Timer
{
public:
    //==========================================================================
    // Constants
    //==========================================================================

    static constexpr unsigned short VENDOR_ID  = 0x0FD9;
    static constexpr unsigned short PRODUCT_ID_PLUS = 0x0084;

    static constexpr int NUM_BUTTONS    = 8;
    static constexpr int NUM_COLS       = 4;
    static constexpr int NUM_ROWS       = 2;
    static constexpr int NUM_DIALS      = 4;

    static constexpr int BUTTON_IMAGE_WIDTH  = 120;
    static constexpr int BUTTON_IMAGE_HEIGHT = 120;

    static constexpr int LCD_STRIP_WIDTH     = 800;
    static constexpr int LCD_STRIP_HEIGHT    = 100;
    static constexpr int LCD_ZONE_WIDTH      = 200;  // Per-dial zone
    static constexpr int LCD_NUM_ZONES       = 4;

    static constexpr int HID_PACKET_SIZE     = 1024;
    static constexpr int BUTTON_HEADER_SIZE  = 8;
    static constexpr int LCD_HEADER_SIZE     = 16;
    static constexpr int INPUT_REPORT_SIZE   = 512;
    static constexpr int FEATURE_REPORT_SIZE = 32;

    //==========================================================================
    // Construction / Destruction
    //==========================================================================

    StreamDeckDevice()
        : juce::Thread ("StreamDeckHID")
    {
        hid_init();
    }

    ~StreamDeckDevice() override
    {
        stopMonitoring();
        hid_exit();
    }

    //==========================================================================
    // Callbacks (set by owner, called on GUI thread)
    //==========================================================================

    std::function<void(int buttonIndex)>               onButtonPressed;
    std::function<void(int buttonIndex)>               onButtonReleased;
    std::function<void(int dialIndex, int direction)>  onDialRotated;   // direction: +1=CW, -1=CCW
    std::function<void(int dialIndex)>                 onDialPressed;
    std::function<void(int dialIndex)>                 onDialReleased;
    std::function<void(int x, int y)>                  onTouchStripTouched;
    std::function<void(bool connected)>                onConnectionChanged;

    //==========================================================================
    // Lifecycle
    //==========================================================================

    /** Start monitoring for device connection. Call once at app startup. */
    void startMonitoring()
    {
        startTimer (2000);  // Poll for device every 2 seconds
        tryConnect();
    }

    /** Stop monitoring and disconnect. Call at app shutdown. */
    void stopMonitoring()
    {
        stopTimer();
        disconnect();
    }

    /** Returns true if the device is currently connected and open. */
    bool isConnected() const { return deviceHandle != nullptr; }

    //==========================================================================
    // Output: Button Images
    //==========================================================================

    /** Send an image to a button (0-7). Image will be JPEG-encoded and sent via HID. */
    void setButtonImage (int buttonIndex, const juce::Image& image)
    {
        if (! isConnected() || buttonIndex < 0 || buttonIndex >= NUM_BUTTONS)
            return;

        // Scale to 120x120 if needed
        juce::Image scaled = image;
        if (image.getWidth() != BUTTON_IMAGE_WIDTH || image.getHeight() != BUTTON_IMAGE_HEIGHT)
        {
            scaled = juce::Image (juce::Image::RGB, BUTTON_IMAGE_WIDTH, BUTTON_IMAGE_HEIGHT, true);
            juce::Graphics g (scaled);
            g.drawImageWithin (image, 0, 0, BUTTON_IMAGE_WIDTH, BUTTON_IMAGE_HEIGHT,
                               juce::RectanglePlacement::centred);
        }

        auto jpegData = jpegEncode (scaled);
        sendButtonImageData (buttonIndex, jpegData);
    }

    /** Clear a single button to black. */
    void clearButton (int buttonIndex)
    {
        juce::Image black (juce::Image::RGB, BUTTON_IMAGE_WIDTH, BUTTON_IMAGE_HEIGHT, true);
        setButtonImage (buttonIndex, black);
    }

    /** Clear all buttons to black. */
    void clearAllButtons()
    {
        for (int i = 0; i < NUM_BUTTONS; ++i)
            clearButton (i);
    }

    //==========================================================================
    // Output: LCD Touchstrip
    //==========================================================================

    /** Send an image to a specific zone of the LCD strip (zone 0-3, each 200x100). */
    void setLcdZoneImage (int zoneIndex, const juce::Image& image)
    {
        if (! isConnected() || zoneIndex < 0 || zoneIndex >= LCD_NUM_ZONES)
            return;

        juce::Image scaled = image;
        if (image.getWidth() != LCD_ZONE_WIDTH || image.getHeight() != LCD_STRIP_HEIGHT)
        {
            scaled = juce::Image (juce::Image::RGB, LCD_ZONE_WIDTH, LCD_STRIP_HEIGHT, true);
            juce::Graphics g (scaled);
            g.drawImageWithin (image, 0, 0, LCD_ZONE_WIDTH, LCD_STRIP_HEIGHT,
                               juce::RectanglePlacement::centred);
        }

        int xOffset = zoneIndex * LCD_ZONE_WIDTH;
        auto jpegData = jpegEncode (scaled);
        sendLcdImageData (xOffset, LCD_ZONE_WIDTH, LCD_STRIP_HEIGHT, jpegData);
    }

    /** Send a full 800x100 image to the entire LCD strip. */
    void setLcdFullImage (const juce::Image& image)
    {
        if (! isConnected())
            return;

        juce::Image scaled = image;
        if (image.getWidth() != LCD_STRIP_WIDTH || image.getHeight() != LCD_STRIP_HEIGHT)
        {
            scaled = juce::Image (juce::Image::RGB, LCD_STRIP_WIDTH, LCD_STRIP_HEIGHT, true);
            juce::Graphics g (scaled);
            g.drawImageWithin (image, 0, 0, LCD_STRIP_WIDTH, LCD_STRIP_HEIGHT,
                               juce::RectanglePlacement::centred);
        }

        auto jpegData = jpegEncode (scaled);
        sendLcdImageData (0, LCD_STRIP_WIDTH, LCD_STRIP_HEIGHT, jpegData);
    }

    /** Clear the entire LCD strip to black. */
    void clearLcdStrip()
    {
        juce::Image black (juce::Image::RGB, LCD_STRIP_WIDTH, LCD_STRIP_HEIGHT, true);
        setLcdFullImage (black);
    }

    //==========================================================================
    // Output: Brightness
    //==========================================================================

    /** Set display brightness (0-100). 0 effectively puts the device to sleep. */
    void setBrightness (int percent)
    {
        if (! isConnected())
            return;

        uint8_t report[FEATURE_REPORT_SIZE] = {};
        report[0] = 0x03;
        report[1] = 0x08;
        report[2] = static_cast<uint8_t> (juce::jlimit (0, 100, percent));

        const juce::ScopedLock sl (writeLock);
        hid_send_feature_report (deviceHandle, report, FEATURE_REPORT_SIZE);
    }

    //==========================================================================
    // Device Info
    //==========================================================================

    /** Get the device serial number (empty string if not connected). */
    juce::String getSerialNumber() const
    {
        if (! isConnected())
            return {};

        uint8_t report[FEATURE_REPORT_SIZE] = {};
        report[0] = 0x03;
        report[1] = 0x06;

        const juce::ScopedLock sl (writeLock);
        int res = hid_get_feature_report (deviceHandle, report, FEATURE_REPORT_SIZE);
        if (res < 0) return {};

        // Serial starts at byte 2, null-terminated ASCII
        return juce::String (reinterpret_cast<const char*> (report + 2));
    }

    /** Get firmware version string (empty string if not connected). */
    juce::String getFirmwareVersion() const
    {
        if (! isConnected())
            return {};

        uint8_t report[FEATURE_REPORT_SIZE] = {};
        report[0] = 0x03;
        report[1] = 0x05;

        const juce::ScopedLock sl (writeLock);
        int res = hid_get_feature_report (deviceHandle, report, FEATURE_REPORT_SIZE);
        if (res < 0) return {};

        return juce::String (reinterpret_cast<const char*> (report + 2));
    }

private:
    //==========================================================================
    // Connection Management
    //==========================================================================

    void tryConnect()
    {
        if (isConnected())
            return;

        auto* devInfo = hid_enumerate (VENDOR_ID, PRODUCT_ID_PLUS);
        if (devInfo == nullptr)
            return;

        deviceHandle = hid_open_path (devInfo->path);
        hid_free_enumeration (devInfo);

        if (deviceHandle != nullptr)
        {
            hid_set_nonblocking (deviceHandle, 0);  // Blocking with timeout in read thread
            startThread (juce::Thread::Priority::normal);

            DBG ("StreamDeck+ connected: serial=" + getSerialNumber()
                 + " firmware=" + getFirmwareVersion());

            juce::MessageManager::callAsync ([this]()
            {
                if (onConnectionChanged)
                    onConnectionChanged (true);
            });
        }
    }

    void disconnect()
    {
        if (! isConnected())
            return;

        // Signal thread to stop and wait
        signalThreadShouldExit();
        if (isThreadRunning())
            waitForThreadToExit (1000);

        // Clear display before closing
        clearAllButtons();
        clearLcdStrip();

        {
            const juce::ScopedLock sl (writeLock);
            hid_close (deviceHandle);
            deviceHandle = nullptr;
        }

        DBG ("StreamDeck+ disconnected");

        juce::MessageManager::callAsync ([this]()
        {
            if (onConnectionChanged)
                onConnectionChanged (false);
        });
    }

    //==========================================================================
    // Timer: Hotplug Detection
    //==========================================================================

    void timerCallback() override
    {
        if (! isConnected())
            tryConnect();
    }

    //==========================================================================
    // Thread: HID Input Reader
    //==========================================================================

    void run() override
    {
        uint8_t buffer[INPUT_REPORT_SIZE];

        while (! threadShouldExit())
        {
            int bytesRead = hid_read_timeout (deviceHandle, buffer, INPUT_REPORT_SIZE, 50);

            if (bytesRead < 0)
            {
                // Device disconnected
                juce::MessageManager::callAsync ([this]()
                {
                    const juce::ScopedLock sl (writeLock);
                    if (deviceHandle != nullptr)
                    {
                        hid_close (deviceHandle);
                        deviceHandle = nullptr;
                    }

                    if (onConnectionChanged)
                        onConnectionChanged (false);

                    DBG ("StreamDeck+ disconnected (read error)");
                });
                return;
            }

            if (bytesRead == 0)
                continue;  // Timeout, no data

            parseInputReport (buffer, bytesRead);
        }
    }

    //==========================================================================
    // Input Report Parsing
    //==========================================================================

    void parseInputReport (const uint8_t* data, int length)
    {
        if (length < 4)
            return;

        uint8_t reportId = data[0];
        if (reportId != 0x01)
            return;

        uint8_t eventType = data[1];

        switch (eventType)
        {
            case 0x00: parseButtonEvent (data, length); break;
            case 0x02: parseTouchEvent (data, length);  break;
            case 0x03: parseDialEvent (data, length);   break;
            default: break;
        }
    }

    void parseButtonEvent (const uint8_t* data, int length)
    {
        if (length < 4 + NUM_BUTTONS)
            return;

        for (int i = 0; i < NUM_BUTTONS; ++i)
        {
            bool pressed = data[4 + i] != 0;
            bool wasPressed = buttonStates[i];

            if (pressed != wasPressed)
            {
                buttonStates[i] = pressed;

                juce::MessageManager::callAsync ([this, i, pressed]()
                {
                    if (pressed)
                    {
                        if (onButtonPressed)
                            onButtonPressed (i);
                    }
                    else
                    {
                        if (onButtonReleased)
                            onButtonReleased (i);
                    }
                });
            }
        }
    }

    void parseDialEvent (const uint8_t* data, int length)
    {
        if (length < 9)
            return;

        uint8_t actionType = data[4];  // 0x00 = press/release, 0x01 = rotation

        if (actionType == 0x01)  // Rotation
        {
            for (int i = 0; i < NUM_DIALS; ++i)
            {
                int8_t value = static_cast<int8_t> (data[5 + i]);
                if (value == 0)
                    continue;

                int direction = (value > 0) ? 1 : -1;
                juce::MessageManager::callAsync ([this, i, direction]()
                {
                    if (onDialRotated)
                        onDialRotated (i, direction);
                });
            }
        }
        else if (actionType == 0x00)  // Press/release: compare against previous state
        {
            for (int i = 0; i < NUM_DIALS; ++i)
            {
                uint8_t current = data[5 + i];
                uint8_t previous = previousDialPressState[i];
                previousDialPressState[i] = current;

                if (current == previous)
                    continue;

                bool pressed = (current != 0);
                juce::MessageManager::callAsync ([this, i, pressed]()
                {
                    if (pressed)
                    {
                        if (onDialPressed)
                            onDialPressed (i);
                    }
                    else
                    {
                        if (onDialReleased)
                            onDialReleased (i);
                    }
                });
            }
        }
    }

    void parseTouchEvent (const uint8_t* data, int length)
    {
        if (length < 10)
            return;

        int x = data[6] | (data[7] << 8);
        int y = data[8] | (data[9] << 8);

        juce::MessageManager::callAsync ([this, x, y]()
        {
            if (onTouchStripTouched)
                onTouchStripTouched (x, y);
        });
    }

    //==========================================================================
    // Image Sending Helpers
    //==========================================================================

    juce::MemoryBlock jpegEncode (const juce::Image& image) const
    {
        juce::MemoryOutputStream stream;
        juce::JPEGImageFormat jpegFormat;
        jpegFormat.setQuality (0.85f);
        jpegFormat.writeImageToStream (image, stream);
        return stream.getMemoryBlock();
    }

    void sendButtonImageData (int buttonIndex, const juce::MemoryBlock& jpegData)
    {
        const uint8_t* src = static_cast<const uint8_t*> (jpegData.getData());
        int remaining = static_cast<int> (jpegData.getSize());
        int offset = 0;
        int packetIndex = 0;
        const int maxPayload = HID_PACKET_SIZE - BUTTON_HEADER_SIZE;

        while (remaining > 0)
        {
            int chunkSize = juce::jmin (remaining, maxPayload);
            bool isLast = (remaining <= maxPayload);

            uint8_t packet[HID_PACKET_SIZE] = {};
            packet[0] = 0x02;
            packet[1] = 0x07;
            packet[2] = static_cast<uint8_t> (buttonIndex);
            packet[3] = isLast ? 0x01 : 0x00;
            packet[4] = static_cast<uint8_t> (chunkSize & 0xFF);
            packet[5] = static_cast<uint8_t> ((chunkSize >> 8) & 0xFF);
            packet[6] = static_cast<uint8_t> (packetIndex & 0xFF);
            packet[7] = static_cast<uint8_t> ((packetIndex >> 8) & 0xFF);

            std::memcpy (packet + BUTTON_HEADER_SIZE, src + offset, chunkSize);

            {
                const juce::ScopedLock sl (writeLock);
                if (deviceHandle != nullptr)
                    hid_write (deviceHandle, packet, HID_PACKET_SIZE);
            }

            offset += chunkSize;
            remaining -= chunkSize;
            ++packetIndex;
        }
    }

    void sendLcdImageData (int xOffset, int width, int height, const juce::MemoryBlock& jpegData)
    {
        const uint8_t* src = static_cast<const uint8_t*> (jpegData.getData());
        int remaining = static_cast<int> (jpegData.getSize());
        int offset = 0;
        int packetIndex = 0;
        const int maxPayload = HID_PACKET_SIZE - LCD_HEADER_SIZE;

        while (remaining > 0)
        {
            int chunkSize = juce::jmin (remaining, maxPayload);
            bool isLast = (remaining <= maxPayload);

            uint8_t packet[HID_PACKET_SIZE] = {};
            packet[0] = 0x02;
            packet[1] = 0x0C;
            packet[2] = static_cast<uint8_t> (xOffset & 0xFF);
            packet[3] = static_cast<uint8_t> ((xOffset >> 8) & 0xFF);
            // bytes 4-5 reserved (0x00)
            packet[6] = static_cast<uint8_t> (width & 0xFF);
            packet[7] = static_cast<uint8_t> ((width >> 8) & 0xFF);
            packet[8] = static_cast<uint8_t> (height & 0xFF);
            packet[9] = static_cast<uint8_t> ((height >> 8) & 0xFF);
            packet[10] = isLast ? 0x01 : 0x00;
            packet[11] = static_cast<uint8_t> (packetIndex & 0xFF);
            packet[12] = static_cast<uint8_t> ((packetIndex >> 8) & 0xFF);
            packet[13] = static_cast<uint8_t> (chunkSize & 0xFF);
            packet[14] = static_cast<uint8_t> ((chunkSize >> 8) & 0xFF);
            // byte 15 reserved (0x00)

            std::memcpy (packet + LCD_HEADER_SIZE, src + offset, chunkSize);

            {
                const juce::ScopedLock sl (writeLock);
                if (deviceHandle != nullptr)
                    hid_write (deviceHandle, packet, HID_PACKET_SIZE);
            }

            offset += chunkSize;
            remaining -= chunkSize;
            ++packetIndex;
        }
    }

    //==========================================================================
    // Member Data
    //==========================================================================

    hid_device* deviceHandle = nullptr;
    mutable juce::CriticalSection writeLock;
    bool buttonStates[NUM_BUTTONS] = {};
    uint8_t previousDialPressState[NUM_DIALS] = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StreamDeckDevice)
};
