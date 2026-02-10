#include "WFSFileManager.h"
#include "WFSParameterIDs.h"

using namespace WFSParameterIDs;

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
        setError ("No project folder specified");
        return false;
    }

    // Create main folder
    if (!projectFolder.createDirectory())
    {
        setError ("Failed to create project folder: " + projectFolder.getFullPathName());
        return false;
    }

    // Create subfolders
    getBackupFolder().createDirectory();
    getInputSnapshotsFolder().createDirectory();
    getOutputSnapshotsFolder().createDirectory();

    return true;
}

void WFSFileManager::chooseProjectFolder (std::function<void (bool)> callback)
{
    auto chooser = std::make_shared<juce::FileChooser> (
        "Select Project Folder",
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
// File Paths
//==============================================================================

juce::File WFSFileManager::getCompleteConfigFile() const
{
    return projectFolder.getChildFile ("show" + juce::String (completeConfigExtension));
}

juce::File WFSFileManager::getSystemConfigFile() const
{
    return projectFolder.getChildFile ("system" + juce::String (systemConfigExtension));
}

juce::File WFSFileManager::getInputConfigFile() const
{
    return projectFolder.getChildFile ("inputs" + juce::String (inputConfigExtension));
}

juce::File WFSFileManager::getOutputConfigFile() const
{
    return projectFolder.getChildFile ("outputs" + juce::String (outputConfigExtension));
}

juce::File WFSFileManager::getReverbConfigFile() const
{
    return projectFolder.getChildFile ("reverbs" + juce::String (reverbConfigExtension));
}

juce::File WFSFileManager::getAudioPatchFile() const
{
    return projectFolder.getChildFile ("audio_patch" + juce::String (audioPatchExtension));
}

juce::File WFSFileManager::getNetworkConfigFile() const
{
    return projectFolder.getChildFile ("network" + juce::String (networkConfigExtension));
}

juce::File WFSFileManager::getBackupFolder() const
{
    return projectFolder.getChildFile ("backups");
}

juce::File WFSFileManager::getInputSnapshotsFolder() const
{
    return projectFolder.getChildFile ("snapshots").getChildFile ("inputs");
}

juce::File WFSFileManager::getOutputSnapshotsFolder() const
{
    return projectFolder.getChildFile ("snapshots").getChildFile ("outputs");
}

//==============================================================================
// Complete Configuration
//==============================================================================

bool WFSFileManager::saveCompleteConfig()
{
    if (!hasValidProjectFolder())
    {
        setError ("No valid project folder");
        return false;
    }

    // Save all individual configuration files
    bool success = true;
    juce::StringArray errors;

    if (!saveSystemConfig())
    {
        success = false;
        errors.add ("System: " + lastError);
    }

    if (!saveNetworkConfig())
    {
        success = false;
        errors.add ("Network: " + lastError);
    }

    if (!saveInputConfig())
    {
        success = false;
        errors.add ("Inputs: " + lastError);
    }

    if (!saveOutputConfig())
    {
        success = false;
        errors.add ("Outputs: " + lastError);
    }

    if (!saveReverbConfig())
    {
        success = false;
        errors.add ("Reverbs: " + lastError);
    }

    if (!success)
        setError (errors.joinIntoString ("; "));

    return success;
}

bool WFSFileManager::loadCompleteConfig()
{
    DBG ("WFSFileManager::loadCompleteConfig() - starting");
    DBG ("  Project folder: " << projectFolder.getFullPathName());

    if (!hasValidProjectFolder())
    {
        setError ("No valid project folder");
        DBG ("  ERROR: No valid project folder");
        return false;
    }

    // Clear any previous errors
    lastError = juce::String();

    // Load all individual configuration files
    bool success = true;
    juce::StringArray errors;

    // Note: No undo transaction needed for config reload - changes are intentional and don't need undo

    DBG ("  Loading system config from: " << getSystemConfigFile().getFullPathName());
    if (!loadSystemConfig())
    {
        success = false;
        errors.add ("System: " + lastError);
        DBG ("  FAILED: System - " << lastError);
    }
    else
        DBG ("  OK: System config loaded");

    DBG ("  Loading network config from: " << getNetworkConfigFile().getFullPathName());
    if (!loadNetworkConfig())
    {
        success = false;
        errors.add ("Network: " + lastError);
        DBG ("  FAILED: Network - " << lastError);
    }
    else
        DBG ("  OK: Network config loaded");

    DBG ("  Loading input config from: " << getInputConfigFile().getFullPathName());
    if (!loadInputConfig())
    {
        success = false;
        errors.add ("Inputs: " + lastError);
        DBG ("  FAILED: Inputs - " << lastError);
    }
    else
        DBG ("  OK: Input config loaded");

    DBG ("  Loading output config from: " << getOutputConfigFile().getFullPathName());
    if (!loadOutputConfig())
    {
        success = false;
        errors.add ("Outputs: " + lastError);
        DBG ("  FAILED: Outputs - " << lastError);
    }
    else
        DBG ("  OK: Output config loaded");

    DBG ("  Loading reverb config from: " << getReverbConfigFile().getFullPathName());
    if (!loadReverbConfig())
    {
        success = false;
        errors.add ("Reverbs: " + lastError);
        DBG ("  FAILED: Reverbs - " << lastError);
    }
    else
        DBG ("  OK: Reverb config loaded");

    if (!success)
        setError (errors.joinIntoString ("; "));

    DBG ("WFSFileManager::loadCompleteConfig() - " << (success ? "SUCCESS" : "FAILED"));
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
        errors.add ("System: " + lastError);
    }

    if (!loadNetworkConfigBackup (backupIndex))
    {
        success = false;
        errors.add ("Network: " + lastError);
    }

    if (!loadInputConfigBackup (backupIndex))
    {
        success = false;
        errors.add ("Inputs: " + lastError);
    }

    if (!loadOutputConfigBackup (backupIndex))
    {
        success = false;
        errors.add ("Outputs: " + lastError);
    }

    if (!loadReverbConfigBackup (backupIndex))
    {
        success = false;
        errors.add ("Reverbs: " + lastError);
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
        setError ("Invalid configuration file structure");
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
        setError ("No valid project folder");
        return false;
    }

    auto file = getSystemConfigFile();

    if (file.existsAsFile())
        createBackup (file);

    // Create a tree with config and audio patch
    juce::ValueTree systemState ("SystemConfig");
    systemState.setProperty (WFSParameterIDs::version, "1.0", nullptr);
    systemState.appendChild (extractConfigSection().createCopy(), nullptr);
    systemState.appendChild (extractAudioPatchSection().createCopy(), nullptr);

    return writeToXmlFile (systemState, file);
}

bool WFSFileManager::loadSystemConfig()
{
    if (!hasValidProjectFolder())
    {
        setError ("No valid project folder");
        return false;
    }

    return importSystemConfig (getSystemConfigFile());
}

bool WFSFileManager::loadSystemConfigBackup (int backupIndex)
{
    auto backups = getBackups ("system");
    if (backupIndex >= 0 && backupIndex < backups.size())
        return importSystemConfig (backups[backupIndex]);

    setError ("Backup not found");
    return false;
}

bool WFSFileManager::exportSystemConfig (const juce::File& file)
{
    juce::ValueTree systemState ("SystemConfig");
    systemState.setProperty (WFSParameterIDs::version, "1.0", nullptr);
    systemState.appendChild (extractConfigSection().createCopy(), nullptr);
    systemState.appendChild (extractAudioPatchSection().createCopy(), nullptr);

    return writeToXmlFile (systemState, file);
}

bool WFSFileManager::importSystemConfig (const juce::File& file)
{
    auto loadedState = readFromXmlFile (file);
    if (!loadedState.isValid())
        return false;

    // Note: Transaction management should be done by caller (e.g., loadCompleteConfig)
    // to avoid nested transactions. Individual callers should begin their own transaction.

    bool appliedSomething = false;

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
        setError ("No valid system data found in file: " + file.getFullPathName());

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
        setError ("No valid project folder");
        return false;
    }

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
        setError ("No valid project folder");
        return false;
    }

    return importNetworkConfig (getNetworkConfigFile());
}

bool WFSFileManager::loadNetworkConfigBackup (int backupIndex)
{
    auto backups = getBackups ("network");
    if (backupIndex >= 0 && backupIndex < backups.size())
        return importNetworkConfig (backups[backupIndex]);

    setError ("Backup not found");
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

    setError ("No network data found in file");
    return false;
}

//==============================================================================
// Input Configuration
//==============================================================================

bool WFSFileManager::saveInputConfig()
{
    if (!hasValidProjectFolder())
    {
        setError ("No valid project folder");
        return false;
    }

    auto file = getInputConfigFile();

    if (file.existsAsFile())
        createBackup (file);

    juce::ValueTree inputState ("InputConfig");
    inputState.setProperty (WFSParameterIDs::version, "1.0", nullptr);
    inputState.appendChild (extractInputsSection().createCopy(), nullptr);

    return writeToXmlFile (inputState, file);
}

bool WFSFileManager::loadInputConfig()
{
    if (!hasValidProjectFolder())
    {
        setError ("No valid project folder");
        return false;
    }

    return importInputConfig (getInputConfigFile());
}

bool WFSFileManager::loadInputConfigBackup (int backupIndex)
{
    auto backups = getBackups ("inputs");
    if (backupIndex >= 0 && backupIndex < backups.size())
        return importInputConfig (backups[backupIndex]);

    setError ("Backup not found");
    return false;
}

bool WFSFileManager::exportInputConfig (const juce::File& file)
{
    juce::ValueTree inputState ("InputConfig");
    inputState.setProperty (WFSParameterIDs::version, "1.0", nullptr);
    inputState.appendChild (extractInputsSection().createCopy(), nullptr);

    return writeToXmlFile (inputState, file);
}

bool WFSFileManager::importInputConfig (const juce::File& file)
{
    auto loadedState = readFromXmlFile (file);
    if (!loadedState.isValid())
        return false;

    auto inputsTree = loadedState.getChildWithName (Inputs);
    if (!inputsTree.isValid())
    {
        setError ("No input data found in file");
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
        setError ("No valid project folder");
        return false;
    }

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
        setError ("No valid project folder");
        return false;
    }

    return importOutputConfig (getOutputConfigFile());
}

bool WFSFileManager::loadOutputConfigBackup (int backupIndex)
{
    auto backups = getBackups ("outputs");
    if (backupIndex >= 0 && backupIndex < backups.size())
        return importOutputConfig (backups[backupIndex]);

    setError ("Backup not found");
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
    auto loadedState = readFromXmlFile (file);
    if (!loadedState.isValid())
        return false;

    auto outputsTree = loadedState.getChildWithName (Outputs);
    if (!outputsTree.isValid())
    {
        setError ("No output data found in file");
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
        setError ("No valid project folder");
        return false;
    }

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
        setError ("No valid project folder");
        return false;
    }

    return importReverbConfig (getReverbConfigFile());
}

bool WFSFileManager::loadReverbConfigBackup (int backupIndex)
{
    auto backups = getBackups ("reverbs");
    if (backupIndex >= 0 && backupIndex < backups.size())
        return importReverbConfig (backups[backupIndex]);

    setError ("Backup not found");
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
    auto loadedState = readFromXmlFile (file);
    if (!loadedState.isValid())
        return false;

    auto reverbsTree = loadedState.getChildWithName (Reverbs);
    if (!reverbsTree.isValid())
    {
        setError ("No reverb data found in file");
        return false;
    }

    bool result = applyReverbsSection (reverbsTree);
    if (result)
        valueTreeState.clearAllUndoHistories();
    return result;
}

//==============================================================================
// Snapshots
//==============================================================================

bool WFSFileManager::saveInputSnapshot (const juce::String& snapshotName, const SnapshotScope& scope)
{
    auto folder = getInputSnapshotsFolder();
    folder.createDirectory();

    auto file = folder.getChildFile (snapshotName + snapshotExtension);

    juce::ValueTree snapshot ("InputSnapshot");
    snapshot.setProperty (WFSParameterIDs::version, "1.0", nullptr);
    snapshot.setProperty (name, snapshotName, nullptr);

    // Store scope settings
    juce::ValueTree scopeTree ("Scope");
    scopeTree.setProperty ("includePosition", scope.includePosition, nullptr);
    scopeTree.setProperty ("includeAttenuation", scope.includeAttenuation, nullptr);
    scopeTree.setProperty ("includeDirectivity", scope.includeDirectivity, nullptr);
    scopeTree.setProperty ("includeLiveSource", scope.includeLiveSource, nullptr);
    scopeTree.setProperty ("includeHackoustics", scope.includeHackoustics, nullptr);
    scopeTree.setProperty ("includeLFO", scope.includeLFO, nullptr);
    scopeTree.setProperty ("includeAutomOtion", scope.includeAutomOtion, nullptr);
    scopeTree.setProperty ("includeMutes", scope.includeMutes, nullptr);

    if (!scope.channelIndices.isEmpty())
    {
        juce::StringArray indices;
        for (auto idx : scope.channelIndices)
            indices.add (juce::String (idx));
        scopeTree.setProperty ("channels", indices.joinIntoString (","), nullptr);
    }
    snapshot.appendChild (scopeTree, nullptr);

    // Store input data
    juce::ValueTree inputsData (Inputs);
    int numInputs = valueTreeState.getNumInputChannels();

    for (int i = 0; i < numInputs; ++i)
    {
        if (scope.channelIndices.isEmpty() || scope.channelIndices.contains (i))
            inputsData.appendChild (extractInputWithScope (i, scope), nullptr);
    }
    snapshot.appendChild (inputsData, nullptr);

    return writeToXmlFile (snapshot, file);
}

bool WFSFileManager::loadInputSnapshot (const juce::String& snapshotName, const SnapshotScope& scope)
{
    auto file = getInputSnapshotsFolder().getChildFile (snapshotName + snapshotExtension);
    auto snapshot = readFromXmlFile (file);

    if (!snapshot.isValid())
        return false;

    auto inputsData = snapshot.getChildWithName (Inputs);
    if (!inputsData.isValid())
    {
        setError ("No input data in snapshot");
        return false;
    }

    valueTreeState.beginUndoTransaction ("Load Input Snapshot: " + snapshotName);

    for (int i = 0; i < inputsData.getNumChildren(); ++i)
    {
        auto inputData = inputsData.getChild (i);
        int channelIndex = static_cast<int> (inputData.getProperty (id)) - 1;

        if (scope.channelIndices.isEmpty() || scope.channelIndices.contains (channelIndex))
            applyInputWithScope (channelIndex, inputData, scope);
    }

    return true;
}

bool WFSFileManager::updateInputSnapshot (const juce::String& snapshotName, const SnapshotScope& scope)
{
    auto file = getInputSnapshotsFolder().getChildFile (snapshotName + snapshotExtension);
    if (!file.existsAsFile())
    {
        setError ("Snapshot does not exist");
        return false;
    }

    createBackup (file);
    return saveInputSnapshot (snapshotName, scope);
}

bool WFSFileManager::deleteInputSnapshot (const juce::String& snapshotName)
{
    auto file = getInputSnapshotsFolder().getChildFile (snapshotName + snapshotExtension);
    if (file.existsAsFile())
        return file.deleteFile();

    setError ("Snapshot not found");
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

WFSFileManager::SnapshotScope WFSFileManager::getSnapshotScope (const juce::String& snapshotName) const
{
    SnapshotScope scope;
    auto file = getInputSnapshotsFolder().getChildFile (snapshotName + snapshotExtension);
    auto snapshot = const_cast<WFSFileManager*>(this)->readFromXmlFile (file);

    if (snapshot.isValid())
    {
        auto scopeTree = snapshot.getChildWithName ("Scope");
        if (scopeTree.isValid())
        {
            scope.includePosition = scopeTree.getProperty ("includePosition", true);
            scope.includeAttenuation = scopeTree.getProperty ("includeAttenuation", true);
            scope.includeDirectivity = scopeTree.getProperty ("includeDirectivity", true);
            scope.includeLiveSource = scopeTree.getProperty ("includeLiveSource", true);
            scope.includeHackoustics = scopeTree.getProperty ("includeHackoustics", true);
            scope.includeLFO = scopeTree.getProperty ("includeLFO", true);
            scope.includeAutomOtion = scopeTree.getProperty ("includeAutomOtion", true);
            scope.includeMutes = scopeTree.getProperty ("includeMutes", true);

            juce::String channels = scopeTree.getProperty ("channels").toString();
            if (channels.isNotEmpty())
            {
                juce::StringArray indices;
                indices.addTokens (channels, ",", "");
                for (auto& idx : indices)
                    scope.channelIndices.add (idx.getIntValue());
            }
        }
    }

    return scope;
}

bool WFSFileManager::setSnapshotScope (const juce::String& snapshotName, const SnapshotScope& scope)
{
    // Reload current data and save with new scope
    auto currentScope = getSnapshotScope (snapshotName);
    return updateInputSnapshot (snapshotName, scope);
}

//==============================================================================
// Extended Snapshot Scope - Static Definitions
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
        { "lsPeakComp", "Peak Comp", LiveSourceTamer, { inputLSpeakThreshold, inputLSpeakRatio } },
        { "lsSlowComp", "Slow Comp", LiveSourceTamer, { inputLSslowThreshold, inputLSslowRatio } },

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
        { "arrayAttens", "Array Attens", Mutes, { inputArrayAtten1, inputArrayAtten2, inputArrayAtten3, inputArrayAtten4, inputArrayAtten5, inputArrayAtten6, inputArrayAtten7, inputArrayAtten8, inputArrayAtten9, inputArrayAtten10 } }
    };
    return items;
}

const std::vector<juce::Identifier>& WFSFileManager::ExtendedSnapshotScope::getSectionIds()
{
    static std::vector<juce::Identifier> sections = {
        Channel, Position, Attenuation, Directivity, LiveSourceTamer,
        Hackoustics, LFO, AutomOtion, Mutes
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
    (void) numChannels;  // Unused - defaults are "all included" which is empty map
    itemChannelStates.clear();
    applyMode = ApplyMode::OnRecall;
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

    return writeToXmlFile (snapshot, file);
}

bool WFSFileManager::loadInputSnapshotWithExtendedScope (const juce::String& snapshotName, const ExtendedSnapshotScope& scope)
{
    auto file = getInputSnapshotsFolder().getChildFile (snapshotName + snapshotExtension);
    auto snapshot = readFromXmlFile (file);

    if (!snapshot.isValid())
        return false;

    auto inputsData = snapshot.getChildWithName (Inputs);
    if (!inputsData.isValid())
    {
        setError ("No input data in snapshot");
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
        setError ("Snapshot not found: " + snapshotName);
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

juce::ValueTree WFSFileManager::extractInputWithExtendedScope (int channelIndex, const ExtendedSnapshotScope& scope) const
{
    auto input = const_cast<WFSValueTreeState&>(valueTreeState).getInputState (channelIndex);
    if (!input.isValid())
        return {};

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

    return filtered;
}

bool WFSFileManager::applyInputWithExtendedScope (int channelIndex, const juce::ValueTree& inputData, const ExtendedSnapshotScope& scope)
{
    auto input = valueTreeState.getInputState (channelIndex);
    if (!input.isValid())
        return false;

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
        setError ("Failed to create XML from state");
        return false;
    }

    // Create human-readable XML with our custom header (without JUCE's default declaration)
    juce::String header = createXmlHeader (file.getFileNameWithoutExtension());
    auto format = juce::XmlElement::TextFormat().withoutHeader();
    juce::String xmlString = header + xml->toString (format);

    if (!file.replaceWithText (xmlString))
    {
        setError ("Failed to write file: " + file.getFullPathName());
        return false;
    }

    return true;
}

juce::ValueTree WFSFileManager::readFromXmlFile (const juce::File& file)
{
    if (!file.existsAsFile())
    {
        setError ("File not found: " + file.getFullPathName());
        return {};
    }

    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr)
    {
        setError ("Failed to parse XML file: " + file.getFullPathName());
        return {};
    }

    auto tree = juce::ValueTree::fromXml (*xml);
    if (!tree.isValid())
    {
        setError ("Failed to create ValueTree from XML: " + file.getFullPathName());
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
        setError ("Config state is invalid");
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
        setError ("Failed to apply: " + failedSections.joinIntoString (", "));
    else if (!success)
        setError ("No network sections found in file");

    return success;
}

juce::ValueTree WFSFileManager::extractInputWithScope (int channelIndex, const SnapshotScope& scope) const
{
    auto input = const_cast<WFSValueTreeState&>(valueTreeState).getInputState (channelIndex);
    if (!input.isValid())
        return {};

    juce::ValueTree filtered (Input);
    filtered.setProperty (id, channelIndex + 1, nullptr);

    // Always include channel section (name, etc.)
    auto channel = input.getChildWithName (Channel);
    if (channel.isValid())
        filtered.appendChild (channel.createCopy(), nullptr);

    // Include sections based on scope
    if (scope.includePosition)
    {
        auto pos = input.getChildWithName (Position);
        if (pos.isValid())
            filtered.appendChild (pos.createCopy(), nullptr);
    }

    if (scope.includeAttenuation)
    {
        auto atten = input.getChildWithName (Attenuation);
        if (atten.isValid())
            filtered.appendChild (atten.createCopy(), nullptr);
    }

    if (scope.includeDirectivity)
    {
        auto dir = input.getChildWithName (Directivity);
        if (dir.isValid())
            filtered.appendChild (dir.createCopy(), nullptr);
    }

    if (scope.includeLiveSource)
    {
        auto ls = input.getChildWithName (LiveSourceTamer);
        if (ls.isValid())
            filtered.appendChild (ls.createCopy(), nullptr);
    }

    if (scope.includeHackoustics)
    {
        auto hack = input.getChildWithName (Hackoustics);
        if (hack.isValid())
            filtered.appendChild (hack.createCopy(), nullptr);
    }

    if (scope.includeLFO)
    {
        auto lfo = input.getChildWithName (LFO);
        if (lfo.isValid())
            filtered.appendChild (lfo.createCopy(), nullptr);
    }

    if (scope.includeAutomOtion)
    {
        auto autom = input.getChildWithName (AutomOtion);
        if (autom.isValid())
            filtered.appendChild (autom.createCopy(), nullptr);
    }

    if (scope.includeMutes)
    {
        auto mutes = input.getChildWithName (Mutes);
        if (mutes.isValid())
            filtered.appendChild (mutes.createCopy(), nullptr);
    }

    return filtered;
}

bool WFSFileManager::applyInputWithScope (int channelIndex, const juce::ValueTree& inputData, const SnapshotScope& scope)
{
    auto input = valueTreeState.getInputState (channelIndex);
    if (!input.isValid())
        return false;

    auto* undoManager = valueTreeState.getUndoManager();

    // Always apply channel section
    auto loadedChannel = inputData.getChildWithName (Channel);
    if (loadedChannel.isValid())
    {
        auto existingChannel = input.getChildWithName (Channel);
        if (existingChannel.isValid())
            mergeTreeRecursive (existingChannel, loadedChannel, undoManager);
    }

    // Apply sections based on scope
    if (scope.includePosition)
    {
        auto loadedPos = inputData.getChildWithName (Position);
        if (loadedPos.isValid())
        {
            auto existingPos = input.getChildWithName (Position);
            if (existingPos.isValid())
                mergeTreeRecursive (existingPos, loadedPos, undoManager);
        }
    }

    if (scope.includeAttenuation)
    {
        auto loaded = inputData.getChildWithName (Attenuation);
        if (loaded.isValid())
        {
            auto existing = input.getChildWithName (Attenuation);
            if (existing.isValid())
                mergeTreeRecursive (existing, loaded, undoManager);
        }
    }

    if (scope.includeDirectivity)
    {
        auto loaded = inputData.getChildWithName (Directivity);
        if (loaded.isValid())
        {
            auto existing = input.getChildWithName (Directivity);
            if (existing.isValid())
                mergeTreeRecursive (existing, loaded, undoManager);
        }
    }

    if (scope.includeLiveSource)
    {
        auto loaded = inputData.getChildWithName (LiveSourceTamer);
        if (loaded.isValid())
        {
            auto existing = input.getChildWithName (LiveSourceTamer);
            if (existing.isValid())
                mergeTreeRecursive (existing, loaded, undoManager);
        }
    }

    if (scope.includeHackoustics)
    {
        auto loaded = inputData.getChildWithName (Hackoustics);
        if (loaded.isValid())
        {
            auto existing = input.getChildWithName (Hackoustics);
            if (existing.isValid())
                mergeTreeRecursive (existing, loaded, undoManager);
        }
    }

    if (scope.includeLFO)
    {
        auto loaded = inputData.getChildWithName (LFO);
        if (loaded.isValid())
        {
            auto existing = input.getChildWithName (LFO);
            if (existing.isValid())
                mergeTreeRecursive (existing, loaded, undoManager);
        }
    }

    if (scope.includeAutomOtion)
    {
        auto loaded = inputData.getChildWithName (AutomOtion);
        if (loaded.isValid())
        {
            auto existing = input.getChildWithName (AutomOtion);
            if (existing.isValid())
                mergeTreeRecursive (existing, loaded, undoManager);
        }
    }

    if (scope.includeMutes)
    {
        auto loaded = inputData.getChildWithName (Mutes);
        if (loaded.isValid())
        {
            auto existing = input.getChildWithName (Mutes);
            if (existing.isValid())
                mergeTreeRecursive (existing, loaded, undoManager);
        }
    }

    return true;
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
