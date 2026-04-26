#include "MCPPromptRegistry.h"

namespace WFSNetwork
{

namespace
{
    //==========================================================================
    // Template bodies — verbatim from Documentation/MCP/prompts/MCP_PROMPTS.md.
    // Update both places together when editing.
    //==========================================================================

    constexpr const char* kSessionStartupTemplate = R"(You are assisting an operator with a WFS-DIY spatial audio session. Begin by calling
session.get_state to understand the current situation. Then:

1. If DSP is not running and no show name is set: ask the operator whether they want
   to start a fresh session or load a previous one.
2. If DSP is running: acknowledge that briefly and ask what they'd like to do next.
3. If a previous session's configuration is loaded but DSP is off: report the session
   name, channel counts, and last-modified date; ask whether to resume.

For new sessions, the typical order of operations is:
- Configure input, output, and reverb channel counts in System Config.
- Set stage dimensions in the Stage section of System Config.
- Place outputs (speakers) and set their orientations (Outputs tab — the Wizard of OutZ helps with array layouts).
- Set up reverb channels if used (Reverb tab).
- Place inputs (sources) at their starting positions (Inputs tab).
- Save the configuration.
- Start the DSP only after everything is in place.

Offer to help with any of these steps, but do not volunteer changes until the operator
confirms what they want. Do not modify any parameters until explicitly asked.)";

    constexpr const char* kSystemTuningTemplate = R"(You are walking the operator through WFS system tuning. Before starting, fetch the
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
executing any Tier 2 tools.)";

    constexpr const char* kArrayDesignTemplate = R"(You are helping the operator design a speaker array for WFS. Fetch
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
complete.)";

    constexpr const char* kSnapshotMgmtTemplate = R"(You are helping the operator manage input snapshots. Call snapshot.list to see
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
and requires the safety gate to be open.)";

    constexpr const char* kRehearsalVoiceTemplate = R"(You are operating WFS-DIY by voice during a rehearsal. The operator's hands may be
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
changes have been made.)";

    constexpr const char* kTroubleshootLocalizationTemplate = R"(You are helping troubleshoot a localization problem reported by the operator. Fetch
wfs://knowledge/parallax_correction and wfs://knowledge/wfs_theory for context.

Common symptoms and likely causes:

- Source sounds "too high up" or "coming from the flown array":
  -> Flown array delay may be too short. Check delay values in Outputs tab, increase
     the flown array delay incrementally.
  -> Parallax correction may not be set properly for the flown array.
  -> Height factor of the input may be set such that elevation doesn't weigh correctly.

- Source sounds "in front of the speakers" but should be on stage:
  -> Distance attenuation may be too low; source sounds forward because it's dry.
  -> Floor reflections may be disabled; turning them on adds perceptual anchoring.
  -> Live source damping may be active and dampening the near speakers, making the
     source feel distant.

- Source localizes "nowhere" or seems diffuse:
  -> Too many speakers active with too flat a distribution. Check distance attenuation
     ratio — raise it for tighter focus.
  -> Directivity or rotation may be set such that the source radiates everywhere.
  -> Parallax settings may be too compensated, resulting in near-zero delay spread
     and strong coupling.

- Source moves but sound seems to trail behind:
  -> Maximum speed may be limiting motion.
  -> Tracking smoothing may be too high.
  -> LFO or jitter may be active and competing with the intended movement.

- Audible Doppler on moving sources:
  -> Expected to some degree. For less: use "curvature only" mode on the input to
     minimize absolute delay changes.
  -> The acceleration profile interacts with Doppler too: sine has smoother starts and
     stops but a higher peak speed in the middle (more Doppler at the fastest point);
     line is constant speed (less peak Doppler but abrupt starts and stops). Pick
     whichever the material tolerates better.

Ask diagnostic questions one at a time. Use get_session_state and per-input queries
to gather information before recommending changes. Do not change parameters without
confirming with the operator.)";

} // anonymous namespace

MCPPromptRegistry::MCPPromptRegistry()
{
    entries.reserve (6);

    // 1. session_startup — no arguments.
    {
        PromptEntry e;
        e.name        = "session_startup";
        e.description = "Begin a new WFS-DIY session: read current state, report what's "
                        "configured, and offer to start fresh, load a previous session, "
                        "or resume.";
        e.templateBody = juce::String::fromUTF8 (kSessionStartupTemplate);
        entries.push_back (std::move (e));
    }

    // 2. system_tuning_workflow — optional reference_source_name.
    {
        PromptEntry e;
        e.name        = "system_tuning_workflow";
        e.description = "Walk the operator through the four-step WFS tuning procedure "
                        "(lower array alone, flown array alone, both together, fine tuning).";
        e.arguments.push_back ({ "reference_source_name",
                                 "Optional. Name of the input channel to use as the tuning "
                                 "reference (e.g. a lavalier mic or a rehearsal track). If "
                                 "omitted, the AI will ask the operator.",
                                 false });
        e.templateBody = juce::String::fromUTF8 (kSystemTuningTemplate);
        entries.push_back (std::move (e));
    }

    // 3. array_design_assist — optional venue_type, audience_size, frontal_or_immersive.
    {
        PromptEntry e;
        e.name        = "array_design_assist";
        e.description = "Plan a speaker array before load-in: array composition, spacing, "
                        "subwoofer placement, surrounds, coverage holes.";
        e.arguments.push_back ({ "venue_type",
                                 "Optional. One of: small, medium, large, outdoor.",
                                 false });
        e.arguments.push_back ({ "audience_size",
                                 "Optional. Approximate audience headcount.",
                                 false });
        e.arguments.push_back ({ "frontal_or_immersive",
                                 "Optional. One of: frontal, surround, dome.",
                                 false });
        e.templateBody = juce::String::fromUTF8 (kArrayDesignTemplate);
        entries.push_back (std::move (e));
    }

    // 4. snapshot_management — optional intent.
    {
        PromptEntry e;
        e.name        = "snapshot_management";
        e.description = "Save, restore, compare, or organize input snapshots. Honors the "
                        "tier confirmations (load is Tier 2, delete is Tier 3).";
        e.arguments.push_back ({ "intent",
                                 "Optional. One of: save_current, restore, compare, organize. "
                                 "If omitted, the AI will ask.",
                                 false });
        e.templateBody = juce::String::fromUTF8 (kSnapshotMgmtTemplate);
        entries.push_back (std::move (e));
    }

    // 5. rehearsal_voice_session — optional mode, default "full_control".
    {
        PromptEntry e;
        e.name        = "rehearsal_voice_session";
        e.description = "Prepare the AI for a hands-busy voice-driven rehearsal: short "
                        "confirmations, spatial-language translation, tier-aware action gating.";
        e.arguments.push_back ({ "mode",
                                 "Optional. One of: full_control (default — normal tier handling), "
                                 "dry_run (all Tier 1 escalated to Tier 2; useful for learning), "
                                 "monitoring_only (describe actions without executing; useful for demos).",
                                 false });
        e.templateBody = juce::String::fromUTF8 (kRehearsalVoiceTemplate);
        entries.push_back (std::move (e));
    }

    // 6. troubleshoot_localization — required symptom_description.
    {
        PromptEntry e;
        e.name        = "troubleshoot_localization";
        e.description = "Diagnose a spatial localization problem the operator is reporting. "
                        "Walks through the common symptoms (source too high, dry, diffuse, "
                        "trailing, Doppler) and the likely causes.";
        e.arguments.push_back ({ "symptom_description",
                                 "Required. Plain-language description of what the operator hears "
                                 "(e.g. 'Marie's voice sounds like it's coming from the ceiling').",
                                 true });
        e.templateBody = juce::String::fromUTF8 (kTroubleshootLocalizationTemplate);
        entries.push_back (std::move (e));
    }
}

const PromptEntry* MCPPromptRegistry::findByName (const juce::String& name) const
{
    for (const auto& e : entries)
        if (e.name == name)
            return &e;
    return nullptr;
}

} // namespace WFSNetwork
