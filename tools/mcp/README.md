# MCP tool generator

Generates `Source/Network/MCP/generated_tools.json` (and a companion
`generated_groups.json`) from the seven `Documentation/WFS-UI_*.csv` source
files. The MCP server (built in a future Claude Code session) loads these at
startup and registers the corresponding tools.

This is **Phase 0** of the MCP roadmap — see `Documentation/MCP/IMPLEMENTATION_ROADMAP.md`.
The authoritative spec for the generator's behaviour is
`Documentation/MCP/specs/GENERATION_SCRIPT_SPEC.md`.

## Usage

From the repo root:

```
python tools/generate_mcp_tools.py
```

Default arguments match this repo's layout. Run with `--help` to see the
full option list.

The script is idempotent: a second run with no input changes is a fast-path
no-op (input hash compared against the existing output).

## Tests

```
python -m pytest tools/mcp/
```

Covers the 5 golden examples from `GENERATION_SCRIPT_SPEC.md` § Appendix,
plus override-precedence, ignore-list filtering, deterministic ordering,
fast-path idempotency, and an integration test that runs the generator
against the live CSVs and verifies a sane output shape.

## Override files

`tool_tier_overrides.json`

Per-Variable tier overrides. Three key conventions:

- **Literal Variable name** for plain rows: `"inputAttenuation": 2`.
- **Verbatim `<band>` placeholder** for per-band rows: `"outputEQshape<band>": 1`.
- **Trailing `*` wildcard** for numeric-suffix families: `"inputArrayAtten*": 2`.
  The wildcard matches by literal prefix; it is **not** a regex. Per-instance
  overrides (e.g. setting only `inputArrayAtten5`) are not supported.

Tier semantics: 1 = reversible/instant, 2 = significant (requires confirmation),
3 = destructive (requires safety gate). See
`Documentation/MCP/specs/MCP_TOOL_SURFACE.md` § Tier assignment heuristics.

`tool_generation_ignores.json`

Variables to skip entirely. Each entry is `{"variable": "name", "reason": "..."}`.
The reason is for documentation only.

The script also has a built-in non-parameter UI filter (`is_non_parameter_row`)
that drops rows whose `UI` cell indicates a documentation-only row (visual
indicator, mouse gesture description, etc.) regardless of whether they appear
in the ignore file.

## What the generator does NOT consume

- `Documentation/WFS-UI_arrayWizard.md` — the Wizard of OutZ preset catalog
  is fed to a hand-written `output.apply_array_preset` tool, not the
  generator.
- `Documentation/WFS-UI_plugins.md` — the DAW plugin Track variants drive
  the same `/wfs/input/*` paths already covered by `WFS-UI_input.csv`; no
  separate plugin tools are needed on the MCP side.
- `Documentation/OSC-Remote_InputParameterTab.csv` — the Android Remote's
  source of truth for `/remoteInput/*` paths. AI clients use `/wfs/*` paths
  via the MCP dispatcher, not `/remoteInput/*`.

## Output schema

`generated_tools.json` contains:

- `schema_version`, `generated_at`, `input_hash`, `source_csvs`
- `tools[]` — sorted by name; each is a JSON-Schema-shaped tool record with
  `name`, `description`, `parameters`, `tier`, `internal_osc_path` (or
  `internal_osc_path_template` for numeric-suffix families),
  `internal_variable` (or `_template`), `csv_section`, `group_key`,
  `supports_relative`, optional `enum_string_to_int` for non-sequential
  enum IDs, optional `relative_tool_name`.
- `nudge_tools[]` — relative-step variants of tools whose CSV row carries
  `OSC inc/dec = y`.
- `ignored_parameters[]` — rows that were skipped (with reason) for
  diagnostics.
- `warnings[]` — non-fatal issues (e.g. missing hover help text).

`generated_groups.json` contains a `group_key → [tool names]` lookup used by
the AI undo system's dependency-chasing logic and by compound-tool
unit tests in later phases.

## Build-system integration

Not wired up yet. The recommended hook is a Projucer pre-build command:

```
python tools/generate_mcp_tools.py
```

Configure this in `WFS-DIY.jucer` once the C++-side MCP server lands. The
fast-path no-op makes incremental builds cheap.

In the meantime, regenerate manually after editing any of the source CSVs,
override files, or the script itself, and commit the updated outputs along
with the source change so users without Python can still build.
