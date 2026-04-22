# MCP Server for WFS-DIY — Implementation Roadmap

**Read this first.** This document orients Claude Code (or any implementer) to the overall plan, phasing, and scope boundaries before diving into the detailed specs.

## Context

WFS-DIY is getting an embedded Model Context Protocol (MCP) server so that AI clients (Claude Desktop, Claude Code, ChatGPT, Mistral, self-hosted models) can control the application — reading its state, modifying parameters, loading snapshots, assisting with session setup and tuning workflows, and eventually supporting voice-operated hands-free use during rehearsals and demos.

This is a post-beta iteration. Not urgent. It should be built only after beta feedback has settled and the CI/notarization work (GitHub Actions) is done. It is NOT an Avignon deliverable.

## Why embedded C++, not a separate process

Pierre-Olivier chose option 3 from the design conversation: the MCP server lives inside WFS-DIY, not as a separate Python or Node process. Reasons:

- Zero new dependencies shipped to users. No second binary to sign, notarize, distribute.
- Direct access to application state. No OSC round-trip for Claude-initiated actions.
- Trivial lifecycle: starts with the app, dies with the app.
- The codebase already handles harder protocols (OSC, OSCQuery, custom Android remote with I/P-frame bundling, hidapi integration). MCP JSON-RPC is simpler than any of these.
- Consistent with the project's DIY-accessible ethos: a single application, open-source, locally controlled, no cloud dependencies.

## Architectural summary

See `specs/MCP_SERVER_DESIGN.md` for detail. Headline decisions:

- **Transport**: Streamable HTTP (the 2025-03-26 MCP spec revision) on a configurable port, default 7400. UI exposes the port for copy-paste into client configs.
- **Threading**: MCP handler runs on its own thread(s); tool invocations post messages to the message thread; audio thread is never touched directly. Same discipline already used for OSCQuery and the Android remote.
- **Three-layer content**: tools (per-parameter actions, mostly auto-generated from CSVs), resources (markdown knowledge documents, fetched on demand), prompts (named multi-step workflows).
- **Tiered confirmation**: tools are tagged Tier 1 (reversible, execute immediately), Tier 2 (significant, confirm before execute), Tier 3 (destructive or mid-show, hard gate). Enforcement is in the server, not in prompts.
- **Source of truth for tools**: the existing parameter CSVs. A build-step generator script emits tool schemas from them, ensuring the MCP surface cannot drift from the parameter system.

## Phased delivery

Each phase is independently shippable and testable. Do not start phase N+1 before phase N is solid.

### Phase 0 — Foundation (no MCP yet)

Before any MCP code is written:

1. **Generation script**: implement the CSV-to-schema generator described in `specs/GENERATION_SCRIPT_SPEC.md`. Output is a single JSON file consumed at MCP server startup. Tests: round-trip a few representative parameters, verify enum unpacking, verify conditional-availability notes are preserved.
2. **Resource file preparation**: the knowledge-base resources in `resources/` are ready to drop into the repo (probably under `third-party/mcp-resources/` or similar). No processing needed — they're markdown files.

### Phase 1 — Minimal viable MCP server

A server that does the smallest useful thing, end-to-end, so the transport, threading, and client handshake are proven before the tool surface grows.

Scope:
- Streamable HTTP transport, listening on configurable port.
- MCP handshake: `initialize`, capability negotiation.
- `tools/list` returning 3–5 hand-written tools only (no generation script yet):
  - `get_session_state()` — returns a JSON summary of current inputs, outputs, reverbs, running/stopped state.
  - `select_input(input_id)` — selects an input channel in the UI.
  - `set_input_position(input_id, x, y, z)` — moves a source.
  - `set_input_attenuation(input_id, db)` — sets input level.
  - `list_snapshots()` — lists saved input snapshots.
- Logging of MCP traffic into the existing Network Log window, filterable as a new protocol.

Success criteria: Claude Desktop connects to `http://localhost:7400/mcp`, lists the 5 tools, and can successfully move a source by saying "move input 3 to position 4, 2, 1.5".

Explicit non-goals in this phase:
- No resources.
- No prompts.
- No auto-generation from CSVs.
- No tiered confirmation (everything is effectively Tier 1).
- No OSCQuery cross-check.

### Phase 2 — Auto-generated tool surface

Integrate the Phase 0 generation script output. The MCP server loads the generated JSON at startup and registers the full tool surface from it.

Scope:
- Load generated tool schema JSON on startup.
- Register all generated tools with the MCP server.
- Implement the generic tool dispatcher: a single C++ function that receives `(tool_name, arguments)` and routes to the appropriate internal API or OSC handler.
- Implement nudge/relative-value variant for any parameter whose CSV row has an `inc/dec` marker.
- Implement the `set_parameter(path, value)` generic fallback tool.
- Parameter namespacing: tools are exposed under logical groups (e.g. `input.position.set_x`, `input.lfo.configure`) so that clients can filter if they want.

Success criteria: all ~450 parameters are reachable through MCP tools. Claude can perform any operation an OSC client could.

Explicit non-goals in this phase:
- No resources or prompts yet.
- No bulk/grouped tools — one tool per parameter. The grouping layer comes later.
- No tier classification yet; continue to treat everything as Tier 1.

### Phase 3 — Resources (knowledge layer)

Expose the curated knowledge base so Claude can answer "how should I think about X" questions without operator assistance.

Scope:
- Implement MCP `resources/list` and `resources/read`.
- Bundle the markdown files from `resources/` directory into the app (resource compilation step, or loaded from disk at runtime — implementer's choice).
- Each resource has a URI like `wfs://knowledge/psychoacoustics` and a descriptive title/description for the `resources/list` response.

Success criteria: Claude answers a question like "explain why I'd use live source damping" by first fetching the relevant resource, then summarizing it in its own words.

### Phase 4 — Prompts (workflow layer)

Named, parameterized multi-step workflows.

Scope:
- Implement MCP `prompts/list` and `prompts/get`.
- Implement the prompts defined in `prompts/MCP_PROMPTS.md`: system tuning, array design assistance, session setup, snapshot management, voice-controlled rehearsal session.
- Each prompt is a template that, when invoked, generates a structured message the AI client uses to orchestrate a conversation with the operator.

Success criteria: invoking the `system_tuning_workflow` prompt in Claude Desktop walks the operator through the four-step WFS tuning procedure with appropriate tool calls at each step.

### Phase 5 — Tier enforcement and safety

Add the confirmation model. This is placed late deliberately: by this point the tool surface is stable and real usage has revealed which tools are actually dangerous in practice.

Scope:
- Tag each tool with its tier (Tier 1/2/3).
- Tier 2: tools return an "awaiting confirmation" structured response. Confirmation requires a second call to the same tool with a `confirm: true` argument.
- Tier 3: add a "safety gate" concept. Gate is opened by the operator (UI button, MIDI note, StreamDeck button — user-configurable). Gate closes automatically after a timeout. Tier 3 tools refuse execution if the gate is closed.
- Add a global dry-run mode flag that escalates all Tier 1 tools to Tier 2 behavior. Useful for training and rehearsal.

Success criteria: attempting a snapshot overwrite from Claude returns "confirm?" and does not execute until confirmed. Attempting a mid-show array reconfiguration refuses with "safety gate closed" unless the operator has opened it.

### Phase 6 — OSCQuery cross-check (polish)

At server startup, query the local OSCQuery endpoint and verify that every auto-generated tool's OSC path actually exists. Log warnings for drift. This catches the case where the CSVs and the live OSC implementation diverge.

## Documents in this handoff

```
IMPLEMENTATION_ROADMAP.md  ← you are here
specs/
  MCP_SERVER_DESIGN.md         architecture, threading, transport, lifecycle
  MCP_TOOL_SURFACE.md          tool taxonomy, naming, grouping decisions
  GENERATION_SCRIPT_SPEC.md    CSV-to-schema generator specification
  MCP_RESOURCES.md             which resources exist and their scope
resources/
  knowledge_psychoacoustics.md
  knowledge_wfs_theory.md
  knowledge_array_design.md
  knowledge_system_tuning.md
  knowledge_parallax_correction.md
  knowledge_live_source_damping.md
  knowledge_floor_reflections.md
  knowledge_reverb_in_wfs.md
  knowledge_level_maps.md
  knowledge_source_movements.md
  knowledge_signal_flow.md
  knowledge_session_concepts.md
  knowledge_tracking.md
prompts/
  MCP_PROMPTS.md               workflow template specifications
```

## Things to keep in mind throughout implementation

**The MCP server is untrusted input to the OSC layer.** LLMs can hallucinate arguments. Clamp, validate, and sanity-check at the server boundary. Treat tool calls with the same skepticism you'd apply to network input from an unknown client.

**Do not touch the audio thread from MCP code.** All parameter changes go through the normal parameter-system path, which already handles thread safety. This is not new discipline; it's the same pattern as the OSC, OSCQuery, and Android remote layers.

**Resist the temptation to add "helpful" high-level tools early.** They are genuinely useful but they're also opinionated about workflows. Build the raw parameter surface first, let usage show what workflows actually matter, then compose the high-level tools in Phase 4.

**The CSVs are the source of truth, not the generated JSON or the C++.** If a parameter needs to change, the CSV changes, the generator reruns, the C++ picks up the new schema. Manual edits to generated code will be lost.

**Voice control is not something to implement here.** It is an emergent capability of the client, not a server feature. The server's job is to have a tool surface that reads naturally when spoken. Good descriptions = natural voice control, automatically.

**MCP is vendor-neutral.** Do not accept any client-specific behavior into the server. If Claude Desktop does something that ChatGPT does not, that's a client difference, not a server concern.
