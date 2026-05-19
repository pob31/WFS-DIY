# Reverb Topology Classification

How `reverb_auto_layout` decides which placement geometry applies, from the
positions of the output channels.

## Premise

The reverb nodes go **behind the speakers**. To know where "behind" is, the
tool needs to know how the speakers are arranged. Five canonical
arrangements cover most real systems; if an arrangement fits more than one,
the tool asks rather than guesses.

## Inputs to Classification

From `session_get_state` and `session_get_global_state`:

- Output positions: `(x_i, y_i, z_i)` for `i ∈ [1, N_outputs]`
- Listener position: from tracking section, or origin if not set
- Stage geometry: `stageShape`, dimensions, origin offsets

## Derived Descriptors

Compute once at entry:

```
output_centroid_xy        = mean(x_i, y_i) across outputs
output_spread_xy          = std deviation of distance from centroid
output_extent_xy          = max - min on each axis
listener_to_centroid_xy   = distance from listener to centroid
output_quadrants          = count of outputs in each quadrant relative to listener:
                             (front_left, front_right, back_left, back_right)
                             where "front" is +Y, "back" is -Y, etc.
output_radial_pattern     = for each output, distance from listener;
                             measure if these distances cluster near a single
                             value (ring) or span a range (cluster/cloud)
output_height_pattern     = std deviation of z; near-zero = planar,
                             nonzero = elevated/dome
output_centroid_to_origin = distance from output centroid to origin
```

## Classification Tests

Apply in order. First test that *clearly* matches wins. If two tests match
comparably, classification returns ambiguous.

### Test A — frontal array

```
Condition:
    all outputs in front of listener (one quadrant or two adjacent ones)
    AND output_extent_xy in the perpendicular axis is small
    AND output_centroid clearly downstage of stage center (or upstage,
        depending on stage orientation)
    AND output_radial_pattern roughly clusters (not a wide range)

Result: stage_halo_behind_frontal_array
```

Typical example: 8–24 speakers in one or two lines along the downstage edge
of a box stage.

### Test B — three-sided perimeter

```
Condition:
    outputs span three sides of the stage (left edge, upstage, right edge)
    AND outputs absent from the audience side
    AND output_height_pattern small (planar)

Result: stage_halo_behind_perimeter_array
```

Typical example: 16+ speakers forming a U around the stage, audience
downstage.

### Test C — surround ring

```
Condition:
    outputs span all four quadrants around the listener
    AND output_radial_pattern clusters around a single radius
        (std deviation < 30% of mean radius)
    AND output_height_pattern small (planar)
    AND output_centroid close to listener (within 20% of mean radius)

Result: surround_ring
```

Typical example: 7.x or full immersive perimeter, audience in the center.

### Test D — outward central cluster

```
Condition:
    output_spread_xy small relative to expected room size
    AND output_centroid close to listener
    AND outputs are pointed outward (would need orientation reads to confirm,
        but position-only heuristic: outputs cluster within 2-3m of each other
        near listener)

Result: outer_ring_beyond_audience
        (reverb goes far outside, requires audience_radius parameter)
```

Typical example: central truss-mounted cluster pointing outward into a
surrounding audience.

### Test E — dome shell

```
Condition:
    output_height_pattern significant (std deviation of z > 1m)
    AND outputs distributed on a partial spherical surface
        (distance from a candidate center clusters around a single value)
    AND outputs cover a significant portion of the upper hemisphere

Result: dome_shell_behind
```

Typical example: hemisphere or dome of speakers, audience underneath.

## Ambiguity Handling

After running all tests, count how many produced a clear match. Cases:

- **0 matches**: return ambiguous with all five topologies offered. Rare; usually means an unusual rig.
- **1 match**: proceed with that topology.
- **2+ matches**: return ambiguous with the matched candidates and short
  explanation of what distinguishes them, so the operator can clarify.

Common ambiguous cases:

- **Three-sided perimeter (B) vs surround ring (C)**: outputs span three
  sides of the stage *plus* extend partway around the audience. Could be
  "elaborate stage monitoring" or "surround active acoustic." Ask.
- **Surround ring (C) vs dome shell (E)**: outputs in a 360° pattern but
  height varies. Could be a single-tier surround at varying heights, or a
  true dome shell. Ask.
- **Frontal array (A) vs perimeter (B)**: outputs in front and on the sides
  but not upstage. Could be either, depending on intent. Ask.

## Confidence Heuristics

Each test produces a confidence score 0–1. Two thresholds:

- **≥ 0.7** = clear match
- **0.4–0.7** = weak match (counted only if no clear match exists)
- **< 0.4** = no match

If two tests both score ≥ 0.7 and within 0.15 of each other, return
ambiguous. If one scores ≥ 0.7 and others score < 0.5, take the clear
winner.

The thresholds are starting points; tune against real session data once
the tool is in use.

## Implementation Notes

- All measurements in meters in operator coordinates.
- "Upstage" / "downstage" / "audience side" depend on stage orientation;
  in WFS-DIY the convention is +Y = upstage, audience at the downstage
  edge (lower Y values, near y=0 for a box stage with originDepth offset).
  Don't hardcode; derive from `originDepth` and `stageDepth`.
- Classification should be **fast** (a few hundred microseconds at most);
  it runs every call. No iterative optimization, just direct geometric tests.
- Don't trust output count alone — a "surround ring" with 7 speakers and
  a "frontal array" with 7 speakers are distinguished by their *spatial
  distribution*, not their count.
