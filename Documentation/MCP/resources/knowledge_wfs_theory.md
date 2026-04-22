# Wave Field Synthesis — Theory and Algorithm

Wave Field Synthesis (WFS) is a spatial audio technique that reconstructs the wavefront of each sound source independently, allowing every listener in the audience to perceive each source as coming from its true position in space — not from a phantom center or a sweet spot.

## The core idea

When a sound source emits sound, it produces an expanding wavefront — like ripples on a pond from a thrown stone. In the 17th century, Christiaan Huygens showed (for light) that any wavefront can be reconstructed by an array of many smaller secondary sources, provided they are properly phase-aligned.

Applied to audio: an array of speakers can recreate the wavefront of a virtual sound source. Each speaker contributes its portion of the wavefront, delayed and attenuated to match what the listener would have heard from the actual source position.

Different WFS flavors exist. Some follow Huygens' theory strictly — infinite arrays of tiny sources with precise phase alignment. This is impractical for live sound reinforcement: the arrays are physically huge, visually obtrusive, and reintroduce a sweet spot where everything reconstructs perfectly (with degraded performance elsewhere).

WFS-DIY takes the pragmatic approach: fewer, larger speakers with good coverage patterns, placed where they make physical and visual sense in a real venue. The mathematical reconstruction is approximate rather than exact, but the perceptual result is excellent across a wide listening area — and it accommodates flown arrays, sub placements, and venue constraints that strict WFS cannot.

## What the algorithm computes

For each **source**, for each **speaker**, the system computes:

1. **A variable delay** that matches the travel time from the source's virtual position to the listener associated with that speaker.
2. **An attenuation** that reflects the distance from the source to the speaker (speakers farther from the source contribute less).
3. **A high-frequency shelf above 800 Hz** with a gentle slope, simulating air absorption.
4. **An angular attenuation** based on both the source's directivity (a source facing away from the audience should have less presence) and the speaker's orientation relative to the source (a speaker pointing away from the source's location shouldn't amplify it).

For reflections (floor reflections, for example), similar calculations apply with additional jitter on the delay line to prevent artificial-sounding exactness.

## Design objectives

A WFS system aimed at live performance prioritizes:

- **Consistent visual-auditory correspondence for all listeners.** A source at stage-left is heard at stage-left by everyone in the audience, not just the center seats.
- **Smooth continuous source movement.** Sources can traverse the stage without perceptual jumps.
- **Audible levels across the full audience** — no dead zones.
- **Minimal signal degradation** — as little filtering and processing as possible to preserve natural timbre.
- **Flexible speaker placement** — touring-friendly, adaptable to frontal stages, surround configurations, or domes.
- **Remote control and automation** — show control integration (QLab, Ableton Live, etc.) for programmed movements and scene recall.
- **Affordability and accessibility** — usable by small theater and dance companies, not just major opera houses.

## Trade-offs

A few trade-offs are inherent to this approach:

- **Doppler effect on moving sources.** Because the algorithm updates delays continuously as sources move, pitch shifts according to the rate of change. This is physically correct (the acoustic world does this too) but can be noticeable with fast-moving sources carrying steady tones. Mitigation: reduce maximum speed, use smoother acceleration curves, or enable "curvature only" mode on the source to minimize absolute delay changes.
- **No focused sources between the speaker array and the listeners.** A focused source — a virtual source perceived as being *in front of* the speaker array — requires the inverse wavefront calculation and is difficult to achieve robustly in live sound without artifacts. WFS-DIY places virtual sources behind the array (from the listener's perspective), which covers the vast majority of theatrical use cases.
- **Array size matters.** An array that's too short or too sparse leaves spatial holes. Guidelines for array design are covered in the Array Design document.

## What the processor does NOT do

- It does not "pan" sources between speakers in the conventional sense. Every source is sent to every speaker (usually), with different delays and levels. There is no "this source is assigned to speaker 3."
- It does not mix per-speaker. The mix is per-source; speakers are a function of the source mix and the array geometry.
- It does not replace conventional mixing artistry. It lets the operator focus on source-level decisions (timbre, dynamics, position, movement) while the algorithm handles the spatial render.

## The object-oriented mixing paradigm

WFS belongs to the family of **object-oriented spatial audio** techniques, which also includes Ambisonics, binaural rendering, Dolby Atmos, VBAP, and DBAP. The defining characteristic: each sound is treated as an independent object with position and other properties, and the system renders it for the specific speaker configuration at hand.

The operator's role shifts from "mixing for each output" to "describing each source and its behavior." The system handles translation to the physical speakers. This is the conceptual leap when coming from conventional PA mixing.
