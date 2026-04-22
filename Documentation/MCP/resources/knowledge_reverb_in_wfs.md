# Reverb in a WFS Context

Reverb serves a different purpose in WFS than in conventional PA mixing. This document covers why reverb matters here, how the system's reverb architecture works, and best practices.

## Why reverb matters in WFS

In a conventional stereo or multichannel system, reverb is typically applied for artistic reasons — to place a source in a virtual space, to add depth, to glue a mix together. In WFS, reverb has all of those purposes plus one more that's central to the system's perceptual integrity:

**Reverb masks the reflections from the speakers themselves.**

The ear uses early reflections not just to estimate room size but to locate *sources* — including the speakers producing reinforced sound. When a speaker is placed on the floor, its floor reflection arrives at listeners' ears a few milliseconds after the direct sound. The brain uses this reflection pattern to figure out where the speaker is, and having done so, locks all of the speaker's output to that physical location.

The result: without some additional reverb, listeners can subconsciously identify the row of speakers, and all sources end up sounding like they're coming from the speakers rather than from the intended virtual positions. Static sources (playback tracks, sound effects without movement) suffer most; as soon as a source moves, the illusion strengthens.

A modest amount of spatial reverb breaks this identification. The ear can no longer cleanly separate the source from the surrounding reflections, and the brain accepts the WFS-provided position as the source's true location.

## Reverb feeds and returns

WFS-DIY's reverb architecture has two channel types:

**Reverb feeds** — mono sends from inputs to reverb processors. Each feed has a virtual position in the space, just like an output. When an input's signal is sent to a reverb feed, the amount sent depends on the input's position relative to the feed's position (following the same coverage and attenuation model as outputs).

This means the mixing into the reverb is spatial: an input near the stage-left reverb feed sends more signal to that feed than to a stage-right feed. Different virtual "rooms" can be associated with different parts of the stage.

**Reverb returns** — mono inputs from reverb processors, routed back into the WFS system as regular inputs. Each return has a virtual position, so the reverberated signal is diffused through the speaker array as if it were a virtual source in that position.

Returns cannot be routed back into feeds. This is a design decision to prevent accidental feedback loops.

## Feed and return positioning

Where should reverb feeds be placed?

- **For small stages**: one or two feeds centered on the stage work fine.
- **For wide stages or in-the-round**: multiple feeds distributed around the performance area, each picking up nearby sources.
- **For varying scenes**: use multiple feeds with different characteristics (a dry room feed, a wet hall feed) and route inputs to them based on scene requirements.

Where should reverb returns be placed?

- **Spread returns widely.** A single centered return fails to take advantage of WFS's spatial capabilities; two returns symmetric left-right give basic spaciousness; four to six distributed around or above the audience produce immersive reverb.
- **Returns aimed at walls or ceiling**: when returns are routed through speakers aimed at reflective surfaces rather than directly at the audience, the resulting sound is naturally diffuse without any processing doing that work.

Feed and return positions are independent; they do NOT have to be co-located, though they can be.

## Processing options

Reverb can be processed two ways:

**External processor** — feeds are sent out to the audio interface, processed by an external reverb (console built-in, outboard hardware, or a multi-effects application like Waves SuperRack or Audioström Live Professor), and the returns come back via the audio interface. Requires one audio input and output per reverb channel. Offers access to high-quality commercial reverb plug-ins and any outboard gear.

**Plug-in host** — feeds are processed by mono VST or AU plug-ins loaded inside the WFS application. No audio interface channels consumed. Simpler routing. Requires some CPU overhead. Some plug-ins (notably certain Waves plug-ins) don't expose their parameters for per-channel copy, which complicates setup when duplicating settings across channels.

Future development may add built-in impulse response (convolution) and algorithmic spatial reverb — these are on the roadmap but not required for current deployments.

## Feed settings

Reverb feeds share most parameters with outputs:

- **Latency/delay**: usually small. Not normally adjusted unless aligning external processing latency.
- **Feed attenuation**: overall level sent to the reverb processor.
- **Position and orientation**: where this virtual "microphone" is in the space.
- **HF damping**: air absorption model; affects how sources at different distances contribute.
- **Distance attenuation factor**: scale of the attenuation-with-distance curve.
- **Minimal latency enable**: whether this feed participates in the minimal-delay pool (usually off for reverbs).

Feeds are automatically in mute group 6 and cannot be moved out of it. This ensures a single mute group covers all reverb sends, for fast toggling during rehearsals.

## Return settings

Reverb returns behave like regular inputs with a restricted parameter set:

- **Latency/delay, attenuation, position, directivity** — same as input channels.
- **Height factor, distance attenuation** — same as inputs.
- **Minimal delay** — optional.
- **No movement, no LFO, no tracking** — returns are stationary by design.

## Typical reverb setup for theater

- 4 to 6 reverb channels.
- Feeds placed as two to three virtual "stage microphones" — one center-stage, one stage-left, one stage-right.
- Returns placed as four surround speakers or distributed on the upper flown array.
- Reverb processor: either the console's built-in (for simple cases) or a plug-in host running a quality plate/hall reverb.
- Return level dialed in to taste — usually modest (-20 to -10 dB relative to direct sound).

The spatial reverb mix comes for free from the positions; no additional mixing work is required beyond setting the reverb algorithm's parameters.

## Common pitfalls

- **Reverb returns too loud**: overwhelms the direct WFS image, listeners hear mostly reverb, localization suffers.
- **Only one reverb return**: defeats the purpose; returns should be spatially distributed.
- **Feeding stage microphones directly to reverb at equal level across all feeds**: this makes feed positioning pointless. Let the positions do the mixing.
- **Using WFS feeds as reverb sends when conventional sends would be simpler**: if the spatial aspect doesn't matter (e.g., a single stereo delay on a playback track), just use the console's sends and bypass the WFS reverb architecture.
- **Placing returns in the same positions as sources**: reverb should feel diffuse and different in position from the sources that caused it. Spread returns to positions sources don't normally occupy.
