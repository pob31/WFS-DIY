# Generation Script Specification

A build-step script that consumes the WFS-DIY parameter CSVs and emits a JSON schema representation of the MCP tool surface. Run as part of the build. Output is committed or regenerated on each build — implementer's choice.

## Purpose

Single source of truth for the MCP tool surface. The CSVs already drive the UI and the OSC layer; they should also drive the MCP tools. Hand-maintaining parallel MCP tool definitions would guarantee drift.

## Language

Python 3.10+ recommended. Reasons: standard `csv` module handles the inputs natively, json output is trivial, iteration speed is fast, and the script is not performance-critical. No runtime dependency for users — this is a build-step only.

Alternative: TypeScript/Node if the team prefers a single-language toolchain. Either is fine. Avoid shell scripts (multi-line CSV handling is painful).

## Inputs

The CSVs located in the repo (paths TBD by the implementer — probably `docs/csv/` or similar):

- `WFS-UI_config.csv`       — system, stage, network, master settings
- `WFS-UI_input.csv`        — input channel parameters
- `WFS-UI_output.csv`       — output channel parameters  
- `WFS-UI_reverb.csv`       — reverb channel parameters
- `WFS-UI_network.csv`      — network targets
- `WFS-UI_audioPatch.csv`   — audio patching
- `WFS-network.csv`         — network/tracking/ADM-OSC parameters

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
   - For per-channel parameters (the Variable starts with `input`, `output`, `reverb`), the first argument is always the channel id as an integer, and the range depends on the configured channel count (script emits the max as 64 — the hard upper limit — and the server validates against the runtime count).

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

## The bulk/compound tools

The generation script does NOT generate the hand-written compound tools (`input.configure_lfo`, etc.). Those are written in C++ directly. The generator's output is only the per-parameter tools.

However, the generator SHOULD emit a metadata file listing which parameters belong to which section, so the C++ compound-tool implementations can be validated against "did we cover every LFO parameter?" as a unit test.

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
- JSON schemas round-trip (serialize, deserialize, re-serialize, compare).

Failures abort the build with a clear error message.

## Override file formats

`tool_tier_overrides.json`:
```json
{
  "inputAttenuation": 2,
  "masterLevel": 3,
  "inputPositionX": 1
}
```

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

Integration test: run against the full CSVs, verify the output loads as valid JSON and contains at least N tools (where N is a sanity check — currently ~400+).

## A note on the reverb CSV's scope

The reverb parameters mostly mirror input and output parameters (position, directivity, attenuation). The generator should treat `reverb` as its own namespace (`reverb.feed.set_attenuation`, `reverb.return.set_directivity`) but use the same transformation logic. The CSV structure should make this straightforward.

## Run as a build step

Add to the build:

```
python3 tools/generate_mcp_tools.py \
  --csv-dir docs/csv \
  --overrides-tier tools/mcp/tool_tier_overrides.json \
  --overrides-ignore tools/mcp/tool_generation_ignores.json \
  --output Source/Network/MCP/generated_tools.json
```

If the output file is already up-to-date, the script should exit quickly (fast-path: hash the inputs, compare to the hash in the existing output, skip if unchanged).

The build step depends on this output; the C++ code includes it at compile time (via resource embedding) or loads it at runtime. Both are acceptable — runtime loading allows hot-reload during development.
