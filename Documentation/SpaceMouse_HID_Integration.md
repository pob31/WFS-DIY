# 3DConnexion SpaceMouse — HID Integration Reference

Standalone reference for integrating 3DConnexion SpaceMouse devices via HIDAPI,
independent of any framework. Covers device identification, HID report formats,
axis interpretation, and known gotchas discovered during development.

---

## Device Identification

| Field | Value |
|---|---|
| Vendor ID | `0x256F` (3DConnexion) |
| Product IDs | Vary by model — use `0x0000` with `hid_enumerate` to match any |
| Models tested | SpaceMouse Compact (combined report mode) |

The official 3DConnexion driver must be **closed** — it will claim exclusive HID
access on some interfaces. Without the driver, the device sends raw HID reports
directly.

---

## HID Interface Selection

3DConnexion devices may expose multiple HID interfaces. Enumerate all paths for
VID `0x256F` and try each with `hid_open_path()` until one succeeds. Use blocking
mode with timeout for the read loop:

```
hid_set_nonblocking(handle, 0);  // blocking mode
hid_read_timeout(handle, buffer, size, 20);  // 50 Hz effective poll rate
```

---

## HID Report Format

### Two report modes

Devices use one of two report modes. The mode is **model-dependent** — detect at
runtime by checking the report length.

#### Split mode (older/larger models)

Three separate report IDs:

| Report ID | Length | Content |
|---|---|---|
| 1 | 7 bytes | Translation: 1 byte ID + 3× int16 LE (TransX, TransY, TransZ) |
| 2 | 7 bytes | Rotation: 1 byte ID + 3× int16 LE (RotX, RotY, RotZ) |
| 3 | 2+ bytes | Buttons: 1 byte ID + bitmask |

#### Combined mode (SpaceMouse Compact, newer models)

Translation and rotation in a single report:

| Report ID | Length | Content |
|---|---|---|
| 1 | 13 bytes | All 6 axes: 1 byte ID + 6× int16 LE (TransXYZ then RotXYZ) |
| 3 | 2+ bytes | Buttons: 1 byte ID + bitmask |

**Detection logic:**
```
if (reportId == 1 && length >= 13)
    // Combined: parse 6 axes from bytes 1-12
else if (reportId == 1 && length >= 7)
    // Split: parse 3 translation axes from bytes 1-6
else if (reportId == 2 && length >= 7)
    // Split: parse 3 rotation axes from bytes 1-6
```

### Axis data format

Each axis is a signed 16-bit little-endian integer:

```
int16_t raw = data[offset] | (data[offset + 1] << 8);
float normalized = clamp(raw / 350.0f, -1.0f, 1.0f);
```

Typical raw range is -350 to +350 (model-dependent). Values represent deflection
magnitude, not absolute position — the puck is spring-loaded and returns to center.

### Axis indices

| Index | Axis | Physical gesture |
|---|---|---|
| 0 | TransX | Push puck right (+) / left (-) |
| 1 | TransY | Push puck away (+) / toward you (-) |
| 2 | TransZ | Push puck down (+) / lift up (-) |
| 3 | RotX | Tilt puck forward (+) / backward (-) |
| 4 | RotY | Tilt puck right (+) / left (-) |
| 5 | RotZ | Twist puck clockwise (+) / counter-clockwise (-) |

### Button data

Report ID 3, byte 1 is a bitmask:
- Bit 0: Left button (SpaceMouse Compact)
- Bit 1: Right button (SpaceMouse Compact)
- Other models may have more buttons in additional bytes.

### Heartbeat report

| Report ID | Length | Content |
|---|---|---|
| 23 (0x17) | 13 bytes | Device status/heartbeat — ignore |

Sent periodically when idle. Not documented officially. Safe to discard.

---

## Axis Mapping for WFS-DIY

The WFS coordinate system uses:
- **0° = audience (-Y), 90° = right (+X), 180° = upstage (+Y)**

Default SpaceMouse mapping:

| Axis | Action | Sensitivity | Inverted | Notes |
|---|---|---|---|---|
| TransX (0) | Move input +X (stage right) | 2.0 m/s | No | |
| TransY (1) | Move input +Y (upstage) | 2.0 m/s | **Yes** | Push away = +Y requires inversion |
| TransZ (2) | Move input Z (height) | 2.0 m/s | **Yes** | Push down in physical = down in scene |
| RotX (3) | Unmapped | — | — | |
| RotY (4) | Unmapped | — | — | |
| RotZ (5) | Rotate input orientation | 90 °/s | **Yes** | Clockwise twist = CW on map |
| Button 0 | Previous input | — | — | |
| Button 1 | Next input | — | — | |

### Velocity integration

Axis values represent deflection, not position. Integrate as velocity:

```
delta_meters = process(rawValue) × sensitivity × dt
```

Where `process()` applies dead zone (0.05), rescale, optional exponent curve,
and inversion. At 50 Hz, `dt = 0.02s`.

---

## Architecture Notes

### Thread model

- **Timer** (GUI thread, 2s interval): hotplug detection. Calls `tryConnect()`
  when device is not connected.
- **Thread** (background): continuous `hid_read_timeout` polling at 50 Hz.
  Dispatches events to GUI thread via `callAsync`.
- **Velocity timer** (GUI thread, 50 Hz): integrates cached axis values into
  position deltas, calls movement callbacks.

### Gotchas discovered during development

1. **Combined vs split reports**: The SpaceMouse Compact sends all 6 axes in
   Report ID 1 (13 bytes). Code that only handles split mode (ID 1 = translation,
   ID 2 = rotation) will miss rotation entirely. Always check report length.

2. **Thread not started on initial connect**: If `tryConnect()` succeeds during
   `startMonitoring()`, the polling thread must be started immediately. The
   timer-based reconnect path starts the thread, but the initial connect path
   may not — this causes the device to appear connected but produce no events.

3. **Driver conflict**: The official 3DConnexion driver intercepts HID reports
   and translates them to scroll/zoom events. If both the driver and direct HID
   access are active simultaneously, inputs are doubled (e.g., push-forward
   both moves an input AND zooms the map).

4. **Button double-fire**: Button events fire for both press and release. Action
   callbacks (prev/next input) should only trigger on `ButtonPressed`, not
   `ButtonReleased`.

5. **Y and Z axis inversion**: Physical "push away" on the SpaceMouse produces
   negative TransY in raw HID data, but maps to +Y (upstage) in WFS convention.
   TransZ is similarly inverted for height. RotZ twist direction also needed
   inversion to match expected CW/CCW on the map.

6. **Destructor ordering**: The base `ControllerDevice` destructor must NOT call
   virtual `disconnect()` — the derived class is already destroyed. Derived
   classes must call `stopMonitoring()` in their own destructor. The base
   destructor only handles thread/timer cleanup.

---

## References

- HIDAPI: https://github.com/libusb/hidapi
- 3DConnexion USB HID report format (community-documented)
- SpaceMouse Compact: VID 0x256F, 6DOF, 2 buttons, combined report mode
