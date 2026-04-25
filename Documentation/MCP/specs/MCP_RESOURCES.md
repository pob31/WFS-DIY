# MCP Resources — Knowledge Base Specification

Resources are readable markdown documents the AI fetches on demand when it needs background knowledge to help the user. They are NOT tool descriptions (those live in `generated_tools.json`) and they are NOT workflow templates (those are prompts). They are *documentation the AI reads for context.*

## When the AI fetches resources

The AI decides to fetch a resource when the conversation requires background knowledge. Examples:

- User asks "what does live source damping actually do?" → AI fetches `knowledge_live_source_damping`.
- User says "help me tune the system" → AI fetches `knowledge_system_tuning` to orient itself, then invokes the `system_tuning_workflow` prompt.
- User asks "why does my source sound wrong when I move it upstage?" → AI fetches `knowledge_parallax_correction`.

The MCP server presents each resource with a clear title and description in `resources/list`, and a URI the AI can fetch with `resources/read`. Good descriptions are essential — they're what the AI reads to decide whether to fetch.

## Resource naming

URIs follow the pattern `wfs://knowledge/<topic>`. Examples:

- `wfs://knowledge/psychoacoustics`
- `wfs://knowledge/wfs_theory`
- `wfs://knowledge/array_design`
- `wfs://knowledge/system_tuning`
- `wfs://knowledge/parallax_correction`
- `wfs://knowledge/live_source_damping`
- `wfs://knowledge/floor_reflections`
- `wfs://knowledge/reverb_in_wfs`
- `wfs://knowledge/gradient_maps`
- `wfs://knowledge/source_movements`
- `wfs://knowledge/signal_flow`
- `wfs://knowledge/session_concepts`
- `wfs://knowledge/tracking`
- `wfs://knowledge/help_cards`
- `wfs://knowledge/glossary`

Each URI maps to a markdown file bundled with the application.

## Resource catalog

The following resources are provided as markdown files in the `resources/` directory of this handoff. Each is written fresh from the raw material (old PDF manual + help cards + CSV notes), topic-organized rather than following the manual's linear structure. All Max/Lemur/old-UI references have been stripped.

| URI | Title | Description (what AI reads) |
|---|---|---|
| `wfs://knowledge/psychoacoustics` | How humans localize sound | Explains binaural localization (time and intensity differences), the precedence effect, the Haas effect, and spectral masking. Useful when explaining WHY WFS works the way it does, or when troubleshooting localization issues. |
| `wfs://knowledge/wfs_theory` | Wave Field Synthesis algorithm | Explains the core WFS algorithm: wave front reconstruction, how delay and attenuation are computed per source per speaker, the design objectives and trade-offs (Doppler effect, no focused sources between array and audience). Useful when the user asks conceptual questions about what the processor is doing. |
| `wfs://knowledge/array_design` | Designing speaker arrays for WFS | Guidelines for speaker selection, spacing, array positioning (front-fill, flown, surround). Maximum speaker spacing formula based on coverage angle and listener distance. Recommendations for small, mid-sized, and large venues. Useful when helping with system design before a venue load-in. |
| `wfs://knowledge/system_tuning` | WFS system tuning procedure | The four-step tuning procedure: tune lower array alone, tune flown array alone, combine and adjust delay, fine-tune parallax and Haas effect. What reference material to use (NOT noise or sine waves). Why WFS tuning is different from conventional PA tuning. Useful when running `system_tuning_workflow` or when user asks how to tune. |
| `wfs://knowledge/parallax_correction` | Parallax correction explained | Why sources-speakers-listeners are not aligned in real venues, how the per-output target listener works, how horizontal and vertical parallax compensate for delay differences, the coupling issues that can arise. Useful when the user reports spatial localization problems. |
| `wfs://knowledge/live_source_damping` | Reducing amplification near loud sources | Explains live source damping: what it does, when to use it (loud acoustic sources on stage, feedback prevention, musician comfort), the four shape profiles (linear, square, log, sine), and the peak/slow compression layer. |
| `wfs://knowledge/floor_reflections` | Simulated floor reflections ("Hackoustics") | Why floor reflections improve realism for played-back material, when to enable them per-output, the filtering and diffusion parameters, CPU cost considerations, interaction with parallax correction. |
| `wfs://knowledge/reverb_in_wfs` | Reverb in a WFS context | Why WFS benefits from added reverb (masking speaker reflections, restoring depth), the difference between reverb feeds and returns, positioning reverb nodes, feedback prevention by design, the three integrated algorithms (SDN, FDN, IR) and when to use each. |
| `wfs://knowledge/gradient_maps` | Gradient maps | How position-driven maps modulate level, effective height, and HF damping as a source moves on stage. Application examples: off-stage muting, height matching for stairs and platforms, creative distance effects, room-based processing. |
| `wfs://knowledge/source_movements` | Movement automation — LFOs, trajectories, jitter | The available movement modes: offset with rotate/scale, one-shot Move with time and curve, jitter (random micro-movement), LFO (periodic movement with per-axis shape/rate/amplitude/phase). Gyrophone for directivity rotation. Global speed control. When to use constant-speed (line) vs. smooth (sine) acceleration. |
| `wfs://knowledge/signal_flow` | Signal flow in a typical session | Console direct-outs post-fader feeding the WFS processor inputs, speaker outputs going back to the console or direct to amps, reverb sends and returns, the object-oriented mixing paradigm (per-source parameters, not per-speaker mixing). |
| `wfs://knowledge/session_concepts` | Sessions, snapshots, and scope | How a session is organized (configuration + snapshots + samples + IRs), what's in each saved file, the snapshot scope system (parameter-level, per-channel granularity), backup/autosave, OSC-driven snapshot operations. |
| `wfs://knowledge/tracking` | Position tracking | Tracking technologies (UWB, computer vision, LiDAR, IR-LED), supported protocols (OSC, PSN, RTTrP, MQTT), the OSC message format, coordinate mapping (offset/scale/flip), smoothing trade-offs, per-input tag assignment, hand-off between tracked and static states. Useful when setting up tracking, calibrating, or troubleshooting positional issues. |
| `wfs://knowledge/help_cards` | Quick-reference help cards | Per-section quick help (System Overview, Session Data, Network, Tracking, ADM-OSC, Array Design, Parallax, System Tuning, Reverb feeds/returns/algorithms, Inputs, Live Source Tamer, Floor Reflections, LFO, AutomOtion, Gradient Maps, Sampler, Clusters, Map). One paragraph per area of the application, mirroring what the in-app help cards show. Useful when the operator asks "what does this tab do" or for the AI to ground itself in WFS-DIY-specific terminology before answering. |
| `wfs://knowledge/glossary` | WFS-DIY glossary | App-specific vocabulary the AI is likely to encounter in tool descriptions, parameter names, and operator queries. Covers spatial concepts, routing/channel terms, the five movement layers, audio-shaping vocabulary, reverb terminology, state-management terms, network protocols, helper tools/dialogs, and disambiguations the AI should remember (Input/Source/Channel as synonyms; Hackoustics = Floor Reflections; Live Source Tamer = Live Source Damping; AutomOtion vs Move; etc.). Fetch once at session start to ground vocabulary. |

## When to add or change resources

Resources should be stable. Their purpose is to carry durable conceptual knowledge, not up-to-the-minute status. Things that belong as resources:

- Theory and physics that doesn't change.
- Design guidelines that hold across versions.
- Procedural workflows (as narrative documentation; the *executable* version is in prompts).
- Conceptual explanations of features.

Things that do NOT belong as resources:

- Current feature status ("this feature is beta") — goes in tool descriptions or is simply omitted.
- Specific version numbers, release notes — those belong elsewhere.
- Per-parameter details — that's what the CSVs and generated tool schemas are for.

## Multilingual resources (optional, post-v1)

For French-speaking users, consider providing French versions of the highest-value resources (psychoacoustics, WFS theory, system tuning). Pattern: `wfs://knowledge/fr/psychoacoustique`. Client can fetch either based on operator language preference. Modern LLMs handle either, but a well-written French original reads better than on-the-fly translation.
There are also Spanish, Italian, Portugese, German, Japanese, Korean and Chinese translations. These need proofreading by native or bilingual speakers before integration in the MCP server.
Not a v1 deliverable. Flag as future work.

## Delivery format

Each resource is a standalone markdown file, ~2-8 KB. Content should be:

- Self-contained (no cross-references to other resources that the AI might not have fetched).
- Written in clear plain-English exposition.
- Organized with headers so the AI can quote specific sections.
- Free of venue-specific anecdotes or operator in-jokes (those belong elsewhere).
- Free of references to the Max/MSP version, Lemur, or other legacy components.
- Focused on concepts and reasoning, not on button names or UI layout.

The included resources in the `resources/` directory of this handoff follow these conventions and are ready to bundle with the application.
