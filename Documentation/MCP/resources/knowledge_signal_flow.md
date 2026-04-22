# Signal Flow in a Typical WFS Session

WFS belongs to the family of object-oriented spatial audio: each sound is treated as an independent input with its own properties, and the system renders for the speaker configuration at hand. The operator does NOT mix per-output; the algorithm does that based on input parameters and array geometry. This document covers how signals flow through a typical session.

## The high-level picture

```
Live mics + DI    ┐
                  ├──→ Console ──→ direct outs post-fader ──→ WFS Inputs
Recorded sources  ┘                                          ↓
                                                         WFS algorithm
                                                          ↓        ↓
Show control (QLab,                              Outputs       Reverb sends
Streamdeck, scripts) ──→ OSC commands ──→ WFS                       ↓
                                              ↑                   external or
                                              │                   internal reverb
                                              │                       ↓
                                              ←─── Reverb returns ────┘

WFS Outputs ──→ console (or direct) ──→ amplifiers ──→ speaker arrays
```

## Inputs

Most signals come from a digital mixing console:

- Microphones, DI boxes, multichannel digital interfaces feed the console.
- The console handles channel-strip processing: dynamics, EQ, fader.
- All channels feed the WFS processor via **direct outs post-fader**. The console is NOT used for summation in this architecture — it's a per-channel router with processing.

Pre-mixing on the console can reduce the channel count to the WFS processor. Sources sharing the same virtual position (e.g., a drum overhead pair, a vocal stack) can be summed on the console before being sent as a single input. This trades flexibility (independent positions) for channel economy.

The audio link from console to WFS processor is typically:

- **MADI or Dante** with a PCIe or Thunderbolt audio interface — lowest latency, most reliable for large channel counts.
- **USB** interfaces — higher latency due to the USB pipeline, with the actual latency often not reported correctly by the driver. Acceptable for smaller channel counts and less latency-critical applications.

## Inside the WFS processor

The processor doesn't mix in the conventional sense. For each input, for each output, it computes:

- A delay (based on geometry).
- An attenuation (based on distance and the input's settings).
- A high-frequency shelf (air absorption, source directivity, output HF damping).
- An optional reflection contribution (floor reflections / Hackoustics).
- An optional level modulation (live source damping if a nearby speaker is in damping range).
- An optional level/height/HF map lookup based on position.

These per-input-per-output values are summed at each output to produce the speaker feed. The whole computation happens continuously — every position change updates the parameters that drive each output's signal.

Reverb feeds receive the same kind of computation as outputs, but instead of going to a speaker, they go to a reverb processor. Reverb returns come back as additional inputs and are processed by the same algorithm to be diffused through the array.

## Outputs

Speaker feeds leave the WFS processor as audio channels. From there:

- **Back to the console**, then to amplifiers. Adds a small amount of latency but offers the option of a "save the show" fallback: if the WFS processor crashes, the operator can momentarily revert to a conventional mix on the console while the system recovers. (In practice, this is rare with a stable system.)
- **Direct to amplifiers**, bypassing the console. Lowest latency, but no fallback path. Used when the console doesn't have enough channels.
- **A mix of both**, with critical channels (front fills, subs) routed back through the console and overflow channels going direct.

## Reverb routing

Two reverb modes:

**External processing**: feeds leave the WFS processor through audio outputs, get processed by an external reverb (console internal effects, outboard hardware, or a multi-effects host), and return through audio inputs back into the WFS processor as reverb returns. Each reverb channel uses one audio output and one audio input.

**Internal plug-in hosting**: reverb feeds are routed to mono VST or AU plug-ins loaded inside the WFS processor itself. No audio interface channels consumed. Adds CPU load.

Future development may include built-in convolution and algorithmic reverb engines, eliminating the need for external processors entirely.

## Control flow

Audio is one part of the picture; control is the other.

OSC is the universal control protocol for the WFS processor. Controllers and show systems include:

- **Show control software** that can trigger cues with audio playback: QLab (Figure 53), Ableton Live (with the WFS OSC devices), Chataigne, Score, custom scripts.
- **OSC-enabled tablets and applications**: TouchOSC (Hexler), various custom remotes, Streamdeck (Elgato) with Bitfocus Companion.
- **Hardware controllers** interfaced via Max/MSP or Pure Data: Roli Lightpad, Sensel Morph, MIDI controllers translated to OSC.
- **Tracking systems**: UWB tags, computer vision, IR-LED systems sending position data over OSC, PSN, or RTTrP.
- **AI assistants** via the MCP server (planned): Claude Desktop, Claude Code, ChatGPT, etc.
- **Console scenes**: a console scene change can trigger an OSC message that recalls a WFS snapshot.

Show structure typically maps OSC cues to scene boundaries. A QLab cue might:

1. Start an audio playback.
2. Send a WFS snapshot recall.
3. Trigger a Move on a specific input.
4. Adjust the LFO state of an ambient source.

All of these happen on a single cue trigger.

## CPU and channel-count budgeting

The processor's CPU cost scales with:

- **Number of inputs** (each one is processed for every output).
- **Number of outputs**.
- **Number of reverb channels** (each functions like an additional input + output).
- **Sample rate**.
- **Floor reflections** — roughly doubles the per-input load for inputs that have them enabled.
- **Live source damping** — small per-input cost.
- **Internal reverb plug-ins** — variable based on plug-in.

A high-frequency CPU with many cores and large cache provides the most headroom. Mid-range CPUs handle moderate channel counts (24 in × 24 out × 6 reverb at 48 kHz, for example) without difficulty.

## Latency budgeting

The total system latency includes:

- Console processing latency (varies by console; typically 1-3 ms).
- Audio interface latency (varies; PCIe/Thunderbolt sub-ms, USB usually 5-10 ms).
- WFS processor's signal vector size (determines per-block latency).
- Reverb processing if external.

The **system latency** parameter in WFS-DIY's master section is added to all delay calculations so that the algorithm can compensate. The **Haas effect** parameter adds an additional global delay specifically to ensure amplified sound arrives after acoustic sound, taking advantage of the precedence effect.

## Stage monitoring

Often overlooked: actors and musicians on stage may feel disconnected from the amplified sound the audience hears. A few small speakers around the stage edge, fed from the WFS system, give performers the same spatial impression as the audience and significantly improve their comfort and timing.

These stage monitors are configured as ordinary outputs in the WFS processor — just speakers in different positions, with their own coverage and parallax settings.
