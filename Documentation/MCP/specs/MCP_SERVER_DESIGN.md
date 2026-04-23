# MCP Server Design — Architecture and Implementation Specification

Target: embedded MCP server inside WFS-DIY (JUCE/C++ application).

## Purpose and scope

Expose WFS-DIY to MCP-capable AI clients as a controllable application. Cover: reading session state, modifying parameters, loading/storing snapshots, providing knowledge-base context to the AI, orchestrating named workflows (system tuning, array design, session setup).

Out of scope: voice transcription, text-to-speech, AI model hosting. These are all client-side concerns. The server's responsibility ends at the MCP protocol boundary.

## Design decisions (locked)

### Transport

**Streamable HTTP** (MCP spec revision 2025-03-26 and later). This superseded the older HTTP+SSE dual-endpoint design. A single HTTP endpoint handles both request/response and server-initiated streaming.

- Default port: 7400 (configurable in the Network tab of WFS-DIY).
- Endpoint path: `/mcp`.
- Bind address: loopback (`127.0.0.1`) by default. A checkbox in the Network tab may enable binding to the selected network interface for remote AI clients — off by default because it broadens the attack surface.
- No authentication in v1. (See "Future authentication" below.)

### Process and threading model

The MCP server lives inside the WFS-DIY process as one or more dedicated threads. The concrete topology:

- **HTTP listener thread**: accepts connections, parses HTTP, hands off requests.
- **JSON-RPC worker thread(s)**: handle the MCP protocol — parse requests, dispatch to tool handlers, format responses. A small pool (2–4 threads) is sufficient.
- **Message thread (JUCE main)**: is where tool handlers ultimately post their work. Any action that modifies the ValueTree parameter state, opens a window, loads a file, etc., runs here via `juce::MessageManager::callAsync` or equivalent.
- **Audio thread**: is NEVER touched by MCP code, directly or indirectly. Parameter changes reach the audio thread through the normal parameter-system path, which already handles thread safety.

This mirrors the pattern already used for OSC, OSCQuery, and the Android remote — the implementer should look at how those handle threading and follow the same conventions.

### Startup and shutdown

- Server starts automatically when WFS-DIY launches, after the audio engine and parameter system are ready.
- Server stops cleanly on application shutdown: refuse new connections, drain in-flight requests (short timeout, 2 seconds), close sockets.
- If the configured port is unavailable at startup, log a warning in the Network Log, try the next three port numbers, and give up if none are free. Display the actual bound port in the UI.
- A status indicator in the Network tab shows: running / stopped / error, plus the actual bound port and the number of connected clients.

### UI exposure

The Network tab gets a small MCP section showing:
- Enable/disable toggle (default: enabled).
- Port number (default 7400, editable).
- Bind scope toggle: loopback only / selected interface.
- Status indicator and connected client count.
- Copy button to copy the full MCP endpoint URL to the clipboard for pasting into an AI client config.
- "Open MCP Log" button — filters the Network Log to MCP-protocol traffic.

### Content layers

The MCP server exposes three logical layers of content:

**Tools** — functions the AI can call to read or modify state. Most are auto-generated from the parameter CSVs (see `GENERATION_SCRIPT_SPEC.md`). A small number are hand-written high-level tools that don't correspond to a single parameter (e.g. `get_session_state`, `setup_frontal_stage_defaults`).

**Resources** — readable markdown documents that describe WFS concepts, design guidelines, and procedural knowledge. Fetched on demand by the AI when relevant. See `MCP_RESOURCES.md`.

**Prompts** — named, parameterized workflow templates. The AI client uses these to orchestrate multi-step conversations with the operator (system tuning, session setup, etc.). See `MCP_PROMPTS.md`.

### Tier enforcement for tools

Every tool is tagged with a tier. Enforcement is in the server (C++ code), NOT in tool descriptions or AI prompts. Tool descriptions are advisory; code is binding.

- **Tier 1** — reversible, bounded, instant. Execute immediately. Examples: move a source, change an input's attenuation within range, mute/unmute a channel, enable/disable an LFO.
- **Tier 2** — significant but recoverable. Execute only after confirmation. The tool returns a structured "awaiting confirmation" response. A subsequent call with `confirm: true` in the arguments actually executes. Confirmation tokens expire after 30 seconds. Examples: load a snapshot, reconfigure array, change channel counts (which requires a DSP restart), overwrite a saved configuration.
- **Tier 3** — destructive or performance-critical. Requires Tier 2 confirmation plus an open "safety gate." The gate is opened by a physical action in the operator's control (UI button, MIDI note, StreamDeck button — configurable). Gate auto-closes after 60 seconds. Examples: delete a snapshot, reset all inputs, master level changes beyond a threshold, any change to the DSP state while audio is running.

A global "dry run" flag promotes all Tier 1 tools to Tier 2 behavior for rehearsal and training scenarios.

### Action history and undo architecture

WFS-DIY's user-side undo is organized as per-section stacks (inputs, outputs, reverbs, stage/network, etc.) so that Ctrl-Z behaves locally to what the operator is currently editing. The AI is treated as a separate actor with its own history. The two systems do NOT share stacks — operator Ctrl-Z never touches AI actions, and the AI's undo tools never touch operator history.

**Why separate stacks.** Overloading Ctrl-Z to sometimes reverse AI work would break the operator's muscle memory at the exact moment it's most important (live performance). The AI is a separate actor, its actions live in their own history, and the operator has distinct affordances to inspect or reverse them.

**Change record format.** Every MCP tool invocation that modifies state produces a structured change record:

```
{
  timestamp,
  tool_name,
  arguments,                  // what the AI passed in
  operator_description,       // short human-readable, e.g. "Moved input 3 (Marie) to x=4.2"
  affected_parameters,        // list of parameter variables touched
  affected_groups,            // list of (channel_id, group_name) tuples derived from CSV Section
  before_state,               // snapshot of affected parameters before the change
  after_state,                // snapshot of affected parameters after the change
  origin: "MCP"               // origin tag, always "MCP" for these records
}
```

Read-only tool calls (get_session_state, get_inputs_summary, etc.) do NOT produce change records — there's nothing to undo.

**Parameter groups.** The `affected_groups` field comes from the CSV Section column, normalized to strip coordinate-system suffixes (`Position`, `Position (Cylindrical)`, `Position (Spherical)` all resolve to the same group). Groups are scoped per channel at runtime, so "input 3's Position" and "input 4's Position" are distinct entries. Groups drive dependency chasing in undo (see "AI Undo UI" below). See `GENERATION_SCRIPT_SPEC.md` for how the generator emits group metadata per tool.

**Origin tags on all parameter changes.** This is architectural and cannot be retrofitted cheaply. Every write to the parameter system — from the UI, OSC, tracking, snapshot recall, LFO, Move, automation, or MCP — carries an origin tag on the change notification. Value set: `UI`, `MCP`, `OSC`, `Tracking`, `Snapshot`, `LFO`, `Move`, `Automation`. The parameter system's existing change-notification infrastructure must be extended to propagate this tag to all listeners (including the Network Log, the AI's undo stack, and any future observers).

**Ring buffer.** The AI's undo stack is a ring buffer of 100 entries. Oldest entries drop off silently when the buffer fills. A separate ring buffer of the same depth holds undone entries for redo.

**Staleness detection.** Before executing an undo, the server checks whether any of the target record's `affected_parameters` have been modified since the AI's change by a non-MCP origin (UI, OSC, tracking, snapshot recall, etc.). If so, the undo refuses with a structured response containing:
- What the AI's last value was.
- What the current value is.
- Which origin changed it.
The AI surfaces this to the operator ("Input 3's x was changed after my action — current value is 5.2, I had set it to 4.0. Should I restore 4.0?"). The operator decides.

**Cross-actor notification.** When a non-MCP change touches a parameter the AI has modified within the active ring-buffer window, the next AI tool-call response includes a notification field:

```
{
  "result": {...},
  "notifications": [
    "Operator reversed your change at 20:14:32: input 3 position (you set x=4.0, current x=2.5)."
  ]
}
```

This keeps the AI's conversational mental model synced with reality. Works for operator Ctrl-Z on their own history, for explicit AI-undo via the overlay, and for external OSC/tracking writes landing on AI-touched parameters.

### AI undo UI

The operator-facing surface for reversing AI actions is a growing toast overlay that appears on the main window when the AI modifies state.

**Visual structure.**
- Non-modal, stacks in a fixed screen corner (top-right suggested, to avoid conflicting with tab navigation).
- Newest row at the top; rows accumulate downward as more AI actions arrive.
- Each row shows: actor identifier ("Claude" or "AI"), a one-line operator-friendly description of the change (uses input/output/reverb names where available), elapsed time since the action, and a × button on the right for per-row undo.
- Close button in the top-left of the toast container dismisses the whole toast without undoing anything. A new AI action spawns a fresh toast.

**Row lifetime.**
- Each row has its own 15-second timer from the action's timestamp.
- When a row's timer expires, it fades out and is removed.
- When all rows have expired, the toast closes.
- Maximum 10 visible rows; older rows beyond the cap are collapsed into an expandable "… and N more" summary.

**Per-row undo semantics.**
- Clicking × on a row performs **targeted undo**: the action for that row is reversed, AND all later rows whose `affected_groups` intersect with this row's `affected_groups` are also reversed.
- Rationale: when an earlier change and a later change act on the same parameter group, they form a coherent dependency chain (a later position tweak built on the earlier position). Undoing the earlier step rolls back the full trajectory for that group. Changes to unrelated groups are independent and are left alone.
- **Hover preview**: when the operator hovers over a × button, all rows that would be reversed together are highlighted, so the outcome is visible before the click.

**Self-correction rule.**
- If a user-origin parameter write lands on a parameter the AI has touched within the toast window, AND the new value differs from the AI's most recent write to that parameter, the corresponding AI row is removed from the toast.
- Rationale: the operator has already overridden the AI's effect. Offering undo would reverse the operator's own correction. The change record stays in the queryable history; it just loses its undo affordance.
- If the user-origin value happens to match the AI's write (coincidental, rare), the row is not removed.

**Keyboard shortcuts.**
- `Cmd/Ctrl-Alt-Z`: undo the most recent AI action (equivalent to clicking × on the top row of the toast, with the same targeted-undo semantics).
- `Cmd/Ctrl-Alt-Y`: redo the most recently undone AI action.
- Shortcuts work regardless of whether the toast is visible or has been dismissed — they operate on the ring buffer directly. The toast is a visual convenience, not the only path.

**StreamDeck integration.** Not in Phase 1. The existing StreamDeck+ mapping is already dense, and keyboard shortcuts plus the overlay cover the common cases. Can be revisited if usage patterns justify a dedicated control.

### Logging

All MCP protocol messages (request in, response out, notifications) are logged to the existing Network Log window under a new protocol filter: `MCP`. This gives operators visibility into what the AI is doing and matches the existing pattern for OSC/ADM-OSC/PSN/RTTrP logging.

Log records include timestamp, direction, method name, abbreviated payload. Full payloads available on double-click, consistent with existing Network Log behavior.

## Library choices

The implementer should prefer small, header-only, permissively-licensed libraries where possible, consistent with the project's existing vendoring approach (e.g. hidapi in `third-party/`).

**JSON**: JUCE already has `juce::var` and `juce::JSON` — sufficient for MCP's needs. If a richer API is wanted, `nlohmann/json` (header-only, MIT) integrates trivially and is already familiar to most C++ developers. Either is acceptable. No preference.

**HTTP**: JUCE has `juce::StreamingSocket` and related primitives. A minimal HTTP/1.1 server implementation on top of these is feasible (the MCP protocol is simple: POST for requests, GET for SSE streaming). Alternatively, a small header-only library such as `cpp-httplib` (MIT) provides a full HTTP server in one header and handles the protocol details. Recommend `cpp-httplib` unless there's a compelling reason to roll it, since it's a solved problem.

**MCP protocol**: there is no mature official C++ MCP SDK at time of writing. Implement the protocol directly against the spec. The protocol is small: a few JSON-RPC methods, a well-defined message format, a capability negotiation step. No more than a few hundred lines of protocol code.

Do not pull in dependencies that would expand the build surface materially (boost, grpc, protobuf, etc.).

## File organization suggestion

The implementer may organize as they see fit, but a natural layout:

```
Source/Network/MCP/
  MCPServer.h / .cpp           top-level server lifecycle
  MCPTransport.h / .cpp        HTTP/SSE transport layer
  MCPDispatcher.h / .cpp       JSON-RPC method dispatch
  MCPToolRegistry.h / .cpp     tool loading, registration, invocation
  MCPResourceRegistry.h / .cpp resource loading and serving
  MCPPromptRegistry.h / .cpp   prompt templates
  MCPTierEnforcement.h / .cpp  confirmation tokens and safety gate
  MCPChangeRecords.h / .cpp    change-record capture, ring buffer, undo stack
  MCPUndoOverlay.h / .cpp      growing-toast UI component
  MCPLogger.h / .cpp           integration with Network Log
  generated_tools.json         built artifact from generation script
  resources/                   bundled markdown files
```

## Integration with existing systems

**WfsParameters / ValueTree**: tool handlers read and write parameters through the existing parameter system. No separate state. When a tool modifies a parameter, the change propagates through ValueTree listeners exactly as if the change came from the UI, OSC, or the Android remote.

**OSC layer**: the MCP server does NOT use OSC internally. It calls directly into the parameter system. OSC is for *external* clients; MCP tool handlers are internal code.

**OSCQuery layer**: Phase 6 cross-checks the auto-generated tool list against the live OSCQuery tree at startup and logs drift. Until then, no runtime interaction between MCP and OSCQuery.

**Network Log**: MCP traffic logs as a new protocol. No new window, no new filter infrastructure — reuse what exists.

**Snapshot system**: snapshot tools (`list_snapshots`, `load_snapshot`, `store_snapshot`, `update_snapshot`, `delete_snapshot`) are Tier 2 except for `list_snapshots` which is Tier 1 and `delete_snapshot` which is Tier 3. They wrap the existing snapshot file operations.

**Project/show state**: reading show name, location, channel counts, and running-state is always Tier 1 (read-only). Changing channel counts is Tier 3 (requires DSP restart).

## Future authentication

Out of scope for v1. When added, expected design:

- A per-install token generated on first run, displayed in the UI and copyable.
- HTTP Bearer token in the `Authorization` header.
- Tokens rotatable from the UI.
- For network-bound deployments (not loopback), authentication is mandatory. For loopback, it remains optional.

Do not implement this in Phase 1–5. Add it when network-bound deployments become a real use case.

## Testing strategy

The implementer should build the following test layers:

- **Unit tests**: tool dispatcher (given a tool name and arguments, the right handler is called); tier enforcement (Tier 2 tools without confirmation are rejected); generated schema loading (the JSON output of the generation script deserializes correctly).
- **Integration tests**: spin up the server in a test harness, connect with a mock MCP client, exercise the tool/resource/prompt surface end-to-end.
- **Manual tests**: Claude Desktop as the client, a live WFS-DIY session. Verify that representative workflows (move source, load snapshot, run tuning workflow) work.

The manual tests are the ones that reveal whether the tool descriptions read well to the AI. Descriptions that look fine to a human reader sometimes produce poor tool selection by the LLM, and the only way to catch this is to use the server with a real client.

## What "done" looks like

Phase 1 is done when a first-time user can install WFS-DIY, launch it, paste the MCP URL into Claude Desktop, and within two minutes be moving sources by voice. No editing config files, no installing Python, no separate services to start.

Later phases add capability without changing this baseline. A user who only wants Phase 1's surface should never be forced to care about the rest.
