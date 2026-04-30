#pragma once

#include <JuceHeader.h>
#include <map>
#include "../MCPToolRegistry.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "../../../Parameters/WFSParameterIDs.h"

namespace WFSNetwork::Tools::StateDelta
{

namespace detail
{
    /** Flat path → value map. Stable, sorted iteration; cheap to diff. */
    using Snapshot = std::map<juce::String, juce::var>;

    inline juce::String pathFor (const juce::String& section, int oneBasedId,
                                   const juce::String& field)
    {
        return section + "." + juce::String (oneBasedId) + "." + field;
    }

    inline void addChannel (Snapshot& s, const juce::String& section,
                              int oneBasedId, const juce::String& name,
                              float x, float y, float z)
    {
        s[pathFor (section, oneBasedId, "name")] = name;
        s[pathFor (section, oneBasedId, "x")]    = x;
        s[pathFor (section, oneBasedId, "y")]    = y;
        s[pathFor (section, oneBasedId, "z")]    = z;
    }

    /** Build the flat snapshot of "what's worth diffing across calls".
        Includes channel counts, stage geometry, origin, every input /
        output / reverb's id+name+position, plus the few outputs-only
        directional params and master / binaural globals an AI agent
        cares about between turns. Loud-but-stable params (EQ, LFO
        details, etc.) are intentionally NOT in here — the AI can pull
        those via session_get_channel_full when it suspects something
        meaningful changed. */
    inline Snapshot capture (WFSValueTreeState& state)
    {
        Snapshot s;

        s["channel_counts.inputs"]  = state.getNumInputChannels();
        s["channel_counts.outputs"] = state.getNumOutputChannels();
        s["channel_counts.reverbs"] = state.getNumReverbChannels();

        s["stage.shape"]            = state.getParameter (WFSParameterIDs::stageShape, -1);
        s["stage.width"]            = state.getParameter (WFSParameterIDs::stageWidth, -1);
        s["stage.depth"]            = state.getParameter (WFSParameterIDs::stageDepth, -1);
        s["stage.height"]           = state.getParameter (WFSParameterIDs::stageHeight, -1);
        s["stage.diameter"]         = state.getParameter (WFSParameterIDs::stageDiameter, -1);
        s["stage.dome_elevation"]   = state.getParameter (WFSParameterIDs::domeElevation, -1);

        s["origin.width"]           = state.getParameter (WFSParameterIDs::originWidth, -1);
        s["origin.depth"]           = state.getParameter (WFSParameterIDs::originDepth, -1);
        s["origin.height"]          = state.getParameter (WFSParameterIDs::originHeight, -1);

        s["master.level"]           = state.getParameter (WFSParameterIDs::masterLevel, -1);

        s["binaural.mode"]              = state.getParameter (WFSParameterIDs::binauralSoloMode, -1);
        s["binaural.output_channel"]    = state.getParameter (WFSParameterIDs::binauralOutputChannel, -1);
        s["binaural.listener_distance"] = state.getParameter (WFSParameterIDs::binauralListenerDistance, -1);
        s["binaural.listener_angle"]    = state.getParameter (WFSParameterIDs::binauralListenerAngle, -1);
        s["binaural.attenuation"]       = state.getParameter (WFSParameterIDs::binauralAttenuation, -1);
        s["binaural.delay"]             = state.getParameter (WFSParameterIDs::binauralDelay, -1);

        for (int i = 0; i < state.getNumInputChannels(); ++i)
        {
            addChannel (s, "inputs", i + 1,
                state.getInputParameter (i, WFSParameterIDs::inputName).toString(),
                state.getInputParameter (i, WFSParameterIDs::inputPositionX),
                state.getInputParameter (i, WFSParameterIDs::inputPositionY),
                state.getInputParameter (i, WFSParameterIDs::inputPositionZ));
        }
        for (int i = 0; i < state.getNumOutputChannels(); ++i)
        {
            addChannel (s, "outputs", i + 1,
                state.getOutputParameter (i, WFSParameterIDs::outputName).toString(),
                state.getOutputParameter (i, WFSParameterIDs::outputPositionX),
                state.getOutputParameter (i, WFSParameterIDs::outputPositionY),
                state.getOutputParameter (i, WFSParameterIDs::outputPositionZ));
            s[pathFor ("outputs", i + 1, "orientation")] = state.getOutputParameter (i, WFSParameterIDs::outputOrientation);
            s[pathFor ("outputs", i + 1, "pitch")]       = state.getOutputParameter (i, WFSParameterIDs::outputPitch);
            s[pathFor ("outputs", i + 1, "array")]       = state.getOutputParameter (i, WFSParameterIDs::outputArray);
        }
        for (int i = 0; i < state.getNumReverbChannels(); ++i)
        {
            addChannel (s, "reverbs", i + 1,
                state.getReverbParameter (i, WFSParameterIDs::reverbName).toString(),
                state.getReverbParameter (i, WFSParameterIDs::reverbPositionX),
                state.getReverbParameter (i, WFSParameterIDs::reverbPositionY),
                state.getReverbParameter (i, WFSParameterIDs::reverbPositionZ));
        }

        return s;
    }

    /** Numeric-aware equality so the diff doesn't fire on int-vs-float
        type drift (e.g. tree storing a `var(1)` vs `var(1.0)`). Mirrors
        MCPUndoEngine's varEqualsLoose. */
    inline bool varEqualsLoose (const juce::var& a, const juce::var& b)
    {
        const bool aNum = a.isInt() || a.isInt64() || a.isDouble() || a.isBool();
        const bool bNum = b.isInt() || b.isInt64() || b.isDouble() || b.isBool();
        if (aNum && bNum)
            return static_cast<double> (a) == static_cast<double> (b);
        return a == b;
    }

    /** Server-wide last snapshot, replaced on every successful call.
        Single-cursor mode: one shared snapshot across all MCP clients.
        Multiple concurrent clients would interfere (each call resets
        the baseline for the next), but the typical setup is one AI
        client per session, so this is fine. The lock is cheap insurance
        in case the dispatcher's thread model ever changes. */
    struct CacheState
    {
        bool       hasSnapshot = false;
        Snapshot   snapshot;
        juce::Time capturedAt;
        juce::CriticalSection lock;
    };

    inline CacheState& cache()
    {
        static CacheState s;
        return s;
    }
} // namespace detail

inline juce::var schema()
{
    auto reset = std::make_unique<juce::DynamicObject>();
    reset->setProperty ("type", "boolean");
    reset->setProperty ("default", false);
    reset->setProperty ("description",
        "When true, drops the cached snapshot and returns a full snapshot "
        "again. Use this when you've just connected or want to re-baseline.");

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("reset", juce::var (reset.release()));

    auto schemaObj = std::make_unique<juce::DynamicObject>();
    schemaObj->setProperty ("type", "object");
    schemaObj->setProperty ("properties", juce::var (props.release()));
    schemaObj->setProperty ("additionalProperties", false);
    return juce::var (schemaObj.release());
}

inline ToolResult getDelta (WFSValueTreeState& state, const juce::var& args)
{
    using namespace detail;

    bool reset = false;
    if (auto* obj = args.getDynamicObject())
        if (obj->hasProperty ("reset"))
            reset = static_cast<bool> (obj->getProperty ("reset"));

    auto& c = cache();
    const juce::ScopedLock sl (c.lock);

    const auto current = capture (state);
    const auto now = juce::Time::getCurrentTime();

    auto root = std::make_unique<juce::DynamicObject>();

    if (reset || ! c.hasSnapshot)
    {
        // Full snapshot — first call, or caller asked for a re-baseline.
        auto snapObj = std::make_unique<juce::DynamicObject>();
        for (const auto& [k, v] : current)
            snapObj->setProperty (k, v);

        root->setProperty ("full", true);
        root->setProperty ("snapshot", juce::var (snapObj.release()));
        root->setProperty ("snapshot_size", static_cast<int> (current.size()));

        c.hasSnapshot = true;
        c.snapshot    = current;
        c.capturedAt  = now;
        return ToolResult::ok (juce::var (root.release()));
    }

    // Diff against the cached snapshot.
    juce::Array<juce::var> changed, added, removed;

    // Find changed and added.
    for (const auto& [k, v] : current)
    {
        auto it = c.snapshot.find (k);
        if (it == c.snapshot.end())
        {
            auto entry = std::make_unique<juce::DynamicObject>();
            entry->setProperty ("path",  k);
            entry->setProperty ("value", v);
            added.add (juce::var (entry.release()));
        }
        else if (! varEqualsLoose (it->second, v))
        {
            auto entry = std::make_unique<juce::DynamicObject>();
            entry->setProperty ("path",   k);
            entry->setProperty ("before", it->second);
            entry->setProperty ("after",  v);
            changed.add (juce::var (entry.release()));
        }
    }
    // Find removed.
    for (const auto& [k, v] : c.snapshot)
    {
        if (current.find (k) == current.end())
        {
            auto entry = std::make_unique<juce::DynamicObject>();
            entry->setProperty ("path", k);
            entry->setProperty ("value", v);
            removed.add (juce::var (entry.release()));
        }
    }

    const double secondsSinceLast = (now - c.capturedAt).inSeconds();

    root->setProperty ("full", false);
    root->setProperty ("seconds_since_last_call", secondsSinceLast);
    root->setProperty ("changed", juce::var (changed));
    root->setProperty ("added",   juce::var (added));
    root->setProperty ("removed", juce::var (removed));
    root->setProperty ("change_count",
        changed.size() + added.size() + removed.size());

    // Replace cache for next diff.
    c.snapshot   = current;
    c.capturedAt = now;
    return ToolResult::ok (juce::var (root.release()));
}

inline ToolDescriptor describe (WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "session_get_state_delta";
    d.description = "Read-only delta of the session state since the previous "
                    "call to this tool. Server caches one shared snapshot "
                    "across all MCP clients; each call diffs the current "
                    "state against the cache and replaces it. First call "
                    "(or `reset: true`) returns a full snapshot. Captures "
                    "every origin (operator UI, OSC, tracking, automation, "
                    "AI) - use this between turns to notice when state "
                    "drifted under you. Snapshot covers channel counts, "
                    "stage + origin, master / binaural globals, and per-"
                    "channel id+name+position (outputs also carry "
                    "orientation, pitch, array assignment). Heavier params "
                    "(EQ, LFO, etc.) are out of scope - pull them via "
                    "session_get_channel_full if a delta hints at trouble.";
    d.inputSchema   = schema();
    d.modifiesState = false;
    d.tier        = 1;
    d.handler = [&state] (const juce::var& args, ChangeRecord*) -> ToolResult
    {
        return getDelta (state, args);
    };
    return d;
}

} // namespace WFSNetwork::Tools::StateDelta
