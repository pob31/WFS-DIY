# MCP tool catalog and vocabulary

The connect-time instructions keep only what you need to start. This is the
rest: which tool to reach for, the vocabulary the parameter filters use, and
why the catalog you see is smaller than the set of tools you can call.

## Why some tools are not listed

Every parameter in WFS-DIY has a dedicated auto-generated tool named
`<area>_set_<param>` — around 393 of them. Listing them all made `tools/list`
roughly 240 KB, which is a large amount of context to spend before you have
been asked anything, so tier-1 and tier-2 generated tools are **registered but
not advertised**.

They still work. If you know the name, `tools/call` resolves it normally, and
`mcp_describe_parameters` reports each parameter's `tool_name` in `mode="full"`.

Tier-3 generated tools **are** listed. `wfs_set_parameter` refuses tier-3
parameters by design, so those tools are the only route to a destructive
change and need to stay visible.

## Reading state

| Tool | Use it for |
| --- | --- |
| `mcp_describe_parameters` | What parameters exist. Call with no arguments for a group map, then filter. |
| `wfs_get_parameter` / `wfs_get_parameters` | Current value of a known parameter, one or many. |
| `session_get_state` | Per-channel id, name and position across the project. |
| `session_get_global_state` | Stage, master, binaural, network and other globals. Use `sections` to keep it small. |
| `session_get_channel_full` | Everything on one channel. Large — prefer the targeted reads above. |
| `session_get_state_delta` | What changed since your last call. Use between turns to notice operator, OSC or automation edits. |
| `mcp_get_ai_change_history` | What you have already done this session. Compact by default. |

## Writing state

| Tool | Use it for |
| --- | --- |
| `wfs_set_parameter` | One absolute value. Validated against the registry: enum membership and min/max. Tier 2. |
| `wfs_set_parameter_batch` | Several writes at once — atomic, one undo entry, one confirmation. Preferred for multi-write flows. |
| `wfs_nudge_parameter` | Relative moves ("a bit louder"). Clamps at the range limit. Tier 1. Numeric, non-EQ, tier-1 parameters only. |
| `input_create` / `input_delete` (also output, reverb) | Channel lifecycle, one channel at a time. Tier 2. |
| `reverb_auto_layout` | High-level reverb placement from speaker topology, written as one atomic batch. |
| `session_save` | Persist the project to disk. Tier 2, overwrites the operator's files, and cannot be undone. |

Undo with `mcp_undo_last_ai_change`, redo with `mcp_redo_last_undone_ai_change`.
Undo steps over records that document irreversible actions such as a save.

## Filter vocabulary for `mcp_describe_parameters`

**`scope`** — `global` (stage, master, network, binaural, config), `input`,
`output`, `reverb`, `cluster`, `eq_band`.

**`domain`** — what the parameter actually affects:

| Domain | Meaning |
| --- | --- |
| `wfs_synthesis` | Changes what the WFS speakers emit |
| `reverb` | Reverb sends, returns and algorithm settings |
| `binaural` | Headphone / binaural rendering |
| `adm_osc` | ADM-OSC interop |
| `floor_reflections` | Floor reflection modelling (called Hackoustics in the UI) |
| `live_source` | Live source damping (Live Source Tamer in the UI) |
| `tracking` | Position tracking input |
| `routing` | Channel and bus routing |
| `network` | Ports, protocols, transport settings |
| `visualisation_only` | Map and lock toggles that do not affect audio |
| `metadata` | Names, labels, notes |

**`group_key`** matches the generated tool family that writes the parameter
(for example `input_position`). The no-argument call returns the full list.

**`prefix`** matches the start of the canonical variable name and is
case-sensitive.

## Tiers

- **Tier 1** runs immediately.
- **Tier 2** needs a confirm token: the first call returns
  `tier_enforcement.confirmation_token`, and you re-call with `confirm` set to
  it within 30 seconds. An open operator window can stand in for the token.
- **Tier 3** needs the operator's safety gate to be open. The gate also covers
  tier 2 — it is the operator's superset trust window.

Both operator windows auto-close after five minutes and can only be opened by
the operator, not by you.

## Guided workflows

`prompts/list` carries templates for session startup, system tuning, array
design, snapshot management, rehearsal voice sessions and localization
troubleshooting. Fetch the matching one with `prompts/get` rather than
improvising a long multi-step plan.
