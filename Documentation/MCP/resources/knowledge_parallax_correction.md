# Parallax Correction

Parallax correction is one of the more subtle features in WFS-DIY. It compensates for the fact that sources, speakers, and listeners are almost never geometrically aligned in a real venue. Getting parallax settings right makes the difference between a soundfield that localizes well and one that smears.

## The problem

A naive WFS algorithm computes, for each source and each speaker, the delay between the source's virtual position and the speaker's physical position. It sends the source to the speaker delayed by the time it would take sound to travel that distance at the speed of sound.

This is only correct if the listener is *at the speaker*. In reality, the listener is somewhere in the audience, and the sound has to travel from the speaker to them, adding another delay component.

For a flat stage with seating right at the edge, these extra delays are small and the naive calculation works well enough. But in most venues:

- The stage is raised, and the audience sits at a lower elevation or the speakers are resting on the floor and the audience is seated with their ears about 1m above the speakers.
- Seats are tiered; back rows are elevated.
- Flown speakers are high above the stage, aimed at mid-to-back rows meters away.
- The audience is wide; side seats see the stage from a different angle than center seats.

In all these cases, the distance from the speaker to the listener varies, and the naive algorithm's delays are "wrong" — not wrong in a physics sense (they describe what sound does from speaker to a listener *at the speaker's position*) but wrong for the listener actually hearing them.

## The correction

For each speaker, WFS-DIY lets you specify a **target listener** — a hypothetical first-row listener that speaker is aimed at. The target is given as two distances:

- **Horizontal distance**: how far in front of the speaker (on the speaker's axis) the target listener sits. Always a positive number.
- **Vertical distance**: the height offset. Negative when the speaker is above the listener (flown array) in the case the sound travels downwards; positive when the speaker is below the listener (near field speakers) in this case the sound travels upwards.

With this target specified, the algorithm computes the delay differently. For each source:

1. Compute the distance from the source's virtual position to the target listener (solid black line in the manual's diagram).
2. Compute the distance from the speaker to the target listener (dashed blue line).
3. Subtract the second from the first — that's the delay to apply for this source through this speaker (dashed red line).

The result: sound from each speaker arrives at the target listener at the time corresponding to the source-listener distance, not the source-speaker distance. The listener hears each source in its "correct" arrival timing.

## Why it works

With parallax correction, every speaker sends its signal timed for its target listener. A listener at or near that target position experiences correct arrival timing from that speaker. Listeners at other positions are not perfectly served, but they're close to the target listeners of nearby speakers, so the overall impression remains coherent across the audience.

Without parallax correction, every speaker's signal arrives at a listener as if the listener were co-located with the speaker, which is physically impossible. The brain interprets the resulting timing confusion as smeared or wrongly-placed sources.

## Side effect: floor reflections

Parallax correction also enables **floor reflections** (the Hackoustics feature). Reflections are computed by sending a slightly delayed and attenuated copy of each speaker's signal, simulating the bounce off the stage floor. Without parallax, the geometry for computing the reflection path is ambiguous. With parallax, each speaker's target listener fixes the geometry.

## The coupling trap

Here's the subtle failure mode: if parallax correction drives the computed delays too close to zero for a given array, the array becomes acoustically coupled — every speaker is playing the source at nearly the same time. This produces:

- **Strong high-frequency boosts on-axis** (coherent summation of many speakers).
- **Blurred localization** (the array acts as one big source rather than a distributed array).
- **Beam narrowing** — off-axis level drops sharply.

This happens most often with the flown array when the target listener is placed too far away or at too similar a height across all speakers. The delays compress toward zero and the array fuses.

**Mitigation**: reduce the parallax compensation for the affected array. Move the horizontal target listener closer, or the vertical target listener to a more geometrically natural position. Listen for the coupling symptoms (HF boost, loss of depth) and back off until they disappear.

**Always verify by ear.** The math doesn't tell you when coupling is a problem for your specific audience geometry. Your ears do.

## Suggested starting values

For a typical theater setup:

- **Lower array speakers** aimed at 2 m horizontal, 1.2 m vertical target (first row seated at ~2 m from the speakers, 1.2 m above the speaker if the speakers are on the stage edge).
- **Flown array speakers** aimed at 10 m horizontal, -4 m vertical (middle of the audience, at ear height) or adjusted for the specific seating geometry.
- **Subwoofers**: parallax is less critical because of the omnidirectional low-frequency behavior. Use values similar to the lower array or disable correction for subs.

These are starting points; the final values come from listening.

## Per-array adjustments from a remote control surface

Parallax correction, along with latency, attenuation, and horizontal/vertical offsets, can be adjusted per speaker group from a remote control surface (Android Remote, OSC tablet, Stream Deck, MIDI controller, or directly from an MCP client). This is useful during tuning because you can walk into the audience and make adjustments while listening.
