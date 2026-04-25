# How Humans Localize Sound

Understanding how the ear and brain identify the position of a sound source is foundational to why wave field synthesis is designed the way it is. This document covers the main perceptual mechanisms at work.

## Binaural localization

Two primary cues tell the brain where a sound is coming from: the **time difference** between the two ears, and the **level difference** between them.

A sound arriving from directly in front reaches both ears simultaneously and at equal level. A sound arriving from the side reaches the near ear first, slightly louder. The brain interprets these small differences to place the source.

The relevant cue depends on frequency:

- **Low frequencies** have wavelengths much larger than the head. Level differences between ears are negligible, but the small phase shift between the two signals is perceptible. Low precision localization.
- **Mid frequencies** have wavelengths comparable to the size of the head. The brain uses mostly the envelope difference — the difference in overall level over time.
- **High frequencies** have wavelengths shorter than the spacing between the ears, so the brain cannot count cycles, but the far ear hears a noticeably less bright sound because the head shadows high frequencies.

Secondary cues include head movement (the brain integrates how rotation affects the signals at both ears) and the spectral coloration imposed by the shape of the outer ears, head, and shoulders — the Head-Related Transfer Functions, HRTFs. HRTFs are what allow front/back discrimination and some above/below discrimination.

Sounds arriving from directly to the left or right are localized imprecisely; sounds from directly in front or behind cannot be distinguished by ITD/ILD alone. This is why humans turn their heads when trying to localize — head movement resolves the ambiguity.

## The precedence effect

In any real space, a sound from a source reaches the ears as a direct sound followed by many reflections off surrounding surfaces, each delayed by a few milliseconds relative to the direct sound.

The brain suppresses most of these reflections when judging localization. It uses the direct sound — the earliest arrival — to determine where the source is, and treats the later arrivals as acoustic information about the room rather than as separate sources.

This is the **precedence effect** (sometimes called the "law of the first wavefront"). Only when a reflection arrives very late (many tens of milliseconds after the direct sound) does it become perceptible as a distinct echo.

The precedence effect has two consequences relevant to WFS:

1. **A listener off-center between two identical stereo speakers hears the closer speaker as the source.** The far speaker's signal, arriving slightly later, is treated as a reflection. This is why a conventional two-speaker PA fails to give a good soundfield to most of the audience — only people on the centerline experience phantom-center stereo.

2. **A microphone and a recording played through speakers cannot reproduce what a live listener experienced**, because the microphone captured everything (direct + all reflections) without the brain's selective suppression. The recording then adds the playback room's reflections on top. This is why recording studios control acoustics carefully.

## The Haas effect

The Haas effect is the deliberate use of the precedence effect. When the same sound is played from two speakers with one delayed by roughly 5-30 ms, listeners perceive the sound as coming from the earlier speaker, even if the delayed speaker is louder (within limits — typically up to about 10 dB louder before the illusion breaks).

In WFS sound reinforcement, the Haas effect is used to let the acoustic sound from an actor or instrument reach the audience first, with the amplified sound arriving slightly later for level support. The reinforced system adds loudness without disturbing the localization cue from the real acoustic source.

## Stereophony

The word "stereophony" comes from Greek roots meaning "solid" and "sound." It does not refer to two-channel playback specifically; it refers to the perceptual quality of sound sources feeling *tangible*, occupying a location in space as real objects do.

This is the goal of WFS: every source sounds like it comes from a real place, for every listener, not just for the person standing at the center sweet spot.

## Spectral masking

When complex sounds contain both loud and quiet components at nearby frequencies, the loud components mask the quieter ones — the ear cannot hear the quieter frequencies in their presence. Conventional mixing addresses this by EQ'ing sources to "make room" for each other, at the cost of the natural timbre of each source.

In a WFS context, sources occupying different spatial positions are partially separated by the brain into different perceptual streams, which reduces the need for aggressive EQ. Sources can retain more of their natural timbre because they're not competing in the same perceptual location.

## Implications for WFS design

Given all of the above, a WFS system is designed to:

- **Preserve the ear's native localization cues** (ITD, ILD, and spectral shaping) by faithfully recreating wavefront timing and level from each source's position.
- **Provide three or more speakers audible to every listener** from at least one array, so the brain has enough spatial information to localize.
- **Use the precedence effect in its favor** — amplified sound arriving slightly after acoustic sound preserves the perception of the real source on stage.
- **Avoid processing that disrupts localization cues**: minimal EQ, minimal compression on individual sources, avoidance of time-smearing effects.

The payoff is that every listener, regardless of position, hears sources where they visually appear to be — which is rare in conventional sound reinforcement.
