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

juce::File WFSFileManager::getAudioPatchFile() const
{
    return projectFolder.getChildFile ("audio_patch" + juce::String (audioPatchExtension));
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

    auto file = getCompleteConfigFile();

    // Create backup if file exists
    if (file.existsAsFile())
        createBackup (file);

    return writeToXmlFile (valueTreeState.getState(), file);
}

bool WFSFileManager::loadCompleteConfig()
{
    if (!hasValidProjectFolder())
    {
        setError ("No valid project folder");
        return false;
    }

    auto file = getCompleteConfigFile();
    auto loadedState = readFromXmlFile (file);

    if (!loadedState.isValid())
        return false;

    if (!valueTreeState.validateState (loadedState))
    {
        setError ("Invalid configuration file structure");
        return false;
    }

    valueTreeState.beginUndoTransaction ("Load Complete Configuration");
    valueTreeState.replaceState (loadedState);
    return true;
}

bool WFSFileManager::loadCompleteConfigBackup (int backupIndex)
{
    auto backups = getBackups ("show");
    if (backupIndex >= 0 && backupIndex < backups.size())
        return importCompleteConfig (backups[backupIndex]);

    setError ("Backup not found");
    return false;
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

    valueTreeState.beginUndoTransaction ("Import Complete Configuration");
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

    valueTreeState.beginUndoTransaction ("Import System Configuration");

    auto configTree = loadedState.getChildWithName (Config);
    if (configTree.isValid())
        applyConfigSection (configTree);

    auto audioPatchTree = loadedState.getChildWithName (AudioPatch);
    if (audioPatchTree.isValid())
        applyAudioPatchSection (audioPatchTree);

    return true;
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

    valueTreeState.beginUndoTransaction ("Import Input Configuration");
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

    valueTreeState.beginUndoTransaction ("Import Output Configuration");
    return applyOutputsSection (outputsTree);
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
    for (auto& type : { "show", "system", "inputs", "outputs" })
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

    // Create human-readable XML with header
    juce::String header = createXmlHeader (file.getFileNameWithoutExtension());
    juce::String xmlString = header + xml->toString();

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

    return juce::ValueTree::fromXml (*xml);
}

juce::ValueTree WFSFileManager::extractConfigSection() const
{
    return valueTreeState.getState().getChildWithName (Config);
}

juce::ValueTree WFSFileManager::extractInputsSection() const
{
    return valueTreeState.getState().getChildWithName (Inputs);
}

juce::ValueTree WFSFileManager::extractOutputsSection() const
{
    return valueTreeState.getState().getChildWithName (Outputs);
}

juce::ValueTree WFSFileManager::extractAudioPatchSection() const
{
    return valueTreeState.getState().getChildWithName (AudioPatch);
}

bool WFSFileManager::applyConfigSection (const juce::ValueTree& configTree)
{
    auto existingConfig = valueTreeState.getConfigState();
    if (existingConfig.isValid())
    {
        existingConfig.copyPropertiesAndChildrenFrom (configTree, valueTreeState.getUndoManager());
        return true;
    }
    return false;
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
