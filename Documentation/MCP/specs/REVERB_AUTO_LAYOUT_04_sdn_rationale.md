# SDN Chaotic Placement — Design Rationale

This document encodes the SDN-specific design knowledge that drives the
chaotic-placement branch in `reverb_auto_layout`. It is the architectural
heart of the tool; the other three documents are mechanical.

## The Problem

The SDN algorithm computes inter-node delay paths from the actual geometric
distances between reverb nodes. Symmetric or quasi-uniform placements
produce identical or near-identical delay path lengths, which sum coherently
in the output and produce **comb filtering and metallic ringing**. This is
not a sound design choice — it is a structural artifact of the reverb model
operating on a too-regular geometry.

FDN and IR convolution algorithms do not have this problem. Their internal
structure (feedback matrix for FDN; fixed impulse response for IR) is
independent of node positions. Symmetric placements sound fine.

The tool's chaotic-placement branch applies only to SDN.

## The Wrong Lesson

The intuitive symmetry-breaking strategy is "jitter the positions a little."
This is **not enough**. Adding `±0.3m` random offsets to nodes placed on a
regular grid leaves the underlying topology symmetric: the nodes still form
three parallel rows on a regular U-shape, still left-right mirror-symmetric
within the jitter scale, and the inter-node distances are still
quasi-uniform. SDN sees the regular structure, not the jitter.

The first attempt at automatic placement in conversation made exactly this
error. The operator's correction was clarifying: jitter the position by
a small amount, but vary the *radial standoff* per node by a much larger
amount. That is the symmetry-breaking that matters.

## The Right Lesson — Verified Empirically

Examined: an operator's manual placement of 9 SDN nodes around a 20×10
stage. Observations:

- **Inter-node distances along the halo perimeter were roughly uniform.**
  Spacings ranged from 5.6m to 8.7m, but the variation was natural and
  smooth, not designed-in. Even arc-length spacing would have been fine.

- **Standoff distances from the stage perimeter varied substantially.**
  Per-node standoffs ranged from 4.8m to 6.6m — a 1.8m peak-to-peak spread,
  which is ~35% of the mean standoff. This is the variation that breaks
  symmetry.

- **Left-right symmetry was preserved within ~1m.** R1/R2/R3 (stage-left)
  mirrored R9/R8/R7 (stage-right) closely. Breaking left-right symmetry
  was not the operator's strategy.

- **Corners were cut diagonally, not traced as rectangles.** The halo
  shape was closer to a smooth arc than a strict rectangular inset.

The conclusion: **for SDN the symmetry-breaking that matters is in the
radial standoff dimension, not the along-perimeter dimension and not the
left-right axis.** Each node sits at a different distance from the speaker
locus; the *along-the-edge* layout can stay regular.

## Why Radial Variation Works

SDN's delay paths are between every pair of nodes, and between sources/
listener and each node. If nodes are at uniform standoff, source-to-node
distances cluster around a single value for each source position, and
node-to-node distances cluster around small integer multiples of the
along-edge spacing. Both produce coherent delay clusters.

Variable standoff disrupts both:

- Source-to-node distances span a wide range, so direct paths from a
  source to the reverb network arrive at varied times.
- Node-to-node distances no longer form small-integer relationships, so
  inter-reflection paths spread their energy across delay rather than
  clustering at multiples of a base delay.

The result is a smoother early-reflection spread and a denser late field,
without metallic comb peaks.

## Concrete Rules for Chaotic Placement

These translate the lesson into specific generation rules. Applied per
node:

1. **Even arc-length spacing along the perimeter/ring/shell.** Do not
   randomize the along-path dimension. Equal spacing is fine and visually
   tidy; the SDN engine is not sensitive to it.

2. **Standoff drawn from a wide distribution.** `Uniform(-1.0m, +1.0m)`
   around a mean of 4–6m gives the empirically-confirmed peak-to-peak
   spread.

3. **Per-node standoff non-clustering.** After sampling, enforce that no
   two adjacent nodes have standoffs within 0.4m of each other. If a pair
   violates this, re-sample one of them. This avoids accidentally placing
   two adjacent nodes at near-identical standoffs, which would locally
   re-introduce the symmetry the rest of the layout avoids.

4. **Odd node count preferred.** For SDN halos and surround rings,
   odd-count distributions avoid the left-right pairing that produces
   phantom-center comb artifacts. The v1 tool does not change channel
   counts, but should note in the rationale if the current count is even
   and SDN is selected.

5. **Small orientation jitter, `±5°`.** Each node's arrow does not need to
   point precisely toward origin. A small randomization helps decorrelate
   the directional response without changing the audible "pointing inward"
   character.

6. **Heights varied independently, `Uniform(2.0, 3.0)m` for non-dome.**
   Even small height differences add a third decorrelation axis at almost
   no design cost.

## What Doesn't Matter for SDN

- **Left-right asymmetry.** Operator manual layouts preserve symmetry to
  within ~1m. The radial variation already does the symmetry-breaking
  work; breaking left-right adds visual messiness without acoustic
  benefit.

- **Along-edge spacing irregularity.** Even spacing is fine.

- **Mathematical "irrational ratios" between distances.** This is a
  pseudo-rigorous overshoot. Real venues have inter-distance ratios that
  span integers and irrationals; SDN doesn't care, as long as the
  distances aren't *clustered*. Distribution width matters more than
  ratio properties.

## Heuristic for Node Count

The v1 tool does not change channel counts. But for documentation and
future reference, the operator's intuition for count is:

- **Stage halo, box stage** (perimeter ≤ 60m): 7–11 nodes. Odd preferred
  for SDN.
- **Stage halo, larger venues** (perimeter 60–100m): 11–15.
- **Surround ring**: ~1 node per 12–15m of perimeter. For an 8m-radius
  speaker ring (50m perimeter), that's about 7 nodes.
- **Dome shell**: scales with surface area; 9–15 typical for a venue-scale
  dome.

## Open Questions

These are worth revisiting once the tool is in field use:

- **Does the chaotic placement still ring metallically for very small
  node counts (3–5)?** Probably yes; below ~7 nodes the geometry is too
  constrained to be effectively aperiodic.

- **How does `sd_nscale` interact with placement?** The SDN scale factor
  multiplies the inter-node delays. At low scales (0.5x) the engine becomes
  more symmetry-sensitive because the delays are shorter and comb peaks
  more prominent. The current tool doesn't read or adjust `sd_nscale`;
  this may need revisiting.

- **Is per-node IR (`per_node_ir`) sensitive to placement symmetry?** If
  each node has its own impulse response, symmetry-induced coherence is
  partially broken by IR variation. May reduce the need for chaotic
  placement. Not investigated.

- **Active acoustic interpretation**: the reverb nodes are modeling room
  surfaces. In a real room, surfaces are at irregular distances from each
  other. The chaotic placement rules above happen to also be the
  physically-realistic placement rules — there's no conflict between
  "what sounds good" and "what models a real room." This alignment is
  worth keeping in mind as the system evolves.

## Acknowledgment

These rules were derived in conversation by examining an operator's manual
placement after several automated attempts produced metallic-sounding
results. The empirical anchor is the operator's manual placement; the
rules are what generates similar placements automatically.

The tool is opinionated about *how* to place SDN nodes but does not
claim to know the *best* placement for a given venue. The operator's ear
remains the final arbiter; the tool provides a good starting point and
the undo system handles regret.
