# Changelog

All notable changes to WFS DIY are documented in this file, organized by release tag (newest first). Sections marked "also tagged" note commits that carry more than one tag (e.g. a plugin-track tag and an app beta tag landing on the same commit).

## v1.0.0beta39 — 2026-07-25

### Changed
- **CPU SDN reverb is now node-parallel** (was strictly single-threaded). The per-sample lockstep is replaced by a chunked node-parallel sweep: within a window of C samples where every *effective* inter-node delay exceeds C, no node can read a delay-line cell another node writes in that window, so the nodes are independent across the whole window rather than only per sample. C is derived per block from the actual geometry — `min over paths of min(D, MAX_DELAY − D)`, using the shorter tap while a path crossfades — and collapses to 1 (reproducing the old lockstep exactly) for degenerate geometry. **Bit-exact**: all 15 CPU goldens pass unchanged, and hashes are identical across 0/3/7/15 workers. Measured at 32 nodes / 48 kHz / 256-sample blocks: **0.935 → 0.460 ms per block (~2×)** on 7 workers, against a 5.33 ms budget. `SDNAlgorithm::setParallelFor` was an empty override, so SDN had been silently ignoring the worker pool the engine already handed it — no engine change was needed to switch it on.
  - Two aliasing hazards the naive form misses are handled explicitly and documented in `chunkSamplesForBlock()`: the crossfade **dual-tap** (both old and new delays are live for ~10 ms after any geometry or `sdnScale` change) and **ring wraparound** (a read at `(base+s−D) mod 8192` aliases a write at `base+s'` once D approaches the 8191 clamp, ~14.3 m at `sdnScale` 4). The old lockstep was immune to the second by program order; a parallel sweep is not.
  - Note this shape was previously removed as buggy (`c3e7ae2`) because sub-block delays made it racy — it returns here with the correct bound rather than an assumption.
- The `Reverb GPU pump:` log line now records **which GPU the reverb backend is bound to**. The reverb and the direct-sound path can sit on different devices, so without it a reverb overrun could not be attributed between GPU co-tenancy and host contention — precisely the ambiguity that blocked the 2026-07-25 SDN investigation.
- Version bumped to 1.0.0beta39 (`WFS-DIY.jucer` + the generated `JuceHeader.h` / `resources.rc`; the macOS project files and the vcxproj `JUCE_APP_VERSION` re-sync on the next Projucer resave).

### Added
- **`offline-render --reverb-workers N`** — drives the CPU reverb paths' `AudioParallelFor` pool at a chosen width (default 0 = the sequential baseline the goldens are recorded against). Added to measure how the reverb algorithms scale with worker count and to verify their worker-count invariance; it is what disproved the 7-worker-cap hypothesis below.
- **CUDA graph submission for the GPU WFS gather path** (opt-in, `WFS_GPU_GRAPHS=1` read at engine start): the constant per-block core (input upload → both kernels → output download) is captured once into a CUDA graph at `prepare()` and replayed with a single `cuGraphLaunch` per block; the upload-diet/ping-pong matrix logic stays outside the graph, and the instantiated graph is pre-uploaded so the first block pays no lazy-init stall. **Bit-exact** (gather goldens hash identically with graphs on/off, verified on sm_120 and sm_75; kernel sources untouched — `kernel_hashes.py` clean). Measured at 96 kHz / 64-sample blocks, 64×128: on a compute-mode (MCDM) Tesla T4 this collapses the per-call submission stall tail — depth-1 underruns 82 → 5 per 30 s, p999 2.2 → ~1.0 ms; on the WDDM display RTX 5070 it is neutral (+~30 µs median), so the default stays off. Legacy path fully retained (the toggle doubles as the A/B switch); HIP twin port is a planned follow-up now the pattern is proven on NVIDIA; Metal needs nothing (already single-command-buffer submission).
- **`tools/validation/pipeline-bench`** — real-time latency/underrun harness. Unlike `offline-render` (synchronous, bypasses the async pipeline), it exercises the REAL `GpuAsyncPipelineT` — pump thread, SPSC rings, depth cushion, underrun accounting — driven by a metronomic simulated audio callback (high-resolution waitable timer + short spin to absolute deadlines, MMCSS-elevated). Reports per-depth underruns (pump-failure aware), backend-time distributions (min/med/p99/p999/max vs block budget), callback wake jitter, and a timestamped ≥1 ms spike log; `--depth-sweep 1..8`, `--json` output. Builds with `REVERB_DIAGNOSTICS=1`.
- **`Documentation/Windows_GPU_Latency_Tuning.md`** — measured runbook for the RTX 5070 + Tesla T4 rig: full depth-sweep tables (gather/scatter × both GPUs × legacy/graphs), per-device recommendations (**depth 3-4 is the safe operating point on either card**; 5070 graphs off; T4 graphs on **plus pinned clocks**; scatter and SDN reverb stay off the T4, FDN reverb is a good fit for it), a field-validated known-good split (WFS gather on the 5070 at depth 2 + 32-node FDN on the T4), clock-pinning / HAGS / MPO tuning steps, and the evidence-ranked optimization roadmap (OB graph path, HIP port, kernel occupancy split, prepare-time warmup launch).
- **Field finding — GPU SDN reverb does not scale past low node counts, on any card.** At 32 nodes the SDN reverb pump peaked at **7.5-9.4 ms against its 5.33 ms budget** (~25× the FDN pump's 0.35 ms at the same node count) and, while it ran, the direct WFS pump collapsed from ~0.2 ms to ~5 ms peaks with 2799 underruns in ~6 s; switching back to FDN restored normal operation instantly. Confirmed on the RTX 5070 as well as the T4 — this is the `sdn_process` `grid = 1` shape (one SM on any GPU, plus a per-sample `__syncthreads()` serial chain), so no GPU upgrade fixes it. The runbook now directs high-node-count SDN to the **CPU** SDN path (which spreads nodes across the `AudioParallelFor` pool) and adds a multi-block SDN kernel redesign to the roadmap, alongside a request to log the reverb backend's bound device (its absence made the episode impossible to attribute between GPU co-tenancy and host contention).
- **Field finding — 32-node SDN runs acceptably on the CPU path** (holds "most of the time" on a 9950X, with occasional dropped-reverb-block warnings), confirming the GPU→CPU redirect above. Measured follow-up **rejected** the initial hypothesis that `ReverbEngine.h`'s 7-worker pool cap was the constraint: `ReverbSDNAlgorithm::setParallelFor` is an empty override (SDN runs a synchronous node-lockstep by design — that is what makes it bit-exact with the GPU backend), so SDN never uses the pool and shows *zero* scaling from 0 to 23 workers; FDN scales well but saturates almost exactly at 7 (156 → 149 ms from 7 to 23 workers), so the existing cap is well chosen. **32-node SDN uses ~1.05 ms of its 5.33 ms budget single-threaded (~20%)**, so the residual dropped-reverb-block warnings are jitter, not throughput — CPU idle/power-state tuning is the fix and no code change would help. Also measured: at 32 nodes the CPU beats the GPU for both algorithms (SDN 1.05 ms CPU vs 7.5-9.4 ms GPU peak; FDN 0.20 ms vs 0.35 ms), so GPU reverb at this node count buys offload rather than speed. Worker-count hash invariance verified for both algorithms across 0/3/7/15/23 workers. The runbook also gains a CPU-side tuning section: AMD BIOS *Power Supply Idle Control → Typical Current Idle* + *Global C-state Control → Disabled* (the exact analogue of the GPU P-state finding), Windows minimum processor state 100%, and a note that CCD placement is entirely unmanaged (no affinity pinning exists anywhere in the app).
- **Measured 2x2 — clock pinning and CUDA graphs are redundant, not additive.** T4 gather, post-warmup underruns summed over depths 1-6 (30 s each): free clocks + legacy submission **211**, free + graphs **30**, pinned + legacy **28**, pinned + graphs **25**. Either intervention alone delivers ~7x; both together add almost nothing, so they attack substantially the same stall population (collapsing five submissions per block into one also removes most of the idle→busy transitions that invite power-state churn). Pinned + legacy was the only cell to reach **zero** underruns (depths 5-6). Medians move barely at all (0.174-0.201 ms across every cell) — all of it acts on the tail. Practical consequence: **pin the clocks and `WFS_GPU_GRAPHS` becomes optional on the T4.** Documented with explicit caveats: only the pinned pair is a strict back-to-back A/B, and the mechanism behind the field "max power" setting is unconfirmed (idle `nvidia-smi` read P0/1590 MHz both before and after it).
- **Measured — CPU affinity/CCD placement is not a first-order concern for this workload.** FDN 7-worker pool over 32 nodes under process affinity masks: all 32 logical CPUs 159.6 ms, one CCD 158.2 ms (within noise), one thread per physical core 153.2 ms (~4%, i.e. SMT-sibling contention, not CCD locality). At 0.20 ms of a 5.33 ms budget none of it is operationally relevant. The runbook additionally records why the thermal/boost-vs-cache-locality trade-off does not bind here (~20% duty cycle never saturates a core, so core rotation buys no boost headroom) and warns that pinning to a non-CPPC-preferred core set can *cost* a boost bin.
- **Field finding — GPU clock pinning is the highest-value knob, especially on compute cards.** Real-time audio at 96k/64 is a ~28% duty cycle, so the GPU goes idle thousands of times a second and default power management churns P-states, paying clock-ramp latency on the next block. Forcing the Tesla T4 to maximum power / locked clocks dropped its pump time considerably — to roughly RTX 5070-comparable — which no code change had achieved. This is idle-driven P-state management, **not** thermal throttling. It is not free, though: pinned clocks hold pinned voltage, and the T4 then draws 38.8 W of 70 W at ~11% utilisation (62 °C with its blower at 95%) — the runbook documents raising the clock *floor* (`-lgc 1200,1590`) as a lower-heat alternative to pinning at the ceiling. The runbook's depth-sweep tables were recorded with free clocks and therefore understate the T4; they are now labelled as the unmanaged baseline. Correspondingly the T4's stall tail is attributed to *both* P-state oscillation and MCDM per-call submission overhead (which the graph work addresses), with the caveat that the two were measured against the same symptom at different times, so their relative weight is not cleanly separated.

### Fixed
- The staged `wfs_cuda.dll` in the Windows build output was **stale** (built 2026-07-06, predating the FR diffusion Max-prototype port and the SDN N-invariant output-gain fix) — the app loads whatever plugin DLL is present, so those two DSP changes were silently absent from GPU processing on this machine. Rebuilt from current sources; per-device GPU goldens re-recorded (`baselines/win-rig-5070-gpu.json`, `win-rig-t4-gpu.json`). The runbook now prescribes rebuilding the plugin after pulling GPU-touching commits.

- **Warmup launch in the CUDA OutputBuffer (scatter) backend's `prepare()`**: the first post-prepare `processBlock` paid a measured 25-85 ms lazy-init stall (deferred driver/allocation commit), surfacing as an underrun burst right after every engine (re)start. `prepare()` now runs one silent block on the setup thread, then restores pristine first-launch state (`reset()` + the `frBasePrev` bootstrap) so the first audible block is bit-identical to a fresh prepare — verified against the GPU goldens on both devices. Re-measured at depth 4: RTX 5070 scatter fully clean (0 underruns, no block-0 spike); Tesla T4 scatter 0 post-warmup underruns (was 226 per 30 s), with a residual ~8 ms first block (was ~83 ms) costing ~10 underruns in the first ~10 ms of audio.

### Chore/Internal
- Architecture-doc drift fixed in both mirrored copies of `audio-engine-map.md` and in `CudaWfsBackend.h`: the end-of-block wait is a blocking-sync event (`cudaEventSynchronize`, yields — no spin), not `cudaStreamSynchronize`.

## v1.0.0beta38 — 2026-07-23

### Fixed
- Remote visualisation: **loading a session left the tablet's Visualisation tab empty** ("Waiting for data…") until the next selection change — the reload path re-dumped state to remotes but never sent the visualisation config/selection/rows (the send had been hooked into the channel-count-change handler instead of the config-reload handler, so it only fired when counts were edited in System Config). Session load now mirrors the full visualisation state. This was the root cause of the "works on a fresh session, dead on a reloaded one" reports.
- Remote visualisation hardening against packet loss: the config (channel counts + output array assignments) was a one-shot connect-time send at the tail of the ~70-bundle dump burst — the worst moment on congested Wi-Fi or tablets whose OS caps the UDP receive buffer. It now repeats with every visualisation update (rides the existing ≤10 Hz throttle) and is embedded in the state dump, so `/remote/requestResync` re-delivers it too; the dump copy reads the channel counts from the actual channel trees, not the Config/IO properties, which can drift in sessions saved by older versions. No protocol change; repeated identical config is a no-op on the tablet.

## v1.0.0beta37 — 2026-07-23

### Fixed
- Remote visualisation: selecting the reference input of a **First Assigned** (or Shared Position / tracked-reference) cluster on the map now mirrors all cluster members to the tablet's Visualisation tab, matching barycenter-selection behaviour — previously only the reference channel itself was shown. The single-input map selection is expanded to the member list (with its cluster id) using the same reference-handle resolution the map's drag logic uses.
- Companion fixes on the tablet side (WFS Control 1.0-beta_9, no protocol change): a **single** input selected on the desktop map is now followed by the Visualisation tab (previously only multi-selections switched the display), the follow-mode header shows the channel actually displayed, and the tab moved next to Input Parameters (Map | Lock | View | Input Parameters | [XY Pad] | Visualisation | Clusters | Array Adjust | Settings).

## v1.0.0beta36 — 2026-07-23

### Added
- **Remote protocol v3 — bargraph on the tablet**: the Inputs → Visualisation bargraph (per-speaker delays and levels) is now mirrored to the Android remote (WFS Control 1.0-beta_9), where a new Visualisation tab shows it full-screen — sized for the tablet that already sits by the console. Designed for training: new users can watch the levels and delays react as they move sources in the app.
  - The tab **follows the app's selected channel** — InputsTab selection, the Map tab's temporary multi-selection, and cluster selection are all mirrored (`/remote/vis/selection`). With several channels selected, the tablet shows one metric (all delays or all levels, chosen on the tablet) across the selection.
  - A tablet-side **pin** (`/remote/vis/pin`) locks the display to one channel; it is view-only and never moves the app's selection. Pins are per-tablet and restored automatically on reconnect.
  - Rows (`/remote/vis/delays` + `/remote/vis/levels`, one bundle per channel) are sent on matrix recalculation, throttled to ≤10 Hz with a guaranteed trailing send, and flow even when the app window is minimized. Levels travel as display-ready dB so the tablet's labels match the desktop bargraph exactly. Value labels are tinted by each output's array assignment (`/remote/vis/outputArrays`). HF damping stays desktop-only.
  - Backwards compatible: a v2 tablet ignores the new messages (amber mismatch banner as usual); the new tab on a v3 tablet shows an "update the server" hint against an older app.
- Snapshots: storing a snapshot or scope template under an existing name now warns before overwriting.

### Fixed
- Sampler: samples referenced by absolute path load again, and every silently-gated trigger is now logged so misfiring pads can be diagnosed.
- Help: the Overview "?" aligns with the Diagnostics "?"; the attenuation-curve digit markers are centred on each profile's dip.

### Chore/Internal
- Regenerated the 9-language translation-proofreading checklists (663 → 687 keys).
- `remote_tablet_mock.py` extended to assert the v3 visualisation contract (config/selection/rows after handshake, pin round-trip without a channel dump, ≤10 Hz row rate during a drag).

## v1.0.0beta35 — 2026-07-21

### Added
- Live Source Tamer help card now shows the two attenuation figures (the four attenuation profiles; the zone of influence around the source) in a compact side-by-side layout. The drawings carry numbered markers only; the legends are drawn next to each figure and localized in all 9 languages (source SVGs under `Documentation/svg/`).

### Changed
- **Upgraded JUCE 8.0.14 → 9.0.0** (released the same day):
  - **Linux multitouch is now native** via XInput2 — the bundled JUCE `canUseTouch` patch, the userland evdev touch backend, and the Touchscreens mapping window are retired. Touch needs no root access or udev rules anymore; on multi-display rigs, map a touchscreen to its display at the OS level with `xinput map-to-output`. Linux builds need `libxi-dev` (XInput2 headers).
  - **Windows keeps raw per-finger multi-touch** on all app windows: JUCE 9 switches to OS gesture recognition by default, and the app explicitly opts back in so multi-finger source drags and the hand-rolled pinch zoom (Map, EQ display, patch matrix) keep working. The DAW plugin editors keep the new gesture default.
  - New lunasvg-based SVG engine renders the help-card drawings more faithfully (better line weights).
  - macOS uses JUCE 9's rewritten CoreAudio backend (aggregate-device API).
  - `juce_opengl` removed from the project — it was compiled in but never used, and dropping it avoids JUCE 9's new EGL build dependency on Linux.
- Help cards now size themselves to their content instead of a fixed fraction of the column, fixing the System Tuning card's clipped drawings and a needless scrollbar on the Parallax card under JUCE 9's text engine.

### Fixed
- Buttons that trigger on a plain click (work-folder select, snapshot scope edit) no longer display the long-press triangle affordance.

### Chore/Internal
- CI: the Linux JUCE-patch step is gone from all workflows; `libxi-dev` added to the app and plugin apt lists; `tools/setup.sh` is submodule-init only.
- Upgrade verified: offline-render CPU baselines bit-exact (15/15 combos) against the pre-upgrade renders; all five control-replay E2E drivers pass; app + plugin suite build clean on all three OSes with zero new warnings.

## v1.0.0beta34 — 2026-07-21

### Added
- Snapshots: Scope Templates in the Snapshot Scope window — store the current channel × parameter inclusion grid as a named template, then reload, update, or delete it from a dropdown. Templates are grid-only (the apply mode and QLab/auto-preselect toggles are left untouched on reload) and are saved as XML files in `<project>/snapshots/scopes/`, so they travel with the project. Store is always available; Reload/Update/Delete enable once a template is selected. Channels added after a template was stored default to included on reload.

## v1.0.0beta33 — 2026-07-20

### Added
- Plugins: Common Attenuation parameter added to all five ADM-OSC Track variants, mirroring the app's input Common Attenuation control and kept in sync via app→plugin OSCQuery subscription (plugin version bumped to 0.0.4).
- Snapshots: new "Update Snapshot Scope" long-press action that rewrites a selected snapshot's stored scope in place, without re-capturing live values — useful when you only meant to narrow what a snapshot covers rather than overwrite its captured state.

### Fixed
- Fixed OSC/QLab fades appearing not to work: QLab can send custom-message arguments as OSC strings rather than numbers, and a string-typed fade value was silently treated as an instant jump to 0. Numeric-looking strings for both the value and fade time are now coerced to floats.
- Added fade support for `inputAttenuation`, `inputCommonAtten`, and the `inputArrayAtten1..10` array sends, none of which were OSC-fade-routable before.
- QLab snapshot load/store cues now quote the snapshot name so names containing spaces round-trip correctly.
- Fixed a Shared Position cluster stale-jump on remote release, affecting clusters saved by older project versions: several ValueTree reads across the OSC layer silently defaulted to 0 for freshly loaded string-typed properties, which also caused per-channel remote state dumps to send zeros for every parameter right after a project load.
- Unified the LFO phase controls onto a single -180..180 range with circular wrap; previously three conflicting conventions (0..360 vs -180..180 in different places) meant a typed value like 270 could silently become 180.
- Fixed LFO period value label rounding/truncation and widened the field so values like "100.0" display correctly.
- Fixed a snapshot-recall bug where gradient-map layers were matched positionally instead of by ID, misapplying partially-scoped snapshots to the wrong layer slots.

### Changed
- Floor Reflections diffusion now uses a faithful port of the Max prototype's shimmer model (fast unipolar noise, <= 0.3 ms, 50-300 Hz) in place of the previous slow bipolar wander, fixing reflections that sounded too mellow.

### Chore/Internal
- Refreshed the prebuilt `wfs_hip.dll` with the Max-prototype Floor Reflections diffusion update.
- Added `.gitignore` entries for import-lib/exp byproducts of the prebuilt HIP plugin.

## v1.0.0beta32 — 2026-07-19 (also tagged plugins-v0.0.3)

### Added
- Remote protocol v2: ping/pong now carries a sequence number and protocol version, so a mismatched Android remote is flagged with an amber row and tooltip in the Network tab instead of failing silently, and roughly 10 seconds of unanswered pings now surfaces a "remote not responding" state.
- Full state dumps to the remote are now framed by matched `/remote/dumpBegin` and `/remote/stateComplete` markers (carrying sequence number and channel count) so the tablet can reliably detect partial dumps and reset its completeness tracking on each dump cycle.

### Fixed
- Fixed a race where dump sends running on detached threads could read the incoming-protocol state unsynchronized and silently drop entire state dumps when a tablet gesture was in flight; this state is now atomic and dump sends no longer go through the per-parameter loop-prevention check.
- Fixed channel 1 (the default-selected channel) sometimes being missing from full dumps since it's never re-dumped by a user tap; every full dump now ends with the selected channel's complete parameter block, sharing code with the per-channel dump path so the two can't drift.
- Removed the dead `sendPacedStateDump` code path.

### Chore/Internal
- Added a UDP mock-tablet validation tool (`remote_tablet_mock.py`) covering handshake, dump shape including channel 1, gesture-blast race regression, and per-channel/full resync, used to verify the protocol v2 changes end-to-end. Requires WFS_DIY_remote protocol v2 (1.0-beta_7).
- Bumped version to 1.0.0beta32.

## v1.0.0beta31 — 2026-07-19

### Added
- EQ gain and the AutomOtion curve editor now support a bipolar dial mode, letting values be adjusted symmetrically around a center point.

### Changed
- Output and reverb capability toggles were reframed as Allowed/Excluded switches for clearer, more intuitive control over which outputs/reverbs are permitted.

### Fixed
- GUI buttons and combo boxes now render with crisp, pixel-aligned frames, removing a stray half-pixel line that appeared along their bottom edge.

## v1.0.0beta30 — 2026-07-19

### Added
- Cluster-wide parameter editing in the Inputs tab via keyboard modifiers, allowing edits to apply across multiple channels at once.
- Ctrl/Cmd modifier in the Outputs tab to bypass array propagation when editing a single channel.
- Keyboard shortcuts for the N/C tabs, plus new Keyboard Shortcuts and Diagnostics help cards in configuration.

### Fixed
- Long-press triangle indicator now fades in on hover instead of always being visible.
- Long-press triangle outline and button border rendering crispened up.

### Performance
- Stopped per-write channel reload floods in the Outputs and Reverb tabs.

### Chore/Internal
- Bumped the spatcore submodule (adds a `.gitignore` entry for Python bytecode caches).

## v1.0.0beta29 — 2026-07-18

### Added
- Inputs now warn when a per-input feature can't take effect.
- GUI: input tab gained dimming for inactive items, a long-press affordance, and a cluster clear button.
- CI: added GitHub Actions builds for plugins covering Windows VST3, macOS VST3+AU, and Linux VST3+LV2.

### Fixed
- `system.xml` no longer gets auto-saved over a configuration that hasn't finished loading yet.
- Fixed array toggle inversion bug during relative propagation to outputs.
- "Find My Remote" network discovery now works with an empty password.
- Fixed universal macOS plugin binaries when building with single-config CMake generators.

### Chore/Internal
- Plugin releases built via GitHub Actions are no longer marked as the repo's "latest" release.

## v1.0.0beta28 — 2026-07-18

### Fixed
- Fixed OSCQuery echo-to-sender bug that punched DAW automation playback out of override: the app's WebSocket push was meant to skip the client that originated a value, but the suppression logic was broken across coalesced updates, several parameter categories (special/polar/AutomOtion/EQ/config), and the OSC bundle path, so the plugin received its own automation values back and Ableton treated them as a manual touch. Origin IP is now captured at queue time and threaded through every push path, including fade-time ramp trajectories; the plugin also now ignores inbound values that already match its current parameter as a defense-in-depth check.
- Fixed OSC fade-time ramps (the optional 3rd "transition time" argument on `/wfs/input` messages) running 4-7x longer than requested, because ramps stepped by an assumed fixed 20 ms tick instead of actual wall-clock elapsed time; durations now land within about 2% of the requested time.
- A running ramp is now cancelled when a newer write (instant OSC set, GUI dial, snapshot recall) touches the same parameter, instead of being silently overwritten by the ramp's next tick.
- Interpolated ramp writes are now rounded for integer-typed parameters (rotation, directivity, tilt, heightFactor, FR frequencies, LFO phases), and ramped position X/Y/Z targets are clamped through the same stage-bound constraint path as instant sets.
- Added OSC log messages for ramp start and for fade-time-ignored-on-non-fade-capable-parameter, making ramp behavior diagnosable from the log window.
- Fixed remote `/arrayAdjust/*` output adjustments (delay/latency, attenuation, H/V parallax) being logged but never applied: the inc/dec dialect was not implemented and always produced a zero-delta no-op, the documented `arrayAdjust/level` address for attenuation wasn't recognized (only "attenuation" was), and output matching used a type check that failed for double- or string-typed output array values so no output ever matched. Inc/dec now applies relative per-output offsets correctly, both attenuation addresses are accepted, and output matching is done numerically regardless of stored value type.

### Chore/Internal
- Repaired three rows in `Documentation/WFS-UI_input.csv` (constraintDistanceMin/Max, sidelinesFringe) that had drifted one column left, which had left their generated MCP tools with an empty internal OSC path; removed the fade marker from the four sampler rows since the sampler is pad-pressure driven, bringing the CSV, code whitelist, and regenerated MCP tools back into agreement on 39 fade-capable parameters.
- Added `tools/validation/control-replay/oscquery_echo_check.py` to verify no echo is sent back to the originating sender across all write paths, including ramp trajectories and multi-sender interleaving.

## v1.0.0beta27 — 2026-07-17

### Fixed
- Wizard of OutZ: corrected preview framing and fixed a stale output channel count issue.
- Output Testing mode: the Tone frequency slider no longer lingers on screen after leaving Testing mode.

### Chore/Internal
- Bumped spatcore to v0.1.1-1-gc7dad5c, carrying an SDN (Scattering Delay Network) fix for N-invariant output gain.
- Refreshed the prebuilt `wfs_hip.dll` from spatcore c7dad5c to pick up the SDN gain fix.

## v1.0.0beta26 — 2026-07-07

### Added
- Full multi-vendor GPU backend overhaul: runtime-polymorphic backend interfaces/factories, per-vendor GPU plugins (`.so`/`.dll`) discovered via `dlopen`, a `GpuDeviceManager` for cross-vendor device enumeration, and a CPU-safe default when no GPU plugin is available. Ships with CUDA and HIP/ROCm plugins on Linux and Windows, plus Metal support on macOS.
- Per-role GPU device selection in the UI, so WFS and reverb processing can each be assigned to a different GPU on multi-GPU systems.
- GPU plugins and their runtime kernel compilers are now bundled in the Windows installer and Linux release tarball; a new headless GPU-plugin smoke test (`test-gpu-plugin.cpp`) exercises all 5 kernel families (WFS, OB, IR, FDN, SDN), including a multi-input reduce case.
- gpu-host optimization program (milestones M0-M3): live pipeline telemetry (duty atomics, meter-window strip), blocking event waits in place of spinning stream sync, an "upload diet" (device ping-pong buffers, change detection, scattered FR-tier uploads), and parallel fork-join host-side prep for the CUDA/HIP/Metal pumps, backed by a new JUCE-free `GpuHostWorkPool` and a thread-priority-elevation helper.
- New validation infrastructure: an offline-render harness with bit-exact CPU/GPU gather-scatter gates for the SDN/FDN/IR kernels and a `--bench` mode, a kernel-hash gate, a spatcore dependency lint, and a control-plane replay harness for the OSC/network layer.
- MCP: `WFS_MCP_AI_ENABLED=1` startup hook plus a new `session_save` tool; WFS-UI CSVs now carry an explicit Tier column as the primary source for parameter tiering.

### Changed
- Major internal restructuring: the audio engine — WFS/reverb DSP, GPU stack, OSC/MCP control-plane, controller device layer, binaural engine, and supporting core utilities — has been extracted from the app into a standalone `spatcore` library (Phases 1-5 in-tree, then Phase 6 split into its own repository and mounted back as a submodule).
- Windows toolchain standardized on Visual Studio 2026 (v145) across the project, CI, and docs.
- JUCE updated from 8.0.13 to 8.0.14.
- CUDA kernels are now compiled at runtime via NVRTC to an architecture-exact cubin instead of PTX.
- HIP backends refined: dropped the deprecated primary-context API.
- Removed Xencelabs Quick Keys controller support.

### Fixed
- Duplicate `ADMPolarMapping` nodes created on every `network.xml` load.
- RT-safety: tracking-receiver ValueTree writes are now marshaled to the message thread instead of being touched from the audio thread.
- Binaural engine: eliminated real-time-thread ValueTree access and fixed worker-thread lifecycle races.
- MCP `masterLevel` tier-casing typo (was tier 2, now correctly tier 3).
- Windows/HIP: fixed the `wfs_hip.dll` plugin build (dropped stray CUDA pragmas, linked `hiprtc`).
- Linux/HIP: plugin build was missing a link to `hiprtc`, causing a `dlopen` (RTLD_NOW) failure at load time.
- gpu-host: fixed a cross-generation data race and a phantom-generation bug in `GpuHostWorkPool`, both found during audit.

### Chore/Internal
- Extensive GPU validation runbooks and results recorded: Linux CUDA, Linux HIP/ROCm (gfx1103, all 5 kernels passing), and macOS Metal (including TSan and GPU/CPU power characterization on M4 Pro; SDN clean at 15 nodes).
- CI: dropped a vestigial CUDA install step, keyed the macOS cache on the spatcore pin, added a validation gate step; releases now bundle the MCP tool surface and knowledge base on all three platforms, with CUDA built on Linux CI.
- New architecture maps and runbooks documenting the spatcore extraction and repo split, plus an updated CLAUDE.md reflecting JUCE 8.0.14 and current GPU acceleration status.

## v1.0.0beta25 — 2026-06-14

### Chore/Internal
- Release CI now builds the Windows Inno Setup installer instead of packaging a bare zip.

## v1.0.0beta24 — 2026-06-14

### Fixed
- Windows: the app now launches even without an NVIDIA driver installed, by delay-loading the CUDA DLLs instead of requiring them at startup.
- Reverb: added status-bar help text for "Apply to all nodes" and finished GPU localization for the reverb node.

### Changed
- Windows distribution: dropped the redundant NVRTC `.alt` file (~85 MB smaller) and ship the CUDA DLLs directly in the installer instead.

### Chore/Internal
- Release CI: excluded the NVRTC `.alt` twin from the Windows zip.
- CI: pinned Windows build jobs to windows-2022, deferring the VS2026 upgrade.
- Bumped version to 1.0.0beta24 across all build files.

## v1.0.0beta23 — 2026-06-14

### Added
- Native Metal WFS backend shipped in the app as a third rendering algorithm — SDK-free and field-verified — built on top of a kernel spike that proved correctness at all buffer sizes with ~7x lower dispatch floor.
- Native CUDA WFS backend, the Windows/NVIDIA twin of the Metal algorithm, with CUDA runtime DLLs now bundled in the Windows build and CUDA installed in CI.
- Native GPU reverb algorithms added across the board: SDN, FDN, IR convolution, and OutputBuffer, each with Metal and CUDA implementations.
- GPU WFS gained Floor Reflections with a per-pair HF shelf (CUDA + Metal), plus an accompanying diffusion fix.
- Floor Reflections: enable fix, ramp engaged, and a shared diffusion model across algorithms.
- Live Source Tamer compression now applies to the native GPU algorithms.
- GPU reverb processing decouples its internal block size from the device buffer size and reports pump telemetry.
- New "Apply to all nodes" link toggle for reverb on the Channel Params subtab.
- New GPU pipeline depth dial (1-8, adjustable live) with underrun visibility.
- Live algorithm switching: changing the algorithm selector now restarts the engine automatically.
- Output channel cap raised to 128 and reverb channel cap raised to 32.

### Fixed
- Non-GPU builds: guarded a GPU-enum reference in the startAudioEngine log that was breaking compilation.
- Registered the GPU SDN reverb files in the project after they were left out, breaking the build.
- AutomOtion: fixed the audio trigger never rearming in Absolute + Stay mode.
- Inputs: AutoMotion threshold/reset controls now refresh their dimming state when the channel changes.
- Inputs: shortened the Tracking ID label to "ID" so the collapsed combobox shows the number.
- Floor Reflections diffusion now block-steps and ramps the grain, fixing OutputBuffer hiss.

### Changed / Internal
- Removed the GPU Audio SDK entirely — submodule, dormant v1 code, and all documentation references — now that native Metal/CUDA backends replace it.
- Added a tools/setup.sh bootstrap script so fresh clones get multitouch support automatically.

### CI/Build
- Linux release now built via build-app-tarball.sh, shipping install.sh with the full Linux payload and the multitouch patch applied in workflow.
- Windows release now ships only the exe and fixes a double-"v" in the release zip file name.
- Dropped the CI submodule working-tree cache, keeping ccache and DerivedData caching only.

### Docs
- Added a handover doc for Linux GPU (CUDA) enablement.

## v1.0.0beta22 — 2026-06-10

### Added
- Delay Line sag support for curved speaker installs
- GitHub Actions CI with release automation, including signed/notarized macOS DMG builds alongside Windows and Linux

### Fixed
- Circle preview label overlap
- ValueTree merge no longer collapses repeated id-less children on load
- Reverb feed path now rebuilds on device restart, fixing a stuck dropout banner
- ClustersTab controls now refresh correctly from the ValueTree on recall
- OutputsTab EQ-graph band edits now propagate to the array correctly in relative mode
- Loud crack when adjusting output, master, or reverb levels to/from silence

### Chore/Internal
- CI: macOS release job now points at the WFS-DIY environment instead of the generic 'release' environment
- CI: upgraded to Node 24 action runtimes and added submodule/build caching

## v1.0.0beta21 — 2026-06-08

### Added
- InputsTab now shows OTOMO trigger/reset indicators alongside the rest of the input controls.
- Localization: added a user-selectable translation tier, choosing between translating only Help & messages or translating Everything.
- Localization: the Translation control itself is now translated when the "Everything" tier is selected.
- Localization: help-card titles now show the native feature name underneath the English title.

### Fixed
- Fixed an UpdateChecker shutdown assert and a log-write error that could occur on quick quit.

### Changed
- Localization: the control surface (parameter and control names) stays in English; only prose and help text are translated.
- Localization: translated help text now references parameter names in English for clarity.

### macOS
- Audio threads now join the CoreAudio audio workgroup for coherent P-core scheduling.
- DSP worker threads now run at realtime priority so they land on performance cores.
- App Nap is now disabled so DSP threads stay on performance cores.

### Chore/Internal
- Added tracking for release-entitlements.plist, used for notarized Release builds.
- Regenerated proofreading checklists for the English-UI localization docs.

## v1.0.0beta20 — 2026-05-20

### Fixed
- Fixed input attenuation having no audible effect, caused by a duplicate ValueTree slot for the parameter.
- Fixed common attenuation on reverbs incorrectly cancelling out input attenuation instead of combining with it.
- Fixed missing schema entries not being back-filled when loading a project via wholesale state replacement (`ensureCompleteSchema`).

## v1.0.0beta19 — 2026-05-20

### Fixed
- Fixed jitter in the Shared Position cluster and added per-channel resync for Remote control.
- Linux: fixed LOC()/MCP resource lookup and shipped a working uninstaller.

### Changed
- Upgraded JUCE to 8.0.13 and dropped the ASIOSDK submodule dependency.

## v1.0.0beta17 — 2026-05-19 (also tagged v1.0.0beta18)

### Added
- Added a third "Shared Position" reference mode for clusters.
- Added an MCP `reverb_auto_layout` tool, plus support for batch channel creation.

### Fixed
- Fixed input attenuation having no audible effect.
- Implemented the missing per-output 6-band EQ DSP (Output EQ was previously a no-op).
- Input visualisation bars now refresh correctly when switching channels.

### Changed
- Inputs tab: added a width-based compact mode with tighter position-section paddings.
- Inputs tab: Flip and Constraint buttons now anchor from the right, constraint labels wrap, and Flip/Constraint switch to a compact two-line layout when the aspect ratio is taller than 16:9.

### Chore/Internal
- Bumped app version to 1.0.0beta17 and locked the macOS microphone permission entitlement.

## v1.0.0beta16 — 2026-05-05

### Fixed
- Fixed stale reverb-feed positions being used in the angular muting calculation.
- Fixed a crash when editing AutomOtion text fields while an Android Remote was connected.

### Changed
- Channel selector close button is now circular, with extra top padding for better spacing.
- Sampler sub-tab layout spacing fixes.

### Chore/Internal
- Linux release packaging now reads language and MCP data from the source-of-truth files instead of duplicated copies.
- Restricted the JUCE_JACK option to the Linux export target only.

## v1.0.0beta15 — 2026-05-03 (also tagged VST-AU-LV2_0.0.2)

### Added
- Linux: the standalone app now builds and launches
- Linux: real HIDAPI backend (hidraw + libudev) for hardware I/O, plus language and MCP postbuild steps
- Linux: multitouch support via a userland evdev backend
- Linux plugin builds and release packaging added; app version bumped to 1.0.0beta15
- Plugins: target-profile system for Master; plugin version bumped from 0.0.1 to 0.0.2
- Localization: touchscreen UI strings added to all locales

### Changed
- ADM-OSC: pure-math mapping extracted into Source/Shared for reuse
- SystemConfig: Linux touchscreens row aligned with the section's visual style
- Localization: comprehensive translation pass across 8 languages
- Localization: filled in missing cluster, MQTT, and ADM-OSC translations
- Localization: diagnostic help wired up and the unpatch button widened
- Localization: per-locale proofreading checklists added

### Fixed
- Restored `.gitmodules`, which had been accidentally removed in an earlier commit

## v1.0.0beta14 — 2026-05-02 (also tagged VST_AU_0.0.2)

### Added
- Introduced the MCP (Model Context Protocol) integration: a parameter registry, expanded state reporting, tier-2 batch parameter writes, and enriched tool documentation.
- Added MCP tools for creating and deleting inputs, outputs, and reverb sends (tier 2).
- Added `wfs_set_parameter_batch` for atomic multi-parameter writes recorded as a single undo entry.
- Added `session_get_state_delta`, which diffs session state against the last call across all origins.
- Tagged every MCP parameter with audio-domain hints to aid client tooling.
- MCP v3: read parity with tier-2 writes, enum coercion, a welcome/onboarding response, global-state filtering, and a tighter permission gate.

### Fixed
- Fixed Android remote cluster rotation and position sync for newly created inputs.
- Null-guarded `beginUndoTransaction` in `WFSValueTreeState` for MCP-originated writes.
- Fixed MCP channel-count resize, `channel_full` reverb handling, `get-param` range reporting, and `requested_as` metadata.
- Baked a tier marker into every tier-2/tier-3 MCP tool description, and ordered `tools/list` by tier (descending) then name so tier-3 tools stay visible.
- Fixed MCP tier-3 enforcement: escalation handling, gate semantics, and the tool classifier.

### Chore/Internal
- Regenerated MCP tool/group manifests.
- Replaced em-dashes with ASCII hyphens in C++ string literals (MCP tool descriptions).
- Annotated the MCP tuning-notes and follow-up documentation with shipped status for each item.
- Bumped version to 1.0.0beta14.

## v1.0.0beta13 — 2026-04-30

### Fixed
- Network tab: the MCP help card now fits properly and no longer hides other controls.

### Chore/Internal
- MCP tool generator now produces deterministic output across rebuilds, avoiding spurious diffs.
- MCP tool generator now correctly reports "unchanged" when a write was skipped.

## v1.0.0beta12 — 2026-04-30

### Added
- Introduced a Model Context Protocol (MCP) server (port 7400) that lets AI assistants and other MCP clients connect to WFS DIY and drive it directly, with a JSON-RPC handshake, a change-record ring buffer, and new `Protocol::MCP` / `OriginTag` tracking (including an Origin column in the Network Log) so AI-originated changes are distinguishable from OSC, Snapshot, UI, Move, and Tracking changes.
- Added the first MCP tool set: five hand-written Phase 1 tools, an auto-generated tool surface built from the parameter CSV, nudge (increment/decrement) tools, and a generic `wfs.set_parameter` escape-hatch tool — with the generator wired into the Projucer pre-build so tools regenerate automatically.
- Added an MCP knowledge-resource catalog (`resources/list` / `resources/read`, with resources copied in via a post-build step) and workflow prompts (`prompts/list` / `prompts/get`).
- Implemented real undo/redo for AI-driven changes: execution against the change-record buffer, targeted undo with group-based dependency chasing, origin-tag completion across UI/Move/Tracking, staleness detection, a self-correction flag, and a cross-actor notification side-channel — surfaced through a new AI History window with an undo toast overlay and keyboard shortcuts.
- Added tier enforcement, a safety gate, and a dry-run mode for MCP actions, followed by tier-2 confirmation in the tool schemas with a 10-minute gate window, throttled UI ticks, and a depleting-countdown display on the gate button.
- Added a cross-protocol audit pass that cross-checks OSCQuery and tags hardware/Remote origins.
- Added an MCP section to the Network tab: a help card, a status row with click-to-copy server URL and hover tooltips, and a simple AI on/off toggle (replacing the earlier AI Preview control), defaulting to OFF.
- Added Cartesian setters for output/reverb parameters as part of an MCP schema cleanup.
- Documented MCP server setup for AI clients in the README.

### Changed
- Renamed MCP tools to use underscores for OpenAI-style regex compliance.
- Reworked the MCP dispatcher to validate and coerce enum-tool and generic parameter values, and to route algo-level reverb tools through the correct WFSValueTreeState subtrees.
- Range-gated incoming OSC values with receiver-thread coalescing behind a bounded FIFO, added a file-load value gate, and reconciled parameter bounds across sources.
- Hardened network input parsing and clamped paths against malformed OSC.
- Localized and applied uiScale to all AI-related UI surfaces (history window, toast, MCP row, status bar).
- Adjusted MCP row button layout for even slack distribution.

### Fixed
- Fixed a shutdown crash by severing the NetworkTab MCP listener before the MCP server destructs, and fixed a shutdown heap-corruption bug in MCPTransport caused by concurrent `juce::String` access (a bind-verification listener was reverted while keeping CORS support).
- Fixed an MCP dispatcher use-after-free, tier-2 token handling, range-gate, and undo-recovery issues found during hardening.
- Fixed a startup assertion in MCPPromptRegistry caused by an em-dash in an argument description.
- Fixed several issues found in live MCP testing (Ctrl+Z scope, map refresh, toast polish), and fixed a protocol-toggle bug alongside the Phase 5d AI History Window work.
- Kept the countdown drain active until the gate actually closes rather than until it reads 0 seconds, constrained the drain fill to the visible button shape, and ticked listeners while the gate is open so the depleting countdown repaints correctly.
- Restored section spacing between the port row and the Network Connections table.

### Chore/Internal
- Regenerated MCP tool/group manifests.
- ASCII-ified all `juce::String` literals in the MCP module.
- Split the MCP help card out from the base HelpCard component.
- Dropped the local SimpleWeb OPTIONS patch now that the equivalent fix merged upstream (previously routed OPTIONS through SimpleWeb's default_resource as an interim local patch).
- Bumped version to 1.0.0beta11 and 1.0.0beta12.

## v1.0.0beta10 — 2026-04-25 (also tagged v1.0.0beta11)

### Chore/Internal
- Began work on an MCP (Model Context Protocol) tool generator script, including its initial generated outputs (Phase 0).
- Removed a redundant `.docx` documentation file, keeping the `.pdf` as the single source of reference.

## v1.0.0beta9 — 2026-04-25

### Changed
- Android Remote sync now sends OSC bundles instead of paced individual messages: initial connection/reconnection state dumps that took roughly a second of trickled packets, and per-input tab-switch refreshes that could take up to 1.9s, now complete in a couple of datagrams.
- Cluster member position updates to Android Remote are sent as a single OSC bundle per move instead of per-member throttled messages, removing the 20Hz cluster-move throttle and the resulting choppiness on non-reference cluster members. JUCE-local cluster edits (MapTab, Stream Deck, SpaceMouse, presets) now also reach the tablet atomically.
- Windows installers for both the main app and the VST3 plugins now bundle LICENSE, README, and third-party notices alongside the installed binaries, and show the GPL-3.0 license on the Inno Setup acceptance page.

### Chore/Internal
- Large documentation pass on the planned MCP (Model Context Protocol) server: filled gaps needed to start implementation cold, added AI-facing knowledge resources (help cards, glossary), audited the spec set for drift against the current app, proofread the knowledge base, and renamed "Level Maps" to "Gradient Maps" throughout to match the in-app feature name.

## v1.0.0beta8 — 2026-04-24

### Fixed
- Output attenuation and per-reverb return attenuation sliders now actually affect the audio. Previously these values were stored, editable via the GUI, and synced over OSC, but never read by the DSP engine, so moving them had no audible effect. Both are now wired in as smoothed per-channel gains (-92 dB floor, 50 ms ramp), using the same approach as the master level gain: output attenuation scales the WFS output buffer between the reverb-return mix and master gain, while reverb attenuation scales each reverb's wet signal before it mixes into the outputs.

## v1.0.0beta7 — 2026-04-24

### Fixed
- ADM-OSC round-trip fixes: corrected the default Y-axis flip, auto-enable OSC transmit when the protocol changes, repaint the map on inbound positions, and stripped leftover exploratory debug traces from ADM-OSC message handling.

### Changed
- Renamed all five Track plugin variants to use dashes instead of parentheses ("WFS-DIY Track - X"), so the VST3 bundle names, in-plugin display name, and installer file list agree with each other.

### Documentation
- Refreshed the WFS-UI CSV/Markdown documentation set (config, network, input, output, reverb, audio patch, plus new clusters and array-wizard/plugin references) to match the current app, so the MCP server can be generated from accurate source data.
- Updated the top-level READMEs and CLAUDE.md to reflect current project status, including per-platform code-signing notes, corrected macOS notarization guidance, refreshed GUI structure/core systems status, and links to companion projects.

## VST-AU_0.0.1 — 2026-04-23 (plugin packaging milestone)

### Added
- Wired the ADM-OSC plugin variants (Cartesian/Polar Track) end-to-end with bidirectional messaging to a new dedicated Master UDP receiver, including echo suppression so async parameter updates don't feed back into the app.
- Added a build-stamped, 4-line diagnostic status pane to each plugin editor for inspecting OSC activity without a debugger.
- Cylindrical and Spherical Track plugin variants now exchange position with the app, converting their native display coordinates (R/Theta/Z, R/Theta/Phi) to/from Cartesian at the app boundary while keeping DAW automation in native units.
- Plugins now fetch current values from the OSCQuery server on connect and subscribe, instead of sitting with stale defaults until the user touches something in the app.
- Added attenuation-law controls to the Cartesian Track plugin (switchable Log or 1/d law, each with its own dial), with app-side subscription to the new OSC paths.
- Plugin slider value labels are now double-click editable for direct numeric entry.

### Fixed
- Fixed OSCQuery HTTP requests being mis-encoded (the bare `?HOST_INFO` query was turned into `?HOST_INFO=`) and fixed the plugin WebSocket client failing to connect due to a malformed `ws://host:port` string.
- Corrected the Cartesian position range to -50..+50 m to match the app's center-referenced coordinate system, fixed a zero step interval that made the increment/decrement buttons no-ops, and fixed the value display losing its 2-decimal formatting.
- Fixed a regression where a slider-range override broke slider-to-parameter sync after JUCE 8's attachment setup.
- Changing a Track's input ID now correctly re-registers it with the OSC bridge so both directions of traffic follow the new channel.
- Corrected the app's position/offset X/Y/Z ranges in WFS-UI_input.csv from 0..50 to -50..+50 to match actual behavior.

### Changed
- The Y-axis flip is now the default for Cartesian mappings, and ADM-OSC targets auto-enable transmit on protocol change.
- Renamed the Master UDP field label to "OSC send port (Tx)" for clarity.

### Chore/Internal
- Made the plugin build portable on macOS (Xcode generator fallback when Ninja is unavailable, dropped an unnecessary TLS link dependency, renamed Track products to avoid shell-escaping issues) and added code-signing timestamp support ahead of notarization.
- Proofread and corrected the MCP tool-surface and resources specs (default input count, terminology, azimuth convention, translation coverage).

## v1.0.0beta6 — 2026-04-23

### Added
- New WFS-DIY VST plugin subproject, bringing the wave-field-synthesis engine to a plugin host in addition to the standalone app.
- Per-cluster show/hide toggle for assigned inputs on the Map view, making it easier to declutter dense layouts.

### Fixed
- Fixed dragging a hidden cluster marker and made the selection ring stay visible/persistent afterward.
- Fixed the sampler's pressure-to-level response not reaching true silence at its zero end.

### Changed
- Input and output patch matrices now track the actual sound-card channel count instead of a fixed assumption.

### Chore/Internal
- Bumped version to 1.0.0beta6 and added the Documents folder usage description (for platform permission prompts).

## v1.0.0beta5 — 2026-04-22

### Added
- OSC parameter ramps for smooth transitions on parameter changes, plus a version label in the UI
- Sampler compound marker, synced cluster LFO toggle, and broadcast preset axes
- Signal-presence tint on Input Patch hardware-input headers to show which inputs are live
- Arrow-key cluster moves, with the cluster selection ring now staying visible while editing
- MCP server documentation and a VST control PRD

### Fixed
- Sampler scope leak and loss of scope-window persistence
- Crash (OSCFormatError) when toggling the sampler while Remote was connected
- Help card cycling now visits every card regardless of the starting position
- Light-theme colour regressions across several components
- Crash on HID shutdown caused by a stray `hid_exit` call in device destructors
- Light-theme colours and stale state left over after reloading a project

### Chore/Internal
- Updated THIRD_PARTY_NOTICES.md
- Ignored the Xcode xcuserdata directory in version control

## v1.0.0beta4 — 2026-04-19

### Added
- Cluster LFO state and presets are now sent to the Android Remote.
- Diagnostics panel and update banner gained localization, rescaling, and a dynamic color scheme.

### Fixed
- Added a delay-target smoother to fix Doppler-related jitter.
- Closed a shared-input vector race condition in the binaural processor (thread-safety audit item B5).
- Closed a levels-matrix race condition in the reverb feed thread (thread-safety audit item B1).
- Additional thread-safety audit follow-up fixes across tiers A, B2, C, and D.
- Fixed help cards and snapshot-scope dirty tracking.
- Fixed sampler dirty tracking in snapshot scope, with thread-safe notifications.
- Fixed diagnostics auto-expand to trigger only on crash, and adjusted the overview button position.

### Chore/Internal
- Suppressed a C4459 compiler warning in the third-party ASIO `deadline_timer_service.hpp` header.

## v1.0.0beta1 — 2026-04-08 (also tagged v1.0.0beta2, v1.0.0beta3)

### Added
- Added a diagnostics section with crash detection (also bumps the app version to 1.0.0beta3)
- Added a GitHub release update checker
- Added support for opening `.wfs` project files, plus a Windows installer
- Added an app icon for all platforms

### Chore/Internal
- Removed stale file references from the project
- Added a post-build script to copy language files on Windows Release builds
- Updated the Xcode project with Developer ID signing configuration

## v1.0.0_beta1 — 2026-04-06

This release covers the entire foundational build-out of WFS DIY, from initial project scaffolding through 546 commits to the first beta tag.

### Foundation & Architecture
- Established the core multithreaded N-to-M WFS audio engine, with configurable delay/level routing, CPU monitoring, and dual-algorithm processing refactored into dedicated wrapper classes.
- Introduced a ValueTree-based parameter system underpinning config save/load, snapshots, and per-channel state throughout the app.
- Explored GPU-Audio SDK integration for offloaded processing (later disabled to unblock the build) and added a dedicated WFS input buffer processor architecture.

### Core GUI & Tabs
- Built out the main tabbed interface from scratch — Config/System Config, Network, Input, Output, Reverb, and Audio Interface & Patching tabs/windows — consolidating what were originally separate preview windows into one main window.
- Added the interactive Map tab (2D stage view with drag, multi-select, rubber-band, secondary-touch rotation/Z, keyboard nudging) and Clusters tab for grouping and moving multiple inputs together with shared LFO/scale/rotation.
- Delivered a unified color scheme system (Default, OLED Black, Light themes), proportional UI scaling for 4K and other resolutions, and extensive layout, spacing, and alignment polish across every tab.
- Added Gradient Map and StreamDeck+ Gradient Map systems for layer/shape-based parameter control with vertex/handle editing.

### WFS & Movement DSP
- Implemented the core WFS delay/level/HF calculation matrix and the Wizard of OutZ speaker-array design helper.
- Added Floor Reflections processing, Live Source Tamer feedback prevention, Input Speed Limiter (tanh smoothing), Sidelines automatic edge muting, and Flip X/Y/Z position mirroring.
- Added AutomOtion input-position automation (polar/cylindrical/spherical modes, audio-triggering, path recording, stage-aware repositioning) and LFO-based position modulation for both individual inputs and clusters.
- Added cylindrical/spherical/box/dome stage shape types with center-referenced coordinates and distance constraints.

### Reverb Engine
- Built the ReverbEngine from the ground up, implementing all three algorithms — FDN, SDN, and IR convolution — with pre/post-processing DSP, per-node parallelization, and crossfaded algorithm switching.
- Hardened IR convolution with non-uniform partitioning, buffer trimming/dynamic ranges, two-phase (silent-hold) loading, and dropout handling; added compressor/expander and gain-reduction metering, and fixed numerous reverb crash and state-persistence bugs.

### Binaural Renderer & Sampler
- Added the Binaural Renderer with solo monitoring, studio preview mode, and a custom rotation-dial UI, later moved off the audio callback for performance.
- Added a Sampler engine with grid/sets/pressure mappings, snapshot scope integration, and ROLI Lightpad-driven transient triggering.

### Networking, OSC & Remote Protocol
- Built the OSC networking stack: an OSCQuery discovery server (spec-compliant, WebSocket LISTEN support), source/sender-IP filtering, a network logging window, and QLab cue export/network integration.
- Added tracking-protocol support: generic OSC tracking, PSN (PosiStageNet), RTTrP, and MQTT, plus a 1-Euro adaptive smoothing filter for tracking input positions.
- Implemented the Remote control protocol end-to-end (handshake/heartbeat, full state dump and resync on reconnect, coalesced/rate-limited updates) along with ADM-OSC mapping (interactive Cartesian/Polar panels) and an Android remote app with a visual grid UI as an alternative sampler controller.

### Hardware Controllers
- Added 3Dconnexion SpaceMouse support for positioning inputs and clusters (pan/zoom, twist-to-fit, LED feedback, macOS support).
- Added Elgato Stream Deck+ integration spanning nearly every tab (Map, Movements, Outputs EQ, Network, System Config, Reverb, Patch window) with per-page dial bindings and bidirectional sync.
- Added Xencelabs Quick Keys and ROLI Lightpad Block controller support, including auto-topology and visual zone assignment.

### Accessibility & Localization
- Added a full internationalization framework with translations for French, Italian, German, Chinese, Portuguese, and Korean covering essentially the entire application.
- Added Text-to-Speech accessibility support, announcing parameter names/values on hover and tab changes, enabled by default.
- Built an in-app Help Card system covering every tab plus a spotlight-style Getting Started tutorial wizard.

### Build, Platform & Documentation
- Fixed and hardened the macOS build (app bundling, SSL, hidapi paths, audio permissions, driver handling) alongside the primary Windows build.
- Converted JUCE, the ASIO SDK, ROLI Blocks Basics, and juce_simpleweb into git submodules for easier project sharing, and tracked Projucer-generated JuceLibraryCode so fresh clones build without re-running Projucer.
- Added the GPLv3 license, third-party attribution notices, and academic references; substantially expanded README and CLAUDE.md project documentation throughout development.
- Bumped the version to 1.0.0 ahead of the first beta tag.
