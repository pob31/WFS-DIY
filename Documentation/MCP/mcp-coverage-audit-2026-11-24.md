# MCP coverage audit - 2026-11-24

What an operator (GUI) or a remote (OSC) can do in WFS-DIY that the MCP server cannot, found on branch `mcp/coverage-gaps` after the effects / map-shapes / input-order tools landed. Nine area auditors listed gaps; three independent skeptics then tried to refute each one, and a gap is kept only when at least two failed. A completeness critic looked for areas nobody checked. 184 agents, 57 confirmed findings (some found from two areas; duplicates are marked), 1 refuted.

Evidence lines are the auditors' file:line references at the time of the audit; check them against the code before relying on them.

## Decisions

- **No timed transitions (fades) over MCP.** An agent's timing is not predictable enough for a fade to mean anything (`EMIT_TRANSITION_SECONDS = False` in `tools/mcp/wfs_codegen_config.py`). The critic's "Timed transitions have no MCP equivalent" is therefore intentional.

- **Anything time-sensitive is triggered by the operator, never by an agent.** Snapshot recall,
  AutomOtion start / stop / pause, cluster LFO preset recall, LFO on / off and sampler playback: an
  agent's response time depends on how long it thinks, and a missed cue is the operator's problem. MCP
  sets the parameters of these and never starts them; it is not for live situations. For the same
  reason the generated `input_lfo_set_active`, `effect_lfo_set_active`, `cluster_lfo_set_active` and
  the two `*_otomo_pause_resume` tools were removed (2026-11-24). The per-tab "Reload config / backup"
  and the system config import restore a stored state wholesale, the same kind of act as a recall, and
  stay the operator's too; so do the output test signals (hearing protection) and the head-tracker Set
  Zero (it needs the person looking at the stage).
- **The audio processing switches are for the operator only.** Run DSP (processing on/off) and the algorithm choice stay off the MCP surface; the critic's finding about them is intentional.


## Group 1 - tools that answer "ok" but change nothing, or cannot express useful values

**Status: fixed (2026-11-24, branch `mcp/coverage-gaps`).** Network targets, the source filter, tracking
and OSC Query written over MCP are applied by the Network tab (`NetworkTab::applyExternalChange`,
triggered by its tree listener for MCP-origin writes); targets are addressed by slot, and
`network_targets_list` / `network_target_create` / `network_target_delete` exist. Output writes follow
Apply to Array with an optional `this_output_only` and undo every member they moved
(`MCPOutputWrites.h`). `reverb_set_output_mute`, `effect_set_output_mute` and
`input|reverb|effect_apply_mute_macro` replace the no-op tools. Gradient white/black are validated per
layer and the shape position/size take stage metres. `input_sampler_set_set_cells`; the sampler
controller mode written over MCP runs the System Config selector's path. `cluster_lfo_preset_set_name`
/ `cluster_lfo_presets_list` (slots 1-16). `networkCurrentIP` and `samplerMidiZoneQuadrant` are no
longer generated; `session_get_global_state` reports the live IPv4 addresses. Gate:
`tools/validation/control-replay/mcp_coverage_check.py` (audit group 1 section).

### Gradient-map layer white/black tools use the wrong units and range

*High impact - broken tool - area: inputs-core*

- **Capability:** Each gradient layer maps white and black to real values. Layer 0 (attenuation) uses -92..0 dB, layer 1 (height) -50..50, layer 2 (HF shelf) -24..0 dB. The default for both is 0.
- **Where:** Source/GradientMap/GradientMapEditor.h:665-673 (getLayerValueBounds), 677-691 and 3164-3183 (white/black clamped to those bounds); Source/Parameters/WFSParameterDefaults.h:695-696
- **Why MCP cannot:** input_gradient_map_set_gm_layer_white and _black declare the range 0.0..1.0 (from Documentation/WFS-UI_input.csv:128-129), and the dispatcher refuses any value outside the schema range (MCPGeneratedToolLoader.cpp:497-527). On the attenuation and HF layers the only value MCP can write is 0. On the height layer it cannot write negative values or anything above 1 m. Every useful attenuation or HF gradient is out of reach.
- **Suggested:** Fix the CSV ranges and add a per-layer validation step (or split by layer) in input_gradient_map_set_gm_layer_white/black

### Sampler Controller tool does not switch the sampler on

*High impact - broken tool - area: inputs-sampler-stereo*

- **Capability:** Choosing Off, Lightpad or Remote in System Config writes two things: SamplerEnabled (true when the mode is not Off) and SamplerControllerMode. It then runs applySamplerControllerMode, which starts or stops the Lightpad manager, sets the Sampler sub-tab's controller mode and pad grid, and sends the pad configuration to the remote. It also shows the per-input sampler toggle and the Sampler sub-tab.
- **Where:** Source/gui/SystemConfigTab.h:976-988; Source/MainComponent.cpp:518-528 and 14310-14345 (applySamplerControllerMode); Source/gui/InputsTab.h:899-908 (setSamplerMasterEnabled)
- **Why MCP cannot:** system_controllers_set_sampler_controller_mode only writes the SamplerControllerMode property (Source/Network/MCP/generated_tools.json:19026-19060). Its description still says "Enable or disable the Sampler feature". No MCP tool writes samplerEnabled (it has no CSV row and no generated tool). SystemConfigTab::valueTreePropertyChanged only reloads the selector with dontSendNotification (Source/gui/SystemConfigTab.h:2712-2719, 2817-2821), so applySamplerControllerMode never runs. Even after the project is reloaded, SamplerEnabled is still false, so the mode is forced to 0 (Source/MainComponent.cpp:16114-16117). Failure: an AI sets mode=Remote and gets success, but the sampler UI stays hidden, the Lightpad does not start, and the tablet never receives the pad configuration.
- **Suggested:** Hand-written system_set_sampler_controller(mode) that writes SamplerEnabled and SamplerControllerMode and then calls applySamplerControllerMode (or a generated post-write hook for this variable)

### Sampler set cell membership (samplerSetCells) cannot be written

*High impact - parameter - area: inputs-sampler-stereo*

- **Capability:** The operator decides which of the 36 cells belong to a sampler set. Ctrl+click on a cell adds it to or removes it from the active set, and Add Set seeds the new set with the selected cells. A set's cell list is what the pad or Lightpad triggers, so a set with no cells plays nothing.
- **Where:** Source/gui/SamplerSubTab.h:428-437 (Ctrl+Click toggles set.cellIndices); Source/gui/SamplerSubTab.h:1358 (onAddSet assigns selectedCells); Source/gui/SamplerSubTab.h:1765 (written as samplerSetCells); Source/Sampler/SamplerData.h:179 (the engine reads cellIndices from samplerSetCells)
- **Why MCP cannot:** The generator skipped the row automatically, not on purpose: Source/Network/MCP/generated_tools.json:27890 gives the reason "non-parameter UI row (UI='(internal)')" (Documentation/WFS-UI_input.csv:163). It is not in the registry, so wfs_set_parameter refuses it (Source/Network/MCP/tools/SetParameterTool.h:83-103). input_sampler_create_set only takes input_id and name (Source/Network/MCP/tools/ChannelLifecycleTools.h:654-684, 753), so every set made over MCP is empty and cannot be filled. The value can be read through session_get_channel_full.
- **Suggested:** input_sampler_set_set_cells(input_id, set_id, cells:[0..35]) (or add an optional cells argument to input_sampler_create_set)

### Network target writes never reach the live OSC engine

*High impact - broken tool - area: network*

- **Capability:** Editing a target row in the GUI (name, IP, Tx port, data mode, Rx/Tx enable, protocol, QLab patch) or the OSC Source Filter applies at once. Each GUI handler saves to the tree and then calls updateOSCManagerConfig(), which builds the TargetConfig and GlobalConfig (allowed IPs, ipFilteringEnabled) and calls oscManager->applyGlobalConfig and the per-target apply. Changing a protocol, IP or port therefore starts or stops sending and receiving right away.
- **Where:** Source/gui/NetworkTab.h:3336-3380 (updateOSCManagerConfig reads the GUI row widgets and oscSourceFilterButton, not the tree); Source/gui/NetworkTab.h:5770 (saveTargetToValueTree ends with updateOSCManagerConfig)
- **Why MCP cannot:** The generated network_set_t_s* tools (network_set_ts_protocol, network_set_t_sip, network_set_t_sport, network_set_t_sdata_mode, network_set_t_srx_enable, network_set_t_stx_enable, network_set_t_sname, network_set_t_sqlab_patch) and network_set_osc_source_filter only set a ValueTree property (MCPGeneratedToolLoader.cpp:210-217 binds them to the NetworkTarget node). Nothing reacts to that property: NetworkTab::valueTreePropertyChanged (NetworkTab.h:4582-4591) handles only networkRxUDPport/TCPport, its child-added and child-removed hooks are empty (4593-4594), and OSCManager::valueTreePropertyChanged (OSCManager.cpp:962-1030) has no networkTS*/networkOscSourceFilter branch. The new values take effect only after a project reload (MainComponent.cpp:16069 refreshFromValueTree), the tool still reports success, and the GUI keeps showing the old values.
- **Suggested:** Add a post-write hook for network-domain variables that calls NetworkTab::refreshFromValueTree() (load + updateOSCManagerConfig) on the message thread, or have OSCManager listen to the Network subtree.

### No way to add or remove a network target

*High impact - structural - area: network*

- **Capability:** The 'Add Target/Server' button creates a target (up to 6). A long press on the per-row Remove button deletes it (child added to or removed from Config/Network).
- **Where:** Source/gui/NetworkTab.h:3445 (addTargetButton.onClick -> addNewTarget); Source/gui/NetworkTab.h:3589 (row.removeButton.onLongPress -> removeTarget); WFSValueTreeState.cpp:3155-3170 (addNetworkTarget/removeNetworkTarget exist)
- **Why MCP cannot:** No MCP tool calls addNetworkTarget/removeNetworkTarget (grep over Source/Network/MCP finds none). MCPServer.cpp:107-226 registers no network lifecycle tool. A new session has no targets (WFSValueTreeState.cpp:5957-5971 createNetworkSection appends no NetworkTarget children), so every network_set_t_s* tool fails with 'No network target N' until someone adds a target in the GUI.
- **Suggested:** network_target_create / network_target_delete (target_id 1-6), going through the same path as NetworkTab::addNewTarget/removeTarget and then updateOSCManagerConfig.

### Tracking settings written over MCP never reconfigure the receiver

*High impact - broken tool - area: network*

- **Capability:** The GUI tracking controls apply at once. The Enabled toggle checks cluster conflicts, then updateTrackingAppearance -> updateTrackingReceiver. Changing the protocol, port, OSC path, PSN interface or MQTT fields restarts the receiver. Offsets, scales and flips call updateTrackingTransformations(), which pushes the transform to the running OSC/PSN/RTTrP/MQTT receiver.
- **Where:** Source/gui/NetworkTab.h:3017-3030 (trackingEnabledButton), 3180-3188 (PSN interface -> updateTrackingReceiver), 4695-4722 (port -> updateTrackingReceiver(true); offset/scale -> updateTrackingTransformations), 4953/4968 (path/MQTT -> updateTrackingReceiver), 4275-4300 (updateTrackingTransformations reads flips from GUI buttons)
- **Why MCP cannot:** network_tracking_set_* (enabled, protocol, port, osc_path, psn_interface, mqtt_*, offset_*, scale_*, flip_*) only set config properties. updateTrackingReceiver/updateTrackingTransformations are called only from NetworkTab GUI handlers (grep: NetworkTab.h lines 3145-4968, 4579 load). OSCManager.cpp:1006-1010 reacts to trackingEnabled/trackingProtocol only to resend tracking state to Remote targets and starts no receiver. So enabling PSN tracking, or setting an offset or scale over MCP, reports success but changes nothing until the project is reloaded. The flip tools never reach the transform, because updateTrackingTransformations reads the GUI buttons, not the tree.
- **Suggested:** A post-write hook for tracking-domain variables that runs the NetworkTab reload path (loadParametersFromValueTree + updateTrackingReceiver(true) + updateTrackingTransformations).

### target_id picks the wrong network target after a remove/re-add

*High impact - broken tool - area: network*

- **Capability:** The GUI keys each target by its stored `id` property (slot 0-5). Adding a target fills the first empty slot and appends its child at the end of the tree. Removing one deletes the child with that id, so the children can be out of order or have gaps (for example ids [0,2,1]).
- **Where:** Source/gui/NetworkTab.h:3632-3654 (addNewTarget picks first inactive row); Source/gui/NetworkTab.h:5735-5752 (find-or-append child by id); Source/gui/NetworkTab.h:5773-5797 (remove by id); Source/gui/NetworkTab.h:5896-5905 (load maps child id -> row)
- **Why MCP cannot:** The MCP binding resolves target_id by child POSITION: MCPGeneratedToolLoader.cpp:213 node = state.getNetworkTargetState(subA - 1), and WFSValueTreeState.cpp:3172-3177 returns network.getChild(targetIndex). Example: targets 1-3 exist and the operator removes target 2. network_set_ts_protocol target_id=2 then edits the GUI's slot 3, and target_id=3 returns 'No network target 3' even though slot 3 is configured.
- **Suggested:** Resolve the NetworkTarget child whose `id` == target_id-1 (findChildByIntProperty), in both the generated loader and getNetworkTargetState.

### Apply to Array is ignored by MCP writes of non-EQ output parameters

*High impact - broken tool - area: outputs*

- **Capability:** In the GUI (and on the Stream Deck), editing an array-linked output parameter (attenuation, delay/latency, orientation, angle on/off, pitch, HF damping, minimal latency, live source atten, floor reflections, distance atten %, H/V parallax, EQ enable) on an output that belongs to an array spreads the change to the other outputs in that array. ABSOLUTE copies the value; RELATIVE applies the same delta, and members set to OFF are skipped. Holding Ctrl/Cmd edits one channel only.
- **Where:** Source/Parameters/ArrayParamEdit.h:39-46 (GUI write calls setOutputParameterWithArrayPropagation); Source/Parameters/WFSValueTreeState.cpp:1253-1269 (isArrayLinkedParameter list), 1337-1424 (propagation with ABSOLUTE/RELATIVE and the member's own mode); Documentation/WFS-UI_output.csv rows Attenuation..EQ Enabled ('changes may affect the rest of the array', Array value = Y)
- **Why MCP cannot:** The MCP non-EQ write paths call plain state.setParameter, which goes to TreeParameterStore::setParameter (spatcore/control/state/TreeParameterStore.cpp:59-64) and never propagates: generated tools at Source/Network/MCP/MCPGeneratedToolLoader.cpp:661, generated nudge tools at :937, wfs_set_parameter via MCPEffect::routeWrite at Source/Network/MCP/tools/SetParameterTool.h:346 (Source/Network/MCP/MCPEffectWrites.h:118), and wfs_set_parameter_batch at SetParameterBatchTool.h:195. Only output EQ band writes propagate (MCPGeneratedToolLoader.cpp:627, SetParameterTool.h:333). So an AI can set output_array_set_apply_to_array, but output_set_attenuation, output_delay_set_latency, output_set_orientation and the rest still change one speaker only, even though their tool descriptions say the change 'may affect the rest of the array'. EQ is the reverse: it always propagates and has no single-channel bypass. OSC /wfs/output/* also writes without propagation (Source/Network/OSCManager.cpp:2615), so only the GUI behaves this way.
- **Suggested:** Send MCP output writes through setOutputParameterWithArrayPropagation, and add an optional 'this_output_only' (bypass) argument to the output set/nudge tools and to wfs_set_parameter for output scope, matching Ctrl/Cmd in the GUI and the EQ path.

### reverb_set_mutes cannot mute anything (no output index, and writes are refused)

*High impact - broken tool - area: reverbs*

- **Capability:** The operator mutes or unmutes one reverb return to one speaker with the Mutes panel toggles. OSC does the same with /wfs/reverb/mutes <ID> <output ID> <value>. The result is stored as a packed per-output row, reverbMutes.
- **Where:** Source/gui/ReverbTab.h:1968 (muteButtons[i].onClick); Documentation/WFS-UI_reverb.csv Mutes row (/wfs/reverb/mutes <ID> <output ID> <value>)
- **Why MCP cannot:** Source/Network/MCP/generated_tools.json:18735: reverb_set_mutes takes only reverb_id and value enum ["unmute","MUTE"], with no output argument. The enum is coerced to a bare 0 or 1, and the write interceptor refuses a bare number on a mute row and keeps the old row (Source/Parameters/WFSValueTreeState.cpp:250-268, whose comment names this MCP enum as the route). The fallback also fails: wfs_set_parameter(reverbMutes, "0,1,...") is rejected because the registry entry is an enum and validateAgainstRegistry rejects any string with a comma (Source/Network/MCP/MCPParameterValidation.h:122-151). Inputs have a dedicated per-output tool (Tools::Input::describeSetOutputMute, MCPServer.cpp:115). Reverbs have none.
- **Suggested:** reverb_set_output_mute(reverb_id, output_id, muted), modelled on input_set_output_mute. Also drop or fix the generated reverb_set_mutes.

### effect_return_set_mute_macro writes a number and changes no mutes

*Medium impact - broken tool - area: effects-remaining*

- **Capability:** In the GUI, the Mute Macro selector (Mute All, Unmute All, Invert, Mute Odd, Mute Even) rewrites the output-mute buttons and saves effectMutes. The macro is applied only inside the panel (applyMuteMacro + saveMuteStates). The panel never even writes effectMuteMacro, and nothing outside the GUI reacts to the property. This is the same situation as inputMuteMacro, which tool_generation_ignores.json drops for exactly this reason. Also, the GUI's fifth macro, Mute Even, is missing from the tool's enum because effectMuteMacroMax is 4.
- **Where:** Source/gui/effects/EffectsChannelPanel.h:771-780 (onChange -> applyMuteMacro + saveMuteStates, then resets the combo) and 930-947 (applyMuteMacro works on the buttons only); Source/Parameters/WFSParameterDefaults.h:1252-1254
- **Why MCP cannot:** Source/Network/MCP/generated_tools.json:8269 (effect_return_set_mute_macro -> effectMuteMacro). grep 'effectMuteMacro' in Source finds no listener or applier, only the IDs, defaults, bounds, router alias (OSCMessageRouter.cpp:317), the QLab label, the snapshot group and the link-exclusion set (WFSValueTreeState.cpp:1991). tools/mcp/tool_generation_ignores.json:100-101 ignores inputMuteMacro for this exact reason but has no entry for effectMuteMacro. effect_return_set_mutes (the packed row) is the only real route.
- **Suggested:** Add effectMuteMacro to tool_generation_ignores.json (or apply the macro to effectMutes in the MCP route, covering macros 1-5) and point the AI to effect_return_set_mutes.

### Gradient-map shape position/scale tools use the wrong range

*Medium impact - broken tool - area: inputs-core*

- **Capability:** Gradient shapes are placed in stage coordinates in metres (centre) and sized by half-extent in metres, anywhere on the stage.
- **Where:** Source/GradientMap/GradientMapData.h:49-51 (posX/posY are the centre in metres, scaleX/Y half-extents in metres), 100-102; Source/Parameters/WFSParameterDefaults.h:722-725 (meters)
- **Why MCP cannot:** input_gradient_map_set_gm_shape_pos_x/pos_y declare -1.0..2.0 and scale_x/scale_y 0.01..4.0. The CSV (Documentation/WFS-UI_input.csv:133-137) still calls them 'normalised' and a 'scale factor'. The dispatcher refuses any value outside that range (MCPGeneratedToolLoader.cpp:497-527), so MCP cannot place a shape more than 2 m from the origin or make one wider than 8 m.
- **Suggested:** Fix the CSV rows to stage-metre ranges (e.g. -50..50 position, 0.01..50 half-extent) and regenerate

### OSC Query enable and port over MCP do not start, stop or move the server

*Medium impact - broken tool - area: network*

- **Capability:** The GUI OSC Query toggle saves to the tree, then calls updateOSCQueryServer(), which starts or stops the OSCQuery HTTP/WebSocket server on the chosen port.
- **Where:** Source/gui/NetworkTab.h:1717-1721 (toggle -> updateOSCQueryServer); Source/gui/NetworkTab.h:5815-5830 (updateOSCQueryServer reads oscQueryEnableButton state, not the tree)
- **Why MCP cannot:** network_set_osc_query_enabled (tier 2) and network_set_osc_query_port (tier 3) only write networkOscQueryEnabled/networkOscQueryPort to the tree. No listener calls updateOSCQueryServer (callers: NetworkTab.h:1721, 4579 only), and the server state changes only on the next project load.
- **Suggested:** The same network-domain post-write refresh (NetworkTab::refreshFromValueTree), or an OSCManager listener on these two properties.

### reverb_mutes_set_mute_macro is a no-op

*Medium impact - broken tool - area: reverbs*

- **Capability:** Mute Macros selector: MUTE ALL, UNMUTE ALL, INVERT, MUTE ODD or EVEN, and MUTE or UNMUTE ARRAY n. These rewrite the reverb's per-output mute row.
- **Where:** Source/gui/ReverbTab.h:1992-2002 (muteMacrosSelector.onChange -> applyMuteMacro), Source/gui/ReverbTab.h:4581 (applyMuteMacro)
- **Why MCP cannot:** Source/Network/MCP/generated_tools.json:17512 advertises reverb_mutes_set_mute_macro, which writes reverbMuteMacro. Nothing reads that property: grep for reverbMuteMacro finds only the default in WFSValueTreeState.cpp:6844, OSC bounds and OSCQuery ranges (OSCParameterBounds.cpp:409, OSCQueryServer.cpp:891), and the router map OSCMessageRouter.cpp:259. The tool sets a number and changes no mutes. The identical inputMuteMacro was deliberately ignored for this reason (tools/mcp/tool_generation_ignores.json:100), but reverbMuteMacro was not.
- **Suggested:** Make reverb_apply_mute_macro(reverb_id, macro) run the macro logic on reverbMutes, or add reverbMuteMacro to tool_generation_ignores.json once a per-output mute tool exists.

### Preset-name tool addressed by cluster_id 1-10, slots 11-16 unreachable

*Low impact - broken tool - area: clusters*

- **Capability:** Rename any of the 16 preset slots (slow-click a tile in the GUI). The remote mirror /remote/cluster/presetName covers all 16.
- **Where:** Source/gui/ClustersTab.h:1029-1036 (tile label writes clusterLFOPresetName on its slot); Source/Network/OSCManager.cpp:5803-5823 (all presets sent to the remote)
- **Why MCP cannot:** In generated_tools.json, cluster_lfo_presets_set_lfo_preset_name takes 'cluster_id' with maximum 10. The resolver uses that id as the preset slot index (WFSValueTreeState.cpp:7483-7489: presets.getChild(channelIndex)), so the tool claims to name a cluster but actually names preset N, and slots 11-16 are outside its schema. wfs_get_parameter caps the cluster scope at 10 (Source/Network/MCP/tools/GetParameterTool.h:124-128), so the names of slots 11-16 cannot be read at all.
- **Suggested:** Give clusterLFOPresetName its own 'preset' (1-16) index scope in the codegen config/registry instead of cluster_id, or fold renaming into cluster_lfo_preset_store / cluster_lfo_preset_get

### input_sampler_set_midi_zone_quadrant writes a parameter nothing reads

*Low impact - broken tool - area: inputs-sampler-stereo*

- **Capability:** The tool says it maps an input to the whole pad or to one quadrant of the Lightpad or Remote pad.
- **Where:** Documentation/WFS-UI_input.csv:145 (row text: 'Lightpad / Remote-pad quadrant routing')
- **Why MCP cannot:** samplerMidiZoneQuadrant is only declared, defaulted and bound (Source/Parameters/WFSValueTreeState.cpp:6314, Source/Network/OSCParameterBounds.cpp:319). Grep finds no reader, and Source/MainComponent.cpp:6664 says so: 'declared and defaulted but read by nothing'. The real zone routing is lightpadZoneId, which MCP cannot write. Failure: an AI that 'routes' input 3 to Q2 gets success and nothing changes.
- **Suggested:** Leave it out of generation (tool_generation_ignores.json) or point it at lightpadZoneId

### network_set_current_ip writes a display-only property; the real IP is unreadable

*Low impact - broken tool - area: network*

- **Capability:** The Network tab shows the processor's current IPv4 on the selected interface (read-only, computed from the OS interface list). Remote operators need this IP to point devices at the processor.
- **Where:** Documentation/WFS-UI_network.csv:3 ('Read-only. Shows IP address of the Processor on the selected interface'); Source/gui/NetworkTab.h:1684-1690 (interface selector drives the displayed IP)
- **Why MCP cannot:** networkCurrentIP is only ever set to the default '127.0.0.1' (WFSValueTreeState.cpp:5961), and nothing else in Source writes or reads it (grep). So network_set_current_ip writes a value nobody uses, and session_get_global_state / wfs_get_parameter always return 127.0.0.1 instead of the live address.
- **Suggested:** Drop network_set_current_ip (add it to tool_generation_ignores.json as read-only). Expose the live interface IP(s) read-only in session_get_global_state.network.


## Group 2 - parameters missing from the CSVs (a row and a regeneration)

**Status: fixed (2026-11-24, branch `mcp/coverage-gaps`), two left out on purpose.** New CSV rows:
`inputLSpeakEnable` / `inputLSslowEnable`, `inputOtomoCoordinateMode`, `inputAdmMapping` (-1..7), the
nine binaural HRTF settings and `RemotePadGridLayout`. The binaural settings and the pad layout were
also missing from the schema templates, so a session that had never opened the System Config section
could not be written at all; they are stamped now (`createBinauralSection`, `createUISection`, and the
stray migration moves an old `<IO>` copy of the pad layout). A Remote pad layout written over MCP
resends the pad configuration; a SOFA file must be a name in the project's `sofa/` folder.
`input_sampler_set_pad_zone` (hand-written: one input per zone, the Lightpad manager and the Remote pad
follow). Left out:
- **GPU pipeline depth** - an engine setting read when processing starts, beside the algorithm choice,
  which stays with the operator (see Decisions).
- **Per-input binaural solo** - transient monitoring state, like `effectSolo` (ignored for the same
  reason) and never in snapshots.

### Live Source Tamer peak/slow compressor enables missing

*High impact - parameter - area: inputs-core*

- **Capability:** The operator switches the Live Source Tamer peak compressor and slow compressor on and off per input. The DSP applies peak or slow gain reduction only while these are on, and both default to OFF. OSC: /wfs/input/LSpeakEnable and LSslowEnable (also liveSourcePeakEnable / liveSourceSlowEnable).
- **Where:** Source/gui/InputsTab.h:2183-2188, 2239-2244 (lsPeakEnableButton / lsSlowEnableButton); Source/DSP/LiveSourceTamerEngine.h:90-91 (peakOn/slowOn gate); Source/Network/OSCMessageRouter.cpp:172-173, 811, 814; Source/Parameters/WFSParameterDefaults.h:542,552 (default 0)
- **Why MCP cannot:** inputLSpeakEnable and inputLSslowEnable have no row in Documentation/WFS-UI_input.csv: the Live Source Tamer rows around 56-63 list threshold and ratio only. They are not in generated_tools.json and not in the ignores, and wfs_set_parameter refuses names outside that registry (SetParameterTool.h:83-105). So input_live_source_tamer_set_live_source_peak_threshold/ratio and slow_threshold/ratio write values that do nothing, because MCP can never enable the compressor.
- **Suggested:** Add CSV rows so the generator emits input_live_source_tamer_set_peak_enable / set_slow_enable (OFF/ON)

### Live Source Tamer peak/slow enable toggles unreachable

*High impact - parameter - area: map-and-remote*

- **Capability:** Turn on or off the Live Source Tamer's peak (fast) and slow compression stages per input (inputLSpeakEnable / inputLSslowEnable). These gate whether the peak and slow gain reductions apply at all. Available from the Inputs tab toggles, /wfs/input/LSpeakEnable|LSslowEnable, and the tablet's /remoteInput/liveSourcePeakEnable|liveSourceSlowEnable.
- **Where:** Source/gui/InputsTab.h:2188 and :2244 (saveInputParam inputLSpeakEnable/inputLSslowEnable); Source/Network/OSCMessageRouter.cpp:172-173 (/wfs/input) and :811,:814 (/remoteInput); Source/Network/OSCMessageBuilder.cpp:69,72; Source/DSP/LiveSourceTamerEngine.h:90-91 (the engine reads them)
- **Why MCP cannot:** Neither variable appears in Source/Network/MCP/generated_tools.json: diffing the /remoteInput address map (OSCMessageRouter.cpp:770-880) against every generated tool's internal_variable leaves only these two. They have no row in Documentation/WFS-UI_input*.csv (threshold and ratio rows are at lines 60-63, but there is no enable row), and they are not in tools/mcp/tool_generation_ignores.json. wfs_set_parameter whitelists against the registry built from generated_tools.json and refuses them as unknown_parameter (Source/Network/MCP/tools/SetParameterTool.h:89-105; MCPParameterRegistry.cpp:201).
- **Suggested:** Add CSV rows so the generator emits input_live_source_set_peak_enable / input_live_source_set_slow_enable (bool, input_id); wfs_set_parameter then reaches them too

### Per-input ADM-OSC mapping assignment is unreachable

*High impact - parameter - area: network*

- **Capability:** On the Inputs tab the operator picks the ADM-OSC mapping for each input (None, Cartesian 1-4, Polar 1-4). This writes inputAdmMapping (-1, 0-3, 4-7) on the input's Position section. OSCManager uses it to convert incoming and outgoing ADM-OSC positions for that input.
- **Where:** Source/gui/InputsTab.h:1115-1134 (admMappingSelector.onChange writes inputAdmMapping); Source/Network/OSCManager.cpp:7019, 7144 (read for ADM conversion); WFSParameterIDs.h:470
- **Why MCP cannot:** inputAdmMapping is in no Documentation/*.csv (grep finds no hit), so generated_tools.json has no tool and the MCPParameterRegistry (built from generated_tools.json, MCPParameterRegistry.cpp:201) rejects it in wfs_set_parameter as unknown_parameter (SetParameterTool.h:90-105). The generated network_adm_osc_* tool descriptions even tell the AI to 'select one through inputAdmMapping' (generated_tools.json:13867), but no tool writes it. The 4+4 mapping tables can be edited over MCP, yet no input can be attached to one.
- **Suggested:** Add an Inputs CSV row for inputAdmMapping (enum None ; Cartesian 1-4 ; Polar 1-4 -> -1..7) to generate input_set_adm_mapping.

### AutomOtion coordinate mode (polar movements) not reachable

*Medium impact - parameter - area: inputs-core*

- **Capability:** The operator sets the AutomOtion destination mode to Cartesian, Cylindrical or Spherical. In the polar modes the processor reads otomoR/Theta/Rsph/Phi and moves along an arc, which is how a whole-turn movement is made. OSC: /wfs/input/otomoCoordinateMode <ID> <0-2>.
- **Where:** Source/gui/InputsTab.h:2812 and 6251-6279 (save inputOtomoCoordinateMode); Source/Automation/AutomOtionProcessor.h:383-411 (coordMode picks the cylindrical/spherical path that reads family.r/theta); Source/Network/OSCMessageRouter.cpp:123
- **Why MCP cannot:** inputOtomoCoordinateMode has no row in Documentation/WFS-UI_input.csv (AutomOtion rows 93-112), no entry in generated_tools.json and none in tool_generation_ignores.json. wfs_set_parameter rejects it because the registry is loaded only from generated_tools.json (MCPParameterRegistry.h:16-24, SetParameterTool.h:83-105). The MCP tools input_automotion_set_otomo_r/theta/r_spherical/phi do exist, but their values are only read when the mode is 1 or 2, which MCP cannot set. CSV row 96 itself says 'AutomOtion reads it in cylindrical mode'.
- **Suggested:** Add a CSV row so the generator emits input_automotion_set_otomo_coordinate_mode (enum Cartesian/Cylindrical/Spherical)

### Pad zone assignment per input (lightpadZoneId) cannot be written

*Medium impact - parameter - area: inputs-sampler-stereo*

- **Capability:** The Lightpad Zone or Remote Zone button on the Sampler sub-tab opens an overlay where the operator picks which Lightpad or Remote XY-pad zone plays this input. The value is stored per channel, saved in snapshots, and used to build the zone-to-input map sent to the tablet.
- **Where:** Source/gui/SamplerSubTab.h:273-279, 2146-2156 and 2210-2214 (saveInputParam(lightpadZoneId)); Source/MainComponent.cpp:14403-14420 (buildZoneToInputMap); Source/Parameters/WFSFileManager.cpp:1671 (in the snapshot 'sampler' scope)
- **Why MCP cannot:** lightpadZoneId has no Variable in any Documentation/WFS-UI_*.csv. The CSV only has a read-only 'Lightpad Zone' display row (WFS-UI_input.csv:183). It has no generated tool and is not in the registry, so wfs_set_parameter refuses it. It can only be read through session_get_channel_full. The only related MCP tool is input_sampler_set_midi_zone_quadrant, which writes a dead parameter (see the next gap).
- **Suggested:** input_sampler_set_pad_zone(input_id, zone:-1..14) with the same one-claimant-per-zone rule (Source/MainComponent.cpp:14370-14399) and a remote pad-config push

### Binaural HRTF settings unreachable: render mode, SOFA file, listener X / ear height / head radius, manual yaw/pitch/roll, head-tracker source

*Medium impact - parameter - area: system-config*

- **Capability:** The System Config Binaural section writes binauralRenderMode (ORTF/Structural/SOFA), binauralSofaFile, binauralListenerX, binauralListenerHeight, binauralHeadRadius (cm->m), binauralListenerYaw/Pitch/Roll and binauralHeadTrackerSource on the Binaural node.
- **Where:** Source/gui/SystemConfigTab.h:1036-1043 (render mode), 1051 + 3932-3934 (SOFA file), 1059-1065 (tracker source), 1229-1239 (yaw dial), 3236-3262 (listener X, height, head radius, yaw, pitch, roll editors); IDs in Source/Parameters/WFSParameterIDs.h:170-178
- **Why MCP cannot:** None of these variables is in generated_tools.json (grep count 0 for each), so wfs_set_parameter's registry (MCPParameterRegistry.h:16, parsed from generated_tools.json) cannot reach them either. They are not in Documentation/WFS-UI_config.csv or tool_generation_ignores.json. The generated Orbit help text (WFS-UI_config.csv, binauralListenerAngle row) tells the AI 'To turn the head use binauralListenerYaw', which no tool can write. They are only readable through session_get_global_state's binaural section.
- **Suggested:** Add CSV rows so codegen produces system_binaural_renderer_set_render_mode / set_sofa_file / set_listener_x / set_listener_height / set_head_radius / set_listener_yaw / pitch / roll / set_head_tracker_source

### Per-input binaural solo cannot be set or cleared

*Low impact - parameter - area: inputs-sampler-stereo*

- **Capability:** The Inputs-tab Solo button, the 'S' buttons in the Level Meter window, and Clear Solo toggle or clear Channel.inputSolo. That chooses which inputs are rendered binaurally to the headphone output, together with binauralSoloMode Single/Multi.
- **Where:** Source/gui/InputsTab.h:260-266 (clearAllSoloStates) and 7916-7921 (toggleSolo -> setInputSoloed); Source/gui/LevelMeterWindow.h:499-509; Source/DSP/BinauralCalculationEngine.h:263-268
- **Why MCP cannot:** inputSolo is not in generated_tools.json, is not in the registry, and has no CSV row. There is also no OSC address: grep 'solo' in OSCMessageRouter.cpp finds only effectSolo at line 290. MCP can set binauralSoloMode (system_binaural_renderer_set_binaural_solo_mode) but cannot solo any input, so that tool does nothing useful. The value can be read through session_get_channel_full. This may be intentional: effectSolo is excluded as 'Transient monitoring state' in tool_generation_ignores.json, and MainComponent.cpp:6659 keeps inputSolo out of snapshots. But no MCP exclusion is recorded for inputSolo.
- **Suggested:** input_set_binaural_solo(input_id, on) + input_clear_binaural_solo(), or an explicit ignore entry

### Remote pad grid layout (3x2 / 5x3) cannot be set

*Low impact - parameter - area: inputs-sampler-stereo*

- **Capability:** System Config > Controllers > Remote Pad Setup opens an overlay that picks the Remote XY-pad grid layout. It writes RemotePadGridLayout and resends the pad configuration to the tablet.
- **Where:** Source/gui/SystemConfigTab.h:3478 and 3592-3599; Documentation/WFS-UI_config.csv:44; Source/MainComponent.cpp:14327-14345
- **Why MCP cannot:** WFS-UI_config.csv:44 has no Variable, so no tool is generated. grep for RemotePadGridLayout in Source/Network/MCP and tools/mcp finds nothing, and it is not in the registry.
- **Suggested:** system_controllers_set_remote_pad_layout(layout: '3x2'|'5x3') that also pushes the pad config

### GPU pipeline depth not addressable

*Low impact - parameter - area: system-config*

- **Capability:** On GPU-native builds, choose the GPU async pipeline depth (1-8 blocks). It is stored as config param GpuPipelineDepth and applied live.
- **Where:** Source/gui/SystemConfigTab.h:589-605 (gpuDepthSelector -> setConfigParam("GpuPipelineDepth")), 2786-2791
- **Why MCP cannot:** GpuPipelineDepth is in neither generated_tools.json (grep count 0) nor WFS-UI_config.csv nor tool_generation_ignores.json.
- **Suggested:** Add a CSV row -> system_wfs_processor_set_gpu_pipeline_depth


## Group 3 - actions MCP cannot trigger

**Status: fixed (2026-11-24, branch `mcp/coverage-gaps`) within the Decisions above.** Built:
`snapshot_store` / `snapshot_update` / `snapshot_delete` (through the Snapshot row's own session),
`snapshot_get_scope` / `snapshot_set_scope` (items, channels, apply mode, MIDI trigger with conflict
check), `snapshot_export_cues` and `input_sampler_export_set_cue` (QLab / Go.dot: the operator fires
them), `cluster_lfo_preset_store` / `cluster_lfo_preset_get`, `cluster_transform` (the Clusters tab's own
transform code), `input_gradient_shapes_list` / `input_gradient_shape_add` /
`input_gradient_shape_arrange`, `stage_scale_inputs`, `channels_shift`, `effect_reset_return_positions`,
`patch_get` / `patch_set` (stopped-only, latches channel numbers first), `output_apply_array_geometry`
(the Wizard of OutZ's geometry). Not built, by decision: every recall, transport and trigger, the config
reloads and import, test signals, Set Zero.

### AutomOtion transport for effect returns (Start / Stop / Stop All / Pause-Resume All)

*High impact - action - area: effects-remaining*

- **Capability:** The Effects Movements panel starts and stops one effect's AutomOtion movement and stops or pauses/resumes every effect movement at once. OSC does the same with /wfs/effect/otomoStart <id>, otomoStop <id>, otomoStopAll and otomoPauseResumeAll [0|1]. Only the per-channel pause latch can be reached over MCP (effect_automotion_set_otomo_pause_resume; AutomOtionProcessor.h:1285-1305 turns a write to the latch into a pause or resume). In Manual trigger mode there is no MCP way to start a movement, so the destination, duration and curve tools set up a movement that never runs. knowledge_effects.md:47 still recommends AutomOtion to the AI.
- **Where:** Source/gui/effects/EffectsMovementsPanel.h:746-783 (startMotion / stopMotion / stopAllMotion / pauseAllMotion / resumeAllMotion); Source/Network/OSCManager.cpp:2302-2306 and 2376-2384 (effect otomo transport verbs -> onOtomoTransport); Documentation/WFS-UI_effects.csv:65-69
- **Why MCP cannot:** No registerTool in Source/Network/MCP/MCPServer.cpp touches the motion processor (45 registrations, none for otomo). grep -i 'otomo|startMotion|onOtomoTransport' over Source/Network/MCP (excluding generated_tools.json) only finds comments in EffectTools.h:176,275 and StateInspectionTools.h:306. The generated effect_automotion_* tools write only stored tree properties.
- **Suggested:** effect_otomo_transport {effect_id?, action: start|stop|stop_all|pause_all|resume_all}, routed like OSCManager::onOtomoTransport(effects=true, ...). It could also serve inputs.

### AutomOtion transport: Start, Stop, Stop All, Pause/Resume All

*High impact - action - area: inputs-core*

- **Capability:** The operator starts or stops a programmed movement for an input's cluster, stops every movement in the show, and pauses or resumes all movements together. OSC does the same with /wfs/input/otomoStart <ID>, otomoStop <ID>, otomoStopAll and otomoPauseResumeAll [0|1]. Only the per-channel pause works over MCP, because a write to the inputOtomoPauseResume latch is acted on.
- **Where:** Source/gui/InputsTab.h:3016-3060 (otomoStartButton -> startClusterMotion, otomoStopButton -> stopClusterMotion, otomoStopAllButton -> stopAllMotion, otomoPauseResumeAllButton); Source/Network/OSCManager.cpp:2291-2386 (handleOtomoTransport verbs); Source/MainComponent.cpp:2846-2891 (onOtomoTransport dispatch); Documentation/WFS-UI_input.csv:108,111,112
- **Why MCP cannot:** Every MCP registration is in MCPServer.cpp:107-226, and none of them is a motion tool. grep -i 'otomo|Motion' over Source/Network/MCP/*.{h,cpp} and tools/*.h finds only the generic gradient/loader code, with no startMotion/stopAllMotion call. The only generated transport tool is input_automotion_set_otomo_pause_resume (the latch: AutomOtionProcessor.h:1285-1306). There is no start or stop property: a Manual-trigger movement can only begin through processor calls. The running state (isMotionActive/getProgress, AutomOtionProcessor.h:851-900) cannot be read over MCP either.
- **Suggested:** input_otomo_transport(input_id?, action: start|stop|pause|resume|stop_all|pause_all|resume_all) plus the motion state (active, paused, progress) in session_get_channel_full

### Snapshot recall (load) not available over MCP

*High impact - action - area: system-config*

- **Capability:** Recall a stored input/effects snapshot by name, applying its scope, through MainComponent::recallSnapshotByName. The GUI does it with Reload (scoped) and Reload w/o Scope, OSC with /wfs/input/snapshot/load <name>, and MIDI notes trigger it too.
- **Where:** Source/Network/OSCManager.cpp:2680-2684 (/wfs/input/snapshot/load -> onSnapshotLoadRequested); Source/MainComponent.cpp:2802-2804 (recallSnapshotByName); Source/gui/snapshots/SnapshotRow.h:60,65 (reload / reloadWithoutScope long-press); Source/gui/snapshots/SnapshotSession.h:276-300
- **Why MCP cannot:** The only snapshot tool is snapshot_list (Source/Network/MCP/tools/SnapshotTools.h:39-50, registered at MCPServer.cpp:135) and it only reads. Its description even says 'Use before snapshot.load', but no such tool is registered (grep registerTool in MCPServer.cpp:107-226). No generated tool covers snapshots.
- **Suggested:** snapshot_recall {name, ignore_scope?: bool} routed through recallSnapshotByName; also fix the snapshot_list description that points at a snapshot.load tool that does not exist

### Snapshot store / update / delete not available over MCP

*High impact - action - area: system-config*

- **Capability:** Store a new named snapshot (scope defaults or reuses the existing one), update the selected snapshot in place, and delete a snapshot. The GUI has long-press Store/Update/Delete; OSC has /wfs/input/snapshot/store <name>.
- **Where:** Source/gui/snapshots/SnapshotRow.h:42,70,80; Source/gui/snapshots/SnapshotSession.h:210-248 (store), 338-384 (update via saveInputSnapshotWithExtendedScope), 386-410 (remove via deleteInputSnapshot); Source/Network/OSCManager.cpp:2685-2689; Source/MainComponent.cpp:2806-2815
- **Why MCP cannot:** SnapshotTools.h holds only snapshot_list (lines 39-50). No tool calls WFSFileManager::saveInputSnapshotWithExtendedScope or deleteInputSnapshot (grep in Source/Network/MCP shows no hits).
- **Suggested:** snapshot_store {name, overwrite?}, snapshot_update {name}, snapshot_delete {name} (tier 2)

### Cluster LFO preset recall

*Medium impact - action - area: clusters*

- **Capability:** Recall one of the 16 global cluster-LFO preset slots into a cluster. This copies 22 properties and applies the shapes last, so the shape-change fade dip uses the new amplitudes. In the GUI, a double-click on a tile also activates the LFO. Over OSC: /wfs/cluster/lfoPresetRecall <cluster> <preset> and the OSCQuery short form /wfs/cluster/<n>/lfoPresetRecall <preset>. QLab and Go.dot cues are built around this OSC message.
- **Where:** Source/Network/OSCManager.cpp:4406-4427 and 4456-4471 (both call state.recallClusterLFOPreset); Source/Parameters/WFSValueTreeState.cpp:559 recallClusterLFOPreset; Source/gui/ClustersTab.h:1577-1615 recallPreset, :801 sdRecallPreset (Stream Deck); Documentation/WFS-UI_clusters.csv 'Preset Tiles' row
- **Why MCP cannot:** grep 'recallClusterLFOPreset' finds no hits under Source/Network/MCP. The only preset-related generated tool is cluster_lfo_presets_set_lfo_preset_name, which writes the name only, although its description says 'Recall, activate, store, or rename'. Emulating a recall needs the preset's contents, and MCP cannot read them (see the next gap).
- **Suggested:** cluster_lfo_preset_recall(cluster_id 1-10, preset 1-16, activate?: bool)

### Cluster LFO preset store and preset bank contents (read/write)

*Medium impact - action - area: clusters*

- **Capability:** Store the current cluster's LFO state (period, phase, and shape/rate/amplitude/phase for X, Y, Z, Rot and Scale) into one of the 16 global preset slots. GUI: middle- or right-click on a tile. Stream Deck: sdStorePreset. The preset bank lives in Config > ClusterLFOPresets and is show state that is saved with the project.
- **Where:** Source/gui/ClustersTab.h:1558-1575 storePreset, :800 sdStorePreset; Source/Parameters/WFSValueTreeState.cpp:510-551 getClusterLFOPresetsSection / ensureClusterLFOPreset
- **Why MCP cannot:** session_get_global_state's 'clusters' section is getClustersState() = Config > Clusters only (Source/Network/MCP/tools/StateInspectionTools.h:158; WFSValueTreeState.cpp:441-444), so the ClusterLFOPresets sibling is never returned. wfs_set_parameter/wfs_get_parameter on clusterLFOperiod, clusterLFOshape*, etc. resolve to the live cluster's ClusterLFO node, by design (WFSValueTreeState.cpp:7477-7504: 'resolve the live cluster first, or a period write would land in a preset slot'). No property of a preset slot other than its name can be reached, and there is no store tool.
- **Suggested:** cluster_lfo_preset_store(cluster_id, preset 1-16, name?) + cluster_lfo_preset_get(preset?) (or add a 'cluster_lfo_presets' section to session_get_global_state)

### Cluster rigid transforms (move / rotate / scale / relative attenuation about the reference point)

*Medium impact - action - area: clusters*

- **Capability:** Move, rotate or scale every member of a cluster as one rigid group, in the selected plane (XY/XZ/YZ), around the cluster reference point (first input / barycenter, or the tracked member when one is fully tracked). A tracked member is moved through its offset rather than its position, stage and distance constraints are applied, and a relative dB change goes to every member's attenuation. Over OSC: /cluster/move, /cluster/barycenter/move, /cluster/positionXY (absolute ref target), /cluster/scale, /cluster/rotation, /cluster/scaleRotation (cumulative gesture). Shared-Position clusters are handled by writing a single member and letting propagation spread it.
- **Where:** Source/gui/ClustersTab.h:2720-2900 applyPositionDelta / applyAttenuationDelta / applyRotationDelta / applyScaleDelta; Source/Network/OSCMessageRouter.cpp:996-1010; Source/Network/OSCManager.cpp:1952-1961, 3910-3960 (shared-mode comment: a per-member delta loop would compound to target + N*delta), 4073+, 4265+
- **Why MCP cannot:** No hand-written tool in Source/Network/MCP/tools/*.h mentions cluster transforms (grep -i cluster there finds only state-inspection and id comments), and generated_tools.json has no cluster position/rotation/scale tool. An AI can only rewrite member inputPositionX/Y/Z one at a time. It would have to recompute the reference point itself, including the tracked-input priority from trackingEnabled, trackingProtocol and inputTrackingActive, and the offset-versus-position rule. In Shared Position mode, a per-member batch write compounds through setInputParameter's propagation (WFSValueTreeState.cpp:846-850).
- **Suggested:** cluster_transform(cluster_id, translate:{dx,dy,dz}? | move_reference_to:{x,y}? , rotate_deg?, scale?, plane: XY|XZ|YZ, attenuation_delta_db?) reusing the OSCManager cluster-move/scale-rotation code paths

### Snapshot recall / store / update (which now carry the effects channels)

*Medium impact - action - area: effects-remaining*

- **Capability:** The shared snapshot row (on both the Inputs and Effects tabs) stores, reloads with or without scope, updates and deletes snapshots that hold both the inputs and the effects. OSC recalls and stores them with /wfs/input/snapshot/load and /store. MCP can only list the names. snapshot_list's own description tells the AI to 'use before snapshot.load', but no such tool exists.
- **Where:** Documentation/WFS-UI_effects.csv:231-237; Source/Network/OSCManager.cpp:2655-2685 (/wfs/input/snapshot/load, /store)
- **Why MCP cannot:** Source/Network/MCP/tools/SnapshotTools.h:37-40 registers only snapshot_list and points to a nonexistent snapshot.load. MCPServer.cpp registers only Snapshot::describe (fileMgr).
- **Suggested:** snapshot_load {name, ignore_scope?} / snapshot_store {name} / snapshot_update {name} (tier 2-3), using the same path as the OSC handler.

### Gradient-map shapes cannot be created, deleted, enabled, reordered or reshaped

*Medium impact - structural - area: inputs-core*

- **Capability:** In the gradient-map editor the operator draws new rectangle, ellipse and polygon shapes, deletes them, duplicates them, copies and pastes a layer, sends a shape to back or front, toggles a shape on or off, and edits polygon vertices and gradient-fill control points. Each of these changes the rasterised map that drives the input's attenuation, height and HF.
- **Where:** Source/GradientMap/GradientMapEditor.h:1643,1686 (shapes.push_back on draw), 375 and 2911-2924 (deleteSelectedShapes), 383-386 (To Back/To Front), 407-409 (copy/paste layer), 3206 (shape.enabled); Source/GradientMap/GradientMapEvaluator.h:107-112 (disabled shapes are skipped when rasterising); Source/GradientMap/GradientMapData.h:292-296 (sorted by gmShapeOrder for painting), 143-163, 193-207 (gmShapeVertices / gmShapeFillParams)
- **Why MCP cannot:** The shape tools only edit existing children. MCPGeneratedToolLoader.cpp:146-153 returns 'A layer starts with no shapes; they are created in the gradient-map editor', and no hand-written tool in MCPServer.cpp:107-226 creates or deletes shapes. gmShapeVertices, gmShapeFillParams and gmLayerEnabled have no CSV row and no generated tool. gmShapeEnabled and gmShapeOrder are ignored as 'UI-state only' (tools/mcp/tool_generation_ignores.json:3-14), but the evaluator does use them for audio (GradientMapEvaluator.h:109, GradientMapData.h:295), so that reason is wrong. On a fresh project every gm_shape tool can only fail.
- **Suggested:** gradient_shape_add/delete/reorder(input_id, layer, type, pos, size, vertices?, fill?) and gradient_layer_copy; generated tools for gmShapeEnabled/gmShapeOrder with the ignore entries corrected

### Cluster LFO preset recall (and stop-all) not available

*Medium impact - action - area: map-and-remote*

- **Capability:** Recall one of the 16 stored cluster LFO presets into a cluster's LFO. This writes the 22 period/rate/amplitude/phase/shape values with shapes last, so the fade dip runs. Also stop every cluster LFO at once. Used by the tablet (lfoPresetRecall, lfoStopAll), Go.dot cues and OSCQuery /wfs/cluster/<n>/lfoPresetRecall.
- **Where:** Source/Network/OSCManager.cpp:4392-4404 (lfoStopAll), :4408-4428 (short form /wfs/cluster/<n>/lfoPresetRecall), :4454-4469 (/wfs/cluster/lfoPresetRecall); Source/Parameters/WFSValueTreeState.cpp:559 (recallClusterLFOPreset); Documentation/CLAUDE.md:2246 (OSCQuery node for Go.dot); tablet D:/dev/WFS_DIY_remote app/src/main/java/com/wfsdiy/wfs_control_2/OSCfunctions.kt:2179,2250
- **Why MCP cannot:** grep -rni 'lfopreset|PresetRecall|recallClusterLFOPreset' Source/Network/MCP (excluding generated_tools.json) finds nothing. The only preset-related generated tool is cluster_lfo_presets_set_lfo_preset_name (generated_tools.json:113-114). Preset contents (the ClusterLFOPresets section) cannot be read either, so an agent cannot rebuild a recall by hand.
- **Suggested:** cluster_lfo_recall_preset {cluster_id, preset 1-16} (+ optional cluster_lfo_stop_all), plus preset contents in session_get_global_state

### Cluster move / scale / rotate as one operation

*Medium impact - action - area: map-and-remote*

- **Capability:** Move a whole cluster by its reference or barycenter, scale it, or rotate it. Each is one gesture that follows the cluster's reference mode: first input or barycenter moves member positions; a tracked reference moves its offset while the members move; Shared Position (mode 2) scales or rotates the inputOffsetX/Y vectors; results are clamped to the stage. Reachable by Map drag and two-finger pinch/rotate on a barycenter, Stream Deck Map dials, and the tablet's /cluster/move, /cluster/barycenter/move, /cluster/positionXY, /cluster/scale, /cluster/rotation and /cluster/scaleRotation.
- **Where:** Source/gui/MapTab.h:2493 (applyClusterScale), :2567 (applyClusterRotation), :3508 (moveClusterRelative), :3541 (moveClusterWithReference); Source/Controllers/DialsAndButtons/pages/MapTabPages.h:588,618,645,668; Source/Network/OSCMessageRouter.cpp:996-1010; Source/Network/OSCManager.cpp:3910 (handleClusterMoveMessage), :3934, :4073, :4228 (cumulative scale+rotation)
- **Why MCP cannot:** No cluster transform tool. Every registerTool in Source/Network/MCP/MCPServer.cpp:107-226 was checked and none is cluster-level. The generated cluster_* tools cover only LFO params, clusterReferenceMode, clusterInputOrder and clusterInputsVisible. An agent would have to re-implement the reference-mode, tracked-offset, shared-offset and stage-clamp logic one input at a time, in separate undo transactions. Writing the reference input's position over MCP does not carry the members in modes 0/1 (MapTab.h:3487-3500 shows that propagation only happens for mode 2).
- **Suggested:** cluster_transform {cluster_id, dx?, dy?, dz?, scale?, rotation_deg?} running the same code as MapTab/applyPendingClusterMove in one undo transaction

### Gradient-map shapes cannot be created, deleted, reordered, or pasted from Map outlines

*Medium impact - structural - area: map-and-remote*

- **Capability:** In the gradient-map editor the operator can add shapes, delete them (long-press), reorder them (To Back / To Front), enable or disable them, edit polygon vertices and gradient fill points, and paste outlines drawn on the Map into a layer in place. The Map shape editor's Ctrl+C plus the editor's Paste from Map does this. Order and enabled are not cosmetic: the evaluator paints enabled shapes in order (painter's algorithm), so they change the rasterised map that drives the audio.
- **Where:** Source/GradientMap/GradientMapEditor.h:375 (delete long-press), :384-387 and :2938 (reorderSelectedShapes), :2911 (deleteSelectedShapes), :3308-3345 (onPaste, Kind::MapShapes -> mapShapesAsGradient); Source/gui/MapShapesEditor.h:177 and :671 (copySelection to the shared ShapeClipboard); Source/GradientMap/GradientMapEvaluator.h:106-112 (skips !enabled, paints in order)
- **Why MCP cannot:** The generated gm_shape tools only edit existing shapes. Their schema says 'A layer starts with no shapes; they are created in the gradient-map editor' (generated_tools.json, input_gradient_map_set_gm_shape_*), and MCPGeneratedToolLoader.cpp:146-153 refuses a missing shape. There is no tool for gmShapeVertices or gmShapeFillParams (WFSParameterIDs.h:601,604; no CSV rows). gmShapeOrder and gmShapeEnabled are excluded in tools/mcp/tool_generation_ignores.json:8,12 as 'UI-state only', but the evaluator above shows they change the output. The map_shape* tools (MapShapeTools.h) write only <MapShapes>, never a gradient layer.
- **Suggested:** gradient_shape_add/delete/reorder {input_id, layer, type, geometry|points, fill...} plus gradient_shapes_from_map {input_id, layer, map_shape_indices}; restate the ignore reasons for gmShapeOrder/gmShapeEnabled

### Wizard of OutZ (array position helper) has no MCP tool

*Medium impact - action - area: outputs*

- **Capability:** The Array Position Helper works out speaker positions and orientations from a preset (Near Field Straight/Curved, Main Flown, Sub Bass, Surround, Delay Line, Circle) and a geometry method (Center+Spacing, Endpoints, Curved, Circle, Surround Pairs). In one undo transaction it writes, to a contiguous range of outputs: X/Y/Z, orientation, HF damping, array assignment, LS/FR enable, H/V parallax and distance atten % (with the Sub Bass 50 % rule for N<=2). It can also enable the EQ with a Low Cut on band 1 and a High Cut on band 6.
- **Where:** Source/gui/OutputArrayHelperWindow.cpp:1538-1660 (applyToOutputs), geometry in Source/Helpers/ArrayGeometryCalculator.cpp; Documentation/WFS-UI_arrayWizard.md; the tool was specified as output.apply_array_preset in Documentation/MCP/specs/MCP_TOOL_SURFACE.md:83 and Documentation/MCP/specs/GENERATION_SCRIPT_SPEC.md:29
- **Why MCP cannot:** No array-preset or geometry tool is registered in Source/Network/MCP/MCPServer.cpp:107-226; grep for ArrayGeometryCalculator in Source/Network/MCP finds nothing. An AI can only rebuild the result by recomputing the wizard geometry itself and issuing about 12 writes per speaker through wfs_set_parameter_batch / output_position_set_cartesian.
- **Suggested:** output_apply_array_preset(array_id, start_output, count, preset, geometry_method, geometry args, acoustic overrides, low/high cut) - tier 2, reusing ArrayGeometryCalculator and one undo transaction

### Reload reverb configuration / reload reverb config backup

*Medium impact - action - area: reverbs*

- **Capability:** Store/Reload footer long-press buttons. 'Reload Reverb Configuration' reloads reverbs.xml from the project folder, and 'Reload Reverb Config. Backup' reloads the newest backup. Both then trigger the DSP recalculation (onConfigReloaded).
- **Where:** Source/gui/ReverbTab.h:2015 and 2020 (buttons), 4659-4706 (reloadReverbConfiguration / reloadReverbConfigBackup -> WFSFileManager::loadReverbConfig / loadReverbConfigBackup(0), Source/Parameters/WFSFileManager.h:250,253)
- **Why MCP cannot:** No MCP tool calls loadReverbConfig or loadReverbConfigBackup: grep -i 'reload|loadReverb|Backup' over Source/Network/MCP finds only the session_save description. The only file tool is session_save (SessionTools.h:144-190), which writes and never reads. Store Reverb Configuration is covered, as a superset, by session_save.
- **Suggested:** reverb_config_reload(source: "file"|"backup"), tier 2. It calls WFSFileManager::loadReverbConfig / loadReverbConfigBackup(0), then the same DSP-recalc hook as onConfigReloaded.

### Audio patch (input/output hardware routing) neither readable nor writable

*Medium impact - structural - area: system-config*

- **Capability:** The Audio Interface and Patching window edits the InputPatch/OutputPatch matrices (which hardware input feeds each input channel, which hardware output each output channel drives). These are saved with the complete configuration.
- **Where:** Source/gui/AudioPatchTab.h:61-104 (InputPatchTab), 120-175 (OutputPatchTab); Source/gui/PatchMatrixShim.cpp:17-45 (patchTree/patchData); Source/Parameters/WFSValueTreeState.cpp:714 (getAudioPatchState), 4037-4060
- **Why MCP cannot:** No generated tool has a patch variable (only network_set_t_sqlab_patch, which is unrelated). No hand-written tool touches AudioPatch (grep 'patch' in Source/Network/MCP/tools only hits comments in ChannelLifecycleTools.h). session_get_global_state's section list (StateInspectionTools.h:70-75, 145-154) has no AudioPatch section.
- **Suggested:** patch_get {kind: input|output} / patch_set {kind, channel, hardware_channel(s)}

### QLab / Go.dot snapshot cue export

*Medium impact - action - area: system-config*

- **Capability:** After a store or update in Write-to-QLab mode, export the snapshot as QLab and/or Go.dot cues, or create a cue that loads the snapshot over OSC. The writeToQLab toggle persists in the show section.
- **Where:** Source/gui/snapshots/SnapshotSession.h:79-90 (onQLabExportRequested / onQLabSnapshotLoadCueRequested), 366-382; Source/MainComponent.cpp:598, 697
- **Why MCP cannot:** No tool in Source/Network/MCP/tools/*.h calls the QLab or Go.dot export (grep QLab in Source/Network/MCP/tools shows no hits). writeToQLab is not a generated-tool variable.
- **Suggested:** snapshot_export_cues {name, target: qlab|godot|both, load_cue_only?}

### Reload complete / system configuration (and their backups) and system config import/export

*Medium impact - action - area: system-config*

- **Capability:** Long-press Reload Complete Configuration, Reload Complete Config Backup, Store System Configuration, Reload System Configuration, Reload System Config Backup, and Import/Export System Configuration (with a file picker).
- **Where:** Source/gui/SystemConfigTab.h:1364-1394; Documentation/WFS-UI_config.csv Files rows
- **Why MCP cannot:** session_save (SessionTools.h:146-197) covers only Store Complete Configuration (saveCompleteConfig). No tool reloads from disk or backup, or stores the system config alone. ProjectFolder is the only Files row in tool_generation_ignores.json, so the reloads are not documented as excluded. Import/export need a path, which MCP can pass as a string.
- **Suggested:** session_reload {scope: complete|system, from_backup?: bool} (tier 3); optionally session_export_system {path} / session_import_system {path}

### Snapshot scope and MIDI note binding: cannot be read or edited

*Medium impact - structural - area: system-config*

- **Capability:** Edit a snapshot's extended scope (which inputs and effects, and which parameter families, it includes) and its MIDI channel/note trigger binding in the Scope window. Write the result with updateInputSnapshotScope.
- **Where:** Source/gui/snapshots/SnapshotRow.h:75 (editScope long-press); Source/gui/snapshots/SnapshotSession.h:414-560 (scope window, midiChannel/midiNote, clearMidiBinding); Source/Parameters/WFSFileManager.h:592 (updateInputSnapshotScope), 917 (MIDI binding on the <InputSnapshot> root)
- **Why MCP cannot:** No MCP tool reads getExtendedSnapshotScope or writes updateInputSnapshotScope or the MIDI binding. snapshot_list returns names only (SnapshotTools.h:21-32).
- **Suggested:** snapshot_get_scope {name} / snapshot_set_scope {name, scope, midi_channel?, midi_note?}

### Reload Effects Configuration / Reload Effects Config Backup

*Low impact - action - area: effects-remaining*

- **Capability:** Long-press buttons in the Effects tab reload the effects configuration from the project's effects file or from its backup (WFSFileManager::loadEffectsConfig / loadEffectsConfigBackup), with dirty-tracking suppressed. Storing is covered by session_save (saveCompleteConfig includes getEffectsConfigFile), but nothing over MCP reverts the effects to the saved or backed-up file.
- **Where:** Source/gui/effects/EffectsTab.h:686-696 and 748-780; Source/Parameters/WFSFileManager.h:294-300
- **Why MCP cannot:** No MCP tool calls loadEffectsConfig/loadEffectsConfigBackup (grep over Source/Network/MCP finds no hit). The MCPServer.cpp registrations offer only session_save among file operations.
- **Suggested:** session_reload_config {section: effects|..., from_backup?: int} (tier 3, confirm), shared with the other tabs' reloads.

### Reset Return Positions (re-run automatic ring placement of effect returns)

*Low impact - action - area: effects-remaining*

- **Capability:** A long press in Effects Settings re-runs the automatic ring placement over every effect channel (redistributeAllEffectPositions) and clears effectPositionsUserOwned, so later channel-count changes re-lay the ring. The Stream Deck has the same action (fxCB.onRelayout). MCP can only write each position by hand, and it cannot clear the user-owned latch, so after an MCP position edit the automatic placement stays off for good.
- **Where:** Source/gui/effects/EffectsSettingsPanel.h:75-82; Source/MainComponent.cpp:1391-1398 (Stream Deck onRelayout); Documentation/WFS-UI_effects.csv:8
- **Why MCP cannot:** grep 'redistributeAllEffectPositions|effectPositionsUserOwned' finds no hit under Source/Network. reverb_auto_layout (Source/Network/MCP/tools/ReverbAutoLayoutTool.h) handles reverbs only (no 'effect' reference in the file). effectPositionsUserOwned is not a generated tool.
- **Suggested:** effect_auto_layout (tier 2): redistributeAllEffectPositions + effectPositionsUserOwned=0, recorded for undo like reverb_auto_layout.

### Reload input configuration / reload input backup

*Low impact - action - area: inputs-core*

- **Capability:** Long-pressing Reload on the Inputs tab reloads inputs.xml from the project folder (after a channel-identity gate). Long-pressing Reload Backup restores the latest input-config backup. Neither needs a file picker.
- **Where:** Source/gui/InputsTab.h:369-374 (onLongPress), 6774-6830 (reloadInputConfiguration -> fm.loadInputConfig(), reloadInputConfigBackup -> fm.loadInputConfigBackup(0))
- **Why MCP cannot:** grep 'loadInputConfig|loadCompleteConfig|reload' over Source/Network/MCP finds no tool that calls them. The only file tools are session_save (SessionTools.h:144-160, save only) and snapshot_list (SnapshotTools.h:19-41, list only).
- **Suggested:** session_reload(scope: inputs|all, from_backup?: bool)

### No way to trigger sampler playback

*Low impact - action - area: inputs-sampler-stereo*

- **Capability:** Over OSC, the tablet's XY pads trigger the next cell of the input's active set, with pressure and XY deflection, and release it, through /remote/pad/touch. The Lightpad does the same.
- **Where:** Source/Network/OSCManager.cpp:2109-2119; Source/MainComponent.cpp:2961-2990 (onRemotePadTouch) and 2200-2213 (triggerNextCell / releaseChannel)
- **Why MCP cannot:** No MCP tool calls SamplerManager::triggerNextCell or releaseChannel. grep for 'trigger' or 'sampler' in Source/Network/MCP/tools/*.h finds only the set create and delete tools (ChannelLifecycleTools.h:723-826).
- **Suggested:** input_sampler_trigger(input_id, pressure=1.0) / input_sampler_release(input_id)

### QLab cue for a sampler set cannot be created

*Low impact - action - area: inputs-sampler-stereo*

- **Capability:** The 'Q' button on the Sampler sub-tab creates a QLab cue that recalls the selected set (/wfs/input/samplerSet <channel> <set>) on the configured QLab target.
- **Where:** Source/gui/SamplerSubTab.h:154-167; Source/MainComponent.cpp:819 (onQLabSetCueRequested handler); Documentation/WFS-UI_input.csv:182
- **Why MCP cannot:** No hand-written tool in Source/Network/MCP/tools/*.h creates QLab cues. Grep for 'qlab' there only matches generated config parameters and ChannelLifecycleTools comments.
- **Suggested:** input_sampler_export_set_qlab_cue(input_id, set_id)

### Reload Network Config / Reload Backup has no MCP equivalent

*Low impact - action - area: network*

- **Capability:** Long-press buttons that restore the Network section from the project's saved network file, or from its backup, then refresh the tab and the OSC engine.
- **Where:** Source/gui/NetworkTab.h:1853-1861 (reloadButton/reloadBackupButton onLongPress); Source/gui/NetworkTab.h:5598-5640 (reloadNetworkConfiguration / reloadNetworkConfigBackup via fileManager.getNetworkConfigFile)
- **Why MCP cannot:** No MCP tool reloads any config section. A grep for reload/load in Source/Network/MCP/tools/*.h finds no such tool, and session_save (SessionTools.h:146-183) only writes. Store is covered by session_save (saveCompleteConfig includes network).
- **Suggested:** network_config_reload { source: 'saved' | 'backup' } (tier 2), calling the same WFSFileManager path and then NetworkTab::refreshFromValueTree.

### Reload Output Configuration / Reload Output Config Backup not reachable

*Low impact - action - area: outputs*

- **Capability:** Long-press buttons on the Outputs tab reload outputs.xml from the project folder, or from its rolling backup, replacing the whole output configuration and triggering the DSP recalculation (onConfigReloaded). Import/Export Output Configuration do the same with a file chosen in a picker.
- **Where:** Source/gui/OutputsTab.h:266-281 (long-press bindings), 2431-2466 (reloadOutputConfiguration / reloadOutputConfigBackup calling WFSFileManager::loadOutputConfig / loadOutputConfigBackup(0)); Source/Parameters/WFSFileManager.h:231-240; Documentation/WFS-UI_output.csv Store/Reload rows
- **Why MCP cannot:** The only file tool over MCP is session_save (Source/Network/MCP/tools/SessionTools.h:179-200, saveCompleteConfig), which covers Store. No MCP tool calls loadOutputConfig, loadOutputConfigBackup, importOutputConfig or exportOutputConfig (grep 'OutputConfig' in Source/Network/MCP returns nothing). The design spec lists configuration reload as a tier-2 MCP action (Documentation/MCP/specs/MCP_SERVER_DESIGN.md:67, 172).
- **Suggested:** output_config_reload(source: 'project' | 'backup', backup_index) - tier 2 with the confirm handshake, plus a DSP recalculation; import/export optional with a path argument

### Head-tracker Set Zero calibration

*Low impact - action - area: system-config*

- **Capability:** 'Set zero while looking at the stage' calibrates the active head tracker (camera or USB dongle) for the binaural renderer.
- **Where:** Source/gui/SystemConfigTab.h:1074-1077 (binauralSetZeroButton -> onHeadTrackerSetZero); Source/MainComponent.cpp:2335 (setHeadTrackerSetZeroCallback)
- **Why MCP cannot:** No MCP tool reaches HeadTrackerManager or a set-zero callback (grep HeadTracker in Source/Network/MCP shows no hits).
- **Suggested:** binaural_head_tracker_set_zero

### Output test signals (speaker identification) not available

*Low impact - action - area: system-config*

- **Capability:** In the Audio Interface window's output tab, switch to Testing mode, pick a signal type and hold, and play it to individual outputs to identify speakers.
- **Where:** Source/gui/AudioPatchTab.cpp:365 (testingButton), 390 (signalTypeCombo), 419 (holdButton); Source/gui/AudioPatchTab.h:157-183
- **Why MCP cannot:** No MCP tool references TestSignalGenerator (grep in Source/Network/MCP shows no hits).
- **Suggested:** output_test_signal {output, signal_type, level_db, hold?} / output_test_signal_stop

### Stage Scale/Fit and Origin Shift follow-up actions

*Low impact - action - area: system-config*

- **Capability:** After stage dimensions change: long-press Scale (scale all input positions proportionally) or Fit (pull outlier inputs back inside the stage). After an origin change: Shift Inputs / Outputs / Reverbs by the origin delta.
- **Where:** Source/gui/SystemConfigTab.h:687-701 (stageScale/Fit/Dismiss), 704-737 (origin shift and dismiss buttons); Documentation/WFS-UI_config.csv Stage rows 'Stage Scale', 'Stage Fit', 'Origin Shift *'
- **Why MCP cannot:** No MCP tool performs scale, fit or shift (grep scale/fit/shift in Source/Network/MCP/tools shows nothing). system_stage_set_* and system_stage_set_origin_* only write the value, so an AI must read every position and rewrite them by batch, with no shared undo transaction or delta bookkeeping.
- **Suggested:** stage_rescale_inputs {mode: scale|fit}, origin_shift_channels {kinds: [input, output, reverb], delta?}


## Group 4 - state MCP cannot read

### Live effect telemetry is unreadable (loop guard tripped, cycle warning, chain latency, module meters)

*Medium impact - read - area: effects-remaining*

- **Capability:** The Effects tab header shows a loop-guard LED (the fx-to-fx feed has been ramped to -inf), a cycle-warning badge (the channel is inside an on-switch cycle), the chain latency and per-module meters, all pushed from the engine. The CSV claims chain latency is surfaced in MCP describeChannelFull. An AI chasing a runaway effect loop (which knowledge_effects.md:73 explains) cannot see whether the guard tripped or which effects form a cycle.
- **Where:** Source/MainComponent.cpp:19352-19377 (isLoopGuardTripped, getEffectCycleMask, getChainLatencySamples -> effectsTab->setLiveState); Documentation/WFS-UI_effects.csv:6-7 and 88
- **Why MCP cannot:** Source/Network/MCP/tools/StateInspectionTools.h:207-284: session_get_channel_full accepts only input/output/reverb and refuses anything else, so effects are rejected despite the CSV:88 claim. Source/Network/MCP/tools/EffectTools.h get() (lines ~160-265) returns only tree properties and sends, with no tripped, cycle or latency fields.
- **Suggested:** Add a 'live' block to effect_get {loop_guard_tripped, in_cycle, chain_latency_samples, fed_by_an_input}, or let session_get_channel_full accept channel_type 'effect'.

### Live (composite) source positions cannot be read

*Low impact - read - area: map-and-remote*

- **Capability:** The Map and the tablet show where each input actually renders: the composite position after offset, input/cluster LFO, tracking, Otomo and the speed limiter. The tablet receives it as /remoteInput/compositeDelta.
- **Where:** Source/MainComponent.cpp:19705-19740 (calculationEngine->getCompositeInputPosition -> sendCompositeDeltaToRemote); Source/Network/OSCManager.cpp:4945-4963; MapTab.h:2672-2719 (LFO/Otomo/speed-limited position callbacks used to draw)
- **Why MCP cannot:** grep -rni 'composite|speedLimit|lfoOffset|isFullyTracked' Source/Network/MCP (excluding generated_tools.json) returns nothing. session_get_state, session_get_channel_full and wfs_get_parameter return only stored target/offset values.
- **Suggested:** input_get_live_positions {input_ids?} -> composite x/y/z per input

### No way to list the project's IR files (or read IR duration) before setting reverbIRfile

*Low impact - read - area: reverbs*

- **Capability:** In IR mode the IR File dropdown lists every file in the project's ir/ folder. Picking one sets reverbIRfile, resets IR Trim to 0, and rescales the Trim/Length ranges to the file's duration.
- **Where:** Source/gui/ReverbTab.h:4403 (refreshIRFileList), 4453-4482 (handleIRFileSelection: writes reverbIRfile, reverbIRtrim=0, updateIRSliderRanges)
- **Why MCP cannot:** reverb_algorithm_set_i_rfile (generated_tools.json) accepts only a plain file name inside ir/ (MCPParameterValidation.h:241-262). No MCP tool or state read lists the ir/ folder or reports an IR's duration: grep 'irfile|ir/|findChildFiles|duration' over Source/Network/MCP/tools finds nothing. An AI has to guess names. The MCP write also leaves the previous file's IR Trim in place, where the GUI resets it to 0.
- **Suggested:** reverb_list_ir_files() returning the file names in ir/ with each file's duration in seconds (read-only).

### Live level meters cannot be read

*Low impact - read - area: system-config*

- **Capability:** The Level Meter window shows live input/output levels (signal present, clipping) for checking a show.
- **Where:** Source/gui/LevelMeterWindow.h
- **Why MCP cannot:** No MCP tool returns meter levels (grep meter in Source/Network/MCP/tools shows only include lines). session_get_state and session_get_global_state return parameters only (SessionTools.h:93-104, StateInspectionTools.h:145-164).
- **Suggested:** session_get_levels {kinds?: [input, output, reverb]} returning peak/RMS dBFS


## From the completeness critic

### completeness: Timed transitions (OSC fade time) have no MCP equivalent

*High impact - parameter* **Intentional - see Decisions.**

- **Capability:** An OSC write to a fade-capable input parameter can carry a third float, a transition time in seconds (clamped to 600 s). OSCParameterRamper then moves the value from its current setting to the target at 50 Hz over that time. This works for input attenuation, common attenuation, arrayAtten1..10, delay/latency, stereo image and positions. Cue systems use it for a source fading or travelling over N seconds.
- **Where:** Source/Network/OSCManager.cpp:2825-2878 (ramp path, parameterRamper.startRamp); Source/Network/OSCMessageRouter.cpp:1444-1470 (isInputParamRampCapable whitelist); Source/Network/OSCParameterRamper.h:15-23

### completeness: Processing (Run DSP) start/stop and WFS algorithm selection cannot be driven over MCP

*Medium impact - action* **Intentional - see Decisions.**

- **Capability:** The operator long-presses Processing in System Config to start or stop the audio engine. toggleProcessing writes ProcessingEnabled and calls onProcessingChanged, which runs MainComponent::handleProcessingChange. When processing is stopped, the operator can also pick the WFS algorithm/device (ProcessingAlgorithm + ProcessingAlgorithmDevice, which calls handleAlgorithmSelectionChange). The Stream Deck can start processing (start only).
- **Where:** Source/gui/SystemConfigTab.h:584-586 (processingButton.onLongPress), 3401-3414 (toggleProcessing), 566-579 (algorithmSelector.onChange); Source/MainComponent.cpp:393-406 (setProcessingCallback / setAlgorithmChangedCallback); Source/MainComponent.cpp:1540-1545 (Stream Deck startProcessing)


## Refuted

- **Array-wide relative adjust (/arrayAdjust deltas) has no MCP route** (map-and-remote): I found no dedicated array-adjust tool, but MCP can already produce the same result, so this is not a capability gap.

What the OSC handler does: OSCManager.cpp:3873-3905 is plain arithmetic over existing state. Inside one undo transaction, for each output whose outputArray matches, it reads the parameter, adds the delta and writes the result with state.setOutputParameter. Two details matter. It writes with the plain setter, not ...WithArrayPropagation. And it has no special semantics beyond read + add + write.

How MCP covers it:
1. wfs_get_parameters (MCPServer.cpp:161) reads outputArray and the target parameter for every output. session_get_state is a second way to read them.
2. wfs_set_parameter_batch (MCPServer.cpp:145) writes the new absolute values for every array member. Its description at SetParameterBatchTool.h:537-545 says: "Apply up to 100 parameter writes atomically and record them as a single undoable entry ... Use this for setting many channels". It has per-entry channel_id (SetParameterBatchTool.h:34-50) and a cap of kMaxBatchSize = 100 at :19.

The result is the same final state as /arrayAdjust/<param> <array> <delta>, in one undo step. The parameters it touches (outputDelayLatency, outputAttenuation, outputHparallax, outputVparallax) are all reachable by variable name.

The absence evidence is wrong on one point: "an agent must list each member and nudge it one at a time". The batch tool does it in one atomic call.

The tablet's inc/dec step mode is a remote-UI convenience for producing a delta, not show state.

Missing a one-call delta shortcut is ergonomics, not missing capability. The only real limit would be an array with more than 100 members, which needs two batch calls.

## Duplicates

Found from two areas and listed twice above: Live Source Tamer peak/slow enables (inputs-core, map-and-remote); cluster transforms and cluster LFO presets (clusters, map-and-remote); gradient-map shape creation (inputs-core, map-and-remote); snapshot recall/store (system-config, effects-remaining); AutomOtion transport (inputs-core, effects-remaining: the same verbs for two families).
