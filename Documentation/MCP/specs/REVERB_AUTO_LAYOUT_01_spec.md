# `reverb_auto_layout` — Tool Specification

## Purpose

Place existing reverb channels around the speaker locus, with geometry and
randomization tuned for the configured reverb algorithm. The reverb nodes are
modeling room behavior as part of the active acoustic system, not adding an
effect. Their placement is structural.

The tool is invoked by the operator (typically through chat) after speakers
have been placed. It reads the scene, identifies the speaker topology, picks
the appropriate placement geometry, and applies positions, orientations, and
pitch in a single atomic batch. The map view is the natural feedback surface.
The standard undo system handles regret.

## Tier

**Tier 2.** Writes `reverbPositionX/Y/Z`, `reverbOrientation`, and
`reverbPitch` (dome topologies only) for every existing reverb channel in one
`wfs_set_parameter_batch` call — single undoable entry.

## Inputs

```
topology?: "stage_halo"
         | "surround_ring"
         | "outer_ring_beyond_audience"
         | "dome_shell_behind"
    Omit on first call. The tool detects the topology from output positions
    and returns a clarification request if more than one topology fits
    comparably. The operator clarifies and re-calls with this set.

audience_radius?: number
    Required when topology resolves to surround_ring or
    outer_ring_beyond_audience. Meters from listener.

seed?: int
    Reproducibility. Same seed → same chaotic placement. Returned in the
    response so the operator can keep a layout they liked.
```

## Behavior

### 1. Read the scene

Read once at entry:

- Stage geometry — `stageShape`, `stageWidth`, `stageDepth`, `stageDiameter`,
  `originWidth`, `originDepth`.
- Output positions — every output's `outputPositionX/Y/Z`.
- Listener position — from the tracking section, or default origin.
- Reverbs — existing channel count and each one's `reverbAlgoType` (the
  algorithm is global per setup; reading one node is sufficient, but verify
  consistency across nodes).

### 2. Refuse if speakers aren't placed

If every output is at the default `(0, 0, 0)`, return:

```json
{
  "status": "speakers_not_placed",
  "message": "Outputs are at default position (0,0,0). Place speakers first; reverbs go behind them."
}
```

No guessing, no fallback heuristic. The tool's whole premise is "speakers are
the reference"; without that reference, there's nothing to anchor the
placement to.

### 3. Classify speaker topology

Run the geometric tests defined in `02-reverb-topology-classification.md` to
get one of:

- `stage_halo_behind_frontal_array`
- `stage_halo_behind_perimeter_array`
- `surround_ring`
- `outer_ring_beyond_audience`
- `dome_shell_behind`
- ambiguous (more than one of the above fits)

### 4. Confidence check

If classification returns ambiguous, return a clarification request with the
plausible candidates and what each implies:

```json
{
  "status": "clarification_needed",
  "observed": "Outputs form a full perimeter around the listener at radius ~8m. This fits two topologies:",
  "options": [
    {
      "topology": "stage_halo",
      "meaning": "Inward perimeter speakers monitoring the stage; reverbs sit behind the upstage/side speakers, audience face open downstage."
    },
    {
      "topology": "surround_ring",
      "meaning": "Surround active-acoustic system; reverbs distributed 360° around the audience, fewer nodes than stage halo."
    }
  ],
  "prompt": "Re-call with topology=<id> matching your system intent."
}
```

No state changes. The operator clarifies and re-calls. If the operator passes
an explicit `topology` on the first call, this gate is skipped.

### 5. Require parameters specific to chosen topology

If the resolved topology is `surround_ring` or `outer_ring_beyond_audience`
and `audience_radius` was not provided, return:

```json
{
  "status": "missing_parameter",
  "parameter": "audience_radius",
  "message": "audience_radius required for <topology>. Meters from listener to outer edge of audience zone."
}
```

### 6. Compute layout

See `03-reverb-placement-geometries.md` for the geometry primitives. In all
cases standoff is measured **behind the speaker locus**, not from the stage
edge — the speaker positions are the reference frame.

### 7. Branch on reverb algorithm

- **SDN**: chaotic placement (irregular per-node standoff, ±5° orientation
  jitter). See `04-reverb-sdn-chaotic-placement.md`.
- **FDN / IR convolution**: regular placement (uniform standoff, no
  orientation jitter, even-count permitted, even arc-length spacing).

### 8. Apply

Single `wfs_set_parameter_batch` writing:

- `reverbPositionX`, `reverbPositionY`, `reverbPositionZ` for every reverb
- `reverbOrientation` for every reverb
- `reverbPitch` for every reverb only if topology is `dome_shell_behind`

With 9 reverbs and dome topology that's 45 writes — well under the 100-write
batch cap. Larger counts may need splitting; check before submitting.

### 9. Return

```json
{
  "status": "applied",
  "topology": "stage_halo_behind_frontal_array",
  "reverb_algorithm": "SDN",
  "node_count": 9,
  "standoff_behind_speakers": "4-6m (irregular for SDN)",
  "seed": 1742,
  "rationale": "Speakers form frontal array downstage; reverbs placed behind the array on three sides with chaotic standoffs and ±5° orientation jitter to avoid SDN metallic ringing. Audience face open."
}
```

The rationale is one or two sentences in plain English. The chat layer
(Claude in conversation) elaborates if asked, and translates to the user's
conversation language if different from the tool's English.

Detailed coordinates are not in the return; the operator sees them on the
map and can read them via `session_get_state` if needed.

## Error Cases

| Status | Meaning |
|---|---|
| `applied` | Layout applied. Operator reviews on map; undo available. |
| `speakers_not_placed` | All outputs at (0,0,0). Place speakers first. |
| `clarification_needed` | Topology ambiguous. Operator clarifies intent and re-calls with explicit `topology`. |
| `missing_parameter` | Topology requires `audience_radius` not supplied. |
| `no_reverbs` | No reverb channels exist. Create some first. |
| `mixed_algorithms` | Different reverb channels have different `algoType`. The active-acoustic model assumes one algorithm per setup; refuse rather than guess which placement style applies. Operator unifies algorithm first. |

## Out of Scope for v1

- **Channel creation/deletion**: tool works with the reverb channels that
  exist. Suggesting count changes (e.g., "you should have 9, you have 6")
  involves Tier 3 channel-count writes. Deferred.
- **Speaker placement**: this tool reads the speaker locus; it does not
  suggest where speakers should go. That's a separate workflow.
- **Cuing / snapshots**: applied layout is the live state. No snapshot or
  cue integration.
- **External reverb channels**: out of scope. External reverb is treated as
  effect, not active acoustic; different problem.
- **Per-node algorithm differences**: refused; the model assumes one
  algorithm per setup.
- **Pitch for non-dome topologies**: the geometric case for pitched reverb
  nodes is weak at WFS distances when the height range is 2-3m. Skipped.

## Notes for the Implementer

- The orientation formula, verified empirically against an operator's
  manual layout: `degrees(atan2(dx, -dy))` where `dx = x_node - x_target`
  and `dy = y_node - y_target`, target defaulting to origin (0, 0). See
  `03-reverb-placement-geometries.md` for usage per topology.
- The "behind the speakers" rule is the unifying principle across all
  topologies. The earlier "around the stage" framing was a special case for
  frontal arrays — physically equivalent because the speakers happen to sit
  on the stage perimeter in that case.
- The undo system is the safety net. The tool applies; the operator reviews
  visually; if wrong, undo. This is preferred over a proposal/apply two-step
  because the map view conveys "is this right?" far better than a numeric
  proposal in chat.
- Always-confirm in the *ambiguous* case (clarification_needed), but
  always-apply in the *unambiguous* case. Operators with clear intent stated
  in chat shouldn't be slowed down by confirmation; the chat layer (Claude)
  passes the intent through as the explicit `topology` argument.
