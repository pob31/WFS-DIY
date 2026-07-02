# Control-Plane Extraction — Open Questions & Judgment Calls

> **For:** Pierre-Olivier. These are decisions the analysis **cannot** make for you — they involve
> product direction, licensing, risk appetite, or repo strategy. Each item gives context, the
> options, a **recommendation**, and **what it blocks**. Facts are in
> [control-plane-map.md](control-plane-map.md); the extraction shape is in
> [core-boundary-proposal-control.md](core-boundary-proposal-control.md).
> **Tag:** **[V]** = grounded in verified code; **[design]** = forward-looking judgment.

---

## Q1. Where does the tier policy live — CSV column, generator heuristic, or app code? **[V]**

> **Status 2026-07-02 — RESOLVED in WFS-DIY (option (a)).** An explicit **`Tier`** column was added
> to all seven `WFS-UI_*.csv` files (immediately after `Default`) and is now the generator's
> **primary** tier source (`parse_tier_cell` → `process_row`). The column was snapshotted from the
> current effective tiers via a new committed helper `tools/mcp/populate_tier_column.py`, so
> `generated_tools.json`'s `tools[]`/`nudge_tools[]`/`warnings[]` are byte-identical (only the
> `input_hash` changed) — zero surface change. Resolution precedence is
> **CSV `Tier` → `tool_tier_overrides.json` (retained fallback) → heuristic**, and the heuristic now
> **warns** (`no explicit Tier in CSV…` in `warnings[]`) when it fires, so unclassified params are
> loud; an invalid cell warns (`invalid Tier value`) and falls through. A generator test asserts the
> live CSVs have full Tier coverage (no heuristic warnings). The override file was **kept** as a
> fallback layer (per the chosen decision), so the casing-bug *class* is dormant rather than deleted;
> it is now redundant since every row has an explicit Tier. Docs updated: `GENERATION_SCRIPT_SPEC.md`,
> `control-plane-map.md` §1.1/§1.2/§5.4, `Documentation/CLAUDE.md`. The generator "override key must
> match a real variable" check (also flagged in Q10) remains open but is now low-risk.

**Context (as analysed, pre-fix).** Tier was decided in three overlapping places (control-plane-map
§5.4): there was **no CSV tier column**; the generator computed tier from a keyword/dB-span
**heuristic** (`heuristic_tier`), overridden by a hand-maintained `tools/mcp/tool_tier_overrides.json`;
hand-written tools set tier inline in C++. This worked but was opaque, and it produced a real safety
bug — the `MasterLevel`/`masterLevel` casing typo that silently downgraded master-level from tier 3 to
tier 2 (control-plane-map Appendix B **[V]**, since fixed). XOA and Tight-WFS will each need their own
tier policy (a 10th-order ambisonics "reset order" or a dense-array "reconfigure" is destructive in
ways the WFS keyword list won't catch).

**Options.**
- **(a) Explicit CSV `Tier` column** — authorable per-parameter, visible next to the value, no hidden
  heuristic. Overrides file disappears; the casing-bug class disappears.
- **(b) Keep heuristic + override file** — least churn, but each app re-tunes keyword lists and
  inherits the casing-typo hazard.
- **(c) Tier lives entirely in app code** — most flexible, but loses the CSV-as-single-spec property
  and duplicates effort for the generated tools.

**Recommendation: (a).** Add an explicit `Tier` column to the CSV schema as the primary source,
keep the heuristic only as a *fallback that warns when it fires* (so unclassified params are loud,
not silent), and keep the override file only for the hand-written tools. This makes tier a reviewable
per-app data decision and structurally eliminates the casing-bug class. It is a small generator
change (one more `HEADER_ALIAS`), backward-compatible (missing column → heuristic as today).

**Blocks:** the CSV schema definition for `spatcore` and the generator's tier logic. Decide before
XOA authors its first CSV.

---

## Q2. Add OSCQuery during extraction, or defer it? **[V]**

**Context.** OSCQuery is fully implemented but welded to WFS (`OSCQueryServer` shell is CORE-ready;
the namespace builders + `getParamRange` are APP — control-plane-map §4.6, boundary §2.5). It is also
one of the **three drifting parameter surfaces** (Q3). If a `NamespaceProvider` seam is introduced,
XOA and Tight-WFS inherit a working OSCQuery discovery server **for free**.

**Options.**
- **(a) Generalize OSCQuery now** (introduce `NamespaceProvider`, §3.4 of the proposal) — modest
  extra work, both new apps get discovery + the drift problem gets solved structurally.
- **(b) Defer** — extract MCP + OSC first, leave OSCQuery WFS-only, generalize later.

**Recommendation: (a), coupled with Q3.** The `NamespaceProvider` interface is the *same* seam that
unifies the three surfaces, so generalizing OSCQuery and fixing drift are one piece of work, not two.
Deferring means shipping XOA with the same hand-maintained OSC-map-vs-tool drift WFS-DIY has today,
plus a second app to keep in sync. The transport shell is already generic; only the namespace source
needs the seam.

**Blocks:** the scope of the first extraction milestone. Low regret either way, but doing it with Q3
is materially cheaper than doing it twice.

---

## Q3. Unify the three parameter surfaces, or preserve the status quo? **[V] [design]**

**Context.** The decisive structural finding (control-plane-map §4.6, boundary §4): CSV→MCP,
router→OSCQuery, and router→live-OSC are **independently maintained** and reconciled only by
`MCPOSCQueryAuditor` logging drift *after the fact*. The CSV OSC-path columns are a fourth,
documentary encoding. This is the single biggest latent-cost decision in the extraction.

**Options.**
- **(a) Generator-first unification** (recommended in the proposal): the build-time generator becomes
  the canonical producer; the OSC router maps are *generated*, not hand-typed; the auditor becomes a
  build-time gate that fails loudly.
- **(b) Registry-first**: a runtime `ParameterRegistry` from the schema descriptor feeds all surfaces;
  removes the JSON artifact from the runtime path. Cleaner, but changes the runtime load model and
  risks the deterministic name pin.
- **(c) Preserve status quo**: move the three surfaces verbatim; keep the runtime auditor.

**Recommendation: (a).** It preserves the CSV→names determinism and committed-JSON fallback (the
compatibility pins in boundary §5), while eliminating the drift class that motivated the auditor.
**(c)** is the lowest-effort but exports WFS-DIY's coordination burden into two more apps forever.

**Blocks:** the core's parameter-registry architecture and whether `OSCMessageRouter` maps remain
hand-written. This is the highest-leverage decision here — recommend deciding it first.

---

## Q4. How much of `juce_simpleweb` to absorb vs. track upstream — and the GPLv3 question. **[V]**

**Context.** `juce_simpleweb` is **vendored** (copied), **GPLv3**, `benkuper/juce_simpleweb` v1.0.0
with asio 1.16.1 + OpenSSL 1.1.1g headers flattened in; no upstream SHA is pinned; the OPTIONS fix
was a local patch dropped once **upstream PR #5** merged (control-plane-map §5.6, boundary §2.5 **[V]**).
Two questions collide: (i) dependency hygiene, (ii) **the GPLv3 license flows into anything
`spatcore` links** — if the core is meant to be permissively licensed or closed for some future
product, embedding this transport is a blocker (migration risk R8).

**Options.**
- **(a) Keep vendored, pin a SHA, track upstream loosely** — simplest; accept GPLv3 for the core.
- **(b) Vendor + record exact upstream commit + a re-sync script** — better hygiene; still GPLv3.
- **(c) Abstract the transport behind a `spatcore` interface** so `juce_simpleweb` is a *swappable,
  app-side* dependency (the core stays transport-agnostic and license-clean).

**Recommendation: (c) for the license boundary, (b) for hygiene.** Put a thin `HttpServer` interface
in the core (MCPTransport already only calls ~6 methods on the vendored server — control-plane-map
§5.6 **[V]**, so the seam is small) and keep the GPLv3 `juce_simpleweb` implementation on the app/
integration side. Separately, record the exact upstream commit now (before more local edits diverge
it) and add a re-sync note. If you are content for `spatcore` itself to be GPLv3, **(a)/(b)** are
fine and cheaper — this is fundamentally a licensing-intent decision only you can make.

**Blocks:** the core's dependency graph and license. Needs your call on `spatcore`'s intended license
before the transport seam is finalized.

---

## Q5. Does the undo/redo transaction model need generalizing before XOA reuses it? **[V]**

**Context.** The AI undo engine is already impressively generic — a before/after transaction log with
`subWrites` batching and dependency-chained targeted undo, capturing a 16-speaker arc as **one** step
(control-plane-map §5.5 **[V]**). The **only** WFS leak is `writePayloadHere`/`isEqBandRecord`
hard-coding output-EQ array-propagation and pre/post-EQ band routing (boundary §2.6, R-seam).
Separately, the human-UI undo is **six per-tab `juce::UndoManager`s** whose domains
(Input/Output/Reverb/Map/Config/Clusters) are WFS tab identities (control-plane-map §2.7).

**Options.**
- **(a) Generalize minimally now** — make `ChangeSubWrite` carry an *opaque write target* applied by
  an injected app writer, so the engine stops knowing about EQ families. Make the `UndoDomain` set
  app-supplied data. Small, contained change.
- **(b) Reuse as-is, EQ leak included** — XOA has no EQ-band arrays, so the leak is dormant; ship it.
- **(c) Redesign the transaction model** — unnecessary; the model is sound.

**Recommendation: (a).** The engine is 95% ready; the write-target indirection is a well-defined seam
that also cleans up the generated-loader EQ branch (same adapter). Do it during extraction so XOA's
own array-valued params (ambisonic coefficient banks) can reuse the batch machinery without patching
the core. Avoid **(b)**: a dormant hard-coded EQ path in a "generic" core is a trap for the next app.
**No redesign is warranted** — the tier/undo model is the strongest part of the control plane.

**Blocks:** the `MCPUndoEngine` + `MCPGeneratedToolLoader` write-adapter interface. Decide alongside
the state write-interceptor (Q6), since both funnel through the same "app applies a write" seam.

---

## Q6. Re-home constraint enforcement to a write-interceptor now, or preserve caller-side clamping? **[V]**

**Context.** Range/geometry validation lives **caller-side** in `WFSConstraints` /
`OSCParameterBounds`, **not** inside the state setters (control-plane-map §2.3 **[V]**). A write that
bypasses those callers persists an out-of-range value. This is safe today only because every real
caller (UI, OSC) remembers to clamp — a discipline the core cannot assume from XOA/Tight-WFS authors.

**Options.**
- **(a) Move validation into a CORE write-interceptor** — every `setParameter` runs an injected
  validator; no path can bypass it. (Migration risk R2.)
- **(b) Preserve caller-side clamping** — least churn, but the seam that lets a bug through survives
  into two more apps.

**Recommendation: (a).** This is a correctness improvement, not just a refactor: it closes the
bypass class permanently and gives app authors one place to declare bounds. It pairs naturally with
the load-time value gate that already exists in persistence (`mergeProperties` → `getBounds`,
control-plane-map §2.6) — unify them into one validator the core owns.

**Blocks:** the `TreeParameterStore` write path. Decide with Q5.

---

## Q7. Fix the remaining RT-safety violation (tracking receivers) before or during extraction? **[V]**

> **Status 2026-07-02 — Violation A (BinauralProcessor) is RESOLVED in WFS-DIY.** RT threads now
> consume a POD `BinauralCalculationEngine::RtParams` snapshot (SpinLock copy per block + relaxed
> atomic for the audio-callback output channel) published from the message thread by
> `refreshRtSnapshot()`; the `WFSValueTreeState` binaural accessors assert message-thread in debug;
> the adjacent lifecycle races (prepare-under-live-worker, enable-before-prepare, shared-buffer
> use-after-free in `releaseResources`) were fixed in the same change. This fix is now the
> **in-repo template** for the `spatcore` `RtSnapshot` primitive (boundary §3.3) and for fixing B.

> **Status 2026-07-02 — Violation B (tracking receivers) is RESOLVED in WFS-DIY** (option **(b)**,
> the recommended path below). The PSN/RTTrP/MQTT receivers no longer touch the `ValueTree` from
> their network threads. Each now decodes + applies its offset/scale/flip transform on the network
> thread, then pushes a POD `TrackingUpdate` into a new `TrackingIngestQueue`
> (`Source/Network/TrackingIngestQueue.{h,cpp}`) — modelled on `OSCIngestQueue` per the
> recommendation: it coalesces newest-wins per tracking-id/slot (merging position + orientation) and
> **drains on the message thread** via a `private juce::Timer` at 60 Hz, so a 100 Hz stream collapses
> to one write per tick and never starves the GUI (the failure mode per-packet `callAsync` would
> cause). The drain invokes each receiver's existing `route*` body, which now runs message-thread-only
> and asserts `JUCE_ASSERT_MESSAGE_THREAD` (as does `drainBatch`). All `ValueTree` writes, listeners
> (`WFSCalculationEngine::valueTreePropertyChanged`), and the shared `TrackingPositionFilter` are now
> message-thread-only. The already-safe `TrackingOSCReceiver` was left untouched. This is now the
> **in-repo template** for the `spatcore` marshalled tracking sink (boundary §2.7, §3.3 rule 2).
> Builds clean (Debug x64). Empirical confirmation (ThreadSanitizer + a 100 Hz stream while dragging
> the tracked input on the Map tab, control-plane-map §3.6) remains a good pre-release check.

**Context (what remained).** Violation B: the PSN/RTTrP/MQTT tracking receivers **wrote** the
`ValueTree` from their own network threads with no `MessageManagerLock` (used nowhere in `Source/`),
which also fired `WFSCalculationEngine::valueTreePropertyChanged` on those threads
(control-plane-map §3.6). A naive extraction would have inherited it.

**Options.**
- **(a) Fix during extraction** — tracking writes marshal via the core's `runOnMessageThread` (or a
  drained queue, as OSC ingest already does). The core then ships a *clean* safety guarantee.
- **(b) Fix now in WFS-DIY, like Violation A** — same pattern, done ahead of extraction; tracking is
  higher-rate than binaural (100 Hz position streams), so the marshalling/coalescing choice matters
  more (the `OSCIngestQueue` drain pattern is probably the better fit than per-packet `callAsync`).
- **(c) Extract as-is, file a bug** — the core's headline promise ships already violated.

**Recommendation: (a) or (b), never (c)** — with a lean toward **(b)** now that A has established
the pattern and its verification script: the fix is self-contained, and live shows use tracking far
more than binaural. Whichever is chosen, the core must provide the marshalled tracking sink so app
authors can't reintroduce the anti-pattern.

**Blocks:** nothing structural, but it sets the extraction's definition-of-done.

---

## Q8. Repo topology for `spatcore` — monorepo, submodule, or separate package? **[design]** *(shared question — HANDOFF-01 answers it in [open-questions-audio.md](open-questions-audio.md) Q1; this section concurs)*

**Context.** Three apps (WFS-DIY, XOA, Tight-WFS) will consume `spatcore` for **both** the control
plane (this analysis) and the audio engine ([audio-engine-map.md](audio-engine-map.md)). The choice
affects CI, the CSV→codegen build step, the vendored GPLv3 dependency (Q4), and how compatibility
golden tests (boundary §7) run across apps.

**Options.**
- **(a) Monorepo:** `spatcore/` + `apps/{wfs-diy,xoa,tight-wfs}/` in one repo. Simplest cross-app
  refactors and shared CI; one place for the generator + golden tests; heaviest single repo.
- **(b) `spatcore` as a git submodule** (like JUCE is today — `.gitmodules` lists only JUCE **[V]**):
  each app pins a core SHA; clean boundaries; submodule friction (the team already vendors
  `juce_simpleweb` rather than submoduling it, suggesting a preference against submodules).
- **(c) `spatcore` as a versioned package** (JUCE module / CMake package): strongest isolation,
  independent versioning; heaviest release process for a 3-app in-house toolkit.

**Recommendation: (a) monorepo, for now.** With three closely-coupled first-party apps sharing both a
control plane and an audio engine, and a build-time codegen step + cross-app golden tests, a monorepo
minimizes coordination cost and lets the compatibility replay suite (boundary §7) run all three apps
in one CI. The team's existing choice to **vendor** rather than submodule `juce_simpleweb` is a signal
that submodule friction is unwelcome (Q4 **[V]**). Revisit **(c)** only if `spatcore` later needs to
ship to third parties. *(Both handoffs land here independently: HANDOFF-01's
[open-questions-audio.md](open-questions-audio.md) Q1 recommends the same mono-repo — treat this as
one shared, already-converged decision, not two.)*

**Blocks:** where the extraction PRs land and how CI is wired. Cross-cutting; worth deciding early.

---

## Q9. Session file format — keep the multi-file XML layout, or consolidate? **[V]**

**Context.** A "session" is a **project folder** of separate XMLs (system/network/inputs/outputs/
reverbs) + a `.wfs` manifest, not one file (control-plane-map §2 persistence). The split is WFS-domain
(`extractConfigSection`/`extractNetworkSection`). XOA/Tight-WFS have different section structures.

**Options.**
- **(a) Core provides a *section-IO primitive*; each app supplies its own split descriptor** — WFS
  keeps its 5-file layout; XOA defines its own. Preserves WFS-DIY compatibility (R5) exactly.
- **(b) Consolidate to a single session file in the core** — simpler core, but breaks WFS-DIY's
  existing on-disk layout and its "load network.xml while DSP runs" feature.

**Recommendation: (a).** The multi-file split buys real features (independent section import,
network-hot-load) and is a hard compatibility pin for existing WFS-DIY shows. Keep the layout as
app-supplied data over a generic section-IO + backup + merge core. Do **not** consolidate.

**Blocks:** the `XmlPersistence` API shape (boundary §2.3). Low urgency; compatibility-driven.

---

## Q10. Fix the discovered defects now, or defer? **[V]**

**Context.** The read-only pass surfaced concrete defects (control-plane-map Appendix B): the
`MasterLevel` tier-casing typo (**safety-relevant**), the silently-no-op codegen prebuild, the inert
`version` field, an apparently-uncalled `cleanupBackups`, and the `StateDeltaTool` shared-cursor
multi-client bug. None were touched (analysis-only).

**Options.** Fix in WFS-DIY before extraction / fix as part of extraction / log and defer.

**Recommendation:** Fix the **`MasterLevel` tier typo now** in WFS-DIY (one-character change, closes a
live safety-gating gap) and add the generator "override key must match a real variable" check (Q1).
Fold the rest (loud codegen failure, `version` gate, `StateDeltaTool` per-client cursor, `cleanupBackups`
wiring) into the extraction as hardening — they map cleanly onto core seams (R7, R9).

> **Status 2026-07-02:** **done** — the `MasterLevel` tier-casing typo is fixed in WFS-DIY. The
> override key in `tools/mcp/tool_tier_overrides.json` is now `masterLevel` (matching the CSV variable),
> and `Source/Network/MCP/generated_tools.json` was regenerated so
> `system_master_section_set_master_level` is now **tier 3** (was silently tier 2); the generator test
> suite passes. Note the dev machine now has Python installed, so the codegen prebuild actually
> regenerates `generated_tools.json` from the CSVs/overrides on build (the silent-no-op hazard remains a
> build-design issue for other machines/CI — the loud-failure hardening recommendation stands).
> Remaining Q10 defects (inert `version` gate, `StateDeltaTool` per-client cursor, `cleanupBackups`
> wiring) and the generator "override key must match a real variable" check (Q1) are still open.

**Blocks:** nothing; but the tier typo is a live safety issue worth a standalone fix.

---

## Summary — recommended decision order

**Resolved in WFS-DIY (2026-07-02):** **Q1** (explicit CSV `Tier` column, option a), **Q7** (both
RT-safety violations fixed — binaural `RtParams` snapshot + `TrackingIngestQueue`), and **Q10**'s
safety item (`masterLevel` tier typo). These now serve as in-repo templates for the extraction
rather than open decisions.

Still open, in recommended order:

1. **Q3** (unify the three surfaces — highest leverage) → drives Q2.
2. **Q4** (`spatcore` license intent) → gates the transport seam.
3. **Q8** (repo topology — shared with HANDOFF-01) → gates where PRs land.
4. **Q5 + Q6** (undo write-adapter + state write-interceptor — same seam) → define the extraction's
   definition-of-done.
5. **Q9, Q2** and the remaining Q10 hardening items (loud codegen failure, `version` gate,
   `StateDeltaTool` cursor, `cleanupBackups`) → lower-urgency / compatibility-driven.

*Evidence: [control-plane-map.md](control-plane-map.md). Extraction shape:
[core-boundary-proposal-control.md](core-boundary-proposal-control.md). Audio-engine counterpart:
[audio-engine-map.md](audio-engine-map.md).*
