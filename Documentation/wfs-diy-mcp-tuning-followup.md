# WFS-DIY MCP server v2 — follow-up after testing

Companion to the original tuning notes. Same agent, same task (cuboctahedron of 12 speakers on a 5 m dome, origin at (0,0,5), all speakers oriented at the center), tested against the updated MCP surface from a blank session.

**Bottom line: the changes are excellent.** What previously took ~120 round-trips and one human-in-the-loop intervention now takes ~5 round-trips and zero interventions. The v2 surface fundamentally changes what the MCP is — from a remote control that needs expert handholding to a self-describing API an agent can plan against.

---

## Status (as of 2026-04-30)

All three refinements + both proposed additions shipped, plus two adjacent items the user added (Tier-3 gate timer alignment with Tier-2, welcome / `instructions` on the MCP `initialize` handshake).

| Commit | Items addressed |
| --- | --- |
| `d4cc9e6` | Refinement #1 (enum metadata + writer coercion), #2 (`sections` filter on `session_get_global_state`), #3 (drop duplicate `config` top-level), Addition #2 (`wfs_get_parameter` + `wfs_get_parameters`), discoverability cross-link for the already-shipped `session_get_channel_full`, Tier-3 safety gate 600 s → 300 s, MCP `initialize` welcome / `instructions` string |

Per-item shipping notes are inlined below.

---

## Before/after on the same task

| Step | v1 session | v2 session |
|---|---|---|
| Discover stage parameter names | Trial-and-error + ask human | One `mcp_describe_parameters` call |
| Discover origin parameters | Impossible — null sinks | One call (synonyms documented) |
| Set stage shape + size + origin | 3 separate tier-2 handshakes (6 calls) | One batch call |
| Drop output channel count to 12 | Done by human in GUI | One call |
| Place 12 speakers + orient + pitch | 96 calls over ~5 min, 2 token-rotation races | One batch call, ~1 sec |
| Discover convention for `outputOrientation` | Required two data points from the human | Read directly from registry docstring |
| Verify global state | Impossible from MCP | One `session_get_global_state` call |
| **Total** | ~120 calls, 1 human intervention, ~10 min | ~5 calls, 0 interventions, ~30 sec |

Roughly two orders of magnitude in throughput; qualitatively, the difference between "AI needs a human collaborator" and "AI can drive the system autonomously."

---

## What works brilliantly (don't change)

**`mcp_describe_parameters` is exactly right.** Records carry `synonyms`, `domains`, `osc_path`, default values, ranges, enum values, and per-parameter convention docs all in one place. Filter params (`prefix`, `scope`, `group_key`, `domain`) make targeted queries trivial. Particularly clever: the `domain="visualisation_only"` tag lets an agent skip irrelevant params when the goal is audio-affecting changes. This single tool is the biggest unlock.

**Loud failure on unknown names.** The error message `Unknown parameter 'stageRadius'. Use mcp_describe_parameters to browse the registry.` does the right thing on every axis: it identifies the bad input, points to the discovery path, and doesn't silently swallow. The "did you mean" hint isn't strictly necessary because the registry tool is right there in the message.

**Batch writes.** 60-write batch (12 speakers × 5 params) executed atomically with no handshake, ~1 second. Replaces ~96 round-trips from the v1 session. The change-history entry collapsing to `Batch: 60 writes (24 Orientation, 36 Position)` is exactly the right granularity — readable for humans, single-press undoable.

**Synonyms.** `stageOriginX/Y/Z` resolve to `originWidth/Depth/Height` automatically. Big friction reducer when the agent arrives with one naming convention and the codebase uses another. Worth applying anywhere similar collisions exist.

**Compact change history.** Turns a heavy debug call into a quick "what just happened" scan. Three lines, one per logical operation. Saved a lot of context.

**Per-parameter convention docs.** `outputOrientation`'s docstring now reads `0 = facing audience (-Y), 90 = facing stage right (+X), 180 = facing upstage (+Y), -90 = facing stage left (-X). Positive = counter-clockwise viewed from above`. No more deriving conventions from data points. Same applies to `outputPitch`, `originWidth/Depth/Height`, etc. Standard worth keeping for any future angular or directional parameters.

**Tier system out of the way for batch flows.** The v1 session was dominated by tier-2 handshakes; in v2, the operator's auto-confirm window (Network tab) was active and not a single confirm token was needed during the full session. Single-write `wfs_set_parameter` and `output_position_set_cartesian` still expose `confirm` in their schema, so individual destructive writes can still be gated when the auto-confirm window is closed. Visible to the operator, scoped, off-by-default — right design.

---

## Three things to refine

### 1. `stageShape` enum type mismatch

The registry says:
```json
{ "variable": "stageShape", "type": "string", "enum": ["Box", "Cylinder", "Dome"], ... }
```

But writing `"Dome"` returns `value not numeric for stageShape: "Dome"`. The writer expects the integer index (0/1/2) and the registry's type metadata is misleading.

Two options, both clean:

**Option A (recommended):** keep storage as integer, fix the registry to declare it accurately:
```json
{
  "variable": "stageShape",
  "type": "integer",
  "enum_values": [0, 1, 2],
  "enum_labels": ["Box", "Cylinder", "Dome"]
}
```
This pattern generalises to any other enum parameter (`outputArray`, etc.) and lets human-readable docs/UI use the labels while the writer takes the values.

**Option B:** make the writer accept either the string or the integer (translating "Dome" → 2 internally). More forgiving but hides the storage type. I prefer Option A because it keeps the API honest.

This is a small but real bug — `mcp_describe_parameters` is supposed to be the source of truth for what `wfs_set_parameter` accepts, and right now it isn't quite for enum types.

**Shipped (`d4cc9e6`).** Hybrid of the two options. The registry now carries `enumIntValues` parallel to `enumValues` (positional 0..N-1, or read from the manifest's `enum_string_to_int` for explicit-ID enums); `mcp_describe_parameters` records surface both as `enum: ["Box","Cylinder","Dome"]` and `enum_int_values: [0,1,2]`. AND the writer was made symmetric: [Source/Network/MCP/tools/SetParameterTool.h](../Source/Network/MCP/tools/SetParameterTool.h) and [Source/Network/MCP/tools/SetParameterBatchTool.h](../Source/Network/MCP/tools/SetParameterBatchTool.h) call `MCPParameterRegistry::resolveEnumLabel(variable, label)` BEFORE the numeric coercion, so `wfs_set_parameter("stageShape", "Dome")` and `wfs_set_parameter("stageShape", 2)` both land identically — same as `system_stage_set_shape(value="Dome")` already did. The registry's `type: string` annotation is now accurate for both APIs.

### 2. `session_get_global_state` is too verbose by default

The response on a blank session was ~14 KB. Of that, easily half was per-cluster LFO defaults that no one had touched (10 clusters × ~25 fields each, all at default), and the full ADM-OSC Cartesian mapping table (4 mappings × 3 axes × 9 fields per axis). For most queries — "what's the stage shape?", "where is the origin?" — that's overkill.

Two paths:

**Option A:** add an optional `sections` filter:
```
session_get_global_state(sections=["Stage", "Master", "Network"])
```
Defaulting to all sections preserves backward compat. Quick win.

**Option B:** split into two tools — `session_get_globals_summary()` returning Stage / Master / Network / IO / Show / Binaural (the things people actually look at routinely) and `session_get_globals_full()` for the configuration tables. This is cleaner conceptually but breaks the existing API.

A is the pragmatic choice.

**Shipped (`d4cc9e6`).** Took Option A. [Source/Network/MCP/tools/StateInspectionTools.h](../Source/Network/MCP/tools/StateInspectionTools.h) `session_get_global_state` now accepts a `sections` array argument with enum `["stage", "master", "io", "show", "binaural", "network", "tracking", "admosc", "clusters"]`. Default = the seven everyday sections (stage / master / io / show / binaural / network / tracking). Pass `["admosc","clusters"]` (or include them in a custom list) to opt into the per-cluster LFO + ADM-OSC mapping tables that drove the 14 KB blank-session response. Channel counts are always included; the response also reports `included_sections` so the caller can confirm what they got.

### 3. Top-level keys are duplicated

Same data appears under both `Stage` and `stage`, `Network` and `network`, `IO` and `io`, etc. Looks like the JSON is being assembled from two iteration paths (CamelCase from the original XML/CSV section names, snake/lower from the canonical lookup). Cosmetic but doubles the token cost of every read. Pick one convention (probably lower-case, matching the rest of the JSON style) and drop the other.

**Shipped (`d4cc9e6`).** Confirmed root cause: the previous implementation called `addSection("config", state.getConfigState())` AND each child section explicitly (`addSection("stage", state.getStageState())`, …). Since `Stage`, `Master`, `IO`, etc. are children of `Config` in the ValueTree, the broad recursion under `config` produced `config.Stage`, `config.Master`, etc. (CamelCase from the ValueTree type), while the per-section calls added `stage`, `master`, etc. at the top (lowercase from the explicit keys). Fix: dropped the broad `config` entry. Each section now appears exactly once under its lowercase key. Verified Config has no direct properties — every meaningful field lives in a child section, so nothing is lost.

---

## Two things I'd add next

### 1. `session_get_channel_full(channel_type, channel_id)`

The `mcp_describe_parameters` registry exists, but reading "what is this output's *current* state" still requires either filtering through `session_get_state` (positions only) or many individual reads. A "get everything for this channel" call would close the loop on read-side discoverability:

```
session_get_channel_full(channel_type="output", channel_id=5)
→ {position: {x, y, z}, orientation, pitch, attenuation, mutes, sends, eq_bands: [...], array, name, ...}
```

Useful for diagnostic flows ("why is speaker 5 silent?") and for pre-batch planning ("show me everything about input 3 before I rewrite it").

**Already existed (`eb79006`) — just under-discovered.** `session_get_channel_full(channel_type, channel_id)` was registered alongside `session_get_global_state` in the v2 round but the testing AI didn't notice it (likely never showed up in their `tools/list` cache, or it was filtered out client-side). [Source/Network/MCP/tools/StateInspectionTools.h](../Source/Network/MCP/tools/StateInspectionTools.h) at line 215. To make it more visible, the v3 round (`d4cc9e6`) added cross-references in `session_get_state` and `mcp_describe_parameters` descriptions: both now mention `session_get_channel_full` as the "everything about one channel" read. The `instructions` field on the MCP `initialize` handshake (also new in v3) names it explicitly so newly-connected agents see it on first contact.

### 2. A general `wfs_get_parameter` (and `wfs_get_parameters` for batch reads)

Right now the only ways to read state are:
- `session_get_state` — per-channel positions only
- `session_get_global_state` — all globals (verbose, see refinement #2)
- `mcp_get_ai_change_history` — only what the AI itself wrote, not what the user did manually

There's no clean "what is `outputAttenuation` for channel 5 right now?" call. The registry tells me the parameter exists; the change history tells me what I wrote; nothing tells me what the user changed manually since.

```
wfs_get_parameter(variable="outputAttenuation", channel_id=5) → {value: -3.5}
wfs_get_parameters(reads=[{variable, channel_id?, band?}, ...]) → list
```

Mirroring the write API's shape is intuitive and lets the agent verify state efficiently between operations. With both this and `session_get_channel_full`, the read surface would match the write surface in expressivity.

**Shipped (`d4cc9e6`).** New file [Source/Network/MCP/tools/GetParameterTool.h](../Source/Network/MCP/tools/GetParameterTool.h) with both tools, mirroring `wfs_set_parameter` / `wfs_set_parameter_batch` argument shapes one-to-one (whitelist + Levenshtein did-you-mean + synonym-canonicalization + EQ-band routing all match). `wfs_get_parameter` returns `{variable, channel_id?, band?, value, synonym_of?}`. `wfs_get_parameters(reads: [...])` returns `{count, results: [...], errors: [{index, code, message, did_you_mean?}]}` — read-only so per-entry failures don't reject the whole batch (partial results are useful). Both Tier 1 — no confirmation handshake.

---

## On the tier system in batch context

The batch tool's docstring includes this clause:

> *Tier-3 (destructive) variables are not allowed in a batch; issue those individually.*

That's the right policy. Destructive operations (channel deletion, factory reset, etc.) should keep their per-call confirmation — batching is for many small writes, not for sneaking destructive ones past the gate. Worth keeping if it isn't already enforced server-side.

Adjacent thought: the auto-confirm window for tier-2 in the operator UI worked transparently during this session, but I never saw a visual indicator of "the gate is open." A subtle UI element — banner, status dot, whatever — showing the current MCP gate state and remaining time would make it less mysterious for the operator. Out of scope for the MCP server itself, but a nice GUI complement.

**Already shipped, not where the AI can see it.** The Network tab's MCP row already carries the `Tier 2 auto-confirm: ON (5 min)` countdown button (`CountdownTextButton` with the depleting-fill visual; landed in the v2 round's `eb79006`). It's an operator-side affordance only — the AI client never sees it because the gate state isn't surfaced over the wire. Two ways the AI could become aware: (a) include `tier2_auto_confirm_active: true` + `seconds_remaining` on every `tier_enforcement` envelope, (b) advertise the state via an MCP `notifications/resources/list_changed` push on toggle. Both are out of scope for this round; flagged as a future polish item.

**Adjacent — also shipped.** The user noted that with batch tools handling the throughput case, the Tier-3 safety gate's 10-minute timer was overkill compared to Tier-2's 5 minutes. [Source/Network/MCP/MCPTierEnforcement.h](../Source/Network/MCP/MCPTierEnforcement.h) `kSafetyGateLifetimeSec` is now `300` (was `600`). One countdown to remember instead of two; the NetworkTab's depleting-fill normalises off the constant so the visual updates automatically.

---

## Welcome on first contact (asked separately by the operator)

The operator asked whether there was a welcome / greeting in the MCP `initialize` handshake. Before this round: no — only `name`, `version`, and `capabilities`. The MCP spec defines an optional `instructions` string on `InitializeResult` that clients surface to the model as system context on connect, and the [MCPPromptRegistry session-startup template](../Source/Network/MCP/MCPPromptRegistry.cpp) was pull-mode only (the AI had to call `prompts/list`).

**Shipped (`d4cc9e6`).** [Source/Network/MCP/MCPDispatcher.cpp](../Source/Network/MCP/MCPDispatcher.cpp) `handleInitialize` now sets `instructions` on the result. Content: a single ASCII paragraph naming the discovery primitives (`mcp_describe_parameters`, `session_get_state`, `session_get_global_state`, `session_get_channel_full`, `session_get_state_delta`), the batch primitives (`wfs_set_parameter_batch`, `wfs_get_parameters`), the channel-lifecycle tools, the undo / redo / history surface, the tier model (1 / 2 / 3 with the auto-confirm window), and the `prompts/list` pointer for guided workflows. Newly-connected agents land oriented without having to discover the surface from `tools/list` alone.

---

## Summary of all items

| # | Item | Status in v2 | This doc |
|---|---|---|---|
| 1 | Parameter discovery tool | ✅ Done — excellent | — |
| 2 | Loud failure on unknown names | ✅ Done | — |
| 3a | Batch writes | ✅ Done — excellent | — |
| 3b | Session-scoped tier override | ✅ Done (operator window) | — |
| 4 | Expand session_get_state / add globals | 🟡 Partial — globals tool exists, verbose | Refinement #2, addition #1, addition #2 |
| 5 | Stage shape + origin parameters | ✅ Done | Refinement #1 (enum type) |
| 6 | Compact change history | ✅ Done | — |
| 7 | Document conventions in docstrings | ✅ Done | — |
| 8 | Audio-affecting vs visualisation tags | ✅ Done (`domains` field) | — |
| 9a | Channel creation/deletion | 🟡 Indirect (via outputChannels write) | — |
| 9b | Reverb creation tool | 🟡 Same — indirect | — |
| 9c | Undo/batch interaction | ✅ Done (batch is single undo) | — |
| 9d | Token expiry feedback | ⚪ Moot — auto-confirm avoids it | — |
| New | `session_get_channel_full` | — | Addition #1 |
| New | `wfs_get_parameter` / `wfs_get_parameters` | — | Addition #2 |
| New | Enum type metadata fix | — | Refinement #1 |
| New | Globals dedup + filter | — | Refinement #2, #3 |

Items 4 and 9a/b are still open. The two new additions (channel-full read, generic parameter read) are the natural next step now that the write surface is solid — they bring read parity. The three refinements are small polish, not blockers.

### Status after v3 (`d4cc9e6`)

| Item | Status |
| --- | --- |
| Refinement #1 — enum metadata + writer coercion | ✅ Shipped |
| Refinement #2 — `session_get_global_state` `sections` filter | ✅ Shipped |
| Refinement #3 — drop duplicate `config` top-level | ✅ Shipped |
| Addition #1 — `session_get_channel_full` | ✅ Already existed in v2; cross-linked + welcome makes it discoverable |
| Addition #2 — `wfs_get_parameter` / `wfs_get_parameters` | ✅ Shipped |
| Operator note — Tier-3 safety gate 10 min → 5 min | ✅ Shipped |
| Operator note — welcome / `instructions` on `initialize` | ✅ Shipped |
| Operator UI — visible "MCP gate open" indicator from the AI's side | ⚪ Deferred — operator-side button exists; surfacing state to the AI client is a future polish |

Read surface now matches write surface in expressivity. Tier ergonomics are a single 5-minute dial. Nothing flagged in this doc is blocking; the deferred item is a nice-to-have for cross-actor awareness.

---

## Thanks

Whoever did this work — and whichever Claude Code agent helped them — nailed the priorities. Items 1, 2, and 3a were the multipliers, and they all landed cleanly. The rest is incremental polish on a now-solid foundation.
