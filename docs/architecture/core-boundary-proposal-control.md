# `spatcore` Control-Plane Boundary Proposal

> **Status:** proposal, analysis-only. No WFS-DIY code was modified. This document specifies *what*
> to extract and *where the seams are*; it does not perform the extraction.
> **Companion:** [control-plane-map.md](control-plane-map.md) is the evidence base (every `path:line`
> lives there). [audio-engine-map.md](audio-engine-map.md) is HANDOFF-01 (audio-rate DSP); its §7
> boundary and this one together specify the `spatcore` extraction.
> **Tags:** **[V]** verified in code, **[I]** inferred. **CORE** = generic, reusable by XOA /
> Tight-WFS unchanged. **APP** = semantically WFS (source/speaker/reverb/cluster/geometry). **TANGLED**
> = generic machinery fused with WFS specifics; the seam to cut is named.

---

## 1. Does the target shape hold?

The brief proposed:

> *An app provides: parameter CSVs, an OSC/MCP namespace, a port, and app-specific tool handlers.
> The core provides: ValueTree-backed state, OSC I/O, the MCP server with tiered confirmation,
> undo/redo, and session persistence.*

**Verdict: the shape is correct, but under-specifies the app's obligations by roughly half.** The
analysis (control-plane-map §6) shows the app must provide substantially more than "CSVs + namespace
+ port + handlers":

1. **CSVs** (build-time) → generated MCP tools. ✔ as stated.
2. **A ValueTree *schema*** — the `Identifier` catalogue, defaults, the hand-coded section builders,
   and the structural validator (`WFSParameterIDs/Defaults`, `create*Section`). The core cannot
   invent the tree shape; today it is compiled-in C++ (control-plane-map §2.2). **Not in the brief.**
2b. **Semantic invariants** — cluster shared-position, tracking uniqueness, output array propagation
   — run as post-write hooks inside the state layer (§2.4). **Not in the brief.**
3. **The OSC namespace as *hand-written maps*, not just "a namespace"** — the CSV OSC-path column is
   a spec the app must re-encode in `OSCMessageRouter`-style maps; there is no generator for OSC
   (§4.5). **Materially understated by the brief.**
4. **Geometry-aware validation** (`WFSConstraints`, `OSCParameterBounds`, `getParamRange`) — three
   separate WFS range/validation tables. **Not in the brief.**
5. **A ValueTree *writer adapter*** — because the generic MCP setter and undo engine hard-know
   EQ-band routing and channel-arg vocabulary (§5.3, §5.5). **Not in the brief.**
6. **Port** ✔ (§5.2) and **custom tool handlers** ✔ (§5.3) as stated.

The most consequential structural finding is **not** in the brief at all: **there are three
independently-maintained parameter surfaces** — CSV→MCP, hand-coded-router→OSCQuery, and
hand-coded-router→live-OSC — reconciled only by a runtime auditor (`MCPOSCQueryAuditor`,
control-plane-map §4.6 **[V]**). Extracting the control plane **without** unifying these would bake a
latent 3-way drift problem into every future app. **The central recommendation of this proposal is a
single `ParameterRegistry` as the one source of truth for all three surfaces** (§4).

---

## 2. CORE / APP / TANGLED classification

Grouped by subsystem. "Seam" = the concrete change that turns a TANGLED item into CORE + an injected
APP adapter. Citations are in control-plane-map.md; representative anchors given here.

### 2.1 CSV → tool generation (`tools/generate_mcp_tools.py`, build-time)

| Component | Class | Rationale / seam |
|---|---|---|
| `read_csv` / `CSVRow` / `HEADER_ALIASES` | **CORE** | Generic tab-CSV→dataclass reader; no WFS knowledge. |
| `camel_to_snake` / `expand_abbreviations` / `parse_enum` | **TANGLED** | Algorithms are generic; the `ABBREVIATIONS`/`WORD_EXPANSIONS` *tables* are WFS vocabulary. **Seam:** inject the tables as a config file. |
| `parse_tier_cell` (CSV `Tier`) / `lookup_tier_override` / `heuristic_tier` | **TANGLED** | Tier now resolves **CSV `Tier` column → override file → heuristic** (2026-07). The column-read + precedence flow is generic risk-classification; the explicit column is APP data, and the keyword lists + dB-span *fallback* rule are WFS/audio-safety policy. **Seam:** the `Tier` column is app data; inject the keyword tables + a range-risk predicate for the fallback. |
| `derive_tool_name/_group_key/_osc_path/_schema` | **TANGLED** | Transform skeleton is generic; it reaches into `CHANNEL_ID_RANGE`, coordinate-suffix stripping, `<band>`/array conventions, per-CSV OSC convention. **Seam:** parameterize over a namespace/range/convention descriptor. |
| `detect_numeric_family` / family pre-scan / `<band>` | **CORE** | Generic array/sub-index collapse; `<band>` literal → a config constant. |
| `validate_tools` / `write_json` / `hash_inputs` | **CORE** | Determinism, byte-identical idempotent writes, schema round-trip. Generic build infra. |
| `derive_domains` + `DOMAIN_*` tables | **APP** | WFS domain taxonomy (wfs_synthesis/reverb/binaural/adm_osc). |
| `CSV_NAMESPACE` / `VARIABLE_PREFIXES` / `OSC_PATH_CONVENTION` / `CHANNEL_ID_RANGE` | **APP** | WFS namespaces + per-kind channel ceilings + `/wfs/*` roots. |
| `generated_tools.json` / `generated_groups.json` | **TANGLED** | The JSON envelope (`schema_version`, `input_hash`, arrays) is a CORE tool-manifest format; the payload is APP data. **Seam is the file boundary itself.** |

### 2.2 State (`WFSValueTreeState`, `WFSParameterIDs/Defaults/Constraints`, `ParameterDirtyTracker`, `WfsParameters`)

| Component | Class | Rationale / seam |
|---|---|---|
| `WFSValueTreeState` typed accessors + callback registry + per-domain UndoManager array + `ScopedUndoDomain` + origin-aware undo suppression | **TANGLED** | ~Half is generic parameter machinery; the same class hard-codes WFS section names, `getParameterScope` prefix heuristics, and semantic invariants. **Seam:** extract `spatcore::TreeParameterStore` (root tree + typed get/set + injected scope-routing table + callback registry + per-domain undo + post-write hook + write-interceptor); a WFS subclass supplies schema + invariants. |
| `getParameterScope` / `getTreeForParameter` | **TANGLED** | Dispatch mechanism generic; routing rules WFS. **Seam:** declarative scope-map table. |
| `valueTreePropertyChanged` invariant block (cluster shared-position, tracking uniqueness) | **APP** | Pure WFS geometry/semantics. Runs via the generic post-write hook. |
| `WFSParameterIDs.h` / `WFSParameterDefaults.h` | **APP** | The WFS schema (Identifier strings + defaults/min/max). Core consumes as injected data. |
| `WFSConstraints` | **APP** | Stage-aware geometry validation. Definitionally app-domain. |
| `ParameterDirtyTracker` | **TANGLED** | Listener+AsyncUpdater+suppression+protocol-gated mark/clear is generic; input-only scoping + Sampler/Gradient special-cases are WFS. **Seam:** inject the scope-item map + "trackable subtree" predicate. |
| `create*Section` / `createDefault{Input,Output,Reverb}Channel` builders | **APP** | Hand-built WFS node trees. Core receives as a schema descriptor. |
| `backfillFromTemplate` / `mergeTreeRecursive` id/type matching | **CORE** | Generic tree algorithms. |
| `migrateADMOSCSection` / `ensure*Section` renames | **APP** | Specific WFS migrations. Register as data-driven passes. |
| `WfsParameters` (legacy string facade) | **APP** | WFS name-mapping shim; slated to be phased out per its own docstring. |

### 2.3 Persistence (`WFSFileManager`)

| Component | Class | Rationale / seam |
|---|---|---|
| `writeToXmlFile` / `readFromXmlFile` | **CORE** | Generic ValueTree↔XML with a header convention. Lift to a `spatcore::XmlPersistence` util. |
| `createBackup` / `getBackups` / `cleanupBackups` | **CORE** | Timestamped rolling backups; `fileType` is just a prefix. |
| `mergeProperties` / `mergeTreeRecursive` | **TANGLED** | Recursive merge (id-match, missing=keep) is generic; the `WFSNetwork::getBounds` numeric gate is WFS. **Seam:** inject a per-property validator `std::function<optional<var>(Identifier,var)>`. |
| `ensureCompleteSchema` / `backfillFromTemplate` | **TANGLED** | Backfill generic; concrete templates + ADM migration WFS. **Seam:** app-provided schema/migration provider. |
| `stripTransientToggles` | **APP** | Hard-codes WFS runtime toggle IDs. **Seam:** caller-provided `set<Identifier>` of non-persisted keys. |
| `extractConfigSection` / `extractNetworkSection` + section split | **APP** | Encodes the WFS system.xml/network.xml layout. **Seam:** app-supplied section-split descriptor over a CORE section-IO primitive. |
| `ExtendedSnapshotScope` + `getScopeItems` | **APP** | 30+ WFS input scope items. The sparse-inclusion engine is reusable; **seam:** external item catalogue. |
| `validateState` | **APP** | Requires `WFSProcessor`/`Config`/`Inputs`/`Outputs`. **Seam:** schema descriptor. |
| Debounced patch autosave (`MainComponent`) | **APP** | Knows the WFS audio-patch lives in system.xml. **Seam:** generic "dirty section → debounced save" hook. |

### 2.4 OSC (`OSCManager`, `OSCMessageRouter`, `OSCParameterBounds`, `OSC*` transport)

| Component | Class | Rationale / seam |
|---|---|---|
| `OSCParser` / `OSCSerializer` / `OSCMessageBuilder` encode primitives | **CORE** | Generic OSC 1.0 wire codec + length-prefix framing. |
| `OSCReceiverWithSenderIP` / `OSCTCPReceiver` | **CORE** | Generic UDP/TCP receivers exposing sender IP + raw-data callback. |
| `OSCConnection` | **CORE** | Generic single-target sender (pull `MAX_TARGETS` to config). |
| `OSCRateLimiter` | **CORE** | Generic per-target coalescing limiter. |
| `OSCIngestQueue` | **TANGLED** | Coalesce-map + bounded FIFO + timed drain is generic; the coalesceable-prefix set + "first int32 = channel" convention are WFS. **Seam:** inject a `std::function` classifier for the coalescing key. |
| `OSCParameterBounds` | **APP** | API generic, data (~230 WFS ranges mirrored from Defaults) WFS. **Seam:** injected bounds table. |
| `OSCMessageRouter` (address maps + `parse*` + `is*Address`) | **APP** | Every leaf name + namespaces are WFS. **Seam:** CORE routing engine parameterized by app-supplied `{prefix → handler}` + `{suffix → paramId}` tables. |
| `OSCManager` dispatch shell (IP filter, NaN gate, origin tagging, prefix chain) | **TANGLED** | Skeleton is generic control-plane plumbing; branches call WFS handlers inline. **Seam:** registered `{prefix → handler}` table. |
| `OSCManager` geometry/cluster/ADM/tablet/snapshot handlers | **APP** | The bulk of the 5431-line file. WFS-domain. |
| `OSCProtocolTypes.h` | **TANGLED** | `TargetConfig`/`GlobalConfig`/rate constants CORE; `Protocol` enum + `/wfs/*` path constants WFS. **Seam:** split CORE transport-config from APP protocol/paths header. |

### 2.5 OSCQuery (`OSCQueryServer`, `MCPOSCQueryAuditor`)

| Component | Class | Rationale / seam |
|---|---|---|
| `OSCQueryServer` transport shell (HTTP routing, HOST_INFO, attribute extraction, LISTEN/IGNORE, throttled binary-OSC push, `juce_simpleweb` wiring) | **CORE** | Pure OSCQuery-protocol machinery; zero WFS. Lets XOA/Tight-WFS inherit OSCQuery for free. |
| `buildFullTree` / `build*ChannelJson` / `getParamRange` / `resolveOSCPath` | **APP** | Hard-code `/wfs/*`, EQ-band special-casing, `WFSParameterIDs` ranges, ValueTree node types. **Seam:** a generic `NamespaceProvider` interface (§3.4). |
| `MCPOSCQueryAuditor` | **TANGLED** | Tree-walk + JSON fetch is a generic OSCQuery client; the reconciliation is bound to `generated_tools.json` + `MCPLogger`. **Seam:** split a CORE `OscQueryTreeClient` from the APP reconciliation policy. |
| `ThirdParty/juce_simpleweb` (+ vendored asio) | **CORE** | Generic HTTP/WS server, shared by MCP + OSCQuery. **GPLv3 — a distribution constraint to carry (see [open-questions-control.md](open-questions-control.md)).** |

### 2.6 MCP (`MCPTransport/Dispatcher/Registry/Tier/Undo/…`, `tools/*`)

| Component | Class | Rationale / seam |
|---|---|---|
| `MCPTransport` | **CORE** | Streamable-HTTP request/response over `juce_simpleweb`; CORS; client-IP. Only WFS leak = the `"WFS-DIY"` serverInfo string. |
| `MCPDispatcher` | **CORE** | JSON-RPC 2.0 + MCP routing, message-thread marshalling, envelope builders, tier wiring. Parameterize serverInfo/instructions. |
| `MCPToolRegistry` | **CORE** | `{name, description, inputSchema, tier, handler}` vector; replace-by-name (`.cpp:11-21` **[V]**). |
| `MCPTierEnforcement` | **CORE** | Pure token/window/gate/AI-toggle state machine; zero WFS vocabulary. Reusable verbatim. |
| `MCPUndoEngine` + `MCPChangeRecordBuffer` + `ChangeRecord/SubWrite` | **TANGLED** | Generic before/after transaction log w/ dependency-chained batch undo; but `writePayloadHere`/`resolveChannelIndex`/`isEqBandRecord` hard-code output-EQ propagation. **Seam:** `ChangeSubWrite` carries an opaque write target applied by an injected app writer. |
| `MCPGeneratedToolLoader` (`dispatchGenericSet/Nudge`) | **TANGLED** | Typed coercing setter is generic; EQ-family inference + `input_id/output_id/reverb_id/cluster_id` detection are WFS. **Seam:** inject a channel/band resolver + EQ routing adapter. |
| `MCPParameterRegistry` | **TANGLED** | Record store + Levenshtein did-you-mean generic; `stageOriginX→originWidth` synonyms + domain vocab WFS. **Seam:** app-provided synonym/domain table. |
| Generic tools: `wfs_get/set_parameter(_batch)`, `mcp_undo/redo/history`, `mcp_describe_parameters`, `session_get_state_delta` | **CORE** | Registry/variable-name driven; no hard-coded geometry. Depend only on registry + a ValueTree setter. |
| `MCPServer` ctor tool wiring | **APP** | The WFS tool manifest. **Seam:** app-side bootstrap that registers custom tools into the CORE registry. |
| `ReverbAutoLayoutTool` | **APP** | Speaker topology, Vogel-hemisphere geometry, SDN/FDN/IR awareness. The strongest APP coupling. |
| `ChannelLifecycleTools` | **APP** | input/output/reverb channel-count + DSP-restart semantics. |
| `Input/Output/ReverbTools` (cartesian/name/atten) | **APP** | ±50 m stage envelope, X/Y/Z convention. |
| `SessionTools` / `StateInspectionTools` / `StateDeltaTool` (capture) | **APP** | Read-only, but hard-code WFS field vocabulary. |

### 2.7 Threading & RT safety primitives

| Component | Class | Rationale / seam |
|---|---|---|
| `MCPDispatcher::runOnMessageThread` | **CORE** | "Run handler on the message thread; block the worker on a `WaitableEvent` w/ timeout." The reusable safe-handoff primitive. |
| `OutputBufferProcessor` matrix-read contract (plain `const float*`) | **CORE** | Generic "RT worker reads aligned floats published by the control thread." Core must **document/enforce** the publisher guarantee. |
| The message→audio publish (`target*` `std::vector<float>`) | **TANGLED** | Benign only for independent aligned scalars. **Seam:** `spatcore` must offer an atomic snapshot-pointer / double-buffer primitive so app authors don't hand-roll a racy struct publish (control-plane-map §3.4). |
| `BinauralProcessor` ring buffer + SpinLock snapshot | **TANGLED** | LockFreeRingBuffer + SpinLock snapshot are reusable RT plumbing; the `calc.calculate()` ValueTree reads on the RT thread were the APP-semantic **violation** (§3.5), **fixed 2026-07-02** — the worker now copies a POD `RtParams` snapshot once per block, exactly the seam proposed here. **Seam (already cut in WFS-DIY):** RT worker consumes a POD snapshot, never the calc engine's tree accessors. |
| `BinauralCalculationEngine` | **APP** | WFS/binaural geometry. Formerly shipped the read-on-RT-thread anti-pattern; since 2026-07-02 it instead ships the **fix** — a message-thread-published `RtParams` snapshot (SpinLock copy + atomic output channel) that is the in-repo template for the core's `RtSnapshot` primitive (§3.3). |
| Tracking receivers PSN/RTTrP/MQTT | **TANGLED** | Self-threaded ingest is generic; the off-message-thread `setProperty` into WFS input sections was both APP and a **thread-safety violation** (§3.6), **fixed 2026-07-02 in WFS-DIY** via `TrackingIngestQueue` (a message-thread-drained coalescing queue) — now the in-repo template for this seam. **Seam:** route every tracking write through the CORE message-thread marshalling + an injected write sink. |

---

## 3. `spatcore` API sketch — "app provides X / core provides Y" made concrete

Header-level only; names illustrative. The through-line: **the core owns the machinery and the
thread/undo/tier guarantees; the app owns the schema, the namespace, the geometry, and the custom
tools.**

### 3.1 Initialization sequence

```cpp
// ---- APP side: one schema object drives all three surfaces (see §4) ----
spatcore::AppDescriptor app {
    .serverInfo      = { "XOA", "1.0" },                 // was hard-coded "WFS-DIY"
    .mcpPort         = 7401,                              // XOA; Tight-WFS = 7402 (control-plane-map §5.2)
    .oscQueryPort    = 5006,                              // per-app; today a NetworkTab magic number
    .oscPorts        = { .udpRx = 8000, .tcpRx = 8001, .tx = 9000 },
    .schema          = xoaSchema,                         // ValueTree builders + Identifiers + defaults + validator
    .generatedTools  = loadFile("generated_tools.json"), // CSV→generator output (build-time)
    .nonPersistedKeys= { runDSP, ... },                  // was stripTransientToggles
    .migrations      = { ... },                           // was migrateADMOSCSection et al.
};

// ---- CORE side: constructs the whole control plane from the descriptor ----
spatcore::ControlPlane core { app };
core.store();        // spatcore::TreeParameterStore&  (the ValueTree wrapper)
core.osc();          // spatcore::OscSurface&
core.oscQuery();     // spatcore::OscQueryServer&      (opt-in)
core.mcp();          // spatcore::McpServer&

// ---- APP registers semantics into the CORE ----
core.store().setValidator(xoaConstraints);               // write-interceptor (was WFSConstraints, caller-side)
core.store().addPostWriteHook(xoaInvariants);            // was valueTreePropertyChanged invariant block
core.osc().setNamespace(xoaNamespaceProvider);           // was OSCMessageRouter maps (§4)
core.mcp().registerTool(reverbAutoLayoutTool);           // custom, geometry-aware
core.mcp().setWriteAdapter(xoaWriteAdapter);             // EQ-band/channel routing (was baked into loader/undo)

core.mcp().start();  // binds mcpPort; REAL bind-failure detection (absent today, control-plane-map §5.2)
```

### 3.2 Custom-tool registration hook

The registry already models this — `registerTool(ToolDescriptor)` with a `std::function` handler and
a `tier` — and hand-written tools already register after generated ones with replace-by-name
semantics (control-plane-map §5.3 **[V]**). The only change is **inverting ownership**: today
`MCPServer`'s ctor hard-codes the WFS tool list; in `spatcore` the app passes them:

```cpp
struct ToolDescriptor {
    std::string name;                 // pinned client-facing contract (§5)
    std::string description;          // core appends the tier suffix
    juce::var   inputSchema;          // JSON Schema
    bool        modifiesState;        // gates ChangeRecord capture
    int         tier = 1;             // 1 immediate / 2 confirm-token / 3 safety-gate
    std::function<ToolResult(const juce::var& args, ChangeRecord*)> handler; // runs on the message thread
};
core.mcp().registerTool(std::move(descriptor));   // core handles tiers, undo capture, marshalling, errors
```

A custom tool that performs a batch fills `record->subWrites` and the core captures it as **one undo
step** for free (control-plane-map §5.5 **[V]**) — the app writes geometry, the core owns the
transaction boundary.

### 3.3 RT-side read API (the safety contract)

The single most important core deliverable. Today the app *is* the broker (control-plane-map §3.4);
`spatcore` should make the safe pattern the *only* easy pattern:

```cpp
// CORE publishes an immutable POD snapshot from the message thread; RT threads read lock-free.
template <class ParamsPod>
class RtSnapshot {
public:
    void  publish(const ParamsPod&);        // MESSAGE THREAD ONLY (atomic pointer swap / double-buffer)
    const ParamsPod* acquire() const;       // audio/RT thread; never allocates, never locks, never touches ValueTree
};

// Scalar-matrix fast path (what WFS-DIY does today by hand): aligned float arrays.
class RtFloatMatrix { const float* data() const noexcept; /* publisher guarantees aligned, independent cells */ };
```

Rules the core must **enforce or assert** (all are guarantees WFS-DIY now holds — the two places it
formerly broke were both fixed 2026-07-02, control-plane-map §3.5–3.6):

1. No RT thread touches `ValueTree` or allocates (fixes Violation A).
2. Every non-message-thread producer marshals via `runOnMessageThread` or a drained queue (fixes
   Violation B — tracking writes).
3. Parameter-change listeners fire on the owning (message) thread only.
4. Anything wider than an aligned scalar crossing the message→audio edge uses `RtSnapshot`, not a
   bare shared struct.

### 3.4 The unification hook — `NamespaceProvider` (recommended, see §4)

```cpp
struct ParamNode { std::string oscPath; ValueType type; juce::var value; Range range;
                   std::string description; Access access; int tier; };
class NamespaceProvider {                    // ONE app object feeds OSC routing, OSCQuery, and MCP
public:
    virtual std::vector<ParamNode> enumerate() const = 0;                 // → OSCQuery tree + MCP registry
    virtual std::optional<Identifier> resolve(std::string oscPath) const = 0; // → live OSC routing
    virtual std::string pathFor(const ValueTree&, Identifier prop) const = 0; // → OSC feedback
};
```

---

## 4. The central recommendation: collapse three surfaces into one registry

**Today** (control-plane-map §4.6 **[V]**): CSV→generator→MCP tools; hand-coded `OSCMessageRouter`
maps→OSCQuery tree **and**→live OSC routing; the three are kept in sync only by `MCPOSCQueryAuditor`
logging drift after the fact. The `OSC-Remote_InputParameterTab.csv` and `WFS-UI_*.csv` OSC-path
columns are a *fourth*, purely-documentary encoding of the same wire contract.

**Proposal:** make a single `spatcore::ParameterRegistry` the source of truth. Two viable shapes:

- **(a) Generator-first (lower risk):** the build-time generator becomes the canonical producer of a
  registry manifest that the runtime OSC router, OSCQuery `NamespaceProvider`, and MCP loader all
  consume. The hand-coded `OSCMessageRouter` maps are *generated* instead of hand-maintained,
  eliminating the drift the auditor was built to catch. Preserves today's build-time codegen model.
- **(b) Registry-first (cleaner, higher risk):** a runtime `ParameterRegistry` built from the schema
  descriptor feeds all surfaces directly; the CSVs feed the registry. Removes the JSON artifact from
  the runtime path.

**Recommendation: (a)** for the extraction, because it preserves the deterministic CSV→names pin
(§5) and the committed-JSON fallback, and lets the `MCPOSCQueryAuditor` become a *build-time* check
that fails loudly instead of a runtime log. Deferring OSCQuery unification is possible but wastes the
opportunity — both new apps would inherit a coherent OSCQuery surface for free (see
[open-questions-control.md](open-questions-control.md)).

---

## 5. Compatibility constraints — what pins the contract, and how the proposal preserves it

Three **independent** string namespaces must survive extraction byte-identical (verified,
control-plane-map §Appendix C / §4-verification):

| Surface | Pinned by | Must stay byte-identical | Freedom | Preservation strategy |
|---|---|---|---|---|
| **Session/preset XML** | `juce::Identifier` strings in `WFSParameterIDs.h` (element = type Id, attribute = property Id, via `ValueTree::createXml`) **[V]** | Every persisted Identifier string; esp. the validated `WFSProcessor/Config/Inputs/Outputs` | C++ symbol names; the inert `version` value; XML header comment; *adding* new attributes | Move `WFSParameterIDs.h` **as data** into the app's schema descriptor unchanged; the CORE store must never rename an Identifier. Golden test: load a pre-extraction session, re-save, `diff`. |
| **OSC namespace** | Hand-written `OSCMessageRouter` map keys + prefix literals (`/wfs/input|output|reverb|config/`, `/remoteInput/`, `/arrayAdjust/`, `/cluster/*`) **[V]** | Every prefix + map key (address suffix) | The `Identifier` each key maps to; descriptions | If adopting §4(a), the generator must emit **exactly** today's keys (add a golden snapshot of the current map). If not, move the maps verbatim behind `NamespaceProvider`. |
| **MCP tool names** | Deterministic from CSVs (`derive_tool_name`, golden + idempotency tests `test_generate_mcp_tools.py:83,466-486` **[V]**) + ~14 hand-written names + `{kind}_create/delete` | The CSV bytes + `CSV_NAMESPACE`/prefix/expansion tables + override JSONs + derive functions, moved **together**; the hand-written names | Descriptions, schemas, `_meta.tier`, list ordering; *adding* tools | Move the generator + its tables + the CSVs as one unit; keep the idempotency test in CI so a post-extraction regen is proven byte-identical. Hand-written names live in app tool descriptors unchanged. |

**Non-obvious pin:** the OSC suffix (`attenuation`) and the persisted attribute (`inputAttenuation`)
are *different strings* bridged by the router map — so OSC compatibility and file compatibility are
pinned by **different** tables and must **both** be preserved (control-plane-map §4-verification **[V]**).

**Also pinned:** the `generated_tools.json` envelope shape (`schema_version`, `input_hash`,
`tools[]`/`nudge_tools[]`/`ignored`/`warnings`, per-tool fields) is consumed by
`MCPGeneratedToolLoader` + `MCPParameterRegistry`; changing field names breaks the C++ loader
(control-plane-map §1.4 **[V]**).

---

## 6. Migration risk list

| # | Risk | Severity | Mitigation |
|---|---|---|---|
| R1 | **State god-class split leaks WFS invariants.** `WFSValueTreeState` fuses generic store + cluster/tracking invariants + per-domain undo. | High | Introduce the post-write hook + write-interceptor seams (§3.1) *before* splitting; keep invariants app-side. |
| R2 | **Constraint enforcement lost.** Range/geometry clamping is caller-side (`WFSConstraints`); a new generic setter would silently drop it (control-plane-map §2.3). | High | Re-home validation to the CORE write-interceptor so *no* write path bypasses it. |
| R3 | **RT violations shipped into the core.** BinauralProcessor RT tree reads + tracking-thread tree writes (§3.5–3.6). *Update 2026-07-02: **both halves are now fixed in WFS-DIY** — binaural RT reads use an `RtParams` snapshot (§3.5), and tracking-thread writes are marshalled through a message-thread-drained `TrackingIngestQueue` (§3.6); both are the in-repo templates.* | High | Fix during extraction: RT reads a `RtSnapshot` POD; tracking writes marshal. Do **not** port the anti-patterns. |
| R4 | **Three-surface drift frozen in.** Extracting without unifying CSV/OSCQuery/OSC (§4). | Medium-High | Adopt §4(a); make `MCPOSCQueryAuditor` a build-time gate. |
| R5 | **Identifier / OSC-key rename breaks old sessions & remotes.** (§5) | High | Move the tables as data unchanged; golden diff tests (below). |
| R6 | **Generator table externalization drift.** `ABBREVIATIONS`/`CSV_NAMESPACE`/tier keywords become config; a transcription error changes names. | Medium | Keep the idempotency + golden-name tests; diff regenerated JSON against the committed baseline. |
| R7 | **Tier-override casing bug travels.** The `MasterLevel` vs `masterLevel` typo (control-plane-map Appendix B) silently downgraded safety; a core that trusts the override file inherits the class. *The typo itself is fixed (`e84716f`, master level back to tier 3).* | Low (mitigated) | *2026-07: the explicit CSV `Tier` column is now the primary source (Q1 done), demoting the override file to a dormant fallback — the class survives only if a row's `Tier` is left blank **and** an override key is mistyped.* Still add a generator check that every override key matches a real CSV variable. |
| R8 | **GPLv3 transport.** `juce_simpleweb` + asio are GPLv3 (§2.5). A permissively-licensed core cannot embed them directly. | Medium | Abstract the transport behind an interface so the GPLv3 server is a swappable app-side dependency, or keep the core GPLv3 (decision: [open-questions-control.md](open-questions-control.md)). |
| R9 | **Silent codegen no-op.** Prebuild no-ops without Python (control-plane-map Appendix B). | Low-Med | Core build should fail loudly or verify `input_hash` at compile time. |
| R10 | **Port bind failures swallowed.** No fallback, `running=true` on dead socket (control-plane-map §5.2). Two apps on the same port collide invisibly. | Medium | Add real bind-failure detection + optional fallback in the CORE transport before shipping XOA/Tight-WFS. |

---

## 7. Validation plan — record / replay / diff

The extraction is behavior-preserving if a fixed interaction script produces byte-identical state and
responses before and after. Concretely:

1. **Golden generator determinism.** Run `tools/generate_mcp_tools.py --force` twice into temp dirs
   and `cmp` the outputs; assert equality with the committed `generated_tools.json`
   (`input_hash 04fcce40…`). Already encoded as `test_idempotency_fast_path`
   (`test_generate_mcp_tools.py:466-486` **[V]**). Run it against the extracted generator.
2. **Session round-trip diff.** Take a representative pre-extraction project folder
   (system/network/inputs/outputs/reverbs.xml). Load → re-save on the post-extraction build → `diff`
   each XML. Zero diff (modulo the `Created:` header timestamp) proves the Identifier pin (R5).
   Include an *old* session (flat ADMOSC / no GradientMaps) to exercise the migration passes.
3. **OSC replay.** Record a scripted OSC session (a `.oscScript` of `/wfs/input/*`, `/remoteInput/*`,
   `/cluster/*`, `/adm/obj/*` messages incl. out-of-range + `inc/dec`) against pre-extraction WFS-DIY;
   replay against post-extraction; diff the resulting session XML **and** the emitted OSC feedback
   (Network log Origin/Tx columns). This exercises the router maps, validation gates, echo
   suppression, and rate limiting (control-plane-map §4).
4. **MCP transcript replay.** Capture a `tools/call` transcript covering: a tier-1 write; a tier-2
   confirm round-trip (observe `awaiting_confirmation` → re-call within 30 s → success; then a
   >30 s call to see `token_expired_recovery`); a tier-3 refusal with the gate closed, then success
   with the gate open; a `wfs_set_parameter_batch` 16-write batch; one `mcp_undo_last_ai_change`
   (assert the whole batch reverts = one step); `mcp_redo`; a `session_get_state_delta`. Replay
   against post-extraction; diff each JSON-RPC envelope (normalizing the UUID token) **and** the
   resulting state. This exercises the single tier choke point, the confirmation state machine, the
   one-record batch-undo boundary, and origin tagging (control-plane-map §5.4–5.5).
5. **OSCQuery drift.** Enable OSCQuery on both builds; `curl …/?HOST_INFO` and `GET /`; diff the
   namespace JSON. Confirm the `MCPOSCQueryAuditor` drift set is unchanged (or eliminated, if §4(a)
   is adopted).
6. **RT safety (ThreadSanitizer).** Build post-extraction with TSan; drive a 100 Hz PSN stream to a
   tracked input while dragging that input on the Map tab (formerly Violation B) and with binaural
   enabled (formerly Violation A). Assert **zero** ValueTree data-race reports. *Both violations were
   fixed in WFS-DIY on 2026-07-02 (R3), so this step now validates that the extraction **preserves**
   the clean RT posture — run the same TSan scenario on pre-extraction WFS-DIY first to establish the
   zero-report baseline.*

Passing 1–5 with zero diffs proves control-plane behavior preservation; 6 proves the extraction
preserves the (now-clean) RT safety posture.

---

*Evidence base: [control-plane-map.md](control-plane-map.md). Judgment calls deferred to
Pierre-Olivier: [open-questions-control.md](open-questions-control.md). Audio-engine boundary:
[audio-engine-map.md](audio-engine-map.md) §7.*
