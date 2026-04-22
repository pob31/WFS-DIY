# MCP Tool Surface — Taxonomy, Naming, and Tier Assignments

This document specifies what tools the MCP server exposes to AI clients, how they're organized, how they're named, and what tier each belongs to. It is both an implementation specification (for Claude Code) and a design document that shapes the user-facing AI experience.

## The three categories of tools

**Auto-generated per-parameter tools.** Most of the surface. Generated from the CSVs by the build-step script described in `GENERATION_SCRIPT_SPEC.md`. Roughly 450 tools covering every parameter in the application. Each is a thin wrapper: "set the value of this specific parameter on this specific channel."

**Hand-written high-level tools.** A smaller set, probably 15–30 tools, that do not correspond to a single parameter. They compose multiple parameter changes, read aggregate state, or provide convenience operations that the AI is likely to want. See the "Hand-written tool catalog" section below.

**The generic fallback tool.** A single `set_parameter(path, value, optional_transition_seconds)` tool that accepts any OSC path from the CSVs. This exists so that in the rare case where the AI wants to touch a parameter that isn't covered by a named tool, it can still do so. The AI is instructed (via the tool description) to prefer named tools when available.

## Why not expose all 450 tools flat

An LLM asked to pick a tool from a flat list of 450 struggles. Tool selection latency goes up, selection accuracy goes down. The strategy is:

1. **Namespace tools hierarchically** so clients can browse a tree rather than a list. MCP clients display namespaced tool names nicely. Example names: `input.position.set_cartesian`, `input.lfo.configure`, `output.eq.set_band`.
2. **Group related parameters into bulk tools** where it makes sense. Example: `input.lfo.configure(input_id, active, period, shape_x, shape_y, shape_z, amplitude_x, amplitude_y, amplitude_z, ...)` sets all LFO parameters for a source in one call. The per-parameter tools still exist underneath for fine-grained control; the bulk tool is a convenience layer.
3. **Keep read-state tools consolidated.** `get_session_state()` returns a single JSON blob with everything the AI is likely to want. This avoids a cascade of individual `get_x` calls.
4. **Use the CSV `Section` column as the primary grouping key.** Each CSV section becomes a namespace in tool names.

## Naming conventions

- **snake_case** for tool names and argument names. MCP clients handle this well; camelCase would work too but snake_case reads more naturally in voice control.
- **Dot-separated hierarchy** in tool names: `category.subcategory.action`. Categories come from the CSV sections.
- **Action verbs in names**: `set_`, `get_`, `configure_`, `load_`, `store_`, `nudge_`, `toggle_`. Avoid bare nouns like `attenuation` — should be `set_attenuation`.
- **Prefer semantic over abbreviated**: the CSV `remoteInput` paths (`/remoteInput/liveSourceActive`) are the naming model, not the raw `wfs` paths (`/wfs/input/LSactive`). Translate `LS` to `live_source`, `FR` to `floor_reflections`, `LFO` stays as `lfo` (it's a well-known term).
- **Source/input/channel terminology**: use "input" in tool names for consistency with the CSVs. In tool *descriptions*, also mention "source" and "channel" so voice queries like "move source 3" or "input 3" or "channel 3" all route correctly.
- **Positions**: default to Cartesian (x, y, z) in named tools. Cylindrical and spherical variants are separate tools (`input.position.set_cylindrical`, `input.position.set_spherical`) per the existing CSV structure. The AI picks the right one based on what the operator says.

## Tool description conventions

These conventions matter more than they seem. The tool description is what the LLM reads per-call to decide whether to use the tool. A description that reads well to a human may read badly to an LLM.

- **Start with the action, not the object.** "Sets the position of an audio source in the listening space" — not "Position of an audio source."
- **Specify units and coordinate frames explicitly.** "x, y, z in meters, origin at stage reference point, +x right (stage-right), +y upstage, +z up."
- **State constraints the AI should respect.** "input_id must be between 1 and the current number of configured inputs (default 24)."
- **Mention related tools when the AI might need them.** "Use `get_session_state()` first if unsure which inputs exist."
- **Use the CSV 'Hover help text' as the starting point** but expand with context the AI needs. The hover text is optimized for a human pointing at a UI element; the tool description is optimized for an LLM deciding whether this tool matches the user's request.

## Hand-written tool catalog

These are NOT auto-generated. The implementer writes them by hand because they compose multiple underlying operations or return aggregate state.

### State reading

| Tool | Tier | Purpose |
|---|---|---|
| `session.get_state` | 1 | Returns a summary of the current session: running/stopped, input/output/reverb counts, master level, currently selected input, active snapshot name, stage dimensions. |
| `session.get_inputs_summary` | 1 | Returns array of inputs with id, name, position, attenuation, mute state, control mode (manual/tracking/matrix), cluster assignment. |
| `session.get_outputs_summary` | 1 | Returns array of outputs with id, name, position, orientation, group assignment, mute state. |
| `session.get_reverbs_summary` | 1 | Returns array of reverb channels with feed position, return position, mute state. |
| `session.get_array_geometry` | 1 | Returns the speaker array structure: outputs grouped by array assignment, with positions and orientations. Useful for Claude to reason about the spatial configuration. |
| `session.get_running_state` | 1 | Whether the DSP is running, with audio interface info and current thread performance if available. |

### High-level actions

| Tool | Tier | Purpose |
|---|---|---|
| `session.start_dsp` | 2 | Starts audio rendering. Confirmation required because it produces sound. |
| `session.stop_dsp` | 2 | Stops audio rendering. Confirmation required because it silences the show. |
| `input.move_relative` | 1 | Nudges a source by dx, dy, dz relative to current position. Useful for "move source 3 a bit to the left." |
| `input.place_by_description` | 1 | Accepts x, y, z AND a free-form description. The description goes in the input's name if the name hasn't been set manually. Lets Claude place and label in one step. |
| `input.mute` / `input.unmute` | 1 | Shortcut for toggling input attenuation to -inf. |
| `cluster.assign` | 1 | Assigns an input to a cluster (1-10) or removes from cluster (0). |
| `snapshot.list` | 1 | Lists saved input snapshots with names and timestamps. |
| `snapshot.load` | 2 | Loads a named snapshot. Confirmation required — changes everything at once. |
| `snapshot.store_new` | 1 | Creates a new snapshot (additive, non-destructive — new file). |
| `snapshot.update` | 2 | Overwrites an existing snapshot. Confirmation required. |
| `snapshot.delete` | 3 | Deletes a snapshot. Safety gate required. |

### Compound setup tools

These wrap common multi-parameter operations that are tedious to do one parameter at a time.

| Tool | Tier | Purpose |
|---|---|---|
| `input.configure_lfo` | 1 | Sets all LFO parameters for an input in one call: active, period, phase, shape/amplitude/phase for each axis, gyroscope. |
| `input.configure_live_source_tamer` | 1 | Sets all live-source-damping parameters. |
| `input.configure_floor_reflections` | 1 | Sets all floor-reflection parameters (Hackoustics section). |
| `input.configure_directivity` | 1 | Sets directivity, rotation, tilt, HF shelf in one call. |
| `output.configure_eq` | 1 | Sets all bands of the output EQ. |
| `input.start_move` | 1 | Starts a one-shot movement (the CSV's "Move" section): target, time, curve, acceleration profile. |
| `input.stop_move` | 1 | Stops a running movement. |
| `input.set_tracking` | 1 | Enables tracking for an input with a given tag ID and smoothing. |

### Generic fallback

| Tool | Tier | Purpose |
|---|---|---|
| `set_parameter` | variable | Generic fallback. Takes a parameter name (matching the CSV Variable column) and a value. Tier is determined by the parameter being set. |

### Diagnostic / meta

| Tool | Tier | Purpose |
|---|---|---|
| `mcp.list_knowledge_resources` | 1 | Lists the available MCP resources with their titles and descriptions so the AI knows what documentation it can fetch. |
| `mcp.describe_parameter` | 1 | Given a parameter name, returns its full schema: type, range, default, unit, enum values, hover help, current value. Useful when the AI wants to explain a parameter to the user. |
| `mcp.get_dry_run_state` | 1 | Returns whether dry-run mode is active. |
| `mcp.set_dry_run_state` | 2 | Enables or disables dry-run mode globally. |

## Tier assignment heuristics for auto-generated tools

The generation script cannot perfectly tier tools automatically. A default assignment is:

- **Read-only tools** (get_*): always Tier 1.
- **Parameters with numeric ranges that cannot produce sudden loud output**: Tier 1. Position changes, timing parameters, LFO settings, filter frequencies, directivity angles.
- **Parameters that can produce sudden loud output**: Tier 2. Attenuation beyond a threshold (e.g., any jump of more than 20 dB), master level, test tone generation, solo buttons, running-state toggles.
- **Parameters that delete or overwrite persistent data**: Tier 3. Delete snapshot, overwrite configuration files, clear projects.
- **Parameters that restart the DSP or reconfigure I/O**: Tier 3. Channel counts, audio interface selection, sample rate.

The generation script should emit a default tier for each tool based on heuristics (keyword matching in parameter names: `delete`, `clear`, `master`, `channels`, `interface`, etc.). Hand-edit the generated JSON via an override file (`tool_tier_overrides.json`) committed to the repo when the defaults are wrong. The generator merges overrides over defaults.

## Argument type mapping

CSV type → JSON Schema type:

| CSV Type | JSON Schema |
|---|---|
| INT | `integer` with `minimum` and `maximum` from CSV Min/Max |
| FLOAT | `number` with `minimum` and `maximum` from CSV Min/Max |
| STRING | `string` |
| IP (4x INT) | `string` with regex pattern for IPv4 |
| enum (from enum column) | `string` with `enum` array OR `integer` with the enum values as description |

Enum handling specifically: the CSV enum column contains semicolon-separated values like `UDP ; TCP` or `OFF ; ON` or `DISABLED ; OSC ; PosiStageNet (PSN) ; RTTrP`. The generator converts these into either:
- A string-enum JSON Schema (preferred for readability by the LLM) with the enum values as strings.
- Plus a mapping layer on the C++ side that converts "UDP" → 0, "TCP" → 1 for the underlying INT parameter.

This way the AI sees and speaks semantic values, but the internal system uses integers.

## The "transition time" argument

Several CSV rows have "extra value is transition time in seconds" in the OSC path notes. This is a valuable feature for voice control — "move source 3 to x=4 over two seconds" — and should be exposed in the generated tools.

For tools where this applies, add an optional `transition_seconds` argument (float, default 0.0 meaning instant). The generation script detects this from the CSV notes.

## The `inc`/`dec` relative mode

Several CSV rows have "y" in the "OSC 'inc' or 'dec' before value" column. This means the parameter supports relative adjustments on the OSC side.

For each such parameter, also generate a `nudge_*` tool that takes a `direction` enum (`up`, `down`, `left`, `right`, `forward`, `back`, etc. — or simply `+` / `-` for scalar parameters) and an `amount`. This maps naturally to voice phrases like "a bit louder" or "move source 3 forward by half a meter."

## Coordinate system clarity

The CSV and the old manual use "width / depth / height" for input positions. This is ambiguous in voice contexts — an operator saying "move it two meters to the right" doesn't think in terms of "width." Tool descriptions should explain:

- **x (width)**: horizontal left-right, +x = stage-right, -x = stage-left.
- **y (depth)**: front-back, +y = upstage (away from audience), -y = downstage (toward audience).
- **z (height)**: vertical, +z = up.

The stage origin default is at the downstage center at floor level, but this is configurable in the Stage tab and the `session.get_state` response includes the current origin.

Polar coordinates (cylindrical/spherical) use "azimuth" with 0° = toward audience, 90° = stage-right, per the CSV comments on `inputPositionTheta`.

## Localization

The multilingual LOC files (`en.json`, `fr.json`, etc.) contain translations of all UI strings. These are NOT directly exposed in the MCP server. Instead:

- Tool descriptions are in English.
- The AI client handles translation if the operator is speaking a non-English language.
- The LOC files inform tool *documentation* — if a French operator says "atténuation de la source 3," the LLM recognizes this maps to `attenuation` even though the tool's English name is `set_attenuation`, because modern LLMs speak French natively.
- However, in resource documents (the knowledge layer), consider bilingual versions for the highest-value resources (WFS theory, tuning procedure) so that French-speaking users get well-written French explanations rather than machine translations.

## What NOT to expose as tools

Some things in the CSVs should not become tools:

- **UI-state-only parameters** (gradient map shape editor states, `gmShapeLocked`, window open/close buttons): these are operator-facing UI affordances, not semantic operations. The AI should not be moving shape editor layer toggles around.
- **Keyboard shortcut markers** in the CSVs: these describe how a human uses the keyboard, not something the AI does.
- **"Find My Remote" password**: not an AI concern.
- **Audio interface selection and patching**: Tier 3 at best, but realistically an operator task. Don't surface as tools; if needed, the AI can direct the operator to the Audio Interface window.

The generation script should have an ignore-list (`tool_generation_ignores.json`) for parameters to skip. Default ignores shipped with the script; user can add more.
