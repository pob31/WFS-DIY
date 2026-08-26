#include "WFSFileManager.h"
#include "WFSParameterIDs.h"
#include "WFSParameterDefaults.h"
#include "../AppSettings.h"
#include "../Localization/LocalizationManager.h"
#include "../Network/OSCParameterBounds.h"
#include "../Network/OSCProtocolTypes.h"
#include "../WFSLogger.h"

#include <cmath>

#if JUCE_WINDOWS
#include <Windows.h>
#endif

using namespace WFSParameterIDs;
using WFSNetwork::OriginTag;
using WFSNetwork::OriginTagScope;

//==============================================================================
// Transient toggle stripping
//==============================================================================

static void stripTransientToggles (juce::ValueTree& tree)
{
    tree.removeProperty (runDSP, nullptr);
    tree.removeProperty (binauralEnabled, nullptr);
    tree.removeProperty (inputLSactive, nullptr);
    tree.removeProperty (inputLSpeakEnable, nullptr);
    tree.removeProperty (inputLSslowEnable, nullptr);

    for (int i = 0; i < tree.getNumChildren(); ++i)
    {
        auto child = tree.getChild (i);
        stripTransientToggles (child);
    }
}

//==============================================================================
// File-load value gate (injected into the core merge engine)
//==============================================================================

// File-load value gate (mirrors the OSC entry path): for any property
// with a documented numeric range in WFSParameterDefaults.h, reject
// non-finite values and out-of-range values before they reach the
// ValueTree. Rejected properties keep their current target value (the
// app's startup default). Properties without a bounds entry — names,
// XML metadata, mute-list strings — pass through unchanged.
static std::optional<juce::var> validateFileLoadProperty (const juce::Identifier& propName,
                                                          const juce::var& v)
{
    if (auto bounds = WFSNetwork::getBounds (propName); bounds.has_value())
    {
        // Coerce to double regardless of the underlying var type:
        // ValueTree::fromXml stores XML attributes as Strings, so
        // "NaN"/"Infinity" need to round-trip through string parsing
        // to surface as non-finite here.
        const double d = static_cast<double> (v);

        if (! std::isfinite (d))
        {
            const char* kind = std::isnan (d) ? "NaN" : "Inf";
            WFSLogger::getInstance().logWarning (
                "File load: rejected " + propName.toString()
                + " (non-finite, " + kind + ")");
            return std::nullopt;
        }
        if (d < bounds->min || d > bounds->max)
        {
            WFSLogger::getInstance().logWarning (
                "File load: rejected " + propName.toString() + " ("
                + WFSNetwork::formatOutOfRangeReason (propName, d) + ")");
            return std::nullopt;
        }

        // LFO phases are circular: projects saved under the legacy 0..360
        // convention pass the compat window above; wrap them into the
        // canonical [-180, 180] here (the store interceptor would do the
        // same, but this keeps the loaded tree normalized at the source).
        if (WFSNetwork::isLFOPhaseParam (propName) && (d < -180.0 || d > 180.0))
            return juce::var (WFSParameterDefaults::wrapPhaseDegrees (juce::roundToInt (d)));
    }

    return v;
}

//==============================================================================
// Construction
//==============================================================================

WFSFileManager::WFSFileManager (WFSValueTreeState& state)
    : valueTreeState (state),
      persistence ({ "WFS Processor Configuration File",
                     WFSParameterIDs::id,
                     &validateFileLoadProperty })
{
}

//==============================================================================
// Project Folder Management
//==============================================================================

void WFSFileManager::setProjectFolder (const juce::File& folder)
{
    // A different folder means the in-memory config no longer matches what's on
    // disk there — block auto-saves until it is loaded or explicitly saved.
    if (folder != projectFolder)
        systemConfigSynced = false;

    projectFolder = folder;

    // Single choke point for MIDI binding-index invalidation: the snapshot
    // folder just changed, so every armed note belongs to the previous project.
    if (onProjectFolderChanged)
        onProjectFolderChanged();
}

bool WFSFileManager::hasValidProjectFolder() const
{
    return projectFolder.isDirectory();
}

bool WFSFileManager::createProjectFolderStructure()
{
    if (projectFolder.getFullPathName().isEmpty())
    {
        setError (LOC ("fileManager.errors.noProjectFolder"));
        return false;
    }

    // Create main folder
    if (!projectFolder.createDirectory())
    {
        setError (LOC ("fileManager.errors.failedCreateFolder").replace ("{path}", projectFolder.getFullPathName()));
        return false;
    }

    // Create subfolders
    getBackupFolder().createDirectory();
    getInputSnapshotsFolder().createDirectory();
    getOutputSnapshotsFolder().createDirectory();
    getScopeTemplatesFolder().createDirectory();
    getIRFolder().createDirectory();
    getSamplesFolder().createDirectory();
    getSofaFolder().createDirectory();

    // Create .wfs manifest if missing
    if (!getManifestFile().existsAsFile())
        createProjectManifest();

    return true;
}

void WFSFileManager::chooseProjectFolder (std::function<void (bool)> callback)
{
    auto chooser = std::make_shared<juce::FileChooser> (
        LOC ("fileManager.dialogs.selectProjectFolder"),
        projectFolder.exists() ? projectFolder : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
        "*",
        true);

    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
        [this, chooser, callback] (const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result.exists())
            {
                setProjectFolder (result);
                AppSettings::setLastFolder ("lastProjectFolder", result);
                createProjectFolderStructure();
                callback (true);
            }
            else
            {
                callback (false);
            }
        });
}

//==============================================================================
// Project Manifest (.wfs)
//==============================================================================

bool WFSFileManager::createProjectManifest()
{
    if (!projectFolder.isDirectory())
        return false;

    auto manifestFile = getManifestFile();

    juce::XmlElement root ("WFSProject");
    root.setAttribute ("projectName", projectFolder.getFileName());
    root.setAttribute ("appVersion", ProjectInfo::versionString);
    root.setAttribute ("createdDate", juce::Time::getCurrentTime().toISO8601 (true));

    if (!root.writeTo (manifestFile))
        return false;

    // Brand the project folder with a custom icon
    brandProjectFolder();

    return true;
}

void WFSFileManager::brandProjectFolder()
{
#if JUCE_WINDOWS
    // Copy app icon into project folder as hidden file
    auto appDir = juce::File::getSpecialLocation (juce::File::currentExecutableFile).getParentDirectory();
    auto sourceIcon = appDir.getChildFile ("WFS-DIY.ico");

    // Fall back to the build directory icon if not installed
    if (!sourceIcon.existsAsFile())
        sourceIcon = appDir.getChildFile ("icon.ico");

    if (!sourceIcon.existsAsFile())
        return;

    auto destIcon = projectFolder.getChildFile (".wfs-icon.ico");
    if (!destIcon.existsAsFile())
        sourceIcon.copyFileTo (destIcon);

    // Hide the icon file
    SetFileAttributesW (destIcon.getFullPathName().toWideCharPointer(),
                        FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

    // Create desktop.ini for folder icon
    auto desktopIni = projectFolder.getChildFile ("desktop.ini");
    if (!desktopIni.existsAsFile())
    {
        desktopIni.replaceWithText ("[.ShellClassInfo]\r\n"
                                    "IconResource=.wfs-icon.ico,0\r\n"
                                    "InfoTip=WFS-DIY Project\r\n");

        SetFileAttributesW (desktopIni.getFullPathName().toWideCharPointer(),
                            FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
    }

    // Set folder as system folder so Explorer reads desktop.ini
    auto folderAttrs = GetFileAttributesW (projectFolder.getFullPathName().toWideCharPointer());
    if (folderAttrs != INVALID_FILE_ATTRIBUTES)
        SetFileAttributesW (projectFolder.getFullPathName().toWideCharPointer(),
                            folderAttrs | FILE_ATTRIBUTE_SYSTEM);
#elif JUCE_MAC
    setFolderIconMac (projectFolder.getFullPathName().toRawUTF8());
#endif
}

juce::File WFSFileManager::getManifestFile (const juce::File& folder)
{
    return folder.getChildFile (folder.getFileName() + projectManifestExtension);
}

juce::File WFSFileManager::getProjectFolderFromManifest (const juce::File& wfsFile)
{
    if (!wfsFile.existsAsFile() || !wfsFile.hasFileExtension ("wfs"))
        return {};

    return wfsFile.getParentDirectory();
}

//==============================================================================
// File Paths
//==============================================================================

juce::File WFSFileManager::getCompleteConfigFile() const
{
    if (! projectFolder.isDirectory()) return {};
    return projectFolder.getChildFile ("show" + juce::String (completeConfigExtension));
}

juce::File WFSFileManager::getSystemConfigFile() const
{
    if (! projectFolder.isDirectory()) return {};
    return projectFolder.getChildFile ("system" + juce::String (systemConfigExtension));
}

juce::File WFSFileManager::getInputConfigFile() const
{
    if (! projectFolder.isDirectory()) return {};
    return projectFolder.getChildFile ("inputs" + juce::String (inputConfigExtension));
}

juce::File WFSFileManager::getOutputConfigFile() const
{
    if (! projectFolder.isDirectory()) return {};
    return projectFolder.getChildFile ("outputs" + juce::String (outputConfigExtension));
}

juce::File WFSFileManager::getReverbConfigFile() const
{
    if (! projectFolder.isDirectory()) return {};
    return projectFolder.getChildFile ("reverbs" + juce::String (reverbConfigExtension));
}

juce::File WFSFileManager::getAudioPatchFile() const
{
    if (! projectFolder.isDirectory()) return {};
    return projectFolder.getChildFile ("audio_patch" + juce::String (audioPatchExtension));
}

juce::File WFSFileManager::getNetworkConfigFile() const
{
    if (! projectFolder.isDirectory()) return {};
    return projectFolder.getChildFile ("network" + juce::String (networkConfigExtension));
}

juce::File WFSFileManager::getBackupFolder() const
{
    if (! projectFolder.isDirectory()) return {};
    return projectFolder.getChildFile ("backups");
}

juce::File WFSFileManager::getInputSnapshotsFolder() const
{
    if (! projectFolder.isDirectory()) return {};
    return projectFolder.getChildFile ("snapshots").getChildFile ("inputs");
}

juce::File WFSFileManager::getOutputSnapshotsFolder() const
{
    if (! projectFolder.isDirectory()) return {};
    return projectFolder.getChildFile ("snapshots").getChildFile ("outputs");
}

juce::File WFSFileManager::getScopeTemplatesFolder() const
{
    if (! projectFolder.isDirectory()) return {};
    return projectFolder.getChildFile ("snapshots").getChildFile ("scopes");
}

juce::File WFSFileManager::getIRFolder() const
{
    if (! projectFolder.isDirectory()) return {};
    return projectFolder.getChildFile ("ir");
}

juce::File WFSFileManager::getSamplesFolder() const
{
    if (! projectFolder.isDirectory()) return {};
    return projectFolder.getChildFile ("samples");
}

juce::File WFSFileManager::getSofaFolder() const
{
    if (! projectFolder.isDirectory()) return {};
    return projectFolder.getChildFile ("sofa");
}

//==============================================================================
// Complete Configuration
//==============================================================================

bool WFSFileManager::saveCompleteConfig()
{
    if (!hasValidProjectFolder())
    {
        setError (LOC ("fileManager.errors.noValidProjectFolder"));
        return false;
    }

    WFSLogger::getInstance().logInfo ("Saving complete config to " + projectFolder.getFullPathName());

    // Save all individual configuration files
    bool success = true;
    juce::StringArray errors;

    if (!saveSystemConfig())
    {
        success = false;
        errors.add (LOC ("fileManager.errors.prefixSystem") + lastError);
    }

    if (!saveNetworkConfig())
    {
        success = false;
        errors.add (LOC ("fileManager.errors.prefixNetwork") + lastError);
    }

    if (!saveInputConfig())
    {
        success = false;
        errors.add (LOC ("fileManager.errors.prefixInputs") + lastError);
    }

    if (!saveOutputConfig())
    {
        success = false;
        errors.add (LOC ("fileManager.errors.prefixOutputs") + lastError);
    }

    if (!saveReverbConfig())
    {
        success = false;
        errors.add (LOC ("fileManager.errors.prefixReverbs") + lastError);
    }

    if (!success)
        setError (errors.joinIntoString ("; "));

    return success;
}

bool WFSFileManager::loadCompleteConfig()
{
    if (!hasValidProjectFolder())
    {
        setError (LOC ("fileManager.errors.noValidProjectFolder"));
        DBG ("  ERROR: No valid project folder");
        return false;
    }

    WFSLogger::getInstance().logInfo ("Loading complete config from " + projectFolder.getFullPathName());

    // Clear any previous errors
    lastError = juce::String();

    // Checked as a PAIR: a consistent system.xml + inputs.xml is safe whatever
    // the session looks like (inputs.xml overwrites everything by number after
    // the inventory rebuilt the list by number), so the only hazard is the two
    // files disagreeing with each other - which "Store System Config" on its
    // own produces trivially. The inner loads then run under a bypass: the
    // per-file gate inside importSystemConfig cannot know that a consistent
    // inputs.xml is about to follow, and would refuse a pristine session
    // opening any project at all.
    if (channelIdentityBypassDepth == 0)
    {
        if (channelIdentityClearance == getSystemConfigFile())
            channelIdentityClearance = juce::File();
        else
        {
            const auto pair = preflightProjectChannelIdentity (getSystemConfigFile(), getInputConfigFile());
            if (! isChannelIdentitySafe (pair, LoadKind::projectPair))
            {
                setError (LOC ("fileManager.errors.channelListMismatchNotConfirmed")
                              .replace ("{path}", getSystemConfigFile().getFullPathName()));
                WFSLogger::getInstance().logWarning ("Load refused: system.xml and inputs.xml disagree about the channel list ("
                                                     + summariseChannelIdentityDiff (pair) + ") and the load was not confirmed");
                return false;
            }
        }
    }
    ScopedChannelIdentityBypass innerLoadsBypass (*this);

    // Load all individual configuration files
    bool success = true;
    juce::StringArray errors;

    // Note: No undo transaction needed for config reload - changes are intentional and don't need undo

    if (!loadSystemConfig())
    {
        success = false;
        errors.add (LOC ("fileManager.errors.prefixSystem") + lastError);
        DBG ("  FAILED: System - " << lastError);
    }

    if (!loadNetworkConfig())
    {
        success = false;
        errors.add (LOC ("fileManager.errors.prefixNetwork") + lastError);
        DBG ("  FAILED: Network - " << lastError);
    }

    if (!loadInputConfig())
    {
        success = false;
        errors.add (LOC ("fileManager.errors.prefixInputs") + lastError);
        DBG ("  FAILED: Inputs - " << lastError);
    }

    if (!loadOutputConfig())
    {
        success = false;
        errors.add (LOC ("fileManager.errors.prefixOutputs") + lastError);
        DBG ("  FAILED: Outputs - " << lastError);
    }

    if (!loadReverbConfig())
    {
        success = false;
        errors.add (LOC ("fileManager.errors.prefixReverbs") + lastError);
        DBG ("  FAILED: Reverbs - " << lastError);
    }

    if (!success)
        setError (errors.joinIntoString ("; "));

    // Only when something was actually read back: pointing at a NEW or empty
    // project folder loads nothing, and that session has to stay fresh — dense
    // display-order numbering while no external reference can exist yet is the
    // whole reason the latch exists. (Saving latches too, at the store funnels,
    // so a fresh session ends at its first save OR its first load.)
    if (success)
        valueTreeState.markChannelNumbersUserOwned ("complete config load");

    return success;
}

bool WFSFileManager::loadCompleteConfigBackup (int backupIndex)
{
    // Clear any previous errors
    lastError = juce::String();

    // Same pair rule as loadCompleteConfig, against the backup set. A backup set
    // with no inputs file degenerates to a system-only load and is checked as
    // one against the live session.
    if (channelIdentityBypassDepth == 0)
    {
        const auto sysBackups = getBackups ("system");
        const auto insBackups = getBackups ("inputs");
        const juce::File sysFile = backupIndex < sysBackups.size() ? sysBackups[backupIndex] : juce::File();
        const juce::File insFile = backupIndex < insBackups.size() ? insBackups[backupIndex] : juce::File();

        if (sysFile != juce::File() && channelIdentityClearance == sysFile)
            channelIdentityClearance = juce::File();
        else if (sysFile != juce::File())
        {
            const bool havePair = insFile.existsAsFile();
            const auto diff = havePair ? preflightProjectChannelIdentity (sysFile, insFile)
                                       : preflightChannelIdentity (sysFile, LoadKind::systemConfig);
            if (! isChannelIdentitySafe (diff, havePair ? LoadKind::projectPair : LoadKind::systemConfig))
            {
                setError (LOC ("fileManager.errors.channelListMismatchNotConfirmed").replace ("{path}", sysFile.getFullPathName()));
                WFSLogger::getInstance().logWarning ("Backup load refused: channel list mismatch ("
                                                     + summariseChannelIdentityDiff (diff) + ") and not confirmed");
                return false;
            }
        }
    }
    ScopedChannelIdentityBypass innerLoadsBypass (*this);

    // Load most recent backups for each file type
    bool success = true;
    juce::StringArray errors;

    // Note: No undo transaction needed for config reload - changes are intentional and don't need undo

    if (!loadSystemConfigBackup (backupIndex))
    {
        success = false;
        errors.add (LOC ("fileManager.errors.prefixSystem") + lastError);
    }

    if (!loadNetworkConfigBackup (backupIndex))
    {
        success = false;
        errors.add (LOC ("fileManager.errors.prefixNetwork") + lastError);
    }

    if (!loadInputConfigBackup (backupIndex))
    {
        success = false;
        errors.add (LOC ("fileManager.errors.prefixInputs") + lastError);
    }

    if (!loadOutputConfigBackup (backupIndex))
    {
        success = false;
        errors.add (LOC ("fileManager.errors.prefixOutputs") + lastError);
    }

    if (!loadReverbConfigBackup (backupIndex))
    {
        success = false;
        errors.add (LOC ("fileManager.errors.prefixReverbs") + lastError);
    }

    if (!success)
        setError (errors.joinIntoString ("; "));

    // Same success-only rule as loadCompleteConfig: a backup set that restored
    // nothing must leave a fresh session's numbering alone.
    if (success)
        valueTreeState.markChannelNumbersUserOwned ("complete config backup load");

    return success;
}

bool WFSFileManager::exportCompleteConfig (const juce::File& file)
{
    return writeToXmlFile (valueTreeState.getState(), file);
}

bool WFSFileManager::importCompleteConfig (const juce::File& file)
{
    auto loadedState = readFromXmlFile (file);

    if (!loadedState.isValid())
        return false;

    if (!valueTreeState.validateState (loadedState))
    {
        setError (LOC ("fileManager.errors.invalidConfigStructure"));
        return false;
    }

    // A wholesale replace brings channels and their data in together, so this
    // always passes - it is here so that "every import primitive gates" stays a
    // property of the code rather than a convention.
    if (! passChannelIdentityGate (file, LoadKind::completeConfig, loadedState))
        return false;

    // Note: No undo transaction needed for config import - changes are intentional and don't need undo
    valueTreeState.replaceState (loadedState);
    valueTreeState.enforceAllSharedClusterInvariants();
    return true;
}

//==============================================================================
// System Configuration
//==============================================================================

bool WFSFileManager::saveSystemConfig()
{
    if (!hasValidProjectFolder())
    {
        setError (LOC ("fileManager.errors.noValidProjectFolder"));
        return false;
    }

    WFSLogger::getInstance().logInfo ("Saving system config");

    // The first save ends the fresh session: this file records the channel
    // counts, the patch and the channel inventory, so the numbers it writes are
    // durable the moment it exists — a later reload restores exactly them.
    // Latched BEFORE extraction so the property lands in this very file, and so
    // covering the session-exit auto-save, which funnels through here.
    valueTreeState.markChannelNumbersUserOwned ("system config save");

    auto file = getSystemConfigFile();

    if (file.existsAsFile())
        createBackup (file);

    // Create a tree with config and audio patch
    juce::ValueTree systemState ("SystemConfig");
    systemState.setProperty (WFSParameterIDs::version, "1.0", nullptr);
    systemState.appendChild (extractConfigSection().createCopy(), nullptr);
    systemState.appendChild (extractAudioPatchSection().createCopy(), nullptr);
    stripTransientToggles (systemState);

    bool ok = writeToXmlFile (systemState, file);
    if (ok)
        systemConfigSynced = true;  // Explicit save re-syncs memory with the folder
    return ok;
}

bool WFSFileManager::autoSaveSystemConfig()
{
    if (!hasValidProjectFolder())
        return false;

    // Never clobber an existing system.xml the user hasn't loaded (or saved) this
    // session: right after selecting a work folder the in-memory state is still
    // the previous/default config. A folder with no system.xml is safe to initialise.
    if (!systemConfigSynced && getSystemConfigFile().existsAsFile())
    {
        WFSLogger::getInstance().logInfo ("Auto-save of system config skipped: config not yet loaded from this project folder");
        return false;
    }

    return saveSystemConfig();
}

bool WFSFileManager::loadSystemConfig()
{
    if (!hasValidProjectFolder())
    {
        setError (LOC ("fileManager.errors.noValidProjectFolder"));
        return false;
    }

    WFSLogger::getInstance().logInfo ("Loading system config");
    bool ok = importSystemConfig (getSystemConfigFile());
    if (ok)
        systemConfigSynced = true;
    return ok;
}

bool WFSFileManager::loadSystemConfigBackup (int backupIndex)
{
    auto backups = getBackups ("system");
    if (backupIndex >= 0 && backupIndex < backups.size())
    {
        bool ok = importSystemConfig (backups[backupIndex]);
        if (ok)
            systemConfigSynced = true;
        return ok;
    }

    setError (LOC ("fileManager.errors.backupNotFound"));
    return false;
}

bool WFSFileManager::exportSystemConfig (const juce::File& file)
{
    juce::ValueTree systemState ("SystemConfig");
    systemState.setProperty (WFSParameterIDs::version, "1.0", nullptr);
    systemState.appendChild (extractConfigSection().createCopy(), nullptr);
    systemState.appendChild (extractAudioPatchSection().createCopy(), nullptr);
    stripTransientToggles (systemState);

    return writeToXmlFile (systemState, file);
}

bool WFSFileManager::importSystemConfig (const juce::File& file)
{
    OriginTagScope originScope { OriginTag::Snapshot };

    auto loadedState = readFromXmlFile (file);
    if (!loadedState.isValid())
        return false;

    // Note: Transaction management should be done by caller (e.g., loadCompleteConfig)
    // to avoid nested transactions. Individual callers should begin their own transaction.

    bool appliedSomething = false;

    stripTransientToggles (loadedState);

    // Before anything is applied: refused unless safe, cleared, or bypassed.
    if (! passChannelIdentityGate (file, LoadKind::systemConfig, loadedState))
        return false;

    auto configTree = loadedState.getChildWithName (Config);
    if (configTree.isValid())
    {
        applyConfigSection (configTree);
        appliedSomething = true;
    }

    auto audioPatchTree = loadedState.getChildWithName (AudioPatch);
    if (audioPatchTree.isValid())
    {
        applyAudioPatchSection (audioPatchTree);
        appliedSomething = true;
    }

    if (!appliedSomething)
        setError (LOC ("fileManager.errors.noSystemDataInFile").replace ("{path}", file.getFullPathName()));

    if (appliedSomething)
    {
        // Cluster modes may have changed without a property transition event
        // (e.g. mode already 2 in both state and file): re-assert coincidence.
        valueTreeState.enforceAllSharedClusterInvariants();
        valueTreeState.clearAllUndoHistories();
    }

    return appliedSomething;
}

//==============================================================================
// Network Configuration
//==============================================================================

bool WFSFileManager::saveNetworkConfig()
{
    if (!hasValidProjectFolder())
    {
        setError (LOC ("fileManager.errors.noValidProjectFolder"));
        return false;
    }

    WFSLogger::getInstance().logInfo ("Saving network config");
    auto file = getNetworkConfigFile();

    if (file.existsAsFile())
        createBackup (file);

    juce::ValueTree networkState ("NetworkConfig");
    networkState.setProperty (WFSParameterIDs::version, "1.0", nullptr);
    networkState.appendChild (extractNetworkSection().createCopy(), nullptr);

    return writeToXmlFile (networkState, file);
}

bool WFSFileManager::loadNetworkConfig()
{
    if (!hasValidProjectFolder())
    {
        setError (LOC ("fileManager.errors.noValidProjectFolder"));
        return false;
    }

    WFSLogger::getInstance().logInfo ("Loading network config");
    return importNetworkConfig (getNetworkConfigFile());
}

bool WFSFileManager::loadNetworkConfigBackup (int backupIndex)
{
    auto backups = getBackups ("network");
    if (backupIndex >= 0 && backupIndex < backups.size())
        return importNetworkConfig (backups[backupIndex]);

    setError (LOC ("fileManager.errors.backupNotFound"));
    return false;
}

bool WFSFileManager::exportNetworkConfig (const juce::File& file)
{
    juce::ValueTree networkState ("NetworkConfig");
    networkState.setProperty (WFSParameterIDs::version, "1.0", nullptr);
    networkState.appendChild (extractNetworkSection().createCopy(), nullptr);

    return writeToXmlFile (networkState, file);
}

bool WFSFileManager::importNetworkConfig (const juce::File& file)
{
    OriginTagScope originScope { OriginTag::Snapshot };

    auto loadedState = readFromXmlFile (file);
    if (!loadedState.isValid())
        return false;

    // Note: Transaction management should be done by caller (e.g., loadCompleteConfig)
    // to avoid nested transactions. Individual callers should begin their own transaction.

    // Look for NetworkSettings container (new format)
    auto networkSettings = loadedState.getChildWithName ("NetworkSettings");
    if (networkSettings.isValid())
    {
        bool result = applyNetworkSection (networkSettings);
        if (result)
            valueTreeState.clearAllUndoHistories();
        return result;
    }

    // Fallback: try loading old format with just Network child
    auto networkTree = loadedState.getChildWithName (Network);
    if (networkTree.isValid())
    {
        // Wrap in container for applyNetworkSection
        juce::ValueTree container ("NetworkSettings");
        container.appendChild (networkTree.createCopy(), nullptr);
        bool result = applyNetworkSection (container);
        if (result)
            valueTreeState.clearAllUndoHistories();
        return result;
    }

    setError (LOC ("fileManager.errors.noNetworkDataInFile"));
    return false;
}

//==============================================================================
// Input Configuration
//==============================================================================

bool WFSFileManager::saveInputConfig()
{
    if (!hasValidProjectFolder())
    {
        setError (LOC ("fileManager.errors.noValidProjectFolder"));
        return false;
    }

    WFSLogger::getInstance().logInfo ("Saving input config");

    // Same first-save rule as saveSystemConfig: inputs.xml persists every
    // channel's number (<Input id=...>), which makes them durable on disk.
    valueTreeState.markChannelNumbersUserOwned ("input config save");

    auto file = getInputConfigFile();

    if (file.existsAsFile())
        createBackup (file);

    juce::ValueTree inputState ("InputConfig");
    inputState.setProperty (WFSParameterIDs::version, "1.0", nullptr);
    inputState.appendChild (extractInputsSection().createCopy(), nullptr);
    {
        // Hardware-input fingerprint per channel, stamped into the COPY: a guard
        // for the next load, never a source to repatch from (see hwInputs).
        auto inputsCopy = inputState.getChildWithName (Inputs);
        stampHardwareFingerprints (inputsCopy);
    }
    stripTransientToggles (inputState);

    return writeToXmlFile (inputState, file);
}

bool WFSFileManager::loadInputConfig()
{
    if (!hasValidProjectFolder())
    {
        setError (LOC ("fileManager.errors.noValidProjectFolder"));
        return false;
    }

    WFSLogger::getInstance().logInfo ("Loading input config");
    return importInputConfig (getInputConfigFile());
}

bool WFSFileManager::loadInputConfigBackup (int backupIndex)
{
    auto backups = getBackups ("inputs");
    if (backupIndex >= 0 && backupIndex < backups.size())
        return importInputConfig (backups[backupIndex]);

    setError (LOC ("fileManager.errors.backupNotFound"));
    return false;
}

bool WFSFileManager::exportInputConfig (const juce::File& file)
{
    juce::ValueTree inputState ("InputConfig");
    inputState.setProperty (WFSParameterIDs::version, "1.0", nullptr);
    inputState.appendChild (extractInputsSection().createCopy(), nullptr);
    {
        // Hardware-input fingerprint per channel, stamped into the COPY: a guard
        // for the next load, never a source to repatch from (see hwInputs).
        auto inputsCopy = inputState.getChildWithName (Inputs);
        stampHardwareFingerprints (inputsCopy);
    }
    stripTransientToggles (inputState);

    return writeToXmlFile (inputState, file);
}

bool WFSFileManager::importInputConfig (const juce::File& file)
{
    OriginTagScope originScope { OriginTag::Snapshot };

    auto loadedState = readFromXmlFile (file);
    if (!loadedState.isValid())
        return false;

    stripTransientToggles (loadedState);

    auto inputsTree = loadedState.getChildWithName (Inputs);
    if (!inputsTree.isValid())
    {
        setError (LOC ("fileManager.errors.noInputDataInFile"));
        return false;
    }

    if (! passChannelIdentityGate (file, LoadKind::inputConfig, loadedState))
        return false;

    bool result = applyInputsSection (inputsTree);
    if (result)
    {
        // Bulk position writes bypass setInputParameter's shared-position
        // propagation; old projects may contain diverged Shared-mode members.
        valueTreeState.enforceAllSharedClusterInvariants();
        valueTreeState.clearAllUndoHistories();
    }
    return result;
}

//==============================================================================
// Output Configuration
//==============================================================================

bool WFSFileManager::saveOutputConfig()
{
    if (!hasValidProjectFolder())
    {
        setError (LOC ("fileManager.errors.noValidProjectFolder"));
        return false;
    }

    WFSLogger::getInstance().logInfo ("Saving output config");
    auto file = getOutputConfigFile();

    if (file.existsAsFile())
        createBackup (file);

    juce::ValueTree outputState ("OutputConfig");
    outputState.setProperty (WFSParameterIDs::version, "1.0", nullptr);
    outputState.appendChild (extractOutputsSection().createCopy(), nullptr);

    return writeToXmlFile (outputState, file);
}

bool WFSFileManager::loadOutputConfig()
{
    if (!hasValidProjectFolder())
    {
        setError (LOC ("fileManager.errors.noValidProjectFolder"));
        return false;
    }

    WFSLogger::getInstance().logInfo ("Loading output config");
    return importOutputConfig (getOutputConfigFile());
}

bool WFSFileManager::loadOutputConfigBackup (int backupIndex)
{
    auto backups = getBackups ("outputs");
    if (backupIndex >= 0 && backupIndex < backups.size())
        return importOutputConfig (backups[backupIndex]);

    setError (LOC ("fileManager.errors.backupNotFound"));
    return false;
}

bool WFSFileManager::exportOutputConfig (const juce::File& file)
{
    juce::ValueTree outputState ("OutputConfig");
    outputState.setProperty (WFSParameterIDs::version, "1.0", nullptr);
    outputState.appendChild (extractOutputsSection().createCopy(), nullptr);

    return writeToXmlFile (outputState, file);
}

bool WFSFileManager::importOutputConfig (const juce::File& file)
{
    OriginTagScope originScope { OriginTag::Snapshot };

    auto loadedState = readFromXmlFile (file);
    if (!loadedState.isValid())
        return false;

    auto outputsTree = loadedState.getChildWithName (Outputs);
    if (!outputsTree.isValid())
    {
        setError (LOC ("fileManager.errors.noOutputDataInFile"));
        return false;
    }

    bool result = applyOutputsSection (outputsTree);
    if (result)
        valueTreeState.clearAllUndoHistories();
    return result;
}

//==============================================================================
// Reverb Configuration
//==============================================================================

bool WFSFileManager::saveReverbConfig()
{
    if (!hasValidProjectFolder())
    {
        setError (LOC ("fileManager.errors.noValidProjectFolder"));
        return false;
    }

    WFSLogger::getInstance().logInfo ("Saving reverb config");
    auto file = getReverbConfigFile();

    if (file.existsAsFile())
        createBackup (file);

    juce::ValueTree reverbState ("ReverbConfig");
    reverbState.setProperty (WFSParameterIDs::version, "1.0", nullptr);
    reverbState.appendChild (extractReverbsSection().createCopy(), nullptr);

    return writeToXmlFile (reverbState, file);
}

bool WFSFileManager::loadReverbConfig()
{
    if (!hasValidProjectFolder())
    {
        setError (LOC ("fileManager.errors.noValidProjectFolder"));
        return false;
    }

    WFSLogger::getInstance().logInfo ("Loading reverb config");
    return importReverbConfig (getReverbConfigFile());
}

bool WFSFileManager::loadReverbConfigBackup (int backupIndex)
{
    auto backups = getBackups ("reverbs");
    if (backupIndex >= 0 && backupIndex < backups.size())
        return importReverbConfig (backups[backupIndex]);

    setError (LOC ("fileManager.errors.backupNotFound"));
    return false;
}

bool WFSFileManager::exportReverbConfig (const juce::File& file)
{
    juce::ValueTree reverbState ("ReverbConfig");
    reverbState.setProperty (WFSParameterIDs::version, "1.0", nullptr);
    reverbState.appendChild (extractReverbsSection().createCopy(), nullptr);

    return writeToXmlFile (reverbState, file);
}

bool WFSFileManager::importReverbConfig (const juce::File& file)
{
    OriginTagScope originScope { OriginTag::Snapshot };

    auto loadedState = readFromXmlFile (file);
    if (!loadedState.isValid())
        return false;

    auto reverbsTree = loadedState.getChildWithName (Reverbs);
    if (!reverbsTree.isValid())
    {
        setError (LOC ("fileManager.errors.noReverbDataInFile"));
        return false;
    }

    bool result = applyReverbsSection (reverbsTree);
    if (result)
        valueTreeState.clearAllUndoHistories();
    return result;
}

//==============================================================================
// Cluster LFO Presets
//==============================================================================

bool WFSFileManager::exportClusterLFOPresets (const juce::File& file)
{
    juce::ValueTree root ("ClusterLFOPresetsConfig");
    root.setProperty (WFSParameterIDs::version, "1.0", nullptr);
    root.appendChild (valueTreeState.getClusterLFOPresetsSection().createCopy(), nullptr);

    return writeToXmlFile (root, file);
}

bool WFSFileManager::importClusterLFOPresets (const juce::File& file)
{
    OriginTagScope originScope { OriginTag::Snapshot };

    auto loadedState = readFromXmlFile (file);
    if (! loadedState.isValid())
        return false;

    auto presetsTree = loadedState.getChildWithName (WFSParameterIDs::ClusterLFOPresets);
    if (! presetsTree.isValid())
    {
        setError (LOC ("fileManager.errors.noLFOPresetDataInFile"));
        return false;
    }

    // Replace existing presets section in Config
    auto config = valueTreeState.getConfigState();
    auto existing = config.getChildWithName (WFSParameterIDs::ClusterLFOPresets);
    if (existing.isValid())
        config.removeChild (existing, nullptr);
    config.appendChild (presetsTree.createCopy(), nullptr);
    return true;
}

//==============================================================================
// Snapshots
//==============================================================================

bool WFSFileManager::deleteInputSnapshot (const juce::String& snapshotName)
{
    auto file = getInputSnapshotsFolder().getChildFile (snapshotName + snapshotExtension);
    if (file.existsAsFile())
        return file.deleteFile();

    setError (LOC ("fileManager.errors.snapshotNotFound"));
    return false;
}

juce::StringArray WFSFileManager::getInputSnapshotNames() const
{
    juce::StringArray names;
    auto folder = getInputSnapshotsFolder();

    if (folder.isDirectory())
    {
        for (auto& file : folder.findChildFiles (juce::File::findFiles, false, "*" + juce::String (snapshotExtension)))
            names.add (file.getFileNameWithoutExtension());
    }

    return names;
}

juce::String WFSFileManager::getDefaultSnapshotName()
{
    return juce::Time::getCurrentTime().formatted ("%Y%m%d_%H%M%S");
}

std::vector<WFSFileManager::MidiBinding> WFSFileManager::scanSnapshotMidiBindings() const
{
    std::vector<MidiBinding> result;

    auto folder = getInputSnapshotsFolder();
    if (! folder.isDirectory())
        return result;

    auto files = folder.findChildFiles (juce::File::findFiles, false,
                                        "*" + juce::String (snapshotExtension));

    // findChildFiles order is filesystem-dependent; sorting makes the winner of
    // a duplicate binding deterministic and identical on every machine.
    files.sort();

    for (const auto& file : files)
    {
        // Outer element only: root tag + attributes, a few hundred bytes, no
        // matter how many megabytes of <Inputs> follow. This is the entire
        // reason the binding lives on the root and not inside <ExtendedScope>.
        juce::XmlDocument doc (file);
        auto root = doc.getDocumentElement (true);

        if (root == nullptr || ! root->hasTagName ("InputSnapshot"))
            continue;

        MidiBinding binding;
        binding.channel = root->getStringAttribute (midiChannel.toString()).getIntValue();
        binding.note    = root->getStringAttribute (midiNote.toString()).getIntValue();

        if (binding.channel < 1 || binding.channel > 16
            || binding.note < 0 || binding.note > 127)
            continue;  // absent or garbage attributes = unbound

        binding.snapshotName = file.getFileNameWithoutExtension();
        result.push_back (std::move (binding));
    }

    return result;
}

//==============================================================================
// Snapshot Scope - Static Definitions
//==============================================================================

const std::vector<WFSFileManager::ScopeItem>& WFSFileManager::ExtendedSnapshotScope::getScopeItems()
{
    static std::vector<ScopeItem> items = {
        // Input Section
        { "inputAttenuation", "Attenuation", Channel, { inputAttenuation } },
        { "inputDelay", "Delay/Latency", Channel, { inputDelayLatency, inputMinimalLatency } },
        // Stereo pairs: the image (how wide, along which axis). Which channels
        // ARE stereo is config-level (stereoInputChannels in System Config),
        // never per-channel state, so snapshots cannot carry or change it.
        // The itemId is the key stored in saved scope templates — it stays
        // "stereo" whatever the group grows to cover.
        { "stereo", "Stereo Image", Channel, { inputStereoWidth, inputStereoAxisOffset } },
        // Map display state. Not show state in the DSP sense, but it is state the
        // operator sets by hand and would otherwise have to redo after every
        // recall. inputSolo is deliberately NOT here: it is transient monitoring.
        // inputHiddenByCluster is deliberately NOT here either — it is a cache of
        // (inputCluster, clusterInputsVisible) that ClustersTab recomputes for
        // every channel in a callAsync after any inputCluster write, so a recalled
        // value is overwritten a message-loop tick later. Snapshotting the cluster
        // toggle itself is the fix, and that is a separate change.
        { "mapDisplay", "Map Lock/Visibility", Channel, { inputMapLocked, inputMapVisible } },

        // Position Section
        { "position", "Position (XYZ)", Position, { inputPositionX, inputPositionY, inputPositionZ, inputCoordinateMode } },
        { "offset", "Offset (XYZ)", Position, { inputOffsetX, inputOffsetY, inputOffsetZ } },
        { "constraints", "Constraints", Position, { inputConstraintX, inputConstraintY, inputConstraintZ, inputConstraintDistance, inputConstraintDistanceMin, inputConstraintDistanceMax } },
        { "flip", "Flip (XYZ)", Position, { inputFlipX, inputFlipY, inputFlipZ } },
        { "cluster", "Cluster", Position, { inputCluster } },
        { "tracking", "Tracking", Position, { inputTrackingActive, inputTrackingID, inputTrackingSmooth } },
        { "speedLimit", "Speed Limit", Position, { inputMaxSpeedActive, inputMaxSpeed } },
        { "pathMode", "Path Mode", Position, { inputPathModeActive } },
        { "heightFactor", "Height Factor", Position, { inputHeightFactor } },

        // Attenuation Section
        { "attenuationLaw", "Attenuation Law", Attenuation, { inputAttenuationLaw, inputDistanceAttenuation, inputDistanceRatio } },
        { "commonAtten", "Common Atten", Attenuation, { inputCommonAtten } },

        // Directivity Section
        { "directivity", "Directivity", Directivity, { inputDirectivity, inputRotation, inputTilt } },
        { "hfShelf", "HF Shelf", Directivity, { inputHFshelf } },

        // Live Source Tamer Section
        { "lsEnable", "Enable", LiveSourceTamer, { inputLSactive } },
        { "lsRadiusShape", "Radius/Shape", LiveSourceTamer, { inputLSradius, inputLSshape } },
        { "lsFixedAtten", "Fixed Atten", LiveSourceTamer, { inputLSattenuation } },
        { "lsPeakComp", "Peak Comp", LiveSourceTamer, { inputLSpeakEnable, inputLSpeakThreshold, inputLSpeakRatio } },
        { "lsSlowComp", "Slow Comp", LiveSourceTamer, { inputLSslowEnable, inputLSslowThreshold, inputLSslowRatio } },

        // Hackoustics Section
        { "frEnable", "Enable", Hackoustics, { inputFRactive } },
        { "frAttenuation", "Attenuation", Hackoustics, { inputFRattenuation } },
        { "frLowCut", "Low Cut", Hackoustics, { inputFRlowCutActive, inputFRlowCutFreq } },
        { "frHighShelf", "High Shelf", Hackoustics, { inputFRhighShelfActive, inputFRhighShelfFreq, inputFRhighShelfGain, inputFRhighShelfSlope } },
        { "frDiffusion", "Diffusion", Hackoustics, { inputFRdiffusion } },
        { "reverbSends", "Reverb Sends", Hackoustics, { inputMuteReverbSends } },

        // LFO Section
        { "lfoEnable", "Enable/Period", LFO, { inputLFOactive, inputLFOperiod, inputLFOphase, inputLFOgyrophone } },
        { "lfoX", "LFO X", LFO, { inputLFOshapeX, inputLFOrateX, inputLFOamplitudeX, inputLFOphaseX } },
        { "lfoY", "LFO Y", LFO, { inputLFOshapeY, inputLFOrateY, inputLFOamplitudeY, inputLFOphaseY } },
        { "lfoZ", "LFO Z", LFO, { inputLFOshapeZ, inputLFOrateZ, inputLFOamplitudeZ, inputLFOphaseZ } },
        { "jitter", "Jitter", LFO, { inputJitter } },

        // AutomOtion Section
        // The destination is whatever the coordinate mode says it is, so all three
        // representations belong to ONE item. Carrying only the Cartesian triplet
        // was not merely lossy, it half-applied: inputOtomoZ is shared with the
        // cylindrical form, so recalling in cylindrical mode restored the height
        // while leaving R and Theta live, producing a destination matching neither
        // the snapshot nor the pre-recall state. AutomOtionProcessor reads the mode
        // and the polar targets at trigger time, so this is live motion, not a
        // display convenience.
        // inputOtomoPauseResume is deliberately absent: it is the run-state of a
        // motion in flight, like inputSolo, not show state.
        { "otomoDestination", "Destination", AutomOtion, { inputOtomoX, inputOtomoY, inputOtomoZ, inputOtomoAbsoluteRelative,
                                                            inputOtomoCoordinateMode, inputOtomoR, inputOtomoTheta,
                                                            inputOtomoRsph, inputOtomoPhi } },
        { "otomoMovement", "Movement", AutomOtion, { inputOtomoStayReturn, inputOtomoDuration, inputOtomoCurve, inputOtomoSpeedProfile } },
        { "otomoAudioTrigger", "Audio Trigger", AutomOtion, { inputOtomoTrigger, inputOtomoThreshold, inputOtomoReset } },

        // Mutes Section
        { "mutes", "Mutes", Mutes, { inputMutes, inputMuteMacro } },
        { "sidelines", "Sidelines", Mutes, { inputSidelinesActive, inputSidelinesFringe } },
        { "arrayAttens", "Array Attens", Mutes, { inputArrayAtten1, inputArrayAtten2, inputArrayAtten3, inputArrayAtten4, inputArrayAtten5, inputArrayAtten6, inputArrayAtten7, inputArrayAtten8, inputArrayAtten9, inputArrayAtten10 } },

        // Gradient Maps Section (subtree-based — parameterIds are layer property IDs for display, actual save/load uses subtree copy)
        { "gmLayer1", "Layer 1", GradientMaps, { gmLayerEnabled, gmLayerParam, gmLayerWhite, gmLayerBlack, gmLayerCurve, gmLayerVisible } },
        { "gmLayer2", "Layer 2", GradientMaps, { gmLayerEnabled, gmLayerParam, gmLayerWhite, gmLayerBlack, gmLayerCurve, gmLayerVisible } },
        { "gmLayer3", "Layer 3", GradientMaps, { gmLayerEnabled, gmLayerParam, gmLayerWhite, gmLayerBlack, gmLayerCurve, gmLayerVisible } },

        // Sampler Section (subtree-based — cells and sets are children, not properties)
        // lightpadZoneId rides the existing "sampler" item deliberately, rather
        // than getting an id of its own: withGlobals force-excludes the literal
        // "sampler" when the sampler master is off, and the grid hides the whole
        // Sampler section in the same condition — a separate id would stay active
        // while being invisible, so the operator could not turn it off.
        { "sampler", "Sampler", Sampler, { inputSamplerActive, inputSamplerActiveSet, lightpadZoneId } },

        // ADM-OSC Section
        { "admMapping", "ADM Mapping", ADMMapping, { inputAdmMapping } }
    };
    return items;
}

const std::vector<juce::Identifier>& WFSFileManager::ExtendedSnapshotScope::getSectionIds()
{
    static std::vector<juce::Identifier> sections = {
        Channel, Position, Attenuation, Directivity, LiveSourceTamer,
        Hackoustics, LFO, AutomOtion, Mutes, GradientMaps, Sampler, ADMMapping
    };
    return sections;
}

std::vector<const WFSFileManager::ScopeItem*> WFSFileManager::ExtendedSnapshotScope::getItemsForSection (const juce::Identifier& sectionId)
{
    std::vector<const ScopeItem*> result;
    for (const auto& item : getScopeItems())
    {
        if (item.sectionId == sectionId)
            result.push_back (&item);
    }
    return result;
}

//==============================================================================
// Extended Snapshot Scope - Instance Methods
//==============================================================================

juce::String WFSFileManager::ExtendedSnapshotScope::makeKey (const juce::String& itemId, int channelIndex)
{
    return itemId + "_" + juce::String (channelIndex);
}

bool WFSFileManager::ExtendedSnapshotScope::isIncluded (const juce::String& itemId, int channelIndex) const
{
    auto key = makeKey (itemId, channelIndex);
    auto it = itemChannelStates.find (key);
    return it == itemChannelStates.end() ? true : it->second;  // Default: included
}

bool WFSFileManager::ExtendedSnapshotScope::isParameterIncluded (const juce::Identifier& paramId, int channelIndex) const
{
    // Find which scope item contains this parameter
    for (const auto& item : getScopeItems())
    {
        for (const auto& pid : item.parameterIds)
        {
            if (pid == paramId)
                return isIncluded (item.itemId, channelIndex);
        }
    }
    return true;  // Unknown parameters are included by default
}

bool WFSFileManager::ExtendedSnapshotScope::isEquivalentTo (const ExtendedSnapshotScope& other, int numChannels) const
{
    if (applyMode != other.applyMode)
        return false;

    // The MIDI trigger is part of the scope object, so a binding-only edit must
    // register as a difference -- this is the sole gate on the scope window's
    // "Update Snapshot Scope" button, and without it such an edit is silently
    // discarded when the window closes.
    if (midiChannel != other.midiChannel || midiNote != other.midiNote)
        return false;

    for (const auto& item : getScopeItems())
        for (int ch = 0; ch < numChannels; ++ch)
            if (isIncluded (item.itemId, ch) != other.isIncluded (item.itemId, ch))
                return false;

    return true;
}

void WFSFileManager::ExtendedSnapshotScope::setIncluded (const juce::String& itemId, int channelIndex, bool included)
{
    auto key = makeKey (itemId, channelIndex);
    if (included)
        itemChannelStates.erase (key);  // Remove from map (default is included)
    else
        itemChannelStates[key] = false;
}

void WFSFileManager::ExtendedSnapshotScope::toggle (const juce::String& itemId, int channelIndex)
{
    setIncluded (itemId, channelIndex, !isIncluded (itemId, channelIndex));
}

void WFSFileManager::ExtendedSnapshotScope::setAllItemsForChannel (int channelIndex, bool included)
{
    for (const auto& item : getScopeItems())
        setIncluded (item.itemId, channelIndex, included);
}

void WFSFileManager::ExtendedSnapshotScope::setItemForAllChannels (const juce::String& itemId, bool included, int numChannels)
{
    for (int ch = 0; ch < numChannels; ++ch)
        setIncluded (itemId, ch, included);
}

void WFSFileManager::ExtendedSnapshotScope::setSectionForAllChannels (const juce::Identifier& sectionId, bool included, int numChannels)
{
    for (const auto& item : getScopeItems())
    {
        if (item.sectionId == sectionId)
        {
            for (int ch = 0; ch < numChannels; ++ch)
                setIncluded (item.itemId, ch, included);
        }
    }
}

void WFSFileManager::ExtendedSnapshotScope::setAll (bool included, int numChannels)
{
    if (included)
    {
        itemChannelStates.clear();  // Clear map = all included (default)
    }
    else
    {
        for (const auto& item : getScopeItems())
        {
            for (int ch = 0; ch < numChannels; ++ch)
                setIncluded (item.itemId, ch, false);
        }
    }
}

WFSFileManager::ExtendedSnapshotScope::InclusionState
WFSFileManager::ExtendedSnapshotScope::getSectionState (const juce::Identifier& sectionId, int numChannels) const
{
    int includedCount = 0;
    int totalCount = 0;

    for (const auto& item : getScopeItems())
    {
        if (item.sectionId == sectionId)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                ++totalCount;
                if (isIncluded (item.itemId, ch))
                    ++includedCount;
            }
        }
    }

    if (includedCount == 0) return InclusionState::AllExcluded;
    if (includedCount == totalCount) return InclusionState::AllIncluded;
    return InclusionState::Partial;
}

WFSFileManager::ExtendedSnapshotScope::InclusionState
WFSFileManager::ExtendedSnapshotScope::getSectionStateForChannel (const juce::Identifier& sectionId, int channelIndex) const
{
    int includedCount = 0;
    int totalCount = 0;

    for (const auto& item : getScopeItems())
    {
        if (item.sectionId == sectionId)
        {
            ++totalCount;
            if (isIncluded (item.itemId, channelIndex))
                ++includedCount;
        }
    }

    if (includedCount == 0) return InclusionState::AllExcluded;
    if (includedCount == totalCount) return InclusionState::AllIncluded;
    return InclusionState::Partial;
}

WFSFileManager::ExtendedSnapshotScope::InclusionState
WFSFileManager::ExtendedSnapshotScope::getChannelState (int channelIndex) const
{
    int includedCount = 0;
    int totalCount = 0;

    for (const auto& item : getScopeItems())
    {
        ++totalCount;
        if (isIncluded (item.itemId, channelIndex))
            ++includedCount;
    }

    if (includedCount == 0) return InclusionState::AllExcluded;
    if (includedCount == totalCount) return InclusionState::AllIncluded;
    return InclusionState::Partial;
}

WFSFileManager::ExtendedSnapshotScope::InclusionState
WFSFileManager::ExtendedSnapshotScope::getOverallState (int numChannels) const
{
    if (itemChannelStates.empty())
        return InclusionState::AllIncluded;

    int includedCount = 0;
    int totalCount = 0;

    for (const auto& item : getScopeItems())
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            ++totalCount;
            if (isIncluded (item.itemId, ch))
                ++includedCount;
        }
    }

    if (includedCount == 0) return InclusionState::AllExcluded;
    if (includedCount == totalCount) return InclusionState::AllIncluded;
    return InclusionState::Partial;
}

void WFSFileManager::ExtendedSnapshotScope::initializeDefaults (int numChannels)
{
    juce::ignoreUnused (numChannels);
    itemChannelStates.clear();
    applyMode = ApplyMode::OnRecall;
    clearMidiBinding();  // a fresh scope must never inherit another snapshot's note
    // All scope items default to included (missing = included convention)
}

WFSFileManager::ExtendedSnapshotScope
WFSFileManager::ExtendedSnapshotScope::withGlobals (bool samplerMasterOn, int numChannels) const
{
    ExtendedSnapshotScope eff = *this;
    if (!samplerMasterOn)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            eff.setIncluded ("sampler", ch, false);
    }
    return eff;
}

//==============================================================================
// Extended Snapshot Scope - File Operations
//==============================================================================

bool WFSFileManager::saveInputSnapshotWithExtendedScope (const juce::String& snapshotName, const ExtendedSnapshotScope& scope)
{
    // The one choke point for the Inputs-tab store button, the auto-store paths
    // and OSC /wfs/input/snapshot/store. Every entry below goes to disk as
    // <Input id="NUMBER"> and recall resolves it through getSlotForChannelNumber,
    // so the moment a single snapshot file exists those numbers are durable and
    // renumbering them would silently repoint every stored channel.
    valueTreeState.markChannelNumbersUserOwned ("input snapshot store");

    auto folder = getInputSnapshotsFolder();
    folder.createDirectory();

    auto file = folder.getChildFile (snapshotName + snapshotExtension);

    juce::ValueTree snapshot ("InputSnapshot");
    snapshot.setProperty (version, "2.0", nullptr);  // Version 2.0 for extended scope
    snapshot.setProperty (name, snapshotName, nullptr);

    // This function builds a BRAND-NEW tree and overwrites the file, so both
    // "Store Snapshot" and "Update Snapshot" would otherwise destroy an
    // existing MIDI binding. The caller is responsible for having carried the
    // previous binding into `scope` when storing over an existing name.
    writeMidiBindingToRoot (snapshot, scope);

    int numInputs = valueTreeState.getNumInputChannels();

    // Serialize extended scope
    snapshot.appendChild (serializeExtendedScope (scope, numInputs), nullptr);

    // Store input data (filtered by scope if ApplyMode is OnSave)
    juce::ValueTree inputsData (Inputs);

    for (int i = 0; i < numInputs; ++i)
    {
        if (scope.applyMode == ExtendedSnapshotScope::ApplyMode::OnSave)
            inputsData.appendChild (extractInputWithExtendedScope (i, scope), nullptr);
        else
            inputsData.appendChild (extractInputWithExtendedScope (i, ExtendedSnapshotScope()), nullptr);  // All included
    }

    // Ghost entries: keep data saved for channel numbers that are currently
    // deleted. Recall skips them, but nothing is silently destroyed — they
    // apply again if the number is ever re-created. Carried over from the
    // existing file before it is overwritten.
    if (file.existsAsFile())
    {
        auto existing = readFromXmlFile (file);
        auto oldInputs = existing.getChildWithName (Inputs);
        for (int i = 0; i < oldInputs.getNumChildren(); ++i)
        {
            auto entry = oldInputs.getChild (i);
            const int number = static_cast<int> (entry.getProperty (id, 0));
            if (number > 0 && valueTreeState.getSlotForChannelNumber (number) < 0)
                inputsData.appendChild (entry.createCopy(), nullptr);
        }
    }

    snapshot.appendChild (inputsData, nullptr);
    stripTransientToggles (snapshot);

    return writeToXmlFile (snapshot, file);
}

bool WFSFileManager::loadInputSnapshotWithExtendedScope (const juce::String& snapshotName, const ExtendedSnapshotScope& scope)
{
    OriginTagScope originScope { OriginTag::Snapshot };

    // A recall normally implies an earlier store or load that already latched,
    // but a snapshots folder can also arrive with the project folder (copied
    // show, shared template) without either having run this session.
    valueTreeState.markChannelNumbersUserOwned ("input snapshot recall");

    auto file = getInputSnapshotsFolder().getChildFile (snapshotName + snapshotExtension);
    auto snapshot = readFromXmlFile (file);

    if (!snapshot.isValid())
        return false;

    stripTransientToggles (snapshot);

    auto inputsData = snapshot.getChildWithName (Inputs);
    if (!inputsData.isValid())
    {
        setError (LOC ("fileManager.errors.noInputDataInSnapshot"));
        return false;
    }

    valueTreeState.beginUndoTransaction ("Load Input Snapshot: " + snapshotName);
    lastRecallSkippedNumbers.clear();

    for (int i = 0; i < inputsData.getNumChildren(); ++i)
    {
        auto inputData = inputsData.getChild (i);
        // Snapshot entries are keyed by permanent channel number. Entries for
        // numbers with no live channel ("ghosts" of deleted channels) are
        // skipped — and deliberately kept in the file: they apply again if
        // the number is ever re-created. Skips are recorded so the recall can
        // say so; it used to be silent about them.
        const int number = static_cast<int> (inputData.getProperty (id));
        int channelIndex = valueTreeState.getSlotForChannelNumber (number);

        if (channelIndex < 0)
            lastRecallSkippedNumbers.push_back (number);

        if (channelIndex >= 0)
        {
            if (scope.applyMode == ExtendedSnapshotScope::ApplyMode::OnRecall)
                applyInputWithExtendedScope (channelIndex, inputData, scope);
            else
                applyInputWithExtendedScope (channelIndex, inputData, ExtendedSnapshotScope());  // All included
        }
    }

    // Snapshot positions can re-diverge a Shared-mode cluster (per-channel raw
    // apply); snap members back onto the first-ordered member.
    valueTreeState.enforceAllSharedClusterInvariants();

    return true;
}

WFSFileManager::ExtendedSnapshotScope WFSFileManager::getExtendedSnapshotScope (const juce::String& snapshotName) const
{
    ExtendedSnapshotScope scope;
    auto file = getInputSnapshotsFolder().getChildFile (snapshotName + snapshotExtension);
    auto snapshot = const_cast<WFSFileManager*>(this)->readFromXmlFile (file);

    if (snapshot.isValid())
    {
        auto scopeTree = snapshot.getChildWithName ("ExtendedScope");
        if (scopeTree.isValid())
            scope = const_cast<WFSFileManager*>(this)->deserializeExtendedScope (scopeTree);

        // Read AFTER the scope tree: deserializeExtendedScope returns a fresh
        // scope object, which would otherwise overwrite the binding.
        readMidiBindingFromRoot (snapshot, scope);
    }

    return scope;
}

bool WFSFileManager::setExtendedSnapshotScope (const juce::String& snapshotName, const ExtendedSnapshotScope& scope)
{
    auto file = getInputSnapshotsFolder().getChildFile (snapshotName + snapshotExtension);
    auto snapshot = readFromXmlFile (file);

    if (!snapshot.isValid())
    {
        setError (LOC ("fileManager.errors.snapshotNotFoundNamed").replace ("{name}", snapshotName));
        return false;
    }

    // Remove existing scope and add new one
    auto existingScope = snapshot.getChildWithName ("ExtendedScope");
    if (existingScope.isValid())
        snapshot.removeChild (existingScope, nullptr);

    int numInputs = valueTreeState.getNumInputChannels();
    snapshot.appendChild (serializeExtendedScope (scope, numInputs), nullptr);

    // Read-modify-write of the existing root, so this overwrites the attributes
    // (or removes them when the binding was cleared).
    writeMidiBindingToRoot (snapshot, scope);

    return writeToXmlFile (snapshot, file);
}

bool WFSFileManager::updateInputSnapshotScope (const juce::String& snapshotName, const ExtendedSnapshotScope& scope)
{
    auto file = getInputSnapshotsFolder().getChildFile (snapshotName + snapshotExtension);
    auto snapshot = readFromXmlFile (file);

    if (!snapshot.isValid())
    {
        setError (LOC ("fileManager.errors.snapshotNotFoundNamed").replace ("{name}", snapshotName));
        return false;
    }

    createBackup (file);

    // Replace the embedded scope
    auto existingScope = snapshot.getChildWithName ("ExtendedScope");
    if (existingScope.isValid())
        snapshot.removeChild (existingScope, nullptr);

    int numInputs = valueTreeState.getNumInputChannels();
    snapshot.appendChild (serializeExtendedScope (scope, numInputs), nullptr);

    // The normal write path for a binding edit made in the scope window.
    writeMidiBindingToRoot (snapshot, scope);

    // OnSave: the stored data is what recall applies, so trim it to the new
    // scope (removal only). OnRecall files keep their full data so the scope
    // can be broadened again later.
    if (scope.applyMode == ExtendedSnapshotScope::ApplyMode::OnSave)
    {
        auto inputsData = snapshot.getChildWithName (Inputs);
        for (int i = 0; i < inputsData.getNumChildren(); ++i)
        {
            auto inputData = inputsData.getChild (i);
            int channelIndex = static_cast<int> (inputData.getProperty (id)) - 1;
            if (channelIndex >= 0)
                trimSnapshotInputToScope (inputData, scope, channelIndex);
        }
    }

    stripTransientToggles (snapshot);
    return writeToXmlFile (snapshot, file);
}

//==============================================================================
// Scope Templates
//==============================================================================

bool WFSFileManager::saveScopeTemplate (const juce::String& templateName, const ExtendedSnapshotScope& scope)
{
    auto folder = getScopeTemplatesFolder();
    if (folder == juce::File())
    {
        setError (LOC ("fileManager.errors.noProjectFolder"));
        return false;
    }
    folder.createDirectory();

    juce::ValueTree tpl ("ScopeTemplate");
    tpl.setProperty (version, "1.0", nullptr);
    tpl.setProperty (name, templateName, nullptr);

    // Same embedded format as snapshot scopes; the applyMode property it
    // carries is ignored on template load (templates are grid-only).
    tpl.appendChild (serializeExtendedScope (scope, valueTreeState.getNumInputChannels()), nullptr);

    return writeToXmlFile (tpl, folder.getChildFile (templateName + snapshotExtension));
}

bool WFSFileManager::loadScopeTemplateGrid (const juce::String& templateName, ExtendedSnapshotScope& target)
{
    auto file = getScopeTemplatesFolder().getChildFile (templateName + snapshotExtension);
    auto tpl = readFromXmlFile (file);

    if (! tpl.isValid())
        return false;

    auto scopeTree = tpl.getChildWithName ("ExtendedScope");
    if (! scopeTree.isValid())
    {
        setError (LOC ("fileManager.errors.noScopeDataInTemplate"));
        return false;
    }

    auto loaded = deserializeExtendedScope (scopeTree);
    target.itemChannelStates = std::move (loaded.itemChannelStates);
    return true;
}

juce::StringArray WFSFileManager::getScopeTemplateNames() const
{
    juce::StringArray names;
    auto folder = getScopeTemplatesFolder();

    if (folder.isDirectory())
    {
        for (auto& file : folder.findChildFiles (juce::File::findFiles, false, "*" + juce::String (snapshotExtension)))
            names.add (file.getFileNameWithoutExtension());
    }

    return names;
}

bool WFSFileManager::deleteScopeTemplate (const juce::String& templateName)
{
    auto file = getScopeTemplatesFolder().getChildFile (templateName + snapshotExtension);
    if (file.existsAsFile())
        return file.deleteFile();

    setError (LOC ("fileManager.errors.snapshotNotFound"));
    return false;
}

namespace
{
    /** The <Channel> node's snapshot contract, in one place.

        <Channel> is the only one of the nine input sections that cannot use the
        generic copySection/applySection/trim loops: those iterate ScopeItems and
        match on the node's own properties, but three items whose parameters live
        on <Channel> are filed under other display sections (`sampler`), and one
        property must always be written whatever the scope says (`inputName`).

        It used to be three hand-written allowlists — extract, apply and trim —
        with no coupling whatsoever, so a property added to one and forgotten in
        another failed silently and differently in each direction: missing from
        extract meant never stored, missing from apply meant stored but never
        recalled, missing from trim meant an excluded item's value survived on
        disk and got applied anyway. This table is the single source all three
        now read.

        `inputName` is deliberately absent: it is always captured and always
        applied, and is handled explicitly at each site. `inputSolo` is
        deliberately absent too — it is transient monitoring state, not show
        state, and must never be carried by a snapshot. Anything else added to
        the <Channel> node should be added HERE, not at a call site. */
    struct ChannelSnapshotProperty
    {
        const char* itemId;                 // ScopeItem that gates it
        const juce::Identifier& propertyId; // property on the <Channel> node
    };

    const std::vector<ChannelSnapshotProperty>& channelSnapshotProperties()
    {
        using namespace WFSParameterIDs;
        static const std::vector<ChannelSnapshotProperty> props = {
            { "inputAttenuation", inputAttenuation },
            { "inputDelay",       inputDelayLatency },
            { "inputDelay",       inputMinimalLatency },
            { "stereo",           inputStereoWidth },
            { "stereo",           inputStereoAxisOffset },
            { "sampler",          inputSamplerActive },
            // Map display state: an operator who has hidden or locked channels
            // on the Map should not have to redo it after every recall.
            { "mapDisplay",       inputMapLocked },
            { "mapDisplay",       inputMapVisible },
            // Pad-zone assignment is real setup work. See resolveLightpadZoneCollisions
            // for what happens when a partial recall makes two channels claim one zone.
            { "sampler",          lightpadZoneId },
        };
        return props;
    }
}

bool WFSFileManager::isPropertyCoveredBySnapshotScope (const juce::Identifier& propertyId)
{
    for (const auto& item : ExtendedSnapshotScope::getScopeItems())
        for (const auto& paramId : item.parameterIds)
            if (paramId == propertyId)
                return true;

    for (const auto& prop : channelSnapshotProperties())
        if (prop.propertyId == propertyId)
            return true;

    return false;
}

void WFSFileManager::trimSnapshotInputToScope (juce::ValueTree& inputData, const ExtendedSnapshotScope& scope, int channelIndex)
{
    // Removal only, in place: anything the scope excludes is deleted; everything
    // else (including unknown/legacy properties) is left untouched. The global
    // sampler master is deliberately NOT folded in here (withGlobals) — it must
    // not silently delete stored sampler data while the master happens to be off.

    // Channel section — inputName is always kept
    auto channelTree = inputData.getChildWithName (Channel);
    if (channelTree.isValid())
    {
        for (const auto& prop : channelSnapshotProperties())
            if (! scope.isIncluded (prop.itemId, channelIndex))
                channelTree.removeProperty (prop.propertyId, nullptr);
    }

    // Property-based sections: drop excluded items' parameters
    static const juce::Identifier propertySections[] = {
        Position, Attenuation, Directivity, LiveSourceTamer,
        Hackoustics, LFO, AutomOtion, Mutes
    };

    for (const auto& sectionId : propertySections)
    {
        auto sectionTree = inputData.getChildWithName (sectionId);
        if (!sectionTree.isValid())
            continue;

        for (const auto& item : ExtendedSnapshotScope::getScopeItems())
        {
            // NOT filtered on item.sectionId. A ScopeItem's sectionId is a
            // DISPLAY grouping — the scope grid is built from it and it mirrors
            // the GUI tab layout, not the ValueTree. Three items are filed under
            // the tab the operator finds them on rather than the node they live
            // on (jitter: Position/shown under LFO; reverbSends: Mutes/shown
            // under Hackoustics; admMapping: Position/shown under its own
            // pseudo-section), and matching on it silently skipped them on BOTH
            // save and recall — symmetrically, so no round-trip test could see
            // it. hasProperty is the exact discriminator anyway: no paramId
            // exists on two different <Input> child nodes, so a param can match
            // at most one section. (The gmLayer* ids repeat across gmLayer1/2/3
            // but live in the GradientMaps subtree, which is copied whole and
            // never reaches these eight property sections.)
            if (! scope.isIncluded (item.itemId, channelIndex))
                for (const auto& paramId : item.parameterIds)
                    sectionTree.removeProperty (paramId, nullptr);
        }

        if (sectionTree.getNumProperties() == 0 && sectionTree.getNumChildren() == 0)
            inputData.removeChild (sectionTree, nullptr);
    }

    // Gradient Maps — layers matched by their 0-based `id` (positional fallback
    // for pre-id-era files; kept layers get tagged so the result is unambiguous)
    auto gmTree = inputData.getChildWithName (GradientMaps);
    if (gmTree.isValid())
    {
        const juce::String layerItemIds[] = { "gmLayer1", "gmLayer2", "gmLayer3" };

        for (int li = gmTree.getNumChildren() - 1; li >= 0; --li)
        {
            auto layerChild = gmTree.getChild (li);
            int layerIdx = static_cast<int> (layerChild.getProperty (id, li));

            if (layerIdx < 0 || layerIdx >= 3)
                continue;  // unrecognisable layer — leave untouched

            if (!scope.isIncluded (layerItemIds[layerIdx], channelIndex))
                gmTree.removeChild (li, nullptr);
            else if (!layerChild.hasProperty (id))
                layerChild.setProperty (id, layerIdx, nullptr);
        }

        if (gmTree.getNumChildren() == 0)
            inputData.removeChild (gmTree, nullptr);
    }

    // Sampler — whole-subtree removal (cells + dynamic set children)
    if (!scope.isIncluded ("sampler", channelIndex))
    {
        auto samplerTree = inputData.getChildWithName (Sampler);
        if (samplerTree.isValid())
            inputData.removeChild (samplerTree, nullptr);
    }

    // admMapping is trimmed by the <Position> pass above (inputAdmMapping lives
    // on <Position>; ADMMapping is a display-only pseudo-section with no node
    // of its own). Before the sectionId filter was dropped it was never
    // captured at all, which is what "nothing to trim" used to mean here.
}

void WFSFileManager::writeMidiBindingToRoot (juce::ValueTree& snapshot, const ExtendedSnapshotScope& scope)
{
    if (scope.hasMidiBinding())
    {
        snapshot.setProperty (midiChannel, scope.midiChannel, nullptr);
        snapshot.setProperty (midiNote,    scope.midiNote,    nullptr);
    }
    else
    {
        // Absence IS the unbound representation. A leftover midiChannel="0"
        // would read back identically but would make every snapshot look bound
        // to a grep, and would lose the "no attribute until bound" property
        // that makes the backward-compatibility story checkable by eye.
        snapshot.removeProperty (midiChannel, nullptr);
        snapshot.removeProperty (midiNote,    nullptr);
    }
}

void WFSFileManager::readMidiBindingFromRoot (const juce::ValueTree& snapshot, ExtendedSnapshotScope& scope)
{
    // juce::ValueTree::fromXml returns EVERY property as a STRING var, so read
    // through getIntValue() rather than an isInt()-guarded cast -- that is this
    // codebase's documented "works new, broken saved" trap.
    scope.midiChannel = snapshot.getProperty (midiChannel).toString().getIntValue();
    scope.midiNote    = snapshot.getProperty (midiNote).toString().getIntValue();

    if (! scope.hasMidiBinding())
        scope.clearMidiBinding();  // normalise garbage / out-of-range to "unbound"
}

juce::ValueTree WFSFileManager::serializeExtendedScope (const ExtendedSnapshotScope& scope, int numChannels) const
{
    juce::ValueTree scopeTree ("ExtendedScope");
    scopeTree.setProperty ("applyMode", scope.applyMode == ExtendedSnapshotScope::ApplyMode::OnSave ? "OnSave" : "OnRecall", nullptr);

    // Find channels that are fully included, fully excluded, or partial
    std::vector<int> fullChannels, excludedChannels, partialChannels;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto state = scope.getChannelState (ch);
        if (state == ExtendedSnapshotScope::InclusionState::AllIncluded)
            fullChannels.push_back (ch);
        else if (state == ExtendedSnapshotScope::InclusionState::AllExcluded)
            excludedChannels.push_back (ch);
        else
            partialChannels.push_back (ch);
    }

    // On disk, channels are identified by their PERMANENT number (identical
    // to slot + 1 for legacy dense files, so old snapshots parse unchanged);
    // in memory the scope stays slot-keyed.
    // Serialize full channels
    if (!fullChannels.empty())
    {
        juce::StringArray indices;
        for (int ch : fullChannels)
            indices.add (juce::String (valueTreeState.getInputChannelNumber (ch)));
        scopeTree.setProperty ("fullChannels", indices.joinIntoString (","), nullptr);
    }

    // Serialize excluded channels
    if (!excludedChannels.empty())
    {
        juce::StringArray indices;
        for (int ch : excludedChannels)
            indices.add (juce::String (valueTreeState.getInputChannelNumber (ch)));
        scopeTree.setProperty ("excludedChannels", indices.joinIntoString (","), nullptr);
    }

    // Serialize partial channels
    for (int ch : partialChannels)
    {
        juce::ValueTree partialTree ("PartialChannel");
        partialTree.setProperty ("index", valueTreeState.getInputChannelNumber (ch), nullptr);

        // Collect excluded items for this channel (store whichever list is shorter)
        juce::StringArray excludedItems;
        for (const auto& item : ExtendedSnapshotScope::getScopeItems())
        {
            if (!scope.isIncluded (item.itemId, ch))
                excludedItems.add (item.itemId);
        }

        if (!excludedItems.isEmpty())
            partialTree.setProperty ("excludedItems", excludedItems.joinIntoString (","), nullptr);

        scopeTree.appendChild (partialTree, nullptr);
    }

    return scopeTree;
}

WFSFileManager::ExtendedSnapshotScope WFSFileManager::deserializeExtendedScope (const juce::ValueTree& scopeTree) const
{
    ExtendedSnapshotScope scope;

    // Parse apply mode
    auto modeStr = scopeTree.getProperty ("applyMode").toString();
    scope.applyMode = (modeStr == "OnSave")
        ? ExtendedSnapshotScope::ApplyMode::OnSave
        : ExtendedSnapshotScope::ApplyMode::OnRecall;

    int numChannels = valueTreeState.getNumInputChannels();

    // Parse excluded channels (stored as permanent numbers; entries whose
    // number has no live channel are dropped)
    auto excludedStr = scopeTree.getProperty ("excludedChannels").toString();
    if (excludedStr.isNotEmpty())
    {
        juce::StringArray indices;
        indices.addTokens (excludedStr, ",", "");
        for (const auto& idx : indices)
        {
            int ch = valueTreeState.getSlotForChannelNumber (idx.getIntValue());
            if (ch >= 0 && ch < numChannels)
                scope.setAllItemsForChannel (ch, false);
        }
    }

    // Parse partial channels
    for (int i = 0; i < scopeTree.getNumChildren(); ++i)
    {
        auto partialTree = scopeTree.getChild (i);
        if (partialTree.getType().toString() == "PartialChannel")
        {
            int ch = valueTreeState.getSlotForChannelNumber (
                         static_cast<int> (partialTree.getProperty ("index")));
            if (ch >= 0 && ch < numChannels)
            {
                auto excludedItems = partialTree.getProperty ("excludedItems").toString();
                if (excludedItems.isNotEmpty())
                {
                    juce::StringArray items;
                    items.addTokens (excludedItems, ",", "");
                    for (const auto& itemId : items)
                        scope.setIncluded (itemId, ch, false);
                }
            }
        }
    }

    return scope;
}

bool WFSFileManager::isSamplerMasterOn() const
{
    auto config = valueTreeState.getConfigState();
    auto ui = config.getChildWithName (WFSParameterIDs::UI);
    return ui.isValid() && (bool) ui.getProperty (samplerEnabled, false);
}

juce::ValueTree WFSFileManager::extractInputWithExtendedScope (int channelIndex, const ExtendedSnapshotScope& scopeIn) const
{
    auto input = const_cast<WFSValueTreeState&>(valueTreeState).getInputState (channelIndex);
    if (!input.isValid())
        return {};

    // Fold the global sampler master into an effective scope so the sampler
    // inclusion decision is the same everywhere in this file.
    const bool samplerMasterOn = isSamplerMasterOn();
    const auto scope = scopeIn.withGlobals (samplerMasterOn, valueTreeState.getNumInputChannels());

    juce::ValueTree filtered (Input);
    // Snapshots are keyed by the PERMANENT channel number, not the slot —
    // after deletions the list has gaps and slot + 1 would mis-key entries.
    filtered.setProperty (id, valueTreeState.getInputChannelNumber (channelIndex), nullptr);
    // Hardware-input fingerprint, so a recall can tell a snapshot stored under a
    // different configuration from one stored under this. Never applied.
    filtered.setProperty (hwInputsFingerprint,
                          InputChannelIdentityDetail::hwInputsToString (valueTreeState.getInputPatchHardwareInputs (channelIndex)),
                          nullptr);

    // Always include input name
    auto channelTree = input.getChildWithName (Channel);
    if (channelTree.isValid())
    {
        juce::ValueTree filteredChannel (Channel);
        filteredChannel.setProperty (inputName, channelTree.getProperty (inputName), nullptr);

        // Everything else on <Channel> comes from the one table. hasProperty is
        // checked on every property, not just some: the old hand-written version
        // guarded stereo and sampler but not attenuation or delay, so a channel
        // node missing one of those wrote a void var into the snapshot.
        for (const auto& prop : channelSnapshotProperties())
            if (scope.isIncluded (prop.itemId, channelIndex) && channelTree.hasProperty (prop.propertyId))
                filteredChannel.setProperty (prop.propertyId, channelTree.getProperty (prop.propertyId), nullptr);

        filtered.appendChild (filteredChannel, nullptr);
    }

    // Helper lambda to copy section properties based on scope items
    auto copySection = [&](const juce::Identifier& sectionId, const juce::ValueTree& sourceSection)
    {
        if (!sourceSection.isValid())
            return;

        juce::ValueTree filteredSection (sectionId);
        bool hasContent = false;

        for (const auto& item : ExtendedSnapshotScope::getScopeItems())
        {
            // NOT filtered on item.sectionId. A ScopeItem's sectionId is a
            // DISPLAY grouping — the scope grid is built from it and it mirrors
            // the GUI tab layout, not the ValueTree. Three items are filed under
            // the tab the operator finds them on rather than the node they live
            // on (jitter: Position/shown under LFO; reverbSends: Mutes/shown
            // under Hackoustics; admMapping: Position/shown under its own
            // pseudo-section), and matching on it silently skipped them on BOTH
            // save and recall — symmetrically, so no round-trip test could see
            // it. hasProperty is the exact discriminator anyway: no paramId
            // exists on two different <Input> child nodes, so a param can match
            // at most one section. (The gmLayer* ids repeat across gmLayer1/2/3
            // but live in the GradientMaps subtree, which is copied whole and
            // never reaches these eight property sections.)
            if (scope.isIncluded (item.itemId, channelIndex))
            {
                for (const auto& paramId : item.parameterIds)
                {
                    if (sourceSection.hasProperty (paramId))
                    {
                        filteredSection.setProperty (paramId, sourceSection.getProperty (paramId), nullptr);
                        hasContent = true;
                    }
                }
            }
        }

        if (hasContent)
            filtered.appendChild (filteredSection, nullptr);
    };

    copySection (Position, input.getChildWithName (Position));
    copySection (Attenuation, input.getChildWithName (Attenuation));
    copySection (Directivity, input.getChildWithName (Directivity));
    copySection (LiveSourceTamer, input.getChildWithName (LiveSourceTamer));
    copySection (Hackoustics, input.getChildWithName (Hackoustics));
    copySection (LFO, input.getChildWithName (LFO));
    copySection (AutomOtion, input.getChildWithName (AutomOtion));
    copySection (Mutes, input.getChildWithName (Mutes));

    // Gradient Maps — subtree copy (layers include variable-length shape children)
    {
        auto gmSource = input.getChildWithName (GradientMaps);
        if (gmSource.isValid())
        {
            juce::ValueTree gmFiltered (GradientMaps);
            bool hasContent = false;
            const juce::String layerItemIds[] = { "gmLayer1", "gmLayer2", "gmLayer3" };

            for (int li = 0; li < 3; ++li)
            {
                if (scope.isIncluded (layerItemIds[li], channelIndex))
                {
                    auto layerChild = gmSource.getChild (li);
                    if (layerChild.isValid())
                    {
                        // Layers are matched by their 0-based `id` at recall; tag
                        // pre-id-era trees so partially-scoped files stay unambiguous.
                        auto layerCopy = layerChild.createCopy();
                        if (!layerCopy.hasProperty (id))
                            layerCopy.setProperty (id, li, nullptr);
                        gmFiltered.appendChild (layerCopy, nullptr);
                        hasContent = true;
                    }
                }
            }

            if (hasContent)
                filtered.appendChild (gmFiltered, nullptr);
        }
    }

    // Sampler — subtree copy (cells + dynamic set children)
    // Effective scope already folds in the global master, so this single check
    // enforces both "master on" and "sampler in scope".
    if (scope.isIncluded ("sampler", channelIndex))
    {
        auto samplerSource = input.getChildWithName (Sampler);
        if (samplerSource.isValid())
            filtered.appendChild (samplerSource.createCopy(), nullptr);
    }

    return filtered;
}

bool WFSFileManager::applyInputWithExtendedScope (int channelIndex, const juce::ValueTree& inputData, const ExtendedSnapshotScope& scopeIn)
{
    auto input = valueTreeState.getInputState (channelIndex);
    if (!input.isValid())
        return false;

    // Fold the global sampler master into an effective scope — same rule as extract.
    const bool samplerMasterOn = isSamplerMasterOn();
    const auto scope = scopeIn.withGlobals (samplerMasterOn, valueTreeState.getNumInputChannels());

    auto* undoManager = valueTreeState.getUndoManager();

    // Helper lambda to apply section properties based on scope
    auto applySection = [&](const juce::Identifier& sectionId, const juce::ValueTree& sourceSection)
    {
        if (!sourceSection.isValid())
            return;

        auto targetSection = input.getChildWithName (sectionId);
        if (!targetSection.isValid())
            return;

        for (const auto& item : ExtendedSnapshotScope::getScopeItems())
        {
            // NOT filtered on item.sectionId. A ScopeItem's sectionId is a
            // DISPLAY grouping — the scope grid is built from it and it mirrors
            // the GUI tab layout, not the ValueTree. Three items are filed under
            // the tab the operator finds them on rather than the node they live
            // on (jitter: Position/shown under LFO; reverbSends: Mutes/shown
            // under Hackoustics; admMapping: Position/shown under its own
            // pseudo-section), and matching on it silently skipped them on BOTH
            // save and recall — symmetrically, so no round-trip test could see
            // it. hasProperty is the exact discriminator anyway: no paramId
            // exists on two different <Input> child nodes, so a param can match
            // at most one section. (The gmLayer* ids repeat across gmLayer1/2/3
            // but live in the GradientMaps subtree, which is copied whole and
            // never reaches these eight property sections.)
            if (scope.isIncluded (item.itemId, channelIndex))
            {
                for (const auto& paramId : item.parameterIds)
                {
                    if (sourceSection.hasProperty (paramId))
                        targetSection.setProperty (paramId, sourceSection.getProperty (paramId), undoManager);
                }
            }
        }
    };

    // Apply Channel section
    auto loadedChannel = inputData.getChildWithName (Channel);
    if (loadedChannel.isValid())
    {
        auto existingChannel = input.getChildWithName (Channel);
        if (existingChannel.isValid())
        {
            // Always apply name
            if (loadedChannel.hasProperty (inputName))
                existingChannel.setProperty (inputName, loadedChannel.getProperty (inputName), undoManager);

            // Same table as extract and trim, so the three can no longer disagree.
            for (const auto& prop : channelSnapshotProperties())
                if (scope.isIncluded (prop.itemId, channelIndex) && loadedChannel.hasProperty (prop.propertyId))
                    existingChannel.setProperty (prop.propertyId, loadedChannel.getProperty (prop.propertyId), undoManager);

        }
    }

    applySection (Position, inputData.getChildWithName (Position));
    applySection (Attenuation, inputData.getChildWithName (Attenuation));
    applySection (Directivity, inputData.getChildWithName (Directivity));
    applySection (LiveSourceTamer, inputData.getChildWithName (LiveSourceTamer));
    applySection (Hackoustics, inputData.getChildWithName (Hackoustics));
    applySection (LFO, inputData.getChildWithName (LFO));
    applySection (AutomOtion, inputData.getChildWithName (AutomOtion));
    applySection (Mutes, inputData.getChildWithName (Mutes));

    // Gradient Maps — subtree replacement (layers include variable-length shape children)
    {
        auto gmSource = inputData.getChildWithName (GradientMaps);
        if (gmSource.isValid())
        {
            auto gmTarget = input.getChildWithName (GradientMaps);
            if (!gmTarget.isValid())
            {
                gmTarget = valueTreeState.ensureInputGradientMapsSection (channelIndex);
            }

            const juce::String layerItemIds[] = { "gmLayer1", "gmLayer2", "gmLayer3" };

            // Match stored layers to live slots by their 0-based `id` (positional
            // fallback for pre-id-era files): scoped OnSave files omit excluded
            // layers, so position alone would assign the remainder to wrong slots.
            for (int si = 0; si < gmSource.getNumChildren(); ++si)
            {
                auto sourceLayer = gmSource.getChild (si);
                int layerIdx = static_cast<int> (sourceLayer.getProperty (id, si));

                if (layerIdx < 0 || layerIdx >= 3 || layerIdx >= gmTarget.getNumChildren())
                    continue;

                if (scope.isIncluded (layerItemIds[layerIdx], channelIndex))
                {
                    // Replace entire layer subtree (properties + shape children).
                    // Copy with `id` normalised to the target slot rather than
                    // stripped — copyPropertiesAndChildrenFrom removes absent
                    // properties, which would delete the live layer's id and
                    // break gradient-map dirty tracking.
                    auto layerData = sourceLayer.createCopy();
                    layerData.setProperty (id, layerIdx, nullptr);
                    auto targetLayer = gmTarget.getChild (layerIdx);
                    targetLayer.copyPropertiesAndChildrenFrom (layerData, undoManager);
                }
            }
        }
    }

    // Sampler — subtree replacement (cells + dynamic set children)
    // Effective scope already folds in the global master.
    if (scope.isIncluded ("sampler", channelIndex))
    {
        auto samplerSource = inputData.getChildWithName (Sampler);
        if (samplerSource.isValid())
        {
            auto samplerTarget = input.getChildWithName (Sampler);
            if (!samplerTarget.isValid())
                samplerTarget = valueTreeState.ensureInputSamplerSection (channelIndex);
            samplerTarget.copyPropertiesAndChildrenFrom (samplerSource, undoManager);
        }
    }

    return true;
}

//==============================================================================
// Backup Management
//==============================================================================

bool WFSFileManager::createBackup (const juce::File& file)
{
    return spatcore::control::state::XmlPersistence::createBackup (file, getBackupFolder());
}

juce::Array<juce::File> WFSFileManager::getBackups (const juce::String& fileType) const
{
    return spatcore::control::state::XmlPersistence::listBackups (getBackupFolder(), fileType);
}

void WFSFileManager::cleanupBackups (int keepCount)
{
    // Clean up each section file type (the WFS multi-file layout)
    spatcore::control::state::XmlPersistence::cleanupBackups (
        getBackupFolder(), { "system", "network", "inputs", "outputs", "reverbs" }, keepCount);
}

juce::String WFSFileManager::getBackupTimestamp()
{
    return spatcore::control::state::XmlPersistence::backupTimestamp();
}

//==============================================================================
// Internal Methods
//==============================================================================

bool WFSFileManager::writeToXmlFile (const juce::ValueTree& tree, const juce::File& file)
{
    using WriteResult = spatcore::control::state::XmlPersistence::WriteResult;

    switch (persistence.writeTreeToFile (tree, file))
    {
        case WriteResult::ok:
            return true;

        case WriteResult::xmlConversionFailed:
            setError (LOC ("fileManager.errors.failedCreateXML"));
            return false;

        case WriteResult::fileWriteFailed:
        default:
            setError (LOC ("fileManager.errors.failedWriteFile").replace ("{path}", file.getFullPathName()));
            return false;
    }
}

juce::ValueTree WFSFileManager::readFromXmlFile (const juce::File& file)
{
    using ReadError = spatcore::control::state::XmlPersistence::ReadError;

    auto result = persistence.readTreeFromFile (file);

    switch (result.error)
    {
        case ReadError::none:
            break;

        case ReadError::fileNotFound:
            setError (LOC ("fileManager.errors.fileNotFound").replace ("{path}", file.getFullPathName()));
            return {};

        case ReadError::parseFailed:
            setError (LOC ("fileManager.errors.failedParseXML").replace ("{path}", file.getFullPathName()));
            return {};

        case ReadError::treeConversionFailed:
        default:
            setError (LOC ("fileManager.errors.failedCreateValueTree").replace ("{path}", file.getFullPathName()));
            return {};
    }

    return result.tree;
}

juce::ValueTree WFSFileManager::extractConfigSection() const
{
    // Extract Config section but exclude Network, ADMOSC, and Tracking
    // (those are saved separately in network.xml)
    auto config = valueTreeState.getState().getChildWithName (Config);
    if (!config.isValid())
        return {};

    juce::ValueTree filtered (Config);

    // Copy properties
    for (int i = 0; i < config.getNumProperties(); ++i)
    {
        auto propName = config.getPropertyName (i);
        filtered.setProperty (propName, config.getProperty (propName), nullptr);
    }

    // Copy children except Network, ADMOSC, and Tracking
    for (int i = 0; i < config.getNumChildren(); ++i)
    {
        auto child = config.getChild (i);
        auto childType = child.getType();

        if (childType != Network && childType != ADMOSC && childType != Tracking)
            filtered.appendChild (child.createCopy(), nullptr);
    }

    // <IO> carries only a channel TOTAL, and the mono/stereo split plus the
    // display order live on the <Input> nodes, which go to inputs.xml. Without
    // the inventory a system config reloaded on its own rebuilds every channel
    // as mono, and the positional patch rows in this very file then land on the
    // wrong channels — a stereo row's two hardware columns on a mono channel.
    // Stamped into the COPY: the runtime tree keeps the <Input> nodes as its
    // single source of truth, so this cannot go stale behind them.
    auto io = filtered.getChildWithName (IO);
    if (io.isValid())
    {
        io.removeChild (io.getChildWithName (InputChannelList), nullptr);   // never two
        io.appendChild (valueTreeState.buildInputChannelInventory(), nullptr);
    }

    // clusterInputOrder is slot-keyed in memory and must not be written that way:
    // slots are defined by inputs.xml, this file is system.xml, and a load that
    // reconciles the channel list moves the slot space out from under the CSV.
    // Convert the COPY to permanent channel numbers and say so on the node — the
    // runtime tree stays slot-keyed, which is what every live consumer wants.
    auto clusters = filtered.getChildWithName (Clusters);
    if (clusters.isValid())
    {
        for (int c = 0; c < clusters.getNumChildren(); ++c)
        {
            auto cluster = clusters.getChild (c);
            const juce::String order = cluster.getProperty (clusterInputOrder, "").toString();
            if (order.isEmpty())
                continue;

            juce::StringArray tokens;
            tokens.addTokens (order, ",", "");
            juce::StringArray numbers;
            for (const auto& tok : tokens)
            {
                // 0 is never a valid channel number, so an unresolvable slot is
                // dropped rather than written out as one.
                const int number = valueTreeState.getInputChannelNumber (tok.trim().getIntValue());
                if (number > 0)
                    numbers.add (juce::String (number));
            }
            cluster.setProperty (clusterInputOrder, numbers.joinIntoString (","), nullptr);
        }

        clusters.setProperty (inputOrderKey, inputOrderKeyNumber, nullptr);
    }

    return filtered;
}

juce::ValueTree WFSFileManager::extractInputsSection() const
{
    return valueTreeState.getState().getChildWithName (Inputs);
}

juce::ValueTree WFSFileManager::extractOutputsSection() const
{
    return valueTreeState.getState().getChildWithName (Outputs);
}

juce::ValueTree WFSFileManager::extractReverbsSection() const
{
    return valueTreeState.getState().getChildWithName (Reverbs);
}

juce::ValueTree WFSFileManager::extractAudioPatchSection() const
{
    return valueTreeState.getState().getChildWithName (AudioPatch);
}

juce::ValueTree WFSFileManager::extractNetworkSection() const
{
    // Extract Network, ADMOSC, and Tracking sections from Config
    auto config = valueTreeState.getState().getChildWithName (Config);
    if (!config.isValid())
        return {};

    // Create a container for all network-related sections
    juce::ValueTree networkContainer ("NetworkSettings");

    auto network = config.getChildWithName (Network);
    if (network.isValid())
        networkContainer.appendChild (network.createCopy(), nullptr);

    auto admOsc = config.getChildWithName (ADMOSC);
    if (admOsc.isValid())
        networkContainer.appendChild (admOsc.createCopy(), nullptr);

    auto tracking = config.getChildWithName (Tracking);
    if (tracking.isValid())
        networkContainer.appendChild (tracking.createCopy(), nullptr);

    return networkContainer;
}

//==============================================================================
// Channel identity gate
//==============================================================================

juce::String WFSFileManager::summariseChannelIdentityDiff (const InputChannelIdentityDiff& diff)
{
    using R = InputChannelIdentityDiff::Relation;
    juce::String rel;
    switch (diff.relation)
    {
        case R::identical:            rel = "identical"; break;
        case R::orderOnly:            rel = "same channels, different order"; break;
        case R::positionalTypesMatch: rel = "arrangement matches by position, numbers differ"; break;
        case R::conflicting:          rel = "conflicting"; break;
        case R::fileHasNoIdentity:    rel = "file carries no channel list (count " + juce::String (diff.fileLegacyCount) + ")"; break;
    }
    return rel + "; retyped " + juce::String ((int) diff.retyped.size())
               + ", removed " + juce::String ((int) diff.removed.size())
               + ", added "   + juce::String ((int) diff.added.size())
               + ", patch differs on " + juce::String ((int) diff.patchDiffers.size());
}

InputChannelIdentity WFSFileManager::readFileChannelIdentity (const juce::ValueTree& root, LoadKind kind,
                                                               const juce::File& file) const
{
    InputChannelIdentity none;
    if (! root.isValid())
        return none;

    if (kind == LoadKind::inputConfig)
    {
        // Same legacy-tail source migrateInputChannelModel reads.
        auto io = const_cast<WFSValueTreeState&> (valueTreeState).getIOState();
        const int tail = io.isValid() ? static_cast<int> (io.getProperty (stereoInputChannels, 0)) : 0;
        return InputChannelIdentity::fromInputNodes (root.getChildWithName (Inputs), tail);
    }

    if (kind == LoadKind::systemConfig)
    {
        auto io = root.getChildWithName (Config).getChildWithName (IO);
        if (! io.isValid())
            return none;

        auto inv = InputChannelIdentity::fromInventory (io.getChildWithName (InputChannelList));
        if (inv.hasIdentity())
            return inv;

        // Pre-inventory file. The project's own inputs.xml is the next best
        // witness - and only the project's own: a backup or an imported file
        // has no sibling that is known to belong with it.
        if (file == getSystemConfigFile())
        {
            auto ins = persistence.readTreeFromFile (getInputConfigFile()).tree.getChildWithName (Inputs);
            auto fromNodes = InputChannelIdentity::fromInputNodes (
                ins, static_cast<int> (io.getProperty (stereoInputChannels, 0)));
            if (fromNodes.hasIdentity())
                return fromNodes;
        }
        none.legacyCount = static_cast<int> (io.getProperty (inputChannels, 0));
    }
    return none;
}

InputChannelIdentityDiff WFSFileManager::preflightChannelIdentity (const juce::File& file, LoadKind kind) const
{
    // persistence.readTreeFromFile, NOT readFromXmlFile: the latter is the
    // real load's reader and writes lastError on failure, so a failed peek
    // would poison the message the operator sees for the load that follows.
    auto root = persistence.readTreeFromFile (file).tree;
    return compareInputChannelIdentity (valueTreeState.getInputChannelIdentity(),
                                        readFileChannelIdentity (root, kind, file));
}

InputChannelIdentityDiff WFSFileManager::preflightProjectChannelIdentity (const juce::File& systemFile,
                                                                          const juce::File& inputsFile) const
{
    auto sysRoot = persistence.readTreeFromFile (systemFile).tree;
    auto insRoot = persistence.readTreeFromFile (inputsFile).tree;

    auto io  = sysRoot.getChildWithName (Config).getChildWithName (IO);
    auto inv = io.isValid() ? InputChannelIdentity::fromInventory (io.getChildWithName (InputChannelList))
                            : InputChannelIdentity();
    auto ins = InputChannelIdentity::fromInputNodes (
        insRoot.getChildWithName (Inputs),
        io.isValid() ? static_cast<int> (io.getProperty (stereoInputChannels, 0)) : 0);

    if (inv.hasIdentity() && ins.hasIdentity())
        return compareInputChannelIdentity (inv, ins);   // file against file

    if (! ins.hasIdentity())
        return compareInputChannelIdentity (valueTreeState.getInputChannelIdentity(),
                                            readFileChannelIdentity (sysRoot, LoadKind::systemConfig, systemFile));

    // Legacy system.xml with a readable inputs.xml: nothing to cross-check.
    InputChannelIdentityDiff same;
    return same;
}

InputChannelIdentityDiff WFSFileManager::preflightSnapshotChannelIdentity (const juce::String& snapshotName) const
{
    auto file = getInputSnapshotsFolder().getChildFile (snapshotName + snapshotExtension);
    auto root = persistence.readTreeFromFile (file).tree;
    return compareInputChannelIdentity (valueTreeState.getInputChannelIdentity(),
                                        InputChannelIdentity::fromSnapshot (root.getChildWithName (Inputs)));
}

bool WFSFileManager::isChannelIdentitySafe (const InputChannelIdentityDiff& diff, LoadKind kind) const
{
    using R = InputChannelIdentityDiff::Relation;

    if (kind == LoadKind::completeConfig)
        return true;   // wholesale replace: channels and their data arrive together

    if (kind == LoadKind::projectPair)
        return diff.relation == R::identical || diff.relation == R::orderOnly;

    // A fingerprint disagreement means "this file came from a differently
    // patched configuration"; even with identical numbers that is the
    // operator's call, not ours.
    if (! diff.patchDiffers.empty())
        return false;

    switch (diff.relation)
    {
        case R::identical:
            return true;

        case R::orderOnly:
            // A system config's own patch lands in file order after the nodes
            // are re-ordered, so order never crosses anything there. An inputs
            // config loads no patch: rows follow their nodes only when the rows
            // are known to be aligned with them (a prior inventory load); the
            // legacy path raw-moves nodes and would leave the rows behind.
            return kind == LoadKind::systemConfig || channelListFromInventory;

        case R::fileHasNoIdentity:
        {
            // The load will rebuild dense 1..N mono from the sum and apply the
            // patch by row. That reproduces the session exactly when the
            // session already IS dense, ascending and of that size.
            if (kind != LoadKind::systemConfig)
                return false;
            const auto live = valueTreeState.getInputChannelIdentity();
            if ((int) live.slots.size() != diff.fileLegacyCount)
                return false;
            for (const auto& r : live.slots)
                if (r.number != r.slot + 1)
                    return false;
            return true;
        }

        case R::positionalTypesMatch:
        case R::conflicting:
        default:
            return false;
    }
}

void WFSFileManager::grantChannelIdentityClearance (const juce::File& file)
{
    channelIdentityClearance = file;
}

bool WFSFileManager::passChannelIdentityGate (const juce::File& file, LoadKind kind, const juce::ValueTree& parsedRoot)
{
    if (channelIdentityBypassDepth > 0)
        return true;

    if (file != juce::File() && channelIdentityClearance == file)
    {
        channelIdentityClearance = juce::File();   // one shot
        return true;
    }

    const auto diff = compareInputChannelIdentity (valueTreeState.getInputChannelIdentity(),
                                                   readFileChannelIdentity (parsedRoot, kind, file));
    if (isChannelIdentitySafe (diff, kind))
        return true;

    // Refused, and loudly: the status bar gets lastError, the log gets the
    // shape of the disagreement. Silently applying it is the bug this exists
    // to prevent; silently refusing it would be a new one.
    setError (LOC ("fileManager.errors.channelListMismatchNotConfirmed").replace ("{path}", file.getFullPathName()));
    WFSLogger::getInstance().logWarning ("Load refused: the channel list in " + file.getFileName()
                                         + " differs from this session (" + summariseChannelIdentityDiff (diff)
                                         + ") and the load was not confirmed");
    return false;
}

void WFSFileManager::stampHardwareFingerprints (juce::ValueTree& inputsCopy) const
{
    if (! inputsCopy.isValid())
        return;
    for (int i = 0; i < inputsCopy.getNumChildren(); ++i)
    {
        auto child = inputsCopy.getChild (i);
        if (child.hasType (Input))
            child.setProperty (hwInputsFingerprint,
                               InputChannelIdentityDetail::hwInputsToString (valueTreeState.getInputPatchHardwareInputs (i)),
                               nullptr);
    }
}

void WFSFileManager::flushPendingClusterOrders()
{
    if (! pendingClusterOrders.valid)
        return;

    auto clusters = valueTreeState.getClustersState();
    if (! clusters.isValid())
        return;

    // Restore the file's own values, discarding whatever the merge and the
    // load-time remaps left behind. For a number-keyed file the conversion to
    // slots happens below; for a pre-marker file the CSVs are already slots and
    // are correct as written, because by now the live slot space is the one the
    // file was saved against.
    for (int c = 0; c < clusters.getNumChildren(); ++c)
    {
        auto cluster = clusters.getChild (c);
        const int clusterId = static_cast<int> (cluster.getProperty (WFSParameterIDs::id, 0));

        auto stored = pendingClusterOrders.byClusterId.find (clusterId);
        if (stored == pendingClusterOrders.byClusterId.end())
            continue;

        if (cluster.getProperty (clusterInputOrder, "").toString() != stored->second)
            cluster.setProperty (clusterInputOrder, stored->second, nullptr);
    }

    if (pendingClusterOrders.numberKeyed)
        valueTreeState.convertClusterOrdersNumbersToSlots();
}

bool WFSFileManager::applyConfigSection (const juce::ValueTree& configTree)
{
    auto existingConfig = valueTreeState.getConfigState();
    if (!existingConfig.isValid())
        return false;

    // A system config is a load like any other, even when it carries no <Inputs>:
    // it restores an inputChannels count from a show whose snapshots and external
    // cues already name channels by number.
    valueTreeState.markChannelNumbersUserOwned ("system config load");

    auto* undoManager = valueTreeState.getUndoManager();

    // Lift the channel inventory off the LOADED tree before the merge. It is a
    // file artifact: the <Input> nodes are the runtime source of truth, and a
    // live second description of them could only drift. Copied because
    // `configTree` is const and the merge below is free to outlive it; the
    // node the merge drags into the live tree is evicted after it.
    juce::ValueTree inventory;
    {
        auto loadedIO = configTree.getChildWithName (IO);
        if (loadedIO.isValid())
        {
            auto stored = loadedIO.getChildWithName (InputChannelList);
            if (stored.isValid())
                inventory = stored.createCopy();
        }
    }

    // Same reason, same timing: lift the cluster orders off the FILE before the
    // merge. They are positional and the channel list is about to be rebuilt
    // underneath them, so whatever the merge and the reconciliation's remaps
    // leave in the live tree is not trustworthy — the file's own values are.
    // Pre-merge is mandatory: the live tree always has clusterInputOrder="" on
    // all ten clusters, so afterwards an absent value and an empty one look
    // identical.
    pendingClusterOrders = {};
    {
        auto loadedClusters = configTree.getChildWithName (Clusters);
        if (loadedClusters.isValid())
        {
            pendingClusterOrders.valid = true;
            pendingClusterOrders.numberKeyed =
                loadedClusters.getProperty (inputOrderKey).toString() == inputOrderKeyNumber;

            for (int c = 0; c < loadedClusters.getNumChildren(); ++c)
            {
                auto cluster = loadedClusters.getChild (c);
                const int clusterId = static_cast<int> (cluster.getProperty (WFSParameterIDs::id, 0));
                if (clusterId > 0)
                    pendingClusterOrders.byClusterId[clusterId] =
                        cluster.getProperty (clusterInputOrder, "").toString();
            }
        }
    }

    // Merge properties and children from loaded config (preserves missing properties/children)
    // Network, ADMOSC, and Tracking are automatically preserved if not in configTree
    mergeTreeRecursive (existingConfig, configTree, undoManager);

    // Ensure channel children exist with proper structure (including EQ sections)
    // Loaded XML may have old-format Reverb children without EQ sections.
    auto ioSection = existingConfig.getChildWithName (IO);
    if (ioSection.isValid())
    {
        int inputCount = ioSection.getProperty (inputChannels, 0);
        int outputCount = ioSection.getProperty (outputChannels, 0);
        int reverbCount = ioSection.getProperty (reverbChannels, 0);

        // The merge copies children too, so the inventory rode in with it —
        // evict it again. Runtime state is the <Input> nodes; keeping a second
        // description of them around invites the two to disagree.
        ioSection.removeChild (ioSection.getChildWithName (InputChannelList), nullptr);

        // The marker rode in with the merge too. It describes a file, not a
        // runtime tree — leaving it would claim the in-memory CSVs are numbers,
        // which they are not.
        if (auto liveClusters = existingConfig.getChildWithName (Clusters); liveClusters.isValid())
            liveClusters.removeProperty (inputOrderKey, nullptr);

        channelListFromInventory = (inventory.isValid() && inventory.getNumChildren() > 0);

        if (channelListFromInventory)
        {
            // Numbers, types, display order and permanent-number gaps all in
            // one go. markChannelNumbersUserOwned() above is what makes this
            // safe: unlatched, the structural ops' tail would renumber the very
            // list being restored.
            valueTreeState.applyInputChannelInventory (inventory);
        }
        else
        {
            // Written before the inventory existed: the sum is all there is, so
            // every channel comes back mono. The patch repair on the load tail
            // is what keeps that from leaving two hardware inputs on a mono row.
            valueTreeState.setNumInputChannels (inputCount);
        }

        valueTreeState.setNumOutputChannels (outputCount);
        valueTreeState.setNumReverbChannels (reverbCount);
    }

    // The channel list has settled, so slots mean what the file meant. This
    // covers a system config loaded on its own; a complete load reaches
    // applyInputsSection afterwards, whose prune can shift slots again, and
    // which therefore flushes a second time. The flush is idempotent by design.
    flushPendingClusterOrders();

    return true;
}

bool WFSFileManager::applyInputsSection (const juce::ValueTree& inputsTree)
{
    auto existingInputs = valueTreeState.getInputsState();
    if (existingInputs.isValid())
    {
        // Captured before the merge: how many channels the config section left
        // in the tree. Compared against the file's own child count below to tell
        // a ghost apart from two files that are simply out of sync.
        const int liveCountBeforeMerge = existingInputs.getNumChildren();

        // Latched before the merge, not after it: the numbers about to come out
        // of the file are the ones OSC, ADM, snapshots, QLab, the plug-in and MCP
        // already point at, and any structural op running while still unlatched
        // would renumber them to display order. A file written by a build that
        // predates the latch carries no such property, which is why every load
        // path has to say so on its own.
        valueTreeState.markChannelNumbersUserOwned ("inputs config load");

        mergeTreeRecursive (existingInputs, inputsTree, valueTreeState.getUndoManager());

        // The fingerprints rode in with the merge. They describe the file's
        // patching, not the live one, and would go stale on the next re-patch.
        for (int i = 0; i < existingInputs.getNumChildren(); ++i)
            existingInputs.getChild (i).removeProperty (hwInputsFingerprint, nullptr);

        // mergeTreeRecursive appends unmatched source children but NEVER removes
        // a target child the source lacks, so anything the config section
        // invented survives the merge. When that section had no inventory to go
        // on it invented a dense 1..N from the sum, and a saved list with a
        // permanent-number gap then comes back one channel too long: save
        // 1,2,3,4,5,7 (inputChannels="6"), the guess makes 1..6, the merge
        // appends 7, and 6 is a ghost — a default channel that eats a render
        // source and, because patch rows are positional, shifts every row after
        // it. Drop those here.
        //
        // Two guards, because the cost of over-pruning is losing channels the
        // operator configured:
        //
        //  - Skipped when the config section HAD an inventory: it rebuilt the
        //    exact set, so a channel missing here means the two files are out of
        //    step, not that the channel was deleted.
        //  - Skipped unless the file's channel count MATCHES what the config
        //    section left behind. That equality is the ghost's signature: the
        //    guess produced the right number of channels and the wrong set, so
        //    every channel the file omits is one the merge replaced with an
        //    appended sibling. Different counts mean the two files disagree
        //    about the size of the show — a system config saved on its own,
        //    say — and there the file with fewer channels must not silently
        //    delete the operator's others.
        if (! channelListFromInventory
            && inputsTree.getNumChildren() == liveCountBeforeMerge)
        {
            juce::SortedSet<int> inFile;
            for (int i = 0; i < inputsTree.getNumChildren(); ++i)
            {
                const int number = static_cast<int> (
                    inputsTree.getChild (i).getProperty (WFSParameterIDs::id, 0));
                if (number > 0)
                    inFile.add (number);
            }

            if (! inFile.isEmpty())
            {
                for (int slot = existingInputs.getNumChildren(); --slot >= 0;)
                {
                    const int number = valueTreeState.getInputChannelNumber (slot);
                    if (number > 0 && ! inFile.contains (number))
                    {
                        WFSLogger::getInstance().logInfo (
                            "Load: dropping input channel " + juce::String (number)
                            + " - not present in the loaded channel list");
                        valueTreeState.removeInputChannel (number);
                    }
                }
            }
        }

        // The merge matches children by type + id and never applies the FILE's
        // child ORDER, so without this a saved drag-reorder comes back in
        // whatever order the pre-existing tree had — ascending, for the default
        // session a load starts from. That is not merely cosmetic: patchData
        // rows are positional (row = slot) and DO load in file order, so a tree
        // left ascending against file-ordered rows hands every reordered
        // channel a different channel's hardware patch.
        {
            int target = 0;
            for (int i = 0; i < inputsTree.getNumChildren(); ++i)
            {
                const int number = static_cast<int> (
                    inputsTree.getChild (i).getProperty (WFSParameterIDs::id, 0));
                if (number <= 0)
                    continue;

                // Resolved against the live tree each time: earlier moves in
                // this loop have already shifted the slots underneath us.
                const int from = valueTreeState.getSlotForChannelNumber (number);
                if (from < 0)
                    continue;   // in the file but not merged in — leave it out

                if (from != target)
                {
                    // Rows follow their nodes when the rows are known to be
                    // aligned with them - true after any inventory load, and
                    // kept true by every live op. Then this is a plain
                    // rearrangement and the live patch must travel with the
                    // channels, exactly as a drag would move it; raw-moving the
                    // node alone left the rows behind whenever inputs.xml was
                    // reloaded on its own, silently mis-patching every moved
                    // channel. On the legacy path the rows arrived in the
                    // FILE's order from the system config that preceded this,
                    // and the loop's job is to bring the nodes to them - so
                    // there the node moves alone. (No undo either way:
                    // structural.)
                    if (channelListFromInventory)
                        valueTreeState.moveInputChannelNodeAndRow (from, target);
                    else
                        existingInputs.moveChild (from, target, nullptr);
                }
                ++target;
            }
        }

        // Sync inputChannels count with actual number of input children.
        // The inputs file may have more entries than the system config's inputChannels property,
        // which was set earlier during loadSystemConfig.
        int actualCount = existingInputs.getNumChildren();
        valueTreeState.setNumInputChannels (actualCount);

        // Migration: ensure GradientMaps section exists for all inputs (handles old configs)
        for (int i = 0; i < actualCount; ++i)
            valueTreeState.ensureInputGradientMapsSection (i);

        // Stable-number model migration: repair ids and stamp per-channel
        // types on file children the merge brought in (tree order is kept —
        // it is the user's display order, restored above).
        valueTreeState.migrateInputChannelModel();

        // Last, because this is the point at which slots finally mean what the
        // system config meant by them: the prune above deletes channels (which
        // remaps the CSVs) and the restore reorders them (which does not), so
        // anything materialised earlier would be wrong again by here. Must also
        // precede importInputConfig's enforceAllSharedClusterInvariants(), which
        // reads the order to pick each shared cluster's reference member.
        flushPendingClusterOrders();

        return true;
    }
    return false;
}

bool WFSFileManager::applyOutputsSection (const juce::ValueTree& outputsTree)
{
    auto existingOutputs = valueTreeState.getOutputsState();
    if (existingOutputs.isValid())
    {
        mergeTreeRecursive (existingOutputs, outputsTree, valueTreeState.getUndoManager());

        // Sync outputChannels count with actual number of output children
        int actualCount = existingOutputs.getNumChildren();
        valueTreeState.setNumOutputChannels (actualCount);

        return true;
    }
    return false;
}

bool WFSFileManager::applyReverbsSection (const juce::ValueTree& reverbsTree)
{
    auto existingReverbs = valueTreeState.getReverbsState();
    if (existingReverbs.isValid())
    {
        mergeTreeRecursive (existingReverbs, reverbsTree, valueTreeState.getUndoManager());
        return true;
    }
    return false;
}

bool WFSFileManager::applyAudioPatchSection (const juce::ValueTree& audioPatchTree)
{
    auto existingPatch = valueTreeState.getAudioPatchState();
    if (existingPatch.isValid())
    {
        mergeTreeRecursive (existingPatch, audioPatchTree, valueTreeState.getUndoManager());
        return true;
    }
    return false;
}

bool WFSFileManager::applyNetworkSection (const juce::ValueTree& networkContainer)
{
    auto config = valueTreeState.getConfigState();
    if (!config.isValid())
    {
        setError (LOC ("fileManager.errors.configStateInvalid"));
        return false;
    }

    auto* undoManager = valueTreeState.getUndoManager();
    bool success = false;
    juce::StringArray failedSections;

    // Apply Network section
    auto loadedNetwork = networkContainer.getChildWithName (Network);
    if (loadedNetwork.isValid())
    {
        auto existingNetwork = config.getChildWithName (Network);
        if (existingNetwork.isValid())
        {
            mergeTreeRecursive (existingNetwork, loadedNetwork, undoManager);
            success = true;
        }
        else
        {
            failedSections.add ("Network (no existing section)");
        }
    }

    // Apply ADMOSC section
    auto loadedAdmOsc = networkContainer.getChildWithName (ADMOSC);
    if (loadedAdmOsc.isValid())
    {
        auto existingAdmOsc = config.getChildWithName (ADMOSC);
        if (existingAdmOsc.isValid())
        {
            mergeTreeRecursive (existingAdmOsc, loadedAdmOsc, undoManager);
            success = true;
        }
        else
        {
            failedSections.add ("ADMOSC (no existing section)");
        }
    }

    // Apply Tracking section
    auto loadedTracking = networkContainer.getChildWithName (Tracking);
    if (loadedTracking.isValid())
    {
        auto existingTracking = config.getChildWithName (Tracking);
        if (existingTracking.isValid())
        {
            mergeTreeRecursive (existingTracking, loadedTracking, undoManager);
            success = true;
        }
        else
        {
            failedSections.add ("Tracking (no existing section)");
        }
    }

    if (!success && failedSections.size() > 0)
        setError (LOC ("fileManager.errors.failedApply").replace ("{sections}", failedSections.joinIntoString (", ")));
    else if (!success)
        setError (LOC ("fileManager.errors.noNetworkSections"));

    return success;
}

//==============================================================================
// Merge Helpers (preserves missing properties)
//==============================================================================

void WFSFileManager::mergeTreeRecursive (juce::ValueTree& target, const juce::ValueTree& source,
                                          juce::UndoManager* undoManager)
{
    // Core merge/backfill engine: "missing = keep" property merge, children
    // matched by type+id (channels, ADM mappings) or by type+ordinal among
    // id-less siblings (e.g. the 16 ClusterLFOPreset nodes). Every property
    // passes through the injected WFS bounds validator (validateFileLoadProperty).
    persistence.mergeTreeRecursive (target, source, undoManager);
}

void WFSFileManager::setError (const juce::String& error)
{
    lastError = error;
    DBG ("WFSFileManager Error: " + error);
}
