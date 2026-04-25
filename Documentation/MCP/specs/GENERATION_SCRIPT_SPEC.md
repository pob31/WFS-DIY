# Generation Script Specification

A build-step script that consumes the WFS-DIY parameter CSVs and emits a JSON schema representation of the MCP tool surface. Run as part of the build. Output is committed or regenerated on each build — implementer's choice.

## Purpose

Single source of truth for the MCP tool surface. The CSVs already drive the UI and the OSC layer; they should also drive the MCP tools. Hand-maintaining parallel MCP tool definitions would guarantee drift.

## Language

Python 3.10+ recommended. Reasons: standard `csv` module handles the inputs natively, json output is trivial, iteration speed is fast, and the script is not performance-critical. No runtime dependency for users — this is a build-step only.

Alternative: TypeScript/Node if the team prefers a single-language toolchain. Either is fine. Avoid shell scripts (multi-line CSV handling is painful).

## Inputs

The tab-level CSVs are located at `Documentation/WFS-UI_*.csv` in the repo:

- `WFS-UI_config.csv`       — SystemConfig tab: show, channel counts, WFS Processor (algorithm + DSP toggle), stage, master, UI, controllers, binaural renderer, files, diagnostics
- `WFS-UI_input.csv`        — InputsTab: per-input parameters including the Sampler subsystem (18 columns — adds `OSC inc/dec` and `OSC path optional value` columns the other CSVs don't have)
- `WFS-UI_output.csv`       — OutputsTab: per-output parameters and 6-band EQ
- `WFS-UI_reverb.csv`       — ReverbTab: per-channel reverb parameters, pre-EQ, post-EQ, algorithm parameters (SDN/FDN/IR), pre-compressor, post-expander
- `WFS-UI_network.csv`      — NetworkTab: targets table, ADM-OSC mappings (4 Cartesian + 4 Polar, exhaustive), tracking (OSC/PSN/RTTrP/MQTT), Find My Remote (12 columns)
- `WFS-UI_clusters.csv`     — ClustersTab: cluster transforms, per-cluster LFO with 5 axes (X/Y/Z/Rotation/Scale), shared 16-slot preset bank
- `WFS-UI_audioPatch.csv`   — AudioInterfaceWindow: device settings, input/output patch matrix, test signal generator

Two companion documents are intentionally **not consumed by the generator** — their content is hand-written into compound tools instead (see `MCP_TOOL_SURFACE.md` § Hand-written tool catalog):

- `WFS-UI_arrayWizard.md` — Wizard of OutZ preset catalog and geometry-method formulas. The natural MCP tool here is `output.apply_array_preset(preset, geometry, target, acoustic_overrides)`, which composes ~12 underlying output parameters per speaker and is best written by hand against the preset table in that document.
- `WFS-UI_plugins.md` — DAW plugin suite reference. The plugin paramIDs here duplicate paths already present in `WFS-UI_input.csv` (the plugin Track variants drive the same `/wfs/input/*` OSC surface); no separate MCP tools are needed.

One additional file in `Documentation/` is **explicitly skipped** by the generator:

- `OSC-Remote_InputParameterTab.csv` — the Android Remote's source of truth for `/remoteInput/*` paths. Those same paths are already documented in column 16 (`OSC remote path`) of `WFS-UI_input.csv`, and the AI client should not be sending them in any case — `/remoteInput/*` is meant for the Android Remote with its handshake-based bidirectional protocol, while AI clients use `/wfs/*` paths or, more commonly, internal parameter writes via the dispatcher. The generator must not pick up this file even if it lives next to the others; the build command's `--csv-dir` glob should select only files matching `WFS-UI_*.csv`.

## CSV column-convention reference

The seven WFS-UI CSVs use three different column layouts. The generator must accept all three.

| File | Columns | Notable differences |
|---|---|---|
| `WFS-UI_config.csv` | 12 | Compact layout: no Formula column, no OSC-path column, no Keyboard-shortcuts column. Matches `WFS-UI_network.csv`. |
| `WFS-UI_network.csv` | 12 | Same compact layout as `WFS-UI_config.csv`. |
| `WFS-UI_audioPatch.csv` | 17 | Standard layout shared with output / reverb / clusters. |
| `WFS-UI_output.csv` | 17 | Standard layout. |
| `WFS-UI_reverb.csv` | 17 | Standard layout. |
| `WFS-UI_clusters.csv` | 17 | Standard layout. |
| `WFS-UI_input.csv` | 18 | Standard layout plus two extra columns: `OSC inc/dec` (column 14) and `OSC path optional value` (column 15). |

The 12-column header order:

`Section, Label, Variable, UI, Type, Min, Max, Default, Unit, enum, Notes, Hover help text in the status bar`

The 17-column header order:

`Section, Label, Variable, UI, Type, Min, Max, Default, Formula for UI elements (x from 0.0 to 1.0), Unit, enum, Notes, Array value, OSC path, OSC remote path, Hover help text in the status bar, Keyboard shortcuts`

The 18-column header (input only) inserts after column 13:

`..., Notes, OSC path, OSC "inc" or "dec" before value to increment of decrement, OSC path optional value, OSC remote path, ...`

The generator should detect the column count from the header row and dispatch to the appropriate column-index map. Do not hardcode column indices.

Plus two override files, also committed to the repo:

- `tool_tier_overrides.json` — per-parameter tier overrides (when the heuristic default is wrong).
- `tool_generation_ignores.json` — parameters to skip entirely.

## Outputs

A single JSON file: `generated_tools.json`. Structure:

```json
{
  "schema_version": "1.0",
  "generated_at": "2026-04-21T20:23:00Z",
  "source_csvs": ["WFS-UI_input.csv", "..."],
  "tools": [
    {
      "name": "input.position.set_x",
      "description": "Sets an input source's X position (horizontal, +x = stage-right). x is in meters. Typical stage range -10 to 10 m. Supports optional transition_seconds for smooth movement.",
      "parameters": {
        "type": "object",
        "properties": {
          "input_id": {
            "type": "integer",
            "minimum": 1,
            "maximum": 64,
            "description": "Input channel number (1-based)."
          },
          "value": {
            "type": "number",
            "minimum": 0.0,
            "maximum": 50.0,
            "description": "Target X position in meters."
          },
          "transition_seconds": {
            "type": "number",
            "minimum": 0.0,
            "maximum": 600.0,
            "default": 0.0,
            "description": "Time to transition to the new value. 0 means instant."
          }
        },
        "required": ["input_id", "value"]
      },
      "tier": 1,
      "internal_osc_path": "/wfs/input/positionX",
      "internal_variable": "inputPositionX",
      "csv_section": "Position",
      "group_key": "input_position",
      "supports_relative": true,
      "relative_tool_name": "input.position.nudge_x"
    }
  ],
  "nudge_tools": [ ... ],
  "ignored_parameters": [
    { "variable": "gmShapeLocked", "reason": "UI-state only" }
  ],
  "warnings": [
    { "variable": "foo", "message": "No hover help text; description may be weak" }
  ]
}
```

## Core transformation logic

For each row in each CSV:

1. **Skip if ignored**: check `tool_generation_ignores.json` for the variable name.

2. **Derive tool name**: 
   - Strip the category prefix from the variable (e.g., `inputPositionX` → `positionX`).
   - Convert camelCase to snake_case.
   - Namespace from the CSV file and section: `input.position.set_x`.
   - For boolean parameters (INT with min=0, max=1, enum like "OFF ; ON"), use `set_` or `toggle_` based on whether the enum has two values or is a proper boolean.

3. **Derive description**:
   - Start with the Hover help text from the CSV (the last column).
   - If it's missing, fall back to the Label column.
   - Append unit info if applicable: "Value in meters." / "Value in dB."
   - Append enum info for enum parameters: "Valid values: 'OFF', 'ON'."
   - Append "Supports optional transition_seconds for smooth interpolation" if the OSC path notes mention transition time.
   - Append "Use get_session_state() first if unsure which channels exist" for per-channel parameters.

4. **Derive JSON schema**:
   - Type mapping per the table in `MCP_TOOL_SURFACE.md`.
   - Min/Max from the CSV columns.
   - Default from the CSV Default column if present.
   - For per-channel parameters (the Variable starts with `input`, `output`, `reverb`, `cluster`, `sampler`), the first argument is always the channel id as an integer, and the range depends on which CSV the row came from. The script emits the per-CSV upper bound: 64 for `WFS-UI_input.csv` and `WFS-UI_output.csv`, 16 for `WFS-UI_reverb.csv`, 10 for `WFS-UI_clusters.csv`, 36 for sampler cell indices in `WFS-UI_input.csv` (the 6×6 grid). The server validates against the runtime channel count, which can be lower than the upper bound.

5. **Determine tier** via heuristics:
   - If the parameter name contains `delete`, `clear`, `remove`, `reset`: Tier 3.
   - If it contains `master`, `channels`, `sampleRate`, `runDSP`, `reconfigure`: Tier 3.
   - If it touches attenuation or level with wide range (more than 40 dB total range): Tier 2.
   - If it's a test tone, solo, or mute-all operation: Tier 2.
   - If it's a read-only get: Tier 1.
   - Default: Tier 1.
   - Override from `tool_tier_overrides.json` takes precedence.

6. **Detect relative/nudge support**: the "OSC 'inc' or 'dec'" column has `y` → generate a paired nudge tool.

7. **Detect transition time support**: the "OSC path optional value" column contains "transition time" → add the `transition_seconds` optional argument.

8. **Warnings**: emit a warning if the Hover help text is missing, the range is inverted, the enum list doesn't match the min/max, or the variable naming doesn't match any known pattern. Warnings are informative — do not abort the build.

9. **Derive group_key** for undo dependency chasing:
   - Take the CSV Section value.
   - Strip coordinate-system suffixes: ` (Cylindrical)` and ` (Spherical)` are removed, so `Position`, `Position (Cylindrical)`, `Position (Spherical)` all resolve to the same base. Same rule applies to `AutomOtion` and its variants.
   - Normalize to snake_case and prefix with the CSV file's namespace: `Position` in `WFS-UI_input.csv` → `input_position`; `EQ` in `WFS-UI_output.csv` → `output_eq`.
   - Skip rows where Section is empty or literally `"Section"` (header-row artifacts). Emit a warning if such rows have non-empty Variable values, since that indicates a malformed CSV.
   - Parameters with unique or orphan sections become singletons — their group_key is derived from their own variable name, so undo chaining only triggers on exact-parameter matches.
   - Group keys are scoped per channel at runtime. The generator just emits the group_key string; the server composes `(channel_id, group_key)` tuples when recording changes.

## The bulk/compound tools

The generation script does NOT generate the hand-written compound tools (`input.configure_lfo`, etc.). Those are written in C++ directly. The generator's output is only the per-parameter tools.

The `generated_groups.json` emitted alongside the main tool file (see "Validation pass" below) lists parameters grouped by their derived group_key. C++ compound-tool implementations can be validated against this via unit tests — "does `input.configure_lfo` cover every parameter in group `input_lfo`?" — catching drift when new parameters are added to an existing group.

## Idempotency and determinism

The same input CSVs must produce byte-identical output JSON across runs. This means:
- No timestamp that changes (well — the `generated_at` field does, but it should be the only thing that changes if nothing else changed; treat it as informational, not used for equality).
- Sort all output arrays by tool name.
- Use consistent JSON formatting (2-space indent, sorted keys).

Determinism matters because this file is either committed to the repo (in which case diffs should be meaningful) or regenerated at build time (in which case build caches should hit).

## Validation pass

After generation, the script runs a validation pass:

- Every auto-generated tool has a non-empty description.
- Every per-channel tool has `input_id` / `output_id` / `reverb_id` as its first required argument.
- No two tools have the same name.
- All tier values are 1, 2, or 3.
- Every tool has a non-empty `group_key`.
- JSON schemas round-trip (serialize, deserialize, re-serialize, compare).

Failures abort the build with a clear error message.

**Group summary output.** The generator additionally emits `generated_groups.json`, a small lookup of group_key → list of tools in that group. The undo system and compound-tool unit tests read this to reason about group membership without having to scan the full tool list. Example:

```json
{
  "input_position": ["input.position.set_x", "input.position.set_y", "input.position.set_z",
                     "input.position.set_r", "input.position.set_theta", ...],
  "input_lfo": ["input.lfo.set_period", "input.lfo.set_phase", ...],
  ...
}
```

This doubles as the validation source for hand-written compound tools: a unit test asserts that `input.configure_lfo` touches every parameter listed under `input_lfo`.

## Override file formats

`tool_tier_overrides.json`:
```json
{
  "inputAttenuation": 2,
  "masterLevel": 3,
  "inputPositionX": 1,
  "outputEQshape<band>": 1,
  "samplerCellOffsetX": 1,
  "inputArrayAtten*": 2
}
```

**Key conventions for sub-indexed parameters.** Some Variable cells carry a literal `<band>` placeholder (the EQ shape/freq/gain/Q rows). For these, **use the same `<band>` placeholder verbatim in the override key**, and the override applies to every band of that parameter — the generator does not generate one tool per band, so there is no need to address bands individually.

For Variable names that already include a numeric suffix at generation time (e.g. `inputArrayAtten1` … `inputArrayAtten10`, the 10 per-array attenuations), use a **trailing `*` wildcard** to override the whole family in one entry. Wildcards are matched by literal prefix only — they are not regular expressions.

Per-instance overrides (e.g. setting only `inputArrayAtten5` to a different tier) are not supported. If that ever becomes necessary, add it as a new convention rather than overloading the wildcard syntax.

`tool_generation_ignores.json`:
```json
{
  "ignored": [
    { "variable": "gmShapeLocked", "reason": "UI-state only" },
    { "variable": "findDevicePassword", "reason": "Security-sensitive, operator task" }
  ]
}
```

## Testing

Unit tests for the script:

- Known row → expected JSON output (golden-file tests for a handful of representative rows).
- Malformed CSV → clear error message, no partial output.
- Override file takes precedence over heuristic.
- Ignored parameters don't appear in output.
- Ordering is deterministic.

Integration test: run against the full CSVs, verify the output loads as valid JSON and contains at least N tools (where N is a sanity check — current row counts across the seven CSVs total ~640, of which ~550 are expected to become tools after the ignore list is applied).

## A note on the reverb CSV's scope

The reverb parameters mostly mirror input and output parameters (position, directivity, attenuation). The generator should treat `reverb` as its own namespace (`reverb.feed.set_attenuation`, `reverb.return.set_directivity`) but use the same transformation logic. The CSV structure should make this straightforward.

## Run as a build step

Add to the build:

```
python3 tools/generate_mcp_tools.py \
  --csv-dir Documentation \
  --overrides-tier tools/mcp/tool_tier_overrides.json \
  --overrides-ignore tools/mcp/tool_generation_ignores.json \
  --output Source/Network/MCP/generated_tools.json
```

If the output file is already up-to-date, the script should exit quickly (fast-path: hash the inputs, compare to the hash in the existing output, skip if unchanged).

The build step depends on this output; the C++ code includes it at compile time (via resource embedding) or loads it at runtime. Both are acceptable — runtime loading allows hot-reload during development.

## Appendix — Golden output examples

Five worked examples covering the patterns the generator must handle correctly. Use these as golden-file fixtures for unit tests.

### A. Plain float per-channel parameter

**CSV row** (from `WFS-UI_input.csv`, 18 columns; only relevant cells shown):

```
Section: Attenuation
Label: Attenuation
Variable: inputAttenuation
UI: H Slider
Type: FLOAT
Min: -92.0
Max: 0.0
Default: 0.0
Formula: 20*log10(...)
Unit: dB
OSC path: /wfs/input/attenuation
OSC inc/dec: y
Hover: Input Channel Attenuation.
```

**Expected JSON output**:

```json
{
  "name": "input.set_attenuation",
  "description": "Sets the attenuation (level) of an input source. Value in dB. Negative values reduce the level; 0 is unity. Use get_session_state() first if unsure which inputs exist.",
  "parameters": {
    "type": "object",
    "properties": {
      "input_id":  {"type": "integer", "minimum": 1, "maximum": 64,
                    "description": "Input channel number (1-based)."},
      "value":     {"type": "number",  "minimum": -92.0, "maximum": 0.0,
                    "description": "Attenuation in dB."},
      "transition_seconds": {"type": "number", "minimum": 0.0, "maximum": 600.0,
                             "default": 0.0,
                             "description": "Time to transition to the new value. 0 means instant."}
    },
    "required": ["input_id", "value"]
  },
  "tier": 2,
  "internal_osc_path": "/wfs/input/attenuation",
  "internal_variable": "inputAttenuation",
  "csv_section": "Attenuation",
  "group_key": "input_attenuation",
  "supports_relative": true,
  "relative_tool_name": "input.nudge_attenuation"
}
```

(`tier: 2` because of the override on `inputAttenuation` — wide dB range can produce sudden loud output.)

### B. Per-channel + per-band parameter using the `<band>` placeholder

**CSV row** (from `WFS-UI_output.csv`, 17 columns):

```
Section: EQ
Label: EQ Frequency
Variable: outputEQfreq<band>
UI: H slider
Type: INT
Min: 20
Max: 20000
Default: 80 / 250 / 1000 / 4000 / 8000 / 12000
Formula: 20*pow(10,3*x)
Unit: Hz
Notes: For each of 6 EQ bands. Defaults per band: 80 / 250 / 1000 / 4000 / 8000 / 12000 Hz...
OSC path: /wfs/output/Eqfreq <ID> <band> <value>
```

**Expected JSON output** (the generator must NOT expand `<band>` into 6 distinct tools — it emits one tool whose schema accepts `band`):

```json
{
  "name": "output.eq.set_frequency",
  "description": "Sets the center frequency of one EQ band on an output channel. Logarithmic frequency mapping. Default frequencies per band: 80 / 250 / 1000 / 4000 / 8000 / 12000 Hz.",
  "parameters": {
    "type": "object",
    "properties": {
      "output_id": {"type": "integer", "minimum": 1, "maximum": 64,
                    "description": "Output channel number (1-based)."},
      "band":      {"type": "integer", "minimum": 1, "maximum": 6,
                    "description": "EQ band number (1-6)."},
      "value":     {"type": "integer", "minimum": 20, "maximum": 20000,
                    "description": "Frequency in Hz."}
    },
    "required": ["output_id", "band", "value"]
  },
  "tier": 1,
  "internal_osc_path": "/wfs/output/Eqfreq",
  "internal_variable": "outputEQfreq",
  "csv_section": "EQ",
  "group_key": "output_eq",
  "supports_relative": false
}
```

### C. Sub-indexed array-family parameter (numeric suffix in Variable)

**CSV row** (from `WFS-UI_input.csv`, one of `inputArrayAtten1` … `inputArrayAtten10`):

```
Section: Array Attenuation
Label: Array 1 Attenuation
Variable: inputArrayAtten1
UI: Dial
Type: FLOAT
Min: -60.0
Max: 0.0
Default: 0.0
Unit: dB
Notes: Dimmed if no outputs assigned to Array 1
OSC path: /wfs/input/arrayAtten1 <ID> <value>
```

**Expected JSON output** (the generator emits ONE tool that takes an `array` argument, NOT 10 separate tools):

```json
{
  "name": "input.set_array_attenuation",
  "description": "Sets the per-array attenuation override on an input — additional level reduction applied only to the speakers belonging to a specific output array (1-10). Useful for muting an input's contribution to a specific zone (e.g., a side fill) without touching the rest of the routing.",
  "parameters": {
    "type": "object",
    "properties": {
      "input_id":  {"type": "integer", "minimum": 1, "maximum": 64,
                    "description": "Input channel number (1-based)."},
      "array":     {"type": "integer", "minimum": 1, "maximum": 10,
                    "description": "Output array number (1-10)."},
      "value":     {"type": "number",  "minimum": -60.0, "maximum": 0.0,
                    "description": "Attenuation in dB."}
    },
    "required": ["input_id", "array", "value"]
  },
  "tier": 2,
  "internal_osc_path_template": "/wfs/input/arrayAtten{array}",
  "internal_variable_template": "inputArrayAtten{array}",
  "csv_section": "Array Attenuation",
  "group_key": "input_array_attenuation",
  "supports_relative": false
}
```

The override `"inputArrayAtten*": 2` in `tool_tier_overrides.json` covers all 10 underlying paramIDs with one entry. The internal-OSC-path and internal-variable fields use a template form because the actual OSC path / paramID depends on the `array` argument.

### D. Enum-typed parameter with non-sequential stored IDs

**CSV row** (from `WFS-UI_output.csv`, the EQ Shape row):

```
Section: EQ
Label: EQ Shape
Variable: outputEQshape<band>
UI: Drop down menu
Type: INT
Min: 1
Max: 7
enum: Low Cut (1) ; Low Shelf (2) ; Peak/Notch (3) ; Band Pass (4) ; High Shelf (5) ; High Cut (6) ; All Pass (7)
OSC path: /wfs/output/EQshape <ID> <band> <value>
```

**Expected JSON output** (note the non-sequential mapping — All Pass has stored ID 7 even though it appears 7th in the visual order):

```json
{
  "name": "output.eq.set_shape",
  "description": "Sets the filter shape of one EQ band on an output channel. Stored IDs are not sequential: BandPass=4, HighShelf=5, HighCut=6, AllPass=7. The OFF state is handled by a separate band-toggle tool, not by an enum value.",
  "parameters": {
    "type": "object",
    "properties": {
      "output_id": {"type": "integer", "minimum": 1, "maximum": 64,
                    "description": "Output channel number (1-based)."},
      "band":      {"type": "integer", "minimum": 1, "maximum": 6,
                    "description": "EQ band number (1-6)."},
      "shape":     {"type": "string",
                    "enum": ["LowCut", "LowShelf", "Peak", "BandPass",
                             "HighShelf", "HighCut", "AllPass"],
                    "description": "Filter shape. The C++ side maps these strings to stored IDs 1, 2, 3, 4, 5, 6, 7 respectively."}
    },
    "required": ["output_id", "band", "shape"]
  },
  "tier": 1,
  "internal_osc_path": "/wfs/output/EQshape",
  "internal_variable": "outputEQshape",
  "enum_string_to_int": {
    "LowCut": 1, "LowShelf": 2, "Peak": 3,
    "BandPass": 4, "HighShelf": 5, "HighCut": 6, "AllPass": 7
  },
  "csv_section": "EQ",
  "group_key": "output_eq",
  "supports_relative": false
}
```

The generator emits string-enum schemas (so the AI sees and speaks semantic values like `"BandPass"`) plus an `enum_string_to_int` map the C++ dispatcher uses to translate to the stored integer.

### E. 12-column-layout row (no Formula, no OSC-path column)

**CSV row** (from `WFS-UI_network.csv`, 12 columns):

```
Section: Network
Label: Target/Server Protocol
Variable: networkTSProtocol
UI: drop down menu
Type: INT
Min: 0
Max: 7
Default: 0
enum: DISABLED ; OSC ; Remote ; ADM-OSC ; QLab
Notes: For each target/server (up to 6 targets/servers). Stored values: 0=DISABLED, 1=OSC, 2=Remote, 3=ADM-OSC, 7=QLab (4-6 reserved).
Hover: Select the Protocol: DISABLED, OSC, REMOTE, or ADM-OSC.
```

**Expected JSON output** (the generator constructs the OSC path from convention since the 12-column layout doesn't carry one explicitly — `/wfs/config/network/<variable>` for config CSV, `/wfs/network/<variable>` for network CSV):

```json
{
  "name": "network.target.set_protocol",
  "description": "Sets the protocol for a network target. Each WFS-DIY install supports up to 6 target/server slots. Stored values are non-sequential: DISABLED=0, OSC=1, Remote=2, ADM-OSC=3, QLab=7 (slots 4-6 are reserved for future protocols).",
  "parameters": {
    "type": "object",
    "properties": {
      "target_index": {"type": "integer", "minimum": 1, "maximum": 6,
                       "description": "Network target slot (1-6)."},
      "protocol":     {"type": "string",
                       "enum": ["DISABLED", "OSC", "Remote", "ADM-OSC", "QLab"],
                       "description": "Protocol to use for this target."}
    },
    "required": ["target_index", "protocol"]
  },
  "tier": 2,
  "internal_osc_path": "/wfs/network/targetProtocol",
  "internal_variable": "networkTSProtocol",
  "enum_string_to_int": {
    "DISABLED": 0, "OSC": 1, "Remote": 2, "ADM-OSC": 3, "QLab": 7
  },
  "csv_section": "Network",
  "group_key": "network_target",
  "supports_relative": false
}
```

The 12-column layout's lack of an explicit OSC-path column means the generator derives the path from a per-CSV convention; document the chosen convention in `MCP_SERVER_DESIGN.md` § Dispatcher pattern so the C++ side stays consistent.
