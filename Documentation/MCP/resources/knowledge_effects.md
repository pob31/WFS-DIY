# Effects channels

WFS-DIY has up to 32 **effects channels**. Each one is a spatialised multi-effect: it is fed by inputs (and by other effects), runs the sound through a chain of eleven modules, and plays the result back as a **return** — a source placed on the stage and rendered through the WFS arrays from its own position, like an input.

The return being a real WFS source is the point. A conventional effect send comes back as a flat stereo return; here the echo, the reverb or the chorus comes from somewhere on stage, can move, and is heard as coming from that place everywhere in the audience. This document explains what that is good for and how to build it through MCP.

Effects channels are separate from the **Reverb tab's** reverb (the SDN / FDN / IR room simulation with its nodes around the stage). The room reverb models the venue. An effect channel is a creative or corrective process with its own sound and its own place.

## How an effect channel is built

```
inputs ──send (level, on/off)──┐
                               ├─> feed ──> chain of 11 modules ──> return (a WFS source on stage)
other effects ──send───────────┘
```

**Feed.** Each input has a send into each effect: a level (−92 to 0 dB) and an on/off switch. Every send starts OFF. The feed is geometric, like the reverb feed: the delay and the level also depend on the distance between the input and the effect's position. The Feed section shapes it with orientation, angle on/off, pitch, HF damping and distance attenuation. Effects can feed each other the same way. An effect can never feed itself.

**Chain.** There are eleven module slots, and every effect has all of them, each switched OFF to begin with:

| Token  | Module | What it does |
|--------|--------|--------------|
| `dist`   | Distortion | Drive, shape, bias, pre/post shelving, output, mix, oversampling |
| `eq1`, `eq2` | EQ | Six bands each (shape, frequency, gain, Q, slope) |
| `dyn1`, `dyn2` | Dynamics | Compressor and expander/gate, look-ahead, side-chain filters, make-up |
| `mod`    | Chorus / Flanger | Mode, rate, depth, delay, feedback, voices, shape, through-zero |
| `phaser` | Phaser | Stages (4/6/8/12), centre, spread, rate, depth, feedback |
| `trem`   | Tremolo | Rate, depth, shape |
| `reverb` | Reverb | Models FDN, Plate, Modulated Hall and Shimmer; early reflections; 23 presets |
| `delay`  | Multitap delay | Up to 8 taps, Manual or Pattern (Equal / Dotted / Triplet / Golden), feedback filters, modulation, diffusion |
| `crush`  | Bit crusher | Bits, sample rate, filter, dither |

The **chain order** is its own setting, a permutation of the eleven tokens, so it can be changed without touching any module's settings. Most modules have their own wet **mix**, and there is a chain bypass.

**Return.** The return renders like an input: position (Cartesian, cylindrical or spherical), attenuation law, distance attenuation, HF shelf, per-output mutes and mute macros, per-array attenuation, an optional mute of its reverb sends, latency handling, and its own LFO and AutomOtion movements. Movements are added as **offsets** to the position you set, so the return always comes home. The feed geometry follows the base position.

## What effects channels are for

These are typical uses, each with the tool sequence that builds it. Create the channel first with `effect_create`, which is stopped-only.

**1. A spatial reverb return for a few sources**
A plate or hall that comes from a place on stage, for example upstage centre and a little above, instead of from everywhere.
`effect_fx_reverb_set_reverb_type` (for example `VocalPlate`, `ConcertHall`) → `effect_set_module_on` reverb → `effect_position_set_x/y/z` → `input_set_effect_send_on` and `input_set_effect_send_level` for each input that should be in it. A preset sets the model, early reflections and room in one go; editing any of those values turns the type into `Custom`, while Tone and Mix stay as set.

**2. A delay throw that travels**
Echoes of a voice that appear stage left and drift across the stage.
Delay module on, `effect_fx_delay_set_delay_time`, taps and feedback → place the return → prepare its LFO (`effect_lfo_*`) or AutomOtion (`effect_automotion_*`) settings → send the voice. Starting the movement is the operator's: MCP prepares, it never fires anything time-sensitive.

**3. Echoes that reverberate**
A delay effect feeding a reverb effect: `effect_set_fx_send_on` and `effect_set_fx_send_level` with `source_effect_id` = the delay and `effect_id` = the reverb. Alternatively, inside a single effect, put `delay` before `reverb` in `effect_set_chain_order`.

**4. Widening or thickening a source**
A chorus or flanger return placed beside the dry source, so the doubled part comes from a different place than the original: `effect_fx_mod_set_mod_mode` (Chorus or Flanger) → `effect_set_module_on` mod → offset the position from the source's.

**5. Parallel compression or a sound-design bus**
Dynamics, distortion or bit crusher on a parallel return that is mixed under the dry sources, for example drums sent into a heavily compressed or crushed effect.

**6. Movement and texture**
Tremolo or phaser on an effect whose return circles the audience (an LFO the operator starts), for ambient or electronic material.

**7. A shared treatment for several voices**
Several inputs sent into one effect get the same processing and come from one place. Link groups (below) keep several such effects in step.

## Link groups

`effect_channel_set_link_group` (1–8, 0 = unlinked) and `effect_channel_set_link_mode` (Off, Absolute, Relative). Within a group, a GUI edit to the chain (order, module switches, module parameters) reaches the other members according to each member's mode. Movements, sends, mutes and position never propagate.

**MCP and OSC writes address one channel only and do not propagate**, so set each member over MCP if they should change together.

## The Effects Matrix and loops

- Every input and every effect can feed every effect, except an effect feeding itself, which is refused.
- A loop through several effects (A → B → A) is allowed. The **loop guard** (Effects Settings, `effect_effects_global_set_loop_guard` and its ceiling) mutes a feed that runs away.
- `effect_clear` flushes an effect's tails and delay lines, or every effect's when `effect_id` is omitted. This is the emergency stop for a runaway loop or a tail that must end now. No setting changes.

## Settings (the nine globals)

The link-group names and the mode new channels start with; whether effects feed each other geometrically or through the matrix only; worker threads; the return cushion; the loop guard and its ceiling; the longest delay a chain may buffer; and the compute device of the feed stage. All of them except the loop-guard switch apply at the **next Processing start**.

## Working with effects over MCP

| Goal | Tool |
|------|------|
| See which effects exist | `session_get_state` (the `effects` list, `channel_counts.effects`) |
| Everything about one effect | `effect_get`: sections, chain in playing order with every module parameter, bands and taps, who feeds it and whom it feeds |
| Add or remove an effect | `effect_create`, `effect_delete` (stopped-only); the count itself is `effect_i_o_set_channels` (tier 3) |
| One value | the generated `effect_<section>_set_<param>` tools, or `wfs_set_parameter` with `channel_id` = the effect number |
| An EQ band, a dynamics stage or a delay tap | the generated tool with `instance` (1–2), `effect_band` (1–6) or `tap` (1–8): `effect_fx_eq_set_eq_gain`, `effect_fx_dyn_set_dyn_comp_threshold`, `effect_fx_delay_set_delay_tap_time`… (`wfs_set_parameter` cannot name these) |
| Module order | `effect_set_chain_order` |
| Switch a module | `effect_set_module_on` |
| Input → effect send | `input_set_effect_send_on`, `input_set_effect_send_level`, `input_get_effect_sends` |
| Effect → effect send | `effect_set_fx_send_on`, `effect_set_fx_send_level` |
| Flush tails | `effect_clear` |

Effect ids are dense (1..N), like reverbs: deleting an effect renumbers the ones after it. `mcp_describe_parameters` with `scope="effect"` or `domain="effects"` lists every effect parameter. Writes made over MCP are undoable with `mcp_undo_last_ai_change`, except creating and deleting channels and switching `eq1/eq2/dyn1/dyn2` with `effect_set_module_on`.

Start quietly. A new effect has every module OFF and every send OFF. Switch on the module, set its mix, then raise one send at a time.
