#pragma once

#include <JuceHeader.h>
#include "WFSParameterIDs.h"
#include "WFSParameterDefaults.h"
#include "../Helpers/ReverbNodePlacement.h"
#include "../../spatcore/control/state/TreeParameterStore.h"
#include "InputChannelIdentity.h"
#include <vector>

/**
 * Undo domain — each tab has its own undo history.
 */
enum class UndoDomain
{
    Input,      // InputsTab
    Output,     // OutputsTab + OutputArrayHelperWindow
    Reverb,     // ReverbTab
    Map,        // MapTab (input positions via map drag)
    Config,     // SystemConfigTab + NetworkTab
    Clusters,   // ClustersTab
    COUNT
};

/**
 * WFS ValueTree State Manager
 *
 * Central management class for all WFS processor parameters using JUCE ValueTree.
 * Derives from spatcore::control::state::TreeParameterStore, which owns the
 * app-agnostic mechanics (root tree, typed get/set, listener registry +
 * change-notification dispatch, per-domain UndoManager array, origin-aware
 * undo suppression, post-write hook). This class supplies everything
 * WFS-schema-shaped:
 * - Hierarchical parameter organization (section builders + accessors)
 * - Scope routing (getTreeForParameter / getParameterScope)
 * - Semantic invariants (cluster shared-position, tracking uniqueness),
 *   registered into the core post-write hook
 * - The six WFS tab undo domains (UndoDomain), mapped onto the core's
 *   integer domain indices
 */
class WFSValueTreeState : public spatcore::control::state::TreeParameterStore
{
public:
    //==========================================================================
    // Construction / Destruction
    //==========================================================================

    WFSValueTreeState();
    ~WFSValueTreeState() override;

    //==========================================================================
    // State Access
    //==========================================================================

    /** Get config section */
    juce::ValueTree getConfigState();
    juce::ValueTree getConfigState() const;
    juce::ValueTree getShowState();
    juce::ValueTree getIOState();
    juce::ValueTree getStageState();
    juce::ValueTree getMasterState();
    juce::ValueTree getNetworkState();
    juce::ValueTree getADMOSCState();

    /** One ADM-OSC mapping, by its `id` property rather than child position —
        Cartesian and Polar mappings are siblings under <ADMOSC>, so an ordinal
        would run off the end of one kind into the other. Both are numbered 0-3
        within their own kind. */
    juce::ValueTree getADMCartMapping (int mappingIndex);
    juce::ValueTree getADMPolarMapping (int mappingIndex);

    /** One axis of a Cartesian mapping, by `admCartAxisId` (0=X, 1=Y, 2=Z). */
    juce::ValueTree getADMCartAxis (int mappingIndex, int axisIndex);
    juce::ValueTree getTrackingState();
    juce::ValueTree getClustersState();
    juce::ValueTree getClustersState() const;
    juce::ValueTree getClusterState (int clusterIndex);
    juce::ValueTree getClusterLFOSection (int clusterIndex);
    juce::ValueTree getClusterLFOPresetsSection();
    juce::ValueTree ensureClusterLFOPreset (int presetIndex);

    /** Recall a cluster LFO preset into a cluster (applies non-shape props first, then shapes) */
    void recallClusterLFOPreset (int clusterId, int presetIndex);

    juce::ValueTree getBinauralState();
    juce::ValueTree getBinauralState() const;

    /** Get input/output states */
    juce::ValueTree getInputsState();
    juce::ValueTree getInputsState() const;
    juce::ValueTree getInputState (int channelIndex);
    juce::ValueTree getOutputsState();
    juce::ValueTree getOutputsState() const;
    juce::ValueTree getOutputState (int channelIndex);

    /** Get reverb states */
    juce::ValueTree getReverbsState();
    juce::ValueTree getReverbsState() const;
    juce::ValueTree getReverbState (int channelIndex);

    /** Get audio patch state */
    juce::ValueTree getAudioPatchState();

    //==========================================================================
    // Parameter Access - Type Safe
    //==========================================================================
    // Typed getters (getFloatParameter / getIntParameter / getStringParameter /
    // getParameter) are inherited from TreeParameterStore; they resolve through
    // this class's getTreeForParameter override.

    /** Set a parameter value (routes channel-count writes to setNumXChannels) */
    void setParameter (const juce::Identifier& id, const juce::var& value, int channelIndex = -1) override;

    /** Set a parameter value without undo */
    void setParameterWithoutUndo (const juce::Identifier& id, const juce::var& value, int channelIndex = -1) override;

    /** Would a setParameter write with these arguments actually land anywhere?

        TreeParameterStore::setParameter is `if (tree.isValid()) write(...)` with no
        else and a void return, so a parameter this class cannot resolve is dropped
        in silence — the caller sees nothing at all. Every remote surface needs to
        be able to ask BEFORE writing, so it can report an error instead of a
        success it did not earn. Answers for the generic path only: callers that
        write a subtree directly (EQ bands today) never consult this. */
    bool canWriteParameter (const juce::Identifier& id, int channelIndex = -1) const;

    /** Is the audio engine running? (`runDSP` on <IO>, what the GUI calls
        "ProcessingEnabled".) Structural channel edits are stopped-only, and until
        now that rule was enforced nowhere but by greying the System Config fields —
        so any remote surface could restructure channels mid-show. */
    bool isProcessingEnabled() const;

    //==========================================================================
    // Input Channel Access
    //==========================================================================

    /** Get input channel parameter */
    juce::var getInputParameter (int channelIndex, const juce::Identifier& id) const;

    /** Set input channel parameter */
    void setInputParameter (int channelIndex, const juce::Identifier& id, const juce::var& value);

    /** Get the ValueTree for a specific input channel subsection */
    juce::ValueTree getInputChannelSection (int channelIndex);
    juce::ValueTree getInputPositionSection (int channelIndex);
    juce::ValueTree getInputAttenuationSection (int channelIndex);
    juce::ValueTree getInputDirectivitySection (int channelIndex);
    juce::ValueTree getInputLiveSourceSection (int channelIndex);
    juce::ValueTree getInputHackousticsSection (int channelIndex);
    juce::ValueTree getInputLFOSection (int channelIndex);
    juce::ValueTree getInputAutoMotionSection (int channelIndex);
    juce::ValueTree getInputMutesSection (int channelIndex);
    juce::ValueTree getInputGradientMapsSection (int channelIndex);
    juce::ValueTree getInputGradientLayer (int channelIndex, int layerIndex);

    /** One shape within a gradient-map layer, by position in draw order. A layer
        starts with none — they are created in the gradient-map editor — so an
        invalid tree here means "no such shape", not "no such layer". */
    juce::ValueTree getInputGradientShape (int channelIndex, int layerIndex, int shapeIndex);

    /** Ensure a GradientMaps section exists for a given input (migration helper) */
    juce::ValueTree ensureInputGradientMapsSection (int channelIndex);

    /** Get the Sampler section for a given input channel */
    juce::ValueTree getInputSamplerSection (int channelIndex);

    /** Ensure a Sampler section exists for a given input (migration helper) */
    juce::ValueTree ensureInputSamplerSection (int channelIndex);

    /** One sampler cell of the 6x6 grid, matched on its `id` property (0-35).
        Cells are pre-created and never added or removed, so the id is stable. */
    juce::ValueTree getInputSamplerCell (int channelIndex, int cellIndex);

    /** One sampler set, by ORDINAL among the SamplerSet children — deliberately
        not by its `id`, because deleting a set does not renumber the survivors,
        so ids go stale while positions do not. Every existing consumer counts
        the nth SamplerSet child; this is that walk, once. A fresh input has no
        sets at all, so an invalid tree here means "no set at that index". */
    juce::ValueTree getInputSamplerSet (int channelIndex, int setIndex);

    //==========================================================================
    // Output Channel Access
    //==========================================================================

    /** Get output channel parameter */
    juce::var getOutputParameter (int channelIndex, const juce::Identifier& id) const;

    /** Set output channel parameter */
    void setOutputParameter (int channelIndex, const juce::Identifier& id, const juce::var& value);

    /** Set output channel parameter with array propagation
     *  If the output is part of an array and applyToArray is enabled,
     *  propagates the change to other array members.
     *  @param channelIndex The output channel being modified (0-based)
     *  @param id The parameter identifier
     *  @param value The new value
     *  @param propagateToArray If true, considers array linking mode (default true)
     */
    void setOutputParameterWithArrayPropagation (int channelIndex,
                                                  const juce::Identifier& id,
                                                  const juce::var& value,
                                                  bool propagateToArray = true);

    /** Set output EQ band parameter with array propagation
     *  @param propagateToArray If true, considers array linking mode (default true)
     */
    void setOutputEQBandParameterWithArrayPropagation (int channelIndex,
                                                        int bandIndex,
                                                        const juce::Identifier& id,
                                                        const juce::var& value,
                                                        bool propagateToArray = true);

    /** Check if a parameter is array-linked (should propagate to array members) */
    static bool isArrayLinkedParameter (const juce::Identifier& paramId);

    /** Check if an EQ band parameter is array-linked */
    static bool isArrayLinkedEQParameter (const juce::Identifier& paramId);

    /** Check if an output parameter is an on/off toggle (no meaningful relative delta) */
    static bool isBooleanOutputParameter (const juce::Identifier& paramId);

    /** Get the ValueTree for a specific output channel subsection */
    juce::ValueTree getOutputChannelSection (int channelIndex);
    juce::ValueTree getOutputPositionSection (int channelIndex);
    juce::ValueTree getOutputOptionsSection (int channelIndex);
    juce::ValueTree getOutputEQSection (int channelIndex);
    juce::ValueTree getOutputEQBand (int channelIndex, int bandIndex);

    //==========================================================================
    // Reverb Channel Access
    //==========================================================================

    /** Get reverb channel parameter */
    juce::var getReverbParameter (int channelIndex, const juce::Identifier& id) const;

    /** Set reverb channel parameter */
    void setReverbParameter (int channelIndex, const juce::Identifier& id, const juce::var& value);

    /** Get the ValueTree for a specific reverb channel subsection */
    juce::ValueTree getReverbChannelSection (int channelIndex);
    juce::ValueTree getReverbPositionSection (int channelIndex);
    juce::ValueTree getReverbFeedSection (int channelIndex);
    juce::ValueTree getReverbEQSection (int channelIndex);
    juce::ValueTree ensureReverbEQSection (int channelIndex);  // Creates if missing
    juce::ValueTree getReverbEQBand (int channelIndex, int bandIndex);
    juce::ValueTree getReverbReturnSection (int channelIndex);

    /** Get the global reverb algorithm section (child of Reverbs node) */
    juce::ValueTree getReverbAlgorithmSection();
    juce::ValueTree ensureReverbAlgorithmSection();  // Creates if missing

    /** Get the global reverb pre-compressor section (child of Reverbs node) */
    juce::ValueTree getReverbPreCompSection();
    juce::ValueTree ensureReverbPreCompSection();  // Creates if missing

    /** Get the global reverb post-processing EQ section (child of Reverbs node) */
    juce::ValueTree getReverbPostEQSection();
    juce::ValueTree ensureReverbPostEQSection();  // Creates if missing
    juce::ValueTree getReverbPostEQBand (int bandIndex);

    /** Get the global reverb post-expander section (child of Reverbs node) */
    juce::ValueTree getReverbPostExpSection();
    juce::ValueTree ensureReverbPostExpSection();  // Creates if missing

    //==========================================================================
    // Cluster Access
    //==========================================================================

    /** Get cluster parameter (1-based cluster index) */
    juce::var getClusterParameter (int clusterIndex, const juce::Identifier& id) const;

    /** Set cluster parameter (1-based cluster index) */
    void setClusterParameter (int clusterIndex, const juce::Identifier& id, const juce::var& value);

    /** If sourceInputIndex belongs to a cluster whose referenceMode == 2 (Shared
        Position), copy the source input's inputPositionX/Y/Z to every other
        member so the shared-position invariant is maintained. No-op for inputs
        not in a Shared-mode cluster. */
    void propagateSharedClusterPosition (int sourceInputIndex);

    /** Enforce the shared-position invariant on a cluster (1-based clusterIndex):
        if referenceMode == 2 and the cluster has at least one member, snap every
        member's position to the first-ordered member's position. Idempotent. */
    void enforceSharedClusterInvariant (int clusterIndex);

    /** Enforce the shared-position invariant on every cluster. Needed after any
        bulk state apply (project load, config import, snapshot recall): those
        paths write positions via raw setProperty, bypassing the per-write
        propagation, so a Shared-mode cluster saved by an older version (or with
        diverged member positions in the file) would otherwise load diverged. */
    void enforceAllSharedClusterInvariants();

    //==========================================================================
    // Binaural Enable/Solo Access
    //==========================================================================

    /** Get binaural processing enabled state */
    bool getBinauralEnabled() const;

    /** Set binaural processing enabled state */
    void setBinauralEnabled (bool isEnabled);

    /** Get binaural solo mode (0=Single, 1=Multi) */
    int getBinauralSoloMode() const;

    /** Set binaural solo mode */
    void setBinauralSoloMode (int mode);

    /** Check if an input is soloed */
    bool isInputSoloed (int inputIndex) const;

    /** Set input solo state */
    void setInputSoloed (int inputIndex, bool soloed);

    /** Clear all input solo states */
    void clearAllSoloStates();

    /** Get number of currently soloed inputs */
    int getNumSoloedInputs() const;

    /** Get binaural output channel (-1 = disabled) */
    int getBinauralOutputChannel() const;

    /** Set binaural output channel */
    void setBinauralOutputChannel (int channel);

    //==========================================================================
    // Network Target Access
    //==========================================================================

    /** Get number of network targets */
    int getNumNetworkTargets() const;

    /** Add a network target */
    void addNetworkTarget();

    /** Remove a network target */
    void removeNetworkTarget (int targetIndex);

    /** Get network target state */
    juce::ValueTree getNetworkTargetState (int targetIndex);

    //==========================================================================
    // Channel Management
    //==========================================================================

    /** Get current channel counts */
    int getNumInputChannels() const;
    int getNumOutputChannels() const;
    int getNumReverbChannels() const;

    /** Set channel counts. For inputs this is the blunt legacy entry point:
        growth appends default mono channels, reduction removes the
        highest-numbered channels; nothing in the middle ever moves. Not
        undoable.

        Its only remaining caller is the CONFIG LOAD path (WFSFileManager, for
        files written before the channel inventory existed). MCP used to land
        here too, via a setParameter(inputChannels) re-route, which is why its
        removal rule disagreed with the GUI's; the two hand-written count tools
        now call setInputChannelCounts instead. */
    void setNumInputChannels (int numChannels);

    /** Number of live stereo channels — derived from the per-channel type
        (inputChannelType), NOT positional. Mono count =
        getNumInputChannels() − this. */
    int getNumStereoInputChannels() const;

    /** Two-count entry point (System Config count fields): a thin loop over the
        structural ops. Additions append after the last channel; reductions
        remove the LAST channel of that type in DISPLAY order - the bottom of the
        Arrange list, which the operator can see - never the highest-numbered
        one, which after a drag on a latched list may sit anywhere. (The
        one-count setNumInputChannels, now the config-load path only, does
        remove by number; the asymmetry is noted there.) Not undoable. */
    void setInputChannelCounts (int numMono, int numStereo);

    //==========================================================================
    // Stable channel numbers + per-channel type (stable-number rework)
    //==========================================================================
    // Every <Input> carries a permanent 1-based number (its `id` property) and
    // a type (`inputChannelType`: "mono"/"stereo"). The number is the external
    // address (OSC, snapshots, QLab cues, DAW plugin, MCP); the child index
    // ("slot") stays the internal dense key (patch row, render-source slot,
    // meter row). Tree order is the user's DISPLAY order: new channels append
    // at the end, drag-to-reorder moves a node (and its patch row) to a new
    // slot — numbers never change once the session has latched them, so no
    // external reference can break. Before the latch (a fresh session) the
    // structural ops renumber to dense display order instead — see
    // `channelNumbersUserOwned` in WFSParameterIDs.h.

    /** Permanent channel number of the channel at a slot (0-based child
        index); 0 if the slot is invalid. */
    int getInputChannelNumber (int slot) const;

    /** Slot holding a permanent channel number; -1 if no live channel has it. */
    int getSlotForChannelNumber (int number) const;

    /** Type of the channel at a slot (reads inputChannelType; absent = mono). */
    bool isInputChannelStereo (int slot) const;

    /** Highest live channel number (0 when the list is empty). */
    int getHighestChannelNumber() const;

    /** Number the next added channel gets (highest live + 1). */
    int getNextChannelNumber() const;

    /** Lowest number in 1..maxInputChannels with no live channel (0 if the
        list is full). Used by the number-exhaustion gap-reuse dialog. */
    int getLowestFreeChannelNumber() const;

    //==========================================================================
    // Structural channel ops (stable-number model)
    //==========================================================================
    // Each op edits the channel tree AND its input-patch row together, keeps
    // the count properties in step, and clears the undo history (structural
    // edits are deliberately not undoable: ValueTree undo cannot span the
    // flat patchData string safely). The caller runs the reconfiguration
    // pass afterwards (MainComponent::handleChannelCountChange tail:
    // recompute render sources, sanitize/auto-patch, reload patches).

    /** Append a channel (number = highest + 1) or, with explicitNumber > 0,
        re-create a retired number in its sorted slot (the UI confirms gap
        reuse with the user first). New channel's patch row continues the
        diagonal past everything already patched (two columns for stereo) —
        that is the LATCHED behaviour, and it follows CREATION order. Until the
        session latches channel numbers, the list is renumbered to display order
        afterwards and the whole patch is re-flowed with it, into a gapless
        diagonal in DISPLAY order. Its default name is one past the highest
        "Mono n" / "Stereo n" ordinal any live name already claims, so a latched
        session — which never resequences — cannot end up with two channels
        sharing a default name. */
    juce::Result addInputChannel (bool stereo, int explicitNumber = 0);

    /** Remove a live channel by permanent number. Its number is retired
        (gap); every other channel keeps its number, slot and patch row. Until
        the session latches channel numbers, the gap is closed by a renumber to
        display order instead. */
    juce::Result removeInputChannel (int channelNumber);

    /** Flip a live channel's type in place. NOT exposed in the UI or MCP —
        the channel's data (patch columns, width, decomposition) cannot
        meaningfully follow a type change, so composition is edited through
        the counts instead. Kept for the self-test and internal use. */
    juce::Result setInputChannelType (int channelNumber, bool stereo);

    /** Move a live channel to a new display slot (drag-to-reorder). The
        channel node and its patch row move together; the permanent number is
        untouched once the session is latched — before that the list is
        renumbered so the numbers follow the new display order. Stopped-only,
        like every structural edit. */
    juce::Result moveInputChannel (int channelNumber, int targetSlot);

    /** Move a channel node TOGETHER with its patch row and remap the slot-keyed
        cluster orders - the live-drag contract. Shared by moveInputChannel and
        by the display-order restore in WFSFileManager::applyInputsSection, which
        used to raw-move the node alone: correct for a complete load, where the
        file's own patch overwrites every row afterwards, and silently wrong for
        Reload Input Config on its own, which loads no patch and left the live
        rows behind. No latch, no undo bookkeeping - callers own both. */
    void moveInputChannelNodeAndRow (int fromSlot, int toSlot);

    //==========================================================================
    // Channel identity (pre-load check + reconciliation)
    //==========================================================================

    /** 1-based hardware inputs the channel at `slot` holds in its patch row,
        ascending (lower = L for a stereo pair); 0..2 entries. Nothing at tree
        level answered this before - MainComponent derived it at runtime - and
        the saved fingerprint needs it. */
    std::vector<int> getInputPatchHardwareInputs (int slot) const;

    /** The live session's identity: every channel's (slot, number, name, type,
        hardware inputs) in display order. A superset of
        buildInputChannelInventory, which stays the file format. Pure. */
    InputChannelIdentity getInputChannelIdentity() const;

    /** RELABEL - give the channel at each slot the number numbersBySlot[slot].
        The operator's "one-time reorder of the internal channel ID", by
        position: every channel keeps its place, settings, name and hardware
        inputs; only its NUMBER changes, so snapshots, cues and OSC written
        against the file's numbers reach the right channels afterwards.
        Validates completely first (size, range, uniqueness) and touches
        nothing on failure. Patch rows, cluster orders and names are positional
        and untouched. Latches ownership - the numbers just became an external
        contract - and clears every undo history. */
    juce::Result assignInputChannelNumbersBySlot (const std::vector<int>& numbersBySlot,
                                                  const juce::String& reason);

    /** REORDER - arrange the live channels so their numbers read
        numbersInDisplayOrder top to bottom. Numbers, settings and hardware
        inputs travel with their channel (moveInputChannel). Unknown numbers
        are skipped; unlisted live channels end up after the listed ones with
        their relative order kept. Latches FIRST: unlatched, every move would
        recompact the numbers underneath the loop. */
    juce::Result reorderInputChannelsToNumbers (const std::vector<int>& numbersInDisplayOrder,
                                                const juce::String& reason);

    /** The clamp setInputChannelCounts applies, so a prediction clamps identically. */
    static void clampInputChannelCounts (int& numMono, int& numStereo);

    /** Which channels setInputChannelCounts (numMono, numStereo) WOULD remove,
        captured before anything mutates: the last channels of each type in
        DISPLAY order, stereo victims first. By SLOT, never by number - on an
        unlatched session removeInputChannel renumbers after every removal, so a
        by-number prediction is stale by the second one. The dialog that shows
        this used to predict by highest number, which is a different channel
        the moment a latched list has been dragged. */
    std::vector<InputChannelRef> predictInputChannelReduction (int numMono, int numStereo) const;

    //==========================================================================
    // Channel inventory (system-config persistence)
    //==========================================================================

    /** Build the <InputChannelList> node the system config carries: one <Ch>
        per live channel, in DISPLAY order, with its permanent number and type.

        `inputChannels` is only a sum, and the mono/stereo split and the display
        order live solely on the <Input> nodes — which are saved to inputs.xml,
        not system.xml. So a system config reloaded on its own rebuilt every
        channel as mono, and the positional patch rows then landed on the wrong
        channels (a stereo row's two hardware columns on a mono channel). This
        node is what closes that hole. It is a FILE artifact: derived at save
        time, consumed at load time, never stored in the runtime tree, so it
        cannot desync from the nodes it describes. */
    juce::ValueTree buildInputChannelInventory() const;

    /** Reconcile the live channel list to an inventory read from a file:
        remove channels it does not list, create the ones it does with their
        recorded number and type, correct any type that disagrees, then order
        the list to match. Numbers, types, display order and permanent-number
        gaps all survive. Structural and not undoable, like the ops it calls.

        Caller must have latched channel numbers first — otherwise the ops'
        fresh-session tail renumbers the very list this is restoring. */
    void applyInputChannelInventory (const juce::ValueTree& inventory);

    /** Reconcile the input-patch row COUNT to the channel list after a
        wholesale patchData rewrite (config load): truncate extras, append
        diagonal-continue rows (capacity from the channel type). The count only —
        an existing row is never re-columned, so a loaded patch is never
        rewritten under the operator. A no-op on a re-flowed patch
        (compactInputPatchToDisplayOrder already emits exactly one row per live
        channel). Idempotent. */
    void normalizeInputPatchRows();

    /** Convert every cluster's `clusterInputOrder` between the SLOT keying used
        in memory and the permanent-channel-NUMBER keying used on disk.

        Slots are defined by the channel list; the CSV is persisted in
        system.xml while that list comes from inputs.xml, so a slot-keyed CSV
        crossing the file boundary is only valid while the two agree. They do
        not agree mid-load: the channel-list reconciliation deletes and reorders
        underneath a CSV the merge has already brought in. Number keying removes
        the dependency entirely — the same boundary conversion
        serializeExtendedScope/deserializeExtendedScope already use for snapshot
        scope, which is proven correct across a reorder.

        Tokens that cannot be resolved are dropped (a number with no live
        channel, a slot past the end). Both are message-thread only and write
        with no undo manager: this is bookkeeping, not an operator edit. */
    void convertClusterOrdersSlotsToNumbers();
    void convertClusterOrdersNumbersToSlots();

    /** One-time model migration for loaded states: repairs missing/duplicate
        ids (one dense renumber — the last renumbering that can ever happen to
        a file) and stamps inputChannelType from the legacy tail split when
        the whole list lacks it. Tree order is preserved (it is the user's
        display order). Idempotent, not undoable. Must run BEFORE
        ensureCompleteSchema on wholesale-replace loads: the schema template
        stamps mono, which would otherwise preempt the tail-split stamp. */
    void migrateInputChannelModel();

    /** Renumber every input channel to its display slot + 1 (dense 1..N). The
        tracking id, stamped FROM the number, follows it; anything the user
        changed stays. Names do NOT follow the number: a default name is a
        per-type ordinal maintained by resequenceDefaultInputNames(). Only ever
        called while the session has not latched channel numbers. Idempotent. */
    void compactChannelNumbersToDisplayOrder();

    /** Re-flow the whole input patch into a gapless diagonal in DISPLAY order:
        walk the slots top to bottom handing out consecutive hardware input
        columns — two ADJACENT columns for a stereo row (lower = L), one for a
        mono row. Strict packing: no gaps, and no alignment to the interface's
        odd/even pairs, so N mono + M stereo always fit in N + 2M inputs.
        Rebuilt FROM the channel list, so the stored rows are discarded outright
        and a wrong row count is repaired rather than reconciled. The diagonal
        may legitimately run past activeHardwareInputs — cols widens and the
        matrix dims those columns, deliberately, so the patch does not depend on
        which interface happened to be plugged in at edit time. Shares the
        `channelNumbersUserOwned` latch with compactChannelNumbersToDisplayOrder()
        (there is no second, patch-specific ownership flag) and is only ever
        called while the session has not latched. Idempotent. */
    void compactInputPatchToDisplayOrder();

    /** Renumber the DEFAULT input names to "Mono n" / "Stereo n", counting each
        type independently in display order. A name the user typed is left
        alone: only the three shapes the app itself stamps — "Input n" (legacy),
        "Mono n", "Stereo n", with any n — count as still-default. Called when
        the arrange dialog closes on a session that has not latched channel
        numbers; latching is not implied by it. Idempotent. */
    void resequenceDefaultInputNames();

    void setNumOutputChannels (int numChannels);
    void setNumReverbChannels (int numChannels);

    /** Update hardware channel count in patch trees based on actual audio device.
     *  Pass 0 for either count when no device is connected to trigger the
     *  "default to 64 or highest patched channel" policy. */
    void updateHardwareChannelCount (int hwInputs, int hwOutputs);

    /** Recompute patch-matrix column counts using the most recent device
     *  channel counts stored on the patch trees. Call after any patch edit
     *  so cols can shrink once overflow routes are removed. */
    void recomputePatchCols();

    /** Max hardware channels the patch matrix can address. */
    static constexpr int maxHardwarePatchChannels = 512;

    //==========================================================================
    // Undo / Redo  (per-domain — one UndoManager per tab)
    //==========================================================================
    // The UndoManager array, active-domain state, undo/redo/canUndo/canRedo,
    // beginUndoTransaction, clearUndoHistory/clearAllUndoHistories and the
    // MCP-origin undo suppression live in TreeParameterStore. These thin
    // wrappers map the WFS tab-domain enum onto the core's integer indices.

    /** Set the currently active undo domain (called by MainComponent on tab change) */
    void setActiveDomain (UndoDomain domain)
    {
        TreeParameterStore::setActiveDomain (static_cast<int> (domain));
    }

    /** Get the currently active undo domain */
    UndoDomain getActiveDomain() const
    {
        return static_cast<UndoDomain> (TreeParameterStore::getActiveDomain());
    }

    /** Get UndoManager for a specific domain */
    juce::UndoManager* getUndoManagerForDomain (UndoDomain domain)
    {
        return TreeParameterStore::getUndoManagerForDomain (static_cast<int> (domain));
    }

    /** RAII helper: writes made while alive bypass the UndoManager entirely.
        Used for externally triggered snapshot recalls (MIDI note, OSC) so a
        cue-driven show does not bury the operator's own edits under one undo
        entry per cue. See TreeParameterStore::ScopedUndoSuppression. */
    using ScopedUndoSuppression = TreeParameterStore::ScopedUndoSuppression;

    /** RAII helper: temporarily switch the active undo domain, restoring on destruction */
    struct ScopedUndoDomain
    {
        ScopedUndoDomain (WFSValueTreeState& s, UndoDomain d)
            : state (s), previous (s.getActiveDomain()) { state.setActiveDomain (d); }
        ~ScopedUndoDomain() { state.setActiveDomain (previous); }
        WFSValueTreeState& state;
        UndoDomain previous;
    };

    //==========================================================================
    // State Management
    //==========================================================================

    /** Reset all parameters to defaults */
    void resetToDefaults();

    /** Reset input channel to defaults */
    void resetInputToDefaults (int channelIndex);

    /** Reset output channel to defaults */
    void resetOutputToDefaults (int channelIndex);

    /** Reset reverb channel to defaults */
    void resetReverbToDefaults (int channelIndex);

    /** Redistribute all input positions evenly within the current stage bounds */
    void redistributeAllInputPositions();

    /** Re-lays ALL reverb nodes on the initial arc for the current stage and
        node count (the reverb twin of redistributeAllInputPositions). */
    void redistributeAllReverbPositions();

    //==========================================================================
    // Position ownership (see positionsUserOwned in WFSParameterIDs.h).

    /** True once the user owns the channel positions: they opened the Map tab
        or manually edited an input/output/reverb position. One-way, persisted
        with the session. While false, stage size/shape/origin and channel-count
        changes silently re-run the initial placement (inputs grid, reverb arc);
        once true, nothing repositions automatically — only the explicit
        buttons do. */
    bool arePositionsUserOwned();

    /** Latches position ownership to the user. Idempotent; not undoable on
        purpose (Ctrl+Z must not re-arm auto-placement). */
    void markPositionsUserOwned();

    //==========================================================================
    // Channel-number ownership (see channelNumbersUserOwned in WFSParameterIDs.h).

    /** True once the input channel numbers are permanent. While false — a
        fresh session nothing has ever exposed a number from — every structural
        edit renumbers the list to dense display order. Returns TRUE when the IO
        tree is invalid: renumbering rewrites ids across the whole list, so
        half-built or malformed state must fall back to the permanent regime
        rather than be mistaken for "fresh". (arePositionsUserOwned returns
        false there; the destructive direction is the opposite one.) */
    bool areChannelNumbersUserOwned();

    /** Latches the channel numbers as permanent. Idempotent; not undoable on
        purpose (Ctrl+Z must not re-arm renumbering). The reason names the
        trigger and is logged once, on the actual fresh->owned transition —
        the latch is otherwise invisible and a spent one is indistinguishable
        from a broken re-flow. */
    void markChannelNumbersUserOwned (const juce::String& reason);

    /** Scale all input positions proportionally from old stage bounds to current bounds */
    void scaleAllInputPositions (float oldW, float oldD, float oldH,
                                 float oldOW, float oldOD, float oldOH);

    /** Clamp all input positions to within the current stage bounds */
    void fitAllInputPositionsToStage();

    /** Shift all input positions by a 3D delta */
    void shiftAllInputPositions (float dx, float dy, float dz);

    /** Shift all output positions by a 3D delta */
    void shiftAllOutputPositions (float dx, float dy, float dz);

    /** Shift all reverb positions by a 3D delta */
    void shiftAllReverbPositions (float dx, float dy, float dz);

    /** Replace entire state (e.g., when loading) */
    void replaceState (const juce::ValueTree& newState);

    /** Validate state structure */
    bool validateState (const juce::ValueTree& stateToValidate) const;

    /** Copy state from another WFSValueTreeState */
    void copyStateFrom (const WFSValueTreeState& other);

    /** Migrate old flat ADM-OSC section to new nested mapping structure */
    void migrateADMOSCSection();

    /** Ensure all inputs have the inputAdmMapping property (migration) */
    void ensureInputAdmMappingProperty();

    /** Back-fill any sections/properties that the default schema defines but a
        loaded state is missing. Used on the wholesale-replace load path
        (importCompleteConfig / full-config snapshot recall) so that an incomplete
        or scope-filtered file cannot leave parameters permanently absent. */
    void ensureCompleteSchema();

protected:
    //==========================================================================
    // TreeParameterStore seams (change-notification dispatch hooks)
    //==========================================================================

    /** Derive the channel index for a changed node (Input/Output/Reverb parent id) */
    int resolveChannelIndex (const juce::ValueTree& changedNode) const override;

    /** POST-WRITE HOOK — WFS semantic invariants (cluster tracking uniqueness,
        shared-position snap) run here, before listener dispatch. */
    void handlePostWrite (juce::ValueTree& changedNode, const juce::Identifier& property,
                          const juce::var& value, int channelIndex) override;

private:
    /** Set one channel's permanent number, dragging its tracking id along only
        while that still matched the old number. Raw setProperty: a renumber is
        bookkeeping and must carry no undo entry, dirty mark or ownership latch.
        Shared by the fresh-session compaction and the relabel. */
    void setInputChannelNumberAtSlot (int slot, int newNumber);
    //==========================================================================
    // Initialization
    //==========================================================================

    void initializeDefaultState();
    void createConfigSection();
    void createShowSection (juce::ValueTree& config);
    void createIOSection (juce::ValueTree& config);
    void createStageSection (juce::ValueTree& config);
    void createMasterSection (juce::ValueTree& config);
    void createNetworkSection (juce::ValueTree& config);

    /** Move config properties older files kept in <IO> into the sections that now
        stamp them, so no value is left duplicated across two homes. */
    void migrateStrayConfigProperties();
    void createADMOSCSection (juce::ValueTree& config);
    void createTrackingSection (juce::ValueTree& config);
    void createClustersSection (juce::ValueTree& config);
    void createBinauralSection (juce::ValueTree& config);
    void createUISection (juce::ValueTree& config);
    void createInputsSection();
    void createOutputsSection();
    void createReverbsSection();
    void createAudioPatchSection();

    /** Create a single default input channel */
    /** @param totalInputsIn  target channel count; pass it explicitly while
        growing the list, since the tree still holds the old count then.
        <= 0 reads the tree (correct for single-channel resets). */
    /** @param channelNumber  permanent channel number; <= 0 derives it from
        the slot (index + 1, dense creation). */
    juce::ValueTree createDefaultInputChannel (int index, int totalInputsIn = -1, int channelNumber = -1);

    /** Create input channel subsections */
    /** @param ordinal  1-based position among the channels of that type; it is
        the counter behind the "Mono n" / "Stereo n" default name. */
    juce::ValueTree createInputChannelSection (bool stereo, int ordinal);
    juce::ValueTree createInputPositionSection (int index, int totalInputs);
    juce::ValueTree createInputAttenuationSection();
    juce::ValueTree createInputDirectivitySection();
    juce::ValueTree createInputLiveSourceSection();
    juce::ValueTree createInputHackousticsSection();
    juce::ValueTree createInputLFOSection();
    juce::ValueTree createInputAutoMotionSection();
    juce::ValueTree createInputMutesSection (int numOutputs);
    juce::ValueTree createInputGradientMapsSection();
    juce::ValueTree createInputSamplerSection();

    /** Create a single default output channel */
    juce::ValueTree createDefaultOutputChannel (int index);

    /** Create output channel subsections */
    juce::ValueTree createOutputChannelSection (int index);
    juce::ValueTree createOutputPositionSection();
    juce::ValueTree createOutputOptionsSection();
    juce::ValueTree createOutputEQSection();

    /** Create a single default reverb channel */
    juce::ValueTree createDefaultReverbChannel (int index, int totalCount);

    /** Stage dimensions in the form the reverb node placement helper wants.
        Falls back to a nominal extent when no stage section exists yet. */
    ReverbNodePlacement::Stage getStageForPlacement();


    /** Create reverb channel subsections */
    juce::ValueTree createReverbChannelSection (int index);
    juce::ValueTree createReverbPositionSection (int index, int totalCount);
    juce::ValueTree createReverbFeedSection();
    juce::ValueTree createReverbEQSection();
    juce::ValueTree createReverbReturnSection (int numOutputs);
    juce::ValueTree createReverbAlgorithmSection();
    juce::ValueTree createReverbPreCompSection();
    juce::ValueTree createReverbPostEQSection();
    juce::ValueTree createReverbPostExpSection();

    /** Create a default network target */
    juce::ValueTree createDefaultNetworkTarget (int index);

    //==========================================================================
    // Helper Methods
    //==========================================================================

    /** Find the correct ValueTree for a given parameter ID (core schema-routing seam) */
    juce::ValueTree getTreeForParameter (const juce::Identifier& id, int channelIndex) const override;

    /** Determine if a parameter belongs to input, output, reverb, or config */
    enum class ParameterScope { Config, Input, Output, Reverb, Cluster, AudioPatch, Unknown };
    ParameterScope getParameterScope (const juce::Identifier& id) const;

    /** Enforce cluster tracking constraint: only one tracked input per cluster
     *  Called when inputTrackingActive or inputCluster changes */
    void enforceClusterTrackingConstraint (int changedInputIndex);

    /** Migration-only: stamp the pre-rework tail split ("the LAST
        stereoCountOverride channels are stereo") onto the per-channel type
        property. The caller reads the count from the legacy IO property. */
    void stampChannelTypesFromLegacySplit (juce::UndoManager* um, int stereoCountOverride);

    /** Patch-row halves of the structural ops: insert a diagonal-continue
        row for a new channel at its slot / remove a deleted channel's row.
        Rows are positional (row = slot), so they must mirror every channel
        tree edit in the same op.

        Once the session has latched, these three ARE the whole story. Unlatched
        their result is immediately overwritten by
        compactInputPatchToDisplayOrder(), and that redundancy is deliberate:
        doing the row bookkeeping here keeps the row count in step with the
        channel list at every instant (a synchronous listener can never observe a
        tree whose row count disagrees with the channel count), it keeps the
        re-flow a pure function of the channel list rather than a repair step it
        is required to perform, and it stops the latched regime resting on the
        unlatched one's correctness. */
    void insertInputPatchRow (int slot, bool stereo);
    void removeInputPatchRow (int slot);
    void moveInputPatchRow (int fromSlot, int toSlot);

    /** clusterInputOrder csvs hold 0-based slot indices; remap them whenever
        a structural edit shifts slots (delete/reorder). Returning -1 from the
        mapper drops the entry. */
    void remapClusterInputOrders (const std::function<int (int)>& oldSlotToNewSlot);

    /** Clamp a value to the valid range for a given output parameter */
    static float clampOutputParamToRange (const juce::Identifier& paramId, float value);

    /** Set output parameter directly without array propagation (internal use) */
    void setOutputParameterDirect (int channelIndex, const juce::Identifier& id, const juce::var& value);

    /** Set EQ band parameter directly without array propagation (internal use) */
    void setOutputEQBandParameterDirect (int channelIndex, int bandIndex, const juce::Identifier& id, const juce::var& value);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WFSValueTreeState)
};
