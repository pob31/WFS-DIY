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
        return applyNetworkSection (networkSettings);

    // Fallback: try loading old format with just Network child
    auto networkTree = loadedState.getChildWithName (Network);
    if (networkTree.isValid())
    {
        // Wrap in container for applyNetworkSection
        juce::ValueTree container ("NetworkSettings");
        container.appendChild (networkTree.createCopy(), nullptr);
        return applyNetworkSection (container);
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

    // Note: Transaction management should be done by caller (e.g., loadCompleteConfig)
    // to avoid nested transactions. Individual callers should begin their own transaction.
    return applyInputsSection (inputsTree);
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

    // Note: Transaction management should be done by caller (e.g., loadCompleteConfig)
    // to avoid nested transactions. Individual callers should begin their own transaction.
    return applyOutputsSection (outputsTree);
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

    // Note: Transaction management should be done by caller (e.g., loadCompleteConfig)
    // to avoid nested transactions. Individual callers should begin their own transaction.
    return applyReverbsSection (reverbsTree);
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

    // Preserve Network, ADMOSC, and Tracking children (they're managed separately in network.xml)
    // because copyPropertiesAndChildrenFrom would wipe them out
    auto preservedNetwork = existingConfig.getChildWithName (Network).createCopy();
    auto preservedAdmOsc = existingConfig.getChildWithName (ADMOSC).createCopy();
    auto preservedTracking = existingConfig.getChildWithName (Tracking).createCopy();

    // Copy properties and children from loaded config
    existingConfig.copyPropertiesAndChildrenFrom (configTree, undoManager);

    // Restore the network-related children if they were removed
    if (preservedNetwork.isValid() && !existingConfig.getChildWithName (Network).isValid())
        existingConfig.appendChild (preservedNetwork, undoManager);
    if (preservedAdmOsc.isValid() && !existingConfig.getChildWithName (ADMOSC).isValid())
        existingConfig.appendChild (preservedAdmOsc, undoManager);
    if (preservedTracking.isValid() && !existingConfig.getChildWithName (Tracking).isValid())
        existingConfig.appendChild (preservedTracking, undoManager);

    return true;
}

bool WFSFileManager::applyInputsSection (const juce::ValueTree& inputsTree)
{
    auto existingInputs = valueTreeState.getInputsState();
    if (existingInputs.isValid())
    {
        existingInputs.copyPropertiesAndChildrenFrom (inputsTree, valueTreeState.getUndoManager());
        return true;
    }
    return false;
}

bool WFSFileManager::applyOutputsSection (const juce::ValueTree& outputsTree)
{
    auto existingOutputs = valueTreeState.getOutputsState();
    if (existingOutputs.isValid())
    {
        existingOutputs.copyPropertiesAndChildrenFrom (outputsTree, valueTreeState.getUndoManager());
        return true;
    }
    return false;
}

bool WFSFileManager::applyReverbsSection (const juce::ValueTree& reverbsTree)
{
    auto existingReverbs = valueTreeState.getReverbsState();
    if (existingReverbs.isValid())
    {
        existingReverbs.copyPropertiesAndChildrenFrom (reverbsTree, valueTreeState.getUndoManager());
        return true;
    }
    return false;
}

bool WFSFileManager::applyAudioPatchSection (const juce::ValueTree& audioPatchTree)
{
    auto existingPatch = valueTreeState.getAudioPatchState();
    if (existingPatch.isValid())
    {
        existingPatch.copyPropertiesAndChildrenFrom (audioPatchTree, valueTreeState.getUndoManager());
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
            existingNetwork.copyPropertiesAndChildrenFrom (loadedNetwork, undoManager);
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
            existingAdmOsc.copyPropertiesAndChildrenFrom (loadedAdmOsc, undoManager);
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
            existingTracking.copyPropertiesAndChildrenFrom (loadedTracking, undoManager);
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
            existingChannel.copyPropertiesAndChildrenFrom (loadedChannel, undoManager);
    }

    // Apply sections based on scope
    if (scope.includePosition)
    {
        auto loadedPos = inputData.getChildWithName (Position);
        if (loadedPos.isValid())
        {
            auto existingPos = input.getChildWithName (Position);
            if (existingPos.isValid())
                existingPos.copyPropertiesAndChildrenFrom (loadedPos, undoManager);
        }
    }

    if (scope.includeAttenuation)
    {
        auto loaded = inputData.getChildWithName (Attenuation);
        if (loaded.isValid())
        {
            auto existing = input.getChildWithName (Attenuation);
            if (existing.isValid())
                existing.copyPropertiesAndChildrenFrom (loaded, undoManager);
        }
    }

    if (scope.includeDirectivity)
    {
        auto loaded = inputData.getChildWithName (Directivity);
        if (loaded.isValid())
        {
            auto existing = input.getChildWithName (Directivity);
            if (existing.isValid())
                existing.copyPropertiesAndChildrenFrom (loaded, undoManager);
        }
    }

    if (scope.includeLiveSource)
    {
        auto loaded = inputData.getChildWithName (LiveSourceTamer);
        if (loaded.isValid())
        {
            auto existing = input.getChildWithName (LiveSourceTamer);
            if (existing.isValid())
                existing.copyPropertiesAndChildrenFrom (loaded, undoManager);
        }
    }

    if (scope.includeHackoustics)
    {
        auto loaded = inputData.getChildWithName (Hackoustics);
        if (loaded.isValid())
        {
            auto existing = input.getChildWithName (Hackoustics);
            if (existing.isValid())
                existing.copyPropertiesAndChildrenFrom (loaded, undoManager);
        }
    }

    if (scope.includeLFO)
    {
        auto loaded = inputData.getChildWithName (LFO);
        if (loaded.isValid())
        {
            auto existing = input.getChildWithName (LFO);
            if (existing.isValid())
                existing.copyPropertiesAndChildrenFrom (loaded, undoManager);
        }
    }

    if (scope.includeAutomOtion)
    {
        auto loaded = inputData.getChildWithName (AutomOtion);
        if (loaded.isValid())
        {
            auto existing = input.getChildWithName (AutomOtion);
            if (existing.isValid())
                existing.copyPropertiesAndChildrenFrom (loaded, undoManager);
        }
    }

    if (scope.includeMutes)
    {
        auto loaded = inputData.getChildWithName (Mutes);
        if (loaded.isValid())
        {
            auto existing = input.getChildWithName (Mutes);
            if (existing.isValid())
                existing.copyPropertiesAndChildrenFrom (loaded, undoManager);
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

void WFSFileManager::setError (const juce::String& error)
{
    lastError = error;
    DBG ("WFSFileManager Error: " + error);
}
