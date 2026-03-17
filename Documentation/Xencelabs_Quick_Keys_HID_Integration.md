# Xencelabs Quick Keys — HID Integration Notes

Reference for integrating the Xencelabs Quick Keys remote with WFS-DIY via HIDAPI.

## Device Identification

| Property | Wired | Wireless (dongle) |
|----------|-------|--------------------|
| Vendor ID | 0x28BD | 0x28BD |
| Product ID | 0x5202 | 0x5203 |
| HID Interface | usage_page 0xFF0A, usage 0x01, interface 2 | Same |

The device exposes **3 HID interfaces**:
- Interface 0: usage_page 0x01, usage 0x02 (generic pointer — ignore)
- Interface 1: usage_page 0x0D, usage 0x02 (digitizer — ignore)
- Interface 2: usage_page 0xFF0A, usage 0x01 (**vendor-specific — this is the one to open**)

Always enumerate and filter by `usage_page == 0xFF0A`. Opening the wrong interface causes immediate read errors.

## Important: Driver Exclusivity

The official Xencelabs driver app holds an exclusive HID handle. **It must be closed** before WFS-DIY can connect. Only one application can communicate with the device at a time.

## Input Reports (Device → Host)

Reports are **10 bytes**, blocking read with 50ms timeout.

### Button/Wheel Event (report[0]=0x02, report[1]=0xF0)

| Byte | Description |
|------|-------------|
| 0 | 0x02 (report ID) |
| 1 | 0xF0 (event command) |
| 2 | Side key bitmask low byte (keys 0-7, one bit each) |
| 3 | Special button bitmask high byte (bit 1 = wheel button press) |
| 4-6 | Reserved |
| 7 | Wheel rotation: 0x01 = CW/right, 0x02 = CCW/left, 0x00 = idle |
| 8-9 | Reserved |

**Button mapping (16-bit LE bitmask at bytes 2-3):**
- Bits 0-7 (byte 2): 8 side keys
- Bit 9 (byte 3, bit 1): wheel button press

### Battery Status (report[0]=0x02, report[1]=0xF2, wireless only)
- Byte 2: battery percentage

## Output Reports (Host → Device)

All output reports are **32 bytes**, report ID 0x02. For wireless devices, bytes 10-15 must contain the device ID discovered during connection.

### Set Key Text (Button OLED Labels)

The **primary and most reliable** display method. Each of the 8 keys (0-7) can show up to 8 characters. Text persists until explicitly changed.

```
report[0]  = 0x02
report[1]  = 0xB1
report[2]  = 0x00
report[3]  = keyIndex + 1  (1-based)
report[5]  = textLength * 2  (UTF-16LE byte count, max 16)
report[16..31] = UTF-16LE encoded text (padded to 16 bytes)
```

Keys 0-3 = top row (left to right), Keys 4-7 = bottom row (left to right).

### Show Overlay Text (Centered Temporary Text)

Displays centered text on the OLED for a specified duration. **Max 32 characters**, sent in chunks of 8.

```
report[0]  = 0x02
report[1]  = 0xB1
report[2]  = 0x05 (first chunk) or 0x06 (continuation)
report[3]  = duration in seconds (1-255, NOT 0 — 0 means "don't show")
report[4]  = 0x00
report[5]  = chunkLength * 2 (max 16)
report[6]  = 0x01 (more chunks follow) or 0x00 (last chunk)
report[16..31] = UTF-16LE encoded text (padded to 16 bytes)
```

**Known issues (observed on firmware as of March 2026):**
- **Chunks accumulate across calls.** If you send a 16-char overlay (2 chunks) then an 8-char overlay (1 chunk), the old chunk at position 2 remains visible. There is no clear/reset command.
- **Very slow refresh rate** (~2-3 FPS). Rapid overlay updates cause screen tearing.
- **Overlay overrides key text.** Sending key text after overlay may cancel the overlay display.
- The official Xencelabs driver uses overlay text only as a **brief transition effect** between button page changes, not for persistent display.
- **Duration 0 = not shown.** Always use duration >= 1.

**Recommendation:** Use `setKeyText` for persistent information display. Reserve overlay text for brief page-change transitions if needed.

### Set Wheel LED Ring Color

```
report[0]  = 0x02
report[1]  = 0xB4
report[2]  = 0x01
report[3]  = 0x01
report[6]  = R (0-255)
report[7]  = G (0-255)
report[8]  = B (0-255)
```

### Set Display Brightness

```
report[0]  = 0x02
report[1]  = 0xB1
report[2]  = 0x0A
report[3]  = 0x01
report[4]  = brightness (0=Off, 1=Low, 2=Medium, 3=Full)
```

### Set Display Orientation

```
report[0]  = 0x02
report[1]  = 0xB1
report[2]  = orientation (1=0deg, 2=90deg, 3=180deg, 4=270deg)
```

### Set Wheel Speed

Controls how many rotation events per physical detent. Lower value = faster.

```
report[0]  = 0x02
report[1]  = 0xB4
report[2]  = 0x04
report[3]  = 0x01
report[4]  = 0x01
report[5]  = speed (1=Fastest, 2=Faster, 3=Normal, 4=Slower, 5=Slowest)
```

### Set Sleep Timeout

```
report[0]  = 0x02
report[1]  = 0xB4
report[2]  = 0x08
report[3]  = 0x01
report[4]  = minutes (0-255, 0=never sleep)
```

### Subscribe to Events

Must be sent after opening the device, before reading input reports.

```
report[0]  = 0x02
report[1]  = 0xB0
report[2]  = 0x04
```

### Wireless Device Discovery

For wireless (PID 0x5203), send a discover command to get the device ID:

```
report[0]  = 0x02
report[1]  = 0xB8
```

Read the response (500ms timeout). Device ID is at bytes 9-14 (6 bytes). This ID must be written to bytes 10-15 of all subsequent output reports.

## Windows-Specific Gotchas

### hid_write blocks the calling thread

On Windows, HIDAPI's `hid_write` uses overlapped I/O with `WaitForSingleObject` (up to 1 second timeout). **Never call hid_write from the GUI thread** — it will freeze the UI.

**Solution:** Use a write queue. The GUI thread enqueues reports (just a memcpy), and the background HID read thread processes the queue between reads.

### Overlapped I/O errors

Sending reports too rapidly causes `ERROR_IO_PENDING` (0x3E5) — "Overlapped I/O operation is in progress." This happens when the previous write hasn't completed before the next one starts. The device hardware is slow (~2-3 FPS for display updates).

**Solution:** Minimize writes per interaction. Only update keys that actually changed. Avoid timer-driven continuous writes.

## Architecture (WFS-DIY Implementation)

### Single Background Thread

`XencelabsDevice` uses one `juce::Thread` for everything:
1. **Connection phase:** enumerate USB, open correct interface, subscribe to events
2. **Read/write phase:** process write queue, then `hid_read_timeout(50ms)`
3. **Auto-reconnect:** on read error, close handle and loop back to connection phase

No timers, no lambda threads. The GUI thread never blocks on HID operations.

### UX Model

- **Traverse mode** (default): Wheel rotates through a list of parameter bindings. LED ring dim (30%).
- **Adjust mode** (wheel button press): Wheel changes the focused parameter value. LED ring full brightness.
- **Wheel button**: Toggles between traverse and adjust modes.
- **8 side buttons**: Reserved for future assignment (page switching, toggles, etc.).
- **Wheel as scroll**: The device can also be used as a mouse scroll wheel over GUI sliders, though this requires mouse positioning which is slower in live situations.

### Display Layout (setKeyText)

```
Top row (keys 0-3):    [Tab]  [Section]  [Param]  [Value]
Bottom row (keys 4-7): [future button labels]
```

LED ring color matches the parameter's GUI slider active track color.

## Protocol References

- Node.js: https://github.com/Julusian/node-xencelabs-quick-keys
- Rust: https://github.com/nilp0inter/xencelabs-quick-keys-rs
