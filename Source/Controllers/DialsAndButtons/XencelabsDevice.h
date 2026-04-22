#pragma once

/**
 * XencelabsDevice — Low-level USB HID driver for Xencelabs Quick Keys
 *
 * Handles direct HID communication: opening/closing the device, sending
 * button text to OLED, LED ring color, receiving button presses and wheel
 * rotation events. Runs a background thread for reading HID input reports
 * and dispatches events to the GUI thread via juce::MessageManager::callAsync.
 *
 * Protocol reference: github.com/julusian/node-xencelabs-quick-keys
 *
 * Usage:
 *   XencelabsDevice device;
 *   device.onButtonPressed = [](int btn) { ... };
 *   device.onWheelRotated = [](int dir) { ... };
 *   device.startMonitoring();
 */

#define HID_API_NO_EXPORT_DEFINE
#include "hidapi/hidapi.h"

#include <JuceHeader.h>

class XencelabsDevice : private juce::Thread
{
public:
    //==========================================================================
    // Constants
    //==========================================================================

    static constexpr unsigned short VENDOR_ID       = 0x28BD;
    static constexpr unsigned short PID_WIRED       = 0x5202;
    static constexpr unsigned short PID_WIRELESS    = 0x5203;

    static constexpr int NUM_BUTTONS        = 10;   // 8 side keys + wheel press (bit 8) + spare (bit 9)
    static constexpr int MAX_KEY_TEXT_CHARS  = 8;    // Max chars per key OLED label
    static constexpr int MAX_OVERLAY_CHARS  = 32;   // Max chars for overlay text
    static constexpr unsigned short VENDOR_USAGE_PAGE = 0xFF0A;
    static constexpr int REPORT_SIZE        = 32;
    static constexpr int INPUT_REPORT_SIZE  = 32;

    /** Display orientation (matches device protocol values). */
    enum Orientation { Rotate0 = 1, Rotate90 = 2, Rotate180 = 3, Rotate270 = 4 };

    /** Display brightness levels. */
    enum Brightness { BrightnessOff = 0, BrightnessLow = 1, BrightnessMedium = 2, BrightnessFull = 3 };

    /** Wheel speed settings (note: lower value = faster). */
    enum WheelSpeed { Slowest = 5, Slower = 4, Normal = 3, Faster = 2, Fastest = 1 };

    //==========================================================================
    // Construction / Destruction
    //==========================================================================

    XencelabsDevice()
        : juce::Thread ("XencelabsHID"),
          alive (std::make_shared<bool> (true))
    {
        hid_init();
    }

    ~XencelabsDevice() override
    {
        *alive = false;
        signalThreadShouldExit();
        if (isThreadRunning())
            waitForThreadToExit (3000);
        disconnect();
        // Do NOT call hid_exit() here — see StreamDeckDevice::~StreamDeckDevice for
        // the rationale. hidapi's hid_exit is not reference-counted and tearing it
        // down from one device invalidates every other HID client still alive.
    }

    //==========================================================================
    // Callbacks (set by owner, called on GUI thread)
    //==========================================================================

    std::function<void (int buttonIndex)>    onButtonPressed;
    std::function<void (int buttonIndex)>    onButtonReleased;
    std::function<void (int direction)>      onWheelRotated;    // +1=CW/right, -1=CCW/left
    std::function<void (bool connected)>     onConnectionChanged;

    //==========================================================================
    // Lifecycle
    //==========================================================================

    void startMonitoring()
    {
        // Start the background thread which handles connection + reading.
        // No blocking HID calls on the GUI thread.
        if (! isThreadRunning())
            startThread (juce::Thread::Priority::normal);
    }

    void stopMonitoring()
    {
        signalThreadShouldExit();
        if (isThreadRunning())
            waitForThreadToExit (2000);
        disconnect();
    }

    bool isConnected() const { return deviceHandle != nullptr; }

    //==========================================================================
    // Output: Button Text (OLED labels)
    //==========================================================================

    /** Set the OLED text label for a button (0-7). Max 8 characters. */
    void setKeyText (int keyIndex, const juce::String& text)
    {
        if (! isConnected() || keyIndex < 0 || keyIndex >= 8)  // Only 8 side keys have OLED labels
            return;

        juce::String truncated = text.substring (0, MAX_KEY_TEXT_CHARS);
        auto utf16 = truncated.toUTF16();

        uint8_t report[REPORT_SIZE] = {};
        report[0] = 0x02;
        report[1] = 0xb1;
        report[3] = static_cast<uint8_t> (keyIndex + 1);
        report[5] = static_cast<uint8_t> (truncated.length() * 2);

        // Write UTF-16LE text starting at offset 16
        int offset = 16;
        for (auto cp = utf16.getAddress(); *cp != 0 && offset < REPORT_SIZE - 1; ++cp)
        {
            uint16_t ch = static_cast<uint16_t> (*cp);
            report[offset++] = static_cast<uint8_t> (ch & 0xFF);
            report[offset++] = static_cast<uint8_t> ((ch >> 8) & 0xFF);
        }

        writeDeviceId (report);
        sendReport (report);
    }

    /** Clear all button text labels. */
    void clearAllKeyText()
    {
        for (int i = 0; i < NUM_BUTTONS; ++i)
            setKeyText (i, "");
    }

    //==========================================================================
    // Output: Overlay Text (temporary full-screen text)
    //==========================================================================

    /** Show an overlay text on the OLED (max 32 chars, displayed for duration seconds).
        Duration must be 1-255; 0 is clamped to 1. */
    void showOverlayText (const juce::String& text, int durationSeconds = 2)
    {
        if (! isConnected())
            return;

        // Duration must be at least 1 (0 = not shown)
        uint8_t duration = static_cast<uint8_t> (juce::jlimit (1, 255, durationSeconds));

        juce::String truncated = text.substring (0, MAX_OVERLAY_CHARS);

        // Split into chunks of 8 chars
        int totalChars = truncated.length();
        int chunkIndex = 0;

        for (int pos = 0; pos < totalChars; pos += 8)
        {
            juce::String chunk = truncated.substring (pos, juce::jmin (pos + 8, totalChars));
            int chunkLen = chunk.length();

            uint8_t report[REPORT_SIZE] = {};
            report[0] = 0x02;
            report[1] = 0xb1;
            report[2] = (chunkIndex == 0) ? 0x05 : 0x06;
            report[3] = duration;
            report[4] = 0x00;
            report[5] = static_cast<uint8_t> (juce::jmin (chunkLen * 2, 16));
            report[6] = (pos + 8 < totalChars) ? 0x01 : 0x00;

            // UTF-16LE text at offset 16, padded to 16 bytes
            auto utf16 = chunk.toUTF16();
            int offset = 16;
            for (auto cp = utf16.getAddress(); *cp != 0 && offset < REPORT_SIZE - 1; ++cp)
            {
                uint16_t ch = static_cast<uint16_t> (*cp);
                report[offset++] = static_cast<uint8_t> (ch & 0xFF);
                report[offset++] = static_cast<uint8_t> ((ch >> 8) & 0xFF);
            }

            writeDeviceId (report);
            sendReport (report);
            ++chunkIndex;
        }
    }

    //==========================================================================
    // Output: Wheel LED Ring Color
    //==========================================================================

    /** Set the wheel LED ring color (RGB, 0-255 each). */
    void setWheelColor (uint8_t r, uint8_t g, uint8_t b)
    {
        if (! isConnected())
            return;

        uint8_t report[REPORT_SIZE] = {};
        report[0] = 0x02;
        report[1] = 0xb4;
        report[2] = 0x01;
        report[3] = 0x01;
        report[6] = r;
        report[7] = g;
        report[8] = b;

        writeDeviceId (report);
        sendReport (report);
    }

    //==========================================================================
    // Output: Display Settings
    //==========================================================================

    /** Set display brightness. */
    void setDisplayBrightness (Brightness brightness)
    {
        if (! isConnected())
            return;

        uint8_t report[REPORT_SIZE] = {};
        report[0] = 0x02;
        report[1] = 0xb1;
        report[2] = 0x0a;
        report[3] = 0x01;
        report[4] = static_cast<uint8_t> (brightness);

        writeDeviceId (report);
        sendReport (report);
    }

    /** Set display orientation. */
    void setDisplayOrientation (Orientation orientation)
    {
        if (! isConnected())
            return;

        uint8_t report[REPORT_SIZE] = {};
        report[0] = 0x02;
        report[1] = 0xb1;
        report[2] = static_cast<uint8_t> (orientation);

        writeDeviceId (report);
        sendReport (report);
    }

    /** Set wheel speed sensitivity. */
    void setWheelSpeed (WheelSpeed speed)
    {
        if (! isConnected())
            return;

        uint8_t report[REPORT_SIZE] = {};
        report[0] = 0x02;
        report[1] = 0xb4;
        report[2] = 0x04;
        report[3] = 0x01;
        report[4] = 0x01;
        report[5] = static_cast<uint8_t> (speed);

        writeDeviceId (report);
        sendReport (report);
    }

    /** Set sleep timeout in minutes (0 = never sleep). */
    void setSleepTimeout (int minutes)
    {
        if (! isConnected())
            return;

        uint8_t report[REPORT_SIZE] = {};
        report[0] = 0x02;
        report[1] = 0xb4;
        report[2] = 0x08;
        report[3] = 0x01;
        report[4] = static_cast<uint8_t> (juce::jlimit (0, 255, minutes));

        writeDeviceId (report);
        sendReport (report);
    }

private:
    //==========================================================================
    // Connection Management
    //==========================================================================

    /** Enumerate all HID interfaces for a given PID and open the vendor-specific one.
        The Quick Keys exposes multiple interfaces (keyboard, consumer, vendor).
        We need usage_page 0xFF0A for the control/event interface. */
    hid_device* openByUsagePage (unsigned short pid)
    {
        auto* devList = hid_enumerate (VENDOR_ID, pid);
        if (devList == nullptr)
            return nullptr;

        hid_device* handle = nullptr;

        for (auto* dev = devList; dev != nullptr; dev = dev->next)
        {
            if (dev->usage_page == VENDOR_USAGE_PAGE)
            {
                handle = hid_open_path (dev->path);
                if (handle != nullptr)
                    break;
            }
        }

        hid_free_enumeration (devList);
        return handle;
    }

    void tryConnect()
    {
        if (isConnected())
            return;

        // Try wired first, then wireless dongle.
        // The device exposes multiple HID interfaces — we need the vendor-specific
        // one (usage_page 0xFF0A) for control messages and event subscription.
        hid_device* handle = nullptr;
        bool wireless = false;

        handle = openByUsagePage (PID_WIRED);

        if (handle == nullptr)
        {
            handle = openByUsagePage (PID_WIRELESS);
            if (handle != nullptr)
                wireless = true;
        }

        if (handle == nullptr)
            return;

        deviceHandle = handle;
        isWireless = wireless;
        hid_set_nonblocking (deviceHandle, 0);

        // For wireless, discover the device ID
        if (isWireless)
            discoverWirelessDevice();

        // Subscribe to key/wheel events (direct write — we're on the background thread)
        subscribeToEvents();

        DBG ("XencelabsQuickKeys connected (" + juce::String (isWireless ? "wireless" : "wired") + ")");

        auto aliveFlag = alive;
        juce::MessageManager::callAsync ([this, aliveFlag]()
        {
            if (*aliveFlag && onConnectionChanged)
                onConnectionChanged (true);
        });
    }

    void disconnect()
    {
        if (! isConnected())
            return;

        {
            const juce::ScopedLock sl (writeLock);
            hid_close (deviceHandle);
            deviceHandle = nullptr;
        }

        memset (deviceIdBytes, 0, sizeof (deviceIdBytes));
        isWireless = false;
        writeQueueCount = 0;

        DBG ("XencelabsQuickKeys disconnected");
    }

    //==========================================================================
    // Protocol: Init / Subscribe
    //==========================================================================

    void discoverWirelessDevice()
    {
        // Send discover command to wireless dongle
        uint8_t report[REPORT_SIZE] = {};
        report[0] = 0x02;
        report[1] = 0xb8;
        sendReport (report);

        // Read response to get device ID
        uint8_t buffer[INPUT_REPORT_SIZE] = {};
        int bytesRead = hid_read_timeout (deviceHandle, buffer, INPUT_REPORT_SIZE, 500);
        if (bytesRead > 15)
        {
            // Device ID is at bytes 9-14 (6 bytes)
            memcpy (deviceIdBytes, buffer + 9, 6);
            hasDeviceId = true;
        }
    }

    void subscribeToEvents()
    {
        // Subscribe to key events
        uint8_t keyReport[REPORT_SIZE] = {};
        keyReport[0] = 0x02;
        keyReport[1] = 0xb0;
        keyReport[2] = 0x04;
        writeDeviceId (keyReport);
        sendReport (keyReport);
    }

    //==========================================================================
    // Thread: Connection + HID Read Loop
    //==========================================================================

    void run() override
    {
        uint8_t buffer[INPUT_REPORT_SIZE];

        while (! threadShouldExit())
        {
            // --- Connection phase: poll for device ---
            if (! isConnected())
            {
                tryConnect();

                if (! isConnected())
                {
                    // Wait 2 seconds before retrying
                    for (int i = 0; i < 40 && ! threadShouldExit(); ++i)
                        juce::Thread::sleep (50);
                    continue;
                }
            }

            // --- Read phase: process writes and read events ---
            processWriteQueue();

            int bytesRead = hid_read_timeout (deviceHandle, buffer, INPUT_REPORT_SIZE, 50);

            if (bytesRead < 0)
            {
                const wchar_t* err = hid_error (deviceHandle);
                juce::String errStr = (err != nullptr) ? juce::String (err) : "unknown";
                DBG ("XencelabsHID read error: " + errStr);

                // Close handle on this thread, notify GUI
                {
                    const juce::ScopedLock sl (writeLock);
                    if (deviceHandle != nullptr)
                    {
                        hid_close (deviceHandle);
                        deviceHandle = nullptr;
                    }
                }

                auto aliveFlag = alive;
                juce::MessageManager::callAsync ([this, aliveFlag]()
                {
                    if (*aliveFlag && onConnectionChanged)
                        onConnectionChanged (false);
                });

                DBG ("XencelabsQuickKeys disconnected (read error)");
                continue;  // Loop back to connection phase
            }

            if (bytesRead == 0)
                continue;

            parseInputReport (buffer, bytesRead);
        }
    }

    //==========================================================================
    // Input Report Parsing
    //==========================================================================

    void parseInputReport (const uint8_t* data, int length)
    {
        if (length < 2)
            return;

        // Report ID 0x02, command 0xf0 = button/wheel event
        if (data[0] == 0x02 && data[1] == 0xf0 && length >= 8)
        {
            parseButtonAndWheelEvent (data, length);
        }
        // Report ID 0x02, command 0xf2 = battery status (wireless only)
        else if (data[0] == 0x02 && data[1] == 0xf2 && length >= 3)
        {
            // Battery percent at data[2] — could expose if needed
        }
    }

    void parseButtonAndWheelEvent (const uint8_t* data, int /*length*/)
    {
        // Button states: 16-bit little-endian at offset 2-3
        // Byte 2 bits 0-7: side keys 0-7
        // Byte 3 bit 1 (= bit 9 overall): wheel button press
        uint16_t buttonBits = static_cast<uint16_t> (data[2]) | (static_cast<uint16_t> (data[3]) << 8);

        for (int i = 0; i < NUM_BUTTONS; ++i)
        {
            bool pressed = (buttonBits & (1 << i)) != 0;
            bool wasPressed = buttonStates[i];

            if (pressed != wasPressed)
            {
                buttonStates[i] = pressed;
                auto aliveFlag = alive;
                juce::MessageManager::callAsync ([this, aliveFlag, i, pressed]()
                {
                    if (! *aliveFlag)
                        return;
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

        // Wheel: byte at offset 7
        // 0x01 = clockwise/right, 0x02 = counter-clockwise/left
        uint8_t wheelByte = data[7];
        if (wheelByte == 0x01 || wheelByte == 0x02)
        {
            int direction = (wheelByte == 0x01) ? 1 : -1;
            auto aliveFlag = alive;
            juce::MessageManager::callAsync ([this, aliveFlag, direction]()
            {
                if (*aliveFlag && onWheelRotated)
                    onWheelRotated (direction);
            });
        }
    }

    //==========================================================================
    // Low-level HID Write
    //==========================================================================

    /** Queue a report for writing on the background thread.
        If called from the background thread itself (e.g., subscribeToEvents during
        tryConnect), writes directly. Otherwise queues for async processing. */
    void sendReport (const uint8_t* report)
    {
        const juce::ScopedLock sl (writeLock);

        if (getCurrentThreadId() == getThreadId())
        {
            // Already on the background thread — write directly
            if (deviceHandle != nullptr)
                hid_write (deviceHandle, report, REPORT_SIZE);
            return;
        }

        // Queue for background thread processing
        if (writeQueueCount < WRITE_QUEUE_SIZE)
        {
            memcpy (writeQueue[writeQueueCount].data, report, REPORT_SIZE);
            ++writeQueueCount;
        }
    }

    /** Process all queued writes — called from the background thread only. */
    void processWriteQueue()
    {
        const juce::ScopedLock sl (writeLock);

        for (int i = 0; i < writeQueueCount; ++i)
        {
            if (deviceHandle != nullptr)
                hid_write (deviceHandle, writeQueue[i].data, REPORT_SIZE);
        }

        writeQueueCount = 0;
    }

    /** Write the cached device ID into report bytes 10-15 (used for wireless routing). */
    void writeDeviceId (uint8_t* report) const
    {
        if (hasDeviceId)
            memcpy (report + 10, deviceIdBytes, 6);
    }

    //==========================================================================
    // Member Data
    //==========================================================================

    hid_device* deviceHandle = nullptr;
    juce::CriticalSection writeLock;
    std::shared_ptr<bool> alive;

    // Write queue: GUI thread enqueues, background thread dequeues and writes
    static constexpr int WRITE_QUEUE_SIZE = 16;
    struct WriteCmd { uint8_t data[REPORT_SIZE] = {}; };
    WriteCmd writeQueue[WRITE_QUEUE_SIZE];
    int writeQueueCount = 0;

    bool buttonStates[NUM_BUTTONS] = {};
    bool isWireless = false;
    bool hasDeviceId = false;
    uint8_t deviceIdBytes[6] = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XencelabsDevice)
};
