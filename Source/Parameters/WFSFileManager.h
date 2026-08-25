#pragma once

#include <JuceHeader.h>
#include "WFSValueTreeState.h"
#include "InputChannelIdentity.h"
#include "../../spatcore/control/state/XmlPersistence.h"

#if JUCE_MAC
void setFolderIconMac (const char* folderPath);
#endif

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
 *
 * Composes spatcore::control::state::XmlPersistence for the app-agnostic
 * machinery (XML file I/O with header convention, rolling backups, and the
 * merge/backfill engine). This class keeps everything WFS-shaped: the
 * section-split file layout (show/system/inputs/outputs/reverbs/audio_patch/
 * network), the .wfs manifest, snapshots + scope filtering, dialogs, and the
 * WFSParameterDefaults-range merge validator injected into the core engine.
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
    // Project Manifest (.wfs)
    //==========================================================================

    /** Create the .wfs manifest file inside the project folder */
    bool createProjectManifest();

    /** Get the manifest file for a given project folder (FolderName/FolderName.wfs) */
    static juce::File getManifestFile (const juce::File& folder);

    /** Get the manifest file for the current project folder */
    juce::File getManifestFile() const { return getManifestFile (projectFolder); }

    /** Resolve a .wfs file to its parent project folder (with validation) */
    static juce::File getProjectFolderFromManifest (const juce::File& wfsFile);

    /** Project manifest extension */
    static constexpr const char* projectManifestExtension = ".wfs";

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

    /** Get IR files folder */
    juce::File getIRFolder() const;

    /** Get samples folder (for sampler feature) */
    juce::File getSamplesFolder() const;

    /** Get SOFA (HRTF) files folder (<project>/sofa) */
    juce::File getSofaFolder() const;

    /** Get scope templates folder (<project>/snapshots/scopes) */
    juce::File getScopeTemplatesFolder() const;

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

    /** Auto-save variant used by the background/shutdown saves (sound-card settings
        persistence). Refuses to overwrite an existing system.xml in a project folder
        whose config hasn't been loaded (or explicitly saved) this session — otherwise
        selecting a work folder and starting audio before reloading would clobber the
        on-disk config with the in-memory defaults. */
    bool autoSaveSystemConfig();

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
    // Cluster LFO Presets
    //==========================================================================

    /** Export cluster LFO presets to specified file */
    bool exportClusterLFOPresets (const juce::File& file);

    /** Import cluster LFO presets from specified file */
    bool importClusterLFOPresets (const juce::File& file);

    //==========================================================================
    // Snapshots
    //==========================================================================

    //==========================================================================
    // Snapshot Scope (parameter-level, per-channel granularity)
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

        /** MIDI note trigger. A note-on above the velocity threshold on
            (midiChannel, midiNote) recalls this snapshot.

            Channel is 1..16, exactly as juce::MidiMessage::getChannel() reports
            it; 0 = unbound, which is also what every pre-beta42 snapshot reads
            back as (absent attribute -> "" -> getIntValue() == 0).

            Deliberately NOT a scope item: a trigger is a property of the
            snapshot, not of an input channel, so it stays out of
            itemChannelStates and out of the channel-count-dependent machinery
            in deserializeExtendedScope(). It is also serialised on the
            <InputSnapshot> ROOT, not inside <ExtendedScope> -- see
            writeMidiBindingToRoot(). That keeps a scope TEMPLATE, which shares
            the scope serializer, from carrying a note. */
        int midiChannel = 0;    // 0 = unbound, else 1..16
        int midiNote    = 0;    // 0..127, meaningful only when midiChannel > 0

        bool hasMidiBinding() const noexcept
        {
            return midiChannel >= 1 && midiChannel <= 16
                && midiNote    >= 0 && midiNote    <= 127;
        }

        void clearMidiBinding() noexcept { midiChannel = 0; midiNote = 0; }

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

        /** Semantic equality: same apply mode and same per-item/per-channel
            inclusion across all scope items and channels. Raw map comparison
            would be wrong — an absent key and an explicit `true` entry both
            mean "included". */
        bool isEquivalentTo (const ExtendedSnapshotScope& other, int numChannels) const;

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

        /** Return a copy of this scope with global-master gates folded in.
            When samplerMasterOn is false, every `sampler_<ch>` key is forced
            to excluded so callers cannot accidentally include sampler data. */
        ExtendedSnapshotScope withGlobals (bool samplerMasterOn, int numChannels) const;

        /** Create key string for itemId and channel */
        static juce::String makeKey (const juce::String& itemId, int channelIndex);
    };

    /** Delete an input snapshot */
    bool deleteInputSnapshot (const juce::String& snapshotName);

    /** Get list of available input snapshots */
    juce::StringArray getInputSnapshotNames() const;

    /** Get default snapshot name (timestamp) */
    static juce::String getDefaultSnapshotName();

    /** One snapshot's MIDI trigger binding. */
    struct MidiBinding
    {
        int channel = 0;
        int note = 0;
        juce::String snapshotName;
    };

    /** Every bound snapshot in the project, in file-name order (which makes the
        winner of a duplicate deterministic and identical on every machine).

        Reads only the OUTER document element of each file -- which is exactly
        why the binding lives on the root rather than inside <ExtendedScope>.
        Cheap enough to call on every scope-editor keystroke. */
    std::vector<MidiBinding> scanSnapshotMidiBindings() const;

    /** Fired at the end of setProjectFolder(). One choke point for every call
        site so MIDI binding-index invalidation is not duplicated. */
    std::function<void()> onProjectFolderChanged;

    //==========================================================================
    // Snapshot Scope Operations
    //==========================================================================

    /** Save a new input snapshot with extended scope. Also latches channel-number
        ownership: the file keys its entries by permanent channel number, so those
        numbers stop being reassignable the moment it is written. */
    /** True if `propertyId` is carried by an input snapshot — i.e. it appears in
        some ScopeItem's parameterIds, or in the <Channel> table.

        Exists for the coverage self-test. A property that lives on an <Input>
        child node and is in NEITHER place is silently absent from every
        snapshot, and because save and recall consult the same tables the
        omission is symmetric and a round-trip test stays green. That is exactly
        how the AutomOtion polar destination went unnoticed. */
    static bool isPropertyCoveredBySnapshotScope (const juce::Identifier& propertyId);

    bool saveInputSnapshotWithExtendedScope (const juce::String& snapshotName, const ExtendedSnapshotScope& scope);

    /** Load an input snapshot with extended scope */
    bool loadInputSnapshotWithExtendedScope (const juce::String& snapshotName, const ExtendedSnapshotScope& scope);

    /** Get extended scope from snapshot file */
    ExtendedSnapshotScope getExtendedSnapshotScope (const juce::String& snapshotName) const;

    /** Save extended scope to snapshot file (updates scope only, not parameters) */
    bool setExtendedSnapshotScope (const juce::String& snapshotName, const ExtendedSnapshotScope& scope);

    /** Rewrite an existing snapshot's embedded scope in place (with backup),
        without re-capturing live values. When the new scope's apply mode is
        OnSave, the stored input data is additionally trimmed down to the new
        scope — removal only: values absent from the file cannot be re-added. */
    bool updateInputSnapshotScope (const juce::String& snapshotName, const ExtendedSnapshotScope& scope);

    /** Current value of the global sampler master switch (Config > UI > samplerEnabled). */
    bool isSamplerMasterOn() const;

    //==========================================================================
    // Scope Templates (grid-only presets, stored in <project>/snapshots/scopes)
    //==========================================================================

    /** Save the grid (itemChannelStates) as a named template. Overwrites silently. */
    bool saveScopeTemplate (const juce::String& templateName, const ExtendedSnapshotScope& scope);

    /** Load a template's grid into target.itemChannelStates.
        applyMode and everything else in target are left untouched. */
    bool loadScopeTemplateGrid (const juce::String& templateName, ExtendedSnapshotScope& target);

    /** Get list of available scope templates */
    juce::StringArray getScopeTemplateNames() const;

    /** Delete a scope template */
    bool deleteScopeTemplate (const juce::String& templateName);

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

    //==========================================================================
    // Channel identity gate (pre-load check)
    //==========================================================================
    // Position is not identity. mergeTreeRecursive matches <Input> children by
    // permanent NUMBER, applyInputChannelInventory rebuilds the list by NUMBER
    // (retyping a live channel to whatever type the file's same-numbered channel
    // has), and patchData rows land by POSITION. So loading a file whose
    // (position <-> number <-> type) relation differs from the session's crosses
    // parameter sets by number and hardware inputs by slot, in opposite
    // directions at once - and a hand-rebuilt arrangement that LOOKS identical
    // is no protection, because the merge never looks at position. These read a
    // file's channel identity WITHOUT applying it and say, before the load,
    // whether that would happen.
    //
    // Three layers. The preflights are pure (const, no setError, no latch) and
    // are what the GUI asks before deciding to show a dialog. The gate inside
    // the three import primitives is the safety net no caller can bypass: an
    // unsafe load with neither a bypass nor a one-shot clearance for that exact
    // file is REFUSED, loudly, rather than applied. Complete project loads
    // check the pair (system.xml against inputs.xml - the only thing that can
    // go wrong there) and run their inner loads under a bypass.

    enum class LoadKind { systemConfig, inputConfig, completeConfig, projectPair };

    /** The live session compared with what `file` describes. */
    InputChannelIdentityDiff preflightChannelIdentity (const juce::File& file, LoadKind kind) const;

    /** system.xml's inventory compared with inputs.xml's <Input> nodes - a
        complete load of a consistent pair is safe whatever the session looks
        like, so the pair is the only thing to check. Falls back to system vs
        session when inputs.xml is unreadable (the load then degenerates to a
        system-only one). */
    InputChannelIdentityDiff preflightProjectChannelIdentity (const juce::File& systemFile,
                                                              const juce::File& inputsFile) const;

    /** The live session compared with a snapshot's entries: which numbers have
        no live channel (they will be skipped) and whose hardware-input
        fingerprint disagrees with the live patch (a different configuration, or
        a re-cable). */
    InputChannelIdentityDiff preflightSnapshotChannelIdentity (const juce::String& snapshotName) const;

    /** The one rule for "may this proceed without the operator's say-so". */
    bool isChannelIdentitySafe (const InputChannelIdentityDiff& diff, LoadKind kind) const;

    /** One-shot permission for the next primitive call on exactly `file`,
        granted by the GUI once the operator confirmed. Consumed by that call. */
    void grantChannelIdentityClearance (const juce::File& file);

    /** RAII: every primitive called inside passes the gate. Complete loads hold
        one around their inner per-file loads; the self-test holds one around the
        phases that load deliberately mismatching files. */
    struct ScopedChannelIdentityBypass
    {
        explicit ScopedChannelIdentityBypass (WFSFileManager& fm) : owner (fm) { ++owner.channelIdentityBypassDepth; }
        ~ScopedChannelIdentityBypass()                                           { --owner.channelIdentityBypassDepth; }
        WFSFileManager& owner;
    };

    /** One line for the log / status bar. */
    static juce::String summariseChannelIdentityDiff (const InputChannelIdentityDiff& diff);

    /** After loadInputSnapshotWithExtendedScope: the entry numbers that had no
        live channel and were skipped. Recall used to be silent about them. */
    const std::vector<int>& getLastRecallSkippedNumbers() const { return lastRecallSkippedNumbers; }

private:
    //==========================================================================
    // Private Members
    //==========================================================================

    WFSValueTreeState& valueTreeState;
    spatcore::control::state::XmlPersistence persistence;
    juce::File projectFolder;
    juce::String lastError;

    // True once the in-memory system config is in sync with the current project
    // folder (loaded from it, or explicitly saved to it). Gates autoSaveSystemConfig
    // so background saves can't clobber a config the user hasn't loaded yet.
    bool systemConfigSynced = false;

    // Whether the last system config applied carried an <InputChannelList>.
    //
    // It decides who owns the channel list when system.xml and inputs.xml
    // disagree. WITH an inventory the config section rebuilt the exact set —
    // numbers, types, gaps — so a channel inputs.xml does not mention means the
    // two files are out of sync, and the richer, explicitly-recorded list wins.
    // WITHOUT one it could only guess a dense 1..N from the sum, so a channel
    // inputs.xml does not mention is that guess's leftover: mergeTreeRecursive
    // never removes a target child, so it would otherwise survive as a ghost
    // (save 6 channels numbered 1,2,3,4,5,7 -> the guess makes 1..6, the merge
    // appends 7, and nothing ever deletes 6) and shift every patch row after it.
    //
    // Deliberately not reset by applyInputsSection: a standalone Reload Input
    // Config after a system load must honour that load's answer.
    bool channelListFromInventory = false;

    /** The `clusterInputOrder` CSVs exactly as the last-loaded system config
        wrote them, lifted off the file BEFORE the merge.

        They cannot be applied when they arrive. The merge puts them into the live
        tree while the channel list is still the pre-load one; the reconciliation
        that follows then deletes channels (which runs remapClusterInputOrders and
        shifts tokens) and reorders them (a raw moveChild, which does not) — so the
        CSV is half-rewritten against a slot space it was never expressed in. The
        fix is to ignore whatever the merge and the remaps leave behind and write
        the file's own values back once the channel list has settled.

        Lifting must happen pre-merge: the live tree always carries
        clusterInputOrder="" on all ten clusters, so afterwards "the file had none"
        and "the file had empty" are indistinguishable.

        `numberKeyed` mirrors the file's <Clusters inputOrderKey> marker. Absent
        means a pre-marker file whose CSVs are slots — still correct to restore
        verbatim, because by flush time the live slot space equals the file's.

        Deliberately NOT consumed by the flush: flushing is idempotent and runs at
        both the system-config-alone tail and the end of applyInputsSection, so a
        prune in the latter cannot leave a damaged CSV behind. */
    struct PendingClusterOrders
    {
        bool valid = false;
        bool numberKeyed = false;
        std::map<int, juce::String> byClusterId;
    };
    PendingClusterOrders pendingClusterOrders;

    /** Write the lifted cluster orders back, converting numbers->slots when the
        file was number-keyed. Safe to call more than once per load. */
    void flushPendingClusterOrders();

    // Channel identity gate state. `channelIdentityClearance` is a one-shot: the
    // GUI grants it for one file after the operator confirmed, and the next
    // primitive call on that file consumes it. Keyed on the exact File so a
    // confirmation for one path can never license a load of another.
    int channelIdentityBypassDepth = 0;
    juce::File channelIdentityClearance;
    std::vector<int> lastRecallSkippedNumbers;

    /** The gate itself. True = proceed. False = refused; lastError and the log
        say why. Takes the already-parsed root so the primitive parses once. */
    bool passChannelIdentityGate (const juce::File& file, LoadKind kind, const juce::ValueTree& parsedRoot);

    /** What a parsed file says about its channel list, for `kind`. For a
        system config without an inventory, the project's own inputs.xml is
        consulted - but ONLY for the project's own system.xml: a backup or an
        imported file has no trustworthy sibling. */
    InputChannelIdentity readFileChannelIdentity (const juce::ValueTree& parsedRoot, LoadKind kind,
                                                  const juce::File& file) const;

    /** Stamp each <Input> in a saved COPY with its hardware-input fingerprint. */
    void stampHardwareFingerprints (juce::ValueTree& inputsCopy) const;

    //==========================================================================
    // Internal Methods
    //==========================================================================

    /** Write ValueTree to XML file with human-readable formatting
        (delegates to spatcore XmlPersistence, mapping failures to localized errors) */
    bool writeToXmlFile (const juce::ValueTree& tree, const juce::File& file);

    /** Read ValueTree from XML file
        (delegates to spatcore XmlPersistence, mapping failures to localized errors) */
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

    /** Extract input data with extended scope filtering */
    juce::ValueTree extractInputWithExtendedScope (int channelIndex, const ExtendedSnapshotScope& scope) const;

    /** Apply input data with extended scope filtering */
    bool applyInputWithExtendedScope (int channelIndex, const juce::ValueTree& inputData, const ExtendedSnapshotScope& scope);

    /** Remove out-of-scope values from a stored snapshot Input tree, in place.
        Used by updateInputSnapshotScope for OnSave scopes; never adds data. */
    void trimSnapshotInputToScope (juce::ValueTree& inputData, const ExtendedSnapshotScope& scope, int channelIndex);

    /** Serialize extended scope to ValueTree */
    juce::ValueTree serializeExtendedScope (const ExtendedSnapshotScope& scope, int numChannels) const;

    /** Deserialize extended scope from ValueTree */
    ExtendedSnapshotScope deserializeExtendedScope (const juce::ValueTree& scopeTree) const;

    /** Write / read the MIDI trigger binding on the <InputSnapshot> ROOT element
        (not inside <ExtendedScope>, so the whole-folder index can find it with
        an outer-element-only XML parse, and so scope templates never carry it). */
    static void writeMidiBindingToRoot (juce::ValueTree& snapshot, const ExtendedSnapshotScope& scope);
    static void readMidiBindingFromRoot (const juce::ValueTree& snapshot, ExtendedSnapshotScope& scope);

    /** Set error message */
    void setError (const juce::String& error);

    /** Brand the project folder with a custom icon (platform-specific) */
    void brandProjectFolder();

    /** Recursively merge tree including children (preserves existing properties/children not in source)
        (delegates to spatcore XmlPersistence with the WFS bounds validator injected) */
    void mergeTreeRecursive (juce::ValueTree& target, const juce::ValueTree& source,
                             juce::UndoManager* undoManager);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WFSFileManager)
};
