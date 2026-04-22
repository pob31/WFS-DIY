# Level Maps and Gradient Maps

Level maps allow parameters to be modulated automatically based on a source's position on stage. WFS-DIY supports three maps per input: **level** (audio attenuation), **height** (vertical position modulation), and **HF damping**. This document covers what they do and how to use them.

## The concept

A level map is an image or gradient painted in a 2D coordinate frame matching the stage. As a source moves through the stage, its x and y coordinates are used to look up a value in the map, and that value is combined with the source's other parameters.

The image can be:

- A monochrome gradient (light to dark = parameter value high to low).
- A color image (RGB combined into a grayscale value).
- An inverted image (for negative maps).
- A shape composed from primitives (rectangles, ellipses, etc.) built in the gradient map editor.

Each input has its own set of maps; they are per-source, not global.

## The three maps

### Level map

Modulates **audio level** based on position.

- **White = no attenuation (0 dB).**
- **Black = full attenuation (-∞ dB, effectively muted).**
- Gradients between white and black produce proportional attenuation.

Applied as an additional attenuation on top of the normal distance-based level calculation.

### Height map

Modulates **effective height** based on position.

Two modes:

- **"0m to Stage Height"**: black = floor, white = full stage height. Useful for matching set geometry — stairs, platforms, raised sections.
- **"−Stage Height to +Stage Height"**: gray = no offset, white = upward, black = downward. Useful for creative effects where sources drift above or below the physical stage.

The height value is combined with the source's manual height setting.

### HF damping map

Modulates **high-frequency attenuation** based on position.

- White = no attenuation.
- Black = full attenuation (up to the configured maximum, default -6 dB).

Combined with the source's inherent HF shelf and the output-level HF damping calculation.

## Application examples

### Muting off-stage

An actor on a wireless mic walks off stage into the wings. Rather than a technician having to mute the channel at the right moment, a level map with the stage area white and the wings black handles the transition automatically. A soft gradient at the boundary creates a smooth fade.

### Room-based processing

A theatrical set with multiple rooms (parlor stage-right, kitchen stage-left, hallway upstage) can be modeled by routing the same actor input to several parallel channels, each with different reverb and EQ insert, and using complementary level maps: the parlor channel is white in the parlor area, black elsewhere; the kitchen channel is white in the kitchen area, black elsewhere. The actor's voice automatically switches processing as they move between rooms.

### Matching set height automatically

A musical with a staircase as a set piece: put a black-to-white gradient on the height map along the staircase axis. As the performer moves up the stairs, the source's rendered height matches, without anyone tracking elevation manually.

### Distance illusion

A grayscale gradient from black (downstage) to white (upstage) on the HF damping map exaggerates the perception of distance as a source moves upstage. Sources farther back sound duller, as if the air or walls were attenuating high frequencies. Useful for dramatic "walking into the distance" effects.

### Creative and non-realistic uses

Nothing requires the map to be realistic. A map with sharp bands can cause sources to pulse as they move through; a map with bright spots can create "hot zones" where sources become louder or clearer; complex patterns can produce unpredictable position-dependent effects for sound design.

## Gradient map editor

WFS-DIY has a built-in editor for creating maps from shape primitives rather than loading image files. Shapes include rectangles, ellipses, diamonds, triangles, hexagons, and stars. Each shape has:

- Position (x, y in normalized coordinates).
- Rotation, scale.
- Fill value (0 = black, 1 = white).
- Fill type (solid, linear gradient, radial gradient).
- Blur (edge softness).
- Ordering within a layer.

Multiple shapes on a layer combine; multiple layers can be enabled or disabled independently. This lets you build complex maps from simple pieces without leaving the application.

## Controls

Each map column has:

- **Enable/disable toggle** — the left-most button at the top.
- **Clear** — removes the loaded image.
- **Load** — opens a file dialog.
- **Flip X / Flip Y** — mirrors the image horizontally or vertically.
- **Invert** — uses the negative of the image values.
- **Current value display** — shows the instantaneous value at the source's position.

## Performance notes

Level maps are inexpensive — they add a small lookup per audio block per input. Enabling maps on all 64 inputs has negligible CPU cost. No reason to be conservative here.

## Combining with other movement features

Maps are evaluated in addition to all other position-affecting features:

- **Tracking** sets the source's position from external data; maps apply based on the tracked position.
- **LFO and Jitter** add periodic or random movement; maps apply based on the instantaneous position including those modulations.
- **Move commands** (one-shot trajectories) also update the position in real time; maps follow.

This means level maps can be used alongside tracking: e.g., actor is tracked, level map mutes the mic when they step off-stage.
