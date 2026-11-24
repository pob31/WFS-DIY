# MCP tool catalog and vocabulary

The connect-time instructions keep only what you need to start. This is the
rest: which tool to reach for, the vocabulary the parameter filters use, and
why the catalog you see is smaller than the set of tools you can call.

## Why some tools are not listed

Every parameter in WFS-DIY has a dedicated auto-generated tool named
`<area>_set_<param>`: about 700 of them with their nudge variants. Listing
them all would make `tools/list` several hundred KB, a large amount of context
to spend before you have been asked anything, so tier-1 and tier-2 generated
tools are **registered but not advertised**.

They still work. If you know the name, `tools/call` resolves it normally, and
`mcp_describe_parameters` reports each parameter's `tool_name` in `mode="full"`.

A few parameters come in numbered families, such as an input's ten array
attenuations `inputArrayAtten1` … `inputArrayAtten10`. A family shares ONE
dedicated tool (`input_set_array_attenuation`) whose extra argument picks the
member; `mode="full"` gives that argument as `tool_args` (`{"array": 3}` for
`inputArrayAtten3`). Each member is also an ordinary parameter name for
`wfs_set_parameter` and `wfs_get_parameter`.

Tier-3 generated tools **are** listed. `wfs_set_parameter` refuses tier-3
parameters by design, so those tools are the only route to a destructive
change and need to stay visible.

## Reading state

| Tool | Use it for |
| --- | --- |
| `mcp_describe_parameters` | What parameters exist. Call with no arguments for a group map, then filter. |
| `wfs_get_parameter` / `wfs_get_parameters` | Current value of a known parameter, one or many. |
| `session_get_state` | Per-channel id, name and position across the project: inputs in display order with their type (mono/stereo), outputs, reverbs and effects (with their mute). |
| `effect_get` | Everything about one effect channel: its sections, its chain in playing order with every module parameter, and who feeds it. |
| `map_shapes_list` | The outline shapes drawn on the Map tab (visual aids), with their index, geometry and style. |
| `session_get_global_state` | Stage, master, binaural, network and other globals. Use `sections` to keep it small. |
| `session_get_channel_full` | Everything on one channel. Large — prefer the targeted reads above. |
| `session_get_state_delta` | What changed since your last call. Use between turns to notice operator, OSC or automation edits. |
| `input_get_effect_sends` | One input's sends into every effect channel: on/off and level in dB per effect. |
| `mcp_get_ai_change_history` | What you have already done this session. Compact by default. |

## Writing state

| Tool | Use it for |
| --- | --- |
| `wfs_set_parameter` | One absolute value. Validated against the registry: enum membership and min/max. Tier 2. |
| `wfs_set_parameter_batch` | Several writes at once — atomic, one undo entry, one confirmation. Preferred for multi-write flows. |
| `wfs_nudge_parameter` | Relative moves ("a bit louder"). Clamps at the range limit. Tier 1. Numeric, non-EQ, tier-1 parameters only. |
| `input_create` / `input_delete` (also output, reverb, effect) | Channel lifecycle. Tier 2, stopped-only. |
| `input_move` / `input_reorder` | Rearrange the inputs' display order; numbers, settings and hardware inputs travel with each channel. Tier 2, stopped-only. |
| `input_renumber` | Give the inputs new permanent numbers without moving them. Tier 3: snapshots, cues and OSC then reach different channels. |
| `effect_set_chain_order` / `effect_set_module_on` | An effect's module order and its module switches. |
| `effect_set_fx_send_level` / `effect_set_fx_send_on` | One effect feeding another (the Effects Matrix). |
| `effect_clear` | Flush an effect's tails and delay lines (no setting changes). |
| `map_shape_add` / `map_shape_edit` / `map_shape_delete` / `map_shapes_clear` | Draw, change and remove the Map tab's outline shapes. Not undoable. |
| `input_set_effect_send_level` / `input_set_effect_send_on` | One input's send into one effect channel: the level in dB (tier 2) or the on/off switch, which keeps the level (tier 1). No generated tool covers these cells - an effect stores them as packed rows. |
| generated `effect_*` tools | One effect value each. EQ bands, dynamics stages and delay taps need these tools (their `instance`, `effect_band` or `tap` argument); `wfs_set_parameter` refuses them. See resource `wfs://knowledge/effects`. |
| `snapshot_store` / `snapshot_update` / `snapshot_delete` | Prepare snapshots. There is no recall over MCP: a recall is a cue, and cues are fired by the operator. |
| `snapshot_get_scope` / `snapshot_set_scope` | What a snapshot covers (per item and channel), when the scope applies, and its MIDI trigger. |
| `snapshot_export_cues` / `input_sampler_export_set_cue` | Write cues into QLab / Go.dot for the operator to fire. |
| `cluster_transform` | Move, rotate, scale or re-level a whole cluster about its reference point. |
| `cluster_lfo_presets_list` / `cluster_lfo_preset_get` / `cluster_lfo_preset_store` / `cluster_lfo_preset_set_name` | The 16 LFO preset slots. Recalling one, and switching an LFO on or off, is the operator's. |
| `input_gradient_shapes_list` / `input_gradient_shape_add` / `input_gradient_shape_arrange` | Draw, remove, switch and stack gradient-map shapes. |
| `stage_scale_inputs` / `channels_shift` | Rescale or fit inputs after a stage change; shift inputs, outputs or reverbs by an offset. |
| `output_apply_array_geometry` | Lay out a run of outputs as one array (the Wizard of OutZ). |
| `patch_get` / `patch_set` | The hardware patch; writes only while processing is stopped. |
| `network_targets_list` / `network_target_create` / `network_target_delete` | The Network tab's targets. |
| `*_set_output_mute` / `*_apply_mute_macro` | Per-output mutes and mute macros for inputs, reverbs and effects. |
| `reverb_auto_layout` | High-level reverb placement from speaker topology, written as one atomic batch. |
| `session_save` | Persist the project to disk. Tier 2, overwrites the operator's files, and cannot be undone. |

**Not for live use.** MCP prepares a show; it does not run one. Nothing here recalls a snapshot,
starts or stops a movement (AutomOtion, LFO), recalls an LFO preset or plays the sampler: an agent's
response time is no cue timing, so those stay with the operator.

Undo with `mcp_undo_last_ai_change`, redo with `mcp_redo_last_undone_ai_change`.
Undo steps over records that document irreversible actions such as a save.

## Filter vocabulary for `mcp_describe_parameters`

**`scope`** — `global` (stage, master, network, binaural, config), `input`,
`output`, `reverb`, `cluster`, `effect`, `eq_band`.

**`domain`** — what the parameter actually affects:

| Domain | Meaning |
| --- | --- |
| `wfs_synthesis` | Changes what the WFS speakers emit |
| `reverb` | Reverb sends, returns and algorithm settings |
| `effects` | Effects channels: their chains, feeds, returns and settings |
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
