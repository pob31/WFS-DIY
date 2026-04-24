# WFS-DIY Array Wizard ("Wizard of OutZ") — MCP Reference

Reference for the Output Array Helper window, used to lay out speaker arrays from presets. This document is a companion to the `WFS-UI_*.csv` files and is intended for consumption by the MCP generator.

**Source files**
- `Source/gui/OutputArrayHelperWindow.h` / `.cpp`
- `Source/Helpers/ArrayGeometryCalculator.h` / `.cpp`
- `Resources/lang/en.json` (section `arrayHelper`)

**Launch point** — button on the Outputs tab header labeled *"Wizard of OutZ…"*.

**Lifecycle** — one-shot dialog. Nothing in this window is persisted to the ValueTree until **Apply** is clicked; then the computed positions and acoustic defaults are written to the target Output channels. Closing the window discards unapplied edits.

**Orientation convention** — 0° = facing audience (−Y); 90° = +X (stage right); 180° = upstage (+Y); −90° = −X (stage left). Normalized to [−180°, +180°].

---

## 1. UI Controls

### Preset section
| Control | Type | Values | Default |
|---|---|---|---|
| Preset | ComboBox | Near Field Straight (1) ; Near Field Curved (2) ; Main Flown Straight (3) ; Sub Bass (4) ; Surround (5) ; Delay Line (6) ; Circle (7) | Near Field Straight |

Selecting a preset reloads **all** acoustic defaults and shows/hides the geometry controls that apply to that preset.

### Geometry section
Fields present depend on the selected preset.

| Control | Type | Unit | Default | Shown when |
|---|---|---|---|---|
| Method | Radio group (`centerSpacingRadio` / `endpointsRadio`) | — | Center + Spacing | preset supports both methods |
| Number of speakers | Integer editor | count | 2–12 (preset-dependent) | always |
| Z (height) | Float editor | m | 0.0 | always |
| Orientation | Float editor | ° | 0.0 | all presets except Circle and Surround |
| Center X | Float editor | m | preset-dependent (−4 to 0) | Method = Center + Spacing |
| Center Y | Float editor | m | preset-dependent (−0.5 to 0) | Method = Center + Spacing |
| Spacing | Float editor | m | preset-dependent (1–2) | Method = Center + Spacing |
| Start X | Float editor | m | preset-dependent (−8 to −4) | Method = Endpoints |
| Start Y | Float editor | m | preset-dependent (−0.5 to 0) | Method = Endpoints |
| End X | Float editor | m | preset-dependent (4 to 8) | Method = Endpoints |
| End Y | Float editor | m | preset-dependent (−0.5 to 0) | Method = Endpoints |
| Sag | Float editor (signed) | m | −1.0 | preset = Near Field Curved |
| Radius | Float editor | m | 5.0 | preset = Circle |
| Start Angle | Float editor | ° | 0.0 (top / −Y, clockwise) | preset = Circle |
| Facing | Radio group (Inward / Outward) | — | Inward | preset = Circle |
| Width | Float editor | m (± offset from center) | 8.0 | preset = Surround |
| Y Start | Float editor | m | 2.0 | preset = Surround |
| Y End | Float editor | m | 10.0 | preset = Surround |
| Direction | Radio group (Front Facing / Back Facing) | — | Front Facing (0°) | preset = Delay Line |

### Acoustic defaults section
Values here are loaded from the preset and written to the target Outputs on Apply.

| Control | Type | Range / Unit | Writes to Output paramId | Default varies per preset (see §2) |
|---|---|---|---|---|
| Live Source | Toggle | ON / OFF | `outputLSattenEnable` | ✓ |
| Floor Reflections | Toggle | ON / OFF | `outputFRenable` | ✓ |
| HF Damping | Float editor | −6 to 0 dB/m | `outputHFdamping` | ✓ |
| H Parallax | Float editor | 0 to 30 m | `outputHparallax` | ✓ |
| V Parallax | Float editor | −10 to +10 m | `outputVparallax` | ✓ |
| Distance Atten | Integer editor | 0 to 200 % | `outputDistanceAttenPercent` | ✓ |
| Low Cut enable | Toggle | ON / OFF | `outputEQshape` band 1 (shape=1 when ON) | ✓ |
| Low Cut freq | Integer editor | 20–20000 Hz | `outputEQfreq` band 1 | 80 Hz |
| High Cut enable | Toggle | ON / OFF | `outputEQshape` band 6 (shape=6 when ON) | ✓ |
| High Cut freq | Integer editor | 20–20000 Hz | `outputEQfreq` band 6 | 300 Hz |

### Target section
| Control | Type | Values | Default | Writes to |
|---|---|---|---|---|
| Array | ComboBox | Array 1 … Array 10 | 1 | `outputArray` |
| Start Output | ComboBox | 1 … (channel count from System Config) | 1 | starting index for the sequential write |

### Preview component
- Read-only canvas on the right of the dialog.
- Renders stage shape (Box / Cylinder / Dome) + origin crosshair + grid + calculated speaker positions (blue circles with orientation arrows) + contextual "Audience" label.
- Updates silently via `autoCalculatePreview()` whenever any editor changes.

### Buttons
| Button | Color | Action |
|---|---|---|
| Apply | green | `calculatePositions()` → validate → `applyToOutputs()` writes to ValueTree via UndoManager, auto-advances Array and Start Output selectors by N speakers, clears preview. Status label shows `"Applied {N} speakers to Array {A}. Ready for next array."` |
| Close | red | Dismisses the window immediately. No unsaved-changes prompt (wizard is ephemeral). |

### Status label
One of: `Ready` · `Calculated {N} positions` · `Applied {N} speakers to Array {A}. Ready for next array.` · `Error: {msg}`.

---

## 2. Preset Catalog

All values below are the **defaults written to each target Output channel** when Apply is clicked. Values are editable in the Acoustic Defaults section before Apply.

| # | Preset name | LS Atten | FR Enable | HF Damp | H Parallax | V Parallax | Dist Atten | Low Cut | High Cut | Default speakers | Geometry options |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Near Field Straight | ON | ON | −0.4 dB/m | 2.0 m | 0.5 m | 100 % | ON 80 Hz | OFF | 8 | Center+Spacing, Endpoints |
| 2 | Near Field Curved | ON | ON | −0.4 dB/m | 2.0 m | 0.5 m | 100 % | ON 80 Hz | OFF | 8 | Endpoints + Sag (default −1 m) |
| 3 | Main Flown Straight | OFF | OFF | −0.2 dB/m | 10.0 m | −4.0 m | 100 % | OFF | OFF | 8 | Center+Spacing, Endpoints |
| 4 | Sub Bass | OFF | OFF | 0.0 dB/m | 0.0 m | 0.0 m | 50 % if N ≤ 2, else 100 % | OFF | ON 300 Hz | 2 | Center+Spacing, Endpoints |
| 5 | Surround | OFF | OFF | −0.3 dB/m | 3.0 m | −2.0 m | 100 % | OFF | OFF | 2 pairs (4 speakers) | Surround Pairs |
| 6 | Delay Line | OFF | OFF | −0.15 dB/m | 3.0 m | −2.0 m | 100 % | OFF | OFF | 4 | Center+Spacing, Endpoints + Front/Back direction |
| 7 | Circle | OFF | OFF | −0.3 dB/m | 0.0 m | 0.0 m | 100 % | OFF | OFF | 12 | Circle (radius=5 m, startAngle=0°, Facing Inward) |

**Sub Bass special rule** — when Number of Speakers ≤ 2, the distance-attenuation percentage is snapped to 50 %; above 2, it auto-returns to 100 %. This is handled in `onPresetChanged()` in response to `numSpeakersEditor` edits.

---

## 3. Geometry Methods

Each method is implemented in `Source/Helpers/ArrayGeometryCalculator.cpp`. The wizard calls the appropriate function based on the selected preset and method radio, and assigns the resulting `(position, orientation)` pairs sequentially to the target Outputs starting at Start Output.

### 3.1 Straight from Center + Spacing
- **Function** — `calculateStraightFromCenter(centerX, centerY, z, numSpeakers, spacing, orientation)`
- **Inputs** — centerX (m), centerY (m), z (m), numSpeakers (int), spacing (m), orientation (°).
- **Output positions** —
  - `totalWidth = (numSpeakers − 1) × spacing`
  - `x[i] = centerX − totalWidth/2 + i × spacing`
  - `y[i] = centerY`
  - `z[i] = z`
- **Output orientations** — all speakers face the same `orientation` value.
- **Used by presets** — Near Field Straight, Main Flown Straight, Sub Bass, Delay Line.

### 3.2 Straight from Endpoints
- **Function** — `calculateStraightFromEndpoints(startX, startY, endX, endY, z, numSpeakers, orientation)`
- **Inputs** — startX, startY, endX, endY (m), z (m), numSpeakers (int), orientation (°).
- **Output positions** — linear interpolation with `t = i / (N − 1)` for `i ∈ [0, N−1]`:
  - `x[i] = startX + t × (endX − startX)`
  - `y[i] = startY + t × (endY − startY)`
  - `z[i] = z`
- **Output orientations** — all speakers face `orientation`.
- **Used by presets** — all straight-capable presets (1, 3, 4, 6).

### 3.3 Curved Array (Quadratic Bézier with Sag)
- **Function** — `calculateCurvedArray(startX, startY, endX, endY, z, numSpeakers, sag)`
- **Inputs** — startX, startY, endX, endY (m), z (m), numSpeakers (int), sag (m, signed — negative bends toward the audience).
- **Output positions** — quadratic Bézier with a control point offset perpendicular to the chord:
  - `mid = ((startX + endX)/2, (startY + endY)/2)`
  - `perp = rotate_90(end − start) / ‖end − start‖`
  - `ctrl = mid + perp × sag`
  - `B(t) = (1 − t)² × start + 2(1 − t)t × ctrl + t² × end`  for `t = i / (N − 1)`
- **Output orientations** — each speaker's orientation is **perpendicular to the curve tangent at that point**, pointing toward the audience (−Y). Produces a natural fan-out pattern.
- **Used by presets** — Near Field Curved.

### 3.4 Circle Array
- **Function** — `calculateCircleArray(centerX, centerY, radius, startAngle, z, numSpeakers, facingInward)`
- **Inputs** — centerX, centerY (m), radius (m), startAngle (°, 0° = top / −Y, clockwise), z (m), numSpeakers (int), facingInward (bool).
- **Output positions** — evenly distributed on a circle:
  - `angle[i] = startAngle + i × (360° / N)`
  - `x[i] = centerX + radius × sin(angle[i])`
  - `y[i] = centerY − radius × cos(angle[i])`
- **Output orientations** —
  - **Facing Inward** — each speaker faces the center: `orientation[i] = atan2(centerX − x[i], −(centerY − y[i]))`
  - **Facing Outward** — each speaker faces away from the center: `orientation[i] = atan2(x[i] − centerX, −(y[i] − centerY))`
- **Used by presets** — Circle.

### 3.5 Surround Pairs (Left / Right Mirrored)
- **Function** — `calculateSurroundPairs(numPairs, centerX, width, yStart, yEnd, z)`
- **Inputs** — numPairs (int), centerX (m), width (m, ± offset from center), yStart, yEnd (m), z (m).
- **Output positions** — each pair shares a Y value, and Y is linearly spaced between `yStart` and `yEnd`:
  - `y[i] = yStart + i/(numPairs − 1) × (yEnd − yStart)` for `i ∈ [0, numPairs−1]`
  - Left of pair `i` — `x = centerX − width`, facing +X (orientation = 90°)
  - Right of pair `i` — `x = centerX + width`, facing −X (orientation = −90°)
- **Output order** — speakers are written in the sequence `[L0, R0, L1, R1, …]`, so `numSpeakers = 2 × numPairs`.
- **Used by presets** — Surround.

---

## 4. Auto-advance behavior on Apply

After a successful Apply, both target selectors advance automatically so that hitting the wizard again immediately targets the next array:

- **Array selector** → advances by 1 (wraps back to 1 after 10).
- **Start Output selector** → advances by the number of speakers just applied.
- **Preview** is cleared.
- **Status label** shows `Applied {N} speakers to Array {A}. Ready for next array.`

If validation fails (e.g. Start Output + Number of Speakers exceeds the total output channel count), Apply is rejected and the status label shows `Error: {message}` without touching the ValueTree.

---

## 5. MCP tool shape (suggested)

A natural MCP tool derived from this wizard:

```
apply_array_preset(
    preset: "Near Field Straight" | "Near Field Curved" | "Main Flown Straight" |
            "Sub Bass" | "Surround" | "Delay Line" | "Circle",
    geometry: {
        method: "centerSpacing" | "endpoints" | "curved" | "circle" | "surround",
        // method-specific fields, see §3
    },
    target: {
        array: 1..10,
        startOutput: 1..N,
        numSpeakers: int
    },
    acoustic_overrides: { /* optional, any of the §1 Acoustic Defaults fields */ }
) -> { appliedOutputs: [ids], nextArray: int, nextStartOutput: int }
```

The preset table (§2) provides the default acoustic values when `acoustic_overrides` is omitted. The geometry table (§3) specifies which method fields are required per method.

Other configurations are possible and the suggested default values may be edited. They only represent classical setups.