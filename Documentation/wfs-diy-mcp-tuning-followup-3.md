# WFS-DIY MCP server v3 — third follow-up

Companion to the first two follow-ups. After the v3 round closed all the structural items, this round caught one real bug, one previously-mistaken claim, and a few related rough edges. Same agent, same incremental testing approach.

**Bottom line: there's a real channel-resize bug.** `inputChannels` and `outputChannels` writes are accepted into the parameter store but never actually resize the channel arrays. Only `reverbChannels` works end-to-end. This is also visible in the GUI — the operator sees no change either, so MCP and UI are in agreement, just both stuck.

This finding also retroactively corrects a claim from follow-up #2 (see "Correction" below).

---

## Status (as of 2026-04-30)

Headline bug + the two related rough edges + the cosmetic rename all shipped in commit `3d2a0d1`.

| Commit | Items addressed |
| --- | --- |
| `3d2a0d1` | Channel-count resize root cause (input + output + reverb), `getNumReverbChannels` consistency (fixes the channel_full reverb rejection), `wfs_get_parameter(s)` strict channel-id range, `synonym_of` → `requested_as` rename. |

The "string coercion on all three" smell is partially addressed: post-fix, the AI-side `after:` value will be a clean integer (the routing through `setNumXChannels` writes int via `static_cast<int>(value)`). The legacy XML-loaded `before:` may still surface as a string for one cycle on a session that pre-dates this commit; that self-corrects after the first write of the session.

Per-item shipping notes are inlined below.

---

## The bug: input/output channel resize doesn't propagate

### Reproduction

Starting from any session state. Read the current count, write a different one, read it back.

```
wfs_get_parameter("inputChannels")          → {value: 8}
wfs_set_parameter("inputChannels", 9)       → {before: "8", after: "9"}   // store accepts
session_get_state()                          → channel_counts.inputs: 8     // array unchanged
                                              inputs: [8 entries]           // unchanged
```

Same pattern for outputs:

```
wfs_set_parameter("outputChannels", 17)     → {before: "12", after: "17"}  // store accepts
session_get_state()                          → channel_counts.outputs: 16   // array unchanged
                                              outputs: [16 entries]         // unchanged
```

Reverbs work correctly:

```
wfs_set_parameter("reverbChannels", 5)      → {before: "4", after: "5"}    // store accepts
session_get_state()                          → channel_counts.reverbs: 5    // array updated
                                              reverbs: [5 entries]          // updated
```

### Diagnosis

| Parameter | Store accepts write | Channel array resizes |
|---|---|---|
| `inputChannels` | ✅ (stored as string) | ❌ |
| `outputChannels` | ✅ (stored as string) | ❌ |
| `reverbChannels` | ✅ (stored as string) | ✅ |

The registry says all three are structurally identical: same `group_key: "system_i_o"`, same `tier: 3`, same scope, same type, same min/max envelope, same auto-generated tool family (`system_i_o_set_input_channels` / `system_i_o_set_output_channels` / `system_i_o_set_reverb_channels`). They should behave the same way. They don't.

The bug confines itself to the input/output handler paths: whatever method invokes the audio-engine resize hook (something like `numChannelsChanged()` / `applyChannelCounts()` / a listener firing on the parameter change) is being called for `reverbChannels` but missed for `inputChannels` and `outputChannels`.

### Confirmation from the operator

Asked the user to look at the GUI after writing `outputChannels = 12`. Their observation: "On my side I can't see the number of channels change." So the bug isn't an MCP/state-sync issue — the resize genuinely doesn't happen. The parameter store and the visible UI are in agreement, both stuck on the original count.

### Likely fix path

A Claude Code agent should diff the three handlers in roughly this shape:

1. Find the handler for `system_i_o_set_reverb_channels` (or whatever invokes on a `reverbChannels` write).
2. Trace what it does after the parameter store update — there will be some call that propagates to the audio engine and triggers the array resize.
3. Find the corresponding `system_i_o_set_input_channels` / `system_i_o_set_output_channels` handlers and check whether they make the equivalent call.

Almost certainly one of:

- A missing call to a `numChannelsChanged()` / `resize()` / `applyChannelCounts()` method on the `InputManager` / `OutputManager`
- A `ValueTree::Listener` that's registered for the reverb side but not for inputs/outputs
- A type-dispatch branch (`if (channelType == ChannelType::Reverb) ...`) that needs to also cover `Input` and `Output`
- An old `prepareToPlay()` invocation pattern that was kept for reverbs but lost for inputs/outputs in some refactor

Worth adding a regression test: write `inputChannels = N+1` after a `prepareToPlay`, read `session_get_state().channel_counts.inputs`, assert it equals `N+1`. Same for outputs and reverbs.

### Related smell: string coercion on all three

All three channel-count writes return `after` as a *string*, not an integer:

```
{before: 8,    after: "9"}    // first write of session
{before: "9",  after: "8"}    // subsequent reads/writes see the string
```

This is consistent across all three (reverb, input, output), so the coercion happens before the path forks into "actually resize" (reverb) vs "silently no-op" (input/output). Probably a `var.toString()` or `String::String(int)` somewhere in the channel-count writer that doesn't appear in other integer parameters (e.g. EQ band index, output array index, etc., which stay clean).

Should be a one-line fix to keep the integer through the write — but worth doing in the same PR as the resize fix, since the two bugs share a code path and verifying one without the other is awkward.

**Fixed (`3d2a0d1`).** Diagnosis was almost right but slightly off: the asymmetry the testing AI saw came from a SECOND bug in the read path, not from the writer's resize behaviour. All three counters had the same brokenness — write the count, leave the children stale. What made reverbs *appear* to work was [Source/Parameters/WFSValueTreeState.cpp](../Source/Parameters/WFSValueTreeState.cpp) `getNumReverbChannels()` reading the `reverbChannels` property directly while `getNumInputChannels()` / `getNumOutputChannels()` read child counts. So `session_get_state` reported `count=5` for reverbs (consistent with the property) but `count=8` for inputs after a `=9` write (still the child count). The AI inferred reverbs were resizing; they weren't.

Fix had two parts:

1. **Writer side.** `WFSValueTreeState::setParameter` and `setParameterWithoutUndo` now intercept the three count IDs and route through `setNumInputChannels` / `setNumOutputChannels` / `setNumReverbChannels` — which actually create / remove child trees. Side effect: every writer path (auto-gen `system_i_o_set_*_channels`, `wfs_set_parameter` escape hatch, batch tool, channel-lifecycle wrappers, OSC ingress, file load) now resizes correctly. The three `setNumXChannels` helpers used to call `setParameter(<count>, ...)` at the end; they were changed to write `setProperty` directly on the IO subtree to avoid recursion into the new routing.

2. **Reader side.** `getNumReverbChannels()` now counts `Reverb`-typed children (the Reverbs subtree also hosts `ReverbAlgorithm` / `ReverbPostEQ` / `ReverbPostExp` globals that must NOT count toward channel total — that's why this read path was historically property-based as a workaround). Now consistent with `getNumInputChannels` / `getNumOutputChannels`: count is always derived from actual children, so a property/array desync can never silently mislead.

The `after:` field will now come back as an int (the routing writes int). A session loaded from XML with string-typed legacy storage may surface `before:` as a string for one cycle; that self-corrects after the first write.

---

## Correction to follow-up #2

In the v2 test session writeup, I claimed "drop the channel count to 12" worked because `wfs_set_parameter("outputChannels", 12)` returned a write success. **That was wrong.** The parameter store accepted the write but the array stayed at 16. The cuboctahedron was placed correctly on outputs 1–12, but outputs 13–16 silently sat at origin (which appeared in the screenshot at the time, and I attributed it to defaults rather than to this bug).

The before/after table in follow-up #2 listed "Drop output channel count to 12" as `One call`. The honest entry is `Cannot be done from MCP — operator must change in GUI`. Doesn't change the rest of the v2 conclusions but worth correcting the record.

In the original v1 session, when the user said "I removed the extra speakers. We only have 12 outputs now" — that was the operator manually adjusting in the GUI, not anything I had done. Consistent with this finding.

---

## Other items found this round

### `session_get_channel_full(reverb, N)` rejects valid channel IDs

```
session_get_channel_full(channel_type="reverb", channel_id=1)
→ Error: channel_id 1 out of range for reverb (1..4)
```

The error message is self-contradicting: it claims `1` is out of range but states the range as `1..4`. Tested with channel_id 1 and 2, both rejected with the same error, even with 5 reverbs in the session. The output and input branches of the same tool work correctly. Bug is confined to the reverb path's range check — likely an off-by-one or a comparison against the wrong upper bound (perhaps it's reading `reverbChannels` as the string `"5"` somewhere and the string comparison breaks the bounds check; that would also link this bug to the string-coercion smell above).

**Fixed (`3d2a0d1`).** Same fix as the headline bug. The error fired because `getNumReverbChannels()` returned the property value (e.g. `5` after a write) but `getReverbState(0)` couldn't find a `Reverb`-typed child at that index — the `! section.isValid()` branch in [Source/Network/MCP/tools/StateInspectionTools.h](../Source/Network/MCP/tools/StateInspectionTools.h) `getChannelFull` then printed the misleading "out of range" error. After the writer-side fix, the children always exist when the count says they do; after the reader-side fix, the count tracks children directly so it can never overreport.

### `wfs_get_parameters` silently returns null for out-of-range channel IDs

```
wfs_get_parameters([
  {variable: "outputPositionX", channel_id: 99}    // 99 doesn't exist
])
→ results: [{variable: "outputPositionX", channel_id: 99, value: null}]
   errors: []
```

Already noted in follow-up #2 (item v3-R1) but worth re-flagging because it co-occurs with the channel-resize bug: an agent doing `for ch in 1..outputChannels: read ch` after a failed `outputChannels` write will *not* catch the inconsistency from this read path — both `channel_counts.outputs` and the silent-null behaviour will hide the bug. Out-of-range channel IDs should land in `errors[]` like unknown parameter names do.

**Fixed (`3d2a0d1`).** [Source/Network/MCP/tools/GetParameterTool.h](../Source/Network/MCP/tools/GetParameterTool.h) `resolveEntry` now takes a `WFSValueTreeState&`, looks up the variable's scope from the registry, and validates `channel_id` against `getNumXChannels()` (or 10 for cluster). EQ-band scope validates against the output channel count. Per-channel reads with an out-of-range channel land in `errors[]` for the batch tool (or surface as a tool error for the single read) with a message like `channel_id 99 out of range for output (1..16)`. The earlier silent `value: null` behaviour is gone.

### Synonym field naming

Also already noted in follow-up #2 (item v3-R2). `wfs_get_parameter("stageOriginZ")` returns `{variable: "originHeight", synonym_of: "stageOriginZ", value: 5.0}`. The field reads upside-down — `requested_as: "stageOriginZ"` would be clearer about which name is canonical and which was the input.

**Fixed (`3d2a0d1`).** Renamed across all four result envelopes: `wfs_set_parameter`, `wfs_set_parameter_batch` per-entry, `wfs_get_parameter`, `wfs_get_parameters` per-entry. Tool-description text updated to match. Reads naturally now: `{variable: "originHeight", requested_as: "stageOriginZ", value: 5.0}`.

---

## Suggested priority order

If a Claude Code agent is going to take a single PR from this document, the channel-resize bug is the headline item:

1. **`inputChannels` / `outputChannels` resize** — fix the missing engine-side propagation, plus the integer-coercion smell on all three counters. Add regression tests for all three.
2. **`session_get_channel_full(reverb, N)` range check** — small, possibly a one-liner, possibly tied to the string-coercion fix above.
3. **`wfs_get_parameters` strict bounds on channel_id** — make it match the strict path used for unknown names.
4. **`synonym_of` → `requested_as`** — cosmetic rename.

Items 1 and 2 might share a root cause (string vs integer in the channel-count store). Worth investigating together.

---

## Cumulative items log

| # | Item | Status |
|---|---|---|
| 1 | Parameter discovery tool | ✅ v2 |
| 2 | Loud failure on unknown names | ✅ v2 |
| 3a | Batch writes | ✅ v2 |
| 3b | Session-scoped tier override | ✅ v2 |
| 4 | Globals tool / channel-full / param read | ✅ v3 |
| 5 | Stage shape + origin parameters | ✅ v2 |
| 6 | Compact change history | ✅ v2 |
| 7 | Document conventions in docstrings | ✅ v2 |
| 8 | Audio-affecting tags | ✅ v2 (`domains`) |
| 9a | Input channel creation/deletion | ✅ Fixed in `3d2a0d1` — setParameter routing through setNumInputChannels |
| 9a' | Output channel creation/deletion | ✅ Fixed in `3d2a0d1` — same routing fix |
| 9b | Reverb channel creation/deletion | ✅ v2 (write path), `3d2a0d1` (read consistency via `getNumReverbChannels` counting children) |
| 9c | Undo/batch interaction | ✅ v2 + v3 |
| v2-A1 | `session_get_channel_full` | ✅ Now works for all three (`3d2a0d1`) |
| v2-A2 | `wfs_get_parameter` / `wfs_get_parameters` | ✅ v3, with strict channel_id validation added in `3d2a0d1` |
| v2-R1 | Enum type metadata fix | ✅ v3 |
| v2-R2 | Globals dedup + filter | ✅ v3 |
| v2-R3 | Globals duplicate keys | ✅ v3 |
| v3-R1 | Out-of-range `channel_id` should error in batch read | ✅ Fixed in `3d2a0d1` |
| v3-R2 | Synonym field naming (`requested_as`) | ✅ Renamed in `3d2a0d1` |
| v3-R3 | `outputChannels` integer-vs-string coercion | 🟡 Mostly fixed in `3d2a0d1` — `after:` is now int; `before:` may still be string for one cycle on XML-loaded sessions, self-corrects on first write |
| **NEW** | **`inputChannels` / `outputChannels` resize doesn't propagate to engine** | ✅ Fixed in `3d2a0d1` |
| **NEW** | **`session_get_channel_full(reverb, N)` rejects valid channel IDs** | ✅ Fixed in `3d2a0d1` |

Once the channel-resize fix lands, the MCP surface really is structurally complete and the remaining items are all polish.

---

## On the iteration

Three rounds of structural work, now one round of bug hunt. The right shape for this kind of feedback loop: structural items first (where the API shape is), then bug items (where the implementation diverges from the shape). The bug round is short because the API surface is now coherent enough that bugs stand out — when `reverbChannels` works and `inputChannels` doesn't despite identical registry metadata, the asymmetry is the diagnosis.

Worth flagging one process improvement for next time: when I claimed in follow-up #2 that "drop output channel count to 12" worked, I should have *verified the array length* via `session_get_state` after the write rather than trusting the writer's success response. The string-coerced `after: "12"` was actually a hint that something was off — same write, returns a different type than the position writes — and I missed it. Habit for next round: writes that succeed in the parameter store don't always mean the underlying state has changed. Check the side effect, not just the call.
