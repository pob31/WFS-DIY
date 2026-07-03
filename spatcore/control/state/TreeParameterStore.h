#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <functional>
#include <memory>
#include <vector>

#include "../osc/OscTransportTypes.h"

namespace spatcore::control::state
{

/**
 * TreeParameterStore — app-agnostic ValueTree-backed parameter store.
 *
 * Extracted from the app's WFSValueTreeState (Phase 4c of the spatcore
 * extraction). Owns the mechanics that carry no app schema:
 *
 *  - the root juce::ValueTree + typed get/set accessors. Schema routing is
 *    delegated to the subclass through the pure-virtual getTreeForParameter()
 *    seam, so the core never names any app parameter or section.
 *  - the parameter-change callback registry + change-notification dispatch.
 *    Origin attribution propagates implicitly: writes and notifications are
 *    synchronous on the writing thread, so listeners read the thread-local
 *    spatcore::control::osc::getCurrentOriginTag() set by the caller's
 *    OriginTagScope.
 *  - the per-domain UndoManager array. The domain set is app data: the
 *    subclass passes the domain count (and optional names, for diagnostics)
 *    at construction — the core has no notion of what a domain means.
 *  - origin-aware undo suppression: MCP-origin writes bypass the JUCE
 *    UndoManager entirely (AI changes have a dedicated undo channel through
 *    the core MCP undo engine). Without this, all AI writes pile into one
 *    open JUCE transaction and a single Ctrl+Z reverts every AI change at
 *    once.
 *  - the POST-WRITE HOOK point (handlePostWrite): the subclass registers its
 *    semantic invariants (e.g. geometry constraints) there; they run after
 *    every property write, before listener dispatch.
 *
 * Everything schema-shaped — section builders, migrations, scope routing,
 * semantic invariants — stays in the app subclass.
 */
class TreeParameterStore : public juce::ValueTree::Listener
{
public:
    //==========================================================================
    // Construction / Destruction
    //==========================================================================

    /** @param numUndoDomains  Number of undo domains (>= 1). The subclass
     *                         addresses domains by index [0..count).
     *  @param domainNames     Optional human-readable domain names for
     *                         diagnostics; not interpreted by the core. */
    explicit TreeParameterStore (int numUndoDomains,
                                 const juce::StringArray& domainNames = {});
    ~TreeParameterStore() override;

    //==========================================================================
    // Root State Access
    //==========================================================================

    /** Get the root ValueTree state */
    juce::ValueTree getState() { return state; }
    const juce::ValueTree getState() const { return state; }

    //==========================================================================
    // Parameter Access - Type Safe
    //==========================================================================

    /** Get a float parameter value */
    float getFloatParameter (const juce::Identifier& id, int channelIndex = -1) const;

    /** Get an int parameter value */
    int getIntParameter (const juce::Identifier& id, int channelIndex = -1) const;

    /** Get a string parameter value */
    juce::String getStringParameter (const juce::Identifier& id, int channelIndex = -1) const;

    /** Get a var parameter value (generic) */
    juce::var getParameter (const juce::Identifier& id, int channelIndex = -1) const;

    /** Set a parameter value (undoable on the active domain). Virtual so the
     *  subclass can route schema-structural writes (e.g. channel counts)
     *  before falling through to this generic write. */
    virtual void setParameter (const juce::Identifier& id, const juce::var& value, int channelIndex = -1);

    /** Set a parameter value without undo */
    virtual void setParameterWithoutUndo (const juce::Identifier& id, const juce::var& value, int channelIndex = -1);

    //==========================================================================
    // Undo / Redo  (per-domain — one UndoManager per app-declared domain)
    //==========================================================================

    /** Set the currently active undo domain */
    void setActiveDomain (int domain);

    /** Get the currently active undo domain */
    int getActiveDomain() const;

    /** Get UndoManager for a specific domain */
    juce::UndoManager* getUndoManagerForDomain (int domain);

    /** Get UndoManager for the currently active domain.
     *  MCP-origin writes return nullptr (origin-aware undo suppression). */
    juce::UndoManager* getActiveUndoManager();

    /** Convenience: get UndoManager (returns active domain's manager) */
    juce::UndoManager* getUndoManager() { return getActiveUndoManager(); }

    /** Perform undo on the active domain */
    bool undo();

    /** Perform redo on the active domain */
    bool redo();

    /** Check if undo is available on the active domain */
    bool canUndo() const;

    /** Check if redo is available on the active domain */
    bool canRedo() const;

    /** Begin a new undo transaction on the active domain */
    void beginUndoTransaction (const juce::String& name);

    /** Clear undo history for the active domain */
    void clearUndoHistory();

    /** Clear undo history for ALL domains */
    void clearAllUndoHistories();

    /** Number of undo domains declared at construction */
    int getNumUndoDomains() const { return static_cast<int> (undoManagers.size()); }

    /** Domain names declared at construction (diagnostics only) */
    const juce::StringArray& getUndoDomainNames() const { return undoDomainNames; }

    /** RAII helper: temporarily switch the active undo domain, restoring on
     *  destruction. Subclasses typically shadow this with an enum-typed
     *  wrapper. */
    struct ScopedUndoDomain
    {
        ScopedUndoDomain (TreeParameterStore& s, int d)
            : store (s), previous (s.getActiveDomain()) { store.setActiveDomain (d); }
        ~ScopedUndoDomain() { store.setActiveDomain (previous); }
        TreeParameterStore& store;
        int previous;
    };

    //==========================================================================
    // Listener Management
    //==========================================================================

    /** Callback type for parameter changes */
    using ParameterCallback = std::function<void (const juce::var&)>;

    /** Add a listener for a specific parameter */
    void addParameterListener (const juce::Identifier& id, ParameterCallback callback, int channelIndex = -1);

    /** Remove listeners for a parameter */
    void removeParameterListeners (const juce::Identifier& id, int channelIndex = -1);

    /** Add a ValueTree listener */
    void addListener (juce::ValueTree::Listener* listener);

    /** Remove a ValueTree listener */
    void removeListener (juce::ValueTree::Listener* listener);

    //==========================================================================
    // ValueTree::Listener overrides
    //==========================================================================

    void valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
                                   const juce::Identifier& property) override;
    void valueTreeChildAdded (juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) override;
    void valueTreeChildRemoved (juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved,
                                int indexFromWhichChildWasRemoved) override;
    void valueTreeChildOrderChanged (juce::ValueTree& parentTreeWhoseChildrenHaveMoved,
                                     int oldIndex, int newIndex) override;
    void valueTreeParentChanged (juce::ValueTree& treeWhoseParentHasChanged) override;

protected:
    //==========================================================================
    // Subclass Seams
    //==========================================================================

    /** Find the ValueTree node that holds a given parameter — the schema
     *  routing seam. The app subclass implements this with its scope map. */
    virtual juce::ValueTree getTreeForParameter (const juce::Identifier& id, int channelIndex) const = 0;

    /** Derive the channel index for a changed node (app schema knows which
     *  node types are channels). Default: no channel (-1). */
    virtual int resolveChannelIndex (const juce::ValueTree& changedNode) const;

    /** POST-WRITE HOOK — runs on every property change, after the value has
     *  landed in the tree and BEFORE listener dispatch. The app subclass
     *  enforces its semantic invariants here (which may issue further
     *  writes; those re-enter this dispatch as they always did). Default:
     *  no-op. */
    virtual void handlePostWrite (juce::ValueTree& changedNode, const juce::Identifier& property,
                                  const juce::var& value, int channelIndex);

    /** Notify registered listeners of a parameter change */
    void notifyParameterListeners (const juce::Identifier& id, const juce::var& value, int channelIndex);

    //==========================================================================
    // Protected Members
    //==========================================================================

    /** Root tree. The subclass builds its default schema into this. */
    juce::ValueTree state;

private:
    //==========================================================================
    // Private Members
    //==========================================================================

    std::vector<std::unique_ptr<juce::UndoManager>> undoManagers;
    juce::StringArray undoDomainNames;
    int activeDomain = 0;

    // Listener management
    struct ListenerEntry
    {
        juce::Identifier parameterId;
        int channelIndex;
        ParameterCallback callback;
    };
    std::vector<ListenerEntry> parameterListeners;
    juce::CriticalSection listenerLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TreeParameterStore)
};

} // namespace spatcore::control::state
