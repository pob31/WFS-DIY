#pragma once

#include <JuceHeader.h>
#include "WFSParameterIDs.h"
#include "WFSParameterDefaults.h"
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

    /** Set output EQ band parameter with array propagation */
    void setOutputEQBandParameterWithArrayPropagation (int channelIndex,
                                                        int bandIndex,
                                                        const juce::Identifier& id,
                                                        const juce::var& value);

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

    /** Set channel counts (creates/removes channel ValueTrees) */
    void setNumInputChannels (int numChannels);
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
    juce::ValueTree createDefaultInputChannel (int index);

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

    /** Clamp a value to the valid range for a given output parameter */
    static float clampOutputParamToRange (const juce::Identifier& paramId, float value);

    /** Set output parameter directly without array propagation (internal use) */
    void setOutputParameterDirect (int channelIndex, const juce::Identifier& id, const juce::var& value);

    /** Set EQ band parameter directly without array propagation (internal use) */
    void setOutputEQBandParameterDirect (int channelIndex, int bandIndex, const juce::Identifier& id, const juce::var& value);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WFSValueTreeState)
};
