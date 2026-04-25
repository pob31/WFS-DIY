# Live Source Damping or Live Source Taming — Reducing Amplification Near Loud Sources

Live source damping is a per-source feature that reduces the contribution of an input to the speakers physically close to it. This document explains when to use it and how its parameters work.

## The problem it solves

In a WFS system with speakers distributed around the stage, a sound source at a specific location will naturally be sent to every speaker including the ones right next to it. For a quiet source (a soft-spoken actor, a distant instrument) this is fine and desirable — the nearby speakers provide intimate reinforcement.

But for a loud acoustic source, the nearby speakers cause problems:

- **Redundant amplification.** An opera singer projecting unamplified is already more than loud enough for the front rows. Adding their voice to the front fills creates discomfort for the audience.
- **Musician monitoring issues.** Musicians on stage often complain about the sound coming from the rear of PA speakers near them. The rear of a typical front-fill speaker is not well controlled, and the output intended for the audience leaks back onto the stage.
- **Feedback risk.** A microphone on stage near a speaker, receiving the same source, can produce feedback. Reducing the level of that source specifically in that nearby speaker breaks the feedback loop without attenuating elsewhere.

## How it works

When live source damping is active on an input, and a speaker is within a configurable radius of the input's position, the level sent from that input to that speaker is attenuated. The distance calculation always includes elevation (3D distance, not 2D ground-plane distance).

Four parameters control the damping:

**Radius** — the distance out to which attenuation is applied. Beyond the radius, no attenuation. Default 3.0 m.

**Fixed attenuation** — the maximum attenuation applied when the source is exactly at the speaker's position. Typical values 3 to 12 dB. The speaker's directivity model may also attenuate the source when it's very close.

**Shape** — the profile of attenuation as distance varies between the source position and the radius boundary. Four options:
- **Linear**: progressive, constant slope.
- **Square (x²)**: pronounced dip close to the source, gentler at the radius boundary.
- **Log**: pronounced drop as soon as the speaker enters the damping zone, gentler closer to the source.
- **Sine**: gentle at both ends, steepest in the middle — close to linear but with smoother transitions.

**Peak and slow compression** — two dynamic attenuators, each with a threshold and a ratio. These allow the system to provide full amplification when the source is quiet and reduce amplification only when the source gets loud. This is useful for sources that need reinforcement during intimate moments but not during projected passages.

## When to use which shape

- **Linear** — general-purpose default. Smooth behavior across the damping zone.
- **Square** — when you want strong suppression very close to the source (e.g., feedback mitigation), with speakers slightly farther away barely affected.
- **Log** — when any speaker inside the radius should be significantly attenuated, regardless of distance. Useful for creating a clear "no reinforcement zone" around a loud source.
- **Sine** — when you want smooth perceptual transitions as a source moves into and out of a speaker's damping range. Good for moving sources.

## Application examples

**Opera singer at the edge of the stage.** Enable damping with a radius of 2-3 m and fixed attenuation of 6-10 dB. The front-fill speakers near the singer contribute less, so audience members in the first rows hear primarily the acoustic voice with subtle WFS placement from farther speakers.

**Musician complaining about stage monitors.** Place the musician as an input, enable damping with a radius that covers the speakers in front of them (usually 1-2 m), and moderate attenuation. The rear of those speakers is quieter.

**Feedback prevention.** When a microphone picks up a speaker near it, enable damping with a small radius and strong attenuation. The feedback loop is broken for that specific source-speaker combination.

**Gradually quiet scenes.** Use the peak compression feature: set a threshold below the loud-passage level. During quiet speech, full reinforcement. During shouted lines, the nearby speakers back off dynamically.

## When NOT to use it

- **Sources that should always receive full reinforcement** (playback tracks, effects, sounds without a visible stage source). These have no acoustic presence to defer to; damping just reduces their level.
- **Reverb return channels**. These don't have a "live source" to defer to.
- **Quiet sources already barely audible acoustically**. Damping makes them less audible, which is the opposite of what you want.

## Per-output override

Each output has an "Enable Live Source Attenuation" toggle. Even if an input has damping enabled, an output can opt out — meaning that speaker always receives the input's full level regardless of distance. This is useful for:

- **Subwoofers**: should not be attenuated based on proximity. They're few and need to contribute consistently.
- **Far-field speakers**: not close enough to matter but the output toggle makes the intent explicit.
- **Flown arrays above the audience**: typically not affected by stage-level damping geometry.

Per-output opt-out is the correct pattern when you want damping enabled per-input but specific outputs should be excluded.
