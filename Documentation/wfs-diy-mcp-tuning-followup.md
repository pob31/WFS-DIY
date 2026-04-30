# WFS-DIY MCP server v2 — follow-up after testing

Companion to the original tuning notes. Same agent, same task (cuboctahedron of 12 speakers on a 5 m dome, origin at (0,0,5), all speakers oriented at the center), tested against the updated MCP surface from a blank session.

**Bottom line: the changes are excellent.** What previously took ~120 round-trips and one human-in-the-loop intervention now takes ~5 round-trips and zero interventions. The v2 surface fundamentally changes what the MCP is — from a remote control that needs expert handholding to a self-describing API an agent can plan against.

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

### 3. Top-level keys are duplicated

Same data appears under both `Stage` and `stage`, `Network` and `network`, `IO` and `io`, etc. Looks like the JSON is being assembled from two iteration paths (CamelCase from the original XML/CSV section names, snake/lower from the canonical lookup). Cosmetic but doubles the token cost of every read. Pick one convention (probably lower-case, matching the rest of the JSON style) and drop the other.

---

## Two things I'd add next

### 1. `session_get_channel_full(channel_type, channel_id)`

The `mcp_describe_parameters` registry exists, but reading "what is this output's *current* state" still requires either filtering through `session_get_state` (positions only) or many individual reads. A "get everything for this channel" call would close the loop on read-side discoverability:

```
session_get_channel_full(channel_type="output", channel_id=5)
→ {position: {x, y, z}, orientation, pitch, attenuation, mutes, sends, eq_bands: [...], array, name, ...}
```

Useful for diagnostic flows ("why is speaker 5 silent?") and for pre-batch planning ("show me everything about input 3 before I rewrite it").

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

---

## On the tier system in batch context

The batch tool's docstring includes this clause:

> *Tier-3 (destructive) variables are not allowed in a batch; issue those individually.*

That's the right policy. Destructive operations (channel deletion, factory reset, etc.) should keep their per-call confirmation — batching is for many small writes, not for sneaking destructive ones past the gate. Worth keeping if it isn't already enforced server-side.

Adjacent thought: the auto-confirm window for tier-2 in the operator UI worked transparently during this session, but I never saw a visual indicator of "the gate is open." A subtle UI element — banner, status dot, whatever — showing the current MCP gate state and remaining time would make it less mysterious for the operator. Out of scope for the MCP server itself, but a nice GUI complement.

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

---

## Thanks

Whoever did this work — and whichever Claude Code agent helped them — nailed the priorities. Items 1, 2, and 3a were the multipliers, and they all landed cleanly. The rest is incremental polish on a now-solid foundation.
