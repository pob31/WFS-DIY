# Source Movements — Offsets, Trajectories, Jitter, and LFOs

WFS-DIY provides several mechanisms for moving sources, each with a different intent. Understanding which mechanism to use for which effect is key to writing good spatial scores.

## The four movement layers

Every source position is the sum of contributions from four independent layers:

1. **Base position** — set manually, by tracking, or by a Lemur/remote marker.
2. **Offset** — a constant shift applied to the base, with optional rotation and scaling.
3. **Move** — a one-shot trajectory from the current position to a target, over a defined time.
4. **Jitter** — a fast random micro-movement around the current position.
5. **LFO** — a periodic oscillation (sine, square, etc.) on each axis.

(Maps don't directly modulate position — they modulate level, height, and HF damping based on position. See the level maps document.)

The displayed "current position" reflects all of these contributions combined.

## Offset

An offset is a fixed shift applied to a source's base position. Unlike a base position change, the offset does not affect tracking or remote-marker assignments — it just moves the rendered position relative to whatever the base is.

Use cases:

- **Linked sources with maintained spacing.** Two singers controlled by the same Lemur marker — apply opposite-sign x offsets so they move together but stay 1.5 m apart.
- **Per-source fine adjustment** without changing the marker position that other sources may share.
- **Symmetric pairs** — apply offsets that mirror each other across the stage center.

The offset has additional **rotate** and **scale** transforms that affect groups of linked sources together. For example, scaling the offsets of a cluster lets the whole group expand or contract while staying centered on a common anchor. Rotation lets a cluster spin around its anchor.

The "Apply to group" and "Apply and Reset" buttons let you commit transforms to the underlying offset values (locking in the current rotated/scaled state) so that further changes start from the new baseline.

## Move (one-shot trajectory)

A Move is a single trajectory from the current position to a target, over a defined time. Once started, the source moves smoothly to the target and stays there.

Parameters:

- **Time** — duration in seconds.
- **Relative or absolute** — when relative, the target X/Y/Z are deltas from the current position; when absolute, they are coordinates relative to the stage origin.
- **Curve** — bend the trajectory upstage (positive) or downstage (negative). Zero means a straight line.
- **Acceleration profile** — interpolated between **line** (constant speed; sudden start and stop with associated Doppler) and **sine** (smooth start and stop, but higher peak speed). Most material benefits from values between the two extremes.

Controls:

- **GO** — start the movement.
- **STOP** — halt it where it currently is.
- **PAUSE** — pause at the current position; resume on next GO.

Move is appropriate for scripted, deterministic motion — a source crossing the stage during a specific cue, an entrance from offstage, a spiral approach for a dramatic moment. Show control software (QLab, Bitfocus Companion driving a StreamDeck, etc.) typically triggers Moves at the appropriate cues.

The **global movement speed** joystick scales all currently-running Moves uniformly. Useful for live tempo adjustments — speeding up or slowing down a sequence of cued movements without re-cueing them. Returns to 100% automatically when all moves complete.

**STOP ALL** and **PAUSE ALL** affect every running Move at once.

## Jitter

Jitter applies a fast, random micro-movement to a source's rendered position in all three axes, with a configurable amplitude.

The motion is small but rapid. It produces a perceptual sense that the source is "alive" — slightly unstable, slightly imprecise. This can be useful for:

- **Synthetic sources that sound too perfect.** A pure synthesizer pad gains presence with subtle jitter.
- **Imitating natural source positions.** A real instrumentalist's sound source isn't a mathematical point; it shifts as the player moves slightly. Jitter approximates this.
- **Disguising static sources during long sustains.** A held note from a fixed source can sound increasingly artificial; jitter masks this.

Increase amplitude carefully — large jitter values produce noticeable Doppler shifts on the audio. Small values (a few centimeters) usually suffice.

## LFO (Low-Frequency Oscillator)

LFO produces periodic, structured movement: circles, ellipses, squares, sawtooths, random walks, or any combination of waveforms across the three axes.

Architecture:

- A **main oscillator** with a base period (in seconds, 0.01 s to 100 s) and a phase offset (0° to 359°).
- For each of the three axes (x, y, z), a **shape** (sine, square, saw, triangle, keystone, log, exp, random, or off), an **amplitude**, a **rate** relative to the main period, and an axis-specific phase offset.

Combinations create geometric patterns:

- **Sine on X and Y, 90° phase difference, equal amplitude** = circle.
- **Sine on X and Y, in phase, different amplitude** = diagonal line oscillation.
- **Triangle on X, sine on Y** = a leaf-shaped path.
- **Keystone on both axes, 90° apart** = a square traversed at constant edge speed.
- **Random on all three** = chaotic positional drift.

The **rate** parameter per axis allows different speeds — e.g., x oscillating twice as fast as y produces a figure-8 or Lissajous pattern.

The **gyrophone** option (separate from the position LFO) makes the source's HF directivity rotate around its axis like a Leslie speaker's horn. This is independent of position movement and can be combined with it.

LFOs are ideal for:

- **Continuous ambient motion.** A drone source slowly orbiting the audience.
- **Geometric sound design.** Sound sources that trace deliberate shapes for visual or thematic reasons.
- **Subtle life on otherwise-static sources.** A small-amplitude slow sine adds organic motion without being obtrusive.

## Choice guide

| If you want... | Use... |
|---|---|
| One-shot motion to a target | Move |
| Continuous repeating motion | LFO |
| Tiny natural-feeling instability | Jitter |
| Constant offset from a marker position | Offset |
| Mirror or scaled group motion | Offset with apply-to-group |
| Live position updates from external tracking | Tracking |
| Scripted scene-by-scene positioning | Snapshots loaded by show control |

## Doppler considerations

All movement produces Doppler shifts in WFS, because the per-speaker delays change as the source moves. The faster the change, the more pronounced the shift.

If Doppler is a problem for specific source material:

- Reduce **maximum speed** for that input.
- Use **sine acceleration** rather than line for Move (smoother start/stop).
- For sources that don't have a real stage position, enable **curvature only** mode — only the relative delays matter, not the absolute distance, which dramatically reduces Doppler.
- Reduce **jitter amplitude**.
- Reduce **LFO amplitude** or increase the **period** (slower oscillation = less Doppler).

## OSC remote control

All movement parameters are remotely controllable via OSC. Show control systems (QLab cues, Ableton Live with the WFS OSC devices, custom scripts) typically drive Moves and LFO state changes via OSC at scene boundaries.
