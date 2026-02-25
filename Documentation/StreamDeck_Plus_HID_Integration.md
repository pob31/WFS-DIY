# Stream Deck+ Direct HID Integration — Reference Guide

> **Purpose**: Standalone reference for integrating Elgato Stream Deck+ hardware into any desktop application via direct USB HID. This document is project-agnostic and designed for reuse.

---

## 1. Integration Approach Comparison

Three main approaches exist for Stream Deck+ integration:

### 1.1 Official Elgato Stream Deck SDK
- **Language**: TypeScript/Node.js only (Node.js v20+)
- **Architecture**: Plugin runs as a Node.js child process inside the Stream Deck desktop application
- **Communication**: WebSocket API between plugin and Stream Deck app
- **Dial support**: Yes — rotate left/right events, press/release
- **Strengths**: Easiest development path; `streamdeck create` CLI scaffolding; hot-reload; built-in property inspector UI
- **Limitations**: Locked to the Elgato ecosystem; requires Stream Deck app running; no built-in support for OSC, MIDI, or other protocols; limited dynamic page management
- **Links**: [SDK GitHub](https://github.com/elgatosf/streamdeck) · [SDK Docs](https://docs.elgato.com/sdk)

### 1.2 Bitfocus Companion (Custom Module)
- **Language**: TypeScript/JavaScript modules
- **Architecture**: Standalone application that connects directly to Stream Deck hardware; custom modules define integrations
- **Dial support**: Full — rotate left/right, press, per-dial actions
- **Strengths**: Built-in OSC/DMX/HTTP/MQTT modules; feedbacks system for dynamic button state; variables for live data display; 700+ existing modules as reference; hot-reload during development; professional broadcast standard
- **Limitations**: Requires Companion application running alongside your app; module API learning curve
- **Best for**: Projects that already use OSC or network protocols; when you want a ready-made controller infrastructure
- **Links**: [Companion GitHub](https://github.com/bitfocus/companion) · [Module Dev Docs](https://companion.free/for-developers/module-development/home/) · [OSC Module](https://github.com/bitfocus/companion-module-generic-osc)

### 1.3 Direct HID (Chosen for Maximum Control)
- **Language**: Any — C, C++, Python, Node.js, Rust (via HIDAPI or platform-native APIs)
- **Architecture**: Your application talks directly to the Stream Deck via USB HID. No middleware.
- **Dial support**: Full raw access to all hardware features including LCD touchstrip
- **Strengths**: Maximum control and performance; no middleware dependencies; can embed into host application; works offline; any programming language
- **Trade-off**: You implement everything — device communication, image rendering, page management, hotplug handling
- **Best for**: Native desktop applications (C++/JUCE, Qt, etc.) where tight integration matters

---

## 2. Stream Deck+ Hardware Specifications

### 2.1 Physical Layout
```
┌─────────────────────────────────────┐
│  [Btn 0] [Btn 1] [Btn 2] [Btn 3]   │  ← Top row: 4 LCD buttons
│  [Btn 4] [Btn 5] [Btn 6] [Btn 7]   │  ← Bottom row: 4 LCD buttons
│                                     │
│  ┌─────────────────────────────┐    │
│  │   LCD Touchstrip 800×100    │    │  ← Capacitive touch LCD
│  └─────────────────────────────┘    │
│   (D0)    (D1)    (D2)    (D3)      │  ← 4 rotary encoder dials
└─────────────────────────────────────┘
```

### 2.2 Specifications

| Feature | Specification |
|---------|--------------|
| LCD Buttons | 8 keys, 120×120 pixels each, full-color JPEG |
| Button Layout | 2 rows × 4 columns |
| Rotary Dials | 4 encoders, 360° continuous rotation, with push-click |
| LCD Touchstrip | 800×100 pixels, capacitive touch, full-color JPEG |
| Connection | USB 2.0 (Type-C) |
| HID Class | Standard USB HID — no special driver required |

### 2.3 USB Identifiers

| Model | Product ID (PID) | Buttons | Dials | LCD Strip |
|-------|-------------------|---------|-------|-----------|
| **Stream Deck+** | **`0x0084`** | **8 (2×4)** | **4** | **800×100** |
| Stream Deck Mini | `0x0063` | 6 (2×3) | — | — |
| Stream Deck Mini (v2) | `0x0090` | 6 (2×3) | — | — |
| Stream Deck Original | `0x0060` | 15 (3×5) | — | — |
| Stream Deck Original (v2) | `0x006D` | 15 (3×5) | — | — |
| Stream Deck MK.2 | `0x0080` | 15 (3×5) | — | — |
| Stream Deck XL | `0x006C` | 32 (4×8) | — | — |
| Stream Deck XL (v2) | `0x008F` | 32 (4×8) | — | — |
| Stream Deck Pedal | `0x0086` | 3 foot pedals | — | — |
| Stream Deck Neo | `0x009A` | 8 (2×4) | — | Small info strip |

**Vendor ID (all models)**: `0x0FD9` (Elgato Systems GmbH)

---

## 3. Complete HID Protocol Reference

### 3.1 Overview

All communication uses standard USB HID reports:
- **Output reports** (Host → Device): Set button images, LCD strip images
- **Input reports** (Device → Host): Button presses, dial rotations, touch events
- **Feature reports** (Bidirectional): Brightness, serial number, firmware version

### 3.2 Button Image Protocol (Output Report)

Sends a JPEG image to one of the 8 LCD button displays. Large images are split across multiple 1024-byte packets.

**Packet structure** (1024 bytes total):

| Offset | Size | Name | Description |
|--------|------|------|-------------|
| 0 | 1 | Report ID | Always `0x02` |
| 1 | 1 | Command | Always `0x07` (set key image) |
| 2 | 1 | Key Index | Button 0–7 (top-left=0, bottom-right=7) |
| 3 | 1 | Is Last | `0x01` = final packet, `0x00` = more follow |
| 4–5 | 2 | Payload Length | JPEG data bytes in this packet (LE-16) |
| 6–7 | 2 | Packet Index | 0-based sequence number (LE-16) |
| 8–1023 | 1016 | Image Data | JPEG payload (zero-padded if short) |

**Image specifications**:
- Resolution: **120×120 pixels**
- Format: **JPEG** (baseline, no progressive)
- Color: RGB
- Max payload per packet: 1016 bytes (1024 − 8 header bytes)
- Button index mapping: `0–3` = top row L→R, `4–7` = bottom row L→R

**Example** — sending a small JPEG (< 1016 bytes) to button 3:
```
02 07 03 01 [len_lo] [len_hi] 00 00 [JPEG data...] [zero padding to 1024]
```

**Multi-packet example** — first packet of a larger image to button 0:
```
02 07 00 00 F8 03 00 00 [1016 bytes of JPEG data]
```

### 3.3 LCD Touchstrip Protocol (Output Report)

Sends JPEG image data to the 800×100 pixel LCD touchstrip. Supports partial updates via X-offset addressing for efficient per-dial zone updates.

**Packet structure** (1024 bytes total):

| Offset | Size | Name | Description |
|--------|------|------|-------------|
| 0 | 1 | Report ID | Always `0x02` |
| 1 | 1 | Command | Always `0x0C` (LCD strip image) |
| 2–3 | 2 | X Offset | Horizontal pixel offset (LE-16) |
| 4–5 | 2 | Reserved | `0x00 0x00` |
| 6–7 | 2 | Width | Image width in pixels (LE-16) |
| 8–9 | 2 | Height | Image height in pixels (LE-16) |
| 10 | 1 | Is Last | `0x01` = final packet, `0x00` = more follow |
| 11–12 | 2 | Packet Index | 0-based sequence number (LE-16) |
| 13–14 | 2 | Payload Length | JPEG data bytes in this packet (LE-16) |
| 15 | 1 | Reserved | `0x00` |
| 16–1023 | 1008 | Image Data | JPEG payload |

**Screen specifications**:
- Total resolution: **800×100 pixels**
- Format: **JPEG** (baseline)
- Partial update: Set X-offset and width to update a sub-region
- Suggested zones (one per dial): 4 × 200px wide at offsets 0, 200, 400, 600

**Example** — updating dial zone 2 (offset 400, width 200):
```
02 0C 90 01 00 00 C8 00 64 00 01 00 00 [len_lo] [len_hi] 00 [JPEG data...]
```
(0x0190 = 400, 0x00C8 = 200, 0x0064 = 100)

### 3.4 Input Events (Input Report)

The device sends 512-byte input reports. The report ID and second byte determine the event type.

#### 3.4.1 Button Press/Release Events

| Offset | Size | Name | Description |
|--------|------|------|-------------|
| 0 | 1 | Report ID | `0x01` |
| 1 | 1 | Event Type | `0x00` (button event) |
| 2 | 1 | Button Count | Number of buttons (8 for SD+) |
| 3 | 1 | Reserved | `0x00` |
| 4–11 | 8 | Button States | One byte per button: `0x01`=pressed, `0x00`=released |

All 8 button states are reported simultaneously (complete state snapshot).

#### 3.4.2 Dial/Knob Rotation and Press Events

| Offset | Size | Name | Description |
|--------|------|------|-------------|
| 0 | 1 | Report ID | `0x01` |
| 1 | 1 | Event Type | `0x03` (dial event) |
| 2 | 1 | Dial Count | Number of dials (4 for SD+) |
| 3 | 1 | Reserved | `0x00` |
| 4 | 1 | Action Type | `0x00`=press/release, `0x01`=rotation |
| 5 | 1 | Dial 0 Value | See below |
| 6 | 1 | Dial 1 Value | See below |
| 7 | 1 | Dial 2 Value | See below |
| 8 | 1 | Dial 3 Value | See below |

**Dial value encoding**:
- **Rotation** (action type = `0x01`): `0x01` = clockwise, `0xFF` = counter-clockwise (signed byte), `0x00` = no change
- **Press** (action type = `0x00`): `0x01` = pressed down, `0x00` = released

#### 3.4.3 Touchstrip Touch Events

| Offset | Size | Name | Description |
|--------|------|------|-------------|
| 0 | 1 | Report ID | `0x01` |
| 1 | 1 | Event Type | `0x02` (touch event) |
| 2–3 | 2 | Header | `0x0E 0x00` |
| 4 | 1 | Touch Count | Number of touch points |
| 5 | 1 | Unknown | `0x01` |
| 6–7 | 2 | X Coordinate | Touch X position (LE-16, 0–799) |
| 8–9 | 2 | Y Coordinate | Touch Y position (LE-16, 0–99) |

### 3.5 Feature Reports

Feature reports use `hid_send_feature_report()` / `hid_get_feature_report()`.

#### 3.5.1 Set Brightness

| Offset | Size | Name | Description |
|--------|------|------|-------------|
| 0 | 1 | Report ID | `0x03` |
| 1 | 1 | Command | `0x08` |
| 2 | 1 | Brightness | 0–100 (percentage). `0x00` = display off/sleep |
| 3–31 | 29 | Padding | `0x00` fill |

**Total size**: 32 bytes

#### 3.5.2 Get Serial Number

| Offset | Size | Name | Description |
|--------|------|------|-------------|
| 0 | 1 | Report ID | `0x03` |
| 1 | 1 | Command | `0x06` |
| 2–31 | 30 | Serial | ASCII null-terminated serial number string |

**Total size**: 32 bytes. Send as feature report with bytes 0–1 set, read response.

#### 3.5.3 Get Firmware Version

| Offset | Size | Name | Description |
|--------|------|------|-------------|
| 0 | 1 | Report ID | `0x03` |
| 1 | 1 | Command | `0x05` |
| 2–31 | 30 | Version | ASCII null-terminated version string |

**Total size**: 32 bytes

#### 3.5.4 Reset Device

Send a feature report with report ID `0x03`, command `0x02`, remaining bytes `0x00`. This resets all button images and LCD to default state.

---

## 4. HIDAPI Library Reference

[HIDAPI](https://github.com/libusb/hidapi) is the standard cross-platform C library for USB HID device communication. It provides a simple, consistent API across Windows, macOS, and Linux.

### 4.1 Platform Backends

| Platform | Backend | Source File | Link Dependencies |
|----------|---------|-------------|-------------------|
| Windows | Native `hid.dll` (SetupAPI) | `windows/hid.c` | `setupapi.lib` |
| macOS | IOHidManager | `mac/hid.c` | `IOKit.framework`, `CoreFoundation.framework` |
| Linux | hidraw | `linux/hid.c` | `libudev` |
| Linux (alt) | libusb | `libusb/hid.c` | `libusb-1.0` |

### 4.2 Integration Options

**Recommended: Single-source embed**
- Copy `hidapi/hidapi.h` (common header) + platform-specific `.c` file into your project
- Compile the `.c` file as part of your build
- No external library installation needed

**Alternatives**:
- Shared library (`.dll` / `.dylib` / `.so`) — install system-wide or bundle
- Package manager: vcpkg (`vcpkg install hidapi`) or Conan (`conan install hidapi/`)

### 4.3 Key API Reference

```c
#include "hidapi.h"

// Initialize library (call once at startup)
int hid_init(void);

// Enumerate connected HID devices
struct hid_device_info* hid_enumerate(unsigned short vendor_id, unsigned short product_id);
void hid_free_enumeration(struct hid_device_info* devs);

// Open device by VID/PID (first match) or by path
hid_device* hid_open(unsigned short vendor_id, unsigned short product_id, const wchar_t* serial_number);
hid_device* hid_open_path(const char* path);

// Close device
void hid_close(hid_device* dev);

// Write output report (returns bytes written, or -1 on error)
int hid_write(hid_device* dev, const unsigned char* data, size_t length);

// Read input report (blocking or with timeout)
int hid_read(hid_device* dev, unsigned char* data, size_t length);
int hid_read_timeout(hid_device* dev, unsigned char* data, size_t length, int milliseconds);

// Feature reports
int hid_send_feature_report(hid_device* dev, const unsigned char* data, size_t length);
int hid_get_feature_report(hid_device* dev, unsigned char* data, size_t length);

// Non-blocking mode
int hid_set_nonblocking(hid_device* dev, int nonblock);

// Error string
const wchar_t* hid_error(hid_device* dev);

// Cleanup (call once at shutdown)
int hid_exit(void);
```

### 4.4 Thread Safety

- Each `hid_device*` handle should be used from a **single thread** at a time
- If you need to read and write concurrently, either:
  - Use a mutex to serialize access, or
  - Open two handles to the same device (one for read, one for write)
- `hid_enumerate()` is safe to call from any thread

### 4.5 Windows Notes

- No special driver installation required — uses built-in Windows HID driver (`HidD_*` functions)
- Compile and link: `cl /c hid.c` + link with `setupapi.lib`
- In Visual Studio: add `setupapi.lib` to Linker → Input → Additional Dependencies
- `hid_read()` on Windows uses overlapped I/O internally for timeout support

### 4.6 macOS Notes

- No special driver or entitlements required for HID devices
- Compile: `cc -c hid.c -framework IOKit -framework CoreFoundation`
- The IOHidManager backend handles device hotplug notifications natively
- Code-signed applications can access HID devices without additional permissions

---

## 5. Reference Implementations

These open-source projects implement the Stream Deck HID protocol and serve as excellent code references:

### 5.1 node-elgato-stream-deck (Node.js/TypeScript)
- **Repository**: https://github.com/julusian/node-elgato-stream-deck
- **Status**: Most complete and actively maintained implementation
- **Latest**: v7.5.2 (January 2026)
- **Packages**: `@elgato-stream-deck/core`, `@elgato-stream-deck/node`, `@elgato-stream-deck/webhid`, `@elgato-stream-deck/tcp`
- **Key reference**: Protocol constants, multi-packet image splitting, device detection

### 5.2 python-elgato-streamdeck (Python)
- **Repository**: https://github.com/abcminiuser/python-elgato-streamdeck
- **Status**: Well-maintained, good protocol documentation
- **Install**: `pip install streamdeck`
- **Key reference**: Device class hierarchy, image format details, dial event parsing
- **Key files**: `src/StreamDeck/Devices/StreamDeckPlus.py`

### 5.3 streamdeckpp (C++)
- **Repository**: https://github.com/drepper/streamdeckpp
- **Status**: C++ native, uses `libhidapi-libusb`
- **Key reference**: C++ API design, image handling with ImageMagick
- **Note**: Button images at half-height when monochrome (add color to fix)

### 5.4 Reverse Engineering Documentation
- **Den Delimarsky — Stream Deck+**: https://den.dev/blog/reverse-engineer-stream-deck-plus/
  - Detailed byte-level protocol analysis for SD+, including LCD touchstrip and dial events
- **Den Delimarsky — Stream Deck Original**: https://den.dev/blog/reverse-engineering-stream-deck/
  - Foundation protocol that SD+ builds upon; brightness, serial, image commands

### 5.5 Official Documentation
- **Elgato HID Docs**: https://docs.elgato.com/streamdeck/hid/
  - Official (limited) protocol reference from Elgato

---

## 6. Architecture Pattern for Host App Integration

This section describes a recommended architecture for embedding Stream Deck+ control into a desktop application.

### 6.1 Component Overview

```
┌─────────────────────────────────────────────────┐
│                Host Application                  │
│                                                  │
│  ┌──────────────┐    ┌────────────────────┐      │
│  │  Controller   │    │   Page Registry    │      │
│  │   Manager     │◄──►│                    │      │
│  │              │    │  Page: Tab A        │      │
│  │  - tracks    │    │    Section 0..3     │      │
│  │    current   │    │      Buttons[4]     │      │
│  │    page      │    │      Dials[4]       │      │
│  │  - routes    │    │                    │      │
│  │    events    │    │  Page: Tab B        │      │
│  │              │    │    ...              │      │
│  └──────┬───────┘    └────────────────────┘      │
│         │                                        │
│  ┌──────▼───────┐    ┌────────────────────┐      │
│  │   Device      │    │    Renderer        │      │
│  │   Driver      │◄──►│                    │      │
│  │              │    │  - Button images    │      │
│  │  - HID I/O   │    │  - LCD strip zones  │      │
│  │  - Thread    │    │  - JPEG encoding    │      │
│  │  - Hotplug   │    │                    │      │
│  └──────┬───────┘    └────────────────────┘      │
│         │                                        │
└─────────┼────────────────────────────────────────┘
          │ USB HID
     ┌────▼────┐
     │ Stream  │
     │ Deck+   │
     └─────────┘
```

### 6.2 Device Driver Layer

**Responsibilities**: Open/close USB HID, send images, receive events, detect hotplug.

**Threading model**:
- Dedicated background thread runs `hid_read_timeout()` in a loop (e.g., 50ms timeout)
- On event received: parse report, dispatch callback to GUI/main thread
- Write operations (`hid_write`) can be called from any thread if protected by mutex
- Hotplug: periodically call `hid_enumerate()` on a timer (e.g., every 2 seconds)

**Lifecycle**:
1. `hid_init()` at app startup
2. `hid_enumerate(0x0FD9, 0x0084)` to find device
3. `hid_open()` to connect
4. Start read thread
5. On disconnect: `hid_read()` returns -1 → clean up, start hotplug polling
6. On reconnect: re-enumerate → re-open → re-send current page images
7. `hid_close()` + `hid_exit()` at app shutdown (clear display first)

### 6.3 Page/Binding System

Define a hierarchy: **Page → Section → Bindings**

A **Page** corresponds to a specific UI context (e.g., a tab + subtab combination).
A **Section** is selected by one of the top-row buttons and defines what the bottom row and dials do.

```
Page "Inputs > Parameters"
├── Section 0: "Input"        ← top button 0 (highlighted when active)
│   ├── Button 4: "Atten Law"  (toggle)
│   ├── Button 5: "Tracking"   (toggle)
│   ├── Button 6: "MaxSpeed"   (toggle)
│   ├── Button 7: "PathMode"   (toggle)
│   ├── Dial 0: Attenuation    (float, -92..0 dB, exp)
│   ├── Dial 1: Delay          (float, -100..100 ms, linear)
│   ├── Dial 2: Height Factor  (float, 0..1, linear)
│   └── Dial 3: Coord Mode     (combobox: XYZ/Cyl/Sph)
├── Section 1: "Position"
│   └── ...
├── Section 2: "Sound"
│   └── ...
└── Section 3: "Mutes"
    └── ...
```

**Binding types**:
- **Float/Int dial**: Rotation changes value by `step` per detent. LCD shows name + formatted value.
- **ComboBox dial**: Press opens selection overlay on LCD. Rotate to browse options. Press again to confirm.
- **Toggle button**: Press toggles state. Button image shows on/off indicator.
- **Momentary button**: Active while held. Image reflects pressed state.
- **Action button**: Single-fire on press (e.g., "Stop All").

### 6.4 Bidirectional Parameter Sync

```
 Dial rotation                    Parameter changed in GUI
      │                                    │
      ▼                                    ▼
 Manager routes              Listener detects change
 to DialBinding                        │
      │                                    ▼
      ▼                         Manager finds affected
 setValue(newVal)              dial on current page
      │                                    │
      ▼                                    ▼
 App parameter              Renderer redraws LCD zone
 updates                     with new value
      │                                    │
      ▼                                    ▼
 Listener fires ──────►    (Skip — same source)
 (use guard flag to prevent feedback loop)
```

**Key pattern**: Use a boolean guard (e.g., `isUpdatingFromController`) set before calling `setValue()` from a dial event. When the parameter change listener fires, check this guard and skip LCD updates if the change originated from the controller.

### 6.5 Image Rendering Pipeline

```
Parameter state change
        │
        ▼
Render to bitmap (e.g., 120×120 for button, 200×100 for LCD zone)
        │
        ▼
JPEG-encode the bitmap (baseline, ~85% quality)
        │
        ▼
Split into 1024-byte HID packets with headers
        │
        ▼
hid_write() each packet sequentially
```

**Optimization**: Only re-render and re-send images that actually changed. Cache the last-sent image hash per button/zone.

### 6.6 Hotplug and Disconnect Handling

1. **Read thread detects disconnect**: `hid_read()` returns -1 → set `connected = false`
2. **Clear all callbacks/state**: Notify manager that device is gone
3. **Start polling timer**: Every 2 seconds, call `hid_enumerate(0x0FD9, 0x0084)`
4. **Device found**: `hid_open()` → send brightness → re-render all buttons/LCD for current page
5. **Notify manager**: Device reconnected, full refresh

---

## 7. Quick Start Checklist

For a new project integrating Stream Deck+:

- [ ] Add HIDAPI source files to project (`hidapi.h` + platform `.c`)
- [ ] Link platform libraries (`setupapi.lib` / `IOKit.framework`)
- [ ] Create device driver class with background read thread
- [ ] Implement button image sending (JPEG → 1024-byte packets)
- [ ] Implement LCD strip zone rendering (JPEG → partial-update packets)
- [ ] Parse input reports: buttons (type `0x00`), dials (type `0x03`), touch (type `0x02`)
- [ ] Create page/binding data structures
- [ ] Create manager to route events to active page bindings
- [ ] Implement bidirectional sync with guard flag
- [ ] Add hotplug detection
- [ ] Handle graceful shutdown (clear display before exit)
