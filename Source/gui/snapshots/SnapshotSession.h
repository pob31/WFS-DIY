#pragma once

#include <JuceHeader.h>
#include <map>
#include "../../WfsParameters.h"
#include "../../Parameters/WFSFileManager.h"
#include "../../Parameters/WFSParameterIDs.h"
#include "../../Localization/LocalizationManager.h"
#include "../SnapshotScopeWindow.h"
#include "../ChannelIdentityGate.h"
#include "../DuplicateNameWarning.h"
#include "../ScreenShareRendering.h"

//==============================================================================
/**
    The snapshot row's model and actions, shared by every tab that shows the row.

    One snapshot file carries the inputs AND the effects (plan revision 8), so
    the Inputs tab and the Effects tab each show the same row - selector, Store,
    Reload, Reload w/o Scope, Update, Edit Scope, Delete - over ONE session: one
    selected snapshot, one session scope for the next store, one scope cache and
    one Scope window. A selection made on either tab is the selection on both.

    This is the Inputs tab's former snapshot code moved out whole, minus the
    widgets (SnapshotRow draws them). MainComponent owns the one instance and
    wires its callbacks where it used to wire the Inputs tab's.

    LIFETIME. MainComponent destroys its tabs (and their rows) AFTER its own
    WfsParameters, so this object is declared before the tab container, outlives
    every row, and never touches the parameters in its destructor: shutdown(),
    called early in ~MainComponent, closes the Scope window while they still
    exist.
*/
class SnapshotSession
{
public:
    using Scope  = WFSFileManager::ExtendedSnapshotScope;
    using Family = WFSFileManager::SnapshotFamily;

    /** A row showing this session. */
    struct Listener
    {
        virtual ~Listener() = default;

        /** The list, the selection or what the selection allows has changed. */
        virtual void snapshotSessionChanged() = 0;

        /** A cue moved the selection: drop any snapshot button being held, and
            say whether one was. Reload, Update and Delete act on the selection
            when released, so a cue landing mid-press would otherwise turn the
            operator's Delete of one snapshot into a Delete of the cue's. */
        virtual bool cancelHeldSnapshotPresses() = 0;
    };

    explicit SnapshotSession (WfsParameters& p) : parameters (p) {}

    ~SnapshotSession() = default;

    /** Close the Scope window while the parameters still exist. */
    void shutdown() { scopeWindow.reset(); }

    void addListener (Listener* l)    { listeners.add (l); }
    void removeListener (Listener* l) { listeners.remove (l); }

    //==========================================================================
    // What MainComponent wires (they were the Inputs tab's)
    //==========================================================================

    /** Recall through MainComponent's single recall seam, shared with the OSC
        address /wfs/input/snapshot/load and the MIDI note trigger. */
    std::function<void (const juce::String& snapshotName)> onSnapshotRecallRequested;

    /** Fired after any snapshot is created, updated, deleted, or has its scope
        rewritten - the MIDI binding index rebuilds from this. */
    std::function<void()> onSnapshotsChanged;

    /** QLab export after a store / update in Write-to-QLab mode. */
    std::function<void (const juce::String& snapshotName, const Scope& scope)> onQLabExportRequested;

    /** Whether a QLab target is configured. */
    std::function<bool()> isQLabAvailable;

    /** Create a QLab cue that loads this snapshot over OSC. */
    std::function<void (const juce::String& snapshotName)> onQLabSnapshotLoadCueRequested;

    /** After a load that bypassed the recall seam (Reload w/o Scope). */
    std::function<void()> onConfigReloaded;

    /** After an identity dialog changed the channel list structurally. */
    std::function<void()> onStructureChanged;

    /** The status bar. */
    std::function<void (const juce::String&)> showStatus;

    //==========================================================================
    // State the rows read
    //==========================================================================

    juce::StringArray getSnapshotNames() const { return parameters.getFileManager().getInputSnapshotNames(); }

    /** The selected snapshot, or empty. */
    const juce::String& getSelected() const noexcept { return selected; }
    bool hasSelection() const noexcept { return selected.isNotEmpty(); }

    /** "Reload Without Scope" only makes sense for snapshots with a read-time
        (OnRecall) scope: OnSave snapshots already hold filtered data. */
    bool canReloadWithoutScope()
    {
        if (! hasSelection())
            return false;

        if (scopes.find (selected) == scopes.end())
            scopes[selected] = parameters.getFileManager().getExtendedSnapshotScope (selected);

        return scopes[selected].applyMode == Scope::ApplyMode::OnRecall;
    }

    //==========================================================================
    // Selection and list
    //==========================================================================

    /** A row's selector picked `name` (empty = the placeholder). */
    void select (const juce::String& name)
    {
        if (selected == name)
            return;

        selected = name;
        notify();
    }

    /** Re-read the snapshot folder. The selection survives if its snapshot still
        exists; otherwise it falls back to none, so a row is never left showing
        a name that is gone. */
    void refreshList()
    {
        if (selected.isNotEmpty() && ! getSnapshotNames().contains (selected))
            selected.clear();

        notify();
    }

    /** Mirror an externally triggered recall in every row. Returns true when that
        cancelled a snapshot button being held. */
    bool selectFromExternalRecall (const juce::String& snapshotName)
    {
        bool cancelled = false;

        if (selected != snapshotName)
            listeners.call ([&cancelled] (Listener& l) { cancelled = l.cancelHeldSnapshotPresses() || cancelled; });

        selected = snapshotName;
        refreshList();
        return cancelled;
    }

    /** The cached scopes describe the previous project's files. */
    void projectFolderChanged()
    {
        scopes.clear();
        juce::MessageManager::callAsync ([weak = juce::WeakReference<SnapshotSession> (this)]
        {
            if (weak != nullptr)
                weak->refreshList();
        });
    }

    /** Restore the scope window's two QLab toggles from the show config. */
    void restoreQLabToggles()
    {
        auto showSection = parameters.getValueTreeState().getConfigState()
                               .getChildWithName (WFSParameterIDs::Show);
        if (showSection.isValid())
        {
            writeToQLabEnabled = static_cast<bool> (showSection.getProperty (WFSParameterIDs::writeToQLab, false));
            writeSnapshotLoadCueEnabled = static_cast<bool> (showSection.getProperty (WFSParameterIDs::writeSnapshotLoadCue, false));
        }
    }

    //==========================================================================
    // Actions (the row's buttons)
    //==========================================================================

    void store (juce::Component* parent)
    {
        juce::ignoreUnused (parent);

        auto& fileManager = parameters.getFileManager();
        if (! fileManager.hasValidProjectFolder())
        {
            status (LOC ("inputs.messages.selectFolderFirst"));
            return;
        }

        auto defaultName = WFSFileManager::getDefaultSnapshotName();

        auto* dialog = new juce::AlertWindow (LOC ("inputs.dialogs.storeSnapshotTitle"),
                                              LOC ("inputs.dialogs.storeSnapshotMessage"),
                                              juce::MessageBoxIconType::NoIcon);

        dialog->addTextEditor ("name", defaultName, LOC ("inputs.dialogs.snapshotNameLabel"));
        dialog->addButton (LOC ("common.ok"), 1, juce::KeyPress (juce::KeyPress::returnKey));
        dialog->addButton (LOC ("common.cancel"), 0, juce::KeyPress (juce::KeyPress::escapeKey));

        // Warn (red field + message) if the name would overwrite an existing snapshot
        auto warning = DuplicateNameWarning::attach (*dialog, "name",
                                                     fileManager.getInputSnapshotNames(),
                                                     LOC ("inputs.dialogs.snapshotOverwriteWarning"));

        ScreenShareRendering::apply (*dialog);
        dialog->enterModalState (true, juce::ModalCallbackFunction::create (
            [weak = juce::WeakReference<SnapshotSession> (this), dialog, warning] (int result)
            {
                if (result == 1 && weak != nullptr)
                {
                    auto name = dialog->getTextEditorContents ("name");
                    if (name.isNotEmpty())
                        weak->storeNamed (name);
                }
                delete dialog;
            }), true);
    }

    void reload (juce::Component* parent)
    {
        if (! hasSelection())
        {
            status (LOC ("inputs.messages.noSnapshotSelected"));
            return;
        }

        // One recall path for UI, OSC and MIDI. The seam re-reads the scope from
        // disk instead of using the cache - the safer of the two, since the cache
        // goes stale if the file is edited outside the app.
        //
        // This is the MANUAL path, so it may ask: a snapshot whose hardware
        // fingerprint disagrees with the live patch was stored under a
        // different configuration (or the rig was re-cabled), and the operator
        // gets to fix the numbers, proceed on purpose, or stop. Cue-driven
        // recalls reach onSnapshotRecallRequested directly and never block.
        const auto name = selected;
        ChannelIdentityGate::confirmThenRecall (makeChannelIdentityContext (parent), name,
            [weak = juce::WeakReference<SnapshotSession> (this), name]
            {
                if (weak != nullptr && weak->onSnapshotRecallRequested)
                    weak->onSnapshotRecallRequested (name);
            });
    }

    void reloadWithoutScope (juce::Component* parent)
    {
        if (! hasSelection())
        {
            status (LOC ("inputs.messages.noSnapshotSelected"));
            return;
        }

        const auto name = selected;
        ChannelIdentityGate::confirmThenRecall (makeChannelIdentityContext (parent), name,
            [weak = juce::WeakReference<SnapshotSession> (this), name]
            {
                if (weak == nullptr)
                    return;

                auto& fm = weak->parameters.getFileManager();

                // A default scope (all included, both families) bypasses the filter
                Scope noScope;

                weak->parameters.getDirtyTracker().beginSuppression();

                if (fm.loadInputSnapshotWithExtendedScope (name, noScope))
                {
                    weak->status (LOC ("inputs.messages.snapshotLoadedWithoutScope").replace ("{name}", name));

                    // Entries with no live channel used to vanish silently.
                    const auto& skipped = fm.getLastRecallSkippedNumbers();
                    if (! skipped.empty())
                    {
                        juce::StringArray nums;
                        for (int n : skipped) nums.add ("#" + juce::String (n));
                        weak->status (LOC ("inputs.messages.snapshotEntriesSkipped")
                                          .replace ("{name}", name)
                                          .replace ("{n}", juce::String ((int) skipped.size()))
                                          .replace ("{numbers}", nums.joinIntoString (", ")));
                    }

                    const auto& skippedEffects = fm.getLastRecallSkippedEffectIds();
                    if (! skippedEffects.empty())
                    {
                        juce::StringArray ids;
                        for (int n : skippedEffects) ids.add (juce::String (n));
                        weak->status (LOC ("inputs.messages.snapshotEffectsSkipped")
                                          .replace ("{name}", name)
                                          .replace ("{n}", juce::String ((int) skippedEffects.size()))
                                          .replace ("{ids}", ids.joinIntoString (", ")));
                    }

                    // Refreshes every tab from the tree, the Inputs and Effects tabs included.
                    if (weak->onConfigReloaded)
                        weak->onConfigReloaded();
                }
                else
                {
                    weak->status (LOC ("inputs.messages.error").replace ("{error}", fm.getLastError()));
                }

                weak->parameters.getDirtyTracker().endSuppressionAndClear();
            });
    }

    void update()
    {
        if (! hasSelection())
        {
            status (LOC ("inputs.messages.noSnapshotSelected"));
            return;
        }

        const auto name = selected;
        auto& fileManager = parameters.getFileManager();

        // Always from disk: this writes the scope and the MIDI binding back
        // into the file, and a cached copy goes stale -- after a project switch
        // it is another project's same-named snapshot (its note included, past
        // the Scope window's conflict check), and after a channel delete or
        // reorder its slot-keyed grid lands on other channels.
        scopes[name] = fileManager.getExtendedSnapshotScope (name);
        auto& scope = scopes[name];

        auto file = fileManager.getInputSnapshotsFolder().getChildFile (name + WFSFileManager::snapshotExtension);

        if (writeToQLabEnabled)
        {
            // Save snapshot first — onQLabExportRequested reads XML from disk
            fileManager.createBackup (file);
            fileManager.saveInputSnapshotWithExtendedScope (name, scope);
            if (onQLabExportRequested)
                onQLabExportRequested (name, scope);
            parameters.getDirtyTracker().clearAll();
        }
        else
        {
            fileManager.createBackup (file);

            if (fileManager.saveInputSnapshotWithExtendedScope (name, scope))
            {
                parameters.getDirtyTracker().clearAll();
                status (LOC ("inputs.messages.snapshotUpdated").replace ("{name}", name));

                if (onSnapshotsChanged)
                    onSnapshotsChanged();

                if (writeSnapshotLoadCueEnabled && onQLabSnapshotLoadCueRequested)
                    onQLabSnapshotLoadCueRequested (name);
            }
            else
            {
                status (LOC ("inputs.messages.error").replace ("{error}", fileManager.getLastError()));
            }
        }
    }

    void remove()
    {
        if (! hasSelection())
        {
            status (LOC ("inputs.messages.noSnapshotSelected"));
            return;
        }

        const auto name = selected;
        auto& fileManager = parameters.getFileManager();

        if (fileManager.deleteInputSnapshot (name))
        {
            scopes.erase (name);
            refreshList();

            if (onSnapshotsChanged)
                onSnapshotsChanged();
            status (LOC ("inputs.messages.snapshotDeleted").replace ("{name}", name));
        }
        else
        {
            status (LOC ("inputs.messages.error").replace ("{error}", fileManager.getLastError()));
        }
    }

    /** Open the Scope window on `family`'s grid, or bring the open one to front
        and switch it there. */
    void editScope (Family family)
    {
        const auto name = selected;
        const bool hasSelectedSnapshot = hasSelection();
        auto& fileManager = parameters.getFileManager();

        // Determine which scope to edit
        Scope* scopePtr = nullptr;
        juce::String windowTitle;

        if (hasSelectedSnapshot)
        {
            // Always from disk, for the reason given in update(): the window can
            // write this scope and its MIDI binding back.
            scopes[name] = fileManager.getExtendedSnapshotScope (name);
            scopePtr = &scopes[name];
            windowTitle = name;
        }
        else
        {
            // The session scope (for new snapshots)
            if (! currentScopeInitialized)
            {
                currentScope.initializeDefaults (parameters.getNumInputChannels());
                currentScopeInitialized = true;
            }
            scopePtr = &currentScope;
            windowTitle = "(New Snapshot)";
        }

        if (scopeWindow != nullptr && scopeWindow->isVisible())
        {
            // Already open (from this tab's row or the other's): show the grid
            // the button belongs to.
            scopeWindow->selectFamily (family);
            scopeWindow->toFront (true);
            return;
        }

        // The window edits a working copy; the close result decides its fate:
        // OK keeps it for the session (new snapshots), the long-press "Update
        // Snapshot Scope" button writes it into the selected snapshot's file,
        // Cancel/X discards it.
        // shared_ptr so the lambda remains copy-constructible for std::function.
        auto working = std::make_shared<Scope> (*scopePtr);

        // The binding as the window opened, to tell whether OK is about to
        // drop a note the operator just set.
        const int openedMidiChannel = scopePtr->midiChannel;
        const int openedMidiNote    = scopePtr->midiNote;

        scopeWindow = std::make_unique<SnapshotScopeWindow> (parameters, windowTitle, *working, hasSelectedSnapshot,
                                                             &parameters.getDirtyTracker(), family);
        scopeWindow->setQLabAvailable (isQLabAvailable ? isQLabAvailable() : false);
        // A named pointer: in a nested lambda's init-capture MSVC resolves a
        // bare `this` to the enclosing closure.
        SnapshotSession* const self = this;

        scopeWindow->onWindowClosed =
            [this, self, name, working, hasSelectedSnapshot, openedMidiChannel, openedMidiNote]
            (SnapshotScopeWindow::CloseResult result, bool writeToQLab, bool writeLoadCue)
        {
            using CloseResult = SnapshotScopeWindow::CloseResult;

            // The QLab toggles are adopted and persisted by OK and by Update
            // only. Cancel / X hands back the window's defaults (both false),
            // not what the toggles showed, so adopting them turned Write to
            // QLab off behind the operator's back every time the window was
            // dismissed.
            if (result != CloseResult::Cancelled)
            {
                writeToQLabEnabled = writeToQLab;
                writeSnapshotLoadCueEnabled = writeLoadCue;

                auto showSection = parameters.getValueTreeState().getConfigState()
                                       .getChildWithName (WFSParameterIDs::Show);
                if (showSection.isValid())
                {
                    showSection.setProperty (WFSParameterIDs::writeToQLab, writeToQLab, nullptr);
                    showSection.setProperty (WFSParameterIDs::writeSnapshotLoadCue, writeLoadCue, nullptr);
                }
            }

            if (result == CloseResult::Saved)
            {
                // OK is session-only: the edited scope becomes the default for the
                // next "Store"; the selected snapshot's file and cached scope stay
                // untouched (the long-press button handles those).
                const bool droppedMidiEdit = hasSelectedSnapshot
                    && (working->midiChannel != openedMidiChannel || working->midiNote != openedMidiNote);

                currentScope = *working;
                // The grid is the reusable part of a scope; a MIDI binding
                // names ONE snapshot. Carrying it into the session default
                // would hand the next created snapshot the same note.
                currentScope.clearMidiBinding();
                currentScopeInitialized = true;

                // A note set in the window and closed with OK was dropped in
                // silence: say so, and where it is saved instead.
                status (LOC (droppedMidiEdit ? "inputs.messages.midiBindingNotSaved"
                                             : "inputs.messages.scopeConfigured"));
            }
            else if (result == CloseResult::ScopeUpdated)
            {
                // Long-press: write the scope into the snapshot file (with backup;
                // OnSave scopes also trim the stored values) and refresh the cache.
                auto& fm = parameters.getFileManager();
                if (fm.updateInputSnapshotScope (name, *working))
                {
                    scopes[name] = *working;
                    status (LOC ("inputs.messages.snapshotScopeUpdated").replace ("{name}", name));
                    notify();   // applyMode drives "Reload w/o Scope" enablement

                    // The scope carries the MIDI binding, so this is the
                    // normal way a note is armed or cleared.
                    if (onSnapshotsChanged)
                        onSnapshotsChanged();
                }
                else
                {
                    status (LOC ("inputs.messages.error").replace ("{error}", fm.getLastError()));
                }
            }
            // Cancel / X / any other close: do nothing — the working copy is
            // discarded together with this lambda when the window is destroyed.
            //
            // Defer the window destruction: onWindowClosed is called from deep
            // inside the OK button's click handler stack. Destroying the window
            // synchronously here tears down the button while its click handler is
            // still executing, which corrupts subsequent scope-window sessions.
            juce::MessageManager::callAsync ([weak = juce::WeakReference<SnapshotSession> (self)]
            {
                if (weak != nullptr)
                    weak->scopeWindow.reset();
            });
        };
    }

private:
    WfsParameters& parameters;
    juce::ListenerList<Listener> listeners;

    juce::String selected;

    std::unique_ptr<SnapshotScopeWindow> scopeWindow;
    std::map<juce::String, Scope> scopes;
    Scope currentScope;                          // the session default for the next store
    bool currentScopeInitialized = false;
    bool writeToQLabEnabled = false;             // the scope window's QLab radio
    bool writeSnapshotLoadCueEnabled = false;    // the scope window's QLab load-cue checkbox

    void notify() { listeners.call ([] (Listener& l) { l.snapshotSessionChanged(); }); }

    void status (const juce::String& text)
    {
        if (showStatus)
            showStatus (text);
    }

    ChannelIdentityGate::Context makeChannelIdentityContext (juce::Component* parent)
    {
        ChannelIdentityGate::Context ctx;
        ctx.parent     = parent;
        ctx.parameters = &parameters;
        ctx.afterStructuralChange = [weak = juce::WeakReference<SnapshotSession> (this)]
        {
            if (weak != nullptr && weak->onStructureChanged)
                weak->onStructureChanged();
        };
        ctx.showStatus = [weak = juce::WeakReference<SnapshotSession> (this)] (const juce::String& text)
        {
            if (weak != nullptr)
                weak->status (text);
        };
        return ctx;
    }

    /** The store dialog's OK: capture the session scope into `name`. */
    void storeNamed (const juce::String& name)
    {
        // Use the session scope if configured, otherwise a default one
        Scope scope;
        if (currentScopeInitialized)
        {
            scope = currentScope;
            // The grid is the reusable part of a scope; a MIDI binding names ONE
            // snapshot. Belt-and-braces - editScope already clears it on the way in.
            scope.clearMidiBinding();
        }
        else
        {
            scope.initializeDefaults (parameters.getNumInputChannels());
        }

        auto& fileManager = parameters.getFileManager();

        // Storing over an EXISTING name is a content replace, not a re-bind:
        // keep whatever note that snapshot already had, otherwise "Store"
        // silently disarms a live cue. The test asks the filesystem, like the
        // write does: on Windows and macOS "scene 3" overwrites "Scene 3.xml",
        // which a case-sensitive name lookup missed, dropping the note.
        if (fileManager.getInputSnapshotsFolder().getChildFile (name + WFSFileManager::snapshotExtension).existsAsFile())
        {
            auto existing = fileManager.getExtendedSnapshotScope (name);
            scope.midiChannel = existing.midiChannel;
            scope.midiNote    = existing.midiNote;
        }

        scopes[name] = scope;

        if (writeToQLabEnabled)
        {
            // Save snapshot first — onQLabExportRequested reads XML from disk
            if (fileManager.saveInputSnapshotWithExtendedScope (name, scope))
            {
                selected = name;
                refreshList();
                if (onSnapshotsChanged)
                    onSnapshotsChanged();
            }
            if (onQLabExportRequested)
                onQLabExportRequested (name, scope);
            parameters.getDirtyTracker().clearAll();
        }
        else
        {
            if (fileManager.saveInputSnapshotWithExtendedScope (name, scope))
            {
                parameters.getDirtyTracker().clearAll();
                selected = name;
                refreshList();
                status (LOC ("inputs.messages.snapshotStored").replace ("{name}", name));

                if (onSnapshotsChanged)
                    onSnapshotsChanged();

                if (writeSnapshotLoadCueEnabled && onQLabSnapshotLoadCueRequested)
                    onQLabSnapshotLoadCueRequested (name);
            }
            else
            {
                status (LOC ("inputs.messages.error").replace ("{error}", fileManager.getLastError()));
            }
        }
    }

    JUCE_DECLARE_WEAK_REFERENCEABLE (SnapshotSession)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SnapshotSession)
};
