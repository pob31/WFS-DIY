# Position Tracking

Tracking allows actors, musicians, or props to drive source positions in real time. Instead of manually positioning sources or running pre-programmed movements, the system follows physical movement on stage. This document covers the concepts, the supported protocols, and practical considerations.

## Why tracking matters in WFS

WFS's central promise is that visual and auditory positions agree for every listener. When an actor moves across the stage, this agreement should hold without the operator having to manually update positions or pre-program every movement.

Without tracking, the operator has these options:

- **Static positions**, with the source's audio location fixed regardless of where the performer goes — visual/auditory mismatch when they move.
- **Pre-programmed Moves** triggered at cues — works if movement is choreographed and reliable, fragile if it isn't.
- **Manual control** via Lemur, Streamdeck, or similar — depends on operator focus and reaction time.

Tracking adds a fourth option: positions are continuously updated from sensor data, and the WFS algorithm renders them in real time.

## Tracking technologies

Several physical tracking technologies feed position data into WFS-DIY:

**UWB (Ultra-Wideband) tags** — small wearable transmitters that send radio pulses to fixed anchor receivers. The system computes position by triangulation. Sub-meter precision in good conditions. Examples: Pozyx, Eliko, custom UWB systems.

**Computer vision systems** — cameras tracking visual markers or doing markerless human-pose estimation. Works well in controlled lighting; can struggle with occlusion. Examples: BlackTrax (with reflective markers), various OpenCV-based systems.

**LiDAR** — laser-based depth sensing, with point cloud processing on the receiving side. Good precision, immune to lighting. Heavier compute requirement on the processing side. Examples: RoboSense, Velodyne, Ouster, Unitree.

**IR-LED systems** — performers wear infrared LEDs picked up by IR-sensitive cameras. Good precision for stage applications. Examples: BlackTrax, custom rigs.

**3D cameras** (depth sensors) — Kinect-style depth + RGB systems. Limited range and precision but easy to deploy.

The choice depends on venue acoustics (UWB is RF-quiet but susceptible to body blockage), lighting (vision systems need controlled light), occlusion (LiDAR sees through some occlusion that cameras don't), and budget.

## Supported protocols

WFS-DIY accepts position data over four protocols:

- **OSC** — flexible, generic, works with any system that can send OSC. Simple format.
- **PosiStageNet (PSN)** — the de-facto standard in entertainment lighting and broadcast for tracking data. Well-supported by professional tracking systems.
- **RTTrP** (Real-Time Tracking Protocol) — BlackTrax's native protocol; also supported by some other vendors.
- **MQTT** — lightweight pub/sub protocol used by some IoT-style tracking deployments.

The choice between protocols depends on what the tracking system natively speaks. PSN is the most common in professional theater contexts.

## OSC format for tracking

When using OSC, the expected message format is:

```
/wfs/tracking/positionXYZ [tag_id] [x] [y] [z] [(quality)]
```

Where:

- `tag_id` is an integer identifying which physical tag/marker.
- `x`, `y`, `z` are floats representing position (in meters, before any offset/scale/flip applied at the WFS side).
- `quality` (optional) is a float indicating the confidence of the measurement. When provided, this is used to weight the smoothing — low-quality measurements have less influence.

## Mapping tracking data to WFS coordinates

Tracking systems use their own coordinate frames. The tracking section in WFS-DIY provides:

- **Offset X/Y/Z** — translation to align the tracking origin with the WFS stage origin.
- **Scale X/Y/Z** — multiplication factors per axis, useful when the tracking system reports in different units or has a different stage size baseline.
- **Flip X/Y/Z** — axis inversion, when the tracking system's axes don't match WFS conventions (e.g., a system where +y is downstage and WFS's +y is upstage).

Calibration procedure: place a tag at a known stage position (e.g., the origin), see what the tracking system reports, set offsets to bring the reported position to 0,0,0. Then move the tag to a known distance and verify scale; adjust if needed. Test corner positions to verify flips.

These settings are global — they apply to all tags. Per-input fine-tuning is done via the input's offset, not the tracking offset.

## Smoothing

A **smoothing percentage** parameter damps position updates. Higher values produce smoother, slower-reacting movement; lower values produce faster, more reactive but potentially jittery movement.

Smoothing is useful because tracking data is rarely perfect:

- Tags can be momentarily blocked (in pockets, behind set pieces).
- RF systems have occasional noise spikes.
- Tags choked between bodies and walls produce position errors.

But smoothing has a cost: it introduces lag. A heavily smoothed source lags behind the actual physical position. For fast-moving performers, low smoothing is required; for slow stationary blocking, high smoothing improves stability.

A practical rule: a clear 2 cm physical gap around the antenna of an RF tag improves tracking precision more than any amount of software smoothing. Hardware design matters as much as algorithms.

## Per-input tracking assignment

Each input channel can be controlled by **one or two tags**. When two tags are assigned, the average of their positions is used.

Why two tags?

- **Redundancy** — if one tag has poor signal momentarily, the other provides position data.
- **Body center estimation** — placing one tag at the front and one at the back of a performer averages to roughly their body center, which is a more stable position than either tag alone.
- **Pair tracking** — for dancers in close partnership where the WFS source represents both.

A single tag can be associated with multiple input channels — useful when a single performer drives several audio inputs (a singer with a wireless mic and a guitar pickup, for example).

Each input has:

- **Tracking active** toggle.
- **Tracking ID** for the tag (1 to 32, or up to 64 depending on the tracking system).
- **Tracking smoothing** override per input, in addition to the global setting.

## When tracking is enabled vs. disabled

When tracking is active for an input:

- The base position is set by the tracking data continuously.
- The **offset** parameter still applies (allowing per-input fine adjustments without recalibrating tracking).
- LFOs, jitter, and Move can still be active but are added to the tracked position rather than driving it.
- The position fields in the UI become read-only — manually entering a position has no effect while tracking is active.

When tracking is not active for an input:

- The base position is whatever was manually set or recalled from a snapshot.
- The Lemur/remote markers can drive the position.
- All other movement layers (offset, jitter, LFO, Move) work normally.

## Tracking with WFS hand-off

A common pattern: a performer wears a tag for parts of the show where they move freely, but the show transitions to a static scene where they sit at a fixed position. Rather than fighting tracking jitter at the static position, **disable tracking** at the scene transition and **manually set** the position. Re-enable when movement resumes.

Snapshots can include tracking-enable state, so this hand-off can be cued from show control.

## Performance considerations

Tracking adds CPU load roughly proportional to the number of tracked inputs and the rate of incoming updates. For 16-32 tracked inputs at 60 Hz update rate, the cost is modest. For 100+ inputs at high rates, more CPU headroom is required.

Network considerations: tracking data on busy networks can be lost (UDP/OSC has no delivery guarantee). Use a dedicated tracking VLAN or wired connection for critical applications.

## Future: AI-assisted tracking

Combining LiDAR with AI-based human-pose estimation (running on a Jetson or similar) is on the project's roadmap as a tracking input. The advantage is markerless tracking — performers don't need to wear anything. The challenge is robustness across lighting and occlusion conditions, and the inference latency that AI models add.
