# Signal Flow in a Typical WFS Session

WFS belongs to the family of object-oriented spatial audio: each sound is treated as an independent input with its own properties, and the system renders for the speaker configuration at hand. The operator does NOT mix per-output; the algorithm does that based on input parameters and array geometry. This document covers how signals flow through a typical session.

## The high-level picture

```
Live mics + DI    ┐
                  ├──→ Console ──→ direct outs post-fader ──→ WFS Inputs
Recorded sources  ┘                                          ↓
                                                         WFS algorithm ──→ Outputs ──→ console / amps ──→ speakers
                                                          ↓     ↑
                                                   Reverb feeds Reverb returns
                                                          ↓     ↑
                                                  Built-in reverb engine
                                                  (SDN / FDN / IR + pre & post processing)

Show control (QLab, Streamdeck, scripts, AI, plug-ins) ──→ OSC ──→ WFS
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

Reverb feeds receive the same kind of computation as outputs, but instead of going to a speaker they go to the built-in reverb engine. Reverb returns come back through the same algorithm to be diffused through the array — they are positioned just like virtual sources, so the wet signal radiates from chosen points in the venue rather than emerging from arbitrary speakers. Conceptual coverage of *why* and the algorithm trade-offs lives in `knowledge_reverb_in_wfs.md`; this document only describes where the audio actually goes.

## Outputs

Speaker feeds leave the WFS processor as audio channels. From there:

- **Back to the console**, then to amplifiers. Adds a small amount of latency but offers the option of a "save the show" fallback: if the WFS processor crashes, the operator can momentarily revert to a conventional mix on the console while the system recovers. (In practice, this is rare with a stable system.)
- **Direct to amplifiers**, bypassing the console. Lowest latency, but no fallback path. Used when the console doesn't have enough channels.
- **A mix of both**, with critical channels (front fills, subs) routed back through the console and overflow channels going direct.

## Reverb routing

The reverb engine is built into the WFS processor. It runs entirely in-process, with no audio-interface I/O consumed and no external plug-in hosting involved. An external loop is still possible for users who prefer their own hardware or DSP — see the *External reverb* section below — but the integrated engine is the default and what everything else in this documentation refers to.

### The integrated chain

For each reverb channel the engine maintains an independent processing node. Audio reaches the engine via per-node lock-free ring buffers fed from the audio callback, and leaves via matching output ring buffers consumed back by the callback.

The chain, in order, is:

1. **Reverb feeds in** — for every reverb node, the WFSCalculationEngine computes per-input feed delay, level, and high-frequency attenuation (the same calculation it does for outputs, just with the reverb's *feed position* in place of a speaker position). The N inputs are summed into the corresponding node's input bus. Per-input *Mute Reverb Sends* zeros that input's contribution to all reverb feeds.

2. **Pre-EQ — per-channel, four bands.** Each reverb node has its own four-band parametric EQ (LowCut / LowShelf / Peak / HighShelf / HighCut, with the OFF state handled by a separate band toggle). Useful for sculpting what each reverb "hears" — for example rolling off bass on a hall reverb but keeping it on a floor reverb.

3. **Pre-compressor — global, per-node envelope.** A single set of threshold / ratio / attack / release applies to every reverb node, but each node tracks its own RMS envelope so a loud event on one input doesn't pump the others. Feed-forward, hard-knee. The post-EQ RMS at this point is also captured per node as a sidechain key for the post-expander further down the chain.

4. **Algorithm stage.** One of three globally selected algorithms runs on every node:
    - **SDN** — N nodes coupled via N×(N−1) inter-node delay paths, geometry-driven, Householder scattering, three-band crossover decay per path, two-stage allpass diffusion.
    - **FDN** — 16 co-prime delay lines per node with a Walsh-Hadamard mixing matrix, three-band decay per line, four-stage input diffusion, an allpass in the feedback path. Nodes are independent.
    - **IR** — partitioned convolution per node (`juce::dsp::Convolution`). Either a shared IR or one IR per node (Per-node IR toggle), with file-level trim and length controls applied to the cached buffer.

   The detailed properties of each algorithm are described in `knowledge_reverb_in_wfs.md`. Switching between algorithms triggers a fade-out → swap → fade-in crossfade of about 50 ms in each direction so swaps mid-show are click-free.

5. **Post-EQ — global, four bands.** Same topology as the pre-EQ, applied uniformly to every node's wet output (a single set of coefficients shared across nodes, with independent filter state per node).

6. **Post-expander — global, sidechain-keyed.** A downward expander whose key signal is the per-node sidechain captured in step 3. When the dry source goes quiet, the wet tail is pulled down so the reverb doesn't bloom alone in pauses. Per-node envelope state, block-rate gain.

7. **Wet level.** A single dB scalar applied uniformly to every node before the engine returns audio to the audio callback.

8. **Reverb returns out — back to outputs.** For every reverb node and every output speaker, the WFSCalculationEngine computes a return delay, level, and HF attenuation based on the reverb's *return position* and the speaker's location. The wet output is then mixed into each speaker's bus exactly like an additional virtual source. Per-reverb-per-output mute is supported via the `reverbMutes` array.

### Tab-level controls that affect the chain

Three header buttons on the Reverb tab tap directly into the engine:

- **Solo Reverbs** — long-press to toggle. Mutes the dry path on outputs, leaving only the wet returns.
- **Mute Pre** — long-press to toggle. Zeros the algorithm input, so existing tails decay naturally but no new excitation enters.
- **Mute Post** — long-press to toggle. Zeros the wet output before the return-routing matrix, so the algorithm keeps running but produces silence at the speakers.

These three are mutually exclusive (engaging one releases the others).

### Parallelization and block latency

The engine processes audio in fixed 256-sample internal blocks (~5 ms at 48 kHz) regardless of the host's block size. Within each block, per-node work is dispatched across a fork-join thread pool (`AudioParallelFor`) sized at `min(hardware_concurrency − 2, numNodes − 1)`, capped at seven workers, with the main thread participating.

FDN and IR parallelize trivially — each node is independent. SDN cannot do that because every node writes into delay paths read by every other node, so it uses a snapshot scheme: each block freezes a `readBasePos` at the start, all nodes read from that snapshot, and write positions advance once after the parallel section completes. The cost is roughly one block of additional latency on the shortest inter-node paths — about 5 ms at 48 kHz — which is acceptable for reverb where end-to-end latency is dominated by delay-line lengths anyway.

### External reverb (still supported)

If a venue prefers to use a console's built-in effect, an outboard reverb, or a third-party plug-in host, the same routing pattern still works: send audio through a regular WFS output, process it externally, and bring the wet signal back in through a regular WFS input that's positioned where the return should appear. This consumes audio-interface channels and adds the round-trip latency of the external path, but lets users keep workflow they already know. The internal engine is then bypassed for that channel by setting its wet level to off or by routing its feeds elsewhere.

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
- **Number of reverb channels** (each functions like an additional input + output for the routing matrix, plus its own algorithm node inside the reverb engine).
- **Sample rate**.
- **Floor reflections** — roughly doubles the per-input load for inputs that have them enabled.
- **Live source damping** — small per-input cost.
- **Reverb engine** — algorithm-dependent. **FDN** and **IR** scale linearly with the number of reverb nodes (independent per node, parallel-safe). **SDN** scales with N×(N−1) inter-node paths plus per-path decay filtering, so its cost rises faster than the others as reverb-channel count goes up. IR cost depends additionally on convolution length and on whether per-node distinct IRs are enabled.

A high-frequency CPU with many cores and large cache provides the most headroom. Mid-range CPUs handle moderate channel counts (24 in × 24 out × 6 reverb at 48 kHz, for example) without difficulty.

## Latency budgeting

The total system latency includes:

- Console processing latency (varies by console; typically 1-3 ms).
- Audio interface latency (varies; PCIe/Thunderbolt sub-ms, USB usually 5-10 ms).
- WFS processor's signal vector size (determines per-block latency).
- Internal reverb engine — runs in fixed 256-sample blocks (~5 ms at 48 kHz) and adds roughly one block of additional latency on the **SDN** path because of the snapshot-based parallel-read scheme. **FDN** and **IR** add no extra block latency beyond the engine's processing block size.
- External reverb processing, when used in place of the internal engine — the round-trip out and back through the audio interface and the external processor.

The **system latency** parameter in WFS-DIY's master section is added to all delay calculations so that the algorithm can compensate. The **Haas effect** parameter adds an additional global delay specifically to ensure amplified sound arrives after acoustic sound, taking advantage of the precedence effect.

## Stage monitoring

Often overlooked: actors and musicians on stage may feel disconnected from the amplified sound the audience hears. A few small speakers around the stage edge, fed from the WFS system, give performers the same spatial impression as the audience and significantly improve their comfort and timing.

These stage monitors are configured as ordinary outputs in the WFS processor — just speakers in different positions, with their own coverage and parallax settings.
