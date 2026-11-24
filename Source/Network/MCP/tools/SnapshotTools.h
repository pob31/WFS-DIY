#pragma once

#include <JuceHeader.h>
#include "../MCPCompat.h"
#include "../../../Parameters/WFSFileManager.h"
#include "../../../Parameters/WFSValueTreeState.h"
#include "ToolArgs.h"

/*
    Snapshots over MCP: list, store, update, delete, read and edit the scope and
    the MIDI binding, export cues to QLab / Go.dot. Every one of these prepares
    a show; none of them plays it.

    RECALL IS NOT HERE, ON PURPOSE. A snapshot recall is a cue, and an agent's
    timing (it thinks before it acts, by an amount nobody can predict) is not a
    cue's timing; the operator recalls - from the row, a MIDI note, OSC, QLab or
    Go.dot. Documentation/MCP/mcp-coverage-audit-2026-11-24.md, Decisions.

    Store / update / delete run through the Snapshot row's own session
    (SnapshotSession::*ForMcp, wired by MainComponent as SnapshotActions), so
    the list, the MIDI bindings, the OSCQuery menu and the dirty marks follow
    exactly as when the operator presses the buttons.
*/
namespace WFSNetwork::Tools::Snapshot
{

/** What MainComponent provides: the session's actions. Each returns an empty
    string on success, otherwise the reason. */
struct Actions
{
    std::function<juce::String (const juce::String&)> store;
    std::function<juce::String (const juce::String&)> update;
    std::function<juce::String (const juce::String&)> remove;
    std::function<void (const juce::String&)>          scopeChanged;
    std::function<juce::String (const juce::String&)>  exportCues;
    /** A cue recalling one sampler set (input number, 1-based set, its name). */
    std::function<juce::String (int, int, const juce::String&)> exportSamplerSetCue;
};

namespace detail
{
    inline juce::var nameArg()
    {
        return Args::stringArg ("Snapshot name, as snapshot_list gives it.");
    }

    inline std::optional<juce::String> existingName (WFSFileManager& fm, const juce::DynamicObject& obj,
                                                     juce::String& message)
    {
        const auto name = obj.getProperty ("name").toString();
        if (name.isEmpty())
        {
            message = "Missing required arg: name";
            return std::nullopt;
        }
        if (! fm.getInputSnapshotNames().contains (name))
        {
            message = "No snapshot named " + name.quoted() + " (snapshot_list gives the names)";
            return std::nullopt;
        }
        return name;
    }

    inline ToolResult runAction (const std::function<juce::String (const juce::String&)>& action,
                                 const juce::String& name, const juce::String& what, ChangeRecord* record)
    {
        if (! action)
            return ToolResult::error ("unavailable", "Snapshots are not available in this session");
        const auto error = action (name);
        if (error.isNotEmpty())
            return ToolResult::error ("failed", error);
        if (record != nullptr)
        {
            record->operatorDescription = what + " snapshot " + name.quoted() + " (file on disk - not undoable)";
            record->affectedParameters.add ("snapshot");
            record->affectedGroups.push_back ({ 0, juce::String ("Snapshots") });
            record->undoable = false;
        }
        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("name", name);
        return ToolResult::ok (juce::var (result.release()));
    }

    using Scope = WFSFileManager::ExtendedSnapshotScope;
    using Matrix = WFSFileManager::ScopeMatrix;

    /** Channel ids per family: inputs by permanent number, effects dense. */
    inline int channelCount (WFSValueTreeState& state, bool effects)
    {
        return effects ? state.getNumEffectChannels() : state.getNumInputChannels();
    }

    inline int idForSlot (WFSValueTreeState& state, bool effects, int slot)
    {
        return effects ? slot + 1 : state.getInputChannelNumber (slot);
    }

    inline int slotForId (WFSValueTreeState& state, bool effects, int id)
    {
        if (effects)
            return (id >= 1 && id <= state.getNumEffectChannels()) ? id - 1 : -1;
        return state.getSlotForChannelNumber (id);
    }

    inline const WFSFileManager::ScopeItemTable& tableFor (bool effects)
    {
        return effects ? WFSFileManager::effectScopeTable() : WFSFileManager::inputScopeTable();
    }

    inline juce::var describeFamily (WFSValueTreeState& state, const Matrix& m, bool effects)
    {
        juce::Array<juce::var> items;
        const int n = channelCount (state, effects);
        for (const auto& item : tableFor (effects).items)
        {
            auto it = std::make_unique<juce::DynamicObject>();
            it->setProperty ("item", item.itemId);
            it->setProperty ("section", item.sectionId.toString());
            it->setProperty ("label", item.displayName);
            juce::Array<juce::var> off;
            for (int slot = 0; slot < n; ++slot)
                if (! m.isIncluded (item.itemId, slot))
                    off.add (idForSlot (state, effects, slot));
            if (! off.isEmpty())
                it->setProperty ("excluded_channels", juce::var (off));
            items.add (juce::var (it.release()));
        }
        auto fam = std::make_unique<juce::DynamicObject>();
        fam->setProperty ("channel_id_kind", effects ? "effect (dense, 1-based)" : "input (permanent number)");
        fam->setProperty ("items", juce::var (items));
        return juce::var (fam.release());
    }
}

//==============================================================================
// snapshot_list
//==============================================================================

inline ToolDescriptor describe (WFSFileManager& fileManager)
{
    ToolDescriptor d;
    d.name        = "snapshot_list";
    d.description = "List the snapshots saved in the current project folder (each one carries the inputs "
                    "and the effect channels), with their MIDI trigger when bound. Recalling one is the "
                    "operator's (the Snapshot row, a MIDI note, OSC, QLab, Go.dot): there is no MCP recall.";
    d.inputSchema   = Args::schemaOf (std::make_unique<juce::DynamicObject>(), {});
    d.modifiesState = false;
    d.tier          = 1;
    d.handler = [&fileManager] (const juce::var&, ChangeRecord*) -> ToolResult
    {
        std::map<juce::String, std::pair<int, int>> bindings;
        for (const auto& b : fileManager.scanSnapshotMidiBindings())
            bindings[b.snapshotName] = { b.channel, b.note };

        juce::Array<juce::var> snapshots;
        juce::Array<juce::var> names;
        for (const auto& n : fileManager.getInputSnapshotNames())
        {
            names.add (n);
            auto s = std::make_unique<juce::DynamicObject>();
            s->setProperty ("name", n);
            if (auto it = bindings.find (n); it != bindings.end())
            {
                s->setProperty ("midi_channel", it->second.first);
                s->setProperty ("midi_note", it->second.second);
            }
            snapshots.add (juce::var (s.release()));
        }
        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("snapshots", juce::var (names));     // names, as before
        result->setProperty ("details", juce::var (snapshots));
        result->setProperty ("count", names.size());
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

//==============================================================================
// snapshot_store / snapshot_update / snapshot_delete
//==============================================================================

inline ToolDescriptor describeStore (const Actions* actions)
{
    ToolDescriptor d;
    d.name        = "snapshot_store";
    d.description = juce::String (
        "Store the current inputs and effect channels as a snapshot, as the Snapshot row's Store "
        "button: the session's scope applies (snapshot_set_scope edits a stored one). Storing over "
        "an existing name replaces its content and keeps its MIDI trigger. Not undoable.")
        + juce::String (kTier2DescriptionSuffix);
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("name", Args::stringArg ("Snapshot name (a new one, or an existing one to replace)."));
    props->setProperty ("confirm", Args::confirmArg());
    d.inputSchema   = Args::schemaOf (std::move (props), { "name" });
    d.modifiesState = true;
    d.tier          = 2;
    d.handler = [actions] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        const auto name = obj != nullptr ? obj->getProperty ("name").toString().trim() : juce::String();
        if (name.isEmpty())
            return ToolResult::error ("invalid_args", "Missing required arg: name");
        return detail::runAction (actions->store, name, "Stored", record);
    };
    return d;
}

inline ToolDescriptor describeUpdate (WFSFileManager& fileManager, const Actions* actions)
{
    ToolDescriptor d;
    d.name        = "snapshot_update";
    d.description = juce::String (
        "Re-capture an existing snapshot from the current state under its own scope, as the "
        "Snapshot row's Update button. Not undoable.") + juce::String (kTier2DescriptionSuffix);
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("name", detail::nameArg());
    props->setProperty ("confirm", Args::confirmArg());
    d.inputSchema   = Args::schemaOf (std::move (props), { "name" });
    d.modifiesState = true;
    d.tier          = 2;
    d.handler = [&fileManager, actions] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        juce::String message;
        const auto name = obj != nullptr ? detail::existingName (fileManager, *obj, message) : std::nullopt;
        if (! name.has_value())
            return ToolResult::error ("invalid_args", obj == nullptr ? juce::String ("Missing required arg: name") : message);
        return detail::runAction (actions->update, *name, "Updated", record);
    };
    return d;
}

inline ToolDescriptor describeDelete (WFSFileManager& fileManager, const Actions* actions)
{
    ToolDescriptor d;
    d.name        = "snapshot_delete";
    d.description = juce::String (
        "Delete a snapshot file, as the Snapshot row's Delete. Cues and MIDI notes that recall it "
        "stop working. Not undoable.") + juce::String (kTier2DescriptionSuffix);
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("name", detail::nameArg());
    props->setProperty ("confirm", Args::confirmArg());
    d.inputSchema   = Args::schemaOf (std::move (props), { "name" });
    d.modifiesState = true;
    d.tier          = 2;
    d.handler = [&fileManager, actions] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        juce::String message;
        const auto name = obj != nullptr ? detail::existingName (fileManager, *obj, message) : std::nullopt;
        if (! name.has_value())
            return ToolResult::error ("invalid_args", obj == nullptr ? juce::String ("Missing required arg: name") : message);
        return detail::runAction (actions->remove, *name, "Deleted", record);
    };
    return d;
}

//==============================================================================
// snapshot_get_scope / snapshot_set_scope
//==============================================================================

inline ToolDescriptor describeGetScope (WFSFileManager& fileManager, WFSValueTreeState& state)
{
    ToolDescriptor d;
    d.name        = "snapshot_get_scope";
    d.description = "Read a snapshot's scope: when it applies (on_recall: the file holds everything and "
                    "the scope filters what a recall writes; on_save: the file holds only what is in scope), "
                    "its MIDI trigger, and for each family (inputs, effects) every scope item with the "
                    "channels it is excluded for. Item ids are what snapshot_set_scope takes. Read-only.";
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("name", detail::nameArg());
    d.inputSchema   = Args::schemaOf (std::move (props), { "name" });
    d.modifiesState = false;
    d.tier          = 1;
    d.handler = [&fileManager, &state] (const juce::var& args, ChangeRecord*) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        juce::String message;
        const auto name = obj != nullptr ? detail::existingName (fileManager, *obj, message) : std::nullopt;
        if (! name.has_value())
            return ToolResult::error ("invalid_args", obj == nullptr ? juce::String ("Missing required arg: name") : message);

        const auto scope = fileManager.getExtendedSnapshotScope (*name);
        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("name", *name);
        result->setProperty ("apply_mode", scope.applyMode == detail::Scope::ApplyMode::OnSave ? "on_save" : "on_recall");
        if (scope.hasMidiBinding())
        {
            result->setProperty ("midi_channel", scope.midiChannel);
            result->setProperty ("midi_note", scope.midiNote);
        }
        result->setProperty ("inputs",  detail::describeFamily (state, scope.inputs, false));
        result->setProperty ("effects", detail::describeFamily (state, scope.effects, true));
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

inline ToolDescriptor describeSetScope (WFSFileManager& fileManager, WFSValueTreeState& state, const Actions* actions)
{
    ToolDescriptor d;
    d.name        = "snapshot_set_scope";
    d.description = juce::String (
        "Edit a stored snapshot's scope in place, without re-capturing any value (the Scope window's "
        "Update Scope). changes is a list applied in order, each {family: inputs|effects, item: an item "
        "id from snapshot_get_scope or \"all\", channel_id: one channel or omitted for all, included: "
        "true|false}. Optional apply_mode (on_recall | on_save; on_save also trims the stored values to the "
        "scope, which cannot be undone) and midi ({channel 1-16, note 0-127} to bind a MIDI trigger, null "
        "to clear it; refused when another snapshot has that note). Not undoable.")
        + juce::String (kTier2DescriptionSuffix);

    auto change = std::make_unique<juce::DynamicObject>();
    change->setProperty ("type", "object");
    auto cprops = std::make_unique<juce::DynamicObject>();
    cprops->setProperty ("family", Args::enumArg ("Which grid.", { "inputs", "effects" }));
    cprops->setProperty ("item", Args::stringArg ("Scope item id, or \"all\"."));
    cprops->setProperty ("channel_id", Args::integerArg ("Input number or effect id; omit for every channel.", 1));
    cprops->setProperty ("included", Args::booleanArg ("true includes, false excludes."));
    change->setProperty ("properties", juce::var (cprops.release()));
    auto changes = std::make_unique<juce::DynamicObject>();
    changes->setProperty ("type", "array");
    changes->setProperty ("items", juce::var (change.release()));
    changes->setProperty ("description", "Scope edits, applied in order.");

    auto midi = std::make_unique<juce::DynamicObject>();
    midi->setProperty ("type", juce::var (juce::Array<juce::var> { "object", "null" }));
    midi->setProperty ("description", "{channel, note} to bind a MIDI trigger, null to clear it.");

    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("name", detail::nameArg());
    props->setProperty ("changes", juce::var (changes.release()));
    props->setProperty ("apply_mode", Args::enumArg ("When the scope applies.", { "on_recall", "on_save" }));
    props->setProperty ("midi", juce::var (midi.release()));
    props->setProperty ("confirm", Args::confirmArg());
    d.inputSchema   = Args::schemaOf (std::move (props), { "name" });
    d.modifiesState = true;
    d.tier          = 2;
    d.handler = [&fileManager, &state, actions] (const juce::var& args, ChangeRecord* record) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        juce::String message, code;
        const auto name = obj != nullptr ? detail::existingName (fileManager, *obj, message) : std::nullopt;
        if (! name.has_value())
            return ToolResult::error ("invalid_args", obj == nullptr ? juce::String ("Missing required arg: name") : message);

        auto scope = fileManager.getExtendedSnapshotScope (*name);
        int edits = 0;

        if (const auto* list = obj->getProperty ("changes").getArray())
        {
            for (int i = 0; i < list->size(); ++i)
            {
                auto* c = list->getReference (i).getDynamicObject();
                const auto where = "changes[" + juce::String (i) + "]: ";
                if (c == nullptr || Args::missing (*c, "family") || Args::missing (*c, "included"))
                    return ToolResult::error ("invalid_args", where + "needs family and included");
                const auto family = c->getProperty ("family").toString();
                if (family != "inputs" && family != "effects")
                    return ToolResult::error ("invalid_args", where + "family must be inputs or effects");
                const bool effects = family == "effects";
                auto& m = effects ? scope.effects : scope.inputs;
                const auto included = Args::boolean (c->getProperty ("included"), "included", code, message);
                if (! included.has_value())
                    return ToolResult::error (code, where + message);

                const auto item = c->hasProperty ("item") ? c->getProperty ("item").toString() : juce::String ("all");
                if (item != "all" && detail::tableFor (effects).find (item) == nullptr)
                    return ToolResult::error ("invalid_args", where + "no " + family + " scope item " + item.quoted()
                                                                + " (snapshot_get_scope lists them)");

                std::vector<int> slots;
                if (c->hasProperty ("channel_id"))
                {
                    const int id = static_cast<int> (c->getProperty ("channel_id"));
                    const int slot = detail::slotForId (state, effects, id);
                    if (slot < 0)
                        return ToolResult::error ("invalid_args", where + "channel_id " + juce::String (id)
                                                                    + " is not a live " + (effects ? "effect" : "input"));
                    slots.push_back (slot);
                }
                else
                    for (int s = 0; s < detail::channelCount (state, effects); ++s)
                        slots.push_back (s);

                for (int slot : slots)
                {
                    if (item == "all")
                        m.setAllItemsForChannel (slot, *included);
                    else
                        m.setIncluded (item, slot, *included);
                }
                ++edits;
            }
        }

        if (obj->hasProperty ("apply_mode"))
        {
            const auto mode = obj->getProperty ("apply_mode").toString();
            if (mode != "on_recall" && mode != "on_save")
                return ToolResult::error ("invalid_args", "apply_mode must be on_recall or on_save");
            scope.applyMode = mode == "on_save" ? detail::Scope::ApplyMode::OnSave : detail::Scope::ApplyMode::OnRecall;
            ++edits;
        }

        if (obj->hasProperty ("midi"))
        {
            const auto midiVar = obj->getProperty ("midi");
            if (midiVar.isVoid() || midiVar.isUndefined() || (! midiVar.isObject() && midiVar.toString() == "null"))
                scope.clearMidiBinding();
            else if (auto* mo = midiVar.getDynamicObject())
            {
                const auto ch   = Args::wholeNumber (mo->getProperty ("channel"), "midi.channel", 1, 16, code, message);
                if (! ch.has_value()) return ToolResult::error (code, message);
                const auto note = Args::wholeNumber (mo->getProperty ("note"), "midi.note", 0, 127, code, message);
                if (! note.has_value()) return ToolResult::error (code, message);
                for (const auto& b : fileManager.scanSnapshotMidiBindings())
                    if (b.channel == *ch && b.note == *note && b.snapshotName != *name)
                        return ToolResult::error ("conflict", "MIDI channel " + juce::String (*ch) + " note "
                                                                + juce::String (*note) + " already recalls snapshot "
                                                                + b.snapshotName.quoted());
                scope.midiChannel = *ch;
                scope.midiNote    = *note;
            }
            else
                return ToolResult::error ("invalid_args", "midi must be {channel, note} or null");
            ++edits;
        }

        if (edits == 0)
            return ToolResult::error ("invalid_args", "Nothing to change: give changes, apply_mode or midi");

        if (! fileManager.updateInputSnapshotScope (*name, scope))
            return ToolResult::error ("failed", fileManager.getLastError());
        if (actions->scopeChanged)
            actions->scopeChanged (*name);

        if (record != nullptr)
        {
            record->operatorDescription = "Edited the scope of snapshot " + name->quoted() + " (not undoable)";
            record->affectedParameters.add ("snapshotScope");
            record->affectedGroups.push_back ({ 0, juce::String ("Snapshots") });
            record->undoable = false;
        }
        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("name", *name);
        result->setProperty ("edits", edits);
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

//==============================================================================
// snapshot_export_cues
//==============================================================================

inline ToolDescriptor describeExportCues (WFSFileManager& fileManager, const Actions* actions)
{
    ToolDescriptor d;
    d.name        = "snapshot_export_cues";
    d.description = juce::String (
        "Write a snapshot's in-scope values as cues into QLab and/or Go.dot (whichever network targets "
        "are configured), as the Scope window's Write to QLab: the operator then fires those cues. "
        "Nothing in WFS-DIY changes.") + juce::String (kTier2DescriptionSuffix);
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("name", detail::nameArg());
    props->setProperty ("confirm", Args::confirmArg());
    d.inputSchema   = Args::schemaOf (std::move (props), { "name" });
    d.modifiesState = false;
    d.tier          = 2;
    d.handler = [&fileManager, actions] (const juce::var& args, ChangeRecord*) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        juce::String message;
        const auto name = obj != nullptr ? detail::existingName (fileManager, *obj, message) : std::nullopt;
        if (! name.has_value())
            return ToolResult::error ("invalid_args", obj == nullptr ? juce::String ("Missing required arg: name") : message);
        if (! actions->exportCues)
            return ToolResult::error ("unavailable", "Cue export is not available in this session");
        const auto error = actions->exportCues (*name);
        if (error.isNotEmpty())
            return ToolResult::error ("failed", error);
        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("name", *name);
        result->setProperty ("exported", true);
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

//==============================================================================
// input_sampler_export_set_cue
//==============================================================================

inline ToolDescriptor describeSamplerSetCue (WFSValueTreeState& state, const Actions* actions)
{
    ToolDescriptor d;
    d.name        = "input_sampler_export_set_cue";
    d.description = juce::String (
        "Write a cue into QLab and/or Go.dot that selects one sampler set of an input (the Sampler "
        "tab's Q button); the operator fires it. Nothing in WFS-DIY changes.")
        + juce::String (kTier2DescriptionSuffix);
    auto props = std::make_unique<juce::DynamicObject>();
    props->setProperty ("input_id", Args::integerArg ("Permanent input number.", 1, WFSParameterDefaults::maxInputChannels));
    props->setProperty ("set_id", Args::integerArg ("Sampler set number (1-based).", 1, WFSParameterDefaults::maxSamplerSets));
    props->setProperty ("confirm", Args::confirmArg());
    d.inputSchema   = Args::schemaOf (std::move (props), { "input_id", "set_id" });
    d.modifiesState = false;
    d.tier          = 2;
    d.handler = [&state, actions] (const juce::var& args, ChangeRecord*) -> ToolResult
    {
        auto* obj = args.getDynamicObject();
        juce::String code, message;
        if (obj == nullptr || Args::missing (*obj, "input_id") || Args::missing (*obj, "set_id"))
            return ToolResult::error ("invalid_args", "Missing required arg: input_id and set_id");
        const auto number = Args::wholeNumber (obj->getProperty ("input_id"), "input_id", 1, WFSParameterDefaults::maxInputChannels, code, message);
        if (! number) return ToolResult::error (code, message);
        const auto setId = Args::wholeNumber (obj->getProperty ("set_id"), "set_id", 1, WFSParameterDefaults::maxSamplerSets, code, message);
        if (! setId) return ToolResult::error (code, message);
        const int slot = state.getSlotForChannelNumber (*number);
        if (slot < 0)
            return ToolResult::error ("invalid_args", "input_id not a live channel: " + juce::String (*number));
        auto set = state.getInputSamplerSet (slot, *setId - 1);
        if (! set.isValid())
            return ToolResult::error ("invalid_args", "No sampler set " + juce::String (*setId) + " on that input");
        if (! actions->exportSamplerSetCue)
            return ToolResult::error ("unavailable", "Cue export is not available in this session");
        auto name = set.getProperty (WFSParameterIDs::samplerSetName).toString();
        if (name.isEmpty())
            name = "Set " + juce::String (*setId);
        const auto error = actions->exportSamplerSetCue (*number, *setId, name);
        if (error.isNotEmpty())
            return ToolResult::error ("failed", error);
        auto result = std::make_unique<juce::DynamicObject>();
        result->setProperty ("input_id", *number);
        result->setProperty ("set_id", *setId);
        result->setProperty ("exported", true);
        return ToolResult::ok (juce::var (result.release()));
    };
    return d;
}

} // namespace WFSNetwork::Tools::Snapshot
