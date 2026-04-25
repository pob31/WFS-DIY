# WFS System Tuning Procedure

Tuning a WFS system is very different from tuning a conventional PA — and in most ways, simpler. A conventional PA balances many compromises across EQ, coverage, and phase. A WFS system is self-consistent by design; tuning just confirms that the physical deployment matches what the algorithm assumes.

## Reference material

**Do not tune with noise or sine waves.** Pink noise, white noise, and pure tones do not reconstruct well in WFS, and they carry almost no localization information. The brain has nothing to latch onto.

Use a **real, known source** that matches the intended application:

- **For theater**: a lavalier microphone on an actor, speaking as they will in performance.
- **For music**: the primary instrument, played by the musician.
- **For playback-heavy productions**: the recorded material itself.

The reference source tells you whether sources will *localize* correctly, which is the point of WFS — not whether the response is flat, which is a conventional PA concern.

**Measurement microphones are not helpful** for WFS tuning. The only reliable instrument is the trained ear of the engineer, listening to the reference material from different positions in the audience.

## Why WFS tuning is fast

Once you've done it a few times, tuning a WFS system takes a few minutes. There's no array-by-array EQ matching, no exhaustive phase alignment sweep, no hunting for the right crossover frequency between sub and tops. The algorithm handles all of that from the geometry.

What you're verifying during tuning is:

- The relative levels of the arrays.
- The delay between arrays.
- The parallax correction, which adjusts the effective listener target for each speaker.
- The global Haas effect, which maintains precedence for the acoustic sound.

## The four-step procedure

### Step 1 — Lower array alone

Mute the flown array. Listen from the first few rows, close to the lower array.

Place the reference source at a known position on stage. Walk around the first rows and verify:

- The source sounds like it's on stage, at roughly the right horizontal position.
- The overall level is appropriate — quiet enough that the reinforcement doesn't dominate the acoustic sound, loud enough to support it.

Adjust the **lower array's overall level** (usually via its output group attenuation). You might also adjust the **HF damping** if the near speakers sound too bright and draw attention to themselves.

Don't worry about the far rows at this step. The lower array may not reach them well — that's what the flown array is for.

### Step 2 — Flown array alone

Mute the lower array; unmute the flown array. Move to the middle of the audience, beyond the transition zone between where the lower array stops being effective and the flown array takes over.

Adjust the **flown array's overall level** to approximately match the impression you got from the lower array in step 1. This doesn't need to be exact — the arrays will work together, not alternately.

If the flown array sounds too bright or too dull at the far rows, adjust its **HF damping**.

### Step 3 — Both arrays, delay calibration

Unmute everything. Now you listen to both arrays together, and this is the step that takes the most attention.

The goal: when you play the reference source, it should sound like it's coming from the stage, not from high up where the flown array is.

Adjust the **flown array's delay** so the flown sound arrives slightly after the lower array's sound. When correctly aligned, the precedence effect makes the lower array win the localization, and the flown array provides level support without pulling the source upward.

Test at multiple listening positions — front, middle, back, left, right. Test at multiple source positions — stage-left, center, stage-right, upstage, downstage. The delay setting is a compromise across positions; the goal is acceptable localization everywhere, not perfect at any one spot.

If the source sounds "too high up," increase the flown array's delay. If the source sounds "too far back" (coming from behind the stage), the flown delay might be too long or the lower array too loud.

### Step 4 — Fine tuning with parallax and Haas

Now that the coarse balance is right, refine:

**Parallax correction** (horizontal and vertical target listener distances per speaker or speaker group). These parameters adjust the "first listener" each speaker assumes, which shifts the computed delay for every source on that speaker. Fine adjustments here tighten the spatial image.

If sources sound diffuse or poorly localized *in line with a specific array*, try reducing the parallax compensation for that array (the over-compensated case produces near-zero delay spread, which couples the array into a single phantom source).

**Global Haas effect** (in System Config). This is a global delay applied to all reinforced sound, deliberately positioning the amplified sound behind the acoustic sound from the stage. Typical values: 5-15 ms. Too short and the amplified sound competes with the acoustic source; too long and the ear perceives the amplification as a distinct echo. 
Remember digital consoles, the computer processing and the digital Class-D amplifiers with built-in processing introduce some delay also.

**Revisit the array delays** after parallax adjustment — they interact.

Test the tuning by moving the source to different positions on stage, playing both speech and sustained material. If the localization holds consistently, the tuning is done.

## Common issues

- **Can't get the source low enough**: lower array level is too soft relative to flown, or flown delay is too short.
- **Source is clearly localized in the center but not when moved to the sides**: lower array may have too-sparse spacing (violating the three-speakers rule for listeners close to the stage).
- **Source sounds fine standing still but smears when moved**: Doppler on moving sources is inherent to WFS. If excessive, reduce maximum speed or enable curvature-only mode on moving sources.
- **Strong HF coloration on-axis of speakers**: parallax may be over-compensated, creating coupling. Reduce horizontal parallax distance.
- **Nothing sounds "anchored" to the stage**: floor reflections may be disabled. Enable them on the lower array outputs.

## After tuning

Save the configuration. The tuning is specific to the venue and speaker deployment; it won't transfer to a different setup. But within a run of performances in the same venue, the tuning should hold — sanity-check at the start of each day, but a full retune is rarely needed.
