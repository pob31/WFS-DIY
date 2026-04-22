# Floor Reflections (Hackoustics)

Floor reflections simulate the bounce of a sound off the stage floor, adding a delayed and attenuated copy of each source to each speaker's output. This document covers why it matters and how to configure it.

## Why simulated floor reflections

In a real acoustic space, a sound from a source reaches the listener both directly and via reflections off nearby surfaces — most immediately, the floor the source is standing on. The ear uses these reflections subconsciously as a tangibility cue: they tell the brain that the source is a real physical object producing a wavefront that interacts with the world.

When sound is reinforced through speakers, this cue is missing. The amplified sound comes directly out of the speakers with no floor reflection. The brain notices (without being able to articulate it) that something is "off" — the source feels disembodied, floating, artificial.

Adding simulated floor reflections to each speaker's output restores this cue. The source regains perceptual weight.

## When it matters most

**Recorded playback.** Sources that never had a physical presence on stage benefit the most. Music tracks, sound effects, foley — all feel more tangible with floor reflections.

**Amplified live sources.** Lavalier microphones on actors, DI'd instruments — the reinforcement layer gains physicality.

**Intelligibility.** Not obvious but real: speech with floor reflections is more intelligible because the brain accepts it as a real source and engages its full localization machinery, including head-tracking and source separation.

## When it matters less

- **Very quiet scenes** where the acoustic sound dominates; the reinforcement is barely audible anyway.
- **Heavily processed sources** (distorted synths, abstract sound design) where tangibility isn't the goal.

## How it's computed

For each source and each speaker, the algorithm computes:

1. A delay based on the path length from source → floor bounce → listener, minus the direct path (which is the normal WFS delay).
2. An attenuation based on the extra distance traveled.
3. Optional filtering (low-cut and high-shelf) to shape the reflection's timbre.
4. A jitter component (diffusion) to avoid the reflection sounding mathematically pristine.

The result is a very short "early reflection" (typically a few milliseconds after the direct sound, 3-10 dB below it) that adds perceptual depth without being heard as a distinct echo.

**This only works correctly if parallax correction is properly set up**, because the floor bounce geometry depends on knowing where the target listener is. Without parallax, floor reflections may arrive at the wrong times and sound artificial rather than supportive.

## Parameters

**Attenuation** — overall level of the reflection relative to the direct sound. Typical range -3 to -10 dB. Default -3 dB.

**Low-cut filter** — removes low-frequency content from the reflection. Floors don't reflect bass efficiently in most cases, and low-frequency reflections muddy the sound. Typical cutoff 100 Hz.

**High-shelf filter** — attenuates high frequencies in the reflection. Real floor reflections lose high frequencies both because of absorption at the surface and because high frequencies are more directional and scatter differently. Typical setting: 3 kHz shelf with -2 dB gain and a moderate slope. Carpet or soft stage surfaces call for more aggressive HF attenuation.

**Diffusion** — adds jitter to the reflection's delay. If the delay were mathematically exact and constant, the reflection would sound unnaturally perfect. A small amount of uncorrelated delay variation (roughly 20% by default) adds naturalness. Pure tones and noise sources may sound strange with diffusion — reduce or disable it for those.

## Per-output enable/disable

Each output has an "Enable Floor Reflections" toggle. Typical usage:

- **Enable on the lower array**: these speakers are near the stage floor and benefit most from the reflection simulation.
- **Disable on subwoofers**: the low-cut filter would attenuate most of their output anyway, and the CPU cost isn't worth it.
- **Disable on flown speakers**: they're above the stage floor, not radiating toward it; the reflection model doesn't apply cleanly.
- **Disable on surround/above speakers**: no floor interaction in their coverage.

## CPU cost

Floor reflections approximately double the CPU load for the affected inputs, because each speaker produces both the direct path and the reflection. Before enabling them in a live context, verify that the system has headroom.

On a mid-range modern CPU with 24 inputs and 24 outputs, floor reflections on all inputs are usually fine. On a more loaded setup (64 inputs, many reverb channels), you may need to be selective about which inputs have reflections enabled.

## Per-input mute of reverb sends

A related parameter, "mute reverb sends" on the input, cuts the input's contribution to all reverb channels without affecting its direct output. Useful when a particular input (typically a playback track that's already reverberant) should not feed the spatial reverb engine.

## The name "Hackoustics"

The feature is named "Hackoustics" in the UI because it's a pragmatic acoustic hack — not a physically accurate simulation of a specific floor material, but a perceptual nudge that makes sources feel more real. The underlying principle is drawn from room acoustics, but the implementation prioritizes controllability and taste over physical fidelity.
