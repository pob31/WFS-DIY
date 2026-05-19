# Reverb Placement Geometries

The four geometry primitives used by `reverb_auto_layout`, one per resolved
topology. Each primitive takes the speaker locus and reverb count as inputs
and produces a list of `(x, y, z, orientation, pitch)` tuples.

## Common Principles

### Speaker locus as reference

Standoff is measured **behind the speakers**, not from the stage edge. The
speakers are the WFS reproduction surface; reverb nodes must sit behind that
surface to be reconstructible as virtual sources. Several meters behind is
the right scale — colocated nodes degenerate to direct playback from one
speaker, and nodes too far behind lose spatial focus.

For each topology, the "speaker locus" is derived from output positions:

- **Frontal array**: the line (or curve) fit through the output positions.
- **Perimeter array**: the rectangle defined by the output positions' bounds.
- **Surround ring**: the circle (or ellipse) fit through the output positions.
- **Dome shell**: the spherical shell fit through the outputs.

### Orientation formula

For all topologies, orientation is computed by:

```
dx = x_node - x_target
dy = y_node - y_target
orientation_deg = degrees(atan2(dx, -dy))
wrap to [-180, +180]
```

`target` defaults to the world origin `(0, 0)`. Empirically verified
against operator manual placements.

For SDN: add `±5°` orientation jitter per node (Uniform). For FDN/IR: no
jitter.

### Pitch

Only `dome_shell_behind` writes pitch. Other topologies leave pitch at its
existing value.

```
dx = x_node - x_target
dy = y_node - y_target
dz = z_node - z_target
pitch_deg = degrees(atan2(dz, sqrt(dx² + dy²)))
```

### Height (z) for non-dome topologies

Random per node, Uniform(2.0, 3.0) meters. Operator-default scale; no
parameter exposed.

---

## Geometry 1: `stage_halo_behind_frontal_array`

### Inputs

- Frontal line/curve of speakers (the speaker locus).
- Stage rectangle (for the audience-face orientation reference).
- Reverb count `N`.
- Algorithm (`SDN` or `FDN_IR`).
- Optional `seed` for randomization.

### Procedure

1. Determine the "behind" side of the speaker locus — opposite the audience.
   For a typical frontal array near the downstage edge, "behind" is upstage
   and to the sides.

2. Build the halo path: a U-shape running along three sides of the stage,
   offset behind the speakers by `standoff_mean = 5.0m`. Corners smoothed
   (corner-smoothing factor 0.6; corners cut diagonally rather than at sharp
   right angles).

3. Compute total halo length `L`. Place `N` nodes at arc-length positions
   `s_i = (i + 0.5) * L / N`.

4. For each node:
   - **SDN**: apply standoff jitter `Uniform(-1.0, +1.0)` along the outward
     normal. Ensure no two adjacent nodes share a standoff within 0.4m
     (regenerate jitter for that node if so).
   - **FDN/IR**: standoff jitter `Uniform(-0.2, +0.2)` or zero.

5. Sample `z = Uniform(2.0, 3.0)` per node.

6. Compute orientation per the common formula, target = origin.

7. **SDN only**: add `Uniform(-5°, +5°)` to each orientation.

### Defaults

```
standoff_mean = 5.0m
standoff_jitter = 1.0m (SDN) or 0.2m (FDN/IR)
corner_smoothing = 0.6
audience_arc_skip = full downstage face open
```

---

## Geometry 2: `stage_halo_behind_perimeter_array`

Mechanically identical to `stage_halo_behind_frontal_array`, except the
halo is a full rectangle (no audience arc skip) offset behind a four-sided
perimeter speaker array.

Distinguish from Geometry 1: here the speakers form a U or a full ring
around the stage, so there's a "behind" position on all four sides of the
stage. If the array doesn't cover the audience side (just three sides),
keep the audience face open as in Geometry 1.

---

## Geometry 3: `surround_ring`

### Inputs

- Speaker ring radius and center (from output positions).
- `audience_radius` (operator-supplied parameter, meters).
- Reverb count `N`.
- Algorithm.
- Optional `seed`.

### Procedure

1. Ring center = speaker ring center (typically near listener).

2. Standoff: 3–5m **outside** the speaker ring. Reverb ring radius =
   `speaker_ring_radius + 4.0m` (mean).

3. **Lower node density than stage halo**. For SDN:
   - Stage halo gets ~1 node per 8–10m of perimeter.
   - Surround ring gets ~1 node per 12–15m of perimeter.
   - Distant boundaries don't need as many nodes for effective coverage.

4. Place `N` nodes at evenly-spaced angles around the ring:
   `θ_i = 2π * i / N + θ_0` where `θ_0` is a random rotation per `seed`.

5. For each node:
   - **SDN**: standoff jitter `Uniform(-1.0, +1.0)` along radial outward.
   - **FDN/IR**: standoff jitter `Uniform(-0.2, +0.2)` or zero.

6. Sample `z = Uniform(2.0, 3.0)`.

7. Compute orientation toward origin via the common formula.

8. **SDN only**: add `Uniform(-5°, +5°)` orientation jitter.

### Defaults

```
standoff_mean = 4.0m (outside speaker ring)
standoff_jitter = 1.0m (SDN) or 0.2m (FDN/IR)
coverage = full 360°
node_count_hint = ~1 per 12-15m of reverb ring perimeter
```

---

## Geometry 4: `outer_ring_beyond_audience`

### Inputs

- Output cluster (central, outward-facing).
- `audience_radius` (operator-supplied).
- Reverb count `N`.
- Algorithm.
- Optional `seed`.

### Procedure

Same as Geometry 3, but the reverb ring radius = `audience_radius + 4.0m`
rather than `speaker_radius + standoff`. The reverbs sit far outside the
audience zone, representing distant venue boundaries.

In this topology, the reverb radiation is supplemental — possibly through
a physical outer ring of speakers placed for this purpose. The tool only
places the reverb *nodes*; suggesting outer speaker placement is a separate
concern and not in v1.

### Defaults

```
reverb_ring_radius = audience_radius + 4.0m
standoff_jitter = 1.0m (SDN) or 0.2m (FDN/IR)
coverage = full 360°
node_count_hint = ~1 per 15-20m of reverb ring perimeter
```

---

## Geometry 5: `dome_shell_behind`

### Inputs

- Speaker shell radius (from output positions; fit a sphere or hemisphere).
- Reverb count `N`.
- Algorithm.
- Optional `seed`.

### Procedure

1. Reverb shell radius = `speaker_shell_radius + 4.0m`.

2. Distribute `N` nodes on the shell, skipping the audience cap (below
   some elevation threshold, typically 0° or slightly negative —
   operator-default).

3. For an even, aperiodic distribution use a Vogel/golden-angle spiral
   on the shell, with a random starting offset from `seed`. This naturally
   avoids the symmetry traps of latitude/longitude grids while preserving
   roughly even surface density.

4. For each node:
   - **SDN**: radial standoff jitter `Uniform(-1.0, +1.0)` in 3D.
   - **FDN/IR**: minimal jitter.

5. Compute orientation toward origin via the common formula (XY projection).

6. Compute pitch toward origin via the pitch formula.

7. **SDN only**: add `Uniform(-5°, +5°)` to orientation and `Uniform(-3°, +3°)` to pitch.

### Defaults

```
shell_offset = 4.0m (radial, behind speaker shell)
shell_offset_jitter = 1.0m (SDN) or 0.2m (FDN/IR)
audience_cap_below_elevation = 0°
distribution = Vogel spiral with seed-based start
```

---

## Notes on Numerical Stability

- All formulas should clamp orientation/pitch to `[-180, +180]` and
  `[-90, +90]` respectively, wrapping at the boundaries.
- The `atan2` formulas are numerically stable at the boundaries (no
  division-by-zero); use the language's standard library implementation.
- For very small stages (< 5m perimeter) the standoffs may produce nodes
  outside the venue. The tool should not enforce venue bounds — operators
  may model rooms larger than the staging area — but should warn if any
  node ends up beyond `±50m` from origin (parameter range limit).

## Notes on Implementation Style

The five geometries share more than they differ. A clean implementation
would factor out:

- `compute_orientation(node_pos, target_pos)` — used by all
- `compute_pitch(node_pos, target_pos)` — used by dome
- `apply_sdn_jitter(positions, magnitude, seed)` — chaotic standoff
- `apply_regular_jitter(positions, magnitude, seed)` — small jitter

And vary only the **path/locus definition** per geometry: the halo path,
the ring circle, the shell sphere. Most of the code is shared.
