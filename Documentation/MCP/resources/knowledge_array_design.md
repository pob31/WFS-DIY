# Designing Speaker Arrays for WFS

Speaker array design is the single most important factor in how well a WFS system performs in a given venue. This document covers the principles and practical guidelines.

## The three-speakers rule

**Every listener should be able to hear at least three speakers of at least one array**, with enough level to contribute meaningfully to the spatial image.

With only one speaker audible, the listener cannot localize — they just hear a speaker. With two, they can get rough horizontal cues but not distance or height. Three is the minimum for reliable WFS reconstruction.

This rule determines maximum speaker spacing relative to listener distance.

## Maximum spacing formula

For speakers pointing toward the audience, the coverage angle and the distance to the nearest listener determine how close speakers must be to each other:

```
max_spacing = nearest_listener_distance × tan(coverage_angle / 2)
```

Examples:

- 120° coverage speakers, first row 1 m away → max 1.7 m between speakers.
- 120° coverage speakers, first row 2 m away → max 3.4 m between speakers.
- 90° coverage speakers, first row 1 m away → max 1 m between speakers.

For speakers flown above the stage, the "nearest listener" is the closest row in their coverage pattern, which may be several meters from the speaker. This relaxes the spacing constraint for flown arrays.

## Typical array structures

A working WFS setup for live performance usually has:

**Lower array** — speakers at stage level or just above the proscenium, close to the first rows. Provides the foundation of the soundfield and anchors sources to the floor. When actors talk quietly on stage, this array does most of the perceptual work. Because it's close to the first rows, speakers are numerous and closely spaced.

**Flown array** — speakers hung above the stage, aimed at the mid-to-back of the house. Extends coverage where the lower array can't reach and contributes an impression of vertical space. Speakers can be more widely spaced because the nearest listener is farther away. Beware: speakers flown too high with steeply raked seating may leave high-frequency holes.

**Subwoofers** — typically 2-4 speakers, placed at the sides or under the stage. These operate outside the strict WFS coverage model (they're omnidirectional at low frequencies) and are usually configured with reduced distance attenuation so that their contribution to each source doesn't drop off as sharply.

**Optional surround/above speakers** — for full immersion. Can be aimed at venue walls for indirect diffuse sound (useful for reverb returns) or placed above the audience. Each such speaker adds CPU load and requires its own position and orientation calibration.

**Optional stage monitors** — small speakers aimed at the performers, so they can perceive the amplified soundfield similarly to the audience. Often overlooked, but important for actor and musician comfort.

## Venue size guidelines

These are starting points, not prescriptions.

**Small venues** (up to ~200 seats, tiered or standing): 6-12 lower array speakers with coaxial or elliptical/conical horn dispersion. 250 W per speaker indoors. 90° coaxial speakers have a practical throw of about 10 m in good acoustics.

**Mid-sized venues** (200-1000 seats): 8-16 lower array speakers plus 4-8 flown speakers. Asymmetric horn speakers (120° below, 60° on-axis, little above) are particularly useful for the flown array because they project to the far rows without spraying high frequencies up into the venue's ceiling acoustics. 400 W per speaker indoors.

**Large venues and outdoor stages**: multiple line-arrays with narrow coverage on top and wider below. At this scale, line-array principles combined with WFS positioning yield the best result.

## Coverage patterns

**Identical speakers within an array.** Mixing speaker models within a single array creates level and timbre inconsistencies as sources move. Different arrays can use different speakers, but each array should be homogeneous.

**Straight or curved arrays** are both supported. Curvature can help match the venue's seating curve. The algorithm does not care about the array's shape; it cares about each speaker's position and orientation.

**Avoid aiming speakers at hard reflective surfaces.** The location of the speaker is given away by the reflections it creates in the room. A flown speaker aimed at a hard back wall will betray itself acoustically, breaking the spatial illusion. Venue acoustics matter even in a well-designed WFS system.

## Symmetrical horns for flown arrays

Asymmetric horns (wide below, narrow on axis, little above) are a good fit for flown speakers because:

- The wide coverage below reaches the closer rows that the lower array doesn't fully cover.
- The narrower coverage on-axis projects efficiently to the far rows.
- Little energy above means less spill into the venue's upper acoustics, reducing unwanted reflections.

If such speakers aren't available, coaxial or symmetrical horn speakers work too, with more attention to the flown array's aim.

## When to add immersive speakers

Adding a few speakers to the sides, behind, or above the audience is worth considering for any non-trivial production. Even if no "direct" content is sent to them, they can carry diffuse reverb returns that restore acoustic depth and mask the location of the front speakers. This often makes the difference between "I'm listening to a PA" and "I'm inside an acoustic space."

Dome configurations (full 3D coverage above and around the audience) require a lot of equipment and CPU, and are usually only justified for specific dramatic or experiential productions.

## Common pitfalls

- **Under-powered front fills**. The lower array carries most of the spatial weight in quiet scenes. Skimping on these is the most common failure mode.
- **Flown array too high**. Above a certain angle, the flown array can't relay the lower array in the near rows — the first rows get no vertical coverage from either array.
- **Asymmetric spacing**. Keep spacing roughly uniform within an array. A single wide gap creates a localization hole that's immediately audible when a source passes through it.
- **Ignoring subwoofer placement**. Subs that are too asymmetric or too few create phase issues at crossover that degrade localization in the lower-mid range.
- **Over-deploying above and behind the audience** without a clear purpose. Adds complexity and CPU load for marginal perceptual gain, unless used deliberately for diffuse reverb.

## The Wizard of OutZ

WFS-DIY has an output positioning helper called the Wizard of OutZ with editable presets for common array configurations. Use it as a starting point for the lower array and flown array positioning, then adjust per-speaker for the actual deployment.
