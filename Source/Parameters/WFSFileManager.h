#pragma once

#include <JuceHeader.h>
#include "WFSValueTreeState.h"

/**
 * WFS File Manager
 *
 * Handles all file operations for saving and loading WFS configurations:
 * - Complete configuration save/load
 * - System configuration save/load
 * - Input/Output configuration save/load
 * - Snapshot management with scope filtering
 * - Automatic backup creation
 * - Project folder structure management
 */
class WFSFileManager
{
public:
    //==========================================================================
    // Construction
    //==========================================================================

    explicit WFSFileManager (WFSValueTreeState& state);
    ~WFSFileManager() = default;

    //==========================================================================
    // Project Folder Management
    //==========================================================================

    /** Set the project folder path */
    void setProjectFolder (const juce::File& folder);

    /** Get the current project folder */
    juce::File getProjectFolder() const { return projectFolder; }

    /** Check if project folder is set and valid */
    bool hasValidProjectFolder() const;

    /** Create project folder structure */
    bool createProjectFolderStructure();

    /** Show folder chooser dialog to select project folder */
    void chooseProjectFolder (std::function<void (bool)> callback);

    //==========================================================================
    // File Paths
    //==========================================================================

    /** Get path for complete configuration file */
    juce::File getCompleteConfigFile() const;

    /** Get path for system configuration file */
    juce::File getSystemConfigFile() const;

    /** Get path for input configuration file */
    juce::File getInputConfigFile() const;

    /** Get path for output configuration file */
    juce::File getOutputConfigFile() const;

    /** Get path for reverb configuration file */
    juce::File getReverbConfigFile() const;

    /** Get path for audio patch file */
    juce::File getAudioPatchFile() const;

    /** Get path for network configuration file */
    juce::File getNetworkConfigFile() const;

    /** Get backup folder */
    juce::File getBackupFolder() const;

    /** Get input snapshots folder */
    juce::File getInputSnapshotsFolder() const;

    /** Get output snapshots folder */
    juce::File getOutputSnapshotsFolder() const;

    //==========================================================================
    // Complete Configuration
    //==========================================================================

    /** Save complete configuration to project folder */
    bool saveCompleteConfig();

    /** Load complete configuration from project folder */
    bool loadCompleteConfig();

    /** Load complete configuration from backup */
    bool loadCompleteConfigBackup (int backupIndex = 0);

    /** Export complete configuration to specified file */
    bool exportCompleteConfig (const juce::File& file);

    /** Import complete configuration from specified file */
    bool importCompleteConfig (const juce::File& file);

    //==========================================================================
    // System Configuration (Config section only)
    //==========================================================================

    /** Save system configuration to project folder */
    bool saveSystemConfig();

    /** Load system configuration from project folder */
    bool loadSystemConfig();

    /** Load system configuration from backup */
    bool loadSystemConfigBackup (int backupIndex = 0);

    /** Export system configuration to specified file */
    bool exportSystemConfig (const juce::File& file);

    /** Import system configuration from specified file */
    bool importSystemConfig (const juce::File& file);

    //==========================================================================
    // Network Configuration (can be loaded while DSP is running)
    //==========================================================================

    /** Save network configuration to project folder */
    bool saveNetworkConfig();

    /** Load network configuration from project folder */
    bool loadNetworkConfig();

    /** Load network configuration from backup */
    bool loadNetworkConfigBackup (int backupIndex = 0);

    /** Export network configuration to specified file */
    bool exportNetworkConfig (const juce::File& file);

    /** Import network configuration from specified file */
    bool importNetworkConfig (const juce::File& file);

    //==========================================================================
    // Input Configuration
    //==========================================================================

    /** Save input configuration to project folder */
    bool saveInputConfig();

    /** Load input configuration from project folder */
    bool loadInputConfig();

    /** Load input configuration from backup */
    bool loadInputConfigBackup (int backupIndex = 0);

    /** Export input configuration to specified file */
    bool exportInputConfig (const juce::File& file);

    /** Import input configuration from specified file */
    bool importInputConfig (const juce::File& file);

    //==========================================================================
    // Output Configuration
    //==========================================================================

    /** Save output configuration to project folder */
    bool saveOutputConfig();

    /** Load output configuration from project folder */
    bool loadOutputConfig();

    /** Load output configuration from backup */
    bool loadOutputConfigBackup (int backupIndex = 0);

    /** Export output configuration to specified file */
    bool exportOutputConfig (const juce::File& file);

    /** Import output configuration from specified file */
    bool importOutputConfig (const juce::File& file);

    //==========================================================================
    // Reverb Configuration
    //==========================================================================

    /** Save reverb configuration to project folder */
    bool saveReverbConfig();

    /** Load reverb configuration from project folder */
    bool loadReverbConfig();

    /** Load reverb configuration from backup */
    bool loadReverbConfigBackup (int backupIndex = 0);

    /** Export reverb configuration to specified file */
    bool exportReverbConfig (const juce::File& file);

    /** Import reverb configuration from specified file */
    bool importReverbConfig (const juce::File& file);

    //==========================================================================
    // Snapshots
    //==========================================================================

    /** Scope options for snapshots (legacy section-level) */
    struct SnapshotScope
    {
        bool includePosition = true;
        bool includeAttenuation = true;
        bool includeDirectivity = true;
        bool includeLiveSource = true;
        bool includeHackoustics = true;
        bool includeLFO = true;
        bool includeAutomOtion = true;
        bool includeMutes = true;
        juce::Array<int> channelIndices;  // Empty = all channels
    };

    //==========================================================================
    // Extended Snapshot Scope (parameter-level, per-channel granularity)
    //==========================================================================

    /** Scope item definition - groups related parameters */
    struct ScopeItem
    {
        juce::String itemId;           // Unique identifier for this scope item
        juce::String displayName;      // Display name in UI
        juce::Identifier sectionId;    // Section this item belongs to (Position, Attenuation, etc.)
        std::vector<juce::Identifier> parameterIds;  // Parameters included in this group
    };

    /** Extended scope supporting parameter-level, per-channel granularity */
    struct ExtendedSnapshotScope
    {
        /** When to apply the scope filtering */
        enum class ApplyMode { OnSave, OnRecall };
        ApplyMode applyMode = ApplyMode::OnRecall;

        /** Per-item, per-channel inclusion state
         *  Key format: "itemId_channelIndex"
         *  Default: all items included (true)
         */
        std::map<juce::String, bool> itemChannelStates;

        //----------------------------------------------------------------------
        // Static scope item definitions
        //----------------------------------------------------------------------

        /** Get all scopeable items with their grouped parameters */
        static const std::vector<ScopeItem>& getScopeItems();

        /** Get all unique section identifiers in order */
        static const std::vector<juce::Identifier>& getSectionIds();

        /** Get scope items for a specific section */
        static std::vector<const ScopeItem*> getItemsForSection (const juce::Identifier& sectionId);

        //----------------------------------------------------------------------
        // Query methods
        //----------------------------------------------------------------------

        /** Check if a scope item is included for a channel */
        bool isIncluded (const juce::String& itemId, int channelIndex) const;

        /** Check if a parameter is included for a channel (via its scope item) */
        bool isParameterIncluded (const juce::Identifier& paramId, int channelIndex) const;

        //----------------------------------------------------------------------
        // Modification methods
        //----------------------------------------------------------------------

        /** Set inclusion state for a scope item and channel */
        void setIncluded (const juce::String& itemId, int channelIndex, bool included);

        /** Toggle inclusion state for a scope item and channel */
        void toggle (const juce::String& itemId, int channelIndex);

        /** Set all items for a specific channel */
        void setAllItemsForChannel (int channelIndex, bool included);

        /** Set a specific item for all channels */
        void setItemForAllChannels (const juce::String& itemId, bool included, int numChannels);

        /** Set all items in a section for all channels */
        void setSectionForAllChannels (const juce::Identifier& sectionId, bool included, int numChannels);

        /** Set all items for all channels */
        void setAll (bool included, int numChannels);

        //----------------------------------------------------------------------
        // State queries for UI
        //----------------------------------------------------------------------

        enum class InclusionState { AllIncluded, AllExcluded, Partial };

        /** Get the inclusion state for a section across all channels */
        InclusionState getSectionState (const juce::Identifier& sectionId, int numChannels) const;

        /** Get the inclusion state for a section in a specific channel */
        InclusionState getSectionStateForChannel (const juce::Identifier& sectionId, int channelIndex) const;

        /** Get the inclusion state for a channel (all items) */
        InclusionState getChannelState (int channelIndex) const;

        /** Get overall state (all items, all channels) */
        InclusionState getOverallState (int numChannels) const;

        //----------------------------------------------------------------------
        // Initialization
        //----------------------------------------------------------------------

        /** Initialize with all items included for all channels */
        void initializeDefaults (int numChannels);

        /** Create key string for itemId and channel */
        static juce::String makeKey (const juce::String& itemId, int channelIndex);
    };

    /** Save a new input snapshot */
    bool saveInputSnapshot (const juce::String& snapshotName, const SnapshotScope& scope);

    /** Load an input snapshot */
    bool loadInputSnapshot (const juce::String& snapshotName, const SnapshotScope& scope);

    /** Update an existing input snapshot */
    bool updateInputSnapshot (const juce::String& snapshotName, const SnapshotScope& scope);

    /** Delete an input snapshot */
    bool deleteInputSnapshot (const juce::String& snapshotName);

    /** Get list of available input snapshots */
    juce::StringArray getInputSnapshotNames() const;

    /** Get default snapshot name (timestamp) */
    static juce::String getDefaultSnapshotName();

    /** Get/set snapshot scope for a named snapshot */
    SnapshotScope getSnapshotScope (const juce::String& snapshotName) const;
    bool setSnapshotScope (const juce::String& snapshotName, const SnapshotScope& scope);

    //==========================================================================
    // Extended Snapshot Scope Operations
    //==========================================================================

    /** Save a new input snapshot with extended scope */
    bool saveInputSnapshotWithExtendedScope (const juce::String& snapshotName, const ExtendedSnapshotScope& scope);

    /** Load an input snapshot with extended scope */
    bool loadInputSnapshotWithExtendedScope (const juce::String& snapshotName, const ExtendedSnapshotScope& scope);

    /** Get extended scope from snapshot file */
    ExtendedSnapshotScope getExtendedSnapshotScope (const juce::String& snapshotName) const;

    /** Save extended scope to snapshot file (updates scope only, not parameters) */
    bool setExtendedSnapshotScope (const juce::String& snapshotName, const ExtendedSnapshotScope& scope);

    //==========================================================================
    // Backup Management
    //==========================================================================

    /** Create a backup of a file */
    bool createBackup (const juce::File& file);

    /** Get list of backups for a file type */
    juce::Array<juce::File> getBackups (const juce::String& fileType) const;

    /** Clean up old backups (keep last N) */
    void cleanupBackups (int keepCount = 10);

    /** Get backup timestamp */
    static juce::String getBackupTimestamp();

    //==========================================================================
    // Error Handling
    //==========================================================================

    /** Get last error message */
    juce::String getLastError() const { return lastError; }

    /** Clear last error */
    void clearError() { lastError.clear(); }

    //==========================================================================
    // File Extensions
    //==========================================================================

    static constexpr const char* completeConfigExtension = ".xml";
    static constexpr const char* systemConfigExtension = ".xml";
    static constexpr const char* networkConfigExtension = ".xml";
    static constexpr const char* inputConfigExtension = ".xml";
    static constexpr const char* outputConfigExtension = ".xml";
    static constexpr const char* reverbConfigExtension = ".xml";
    static constexpr const char* audioPatchExtension = ".xml";
    static constexpr const char* snapshotExtension = ".xml";

private:
    //==========================================================================
    // Private Members
    //==========================================================================

    WFSValueTreeState& valueTreeState;
    juce::File projectFolder;
    juce::String lastError;

    //==========================================================================
    // Internal Methods
    //==========================================================================

    /** Write ValueTree to XML file with human-readable formatting */
    bool writeToXmlFile (const juce::ValueTree& tree, const juce::File& file);

    /** Read ValueTree from XML file */
    juce::ValueTree readFromXmlFile (const juce::File& file);

    /** Extract config section from state */
    juce::ValueTree extractConfigSection() const;

    /** Extract inputs section from state */
    juce::ValueTree extractInputsSection() const;

    /** Extract outputs section from state */
    juce::ValueTree extractOutputsSection() const;

    /** Extract reverbs section from state */
    juce::ValueTree extractReverbsSection() const;

    /** Extract audio patch section from state */
    juce::ValueTree extractAudioPatchSection() const;

    /** Extract network section from config */
    juce::ValueTree extractNetworkSection() const;

    /** Apply config section to state */
    bool applyConfigSection (const juce::ValueTree& config);

    /** Apply inputs section to state */
    bool applyInputsSection (const juce::ValueTree& inputs);

    /** Apply outputs section to state */
    bool applyOutputsSection (const juce::ValueTree& outputs);

    /** Apply reverbs section to state */
    bool applyReverbsSection (const juce::ValueTree& reverbs);

    /** Apply audio patch section to state */
    bool applyAudioPatchSection (const juce::ValueTree& audioPatch);

    /** Apply network section to state */
    bool applyNetworkSection (const juce::ValueTree& network);

    /** Extract input data with scope filtering */
    juce::ValueTree extractInputWithScope (int channelIndex, const SnapshotScope& scope) const;

    /** Apply input data with scope filtering */
    bool applyInputWithScope (int channelIndex, const juce::ValueTree& inputData, const SnapshotScope& scope);

    /** Extract input data with extended scope filtering */
    juce::ValueTree extractInputWithExtendedScope (int channelIndex, const ExtendedSnapshotScope& scope) const;

    /** Apply input data with extended scope filtering */
    bool applyInputWithExtendedScope (int channelIndex, const juce::ValueTree& inputData, const ExtendedSnapshotScope& scope);

    /** Serialize extended scope to ValueTree */
    juce::ValueTree serializeExtendedScope (const ExtendedSnapshotScope& scope, int numChannels) const;

    /** Deserialize extended scope from ValueTree */
    ExtendedSnapshotScope deserializeExtendedScope (const juce::ValueTree& scopeTree) const;

    /** Create file header comment */
    static juce::String createXmlHeader (const juce::String& fileType);

    /** Set error message */
    void setError (const juce::String& error);

    /** Merge properties from source to target (only set properties that exist in source) */
    void mergeProperties (juce::ValueTree& target, const juce::ValueTree& source,
                          juce::UndoManager* undoManager);

    /** Recursively merge tree including children (preserves existing properties/children not in source) */
    void mergeTreeRecursive (juce::ValueTree& target, const juce::ValueTree& source,
                             juce::UndoManager* undoManager);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WFSFileManager)
};
