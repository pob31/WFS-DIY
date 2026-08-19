#pragma once

#include <JuceHeader.h>
#include "WFSParameterIDs.h"
#include "WFSParameterDefaults.h"
#include "../Helpers/ReverbNodePlacement.h"
#include "../../spatcore/control/state/TreeParameterStore.h"

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

    /** Ensure a GradientMaps section exists for a given input (migration helper) */
    juce::ValueTree ensureInputGradientMapsSection (int channelIndex);

    /** Get the Sampler section for a given input channel */
    juce::ValueTree getInputSamplerSection (int channelIndex);

    /** Ensure a Sampler section exists for a given input (migration helper) */
    juce::ValueTree ensureInputSamplerSection (int channelIndex);

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

    /** Set channel counts. For inputs this is the blunt legacy entry point
        (config-load sync, OSC/MCP inputChannels writes): growth appends
        default mono channels, reduction removes the highest-numbered
        channels; nothing in the middle ever moves. Not undoable. */
    void setNumInputChannels (int numChannels);

    /** Number of live stereo channels — derived from the per-channel type
        (inputChannelType), NOT positional. Mono count =
        getNumInputChannels() − this. */
    int getNumStereoInputChannels() const;

    /** Legacy two-count entry point (System Config count fields until the
        list editor lands): a thin loop over the structural ops. Additions
        append after the last channel; reductions remove the highest-numbered
        channel of that type. Not undoable. */
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
    // slot — numbers never change, so no external reference can break.

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
        diagonal past everything already patched (two columns for stereo). */
    juce::Result addInputChannel (bool stereo, int explicitNumber = 0);

    /** Remove a live channel by permanent number. Its number is retired
        (gap); every other channel keeps its number, slot and patch row. */
    juce::Result removeInputChannel (int channelNumber);

    /** Flip a live channel's type in place. NOT exposed in the UI or MCP —
        the channel's data (patch columns, width, decomposition) cannot
        meaningfully follow a type change, so composition is edited through
        the counts instead. Kept for the self-test and internal use. */
    juce::Result setInputChannelType (int channelNumber, bool stereo);

    /** Move a live channel to a new display slot (drag-to-reorder). The
        channel node and its patch row move together; the permanent number is
        untouched. Stopped-only, like every structural edit. */
    juce::Result moveInputChannel (int channelNumber, int targetSlot);

    /** Reconcile the input-patch row count to the channel list after a
        wholesale patchData rewrite (config load): truncate extras, append
        diagonal-continue rows (capacity from the channel type). Idempotent. */
    void normalizeInputPatchRows();

    /** One-time model migration for loaded states: repairs missing/duplicate
        ids (one dense renumber — the last renumbering that can ever happen to
        a file) and stamps inputChannelType from the legacy tail split when
        the whole list lacks it. Tree order is preserved (it is the user's
        display order). Idempotent, not undoable. Must run BEFORE
        ensureCompleteSchema on wholesale-replace loads: the schema template
        stamps mono, which would otherwise preempt the tail-split stamp. */
    void migrateInputChannelModel();

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
    juce::ValueTree createInputChannelSection (int index);
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
    enum class ParameterScope { Config, Input, Output, Reverb, AudioPatch, Unknown };
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
        tree edit in the same op. */
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
