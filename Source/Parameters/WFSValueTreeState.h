#pragma once

#include <JuceHeader.h>
#include "WFSParameterIDs.h"
#include "WFSParameterDefaults.h"

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
 * Provides:
 * - Hierarchical parameter organization
 * - Per-tab Undo/Redo support (one UndoManager per UndoDomain)
 * - Type-safe parameter access
 * - Listener registration for UI components
 * - Thread-safe parameter updates
 */
class WFSValueTreeState : public juce::ValueTree::Listener
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

    /** Get the root ValueTree state */
    juce::ValueTree getState() { return state; }
    const juce::ValueTree getState() const { return state; }

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

    /** Get a float parameter value */
    float getFloatParameter (const juce::Identifier& id, int channelIndex = -1) const;

    /** Get an int parameter value */
    int getIntParameter (const juce::Identifier& id, int channelIndex = -1) const;

    /** Get a string parameter value */
    juce::String getStringParameter (const juce::Identifier& id, int channelIndex = -1) const;

    /** Get a var parameter value (generic) */
    juce::var getParameter (const juce::Identifier& id, int channelIndex = -1) const;

    /** Set a parameter value */
    void setParameter (const juce::Identifier& id, const juce::var& value, int channelIndex = -1);

    /** Set a parameter value without undo */
    void setParameterWithoutUndo (const juce::Identifier& id, const juce::var& value, int channelIndex = -1);

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

    //==========================================================================
    // Undo / Redo  (per-domain — one UndoManager per tab)
    //==========================================================================

    /** Set the currently active undo domain (called by MainComponent on tab change) */
    void setActiveDomain (UndoDomain domain);

    /** Get the currently active undo domain */
    UndoDomain getActiveDomain() const;

    /** Get UndoManager for a specific domain */
    juce::UndoManager* getUndoManagerForDomain (UndoDomain domain);

    /** Get UndoManager for the currently active domain */
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

    /** Replace entire state (e.g., when loading) */
    void replaceState (const juce::ValueTree& newState);

    /** Validate state structure */
    bool validateState (const juce::ValueTree& stateToValidate) const;

    /** Copy state from another WFSValueTreeState */
    void copyStateFrom (const WFSValueTreeState& other);

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

private:
    //==========================================================================
    // Private Members
    //==========================================================================

    juce::ValueTree state;
    juce::UndoManager undoManagers[static_cast<int> (UndoDomain::COUNT)];
    UndoDomain activeDomain = UndoDomain::Input;

    // Listener management
    struct ListenerEntry
    {
        juce::Identifier parameterId;
        int channelIndex;
        ParameterCallback callback;
    };
    std::vector<ListenerEntry> parameterListeners;
    juce::CriticalSection listenerLock;

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

    /** Create a single default output channel */
    juce::ValueTree createDefaultOutputChannel (int index);

    /** Create output channel subsections */
    juce::ValueTree createOutputChannelSection (int index);
    juce::ValueTree createOutputPositionSection();
    juce::ValueTree createOutputOptionsSection();
    juce::ValueTree createOutputEQSection();

    /** Create a single default reverb channel */
    juce::ValueTree createDefaultReverbChannel (int index);

    /** Create reverb channel subsections */
    juce::ValueTree createReverbChannelSection (int index);
    juce::ValueTree createReverbPositionSection();
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

    /** Find the correct ValueTree for a given parameter ID */
    juce::ValueTree getTreeForParameter (const juce::Identifier& id, int channelIndex) const;

    /** Notify registered listeners of a parameter change */
    void notifyParameterListeners (const juce::Identifier& id, const juce::var& value, int channelIndex);

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
