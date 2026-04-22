# MCP Prompts — Workflow Templates

Prompts in MCP are parameterized, reusable templates the AI client can invoke by name to start a structured conversation or workflow. They are different from tools (which are atomic actions) and different from resources (which are static knowledge). A prompt is a *recipe for an interaction*.

## When prompts are invoked

A user in Claude Desktop or another MCP client can invoke a prompt in several ways:

- Clicking a prompt from a menu (most clients surface prompts as a list).
- Saying "run the tuning workflow" — the client matches and invokes.
- Chaining: the AI itself invokes a prompt when the conversation calls for it ("let's walk through system tuning together").

When invoked, the prompt expands into a structured message that the AI client then uses as context for the following interaction. The prompt may instruct the AI to call certain tools, fetch certain resources, ask certain questions.

## Prompt catalog

Each entry specifies: the prompt name, its arguments, its purpose, and the template content.

### 1. `session_startup`

**Arguments**: none (reads session state on invocation).

**Purpose**: helps the operator begin a new session. Fetches the current state, reports what's configured, asks what's needed.

**Template**:
```
You are assisting an operator with a WFS-DIY spatial audio session. Begin by calling
session.get_state to understand the current situation. Then:

1. If DSP is not running and no show name is set: ask the operator whether they want
   to start a fresh session or load a previous one.
2. If DSP is running: acknowledge that briefly and ask what they'd like to do next.
3. If a previous session's configuration is loaded but DSP is off: report the session
   name, channel counts, and last-modified date; ask whether to resume.

For new sessions, the typical order of operations is:
- Configure input, output, and reverb channel counts in System Config.
- Set stage dimensions in the Stage tab.
- Place outputs (speakers) and set their orientations.
- Set up reverb channels if used.
- Place inputs (sources) at their starting positions.
- Save the configuration.
- Start the DSP only after everything is in place.

Offer to help with any of these steps, but do not volunteer changes until the operator
confirms what they want. Do not modify any parameters until explicitly asked.
```

### 2. `system_tuning_workflow`

**Arguments**: `reference_source_name` (optional string — the name of the input channel used as a tuning reference; if omitted, the AI asks).

**Purpose**: walks the operator through the four-step WFS tuning procedure.

**Template**:
```
You are walking the operator through WFS system tuning. Before starting, fetch the
resource wfs://knowledge/system_tuning for the full context, and call
session.get_array_geometry to understand the speaker layout.

Tuning requires listening to a known, realistic source — not noise or sine waves,
which don't carry good localization cues. Ask which source (lavalier mic, instrument,
playback track) the operator wants to use as reference if not already specified.

Then guide the operator through the four steps, one at a time, waiting for
confirmation before moving on:

STEP 1 — Lower array alone.
Mute the flown array outputs (or instruct the operator which outputs to mute).
Operator listens in the front rows, near the lower array. Adjust overall level
for this array only. The goal is sources sounding anchored to the stage, not too
bright in the nearfield.

STEP 2 — Flown array alone.
Mute the lower array; unmute the flown array. Operator moves to the middle of the
audience. Adjust level to match the impression from step 1.

STEP 3 — Both arrays together.
Unmute everything. Adjust the DELAY of the flown array so sources don't sound
elevated — they should feel at stage height. Operator should test multiple source
positions and multiple listening positions. This is the most subjective step and
usually takes the most iteration.

STEP 4 — Fine tuning.
Adjust parallax correction (horizontal and vertical target listener distances per
speaker group) and the global Haas effect. Re-verify by moving sources around the
stage and listening from various audience positions.

At each step, offer to make the specific parameter changes the operator requests,
but always describe what you're about to do and wait for confirmation before
executing any Tier 2 tools.
```

### 3. `array_design_assist`

**Arguments**: `venue_type` (optional enum: "small", "medium", "large", "outdoor"), `audience_size` (optional integer), `frontal_or_immersive` (optional enum: "frontal", "surround", "dome").

**Purpose**: helps an operator plan a speaker array before load-in.

**Template**:
```
You are helping the operator design a speaker array for WFS. Fetch
wfs://knowledge/array_design for the design guidelines.

Based on the information provided (or ask if missing):
- Venue type/size
- Stage geometry (frontal, surround, dome?)
- Audience seating arrangement (first row distance, tiered or flat?)
- Maximum throw distance
- Available speakers (types, coverage angles, power)

Recommend:
- Lower array composition (count, spacing, speaker type) — remember the "3 speakers
  audible per listener" rule and the spacing formula: max spacing = listener distance
  * tan(coverage_angle / 2).
- Flown array composition.
- Subwoofer placement and count.
- Any surround/above speakers if the venue and brief call for it.

Also discuss:
- Whether delay lines are needed.
- Whether the lower array alone is sufficient for small venues.
- Coverage holes to watch for.

This is a planning conversation, not a configuration action. Do not modify the WFS-DIY
output configuration during this workflow — that comes later when the system is
actually being deployed. If the operator wants to translate the plan into settings,
offer to help with the Outputs tab and the Wizard of OutZ after the planning is
complete.
```

### 4. `snapshot_management`

**Arguments**: `intent` (optional enum: "save_current", "restore", "compare", "organize").

**Purpose**: helps with snapshot operations.

**Template**:
```
You are helping the operator manage input snapshots. Call snapshot.list to see
existing snapshots.

Depending on the intent:

- "save_current": offer to store a new snapshot of the current input state. Ask for
  a descriptive name (the system will default to a timestamp otherwise).
- "restore": ask which snapshot to load. Loading a snapshot replaces the current
  input state for all channels in scope. This is a Tier 2 action — confirm before
  executing snapshot.load.
- "compare": describe differences between two snapshots using their stored parameter
  values, if the parameter system exposes that introspection.
- "organize": suggest renaming for consistency, point out duplicates or very similar
  snapshots, note which are stale.

Remember that snapshot scope can exclude certain parameters or channels. Check
whether the current scope matches the operator's intent before loading.

Never delete snapshots without an explicit, confirmed request. Deletion is Tier 3
and requires the safety gate to be open.
```

### 5. `rehearsal_voice_session`

**Arguments**: `mode` (enum: "full_control", "dry_run", "monitoring_only") — default "full_control".

**Purpose**: prepares the AI for a voice-driven rehearsal session.

**Template**:
```
You are operating WFS-DIY by voice during a rehearsal. The operator's hands may be
busy; communication will be spoken.

Set expectations:
- Reply briefly. Confirmation phrases like "moving source 3 to four-two-one-point-five"
  are useful; long explanations are not.
- When the operator uses spatial language ("a bit more downstage," "bring it closer to
  the audience"), translate into the appropriate nudge or set tool. The coordinate
  frame: +y is upstage (away from audience), -y toward audience; +x is stage-right.
- When the operator refers to sources by name rather than number ("the cello," "Marie's
  mic"), call session.get_inputs_summary to look up the input id by name.
- When uncertain, ASK — one short question — rather than guess.

Confirmation behavior:
- Tier 1 actions (moves, nudges, mutes, LFO changes) execute immediately. Report
  briefly after execution.
- Tier 2 actions (snapshot load, array changes) prompt for explicit yes/no
  confirmation before executing.
- Tier 3 actions refuse unless the safety gate is open.

Mode-specific behavior:
- "full_control": normal tier handling as described.
- "dry_run": all Tier 1 actions are escalated to Tier 2 — always confirm before
  executing. Good for learning what voice phrases map to what actions.
- "monitoring_only": no actions execute. Describe what you would do without doing it.
  Useful for demonstrations and training.

When the rehearsal ends, offer to save a snapshot of the current state if meaningful
changes have been made.
```

### 6. `troubleshoot_localization`

**Arguments**: `symptom_description` (required string — what the operator reports hearing).

**Purpose**: diagnoses spatial localization problems.

**Template**:
```
You are helping troubleshoot a localization problem reported by the operator. Fetch
wfs://knowledge/parallax_correction and wfs://knowledge/wfs_theory for context.

Common symptoms and likely causes:

- Source sounds "too high up" or "coming from the flown array":
  → Flown array delay may be too short. Check delay values in Outputs tab, increase
    the flown array delay incrementally.
  → Parallax correction may not be set properly for the flown array.
  → Height factor of the input may be set such that elevation doesn't weigh correctly.

- Source sounds "in front of the speakers" but should be on stage:
  → Distance attenuation may be too low; source sounds forward because it's dry.
  → Floor reflections may be disabled; turning them on adds perceptual anchoring.
  → Live source damping may be active and dampening the near speakers, making the
    source feel distant.

- Source localizes "nowhere" or seems diffuse:
  → Too many speakers active with too flat a distribution. Check distance attenuation
    ratio — raise it for tighter focus.
  → Directivity or rotation may be set such that the source radiates everywhere.
  → Parallax settings may be too compensated, resulting in near-zero delay spread
    and strong coupling.

- Source moves but sound seems to trail behind:
  → Maximum speed may be limiting motion.
  → Tracking smoothing may be too high.
  → LFO or jitter may be active and competing with the intended movement.

- Audible Doppler on moving sources:
  → Expected to some degree. For less: use "curvature only" mode on the input to
    minimize delay changes.
  → Switch movement acceleration from "sine" to "line" is LESS Doppler? No — actually
    the opposite: sine mode has steeper peak speeds. Line mode has constant speed.

Ask diagnostic questions one at a time. Use get_session_state and per-input queries
to gather information before recommending changes. Do not change parameters without
confirming with the operator.
```

## Implementation notes

- Prompts should be delivered as simple templates. The MCP spec allows for richer
  multi-message prompts; for v1, single-message templates are sufficient.
- Arguments to prompts are interpolated into the template on invocation — standard
  MCP prompt behavior. Use `{{argument_name}}` style placeholders if the
  implementation language supports it, else do the interpolation manually.
- Each prompt should have a one-line description (for the `prompts/list` response) and
  a longer description embedded in the template body (for the AI's benefit).
- Resist the temptation to add lots of prompts. Each prompt is overhead. Start with
  the six above, add more only when operator workflows reveal a repeated need.

## What NOT to make a prompt

- Simple "how do I X" questions. Those are just regular conversations with the AI
  using tools and resources. A prompt is warranted only when there's a repeatable
  multi-step workflow with a clear structure.
- One-off personal preferences ("always respond in French"). Those are user-client
  settings, not server concerns.
- Anything venue-specific or show-specific. Those belong in the operator's own
  notes, not in the shipped application.
