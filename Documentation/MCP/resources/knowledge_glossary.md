# WFS-DIY Glossary

App-specific vocabulary the AI is likely to encounter in tool descriptions, parameter names, and operator queries. Most terms here are either WFS-DIY coinages, distinguish similar-sounding parameters from each other, or carry a meaning that differs from common audio-engineering use. Fetch this resource once at the start of a session to ground vocabulary, then rely on it without re-fetching.

For deeper conceptual coverage, see the topic-specific resources (`wfs://knowledge/wfs_theory`, `wfs://knowledge/reverb_in_wfs`, `wfs://knowledge/source_movements`, etc.). This file is reference, not background reading.

---

## Spatial concepts

- **Stage** — the physical performance area in the venue. Configurable shape (Box / Cylinder / Dome) with explicit dimensions in System Config.
- **Origin** — the (0, 0, 0) reference point used by every position parameter. Default: center of the stage at ground level (half the width and half the depth from any edge), regardless of stage shape. Configurable via Origin Width / Depth / Height in System Config; presets named *Front*, *Center Ground*, *Center*.
- **Coordinate modes** — every per-channel position can be entered and displayed in any of three modes: **Cartesian** (x, y, z in meters), **Cylindrical** (r, θ, z), **Spherical** (r, θ, φ). Internally always stored as Cartesian; conversion happens at the UI/OSC boundary.
- **Width / Depth / Height** — the axis names sometimes used in older UI labels and CSV columns. Equivalent to **x / y / z**. Voice queries about "left and right" / "front and back" / "up and down" map to x / y / z.
- **Azimuth** — angle in the horizontal plane. 0° = toward audience (−Y), 90° = stage-right (+X). Used in cylindrical and spherical modes (`positionTheta`).
- **Elevation** — angle out of the horizontal plane. 0° = horizontal, +90° = up (+Z). Used in spherical mode (`positionPhi`).
- **Parallax correction** — per-output adjustment that defines a virtual "target listener" each speaker is aiming for. Used to compute correct delays when the actual listener isn't at the speaker. See `wfs://knowledge/parallax_correction`.
- **Haas effect** — a global delay added to all reinforced sound so that the acoustic source on stage arrives at the audience first. Lives in System Config > Master Section.

## Routing and channels

- **Input** / **Source** / **Channel** — three names for the same thing. Documentation tends to say "input" (matches CSV variable prefix `input*`); operators often say "source" or "channel" interchangeably. The AI should treat them as synonyms and use whichever the operator used.
- **Output** — a physical speaker. Each output has a position, orientation, and contributes to the soundfield.
- **Array** — a group of outputs that share parameters (lower array, flown array, surround, sub-bass, etc.). Up to 10 arrays. Per-output `outputArray` selects membership; `outputApplyToArray` controls whether parameter changes broadcast (Absolute / Relative) to siblings.
- **Cluster** — a group of inputs that can be moved, rotated, scaled, attenuated, and LFO-modulated as a whole. Up to 10 clusters. Distinct from arrays (clusters group inputs, arrays group outputs). Per-input `inputCluster` selects membership.
- **Cluster reference** — the pivot point used by cluster rotation and scale. Either *First Input* in the cluster, or *Barycenter* of the assigned inputs. A tracked input in the cluster, when present, overrides this and becomes the reference automatically.
- **Reverb feed** — a mono per-input send into the reverb engine. Each reverb channel has a feed position used for the input-to-reverb routing matrix.
- **Reverb return** — the wet output of one reverb channel, routed back to the speakers. Each reverb channel has a return position used for the reverb-to-output routing matrix. Feed and return positions can be co-located or independent.

## Movement layers

A source's rendered position is the sum of five independent layers. They compose, they don't override.

- **Base position** — set manually, by tracking, by Android Remote, or by a controller (SpaceMouse, joystick, gamepad).
- **Offset** — a constant shift applied on top of the base. Used for fine adjustments without disturbing tracking.
- **AutomOtion** — a one-shot trajectory from the current position to a target, over a defined duration. Can be manually triggered or audio-level-triggered. Distinct from *automation* in the DAW sense (that's the host's job).
- **Jitter** — fast random micro-movement around the current position. Adds tangibility to otherwise-static sources.
- **LFO** — periodic structured movement on each axis, with per-axis shape (sine / square / sawtooth / triangle / keystone / log / exp / random). Plus a *gyrophone* option that rotates HF directivity rather than position.
- **Gradient maps** — NOT a movement layer; gradient maps modulate level, height, and HF damping based on the source's current position. They follow the position, they don't drive it.

## Audio shaping (per-input)

- **Directivity** — angular width of the input's brightness (HF) cone, in degrees (2°–360°). 360° = omnidirectional. Distinct from **Rotation** (where the cone points in the horizontal plane, in degrees) and **Tilt** (where the cone points in the vertical plane, in degrees). All three together define a 3D radiation pattern.
- **HF Shelf** — the maximum high-frequency attenuation applied at the back of the directivity cone. Smooth cosine fade between full brightness on-axis and damped at the rear.
- **Live Source Tamer** (also called **Live Source Damping** in older docs — same feature) — reduces speaker contribution near a loud acoustic input on stage. Per-input radius / shape / fixed attenuation, plus optional level-dependent peak and slow compression. Per-output `outputLSattenEnable` toggle to opt out specific speakers (sub-bass, far-field) from being affected.
- **Sidelines** — automatic muting when an input approaches the edge of a rectangular stage (excluding downstage). Configurable fringe width.
- **Floor reflections** (also called **Hackoustics**) — simulated bounce off the stage floor. Adds early-reflection cues to amplified sound so it feels physically present rather than disembodied. Per-input enable + filtering; per-output enable to skip subs and flown speakers.
- **Attenuation Law** — distance-attenuation model. *Log* uses a linear dB/m drop (`distanceAttenuation`). *1/d²* uses inverse-square (`distanceRatio`). The two share UI position; visibility depends on which law is selected.
- **Common Attenuation** — percentage that lifts upstage sources back toward the loudest near-field speaker's level so that distant sources don't drop too far in the mix. 100 % = no lift, 0 % = full lift.
- **Height Factor** — 0–100 % weight given to z (height) in distance and level calculations. 0 % collapses the system to 2D for level/delay; 100 % is full 3D. Doesn't affect angular/directivity calculations.

## Reverb

- **SDN** — Scattering Delay Network. Reverb nodes coupled by inter-node delay paths with a Householder scattering matrix. Geometry-driven; node positions matter.
- **FDN** — Feedback Delay Network. Each node is independent; 16 delay lines per node with a Walsh-Hadamard mixing matrix. Classical reverb topology, multi-instanced per channel.
- **IR** — Impulse Response. Convolution per node, optionally per-node distinct IRs.
- **Pre-EQ / Pre-compressor** — per-channel 4-band parametric EQ feeding a global compressor before the algorithm stage. Sidechain captures the post-EQ RMS for the post-expander.
- **Post-EQ / Post-expander** — global 4-band parametric EQ and a sidechain-keyed expander after the algorithm stage. Sidechain key comes from the pre-stage tap (so the expander ducks the wet tail when the source goes quiet).
- **Wet level** — global gain on the algorithm output, applied after post-processing. Defaults to 0 dB.
- **Solo Reverbs / Mute Pre / Mute Post** — three mutually-exclusive tab-level long-press toggles. Solo Reverbs mutes dry, leaves wet. Mute Pre stops new excitation but lets existing tails decay. Mute Post zeros wet output entirely. See the Reverb tab header.

## State management

- **Snapshot** — a saved state of all input parameters at a moment in time. Files in `<project>/snapshots/inputs/`. Recall during a show progresses scene-by-scene.
- **Scope** — a per-snapshot filter that selects which parameters and which channels are included. Distinct from the snapshot itself; scope can be applied either *when saving* (small file, no recovery of skipped params) or *when recalling* (full file written, scope applied at load time).
- **Filter** — synonym for scope when used at recall time. The "record all data and a filter in local files" option in the snapshot scope window stores everything but uses the filter to control which params overwrite the current state on load.
- **Apply mode** — UI choice between *OnSave* and *OnRecall* scope filtering for a given snapshot. *OnRecall* is the default.
- **Project / Show / Configuration** — three nested concepts. *Configuration* = what the system is (channel counts, speakers, reverbs); rarely changes after venue load-in. *Show state* = what's happening right now; changes constantly. *Snapshots* = saved show states for cue-based recall. A *project folder* contains all three.

## Network protocols

- **OSC** — generic Open Sound Control. UDP and TCP both supported. The MCP server itself does NOT use OSC internally — it calls the parameter system directly.
- **OSC Query** — companion HTTP/WebSocket protocol that lets clients discover available OSC paths and subscribe to value changes. Used by the DAW Master plugin to track app-side state.
- **Remote** — the Android-Remote-specific protocol on `/remoteInput/*` paths. Distinct from regular OSC because it's bidirectional with handshake. The AI client should NOT send `/remoteInput/*` paths; the MCP server uses `/wfs/*` paths or, more often, internal parameter writes.
- **ADM-OSC** — Audio Definition Model OSC, an industry interop standard. Sends normalized positions (−1..+1 Cartesian, or azimuth/elevation/distance polar). WFS-DIY supports four Cartesian and four Polar mappings simultaneously, each with its own per-axis swap / sign-flip / center / breakpoint / inner-outer width.
- **PSN** — PosiStageNet, the de-facto entertainment-tracking protocol.
- **RTTrP** — BlackTrax's native tracking protocol; also supported by some other vendors.
- **MQTT** — lightweight pub/sub used by some IoT-style tracking deployments.
- **QLab** — show-control software (Figure 53). WFS-DIY can both send to QLab (creating network cues that mirror snapshots) and receive snapshot-recall triggers from QLab.

## Helper tools and dialogs

- **Wizard of OutZ** — output-array helper dialog. Seven preset profiles (Near Field Straight / Curved, Main Flown Straight, Sub Bass, Surround, Delay Line, Circle) plus five geometry methods (Center+Spacing, Endpoints, Curved, Circle, Surround Pairs). Writes to a contiguous range of outputs on Apply, then auto-advances to the next array. See `wfs://knowledge/array_design` and `Documentation/WFS-UI_arrayWizard.md`.
- **Set All Inputs window** — long-press dialog from the Inputs tab. Bulk parameter changes that apply to every input simultaneously. Useful for resetting flips, disabling LFO across all sources, applying a coordinate mode globally, etc.
- **Snapshot Scope window** — the per-snapshot scope editor. Parameter-level, per-channel granularity. Has a "Write to QLab" mode that exports the snapshot as a QLab cue sequence instead of a local XML file.
- **Audio Interface and Patching window** — floating window for audio device selection and input/output patching. Test-signal generator (Off / Pink Noise / Tone / Sweep / Dirac Pulse) lives here; it's not part of the WFS DSP path.

## Disambiguations the AI should remember

- **"Input" vs "Source" vs "Channel"** — synonyms. Use the operator's word.
- **"Live Source Tamer" vs "Live Source Damping"** — same feature, two names. The CSV variable prefix is `inputLS*`.
- **"Hackoustics" vs "Floor Reflections"** — same feature. The CSV variable prefix is `inputFR*`.
- **"Move" vs "AutomOtion"** — current name is **AutomOtion**. Older docs and prompt templates may still say "Move" or "Move command".
- **"Filter" (snapshot)** vs **"Filter" (OSC source filter)** — snapshot scope when applied at recall time vs the network-tab toggle that decides whether unregistered OSC senders are accepted.
- **"Group" (output)** vs **"Cluster" (input)** vs **"Array" (output, again)** — outputs can be grouped into arrays (1–10). Inputs can be grouped into clusters (1–10). "Group" loosely refers to either depending on context; prefer the precise word.
- **"Latency" vs "Delay"** — at the per-input level, `inputDelayLatency` is one bidirectional slider. Negative values are *latency compensation* (subtract from delay), positive values are *added delay*. The label changes accordingly in the UI.
- **"Master Level" vs "Wet Level"** — Master Level (System Config) is the global output gain. Wet Level (Reverb tab > Algorithm) is only the reverb wet mix.
