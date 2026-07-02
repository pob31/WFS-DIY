# WFS-DIY Control-Plane Map (Parameter CSV → ValueTree → OSC → MCP)

> **Status:** analysis-only. Nothing in WFS-DIY was modified to produce this document.
> **Method:** every factual claim cites `path:line`. Claims are tagged **[V]** (verified — the
> cited lines were opened and read, and in most cases independently re-checked by a second
> adversarial pass) or **[I]** (inferred — reasoned from surrounding code, not line-proven).
> Line numbers are accurate to within 1–2 lines; where a range is given the quoted token
> sometimes sits at the last line of the range.
> **Placeholder:** the shared core library is referred to throughout as `spatcore` (same name as
> [audio-engine-map.md](audio-engine-map.md), which covers the audio-rate DSP from HANDOFF-01).
> **Scope:** the control plane only — parameter definitions, ValueTree state, persistence, OSC,
> OSCQuery, and the embedded MCP server. The CPU/GPU audio engine is
> [audio-engine-map.md](audio-engine-map.md); §3 here is the control-side complement to that
> document's §1/§7.

---

## 0. Reconciliation — where the brief's mental model diverges from the code

The task brief was written without intimate knowledge of the codebase. Several landmarks are
wrong or imprecise. Per instructions, **the code wins**; the differences are flagged up front and
substantiated in the sections that follow.

| Brief's landmark | Code reality | Where |
|---|---|---|
| CSVs "auto-generate **both the OSC address space and the MCP tool surface**." | The generator (`tools/generate_mcp_tools.py`) emits **only** two MCP JSON artifacts. The **OSC address space is hand-written C++** (`OSCMessageRouter` static maps). The CSV OSC-path column is a *spec* the C++ replicates by convention; it is never consumed at runtime. There are **three independently-maintained parameter surfaces** reconciled only by a runtime auditor. | §1, §4 |
| "~414 tools." | **393 generated** (330 setters + 63 nudges) + **~25 hand-written** − **2 name collisions** ≈ **416 registered**. The dispatcher's own comment says "393 tools". `audioPatch.csv` yields **0** tools. | §1.4, §5.3 |
| "Three-tier confirmation model (**read / write / destructive**, or similar)." | Tiers are **1 = immediate**, **2 = confirm-token handshake**, **3 = destructive, needs an open safety-gate** — an *action-risk* ladder, not a read/write/destructive split. Plus a master **AI-enabled** toggle (default **off**) and two operator time-windows. | §5.4 |
| "Streamable HTTP … built on **JUCESimpleWebServer** (OPTIONS fix contributed upstream) — vendored, submoduled, or fetched? which revision?" | **Vendored** (source copied in), **not** a submodule, **not** fetched. GPLv3, `benkuper/juce_simpleweb` module v1.0.0 with **asio 1.16.1** + **OpenSSL 1.1.1g** headers flattened in. The OPTIONS fix was a **local patch, later dropped** once **upstream PR #5** merged. Transport is **request/response only** — no SSE, no `Mcp-Session-Id` sessions. | §5.1, §5.6 |
| "Port 7400 … **fallback to the next three port numbers**." | **The fallback does not exist.** One `constexpr kDefaultPort = 7400`; `MCPTransport::start` binds exactly the passed port and **swallows bind failures** (reports `running=true` on a dead socket). "Port-fallback" survives only in stale UI comments. | §5.2 |
| "Undo/redo … **JUCE `UndoManager`, custom transaction log, or both**." | **Both, split by origin.** AI/MCP edits use a **custom transaction log** (`MCPUndoEngine` + two ring buffers), never `UndoManager`. Human UI edits use **six per-tab `juce::UndoManager` instances**. MCP-origin writes deliberately pass a `nullptr` UndoManager so the two channels never mix. | §2.7, §5.5 |
| "State **presumed** `ValueTree`; audio thread should not touch it." | **Confirmed** raw `juce::ValueTree` in a bespoke wrapper (**not** APVTS). Main audio path is ValueTree-clean; **two real RT-safety violations existed and are now both fixed**: `BinauralProcessor` read the tree on a realtime thread (**fixed 2026-07-02**, §3.5), and three tracking receivers wrote the tree off the message thread (**fixed 2026-07-02**, §3.6 — now marshalled via a `TrackingIngestQueue` drained on the message thread). | §2.1, §3 |
| "AI batch undo/redo ('Phase 5') — what is one undo step when the AI places a 16-speaker arc?" | **One step.** A batch is captured as a **single `ChangeRecord` with a `subWrites` vector**; one `mcp_undo_last_ai_change` reverts all ~80 writes together. | §5.5 |

Two concrete defects were also found and are catalogued in **Appendix B**: a tier-override
key-casing typo that silently downgraded master-level safety (**fixed 2026-07-02**, `e84716f`,
and the tier source moved into an explicit CSV `Tier` column, `2a3bd70`), and a build step that
no-ops silently when Python is absent (still open as a build-design issue).

---

## 1. CSV schema & code generation

### 1.1 The source files and their real format

Parameter definitions live in eight files under `Documentation/`. Seven are consumed by the
generator; the eighth (`OSC-Remote_InputParameterTab.csv`) is a spec mirror that is **not**
consumed (`tools/generate_mcp_tools.py:1293-1301` **[V]**).

Despite the `.csv` extension **all files are TAB-separated** — the reader opens them with
`csv.reader(f, delimiter="\t")` (`tools/generate_mcp_tools.py:331` **[V]**). Columns are mapped
by **header name**, not position (`HEADER_ALIASES`, `:305-325`; `read_csv`, `:328-353` **[V]**),
which lets three different column layouts coexist without the generator branching on width:

| Layout | Files | Columns | Distinguishing columns |
|---|---|---|---|
| 13-col | `WFS-UI_config.csv`, `WFS-UI_network.csv` | 13 | no Formula, no Array-value, **no OSC-path column** |
| 18-col | `WFS-UI_output.csv`, `WFS-UI_reverb.csv`, `WFS-UI_clusters.csv`, `WFS-UI_audioPatch.csv` | 18 | + Formula, Array value, **OSC path**, OSC remote path, Keyboard shortcuts |
| 19-col | `WFS-UI_input.csv` | 19 | 18-col + `OSC "inc"/"dec"` column + `OSC path optional value` |

All layouts carry a **`Tier`** column immediately after `Default` (added 2026-07 — see §5.4).

**[V]** (column counts confirmed by field-count; matches `Documentation/MCP/specs/GENERATION_SCRIPT_SPEC.md`).

### 1.2 Column semantics

| Column | Meaning / how the generator uses it | Cite |
|---|---|---|
| **Section** | UI grouping; drives `group_key` and the middle segment of the tool name. Coordinate suffixes `(Cylindrical\|Spherical\|Cartesian)` are stripped. | `:615,694` **[V]** |
| **Label** | Human label; fallback description; value-arg description. | `:746` **[V]** |
| **Variable** | **The paramID** — primary identity for naming, tier/ignore lookup, dedup, family/band detection. Blank Variable → row dropped. | `:344-345,662-666` **[V]** |
| **UI** | Control kind. Values in `NON_PARAMETER_UI` (`Display label`, `2D grid`, `Internal state`, `meter`, …) disqualify a row from becoming a tool. | `:640-671` **[V]** |
| **Type** | `INT` / `FLOAT` / `STRING` / `IP (4x INT)` / `Array INT`; matched by `.startswith()` on the first token → JSON-Schema value type. | `:925-1016` **[V]** |
| **Min / Max** | Numeric bounds → schema `minimum`/`maximum` when parseable. | `:949-976` **[V]** |
| **Default** | Coerced to schema `default`; enum defaults reverse-mapped; template/prose defaults dropped to null. | `:806-867` **[V]** |
| **Tier** | Explicit MCP confirmation tier `1/2/3` — the **primary** tier source. Blank → override file → heuristic (see §5.4). | `parse_tier_cell`, `process_row` tier block **[V]** |
| **Formula** | UI normalization math — **ignored by the generator**. | (no consumer) **[V]** |
| **Unit** | Appended to description + value-arg description. | `:751,959` **[V]** |
| **enum** | `;`-separated. `Label (N)` form yields explicit non-sequential int IDs; single-item cells dropped as stray comments. | `:491-531` **[V]** |
| **OSC path** (17/18-col) | Explicit path; first token taken, trailing `<ID> <value>` placeholders stripped. | `:1074-1085` **[V]** |
| **OSC inc/dec** (18-col) | `y` → `supports_relative`, generates a **nudge** tool. | `:719,1176` **[V]** |
| **OSC path optional value** (18-col) | contains "transition" → adds a `transition_seconds` arg. | `:1019-1026` **[V]** |
| **Hover** | Primary description source; missing hover emits a warning. | `:746,1202-1207` **[V]** |

**Tier is now an explicit CSV column** (`Tier`, primary source) — see §5.4 for the full
resolution order and the retained override/heuristic fallbacks.

### 1.3 One fully-annotated row: `inputAttenuation`

Source row (`Documentation/WFS-UI_input.csv:7` **[V]**):

```
Section=Attenuation  Label=Attenuation  Variable=inputAttenuation  UI=H Slider
Type=FLOAT  Min=-92.0  Max=0.0  Default=0.0  Formula=20*log10(...)  Unit=dB
enum=(empty)  OSC path=/wfs/input/attenuation  OSC inc/dec=y  Hover=Input Channel Attenuation.
```

Traces to the generated record (`generated_tools.json:5928-5966` **[V]**):

1. Prefix `input` stripped → `Attenuation` → snake `attenuation` (`strip_variable_prefix :443`, `variable_to_snake :431`).
2. Section `Attenuation` equals the param → redundant, dropped (`_is_redundant_section :728`) → tool name **`input_set_attenuation`** (`:713-715`).
3. Input CSV → per-channel → a required first arg **`input_id`**, range `[1,64]` (`CHANNEL_ID_RANGE :794-799`).
4. FLOAT value arg, `minimum -92.0`, `maximum 0.0`, `unit "dB"`, `default 0.0`.
5. `OSC inc/dec=y` → `supports_relative:true`, `relative_tool_name:"input_nudge_attenuation"`, plus a `nudge_tools[]` entry with `direction`/`amount` args (`:1209-1250`).
6. `Tier` CSV column = `2` (snapshotted from the former `inputAttenuation:2` override) → **tier 2**, a `TIER-2` suffix on the description, and a `confirm` string property injected into the schema (`:1046-1059,1156`).
7. `internal_osc_path:"/wfs/input/attenuation"` and `internal_variable:"inputAttenuation"` are recorded as **metadata for the C++ dispatcher** (the runtime does NOT dispatch OSC from this).

### 1.4 What is generated, when, and how many

**Timing — build-time prebuild, with a silent-fallback trap.** The generator runs as a Visual
Studio prebuild command: `... && (where python >nul 2>&1 && python tools\generate_mcp_tools.py) & exit /b 0`
(`WFS-DIY.jucer:555`, `Builds/VisualStudio2022/WFS-DIY_App.vcxproj:110,170` **[V]**). The
`where python` guard + `exit /b 0` mean the step **no-ops gracefully if Python is absent**,
falling back to the committed JSON. A SHA-256 **input-hash fast-path** (`hash_inputs :1304-1321`;
up-to-date check `:1383-1391` **[V]**) skips regeneration when the seven CSVs + two override files
are unchanged; `write_json` additionally byte-compares to avoid dirtying mtimes (`:1324-1353`).

**Generated artifacts — exactly two files, nothing else:**

- `Source/Network/MCP/generated_tools.json` (473 KB) — tool records (name, description, **per-tool
  JSON Schema**, tier, `csv_section`, `group_key`, `supports_relative`, `domains`,
  `internal_osc_path[_template]`, `internal_variable[_template]`, enum maps) plus `nudge_tools[]`,
  `ignored_parameters[]`, `warnings[]` (`:1484-1493` **[V]**).
- `Source/Network/MCP/generated_groups.json` — `group_key → [tool names]`, 62 groups (`:1495-1500` **[V]**).

**NOT generated (all hand-written C++):** the OSC handlers/address space (§4), the ValueTree
structure (§2 — the loader *reflects* the live tree at runtime, `MCPGeneratedToolLoader.cpp:55-60`
**[V]**), UI bindings, and the ~25 compound MCP tools (explicitly out of scope,
`GENERATION_SCRIPT_SPEC.md:176-179` **[V]**).

**Tool count** (counted in `generated_tools.json` **[V]**):

| Namespace | setters | Namespace | setters |
|---|---|---|---|
| input | 137 | system | 28 |
| reverb | 59 | cluster | 27 |
| network | 51 | audio | **0** |
| output | 28 | | |

→ **330 setters + 63 nudges (input-only) = 393 generated** (matches `MCPDispatcher.cpp:176`
comment). Plus ~25 hand-written (§5.3), minus 2 name collisions (`input_set_attenuation`,
`input_set_name`, which the hand-written variants override) → **~416 registered** [I].

**Indexed/array parameters are NOT exploded into per-instance tools** — one tool takes a
channel-id arg (`input 1-64`, `output 1-128`, `reverb 1-32`, `cluster 1-10`,
`CHANNEL_ID_RANGE :794-799` **[V]**). Three sub-index collapse mechanisms: `<band>` placeholder →
a `band` arg; numeric-suffix families (`inputArrayAtten1..10`) → one tool + an `array` arg (only
member `1` survives, guarded by a family pre-scan so `reverbRT60` is not mistaken for member 60,
`:1396-1413,1129-1132`); within-CSV dedup by Variable (`:1448-1452` **[V]**).

### 1.5 Naming & addressing conventions

- **`CSV_NAMESPACE`** (`:24-32`): input→`input`, output→`output`, reverb→`reverb`,
  clusters→`cluster`, network→`network`, config→**`system`**, audioPatch→`audio`. This is the
  tool-name and group-key prefix. **[V]**
- **Variable-prefix stripping** (`VARIABLE_PREFIXES :41-50`, longest-match): input strips `input`,
  network strips `network/tracking/admCart/admPolar/admOsc/findDevice`, config strips nothing
  (PascalCase). **[V]**
- **Abbreviation/word expansion** (`ABBREVIATIONS :54-121`, `WORD_EXPANSIONS :124-129`): `LFO→lfo`,
  `EQfreq→eq_frequency`, `atten→attenuation`, `RT60→rt60`, etc. **[V]**
- **Tool names use underscores only** (OpenAI regex `^[A-Za-z0-9_-]{1,64}$`) — zero dot-containing
  names in the output. The spec's dotted golden examples (`input.set_attenuation`) are **stale**;
  code + tests emit `input_set_attenuation` (`test_generate_mcp_tools.py:83` **[V]**).
- **OSC path derivation** (`derive_osc_path :1062-1093`): explicit column when present, else
  `OSC_PATH_CONVENTION` — config→`/wfs/config/<Variable>`, network→`/wfs/network/<Variable>`. Note
  reverb *algorithm* params carry explicit `/wfs/config/reverb/...` paths, so a `reverb`-namespace
  tool can emit a `/wfs/config/...` OSC path (`reverbRT60 → /wfs/config/reverb/rt60`, **[V]**).

---

## 2. State model (ValueTree)

### 2.1 Representation: a bespoke wrapper over a raw ValueTree — NOT APVTS

State is a **single raw `juce::ValueTree state;`** held privately in a hand-written manager,
`WFSValueTreeState` (`Source/Parameters/WFSValueTreeState.h:462` **[V]**). It is **not** a
`juce::AudioProcessorValueTreeState` — there is no `createParameterLayout`, `RangedAudioParameter`,
or attachment machinery. The class **is itself** a `juce::ValueTree::Listener` (`.h:32` **[V]**).
Two wrapper layers exist: `WfsParameters` (a legacy *string-name* facade whose own docstring says
"For new code, prefer using WFSValueTreeState directly", `Source/WfsParameters.h:10-17` **[V]**) →
`WFSValueTreeState` (typed `Identifier` API) → raw `ValueTree`.

### 2.2 Tree shape

Root type `WFSProcessor` with a `version="1.0"` property (`WFSValueTreeState.cpp:2226-2227` **[V]**).
Indexed children carry a **1-based `id` property**, but most per-channel lookups index by **child
order** (`getInputState → inputs.getChild(channelIndex)`, `:234-240`); `id` is authoritative only
in tree-merge/back-fill matching (§2.6) and in listener channel-recovery (`:2159-2164`). Built by
`initializeDefaultState` (`:2224-2540` **[V]**):

```
WFSProcessor                          [version="1.0"]
├─ Config
│  ├─ Show          [showName, showLocation, autoPreselectDirty]
│  ├─ IO            [inputChannels, outputChannels, reverbChannels, algorithmDSP, runDSP]
│  ├─ Stage         [stageShape, stageWidth/Depth/Height, domeElevation, speedOfSound, temperature, …]
│  ├─ Master        [masterLevel, systemLatency, haasEffect, reverbsMapVisible]
│  ├─ Network       [networkInterface, networkCurrentIP, networkRx{UDP,TCP}port, …]  └─ Target* (≤6)
│  ├─ ADMOSC        └─ ADMCartMapping* (4) └─ ADMCartAxis (3) ; ADMPolarMapping* (4)
│  ├─ Tracking      [trackingEnabled/Protocol/Port, offset/scale/flip XYZ]
│  ├─ Clusters      └─ Cluster (10) [id, clusterReferenceMode, …] └─ ClusterLFO [22 props]
│  ├─ Binaural      [binauralEnabled/…, inputSoloStates(csv string)]
│  ├─ UI            [streamDeckEnabled, samplerEnabled, lightpad*, …]
│  └─ ClusterLFOPresets (lazily created; 16 slots)
├─ Inputs  └─ Input* (id) ├─ Channel ├─ Position(~26 props) ├─ Attenuation | Directivity |
│                          LiveSourceTamer | Hackoustics | LFO | AutomOtion | Mutes
│                          ├─ GradientMaps └─ GradientLayer* └─ GradientShape*   └─ Sampler
├─ Outputs └─ Output* (id) ├─ Channel ├─ Position ├─ Options └─ EQ └─ Band (6)
├─ Reverbs ├─ Reverb* (id) → Channel/Position/Feed/EQ(Band×4)/Return
│          └─ ReverbAlgorithm, ReverbPreComp, ReverbPostEQ(→PostEQBand×4), ReverbPostExp  (global siblings)
└─ AudioPatch [driverMode, audioInterface] ├─ InputPatch └─ OutputPatch  [patchData csv-packed]
```

(node/line map: `WFSValueTreeState.cpp:2254-2501` **[V]**). Note `Reverbs` holds both indexed
`Reverb` children **and** four global sibling sections, which is why `getReverbState` walks only
`Reverb`-typed children (`:270-285` **[V]**).

### 2.3 Defaults, constraints — applied *outside* the setters

Defaults are compile-time constants in `WFSParameterDefaults.h` (`inputChannelsDefault=8`,
`outputChannelsDefault=16`, per-param `*Default/*Min/*Max`, `:11-1012` **[V]**), applied only at
**three moments**: construction, `resetToDefaults()`/`resetXToDefaults()`, and **schema back-fill
on load** (`ensureCompleteSchema` + `backfillFromTemplate`, `:1964-2048`). An individual
`setProperty` writes **exactly what it is given** — no per-write default re-application **[V]**.

Range clamping is **not** inside the generic `setParameter`/`setInputParameter`/… paths — those
call `tree.setProperty(id, value, undoMgr)` unclamped (`:328-436` **[V]**). Enforcement lives
**caller-side** in `WFSConstraints` (stage-aware geometry clamping invoked by UI/OSC callers) and
in output array-propagation (`clampOutputParamToRange`, `:633-676`). **Consequence:** a write that
bypasses those callers can persist an out-of-range value. This is the single most important state
seam for `spatcore` — see §6 and [open-questions-control.md](open-questions-control.md).

### 2.4 Listener architecture — decentralized broadcast-and-filter

No central listener. Three tiers coexist **[V]**:

1. **`WFSValueTreeState` itself** — its `valueTreePropertyChanged` (`:2150-2198`) enforces
   WFS invariants (cluster tracking/shared-position) **and** fans out to a **callback registry**
   (`addParameterListener`/`notifyParameterListeners`, `:1659-1665,3250-3264`) keyed by
   paramId+channelIndex.
2. **`ParameterDirtyTracker`** — a *second* independent `ValueTree::Listener` + `AsyncUpdater`
   marking which per-input scope items the user touched, for snapshot auto-preselect
   (`ParameterDirtyTracker.h:31-47` **[V]**). Orthogonal to undo and to the values themselves.
3. **~40 direct subscribers** across GUI/DSP/network attach via `WFSValueTreeState::addListener` →
   `state.addListener` (`:1679-1687`), so **every listener sees every whole-tree change and
   self-filters**. `OSCManager` (OSC-out feedback, §4.4) and `WFSCalculationEngine` (DSP, §3) both
   consume changes this way.

### 2.5 Snapshots

Whole-tree/per-node copies use JUCE primitives: `replaceState` →
`copyPropertiesAndChildrenFrom` (`:1914`); `WFSFileManager` export builds containers with
`child.createCopy()`; scoped recall uses `mergeTreeRecursive` (§2.6). There is no atomic
snapshot-to-audio-thread handoff at the state layer — the DSP handoff is a separate float-array
mechanism (§3.4).

### 2.6 Versioning & migration — the `version` field is inert

Every root/section carries a `version` attribute ("1.0"; snapshots "2.0"), but **no code ever
reads it** — a grep finds only `setProperty(version,...)` sites, never `getProperty(version)`
**[V]**. So there is **no schema-version-gated migration**. Load validation is purely structural
(root must be `WFSProcessor` with `Config`/`Inputs`/`Outputs`, `:2124-2139`). Migration is
**presence-based/heuristic**, run on the `replaceState` path: `migrateADMOSCSection` (legacy flat
`admOscOffsetX` → nested mapping, `:2050-2108`), `ensureInputAdmMappingProperty`, and
`ensureCompleteSchema` back-fill, plus lazy `ensure*Section` creation inside accessors. Two
divergent migration paths (whole-config `replaceState` vs. per-section load fan-out) must stay in
sync — see [open-questions-control.md](open-questions-control.md).

### 2.7 Undo at the state layer — six per-tab UndoManagers, origin-aware

Undo is built into the wrapper with an **unusual per-tab design**: `juce::UndoManager
undoManagers[UndoDomain::COUNT]` — **six** managers, one per tab (Input/Output/Reverb/Map/Config/
Clusters, `.h:463`, enum `:10-19` **[V]**). `getActiveUndoManager()` is threaded into most
setters; a `ScopedUndoDomain` RAII helper switches the active domain on tab change. **MCP-origin
writes deliberately pass `nullptr`** — `getActiveUndoManager()` returns null when the thread-local
origin tag is `MCP` (`:1601-1611` **[V]**), because AI edits have a **separate** undo channel
(`MCPUndoEngine`, §5.5). This origin-aware split is why the brief's "UndoManager or custom log" is
**both**. Managers are attached **per-mutation** (JUCE's transaction-token model), not via
`ValueTree::setUndoManager`.

---

## 3. Threading & real-time safety

This is the most safety-critical section: `spatcore` must preserve these guarantees for app
authors who are not thinking about them. JUCE `ValueTree` is **not thread-safe**. The intended
model — *message thread owns the tree; the audio thread reads plain floats; other producers
marshal* — mostly holds, **with two concrete violations** the core must fix rather than inherit.
This complements [audio-engine-map.md](audio-engine-map.md) §1/§7, which covers audio-**buffer**
RT safety; here the shared resource is the **ValueTree**.

### 3.1 Thread inventory (who touches the ValueTree)

| Thread | Role | Touches ValueTree? |
|---|---|---|
| JUCE **message thread** | UI, all `juce::Timer`s, ValueTree owner | **Yes — the designated owner** |
| **Audio callback** (`getNextAudioBlock`) | driver thread | No **[V]** |
| `OutputBufferProcessor` (N RT threads) | per-output DSP | No — reads plain `const float*` |
| **`BinauralProcessor`** (RT thread) | binaural render | ~~Yes — VIOLATION~~ **fixed 2026-07-02** — reads `RtParams` snapshot (§3.5) |
| OSC socket receivers (`juce::Thread`) | UDP/TCP ingest | No — only `ingestQueue->push()` |
| **PSN / RTTrP / MQTT tracking receivers** (`juce::Thread`) | tracking ingest | ~~Yes — VIOLATION (§3.6)~~ **fixed 2026-07-02** — only `ingestQueue.push()`; writes drain on the message thread (§3.6) |
| SimpleWeb io_context workers (MCP HTTP) | MCP transport | No — marshals to message thread |

### 3.2 UI write path (safe)

Widget → `WFSValueTreeState::setInputParameter/setParameter` →
`tree.setProperty(paramId, value, undoManager)` (`WFSValueTreeState.cpp:355,407` **[V]**). All UI
handling and the ~200 Hz (5 ms) control timer run on the message thread
(`MainComponent.h:98-99`, `timerCallback` `.cpp:5166` **[V]**). Safe because it is the single
owning thread. Audio visibility then flows through §3.4.

### 3.3 OSC write path (safe)

Socket receiver `run()` reads a datagram and only calls `ingestQueue->push(std::move(data), …)`
(`OSCReceiverWithSenderIP.cpp:109-115`; wiring `OSCManager.cpp:205-210` **[V]**). `push` inserts
into a coalescing map/FIFO **under a `juce::CriticalSection`** — the queue is **lock-guarded, NOT
lock-free**, contrary to any "lock-free FIFO" assumption (`OSCIngestQueue.cpp:147`, `.h:100-102`
**[V]**). It **drains on the message thread** via `OSCIngestQueue : private juce::Timer`
(`.h:40`, `.cpp:24-27`) → `OSCManager::dispatchIngestedItem` → … → `state.setParameter(...)`. The
socket thread never writes shared audio state directly. TCP is identical (`OSCTCPReceiver.cpp:257-268`).

### 3.4 MCP write path (safe — the exemplar) and the handoff to the audio thread

**MCP is the reference pattern.** The HTTP request lands on a SimpleWeb io_context worker thread
(`MCPTransport::handleHTTPRequest`); the dispatcher **marshals every tool handler onto the message
thread** via `juce::MessageManager::callAsync` and blocks the worker on a `juce::WaitableEvent`
(`payload->done.wait(toolTimeoutMs)`, `MCPDispatcher.cpp:537-592`, esp. `:565,585` **[V]**). Only
inside that message-thread lambda does `state.setParameter(...)` run (`SetParameterTool.h:270`).
This is exactly the safe-handoff primitive `spatcore` should expose.

**The message → audio handoff** is a **plain-float, atomic-free publish** (not a lock-free FIFO or
pointer swap) **[V]**:

1. Message-thread 200 Hz timer runs `calculationEngine->recalculateMatrixIfDirty()`, which reads
   the ValueTree under the engine's own `positionLock` (`WFSCalculationEngine.cpp:882-935`).
2. Still on the message thread, results are copied into `MainComponent`'s `targetDelayTimesMs /
   targetLevels / hfAttenuation` **`std::vector<float>`** (`MainComponent.h:353-366`,
   `.cpp:5617-5635`).
3. The **audio callback** one-pole-smooths `target*` → `delayTimesMs/levels` on the driver thread
   (`.cpp:4763-4773`).
4. `OutputBufferProcessor` RT workers read `delayTimesMs.data()/…` via raw `const float*` captured
   at construction (`OutputBufferProcessor.h:28-43`, wired `MainComponent.cpp:4348`).

The message→audio edge (`target*`) has **no atomics, lock, or double-buffer**. It is a data race
by the C++ memory model but a **benign** one: independent, aligned 32-bit floats are not torn on
x86-64/ARM64, and the one-pole smoother absorbs any stale-vs-fresh mix. `audio-engine-map.md` §7
calls `MainComponent` the "broker" for this. **Core guarantee:** if an app author ever publishes
anything wider than an aligned scalar (a struct, pointer, or size) across this edge, they MUST use
an atomic snapshot-pointer swap or double buffer — `spatcore` should provide that primitive so
authors don't hand-roll a racy version.

### 3.5 VIOLATION A — BinauralProcessor reads the ValueTree on a realtime thread

`BinauralProcessor::run()` runs on a realtime thread (`startRealtimeThread`,
`BinauralProcessor.h:171-173` **[V]**). Its process loop calls
`binauralCalc.getNumSoloedInputs()/isInputSoloed()/calculate()` (`.h:365,371,391`), each of which
reads the ValueTree directly: `BinauralCalculationEngine` → `getBinauralState().getProperty(...)`
(`BinauralCalculationEngine.h:103-122`), and solo queries build a `juce::StringArray` via
`addTokens` (`WFSValueTreeState.cpp:1185-1263` **[V]**). That is **ValueTree traversal plus heap
allocation on a realtime thread**, racing the message thread that mutates the same Binaural
subtree. **Core guarantee:** no realtime thread may touch `ValueTree` or allocate — audio threads
must read only a plain POD snapshot published by the message thread.

> **STATUS: FIXED (2026-07-02, branch `linux-gpu-rocm-hip`).** RT threads now consume a POD
> `BinauralCalculationEngine::RtParams` snapshot published from the message thread
> (`refreshRtSnapshot()`, SpinLock copy + relaxed atomic for the audio-callback output channel);
> the `WFSValueTreeState` binaural accessors now `JUCE_ASSERT_MESSAGE_THREAD`. Adjacent lifecycle
> races (prepare-under-live-worker, enable-before-prepare, shared-buffer use-after-free in
> `releaseResources`) were fixed in the same change.

### 3.6 VIOLATION B — tracking receivers write the ValueTree off the message thread

`TrackingPSNReceiver`, `TrackingRTTrPReceiver`, and `TrackingMQTTReceiver` each derive from
`juce::Thread` and, from their own `run()` loop, write the tree via
`posSection.setProperty(inputOffsetX/Y/Z, …)` / `directivitySection.setProperty(...)` with **no
`MessageManagerLock` and no marshalling** (`TrackingPSNReceiver.cpp:213-215`,
`TrackingRTTrPReceiver.cpp:213-215`, `TrackingMQTTReceiver.cpp:591-593` **[V]**).
**`MessageManagerLock` is used nowhere in `Source/`** (grep → zero hits **[V]**). Worse, because
`setProperty` synchronously fires listeners, a tracking-thread write runs
`WFSCalculationEngine::valueTreePropertyChanged` **on the tracking thread**, reading sibling
ValueTree properties and mutating engine caches (`WFSCalculationEngine.cpp:2322-2356`). The
engine's `positionLock` protects its own vectors but **not** the concurrent ValueTree access.
Contrast `TrackingOSCReceiver`, which is **safe** — it registers as a `Listener` and is delivered
via the legacy message-thread `callAsync` path, not its own thread
(`TrackingOSCReceiver.cpp:38-41`; `OSCReceiverWithSenderIP.cpp:118-124` **[V]**). **Core
guarantee:** the ValueTree is single-writer, message-thread-only; every non-message-thread
producer MUST marshal (as MCP does) or push into a drained queue (as OSC does).

> **STATUS: FIXED (2026-07-02, branch `linux-gpu-rocm-hip`).** The three self-threaded receivers
> no longer touch the ValueTree from their `run()` loops. Each now decodes + applies its atomic
> offset/scale/flip transform on the network thread, builds a POD `TrackingUpdate`, and calls
> `TrackingIngestQueue::push` (`Source/Network/TrackingIngestQueue.{h,cpp}`, modelled on
> `OSCIngestQueue`). The queue coalesces newest-wins per tracking-id/slot — merging position and
> orientation — and drains on the message thread via a `private juce::Timer` at 60 Hz, invoking each
> receiver's existing `routePositionToInputs`/`routeOrientationToInputs`/`routePositionToInput`
> body. Those bodies (channel match, `TrackingPositionFilter`, `OriginTagScope`, `setProperty`,
> logging) now run **only** on the message thread and open with `JUCE_ASSERT_MESSAGE_THREAD`, as
> does `TrackingIngestQueue::drainBatch`. Net effect: all ValueTree writes, listener callbacks
> (`WFSCalculationEngine::valueTreePropertyChanged`), and the shared `TrackingPositionFilter` are
> message-thread-only. This is the in-repo template for the `spatcore` marshalled tracking sink
> (boundary §2.7, §3.3 rule 2). `TrackingOSCReceiver` was already safe and was left untouched.

> **Verification status — independently confirmed.** Two separate adversarial passes read every
> cited file and audited **all 18 `juce::Thread` subclasses in `Source/`**. Both violations are
> **CONFIRMED**, and **no additional off-message-thread tree-toucher was found**: the WFS matrix
> path is tree-free (the `getNextAudioBlock` body, `MainComponent.cpp:4671-5039`, has a clean
> line-bounded gap with no `getProperty/setProperty/isInputSoloed` — the next tree hit is inside
> `timerCallback` at `:5277`), `ReverbFeedThread`/`ReverbEngine`/GPU pump/metering/controller
> threads are all clean or marshal via `callAsync`, and `Violation A` also reaches the tree through
> `calculate()→getBinauralAttenuation/Delay` (`BinauralCalculationEngine.h:103-122`), not only the
> solo-state getters. The static evidence — realtime-thread `getProperty`, and `juce::Thread`
> `setProperty` with zero `MessageManagerLock` in `Source/` — is conclusive; only the crash
> *frequency* is load-dependent. Suggested empirical confirmation: a ThreadSanitizer build fed a
> 100 Hz PSN position stream to a tracked input while dragging that same input on the Map tab.

### 3.7 Rate limiting / ramping / coalescing

- **Inbound coalescing** — `OSCIngestQueue` collapses hot `(address,channel)` updates newest-wins
  (4096-key cap) + bounds a 256-entry FIFO, drains ≤32 items/tick at ~60 Hz on the message thread
  (`OSCIngestQueue.cpp:135-247` **[V]**).
- **Outbound rate limiting** — `OSCRateLimiter : juce::Timer` coalesces per `address:channel` and
  flushes ≤50 Hz on the message thread (`OSCRateLimiter.h:23` **[V]**).
- **Parameter ramping** — `OSCParameterRamper::process` steps ramps at 50 Hz from the message-thread
  timer (`MainComponent.cpp:5272` **[V]**).

---

## 4. OSC surface

### 4.1 Library, transports, ports

**Mixed JUCE + custom.** Outbound uses `juce::OSCSender` (UDP) or a raw `juce::StreamingSocket`
with 4-byte big-endian length-prefix framing (TCP) (`OSCConnection.cpp:264-293` **[V]**). Inbound
is **custom** because JUCE's `OSCReceiver` hides the sender IP the app needs for filtering:
`OSCReceiverWithSenderIP` (UDP) and `OSCTCPReceiver` (TCP, ≤16 clients) are `juce::Thread`s using
a **hand-rolled `OSCParser`** (header-only, "because `juce::OSCInputStream` is a private class",
`OSCParser.h:12-14` **[V]**). Both UDP and TCP listen simultaneously; TCP bind failure is
non-fatal (`OSCManager.cpp:185-241`). **Default ports** (`OSCProtocolTypes.h:135-168` **[V]**):
UDP RX **8000**, TCP RX **8001**, target TX **9000**, QLab **53000/53001**; tracking PSN **56565**,
RTTrP default port; OSCQuery HTTP default **5005** (§4.6).

### 4.2 Address grammar

WFS-specific, prefix-routed by `OSCMessageRouter` (`.cpp:386-435` **[V]**):

| Namespace | Grammar |
|---|---|
| `/wfs/input/` | `/wfs/input/<param> <chID> <value>` **or** OSCQuery form `/wfs/input/<chID>/<param> <value>` |
| `/wfs/output/`, `/wfs/reverb/` | `<param> <chID> <value>` (+ band arg for EQ) |
| `/wfs/config/` | full-path keyed, no channel (e.g. `/wfs/config/stage/width <v>`) |
| `/remoteInput/` | `/remoteInput/<param> <ID> <value \| inc/dec [delta]>` (tablet dialect) |
| `/cluster/{move,scale,rotation,…}` | `<clusterId 1-10> <…>` |
| `/arrayAdjust/` | `/arrayAdjust/<param> <arrayId> <delta>` |
| `/adm/obj/<N>/…` | ADM-OSC objects |

Two accepted input forms per family (`parseInputMessage :590-648`); channel ID = `extractInt(arg0)`
or the numeric path segment, then `channelIndex = id - 1`. **Only guard in the standard dispatch is
`channelIndex >= 0`** — upper-bound safety is **delegated to `state.setInputParameter`**
(`OSCManager.cpp:1892` etc. **[V]**); cluster IDs *are* explicitly bounded 1-10 in the router.

### 4.3 Validation & coercion — reject, don't clamp

Three stacked gates: (1) **finite-float gate** rejects any NaN/Inf arg before dispatch
(`OSCMessageRouter.cpp:524-542`, enforced `OSCManager.cpp:1428-1436` **[V]**); (2) **blob-size /
unknown-type** gate throws in the parser (`OSCParser.h:112-139`); (3) **range gate**
(`valueWithinBounds` vs `OSCParameterBounds::getBounds`, ~230 entries mirrored from
`WFSParameterDefaults`) marks the parse **invalid** on out-of-range → the value is **dropped and
logged, not clamped** (`OSCMessageRouter.cpp:544-567`, `OSCManager.cpp:1996-2001` **[V]**). Two
exceptions clamp: ramp time `jlimit(0,600)` and ADM polar conversions. Wrong-type is coerced softly
(`extractFloat/Int` accept int32 or float32, else 0). `inc/dec` relative addressing:
`handleRemoteParameterDelta` reads current, adds ±delta — but **non-position deltas are not
re-clamped against `OSCParameterBounds`** (they rely on `setInputParameter`, `:2385-2513` **[V]**).

### 4.4 Feedback / echo suppression

The app **does** emit OSC when internal state changes, from a single sink:
`OSCManager::valueTreePropertyChanged` (registered as `ValueTree::Listener`, `.cpp:37`). Any write
— UI, MCP, tracking, LFO — fires it; it walks up to the owning channel node, builds messages via
`OSCMessageBuilder`, and sends to every target with `protocol==OSC && txEnabled`, plus Remote
tablets and ADM receivers (`:785-1235` **[V]**). **Echo suppression is protocol-scoped**: a
thread-local-ish `incomingProtocol` (set by RAII `ScopedIncomingProtocol`) causes a target to be
skipped when `config.protocol == incomingProtocol` — so an OSC-in change is **not** re-sent to OSC
targets but **is** forwarded cross-protocol (OSC→Remote/ADM) by design (`:1068-1069` **[V]**). ADM
has a second `admReceiving` guard across the async boundary. **This correctness depends on writes
and their listener callbacks running single-threaded on the message thread** — another reason the
§3.6 tracking-thread writes are hazardous.

### 4.5 Generic vs WFS-specific

The dispatch backbone is generic (prefix router + uniform `pendingParamUpdates` coalescing +
`state.setParameter`), but the address→`Identifier` relation is **five hand-maintained static
`std::map`s** in `OSCMessageRouter` (`getInputAddressMap` etc., `.cpp:14-380` **[V]**), and the
bulk of the 5431-line `OSCManager.cpp` is WFS geometry (polar/spherical conversion, position
constraints, cluster barycenter math, ADM mapping, tablet handshake, snapshot commands, QLab cues).
**These maps are hand-synchronized with the CSVs — there is no generator for them** (§0).

### 4.6 OSCQuery — a third, independent surface

`OSCQueryServer` is a **fully-implemented** HTTP+WebSocket OSCQuery server on the vendored
`juce_simpleweb`, single port (default **5005**), opt-in via a NetworkTab toggle, owned by
`OSCManager` (`OSCQueryServer.h:3-34`; `OSCManager.cpp:478-509` **[V]**). It implements HOST_INFO,
the full namespace-tree JSON, per-node attribute queries (VALUE/TYPE/RANGE/ACCESS/DESCRIPTION/
CLIPMODE), and LISTEN/IGNORE WebSocket subscriptions that push **binary OSC packets** on value
change (30 ms coalesced) (`:110-367` **[V]**).

**Critically, the OSCQuery namespace is built from the `OSCMessageRouter` maps + a hand-written
`getParamRange()` switch — NOT from the CSVs** (`:917-1111,624-819` **[V]**). So there are **three
independently-maintained parameter surfaces**:

- **CSV → generator → MCP tools** (`generated_tools.json`)
- **hand-coded router maps → OSCQuery tree** and **→ live OSC routing** (these two share a source)

`MCPOSCQueryAuditor` exists **precisely because** of this split: ~1 s after MCP start it HTTP-fetches
the live OSCQuery tree and logs any generated-tool `internal_osc_path` **missing** from it — a
one-way drift detector (`MCPOSCQueryAuditor.cpp:71-183`; the class docstring states the three are
"independently maintained … easy for the CSV to declare a path the router doesn't handle" **[V]**).
The auditor's very existence is the decisive evidence that **there is no unified registry** today —
the single most important finding for the `spatcore` boundary (see
[core-boundary-proposal-control.md](core-boundary-proposal-control.md) §"Unify the three surfaces").
The tree exposes only `/wfs/{input,output,reverb,config}` — it builds **no `/wfs/cluster`
container**, so cluster tools are expected to show as false-positive drift [I].

---

## 5. MCP server

### 5.1 Transport — Streamable HTTP, request/response only

`MCPTransport` implements MCP Streamable HTTP at one path `/mcp` over the vendored `juce_simpleweb`
HTTP server, subclassing `RequestHandler::handleHTTPRequest` (`MCPTransport.cpp:8`; `.h:24` **[V]**).
It is deliberately **stateless request/response**:

- **No session handling** — `Mcp-Session-Id` is never read or written (grep → none **[V]**). The
  dispatcher's `initialized` atomic is **set-only and gates nothing** (`MCPDispatcher.cpp:156` **[V]**).
- **OPTIONS** → 204 + CORS `Allow-*` (`:102-109`). **GET** → 405 (SSE explicitly deferred).
  **POST** → run handler, return JSON-RPC envelope as 200. **DELETE** → falls to 405 (no
  session-end verb). `/mcp` and `/mcp/` both match (`:92-94`).
- **Concurrency** — the vendored server runs `thread_pool_size = 4` asio workers
  (`SimpleWebSocketServer.cpp:237-241`); `handleHTTPRequest` runs on a worker thread, but **every
  tool handler funnels through the single message thread** (§3.4), so concurrent state writes
  effectively **serialize** — a long tool blocks other clients up to `toolTimeoutMs` (default 5000).
- **Protocol version** advertised is `2024-11-05` (`MCPDispatcher.h:98` **[V]**) — an older revision
  that predates the formalized session model, consistent with the stateless posture.

### 5.2 Bind scope & port

`start(port, loopbackOnly)`: `loopbackOnly==true` → bind `127.0.0.1` + CORS `Allow-Origin: *`;
LAN → any-interface + `Allow-Origin: null` (`MCPTransport.cpp:20,42,50` **[V]**). MainComponent
starts loopback-only (`MainComponent.cpp:830`). **Port is a single config point** — `static
constexpr int kDefaultPort = 7400` (`MCPServer.h:39`) is the only definition; the only other `7400`
literals in `Source/**` are two **UI hint strings** in NetworkTab. **There is NO fallback-to-next-3**
— `MCPTransport::start` binds exactly the passed port and swallows bind failures (reports
`running=true` on a dead socket, `:44-52` **[V]**). The plumbing already takes a port arg and the
UI renders `getBoundPort()`, so XOA=7401 / Tight-WFS=7402 is a one-line change at the
`MainComponent.cpp:830` call site — but **real bind-failure detection would need to be added**
(currently absent). OSC RX/TX ports are clean named constants; the OSCQuery 5005 and OSC-loopback
9001 defaults are scattered magic numbers in `NetworkTab.h` that should be centralized first (§6).

### 5.3 Tool loading & registry

`MCPToolRegistry` is a flat `std::vector<ToolDescriptor>` (`name`, `description`, `inputSchema`,
`modifiesState`, `tier`, handler `std::function`) (`MCPToolRegistry.h:50-92` **[V]**). Loading is
two-pass, in the `MCPServer` ctor (`MCPServer.cpp:59-127` **[V]**):

1. **Generated pass** — `loadGeneratedTools` parses `generated_tools.json` into generic
   `ToolBinding`s whose handler is a generic `dispatchGenericSet`/`dispatchGenericNudge` closure
   (range-gates, enum/type-coerces, writes the ValueTree, `MCPGeneratedToolLoader.cpp:120-364,561-694`).
   Never throws — missing/malformed file just logs.
2. **Hand-written pass** — ~25 WFS tools registered after, so a name collision **overwrites**:
   `registerTool` **replaces in place by name** (not a blind append — verified in the `.cpp`,
   `MCPToolRegistry.cpp:11-21` **[V]**), so exactly one descriptor/tier per name and the hand-written
   variant wins.

`tools/list` copies the vector, **sorts by (tier DESC, name ASC)** so rare tier-3 tools survive
client truncation, and emits `{name, description, inputSchema, _meta:{tier}}` plus server-state
`_meta` (`ai_enabled`, `critical_actions_allowed`) (`MCPDispatcher.cpp:172-221` **[V]**). Separately,
`MCPParameterRegistry` (singleton) parses the *same* JSON for `mcp_describe_parameters` and the
`wfs_set_parameter` whitelist, adding hard-coded WFS synonyms (`stageOriginX→originWidth`, `:267-302`).

The ~25 hand-written tools: `session_get_state/_global_state/_channel_full/_state_delta`,
`wfs_get_parameter(s)`, `wfs_set_parameter`, `wfs_set_parameter_batch`, `mcp_describe_parameters`,
`snapshot_list`, `reverb_auto_layout`, `{input,output,reverb}_position_set_cartesian`,
`input_set_name`, `input_set_attenuation`, `{input,output,reverb}_{create,delete}` (a 3× loop),
`mcp_undo_last_ai_change`, `mcp_redo_last_undone_ai_change`, `mcp_get_ai_change_history` **[V]**.

### 5.4 The three-tier confirmation model

Tier is a per-tool int on `ToolDescriptor` (default 1). **Declared** in two places: the generated
`tier` field (`jlimit(1,3)`, `MCPGeneratedToolLoader.cpp:632-634`; **357 tier-1, 28 tier-2, 8
tier-3** across all 393 generated entries — i.e. 300/22/8 over the 330 setters plus 57/6/0 over the
63 nudges, counts as of the 2026-07-02 `masterLevel` tier-3 fix) and hand-written `describe()` bodies. For generated tools it is resolved by the generator
in this precedence (`generate_mcp_tools.py` `process_row` tier block **[V]**): **(1)** the explicit
**`Tier` CSV column** (`parse_tier_cell`, primary source, added 2026-07); **(2)** fallback to
`lookup_tier_override` (from `tool_tier_overrides.json`) when the cell is blank; **(3)** last-resort
`heuristic_tier` keyword/word-boundary + wide-dB rule (`:561-609`), which appends a loud
`warnings[]` entry so an unclassified param is visible. An out-of-range/non-integer cell is ignored
with an `invalid Tier value` warning. The live CSVs are fully populated (snapshot of the pre-column
effective tiers via `tools/mcp/populate_tier_column.py`), so today the column always resolves and
neither fallback fires. The 8 tier-3 tools are the 7 network/system reconfigurations (port changes,
channel-count changes) plus master level (`masterLevel`, restored to its intended tier 3 by the
`e84716f` casing fix — see Appendix B item 1).

**Enforced at a single choke point** — `MCPDispatcher::handleToolsCall` calls
`tierEnforcement.evaluate(name, tool->tier, args)` **before** the handler runs, and this is the
**only** `evaluate` call site; all `tools/call` traffic routes here
(`MCPDispatcher.cpp:253,66,314` **[V]**, independently confirmed — no tool handler re-enters the
dispatcher or runs side effects around the gate).

Decision logic (`MCPTierEnforcement::evaluate`, `:265-357` **[V]**):

1. **AI master toggle** — if `!aiEnabled` (default **off**, opt-in via NetworkTab), refuse **every**
   tier (`AIDisabled`).
2. **Tier 1** → `Execute` immediately.
3. **Tier 3, gate closed** → `SafetyGateClosed`, no token offered.
4. **Safety gate open** (300 s operator window) → `Execute` for tier-2 **and** tier-3 (the gate is a
   superset that also auto-confirms tier-2).
5. **Tier 2, tier-2 auto-confirm window open** (independent 300 s window) → `Execute`.
6. **Tier 2, matching confirm token present** → consume (single-use) → `Execute`.
7. Otherwise → issue a fresh token, `AwaitConfirmation`, `expires_in 30 s`.

**Confirmation round-trip:** `issueToken` stores `{toolName, canonicalArgsJson, expiresAt}` keyed by
a UUID. `AwaitConfirmation` returns a **non-error** result carrying
`tier_enforcement.awaiting_confirmation=true`, `confirmation_token`, `expires_in_seconds` (and
`token_expired_recovery=true` if a presented token had already expired, detected via
`peekExpiredMatch` before purge). The AI re-calls with `confirm:"<token>"`; args are **canonicalized**
(numeric `1==1.0`, sorted keys, `confirm` stripped) so reordering/retyping between the two calls
still matches (`MCPTierEnforcement.cpp:67-101,198-252` **[V]**). `SafetyGateClosed`/`AIDisabled` are
`isError=true`; `AwaitConfirmation` is `isError=false`. All accessors take a `juce::ScopedLock`, so
the 4 Hz expiry timer cannot race `evaluate`.

**No smuggling:** `wfs_set_parameter_batch` (tier 2) refuses any tier-3 sub-write (`tier_3_in_batch`),
and `wfs_set_parameter` (tier 2) refuses a tier-3 underlying param (`tier_3_underlying`)
(`SetParameterBatchTool.h:325-338`, `SetParameterTool.h:117-130` **[V]**). Operator-UI undo/redo
(overlay/history window) intentionally bypasses the dispatcher/tiers (it is operator-initiated
reversal, not an AI call) but is still wrapped in `OriginTagScope{MCP}` (`MCPUndoOverlay.cpp:295-296`).

### 5.5 Undo/redo — custom transaction log; a batch is one step

AI undo is a **custom transaction log, not `juce::UndoManager`**: `MCPUndoEngine` owns two
`MCPChangeRecordBuffer` ring buffers (shared undo ring + private redo ring,
`MCPUndoEngine.h:154-155` **[V]**). The transaction boundary is **one `ChangeRecord` per
`tools/call`**, created in the dispatcher and pushed **once** on success
(`MCPDispatcher.cpp:306-327` **[V]**).

**A batch = one undo step.** A `ChangeRecord` carries a `std::vector<ChangeSubWrite> subWrites`;
`SetParameterBatchTool` and `ReverbAutoLayoutTool` fill **one** record's `subWrites` with all
constituent writes (a 16-node arc = 16 nodes × up to 5 params ≤ 80 sub-writes, asserted ≤100)
(`SetParameterBatchTool.h:357-428`, `ReverbAutoLayoutTool.h:871-950` **[V]**). Undo pops the one
record and replays sub-writes in **reverse** order (forward on redo, `MCPUndoEngine.cpp:592-626`).
`undoByIndex` does **dependency-chased** targeted undo — it also reverses later records whose
`affectedGroups` intersect, stamped with a shared `redoBatchId` so `redoLast` re-applies the chain
atomically (`:354-524` **[V]**). `onNewStateModifyingRecord` clears the redo ring on any fresh
state-modifying call. A **staleness check** refuses undo (soft `is_stale`) if a non-MCP origin wrote
a *different* value to an affected param after the record's timestamp (`:218-299` **[V]**).

**Origin tagging** is a thread-local `OriginTag` set via `OriginTagScope{MCP}` around every
message-thread handler hop (`OSCProtocolTypes.h:31-68`, `MCPDispatcher.cpp:565-567`). Non-MCP writes
to AI-touched params produce `operator_override` notifications drained onto every tool response and
mark records `isSelfCorrected` (`MCPUndoEngine.cpp:110-208` **[V]**). Origin also drives the per-tab
UndoManager suppression (§2.7) and the Network log.

### 5.6 Error model & the vendored dependency

**Error model — two layers** (`MCPDispatcher.cpp` **[V]**): JSON-RPC errors
(`-32700/-32600/-32601/-32602/-32603`) are reserved for protocol/shape/timeout failures;
`handleRequest` always returns a valid envelope even for garbage. **Tool-execution failures are NOT
JSON-RPC errors** — they return a normal `result` with `content:[{type:text}]` + `isError:true`,
matching MCP's tool-error convention. Tier refusals ride back the same way with a structured
`tier_enforcement` object. HTTP status is 200 even for JSON-RPC errors (error is in the body).

**Dependency — `ThirdParty/juce_simpleweb`** is **vendored (copied source), NOT a submodule, NOT
fetched** — top-level `.gitmodules` lists only JUCE; `git ls-tree` shows a normal tree of blobs, and
its inner `.gitmodules` (declaring asio) is inert because asio is flattened in too
(`.gitmodules:1-4`; `ThirdParty/juce_simpleweb/.gitmodules:1-3` **[V]**). It is a GPLv3 JUCE module
(`benkuper/juce_simpleweb` v1.0.0, author "bkupe" 2020) with **asio 1.16.1** and **OpenSSL 1.1.1g**
headers; the project compiles it with `SIMPLEWEB_SECURE_SUPPORTED=0` (TLS off) plus a local
`common/WSCrypto.*` no-SSL shim. **OPTIONS provenance:** OPTIONS is routed through the server's
`default_resource` (`SimpleWebSocketServer.cpp:224,576`); git history shows a **local patch**
(`8a15dc5`, "route OPTIONS through default_resource") that was **dropped** (`76930c5`) once
**upstream benkuper/juce_simpleweb#5** merged — so the vendored copy carries the upstreamed behavior.
No exact upstream SHA is pinned in-repo; the effective pin is the vendoring commit `c548d99`. The
**GPLv3 license is a distribution constraint** any `spatcore` extraction inherits (see
[open-questions-control.md](open-questions-control.md)).

---

## 6. Coupling audit (generic vs WFS-semantic)

Full CORE / APP / TANGLED classification of every component is in
[core-boundary-proposal-control.md](core-boundary-proposal-control.md). The one-paragraph split:

- **Generic (CORE-ready):** the CSV→tool *generator machinery* (parser, transform skeleton, family
  collapse, deterministic writer), MCP transport, dispatcher, registry, generated-tool loader
  *pipeline*, tier-enforcement engine, undo engine + change-record ring, OSC wire parser/serializer/
  receivers/rate-limiter, OSCQuery protocol shell, XML file IO + rolling backups, and the
  message-thread marshalling primitive (`MCPDispatcher::runOnMessageThread`).
- **WFS-semantic (APP):** the CSVs and the tables baked into the generator (`CSV_NAMESPACE`,
  `ABBREVIATIONS`, `DOMAIN_*`, `CHANNEL_ID_RANGE`, tier keywords), `WFSParameterIDs`/`Defaults`/
  `Constraints`, the hand-coded `OSCMessageRouter` address maps + `OSCParameterBounds` data +
  `getParamRange`, every geometry-aware handler in `OSCManager`, and all hand-written MCP tools
  (`ReverbAutoLayout`, `ChannelLifecycle`, `Input/Output/Reverb` cartesian setters, session capture).
- **TANGLED (seam to cut):** `WFSValueTreeState` (generic store fused with WFS invariants + the
  per-domain/origin undo), `WFSFileManager` (generic XML/merge/backup fused with the WFS section
  split + `stripTransientToggles` + `WFSNetwork::getBounds` validation), `OSCIngestQueue` (generic
  queue with hard-coded `/wfs/*` coalescing prefixes), `MCPGeneratedToolLoader`/`MCPParameterRegistry`
  (generic dispatch fused with EQ-family routing + channel-arg vocabulary + synonyms), and
  `MCPOSCQueryAuditor` (generic OSCQuery client fused with the WFS-tool reconciliation policy).

**The single deepest finding:** the three parameter surfaces (CSV→MCP, router→OSCQuery,
router→live-OSC) are reconciled only by a runtime auditor. The highest-value move for `spatcore` is
to make **one** registry the source of truth for all three (§4.6, boundary proposal).

---

## Appendix A — Success-criterion walkthrough: add a new parameter end-to-end

Goal (per the brief): add a parameter to WFS-DIY — CSV row → state → OSC → MCP tool → persistence —
**without reading the source**. Worked example: a per-input `inputProximityFade` FLOAT, 0–1, dB-ish.

1. **CSV row** — add a row to `Documentation/WFS-UI_input.csv` (19-col, TAB-separated): `Section`,
   `Label`, `Variable=inputProximityFade`, `UI=H Slider`, `Type=FLOAT`, `Min=0.0`, `Max=1.0`,
   `Default=0.0`, `Tier=1` (or 2/3 — see §5.4), `Unit`, `OSC path=/wfs/input/proximityFade`,
   `OSC inc/dec=y` if you want a nudge tool. (§1.2–1.3)
2. **State** — add the property to the `Input`'s relevant section builder in
   `WFSValueTreeState.cpp` (`create*Section`/`createDefaultInputChannel`, §2.2) using a new
   `Identifier` in `WFSParameterIDs.h` **whose string exactly equals the CSV `Variable`**
   (`inputProximityFade`), and a default in `WFSParameterDefaults.h`. **The Identifier string is the
   pin** — it becomes the XML attribute name and the MCP write target (§2.1, Appendix "pins"). Add a
   range entry so load/OSC validation covers it: `OSCParameterBounds.cpp` and `getParamRange()`.
3. **OSC** — add the address→Identifier mapping **by hand** to `OSCMessageRouter::getInputAddressMap`
   (`"proximityFade" → inputProximityFade`) so live OSC and OSCQuery route it, and the inverse in
   `OSCMessageBuilder` if you want feedback (§4.5). *This is the step the CSV does **not** do for
   you.*
4. **MCP tool** — nothing to hand-write: the prebuild regenerates `generated_tools.json`, producing
   `input_set_proximity_fade` (+ `input_nudge_proximity_fade`) deterministically from the CSV row
   (§1.4). Set its **`Tier` column** (1/2/3) in the CSV row; leaving it blank makes the generator
   fall back to the heuristic and emit a loud `no explicit Tier` warning (§5.4). Ensure Python is on
   `PATH` or the prebuild silently uses stale JSON (Appendix B).
5. **Persistence** — automatic: the new `Input` property is serialized into `inputs.xml` by
   `ValueTree::createXml`; older sessions lacking it get the default via `ensureCompleteSchema`
   back-fill (§2.3, §2.6). If it should never persist, add its Identifier to `stripTransientToggles`.
6. **Verify no drift** — enable OSCQuery + start MCP, check the Network Log for `MCPOSCQueryAuditor`
   drift lines; run the generator's idempotency test (`tools/mcp/test_generate_mcp_tools.py`).

The friction points (steps 2–3 are hand-edits in ≥3 files, and the OSC map is not generated) are
exactly what the `spatcore` boundary proposal targets.

## Appendix B — Defects & discrepancies discovered (as found by the audit; per-item status noted)

1. **Tier-override casing typo silently downgrades master-level safety.**
   `tools/mcp/tool_tier_overrides.json` has key `"MasterLevel": 3`, but the CSV variable is
   `masterLevel`. Override lookup is exact-string (`generate_mcp_tools.py:562`), so it misses;
   `masterLevel` then hits the wide-dB heuristic → **tier 2, not the intended tier 3** (confirmed in
   `generated_tools.json`). **[V]**
   **STATUS: FIXED 2026-07-02** — key corrected to `masterLevel` (`e84716f`; master level is tier 3),
   and the defect *class* was closed by `2a3bd70`: tier now lives in an explicit CSV `Tier` column
   (primary source, fully populated), the heuristic warns loudly when it fires, and new generator
   tests gate Tier-column completeness against the live CSVs.
2. **Build step silently no-ops when Python is absent.** The prebuild's `where python … & exit /b 0`
   means edited CSVs can drift from committed JSON with **no error** on a machine without Python on
   `PATH` (e.g. the Windows Store stub). **[V]** *(Status: still open as a build-design issue; the
   primary dev machine now has Python installed, so the generator runs there — the hazard remains
   for other machines/CI.)*
3. **`version` attribute is inert** — written everywhere, read nowhere; there is no version-gated
   migration guard for a future incompatible schema change. **[V]**
4. **`cleanupBackups(keepCount)` appears to have no caller** — backups may accumulate unbounded,
   especially given the debounced patch-autosave backs up `system.xml` on every routing change. [I]
5. **`StateDeltaTool` uses one shared server-wide snapshot cursor** — concurrent MCP clients would
   corrupt each other's deltas (latent multi-client bug; fine for one-AI-per-session). **[V]**
6. **Spec drift** — `GENERATION_SCRIPT_SPEC.md` shows dotted tool names, `/wfs/config/network/…`
   paths, and output=64/reverb=16 ranges that do **not** match the code (underscores,
   `/wfs/config/<var>`, output=128/reverb=32). **[V]**
7. **Output snapshots unimplemented** — `snapshots/outputs/` is created but only input snapshots have
   save/load. [I]

## Appendix C — Compatibility pins (what MUST survive extraction byte-identical)

Three independent string namespaces (details in
[core-boundary-proposal-control.md](core-boundary-proposal-control.md) §Compatibility):

- **Session/preset files** are pinned by the **`juce::Identifier` strings** in `WFSParameterIDs.h`
  (element = type Identifier, attribute = property Identifier, via `ValueTree::createXml`). The four
  structurally-validated Identifiers `WFSProcessor/Config/Inputs/Outputs` are load-bearing. Renaming
  any persisted Identifier string silently drops that value on load. **[V]**
- **OSC namespace** is pinned by the **hand-written `OSCMessageRouter` map keys + prefix literals**
  (`/wfs/input|output|reverb|config/`, `/remoteInput/`, `/arrayAdjust/`, `/cluster/*`). The CSV
  OSC-path column must equal these. **[V]**
- **MCP tool names** derive **deterministically** from the CSVs (`derive_tool_name`, golden +
  idempotency tests, `test_generate_mcp_tools.py:83,466-486`), so regenerating post-extraction yields
  identical names **iff** the CSV bytes + `CSV_NAMESPACE`/prefix/expansion tables + override JSONs +
  the derive functions all move together. The ~14 hand-written `wfs_/mcp_/session_` names and the
  `{kind}_create|delete` pattern are pinned separately in C++. **[V]**

---

*Cross-references: audio-rate DSP and the audio-buffer RT model are in
[audio-engine-map.md](audio-engine-map.md) (HANDOFF-01). The extraction plan and open questions are
in [core-boundary-proposal-control.md](core-boundary-proposal-control.md) and
[open-questions-control.md](open-questions-control.md).*
