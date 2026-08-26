# WFS-DIY DAW Plugin Suite — MCP Reference & Setup Guide

DAW-native controller plugins (VST3 / AU / Standalone) that let a DAW drive a running WFS-DIY app over OSC — for curve-drawing, automation recording, and parameter control directly from the session timeline. Complement to QLab / Android Remote / hardware controllers.

**Source** — `Plugin/Source/` (`Master/`, `Track/`, `Variants/`, `Shared/`, `Bridge/`).
**Status** — Phases 0–4 functionally complete. All five Track variants plus Master exchange basic parameters (including positions) end-to-end with the WFS-DIY app. Remaining plugin work is UI polish, multi-variant stacking validation, and release-label bookkeeping — see `Plugin/docs/PRD.md` for the phase roadmap.
**Formats** — macOS: VST3 + AU + Standalone (Universal). Windows: VST3 + Standalone (x64). No AAX.
**Code signing** — macOS `.pkg` is codesigned + notarized. Windows installers are unsigned pending external sponsorship of a certificate.

---

## 1. Plugin Catalog

| Plugin | Role | Instances per project | Bundle suffix |
|---|---|---|---|
| **WFS-DIY Master** | Owns the network connection to the WFS-DIY app (UDP OSC + OSCQuery HTTP/WebSocket + optional ADM-OSC Rx). One-per-project. | 1 | `Master` |
| **WFS-DIY Track – Cartesian** | Native WFS-DIY OSC, X/Y/Z in meters. | 1 per input | `trackCart` |
| **WFS-DIY Track – Cylindrical** | Native, R / θ / Z. | 1 per input | `trackCyl` |
| **WFS-DIY Track – Spherical** | Native, R / θ / φ. | 1 per input | `trackSph` |
| **WFS-DIY Track – ADM Cartesian** | ADM-OSC normalized X/Y/Z (-1..+1). | 1 per input | `trackAdmCart` |
| **WFS-DIY Track – ADM Polar** | ADM-OSC azimuth / elevation / distance. | 1 per input | `trackAdmPol` |

All six plugins share one code base. The five Track variants differ only in a compile-time `VariantConfig` (addressing scheme + 3-element position param spec); the rest of the plugin (network, DAW automation, UI, bridge) is identical.

---

## 2. Master Plugin

### 2.1 AudioProcessor parameters (DAW-automatable)

| paramID | Label | Type | Range | Default | Notes |
|---|---|---|---|---|---|
| `enabled` | Enabled | Bool | 0 / 1 | 1 | Master enable. When off, no OSC traffic is sent regardless of Connect state. |

That's the only DAW-automatable parameter. All other Master state is persisted via `getStateInformation()` / `setStateInformation()`, not exposed as automatable parameters.

### 2.2 UI controls (editor)

| Field | Type | Stored as | Default | Purpose |
|---|---|---|---|---|
| Host | TextEditor | plugin state | `127.0.0.1` | IP / hostname of the running WFS-DIY app |
| OSC send port (Tx) | TextEditor (int) | plugin state | `8000` | UDP port on the app to send to. Must match **Network tab → UDP Port** in WFS-DIY. |
| OSCQuery HTTP port | TextEditor (int) | plugin state | `5005` | HTTP port the app serves OSCQuery on. Must match **Network tab → OSC Query Port**. |
| ADM-OSC Rx port | TextEditor (int) | plugin state | `4001` | UDP port on the plugin that will receive ADM-OSC echoes from the app. Only needed when any ADM variant is in use. Set to 0 to disable the ADM receiver. |
| Connect | TextButton | — | — | Opens both the UDP send socket and the OSCQuery WebSocket. If ADM Rx > 0, also opens the ADM UDP listener on that port. |
| Status | Label | — | "Disconnected" | Live connection state: "Disconnected" / "Connected" / "Error: …". |
| Registered Tracks | Label | — | "Registered Tracks: 0" | Count of Track plugins currently talking to this Master via the in-process bridge. |
| Build stamp | Label | — | generated | Build date/time of the plugin for support diagnostics. |

### 2.3 Networking behavior

- Master owns all OSC I/O; Track plugins have **no network code**. Tracks register with Master via the process-wide `WFS-DIY-PluginBridge` shared library (see §5).
- Outbound from Master to the app:
  - Native Track outbound → `/wfs/input/<param> <id> <value>` (UDP to the configured host/UDP port)
  - ADM Track outbound → `/adm/obj/<id>/xyz <x> <y> <z>` or `/adm/obj/<id>/aed <az> <el> <dist>` (UDP, combined path)
- Inbound to Master:
  - OSCQuery WebSocket pushes (from the app's OSCQuery server): used to sync plugin parameters when the app is edited directly (e.g. from the Map tab).
  - ADM-OSC UDP on the ADM Rx port: used to sync ADM Track positions when the app echoes updates.
- Rate limiting: outbound OSC is rate-limited per parameter (see `Plugin/Source/Shared/RateLimiter.cpp`) to avoid flooding the app during automation replay.

---

## 3. Track Plugins (all five variants)

### 3.1 Shared non-position parameters (12) — `getSharedTrackParams()` in `Plugin/Source/Track/TrackProcessor.cpp`

All five variants expose the same 12 parameters in addition to their 3 position parameters. paramID strings below are what shows up in the DAW's automation lane.

| paramID | Label | OSC path | Type | Min | Max | Default | Unit | Widget | Log skew midpoint |
|---|---|---|---|---|---|---|---|---|---|
| `attenuation` | Attenuation | `/wfs/input/attenuation` | float | −92 | 0 | 0 | dB | H slider (log) | −12 |
| `attenuationLaw` | Attenuation Law | `/wfs/input/attenuationLaw` | int | 0 | 1 | 0 | — | TwoStateCombo (`Log` / `1/d²`) | — |
| `distanceAttenuation` | Distance Atten. | `/wfs/input/distanceAttenuation` | float | −6 | 0 | −0.7 | dB/m | H slider (linear) | — |
| `distanceRatio` | Distance Ratio | `/wfs/input/distanceRatio` | float | 0.1 | 10 | 1.0 | × | H slider (log) | 1.0 |
| `directivity` | Directivity | `/wfs/input/directivity` | int | 2 | 360 | 360 | ° | H slider (linear) | — |
| `rotation` | Rotation | `/wfs/input/rotation` | int | −179 | 180 | 0 | ° | Rotary dial | — |
| `tilt` | Tilt | `/wfs/input/tilt` | int | −90 | 90 | 0 | ° | Bidirectional bar | — |
| `hfShelf` | HF Shelf | `/wfs/input/HFshelf` | float | −24 | 0 | −6 | dB | H slider (log) | −6 |
| `lfoActive` | LFO Active | `/wfs/input/LFOactive` | int | 0 | 1 | 0 | — | Toggle | — |
| `commonAtten` | Common Atten. | `/wfs/input/commonAtten` | int | 0 | 100 | 100 | % | H slider (linear) | — |
| `stereoWidth` | Stereo Width | `/wfs/input/stereoWidth` | float | 0 | 50 | 4.0 | m | H slider | 4.0 |
| `stereoAxisOffset` | Stereo Axis | `/wfs/input/stereoAxisOffset` | int | −179 | 180 | 0 | ° | Rotary dial | — |

The last two are the **stereo image**, and they exist only on a stereo input
channel. Width is the full L-to-R distance in **metres** — not the 0–100 % of the
array's X extent it was before the app's stereo rework. Axis rotates the automatic
tangential axis; ±180 swaps L and R.

On a mono channel the Track hides both controls (and shrinks by the height of the
section) and, more importantly, **stops transmitting them**. Hiding alone would
not be enough: a host automation lane goes on writing whatever the editor happens
to be showing, and the app would then store a stereo image on a channel that has
no stereo legs. The Track learns the channel's type from the Master; see §3.3.

Plus a routing parameter present on every Track regardless of variant:

| paramID | Label | Type | Min | Max | Default | Notes |
|---|---|---|---|---|---|---|
| `inputId` | Input ID | int | 1 | 64 | 1 | The **permanent channel number**, not an index — see §3.3. Must be unique per Track in the session (two Tracks on the same ID would fight each other). |

### 3.3 Input ID is a permanent channel number

The app numbers input channels permanently. A number is assigned once and stays
with its channel: **deleting a channel leaves its number as a permanent gap, and
dragging the channel list into a new order does not renumber anything.** So the
live numbers are not `1..count`, they need not be contiguous, and their order in
the app's channel list is unrelated to their numeric order.

For a Track this is good news and needs no action: `inputId` has always been the
number, so a saved DAW session keeps pointing at the same channel after the
operator reorders the app's list. Two consequences are worth knowing:

- A Track can be pointed at a number that names nothing — a retired gap, or a
  channel not created yet. The app drops such writes silently. The Track shows
  `#7 - no such channel` under its Input ID so this is visible rather than a
  mystery. When the channel is present the same line reads `#12 "Kick" (stereo)`.
- **Any Track output latches the app's channel numbering.** The app re-flows
  numbers, names and the input patch to match display order only while a session
  is "fresh"; the first external message naming a channel by number ends that,
  permanently, for that session. This is deliberate — the plugin holds numbers, and
  re-flowing them underneath it would silently re-point every Track — but it means
  a session a DAW has talked to never re-flows again. Nothing is lost by it.

The identity line and the mono/stereo verdict come from the Master, which reads
the app's input namespace over OSCQuery (`GET /wfs/input`, refetched whenever the
app reports `PATH_CHANGED`) and forwards the result to each Track over the bridge.
Until the Master has actually read that list, the line is blank: a Track that has
not been told anything says nothing rather than accusing a good channel of being
absent.

### 3.2 Position parameters (per variant)

Each variant supplies 3 position params. paramIDs are identical across variants where convenient, but the ranges / units / OSC paths differ.

#### Cartesian — `Plugin/Source/Variants/CartesianMain.cpp`
| paramID | Label | OSC path | Min | Max | Default | Unit |
|---|---|---|---|---|---|---|
| `positionX` | Position X | `/wfs/input/positionX` | −50 | 50 | 0 | m |
| `positionY` | Position Y | `/wfs/input/positionY` | −50 | 50 | 0 | m |
| `positionZ` | Position Z | `/wfs/input/positionZ` | −50 | 50 | 0 | m |

#### Cylindrical — `CylindricalMain.cpp`
| paramID | Label | OSC path | Min | Max | Default | Unit |
|---|---|---|---|---|---|---|
| `positionR` | Position R | `/wfs/input/positionR` | 0 | 50 | 0 | m |
| `positionTheta` | Position Theta | `/wfs/input/positionTheta` | −180 | 180 | 0 | ° |
| `positionZ` | Position Z | `/wfs/input/positionZ` | −50 | 50 | 0 | m |

#### Spherical — `SphericalMain.cpp`
| paramID | Label | OSC path | Min | Max | Default | Unit |
|---|---|---|---|---|---|---|
| `positionR` | Position R | `/wfs/input/positionRsph` | 0 | 50 | 0 | m |
| `positionTheta` | Position Theta | `/wfs/input/positionTheta` | −180 | 180 | 0 | ° |
| `positionPhi` | Position Phi | `/wfs/input/positionPhi` | −90 | 90 | 0 | ° |

#### ADM Cartesian — `AdmCartesianMain.cpp`
Normalized −1..+1 per axis. Sent as the combined `/adm/obj/<id>/xyz` path.

| paramID | Label | Min | Max | Default | Unit |
|---|---|---|---|---|---|
| `admX` | X | −1 | 1 | 0 | (normalized) |
| `admY` | Y | −1 | 1 | 0 | (normalized) |
| `admZ` | Z | −1 | 1 | 0 | (normalized) |

#### ADM Polar — `AdmPolarMain.cpp`
Sent as the combined `/adm/obj/<id>/aed` path.

| paramID | Label | Min | Max | Default | Unit |
|---|---|---|---|---|---|
| `admAzimuth` | Azimuth | −180 | 180 | 0 | ° |
| `admElevation` | Elevation | −90 | 90 | 0 | ° |
| `admDistance` | Distance | 0 | 1 | 0 | (normalized) |

**Internal coordinate handling.** Native variants (Cyl/Sph) expose their own coordinate system to the DAW but convert to/from Cartesian at the OSC boundary, because the app's OSC router only accepts Cartesian X/Y/Z. See `TrackProcessor::displayToCartesian()` / `cartesianToDisplay()` for the conversion.

**Feedback-loop suppression (ADM variants).** When the app echoes an ADM position back (with small drift from mapping rounding / constraints), the plugin caches the last inbound triple (`lastRxAdmV1/V2/V3`) and skips the outbound that would just echo it back.

---

## 4. Setup Guide — Connecting a DAW Session to WFS-DIY

This is the happy-path sequence to get a new DAW project driving a running WFS-DIY app.

### 4.1 App side (WFS-DIY desktop app)

1. **Start the WFS-DIY app**, open (or create) a project.
2. **Open the Network tab.** Confirm:
   - **Current IPv4** — note this if the DAW is on another machine.
   - **UDP Port** — default `8000`. The plugin's *OSC send port (Tx)* must match this value.
   - **OSC Query Enable** — toggle ON.
   - **OSC Query Port** — default `5005`. The plugin's *OSCQuery HTTP port* must match this value.
   - **OSC Source Filter** — set to *Accept All* for first-time setup; tighten later if desired.
3. **(Optional, for ADM-OSC variants only)** On the Network tab, add a target with Protocol = `ADM-OSC`. Then under the ADM-OSC section of the Network tab:
   - Pick one of the 4 Cartesian or 4 Polar mappings (see `WFS-UI_network.csv`).
   - Adjust the per-axis Source Axis / Sign Flip / Center / Breakpoint / Widths to describe your stage in ADM-normalized coordinates.
   - Use the *Input Assignment* grid to map each input channel to a specific ADM mapping.

### 4.2 DAW side — Master

1. **Insert the WFS-DIY Master plugin** on any track (a dedicated MIDI / utility track is cleanest — it doesn't process audio). Only one instance is needed for the project.
2. In the Master editor:
   - **Host** → the app's IPv4 (use `127.0.0.1` if both app and DAW are on the same machine).
   - **OSC send port (Tx)** → match the app's *UDP Port* (default `8000`).
   - **OSCQuery HTTP port** → match the app's *OSC Query Port* (default `5005`).
   - **ADM-OSC Rx port** → defaults to `4001`. Leave at `0` if you're not using any ADM variant. **If you keep a non-zero value, configure the app's ADM-OSC target to send to this port.**
3. Click **Connect**. Status should flip to `Connected`. *Registered Tracks* will still read `0` until you add Tracks.

### 4.3 DAW side — Tracks

1. **Insert a Track variant** on each audio track whose input you want to control. Pick a coordinate system per track:
   - *Cartesian / Cylindrical / Spherical* — native WFS-DIY OSC. Use these unless you specifically need ADM.
   - *ADM Cartesian / ADM Polar* — only when driving ADM-OSC mappings configured in §4.1 step 3.
2. In each Track editor, set **Input ID** to the WFS-DIY input channel you want this track to control. This is the channel's permanent number (§3.3), not its position in the app's list; the line under the field confirms which channel you landed on. **Unique per session** — two Tracks on the same ID will fight each other.
3. Automate or manually move the 3 position parameters + any of the 12 non-position parameters. The Master's *Registered Tracks* count should increase as each Track loads.
4. Change a parameter in the app (e.g. drag the input on the Map tab). The OSCQuery WebSocket push should update the corresponding Track's plugin parameter in near-real-time.

### 4.4 Troubleshooting

| Symptom | Likely cause |
|---|---|
| Master shows "No WFS-DIY Master found" on a Track | Bridge singleton unreachable — DAW is running Master and Track in separate processes (Reaper "Run as separate process", Bitwig per-plugin sandbox). Disable sandboxing for Master + Tracks in the same project. |
| Master stays "Disconnected" after Connect | Host/port mismatch with the app's Network tab. Also check firewall rules for both UDP (send port) and TCP (OSCQuery HTTP port). |
| Track parameters move but the app doesn't react | Input ID on the Track doesn't exist in the app — the line under the field reads *no such channel*. Numbers are permanent and deletions leave gaps, so a number below the channel count can still name nothing (check System Config → Input Channels). Otherwise the app's OSC Source Filter is set to *Registered Only* without the DAW's IP registered as a target. |
| Stereo Width / Stereo Axis are missing from the Track | That channel is mono. The two controls appear only when the app reports the channel as stereo; the identity line under Input ID says which it is. If it is blank, the Master has not read the app's channel list yet — check the Master is connected. |
| App moves on the Map tab, plugin doesn't follow | OSCQuery connection dropped (check Status label). Reconnect. |
| ADM Track moves but the app doesn't | No ADM-OSC target configured on the app side, or no ADM mapping has the relevant input assigned. See §4.1 step 3. |
| ADM app changes don't update the ADM Track | ADM-OSC Rx port on Master is 0 or blocked by firewall, or the app's ADM target isn't configured to send to that port. |

---

## 5. Architecture Notes

- **PluginBridge** — a tiny shared library (`WFS-DIY-PluginBridge`) installed next to the VST3 bundles. Provides an in-process singleton that Master owns and Tracks look up. All inter-plugin communication within one DAW process goes through this bridge — Tracks never open a socket.
- **Sandboxing incompatibility** — when a host puts Master and Tracks in separate processes, the shared singleton is broken. The plugin UI shows `No WFS-DIY Master found`. Known-bad modes: Reaper "Run as separate process", Bitwig per-plugin sandboxing.
- **Variant is compile-time** — every Track variant is the same binary built against a different `VariantConfig`. No branching inside the Track processor on coordinate system or addressing scheme.
- **State persistence** — plugin state is saved via the standard DAW project-state mechanism (`getStateInformation` / `setStateInformation`). Both DAW-automatable parameters (via JUCE `AudioProcessorValueTreeState`) and non-parameter plugin settings (Master host/ports) survive project save/load.

---

## 6. MCP Tool Surface (Suggested)

Plugin-oriented MCP tools that pair naturally with the above:

```
connect_daw_bridge(host: str = "127.0.0.1",
                   udp_port: int = 8000,
                   oscquery_port: int = 5005,
                   adm_rx_port: int | None = None) -> {status, registered_tracks}

set_track_parameter(input_id: int, paramID: str, value: float)
    # Native non-position: paramID ∈ {attenuation, attenuationLaw, distanceAttenuation,
    #                                  distanceRatio, directivity, rotation, tilt,
    #                                  hfShelf, lfoActive}
    # Native position:     {positionX|Y|Z, positionR, positionTheta, positionPhi}
    # ADM:                 {admX|Y|Z, admAzimuth, admElevation, admDistance}

get_registered_tracks() -> [{input_id, variant_tag, build_stamp}]
```

Notes for the generator:
- The full paramID → OSC-path mapping lives in the tables above (§2.1, §3.1, §3.2) — the MCP server can look it up rather than hardcoding the translation.
- Ranges, units, and widget types are all present so an LLM can validate inputs before sending.
- `lfoActive` is the only LFO parameter currently exposed in the Phase 0 Track surface; the full LFO shape / rate / amplitude / phase set lives in the main app and is not yet wired into the plugin.
