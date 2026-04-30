# WFS-DIY MCP server — fine-tuning notes

Notes from a session placing 12 speakers on a cuboctahedron and orienting them at the origin. Roughly ordered by impact. Pick whatever looks worth doing — these aren't formal issues, just observations from one agent's perspective.

---

## 1. Add a parameter discovery tool

The single biggest gap. `wfs_set_parameter` references `WFSParameterIDs.h` in its docstring, but there's no way for the model to read it. This session burned multiple tier-2 handshakes guessing names: `stageRadius`, `originX`, `stageOriginX`, `stageCenterX` — none of which existed (or worked). Eventually had to ask the human for `stageDiameter`, `outputOrientation`, `outputPitch`. Without a human in the loop, this would have stalled hard.

**Suggested:** a read-only MCP tool — `mcp_list_parameters` or `mcp_describe_parameters` — returning the registry. For each parameter:

- canonical name (case-sensitive)
- scope: `global` / `input` / `output` / `reverb` / `eq_band`
- type: `float` / `int` / `enum` / `bool` / `string`
- range (min/max) and units
- default value
- one-line description
- for enums: the value→label mapping (e.g. `stageShape: 0=box, 1=cylinder, 2=sphere`)
- for angular params: zero-direction and sign convention (see #7)
- optional tag: `audio_relevant` vs `visualisation_only` vs `metadata` (see #8)

Filterable by prefix (`prefix="output"`) or scope. Even a flat dump of the registry would be enough — the model can grep it.

This change alone would multiply MCP usefulness more than everything else combined.

---

## 2. Make `wfs_set_parameter` fail loudly on unknown names

Currently it accepts any string as `variable` and returns `before: null, after: null` for unknown names. From the model's side this looks identical to "the parameter exists but was previously unset." Actively misleading.

**Suggested:** whitelist check at the top of the handler. Unknown names should return an explicit error like `unknown_parameter: stageRadius (did you mean: stageDiameter?)`. The "did you mean" is a small touch — Levenshtein against the registry — but huge in practice.

Same applies to writes that succeed but where the value is silently coerced or truncated. Right now `value: "2"` (string) succeeded for `stageShape` which expects an int. That's fine if intentional but worth either documenting or tightening.

---

## 3. Tier-2 confirmation handshake is too granular for batch work

Placing 12 outputs and then orienting/pitching them (24 writes) took ~36 round-trip pairs. Each pair is: call → token → re-call with token → result. Three concrete problems showed up:

- **Token expiry races.** Twice during the session, tokens rotated mid-flight because the model's round-trip exceeded the 30-second window. Had to re-loop with the new token. Once it took three iterations to land speaker 12's pitch.
- **Context bloat.** Every handshake eats two tool calls and two tool results. By the end of this session, well over half the conversation context was tier-2 plumbing.
- **No batch primitive.** No way to say "do these N writes atomically with one confirmation."

**Three candidate fixes**, ranked:

### 3a. Add batch tools (best)

```
wfs_set_parameter_batch(writes: [{variable, value, channel_id?, band?}, ...], confirm?)
output_position_set_cartesian_batch(positions: [{output_id, x, y, z}, ...], confirm?)
```

Atomic on success, all-or-nothing on failure with an index pointing to the first rejection. One confirmation handshake covers the whole batch. The undo system would also need to record the batch as a single entry (see #9).

### 3b. Session-scoped tier override

Let the human grant "this session only, skip tier-2 confirms for `wfs_set_parameter` and `output_position_set_cartesian`" via a single explicit toggle in the WFS-DIY UI. The human informally did this in the session ("the gate is open!") — formalising it as a UI affordance with a visible indicator (a banner showing "MCP gate open until session end / for next 5 minutes / for next N writes") would make it usable without ambiguity.

### 3c. Adaptive window

Bump the 30s window to 90s or 120s after the same MCP session has done >5 successful tier-2 writes recently. Half-measure compared to 3a/3b but trivial to implement.

Recommend doing **3a + 3b**. They solve different problems (3a = throughput, 3b = friction).

---

## 4. Expand `session_get_state`

Currently returns `id`, `name`, `(x, y, z)`, and `array` per channel. Missing: orientation, pitch, attenuation, mute, send levels, EQ state, output assignments, stage shape and dimensions, stage origin, master settings, binaural settings, network config — basically everything except positions.

So when the human said "right now the origin is at (-5, 0, 0)" the model had no way to verify that from the MCP side. And when checking work mid-session, the only way to confirm `outputOrientation` had landed was to re-read `mcp_get_ai_change_history` (verbose) or ask the human to look at the GUI.

**Suggested:** either expand `session_get_state` with a `verbosity` param (`summary` / `full` / `delta-since-last-call`), or add complementary tools:

- `session_get_global_state()` — all stage / master / network / binaural globals
- `session_get_channel_full(channel_type, channel_id)` — every parameter for one channel
- `session_get_state(verbosity="delta-since-last-call")` — only what changed since this MCP session's last read

The "delta since last call" mode would be especially useful for long sessions — gives the model a way to notice when state has been changed by the human or by external OSC without re-fetching everything.

---

## 5. Stage shape and origin parameters need a coherent model

Three things that were either guess-work or impossible:

- **Enum values for `stageShape`.** Human told me 0/1/2 = box/cylinder/sphere. Discoverable via the registry (#1).
- **Stage dimension parameters per shape.** `stageDiameter` worked for sphere. `stageRadius` was accepted but did nothing (null sink). For consistency, either alias them with mutual update (radius = diameter/2) or pick one canonical name per shape and document. Probably `stageWidth/stageHeight/stageDepth` for box, `stageDiameter` (or `stageRadius`) and `stageHeight` for cylinder, `stageDiameter` for sphere.
- **Stage origin offset.** Doesn't appear to be a writable parameter at all. The human moved it manually in the UI. But the position tools' docstring explicitly says "Origin is the user-configured stage origin" — so it's a real concept. It needs to be a writable global: `stageOriginX/Y/Z`. Without that, the MCP can't fully script a session from a blank state.

---

## 6. Add a compact mode to `mcp_get_ai_change_history`

Currently returns full records: arguments, before_state, after_state, affected_parameters, affected_groups, timestamps. Useful but heavy — five entries can run several KB.

**Suggested:** `compact=true` mode returning just `{index, timestamp, operator_description}`. Sufficient for "what did I just do" queries, much cheaper for the model to scan when deciding whether to undo.

---

## 7. Document coordinate and angle conventions in the docstrings

Position tools document `+X = stage right, +Y = upstage, +Z = up`. Good. But:

- `outputOrientation`: zero is +Y (upstage), positive is CCW from above. Had to derive this from two of the human's data points after one wrong guess — see the conversation transcript for the painful sequence.
- `outputPitch`: positive = upward tilt. Guessed correctly but it was a guess.
- Range conventions: does it accept −180…+180, 0…360, or both with auto-wrap?

Every angular parameter should declare its **zero direction**, **sign convention**, and **range** in its description (which the registry from #1 would carry).

---

## 8. Tag parameters as audio-affecting vs visualisation-only

When the model asked "do speakers carry an aim parameter?", it had to infer absence from missing tools. Turns out `outputOrientation` and `outputPitch` exist — but it's not clear whether they affect WFS synthesis, ambisonic encoding, binaural rendering, or just the GUI display.

For an agent deciding what to set, "this parameter exists and the write succeeds" is much weaker signal than "this parameter exists and changes what gets rendered."

**Suggested:** add a tag (or tags) on each parameter in the registry — something like `["wfs_synthesis"]`, `["binaural"]`, `["visualisation_only"]`, `["routing"]`, `["metadata"]`. The model uses this to decide whether a write is meaningful for the user's stated goal.

---

## 9. Smaller items

- **Type coercion is loose.** `value: "2"` (string) was accepted for an integer enum (`stageShape`). Not necessarily wrong, but the registry should declare expected types so the model sends the right thing.
- **The `array` field on outputs** appears in `session_get_state` but is undocumented. What is it? How is it set? Worth a one-liner.
- **Channel creation/deletion isn't exposed.** The human added/removed outputs (16 → 12) via the GUI. There's no MCP tool to create or delete a channel. For full session scripting, add `output_create`, `output_delete`, `input_create`, `input_delete`, `reverb_create`, `reverb_delete`. Probably tier-2 with confirmation.
- **Reverbs:** zero exist by default and there's no creation tool, so the reverb position tool is currently useless from a blank session.
- **Undo and batch writes interact.** If batch writes from #3a land, the undo system needs `undo_batch_as_one` semantics, otherwise undoing a 24-write orientation pass becomes 24 separate undo presses.
- **Token expiry is silent.** When a tier-2 confirm token expires, the server returns a fresh token instead of an explicit "token expired, here's a new one" — which works but is hard to distinguish from a normal first-call response. Consider returning `{token_expired: true, new_token: "..."}` so the model can log it.

---

## 10. The single biggest win

**#1 (parameter registry) + #2 (loud failure on unknown names)** together. They turn this session's pattern from "guess and check with a domain expert in the loop" into "model reads the schema, plans the writes, executes."

Everything else on this list is incremental on top of those two.

---

## Appendix: session context

These notes come from a single ~hour-long session with these characteristics:

- Blank session → cuboctahedron of 12 speakers on a 5 m sphere, all aimed at origin
- Stage shape changed from box (default) to sphere via `wfs_set_parameter("stageShape", 2)`
- Stage diameter changed from 20 m to 10 m via `wfs_set_parameter("stageDiameter", 10)`
- Origin moved from (-5, 0, 0) to (0, 0, 5) — manually by the human, since `stageOriginX/Y/Z` aren't writable
- 12 speaker positions placed via `output_position_set_cartesian` (24 round-trips)
- 12 speaker orientations + 12 pitches via `wfs_set_parameter("outputOrientation"/"outputPitch", ...)` (48 round-trips)
- Two manual workarounds: human told the agent the parameter names; human moved the stage origin in the GUI
- One mid-session race: speaker 12's pitch confirmation token rotated three times before landing

The transcript is the canonical source for any specifics.
