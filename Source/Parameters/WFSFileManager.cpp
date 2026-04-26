#include "WFSFileManager.h"
#include "WFSParameterIDs.h"
#include "../AppSettings.h"
#include "../Localization/LocalizationManager.h"
#include "../Network/OSCProtocolTypes.h"
#include "../WFSLogger.h"

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
// Construction
//==============================================================================

WFSFileManager::WFSFileManager (WFSValueTreeState& state)
    : valueTreeState (state)
{
}

//==============================================================================
// Project Folder Management
//==============================================================================

void WFSFileManager::setProjectFolder (const juce::File& folder)
{
    projectFolder = folder;
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
    getIRFolder().createDirectory();
    getSamplesFolder().createDirectory();

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

    return success;
}

bool WFSFileManager::loadCompleteConfigBackup (int backupIndex)
{
    // Clear any previous errors
    lastError = juce::String();

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

    // Note: No undo transaction needed for config import - changes are intentional and don't need undo
    valueTreeState.replaceState (loadedState);
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
    auto file = getSystemConfigFile();

    if (file.existsAsFile())
        createBackup (file);

    // Create a tree with config and audio patch
    juce::ValueTree systemState ("SystemConfig");
    systemState.setProperty (WFSParameterIDs::version, "1.0", nullptr);
    systemState.appendChild (extractConfigSection().createCopy(), nullptr);
    systemState.appendChild (extractAudioPatchSection().createCopy(), nullptr);
    stripTransientToggles (systemState);

    return writeToXmlFile (systemState, file);
}

bool WFSFileManager::loadSystemConfig()
{
    if (!hasValidProjectFolder())
    {
        setError (LOC ("fileManager.errors.noValidProjectFolder"));
        return false;
    }

    WFSLogger::getInstance().logInfo ("Loading system config");
    return importSystemConfig (getSystemConfigFile());
}

bool WFSFileManager::loadSystemConfigBackup (int backupIndex)
{
    auto backups = getBackups ("system");
    if (backupIndex >= 0 && backupIndex < backups.size())
        return importSystemConfig (backups[backupIndex]);

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
        valueTreeState.clearAllUndoHistories();

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
    auto file = getInputConfigFile();

    if (file.existsAsFile())
        createBackup (file);

    juce::ValueTree inputState ("InputConfig");
    inputState.setProperty (WFSParameterIDs::version, "1.0", nullptr);
    inputState.appendChild (extractInputsSection().createCopy(), nullptr);
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

    bool result = applyInputsSection (inputsTree);
    if (result)
        valueTreeState.clearAllUndoHistories();
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

//==============================================================================
// Snapshot Scope - Static Definitions
//==============================================================================

const std::vector<WFSFileManager::ScopeItem>& WFSFileManager::ExtendedSnapshotScope::getScopeItems()
{
    static std::vector<ScopeItem> items = {
        // Input Section
        { "inputAttenuation", "Attenuation", Channel, { inputAttenuation } },
        { "inputDelay", "Delay/Latency", Channel, { inputDelayLatency, inputMinimalLatency } },

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
        { "otomoDestination", "Destination", AutomOtion, { inputOtomoX, inputOtomoY, inputOtomoZ, inputOtomoAbsoluteRelative } },
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
        { "sampler", "Sampler", Sampler, { inputSamplerActive, inputSamplerActiveSet } },

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
    auto folder = getInputSnapshotsFolder();
    folder.createDirectory();

    auto file = folder.getChildFile (snapshotName + snapshotExtension);

    juce::ValueTree snapshot ("InputSnapshot");
    snapshot.setProperty (version, "2.0", nullptr);  // Version 2.0 for extended scope
    snapshot.setProperty (name, snapshotName, nullptr);

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
    snapshot.appendChild (inputsData, nullptr);
    stripTransientToggles (snapshot);

    return writeToXmlFile (snapshot, file);
}

bool WFSFileManager::loadInputSnapshotWithExtendedScope (const juce::String& snapshotName, const ExtendedSnapshotScope& scope)
{
    OriginTagScope originScope { OriginTag::Snapshot };

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

    for (int i = 0; i < inputsData.getNumChildren(); ++i)
    {
        auto inputData = inputsData.getChild (i);
        int channelIndex = static_cast<int> (inputData.getProperty (id)) - 1;

        if (channelIndex >= 0)
        {
            if (scope.applyMode == ExtendedSnapshotScope::ApplyMode::OnRecall)
                applyInputWithExtendedScope (channelIndex, inputData, scope);
            else
                applyInputWithExtendedScope (channelIndex, inputData, ExtendedSnapshotScope());  // All included
        }
    }

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

    return writeToXmlFile (snapshot, file);
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

    // Serialize full channels
    if (!fullChannels.empty())
    {
        juce::StringArray indices;
        for (int ch : fullChannels)
            indices.add (juce::String (ch + 1));  // 1-based for user display
        scopeTree.setProperty ("fullChannels", indices.joinIntoString (","), nullptr);
    }

    // Serialize excluded channels
    if (!excludedChannels.empty())
    {
        juce::StringArray indices;
        for (int ch : excludedChannels)
            indices.add (juce::String (ch + 1));
        scopeTree.setProperty ("excludedChannels", indices.joinIntoString (","), nullptr);
    }

    // Serialize partial channels
    for (int ch : partialChannels)
    {
        juce::ValueTree partialTree ("PartialChannel");
        partialTree.setProperty ("index", ch + 1, nullptr);

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

    // Parse excluded channels
    auto excludedStr = scopeTree.getProperty ("excludedChannels").toString();
    if (excludedStr.isNotEmpty())
    {
        juce::StringArray indices;
        indices.addTokens (excludedStr, ",", "");
        for (const auto& idx : indices)
        {
            int ch = idx.getIntValue() - 1;  // Convert from 1-based
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
            int ch = static_cast<int> (partialTree.getProperty ("index")) - 1;
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
    filtered.setProperty (id, channelIndex + 1, nullptr);

    // Always include input name
    auto channelTree = input.getChildWithName (Channel);
    if (channelTree.isValid())
    {
        juce::ValueTree filteredChannel (Channel);
        filteredChannel.setProperty (inputName, channelTree.getProperty (inputName), nullptr);

        // Add other Channel properties based on scope
        if (scope.isIncluded ("inputAttenuation", channelIndex))
            filteredChannel.setProperty (inputAttenuation, channelTree.getProperty (inputAttenuation), nullptr);
        if (scope.isIncluded ("inputDelay", channelIndex))
        {
            filteredChannel.setProperty (inputDelayLatency, channelTree.getProperty (inputDelayLatency), nullptr);
            filteredChannel.setProperty (inputMinimalLatency, channelTree.getProperty (inputMinimalLatency), nullptr);
        }
        if (scope.isIncluded ("sampler", channelIndex) && channelTree.hasProperty (inputSamplerActive))
            filteredChannel.setProperty (inputSamplerActive, channelTree.getProperty (inputSamplerActive), nullptr);

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
            if (item.sectionId == sectionId && scope.isIncluded (item.itemId, channelIndex))
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
                        gmFiltered.appendChild (layerChild.createCopy(), nullptr);
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
            if (item.sectionId == sectionId && scope.isIncluded (item.itemId, channelIndex))
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

            if (scope.isIncluded ("inputAttenuation", channelIndex) && loadedChannel.hasProperty (inputAttenuation))
                existingChannel.setProperty (inputAttenuation, loadedChannel.getProperty (inputAttenuation), undoManager);

            if (scope.isIncluded ("inputDelay", channelIndex))
            {
                if (loadedChannel.hasProperty (inputDelayLatency))
                    existingChannel.setProperty (inputDelayLatency, loadedChannel.getProperty (inputDelayLatency), undoManager);
                if (loadedChannel.hasProperty (inputMinimalLatency))
                    existingChannel.setProperty (inputMinimalLatency, loadedChannel.getProperty (inputMinimalLatency), undoManager);
            }
            if (scope.isIncluded ("sampler", channelIndex) && loadedChannel.hasProperty (inputSamplerActive))
                existingChannel.setProperty (inputSamplerActive, loadedChannel.getProperty (inputSamplerActive), undoManager);
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

            for (int li = 0; li < gmSource.getNumChildren() && li < 3; ++li)
            {
                if (scope.isIncluded (layerItemIds[li], channelIndex))
                {
                    auto sourceLayer = gmSource.getChild (li);
                    if (sourceLayer.isValid() && li < gmTarget.getNumChildren())
                    {
                        // Replace entire layer subtree (properties + shape children)
                        auto targetLayer = gmTarget.getChild (li);
                        targetLayer.copyPropertiesAndChildrenFrom (sourceLayer, undoManager);
                    }
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
    if (!file.existsAsFile())
        return true;

    auto backupFolder = getBackupFolder();
    backupFolder.createDirectory();

    auto timestamp = getBackupTimestamp();
    auto backupFile = backupFolder.getChildFile (
        file.getFileNameWithoutExtension() + "_" + timestamp + file.getFileExtension());

    return file.copyFileTo (backupFile);
}

juce::Array<juce::File> WFSFileManager::getBackups (const juce::String& fileType) const
{
    juce::Array<juce::File> backups;
    auto backupFolder = getBackupFolder();

    if (backupFolder.isDirectory())
    {
        auto files = backupFolder.findChildFiles (juce::File::findFiles, false, fileType + "_*.*");

        // Sort by modification time (newest first)
        std::sort (files.begin(), files.end(),
            [] (const juce::File& a, const juce::File& b)
            {
                return a.getLastModificationTime() > b.getLastModificationTime();
            });

        for (auto& file : files)
            backups.add (file);
    }

    return backups;
}

void WFSFileManager::cleanupBackups (int keepCount)
{
    // Clean up each file type
    for (auto& type : { "system", "network", "inputs", "outputs", "reverbs" })
    {
        auto backups = getBackups (type);
        for (int i = keepCount; i < backups.size(); ++i)
            backups[i].deleteFile();
    }
}

juce::String WFSFileManager::getBackupTimestamp()
{
    return juce::Time::getCurrentTime().formatted ("%Y%m%d_%H%M%S");
}

//==============================================================================
// Internal Methods
//==============================================================================

bool WFSFileManager::writeToXmlFile (const juce::ValueTree& tree, const juce::File& file)
{
    auto xml = tree.createXml();
    if (xml == nullptr)
    {
        setError (LOC ("fileManager.errors.failedCreateXML"));
        return false;
    }

    // Create human-readable XML with our custom header (without JUCE's default declaration)
    juce::String header = createXmlHeader (file.getFileNameWithoutExtension());
    auto format = juce::XmlElement::TextFormat().withoutHeader();
    juce::String xmlString = header + xml->toString (format);

    if (!file.replaceWithText (xmlString))
    {
        setError (LOC ("fileManager.errors.failedWriteFile").replace ("{path}", file.getFullPathName()));
        return false;
    }

    return true;
}

juce::ValueTree WFSFileManager::readFromXmlFile (const juce::File& file)
{
    if (!file.existsAsFile())
    {
        setError (LOC ("fileManager.errors.fileNotFound").replace ("{path}", file.getFullPathName()));
        return {};
    }

    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr)
    {
        setError (LOC ("fileManager.errors.failedParseXML").replace ("{path}", file.getFullPathName()));
        return {};
    }

    auto tree = juce::ValueTree::fromXml (*xml);
    if (!tree.isValid())
    {
        setError (LOC ("fileManager.errors.failedCreateValueTree").replace ("{path}", file.getFullPathName()));
        return {};
    }

    return tree;
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

bool WFSFileManager::applyConfigSection (const juce::ValueTree& configTree)
{
    auto existingConfig = valueTreeState.getConfigState();
    if (!existingConfig.isValid())
        return false;

    auto* undoManager = valueTreeState.getUndoManager();

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

        valueTreeState.setNumInputChannels (inputCount);
        valueTreeState.setNumOutputChannels (outputCount);
        valueTreeState.setNumReverbChannels (reverbCount);
    }

    return true;
}

bool WFSFileManager::applyInputsSection (const juce::ValueTree& inputsTree)
{
    auto existingInputs = valueTreeState.getInputsState();
    if (existingInputs.isValid())
    {
        mergeTreeRecursive (existingInputs, inputsTree, valueTreeState.getUndoManager());

        // Sync inputChannels count with actual number of input children.
        // The inputs file may have more entries than the system config's inputChannels property,
        // which was set earlier during loadSystemConfig.
        int actualCount = existingInputs.getNumChildren();
        valueTreeState.setNumInputChannels (actualCount);

        // Migration: ensure GradientMaps section exists for all inputs (handles old configs)
        for (int i = 0; i < actualCount; ++i)
            valueTreeState.ensureInputGradientMapsSection (i);

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

juce::String WFSFileManager::createXmlHeader (const juce::String& fileType)
{
    juce::String header;
    header << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    header << "<!-- WFS Processor Configuration File -->\n";
    header << "<!-- Type: " << fileType << " -->\n";
    header << "<!-- Created: " << juce::Time::getCurrentTime().toString (true, true) << " -->\n";
    header << "\n";
    return header;
}

//==============================================================================
// Merge Helpers (preserves missing properties)
//==============================================================================

void WFSFileManager::mergeProperties (juce::ValueTree& target, const juce::ValueTree& source,
                                       juce::UndoManager* undoManager)
{
    // Only copy properties that exist in source - missing properties keep their current value
    for (int i = 0; i < source.getNumProperties(); ++i)
    {
        auto propName = source.getPropertyName (i);
        target.setProperty (propName, source.getProperty (propName), undoManager);
    }
}

void WFSFileManager::mergeTreeRecursive (juce::ValueTree& target, const juce::ValueTree& source,
                                          juce::UndoManager* undoManager)
{
    // Merge properties (only those in source)
    mergeProperties (target, source, undoManager);

    // Merge children
    for (int i = 0; i < source.getNumChildren(); ++i)
    {
        auto sourceChild = source.getChild (i);
        juce::ValueTree targetChild;

        // For children with an "id" property (Input, Output, Reverb channels),
        // match by both type AND id to avoid mixing up channels
        if (sourceChild.hasProperty (id))
        {
            targetChild = target.getChildWithProperty (id, sourceChild.getProperty (id));
            // Verify type also matches (in case id is used elsewhere)
            if (targetChild.isValid() && targetChild.getType() != sourceChild.getType())
                targetChild = juce::ValueTree();
        }
        else
        {
            // For children without id, match by type name only
            targetChild = target.getChildWithName (sourceChild.getType());
        }

        if (targetChild.isValid())
        {
            // Child exists - recursively merge
            mergeTreeRecursive (targetChild, sourceChild, undoManager);
        }
        else
        {
            // Child doesn't exist in target - add it (new section in file)
            target.appendChild (sourceChild.createCopy(), undoManager);
        }
    }
}

void WFSFileManager::setError (const juce::String& error)
{
    lastError = error;
    DBG ("WFSFileManager Error: " + error);
}
