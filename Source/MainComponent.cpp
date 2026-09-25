#include "MainComponent.h"
#include "../spatcore/wfs/RenderSourceMap.h"
#include "../spatcore/effects/EffectPresets.h"
#include "WFSLogger.h"
#include "Parameters/VarCoercion.h"
#include "gui/ChannelIdentityGate.h"
#include "AppSettings.h"
#include "Parameters/WFSParameterIDs.h"
#include "Parameters/EffectsSnapshotScope.h"
#include "Localization/LocalizationManager.h"
#include "Accessibility/TTSManager.h"
#include "Network/QLabCueBuilder.h"
#include "Network/OSCMessageRouter.h"
#include "Network/MCP/tools/ChannelLifecycleTools.h"
#include "Network/MCP/MCPSurfaceAudit.h"
#include "Controllers/DialsAndButtons/pages/InputsTabPages.h"
#include "Controllers/DialsAndButtons/pages/GradientMapPages.h"
#include "Controllers/DialsAndButtons/pages/NetworkTabPages.h"
#include "Controllers/DialsAndButtons/pages/OutputsTabPages.h"
#include "Controllers/DialsAndButtons/pages/SystemConfigTabPages.h"
#include "Controllers/DialsAndButtons/pages/MapTabPages.h"
#include "Controllers/DialsAndButtons/pages/ReverbTabPages.h"
#include "Controllers/DialsAndButtons/pages/EffectsTabPages.h"
#include "Controllers/DialsAndButtons/pages/ClustersTabPages.h"
#include "Controllers/DialsAndButtons/pages/PatchWindowPages.h"
#include "../spatcore/controllers/spacemouse/SpaceMouseDevice.h"
#include "../spatcore/controllers/lightpad/LightpadManager.h"
#include "../spatcore/binaural/SofaLoader.h"

namespace
{
    /** Where the generated MCP tool manifest lives, across the four layouts this
        app is run from. Shared by the MCP server and the surface self-test so the
        two can never disagree about which manifest they are talking about. */
    juce::File findGeneratedToolsJson()
    {
        auto exeDir = juce::File::getSpecialLocation (juce::File::currentExecutableFile).getParentDirectory();

        auto candidate = exeDir.getChildFile ("MCP/generated_tools.json");
        if (candidate.existsAsFile())
            return candidate;

        // macOS bundle: exe is at Contents/MacOS/, postbuild stages this at
        // Contents/Resources/MCP/ (same convention as lang/).
        candidate = exeDir.getParentDirectory().getChildFile ("Resources/MCP/generated_tools.json");
        if (candidate.existsAsFile())
            return candidate;

        // Windows VS dev: exe at Builds/VisualStudio2022/x64/Debug/App/
        candidate = exeDir.getParentDirectory()   // x64/Debug
                          .getParentDirectory()   // x64
                          .getParentDirectory()   // VisualStudio2022
                          .getParentDirectory()   // Builds
                          .getParentDirectory()   // project root
                          .getChildFile ("Source/Network/MCP/generated_tools.json");
        if (candidate.existsAsFile())
            return candidate;

        // macOS dev: exe at Builds/MacOSX/build/Debug/WFS-DIY.app/Contents/MacOS/
        return exeDir.getParentDirectory()   // Contents
                     .getParentDirectory()   // .app
                     .getParentDirectory()   // Debug
                     .getParentDirectory()   // build
                     .getParentDirectory()   // MacOSX
                     .getParentDirectory()   // Builds
                     .getParentDirectory()   // project root
                     .getChildFile ("Source/Network/MCP/generated_tools.json");
    }
}

//==============================================================================
MainComponent::MainComponent()
{
    // Initialize localization - try to load language file from Resources/lang/
    auto& locMgr = LocalizationManager::getInstance();
    auto exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();

    // Try multiple locations for Resources folder:
    // 1. Next to executable (production: lang/ sits beside the .exe)
    // 2. Next to executable under Resources/ subfolder
    // 3. macOS bundle: Contents/Resources (standard bundle structure)
    // 4. Project root (development from Visual Studio - 5 levels up from exe)
    // 5. Project root (development from macOS - 6 levels up from Contents/MacOS/)
    juce::File resourceDir = exeDir;

    if (!resourceDir.getChildFile("lang/en.json").existsAsFile())
    {
        // Resources subfolder next to exe
        resourceDir = exeDir.getChildFile("Resources");
    }

    if (!resourceDir.getChildFile("lang/en.json").existsAsFile())
    {
        // macOS bundle path: exe is at Contents/MacOS/, resources at Contents/Resources/
        resourceDir = exeDir.getParentDirectory().getChildFile("Resources");
    }

    if (!resourceDir.getChildFile("lang/en.json").existsAsFile())
    {
        // Linux installed layout (FHS / XDG): bin at <prefix>/bin/WFS-DIY,
        // data at <prefix>/share/wfs-diy/. This is what tools/linux/build-app-tarball.sh
        // produces via install.sh for both --user (~/.local) and --system (/opt/wfs-diy).
        resourceDir = exeDir.getParentDirectory().getChildFile("share/wfs-diy");
    }

    if (!resourceDir.getChildFile("lang/en.json").existsAsFile())
    {
        // Windows dev path: go up from Builds/VisualStudio2022/x64/Debug/App to project root
        auto projectRoot = exeDir.getParentDirectory()  // x64/Debug
                                 .getParentDirectory()  // x64
                                 .getParentDirectory()  // VisualStudio2022
                                 .getParentDirectory()  // Builds
                                 .getParentDirectory(); // Project root
        resourceDir = projectRoot.getChildFile("Resources");
    }

    if (!resourceDir.getChildFile("lang/en.json").existsAsFile())
    {
        // macOS dev path: exe is at Builds/MacOSX/build/Debug/WFS-DIY.app/Contents/MacOS/
        // Go up 7 levels to reach project root
        auto projectRoot = exeDir.getParentDirectory()  // Contents/
                                 .getParentDirectory()  // WFS-DIY.app/
                                 .getParentDirectory()  // Debug/
                                 .getParentDirectory()  // build/
                                 .getParentDirectory()  // MacOSX/
                                 .getParentDirectory()  // Builds/
                                 .getParentDirectory(); // Project root
        resourceDir = projectRoot.getChildFile("Resources");
    }

    locMgr.setResourceDirectory(resourceDir);

    // Load saved channel counts and device state
    juce::PropertiesFile::Options options;
    options.applicationName = "WFS-DIY";
    options.filenameSuffix = ".settings";
    options.osxLibrarySubFolder = "Application Support";
    options.folderName = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                            .getChildFile("WFS-DIY").getFullPathName();

    // Ensure the settings folder exists
    juce::File settingsFolder(options.folderName);
    if (!settingsFolder.exists())
        settingsFolder.createDirectory();

    juce::PropertiesFile props(options);

    // Load saved language preference from app settings, default to "en"
    juce::String savedLanguage = props.getValue("language", "en");
    // Load saved translation tier (how much of the UI is translated), default Minimal.
    auto savedTier = LocalizationManager::tierFromString(props.getValue("translationTier", "minimal"));
    if (!locMgr.loadLanguage(savedLanguage, savedTier))
    {
        // Fall back to English if saved language fails to load
        if (savedLanguage != "en")
            locMgr.loadLanguage("en");
        DBG("LocalizationManager: Could not load " + savedLanguage + ".json - using fallback");
    }

    // Load channel counts (persisted), clamp, and fall back to a usable default
    numInputChannels = props.getIntValue("numInputChannels", 0);
    numOutputChannels = props.getIntValue("numOutputChannels", 2);
    numInputChannels = juce::jlimit(0, WFSParameterDefaults::maxInputChannels, numInputChannels);
    numOutputChannels = juce::jlimit(2, WFSParameterDefaults::maxOutputChannels, numOutputChannels);
    if (numInputChannels == 0)
        numInputChannels = 2; // Default to a sensible input count to avoid silent processing
    recomputeRenderSourceCount();  // keep the renderer dimension in lockstep

    // Initialize routing matrices with default values
    resizeRoutingMatrices();

    // Load saved audio device state (XML for fast restoration, type/name for fallback)
    juce::String savedDeviceStateXml = props.getValue("audioDeviceState");
    juce::String savedDeviceType = props.getValue("audioDeviceType");
    juce::String savedDeviceName = props.getValue("audioDeviceName");

    // Audio device selector moved to AudioInterfaceWindow
    // Accessible via "Audio Interface and Patching Window" button in System Config tab
    /* Original code - now in AudioInterfaceWindow:
    audioSetupComp.reset(new juce::AudioDeviceSelectorComponent(
        deviceManager, 0, 64, 2, 64, false, false, false, false));
    addAndMakeVisible(audioSetupComp.get());
    */

    // Processing toggle moved to System Config tab
    // Processing state is now managed via parameters.getConfigParam("ProcessingEnabled")
    /* Original processing toggle code - now in SystemConfigTab:
    processingToggle.setButtonText("Processing ON/OFF");
    processingToggle.setToggleState(false, juce::dontSendNotification);
    processingToggle.onClick = [this]() { ... };
    addAndMakeVisible(processingToggle);
    */

    // Load initial processing state from parameters
    processingEnabled = (bool)parameters.getConfigParam("ProcessingEnabled");
    if (processingEnabled && !audioEngineStarted)
    {
        startAudioEngine();
    }

    // Input/Output channel count controls moved to System Config tab
    // Channel counts are now managed via parameters
    /* Original channel count controls - now in SystemConfigTab:
    numInputsLabel / numInputsSlider - "Input Channels"
    numOutputsLabel / numOutputsSlider - "Output Channels"
    */

    // Load initial channel counts from parameters
    numInputChannels = (int)parameters.getConfigParam("InputChannels");
    numOutputChannels = (int)parameters.getConfigParam("OutputChannels");
    recomputeRenderSourceCount();  // keep the renderer dimension in lockstep
    resizeRoutingMatrices();

    // Algorithm selector moved to System Config tab
    // Listen for algorithm changes from parameters and apply them
    // Note: The algorithmSelector UI is now in SystemConfigTab.h
    /* Original algorithm selector code - now in SystemConfigTab:
    algorithmLabel.setText("Algorithm:", juce::dontSendNotification);
    algorithmLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(algorithmLabel);

    algorithmSelector.addItem("InputBuffer (read-time delays)", 1);
    algorithmSelector.addItem("OutputBuffer (write-time delays)", 2);
    algorithmSelector.setSelectedId(1, juce::dontSendNotification);
    */

    // Load initial algorithm from parameters
    int algorithmId = (int)parameters.getConfigParam("ProcessingAlgorithm");
    if (algorithmId == 1)
        currentAlgorithm = ProcessingAlgorithm::InputBuffer;
    else if (algorithmId == 2)
        currentAlgorithm = ProcessingAlgorithm::OutputBuffer;
#if WFS_GPU_NATIVE
    else if (algorithmId == 3)
        currentAlgorithm = ProcessingAlgorithm::NativeGpuWfs;
    else if (algorithmId == 4)
        currentAlgorithm = ProcessingAlgorithm::NativeGpuOutputBuffer;
#endif

    /* Algorithm change handler - no longer needed as UI is in SystemConfigTab
    auto algorithmChangeHandler = [this]() {
        int selectedId = (int)parameters.getConfigParam("ProcessingAlgorithm");
        ProcessingAlgorithm newAlgorithm = currentAlgorithm;
        if (selectedId == 1)
            newAlgorithm = ProcessingAlgorithm::InputBuffer;
        else if (selectedId == 2)
            newAlgorithm = ProcessingAlgorithm::OutputBuffer;

        // Only act if algorithm actually changed
        if (newAlgorithm != currentAlgorithm)
        {
            // If audio engine is running, clean up old processors
            if (audioEngineStarted)
            {
                // Remember if processing was enabled
                bool wasEnabled = processingEnabled;
                processingEnabled = false;

                // Stop and clear old processors based on CURRENT algorithm
                if (currentAlgorithm == ProcessingAlgorithm::InputBuffer)
                {
                    inputAlgorithm.releaseResources();
                    inputAlgorithm.clear();
                }
                else if (currentAlgorithm == ProcessingAlgorithm::OutputBuffer)
                {
                    outputAlgorithm.releaseResources();
                    outputAlgorithm.clear();
                }

                // Mark engine as not started (processors cleared)
                audioEngineStarted = false;

                // Update to new algorithm
                currentAlgorithm = newAlgorithm;

                // Restart with new algorithm if processing was enabled
                if (wasEnabled)
                {
                    startAudioEngine();
                    processingEnabled = true;
                    processingToggle.setToggleState(true, juce::dontSendNotification);
                }
            }
            else
            {
                // Engine not running, just update the algorithm
                currentAlgorithm = newAlgorithm;
            }
        }
    }; // End of commented algorithm change handler
    */
    // Create and apply custom LookAndFeel before any UI components are constructed,
    // so TextEditors (which cache their text color at construction time) pick up the
    // correct WfsLookAndFeel colors rather than JUCE's default dark-text LookAndFeel_V4.
    wfsLookAndFeel = std::make_unique<WfsLookAndFeel>();
    juce::LookAndFeel::setDefaultLookAndFeel(wfsLookAndFeel.get());

    // Set up tabbed interface
    addAndMakeVisible(tabbedComponent);
    tabbedComponent.setOutline(0);

    // Create status bar
    statusBar = new StatusBar();
    addAndMakeVisible(statusBar);

    // Create update banner (hidden by default)
    updateBanner = std::make_unique<UpdateBanner>();
    updateBanner->onDismiss = [this]() { resized(); };
    addChildComponent (updateBanner.get());

    // A different project means a different snapshot folder, so every armed
    // MIDI note belongs to the previous project. Wired before the restore below
    // so later folder changes are covered; the initial index build happens
    // explicitly once midiSnapshotTrigger exists (this fires before that).
    snapshotSession = std::make_unique<SnapshotSession> (parameters);

    parameters.getFileManager().onProjectFolderChanged = [this]() {
        refreshMidiSnapshotBindings();
        if (snapshotSession != nullptr)
            snapshotSession->projectFolderChanged();
    };

    // Restore project folder from AppSettings (persists across sessions)
    {
        auto folder = AppSettings::getLastFolder ("lastProjectFolder", juce::File());
        if (folder.isDirectory())
            parameters.getFileManager().setProjectFolder (folder);
    }

    // Create tabs
    systemConfigTab = new SystemConfigTab(parameters);
    networkTab = new NetworkTab(parameters);
    outputsTab = new OutputsTab(parameters);
    inputsTab = new InputsTab(parameters, *snapshotSession);
    clustersTab = new ClustersTab(parameters);
    reverbTab = new ReverbTab(parameters);
    effectsTab = new EffectsTab(parameters, *snapshotSession);
    mapTab = std::make_unique<MapTab>(parameters);

    // Set accessible names for screen readers (prevents "Custom" announcement)
    systemConfigTab->setName("System Configuration");
    networkTab->setName("Network");
    outputsTab->setName("Outputs");
    inputsTab->setName("Inputs");
    clustersTab->setName("Clusters");
    reverbTab->setName("Reverb");
    effectsTab->setName("Effects");
    mapTab->setName("Map");

    // Pass status bar to tabs that support it
    systemConfigTab->setStatusBar(statusBar);
    systemConfigTab->setAudioDeviceManager(&deviceManager);
    networkTab->setStatusBar(statusBar);
    outputsTab->setStatusBar(statusBar);
    inputsTab->setStatusBar(statusBar);
    reverbTab->setStatusBar(statusBar);
    effectsTab->setStatusBar(statusBar);
    clustersTab->setStatusBar(statusBar);
    mapTab->setStatusBar(statusBar);

    // Note: AutomOtionProcessor is set after it's created (later in constructor)

    // Set up callbacks from System Config tab
    systemConfigTab->setProcessingCallback([this](bool enabled) {
        handleProcessingChange(enabled);
        // Refresh Stream Deck page so start button appears/disappears
        if (streamDeckManager && streamDeckManager->getCurrentMainTab() == SystemConfigTabPages::SYSCONFIG_MAIN_TAB_INDEX)
            streamDeckManager->refreshCurrentPage();
    });

    systemConfigTab->setChannelCountCallback([this] {
        handleChannelCountChange();
    });

    systemConfigTab->setAlgorithmChangedCallback([this](int selectedId) {
        handleAlgorithmSelectionChange(selectedId);
    });

#if WFS_GPU_NATIVE
    systemConfigTab->setGpuDepthChangedCallback([this](int depthBlocks) {
        handleGpuDepthChange(depthBlocks);
    });
#endif

    // Solo/Mute/EditOnMap callbacks set later (after shared state created for StreamDeck sync)

    systemConfigTab->setAudioInterfaceCallback([this]() {
        openAudioInterfaceWindow();
    });

    systemConfigTab->setConfigReloadedCallback([this]() {
        handleConfigReloaded();
    });

    systemConfigTab->setGettingStartedCallback([this]() {
        openGettingStartedWizard();
    });

    systemConfigTab->setDialsAndButtonsCallback ([this] (int deviceIndex)
    {
        // 0=Off, 1=Stream Deck+

        // Stream Deck+: check for Elgato app conflict
        if (deviceIndex == 1 && StreamDeckDevice::isStreamDeckAppRunning())
        {
            auto options = juce::MessageBoxOptions()
                .withIconType (juce::MessageBoxIconType::WarningIcon)
                .withTitle ("Stream Deck App Conflict")
                .withMessage ("The Elgato Stream Deck app is running and will prevent "
                              "direct HID access to the device.\n\n"
                              "Close the Stream Deck app to use it with WFS-DIY?")
                .withButton ("Close App")
                .withButton ("Cancel")
                .withAssociatedComponent (this);

            juce::AlertWindow::showAsync (options, [this] (int result)
            {
                if (result == 1)  // Close App
                {
                    StreamDeckDevice::killStreamDeckApp();
                    if (streamDeckManager)
                        streamDeckManager->setEnabled (true);
                }
                else  // Cancel — revert to Off
                {
                    parameters.setConfigParam ("DialsAndButtonsDevice", 0);
                    if (systemConfigTab)
                        systemConfigTab->reloadDialsAndButtonsSelector();
                    if (streamDeckManager)
                        streamDeckManager->setEnabled (false);
                }
            });
            return;
        }

        if (streamDeckManager)
            streamDeckManager->setEnabled (deviceIndex == 1);
    });

    systemConfigTab->setPositionControlCallback ([this] (int deviceIndex)
    {
        // 0=Off, 1=SpaceMouse, 2=Joystick, 3=GamePad
        if (! controllerManager)
            return;

        if (deviceIndex == 0)
        {
            controllerManager->setEnabled (false);
            return;
        }

        // SpaceMouse: check for 3DxWare driver conflict
        if (deviceIndex == 1 && SpaceMouseDevice::is3DxWareRunning())
        {
            auto options = juce::MessageBoxOptions()
                .withIconType (juce::MessageBoxIconType::WarningIcon)
                .withTitle ("3DConnexion Driver Conflict")
                .withMessage ("The 3DxWare driver is running and will send duplicate scroll/zoom "
                              "events that interfere with the map.\n\n"
                              "Close the driver to use the SpaceMouse directly?")
                .withButton ("Close Driver")
                .withButton ("Cancel")
                .withAssociatedComponent (this);

            juce::AlertWindow::showAsync (options, [this] (int result)
            {
                if (result == 1)  // Close Driver
                {
                    SpaceMouseDevice::kill3DxWareProcesses();
                    if (controllerManager)
                        controllerManager->setEnabled (true);
                }
                else  // Cancel — revert to Off
                {
                    parameters.setConfigParam ("PositionControlDevice", 0);
                    if (systemConfigTab)
                        systemConfigTab->reloadPositionControlSelector();
                    if (controllerManager)
                        controllerManager->setEnabled (false);
                }
            });
            return;
        }

        controllerManager->setEnabled (true);
    });

    systemConfigTab->setSamplerCallback([this](bool enabled) {
        if (inputsTab)
            inputsTab->setSamplerMasterEnabled(enabled);

        // Apply controller mode when sampler is toggled
        int ctrlMode = enabled ? static_cast<int> (parameters.getConfigParam ("SamplerControllerMode")) : 0;
        applySamplerControllerMode (ctrlMode);
    });

    systemConfigTab->onSamplerControllerModeChanged = [this] (int mode) {
        applySamplerControllerMode (mode);
    };

    systemConfigTab->setLightpadSplitCallback([this](int padIndex, bool split) {
        if (lightpadManager)
            lightpadManager->setPadSplit (padIndex, split);
    });

    systemConfigTab->setBinauralCallback([this](bool /*enabled*/) {
        // Refresh Stream Deck page so start button appears/disappears
        if (streamDeckManager && streamDeckManager->getCurrentMainTab() == SystemConfigTabPages::SYSCONFIG_MAIN_TAB_INDEX)
            streamDeckManager->refreshCurrentPage();
    });

    systemConfigTab->onQuickLongPressChanged = [this](bool enabled) {
        parameters.setConfigParam("QuickLongPress", enabled ? 1 : 0);
    };

    // Set up callbacks for individual tab config reloads
    outputsTab->onConfigReloaded = [this]() {
        handleConfigReloaded();
    };

    // A relabel or rearrangement done from an identity dialog on the Inputs
    // tab: the one funnel that also re-sends the remote channel inventory.
    inputsTab->onStructureChanged = [this]() {
        handleChannelCountChange();
    };

    inputsTab->onConfigReloaded = [this]() {
        handleConfigReloaded();
    };

    // The snapshot row's "Reload Snapshot" long-press (on the Inputs tab or the
    // Effects tab) goes through the same seam as OSC and MIDI, so the recall
    // logic exists in exactly one place.
    snapshotSession->onSnapshotRecallRequested = [this](const juce::String& snapshotName) {
        recallSnapshotByName (snapshotName);
    };

    // Any snapshot created / updated / deleted / re-scoped can change a binding.
    snapshotSession->onSnapshotsChanged = [this]() {
        refreshMidiSnapshotBindings();
    };

    snapshotSession->onConfigReloaded = [this]() {
        handleConfigReloaded();
    };

    snapshotSession->onStructureChanged = [this]() {
        handleChannelCountChange();
    };

    snapshotSession->showStatus = [this](const juce::String& text) {
        if (statusBar != nullptr)
            statusBar->showTemporaryMessage (text, 3000);
    };

    snapshotSession->isQLabAvailable = [this]() {
        return oscManager && oscManager->hasQLabTarget();
    };

    // QLab export callback for the snapshot row
    snapshotSession->onQLabExportRequested = [this](const juce::String& snapshotName,
                                                     const WFSFileManager::ExtendedSnapshotScope& scope) {
        if (!oscManager || !oscManager->hasQLabTarget())
        {
            if (inputsTab != nullptr)
                inputsTab->showStatusMessage (LOC("snapshot.qlabNoTarget"));
            return;
        }

        auto& fileManager = parameters.getFileManager();
        auto snapshotFile = fileManager.getInputSnapshotsFolder().getChildFile (snapshotName + ".xml");
        auto xml = juce::XmlDocument::parse (snapshotFile);

        if (xml == nullptr)
        {
            if (inputsTab != nullptr)
                inputsTab->showStatusMessage ("QLab export: could not read snapshot file");
            return;
        }

        auto snapshot = juce::ValueTree::fromXml (*xml);
        auto inputsData = snapshot.getChildWithName (WFSParameterIDs::Inputs);
        auto effectsData = snapshot.getChildWithName (WFSParameterIDs::Effects);   // one file, both families

        if (!inputsData.isValid() && !effectsData.isValid())
        {
            if (inputsTab != nullptr)
                inputsTab->showStatusMessage ("QLab export: no input data in snapshot");
            return;
        }

        int numChannels = parameters.getNumInputChannels();
        int patchNumber = oscManager->getQLabPatchNumber();

        // Fold the global sampler master into the scope so QLab applies the
        // same rule as the file-manager save/recall paths.
        const auto effScope = scope.withGlobals (fileManager.isSamplerMasterOn(), numChannels);

        // The snapshot file keys its <Input> nodes by permanent number while the
        // scope mask is keyed by slot. Only live state can pair the two: without
        // this resolver the builder falls back to number - 1, which drops a
        // channel whose number exceeds the count and filters every other channel
        // through a neighbour's scope column once the list is reordered.
        auto& vts = parameters.getValueTreeState();
        const auto numberToSlot = [&vts] (int number) { return vts.getSlotForChannelNumber (number); };

        const int numEffects = parameters.getNumEffectChannels();
        const int numOutputs = parameters.getNumOutputChannels();
        int cueCount = WFSNetwork::QLabCueBuilder::countCues (inputsData, effScope, numChannels, numberToSlot,
                                                             effectsData, numEffects, numOutputs);

        if (cueCount == 0)
        {
            if (inputsTab != nullptr)
                inputsTab->showStatusMessage ("QLab export: no parameters in scope");
            return;
        }

        auto sequence = WFSNetwork::QLabCueBuilder::buildSnapshotCues (
            snapshotName, inputsData, effScope, numChannels, patchNumber, numberToSlot,
            numOutputs, effectsData, numEffects);

        // A one-output rig's per-output mute row of an effect is a lone number,
        // which the receiver refuses as a row: say so rather than drop it silently.
        {
            int skippedRows = 0;
            WFSNetwork::QLabCueBuilder::collectEffectCues (effectsData, effScope.effects, numEffects,
                                                           numOutputs, &skippedRows);
            if (skippedRows > 0)
                WFSLogger::getInstance().logWarning ("QLab export of '" + snapshotName + "': "
                                                     + juce::String (skippedRows) + " effect mute row(s) not exported"
                                                     " (one output: a one-token row is not sendable; the snapshot"
                                                     " load cue still recalls them)");
        }

        oscManager->sendToQLab (sequence, [this, cueCount](int /*sentCount*/) {
            if (inputsTab != nullptr)
                inputsTab->showStatusMessage (
                    LOC("snapshot.qlabExportDone").replace ("{count}", juce::String (cueCount)));
        });

        if (inputsTab != nullptr)
            inputsTab->showStatusMessage (
                LOC("snapshot.qlabExportStarted").replace ("{count}", juce::String (cueCount)));
    };

    // QLab snapshot load cue callback
    snapshotSession->onQLabSnapshotLoadCueRequested = [this](const juce::String& snapshotName) {
        if (!oscManager || !oscManager->hasQLabTarget())
            return;

        int patchNumber = oscManager->getQLabPatchNumber();
        auto sequence = WFSNetwork::QLabCueBuilder::buildSnapshotLoadCue (snapshotName, patchNumber);

        oscManager->sendToQLab (sequence, [this, snapshotName](int /*sentCount*/) {
            if (inputsTab != nullptr)
                inputsTab->showStatusMessage ("QLab load cue created for: " + snapshotName);
        });
    };

    // Level Meter window callbacks for InputsTab and OutputsTab
    inputsTab->onLevelMeterWindowRequested = [this]() {
        openLevelMeterWindow();
    };

    outputsTab->onLevelMeterWindowRequested = [this]() {
        openLevelMeterWindow();
    };

    outputsTab->onArrayHelperOpened = [this]() {
        // If wizard is on the "Open Wizard of OutZ" step, auto-advance
        if (gettingStartedWizard && gettingStartedWizard->isActive()
            && gettingStartedWizard->getCurrentStepIndex() == 8)
        {
            gettingStartedWizard->nextStep();
        }
    };

    reverbTab->onConfigReloaded = [this]() {
        handleConfigReloaded();
    };

    // Gradient map editor change callback — re-rasterize for current channel
    inputsTab->getGradientMapEditor().onGradientMapsChanged = [this]() {
        // getCurrentChannel() is the permanent channel NUMBER; the evaluator
        // array is slot-indexed, and numbers have gaps and are not in slot
        // order after a reorder. rebuildGradientMapForInput ignores -1.
        int ch = parameters.getValueTreeState().getSlotForChannelNumber (inputsTab->getCurrentChannel());
        rebuildGradientMapForInput (ch);
    };

    // Bidirectional sync: editor → StreamDeck
    inputsTab->getGradientMapEditor().onActiveLayerChanged = [this]()
    {
        if (streamDeckManager && streamDeckManager->getCurrentMainTab() == InputsTabPages::INPUTS_MAIN_TAB_INDEX
            && streamDeckManager->getCurrentSubTab() == 3)
        {
            auto* ed = &inputsTab->getGradientMapEditor();
            streamDeckManager->setActiveSection (ed->getActiveLayerIndex());
        }
    };

    inputsTab->getGradientMapEditor().onSelectionChanged = [this]()
    {
        if (streamDeckManager && streamDeckManager->getCurrentMainTab() == InputsTabPages::INPUTS_MAIN_TAB_INDEX
            && streamDeckManager->getCurrentSubTab() == 3)
            streamDeckManager->refreshCurrentPage();
    };

    // Sampler subtab change callback — push updated data to SamplerManager
    inputsTab->getSamplerSubTab().onSamplerDataChanged = [this]() {
        if (samplerManager == nullptr) return;
        // getCurrentChannel() is the permanent channel NUMBER; the sampler
        // tree section and the SamplerManager arrays below are slot-indexed,
        // and numbers have gaps and are not in slot order after a reorder.
        int ch = parameters.getValueTreeState().getSlotForChannelNumber (inputsTab->getCurrentChannel());
        if (ch < 0) return;
        auto samplerTree = parameters.getValueTreeState().getInputSamplerSection (ch);
        if (samplerTree.isValid())
        {
            auto samplesFolder = parameters.getFileManager().getSamplesFolder();
            samplerManager->loadChannelCells (ch, samplerTree, samplesFolder);
            int setIdx = inputsTab->getSamplerSubTab().getActiveSetIndex();
            samplerManager->loadChannelSetFromTree (ch, samplerTree, setIdx);
            applySamplerSetPosition (ch, samplerTree, setIdx);
        }
    };

    // Sampler preview callback — trigger/stop cell playback
    inputsTab->getSamplerSubTab().onPreviewCell = [this] (int channelIndex, int cellIndex, bool noteOn)
    {
        if (samplerManager == nullptr) return;

        SamplerEngine::TouchEvent event;
        if (noteOn)
        {
            event.type = SamplerEngine::TouchEvent::NoteOn;
            event.cellIndex = cellIndex;
            event.pressure = 1.0f;
        }
        else
        {
            event.type = SamplerEngine::TouchEvent::NoteOff;
        }
        samplerManager->pushTouchEvent (channelIndex, event);
    };

    // Query callback for playing cell visual feedback
    inputsTab->getSamplerSubTab().getPlayingCellIndex = [this] (int channelIndex) -> int
    {
        if (samplerManager == nullptr) return -1;
        return samplerManager->getPlayingCellIndex (channelIndex);
    };

    // QLab sampler set cue creation
    inputsTab->getSamplerSubTab().isQLabAvailable = [this]() {
        return oscManager && oscManager->hasQLabTarget();
    };

    inputsTab->getSamplerSubTab().onQLabSetCueRequested = [this] (int channelId, int setNumber, const juce::String& setName) {
        if (! oscManager || ! oscManager->hasQLabTarget()) return;
        int patchNumber = oscManager->getQLabPatchNumber();
        auto sequence = WFSNetwork::QLabCueBuilder::buildSamplerSetCue (channelId, setNumber, setName, patchNumber);
        oscManager->sendToQLab (sequence, [this, channelId, setName] (int) {
            if (inputsTab != nullptr)
                inputsTab->showStatusMessage (LOC ("sampler.qlabSetCueCreated")
                    .replace ("{channel}", juce::String (channelId))
                    .replace ("{name}", setName));
        });
    };

    // Create global tooltip window for hover tooltips
    tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 500);

    // Add tabs to tabbed component (using localized names)
    // Store names in local variables to ensure proper String lifetime
    juce::String tabSystemConfig = LOC("tabs.systemConfig");
    juce::String tabNetwork = LOC("tabs.network");
    juce::String tabOutputs = LOC("tabs.outputs");
    juce::String tabReverb = LOC("tabs.reverb");
    juce::String tabEffects = LOC("tabs.effects");
    juce::String tabInputs = LOC("tabs.inputs");
    juce::String tabClusters = LOC("tabs.clusters");
    juce::String tabMap = LOC("tabs.map");

    tabbedComponent.addTab(tabSystemConfig, ColorScheme::get().chromeBackground, systemConfigTab, true);
    tabbedComponent.addTab(tabNetwork, ColorScheme::get().chromeBackground, networkTab, true);
    tabbedComponent.addTab(tabOutputs, ColorScheme::get().chromeBackground, outputsTab, true);
    tabbedComponent.addTab(tabReverb, ColorScheme::get().chromeBackground, reverbTab, true);
    tabbedComponent.addTab(tabEffects, ColorScheme::get().chromeBackground, effectsTab, true);
    tabbedComponent.addTab(tabInputs, ColorScheme::get().chromeBackground, inputsTab, true);
    tabbedComponent.addTab(tabClusters, ColorScheme::get().chromeBackground, clustersTab, true);
    tabbedComponent.addTab(tabMap, ColorScheme::get().chromeBackground, mapTab.get(), false);

    // Wire per-tab undo domain: Ctrl+Z only affects the currently focused tab
    tabbedComponent.onTabChanged = [this](int tabIndex) {
        // Indexed by TabIndex::*, sized by TabIndex::Count - the order here is
        // the order of the addTab calls above and nothing else may set it.
        static const UndoDomain domainForTab[TabIndex::Count] = {
            UndoDomain::Config,   // SystemConfig
            UndoDomain::Config,   // Network
            UndoDomain::Output,   // Outputs
            UndoDomain::Reverb,   // Reverb
            UndoDomain::Effects,  // Effects
            UndoDomain::Input,    // Inputs
            UndoDomain::Clusters, // Clusters
            UndoDomain::Map       // Map
        };
        if (tabIndex >= 0 && tabIndex < TabIndex::Count)
            parameters.getValueTreeState().setActiveDomain (domainForTab[tabIndex]);
        if (controllerManager)
            controllerManager->activeTab = tabIndex;
        if (streamDeckManager)
        {
            // Sync subtab + channel state atomically before page render
            if (tabIndex == TabIndex::Reverb && reverbTab != nullptr)
                streamDeckManager->syncNavigation (tabIndex, reverbTab->getCurrentSubTab(), reverbTab->getCurrentChannel());
            else if (tabIndex == TabIndex::Effects && effectsTab != nullptr)
                streamDeckManager->syncNavigation (tabIndex, effectsTab->getCurrentSubTab(), effectsTab->getCurrentChannel());
            else if (tabIndex == TabIndex::Outputs && outputsTab != nullptr)
                streamDeckManager->syncNavigation (tabIndex, 0, outputsTab->getCurrentChannel());
            else if (tabIndex == TabIndex::Inputs && inputsTab != nullptr)
                streamDeckManager->syncNavigation (tabIndex, 0, inputsTab->getCurrentChannel());
            else
                streamDeckManager->setMainTab (tabIndex);
        }
        // Ownership rule: only the MAP tab latches position ownership — merely
        // looking at the Inputs/Outputs/Reverb/Effects tabs does not (editing a
        // position there latches it via the parameter setters instead).
        // Channel NUMBER ownership deliberately does NOT latch on tab visits:
        // merely looking at the numbers keeps the session fresh. It is spent by
        // the acts that commit them — a project save/load, an actual patch
        // edit, snapshots, or any wire message naming a channel by number.
        if (tabIndex == TabIndex::Map && systemConfigTab != nullptr)
            systemConfigTab->setMapTabVisited();
        resetHelpCycle();
    };

    // Load saved color scheme from parameters and apply it
    // This will trigger WfsLookAndFeel::colorSchemeChanged() to update widget colors
    int colorSchemeId = (int)parameters.getConfigParam("ColorScheme");
    ColorScheme::Manager::getInstance().setTheme(colorSchemeId);

    // Load quick long press mode
    bool quickLP = (int)parameters.getConfigParam("QuickLongPress") != 0;
    LongPressButton::setShortMode(quickLP);

    // Show system overview on launch (unless user dismissed it permanently)
    juce::Timer::callAfterDelay(500, [this]() {
        if (systemConfigTab) systemConfigTab->showOverviewIfNeeded();
    });

    // Subscribe to color scheme changes for component repaints
    ColorScheme::Manager::getInstance().addListener(this);

    // Force initial color refresh: setTheme() was called before addListener(this), so
    // colorSchemeChanged() was never triggered at startup. This ensures TextEditor cached
    // colors match the active theme from the first frame.
    colorSchemeChanged();

    // Wire detach button on Map tab
    mapTab->onDetachRequested = [this]() { detachMapTab(); };

    // Set up navigation callback from Map tab to other tabs via long-press gesture
    // Parameters: (tabType, index) where tabType is:
    //   0=Input, 1=Cluster, 2=Output, 3=Reverb, 4=Effect
    mapTab->setNavigateToItemCallback([this](int tabType, int index) {
        switch (tabType)
        {
            case 0:  // Input
                tabbedComponent.setCurrentTabIndex(TabIndex::Inputs);
                // The Map hands out a SLOT; the selector holds permanent
                // channel NUMBERS, which have gaps and are not in slot order
                // after a reorder. Cluster/output/reverb/effect ids below ARE
                // dense slot positions, so their + 1 stays.
                inputsTab->selectChannel (parameters.getValueTreeState().getInputChannelNumber (index));
                break;
            case 1:  // Cluster
                tabbedComponent.setCurrentTabIndex(TabIndex::Clusters);
                clustersTab->setSelectedCluster(index);
                break;
            case 2:  // Output
                tabbedComponent.setCurrentTabIndex(TabIndex::Outputs);
                outputsTab->selectChannel(index + 1);   // Convert 0-based to 1-based
                break;
            case 3:  // Reverb
                tabbedComponent.setCurrentTabIndex(TabIndex::Reverb);
                reverbTab->selectChannel(index + 1);    // Convert 0-based to 1-based
                break;
            case 4:  // Effect
                tabbedComponent.setCurrentTabIndex(TabIndex::Effects);
                if (effectsTab != nullptr)
                    effectsTab->selectChannel(index + 1);
                break;
        }
    });

    // Initialize OSC Manager for network communication
    oscManager = std::make_unique<WFSNetwork::OSCManager>(parameters.getValueTreeState());
    oscManager->setDirtyTracker(&parameters.getDirtyTracker());

    // Initialize MCP server (AI control surface). Phase 2 Block 1: also
    // loads the auto-generated tool surface from generated_tools.json.
    // Resolve the JSON file path by checking, in order:
    //   1. <exeDir>/MCP/generated_tools.json         (Windows/Linux production, after postbuild)
    //   2. <exeDir>/../Resources/MCP/generated_tools.json (macOS bundle production)
    //   3. <projectRoot>/Source/Network/MCP/...       (Windows VS2022 dev)
    //   4. <projectRoot>/Source/Network/MCP/...       (macOS dev)
    juce::File generatedToolsJson = findGeneratedToolsJson();

    // Phase 3 — knowledge-resource directory. Same fallback chain as
    // generated_tools.json but pointing at MCP/resources/.
    juce::File knowledgeResourcesDir;
    {
        auto exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
        knowledgeResourcesDir = exeDir.getChildFile("MCP/resources");
        if (! knowledgeResourcesDir.isDirectory())
        {
            // Linux installed layout: <prefix>/bin/WFS-DIY → <prefix>/share/wfs-diy/MCP/resources
            knowledgeResourcesDir = exeDir.getParentDirectory().getChildFile("share/wfs-diy/MCP/resources");
        }
        if (! knowledgeResourcesDir.isDirectory())
        {
            // macOS bundle: exe is at Contents/MacOS/, postbuild stages this at
            // Contents/Resources/MCP/resources (same convention as lang/, see resourceDir above).
            knowledgeResourcesDir = exeDir.getParentDirectory().getChildFile("Resources/MCP/resources");
        }
        if (! knowledgeResourcesDir.isDirectory())
        {
            // Windows VS2022 dev: project root is 5 levels up from exe
            auto projectRoot = exeDir.getParentDirectory()  // x64/Debug
                                     .getParentDirectory()  // x64
                                     .getParentDirectory()  // VisualStudio2022
                                     .getParentDirectory()  // Builds
                                     .getParentDirectory(); // Project root
            knowledgeResourcesDir = projectRoot.getChildFile("Documentation/MCP/resources");
        }
        if (! knowledgeResourcesDir.isDirectory())
        {
            // macOS dev: project root is 7 levels up from exe in Contents/MacOS
            auto projectRoot = exeDir.getParentDirectory()  // Contents
                                     .getParentDirectory()  // .app
                                     .getParentDirectory()  // Debug
                                     .getParentDirectory()  // build
                                     .getParentDirectory()  // MacOSX
                                     .getParentDirectory()  // Builds
                                     .getParentDirectory(); // Project root
            knowledgeResourcesDir = projectRoot.getChildFile("Documentation/MCP/resources");
        }
    }

    mcpServer = std::make_unique<WFSNetwork::MCPServer>(parameters.getValueTreeState(),
                                                        parameters.getFileManager(),
                                                        oscManager->getLogger(),
                                                        generatedToolsJson,
                                                        knowledgeResourcesDir);
    if (! mcpServer->start (WFSNetwork::MCPServer::kDefaultPort, /*loopbackOnly*/ true))
    {
        // Non-fatal: the app runs fine without MCP. But it used to report
        // success unconditionally, so a port clash left the Network tab
        // claiming a listening server that no client could ever reach.
        oscManager->getLogger().logText (
            "MCP server FAILED to start on port "
            + juce::String (WFSNetwork::MCPServer::kDefaultPort)
            + " - the port is in use (another WFS-DIY instance?). "
              "MCP clients will not be able to connect.");
    }

    // Structural channel edits from MCP (create/delete/type flip) run the
    // same reconfiguration pass as the System Config editor. Handlers run on
    // the message thread (the dispatcher hops there), so this is direct.
    mcpServer->setSamplerChangedCallback ([this] (int slot)
    {
        // Same shape as the gradient callback: reload the channel that CHANGED,
        // not the one the Sampler tab happens to be showing. samplerCellFile is
        // the one that matters most - without loadChannelCells running again the
        // audio file named by the write is never actually read.
        if (samplerManager == nullptr || slot < 0)
            return;
        auto samplerTree = parameters.getValueTreeState().getInputSamplerSection (slot);
        if (! samplerTree.isValid())
            return;
        auto samplesFolder = parameters.getFileManager().getSamplesFolder();
        samplerManager->loadChannelCells (slot, samplerTree, samplesFolder);
        const int setIdx = WFSVar::toInt (samplerTree.getProperty (WFSParameterIDs::inputSamplerActiveSet), 0);
        samplerManager->loadChannelSetFromTree (slot, samplerTree, setIdx);
        applySamplerSetPosition (slot, samplerTree, setIdx);
    });

    mcpServer->setGradientMapChangedCallback ([this] (int slot)
    {
        // Rebuild the channel that CHANGED. The editor's own callback rebuilds
        // whichever channel the Inputs tab is showing, which is the right answer
        // for an operator edit and the wrong one for a remote write.
        rebuildGradientMapForInput (slot);
    });

    mcpServer->setChannelTopologyChangedCallback ([this]
    {
        handleChannelCountChange();
    });

    // Automation hook (control-replay harnesses): WFS_MCP_AI_ENABLED=1 in the
    // environment flips the MCP AI master toggle on at startup — the exact
    // equivalent of the operator clicking the Network-tab "AI" button. MCP
    // stays loopback-only and tier enforcement still applies; the tier-3
    // safety gate deliberately has no such hook and remains UI-only.
    if (juce::SystemStats::getEnvironmentVariable ("WFS_MCP_AI_ENABLED", {}) == "1")
    {
        mcpServer->getTierEnforcement().setAIEnabled (true);
        oscManager->getLogger().logText ("MCP AI enabled at startup via WFS_MCP_AI_ENABLED=1");
    }

    // Phase 7: kick the OSCQuery cross-check if OSCQuery is already up
    // (e.g. saved-on-startup setting). When the user toggles OSCQuery
    // later, NetworkTab calls runOSCQueryAudit again with the new URL.
    if (oscManager->isOSCQueryRunning())
    {
        const auto port = oscManager->getOSCQueryHttpPort();
        if (port > 0)
            mcpServer->runOSCQueryAudit ("http://127.0.0.1:" + juce::String (port) + "/");
    }

    // Phase 5c: AI-undo toast overlay. Positioned in top-right of this
    // component; sized in resized(). The overlay polls the change-record
    // ring buffer at 5 Hz and renders rows with × buttons for targeted
    // undo via MCPUndoEngine::undoByIndex.
    mcpUndoOverlay = std::make_unique<MCPUndoOverlay>(mcpServer->getUndoEngine(),
                                                       mcpServer->getChangeRecords());
    addAndMakeVisible(*mcpUndoOverlay);
    mcpUndoOverlay->toFront(false);

    // Initialize Stream Deck+ physical controller
    streamDeckManager = std::make_unique<StreamDeckManager>();

    // App binding: the brightness to re-apply on (re)connect comes from the
    // persisted app settings (spatcore's manager no longer reads AppSettings).
    streamDeckManager->getConnectBrightness = [] { return AppSettings::getStreamDeckBrightness(); };

    // One undo step per deck gesture - the GUI's rule for a drag. The manager
    // announces a run of turns of one dial, a press or a confirmed choice
    // before its first write; the step opens in the active tab's history,
    // which is where the deck's page writes (the deck follows the tab).
    streamDeckManager->onEditGestureStart = [this] (const juce::String& what)
    {
        parameters.getValueTreeState().beginUndoTransaction ("Stream Deck: " + what);
    };

    // Apply initial Dials & Buttons device selection (default Off)
    {
        int dbDevice = static_cast<int> (parameters.getConfigParam ("DialsAndButtonsDevice"));
        streamDeckManager->setEnabled (dbDevice == 1);
    }

    // Position Control enable state is applied at creation time (see controllerManager init below)

    // Apply initial sampler master enable state
    bool samplerOn = (bool)parameters.getConfigParam("SamplerEnabled");
    if (inputsTab)
        inputsTab->setSamplerMasterEnabled(samplerOn);

    // Register Inputs tab pages with real parameter bindings
    {
        auto& vts = parameters.getValueTreeState();
        auto flipModeState    = std::make_shared<bool> (false);
        auto stereoParamsState = std::make_shared<bool> (false);
        auto lfoSubModeState  = std::make_shared<int> (0);
        auto outputEqBandState = std::make_shared<int> (0);

        InputsTabPages::MovementCallbacks movCB;
        movCB.startMotion  = [this](int ch) { if (automOtionProcessor) automOtionProcessor->startClusterMotion (ch); };
        movCB.stopMotion   = [this](int ch) { if (automOtionProcessor) automOtionProcessor->stopClusterMotion (ch); };
        movCB.pauseMotion  = [this](int ch) { if (automOtionProcessor) automOtionProcessor->pauseClusterMotion (ch); };
        movCB.resumeMotion = [this](int ch) { if (automOtionProcessor) automOtionProcessor->resumeClusterMotion (ch); };
        movCB.stopAll      = [this]()       { if (automOtionProcessor) automOtionProcessor->stopAllMotion(); };

        // The Effect Sends page: which four effects the deck holds, shared
        // across rebuilds like the Chain page's bank. A shift lays the page out
        // again on the next turn (never from inside the press), and the GUI's
        // strips mark the four - only while the Dials & Buttons device is the
        // Stream Deck, so a bare GUI shows no phantom deck.
        auto inputSendsWindow = std::make_shared<int> (0);
        InputsTabPages::EffectSendsCallbacks sendsCB;
        sendsCB.requestRebuild = [this]
        {
            juce::MessageManager::callAsync ([this]
            {
                if (streamDeckManager && streamDeckManager->getCurrentMainTab() == InputsTabPages::INPUTS_MAIN_TAB_INDEX)
                    streamDeckManager->refreshCurrentPage();
            });
        };
        sendsCB.onWindowChanged = [this] (int first, int count)
        {
            const bool deckSelected = static_cast<int> (parameters.getConfigParam ("DialsAndButtonsDevice")) == 1;
            auto mark = [this, first, count, deckSelected]
            {
                if (inputsTab)
                    inputsTab->getEffectSendsSubTab().setDeckWindow (deckSelected ? first : -1, count);
            };
            // The page is built on the message thread (the manager polls its
            // device from a timer), so the mark lands with the page; anything
            // else waits for the next turn.
            if (juce::MessageManager::getInstance()->isThisTheMessageThread())
                mark();
            else
                juce::MessageManager::callAsync (mark);
        };

        for (int subTab : { 0, 1, 2, 3, 4, 6 })     // 5 is the Sampler, which has no page
        {
            if (subTab == 3)
            {
                // Gradient Map subtab — use dedicated page
                GradientMapPages::GradientMapCallbacks gmCB;
                gmCB.getEditor = [this]() -> GradientMapEditor*
                {
                    return inputsTab ? &inputsTab->getGradientMapEditor() : nullptr;
                };
                streamDeckManager->registerPage (
                    InputsTabPages::INPUTS_MAIN_TAB_INDEX, 3,
                    GradientMapPages::createGradientMapPage (gmCB));
            }
            else
            {
                streamDeckManager->registerPage (
                    InputsTabPages::INPUTS_MAIN_TAB_INDEX, subTab,
                    InputsTabPages::createPage (subTab, vts, parameters.getClusterEdit(), 0, flipModeState, stereoParamsState, lfoSubModeState, movCB, inputSendsWindow, sendsCB));
            }
        }

        // Callback: sync Stream Deck band selection to the GUI EQ display
        auto onEqBandSelectedGui = [this](int bandIndex)
        {
            juce::MessageManager::callAsync ([this, bandIndex]()
            {
                if (outputsTab)
                    outputsTab->selectEqBand (bandIndex);
            });
        };

        // Register Outputs tab pages (subtab 0 = Parameters, 1 = EQ)
        for (int subTab = 0; subTab < 2; ++subTab)
        {
            streamDeckManager->registerPage (
                OutputsTabPages::OUTPUTS_MAIN_TAB_INDEX, subTab,
                OutputsTabPages::createPage (subTab, vts, parameters.getArrayEdit(), 0, outputEqBandState, onEqBandSelectedGui));
        }

        // Register Reverb tab pages (subtab 0 = Channel Params, 1 = Pre-Processing, 3 = Post-Processing)
        auto reverbPreEqBandState  = std::make_shared<int> (0);
        auto reverbPreDynMode      = std::make_shared<bool> (false);
        auto reverbPostEqBandState = std::make_shared<int> (0);
        auto reverbPostDynMode     = std::make_shared<bool> (false);
        auto reverbSoloState       = std::make_shared<bool> (false);
        auto reverbMutePreState    = std::make_shared<bool> (false);
        auto reverbMutePostState   = std::make_shared<bool> (false);
        auto reverbEditOnMapState  = std::make_shared<bool> (false);
        auto reverbAlgoSubMode     = std::make_shared<int> (0);
        auto reverbIRDuration      = std::make_shared<float> (WFSParameterDefaults::reverbIRlengthDefault);

        // Solo/Mute/EditOnMap callbacks (Stream Deck → audio engine + GUI sync)
        auto onSoloReverbSD = [this, reverbSoloState] (bool active)
        {
            soloReverbs.store (active, std::memory_order_relaxed);
            juce::MessageManager::callAsync ([this, active]()
            {
                if (reverbTab) reverbTab->setSoloReverbsFromExternal (active);
            });
        };
        auto onMutePreSD = [this, reverbMutePreState] (bool active)
        {
            muteReverbPre.store (active, std::memory_order_relaxed);
            juce::MessageManager::callAsync ([this, active]()
            {
                if (reverbTab) reverbTab->setMutePreFromExternal (active);
            });
        };
        auto onMutePostSD = [this, reverbMutePostState] (bool active)
        {
            muteReverbPost.store (active, std::memory_order_relaxed);
            juce::MessageManager::callAsync ([this, active]()
            {
                if (reverbTab) reverbTab->setMutePostFromExternal (active);
            });
        };
        auto onEditOnMapSD = [this, reverbEditOnMapState] (bool enabled)
        {
            juce::MessageManager::callAsync ([this, enabled]()
            {
                if (reverbTab) reverbTab->setEditOnMapFromExternal (enabled);
                if (mapTab) mapTab->setReverbEditMode (enabled);
            });
        };

        reverbTab->sharedIRDuration = reverbIRDuration;

        for (int subTab : { 0, 1, 2, 3 })
        {
            streamDeckManager->registerPage (
                ReverbTabPages::REVERB_MAIN_TAB_INDEX, subTab,
                ReverbTabPages::createPage (subTab, vts, 0,
                    reverbPreEqBandState, reverbPreDynMode,
                    reverbPostEqBandState, reverbPostDynMode,
                    reverbSoloState, reverbMutePreState,
                    reverbMutePostState, reverbEditOnMapState,
                    reverbAlgoSubMode, reverbIRDuration,
                    nullptr, nullptr,
                    onSoloReverbSD, onMutePreSD, onMutePostSD, onEditOnMapSD));
        }

        // The Effects pages: shared toggles the GUI mirrors, the LFO sub-mode
        // and the chain's selected module, and the callbacks that reach the
        // engine, the movement processor, the map and the sends widget. Every
        // per-channel write on the deck goes through the effect funnel, so a
        // hardware edit propagates to the link group like a GUI one.
        auto effectsSoloState     = std::make_shared<bool> (false);
        auto effectsEditOnMapState = std::make_shared<bool> (false);
        auto effectsLfoSubMode    = std::make_shared<int> (0);
        auto effectsChainSlot     = std::make_shared<int> (effectsTab != nullptr ? effectsTab->getChainSlot() : 0);   // the tab's first tile
        auto effectsChainBank     = std::make_shared<int> (0);    // which twelve of the module's controls

        EffectsTabPages::EffectsCallbacks fxCB;
        fxCB.onSoloEffectsChanged = [this] (bool active)
        {
            soloEffects.store (active, std::memory_order_relaxed);
            juce::MessageManager::callAsync ([this, active] { if (effectsTab) effectsTab->setSoloEffectsFromExternal (active); });
        };
        fxCB.onEditOnMapChanged = [this] (bool enabled)
        {
            juce::MessageManager::callAsync ([this, enabled]
            {
                if (effectsTab) effectsTab->setEditOnMapFromExternal (enabled);
                if (mapTab) mapTab->setEffectEditMode (enabled);
            });
        };
        fxCB.onClear = [this] (int fx) { if (effectsHost != nullptr) effectsHost->requestClear (fx); };
        fxCB.onChainSlotSelected = [this] (int slot)
        {
            juce::MessageManager::callAsync ([this, slot] { if (effectsTab) effectsTab->selectChainSlot (slot); });
        };
        fxCB.onRelayout = [this]
        {
            juce::MessageManager::callAsync ([this]
            {
                auto& vts = parameters.getValueTreeState();
                vts.redistributeAllEffectPositions();
                vts.getEffectsState().setProperty (WFSParameterIDs::effectPositionsUserOwned, 0, nullptr);
            });
        };
        fxCB.startMotion  = [this] (int fx) { if (effectOtomoProcessor) effectOtomoProcessor->startMotion (fx); };
        fxCB.stopMotion   = [this] (int fx) { if (effectOtomoProcessor) effectOtomoProcessor->stopMotion (fx); };
        fxCB.pauseMotion  = [this] (int fx) { if (effectOtomoProcessor) effectOtomoProcessor->pauseMotion (fx); };
        fxCB.resumeMotion = [this] (int fx) { if (effectOtomoProcessor) effectOtomoProcessor->resumeMotion (fx); };
        fxCB.stopAll      = [this]         { if (effectOtomoProcessor) effectOtomoProcessor->stopAllMotion(); };
        fxCB.sendsMove       = [this] (int dx, int dy) { juce::MessageManager::callAsync ([this, dx, dy] { if (effectsTab) effectsTab->sendsMove (dx, dy); }); };
        fxCB.sendsToggle     = [this]                  { juce::MessageManager::callAsync ([this] { if (effectsTab) effectsTab->sendsToggle(); }); };
        fxCB.sendsLevelDb    = [this]                  { return effectsTab ? effectsTab->sendsLevelDb() : 0.0f; };
        fxCB.sendsSetLevelDb = [this] (float db)       { juce::MessageManager::callAsync ([this, db] { if (effectsTab) effectsTab->sendsSetLevelDb (db); }); };
        fxCB.sendsSetAll     = [this] (bool on)        { juce::MessageManager::callAsync ([this, on] { if (effectsTab) effectsTab->sendsSetAll (on); }); };

        // The reverb's model decides which of its controls the Chain page
        // shows: when it moves - from the deck or from anywhere the GUI sees -
        // the page is laid out again, on the next message-loop turn rather
        // than from inside the dial or the panel that moved it.
        auto relayoutEffectsDeck = [this]
        {
            juce::MessageManager::callAsync ([this]
            {
                if (streamDeckManager && streamDeckManager->getCurrentMainTab() == EffectsTabPages::EFFECTS_MAIN_TAB_INDEX)
                    streamDeckManager->refreshCurrentPage();
            });
        };
        fxCB.onModuleLayoutChanged = relayoutEffectsDeck;
        effectsTab->onModuleLayoutChanged = relayoutEffectsDeck;

        for (int subTab : { 0, 1, 2, 3, 4 })
        {
            streamDeckManager->registerPage (
                EffectsTabPages::EFFECTS_MAIN_TAB_INDEX, subTab,
                EffectsTabPages::createPage (subTab, vts, parameters.getEffectEdit(), 0,
                    effectsSoloState, effectsEditOnMapState, effectsLfoSubMode, effectsChainSlot, effectsChainBank, fxCB));
        }

        // Wire EffectsTab GUI callbacks to the engine and the calculation mask.
        // Solo is a mask the calculation engine applies to the direct rows, so
        // it costs one atomic and never touches the audio thread; Clear reaches
        // the engine, which honours it at the next batch boundary. The two
        // toggles also land in the deck's shared state, so its buttons follow.
        effectsTab->onSoloEffectsChanged = [this, effectsSoloState] (bool active)
        {
            soloEffects.store (active, std::memory_order_relaxed);
            *effectsSoloState = active;
        };
        effectsTab->onClearRequested = [this] (int fx)
        {
            if (effectsHost != nullptr)
                effectsHost->requestClear (fx);
        };
        effectsTab->onMapEditChanged = [this, effectsEditOnMapState] (bool enabled)
        {
            if (mapTab)
                mapTab->setEffectEditMode (enabled);
            *effectsEditOnMapState = enabled;
        };
        effectsTab->onChainSlotSelected = [this, effectsChainSlot, effectsChainBank] (int slot)
        {
            if (*effectsChainSlot == slot)
                return;
            *effectsChainSlot = slot;
            *effectsChainBank = 0;          // another module: its first twelve
            if (streamDeckManager && streamDeckManager->getCurrentMainTab() == EffectsTabPages::EFFECTS_MAIN_TAB_INDEX)
                streamDeckManager->refreshCurrentPage();
        };
        effectsTab->onConfigReloaded = [this]()
        {
            handleChannelCountChange();
        };

        // Wire ReverbTab GUI callbacks to sync audio engine + shared state for StreamDeck
        reverbTab->onSoloReverbsChanged = [this, reverbSoloState, reverbMutePreState, reverbMutePostState] (bool active)
        {
            soloReverbs.store (active, std::memory_order_relaxed);
            *reverbSoloState = active;
            if (active) { *reverbMutePreState = false; *reverbMutePostState = false; }
        };
        reverbTab->onMutePreChanged = [this, reverbSoloState, reverbMutePreState, reverbMutePostState] (bool active)
        {
            muteReverbPre.store (active, std::memory_order_relaxed);
            *reverbMutePreState = active;
            if (active) { *reverbSoloState = false; *reverbMutePostState = false; }
        };
        reverbTab->onMutePostChanged = [this, reverbSoloState, reverbMutePreState, reverbMutePostState] (bool active)
        {
            muteReverbPost.store (active, std::memory_order_relaxed);
            *reverbMutePostState = active;
            if (active) { *reverbSoloState = false; *reverbMutePreState = false; }
        };
        reverbTab->onMapEditChanged = [this, reverbEditOnMapState] (bool enabled)
        {
            if (mapTab) mapTab->setReverbEditMode (enabled);
            *reverbEditOnMapState = enabled;
        };
        reverbTab->onAlgorithmChanged = [this, reverbAlgoSubMode]()
        {
            if (streamDeckManager && streamDeckManager->getCurrentMainTab() == ReverbTabPages::REVERB_MAIN_TAB_INDEX)
            {
                *reverbAlgoSubMode = 0;
                streamDeckManager->refreshCurrentPage();
            }
        };

        // Network tab callbacks (actions go through the GUI for proper logic)
        NetworkTabPages::NetworkCallbacks netCB;
        netCB.toggleOscFilter = [this]()
        {
            juce::MessageManager::callAsync ([this]()
            {
                if (networkTab) networkTab->toggleOscFilter();
            });
        };
        netCB.toggleTracking = [this]()
        {
            juce::MessageManager::callAsync ([this]()
            {
                if (networkTab) networkTab->toggleTracking();
            });
        };
        netCB.openLogWindow = [this]()
        {
            juce::MessageManager::callAsync ([this]()
            {
                openNetworkLogWindow();
            });
        };

        // Register Network tab page
        streamDeckManager->registerPage (
            NetworkTabPages::NETWORK_MAIN_TAB_INDEX, 0,
            NetworkTabPages::createPage (0, vts, netCB));

        // System Config tab callbacks
        SystemConfigTabPages::SysConfigCallbacks sysCB;
        sysCB.openAudioPatchWindow = [this]()
        {
            juce::MessageManager::callAsync ([this]()
            {
                openAudioInterfaceWindow();
            });
        };
        sysCB.startProcessing = [this]()
        {
            juce::MessageManager::callAsync ([this]()
            {
                if (systemConfigTab) systemConfigTab->requestStartProcessing();
            });
        };
        sysCB.startBinaural = [this]()
        {
            juce::MessageManager::callAsync ([this]()
            {
                if (systemConfigTab) systemConfigTab->requestStartBinaural();
            });
        };
        sysCB.setBrightness = [this] (int percent)
        {
            if (streamDeckManager)
                streamDeckManager->setBrightness (percent);
        };

        // Register System Config tab page
        streamDeckManager->registerPage (
            SystemConfigTabPages::SYSCONFIG_MAIN_TAB_INDEX, 0,
            SystemConfigTabPages::createPage (0, vts, sysCB));

        // Map tab callbacks and state queries
        auto mapPosOffsetMode = std::make_shared<bool> (false);

        MapTabPages::MapCallbacks mapCB;
        mapCB.toggleLevelOverlay = [this]()
        {
            juce::MessageManager::callAsync ([this]() { if (mapTab) mapTab->toggleLevelOverlay(); });
        };
        mapCB.fitStageToScreen = [this]()
        {
            juce::MessageManager::callAsync ([this]() { if (mapTab) mapTab->requestResetView(); });
        };
        mapCB.fitAllInputsToScreen = [this]()
        {
            juce::MessageManager::callAsync ([this]() { if (mapTab) mapTab->requestFitAllInputsToScreen(); });
        };
        mapCB.selectInput = [this](int idx)
        {
            juce::MessageManager::callAsync ([this, idx]() { if (mapTab) mapTab->selectInputProgrammatically (idx); });
        };
        mapCB.selectCluster = [this](int num)
        {
            juce::MessageManager::callAsync ([this, num]() { if (mapTab) mapTab->selectClusterProgrammatically (num); });
        };
        mapCB.moveClusterRef = [this](int c, float x, float y)
        {
            juce::MessageManager::callAsync ([this, c, x, y]() { if (mapTab) mapTab->moveClusterRefFromStreamDeck (c, x, y); });
        };
        mapCB.scaleCluster = [this](int c, float s)
        {
            juce::MessageManager::callAsync ([this, c, s]() { if (mapTab) mapTab->scaleClusterFromStreamDeck (c, s); });
        };
        mapCB.rotateCluster = [this](int c, float a)
        {
            juce::MessageManager::callAsync ([this, c, a]() { if (mapTab) mapTab->rotateClusterFromStreamDeck (c, a); });
        };
        mapCB.repaintMap = [this]()
        {
            juce::MessageManager::callAsync ([this]() { if (mapTab) mapTab->repaint(); });
        };
        mapCB.deselectAll = [this]()
        {
            juce::MessageManager::callAsync ([this]() { if (mapTab) mapTab->deselectAllProgrammatically(); });
        };
        mapCB.getViewCenterX = [this]() { return mapTab ? mapTab->getViewCenterX() : 0.0f; };
        mapCB.getViewCenterY = [this]() { return mapTab ? mapTab->getViewCenterY() : 0.0f; };
        mapCB.setViewCenterX = [this] (float x) { juce::MessageManager::callAsync ([this, x]() { if (mapTab) mapTab->setViewCenterX (x); }); };
        mapCB.setViewCenterY = [this] (float y) { juce::MessageManager::callAsync ([this, y]() { if (mapTab) mapTab->setViewCenterY (y); }); };
        mapCB.getViewScale   = [this]() { return mapTab ? mapTab->getViewScale() : 30.0f; };
        mapCB.setViewScale   = [this] (float s) { juce::MessageManager::callAsync ([this, s]() { if (mapTab) mapTab->setViewScale (s); }); };
        mapCB.moveSelectedDelta = [this] (float dx, float dy, float dz)
        {
            juce::MessageManager::callAsync ([this, dx, dy, dz]() { if (mapTab) mapTab->moveSelectedInputsDelta (dx, dy, dz); });
        };

        MapTabPages::MapStateQueries mapQ;
        mapQ.getSelectedInput       = [this]() { return mapTab ? mapTab->getSelectedInput() : -1; };
        mapQ.getSelectedCluster     = [this]() { return mapTab ? mapTab->getSelectedBarycenter() : -1; };
        mapQ.isDragging             = [this]() { return mapTab ? mapTab->getIsDragging() : false; };
        mapQ.getNumInputs           = [this]() { return parameters.getNumInputChannels(); };
        mapQ.getLevelOverlayEnabled = [this]() { return mapTab ? mapTab->getLevelOverlayEnabled() : false; };
        mapQ.getClusterRefPosition  = [this](int c)
        {
            return mapTab ? mapTab->getClusterRefPosition (c) : juce::Point<float> (0.0f, 0.0f);
        };
        mapQ.getMultiSelectionCount = [this]() { return mapTab ? mapTab->getMultiSelectionCount() : 0; };

        // Register Map tab page
        streamDeckManager->registerPage (
            MapTabPages::MAP_MAIN_TAB_INDEX, 0,
            MapTabPages::createPage (0, vts, mapCB, mapQ, mapPosOffsetMode));

        // Wire map selection changes to rebuild Stream Deck page and mirror the
        // new selection to connected tablets
        mapTab->setMapSelectionChangedCallback ([this]()
        {
            if (streamDeckManager && streamDeckManager->getCurrentMainTab() == MapTabPages::MAP_MAIN_TAB_INDEX)
                streamDeckManager->refreshCurrentPage();
            sendVisualisationToRemotes();
        });

        // Register Clusters tab page (LFO controls)
        auto clusterLfoSubMode = std::make_shared<int> (0);
        auto presetCol         = std::make_shared<int> (0);
        auto presetRow         = std::make_shared<int> (0);

        ClustersTabPages::ClusterLFOCallbacks clusterCB;
        clusterCB.stopAllClusterLFOs = [this]()
        {
            juce::MessageManager::callAsync ([this]() { if (clustersTab) clustersTab->sdStopAllClusterLFOs(); });
        };
        clusterCB.storePreset = [this] (int idx)
        {
            juce::MessageManager::callAsync ([this, idx]() { if (clustersTab) clustersTab->sdStorePreset (idx); });
        };
        clusterCB.recallPreset = [this] (int idx)
        {
            juce::MessageManager::callAsync ([this, idx]() { if (clustersTab) clustersTab->sdRecallPreset (idx); });
        };
        clusterCB.recallAndStart = [this] (int idx)
        {
            juce::MessageManager::callAsync ([this, idx]()
            {
                if (clustersTab)
                {
                    clustersTab->sdRecallPreset (idx);
                    clustersTab->sdActivateCurrentClusterLFO();
                }
            });
        };
        clusterCB.highlightPreset = [this] (int idx)
        {
            juce::MessageManager::callAsync ([this, idx]() { if (clustersTab) clustersTab->setHighlightedPresetTile (idx); });
        };

        streamDeckManager->registerPage (
            ClustersTabPages::CLUSTERS_MAIN_TAB_INDEX, 0,
            ClustersTabPages::createPage (0, vts, 1, clusterLfoSubMode, presetCol, presetRow, clusterCB));

        // Wire cluster selection to Stream Deck channel
        if (clustersTab)
        {
            clustersTab->onClusterSelected = [this] (int clusterNum)
            {
                if (streamDeckManager && streamDeckManager->getCurrentMainTab() == ClustersTabPages::CLUSTERS_MAIN_TAB_INDEX)
                    streamDeckManager->setChannel (clusterNum);
            };
        }

        // QLab cluster preset cue creation
        clustersTab->isQLabAvailable = [this]() {
            return oscManager && oscManager->hasQLabTarget();
        };

        clustersTab->onQLabPresetCueRequested = [this] (int clusterId, int presetNumber, const juce::String& presetName) {
            if (! oscManager || ! oscManager->hasQLabTarget()) return;
            int patchNumber = oscManager->getQLabPatchNumber();
            auto sequence = WFSNetwork::QLabCueBuilder::buildClusterLFOPresetCue (clusterId, presetNumber, presetName, patchNumber);
            oscManager->sendToQLab (sequence, [this, clusterId, presetName] (int) {
                if (clustersTab != nullptr && clustersTab->getStatusBar() != nullptr)
                    clustersTab->getStatusBar()->showTemporaryMessage (
                        LOC ("clusters.qlabPresetCueCreated")
                            .replace ("{cluster}", juce::String (clusterId))
                            .replace ("{name}", presetName), 3000);
            });
        };

        // Set page rebuild callback for channel changes and binding swaps
        streamDeckManager->onPageNeedsRebuild = [this, flipModeState, stereoParamsState, lfoSubModeState, movCB, inputSendsWindow, sendsCB, outputEqBandState, onEqBandSelectedGui, netCB, sysCB, mapCB, mapQ, mapPosOffsetMode, reverbPreEqBandState, reverbPreDynMode, reverbPostEqBandState, reverbPostDynMode, reverbSoloState, reverbMutePreState, reverbMutePostState, reverbEditOnMapState, reverbAlgoSubMode, reverbIRDuration, onSoloReverbSD, onMutePreSD, onMutePostSD, onEditOnMapSD, clusterLfoSubMode, presetCol, presetRow, clusterCB, effectsSoloState, effectsEditOnMapState, effectsLfoSubMode, effectsChainSlot, effectsChainBank, fxCB](int mainTab, int subTab, int channel)
        {
            if (mainTab == InputsTabPages::INPUTS_MAIN_TAB_INDEX)
            {
                if (subTab == 3)
                {
                    // Gradient Map subtab — use dedicated page
                    GradientMapPages::GradientMapCallbacks gmCB;
                    gmCB.getEditor = [this]() -> GradientMapEditor*
                    {
                        return inputsTab ? &inputsTab->getGradientMapEditor() : nullptr;
                    };
                    streamDeckManager->registerPage (mainTab, 3,
                        GradientMapPages::createGradientMapPage (gmCB));
                }
                else
                {
                    auto& vts = parameters.getValueTreeState();
                    // `channel` arrives as the permanent channel NUMBER (fed
                    // from InputsTab), while InputsTabPages indexes by SLOT;
                    // numbers have gaps and are not in slot order after a
                    // reorder. A dead number has no page to build.
                    const int inputSlot = vts.getSlotForChannelNumber (channel);
                    if (inputSlot < 0)
                        return;
                    streamDeckManager->registerPage (mainTab, subTab,
                        InputsTabPages::createPage (subTab, vts, parameters.getClusterEdit(), inputSlot, flipModeState, stereoParamsState, lfoSubModeState, movCB, inputSendsWindow, sendsCB));
                }
            }
            else if (mainTab == OutputsTabPages::OUTPUTS_MAIN_TAB_INDEX)
            {
                auto& vts = parameters.getValueTreeState();
                streamDeckManager->registerPage (mainTab, subTab,
                    OutputsTabPages::createPage (subTab, vts, parameters.getArrayEdit(), channel - 1, outputEqBandState, onEqBandSelectedGui));
            }
            else if (mainTab == NetworkTabPages::NETWORK_MAIN_TAB_INDEX)
            {
                auto& vts = parameters.getValueTreeState();
                streamDeckManager->registerPage (mainTab, subTab,
                    NetworkTabPages::createPage (subTab, vts, netCB));
            }
            else if (mainTab == SystemConfigTabPages::SYSCONFIG_MAIN_TAB_INDEX)
            {
                auto& vts = parameters.getValueTreeState();
                streamDeckManager->registerPage (mainTab, subTab,
                    SystemConfigTabPages::createPage (subTab, vts, sysCB));
            }
            else if (mainTab == MapTabPages::MAP_MAIN_TAB_INDEX)
            {
                auto& vts = parameters.getValueTreeState();
                streamDeckManager->registerPage (mainTab, subTab,
                    MapTabPages::createPage (subTab, vts, mapCB, mapQ, mapPosOffsetMode));
            }
            else if (mainTab == ReverbTabPages::REVERB_MAIN_TAB_INDEX)
            {
                auto& vts = parameters.getValueTreeState();
                streamDeckManager->registerPage (mainTab, subTab,
                    ReverbTabPages::createPage (subTab, vts, channel - 1,
                        reverbPreEqBandState, reverbPreDynMode,
                        reverbPostEqBandState, reverbPostDynMode,
                        reverbSoloState, reverbMutePreState,
                        reverbMutePostState, reverbEditOnMapState,
                        reverbAlgoSubMode, reverbIRDuration,
                        nullptr, nullptr,
                        onSoloReverbSD, onMutePreSD, onMutePostSD, onEditOnMapSD));
            }
            else if (mainTab == EffectsTabPages::EFFECTS_MAIN_TAB_INDEX)
            {
                auto& vts = parameters.getValueTreeState();
                streamDeckManager->registerPage (mainTab, subTab,
                    EffectsTabPages::createPage (subTab, vts, parameters.getEffectEdit(), channel - 1,
                        effectsSoloState, effectsEditOnMapState, effectsLfoSubMode, effectsChainSlot, effectsChainBank, fxCB));
            }
            else if (mainTab == ClustersTabPages::CLUSTERS_MAIN_TAB_INDEX)
            {
                auto& vts = parameters.getValueTreeState();
                streamDeckManager->registerPage (mainTab, subTab,
                    ClustersTabPages::createPage (subTab, vts, channel,
                        clusterLfoSubMode, presetCol, presetRow, clusterCB));
            }
        };

        // Allow Stream Deck buttons to switch the main tab (e.g., → Map)
        streamDeckManager->onRequestMainTabChange = [this](int tabIndex)
        {
            juce::MessageManager::callAsync ([this, tabIndex]()
            {
                tabbedComponent.setCurrentTabIndex (tabIndex);
            });
        };

        // Allow Stream Deck buttons to select an item (channel) after switching tab
        streamDeckManager->onRequestItemSelect = [this](int tabIndex, int itemIndex)
        {
            juce::MessageManager::callAsync ([this, tabIndex, itemIndex]()
            {
                // Output and reverb ids ARE dense slot positions, so + 1 is
                // their identity. The input case cannot share it: itemIndex is
                // a SLOT there and the selector holds permanent channel
                // NUMBERS, which have gaps and are not in slot order after a
                // reorder.
                int channel = itemIndex + 1;
                switch (tabIndex)
                {
                    case 4:
                        if (inputsTab)
                            inputsTab->selectChannel (parameters.getValueTreeState().getInputChannelNumber (itemIndex));
                        break;
                    case 2:  if (outputsTab)  outputsTab->selectChannel (channel);  break;
                    case 3:  if (reverbTab)   reverbTab->selectChannel (channel);   break;
                    default: break;
                }
            });
        };

        // Allow Stream Deck buttons to switch subtabs (e.g., → Output EQ)
        streamDeckManager->onRequestSubTabChange = [this](int subTabIndex)
        {
            juce::MessageManager::callAsync ([this, subTabIndex]()
            {
                int tab = tabbedComponent.getCurrentTabIndex();
                if (tab == 2 && outputsTab != nullptr)
                    outputsTab->setSubTabIndex (subTabIndex);
            });
        };
    }

    // Initialize Input Controller Manager (SpaceMouse, joystick, gamepad)
    {
        controllerManager = std::make_unique<ControllerManager>();

        // Wire callbacks to MapTab and InputsTab
        controllerManager->callbacks.moveSelectedDelta = [this] (float dx, float dy, float dz)
        {
            juce::MessageManager::callAsync ([this, dx, dy, dz]()
            {
                if (mapTab)
                    mapTab->moveSelectedInputsDelta (dx, dy, dz);
            });
        };

        // Inputs a Space Mouse twist / Shift gesture applies to. On the Inputs
        // tab that is the channel on screen and nothing else - the one
        // moveCurrentChannel moves - even with inputs still selected on the
        // map; everywhere else it is the map selection.
        auto resolveControllerTargets = [this]()
        {
            std::set<int> targets;
            if (tabbedComponent.getCurrentTabIndex() == TabIndex::Inputs)
            {
                if (inputsTab)
                {
                    int ch = inputsTab->getSelectedInputIndex();
                    if (ch >= 0 && ch < parameters.getNumInputChannels())
                        targets.insert (ch);
                }
            }
            else if (mapTab)
            {
                targets = mapTab->getSelectedInputSet();
            }
            return targets;
        };

        controllerManager->callbacks.rotateSelected = [this, resolveControllerTargets] (float deltaDeg)
        {
            juce::MessageManager::callAsync ([this, resolveControllerTargets, deltaDeg]()
            {
                const auto targets = resolveControllerTargets();

                for (int idx : targets)
                {
                    int current = static_cast<int> (parameters.getInputParam (idx, "inputRotation"));
                    int newRot = current + static_cast<int> (std::round (deltaDeg));
                    while (newRot > 180) newRot -= 360;
                    while (newRot < -179) newRot += 360;
                    parameters.setInputParam (idx, "inputRotation", newRot);
                }
                if (mapTab) mapTab->repaint();
            });
        };

        // Shift layer: push/pull widens or narrows the stereo image of every
        // stereo target. Mono targets are left alone - Shift is the stereo
        // layer, not a second name for height.
        controllerManager->callbacks.adjustStereoWidth = [this, resolveControllerTargets] (float deltaMetres)
        {
            juce::MessageManager::callAsync ([this, resolveControllerTargets, deltaMetres]()
            {
                auto& vts = parameters.getValueTreeState();
                bool changed = false;
                for (int idx : resolveControllerTargets())
                {
                    if (! vts.isInputChannelStereo (idx))
                        continue;
                    float current = static_cast<float> (parameters.getInputParam (idx, WFSParameterIDs::inputStereoWidth.toString()));
                    float next = juce::jlimit (WFSParameterDefaults::inputStereoWidthMin,
                                               WFSParameterDefaults::inputStereoWidthMax,
                                               current + deltaMetres);
                    parameters.setInputParam (idx, WFSParameterIDs::inputStereoWidth.toString(), next);
                    changed = true;
                }
                if (changed && mapTab) mapTab->repaint();
            });
        };

        // Shift layer: twist rotates the stereo image axis. The axis is an
        // integer in degrees but a 50 Hz tick at partial deflection is well
        // under 1 degree, so carry the sub-degree remainder between ticks
        // instead of rounding each one to zero.
        controllerManager->callbacks.adjustStereoAxis = [this, resolveControllerTargets] (float deltaDeg)
        {
            juce::MessageManager::callAsync ([this, resolveControllerTargets, deltaDeg]()
            {
                stereoAxisControllerAccum += deltaDeg;
                const int step = static_cast<int> (stereoAxisControllerAccum);  // truncates toward zero
                if (step == 0)
                    return;
                stereoAxisControllerAccum -= static_cast<float> (step);

                auto& vts = parameters.getValueTreeState();
                bool changed = false;
                for (int idx : resolveControllerTargets())
                {
                    if (! vts.isInputChannelStereo (idx))
                        continue;
                    int current = static_cast<int> (parameters.getInputParam (idx, WFSParameterIDs::inputStereoAxisOffset.toString()));
                    parameters.setInputParam (idx, WFSParameterIDs::inputStereoAxisOffset.toString(),
                                              WFSParameterDefaults::wrapAxisDegrees (current + step));
                    changed = true;
                }
                if (changed && mapTab) mapTab->repaint();
            });
        };

        controllerManager->callbacks.cycleInput = [this] (int delta)
        {
            juce::MessageManager::callAsync ([this, delta]()
            {
                if (mapTab == nullptr) return;
                int numInputs = parameters.getNumInputChannels();
                if (numInputs <= 0) return;

                auto& selected = mapTab->getSelectedInputSet();
                int current = selected.empty() ? 0 : *selected.begin();
                int next = current + delta;
                if (next >= numInputs) next = 0;
                else if (next < 0) next = numInputs - 1;

                mapTab->selectInputProgrammatically (next);

                // Also sync InputsTab channel selector. `next` is a SLOT (as
                // passed to selectInputProgrammatically above); the selector
                // holds permanent channel NUMBERS, which have gaps and are not
                // in slot order after a reorder.
                if (inputsTab)
                    inputsTab->selectChannel (parameters.getValueTreeState().getInputChannelNumber (next));
            });
        };

        controllerManager->callbacks.cycleChannel = [this] (int delta)
        {
            juce::MessageManager::callAsync ([this, delta]()
            {
                cycleChannel (delta);
            });
        };

        controllerManager->callbacks.cycleCluster = [this] (int delta)
        {
            juce::MessageManager::callAsync ([this, delta]()
            {
                if (clustersTab)
                {
                    if (delta > 0)
                        clustersTab->selectNextCluster();
                    else
                        clustersTab->selectPreviousCluster();
                }
            });
        };

        controllerManager->callbacks.moveCurrentChannel = [this] (float dx, float dy, float dz)
        {
            juce::MessageManager::callAsync ([this, dx, dy, dz]()
            {
                if (inputsTab == nullptr) return;
                int ch = inputsTab->getSelectedInputIndex();  // 0-based
                if (ch < 0 || ch >= parameters.getNumInputChannels()) return;
                mapTab->moveInputByDelta (ch, dx, dy, dz);
            });
        };

        controllerManager->callbacks.getSelectedClusterRef = [this]() -> int
        {
            if (mapTab == nullptr) return 0;

            // Check for selected barycenter first
            int bary = mapTab->getSelectedBarycenter();
            if (bary > 0)
                return bary;

            // Check for selected cluster reference input
            auto& selected = mapTab->getSelectedInputSet();
            if (selected.size() != 1) return 0;
            int idx = *selected.begin();
            int cluster = static_cast<int> (parameters.getInputParam (idx, "inputCluster"));
            if (cluster > 0 && mapTab->getClusterRef (cluster) == idx)
                return cluster;
            return 0;
        };

        controllerManager->callbacks.fitAllInputs = [this]()
        {
            if (mapTab)
                mapTab->requestFitAllInputsToScreen();
        };

        controllerManager->callbacks.fitStage = [this]()
        {
            if (mapTab)
                mapTab->requestResetView();
        };

        controllerManager->callbacks.panMap = [this] (float dx, float dy)
        {
            if (mapTab)
            {
                mapTab->setViewCenterX (mapTab->getViewCenterX() + dx * 3.0f);
                mapTab->setViewCenterY (mapTab->getViewCenterY() - dy * 3.0f);
            }
        };

        controllerManager->callbacks.zoomMap = [this] (float factor)
        {
            if (mapTab)
                mapTab->setViewScale (mapTab->getViewScale() * factor);
        };

        controllerManager->callbacks.axisDeflection = [this] (float x, float y, float z)
        {
            // Visual-only: show SpaceMouse deflection on the active tab's joystick
            int tab = tabbedComponent.getCurrentTabIndex();
            if (tab == 4 && inputsTab)
                inputsTab->setControllerDeflection (x, y, z);
            else if (tab == 5 && clustersTab)
                clustersTab->setControllerDeflection (x, y, z);
        };

        controllerManager->callbacks.getNumInputs = [this]()
        {
            return parameters.getNumInputChannels();
        };

        controllerManager->callbacks.getSelectedInputs = [this]() -> std::set<int>
        {
            if (mapTab)
                return mapTab->getSelectedInputSet();
            return {};
        };

        controllerManager->callbacks.repaintMap = [this]()
        {
            juce::MessageManager::callAsync ([this]()
            {
                if (mapTab) mapTab->repaint();
            });
        };

        // Cluster callbacks — resolve cluster from Clusters tab or Map tab selection
        auto getActiveCluster = [this]() -> int
        {
            int tab = tabbedComponent.getCurrentTabIndex();
            if (tab == 5)
                return clustersTab ? clustersTab->getSelectedCluster() : 0;

            // Map tab: check for barycenter or cluster reference input
            if (tab == 6 && mapTab)
            {
                // Barycenter selection
                int bary = mapTab->getSelectedBarycenter();
                if (bary > 0)
                    return bary;

                // Cluster reference input selection
                auto& selected = mapTab->getSelectedInputSet();
                if (selected.size() == 1)
                {
                    int idx = *selected.begin();
                    int cluster = static_cast<int> (parameters.getInputParam (idx, "inputCluster"));
                    if (cluster > 0 && mapTab->getClusterRef (cluster) == idx)
                        return cluster;
                }
            }
            return 0;
        };

        controllerManager->callbacks.moveClusterDelta = [this, getActiveCluster] (float dx, float dy, float dz)
        {
            juce::MessageManager::callAsync ([this, getActiveCluster, dx, dy, dz]()
            {
                int cluster = getActiveCluster();
                if (cluster > 0 && mapTab)
                    mapTab->moveClusterDelta (cluster, dx, dy, dz);
            });
        };

        controllerManager->callbacks.rotateCluster = [this, getActiveCluster] (float deltaDeg)
        {
            juce::MessageManager::callAsync ([this, getActiveCluster, deltaDeg]()
            {
                int cluster = getActiveCluster();
                if (cluster > 0 && mapTab)
                    mapTab->rotateClusterFromStreamDeck (cluster, deltaDeg);
            });
        };

        controllerManager->callbacks.scaleCluster = [this, getActiveCluster] (float scaleFactor)
        {
            juce::MessageManager::callAsync ([this, getActiveCluster, scaleFactor]()
            {
                int cluster = getActiveCluster();
                if (cluster > 0 && mapTab)
                    mapTab->scaleClusterFromStreamDeck (cluster, scaleFactor);
            });
        };

        // One undo step per push of the puck, as a map drag is one: the step
        // opens on the tick the push starts, before its writes are queued, in
        // the active tab's history - the tab the manager drives.
        controllerManager->callbacks.onEditGestureStart = [this]()
        {
            parameters.getValueTreeState().beginUndoTransaction ("Space Mouse");
        };

        // Add SpaceMouse device
        controllerManager->addDevice (std::make_unique<SpaceMouseDevice>());

        // Start velocity integration, then disable if Position Control is Off
        controllerManager->start();
        controllerManager->activeTab = tabbedComponent.getCurrentTabIndex();
        int pcDevice = static_cast<int> (parameters.getConfigParam ("PositionControlDevice"));
        if (pcDevice == 0)
            controllerManager->setEnabled (false);
    }

    // Initialize Lightpad Manager (ROLI Lightpad Blocks)
    {
        lightpadManager = std::make_unique<LightpadManager> (WFSParameterDefaults::lightpadSensitivityDefault);

        lightpadManager->callbacks.moveInputDelta = [this] (int inputIdx, float dx, float dy)
        {
            // When sampler is active on this channel, route to sampler engine (transient cell offset)
            if (samplerManager && samplerManager->isChannelActive (inputIdx))
            {
                samplerManager->updatePosition (inputIdx, dx, dy);
                return;
            }

            // Normal mode: move input position
            juce::MessageManager::callAsync ([this, inputIdx, dx, dy]()
            {
                if (mapTab)
                    mapTab->moveInputByDelta (inputIdx, dx, dy);
            });
        };

        lightpadManager->callbacks.applyPressure = [this] (int inputIdx, float pressure)
        {
            if (samplerManager == nullptr || ! samplerManager->isChannelActive (inputIdx))
                return;

            SamplerEngine::TouchEvent event;
            event.type = SamplerEngine::TouchEvent::Pressure;
            event.pressure = pressure;
            samplerManager->pushTouchEvent (inputIdx, event);
        };

        lightpadManager->callbacks.onTouchStart = [this] (int inputIdx, float pressure)
        {
            if (samplerManager == nullptr || ! samplerManager->isChannelActive (inputIdx))
                return;

            samplerManager->triggerNextCell (inputIdx, pressure);
        };

        lightpadManager->callbacks.onTouchEnd = [this] (int inputIdx)
        {
            if (samplerManager == nullptr || ! samplerManager->isChannelActive (inputIdx))
                return;

            samplerManager->releaseChannel (inputIdx);
        };

        // Start Lightpad only if controller mode is Lightpad
        bool lpSamplerOn = static_cast<bool> (parameters.getConfigParam ("SamplerEnabled"));
        int ctrlMode = static_cast<int> (parameters.getConfigParam ("SamplerControllerMode"));
        if (lpSamplerOn && ctrlMode == 1)
            lightpadManager->start();

        // Restore sensitivity from ValueTree
        {
            auto config = parameters.getValueTreeState().getConfigState();
            auto ui = config.getChildWithName (WFSParameterIDs::UI);
            if (ui.isValid())
            {
                float sens = static_cast<float> (ui.getProperty (
                    WFSParameterIDs::lightpadSensitivity, 0.05f));
                lightpadManager->setSensitivity (sens);

                // Restore split states (applied when topology is detected)
                bool splits[3] = {
                    static_cast<int> (ui.getProperty (WFSParameterIDs::lightpadPad0Split, 0)) != 0,
                    static_cast<int> (ui.getProperty (WFSParameterIDs::lightpadPad1Split, 0)) != 0,
                    static_cast<int> (ui.getProperty (WFSParameterIDs::lightpadPad2Split, 0)) != 0
                };
                for (int p = 0; p < 3; ++p)
                    lightpadManager->setPadSplit (p, splits[p]);
            }
        }

        // Restore saved zone-to-input assignments from ValueTree
        {
            auto inputs = parameters.getValueTreeState().getInputsState();
            for (int i = 0; i < inputs.getNumChildren(); ++i)
            {
                auto ch = inputs.getChild (i);
                auto channelSection = ch.getChildWithName (WFSParameterIDs::Channel);
                if (channelSection.isValid())
                {
                    int zoneId = static_cast<int> (channelSection.getProperty (
                        WFSParameterIDs::lightpadZoneId, -1));
                    if (zoneId >= 0)
                        lightpadManager->assignZoneToInput (zoneId, i);
                }
            }
        }

        // Wire topology change notification to SystemConfigTab mini-map
        lightpadManager->onTopologyChanged = [this] (const std::vector<PadLayoutInfo>& pads)
        {
            if (systemConfigTab)
                systemConfigTab->updateLightpadLayout (pads);
        };

        // Wire LightpadZoneQuery into SamplerSubTab via InputsTab
        if (inputsTab)
        {
            SamplerSubTab::LightpadZoneQuery zoneQuery;
            zoneQuery.getAllZones = [this]() {
                return lightpadManager ? lightpadManager->getAllZonesWithNames()
                                       : std::vector<std::pair<int, juce::String>>();
            };
            zoneQuery.getAssignedZoneIds = [this]() {
                return lightpadManager ? lightpadManager->getAssignedZoneIds()
                                       : std::set<int>();
            };
            zoneQuery.getAssignedZones = [this]() {
                return lightpadManager ? lightpadManager->getAssignedZonesMap()
                                       : std::map<int, int>();
            };
            zoneQuery.showZoneNumbers = [this](bool show) {
                if (lightpadManager)
                    lightpadManager->showZoneNumbersOnLeds (show);
            };
            zoneQuery.getPadLayouts = [this]() {
                return lightpadManager ? lightpadManager->getPadLayouts()
                                       : std::vector<PadLayoutInfo>();
            };
            inputsTab->setLightpadZoneQuery (std::move (zoneQuery));
            inputsTab->setLightpadZoneChangedCallback ([this](int inputIndex, int zoneId) {
                if (lightpadManager)
                    lightpadManager->assignZoneToInput (zoneId, inputIndex);
                // Re-send pad config to remote with updated zone assignments
                resendRemotePadConfig();
            });
            // Set controller mode based on saved config
            if (lpSamplerOn)
                inputsTab->getSamplerSubTab().setControllerMode (ctrlMode);
            else
                inputsTab->getSamplerSubTab().setControllerMode (0);
        }
    }

    // Initialize WFS Calculation Engine for DSP parameter generation
    calculationEngine = std::make_unique<WFSCalculationEngine>(parameters.getValueTreeState());

    // Constructed here rather than in prepareToPlay() because the stereo image
    // the Map draws is read back from the slice states this owns: an operator
    // plotting a show with no interface attached — first launch, or the
    // "no audio device is running" path — never reaches prepareToPlay(), and a
    // null manager would leave every pair drawn with no spread bar and no
    // explanation. prepareToPlay() still prepares it once a device opens.
    stereoChannelManager = std::make_unique<StereoChannelManager>();

    // Initialize Binaural Solo Monitoring
    binauralCalcEngine = std::make_unique<BinauralCalculationEngine>(
        parameters.getValueTreeState(), *calculationEngine);
    binauralProcessor = std::make_unique<BinauralProcessor>(*binauralCalcEngine);
    binauralProcessor->setWorkgroupCoordinator(&workgroupCoordinator);
    headTrackerManager = std::make_unique<HeadTrackerManager>();
    if (systemConfigTab != nullptr)
        systemConfigTab->setHeadTrackerListProvider([this]()
        {
            std::vector<std::pair<juce::String, juce::String>> list;
            if (headTrackerManager != nullptr)
                for (const auto& s : headTrackerManager->enumerateSources())
                    list.emplace_back(s.id, s.displayName);
            return list;
        });

    if (systemConfigTab != nullptr)
        systemConfigTab->setHeadTrackerSetZeroCallback([this]()
        {
            if (headTrackerManager != nullptr)
                headTrackerManager->setZeroOnActiveSource();
        });

    if (systemConfigTab != nullptr)
        systemConfigTab->setHeadTrackerAttitudeProvider([this](float& yaw, float& pitch, float& roll)
        {
            if (headTrackerManager == nullptr)
                return false;
            auto* source = headTrackerManager->getActiveSource();
            if (source == nullptr)
                return false;
            const auto o = source->getOrientation();   // POD snapshot copy, message-thread safe
            yaw = o.yawRad; pitch = o.pitchRad; roll = o.rollRad;
            return o.valid;
        });

    // Initialize Reverb Engine
    reverbEngine = std::make_unique<ReverbEngine>();
    reverbEngine->setWorkgroupCoordinator(&workgroupCoordinator);

    // The effects engine host. Prepared by setupSharedInputFeed once the rings
    // exist and effect channels do (an empty session runs no engine), released
    // wherever the rings are cleared. The trace hook is a diagnostic: with
    // WFS_EFFECTS_TRACE set, the engine's telemetry is logged once per second,
    // which is how an audio check reads without a GUI.
    effectsHost = std::make_unique<EffectsHost>(parameters.getValueTreeState());
    effectsHost->setWorkgroupCoordinator(&workgroupCoordinator);
    effectsTraceEnabled = std::getenv("WFS_EFFECTS_TRACE") != nullptr;

    // Initialize LFO Processor for input position modulation
    lfoProcessor = std::make_unique<LFOProcessor>(parameters.getValueTreeState(), 64);

    // Initialize AutomOtion Processor for programmed input position movement
    automOtionProcessor = std::make_unique<AutomOtionProcessor>(parameters.getValueTreeState(), 64);
    automOtionProcessor->setDirtyTracker(&parameters.getDirtyTracker());

    // The same processor over the effect returns, in offset mode: an effect's
    // authored position is where the operator put that room in the show, so a
    // movement travels as an offset the calculation engine adds and the
    // position itself is never written. No dirty tracker for the same reason -
    // this instance does not write the tree at all.
    effectOtomoProcessor = std::make_unique<AutomOtionProcessor> (
        parameters.getValueTreeState(),
        AutomOtionFamily::effects (parameters.getValueTreeState(),
                                   [this] (int fx, float x, float y, float z)
                                   {
                                       if (calculationEngine != nullptr)
                                           calculationEngine->setEffectOtomoOffset (fx, x, y, z);
                                   }));

    // The LFO's effects twin: the same waveform engine over effectLFO*, no
    // gyrophone, published per tick to the engine's second offset slot.
    effectLfoProcessor = std::make_unique<LFOProcessor> (
        LFOFamily::effects (parameters.getValueTreeState()));

    // Initialize Input Speed Limiter for smooth position movement
    speedLimiter = std::make_unique<InputSpeedLimiter>();
    speedLimiter->resize(WFSParameterDefaults::maxInputChannels);

    // Pass AutomOtionProcessor to InputsTab for UI control
    if (inputsTab != nullptr)
        inputsTab->setAutoMotionProcessor(automOtionProcessor.get());
    if (effectsTab != nullptr)
        effectsTab->setOtomoProcessor(effectOtomoProcessor.get());

    // Initialize Live Source Tamer engine for per-speaker gain reduction.
    // Row dimension is maxRenderSources, NOT maxInputChannels: lsGains is indexed
    // with the calculation engine's matrixIdx, whose rows cover derived stereo
    // slice sources and effect returns too. Sizing this smaller than the
    // engine's matrix is an out-of-bounds read on the 50 Hz path. The return
    // rows have no Live Source section, so they read lsActive = false and stay
    // at unity - 32 no-op lookups per tick, paid for the bounds guarantee.
    lsTamerEngine = std::make_unique<LiveSourceTamerEngine>(
        parameters.getValueTreeState(),
        *calculationEngine,
        WFSParameterDefaults::maxRenderSources,
        WFSParameterDefaults::maxOutputChannels);

    // Initialize Test Signal Generator for audio interface testing
    testSignalGenerator = std::make_unique<TestSignalGenerator>();

    // Set up LFO offset callback for MapTab visualization
    if (mapTab != nullptr)
    {
        mapTab->setLFOOffsetCallback([this](int inputIndex, float& x, float& y, float& z) {
            if (calculationEngine != nullptr)
            {
                auto offset = calculationEngine->getLFOOffset(inputIndex);
                x = offset.x;
                y = offset.y;
                z = offset.z;
            }
        });

        // The effects twin. An effect return's AutomOtion and LFO travel as
        // offsets the engine adds and the tree never carries, so the Map's grey
        // dot can only come from here - and it shows the SUM, which is where
        // the return renders.
        mapTab->setEffectOtomoOffsetCallback([this](int effectIndex, float& x, float& y, float& z) {
            if (calculationEngine != nullptr)
            {
                auto offset = calculationEngine->getEffectMovementOffset(effectIndex);
                x = offset.x;
                y = offset.y;
                z = offset.z;
            }
        });

        // Set up speed-limited position callback for MapTab visualization
        mapTab->setSpeedLimitedPositionCallback([this](int inputIndex, float& x, float& y, float& z) {
            if (speedLimiter != nullptr)
            {
                speedLimiter->getPosition(inputIndex, x, y, z);
            }
        });

        // Sampler-playing state + pad XY offset for the compound marker.
        mapTab->setSamplerStateCallback([this](int inputIndex, float& offX, float& offY) -> bool {
            offX = 0.0f;
            offY = 0.0f;
            if (samplerManager == nullptr)
                return false;
            if (! samplerManager->isChannelPlaying(inputIndex))
                return false;
            float sx = 0.0f, sy = 0.0f, sz = 0.0f;
            samplerManager->getPositionOverride(inputIndex, sx, sy, sz);
            offX = sx;
            offY = sy;
            return true;
        });

        // Stereo image legs for the pair marker. Handing back the cache the
        // geometry refresh filled — rather than recomputing from the width and
        // the anchor here — is what guarantees the drawn image is the one being
        // rendered; false for every mono channel and until the first refresh.
        mapTab->setStereoImageCallback([this](int inputSlot,
                                              juce::Point<float>& outLeft,
                                              juce::Point<float>& outRight) -> bool {
            if (inputSlot < 0 || inputSlot >= (int) stereoImageLegs.size())
                return false;

            const auto& legs = stereoImageLegs[static_cast<size_t>(inputSlot)];
            if (! legs.valid)
                return false;

            outLeft = legs.left;
            outRight = legs.right;
            return true;
        });

        // Set up path mode waypoint capture callbacks
        mapTab->setDragStartCallback([this](int inputIndex) {
            if (speedLimiter == nullptr)
                return;

            // Only start recording if BOTH speed limiting AND path mode are active
            auto& vts = parameters.getValueTreeState();
            auto posSection = vts.getInputPositionSection(inputIndex);
            bool maxSpeedActive = static_cast<int>(posSection.getProperty(WFSParameterIDs::inputMaxSpeedActive, 0)) != 0;
            bool pathModeActive = static_cast<int>(posSection.getProperty(WFSParameterIDs::inputPathModeActive, 0)) != 0;

            if (maxSpeedActive && pathModeActive)
                speedLimiter->startRecording(inputIndex);
        });

        mapTab->setDragEndCallback([this](int inputIndex) {
            if (speedLimiter != nullptr)
                speedLimiter->stopRecording(inputIndex);
        });

        mapTab->setWaypointCaptureCallback([this](int inputIndex, float x, float y, float z) {
            if (speedLimiter != nullptr)
                speedLimiter->addWaypoint(inputIndex, x, y, z);
        });
    }

    // Initialize level metering manager
    levelMeteringManager = std::make_unique<LevelMeteringManager>(
        parameters.getNumInputChannels(),
        parameters.getNumOutputChannels());
    levelMeteringManager->setAlgorithms(&inputAlgorithm, &outputAlgorithm);
#if WFS_GPU_NATIVE
    levelMeteringManager->setGpuAlgorithms(&nativeGpuAlgorithm, &nativeGpuOutputAlgorithm);
#endif

    // Set up MapTab level overlay callbacks
    mapTab->setLevelOverlayChangedCallback([this](bool enabled) {
        if (levelMeteringManager)
            levelMeteringManager->setMapOverlayEnabled(enabled);
    });

    mapTab->setInputLevelCallback([this](int inputIndex) -> float {
        if (levelMeteringManager)
            return levelMeteringManager->getInputLevel(inputIndex).peakDb;
        return -200.0f;
    });

    mapTab->setOutputLevelCallback([this](int outputIndex) -> float {
        if (levelMeteringManager)
            return levelMeteringManager->getOutputLevel(outputIndex).peakDb;
        return -200.0f;
    });

    // Binaural listener glyph: the seat comes from the ValueTree, but the head's
    // facing needs the TRACKED yaw, which only exists on the RT fast path.
    mapTab->setBinauralTrackedAttitudeProvider([this](float& yaw, float& pitch, float& roll) {
        if (headTrackerManager == nullptr)
            return false;
        auto* source = headTrackerManager->getActiveSource();
        if (source == nullptr)
            return false;
        const auto o = source->getOrientation();
        if (! o.valid || ! spatcore::binaural::isFiniteAttitude(o))
            return false;
        yaw = o.yawRad;
        pitch = o.pitchRad;
        roll = o.rollRad;
        return true;
    });

    // Configure OSC Manager with initial network settings from parameters
    WFSNetwork::GlobalConfig oscGlobalConfig;
    oscGlobalConfig.udpReceivePort = (int)parameters.getConfigParam("NetworkRxUDPport");
    oscGlobalConfig.tcpReceivePort = (int)parameters.getConfigParam("NetworkRxTCPport");
    oscManager->applyGlobalConfig(oscGlobalConfig);

    // Pass OSCManager to NetworkTab for UI integration
    networkTab->setOSCManager(oscManager.get());

    // Pass MCPServer too — the NetworkTab MCP section needs it to read the
    // bound port + running status for the Copy URL button.
    networkTab->setMCPServer(mcpServer.get());

    // Set up NetworkLogWindow callback
    networkTab->setNetworkLogWindowCallback([this]() {
        openNetworkLogWindow();
    });

    // Set up MCP AI History Window callback (Phase 5d)
    networkTab->setMCPHistoryWindowCallback([this]() {
        openMCPHistoryWindow();
    });

    // Query callback: check if AutomOtion is actively moving an input
    inputsTab->isAutoMotionActive = [this] (int inputIndex) -> bool
    {
        return automOtionProcessor && automOtionProcessor->isMotionActive (inputIndex);
    };
    mapTab->isAutoMotionActive = [this] (int inputIndex) -> bool
    {
        return automOtionProcessor && automOtionProcessor->isMotionActive (inputIndex);
    };

    // Connect InputsTab channel selection to OSCManager and StreamDeck
    inputsTab->onChannelSelected = [this](int channelId)
    {
        if (oscManager)
            oscManager->setRemoteSelectedChannel(channelId);
        if (streamDeckManager)
            streamDeckManager->setChannel(channelId);
        sendVisualisationToRemotes();
    };

    // Activate/deactivate sampler channel in the audio engine
    // The two ctor-time recomputes ran before the tabs existed — publish the
    // current render-source total now that SystemConfigTab is wired
    systemConfigTab->setRenderSourceTotal (numInputChannels, numRenderSources);

    inputsTab->onSamplerActiveChanged = [this] (int channelIndex, bool active)
    {
        if (samplerManager != nullptr)
        {
            samplerManager->setChannelActive (channelIndex, active);

            // Load cells + set when activating
            if (active)
            {
                auto samplerTree = parameters.getValueTreeState().getInputSamplerSection (channelIndex);
                if (samplerTree.isValid())
                {
                    auto samplesFolder = parameters.getFileManager().getSamplesFolder();
                    samplerManager->loadChannelCells (channelIndex, samplerTree, samplesFolder);
                    int setIdx = inputsTab->getSamplerSubTab().getActiveSetIndex();
                    samplerManager->loadChannelSetFromTree (channelIndex, samplerTree, setIdx);
                    applySamplerSetPosition (channelIndex, samplerTree, setIdx);
                }
            }
        }
    };

    // Connect InputsTab subtab selection to StreamDeck
    inputsTab->onSubTabChanged = [this](int subTabIndex)
    {
        if (streamDeckManager)
            streamDeckManager->setSubTab (subTabIndex);
    };

    // A channel retyped mono <-> stereo in place: setChannel() early-returns when
    // the selected number has not moved, so nothing else rebuilds the Stream Deck
    // page -- and its Inputs > Parameters dials bind different parameters either
    // side of the split. Guarded three ways because inputChannelType is written one
    // channel at a time in a loop (applyStereoSplit, the inventory apply path): only
    // the channel actually on the surface refreshes, and never while the Patch
    // window owns it, since refreshCurrentPage() short-circuits to the override page.
    // The patch matrix colours its rows through a provider evaluated at paint time, but its
    // own ValueTree listener only branches on the patch data, so a colour change reaches it
    // with nothing to trigger a repaint. Everything else that shows an input colour either
    // coalesces its own repaint (the map) or is rebuilt on open (the selector tiles).
    inputsTab->onInputColourChanged = [this]()
    {
        if (audioInterfaceWindow == nullptr)
            return;   // window is created on demand; nothing to repaint until it exists

        if (auto* content = audioInterfaceWindow->getContent())
            if (auto* inTab = content->getInputPatchTab())
                if (auto* matrix = inTab->getPatchMatrix())
                    matrix->repaint();
    };

    inputsTab->onChannelTypeChanged = [this](int changedChannelNumber)
    {
        if (streamDeckManager
            && ! streamDeckManager->hasOverride()
            && streamDeckManager->getCurrentMainTab() == InputsTabPages::INPUTS_MAIN_TAB_INDEX
            && streamDeckManager->getChannel() == changedChannelNumber)
            streamDeckManager->refreshCurrentPage();
    };

    // Sync StreamDeck to InputsTab's initial channel (1-indexed)
    if (streamDeckManager)
        streamDeckManager->setChannel (1);

    // Connect OutputsTab channel and subtab selection to StreamDeck
    outputsTab->onChannelSelected = [this](int channelId)
    {
        if (streamDeckManager && tabbedComponent.getCurrentTabIndex() == TabIndex::Outputs)
            streamDeckManager->setChannel (channelId);
    };

    outputsTab->onSubTabChanged = [this](int subTabIndex)
    {
        if (streamDeckManager && tabbedComponent.getCurrentTabIndex() == TabIndex::Outputs)
            streamDeckManager->setSubTab (subTabIndex);
    };

    // Connect ReverbTab channel and subtab selection to StreamDeck
    reverbTab->onChannelSelected = [this](int channelId)
    {
        if (streamDeckManager && tabbedComponent.getCurrentTabIndex() == TabIndex::Reverb)
            streamDeckManager->setChannel (channelId);
    };

    reverbTab->onSubTabChanged = [this](int subTabIndex)
    {
        if (streamDeckManager && tabbedComponent.getCurrentTabIndex() == TabIndex::Reverb)
            streamDeckManager->setSubTab (subTabIndex);
    };

    // The Effects tab, likewise
    effectsTab->onChannelSelected = [this](int channelId)
    {
        if (streamDeckManager && tabbedComponent.getCurrentTabIndex() == TabIndex::Effects)
            streamDeckManager->setChannel (channelId);
    };

    effectsTab->onSubTabChanged = [this](int subTabIndex)
    {
        if (streamDeckManager && tabbedComponent.getCurrentTabIndex() == TabIndex::Effects)
            streamDeckManager->setSubTab (subTabIndex);
    };

    // MIDI note -> snapshot recall. Created before the snapshot OSC callbacks so
    // both external trigger paths are wired next to each other.
    midiSnapshotTrigger = std::make_unique<MidiSnapshotTrigger>();

    // Wired BEFORE the first index build below, so a collision already on
    // disk at launch is reported instead of being resolved in silence.
    midiSnapshotTrigger->onDuplicateBinding =
        [this] (int ch, int note, const juce::String& winner, const juce::String& loser)
    {
        const auto msg = LOC("inputs.messages.midiBindingConflict")
                            .replace ("{ch}",    juce::String (ch))
                            .replace ("{note}",  juce::String (note))
                            .replace ("{first}", winner)
                            .replace ("{other}", loser);

        WFSLogger::getInstance().logWarning (msg);

        if (statusBar != nullptr)
            statusBar->showTemporaryMessage (msg, 5000);
    };

    midiSnapshotTrigger->onPortStateChanged =
        [this] (MidiSnapshotTrigger::PortState previous, MidiSnapshotTrigger::PortState current)
    {
        reportMidiPortState (previous, current);
    };

    // The trigger tried its port in its constructor, before anything listened.
    reportMidiPortState (MidiSnapshotTrigger::PortState::off, midiSnapshotTrigger->getPortState());

    // REQUIRED, not belt-and-braces: FileManager::onProjectFolderChanged fires
    // during the project restore earlier in this constructor, before the trigger
    // exists, and handleConfigReloaded() is never called on a plain launch (only
    // openProjectFromFile calls it). Without this the index stays empty until the
    // operator re-picks the project folder or stores a snapshot.
    refreshMidiSnapshotBindings();

    // An accepted channel-count write over OSC (/wfs/config/effectChannels, the
    // only one there is) reconfigures through the SAME funnel as the System
    // Config editor and the MCP lifecycle tools. OSCManager refuses the write
    // outright while processing runs, so this only ever fires from a stopped
    // engine.
    oscManager->onChannelTopologyChanged = [this] {
        handleChannelCountChange();
    };

    // The Effects tab verbs. Selection and the map toggle are GUI state, so
    // they go through the tab's external setters on the message thread; Clear
    // goes to the engine, which honours it at the next batch boundary.
    oscManager->onEffectSelected = [this] (int effectId) {
        juce::MessageManager::callAsync ([this, effectId] {
            if (effectsTab == nullptr) return;
            effectsTab->selectChannel (effectId);
            if (streamDeckManager && tabbedComponent.getCurrentTabIndex() == TabIndex::Effects)
                streamDeckManager->setChannel (effectsTab->getCurrentChannel());
        });
    };
    oscManager->onEffectEditOnMap = [this] (bool enabled) {
        juce::MessageManager::callAsync ([this, enabled] {
            if (effectsTab) effectsTab->setEditOnMapFromExternal (enabled);
            if (mapTab) mapTab->setEffectEditMode (enabled);
        });
    };
    oscManager->onEffectClear = [this] (int effectIdOrMinusOne) {
        if (effectsHost != nullptr)
            effectsHost->requestClear (effectIdOrMinusOne > 0 ? effectIdOrMinusOne - 1 : -1);
    };

    // Snapshot OSC command callbacks
    // Both external trigger paths and the Inputs long-press funnel through the
    // one seam, so the recall logic cannot drift into three copies again.
    oscManager->onSnapshotLoadRequested = [this](const juce::String& snapshotName) {
        recallSnapshotByName (snapshotName, /*fromMidi*/ false, /*fromOsc*/ true);
    };

    oscManager->onSnapshotStoreRequested = [this](const juce::String& snapshotName) {
        auto& fileManager = parameters.getFileManager();
        if (!fileManager.hasValidProjectFolder())
        {
            DBG ("OSC snapshot/store: no project folder configured");
            return;
        }

        auto scope = fileManager.getExtendedSnapshotScope (snapshotName);

        if (fileManager.saveInputSnapshotWithExtendedScope (snapshotName, scope))
        {
            parameters.getDirtyTracker().clearAll();

            if (inputsTab != nullptr)
            {
                snapshotSession->refreshList();
                inputsTab->showStatusMessage (
                    LOC("inputs.messages.snapshotUpdated").replace ("{name}", snapshotName));
            }

            // An OSC-stored snapshot carries whatever binding getExtendedSnapshotScope
            // just read back from the file, so the index may have changed.
            refreshMidiSnapshotBindings();
        }
        else
        {
            DBG ("OSC snapshot/store: failed to save: " << fileManager.getLastError());
        }
    };

    // Wire dirty tracker source detection delegate
    parameters.getDirtyTracker().getIncomingProtocol = [this]() -> WFSNetwork::Protocol {
        return oscManager ? oscManager->getIncomingProtocol()
                          : WFSNetwork::Protocol::Disabled;
    };

    // Connect remote position updates to map repaint
    oscManager->onRemotePositionReceived = [this]()
    {
        if (mapTab != nullptr)
            mapTab->repaint();
    };

    // Connect remote/OSC position updates to path mode waypoint capture
    oscManager->onRemoteWaypointCapture = [this](int channelIndex, float x, float y, float z)
    {
        if (speedLimiter == nullptr)
            return;

        // Check if max speed AND path mode are enabled for this channel
        auto& vts = parameters.getValueTreeState();
        juce::var maxSpeedActiveVar = vts.getInputParameter(channelIndex, WFSParameterIDs::inputMaxSpeedActive);
        juce::var pathModeActiveVar = vts.getInputParameter(channelIndex, WFSParameterIDs::inputPathModeActive);

        // NOT isInt()-guarded: after any load these are STRING vars, so both
        // read false and remote/OSC path-mode waypoint capture was dead until
        // the operator toggled one of the two controls by hand.
        bool maxSpeedActive = WFSVar::toBool (maxSpeedActiveVar);
        bool pathModeActive = WFSVar::toBool (pathModeActiveVar);

        if (maxSpeedActive && pathModeActive)
        {
            // Auto-start recording if not already recording
            if (!speedLimiter->isRecording(channelIndex))
            {
                speedLimiter->startRecording(channelIndex);
            }

            // Add waypoint (rate-limited internally by speedLimiter)
            speedLimiter->addWaypoint(channelIndex, x, y, z);

            // Track timestamp for auto-stop (stored per channel)
            remoteWaypointTimestamps[channelIndex] = juce::Time::currentTimeMillis();
        }
    };

    // Connect remote position XY updates to composite delta tracking
    // This prevents the "back and forth" movement of the grey dot when speed limiting is active
    oscManager->onRemotePositionXYUpdated = [this](int channelIndex, float targetX, float targetY)
    {
        if (calculationEngine == nullptr)
            return;

        // Get the current composite position (what the DSP is actually using)
        auto compositePos = calculationEngine->getCompositeInputPosition(channelIndex);

        // Compute the delta that will be calculated on the next timer tick
        // By pre-storing this, we prevent the delta from appearing to "change"
        float deltaX = compositePos.x - targetX;
        float deltaY = compositePos.y - targetY;

        // Store in lastSentCompositeDeltas so the timer tick won't see it as a change
        lastSentCompositeDeltas[channelIndex] = std::make_pair(deltaX, deltaY);
    };

    // Handle remote pad touch events from Android app
    oscManager->onRemotePadTouch = [this] (int zoneId, int touchState, float dx, float dy, float pressure)
    {
        // Dropped touches are logged on DOWN only (MOVE would spam the log)
        const bool logDrop = (touchState == 1);

        // Check controller mode is Remote
        int ctrlMode = static_cast<int> (parameters.getConfigParam ("SamplerControllerMode"));
        if (ctrlMode != 2)
        {
            if (logDrop)
                WFSLogger::getInstance().logWarning ("Sampler: remote pad touch ignored - controller mode is "
                                                     + juce::String (ctrlMode) + ", not Remote (2)");
            return;
        }

        // Look up zone → input
        auto zoneMap = buildZoneToInputMap();
        auto it = zoneMap.find (zoneId);
        if (it == zoneMap.end())
        {
            if (logDrop)
                WFSLogger::getInstance().logWarning ("Sampler: remote pad touch ignored - zone "
                                                     + juce::String (zoneId) + " not assigned to any input");
            return;
        }
        int inputIdx = it->second;

        if (samplerManager == nullptr || ! samplerManager->isChannelActive (inputIdx))
        {
            if (logDrop)
                WFSLogger::getInstance().logWarning ("Sampler: remote pad touch ignored - sampler not active on input "
                                                     + juce::String (inputIdx + 1));
            return;
        }

        float sensitivity = static_cast<float> (parameters.getConfigParam ("lightpadSensitivity"));
        if (sensitivity <= 0.0f) sensitivity = 0.05f;

        if (touchState == 1)  // DOWN
        {
            samplerManager->triggerNextCell (inputIdx, pressure);
        }
        else if (touchState == 2)  // MOVE
        {
            samplerManager->updatePosition (inputIdx, dx * sensitivity, -dy * sensitivity);
            SamplerEngine::TouchEvent evt;
            evt.type = SamplerEngine::TouchEvent::Pressure;
            evt.pressure = pressure;
            samplerManager->pushTouchEvent (inputIdx, evt);
        }
        else if (touchState == 0)  // UP
        {
            samplerManager->releaseChannel (inputIdx);
        }
    };

    // Answer tablet /remote/vis/pin requests with that channel's rows, sent to
    // that tablet only. View-only: desktop selection is never touched.
    oscManager->onRemoteVisPinRequested = [this](int targetIndex, int channelId)
    {
        if (calculationEngine == nullptr || oscManager == nullptr)
            return;
        // A tablet naming a channel BY NUMBER is an external reference to that
        // number, so latch before the number is resolved — including when the
        // number turns out to be dead below, because the tablet is addressing
        // this list by number either way. OSCManager fires this through
        // callAsync, so the tree write here is already on the message thread.
        parameters.getValueTreeState().markChannelNumbersUserOwned ("Remote visualisation pin request");
        // channelId is a permanent channel number — dead numbers are dropped
        if (parameters.getValueTreeState().getSlotForChannelNumber(channelId) < 0)
            return;

        oscManager->sendRemoteVisRows(channelId,
                                      calculationEngine->getDelayTimesMs(),
                                      calculationEngine->getLevels(),
                                      calculationEngine->getNumOutputs(),
                                      parameters.getNumOutputChannels(),
                                      calculationEngine->getInputReverbDelayTimesMs(),
                                      calculationEngine->getInputReverbLevels(),
                                      calculationEngine->getNumReverbs(),
                                      parameters.getNumReverbChannels(),
                                      targetIndex);
    };

    // Answer tablet /remote/vis/request (and the refresh that follows a full
    // state dump) with the whole vis state, to that tablet only. View-only,
    // like the pin: desktop selection is never touched.
    oscManager->onRemoteVisRefreshRequested = [this](int targetIndex, int restatedPin)
    {
        if (calculationEngine == nullptr || oscManager == nullptr)
            return;
        // A restated pin names a channel BY NUMBER — the same external
        // reference as a /remote/vis/pin, so the same latch. 0 (none) and -1
        // (not restated) name nothing.
        if (restatedPin > 0)
            parameters.getValueTreeState().markChannelNumbersUserOwned ("Remote visualisation request");
        // Config + outputArrays + selection + rows, plus this tablet's pinned
        // rows — which is how a restated pin gets answered.
        sendVisualisationToRemotes(targetIndex);
    };

    // Send composite deltas for all inputs when a Remote client connects and initial data has been sent
    oscManager->onRemoteConnectionReady = [this](int targetIndex)
    {
        if (calculationEngine == nullptr || oscManager == nullptr)
            return;

        // Initial /remote/vis/* state for the tablet that just connected
        // (sendVisualisationToRemotes carries config + selection + rows).
        sendVisualisationToRemotes(targetIndex);

        // Get number of input channels
        int numInputChannels = parameters.getNumInputChannels();

        constexpr float deltaThreshold = 0.01f;  // 1cm threshold for considering delta significant

        // Send composite delta for each input
        for (int i = 0; i < numInputChannels; ++i)
        {
            // The wire addresses inputs by permanent channel NUMBER; `i` is a
            // SLOT, and numbers have gaps and are not in slot order after a
            // reorder.
            int channelId = parameters.getValueTreeState().getInputChannelNumber (i);

            // Get target position (raw user-controlled position)
            auto posSection = parameters.getValueTreeState().getInputPositionSection(i);
            float targetX = posSection.getProperty(WFSParameterIDs::inputPositionX, 0.0f);
            float targetY = posSection.getProperty(WFSParameterIDs::inputPositionY, 0.0f);

            // Get composite position (final DSP position after all transformations)
            auto compositePos = calculationEngine->getCompositeInputPosition(i);

            // Compute delta (composite - target)
            float deltaX = compositePos.x - targetX;
            float deltaY = compositePos.y - targetY;

            // Only send if delta is significant
            bool deltaIsSignificant = std::abs(deltaX) > deltaThreshold || std::abs(deltaY) > deltaThreshold;

            if (deltaIsSignificant)
            {
                oscManager->sendCompositeDeltaToRemote(channelId, deltaX, deltaY);
                lastSentCompositeDeltas[i] = std::make_pair(deltaX, deltaY);
            }
        }

    };

    // Configure the visualisation component with user-configured channel counts
    inputsTab->configureVisualisation(parameters.getNumOutputChannels(),
                                      parameters.getNumReverbChannels(),
                                      parameters.getValueTreeState().getNumEffectChannels());

    // Make sure you set the size of the component after
    // you add any child components.
    // Set initial size to 90% of the screen, with minimum bounds, but never wider
    // than 16:9. Height drives the clamp: 90% of an ultrawide's width is a letterbox
    // strip (32:9 gives ~3.5:1) that the tab layouts were never designed for, and it
    // puts a whole screen-width of mouse travel between controls that belong together.
    // Only displays wider than 16:9 are affected — on 16:9 and 16:10 the min() never
    // binds, so their default size is unchanged. The window stays freely resizable, so
    // anyone who does want the full width can still drag to it.
    auto displayArea = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userBounds.toNearestInt();
    constexpr float maxAspectRatio = 16.0f / 9.0f;
    int windowHeight = juce::jmax(720, (int)(displayArea.getHeight() * 0.9f));
    int windowWidth  = juce::jmax(1280, juce::jmin((int)(displayArea.getWidth() * 0.9f),
                                                   (int)(windowHeight * maxAspectRatio)));
    setSize (windowWidth, windowHeight);

    // Enable keyboard focus for keyboard shortcuts
    setWantsKeyboardFocus(true);

    // IMPORTANT: Don't call setAudioChannels() here - it causes 887D0003 errors
    // on systems where the default Windows Audio device doesn't exist/work.
    // Instead, we'll restore the saved ASIO device asynchronously, which handles
    // initialization properly.

    // Initialize tracking with SAVED values to prevent overwriting during restore
    // These will be updated after successful restore or user device change
    lastSavedDeviceType = savedDeviceType;
    lastSavedDeviceName = savedDeviceName;

    // Restore saved device asynchronously (like DAWs do)
    // This happens after the window is shown for faster perceived startup
    juce::MessageManager::callAsync([this, savedDeviceStateXml, savedDeviceType, savedDeviceName]()
    {
        bool deviceRestored = false;

        // FAST PATH: Try to restore from saved XML state (skips device enumeration)
        if (savedDeviceStateXml.isNotEmpty())
        {
            // Parse saved XML state
            auto savedStateXml = juce::XmlDocument::parse(savedDeviceStateXml);
            if (savedStateXml != nullptr)
            {
                // Restore from the saved state (faster than manual setup), then
                // open every channel the device actually has.
                // selectDefaultDeviceOnFailure=false avoids a full rescan when
                // the saved device is unavailable.
                auto error = deviceHost.restoreFromXml(savedStateXml.get(), false);

                if (error.isEmpty() && deviceManager.getCurrentAudioDevice() != nullptr)
                {
                    deviceRestored = true;
                    auto* device = deviceManager.getCurrentAudioDevice();
                    lastSavedDeviceType = deviceManager.getCurrentAudioDeviceType();
                    lastSavedDeviceName = device->getName();
                }
                else
                {
                    DBG("Fast restore failed: " + (error.isEmpty() ? "No device available" : error));
                }
            }
        }

        // FALLBACK: If no XML state or XML restore failed, try manual setup
        if (!deviceRestored && savedDeviceType.isNotEmpty() && savedDeviceName.isNotEmpty())
        {
            // This path is slower as it triggers device enumeration
            auto error = deviceHost.openNamedDevice(savedDeviceType, savedDeviceName);
            deviceRestored = error.isEmpty();

            if (deviceRestored)
            {
                lastSavedDeviceType = savedDeviceType;
                lastSavedDeviceName = savedDeviceName;
            }
            else
            {
                DBG("Fallback restore failed: " + error);
            }
        }

        if (deviceRestored)
        {
            deviceRestoreComplete = true;
        }
        else
        {
            DBG("No audio device restored - please configure in Audio Interface window");
            // Keep deviceRestoreComplete = false to prevent saving until user selects device
        }

        attachAudioCallbacksIfNeeded();
    });

    // Log startup configuration
    WFSLogger::getInstance().logInfo ("Channels: " + juce::String (numInputChannels) + " inputs, "
                                      + juce::String (numOutputChannels) + " outputs");
    WFSLogger::getInstance().logInfo ("Language: " + savedLanguage);

    // Initialize master level gain from saved config
    {
        float masterLevelDb = (float)parameters.getConfigParam("MasterLevel");
        masterLevelGainTarget.store(
            juce::Decibels::decibelsToGain(masterLevelDb, -92.0f),
            std::memory_order_relaxed);
    }

    // Start timer for device monitoring and parameter smoothing
    startTimer(5); // 5ms timer for smooth parameter updates

    // Listen for device manager changes to re-attach audio callbacks when device changes
    deviceManager.addChangeListener(this);

    // Hidden diagnostic: WFS_TEST_CHANNEL_LIST=1 drives the structural
    // channel ops (append mono/stereo, delete-with-gap, in-place type flip,
    // gap reuse, budget enforcement) exactly like the UI will, first under the
    // fresh-session regime where every op renumbers to display order and then
    // under the latched permanent-number regime, asserts the invariants of each
    // after every step, and logs PASS/FAIL lines to the session log — no GUI
    // needed.
    if (std::getenv("WFS_TEST_CHANNEL_LIST") != nullptr)
        runChannelListSelfTest();
    else if (std::getenv("WFS_TEST_STEREO_COUNTS") != nullptr)
        WFSLogger::getInstance().logInfo("SELF-TEST: WFS_TEST_STEREO_COUNTS is superseded by WFS_TEST_CHANNEL_LIST");

    // Hidden diagnostic: WFS_TEST_STEREO_GEOMETRY=1 exercises the stereo image
    // mapping on its own. Kept out of the chain above because it shares nothing
    // with the channel-list test — that one must stay purely structural, and
    // this one touches no session state at all, so both may run in one launch.
    if (std::getenv("WFS_TEST_STEREO_GEOMETRY") != nullptr)
        runStereoGeometrySelfTest();

    // Hidden diagnostic: WFS_TEST_MCP_SURFACE=1 checks the generated MCP tool
    // manifest against what this state can actually write. Independent of the two
    // above — it mutates nothing at all, it only asks questions.
    if (std::getenv("WFS_TEST_MCP_SURFACE") != nullptr)
        runMcpSurfaceSelfTest();

    // Hidden diagnostic: WFS_TEST_LS_PERSIST=1 round-trips the Live Source
    // Tamer toggles through an exported input config. Restores what it touched.
    if (std::getenv("WFS_TEST_LS_PERSIST") != nullptr)
        runLiveSourcePersistSelfTest();

    // Hidden diagnostic: WFS_TEST_ARRAY_ATTEN_PERSIST=1 takes the ten per-input
    // array attenuations through every store and recall path. Restores what it
    // touched, but latches the channel numbers: run it in a throwaway session.
    if (std::getenv("WFS_TEST_ARRAY_ATTEN_PERSIST") != nullptr)
        runArrayAttenPersistSelfTest();

    // Hidden diagnostic: WFS_TEST_MUTES_PERSIST=1 takes the per-input output
    // mute lists through every store, recall, OSC and QLab path. Restores what
    // it touched, but latches the channel numbers: run it in a throwaway session.
    if (std::getenv("WFS_TEST_MUTES_PERSIST") != nullptr)
        runInputMutesPersistSelfTest();

    // Hidden diagnostic: WFS_TEST_RENDER_UI=<folder> renders every main tab and
    // the Snapshot Scope window (both family grids) to PNG files in that folder,
    // 8 s after launch - after a project given on the command line has loaded.
    // A component snapshot paints offscreen, so this still works behind a locked
    // workstation, where screen captures and injected clicks do not.
    if (const char* renderDir = std::getenv("WFS_TEST_RENDER_UI"))
    {
        const juce::File dir (juce::String::fromUTF8 (renderDir));
        MainComponent* const self = this;
        juce::Timer::callAfterDelay (8000, [safe = juce::Component::SafePointer<MainComponent> (self), dir]
        {
            if (safe != nullptr)
                safe->renderUiSnapshots (dir);
        });
    }
}

void MainComponent::renderUiSnapshots (const juce::File& dir)
{
    dir.createDirectory();

    auto save = [&dir] (juce::Component& c, const juce::String& name)
    {
        if (c.getWidth() <= 0 || c.getHeight() <= 0)
        {
            WFSLogger::getInstance().logInfo ("RENDER-UI skipped " + name + " (no size)");
            return;
        }

        auto image = c.createComponentSnapshot (c.getLocalBounds(), true, 1.0f);
        auto file = dir.getChildFile (juce::File::createLegalFileName (name) + ".png");
        file.deleteFile();
        juce::FileOutputStream out (file);
        if (out.openedOk() && juce::PNGImageFormat().writeImageToStream (image, out))
            WFSLogger::getInstance().logInfo ("RENDER-UI wrote " + file.getFullPathName());
    };

    for (int i = 0; i < tabbedComponent.getNumTabs(); ++i)
        if (auto* content = tabbedComponent.getTabContentComponent (i))
            save (*content, "tab-" + juce::String (i) + "-" + tabbedComponent.getTabNames()[i]);

    // Every sub-tab of the Effects tab, switched as a click on its bar would.
    if (effectsTab != nullptr)
    {
        for (auto* child : effectsTab->getChildren())
        {
            if (auto* bar = dynamic_cast<juce::TabbedButtonBar*> (child))
            {
                const int original = bar->getCurrentTabIndex();
                for (int s = 0; s < bar->getNumTabs(); ++s)
                {
                    bar->setCurrentTabIndex (s, false);
                    bar->sendSynchronousChangeMessage();
                    save (*effectsTab, "effects-subtab-" + juce::String (s) + "-" + bar->getTabNames()[s]);
                }
                bar->setCurrentTabIndex (original, false);
                bar->sendSynchronousChangeMessage();
                break;
            }
        }

        // The reverb module's panel once per model, on the first effect: the
        // rows each model shows. The model is put back as it was, and a
        // session with no effect channel gets one for the render only.
        auto& vts = parameters.getValueTreeState();
        const int effectsBefore = vts.getNumEffectChannels();
        if (effectsBefore == 0)
            vts.setNumEffectChannels (1);
        {
            auto reverb = vts.getEffectModuleSection (0, WFSParameterIDs::FxReverb);
            const juce::var storedModel = reverb.getProperty (WFSParameterIDs::effectReverbModel);
            auto& panel = effectsTab->getModulePanel (8);
            if (panel.getWidth() <= 0 || panel.getHeight() <= 0)
                panel.setSize (juce::jmax (800, effectsTab->getWidth()), juce::jmax (640, effectsTab->getHeight() - 120));

            for (const auto& model : EffectsModulePanel::reverbControl (WFSParameterIDs::effectReverbModel).items)
            {
                reverb.setProperty (WFSParameterIDs::effectReverbModel, model.value, nullptr);
                panel.loadParameters();
                save (panel, "effects-reverb-model-" + juce::String (model.value) + "-" + juce::String (model.slug));
            }

            reverb.setProperty (WFSParameterIDs::effectReverbModel, storedModel, nullptr);
            panel.loadParameters();
        }

        // Every module's panel as stored and switched on, then the Chain
        // sub-tab with every module on: the greying, the header line and the
        // colours. Each bypass is put back as it was.
        {
            std::vector<std::pair<juce::ValueTree, std::pair<juce::Identifier, juce::var>>> bypasses;
            for (int slot = 0; slot < WFSParameterDefaults::numEffectModuleSlots; ++slot)
            {
                auto& panel = effectsTab->getModulePanel (slot);
                if (panel.getWidth() <= 0 || panel.getHeight() <= 0)
                    panel.setSize (juce::jmax (800, effectsTab->getWidth()), juce::jmax (640, effectsTab->getHeight() - 120));

                const auto controls = EffectsUi::controlsForSlot (slot);
                auto module = vts.getEffectModuleSection (0, WFSValueTreeState::getEffectModuleType (slot));
                const juce::String name = "effects-module-" + juce::String (slot) + "-"
                                          + spatcore::effects::kSlots[static_cast<size_t> (slot)].token;
                panel.loadParameters();
                save (panel, name + "-stored");

                for (int k = 0; k < controls.count; ++k)
                    if (controls.controls[k].kind == EffectsUi::Kind::Bypass && module.isValid())
                    {
                        const auto& id = controls.controls[k].id;
                        bypasses.push_back ({ module, { id, module.getProperty (id) } });
                        module.setProperty (id, 0, nullptr);
                    }
                panel.loadParameters();
                save (panel, name + "-on");
            }

            for (auto* child : effectsTab->getChildren())
                if (auto* bar = dynamic_cast<juce::TabbedButtonBar*> (child))
                {
                    const int original = bar->getCurrentTabIndex();
                    bar->setCurrentTabIndex (1, false);                 // Chain
                    bar->sendSynchronousChangeMessage();
                    save (*effectsTab, "effects-chain-all-on");
                    bar->setCurrentTabIndex (original, false);
                    bar->sendSynchronousChangeMessage();
                    break;
                }

            for (auto& [module, prop] : bypasses)
            {
                if (prop.second.isVoid())
                    module.removeProperty (prop.first, nullptr);
                else
                    module.setProperty (prop.first, prop.second, nullptr);
            }
            for (int slot = 0; slot < WFSParameterDefaults::numEffectModuleSlots; ++slot)
                effectsTab->getModulePanel (slot).loadParameters();
        }

        if (effectsBefore == 0)
            vts.setNumEffectChannels (0);
    }

    // Every sub-tab of the Inputs tab the same way. The Effect Sends bank has a
    // strip per effect, so a session with no effect channel gets one for the
    // render only, as the reverb-model render above does.
    if (inputsTab != nullptr)
    {
        auto& vts = parameters.getValueTreeState();
        const int effectsBefore = vts.getNumEffectChannels();
        if (effectsBefore == 0)
            vts.setNumEffectChannels (1);

        // The deck follows the bar as it does when the operator is on the
        // Inputs tab, so a sub-tab whose page marks the GUI (Effect Sends
        // outlines the four strips its page holds) renders as the operator
        // sees it; the deck is put back where it was afterwards.
        const int deckMain = streamDeckManager ? streamDeckManager->getCurrentMainTab() : 0;
        const int deckSub = streamDeckManager ? streamDeckManager->getCurrentSubTab() : 0;
        const int deckChannel = streamDeckManager ? streamDeckManager->getChannel() : 0;
        if (streamDeckManager)
            streamDeckManager->syncNavigation (InputsTabPages::INPUTS_MAIN_TAB_INDEX, 0, inputsTab->getCurrentChannel());

        for (auto* child : inputsTab->getChildren())
        {
            if (auto* bar = dynamic_cast<juce::TabbedButtonBar*> (child))
            {
                const int original = bar->getCurrentTabIndex();
                for (int s = 0; s < bar->getNumTabs(); ++s)
                {
                    bar->setCurrentTabIndex (s, false);
                    bar->sendSynchronousChangeMessage();
                    save (*inputsTab, "inputs-subtab-" + juce::String (s) + "-" + bar->getTabNames()[s]);
                }
                bar->setCurrentTabIndex (original, false);
                bar->sendSynchronousChangeMessage();
                break;
            }
        }

        if (streamDeckManager)
            streamDeckManager->syncNavigation (deckMain, deckSub, deckChannel);
        if (effectsBefore == 0)
            vts.setNumEffectChannels (0);
    }

    // The Scope window, opened as the Effects tab's row opens it, then switched
    // to the inputs grid as the Inputs tab's row would switch it.
    if (snapshotSession != nullptr)
    {
        snapshotSession->editScope (WFSFileManager::SnapshotFamily::Effects);

        for (int i = juce::Desktop::getInstance().getNumComponents(); --i >= 0;)
        {
            if (auto* window = dynamic_cast<SnapshotScopeWindow*> (juce::Desktop::getInstance().getComponent (i)))
            {
                if (auto* content = window->getContentComponent())
                {
                    save (*content, "scope-effects");
                    snapshotSession->editScope (WFSFileManager::SnapshotFamily::Inputs);
                    save (*content, "scope-inputs");
                }
                window->closeButtonPressed();
                break;
            }
        }
    }

    WFSLogger::getInstance().logInfo ("RENDER-UI done");
}

void MainComponent::runLiveSourcePersistSelfTest()
{
    using namespace WFSParameterIDs;
    auto& vts = parameters.getValueTreeState();
    auto& fm = parameters.getFileManager();
    int failures = 0;

    auto logLine = [](const juce::String& s) { WFSLogger::getInstance().logInfo(s); };
    auto check = [&](bool ok, const juce::String& what)
    {
        if (! ok) ++failures;
        logLine(juce::String("SELF-TEST ") + (ok ? "PASS " : "FAIL ") + what);
    };

    logLine("SELF-TEST begin (Live Source Tamer toggle persistence)");

    if (vts.getNumInputChannels() < 1)
    {
        logLine("SELF-TEST SKIP L: this session has no input channel");
        logLine("SELF-TEST RESULT: SKIPPED");
        return;
    }

    const juce::Identifier toggles[] = { inputLSactive, inputLSpeakEnable, inputLSslowEnable };
    auto ls = vts.getInputLiveSourceSection(0);
    juce::var original[3];
    for (int i = 0; i < 3; ++i)
        original[i] = ls.getProperty(toggles[i]);

    auto setAll = [&](int v)
    {
        for (const auto& toggle : toggles)
            ls.setProperty(toggle, v, nullptr);
    };
    auto allRead = [&](int v)
    {
        for (const auto& toggle : toggles)
            if ((static_cast<int>(ls.getProperty(toggle, -1)) != 0) != (v != 0))
                return false;
        return true;
    };

    auto file = juce::File::getSpecialLocation(juce::File::tempDirectory)
                    .getChildFile("wfs-selftest-ls-persist-inputs.xml");
    file.deleteFile();

    setAll(1);
    check(fm.exportInputConfig(file), "L1: export the input config with every tamer toggle on");
    {
        const auto text = file.loadFileAsString();
        check(text.contains("inputLSactive=\"1\""), "L1: the file carries inputLSactive=1");
        check(text.contains("inputLSpeakEnable=\"1\""), "L1: the file carries inputLSpeakEnable=1");
        check(text.contains("inputLSslowEnable=\"1\""), "L1: the file carries inputLSslowEnable=1");
    }

    setAll(0);
    check(fm.importInputConfig(file), "L2: import it back with the toggles cleared");
    ls = vts.getInputLiveSourceSection(0);
    check(allRead(1), "L2: every tamer toggle came back on");

    for (int i = 0; i < 3; ++i)
    {
        if (original[i].isVoid())
            ls.removeProperty(toggles[i], nullptr);
        else
            ls.setProperty(toggles[i], original[i], nullptr);
    }
    file.deleteFile();

    logLine(failures == 0 ? "SELF-TEST RESULT: ALL PASS"
                          : "SELF-TEST RESULT: FAIL (" + juce::String(failures) + ")");
}

void MainComponent::runArrayAttenPersistSelfTest()
{
    using namespace WFSParameterIDs;
    using Scope = WFSFileManager::ExtendedSnapshotScope;
    auto& vts = parameters.getValueTreeState();
    auto& fm = parameters.getFileManager();
    int failures = 0;

    auto logLine = [](const juce::String& s) { WFSLogger::getInstance().logInfo(s); };
    auto check = [&](bool ok, const juce::String& what)
    {
        if (! ok) ++failures;
        logLine(juce::String("SELF-TEST ") + (ok ? "PASS " : "FAIL ") + what);
    };

    logLine("SELF-TEST begin (array attenuation store/recall and tablet typing)");

    const int numChannels = vts.getNumInputChannels();
    if (numChannels < 2)
    {
        logLine("SELF-TEST SKIP A: this session has fewer than two input channels");
        logLine("SELF-TEST RESULT: SKIPPED");
        return;
    }

    static const juce::Identifier attenIds[10] = {
        inputArrayAtten1, inputArrayAtten2, inputArrayAtten3, inputArrayAtten4, inputArrayAtten5,
        inputArrayAtten6, inputArrayAtten7, inputArrayAtten8, inputArrayAtten9, inputArrayAtten10
    };

    // No undo entries and no "modified" marks for the test's own writes
    WFSValueTreeState::ScopedUndoSuppression noUndo (vts);
    parameters.getDirtyTracker().beginSuppression();

    // Every <Mutes> property written, with what it held, to put back at the end
    struct Touched { int slot; juce::Identifier property; juce::var original; bool existed; };
    std::vector<Touched> touched;
    auto setMutes = [&](int slot, const juce::Identifier& property, const juce::var& value)
    {
        auto mutes = vts.getInputMutesSection(slot);
        const bool known = std::any_of(touched.begin(), touched.end(),
                                       [&](const Touched& t) { return t.slot == slot && t.property == property; });
        if (! known)
            touched.push_back({ slot, property, mutes.getProperty(property), mutes.hasProperty(property) });
        mutes.setProperty(property, value, nullptr);
    };
    auto atten = [&](int slot, int array) { return vts.getInputMutesSection(slot).getProperty(attenIds[array - 1]); };
    // (not "near": <windows.h> defines that as an empty macro)
    auto isNear = [](const juce::var& v, double expected)
    {
        return WFSVar::isNumeric(v) && std::abs(WFSVar::toFloat(v) - expected) < 1.0e-6;
    };

    struct Level { int slot; int array; double db; };
    const Level levels[] = { { 0, 1, -6.5 }, { 0, 3, -12.25 }, { 0, 10, -60.0 },
                             { 1, 2, -3.5 }, { 1, 5, -42.75 } };
    auto setLevels = [&](bool zero)
    {
        for (const auto& l : levels)
            setMutes(l.slot, attenIds[l.array - 1], zero ? 0.0 : l.db);
    };
    auto levelsBack = [&]()
    {
        for (const auto& l : levels)
            if (! isNear(atten(l.slot, l.array), l.db))
                return false;
        return true;
    };

    // --- A0: every channel carries all ten, numeric and in range
    {
        bool present = true, inRange = true;
        for (int slot = 0; slot < numChannels; ++slot)
            for (int array = 1; array <= 10; ++array)
            {
                const auto v = atten(slot, array);
                if (v.isVoid())
                    present = false;
                else if (! WFSVar::isNumeric(v) || WFSVar::toFloat(v) < -60.0f || WFSVar::toFloat(v) > 0.0f)
                    inRange = false;
            }
        check(present, "A0: every channel carries inputArrayAtten1..10");
        check(inRange, "A0: every one is a number within -60..0 dB");
    }

    // --- A1: an exported input config brings them back exactly
    const auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    auto exportFile = tempDir.getChildFile("wfs-selftest-arrayatten-inputs.xml");
    exportFile.deleteFile();

    setLevels(false);
    check(fm.exportInputConfig(exportFile), "A1: export the input config with five distinct levels on two channels");
    const auto exported = exportFile.loadFileAsString();
    check(exported.contains("inputArrayAtten3=\"-12.25\""), "A1: the file carries inputArrayAtten3=\"-12.25\"");

    setLevels(true);
    check(fm.importInputConfig(exportFile), "A1: import it back with the five levels zeroed");
    check(levelsBack(), "A1: all five came back (-6.5, -12.25, -60 / -3.5, -42.75)");
    logLine(juce::String("SELF-TEST note A1: imported levels are held as ")
            + (atten(0, 3).isString() ? "strings" : "numbers"));

    // A1c: a file value outside -60..0 is refused at load and the live value kept
    {
        const auto corrupted = exported.replace("inputArrayAtten5=\"-42.75\"", "inputArrayAtten5=\"-75.0\"");
        check(corrupted != exported, "A1c: the export holds inputArrayAtten5=\"-42.75\" to corrupt");
        exportFile.replaceWithText(corrupted);
        setMutes(1, inputArrayAtten5, -1.5);
        check(fm.importInputConfig(exportFile), "A1c: import a file with inputArrayAtten5=\"-75.0\"");
        check(isNear(atten(1, 5), -1.5), "A1c: the out-of-range -75 is refused; the live -1.5 stays");
    }

    // --- A2: snapshots, in a scratch project folder (at a cold start the project
    // folder is the user's last project; nothing is ever written there)
    const auto previousFolder = fm.getProjectFolder();
    auto scratch = tempDir.getChildFile("wfs-selftest-arrayatten");
    scratch.deleteRecursively();
    scratch.createDirectory();
    fm.setProjectFolder(scratch);
    fm.createProjectFolderStructure();

    // The stored <Mutes> of the snapshot entry for one channel number
    auto storedMutes = [&](const juce::String& snapshotName, int number) -> juce::ValueTree
    {
        const auto file = fm.getInputSnapshotsFolder().getChildFile(snapshotName + ".xml");
        if (auto xml = juce::XmlDocument::parse(file))
        {
            const auto inputs = juce::ValueTree::fromXml(*xml).getChildWithName(Inputs);
            for (int i = 0; i < inputs.getNumChildren(); ++i)
                if (static_cast<int>(inputs.getChild(i).getProperty(id)) == number)
                    return inputs.getChild(i).getChildWithName(Mutes);
        }
        return {};
    };
    const int number0 = vts.getInputChannelNumber(0);
    const int number1 = vts.getInputChannelNumber(1);

    Scope all;
    all.initializeDefaults(numChannels);

    setLevels(false);
    setMutes(0, inputSidelinesFringe, 2.5);
    check(fm.saveInputSnapshotWithExtendedScope("aa-selftest", all), "A2: store a snapshot with the default scope");
    check(isNear(storedMutes("aa-selftest", number0).getProperty(inputArrayAtten3), -12.25),
          "A2: the entry for channel #" + juce::String(number0) + " stores inputArrayAtten3 = -12.25");

    setLevels(true);
    check(fm.loadInputSnapshotWithExtendedScope("aa-selftest", fm.getExtendedSnapshotScope("aa-selftest")),
          "A2: recall it with its own scope");
    check(levelsBack(), "A2: all five levels came back");

    // A2b: left out of the scope, they stay; the sidelines fringe on the same
    // node is the positive control that the recall did run
    setMutes(0, inputArrayAtten3, -1.5);
    setMutes(0, inputSidelinesFringe, 1.25);
    {
        auto withoutLevels = all;
        withoutLevels.setItemForAllChannels("arrayAttens", false, numChannels);
        check(fm.loadInputSnapshotWithExtendedScope("aa-selftest", withoutLevels),
              "A2b: recall with Array Attens left out of the scope");
    }
    check(isNear(atten(0, 3), -1.5), "A2b: the level stays at its live -1.5");
    check(isNear(vts.getInputMutesSection(0).getProperty(inputSidelinesFringe), 2.5),
          "A2b: the sidelines fringe on the same node was recalled (2.5)");

    // A2c: When Saving, left out on slot 1 only: that entry stores none of them
    setLevels(false);
    {
        Scope onSave;
        onSave.initializeDefaults(numChannels);
        onSave.applyMode = Scope::ApplyMode::OnSave;
        onSave.setIncluded("arrayAttens", 1, false);
        check(fm.saveInputSnapshotWithExtendedScope("aa-selftest-onsave", onSave),
              "A2c: store When Saving with Array Attens left out on slot 1");
    }
    check(! storedMutes("aa-selftest-onsave", number1).hasProperty(inputArrayAtten2),
          "A2c: the entry for channel #" + juce::String(number1) + " stores no levels");
    check(isNear(storedMutes("aa-selftest-onsave", number0).getProperty(inputArrayAtten3), -12.25),
          "A2c: the entry for channel #" + juce::String(number0) + " keeps them");

    // --- A3: the When Saving re-scope pairs each stored number with its live
    // slot. #2 is at slot 0, #1 at slot 1, #7 has no live channel: the old
    // number - 1 rule trimmed #1 instead of #2 and trimmed #7 with slot 6.
    {
        juce::ValueTree inputs (Inputs);
        for (int number : { 1, 2, 7 })
        {
            juce::ValueTree input (Input);
            input.setProperty(id, number, nullptr);
            juce::ValueTree mutes (Mutes);
            for (const auto& attenId : attenIds)
                mutes.setProperty(attenId, -6.0, nullptr);
            mutes.setProperty(inputSidelinesFringe, 2.0, nullptr);
            input.appendChild(mutes, nullptr);
            inputs.appendChild(input, nullptr);
        }

        Scope scope;
        scope.initializeDefaults(8);
        scope.applyMode = Scope::ApplyMode::OnSave;
        scope.setIncluded("arrayAttens", 0, false);

        WFSFileManager::trimSnapshotInputsToScope(inputs, scope,
            [](int number) { return number == 2 ? 0 : number == 1 ? 1 : -1; });

        auto mutesOf = [&inputs](int number)
        {
            for (int i = 0; i < inputs.getNumChildren(); ++i)
                if (static_cast<int>(inputs.getChild(i).getProperty(id)) == number)
                    return inputs.getChild(i).getChildWithName(Mutes);
            return juce::ValueTree();
        };
        check(! mutesOf(2).hasProperty(inputArrayAtten1) && mutesOf(2).hasProperty(inputSidelinesFringe),
              "A3: #2 (slot 0) lost its levels and kept its fringe");
        check(mutesOf(1).hasProperty(inputArrayAtten1), "A3: #1 (slot 1) kept its levels");
        check(mutesOf(7).hasProperty(inputArrayAtten1), "A3: #7 (no live channel) was left whole");
    }

    // A3b: the real update on a stored snapshot (this session's numbers are dense)
    {
        Scope rescope;
        rescope.initializeDefaults(numChannels);
        rescope.applyMode = Scope::ApplyMode::OnSave;
        rescope.setIncluded("arrayAttens", 0, false);
        check(fm.updateInputSnapshotScope("aa-selftest", rescope),
              "A3b: update the stored snapshot's scope to When Saving without slot 0's levels");
    }
    check(! storedMutes("aa-selftest", number0).hasProperty(inputArrayAtten3)
              && isNear(storedMutes("aa-selftest", number1).getProperty(inputArrayAtten2), -3.5),
          "A3b: channel #" + juce::String(number0) + " lost its stored levels, #" + juce::String(number1) + " kept them");

    fm.setProjectFolder(previousFolder);
    scratch.deleteRecursively();
    exportFile.deleteFile();

    // --- A4: a channel whose node lacks a level (appended whole from an old
    // inputs.xml) still takes writes: both write paths create it in <Mutes>
    {
        auto mutes = vts.getInputMutesSection(0);
        setMutes(0, inputArrayAtten7, 0.0);       // remembered, restored at the end
        mutes.removeProperty(inputArrayAtten7, nullptr);

        check(vts.canWriteParameter(inputArrayAtten7, 0), "A4: a channel missing inputArrayAtten7 reports it writable");
        vts.setInputParameter(0, inputArrayAtten7, -9.5);
        check(isNear(mutes.getProperty(inputArrayAtten7), -9.5), "A4: setInputParameter recreated it in <Mutes> at -9.5");

        bool elsewhere = false;
        auto input = vts.getInputState(0);
        for (int i = 0; i < input.getNumChildren(); ++i)
            if (! input.getChild(i).hasType(Mutes) && input.getChild(i).hasProperty(inputArrayAtten7))
                elsewhere = true;
        check(! elsewhere, "A4: and nowhere else");

        mutes.removeProperty(inputArrayAtten7, nullptr);
        vts.setParameter(inputArrayAtten7, -8.5, 0);
        check(isNear(mutes.getProperty(inputArrayAtten7), -8.5), "A4: setParameter (the /wfs and MCP path) recreated it at -8.5");

        vts.setInputParameter(0, inputArrayAtten7, -75.0);
        check(isNear(mutes.getProperty(inputArrayAtten7), -60.0), "A4: a -75 write clamps to -60");
    }

    // --- A5: the tablet echo of a value held as text is typed by the parameter
    {
        using WFSNetwork::OSCMessageBuilder;
        auto tagsOf = [](const std::optional<juce::OSCMessage>& msg) -> juce::String
        {
            if (! msg.has_value())
                return "none";
            juce::String tags (",");
            for (const auto& arg : *msg)
                tags << (arg.isInt32() ? "i" : arg.isFloat32() ? "f" : arg.isString() ? "s" : "?");
            return tags;
        };

        const auto level = OSCMessageBuilder::buildRemoteEchoMessage(inputArrayAtten3, 4, juce::var("-6.0"));
        check(tagsOf(level) == ",if" && level->getAddressPattern().toString() == "/remoteInput/arrayAtten3"
                  && (*level)[1].getFloat32() == -6.0f,
              "A5: a level held as \"-6.0\" echoes as /remoteInput/arrayAtten3 ,if 4 -6.0");
        check(tagsOf(OSCMessageBuilder::buildRemoteEchoMessage(inputCluster, 4, juce::var("3"))) == ",ii",
              "A5: a cluster held as \"3\" echoes as ,ii");
        check(tagsOf(OSCMessageBuilder::buildRemoteEchoMessage(inputName, 4, juce::var("Kick"))) == ",is",
              "A5: a name still echoes as ,is");
        check(tagsOf(OSCMessageBuilder::buildRemoteEchoMessage(inputMutes, 4, juce::var("0,1"))) == ",is",
              "A5: a mute list still echoes as ,is");
        check(tagsOf(OSCMessageBuilder::buildRemoteEchoMessage(inputArrayAtten3, 4, juce::var("abc"))) == "none",
              "A5: text that is no number on a bounded parameter sends nothing");
        check(tagsOf(OSCMessageBuilder::buildRemoteEchoMessage(inputLFOshapeX, 4, juce::var(3))) == ",ii",
              "A5: an int var still echoes as ,ii");
        check(tagsOf(OSCMessageBuilder::buildRemoteEchoMessage(inputAttenuation, 4, juce::var(-6.0))) == ",if",
              "A5: a double var still echoes as ,if");
        check(tagsOf(OSCMessageBuilder::buildRemoteEchoMessage(inputArrayAtten3, 4, juce::var())) == "none",
              "A5: a void var sends nothing");
    }

    // Put back every level (and the fringe) this test wrote
    for (const auto& t : touched)
    {
        auto mutes = vts.getInputMutesSection(t.slot);
        if (t.existed)
            mutes.setProperty(t.property, t.original, nullptr);
        else
            mutes.removeProperty(t.property, nullptr);
    }
    parameters.getDirtyTracker().endSuppressionAndClear();

    logLine("SELF-TEST note: the import and the snapshot store latched the channel numbers and "
            "the import cleared the undo history; treat this session as disposable");
    logLine(failures == 0 ? "SELF-TEST RESULT: ALL PASS"
                          : "SELF-TEST RESULT: FAIL (" + juce::String(failures) + ")");
}

void MainComponent::runInputMutesPersistSelfTest()
{
    using namespace WFSParameterIDs;
    using Scope = WFSFileManager::ExtendedSnapshotScope;
    using WFSNetwork::OSCMessageRouter;
    auto& vts = parameters.getValueTreeState();
    auto& fm = parameters.getFileManager();
    int failures = 0;

    auto logLine = [](const juce::String& s) { WFSLogger::getInstance().logInfo(s); };
    auto check = [&](bool ok, const juce::String& what)
    {
        if (! ok) ++failures;
        logLine(juce::String("SELF-TEST ") + (ok ? "PASS " : "FAIL ") + what);
    };

    logLine("SELF-TEST begin (input mute lists: format, store and recall, OSC, QLab)");

    const int numChannels = vts.getNumInputChannels();
    const int numOutputs = vts.getNumOutputChannels();
    if (numChannels < 2 || numOutputs < 6)
    {
        logLine("SELF-TEST SKIP M: this session needs two inputs and six outputs");
        logLine("SELF-TEST RESULT: SKIPPED");
        return;
    }

    WFSValueTreeState::ScopedUndoSuppression noUndo (vts);
    parameters.getDirtyTracker().beginSuppression();

    // Every list, to put back at the end
    std::vector<juce::var> originalMutes;
    for (int slot = 0; slot < numChannels; ++slot)
        originalMutes.push_back(vts.getInputMutesSection(slot).getProperty(inputMutes));

    auto mutesOf = [&](int slot) { return vts.getInputMutesSection(slot).getProperty(inputMutes).toString(); };
    auto setRaw = [&](int slot, const juce::String& list) { vts.getInputMutesSection(slot).setProperty(inputMutes, list, nullptr); };
    // The canonical list for this rig with the given outputs (1-based) muted
    auto listWith = [numOutputs](std::initializer_list<int> muted)
    {
        juce::StringArray tokens;
        for (int out = 1; out <= numOutputs; ++out)
            tokens.add(std::find(muted.begin(), muted.end(), out) != muted.end() ? "1" : "0");
        return tokens.joinIntoString(",");
    };
    auto tokensOf = [](const juce::String& list) { juce::StringArray t; t.addTokens(list, ",", ""); return t; };

    // --- M1: the list format
    check(WFSValueTreeState::normaliseMuteList("0,1,0", 5) == "0,1,0,0,0", "M1: a short list is padded with unmuted outputs");
    check(WFSValueTreeState::normaliseMuteList("1,1,1,1,1,1", 4) == "1,1,1,1", "M1: a long list is cut to the output count");
    check(WFSValueTreeState::normaliseMuteList("1.0", 3) == "1,0,0", "M1: a collapsed \"1.0\" reads as output 1 muted");
    check(WFSValueTreeState::normaliseMuteList(" 0 , 5 ,x", 3) == "0,1,0", "M1: non-zero is muted, text unmuted, spaces ignored");
    check(WFSValueTreeState::normaliseMuteList("1,1,1,1", 4, 2) == "1,1,0,0", "M1: entries past keepTokens start unmuted");
    check(WFSValueTreeState::normaliseMuteList({}, 3) == "0,0,0", "M1: no list is all unmuted");
    check(WFSValueTreeState::normaliseMuteList("1,0,1", 0) == "1,0,1", "M1: numOutputs 0 canonicalises without resizing");

    // --- M2: the store's guard
    const auto pattern = listWith({ 2, 4, 5, 6 });
    vts.setInputParameter(0, inputMutes, pattern);
    check(mutesOf(0) == pattern, "M2: a full list is stored as sent");
    vts.setInputParameter(0, inputMutes, 1.0);
    check(mutesOf(0) == pattern, "M2: a bare number (what a QLab cue sent) leaves the list alone");
    vts.setParameter(inputMutes, 0, 0);
    check(mutesOf(0) == pattern, "M2: through setParameter, the OSC and MCP path, too");
    {
        juce::StringArray grid;
        for (int i = 0; i < WFSParameterDefaults::maxOutputChannels; ++i)
            grid.add(i % 2 == 0 ? "1" : "0");
        vts.setInputParameter(0, inputMutes, grid.joinIntoString(","));
        check(tokensOf(mutesOf(0)).size() == numOutputs, "M2: a 128-entry list from the old grid is cut to the "
                                                         + juce::String(numOutputs) + " outputs");
    }

    // --- M3: one output at a time
    vts.setInputParameter(0, inputMutes, listWith({}));
    check(vts.setInputOutputMute(0, 2, true) && mutesOf(0) == listWith({ 3 }), "M3: mute output 3 alone");
    check(vts.setInputOutputMute(0, 4, true) && mutesOf(0) == listWith({ 3, 5 }), "M3: output 5 joins, output 3 stays");
    check(vts.setInputOutputMute(0, 2, false) && mutesOf(0) == listWith({ 5 }), "M3: unmute output 3, output 5 stays");
    check(! vts.setInputOutputMute(0, numOutputs, true) && mutesOf(0) == listWith({ 5 }),
          "M3: an output past the count is refused and changes nothing");

    // --- M4: an exported input config brings them back exactly
    const auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    auto exportFile = tempDir.getChildFile("wfs-selftest-mutes-inputs.xml");
    exportFile.deleteFile();

    vts.setInputParameter(0, inputMutes, pattern);
    vts.setInputParameter(1, inputMutes, listWith({ 1 }));
    check(fm.exportInputConfig(exportFile), "M4: export the input config with two mute patterns");
    check(exportFile.loadFileAsString().contains("inputMutes=\"" + pattern + "\""),
          "M4: the file carries inputMutes=\"" + pattern + "\"");
    setRaw(0, listWith({}));
    setRaw(1, listWith({}));
    check(fm.importInputConfig(exportFile), "M4: import it back with both lists cleared");
    check(mutesOf(0) == pattern && mutesOf(1) == listWith({ 1 }), "M4: both patterns came back");

    // --- M5: snapshots, in a scratch project folder
    const auto previousFolder = fm.getProjectFolder();
    auto scratch = tempDir.getChildFile("wfs-selftest-mutes");
    scratch.deleteRecursively();
    scratch.createDirectory();
    fm.setProjectFolder(scratch);
    fm.createProjectFolderStructure();

    Scope all;
    all.initializeDefaults(numChannels);
    vts.setInputParameter(0, inputMutes, pattern);
    check(fm.saveInputSnapshotWithExtendedScope("mutes-selftest", all), "M5: store a snapshot with the default scope");
    vts.setInputParameter(0, inputMutes, listWith({}));
    check(fm.loadInputSnapshotWithExtendedScope("mutes-selftest", fm.getExtendedSnapshotScope("mutes-selftest")),
          "M5: recall it");
    check(mutesOf(0) == pattern, "M5: the pattern came back");

    // --- M6: the QLab cues built from that snapshot
    // What QLab does with a custom string: split at spaces outside quotes,
    // send a quoted token as a string, a bare number as a number.
    auto sendLikeQLab = [](const juce::String& customString)
    {
        std::vector<std::pair<juce::String, bool>> tokens;   // text, was quoted
        juce::String current;
        bool inQuotes = false, quoted = false;
        for (int ci = 0; ci < customString.length(); ++ci)
        {
            const auto c = customString[ci];
            if (c == '"') { inQuotes = ! inQuotes; quoted = true; continue; }
            if (c == ' ' && ! inQuotes)
            {
                if (current.isNotEmpty() || quoted) tokens.push_back({ current, quoted });
                current.clear(); quoted = false;
                continue;
            }
            current += c;
        }
        if (current.isNotEmpty() || quoted) tokens.push_back({ current, quoted });

        juce::OSCMessage msg (juce::OSCAddressPattern (tokens.empty() ? juce::String("/") : tokens.front().first));
        for (size_t i = 1; i < tokens.size(); ++i)
        {
            const auto& [text, wasQuoted] = tokens[i];
            const bool isInt = ! wasQuoted && text.isNotEmpty()
                               && text.trimCharactersAtStart("-+").containsOnly("0123456789")
                               && text.trimCharactersAtStart("-+").isNotEmpty();
            const bool isFloat = ! wasQuoted && ! isInt && text.containsAnyOf("0123456789")
                                 && text.containsOnly("0123456789.-+eE");
            if (isInt)        msg.addInt32(text.getIntValue());
            else if (isFloat) msg.addFloat32(text.getFloatValue());
            else              msg.addString(text);
        }
        return msg;
    };
    auto customStringOf = [](const WFSNetwork::QLabCueSequence::NetworkCue& cue, const juce::String& address)
    {
        for (const auto& m : cue.messages)
            if (m.getAddressPattern().toString() == address && m.size() > 0 && m[0].isString())
                return m[0].getString();
        return juce::String();
    };

    {
        const auto file = fm.getInputSnapshotsFolder().getChildFile("mutes-selftest.xml");
        juce::ValueTree inputsData;
        if (auto xml = juce::XmlDocument::parse(file))
            inputsData = juce::ValueTree::fromXml(*xml).getChildWithName(Inputs);
        check(inputsData.isValid(), "M6: read the stored snapshot");

        const auto effScope = all.withGlobals(fm.isSamplerMasterOn(), numChannels);
        const auto numberToSlot = [&vts](int number) { return vts.getSlotForChannelNumber(number); };
        auto sequence = WFSNetwork::QLabCueBuilder::buildSnapshotCues("mutes-selftest", inputsData, effScope,
                                                                      numChannels, 1, numberToSlot, numOutputs);
        check(WFSNetwork::QLabCueBuilder::countCues(inputsData, effScope, numChannels, numberToSlot)
                  == static_cast<int>(sequence.networkCues.size()),
              "M6: countCues agrees with the " + juce::String(static_cast<int>(sequence.networkCues.size())) + " cues built");

        const int number0 = vts.getInputChannelNumber(0);
        const auto expectedMuteCue = "/wfs/input/mutes " + juce::String(number0) + " \"" + pattern + "\"";
        bool foundMuteCue = false, muteCueNamed = false;
        int total = 0, parsedBack = 0;
        juce::StringArray refused;
        const auto& mappings = WFSNetwork::OSCMessageBuilder::getInputMappings();

        for (const auto& cue : sequence.networkCues)
        {
            const auto customString = customStringOf(cue, "/cue/selected/customString");
            if (customString.isEmpty())
                continue;
            ++total;

            if (customString == expectedMuteCue)
            {
                foundMuteCue = true;
                muteCueNamed = customStringOf(cue, "/cue/selected/name")
                               == "Input " + juce::String(number0) + " Mutes: 2, 4-6";
            }

            const auto msg = sendLikeQLab(customString);
            const auto parsed = OSCMessageRouter::parseInputMessage(msg);
            const auto it = mappings.find(parsed.paramId);
            if (parsed.valid && it != mappings.end()
                && it->second.oscPath == msg.getAddressPattern().toString())
                ++parsedBack;
            else if (refused.size() < 12)
                refused.add(customString + (parsed.invalidReason.isNotEmpty() ? " (" + parsed.invalidReason + ")" : ""));

            if (customString == expectedMuteCue)
                check(parsed.valid && parsed.muteOutput == 0 && parsed.value.toString() == pattern,
                      "M6: the mute cue parses back to the whole list");
        }

        check(foundMuteCue, "M6: the mute cue carries the whole list, quoted: " + expectedMuteCue);
        check(muteCueNamed, "M6: the mute cue is named \"Input " + juce::String(number0) + " Mutes: 2, 4-6\"");
        check(total > 0 && parsedBack == total,
              "M6: " + juce::String(parsedBack) + " of " + juce::String(total)
                  + " cues parse back to their own parameter"
                  + (refused.isEmpty() ? juce::String() : "; refused: " + refused.joinIntoString(" | ")));
    }

    // M6b: the sampler set goes out counted from 1, the way the address reads it
    {
        juce::ValueTree inputs (Inputs), input (Input), sampler (Sampler);
        input.setProperty(id, vts.getInputChannelNumber(0), nullptr);
        sampler.setProperty(inputSamplerActiveSet, "2", nullptr);   // third set, as a file holds it
        input.appendChild(sampler, nullptr);
        inputs.appendChild(input, nullptr);

        const auto numberToSlot = [&vts](int number) { return vts.getSlotForChannelNumber(number); };
        auto sequence = WFSNetwork::QLabCueBuilder::buildSnapshotCues("sampler-selftest", inputs, Scope(),
                                                                      numChannels, 1, numberToSlot, numOutputs);
        const auto customString = sequence.networkCues.empty() ? juce::String()
                                      : customStringOf(sequence.networkCues.front(), "/cue/selected/customString");
        check(customString == "/wfs/input/samplerSet " + juce::String(vts.getInputChannelNumber(0)) + " 3",
              "M6b: stored set 2 (the third) goes out as set 3: " + customString);
    }

    // M6c: on a one-output rig the whole list is a lone "1", which the receiver
    // refuses as a bare number, so the cue uses the one-output form
    {
        juce::ValueTree inputs (Inputs), input (Input), mutes (Mutes);
        input.setProperty(id, vts.getInputChannelNumber(0), nullptr);
        mutes.setProperty(inputMutes, "1", nullptr);
        input.appendChild(mutes, nullptr);
        inputs.appendChild(input, nullptr);

        const auto numberToSlot = [&vts](int number) { return vts.getSlotForChannelNumber(number); };
        auto sequence = WFSNetwork::QLabCueBuilder::buildSnapshotCues("one-output-selftest", inputs, Scope(),
                                                                      numChannels, 1, numberToSlot, 1);
        const auto customString = sequence.networkCues.empty() ? juce::String()
                                      : customStringOf(sequence.networkCues.front(), "/cue/selected/customString");
        const auto parsed = OSCMessageRouter::parseInputMessage(sendLikeQLab(customString));
        check(customString == "/wfs/input/mutes " + juce::String(vts.getInputChannelNumber(0)) + " 1 1"
                  && parsed.valid && parsed.muteOutput == 1 && static_cast<int>(parsed.value) == 1,
              "M6c: a one-output list goes out as output 1 muted and is received: " + customString);
    }

    fm.setProjectFolder(previousFolder);
    scratch.deleteRecursively();
    exportFile.deleteFile();

    // --- M7: the OSC argument forms
    {
        auto standard = [](std::function<void (juce::OSCMessage&)> fill)
        {
            juce::OSCMessage m ("/wfs/input/mutes");
            m.addInt32(3);
            fill(m);
            return OSCMessageRouter::parseInputMessage(m);
        };

        auto p = standard([](auto& m) { m.addString("0,1,0"); });
        check(p.valid && p.muteOutput == 0 && p.value.toString() == "0,1,0", "M7: <ch> \"<list>\" sets the whole list");
        p = standard([](auto& m) { m.addInt32(5); m.addInt32(1); });
        check(p.valid && p.muteOutput == 5 && static_cast<int>(p.value) == 1, "M7: <ch> 5 1 mutes output 5");
        p = standard([](auto& m) { m.addString("5"); m.addString("0"); });
        check(p.valid && p.muteOutput == 5 && static_cast<int>(p.value) == 0, "M7: numeric strings (QLab) work too");
        p = standard([](auto& m) { m.addFloat32(5.0f); m.addFloat32(1.0f); });
        check(p.valid && p.muteOutput == 5, "M7: so do floats");
        p = standard([](auto& m) { m.addInt32(0); });
        check(! p.valid && p.invalidReason.isNotEmpty(), "M7: a lone number is refused, with a reason");
        p = standard([](auto& m) { m.addString("1"); });
        check(! p.valid, "M7: a lone numeric string is refused");
        p = standard([](auto& m) { m.addInt32(5); m.addInt32(2); });
        check(! p.valid, "M7: a state other than 0 or 1 is refused");
        p = standard([](auto& m) { m.addInt32(0); m.addInt32(1); });
        check(! p.valid, "M7: output 0 is refused");
        p = standard([](auto& m) { m.addInt32(WFSParameterDefaults::maxOutputChannels + 1); m.addInt32(1); });
        check(! p.valid, "M7: an output past the maximum is refused");

        juce::OSCMessage shortList ("/wfs/input/3/mutes");
        shortList.addString("1,0");
        p = OSCMessageRouter::parseInputMessage(shortList);
        check(p.valid && p.channelId == 3 && p.value.toString() == "1,0", "M7: /wfs/input/3/mutes \"<list>\"");
        juce::OSCMessage shortOne ("/wfs/input/3/mutes");
        shortOne.addInt32(4);
        shortOne.addInt32(1);
        p = OSCMessageRouter::parseInputMessage(shortOne);
        check(p.valid && p.muteOutput == 4, "M7: /wfs/input/3/mutes 4 1");
        juce::OSCMessage shortScalar ("/wfs/input/3/mutes");
        shortScalar.addInt32(1);
        check(! OSCMessageRouter::parseInputMessage(shortScalar).valid, "M7: /wfs/input/3/mutes 1 is refused");

        // The sending names the receiver did not know
        for (const auto& [alias, param] : OSCMessageRouter::getInputInboundAliases())
        {
            juce::OSCMessage m ("/wfs/input/" + alias);
            m.addInt32(3);
            m.addInt32(1);
            const auto parsed = OSCMessageRouter::parseInputMessage(m);
            check(parsed.valid && parsed.paramId == param, "M7: /wfs/input/" + alias + " is received");
        }
        juce::OSCMessage legacy ("/wfs/input/LSenable");
        legacy.addInt32(3);
        legacy.addInt32(1);
        check(OSCMessageRouter::parseInputMessage(legacy).paramId == inputLSactive, "M7: /wfs/input/LSenable still is");
    }

    // --- M8: a count change starts added outputs unmuted when the list is the
    // old grid's (every button written), and keeps any other list's real tail
    const int added = juce::jmin(2, WFSParameterDefaults::maxOutputChannels - numOutputs);
    if (added <= 0)
    {
        logLine("SELF-TEST SKIP M8: no room above " + juce::String(numOutputs) + " outputs");
    }
    else
    {
        auto allOnes = [](int n) { juce::StringArray t; for (int i = 0; i < n; ++i) t.add("1"); return t.joinIntoString(","); };

        setRaw(0, allOnes(WFSParameterDefaults::maxOutputChannels));   // the old grid's Mute All, hidden buttons included
        setRaw(1, allOnes(numOutputs + added));                        // a real list saved on a bigger rig, reloaded raw

        vts.setNumOutputChannels(numOutputs + added);
        const auto grid = tokensOf(mutesOf(0));
        const auto real = tokensOf(mutesOf(1));
        bool keptLive = grid.size() == numOutputs + added, addedClear = keptLive;
        for (int i = 0; keptLive && i < numOutputs; ++i)
            keptLive = grid[i] == "1";
        for (int i = numOutputs; addedClear && i < numOutputs + added; ++i)
            addedClear = grid[i] == "0";
        check(keptLive, "M8: the live outputs keep their mutes through the count change");
        check(addedClear, "M8: outputs added start unmuted despite the old grid's tail");
        const int realLength = numOutputs + added;
        if (realLength == 64 || realLength == WFSParameterDefaults::maxOutputChannels)
            logLine("SELF-TEST SKIP M8: a " + juce::String(realLength) + "-entry list reads as the old grid's here");
        else
            check(real.size() == realLength && ! real.contains("0"),
                  "M8: a shorter list's real tail is kept for the outputs added");

        vts.setNumOutputChannels(numOutputs);
        check(vts.getNumOutputChannels() == numOutputs, "M8: the output count is back to " + juce::String(numOutputs));
    }

    // Put every list back
    for (int slot = 0; slot < numChannels && slot < static_cast<int>(originalMutes.size()); ++slot)
    {
        auto mutes = vts.getInputMutesSection(slot);
        if (originalMutes[static_cast<size_t>(slot)].isVoid())
            mutes.removeProperty(inputMutes, nullptr);
        else
            mutes.setProperty(inputMutes, originalMutes[static_cast<size_t>(slot)], nullptr);
    }
    parameters.getDirtyTracker().endSuppressionAndClear();

    logLine("SELF-TEST note: the import and the snapshot store latched the channel numbers and "
            "the import cleared the undo history; treat this session as disposable");
    logLine(failures == 0 ? "SELF-TEST RESULT: ALL PASS"
                          : "SELF-TEST RESULT: FAIL (" + juce::String(failures) + ")");
}

void MainComponent::runMcpSurfaceSelfTest()
{
    auto& vts = parameters.getValueTreeState();
    auto& log = WFSLogger::getInstance();
    auto logLine = [&log] (const juce::String& s) { log.logInfo (s); };

    logLine ("SELF-TEST begin (MCP surface writability)");

    const auto manifestFile = findGeneratedToolsJson();
    if (! manifestFile.existsAsFile())
    {
        logLine ("SELF-TEST SKIP M: generated_tools.json not found at "
                 + manifestFile.getFullPathName());
        logLine ("SELF-TEST RESULT: SKIPPED");
        return;
    }

    const auto parsed = juce::JSON::parse (manifestFile);
    if (parsed.getDynamicObject() == nullptr)
    {
        logLine ("SELF-TEST FAIL M: manifest is not a JSON object");
        logLine ("SELF-TEST RESULT: FAIL");
        return;
    }

    // The audit itself lives in MCPSurfaceAudit.h, shared with the startup
    // auditor so the two cannot drift apart about what counts as dead.
    const auto result = WFSNetwork::SurfaceAudit::run (parsed, vts);

    for (const auto& line : result.deadDetails)
        logLine ("SELF-TEST FAIL M " + line);

    logLine ("SELF-TEST M: " + WFSNetwork::SurfaceAudit::summarise (result));

    if (result.skippedNoChannel > 0)
        logLine ("SELF-TEST M: NOTE - this session has no channels of some kinds, so those "
                 "parameters were not judged. Re-run with a project that has them.");
    if (result.skippedSubTree > 0)
        logLine ("SELF-TEST M: NOTE - sub-tree routed tools resolve their own node, so this "
                 "check cannot speak for them. They are covered by driving them over MCP.");

    if (result.deadCount() > 0)
        logLine ("SELF-TEST M: distinct unwritable parameters: "
                 + result.deadVariables.joinIntoString (", "));

    logLine (result.deadCount() == 0 ? "SELF-TEST RESULT: ALL PASS"
                                     : "SELF-TEST RESULT: FAIL");
}

void MainComponent::runChannelListSelfTest()
{
    auto& vts = parameters.getValueTreeState();
    int failures = 0;

    auto logLine = [](const juce::String& s) { WFSLogger::getInstance().logInfo(s); };

    auto patchRows = [this]() -> juce::StringArray
    {
        auto audioPatchTree = parameters.getValueTreeState().getState().getChildWithName(WFSParameterIDs::AudioPatch);
        auto inputPatchTree = audioPatchTree.getChildWithName(WFSParameterIDs::InputPatch);
        return juce::StringArray::fromTokens(
            inputPatchTree.getProperty(WFSParameterIDs::patchData).toString(), ";", "");
    };

    auto colsOfRow = [](const juce::String& row) -> juce::String
    {
        juce::StringArray cols = juce::StringArray::fromTokens(row, ",", "");
        juce::String patched;
        for (int c = 0; c < cols.size(); ++c)
            if (cols[c].getIntValue() == 1)
                patched += (patched.isEmpty() ? "" : "+") + juce::String(c + 1);
        return patched.isEmpty() ? "-" : patched;
    };

    auto patchOfNumber = [&](int number) -> juce::String
    {
        const int slot = vts.getSlotForChannelNumber(number);
        auto rows = patchRows();
        return (slot >= 0 && slot < rows.size()) ? colsOfRow(rows[slot]) : juce::String("?");
    };

    auto check = [&](bool ok, const juce::String& what)
    {
        if (! ok) ++failures;
        logLine(juce::String("SELF-TEST ") + (ok ? "PASS " : "FAIL ") + what);
    };

    // One line per step: "number(type):patched-cols" in slot (display) order.
    auto verify = [&](const char* label)
    {
        const int n = vts.getNumInputChannels();
        auto rows = patchRows();

        bool numbersUnique = true;
        juce::SortedSet<int> seen;
        juce::String summary;
        for (int slot = 0; slot < n; ++slot)
        {
            const int number = vts.getInputChannelNumber(slot);
            if (number <= 0 || seen.contains(number)) numbersUnique = false;
            seen.add(number);
            summary += juce::String(number) + (vts.isInputChannelStereo(slot) ? "s" : "m") + ":"
                       + (slot < rows.size() ? colsOfRow(rows[slot]) : juce::String("?")) + " ";
        }
        logLine(juce::String("SELF-TEST ") + label + " [" + summary.trim() + "]");

        check(numbersUnique, juce::String(label) + ": numbers unique and positive");
        check(rows.size() == n, juce::String(label) + ": patch rows == live channels");
        check(renderSourceMap.count == n + 5 * vts.getNumStereoInputChannels() + vts.getNumEffectChannels(),
              juce::String(label) + ": render sources = N + 5*stereo + effects");
    };

    logLine("SELF-TEST begin (channel list flow)");
    auto reconfig = [&]() { handleChannelCountChange(); };

    auto numbersInSlotOrder = [&]() -> juce::String
    {
        juce::String s;
        for (int slot = 0; slot < vts.getNumInputChannels(); ++slot)
            s += (slot == 0 ? "" : ",") + juce::String(vts.getInputChannelNumber(slot));
        return s;
    };

    auto nameOfSlot = [&](int slot) -> juce::String
    {
        return vts.getInputChannelSection(slot).getProperty(WFSParameterIDs::inputName).toString();
    };

    auto namesInSlotOrder = [&]() -> juce::String
    {
        juce::String s;
        for (int slot = 0; slot < vts.getNumInputChannels(); ++slot)
            s += (slot == 0 ? "" : ",") + nameOfSlot(slot);
        return s;
    };

    auto patchInSlotOrder = [&]() -> juce::String
    {
        auto rows = patchRows();
        juce::String s;
        for (int slot = 0; slot < vts.getNumInputChannels(); ++slot)
            s += (slot == 0 ? "" : ",")
                 + (slot < rows.size() ? colsOfRow(rows[slot]) : juce::String("?"));
        return s;
    };

    auto trackingIdOfSlot = [&](int slot) -> int
    {
        return (int) vts.getInputPositionSection(slot).getProperty(WFSParameterIDs::inputTrackingID, 0);
    };

    // Unlatched phase: on a session nothing has ever exposed a number from, every
    // structural op renumbers the list to display order, so the numbers read 1..N
    // top to bottom. Unreachable once anything has latched — a project loaded at
    // startup already has — so that case skips instead of failing.
    if (vts.areChannelNumbersUserOwned())
    {
        logLine("SELF-TEST SKIP U: channel numbers already user-owned (a project was loaded "
                "at startup), so the fresh-session renumbering cannot be exercised");
    }
    else
    {
        // The operator's repro, verbatim: the default 8 monos, two stereo pairs
        // added, four more monos added, then one of the new monos dragged in
        // between the two pairs. U0b pins the hardware inputs to the display
        // order — 9+10,11,12+13 — which is the whole point of the patch re-flow:
        // insertInputPatchRow allocates from the GLOBAL highest patched column,
        // so its columns follow creation order, and moveInputPatchRow carries
        // them with the row, together yielding 9+10,13,11+12 unless the tail
        // re-flow runs. U1's setInputChannelCounts below is the restore, so
        // nothing under it sees this shape.
        vts.setInputChannelCounts(8, 0);
        reconfig();
        vts.setInputChannelCounts(8, 2);
        reconfig();
        vts.setInputChannelCounts(12, 2);
        reconfig();
        verify("U0a: 8 mono + 2 stereo + 4 mono");
        check(numbersInSlotOrder() == "1,2,3,4,5,6,7,8,9,10,11,12,13,14",
              "U0a: the grown list numbers 1..14 in display order");
        check(patchInSlotOrder() == "1,2,3,4,5,6,7,8,9+10,11+12,13,14,15,16",
              "U0a: appending in display order leaves a strictly packed diagonal");

        // Slots after U0a: 0..7 mono, 8 = Stereo 1, 9 = Stereo 2, 10..13 mono.
        check(vts.moveInputChannel(vts.getInputChannelNumber(10), 9).wasOk(),
              "U0b: drag the first of the new monos between the two stereo pairs");
        reconfig();
        verify("U0b");
        check(numbersInSlotOrder() == "1,2,3,4,5,6,7,8,9,10,11,12,13,14",
              "U0b: the move renumbered the list back to display order");
        check(patchInSlotOrder() == "1,2,3,4,5,6,7,8,9+10,11,12+13,14,15,16",
              "U0b: the move re-flowed the patch into a packed diagonal, two adjacent columns per stereo row");

        vts.setInputChannelCounts(3, 0);
        reconfig();
        verify("U1: 3 mono, unlatched");
        check(numbersInSlotOrder() == "1,2,3", "U1: fresh list numbers 1..3");
        check(namesInSlotOrder() == "Mono 1,Mono 2,Mono 3",
              "U1: default names count within their type, not from the number");
        check(patchInSlotOrder() == "1,2,3", "U1: three monos take hardware inputs 1..3");

        check(vts.addInputChannel(true).wasOk(), "U2: add stereo");
        reconfig();
        verify("U2");
        check(vts.getInputChannelNumber(3) == 4, "U2: the appended stereo is number 4");
        check(nameOfSlot(3) == "Stereo 1", "U2: the appended stereo is born on the stereo counter");
        check(patchInSlotOrder() == "1,2,3,4+5",
              "U2: the appended stereo takes the next two adjacent columns");

        // Captured before the drag: a default name belongs to its CHANNEL, so
        // both of these must survive a move that changes the numbers under them.
        const juce::String draggedName = nameOfSlot(3);
        const juce::String tailMonoName = nameOfSlot(2);
        check(vts.moveInputChannel(4, 1).wasOk(), "U3: drag the stereo to display slot 1");
        reconfig();
        verify("U3");
        check(numbersInSlotOrder() == "1,2,3,4", "U3: numbers follow the new display order");
        check(patchInSlotOrder() == "1,2+3,4,5", "U3: the patch follows the new display order too");
        check(vts.isInputChannelStereo(1) && vts.getInputChannelNumber(1) == 2,
              "U3: the dragged stereo is now number 2 at slot 1");
        check(nameOfSlot(1) == draggedName && nameOfSlot(3) == tailMonoName,
              "U3: the names stayed with their channels while the numbers moved");
        check(trackingIdOfSlot(1) == 2, "U3: the tracking id followed the number");

        // A name the operator typed is not derived from the number, so the
        // renumber must leave it alone while the number under it changes.
        vts.setInputParameter(1, WFSParameterIDs::inputName, "Grand Piano");
        check(vts.moveInputChannel(2, 0).wasOk(), "U4: drag the renamed stereo to display slot 0");
        reconfig();
        verify("U4");
        check(numbersInSlotOrder() == "1,2,3,4", "U4: numbers still dense after the second move");
        check(patchInSlotOrder() == "1+2,3,4,5",
              "U4: the patch is still a packed diagonal after the second move");
        check(vts.getInputChannelNumber(0) == 1 && nameOfSlot(0) == "Grand Piano",
              "U4: the custom name survived the number change (2 -> 1)");

        // The per-type ordinal is a property of the DISPLAY order, so a mono
        // that jumps its siblings only takes its new name at the resequence —
        // the pass the arrange dialog runs when it closes.
        check(vts.moveInputChannel(vts.getInputChannelNumber(3), 1).wasOk(),
              "U4b: drag the last mono to display slot 1");
        reconfig();
        verify("U4b");
        check(namesInSlotOrder() == "Grand Piano,Mono 3,Mono 1,Mono 2",
              "U4b: the move alone renamed nothing");
        check(patchInSlotOrder() == "1+2,3,4,5",
              "U4b: the patch re-flowed at the move, without waiting for the resequence");
        vts.resequenceDefaultInputNames();
        check(namesInSlotOrder() == "Grand Piano,Mono 1,Mono 2,Mono 3",
              "U4b: the resequence renumbers the mono defaults in display order and spares the custom name");

        check(vts.removeInputChannel(vts.getInputChannelNumber(0)).wasOk(),
              "U5: delete the channel at slot 0");
        reconfig();
        verify("U5");
        check(numbersInSlotOrder() == "1,2,3" && vts.getHighestChannelNumber() == 3,
              "U5: the delete left no gap");
        check(patchInSlotOrder() == "1,2,3", "U5: the delete closed the hardware-input gap too");

        // U7: the FIRST project save ends the fresh session — saveSystemConfig
        // persists the counts, the patch and the channel inventory, so the
        // numbers it writes are durable the moment the file exists. Saved into
        // a scratch folder; the real project folder (restored from settings at
        // startup) is put back afterwards, and setProjectFolder's synced reset
        // re-arms the auto-save guard for it.
        {
            auto& fm = parameters.getFileManager();
            const auto previousFolder = fm.getProjectFolder();
            auto scratchFolder = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                     .getChildFile("wfs-selftest-firstsave");
            scratchFolder.deleteRecursively();
            scratchFolder.createDirectory();

            fm.setProjectFolder(scratchFolder);
            check(! vts.areChannelNumbersUserOwned(), "U7: still fresh before the first save");
            check(fm.saveSystemConfig(), "U7: first system-config save succeeded");
            check(vts.areChannelNumbersUserOwned(), "U7: the first save latched the numbers");

            fm.setProjectFolder(previousFolder);
            scratchFolder.deleteRecursively();
        }

        // Latching is what hands the numbers to the outside world; everything
        // below it is the append-only regime the rest of this test asserts.
        // (U7's save latched already; this mark is the idempotent no-op case.)
        vts.markChannelNumbersUserOwned ("self-test U6");
        check(vts.areChannelNumbersUserOwned(), "U6: numbers latched");
        check(vts.moveInputChannel(3, 0).wasOk(), "U6: drag channel 3 to display slot 0");
        reconfig();
        verify("U6");
        check(numbersInSlotOrder() == "3,1,2", "U6: a latched move does NOT renumber");
        check(patchInSlotOrder() == "3,1,2",
              "U6: a latched move carries the columns WITH the rows and does NOT re-flow");
    }

    // The numbers are latched from here (the branch above either latched them or
    // found them latched): steps E, F and J only mean anything under the
    // permanent-number regime. The counts below already match, so this is a
    // no-op that just re-states the starting shape.
    vts.setInputChannelCounts(3, 0);
    reconfig();
    verify("A: 3 mono");

    check(vts.addInputChannel(true).wasOk(), "B: add stereo (number 4)");
    reconfig();
    verify("B");

    check(vts.addInputChannel(false).wasOk(), "C: add mono AFTER the stereo (number 5, interleaved)");
    reconfig();
    verify("C");

    check(vts.addInputChannel(true).wasOk(), "D: add stereo (number 6)");
    reconfig();
    verify("D");

    const auto p3 = patchOfNumber(3), p4 = patchOfNumber(4), p6 = patchOfNumber(6);
    check(vts.removeInputChannel(2).wasOk(), "E: remove channel 2 (leaves a gap)");
    reconfig();
    verify("E");
    check(vts.getSlotForChannelNumber(2) < 0, "E: number 2 retired");
    check(patchOfNumber(3) == p3 && patchOfNumber(4) == p4 && patchOfNumber(6) == p6,
          "E: surviving channels keep their patch columns");

    check(vts.addInputChannel(false).wasOk(), "F: add appends number 7 (gap NOT reused)");
    reconfig();
    verify("F");
    check(vts.getHighestChannelNumber() == 7 && vts.getSlotForChannelNumber(2) < 0,
          "F: numbering is append-only");

    check(vts.setInputChannelType(5, true).wasOk(), "G: flip channel 5 mono->stereo in place");
    reconfig();
    verify("G");
    check(vts.isInputChannelStereo(vts.getSlotForChannelNumber(5)), "G: type persisted");

    check(vts.setInputChannelType(5, false).wasOk(), "H: flip channel 5 back to mono");
    reconfig();
    verify("H");

    check(! vts.setInputChannelType(2, true).wasOk(), "I: type flip on a dead number is rejected");

    check(vts.addInputChannel(false, 2).wasOk(), "J: explicit re-create of retired number 2");
    reconfig();
    verify("J");
    check(vts.getSlotForChannelNumber(2) == vts.getNumInputChannels() - 1,
          "J: re-created number 2 appended at the END of the display order");

    // Drag-to-reorder: move stereo channel 6 to display slot 1 — its patch
    // columns must travel with it, and every number must stay put.
    {
        // The tablet reads its display order out of the /remote/channelList
        // payload, so the payload must follow TREE order and not numeric order:
        // built from a sort (or from 1..N) it would come out identical across a
        // reorder, and every channel below the moved one would then be drawn,
        // picked and pinned at the wrong position on the tablet with nothing on
        // the wire looking wrong. No socket involved — this reads the builder.
        auto sortedNumbers = [](const std::vector<int>& payload)
        {
            std::vector<int> numbers;
            for (size_t i = 1; i + 1 < payload.size(); i += 2)
                numbers.push_back(payload[i]);
            std::sort(numbers.begin(), numbers.end());
            return numbers;
        };

        const auto p6before = patchOfNumber(6);
        const auto inventoryBefore = oscManager->buildRemoteChannelListPayload();
        check(vts.moveInputChannel(6, 1).wasOk(), "L: drag channel 6 to display slot 1");
        reconfig();
        verify("L");
        check(vts.getSlotForChannelNumber(6) == 1, "L: channel 6 now at slot 1");
        check(patchOfNumber(6) == p6before, "L: channel 6 kept its patch columns through the move");

        const auto inventoryAfter = oscManager->buildRemoteChannelListPayload();
        check(inventoryAfter != inventoryBefore
                  && sortedNumbers(inventoryAfter) == sortedNumbers(inventoryBefore),
              "L: the remote channel inventory re-ordered with the move and kept the same set of numbers");
    }

    int added = 0;
    while (added <= 10 && vts.addInputChannel(true).wasOk())
    {
        reconfig();
        ++added;
    }
    check(added == 6 && vts.getNumStereoInputChannels() == 8, "K: stereo budget enforced at 8");
    check(! vts.setInputChannelType(5, true).wasOk(), "K: type flip beyond the stereo budget is rejected");
    verify("K");

    // Phases R and S load files that deliberately do NOT match the session -
    // that is what they test - so they run under an identity-gate bypass. The
    // identity phases that follow (I, V) run without one.
    {
    WFSFileManager::ScopedChannelIdentityBypass identityBypassForRS (parameters.getFileManager());

    // ---- R: system-config round trip -------------------------------------
    // The regression this phase exists for: <IO inputChannels> is a SUM, and the
    // mono/stereo split and the display order live on the <Input> nodes, which
    // go to inputs.xml. So reloading a system config on its own rebuilt every
    // channel as mono — and because patch rows are positional and DID load in
    // file order, a stereo row's two hardware columns landed on a mono channel.
    // Export/import take an explicit file, so this needs no project folder.
    {
        auto typesInSlotOrder = [&]() -> juce::String
        {
            juce::String t;
            for (int slot = 0; slot < vts.getNumInputChannels(); ++slot)
                t += (slot == 0 ? "" : ",") + juce::String(vts.isInputChannelStereo(slot) ? "s" : "m");
            return t;
        };

        auto& fm = parameters.getFileManager();
        auto roundTripFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                 .getChildFile("wfs-selftest-system.xml");
        roundTripFile.deleteFile();

        // A deliberately awkward shape: stereo at BOTH ends with monos between,
        // which is exactly what two counts (or a "last N are stereo" tail split)
        // cannot describe and only a per-channel inventory can.
        vts.setInputChannelCounts(4, 0);
        reconfig();
        check(vts.addInputChannel(true).wasOk(), "R0: add a stereo pair");
        reconfig();
        check(vts.moveInputChannel(vts.getInputChannelNumber(vts.getNumInputChannels() - 1), 0).wasOk(),
              "R0: drag it to the top of the display order");
        reconfig();
        check(vts.addInputChannel(true).wasOk(), "R0: add a second stereo pair at the bottom");
        reconfig();
        verify("R0: stereo at both ends");

        const auto numbersBefore = numbersInSlotOrder();
        const auto typesBefore   = typesInSlotOrder();
        const auto patchBefore   = patchInSlotOrder();
        const int  countBefore   = vts.getNumInputChannels();
        const int  stereoBefore  = vts.getNumStereoInputChannels();

        check(fm.exportSystemConfig(roundTripFile), "R1: export the system config");
        check(roundTripFile.loadFileAsString().contains("InputChannelList"),
              "R1: the file carries an explicit channel inventory, not just a count");

        // Mutate hard enough that a load which only honours the SUM cannot
        // accidentally look correct: different count AND all-mono.
        vts.setInputChannelCounts(9, 0);
        reconfig();
        check(vts.getNumStereoInputChannels() == 0, "R2: scrambled to 9 mono, 0 stereo");

        check(fm.importSystemConfig(roundTripFile), "R3: reload the system config alone");
        reconfig();
        verify("R3: after reloading the system config alone");

        check(vts.getNumInputChannels() == countBefore,
              "R3: the channel count came back");
        check(vts.getNumStereoInputChannels() == stereoBefore,
              "R3: the stereo channels came back as stereo, not as mono");
        check(typesInSlotOrder() == typesBefore,
              "R3: the mono/stereo arrangement came back in the same slots");
        check(numbersInSlotOrder() == numbersBefore,
              "R3: permanent channel numbers and display order survived");
        check(patchInSlotOrder() == patchBefore,
              "R3: every channel got its own hardware inputs back");

        // The reported symptom, stated as its own check: a mono channel must
        // never end up holding a stereo row's two hardware inputs.
        {
            bool monoRowOverPatched = false;
            auto rows = patchRows();
            for (int slot = 0; slot < vts.getNumInputChannels() && slot < rows.size(); ++slot)
            {
                if (vts.isInputChannelStereo(slot))
                    continue;
                juce::StringArray cols = juce::StringArray::fromTokens(rows[slot], ",", "");
                int patched = 0;
                for (int c = 0; c < cols.size(); ++c)
                    patched += (cols[c].getIntValue() == 1) ? 1 : 0;
                if (patched > 1)
                    monoRowOverPatched = true;
            }
            check(! monoRowOverPatched, "R3: no mono channel holds two hardware inputs");
        }

        // One hardware input feeds one channel. This is the invariant the
        // interactive editor upholds and a merged-in patchData string does not.
        {
            juce::SortedSet<int> claimed;
            bool duplicateColumn = false;
            auto rows = patchRows();
            for (int slot = 0; slot < vts.getNumInputChannels() && slot < rows.size(); ++slot)
            {
                juce::StringArray cols = juce::StringArray::fromTokens(rows[slot], ",", "");
                for (int c = 0; c < cols.size(); ++c)
                    if (cols[c].getIntValue() == 1)
                    {
                        if (claimed.contains(c)) duplicateColumn = true;
                        claimed.add(c);
                    }
            }
            check(! duplicateColumn, "R3: no hardware input is claimed by two channels");
        }

        // Idempotence: a second load of the same file must not drift.
        check(fm.importSystemConfig(roundTripFile), "R4: load the same file again");
        reconfig();
        check(numbersInSlotOrder() == numbersBefore && typesInSlotOrder() == typesBefore
                  && patchInSlotOrder() == patchBefore,
              "R4: reloading the same file twice is a fixed point");

        roundTripFile.deleteFile();
    }

    // ---- S: cluster order across a load that deletes a LOW slot -------------
    // clusterInputOrder is slot-keyed in memory but lives in system.xml while
    // the slot space is defined by inputs.xml. The load-time channel-list
    // reconciliation used to shift it: deletions ran remapClusterInputOrders
    // (against a CSV already in the FILE's slot space) and the display-order
    // restore, a raw moveChild, did not remap at all. Number keying on disk
    // removes the dependency; this phase is the gate on that.
    //
    // The deletion must be of a LOW slot. Removing the highest shifts nothing
    // below it, which is why the old setNumInputChannels path never showed this
    // and why an ascending, prefix-shaped test would pass while broken.
    {
        auto& fm = parameters.getFileManager();
        auto roundTripFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                 .getChildFile("wfs-selftest-clusters.xml");
        roundTripFile.deleteFile();

        auto orderOfCluster = [&](int cluster) -> juce::String
        {
            // Rendered as NUMBERS, so the assertion is about which channels are
            // in which order — not about which slots they happen to occupy.
            juce::StringArray tokens;
            tokens.addTokens(vts.getClusterParameter(cluster, WFSParameterIDs::clusterInputOrder).toString(), ",", "");
            juce::String out;
            for (const auto& tok : tokens)
            {
                const int number = vts.getInputChannelNumber(tok.trim().getIntValue());
                out += (out.isEmpty() ? "" : ",") + juce::String(number);
            }
            return out;
        };

        vts.setInputChannelCounts(6, 0);
        reconfig();

        // Cluster 1 gets the last three channels, in a deliberately
        // non-ascending order so a lost ordering cannot pass by accident.
        const int chA = vts.getInputChannelNumber(3);
        const int chB = vts.getInputChannelNumber(4);
        const int chC = vts.getInputChannelNumber(5);
        for (int n : { chA, chB, chC })
            vts.setInputParameter(vts.getSlotForChannelNumber(n), WFSParameterIDs::inputCluster, 1);
        const juce::String slotCsv =
            juce::String(vts.getSlotForChannelNumber(chC)) + ","
          + juce::String(vts.getSlotForChannelNumber(chA)) + ","
          + juce::String(vts.getSlotForChannelNumber(chB));
        vts.setClusterParameter(1, WFSParameterIDs::clusterInputOrder, slotCsv);

        const juce::String orderBefore = orderOfCluster(1);
        check(orderBefore == juce::String(chC) + "," + juce::String(chA) + "," + juce::String(chB),
              "S0: cluster order set to a non-ascending arrangement");

        check(fm.exportSystemConfig(roundTripFile), "S1: export the system config");
        check(roundTripFile.loadFileAsString().contains("inputOrderKey"),
              "S1: the file marks its cluster orders as number-keyed");

        // Delete a LOW-slot channel that is NOT in the cluster, so every cluster
        // member's slot shifts down by one while its NUMBER does not.
        const int doomed = vts.getInputChannelNumber(0);
        check(vts.removeInputChannel(doomed).wasOk(), "S2: delete the lowest-slot channel");
        reconfig();
        check(orderOfCluster(1) == orderBefore,
              "S2: the live edit remapped the order, so it still names the same channels");

        // Scramble. The added channel is then dragged to slot 0, and THAT is the
        // point of this step, not the count: the reload must delete a channel the
        // file does not list from a LOW slot. Deleting a high slot shifts nothing
        // beneath it, which is exactly why the old setNumInputChannels path
        // (remove-the-highest) never exposed this and why a naive grow-then-reload
        // test passes on broken code.
        vts.setClusterParameter(1, WFSParameterIDs::clusterInputOrder, "");
        check(vts.addInputChannel(false).wasOk(), "S2b: add a channel the saved file does not list");
        reconfig();
        const int intruder = vts.getInputChannelNumber(vts.getNumInputChannels() - 1);
        check(vts.moveInputChannel(intruder, 0).wasOk(), "S2b: drag it to the lowest slot");
        reconfig();
        check(vts.getSlotForChannelNumber(intruder) == 0,
              "S2b: the unlisted channel sits below every cluster member");

        check(fm.importSystemConfig(roundTripFile), "S3: reload the system config");
        reconfig();
        check(orderOfCluster(1) == orderBefore,
              "S3: the cluster order came back naming the same channels in the same order");

        check(fm.importSystemConfig(roundTripFile), "S4: load the same file again");
        reconfig();
        check(orderOfCluster(1) == orderBefore, "S4: loading twice is a fixed point");

        // S5: a file written before the marker existed. Its CSVs are SLOTS, and
        // they must still load correctly — by flush time the live slot space is
        // the one the file was saved against, so the values are restored
        // verbatim rather than converted. Built by rewriting the exported file
        // rather than by keeping an old fixture, so it cannot drift from the
        // real format.
        {
            auto legacyFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                  .getChildFile("wfs-selftest-clusters-legacy.xml");
            legacyFile.deleteFile();

            juce::String xml = roundTripFile.loadFileAsString();
            xml = xml.replace(" inputOrderKey=\"number\"", "");                 // no marker == legacy
            xml = xml.replace("clusterInputOrder=\"" + orderOfCluster(1) + "\"",
                              "clusterInputOrder=\"" + slotCsv + "\"");          // numbers -> slots
            check(! xml.contains("inputOrderKey"), "S5: the legacy fixture carries no marker");
            check(xml.contains("clusterInputOrder=\"" + slotCsv + "\""),
                  "S5: the legacy fixture holds slot indices");
            legacyFile.replaceWithText(xml);

            vts.setClusterParameter(1, WFSParameterIDs::clusterInputOrder, "");
            check(fm.importSystemConfig(legacyFile), "S5: load the legacy-shaped file");
            reconfig();
            check(orderOfCluster(1) == orderBefore,
                  "S5: a pre-marker slot-keyed file still restores the same channels in order");

            legacyFile.deleteFile();
        }

        roundTripFile.deleteFile();
    }

    }   // end of the R/S bypass

    // ---- T: every per-input property is either snapshotted or explicitly not --
    // The AutomOtion polar destination sat in no scope item for the whole life of
    // the feature and nothing noticed, because save and recall consult the SAME
    // tables: an omission is symmetric, so a store/recall round trip of it comes
    // back perfectly green. Only a structural check catches that class. Any
    // property added to an <Input> child node from now on must either be carried
    // by a scope item or be named here with a reason.
    {
        struct Excluded { const char* name; const char* why; };
        static const Excluded kNotSnapshotted[] = {
            { "inputName",               "always captured, outside the scope system by design" },
            { "inputColour",             "per-channel identity like inputName: it travels with the "
                                         "channel and belongs to the operator's labelling of the rig, "
                                         "not to show state, so recalling a cue must never repaint it" },
            { "inputSolo",               "transient monitoring state, not show state" },
            { "inputOtomoPauseResume",   "run-state of a motion in flight, not a destination" },
            { "inputHiddenByCluster",    "cache of (inputCluster, clusterInputsVisible); ClustersTab "
                                         "recomputes it for every channel in a callAsync after a recall, "
                                         "so a restored value would be overwritten a tick later" },
            { "samplerMidiZoneQuadrant", "declared and defaulted but read by nothing; snapshotting it "
                                         "would be a no-op" },
        };

        // The nodes the snapshot walks as PROPERTY sections. GradientMaps and
        // Sampler are copied as subtrees and are deliberately not in this list.
        const juce::Identifier sections[] = {
            WFSParameterIDs::Channel, WFSParameterIDs::Position, WFSParameterIDs::Attenuation,
            WFSParameterIDs::Directivity, WFSParameterIDs::LiveSourceTamer, WFSParameterIDs::Hackoustics,
            WFSParameterIDs::LFO, WFSParameterIDs::AutomOtion, WFSParameterIDs::Mutes
        };

        auto input = vts.getInputState(0);
        int uncovered = 0;
        for (const auto& sectionId : sections)
        {
            auto section = input.getChildWithName(sectionId);
            if (! section.isValid())
                continue;

            for (int i = 0; i < section.getNumProperties(); ++i)
            {
                const auto prop = section.getPropertyName(i);
                if (WFSFileManager::isPropertyCoveredBySnapshotScope(prop))
                    continue;

                bool listed = false;
                for (const auto& e : kNotSnapshotted)
                    if (prop.toString() == e.name) { listed = true; break; }

                if (! listed)
                {
                    ++uncovered;
                    logLine("SELF-TEST FAIL T: <" + sectionId.toString() + "> property '"
                            + prop.toString() + "' is in no scope item and is not on the "
                            "deliberately-not-snapshotted list - it will be silently absent "
                            "from every snapshot");
                }
            }
        }
        check(uncovered == 0, "T: every per-input property is either snapshotted or explicitly excluded");

        // The other direction: an exclusion that is no longer real is a stale
        // comment claiming a decision that nothing enforces.
        for (const auto& e : kNotSnapshotted)
            check(! WFSFileManager::isPropertyCoveredBySnapshotScope(juce::Identifier(e.name)),
                  juce::String("T: '") + e.name + "' is still deliberately excluded");
    }

    // ---- Q: every per-effect property is either snapshotted or explicitly not --
    // T's twin for the effects half of a snapshot (plan revision 8: one file
    // carries both families). The same trap applies - store and recall read the
    // same table, so an omission round-trips perfectly green - and one more: the
    // effects scope is NODE-driven for the eleven modules, so the walk visits
    // every direct child of an <Effect> AND their <Band>/<Tap> children, asking
    // the predicate the store, the recall and the trim all use.
    {
        namespace ESS = EffectsSnapshotScope;

        const int effectsBefore = vts.getNumEffectChannels();
        if (effectsBefore == 0)
            vts.setNumEffectChannels(1);

        auto effect = vts.getEffectState(0);
        check(effect.isValid(), "Q: an effect channel to walk");

        int walked = 0;
        int uncovered = 0;

        std::function<void (const juce::ValueTree&, const juce::Identifier&)> walk;
        walk = [&](const juce::ValueTree& node, const juce::Identifier& childOfEffect)
        {
            for (int i = 0; i < node.getNumProperties(); ++i)
            {
                const auto prop = node.getPropertyName(i);
                if (prop == WFSParameterIDs::id)
                    continue;

                ++walked;
                if (ESS::isEffectPropertyCovered(childOfEffect, prop))
                    continue;

                bool listed = false;
                for (const auto& e : ESS::notSnapshotted())
                    if (prop.toString() == e.name) { listed = true; break; }

                if (! listed)
                {
                    ++uncovered;
                    logLine("SELF-TEST FAIL Q: <" + childOfEffect.toString() + "> property '"
                            + prop.toString() + "' is in no effects scope item and is not on the "
                            "deliberately-not-snapshotted list - it will be silently absent "
                            "from every snapshot");
                }
            }

            for (int c = 0; c < node.getNumChildren(); ++c)
                walk(node.getChild(c), childOfEffect);
        };

        for (int c = 0; c < effect.getNumChildren(); ++c)
            walk(effect.getChild(c), effect.getChild(c).getType());

        // 257 today; the floor only says the walk reached the modules at all.
        check(walked > 200 && uncovered == 0,
              "Q: every per-effect property (" + juce::String(walked)
              + " walked, bands and taps included) is either snapshotted or explicitly excluded");

        // The other direction, asked of the node each excluded property LIVES
        // on (any module node answers "covered" for any name, since a module is
        // carried whole) - and an exclusion whose property no node carries any
        // more is a stale claim, so that fails too.
        for (const auto& e : ESS::notSnapshotted())
        {
            const juce::Identifier prop (e.name);
            juce::ValueTree home;
            for (int c = 0; c < effect.getNumChildren() && ! home.isValid(); ++c)
                if (effect.getChild(c).hasProperty(prop))
                    home = effect.getChild(c);

            check(home.isValid() && ! ESS::isEffectPropertyCovered(home.getType(), prop),
                  juce::String("Q: '") + e.name + "' is still deliberately excluded (on <"
                  + (home.isValid() ? home.getType().toString() : juce::String("no node")) + ">)");
        }

        // And the table names nothing the channel lacks: a mistyped property in
        // an item would be carried by no snapshot while the grid offered it.
        int ghosts = 0;
        for (const auto& item : WFSFileManager::effectScopeTable().items)
        {
            if (item.nodeType.isValid())
            {
                if (! effect.getChildWithName(item.nodeType).isValid())
                {
                    ++ghosts;
                    logLine("SELF-TEST FAIL Q: item '" + item.itemId + "' names module node <"
                            + item.nodeType.toString() + "> which the channel does not have");
                }
                continue;
            }

            for (const auto& p : item.parameterIds)
            {
                bool found = false;
                for (int c = 0; c < effect.getNumChildren() && ! found; ++c)
                    found = ESS::isFlatNode(effect.getChild(c).getType()) && effect.getChild(c).hasProperty(p);

                if (! found)
                {
                    ++ghosts;
                    logLine("SELF-TEST FAIL Q: item '" + item.itemId + "' names '" + p.toString()
                            + "', which no flat node of the channel carries");
                }
            }
        }
        check(ghosts == 0, "Q: every effects scope item names a node or property the channel has");

        // The walk above takes a module's word for its properties: a module is
        // carried whole, so the predicate answers "covered" for any name on it.
        // That hides the one mistake a flat node cannot make - a property that
        // is not a setting (a meter, a run-state flag) stamped onto a module
        // node would be stored and recalled like one. So every module property
        // must be one of that module's CSV controls, bands and taps included.
        {
            std::set<juce::String> bandControls, tapControls;
            for (const auto* d : { &EffectsUi::descEQshape(), &EffectsUi::descEQfreq(), &EffectsUi::descEQgain(),
                                   &EffectsUi::descEQq(), &EffectsUi::descEQslope() })
                bandControls.insert(d->id.toString());
            for (const auto* d : { &EffectsUi::descDelayTapTime(), &EffectsUi::descDelayTapLevel() })
                tapControls.insert(d->id.toString());
            const std::set<juce::String> noControls;

            int strays = 0;
            auto vet = [&](const juce::ValueTree& node, const std::set<juce::String>& controls, const juce::String& where)
            {
                for (int i = 0; i < node.getNumProperties(); ++i)
                {
                    const auto prop = node.getPropertyName(i);
                    if (prop == WFSParameterIDs::id || controls.count(prop.toString()) > 0)
                        continue;

                    ++strays;
                    logLine("SELF-TEST FAIL Q: <" + where + "> carries '" + prop.toString() + "', which is not one "
                            "of its module's CSV controls - every snapshot would store and recall it as a setting");
                }
            };

            for (int slot = 0; slot < WFSParameterDefaults::numEffectModuleSlots; ++slot)
            {
                const auto& type = WFSValueTreeState::getEffectModuleType(slot);
                const auto module = effect.getChildWithName(type);
                const auto own = EffectsUi::controlsForSlot(slot);

                std::set<juce::String> ownControls;
                for (int k = 0; k < own.count; ++k)
                    ownControls.insert(own.controls[k].id.toString());

                vet(module, ownControls, type.toString());
                for (int c = 0; c < module.getNumChildren(); ++c)
                {
                    const auto child = module.getChild(c);
                    vet(child,
                        child.hasType(WFSParameterIDs::Band) ? bandControls
                            : child.hasType(WFSParameterIDs::Tap) ? tapControls : noControls,
                        type.toString() + "><" + child.getType().toString());
                }
            }
            check(strays == 0, "Q: every module property is one of its module's CSV controls, bands and taps included");
        }

        if (effectsBefore == 0)
            vts.setNumEffectChannels(0);
    }

    // ---- RP: the reverb's presets are an action, and Custom means edited -----
    // Plan revision 9, section 9. A preset writes its fifteen values and then
    // its type, from every surface; a real edit to one of those fifteen makes
    // the reverb Custom first, on the source and on each linked member; OSC
    // expands without propagating, and a burst reads "preset, then tweaks";
    // snapshot recall writes raw.
    {
        namespace P = WFSParameterIDs;
        namespace FX = spatcore::effects;
        WFSValueTreeState::ScopedUndoDomain undoScope (vts, UndoDomain::Effects);

        const int effectsBefore = vts.getNumEffectChannels();
        vts.setNumEffectChannels(3);

        auto reverbOf = [&](int ch) { return vts.getEffectModuleSection(ch, P::FxReverb); };
        auto num = [](const juce::var& v) { return static_cast<double> (v); };
        auto approxEq = [](double a, double b) { return std::abs(a - b) <= 1.0e-4 * juce::jmax(1.0, std::abs(b)); };
        const int custom = static_cast<int>(FX::ReverbType::Custom);

        // What a row owns, as the tree must hold it after an expansion.
        auto holdsRow = [&](int ch, int type, juce::String& why)
        {
            const auto* row = FX::findReverbPreset(type);
            auto r = reverbOf(ch);
            if (row == nullptr || ! r.isValid()) { why = "no row / no node"; return false; }

            const std::pair<juce::Identifier, double> want[] = {
                { P::effectReverbModel, row->model }, { P::effectReverbERProfile, row->erProfile },
                { P::effectReverbERLevel, row->erLevelDb }, { P::effectReverbPredelay, row->predelayMs },
                { P::effectReverbRT60, row->rt60 }, { P::effectReverbRT60LowMult, row->rt60LowMult },
                { P::effectReverbRT60HighMult, row->rt60HighMult }, { P::effectReverbCrossoverLow, row->crossoverLow },
                { P::effectReverbCrossoverHigh, row->crossoverHigh }, { P::effectReverbDiffusion, row->diffusion },
                { P::effectReverbSize, row->size }, { P::effectReverbModRate, row->modRateHz },
                { P::effectReverbModDepth, row->modDepth }, { P::effectReverbShimmerPitch, row->shimmerPitch },
                { P::effectReverbShimmerAmount, row->shimmerAmount } };

            for (const auto& [id, v] : want)
                if (! approxEq(num(r.getProperty(id)), v))
                {
                    why = id.toString() + " is " + r.getProperty(id).toString() + ", the row says " + juce::String(v);
                    return false;
                }
            if (static_cast<int>(r.getProperty(P::effectReverbType)) != type)
            {
                why = "type is " + r.getProperty(P::effectReverbType).toString();
                return false;
            }
            return true;
        };

        auto typeOf = [&](int ch) { return static_cast<int>(reverbOf(ch).getProperty(P::effectReverbType)); };
        auto gui = [&](int ch, const juce::Identifier& id, const juce::var& v, bool propagate = false)
        {
            vts.setEffectModuleParameterWithLinkPropagation(ch, P::FxReverb, id, v, propagate);
        };

        // RP1: the owned set is the reverb's CSV controls minus bypass, type,
        // tone and mix - and exactly what spatcore's expansion writes (15).
        {
            const auto controls = EffectsUi::controlsForSlot(8);
            int owned = 0, wrong = 0;
            for (int k = 0; k < controls.count; ++k)
            {
                const auto& id = controls.controls[k].id;
                const bool taste = id == P::effectReverbBypass || id == P::effectReverbType
                                || id == P::effectReverbTone || id == P::effectReverbMix;
                const bool isOwned = WFSValueTreeState::isEffectReverbPresetOwned(id);
                owned += isOwned ? 1 : 0;
                if (isOwned == taste)
                {
                    ++wrong;
                    logLine("SELF-TEST FAIL RP1: '" + id.toString() + "' is " + (isOwned ? "" : "not ")
                            + "preset-owned");
                }
            }
            check(wrong == 0 && owned == 15, "RP1: a preset owns the reverb's controls but bypass, type, tone and mix (15)");
        }

        // RP2: the Preset combo is spatcore's table: 23 ids, 5 alone without a row.
        {
            const auto& d = EffectsModulePanel::reverbControl (P::effectReverbType);
            bool table = d.id == P::effectReverbType && static_cast<int>(d.items.size()) == static_cast<int>(FX::ReverbType::Count);
            for (int k = 0; table && k < static_cast<int>(d.items.size()); ++k)
                table = d.items[static_cast<size_t>(k)].value == k
                     && ((k == custom) == (FX::findReverbPreset(k) == nullptr));
            check(table, "RP2: the Preset combo lists spatcore's 23 ids, Custom the only one without a row");
        }

        // RP3: a fresh channel is Medium Hall, and holds it.
        {
            juce::String why;
            check(holdsRow(0, static_cast<int>(FX::ReverbType::MediumHall), why) && typeOf(1) == 6,
                  "RP3: a fresh channel is Medium Hall and holds its row (" + why + ")");
        }

        // RP4: the GUI funnel expands a preset, and one undo takes all of it
        // back - and nothing else: an edit made just before, in the step that
        // was open (a deck turn, an OSC value), must not go with it, so the
        // preset opens a step of its own.
        {
            vts.beginUndoTransaction("self-test: the mix before a preset");
            gui(0, P::effectReverbMix, 41.0);
            gui(0, P::effectReverbType, static_cast<int>(FX::ReverbType::VocalPlate));
            juce::String why;
            check(holdsRow(0, static_cast<int>(FX::ReverbType::VocalPlate), why), "RP4: Vocal Plate lands whole (" + why + ")");

            vts.undo();
            check(holdsRow(0, static_cast<int>(FX::ReverbType::MediumHall), why),
                  "RP4: ...and one undo restores Medium Hall, all fifteen (" + why + ")");
            check(approxEq(num(reverbOf(0).getProperty(P::effectReverbMix)), 41.0),
                  "RP4: ...and only the preset: the edit made just before it stays");

            // The generic funnel - a plain per-channel write, which a single-
            // instance module's property may also take - expands and flips
            // the same way.
            vts.setEffectParameterWithLinkPropagation(0, P::effectReverbType, static_cast<int>(FX::ReverbType::DrumPlate), false);
            const bool expands = holdsRow(0, static_cast<int>(FX::ReverbType::DrumPlate), why);
            vts.setEffectParameterWithLinkPropagation(0, P::effectReverbPredelay, 37.0, false);
            check(expands && typeOf(0) == custom && approxEq(num(reverbOf(0).getProperty(P::effectReverbPredelay)), 37.0),
                  "RP4: the generic funnel expands a preset, and an owned edit through it makes Custom (" + why + ")");
        }

        // RP5: only a REAL edit to an owned value flips; taste never does.
        {
            gui(0, P::effectReverbType, static_cast<int>(FX::ReverbType::VocalPlate));
            gui(0, P::effectReverbRT60, FX::findReverbPreset(static_cast<int>(FX::ReverbType::VocalPlate))->rt60);
            check(typeOf(0) == 15, "RP5: re-sending a preset's own value keeps the preset");
            gui(0, P::effectReverbMix, 44.0);
            gui(0, P::effectReverbTone, 7000.0);
            check(typeOf(0) == 15, "RP5: tone and mix are taste - editing them keeps the preset");
            gui(0, P::effectReverbRT60, 2.3);
            check(typeOf(0) == custom && approxEq(num(reverbOf(0).getProperty(P::effectReverbRT60)), 2.3),
                  "RP5: a real RT60 edit makes it Custom and lands");
            gui(0, P::effectReverbType, 15);
            gui(0, P::effectReverbModel, static_cast<int>(FX::ReverbModel::ModulatedHall));
            check(typeOf(0) == custom, "RP5: so does changing the model by hand");
        }

        // RP6: the Stream Deck's write (EffectParamEdit, the object its dials
        // hold) expands and flips the same way.
        {
            auto& edit = parameters.getEffectEdit();
            edit.writeModule(1, P::FxReverb, P::effectReverbType, static_cast<int>(FX::ReverbType::DarkPlate));
            juce::String why;
            check(holdsRow(1, static_cast<int>(FX::ReverbType::DarkPlate), why), "RP6: a deck preset expands (" + why + ")");
            edit.writeModule(1, P::FxReverb, P::effectReverbSize, 1.55);
            check(typeOf(1) == custom, "RP6: a deck edit to an owned value flips");
        }

        // RP7: a link group - an ABSOLUTE and a RELATIVE member take the exact
        // row (never a delta), a member set OFF takes nothing; then an owned
        // edit flips every member whose own value moved.
        {
            for (int ch = 0; ch < 3; ++ch)
            {
                vts.setEffectParameter(ch, P::effectLinkGroup, 1);
                gui(ch, P::effectReverbType, static_cast<int>(FX::ReverbType::SmallRoom));
            }
            vts.setEffectParameter(0, P::effectLinkMode, 1);    // ABSOLUTE
            vts.setEffectParameter(1, P::effectLinkMode, 2);    // RELATIVE
            vts.setEffectParameter(2, P::effectLinkMode, 0);    // OFF

            gui(1, P::effectReverbRT60, 0.9);                   // member 1 off its row: relative offset
            gui(0, P::effectReverbType, static_cast<int>(FX::ReverbType::StoneCathedral), true);
            juce::String why0, why1, why2;
            check(holdsRow(0, static_cast<int>(FX::ReverbType::StoneCathedral), why0)
                      && holdsRow(1, static_cast<int>(FX::ReverbType::StoneCathedral), why1),
                  "RP7: the ABSOLUTE and the RELATIVE member both hold Stone Cathedral exactly (" + why0 + why1 + ")");
            check(holdsRow(2, static_cast<int>(FX::ReverbType::SmallRoom), why2),
                  "RP7: the member set OFF keeps its own preset (" + why2 + ")");

            gui(0, P::effectReverbDiffusion, 0.66, true);
            check(typeOf(0) == custom && typeOf(1) == custom && typeOf(2) == 7,
                  "RP7: a propagated owned edit flips the members it moved, not the one set OFF");

            for (int ch = 0; ch < 3; ++ch)
                vts.setEffectParameter(ch, P::effectLinkGroup, 0);
        }

        // RP8: OSC - a type expands on its channel alone, and a burst of a
        // preset and a tweak ends Custom in either order (presets drain first).
        if (oscManager != nullptr)
        {
            auto msg = [](const juce::String& address, int effectId, const juce::var& v)
            {
                juce::OSCMessage m { juce::OSCAddressPattern { address } };
                m.addInt32 (effectId);
                if (v.isInt()) m.addInt32 (static_cast<int> (v));
                else           m.addFloat32 (static_cast<float> (static_cast<double> (v)));
                return m;
            };

            vts.setEffectParameter(0, P::effectLinkGroup, 1);
            vts.setEffectParameter(1, P::effectLinkGroup, 1);
            vts.setEffectParameter(0, P::effectLinkMode, 1);
            vts.setEffectParameter(1, P::effectLinkMode, 1);
            gui(1, P::effectReverbType, static_cast<int>(FX::ReverbType::LiveChamber));

            oscManager->receiveBurstForSelfTest({ msg("/wfs/effect/reverbType", 1, static_cast<int>(FX::ReverbType::ShimmerOctave)) });
            juce::String why0, why1;
            check(holdsRow(0, static_cast<int>(FX::ReverbType::ShimmerOctave), why0)
                      && holdsRow(1, static_cast<int>(FX::ReverbType::LiveChamber), why1),
                  "RP8: an OSC preset expands on its channel and reaches no linked member (" + why0 + why1 + ")");

            oscManager->receiveBurstForSelfTest({ msg("/wfs/effect/reverbRT60", 1, 2.5),
                                                  msg("/wfs/effect/reverbType", 1, static_cast<int>(FX::ReverbType::ConcertHall)) });
            const bool tweakFirst = typeOf(0) == custom && approxEq(num(reverbOf(0).getProperty(P::effectReverbRT60)), 2.5)
                                 && static_cast<int>(reverbOf(0).getProperty(P::effectReverbModel)) == 4;
            oscManager->receiveBurstForSelfTest({ msg("/wfs/effect/reverbType", 1, static_cast<int>(FX::ReverbType::ConcertHall)),
                                                  msg("/wfs/effect/reverbRT60", 1, 2.7) });
            const bool presetFirst = typeOf(0) == custom && approxEq(num(reverbOf(0).getProperty(P::effectReverbRT60)), 2.7);
            check(tweakFirst && presetFirst, "RP8: a {tweak, preset} burst ends Custom with the tweak, in either order");

            // A QLab-style replay of a preset's own state lands exactly and
            // stays that preset: the values it re-sends are the row's.
            const auto* lush = FX::findReverbPreset(static_cast<int>(FX::ReverbType::LushHall));
            oscManager->receiveBurstForSelfTest({ msg("/wfs/effect/reverbModel", 1, static_cast<int>(lush->model)),
                                                  msg("/wfs/effect/reverbRT60", 1, lush->rt60),
                                                  msg("/wfs/effect/reverbSize", 1, lush->size),
                                                  msg("/wfs/effect/reverbType", 1, static_cast<int>(FX::ReverbType::LushHall)) });
            check(holdsRow(0, static_cast<int>(FX::ReverbType::LushHall), why0),
                  "RP8: replaying a preset's own values over OSC leaves that preset, unflipped (" + why0 + ")");

            vts.setEffectParameter(0, P::effectLinkGroup, 0);
            vts.setEffectParameter(1, P::effectLinkGroup, 0);
        }
        else
        {
            check(false, "RP8: the OSC manager exists");
        }

        // RP9: a snapshot recall writes raw - no expansion, no flip - even a
        // state the funnels could never have produced (a preset label over a
        // value that is not its row's).
        {
            auto& fm = parameters.getFileManager();
            const auto previousProject = fm.getProjectFolder();
            auto tempProject = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                   .getChildFile("wfs-selftest-reverb-presets");
            tempProject.deleteRecursively();
            fm.setProjectFolder(tempProject);
            fm.createProjectFolderStructure();

            gui(0, P::effectReverbType, static_cast<int>(FX::ReverbType::VocalPlate));
            reverbOf(0).setProperty(P::effectReverbRT60, 3.3, nullptr);        // raw: under the funnels
            const bool stored = fm.saveInputSnapshotWithExtendedScope("rp-raw", WFSFileManager::ExtendedSnapshotScope());

            gui(0, P::effectReverbType, static_cast<int>(FX::ReverbType::ConcertHall));
            const bool recalled = fm.loadInputSnapshotWithExtendedScope("rp-raw", fm.getExtendedSnapshotScope("rp-raw"));
            check(stored && recalled && typeOf(0) == 15
                      && approxEq(num(reverbOf(0).getProperty(P::effectReverbRT60)), 3.3)
                      && static_cast<int>(reverbOf(0).getProperty(P::effectReverbModel)) == 1,
                  "RP9: a recall restores the reverb raw - the preset label, the odd value, the model");

            fm.setProjectFolder(previousProject);
            tempProject.deleteRecursively();
        }

        vts.setNumEffectChannels(effectsBefore);
    }

    // ---- RD: the reverb shows what its model uses, on screen and on the deck --
    // Plan revision 9, section 8. The panel's rows follow the model (the CSV's
    // Models column, the model resolved as the engine resolves it) and never
    // overlap; the deck's Chain page reaches every control of every module
    // exactly once across its banks - Distortion, Delay and Dynamics included,
    // which a twelve-dial page used to cut short - and a deck turn that moves
    // the reverb's model asks for the page to be laid out again.
    {
        namespace P = WFSParameterIDs;
        namespace FX = spatcore::effects;

        const int effectsBefore = vts.getNumEffectChannels();
        vts.setNumEffectChannels(1);
        auto reverb = vts.getEffectModuleSection(0, P::FxReverb);
        const juce::var modelBefore = reverb.getProperty(P::effectReverbModel);

        const auto all = EffectsUi::controlsForReverb();
        auto expectedFor = [&](int storedModel)
        {
            std::vector<juce::Identifier> ids;
            const int m = FX::resolveReverbModel(storedModel);
            for (int k = 0; k < all.count; ++k)
                if (EffectsUi::isVisibleForModel(all.controls[k], m))
                    ids.push_back(all.controls[k].id);
            return ids;
        };

        // RD1 / RD2: the panel, for every stored model id 0..5.
        {
            EffectsTabContext rdCtx (parameters);
            rdCtx.currentChannel = 1;
            EffectsModulePanel panel (rdCtx, 8);
            panel.setSize(1100, 640);

            const int expectedCount[] = { 15, 17, 15, 15, 17, 19 };     // bypass included
            bool rows = true, noOverlap = true, menu = true, presetFirst = true;
            int lowestRow[6] = {};
            for (int model = 0; model <= 5; ++model)
            {
                reverb.setProperty(P::effectReverbModel, model, nullptr);
                panel.loadParameters();
                const auto shown = panel.getShownRowIds();
                const auto preset = std::find(shown.begin(), shown.end(), P::effectReverbType);
                if (preset == shown.end() || std::find(shown.begin(), preset, P::effectReverbModel) != preset)
                {
                    presetFirst = false;
                    logLine("SELF-TEST FAIL RD1: model " + juce::String(model) + " does not show Preset above Model");
                }
                const int menuId = panel.getComboSelectedId(P::effectReverbModel);
                if (menuId != FX::resolveReverbModel(model) + 1)
                {
                    menu = false;
                    logLine("SELF-TEST FAIL RD1: stored model " + juce::String(model) + " shows menu id " + juce::String(menuId));
                }
                if (shown != expectedFor(model) || static_cast<int>(shown.size()) != expectedCount[model])
                {
                    rows = false;
                    logLine("SELF-TEST FAIL RD1: model " + juce::String(model) + " shows "
                            + juce::String(static_cast<int>(shown.size())) + " rows");
                }

                const auto bounds = panel.getShownRowBounds();
                for (size_t a = 0; a < bounds.size(); ++a)
                {
                    lowestRow[model] = juce::jmax (lowestRow[model], bounds[a].getBottom());
                    noOverlap = noOverlap && ! bounds[a].isEmpty();
                    for (size_t b = a + 1; b < bounds.size(); ++b)
                        if (bounds[a].intersects(bounds[b]))
                        {
                            noOverlap = false;
                            logLine("SELF-TEST FAIL RD2: model " + juce::String(model) + ": rows "
                                    + juce::String(static_cast<int>(a)) + " and " + juce::String(static_cast<int>(b)) + " overlap");
                        }
                }
            }
            check(rows, "RD1: the reverb shows its model's rows - 15 FDN, 17 Plate and Hall, 19 Shimmer; 2 and 3 the FDN's");
            check(menu, "RD1: the Model menu names what runs - the FDN for the reserved 2 and 3");
            check(presetFirst, "RD1: Preset sits above Model - a preset sets the model");
            check(noOverlap, "RD2: no two rows overlap, for any model");
            check(lowestRow[0] < lowestRow[5] && lowestRow[1] == lowestRow[4],
                  "RD2: hidden rows take no room - the FDN's columns end above the shimmer's");
        }

        // RD3: the deck's banks reach every control of every module exactly
        // once, in CSV order; for the reverb, every model's own set.
        {
            auto& edit = parameters.getEffectEdit();
            auto chainSlot = std::make_shared<int>(0);
            auto chainBank = std::make_shared<int>(0);
            EffectsTabPages::EffectsCallbacks noCallbacks;

            auto dialNames = [&](int slot)
            {
                // Every bank, in order, collected until the page wraps.
                std::vector<juce::String> names;
                *chainSlot = slot;
                *chainBank = 0;
                for (int guard = 0; guard < 8; ++guard)
                {
                    auto page = EffectsTabPages::createPage(1, vts, edit, 0, nullptr, nullptr, nullptr, chainSlot, chainBank, noCallbacks);
                    for (int s = 1; s < 4; ++s)
                        for (int d = 0; d < 4; ++d)
                            if (page.sections[s].dials[d].setValue != nullptr)
                                names.push_back(page.sections[s].dials[d].paramName);

                    auto& pageButton = page.sections[1].buttons[3];
                    if (pageButton.onPress == nullptr)
                        break;                                  // one bank
                    pageButton.onPress();
                    if (*chainBank == 0)
                        break;                                  // wrapped: every bank seen
                }
                return names;
            };

            auto wanted = [&](int slot)
            {
                std::vector<juce::String> names;
                for (const auto* d : EffectsTabPages::chainPageControls(vts, 0, slot))
                    names.push_back(LOC("effects.labels." + juce::String(d->key)).trimCharactersAtEnd(":"));
                return names;
            };

            bool every = true, banked = true;
            for (int slot = 0; slot < WFSParameterDefaults::numEffectModuleSlots; ++slot)
            {
                const auto got = dialNames(slot);
                if (got != wanted(slot))
                {
                    every = false;
                    logLine("SELF-TEST FAIL RD3: slot " + juce::String(slot) + " reached "
                            + juce::String(static_cast<int>(got.size())) + " of "
                            + juce::String(static_cast<int>(wanted(slot).size())) + " controls");
                }
                if (slot == 0 || slot == 3 || slot == 9)
                    banked = banked && got.size() > 12;         // the three a single page used to cut short
            }
            const std::pair<int, size_t> reverbDials[] = { { 0, 14 }, { 1, 16 }, { 4, 16 }, { 5, 18 } };
            for (const auto& [model, count] : reverbDials)
            {
                reverb.setProperty(P::effectReverbModel, model, nullptr);
                const auto got = dialNames(8);
                every = every && got == wanted(8) && got.size() == count;
            }
            check(every, "RD3: across its banks the deck reaches every control of every module once, and the reverb's per model (14 / 16 / 16 / 18)");
            check(banked, "RD3: ...Distortion, Dynamics and Delay included, past twelve dials");

            // A module change starts at the first bank.
            *chainSlot = 3;
            *chainBank = 1;
            auto page = EffectsTabPages::createPage(1, vts, edit, 0, nullptr, nullptr, nullptr, chainSlot, chainBank, noCallbacks);
            if (page.sections[0].buttons[1].onPress != nullptr)
                page.sections[0].buttons[1].onPress();          // Next module
            check(*chainSlot == 4 && *chainBank == 0, "RD3: the next module opens at its first bank");

            // Prev / Next walk the chain as the strip shows it, not the slot
            // numbers: Distortion is followed by the Bitcrusher, the ends
            // wrap, and a reordered chain is followed at the press.
            {
                auto press = [&](int from, int button)
                {
                    *chainSlot = from;
                    auto p = EffectsTabPages::createPage(1, vts, edit, 0, nullptr, nullptr, nullptr, chainSlot, chainBank, noCallbacks);
                    if (p.sections[0].buttons[button].onPress != nullptr)
                        p.sections[0].buttons[button].onPress();
                    return *chainSlot;
                };
                auto chain = vts.getEffectChainSection(0);
                const juce::var orderBefore = chain.getProperty(P::effectChainOrder);

                chain.setProperty(P::effectChainOrder, WFSParameterDefaults::effectChainOrderDefault, nullptr);
                const bool defaultWalk = press(0, 1) == 10 && press(10, 0) == 0     // dist -> crush -> dist
                                      && press(1, 0) == 8 && press(8, 1) == 1;      // eq1 <- wraps -> reverb
                chain.setProperty(P::effectChainOrder, "crush,delay,reverb,trem,phaser,mod,dyn2,dyn1,eq2,eq1,dist", nullptr);
                const bool reorderedWalk = press(9, 1) == 8 && press(10, 0) == 0;  // delay -> reverb; crush <- wraps -> dist

                chain.setProperty(P::effectChainOrder, orderBefore, nullptr);
                check(defaultWalk && reorderedWalk, "RD3: Prev / Next walk the chain order the strip shows, wrapping at the ends");
            }
        }

        // RD4: a deck turn that moves the reverb's model - the Model dial, or a
        // preset of another model - asks for a relayout; one that does not
        // move it asks for nothing.
        {
            auto& edit = parameters.getEffectEdit();
            auto chainSlot = std::make_shared<int>(8);
            auto chainBank = std::make_shared<int>(0);
            int relayouts = 0;
            EffectsTabPages::EffectsCallbacks cb;
            cb.onModuleLayoutChanged = [&relayouts] { ++relayouts; };

            vts.setEffectModuleParameterWithLinkPropagation(0, P::FxReverb, P::effectReverbType,
                                                            static_cast<int>(FX::ReverbType::MediumHall), false);

            auto findDial = [&](StreamDeckPage& page, const juce::Identifier& id) -> DialBinding*
            {
                juce::String key;
                for (int k = 0; k < all.count; ++k)
                    if (all.controls[k].id == id)
                        key = all.controls[k].key;
                const auto name = LOC("effects.labels." + key).trimCharactersAtEnd(":");
                for (int s = 1; s < 4; ++s)
                    for (int d = 0; d < 4; ++d)
                        if (page.sections[s].dials[d].paramName == name && page.sections[s].dials[d].setValue != nullptr)
                            return &page.sections[s].dials[d];
                return nullptr;
            };

            auto page = EffectsTabPages::createPage(1, vts, edit, 0, nullptr, nullptr, nullptr, chainSlot, chainBank, cb);
            auto* modelDial = findDial(page, P::effectReverbModel);
            bool ok = modelDial != nullptr;
            if (ok)
            {
                modelDial->setValue(1.0f);                      // the Plate: index 1 of FDN, Plate, Hall, Shimmer
                ok = relayouts == 1 && static_cast<int>(reverb.getProperty(P::effectReverbModel)) == 1;
            }
            check(ok, "RD4: turning the deck's Model dial relays the page out");

            page = EffectsTabPages::createPage(1, vts, edit, 0, nullptr, nullptr, nullptr, chainSlot, chainBank, cb);
            relayouts = 0;
            if (auto* presetDial = findDial(page, P::effectReverbType))
            {
                // Index 16 of the combo is id 16, Bright Plate: still a plate.
                presetDial->setValue(static_cast<float>(FX::ReverbType::BrightPlate));
                const bool samePlate = relayouts == 0 && static_cast<int>(reverb.getProperty(P::effectReverbType)) == 16;
                presetDial->setValue(static_cast<float>(FX::ReverbType::ConcertHall));
                check(samePlate && relayouts == 1 && static_cast<int>(reverb.getProperty(P::effectReverbModel)) == 4,
                      "RD4: a deck preset relays out only when its model differs (Bright Plate no, Concert Hall yes)");
            }
            else
            {
                check(false, "RD4: the deck shows the reverb's Preset dial");
            }
        }

        reverb.setProperty(P::effectReverbModel, modelBefore, nullptr);
        vts.setNumEffectChannels(effectsBefore);
    }

    // ---- SD: a Stream Deck gesture is one undo step ----------------------------
    // The GUI's rule for a drag, on the deck: the manager announces each
    // gesture - a run of turns of one dial, a press - and MainComponent opens a
    // step in the active tab's history before its first write. Driven through
    // the manager's own device callbacks on the Effects tab's Channel
    // Parameters page, whose first section has four dials and two toggles; no
    // device is needed (nothing is sent to one).
    if (streamDeckManager != nullptr)
    {
        namespace P = WFSParameterIDs;

        const int effectsBefore = vts.getNumEffectChannels();
        const int mainBefore = streamDeckManager->getCurrentMainTab();
        const int subBefore = streamDeckManager->getCurrentSubTab();
        const int channelBefore = streamDeckManager->getChannel();
        vts.setNumEffectChannels(1);

        {
            WFSValueTreeState::ScopedUndoDomain undoScope (vts, UndoDomain::Effects);
            auto* effectsUndo = vts.getUndoManagerForDomain(UndoDomain::Effects);
            effectsUndo->clearUndoHistory();

            streamDeckManager->syncNavigation(EffectsTabPages::EFFECTS_MAIN_TAB_INDEX, 0, 1);
            streamDeckManager->refreshCurrentPage();
            streamDeckManager->setActiveSection(0);
            auto& dev = streamDeckManager->getDevice();

            auto dial = [this](int d) -> const DialBinding*
            {
                auto* page = streamDeckManager->getCurrentPage();
                return page != nullptr && page->sections[0].dials[d].isValid() ? &page->sections[0].dials[d] : nullptr;
            };
            auto value = [&](int d) { const auto* b = dial(d); return b != nullptr ? b->getValue() : -1.0e9f; };
            auto away = [&](int d) { const auto* b = dial(d); return b != nullptr && b->getValue() + b->step > b->maxValue ? -1 : +1; };
            auto latency = [&] { return static_cast<int>(vts.getEffectParameter(0, P::effectMinimalLatency)); };

            const int dirA = away(0), dirB = away(1);
            const float a0 = value(0), b0 = value(1);
            const int latency0 = latency();

            // Three turns of one dial are one run; another dial is another.
            dev.onDialRotated(0, dirA);
            dev.onDialRotated(0, dirA);
            dev.onDialRotated(0, dirA);
            dev.onDialRotated(1, dirB);
            const float a3 = value(0);
            check(dial(0) != nullptr && dial(1) != nullptr && a3 != a0 && value(1) != b0,
                  "SD: the deck's turns reach their parameters");
            vts.undo();
            const bool lastRunOnly = value(1) == b0 && value(0) == a3;
            vts.undo();
            check(lastRunOnly && value(0) == a0,
                  "SD: one undo takes back one run of turns - the other dial's, then all three of the first");

            // A pause longer than the idle time starts a new step.
            streamDeckManager->setGestureIdleMs(40);
            dev.onDialRotated(0, dirA);
            const float afterFirst = value(0);
            juce::Thread::sleep(120);
            dev.onDialRotated(0, dirA);
            vts.undo();
            const bool pauseSplits = value(0) == afterFirst;
            vts.undo();
            streamDeckManager->setGestureIdleMs(800);
            check(pauseSplits && value(0) == a0, "SD: a pause longer than the idle time starts a new step");

            // Navigation ends a run: the app re-selecting the section splits
            // it, and so does the deck's own section button (device button 0,
            // this page's first section).
            dev.onDialRotated(0, dirA);
            const float beforeSync = value(0);
            streamDeckManager->setActiveSection(0);
            dev.onDialRotated(0, dirA);
            vts.undo();
            const bool syncSplits = value(0) == beforeSync;
            vts.undo();
            check(syncSplits && value(0) == a0, "SD: the app selecting a section ends a run of turns");

            dev.onDialRotated(0, dirA);
            const float beforeButton = value(0);
            dev.onButtonPressed(0);
            dev.onButtonReleased(0);
            dev.onDialRotated(0, dirA);
            vts.undo();
            const bool buttonSplits = value(0) == beforeButton;
            vts.undo();
            check(buttonSplits && value(0) == a0, "SD: the deck's section button ends a run of turns");

            // Each press of a toggle (Minimal Latency, the second button of
            // the section: device button 5) is a step of its own...
            dev.onButtonPressed(5);
            dev.onButtonReleased(5);
            dev.onButtonPressed(5);
            dev.onButtonReleased(5);
            vts.undo();
            const bool pressAlone = latency() != latency0;
            vts.undo();
            check(pressAlone && latency() == latency0, "SD: each press is its own step");

            // ...and a turn right after a press is another, even with nothing
            // between them: the press ended the run.
            dev.onDialRotated(0, dirA);
            const float beforePress = value(0);
            dev.onButtonPressed(5);
            dev.onButtonReleased(5);
            dev.onDialRotated(0, dirA);
            vts.undo();
            const bool turnAlone = value(0) == beforePress && latency() != latency0;
            vts.undo();
            const bool pressNext = value(0) == beforePress && latency() == latency0;
            vts.undo();
            check(turnAlone && pressNext && value(0) == a0, "SD: a turn after a press is a step of its own");

            effectsUndo->clearUndoHistory();
        }

        streamDeckManager->syncNavigation(mainBefore, subBefore, channelBefore);
        vts.setNumEffectChannels(effectsBefore);
    }
    else
    {
        check(false, "SD: the Stream Deck manager exists");
    }

    // ---- SA: every click of a turn counts, and a fast turn goes further --------
    // The deck reports a turning dial every 50 ms with the clicks of that window
    // - up to 16 on a flick. Each click counts; a report of more than a couple
    // multiplies the step, up to the dial's ceiling (StreamDeckDialAcceleration);
    // press + turn stays the exact fine step. Driven through the device
    // callbacks as SD is, on the same page's widest dial, from the middle of its
    // range; the expected moves come from the helper itself, so tuning its
    // constants never breaks the phase.
    {
        // A copied binding keeps every field: the pages copy some they build.
        DialBinding original;
        original.maxAcceleration = 7;
        original.invertDirection = true;
        int presses = 0;
        original.onPress = [&presses] { ++presses; };
        original.altBinding = std::make_unique<DialBinding>();
        original.altBinding->paramName = "alt";
        original.altBinding->maxAcceleration = 3;

        DialBinding copied (original);
        DialBinding assigned;
        assigned = original;
        bool kept = true;
        for (auto* c : { &copied, &assigned })
        {
            kept = kept && c->maxAcceleration == 7 && c->invertDirection && c->onPress != nullptr
                        && c->altBinding != nullptr && c->altBinding.get() != original.altBinding.get()
                        && c->altBinding->paramName == "alt" && c->altBinding->maxAcceleration == 3;
            if (c->onPress != nullptr)
                c->onPress();
        }
        check(kept && presses == 2, "SA: a copied dial binding keeps its cap, its direction, its press and a copy of its alternate");
    }

    if (streamDeckManager != nullptr)
    {
        using Acceleration = spatcore::controllers::StreamDeckDialAcceleration;

        const int effectsBefore = vts.getNumEffectChannels();
        const int mainBefore = streamDeckManager->getCurrentMainTab();
        const int subBefore = streamDeckManager->getCurrentSubTab();
        const int channelBefore = streamDeckManager->getChannel();
        vts.setNumEffectChannels(1);

        {
            WFSValueTreeState::ScopedUndoDomain undoScope (vts, UndoDomain::Effects);
            auto* effectsUndo = vts.getUndoManagerForDomain(UndoDomain::Effects);
            effectsUndo->clearUndoHistory();

            streamDeckManager->syncNavigation(EffectsTabPages::EFFECTS_MAIN_TAB_INDEX, 0, 1);
            streamDeckManager->refreshCurrentPage();
            streamDeckManager->setActiveSection(0);
            auto& dev = streamDeckManager->getDevice();

            auto binding = [this](int d) -> DialBinding*
            {
                auto* page = streamDeckManager->getCurrentPage();
                return page != nullptr && page->sections[0].dials[d].isValid() ? &page->sections[0].dials[d] : nullptr;
            };
            auto ceilingOf = [](const DialBinding& b)
            {
                return Acceleration::ceilingFor (b.maxAcceleration, b.minValue, b.maxValue, b.step, b.isExponential);
            };

            // The widest dial of the section: the one a fast turn speeds up most.
            int d = -1;
            for (int i = 0; i < 4; ++i)
                if (auto* b = binding(i); b != nullptr && b->type != DialBinding::ComboBox
                                          && (d < 0 || ceilingOf(*b) > ceilingOf(*binding(d))))
                    d = i;
            const int ceiling = d >= 0 ? ceilingOf(*binding(d)) : 0;
            check(ceiling >= 3, "SA: the page has a dial wide enough to speed up (ceiling " + juce::String(ceiling) + ")");

            if (ceiling >= 3)
            {
                const float step = binding(d)->step;
                const float mid = 0.5f * (binding(d)->minValue + binding(d)->maxValue);
                auto value = [&] { return binding(d)->getValue(); };
                auto closeTo = [&](float v, float expected) { return std::abs(v - expected) < 0.1f * step; };
                auto from = [&](int steps, bool fine) { return binding(d)->applyStep(steps, fine); };
                auto start = [&]
                {
                    binding(d)->setValue(mid);
                    effectsUndo->clearUndoHistory();
                };

                start();
                float expected = from(1, false);
                dev.onDialRotated(d, 1);
                check(closeTo(value(), expected), "SA: a report of one click moves one step");

                start();
                expected = from(2, false);
                dev.onDialRotated(d, 2);
                check(closeTo(value(), expected), "SA: a report of two clicks moves two steps - no click is lost");

                start();
                const int fastSteps = 12 * Acceleration::multiplier(12, ceiling);
                expected = from(fastSteps, false);
                dev.onDialRotated(d, 12);
                check(fastSteps > 12 && closeTo(value(), expected),
                      "SA: a report of twelve clicks moves " + juce::String(fastSteps) + " steps");

                start();
                expected = from(12, true);
                dev.onDialPressed(d);
                dev.onDialRotated(d, 12);
                dev.onDialReleased(d);
                check(closeTo(value(), expected), "SA: pressed, twelve clicks are twelve fine steps - never faster");

                // The wrong-way turn the operator saw: a flick, then two slow
                // clicks back. Counting reports, not clicks, it came out one
                // step BELOW where it started.
                start();
                dev.onDialRotated(d, 11);
                dev.onDialRotated(d, -1);
                dev.onDialRotated(d, -1);
                check(value() > mid + 8.5f * step, "SA: a flick then two clicks back ends ahead of the start");

                binding(d)->maxAcceleration = 1;
                start();
                expected = from(12, false);
                dev.onDialRotated(d, 12);
                const bool neverFaster = closeTo(value(), expected);
                binding(d)->maxAcceleration = 2;
                start();
                const int cappedSteps = 12 * Acceleration::multiplier(12, 2);
                expected = from(cappedSteps, false);
                dev.onDialRotated(d, 12);
                const bool capped = cappedSteps > 12 && closeTo(value(), expected);
                binding(d)->maxAcceleration = 0;
                check(neverFaster && capped, "SA: a dial's own cap holds - 1 never speeds up, 2 at most doubles");

                start();
                dev.onDialRotated(d, 5);
                dev.onDialRotated(d, 12);
                dev.onDialRotated(d, 3);
                const bool moved = value() > mid + 20.0f * step;
                vts.undo();
                check(moved && closeTo(value(), mid), "SA: one undo takes back a fast run, as it does a slow one");
            }

            effectsUndo->clearUndoHistory();
        }

        streamDeckManager->syncNavigation(mainBefore, subBefore, channelBefore);
        vts.setNumEffectChannels(effectsBefore);
    }
    else
    {
        check(false, "SA: the Stream Deck manager exists");
    }

    // ---- ES: the deck's Effect Sends page holds four effects of one input --------
    // The Inputs tab's Effect Sends sub-tab on the deck: four dials for four
    // effects' send levels from the shown input, four switches under them that
    // keep the level, and a top row moving the window by one or by four. Built
    // and driven without a device, as the RD3 bank checks are.
    {
        const int effectsBefore = vts.getNumEffectChannels();
        vts.setNumEffectChannels(6);

        // The sub-tab itself exists only while the session has effect channels.
        auto inputsBarHasEffectSends = [this]
        {
            if (inputsTab == nullptr)
                return false;
            for (auto* child : inputsTab->getChildren())
                if (auto* bar = dynamic_cast<juce::TabbedButtonBar*>(child))
                    return bar->getTabNames().contains(LOC("inputs.tabs.effectSends"));
            return false;
        };
        check(inputsBarHasEffectSends(), "ES0: with effect channels the Inputs tab shows its Effect Sends sub-tab, last");

        auto window = std::make_shared<int>(0);
        InputsTabPages::EffectSendsCallbacks cb;
        int rebuilds = 0;
        int seenFirst = -1, seenCount = -1;
        cb.requestRebuild  = [&rebuilds] { ++rebuilds; };
        cb.onWindowChanged = [&seenFirst, &seenCount] (int f, int c) { seenFirst = f; seenCount = c; };

        const int slot = 0;
        const int number = vts.getInputChannelNumber(slot);
        auto build = [&]
        {
            return InputsTabPages::createPage(6, vts, parameters.getClusterEdit(), slot,
                                              nullptr, nullptr, nullptr, {}, window, cb);
        };

        auto page = build();
        check(page.numSections == 1 && seenFirst == 0 && seenCount == 4 && page.lcdMessage.isEmpty()
              && page.topRowButtons[0].onPress != nullptr && page.topRowButtons[3].onPress != nullptr,
              "ES1: the Effect Sends page opens on effects 1-4 with its four shift buttons");

        auto& sec = page.sections[0];
        const bool bound = sec.dials[2].setValue != nullptr && sec.dials[2].getValue != nullptr
                        && sec.buttons[2].onPress != nullptr && sec.buttons[2].getState != nullptr;
        check(bound, "ES2: dial 3 and switch 3 are bound");
        if (bound)
        {
            sec.dials[2].setValue(-18.0f);
            check(juce::approximatelyEqual(vts.getEffectSendLevelFromInput(2, number), -18.0f)
                  && juce::approximatelyEqual(sec.dials[2].getValue(), -18.0f)
                  && juce::approximatelyEqual(vts.getEffectSendLevelFromInput(1, number), 0.0f)
                  && juce::approximatelyEqual(vts.getEffectSendLevelFromInput(3, number), 0.0f),
                  "ES2: dial 3 sets effect 3's send from this input, and only that cell");
            sec.buttons[2].onPress();
            check(vts.getEffectSendOnFromInput(2, number) && sec.buttons[2].getState()
                  && juce::approximatelyEqual(vts.getEffectSendLevelFromInput(2, number), -18.0f),
                  "ES2: switch 3 turns the send on and keeps its level");
            sec.buttons[2].onPress();
            check(! vts.getEffectSendOnFromInput(2, number) && ! sec.buttons[2].getState(),
                  "ES2: ...and off again");
        }

        page.topRowButtons[2].onPress();
        check(*window == 1 && rebuilds == 1, "ES3: one step right moves the window to effects 2-5 and asks for the page again");
        page.topRowButtons[3].onPress();
        check(*window == 2 && rebuilds == 2, "ES3: a page right stops at the last four (3-6)");
        page.topRowButtons[0].onPress();
        check(*window == 0 && rebuilds == 3, "ES3: a page left goes back to the first four");
        page.topRowButtons[1].onPress();
        check(*window == 0 && rebuilds == 3, "ES3: at the first effect a step left asks for nothing");

        *window = 2;
        page = build();
        check(seenFirst == 2 && seenCount == 4 && page.sections[0].dials[0].getValue != nullptr
              && juce::approximatelyEqual(page.sections[0].dials[0].getValue(), -18.0f),
              "ES4: after the shift the first dial is effect 3, at the level set above");

        *window = 9;
        page = build();
        check(*window == 2 && seenFirst == 2, "ES4: a window past the end is pulled back to the last four");

        vts.setNumEffectChannels(0);
        page = build();
        check(page.lcdMessage.isNotEmpty() && page.sections[0].dials[0].getValue == nullptr
              && page.sections[0].buttons[0].onPress == nullptr,
              "ES5: without effect channels the page says so and binds nothing");
        check(! inputsBarHasEffectSends(), "ES5: ...and the Inputs tab hides the sub-tab");

        vts.setNumEffectChannels(effectsBefore);
    }

    // ---- SM: one push of the Space Mouse is one undo step --------------------
    // The map's rule - one drag, one step - for the puck, the joysticks and the
    // auto-centering sliders. The manager runs without a device or a message
    // loop: events as a device delivers them, 50 Hz ticks by hand, and the move
    // callbacks swapped for synchronous ones while the test runs (the real ones
    // post their writes to the message loop). The gesture announcement stays
    // MainComponent's own.
    if (controllerManager != nullptr && inputsTab != nullptr && clustersTab != nullptr && mapTab != nullptr
        && parameters.getNumInputChannels() > 0)
    {
        auto& cm = *controllerManager;
        constexpr int testDevice = 9901;
        const int ch = 0;

        const auto savedCallbacks = cm.callbacks;
        const int savedTab = cm.activeTab;
        const bool savedEnabled = cm.isEnabled();

        {
            WFSValueTreeState::ScopedUndoDomain undoScope (vts, UndoDomain::Input);
            auto* inputUndo = vts.getUndoManagerForDomain(UndoDomain::Input);
            inputUndo->clearUndoHistory();

            auto approx = [](float a, float b) { return std::abs(a - b) < 1.0e-4f; };
            auto posX = [&] { return static_cast<float>(parameters.getInputParam(ch, "inputPositionX")); };

            cm.callbacks.moveCurrentChannel = [this, ch](float dx, float dy, float dz) { mapTab->moveInputByDelta(ch, dx, dy, dz); };
            cm.callbacks.moveSelectedDelta = [this, ch](float dx, float dy, float dz) { mapTab->moveInputByDelta(ch, dx, dy, dz); };
            cm.callbacks.getSelectedInputs = [ch] { return std::set<int> { ch }; };
            cm.callbacks.getSelectedClusterRef = [] { return 0; };
            cm.callbacks.rotateSelected = nullptr;
            cm.callbacks.axisDeflection = nullptr;
            cm.callbacks.panMap = nullptr;
            cm.callbacks.zoomMap = nullptr;
            cm.callbacks.fitAllInputs = nullptr;
            cm.callbacks.fitStage = nullptr;
            cm.setEnabled(true);
            cm.activeTab = TabIndex::Inputs;

            ControllerEvent connect;
            connect.type = ControllerEvent::Connected;
            connect.deviceId = testDevice;
            connect.deviceName = "SpaceMouse (self-test)";
            cm.injectEventForTest(connect);

            // Push along X towards the stage centre, so no constraint stops it.
            const float dir = posX() > 0.0f ? -0.5f : 0.5f;
            auto axisX = [&](float v)
            {
                ControllerEvent e;
                e.type = ControllerEvent::AxisMoved;
                e.deviceId = testDevice;
                e.axisOrButton = 0;
                e.value = v;
                cm.injectEventForTest(e);
            };
            auto push = [&](int ticks) { axisX(dir); for (int i = 0; i < ticks; ++i) cm.tickForTest(); };
            auto rest = [&](int ticks) { axisX(0.0f); for (int i = 0; i < ticks; ++i) cm.tickForTest(); };

            rest(ControllerManager::kGestureRestTicks);
            const float x0 = posX();
            push(5);
            const float x1 = posX();
            rest(ControllerManager::kGestureRestTicks);
            push(3);
            const float x2 = posX();
            check(! approx(x1, x0) && ! approx(x2, x1), "SM: a push of the puck moves the input");
            vts.undo();
            const bool secondOnly = approx(posX(), x1);
            vts.undo();
            check(secondOnly && approx(posX(), x0), "SM: one undo takes back one push - the second, then the first");

            // A return to rest shorter than the rest time is the same push.
            rest(ControllerManager::kGestureRestTicks);
            push(4);
            rest(3);
            push(4);
            const float flickered = posX();
            vts.undo();
            check(! approx(flickered, x0) && approx(posX(), x0), "SM: a return to rest shorter than the rest time stays in one step");

            // The same push driving another tab is another step.
            rest(ControllerManager::kGestureRestTicks);
            push(4);
            const float onInputs = posX();
            cm.activeTab = TabIndex::Map;
            push(4);
            vts.undo();
            const bool mapPushOnly = approx(posX(), onInputs);
            vts.undo();
            check(mapPushOnly && approx(posX(), x0), "SM: a change of tab starts a new step");

            rest(1);
            cm.forgetDeviceForTest(testDevice);

            // The joystick and the auto-centering slider fire their gesture
            // hook when pressed...
            {
                WfsJoystickComponent joystick;
                WfsAutoCenterSlider slider { WfsAutoCenterSlider::Orientation::vertical };
                joystick.setSize(100, 100);
                slider.setSize(30, 100);
                int joystickGestures = 0, sliderGestures = 0;
                joystick.onGestureStart = [&] { ++joystickGestures; };
                slider.onGestureStart = [&] { ++sliderGestures; };

                auto press = [](juce::Component& c, juce::Point<float> at)
                {
                    const auto now = juce::Time::getCurrentTime();
                    const juce::MouseEvent e (juce::Desktop::getInstance().getMainMouseSource(), at, juce::ModifierKeys(),
                                              juce::MouseInputSource::defaultPressure, juce::MouseInputSource::defaultOrientation,
                                              juce::MouseInputSource::defaultRotation, juce::MouseInputSource::defaultTiltX,
                                              juce::MouseInputSource::defaultTiltY, &c, &c, now, at, now, 1, false);
                    c.mouseDown(e);
                    c.mouseUp(e);
                };
                press(joystick, { 70.0f, 50.0f });
                press(slider, { 15.0f, 20.0f });
                check(joystickGestures == 1 && sliderGestures == 1,
                      "SM: a press of a joystick or of an auto-centering slider starts a gesture");
            }

            // ...and the tabs open a step with it: a write made before the
            // press survives the undo of the drag.
            {
                auto stepOpenedBy = [&](const std::function<void()>& hook)
                {
                    if (hook == nullptr)
                        return false;
                    const float att0 = static_cast<float>(parameters.getInputParam(ch, "inputAttenuation"));
                    const float att1 = approx(att0, -7.0f) ? -8.0f : -7.0f;
                    vts.beginUndoTransaction("self-test: before the drag");
                    parameters.setInputParam(ch, "inputAttenuation", att1);
                    hook();
                    const float before = posX();
                    parameters.setInputParam(ch, "inputPositionX", before + dir * 0.5f);
                    vts.undo();
                    const bool dragOnly = approx(posX(), before)
                                       && approx(static_cast<float>(parameters.getInputParam(ch, "inputAttenuation")), att1);
                    vts.undo();
                    return dragOnly && approx(static_cast<float>(parameters.getInputParam(ch, "inputAttenuation")), att0);
                };
                check(stepOpenedBy(inputsTab->getPositionJoystickForTest().onGestureStart)
                          && stepOpenedBy(inputsTab->getPositionZSliderForTest().onGestureStart)
                          && stepOpenedBy(clustersTab->getPositionJoystickForTest().onGestureStart),
                      "SM: the Inputs tab's joystick and Z slider and the Clusters tab's joystick each open a step");
            }

            inputUndo->clearUndoHistory();
        }

        cm.callbacks = savedCallbacks;
        cm.activeTab = savedTab;
        cm.setEnabled(savedEnabled);
    }
    else
    {
        check(false, "SM: the controller manager, the Inputs, Clusters and Map tabs and an input exist");
    }

    // ---- N: one snapshot carries the inputs AND the effects ------------------
    // Plan revision 8. Every assertion reads a FILE the store wrote or a value
    // a recall of that file wrote back, never the builder's own output: a
    // store and a recall that agree with each other about a wrong table would
    // round-trip green, which is what Q exists for; N checks the plumbing -
    // that every node KIND survives (flat property, instanced module, band,
    // tap, the two send-row shapes, the per-output row), that the scope is
    // honoured in both directions and on disk, and that each half of a recall
    // lands in its own tab's undo history.
    {
        namespace P = WFSParameterIDs;
        using Scope = WFSFileManager::ExtendedSnapshotScope;

        auto& fm = parameters.getFileManager();
        const auto previousProject = fm.getProjectFolder();
        auto tempProject = juce::File::getSpecialLocation(juce::File::tempDirectory)
                               .getChildFile("wfs-selftest-snapshots-project");
        tempProject.deleteRecursively();
        fm.setProjectFolder(tempProject);
        check(fm.createProjectFolderStructure(), "N0: a throwaway project folder");

        // A store and a recall both latch the channel numbers; put it back.
        auto ioLatch = vts.getIOState();
        const bool latchHadProperty = ioLatch.hasProperty(P::channelNumbersUserOwned);
        const juce::var latchBefore = ioLatch.getProperty(P::channelNumbersUserOwned);

        const int effectsBefore = vts.getNumEffectChannels();
        vts.setNumEffectChannels(2);
        const int numInputs = vts.getNumInputChannels();
        const bool twoOutputs = vts.getNumOutputChannels() >= 2;

        auto snapFile = [&](const juce::String& name)
        {
            return fm.getInputSnapshotsFolder().getChildFile(name + ".xml");
        };

        auto readSnap = [&](const juce::String& name) -> juce::ValueTree
        {
            if (auto xml = juce::XmlDocument::parse(snapFile(name)))
                return juce::ValueTree::fromXml(*xml);
            return {};
        };

        auto writeSnap = [&](const juce::String& name, const juce::ValueTree& tree)
        {
            if (auto xml = tree.createXml())
                return xml->writeTo(snapFile(name));
            return false;
        };

        auto effectEntry = [](const juce::ValueTree& snap, int effectId) -> juce::ValueTree
        {
            auto effects = snap.getChildWithName(P::Effects);
            for (int i = 0; i < effects.getNumChildren(); ++i)
                if (effects.getChild(i).hasType(P::Effect)
                    && effects.getChild(i).getProperty(P::id).toString() == juce::String(effectId))
                    return effects.getChild(i);
            return {};
        };

        auto tokenOf = [](const juce::var& row, int col) -> juce::String
        {
            juce::StringArray t;
            t.addTokens(row.toString(), ",", "");
            return col < t.size() ? t[col].trim() : juce::String();
        };

        auto withToken = [](const juce::var& row, int col, const juce::String& token) -> juce::String
        {
            juce::StringArray t;
            t.addTokens(row.toString(), ",", "");
            if (col < t.size())
                t.set(col, token);
            return t.joinIntoString(",");
        };

        auto num = [](const juce::var& v) { return static_cast<double>(v); };
        auto approx = [](double a, double b) { return std::abs(a - b) < 1.0e-4; };

        auto eq2band3 = [&] { return vts.getEffectEQBand(1, 1, 2); };
        auto dyn2     = [&] { return vts.getEffectDynSection(1, 1); };
        auto tap5     = [&] { return vts.getEffectDelayTap(1, 4); };

        const juce::String orderA = "crush,delay,reverb,trem,phaser,mod,dyn2,dyn1,eq2,eq1,dist";
        const juce::String orderB = "dist,eq1,eq2,dyn1,dyn2,mod,phaser,trem,reverb,delay,crush";

        // Set A is what gets stored; set B disturbs it. Every node kind once:
        // Channel, Position, Feed, Return (a scalar, an array trim and the
        // per-output row), AutomOtion, LFO, Chain, an FxEq2 band, FxDyn2, an
        // FxDelay tap, both send-row shapes - and the name, always carried.
        auto stamp = [&](bool a)
        {
            vts.setEffectParameter(1, P::effectName, a ? "N-fx-2" : "disturbed");
            vts.setEffectParameter(1, P::effectAttenuation, a ? -7.25 : -1.5);
            vts.setEffectParameter(1, P::effectReturnOffsetX, a ? 0.75 : -0.5);
            vts.setEffectParameter(1, P::effectAngleOn, a ? 77 : 40);
            vts.setEffectParameter(1, P::effectArrayAtten3, a ? -4.5 : -0.5);
            vts.setEffectParameter(1, P::effectOtomoR, a ? 3.5 : 1.0);
            vts.setEffectParameter(1, P::effectLFOrateY, a ? 2.5 : 0.5);
            vts.setEffectParameter(1, P::effectChainOrder, a ? orderA : orderB);
            eq2band3().setProperty(P::effectEQgain, a ? 5.5 : -2.0, nullptr);
            dyn2().setProperty(P::effectDynCompThreshold, a ? -31.0 : -12.0, nullptr);
            tap5().setProperty(P::effectDelayTapTime, a ? 123.0 : 45.0, nullptr);
            vts.setEffectSendLevelFromInput(1, 3, a ? -9.5f : -20.0f);
            vts.setEffectFxSendOnFromEffect(1, 0, a);
            if (twoOutputs)
                vts.setEffectParameter(1, P::effectMutes,
                                       withToken(vts.getEffectParameter(1, P::effectMutes), 1, a ? "1" : "0"));
            vts.setEffectParameter(0, P::effectAttenuation, a ? -3.0 : -0.25);
            vts.setEffectSendLevelFromInput(0, 3, a ? -8.0f : -30.0f);
        };

        // Empty when every value is set A (a) or set B (! a); otherwise the
        // names of the ones that are not, so a failure says what it lost.
        auto faults = [&](bool a) -> juce::String
        {
            juce::StringArray bad;
            auto want = [&](bool ok, const char* what) { if (! ok) bad.add(what); };

            want(vts.getEffectParameter(1, P::effectName).toString() == (a ? "N-fx-2" : "disturbed"), "name");
            want(approx(num(vts.getEffectParameter(1, P::effectAttenuation)), a ? -7.25 : -1.5), "attenuation");
            want(approx(num(vts.getEffectParameter(1, P::effectReturnOffsetX)), a ? 0.75 : -0.5), "returnOffsetX");
            want(approx(num(vts.getEffectParameter(1, P::effectAngleOn)), a ? 77 : 40), "angleOn");
            want(approx(num(vts.getEffectParameter(1, P::effectArrayAtten3)), a ? -4.5 : -0.5), "arrayAtten3");
            want(approx(num(vts.getEffectParameter(1, P::effectOtomoR)), a ? 3.5 : 1.0), "otomoR");
            want(approx(num(vts.getEffectParameter(1, P::effectLFOrateY)), a ? 2.5 : 0.5), "lfoRateY");
            want(vts.getEffectParameter(1, P::effectChainOrder).toString() == (a ? orderA : orderB), "chainOrder");
            want(approx(num(eq2band3().getProperty(P::effectEQgain)), a ? 5.5 : -2.0), "EQ2 band 3 gain");
            want(approx(num(dyn2().getProperty(P::effectDynCompThreshold)), a ? -31.0 : -12.0), "Dyn2 threshold");
            want(approx(num(tap5().getProperty(P::effectDelayTapTime)), a ? 123.0 : 45.0), "tap 5 time");
            want(approx(vts.getEffectSendLevelFromInput(1, 3), a ? -9.5 : -20.0), "send from input 3");
            want(vts.getEffectFxSendOnFromEffect(1, 0) == a, "send on from effect 1");
            if (twoOutputs)
                want(tokenOf(vts.getEffectParameter(1, P::effectMutes), 1) == (a ? "1" : "0"), "mutes output 2");
            want(approx(num(vts.getEffectParameter(0, P::effectAttenuation)), a ? -3.0 : -0.25), "effect 1 attenuation");
            want(approx(vts.getEffectSendLevelFromInput(0, 3), a ? -8.0 : -30.0), "effect 1 send from input 3");

            return bad.joinIntoString(", ");
        };

        auto recall = [&](const juce::String& name)
        {
            return fm.loadInputSnapshotWithExtendedScope(name, fm.getExtendedSnapshotScope(name));
        };

        // ---- N1/N2: the whole channel round-trips, every node kind ----------
        stamp(true);
        check(faults(true).isEmpty(), "N0: set A reads back live (" + faults(true) + ")");
        check(fm.saveInputSnapshotWithExtendedScope("nn-full", Scope()), "N1: a full snapshot is stored");
        {
            auto snap = readSnap("nn-full");
            check(snap.hasType("InputSnapshot") && snap.getChildWithName(P::Effects).getNumChildren() == 2
                      && effectEntry(snap, 2).isValid() && snap.getProperty(P::version).toString() == "2.1",
                  "N1: the file is <InputSnapshot version=2.1> with <Effects> holding <Effect id=1> and <Effect id=2>");
        }

        stamp(false);
        check(recall("nn-full"), "N2: the snapshot recalls");
        check(faults(true).isEmpty(), "N2: every node kind came back (" + faults(true) + ")");

        // ---- N3: a partial effects scope, read back off the file -------------
        {
            Scope partial;
            partial.effects.setIncluded("fxEq2", 1, false);
            partial.effects.setIncluded("fxSendsInputs", 0, false);
            check(fm.saveInputSnapshotWithExtendedScope("nn-partial", partial), "N3: a partial-scope snapshot is stored");

            stamp(false);
            check(recall("nn-partial"), "N3: the partial snapshot recalls");
            check(approx(num(eq2band3().getProperty(P::effectEQgain)), -2.0),
                  "N3: an excluded module (EQ 2 on effect 2) stays as it was");
            check(approx(vts.getEffectSendLevelFromInput(0, 3), -30.0),
                  "N3: an excluded flat item (effect 1's sends from inputs) stays as it was");
            check(approx(num(dyn2().getProperty(P::effectDynCompThreshold)), -31.0)
                      && approx(num(vts.getEffectParameter(1, P::effectAttenuation)), -7.25)
                      && approx(num(vts.getEffectParameter(0, P::effectAttenuation)), -3.0),
                  "N3: ...while the included items beside them are recalled");
        }

        // ---- N4: a snapshot from before the effects touches none of them -----
        {
            stamp(true);
            auto snap = readSnap("nn-full");
            snap.removeChild(snap.getChildWithName(P::Effects), nullptr);
            check(writeSnap("nn-old", snap), "N4: an old-format snapshot (no <Effects>) is written");

            stamp(false);
            check(recall("nn-old"), "N4: the old snapshot recalls");
            check(faults(false).isEmpty(), "N4: every effect value stayed as it was (" + faults(false) + ")");
        }

        // ---- N5: entries beyond the live count are skipped and reported ------
        {
            stamp(true);
            vts.setNumEffectChannels(4);
            vts.setEffectParameter(2, P::effectAttenuation, -5.0);
            check(fm.saveInputSnapshotWithExtendedScope("nn-four", Scope()), "N5: a four-effect snapshot is stored");
            vts.setNumEffectChannels(2);

            vts.setEffectParameter(1, P::effectAttenuation, -1.5);
            check(recall("nn-four"), "N5: it recalls into a two-effect session");
            const auto& skipped = fm.getLastRecallSkippedEffectIds();
            check(skipped.size() == 2 && skipped[0] == 3 && skipped[1] == 4,
                  "N5: effects 3 and 4 are reported skipped");
            check(approx(num(vts.getEffectParameter(1, P::effectAttenuation)), -7.25),
                  "N5: ...and effects 1 and 2 are applied");
        }

        // ---- N6: a stored row goes through the store's row guards ------------
        // The file offers effect 1 a send from ITSELF (the diagonal) and from
        // effect 2. The diagonal must come back off - it is forced where the row
        // is written - and the real send must come back on, which proves the
        // row was written at all.
        {
            auto snap = readSnap("nn-full");
            auto sends = effectEntry(snap, 1).getChildWithName(P::Sends);
            const auto crafted = withToken(withToken(sends.getProperty(P::effectFxSendOns), 0, "1"), 1, "1");
            sends.setProperty(P::effectFxSendOns, crafted, nullptr);
            check(writeSnap("nn-diag", snap), "N6: a snapshot with a self-send in effect 1's row is written");

            vts.setEffectFxSendOnFromEffect(0, 1, false);
            check(recall("nn-diag"), "N6: it recalls");
            const auto row = vts.getEffectParameter(0, P::effectFxSendOns);
            check(tokenOf(row, 1) == "1", "N6: the send from effect 2 came back on (the row was written)");
            check(tokenOf(row, 0) == "0", "N6: the self-send came back OFF (the row went through the guard)");
        }

        // ---- N7: monitoring and run-state never reach the file ---------------
        {
            vts.setEffectParameter(0, P::effectSolo, 1);
            vts.getEffectAutoMotionSection(0).setProperty(P::effectOtomoPauseResume, 0, nullptr);
            check(fm.saveInputSnapshotWithExtendedScope("nn-transient", Scope()), "N7: a snapshot is stored");
            const auto text = snapFile("nn-transient").loadFileAsString();
            check(text.contains("<Effect ") && ! text.contains("effectSolo") && ! text.contains("effectOtomoPauseResume"),
                  "N7: effectSolo and effectOtomoPauseResume are in no snapshot");
            vts.setEffectParameter(0, P::effectSolo, 0);
            vts.getEffectAutoMotionSection(0).setProperty(P::effectOtomoPauseResume,
                                                          WFSParameterDefaults::effectOtomoPauseResumeDefault, nullptr);
        }

        // ---- N8: the effects grid survives the file --------------------------
        {
            Scope s;
            s.setIncluded("position", 0, false);
            s.effects.setAllItemsForChannel(0, false);
            s.effects.setIncluded("fxDist", 1, false);
            s.effects.setIncluded("fxLfoX", 1, false);
            check(fm.updateInputSnapshotScope("nn-full", s), "N8: a scope with an effects grid is written into a snapshot");

            const auto back = fm.getExtendedSnapshotScope("nn-full");
            check(back.isEquivalentTo(s, numInputs, 2), "N8: it reads back equivalent, both grids");
            check(back.effects.getChannelState(0) == Scope::InclusionState::AllExcluded
                      && ! back.effects.isIncluded("fxDist", 1) && ! back.effects.isIncluded("fxLfoX", 1)
                      && back.effects.isIncluded("fxEq1", 1) && ! back.isIncluded("position", 0),
                  "N8: ...cell for cell: effect 1 out, effect 2 partial, the input cell kept");
        }

        // ---- N9: an OnSave scope trims the stored effects --------------------
        {
            check(fm.saveInputSnapshotWithExtendedScope("nn-trim", Scope()), "N9: a full snapshot to trim");

            Scope t;
            t.applyMode = Scope::ApplyMode::OnSave;
            t.effects.setIncluded("fxEq2", 0, false);
            t.effects.setIncluded("fxLevel", 0, false);
            check(fm.updateInputSnapshotScope("nn-trim", t), "N9: re-scoped to OnSave");

            auto snap = readSnap("nn-trim");
            auto e1 = effectEntry(snap, 1);
            auto e2 = effectEntry(snap, 2);
            check(e1.isValid() && ! e1.getChildWithName(P::FxEq2).isValid() && e1.getChildWithName(P::FxEq1).isValid(),
                  "N9: effect 1 lost its EQ 2 and kept its EQ 1");
            check(e1.getChildWithName(P::Channel).hasProperty(P::effectName)
                      && ! e1.getChildWithName(P::Channel).hasProperty(P::effectAttenuation),
                  "N9: effect 1 kept its name and lost its attenuation");
            check(e2.getChildWithName(P::FxEq2).isValid() && e2.getChildWithName(P::Channel).hasProperty(P::effectAttenuation),
                  "N9: effect 2 was not touched");

            Scope u;
            u.applyMode = Scope::ApplyMode::OnSave;
            u.effects.setIncluded("fxDelay", 1, false);
            check(fm.saveInputSnapshotWithExtendedScope("nn-onsave", u), "N9: an OnSave snapshot is stored");
            auto stored = readSnap("nn-onsave");
            check(! effectEntry(stored, 2).getChildWithName(P::FxDelay).isValid()
                      && effectEntry(stored, 1).getChildWithName(P::FxDelay).isValid(),
                  "N9: an OnSave store leaves out what the grid excludes, for that channel only");
        }

        // ---- N10: each half of a recall lands in its own tab's undo history ---
        {
            stamp(true);
            check(fm.saveInputSnapshotWithExtendedScope("nn-undo", Scope()), "N10: a snapshot is stored");

            auto inputChannel = vts.getInputChannelSection(0);
            const juce::var storedInputAtten = inputChannel.getProperty(P::inputAttenuation);

            auto disturbBoth = [&]
            {
                inputChannel.setProperty(P::inputAttenuation, -33.0, nullptr);
                vts.getEffectChannelSection(1).setProperty(P::effectAttenuation, -1.5, nullptr);
            };

            disturbBoth();
            vts.clearAllUndoHistories();
            check(fm.loadInputSnapshotWithExtendedScope("nn-undo", Scope()), "N10: a manual recall");
            check(vts.getUndoManagerForDomain(UndoDomain::Input)->canUndo()
                      && vts.getUndoManagerForDomain(UndoDomain::Effects)->canUndo(),
                  "N10: a manual recall is undoable on the Inputs tab AND on the Effects tab");

            disturbBoth();
            vts.clearAllUndoHistories();
            {
                WFSValueTreeState::ScopedUndoSuppression noUndo (vts);
                check(fm.loadInputSnapshotWithExtendedScope("nn-undo", Scope()), "N10: a cue-driven recall");
            }
            check(! vts.getUndoManagerForDomain(UndoDomain::Input)->canUndo()
                      && ! vts.getUndoManagerForDomain(UndoDomain::Effects)->canUndo(),
                  "N10: a cue-driven recall writes no undo entry in either");
            check(approx(num(vts.getEffectParameter(1, P::effectAttenuation)), -7.25)
                      && inputChannel.getProperty(P::inputAttenuation).toString() == storedInputAtten.toString(),
                  "N10: ...and still applied both halves");
            vts.clearAllUndoHistories();
        }

        // ---- N14: a ghost's scope travels with its data ----------------------
        // The file keeps an <Effect> the session lacks today (N5), so it has to
        // keep that effect's column of the grid too. Shrink the count, write the
        // scope back while the effects are absent, grow the count again: what the
        // operator excluded must still be excluded, and the recall must say so.
        {
            vts.setNumEffectChannels(4);
            vts.setEffectParameter(2, P::effectAttenuation, -6.0);
            vts.setEffectParameter(3, P::effectAttenuation, -6.0);

            Scope g;
            g.effects.setAllItemsForChannel(3, false);
            g.effects.setIncluded("fxEq2", 2, false);
            check(fm.saveInputSnapshotWithExtendedScope("nn-ghost", g),
                  "N14: a four-effect snapshot with effect 4 excluded and effect 3 partial");

            vts.setNumEffectChannels(2);
            const auto shrunk = fm.getExtendedSnapshotScope("nn-ghost");
            check(! shrunk.effects.isIncluded("fxLevel", 3) && ! shrunk.effects.isIncluded("fxEq2", 2)
                      && shrunk.effects.isIncluded("fxEq1", 2),
                  "N14: read into a two-effect session, the scope still holds effects 3 and 4's cells");
            check(fm.updateInputSnapshotScope("nn-ghost", shrunk), "N14: the scope is written back while they are absent");

            vts.setNumEffectChannels(4);
            const auto regrown = fm.getExtendedSnapshotScope("nn-ghost");
            check(regrown.effects.getChannelState(3) == Scope::InclusionState::AllExcluded
                      && ! regrown.effects.isIncluded("fxEq2", 2) && regrown.effects.isIncluded("fxEq1", 2),
                  "N14: grown back to four, effect 4 is still out and effect 3 still partial");

            vts.setEffectParameter(2, P::effectAttenuation, -1.0);
            vts.setEffectParameter(3, P::effectAttenuation, -1.0);
            check(recall("nn-ghost"), "N14: it recalls");
            check(approx(num(vts.getEffectParameter(2, P::effectAttenuation)), -6.0)
                      && approx(num(vts.getEffectParameter(3, P::effectAttenuation)), -1.0),
                  "N14: ...recalling effect 3 and leaving the excluded effect 4 alone");

            // A Store over the same name, with a scope built for two effects,
            // says nothing about effects 3 and 4: their columns come over with
            // the data the store carries for them.
            vts.setNumEffectChannels(2);
            check(fm.saveInputSnapshotWithExtendedScope("nn-ghost", Scope()),
                  "N14: stored over with a fresh scope while effects 3 and 4 are absent");
            vts.setNumEffectChannels(4);
            const auto overwritten = fm.getExtendedSnapshotScope("nn-ghost");
            check(overwritten.effects.getChannelState(3) == Scope::InclusionState::AllExcluded
                      && ! overwritten.effects.isIncluded("fxEq2", 2) && overwritten.effects.isIncluded("fxEq1", 2)
                      && overwritten.effects.getChannelState(0) == Scope::InclusionState::AllIncluded,
                  "N14: ...effect 4 is still out and effect 3 still partial, the live effects as the new scope says");
            vts.setNumEffectChannels(2);
        }

        // ---- N15: a template with no effects grid leaves the effects grid ----
        // A template saved before the effects existed has no <EffectsScope>; it
        // has no opinion about them, so loading it must not include them all.
        {
            Scope both;
            both.setIncluded("position", 0, false);
            both.effects.setIncluded("fxDist", 1, false);
            check(fm.saveScopeTemplate("nn-tpl-both", both), "N15: a template with both grids is saved");

            auto templateFile = [&](const juce::String& name)
            {
                return fm.getScopeTemplatesFolder().getChildFile(name + WFSFileManager::snapshotExtension);
            };
            juce::ValueTree tpl;
            if (auto xml = juce::XmlDocument::parse(templateFile("nn-tpl-both")))
                tpl = juce::ValueTree::fromXml(*xml);
            auto scopeTree = tpl.getChildWithName("ExtendedScope");
            check(scopeTree.getChildWithName("EffectsScope").isValid(), "N15: ...and it carries <EffectsScope>");
            scopeTree.removeChild(scopeTree.getChildWithName("EffectsScope"), nullptr);
            auto oldXml = tpl.createXml();
            check(oldXml != nullptr && oldXml->writeTo(templateFile("nn-tpl-inputs")),
                  "N15: the same template as a build without effects wrote it");

            Scope target;
            target.effects.setIncluded("fxChain", 0, false);
            check(fm.loadScopeTemplateGrid("nn-tpl-inputs", target), "N15: the inputs-only template loads");
            check(! target.isIncluded("position", 0) && ! target.effects.isIncluded("fxChain", 0)
                      && target.effects.isIncluded("fxDist", 1),
                  "N15: it sets the input grid and leaves the effects grid as it was");

            check(fm.loadScopeTemplateGrid("nn-tpl-both", target), "N15: the template with both grids loads");
            check(target.effects.isIncluded("fxChain", 0) && ! target.effects.isIncluded("fxDist", 1),
                  "N15: ...and that one replaces the effects grid");
        }

        // Leave nothing behind.
        vts.setNumEffectChannels(effectsBefore);
        if (latchHadProperty)
            ioLatch.setProperty(P::channelNumbersUserOwned, latchBefore, nullptr);
        else
            ioLatch.removeProperty(P::channelNumbersUserOwned, nullptr);
        fm.setProjectFolder(previousProject);
        tempProject.deleteRecursively();
    }

    // ---- N11: an effect edit marks ITS effects scope item dirty --------------
    // What the Scope window's "auto-preselect modified" and "Select modified"
    // read. A module is one item whatever node inside it moved - a band reports
    // its EQ instance, never the other one - and monitoring state marks nothing.
    {
        namespace P = WFSParameterIDs;
        auto& tracker = parameters.getDirtyTracker();

        const int effectsBefore = vts.getNumEffectChannels();
        vts.setNumEffectChannels(2);
        tracker.endSuppressionAndClear();

        vts.getEffectModuleSection(1, P::FxDist).setProperty(P::effectDistDrive, 7.0, nullptr);
        check(tracker.isDirty("fxDist", 1) && ! tracker.isDirty("fxDist", 0),
              "N11: a module write marks that module, on that effect only");

        vts.getEffectEQBand(1, 1, 2).setProperty(P::effectEQgain, 3.0, nullptr);
        check(tracker.isDirty("fxEq2", 1) && ! tracker.isDirty("fxEq1", 1),
              "N11: a band write on EQ 2 marks EQ 2, not EQ 1");

        vts.setEffectParameter(1, P::effectAttenuation, -2.0);
        check(tracker.isDirty("fxLevel", 1), "N11: a flat write marks its property item");

        tracker.clearAll();
        vts.setEffectParameter(1, P::effectSolo, 1);
        check(! tracker.hasAnyDirty(), "N11: effectSolo marks nothing");
        vts.setEffectParameter(1, P::effectSolo, 0);

        tracker.clearAll();
        vts.setNumEffectChannels(effectsBefore);
    }

    // ---- N12: the QLab export carries the effects, each value in its own shape --
    // Built from a stored file, as the export reads it, and every effect cue sent
    // back through the /wfs/effect/ parser the way QLab would send it: a cue in
    // the wrong shape is a cue that does nothing on show night.
    {
        namespace P = WFSParameterIDs;
        using Scope = WFSFileManager::ExtendedSnapshotScope;

        auto& fm = parameters.getFileManager();
        const auto previousProject = fm.getProjectFolder();
        auto tempProject = juce::File::getSpecialLocation(juce::File::tempDirectory)
                               .getChildFile("wfs-selftest-qlab-effects-project");
        tempProject.deleteRecursively();
        fm.setProjectFolder(tempProject);
        check(fm.createProjectFolderStructure(), "N12: a throwaway project folder");

        auto ioLatch = vts.getIOState();
        const bool latchHadProperty = ioLatch.hasProperty(P::channelNumbersUserOwned);
        const juce::var latchBefore = ioLatch.getProperty(P::channelNumbersUserOwned);

        const int effectsBefore = vts.getNumEffectChannels();
        vts.setNumEffectChannels(2);
        vts.getEffectEQBand(1, 1, 2).setProperty(P::effectEQgain, 5.5, nullptr);
        vts.getEffectDynSection(1, 1).setProperty(P::effectDynCompThreshold, -31.0, nullptr);
        vts.getEffectDelayTap(1, 4).setProperty(P::effectDelayTapTime, 123.0, nullptr);
        vts.setEffectSendLevelFromInput(1, 3, -9.5f);

        check(fm.saveInputSnapshotWithExtendedScope("nq-cues", Scope()), "N12: a snapshot to export");

        juce::ValueTree inputsData, effectsData;
        if (auto xml = juce::XmlDocument::parse(fm.getInputSnapshotsFolder().getChildFile("nq-cues.xml")))
        {
            const auto snap = juce::ValueTree::fromXml(*xml);
            inputsData = snap.getChildWithName(P::Inputs);
            effectsData = snap.getChildWithName(P::Effects);
        }
        check(effectsData.isValid(), "N12: the stored snapshot has its <Effects>");

        const int numInputs = vts.getNumInputChannels();
        const int numOutputs = vts.getNumOutputChannels();
        const auto numberToSlot = [&vts](int number) { return vts.getSlotForChannelNumber(number); };

        auto effectCueStrings = [](const WFSNetwork::QLabCueSequence& sequence)
        {
            juce::StringArray out;
            for (const auto& cue : sequence.networkCues)
                for (const auto& m : cue.messages)
                    if (m.getAddressPattern().toString() == "/cue/selected/customString"
                        && m.size() > 0 && m[0].isString() && m[0].getString().startsWith("/wfs/effect/"))
                        out.add(m[0].getString());
            return out;
        };
        auto hasPrefix = [](const juce::StringArray& strings, const juce::String& prefix)
        {
            for (const auto& s : strings)
                if (s.startsWith(prefix))
                    return true;
            return false;
        };

        // What QLab does with a custom string: split at spaces outside quotes,
        // send a quoted token as a string, a bare number as a number.
        auto sendLikeQLab = [](const juce::String& customString)
        {
            std::vector<std::pair<juce::String, bool>> tokens;
            juce::String current;
            bool inQuotes = false, quoted = false;
            for (int ci = 0; ci < customString.length(); ++ci)
            {
                const auto c = customString[ci];
                if (c == '"') { inQuotes = ! inQuotes; quoted = true; continue; }
                if (c == ' ' && ! inQuotes)
                {
                    if (current.isNotEmpty() || quoted) tokens.push_back({ current, quoted });
                    current.clear(); quoted = false;
                    continue;
                }
                current += c;
            }
            if (current.isNotEmpty() || quoted) tokens.push_back({ current, quoted });

            juce::OSCMessage msg (juce::OSCAddressPattern (tokens.empty() ? juce::String("/") : tokens.front().first));
            for (size_t i = 1; i < tokens.size(); ++i)
            {
                const auto& [text, wasQuoted] = tokens[i];
                const bool isInt = ! wasQuoted && text.isNotEmpty()
                                   && text.trimCharactersAtStart("-+").containsOnly("0123456789")
                                   && text.trimCharactersAtStart("-+").isNotEmpty();
                const bool isFloat = ! wasQuoted && ! isInt && text.containsAnyOf("0123456789")
                                     && text.containsOnly("0123456789.-+eE");
                if (isInt)        msg.addInt32(text.getIntValue());
                else if (isFloat) msg.addFloat32(text.getFloatValue());
                else              msg.addString(text);
            }
            return msg;
        };

        const Scope all;
        const auto sequence = WFSNetwork::QLabCueBuilder::buildSnapshotCues("nq-cues", inputsData, all, numInputs, 1,
                                                                            numberToSlot, numOutputs, effectsData, 2);
        const auto strings = effectCueStrings(sequence);

        check(hasPrefix(strings, "/wfs/effect/EQgain 2 2 3 5.5"),
              "N12: an EQ band cue is <ID> <instance> <band> <value> (effect 2, EQ 2, band 3)");
        check(hasPrefix(strings, "/wfs/effect/dynCompThreshold 2 2 -31"),
              "N12: a dynamics cue is <ID> <instance> <value>");
        check(hasPrefix(strings, "/wfs/effect/delayTapTime 2 5 123"),
              "N12: a delay tap cue is <ID> <tap> <value>");
        check(hasPrefix(strings, "/wfs/effect/sendLevels 2 \"") && hasPrefix(strings, "/wfs/effect/chainOrder 1 \""),
              "N12: a row goes out whole, as one quoted string");
        check(WFSNetwork::QLabCueBuilder::countCues(inputsData, all, numInputs, numberToSlot, effectsData, 2, numOutputs)
                  == static_cast<int>(sequence.networkCues.size()),
              "N12: countCues agrees with the " + juce::String(static_cast<int>(sequence.networkCues.size())) + " cues built");

        int parsedBack = 0;
        juce::StringArray refused;
        const auto& mappings = WFSNetwork::OSCMessageBuilder::getEffectMappings();
        for (const auto& s : strings)
        {
            const auto msg = sendLikeQLab(s);
            const auto parsed = WFSNetwork::OSCMessageRouter::parseEffectMessage(msg);
            const auto it = mappings.find(parsed.paramId);
            if (parsed.valid && it != mappings.end() && it->second.oscPath == msg.getAddressPattern().toString())
                ++parsedBack;
            else if (refused.size() < 8)
                refused.add(s + (parsed.invalidReason.isNotEmpty() ? " (" + parsed.invalidReason + ")" : ""));
        }
        check(strings.size() > 200 && parsedBack == strings.size(),
              "N12: every one of the " + juce::String(strings.size()) + " effect cues parses back through the router"
              + (refused.isEmpty() ? juce::String() : ": " + refused.joinIntoString(" | ")));

        // The other direction: every value the file stores gets its cue. The
        // builder skips a parameter the address map lacks in silence, so the
        // check above - which only sees the cues that were built - cannot miss
        // one; a count can.
        {
            int stored = 0;
            juce::StringArray unaddressed;
            std::function<void (const juce::ValueTree&)> tally = [&](const juce::ValueTree& node)
            {
                for (int p = 0; p < node.getNumProperties(); ++p)
                {
                    const auto prop = node.getPropertyName(p);
                    if (prop == P::id || prop == P::effectName)
                        continue;
                    ++stored;
                    if (mappings.find(prop) == mappings.end())
                        unaddressed.addIfNotAlreadyThere(prop.toString());
                }
                for (int c = 0; c < node.getNumChildren(); ++c)
                    tally(node.getChild(c));
            };
            tally(effectsData);

            int oneTokenRows = 0;
            const auto built = WFSNetwork::QLabCueBuilder::collectEffectCues(effectsData, all.effects, 2, numOutputs, &oneTokenRows);
            check(stored > 400 && stored == static_cast<int>(built.size()) + oneTokenRows && unaddressed.isEmpty(),
                  "N12: every one of the " + juce::String(stored) + " stored effect values gets a cue"
                  + (unaddressed.isEmpty() ? juce::String() : " - no address for " + unaddressed.joinIntoString(", ")));
        }

        // The grid decides what is exported, per item and per channel.
        Scope partial;
        partial.effects.setIncluded("fxEq2", 1, false);
        const auto partialStrings = effectCueStrings(WFSNetwork::QLabCueBuilder::buildSnapshotCues(
            "nq-cues", inputsData, partial, numInputs, 1, numberToSlot, numOutputs, effectsData, 2));
        check(! hasPrefix(partialStrings, "/wfs/effect/EQgain 2 2 ") && ! hasPrefix(partialStrings, "/wfs/effect/EQBypass 2 2 ")
                  && hasPrefix(partialStrings, "/wfs/effect/EQgain 2 1 ") && hasPrefix(partialStrings, "/wfs/effect/EQgain 1 2 "),
              "N12: an excluded module (EQ 2 of effect 2) exports nothing, its neighbours still do");

        // N13: one address per parameter, and none for a cell or a global.
        juce::StringArray paths;
        for (const auto& [paramId, mapping] : mappings)
            paths.add(mapping.oscPath);
        paths.removeDuplicates(false);
        check(static_cast<int>(mappings.size()) == paths.size()
                  && mappings.count(P::effectSendLevel) == 0 && mappings.count(P::effectFxSendOn) == 0
                  && mappings.count(P::effectsMapVisible) == 0 && mappings.count(P::effectEQgain) == 1,
              "N13: the effect address map has one path per parameter and none for a cell or a global");

        vts.setNumEffectChannels(effectsBefore);
        if (latchHadProperty)
            ioLatch.setProperty(P::channelNumbersUserOwned, latchBefore, nullptr);
        else
            ioLatch.removeProperty(P::channelNumbersUserOwned, nullptr);
        fm.setProjectFolder(previousProject);
        tempProject.deleteRecursively();
    }

    // ---- N16: dismissing the Scope window keeps the QLab toggles -------------
    // Cancel and the close box hand the session the window's DEFAULTS (both
    // off), not what its toggles showed; adopting them switched Write to QLab
    // off every time the window was dismissed. Driven through the real session
    // and window, as the Effects tab's Edit Scope opens it.
    if (snapshotSession != nullptr)
    {
        namespace P = WFSParameterIDs;
        auto show = vts.getConfigState().getChildWithName(P::Show);
        const bool hadQLab = show.hasProperty(P::writeToQLab);
        const bool hadLoadCue = show.hasProperty(P::writeSnapshotLoadCue);
        const juce::var qlabBefore = show.getProperty(P::writeToQLab);
        const juce::var loadCueBefore = show.getProperty(P::writeSnapshotLoadCue);
        show.setProperty(P::writeToQLab, true, nullptr);
        show.setProperty(P::writeSnapshotLoadCue, true, nullptr);

        snapshotSession->editScope(WFSFileManager::SnapshotFamily::Effects);
        SnapshotScopeWindow* window = nullptr;
        for (int i = juce::Desktop::getInstance().getNumComponents(); --i >= 0 && window == nullptr;)
            window = dynamic_cast<SnapshotScopeWindow*>(juce::Desktop::getInstance().getComponent(i));
        check(window != nullptr && window->isVisible(), "N16: Edit Scope opens the Scope window");
        if (window != nullptr)
            window->closeButtonPressed();

        check(show.isValid() && (bool) show.getProperty(P::writeToQLab) && (bool) show.getProperty(P::writeSnapshotLoadCue),
              "N16: closing it with X leaves Write to QLab and the load cue as they were (on)");

        if (hadQLab) show.setProperty(P::writeToQLab, qlabBefore, nullptr);
        else         show.removeProperty(P::writeToQLab, nullptr);
        if (hadLoadCue) show.setProperty(P::writeSnapshotLoadCue, loadCueBefore, nullptr);
        else            show.removeProperty(P::writeSnapshotLoadCue, nullptr);
    }

    // ---- I: channel identity gate --------------------------------------------
    // Position is not identity: a file's <Input> entries merge BY NUMBER, the
    // inventory rebuilds the list BY NUMBER, and patch rows land BY POSITION.
    // These phases pin down what the gate must see, refuse, and repair.
    {
        using Rel  = InputChannelIdentityDiff::Relation;
        using Kind = WFSFileManager::LoadKind;
        auto& fm = parameters.getFileManager();
        auto tmp = [] (const char* name)
        {
            return juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile(name);
        };
        auto F = tmp("wfs-selftest-identity-system.xml");
        auto G = tmp("wfs-selftest-identity-inputs.xml");
        F.deleteFile(); G.deleteFile();

        // A latched, mixed, reordered list.
        vts.setInputChannelCounts(5, 0);
        reconfig();
        check(vts.addInputChannel(true).wasOk(), "I0: add a stereo pair");
        reconfig();
        const int stereoNum = vts.getInputChannelNumber(vts.getNumInputChannels() - 1);
        check(vts.moveInputChannel(stereoNum, 1).wasOk(), "I0: drag it to slot 1");
        reconfig();
        check(vts.areChannelNumbersUserOwned(), "I0: the session is latched");

        check(fm.exportSystemConfig(F), "I1: export the system config");
        check(fm.exportInputConfig(G), "I1: export the input config");
        check(fm.preflightChannelIdentity(F, Kind::systemConfig).relation == Rel::identical,
              "I1: the system file is identical to the session");
        {
            const auto d = fm.preflightChannelIdentity(G, Kind::inputConfig);
            check(d.relation == Rel::identical && d.patchDiffers.empty(),
                  "I1: the input file is identical and its fingerprints match the live patch");
        }
        check(G.loadFileAsString().contains("hwInputs="), "I1: the input file carries hardware fingerprints");

        // I2: same channels, different order - safe for a system config.
        const int firstNum = vts.getInputChannelNumber(0);
        check(vts.moveInputChannel(firstNum, vts.getNumInputChannels() - 1).wasOk(), "I2: move the first channel to the end");
        reconfig();
        {
            const auto d = fm.preflightChannelIdentity(F, Kind::systemConfig);
            check(d.relation == Rel::orderOnly, "I2: the system file now differs by order only");
            check(fm.isChannelIdentitySafe(d, Kind::systemConfig), "I2: order-only is safe for a system config");
        }
        check(fm.importSystemConfig(F), "I2: it loads without clearance");
        reconfig();
        check(vts.getInputChannelNumber(0) == firstNum, "I2: the file's order came back");

        // I5: inputs config, order only: the patch row must follow its channel.
        check(vts.moveInputChannel(firstNum, vts.getNumInputChannels() - 1).wasOk(), "I5: move it to the end again");
        reconfig();
        const auto p5 = patchOfNumber(firstNum);
        {
            const auto d = fm.preflightChannelIdentity(G, Kind::inputConfig);
            check(d.relation == Rel::orderOnly && d.patchDiffers.empty(), "I5: the input file differs by order only");
            check(fm.isChannelIdentitySafe(d, Kind::inputConfig), "I5: safe, because the rows are aligned with the nodes");
        }
        check(fm.importInputConfig(G), "I5: reload the input config");
        reconfig();
        check(vts.getInputChannelNumber(0) == firstNum, "I5: the file's order came back");
        check(patchOfNumber(firstNum) == p5, "I5: the moved channel kept its hardware inputs - the row followed the node");

        // I3: a type conflict is refused by the primitive itself.
        int monoNum = -1;
        for (int slot = 0; slot < vts.getNumInputChannels(); ++slot)
            if (! vts.isInputChannelStereo(slot)) { monoNum = vts.getInputChannelNumber(slot); break; }
        check(monoNum > 0, "I3: found a mono channel");
        check(vts.setInputChannelType(monoNum, true).wasOk(), "I3: flip it to stereo");
        reconfig();
        {
            const auto d = fm.preflightChannelIdentity(F, Kind::systemConfig);
            check(d.relation == Rel::conflicting && d.retyped.size() == 1 && d.retyped[0].live.number == monoNum,
                  "I3: the system file now conflicts on exactly that channel");
            check(! fm.isChannelIdentitySafe(d, Kind::systemConfig), "I3: a type conflict is not safe");
        }
        check(! fm.importSystemConfig(F), "I3: the primitive REFUSES the load");
        check(fm.getLastError().isNotEmpty(), "I3: ...and says why");
        check(vts.isInputChannelStereo(vts.getSlotForChannelNumber(monoNum)), "I3: nothing was applied");
        {
            WFSFileManager::ScopedChannelIdentityBypass bypass(fm);
            check(fm.importSystemConfig(F), "I3: under a bypass it loads");
        }
        reconfig();
        check(! vts.isInputChannelStereo(vts.getSlotForChannelNumber(monoNum)),
              "I3: the bypassed load applied the file's type by number (the crossing, on purpose)");

        // I4: the reported case - arrangement matches by position, numbers differ.
        {
            std::vector<int> shifted;
            for (int slot = 0; slot < vts.getNumInputChannels(); ++slot)
                shifted.push_back(vts.getInputChannelNumber(slot) + 20);
            const auto namesBefore = namesInSlotOrder();
            const auto patchBefore = patchInSlotOrder();
            check(vts.assignInputChannelNumbersBySlot(shifted, "self-test I4").wasOk(), "I4: relabel every channel (+20)");
            reconfig();
            const auto d = fm.preflightChannelIdentity(F, Kind::systemConfig);
            check(d.relation == Rel::positionalTypesMatch, "I4: the arrangement matches position by position, numbers differ");
            check(! fm.isChannelIdentitySafe(d, Kind::systemConfig), "I4: ...and that is NOT safe - the reported bug");
            check(! fm.importSystemConfig(F), "I4: refused");
            check(vts.assignInputChannelNumbersBySlot(d.fileNumbersBySlot, "self-test I4 take file numbers").wasOk(),
                  "I4: take the file's numbers");
            reconfig();
            check(fm.preflightChannelIdentity(F, Kind::systemConfig).relation == Rel::identical, "I4: now identical");
            check(namesInSlotOrder() == namesBefore && patchInSlotOrder() == patchBefore,
                  "I4: names and patch stayed with their positions through the relabel");
            check(fm.importSystemConfig(F), "I4: the load passes without clearance");
            reconfig();
        }

        // I6: a project's own pair.
        {
            check(fm.exportSystemConfig(F) && fm.exportInputConfig(G), "I6: export a consistent pair");
            check(fm.preflightProjectChannelIdentity(F, G).relation == Rel::identical, "I6: the pair is consistent");
            check(fm.isChannelIdentitySafe(fm.preflightProjectChannelIdentity(F, G), Kind::projectPair),
                  "I6: a consistent pair is a safe project load");

            const int last = vts.getInputChannelNumber(vts.getNumInputChannels() - 1);
            check(vts.moveInputChannel(last, 0).wasOk(), "I6: move a channel");
            reconfig();
            auto G2 = tmp("wfs-selftest-identity-inputs2.xml");
            G2.deleteFile();
            check(fm.exportInputConfig(G2), "I6: export inputs again");
            const auto d = fm.preflightProjectChannelIdentity(F, G2);
            check(d.relation == Rel::orderOnly, "I6: system.xml and the newer inputs.xml differ by order");
            check(fm.isChannelIdentitySafe(d, Kind::projectPair), "I6: an order-only pair is still a safe project load");

            int m = -1;
            for (int slot = 0; slot < vts.getNumInputChannels(); ++slot)
                if (! vts.isInputChannelStereo(slot)) { m = vts.getInputChannelNumber(slot); break; }
            check(vts.setInputChannelType(m, true).wasOk(), "I6: flip a type in the session only");
            reconfig();
            auto G3 = tmp("wfs-selftest-identity-inputs3.xml");
            G3.deleteFile();
            check(fm.exportInputConfig(G3), "I6: export inputs with the flipped type");
            const auto d3 = fm.preflightProjectChannelIdentity(F, G3);
            check(d3.relation == Rel::conflicting && d3.retyped.size() == 1 && d3.retyped[0].live.number == m,
                  "I6: the pair now conflicts on exactly the flipped channel");
            check(! fm.isChannelIdentitySafe(d3, Kind::projectPair), "I6: a conflicting pair is not a safe project load");
            check(vts.setInputChannelType(m, false).wasOk(), "I6: flip it back");
            reconfig();
            G2.deleteFile(); G3.deleteFile();
        }

        // I7: the hardware fingerprint.
        {
            check(fm.exportInputConfig(G), "I7: export inputs with fingerprints that match the live patch");
            // Re-patch slot 0 onto a column nothing else uses.
            auto patchTree = vts.getAudioPatchState().getChildWithName(WFSParameterIDs::InputPatch);
            auto rows = patchRows();
            juce::StringArray cols = juce::StringArray::fromTokens(rows[0], ",", "");
            for (int c = 0; c < cols.size(); ++c) cols.set(c, "0");
            while (cols.size() < 62) cols.add("0");
            cols.set(60, "1");
            if (vts.isInputChannelStereo(0)) cols.set(61, "1");
            rows.set(0, cols.joinIntoString(","));
            patchTree.setProperty(WFSParameterIDs::patchData, rows.joinIntoString(";"), nullptr);
            reconfig();

            const auto d = fm.preflightChannelIdentity(G, Kind::inputConfig);
            check(d.relation == Rel::identical, "I7: same channel list");
            check(d.patchDiffers.size() == 1 && d.patchDiffers[0].live.number == vts.getInputChannelNumber(0),
                  "I7: the re-patched channel is the one fingerprint that differs");
            check(! fm.isChannelIdentitySafe(d, Kind::inputConfig), "I7: a fingerprint mismatch is the operator's call, not safe");
            check(! fm.importInputConfig(G), "I7: refused without clearance");
            fm.grantChannelIdentityClearance(G);
            check(fm.importInputConfig(G), "I7: passes with a one-shot clearance");
            reconfig();
            check(! fm.importInputConfig(G), "I7: the clearance was consumed by that one load");
            check(fm.exportInputConfig(G), "I7: re-export so the fingerprints match again");
        }

        // I8: the hardware-derived relabel.
        {
            std::vector<int> nums;
            for (int slot = 0; slot < vts.getNumInputChannels(); ++slot)
                nums.push_back(vts.getInputChannelNumber(slot));
            std::vector<int> orig = nums;
            std::swap(nums[0], nums[1]);
            check(vts.assignInputChannelNumbersBySlot(nums, "self-test I8 swap").wasOk(), "I8: swap two channels' numbers");
            reconfig();
            const auto d = fm.preflightChannelIdentity(G, Kind::inputConfig);
            check(d.hardwareRelabel.has_value(), "I8: a hardware-derived relabel is offered");
            check(d.hardwareRelabel && *d.hardwareRelabel == orig, "I8: ...and it is exactly the inverse swap");
            check(vts.assignInputChannelNumbersBySlot(*d.hardwareRelabel, "self-test I8 apply").wasOk(), "I8: apply it");
            reconfig();
            check(fm.preflightChannelIdentity(G, Kind::inputConfig).relation == Rel::identical, "I8: identical again");

            // An unpatched channel withholds the suggestion rather than guessing.
            auto patchTree = vts.getAudioPatchState().getChildWithName(WFSParameterIDs::InputPatch);
            auto rows = patchRows();
            juce::StringArray cols = juce::StringArray::fromTokens(rows[0], ",", "");
            for (int c = 0; c < cols.size(); ++c) cols.set(c, "0");
            rows.set(0, cols.joinIntoString(","));
            patchTree.setProperty(WFSParameterIDs::patchData, rows.joinIntoString(";"), nullptr);
            reconfig();
            check(vts.assignInputChannelNumbersBySlot(nums, "self-test I8 swap again").wasOk(), "I8: swap again");
            reconfig();
            check(! fm.preflightChannelIdentity(G, Kind::inputConfig).hardwareRelabel.has_value(),
                  "I8: with an unpatched channel no suggestion is made");
            check(vts.assignInputChannelNumbersBySlot(orig, "self-test I8 restore").wasOk(), "I8: restore the numbers");
            reconfig();
        }

        F.deleteFile(); G.deleteFile();
    }

    // ---- V: a count reduction names the channel it removes ------------------
    // The dialog used to predict by HIGHEST NUMBER while the code removed the
    // LAST IN DISPLAY ORDER; identical until a latched list is dragged.
    {
        vts.setInputChannelCounts(4, 2);
        reconfig();

        auto highestNumberedOfType = [&](bool stereo)
        {
            int best = -1;
            for (int slot = 0; slot < vts.getNumInputChannels(); ++slot)
                if (vts.isInputChannelStereo(slot) == stereo)
                    best = juce::jmax(best, vts.getInputChannelNumber(slot));
            return best;
        };
        auto lastInDisplayOrderOfType = [&](bool stereo)
        {
            for (int slot = vts.getNumInputChannels(); --slot >= 0;)
                if (vts.isInputChannelStereo(slot) == stereo)
                    return vts.getInputChannelNumber(slot);
            return -1;
        };

        const int hiStereo = highestNumberedOfType(true);
        check(vts.moveInputChannel(hiStereo, 0).wasOk(), "V0: drag the highest-numbered stereo to slot 0");
        reconfig();
        const int lastStereo = lastInDisplayOrderOfType(true);
        check(lastStereo > 0 && lastStereo != hiStereo, "V0: a lower-numbered stereo is now last in display order");

        const auto predictedS = vts.predictInputChannelReduction(4, 1);
        check(predictedS.size() == 1 && predictedS[0].stereo && predictedS[0].number == lastStereo,
              "V1: the prediction names the LAST stereo in display order, not the highest-numbered");
        vts.setInputChannelCounts(4, 1);
        reconfig();
        check(vts.getSlotForChannelNumber(lastStereo) < 0, "V1: that channel is the one that went");
        check(vts.getSlotForChannelNumber(hiStereo) >= 0, "V1: the highest-numbered stereo is still live");

        const int hiMono = highestNumberedOfType(false);
        check(vts.moveInputChannel(hiMono, 0).wasOk(), "V2: drag the highest-numbered mono to slot 0");
        reconfig();
        const int lastMono = lastInDisplayOrderOfType(false);
        check(lastMono > 0 && lastMono != hiMono, "V2: a lower-numbered mono is now last in display order");

        const auto predictedM = vts.predictInputChannelReduction(3, 1);
        check(predictedM.size() == 1 && ! predictedM[0].stereo && predictedM[0].number == lastMono,
              "V3: the prediction names the LAST mono in display order");
        vts.setInputChannelCounts(3, 1);
        reconfig();
        check(vts.getSlotForChannelNumber(lastMono) < 0, "V3: that channel is the one that went");
        check(vts.getSlotForChannelNumber(hiMono) >= 0, "V3: the highest-numbered mono is still live");
    }

    // ---- W: the MCP input-count tools -------------------------------------
    // The two tools are tier 3, and the safety gate is deliberately UI-only, so
    // they cannot be driven from the headless harness. What CAN be driven is the
    // logic underneath them, which is where all the behaviour lives: substitute
    // one axis, clamp exactly as the GUI clamps, and report what went.
    {
        using namespace WFSNetwork::Tools::ChannelLifecycle;

        auto payloadInt = [] (const WFSNetwork::ToolResult& r, const char* key) -> int
        {
            if (auto* obj = r.value.getDynamicObject())
                return static_cast<int> (obj->getProperty (key));
            return -1;
        };
        auto payloadHas = [] (const WFSNetwork::ToolResult& r, const char* key) -> bool
        {
            auto* obj = r.value.getDynamicObject();
            return obj != nullptr && obj->hasProperty (key);
        };

        vts.setInputChannelCounts(6, 2);
        reconfig();
        const int monoBefore   = vts.getNumInputChannels() - vts.getNumStereoInputChannels();
        const int stereoBefore = vts.getNumStereoInputChannels();
        check(monoBefore == 6 && stereoBefore == 2, "W0: fixture is 6 mono + 2 stereo");

        // W1: setting the stereo axis leaves mono untouched. This is the whole
        // point of the pair — the generated tool it replaces set the TOTAL.
        auto r1 = setCounts(vts, true, 3, nullptr);
        reconfig();
        check(r1.success, "W1: setting the stereo count succeeds");
        check(payloadInt(r1, "stereo") == 3, "W1: stereo became 3");
        check(payloadInt(r1, "mono") == monoBefore, "W1: mono is untouched");
        check(vts.getNumStereoInputChannels() == 3, "W1: and the tree agrees");

        // W2: setting the mono axis leaves stereo untouched.
        auto r2 = setCounts(vts, false, 9, nullptr);
        reconfig();
        check(r2.success, "W2: setting the mono count succeeds");
        check(payloadInt(r2, "mono") == 9, "W2: mono became 9");
        check(payloadInt(r2, "stereo") == 3, "W2: stereo is untouched");
        check(vts.getNumInputChannels() == 12, "W2: total is mono + stereo, not the mono value");

        // W3: a reduction names its victims, and they are the ones that go.
        int lastMonoW = 0;
        for (int slot = vts.getNumInputChannels(); --slot >= 0;)
            if (! vts.isInputChannelStereo(slot)) { lastMonoW = vts.getInputChannelNumber(slot); break; }
        auto r3 = setCounts(vts, false, 8, nullptr);
        reconfig();
        check(r3.success && payloadHas(r3, "removed_channels"),
              "W3: a reduction reports removed_channels");
        check(vts.getSlotForChannelNumber(lastMonoW) < 0,
              "W3: the last mono in display order is the one that went");

        // W4: a no-op says so rather than pretending to have done something.
        auto r4 = setCounts(vts, false, 8, nullptr);
        check(r4.success && payloadInt(r4, "mono") == 8, "W4: asking for the current count succeeds");
        if (auto* obj = r4.value.getDynamicObject())
            check(! static_cast<bool>(obj->getProperty("changed")), "W4: and reports changed=false");

        // W5: the clamp is the GUI clamp, from the same function. 64 mono cannot
        // coexist with 3 stereo, so the request is cut down rather than refused,
        // and the payload says what was asked for.
        auto r5 = setCounts(vts, false, WFSParameterDefaults::maxInputChannels, nullptr);
        reconfig();
        const int clampedMono = payloadInt(r5, "mono");
        check(r5.success, "W5: an over-large mono request succeeds");
        check(clampedMono < WFSParameterDefaults::maxInputChannels,
              "W5: it was clamped rather than granted");
        check(vts.getNumInputChannels() <= WFSParameterDefaults::maxInputChannels,
              "W5: the 64-channel budget still holds");
        check(payloadHas(r5, "requested"), "W5: the clamp is visible as `requested`");

        // W6: the stopped-only guard reads the same flag the GUI greys its
        // controls on. The tool asks isProcessingEnabled(); this is that answer.
        auto io = vts.getIOState();
        const bool runWas = static_cast<bool>(io.getProperty(WFSParameterIDs::runDSP, false));
        io.setProperty(WFSParameterIDs::runDSP, true, nullptr);
        check(vts.isProcessingEnabled(), "W6: engine-running is visible to the guard");
        io.setProperty(WFSParameterIDs::runDSP, false, nullptr);
        check(! vts.isProcessingEnabled(), "W6: and clears again");
        io.setProperty(WFSParameterIDs::runDSP, runWas, nullptr);

        // W7: the stopped-only guard, through the REAL tool handler rather than
        // the logic underneath it. The handler cannot be reached from the headless
        // MCP harness (tier 3, and the safety gate is deliberately UI-only), and it
        // cannot be reached from a project fixture either, because runDSP is a
        // transient toggle that WFSFileManager::stripTransientToggles removes on
        // save — a project can never load with DSP flagged running, by design. So
        // build the descriptor and call its handler directly.
        {
            auto descriptor = describeSetCount(vts, false, nullptr);
            auto argsObj = std::make_unique<juce::DynamicObject>();
            argsObj->setProperty("value", 4);
            const juce::var handlerArgs (argsObj.release());

            auto ioW = vts.getIOState();
            const bool runWas7 = static_cast<bool>(ioW.getProperty(WFSParameterIDs::runDSP, false));
            const int monoAtGuard = vts.getNumInputChannels() - vts.getNumStereoInputChannels();

            ioW.setProperty(WFSParameterIDs::runDSP, true, nullptr);
            auto refused = descriptor.handler(handlerArgs, nullptr);
            check(! refused.success, "W7: the tool refuses while the engine is running");
            check(refused.errorCode == "engine_running", "W7: and says why");
            check(vts.getNumInputChannels() - vts.getNumStereoInputChannels() == monoAtGuard,
                  "W7: a refused call changes nothing");

            ioW.setProperty(WFSParameterIDs::runDSP, false, nullptr);
            auto allowed = descriptor.handler(handlerArgs, nullptr);
            reconfig();
            check(allowed.success, "W7: and goes through once the engine is stopped");
            check(vts.getNumInputChannels() - vts.getNumStereoInputChannels() == 4,
                  "W7: with the mono count actually applied");

            ioW.setProperty(WFSParameterIDs::runDSP, runWas7, nullptr);
        }

        // W8: the topology callback fires on success and NOT on a refusal. This is
        // the half of the defect that was invisible: the generated count tools
        // changed the tree and never re-prepared the renderer, routing matrices,
        // patch rows or meters, so they looked like they worked and left the engine
        // describing a channel list that no longer existed.
        {
            int topologyCalls = 0;
            std::function<void()> onTopology = [&topologyCalls] { ++topologyCalls; };
            auto descriptor = describeSetCount(vts, true, &onTopology);

            auto argsObj = std::make_unique<juce::DynamicObject>();
            argsObj->setProperty("value", 2);
            const juce::var handlerArgs (argsObj.release());

            auto ioW8 = vts.getIOState();
            const bool runWas8 = static_cast<bool>(ioW8.getProperty(WFSParameterIDs::runDSP, false));

            ioW8.setProperty(WFSParameterIDs::runDSP, true, nullptr);
            (void) descriptor.handler(handlerArgs, nullptr);
            check(topologyCalls == 0, "W8: a refused call does not re-prepare the engine");

            ioW8.setProperty(WFSParameterIDs::runDSP, false, nullptr);
            auto ok8 = descriptor.handler(handlerArgs, nullptr);
            reconfig();
            check(ok8.success && topologyCalls == 1, "W8: a successful call re-prepares it exactly once");

            ioW8.setProperty(WFSParameterIDs::runDSP, runWas8, nullptr);
        }
    }

    // ---- X: the effects family survives a save and a load -------------------
    // The commit that put <Effects> in the tree could gate none of this: nothing
    // could set a non-zero count, so the channel builder, the ring layout, add,
    // remove and the whole per-channel path were compile-verified and never run.
    //
    // Every shape assertion below is made AFTER a save and a reload, never on a
    // freshly built tree. A fresh tree is built by the very builder the
    // assertions describe, so it agrees with itself whatever the file path does;
    // only a reloaded one can see the merge appending a duplicate, the backfill
    // matching the wrong sibling, or the eviction hook failing to run.
    {
        namespace P = WFSParameterIDs;
        namespace D = WFSParameterDefaults;

        auto& fm = parameters.getFileManager();
        const auto previousProject = fm.getProjectFolder();

        auto tempProject = juce::File::getSpecialLocation(juce::File::tempDirectory)
                               .getChildFile("wfs-selftest-effects-project");
        tempProject.deleteRecursively();
        fm.setProjectFolder(tempProject);
        check(fm.createProjectFolderStructure(), "X0: a throwaway project folder");

        auto effectsFile = [&] { return fm.getEffectsConfigFile(); };

        auto occurrences = [](const juce::String& haystack, const juce::String& needle)
        {
            int n = 0;
            for (int at = haystack.indexOf(needle); at >= 0;
                 at = haystack.indexOf(at + needle.length(), needle))
                ++n;
            return n;
        };

        auto childrenOfType = [](const juce::ValueTree& parent, const juce::Identifier& type)
        {
            int n = 0;
            for (int i = 0; i < parent.getNumChildren(); ++i)
                if (parent.getChild(i).hasType(type))
                    ++n;
            return n;
        };

        // THE LOG IS EVIDENCE HERE, not decoration. stripObsoleteEffectProperties
        // passes nullptr for the UndoManager on purpose - a schema change is not
        // a user edit - so a wrong eviction cannot be undone, and the warning is
        // the only trace one will ever leave. A warning nothing asserts is a
        // warning that can silently stop being emitted, which is exactly the
        // state this family was in before it existed, so both warnings are read
        // back off disk. WFSLogger writes through juce::FileLogger, which opens,
        // appends and closes per line, so everything a load emitted is on disk by
        // the time the load returns.
        auto logMark = [] { return WFSLogger::getInstance().getCurrentLogFile().getSize(); };

        auto logSince = [](juce::int64 mark) -> juce::String
        {
            juce::FileInputStream in (WFSLogger::getInstance().getCurrentLogFile());
            if (! in.openedOk() || ! in.setPosition (mark))
                return {};
            return in.readEntireStreamAsString();
        };

        // The whole node shape of one channel, read off the tree. Returns an
        // empty string when the channel is exactly right, otherwise the first
        // thing wrong with it - so a failure names the defect instead of just
        // saying "false".
        auto faultInChannel = [&](int ch) -> juce::String
        {
            const juce::String who = "effect " + juce::String(ch + 1) + ": ";
            auto effect = vts.getEffectState(ch);
            if (! effect.isValid())
                return who + "no <Effect> node";

            // Dense ids: id == index + 1, no gaps, no reuse.
            if (static_cast<int>(effect.getProperty(P::id, -1)) != ch + 1)
                return who + "id is " + effect.getProperty(P::id).toString()
                           + ", expected " + juce::String(ch + 1);

            // The seven flat sections and the sends node, exactly once each.
            const juce::Identifier flat[] = { P::Channel, P::Position, P::Feed, P::ReverbReturn,
                                              P::AutomOtion, P::LFO, P::Chain, P::Sends };
            for (const auto& type : flat)
            {
                const int n = childrenOfType(effect, type);
                if (n != 1)
                    return who + juce::String(n) + " <" + type.toString() + "> nodes, expected 1";
            }

            // The eleven module types, exactly once each. Named from the slot
            // table rather than listed here, so a slot added to the chain is
            // covered without touching this test.
            for (int slot = 0; slot < D::numEffectModuleSlots; ++slot)
            {
                const auto& type = WFSValueTreeState::getEffectModuleType(slot);
                const int n = childrenOfType(effect, type);
                if (n != 1)
                    return who + juce::String(n) + " <" + type.toString() + "> nodes, expected 1";
            }
            if (effect.getNumChildren() != 8 + D::numEffectModuleSlots)
                return who + juce::String(effect.getNumChildren()) + " child nodes, expected "
                           + juce::String(8 + D::numEffectModuleSlots);

            // Six <Band id="1".."6"> under EACH of the two EQ instances. The two
            // are different node TYPES carrying identical property names, which
            // is exactly the arrangement a by-name merge or backfill collapses
            // into one - so both are counted, not just the first.
            const juce::Identifier eqs[] = { P::FxEq1, P::FxEq2 };
            for (const auto& eqType : eqs)
            {
                auto eq = effect.getChildWithName(eqType);
                const int bands = childrenOfType(eq, P::Band);
                if (bands != D::numEffectEQBands)
                    return who + "<" + eqType.toString() + "> has " + juce::String(bands)
                               + " <Band> nodes, expected " + juce::String(D::numEffectEQBands);
                for (int b = 0; b < D::numEffectEQBands; ++b)
                    if (static_cast<int>(eq.getChild(b).getProperty(P::id, -1)) != b + 1)
                        return who + "<" + eqType.toString() + "> band ids are not dense 1.."
                                   + juce::String(D::numEffectEQBands);
            }

            // Eight <Tap id="1".."8"> under <FxDelay>.
            auto delay = effect.getChildWithName(P::FxDelay);
            const int taps = childrenOfType(delay, P::Tap);
            if (taps != D::numEffectDelayTaps)
                return who + "<FxDelay> has " + juce::String(taps) + " <Tap> nodes, expected "
                           + juce::String(D::numEffectDelayTaps);
            for (int t = 0; t < D::numEffectDelayTaps; ++t)
                if (static_cast<int>(delay.getChild(t).getProperty(P::id, -1)) != t + 1)
                    return who + "<FxDelay> tap ids are not dense 1.."
                               + juce::String(D::numEffectDelayTaps);

            return {};
        };

        // Every live channel, plus the two bookkeeping copies of the count and
        // the container's child list. Asserting the count in three places is the
        // point: mergeTreeRecursive only ever appends, so a list that grew past
        // <Effects count> and Config/IO/effectChannels is exactly the drift
        // getNumReverbChannels' counting loop exists to paper over.
        auto verifyFamily = [&](const char* label, int expected)
        {
            auto effects = vts.getEffectsState();
            check(effects.isValid(), juce::String(label) + ": the <Effects> container is present");
            check(vts.getNumEffectChannels() == expected,
                  juce::String(label) + ": " + juce::String(expected) + " live effect channels");
            check(effects.getNumChildren() == expected,
                  juce::String(label) + ": no orphan children beside them");
            check(static_cast<int>(effects.getProperty(P::count, -1)) == expected,
                  juce::String(label) + ": <Effects count> agrees");
            check(static_cast<int>(vts.getIOState().getProperty(P::effectChannels, -1)) == expected,
                  juce::String(label) + ": Config/IO/effectChannels agrees");

            juce::String fault;
            for (int ch = 0; ch < expected && fault.isEmpty(); ++ch)
                fault = faultInChannel(ch);
            check(fault.isEmpty(), juce::String(label) + ": every channel is nineteen nodes deep"
                                 + (fault.isEmpty() ? juce::String() : " - " + fault));
        };

        // X0: an ABSENT effects.xml is SUCCESS. Every project this application
        // has ever saved has none, and a false here would not merely show an
        // error: loadCompleteConfig gates markChannelNumbersUserOwned on
        // success, so every one of those opens would go unlatched.
        check(! effectsFile().existsAsFile(), "X0: the throwaway project has no effects.xml");
        check(fm.loadEffectsConfig(), "X0: an absent effects.xml loads as SUCCESS");
        verifyFamily("X0", 0);
        check(fm.loadEffectsConfigBackup(0), "X0: an empty effects backup set is SUCCESS too");

        // ...but the same missing file named through the IMPORT primitive is an
        // error, because the caller named it. The distinction is the divergence.
        check(! fm.importEffectsConfig(effectsFile()),
              "X0: importEffectsConfig on a file that is not there is an ERROR");

        // X1: create channels and write them out.
        vts.setNumEffectChannels(3);
        check(vts.getNumEffectChannels() == 3, "X1: three effect channels created");
        check(fm.saveEffectsConfig(), "X1: save effects.xml");
        check(effectsFile().existsAsFile(), "X1: effects.xml appears");
        check(occurrences(effectsFile().loadFileAsString(), "<Effect ") == 3,
              "X1: the file holds exactly three <Effect> nodes");

        // X2: empty the family in memory, then bring it back from the file. The
        // shape assertions run on THIS tree, not the one X1 built.
        vts.setNumEffectChannels(0);
        check(vts.getNumEffectChannels() == 0, "X2: the family is emptied in memory");
        check(fm.loadEffectsConfig(), "X2: reload effects.xml");
        verifyFamily("X2 (after save + reload)", 3);

        // X3: raising then lowering the count leaves no orphan - in memory, and
        // then through a full round trip so a stale child cannot hide in the file.
        vts.setNumEffectChannels(5);
        verifyFamily("X3: raised to five", 5);
        vts.setNumEffectChannels(2);
        verifyFamily("X3: lowered to two", 2);
        check(fm.saveEffectsConfig(), "X3: save the lowered family");
        check(occurrences(effectsFile().loadFileAsString(), "<Effect ") == 2,
              "X3: the three removed channels are not in the file");
        vts.setNumEffectChannels(0);
        check(fm.loadEffectsConfig(), "X3: reload it");
        verifyFamily("X3 (after save + reload)", 2);

        // X4: a file that holds MORE channels than the session re-syncs the count
        // from the child list. mergeTreeRecursive appends and never removes, so
        // without the outputs-style re-sync the list would grow while both copies
        // of the count stayed at the session's smaller number.
        vts.setNumEffectChannels(4);
        check(fm.saveEffectsConfig(), "X4: save four channels");
        vts.setNumEffectChannels(1);
        check(vts.getNumEffectChannels() == 1, "X4: the session drops to one");
        check(fm.loadEffectsConfig(), "X4: load the four-channel file over it");
        verifyFamily("X4 (file longer than the session)", 4);

        // X5: a PRESENT but malformed effects.xml is an error, like every other
        // section file. Both shapes: well-formed XML with no <Effects> in it, and
        // something that is not XML at all.
        {
            const juce::String good = effectsFile().loadFileAsString();

            effectsFile().replaceWithText("<?xml version=\"1.0\"?>\n<EffectsConfig version=\"1.0\"/>\n");
            fm.clearError();
            check(! fm.loadEffectsConfig(), "X5: an effects.xml with no <Effects> is an ERROR");
            check(fm.getLastError().isNotEmpty(), "X5: ...and says why");

            effectsFile().replaceWithText("this is not xml at all\n");
            fm.clearError();
            check(! fm.loadEffectsConfig(), "X5: an unparseable effects.xml is an ERROR");

            effectsFile().replaceWithText(good);
            check(fm.loadEffectsConfig(), "X5: the good file still loads");
            verifyFamily("X5", 4);
        }

        // X6: the eviction hook. Nothing removes a property on the load path -
        // mergeTreeRecursive and backfillFromTemplate both only ever ADD - so
        // without stripObsoleteEffectProperties a retired attribute would ride
        // along in the live tree and be re-saved for ever. Planted at three
        // depths, because the walk has to match id'd and id-less children by
        // different rules.
        {
            static const juce::Identifier ghost("effectRetiredGhost");
            juce::String xml = effectsFile().loadFileAsString();
            xml = xml.replace("<Effect id=", "<Effect effectRetiredGhost=\"1\" id=");
            xml = xml.replace("<FxDist ", "<FxDist effectRetiredGhost=\"1\" ");
            xml = xml.replace("<Band id=", "<Band effectRetiredGhost=\"1\" id=");
            // Two shapes X6 never covered: a CHANNEL-section node (<Chain> - id-less
            // like <FxDist>, but not a chain module) and an id'd <Tap>, matched by
            // type AND id exactly as <Band> is. Structurally identical to what is
            // already here, which is the point: a rule asserted on two of the four
            // shapes it has to handle is a rule half asserted.
            xml = xml.replace("<Chain ", "<Chain effectRetiredGhost=\"1\" ");
            xml = xml.replace("<Tap id=", "<Tap effectRetiredGhost=\"1\" id=");
            effectsFile().replaceWithText(xml);
            check(occurrences(effectsFile().loadFileAsString(), "effectRetiredGhost") > 0,
                  "X6: the file carries a retired attribute the schema no longer declares");

            const auto beforeGhostLoad = logMark();
            check(fm.loadEffectsConfig(), "X6: load it");
            verifyFamily("X6 (after the ghost load)", 4);

            auto effect = vts.getEffectState(0);
            check(! effect.hasProperty(ghost), "X6: the retired attribute is evicted from <Effect>");
            check(! effect.getChildWithName(P::FxDist).hasProperty(ghost),
                  "X6: ...from an id-less module node");
            check(! effect.getChildWithName(P::FxEq2).getChild(0).hasProperty(ghost),
                  "X6: ...and from an id'd <Band> under the SECOND EQ instance");
            check(! effect.getChildWithName(P::Chain).hasProperty(ghost),
                  "X6: ...from a channel-section node, not only a chain module");
            check(! effect.getChildWithName(P::FxDelay).getChild(0).hasProperty(ghost),
                  "X6: ...and from an id'd <Tap>");

            // The eviction SAYS what it took. Nothing else does: it is not
            // undoable, it trips no flag a user can see, and the next save simply
            // writes the shorter file.
            const juce::String ghostLog = logSince(beforeGhostLoad);
            check(ghostLog.contains("Effects schema: dropped")
                      && ghostLog.contains("effectRetiredGhost"),
                  "X6: the eviction is not silent - one warning, naming what went");

            check(fm.saveEffectsConfig(), "X6: save again");
            check(occurrences(effectsFile().loadFileAsString(), "effectRetiredGhost") == 0,
                  "X6: and it is gone from the file rather than re-saved for ever");
        }

        // X7: THE BACKWARD-COMPATIBILITY CASE, through the complete orchestration
        // rather than the section primitive - a project folder written before this
        // family existed. It must open with SUCCESS, with the family present, and
        // it must latch the channel numbers, which loadCompleteConfig does only
        // when every section reported success.
        //
        // Deleting effects.xml is NOT enough to make the folder look old: this
        // version's system.xml still carries effectChannels, effectsMapVisible
        // and <EffectsGlobal>, none of which a pre-effects save ever wrote. The
        // case that matters is all four absent together, so strip the three as
        // well - otherwise this gate passes on a folder no existing show
        // resembles.
        {
            vts.setNumEffectChannels(0);
            check(fm.saveCompleteConfig(), "X7: save a complete project");
            check(effectsFile().existsAsFile(), "X7: the save wrote effects.xml");

            check(effectsFile().deleteFile(), "X7: delete effects.xml");

            const auto systemFile = fm.getSystemConfigFile();
            if (auto sys = juce::XmlDocument::parse(systemFile))
            {
                if (auto* cfg = sys->getChildByName("Config"))
                {
                    if (auto* io = cfg->getChildByName("IO"))
                        io->removeAttribute(P::effectChannels);
                    if (auto* master = cfg->getChildByName("Master"))
                        master->removeAttribute(P::effectsMapVisible);
                    cfg->removeChildElement(cfg->getChildByName(P::EffectsGlobal.toString()), true);
                }
                check(sys->writeTo(systemFile),
                      "X7: strip system.xml of all three things this version added - NOW the "
                      "folder looks like every project ever saved");
            }
            else
            {
                check(false, "X7: system.xml parses");
            }

            const juce::String preEffects = systemFile.loadFileAsString();
            check(! preEffects.contains("effectChannels") && ! preEffects.contains("effectsMapVisible")
                      && ! preEffects.contains("EffectsGlobal"),
                  "X7: the pre-effects system.xml names none of the three");

            fm.clearError();
            check(fm.loadCompleteConfig(), "X7: a project with no effects.xml loads with SUCCESS");
            check(fm.getLastError().isEmpty(), "X7: ...and reports no error");
            verifyFamily("X7 (no effects.xml)", 0);
            check(vts.areChannelNumbersUserOwned(),
                  "X7: the load latched the channel numbers - the thing a false here would have cost");
            reconfig();

            check(fm.saveCompleteConfig(), "X7: save the project again");
            check(effectsFile().existsAsFile(), "X7: effects.xml is back");
            const juce::String upgraded = systemFile.loadFileAsString();
            check(upgraded.contains("effectChannels") && upgraded.contains("EffectsGlobal"),
                  "X7: ...and one save upgrades the old show to this version's baseline");
        }

        // X8: THE COUNT MAY NOT LIE. <IO>/effectChannels is the config section's
        // statement of how many effect channels the show has, and every other
        // family is BUILT from its equivalent by applyConfigSection before its
        // own file is merged. Effects were not, so a system.xml naming four
        // beside a project with no effects.xml - which is exactly what "Load
        // System Config" on its own leaves behind, and what the exit auto-save
        // writes, since that saves system.xml alone - produced a session
        // claiming four channels with none in the tree, and the two stayed at
        // odds through every later save. The load has to materialise them.
        {
            vts.setNumEffectChannels(4);
            check(fm.saveCompleteConfig(), "X8: save a four-channel project");
            check(effectsFile().deleteFile(),
                  "X8: delete effects.xml, leaving system.xml alone to say four");

            vts.setNumEffectChannels(0);
            check(vts.getNumEffectChannels() == 0, "X8: the session is emptied first");

            fm.clearError();
            check(fm.loadCompleteConfig(), "X8: it loads with SUCCESS");
            check(fm.getLastError().isEmpty(), "X8: ...and reports no error");
            verifyFamily("X8 (count from system.xml, no effects.xml)", 4);
            reconfig();

            check(fm.saveCompleteConfig(), "X8: save it back");
            check(occurrences(effectsFile().loadFileAsString(), "<Effect ") == 4,
                  "X8: the regenerated effects.xml holds the four channels the count promised");
        }

        // X9: A FILE SHORT OF THE SCHEMA IS COMPLETED, not accepted half-built.
        // mergeTreeRecursive appends an <Effect> the session does not have
        // VERBATIM and adds nothing to it, so without the template backfill on
        // this path an older or hand-edited file went live missing whatever it
        // did not carry - and setEffectParameter writes only where some child
        // already hasProperty(), which would make every later GUI/OSC/MCP write
        // of the absent parameter a silent no-op for the life of that show.
        // Planted at three depths, plus a channel that is nothing but an id.
        {
            vts.setNumEffectChannels(2);
            check(fm.saveEffectsConfig(), "X9: save two channels");

            if (auto doc = juce::XmlDocument::parse(effectsFile()))
            {
                auto* effectsEl = doc->getChildByName(P::Effects.toString());
                auto* first = effectsEl != nullptr ? effectsEl->getChildByName(P::Effect.toString())
                                                   : nullptr;
                check(first != nullptr, "X9: the saved file holds an <Effect>");

                if (first != nullptr)
                {
                    // A whole module node, one band of the SECOND EQ instance,
                    // and a single property of <Channel>.
                    first->removeChildElement(first->getChildByName(P::FxCrush.toString()), true);
                    if (auto* eq2 = first->getChildByName(P::FxEq2.toString()))
                        eq2->removeChildElement(eq2->getChildElement(D::numEffectEQBands - 1), true);
                    if (auto* channel = first->getChildByName(P::Channel.toString()))
                    {
                        channel->removeAttribute(P::effectMute);

                        // Values the file DOES carry, distinct from every
                        // default: a backfill that overwrote instead of filling
                        // in would reset them, and not one shape assertion in
                        // this phase would notice.
                        channel->setAttribute(P::effectName, "Alpha");
                        channel->setAttribute(P::effectAttenuation, -12.5);
                    }
                    if (auto* position = first->getChildByName(P::Position.toString()))
                        position->setAttribute(P::effectPositionX, 1.25);
                }

                if (effectsEl != nullptr)
                {
                    // ...and a third channel beyond the count, so the merge
                    // appends it and nothing ever built it.
                    effectsEl->createNewChildElement(P::Effect.toString())->setAttribute(P::id, 3);
                    check(doc->writeTo(effectsFile()), "X9: write the short file back");
                }
            }
            else
            {
                check(false, "X9: the saved file parses");
            }

            check(occurrences(effectsFile().loadFileAsString(), "<FxCrush") == 1,
                  "X9: the file is one <FxCrush> short of the channels it describes");

            vts.setNumEffectChannels(0);
            check(fm.loadEffectsConfig(), "X9: load the short file");
            verifyFamily("X9 (short file completed from the template)", 3);

            auto shortChannel = vts.getEffectChannelSection(0);
            check(shortChannel.hasProperty(P::effectMute),
                  "X9: the missing <Channel> property is back");
            check(static_cast<int>(shortChannel.getProperty(P::effectMute, -1))
                      == static_cast<int>(D::effectMuteDefault),
                  "X9: ...at its DEFAULT, not at a neighbour's value");

            // The other half of "backfill", and the half no shape assertion can
            // see: what the file DID carry has to come through untouched.
            check(shortChannel.getProperty(P::effectName).toString() == "Alpha",
                  "X9: a string the short file carried is not overwritten by the template");
            check(std::abs(static_cast<double>(shortChannel.getProperty(P::effectAttenuation)) + 12.5) < 1.0e-6,
                  "X9: ...nor is a float");
            check(std::abs(static_cast<double>(vts.getEffectPositionSection(0)
                                                   .getProperty(P::effectPositionX)) - 1.25) < 1.0e-6,
                  "X9: ...nor one a node deeper, where the ring default would have landed");

            check(fm.saveEffectsConfig(), "X9: save it again");
            check(occurrences(effectsFile().loadFileAsString(), "<FxCrush") == 3,
                  "X9: and the completed shape is what goes back to disk");
        }

        // X10: THE SEND ROWS ARE SCHEMA NOW, and the exemption that stood in
        // for that is gone. While <Sends> was built EMPTY the four packed rows
        // were declared, written at runtime, and therefore indistinguishable
        // from RETIRED names by the only evidence stripObsoleteEffectProperties
        // has - absence from a freshly built channel - so the hook had to name
        // and skip them by hand, or the load after the first write would have
        // deleted an operator's entire routing with no error and no undo entry.
        //
        // createEffectSendsSection stamps all four now, so the template carries
        // them like any other property and that hand-maintained list is deleted.
        // Three things change with it, and this phase asserts each: a row written
        // from OUTSIDE the app still arrives intact and exact; a genuine ghost on
        // <Sends> itself still goes; and a row NAME on a node that is not <Sends>
        // is a ghost again rather than an exempt stowaway nothing could ever
        // clean up. X6 above proves eviction works at all; this proves it stops
        // in the right place.
        {
            auto packedRow = [](int width, const juce::String& idle,
                                int at1, const juce::String& v1,
                                int at2, const juce::String& v2)
            {
                juce::StringArray cells;
                for (int i = 0; i < width; ++i)
                    cells.add(idle);
                cells.set(at1, v1);
                cells.set(at2, v2);
                return cells.joinIntoString(",");
            };

            // Plausible, and distinct from anything a default or a neighbour
            // would produce: effectSend* are input-wide and keyed by permanent
            // number, effectFxSend* are effect-wide, levels in dB and switches
            // as 0/1.
            const juce::String sendLevels   = packedRow(D::maxInputChannels,  "0",  2, "-6.5",  17, "-12.25");
            const juce::String sendOns      = packedRow(D::maxInputChannels,  "0",  2, "1",     17, "1");
            const juce::String fxSendLevels = packedRow(D::maxEffectChannels, "0",  1, "-3.75",  9, "-24");
            const juce::String fxSendOns    = packedRow(D::maxEffectChannels, "0",  1, "1",      9, "1");

            vts.setNumEffectChannels(2);
            check(fm.saveEffectsConfig(), "X10: save two channels");

            if (auto doc = juce::XmlDocument::parse(effectsFile()))
            {
                auto* effectsEl = doc->getChildByName(P::Effects.toString());
                auto* first = effectsEl != nullptr ? effectsEl->getChildByName(P::Effect.toString())
                                                   : nullptr;
                auto* sends = first != nullptr ? first->getChildByName(P::Sends.toString()) : nullptr;
                check(sends != nullptr, "X10: the saved channel carries a <Sends> node");
                check(sends != nullptr && sends->hasAttribute(P::effectSendLevels)
                          && sends->hasAttribute(P::effectSendOns)
                          && sends->hasAttribute(P::effectFxSendLevels)
                          && sends->hasAttribute(P::effectFxSendOns),
                      "X10: ...with all four rows stamped on it, straight from the builder");

                if (sends != nullptr)
                {
                    // An operator's routing, written by something that is not
                    // this build - a hand edit, an older save, a show file from
                    // another machine. Distinct from every default, so a row that
                    // came back re-defaulted cannot pass for one that survived.
                    sends->setAttribute(P::effectSendLevels,   sendLevels);
                    sends->setAttribute(P::effectSendOns,      sendOns);
                    sends->setAttribute(P::effectFxSendLevels, fxSendLevels);
                    sends->setAttribute(P::effectFxSendOns,    fxSendOns);
                    check(doc->writeTo(effectsFile()), "X10: write the operator's send routing back to the file");
                }
            }
            else
            {
                check(false, "X10: the saved file parses");
            }

            vts.setNumEffectChannels(0);
            check(fm.loadEffectsConfig(), "X10: load the routed file");
            verifyFamily("X10 (Sends rows written at runtime)", 2);

            auto sendsNode = vts.getEffectSendsSection(0);
            check(sendsNode.isValid(), "X10: <Sends> is on the loaded channel");
            check(sendsNode.getProperty(P::effectSendLevels).toString() == sendLevels,
                  "X10: effectSendLevels survives the load with its EXACT value");
            check(sendsNode.getProperty(P::effectSendOns).toString() == sendOns,
                  "X10: ...and effectSendOns");
            check(sendsNode.getProperty(P::effectFxSendLevels).toString() == fxSendLevels,
                  "X10: ...and effectFxSendLevels");
            check(sendsNode.getProperty(P::effectFxSendOns).toString() == fxSendOns,
                  "X10: ...and effectFxSendOns");

            // The other channel was never routed, so it carries the four
            // DEFAULT rows - not an empty node, which is what it would have been
            // before the builder stamped them, and not the first channel's
            // routing either.
            {
                auto unrouted = vts.getEffectSendsSection(1);
                check(unrouted.getNumProperties() == 4,
                      "X10: an unrouted channel's <Sends> carries exactly the four rows");
                check(unrouted.getProperty(P::effectSendLevels).toString()
                          == juce::String::repeatedString("0,", D::maxInputChannels - 1) + "0",
                      "X10: ...all of them at the default, every send at unity into a switch that is off");
                check(unrouted.getProperty(P::effectSendOns).toString()
                          != sendOns,
                      "X10: ...and not the routing the OTHER channel was given");
            }

            check(fm.saveEffectsConfig(), "X10: save the routed session again");
            const juce::String routed = effectsFile().loadFileAsString();
            check(occurrences(routed, "effectSendLevels") == 2
                      && occurrences(routed, "effectFxSendOns") == 2,
                  "X10: the rows go back to disk, one of each per channel");
            check(routed.contains(sendLevels) && routed.contains(fxSendLevels),
                  "X10: ...with the operator's values, not a re-defaulted row");

            // ...and a real ghost planted on that SAME node is still evicted.
            // The exemption names four identifiers; it does not turn <Sends>
            // into a place where retired attributes can hide.
            {
                static const juce::Identifier sendsGhost("effectSendRetiredGhost");
                juce::String xml = effectsFile().loadFileAsString();
                xml = xml.replace("<Sends ", "<Sends effectSendRetiredGhost=\"1\" ");
                effectsFile().replaceWithText(xml);
                check(occurrences(effectsFile().loadFileAsString(), "effectSendRetiredGhost") > 0,
                      "X10: plant a retired attribute on the very node the exemption protects");

                vts.setNumEffectChannels(0);
                check(fm.loadEffectsConfig(), "X10: load it");
                check(! vts.getEffectSendsSection(0).hasProperty(sendsGhost),
                      "X10: the ghost is evicted from <Sends> anyway");
                check(vts.getEffectSendsSection(0).getProperty(P::effectSendLevels).toString() == sendLevels,
                      "X10: ...and the legitimate row beside it is untouched");
            }

            // WHAT THE EXEMPTION USED TO COST, and no longer does. It was keyed
            // on the property NAME at every depth - one hand-maintained fact
            // rather than two - so one of those four names used as junk on a node
            // that is not <Sends> could never be evicted from anyone's file. It
            // was not data loss, nothing was deleted, but it WAS a send row
            // sitting where no reader would ever look, and the hook could only
            // report it. With the rows in the template that whole trade is off:
            // <Chain> has no effectSendLevels in a freshly built channel, so one
            // in a file is a ghost like any other and goes.
            {
                check(fm.saveEffectsConfig(), "X10: write the cleaned tree back before the next plant");

                juce::String xml = effectsFile().loadFileAsString();
                xml = xml.replace("<Chain ", "<Chain effectSendLevels=\"JUNK-ON-CHAIN\" ");
                effectsFile().replaceWithText(xml);

                const auto beforeMisplaced = logMark();
                vts.setNumEffectChannels(0);
                check(fm.loadEffectsConfig(), "X10: load a send row planted on <Chain>");
                const juce::String misplacedLog = logSince(beforeMisplaced);

                check(! vts.getEffectChainSection(0).hasProperty(P::effectSendLevels),
                      "X10: it is EVICTED - a row name off <Sends> is a ghost again");
                check(misplacedLog.contains("Effects schema: dropped")
                          && misplacedLog.contains("effectSendLevels"),
                      "X10: ...and the eviction warning names it");
                check(! misplacedLog.contains("exempt attribute(s) found outside <Sends>"),
                      "X10: the exemption's own warning is gone with the exemption");
                check(vts.getEffectSendsSection(0).getProperty(P::effectSendLevels).toString() == sendLevels,
                      "X10: ...and the real row on <Sends>, the same NAME one node up, is untouched");
            }

            // THE UPGRADE PATH, which is every effects.xml this branch has saved
            // so far: <Sends> with no rows on it at all. The template backfill
            // has to put all four back, or every channel of every existing show
            // goes live with a send matrix that no write can reach - setEffect-
            // Parameter only writes where some child already hasProperty().
            {
                check(fm.saveEffectsConfig(), "X10: save before the empty-node plant");

                juce::String xml = effectsFile().loadFileAsString();
                const int at = xml.indexOf("<Sends ");
                const int end = at >= 0 ? xml.indexOf(at, "/>") : -1;
                check(at >= 0 && end > at, "X10: the file holds a <Sends> to empty out");
                if (at >= 0 && end > at)
                    xml = xml.substring(0, at) + "<Sends " + xml.substring(end);
                effectsFile().replaceWithText(xml);
                check(occurrences(effectsFile().loadFileAsString(), "effectSendLevels") == 1,
                      "X10: one channel's <Sends> is now as empty as last commit wrote it");

                vts.setNumEffectChannels(0);
                check(fm.loadEffectsConfig(), "X10: load the pre-send-matrix file");
                auto restored = vts.getEffectSendsSection(0);
                check(restored.getNumProperties() == 4,
                      "X10: the backfill puts all four rows back on the empty node");
                check(juce::StringArray::fromTokens(
                          restored.getProperty(P::effectSendOns).toString(), ",", "").size()
                              == D::maxInputChannels,
                      "X10: ...at the width the template declares, not at nothing");
            }
        }

        // X11: THE COUNT AND THE ACCESSORS MUST AGREE ABOUT EVERY CHANNEL.
        // getNumEffectChannels counts <Effect> children BY TYPE; getEffectState
        // used to index the child list POSITIONALLY and return an invalid tree
        // when the child at that index was not an <Effect>, treating the type
        // test as a guard on an invariant rather than as a search. The invariant
        // holds for everything this application writes - and a FILE is not this
        // application: mergeTreeRecursive appends an unmatched source child
        // verbatim, and applyEffectsSection is exactly the path a hand-edited or
        // foreign effects.xml takes. <Effect id="1"/>, <Foo/>, <Effect id="2"/>
        // made the count say two while getEffectState(1) returned invalid, so
        // channel 2 was unreachable to every section accessor and to
        // redistributeAllEffectPositions - while setNumEffectChannels wrote that
        // same two into <Effects count> AND Config/IO/effectChannels.
        //
        // The foreign child is KEPT, not dropped: an unrecognised node is
        // evidence of nothing, and deleting it on load would be a second silent
        // data loss rather than a fix for the first.
        {
            static const juce::Identifier foreign("Foo");

            vts.setNumEffectChannels(3);
            check(fm.saveEffectsConfig(), "X11: save three channels");

            if (auto doc = juce::XmlDocument::parse(effectsFile()))
            {
                auto* effectsEl = doc->getChildByName(P::Effects.toString());
                check(effectsEl != nullptr, "X11: the saved file holds <Effects>");

                if (effectsEl != nullptr)
                {
                    // BETWEEN the first channel and the second, never after the
                    // last: an unknown node at the END leaves positional
                    // indexing accidentally right for every live channel, and
                    // this gate would then pass on the broken code.
                    auto* intruder = new juce::XmlElement(foreign.toString());
                    intruder->setAttribute("note", "a node written by a schema this build does not know");
                    effectsEl->insertChildElement(intruder, 1);
                    check(doc->writeTo(effectsFile()),
                          "X11: write it back with a foreign child between channel 1 and channel 2");
                }
            }
            else
            {
                check(false, "X11: the saved file parses");
            }

            vts.setNumEffectChannels(0);
            check(fm.loadEffectsConfig(), "X11: load the foreign file");

            auto effects = vts.getEffectsState();
            check(effects.getNumChildren() == 4,
                  "X11: the container holds the foreign child beside the three channels");
            check(effects.getChild(1).hasType(foreign),
                  "X11: ...and it sits in the middle, where it breaks positional indexing");
            check(vts.getNumEffectChannels() == 3, "X11: the count by type says three");
            check(static_cast<int>(effects.getProperty(P::count, -1)) == 3,
                  "X11: <Effects count> says three");
            check(static_cast<int>(vts.getIOState().getProperty(P::effectChannels, -1)) == 3,
                  "X11: Config/IO/effectChannels says three");

            // THE CRUX: every channel the count promises is reachable, IS the
            // channel it claims to be, and has its sections. Shaped like
            // faultInChannel so a failure names the defect.
            juce::String unreachable;
            for (int ch = 0; ch < 3 && unreachable.isEmpty(); ++ch)
            {
                const juce::String who = "channel " + juce::String(ch + 1) + " ";
                auto e = vts.getEffectState(ch);

                if (! e.isValid())
                    unreachable = who + "is unreachable - getEffectState returns an invalid tree";
                else if (! e.hasType(P::Effect))
                    unreachable = who + "resolves to a <" + e.getType().toString() + ">";
                else if (static_cast<int>(e.getProperty(P::id, -1)) != ch + 1)
                    unreachable = who + "resolves to the channel with id "
                                      + e.getProperty(P::id).toString();
                else if (! vts.getEffectChannelSection(ch).isValid()
                      || ! vts.getEffectPositionSection(ch).isValid()
                      || ! vts.getEffectFeedSection(ch).isValid()
                      || ! vts.getEffectSendsSection(ch).isValid()
                      || ! vts.getEffectModuleSection(ch, 0).isValid())
                    unreachable = who + "has sections the accessors cannot reach";

                // getTreeForParameter is a SECOND resolver, not a caller of
                // getEffectState, and it indexed the child list on its own - so
                // it needs its own assertion or half this fix is ungated. It is
                // the path every OSC, MCP and GUI write takes, and an invalid
                // tree there makes canWriteParameter answer FALSE: a remote
                // surface is told the channel cannot be written, forever.
                else if (! vts.canWriteParameter(P::effectAttenuation, ch))
                    unreachable = who + "is not writable through the generic "
                                        "parameter path (getTreeForParameter)";
            }
            check(unreachable.isEmpty(),
                  juce::String("X11: the count and every accessor agree about all three channels")
                      + (unreachable.isEmpty() ? juce::String() : " - " + unreachable));

            // Reachable is not the same as CORRECT: a resolver that is off by
            // one is reachable for every channel and writes to the wrong one.
            // Three distinct values, read back per channel through the same
            // generic path, is what tells those two apart.
            juce::String misrouted;
            for (int ch = 0; ch < 3; ++ch)
                vts.setParameterWithoutUndo(P::effectAttenuation, -3.0 - ch, ch);
            for (int ch = 0; ch < 3 && misrouted.isEmpty(); ++ch)
            {
                const double want = -3.0 - ch;
                const double got  = static_cast<double>(vts.getFloatParameter(P::effectAttenuation, ch));
                if (std::abs(got - want) > 1.0e-4)
                    misrouted = "channel " + juce::String(ch + 1) + " reads back "
                              + juce::String(got) + " where " + juce::String(want) + " was written";
            }
            check(misrouted.isEmpty(),
                  juce::String("X11: a generic write lands on the channel it names")
                      + (misrouted.isEmpty() ? juce::String() : " - " + misrouted));

            // redistributeAllEffectPositions walks 0..count-1 through that same
            // accessor and silently skips whatever it cannot resolve, so an
            // unreachable channel keeps the position it had while the ring is
            // laid out around it. An invalid section is a fault outright;
            // pairwise equality is the shape the failure actually takes.
            vts.redistributeAllEffectPositions();
            juce::String stacked;
            for (int a = 0; a < 3 && stacked.isEmpty(); ++a)
            {
                auto pa = vts.getEffectPositionSection(a);
                if (! pa.isValid())
                {
                    stacked = "channel " + juce::String(a + 1) + " has no <Position> to lay out";
                    break;
                }

                for (int bb = a + 1; bb < 3 && stacked.isEmpty(); ++bb)
                {
                    auto pb = vts.getEffectPositionSection(bb);
                    if (! pb.isValid())
                    {
                        stacked = "channel " + juce::String(bb + 1) + " has no <Position> to lay out";
                        break;
                    }

                    const auto dx = static_cast<double>(pa.getProperty(P::effectPositionX))
                                  - static_cast<double>(pb.getProperty(P::effectPositionX));
                    const auto dy = static_cast<double>(pa.getProperty(P::effectPositionY))
                                  - static_cast<double>(pb.getProperty(P::effectPositionY));
                    if (std::abs(dx) < 1.0e-9 && std::abs(dy) < 1.0e-9)
                        stacked = "channels " + juce::String(a + 1) + " and " + juce::String(bb + 1)
                                              + " were laid on the same spot";
                }
            }
            check(stacked.isEmpty(),
                  juce::String("X11: the re-layout reaches all three returns")
                      + (stacked.isEmpty() ? juce::String() : " - " + stacked));

            // The remove path indexed that same child list AND renumbered it, so
            // it is the other half of this fix: it reached for child 1, found
            // the foreign node and refused the edit - and had it got past that,
            // it would have stamped id="2" onto a node that is not a channel,
            // which the merge then matches by type AND id for ever after.
            check(vts.removeEffectChannel(1).wasOk(),
                  "X11: removing channel 2 finds an <Effect>, not the foreign child");
            check(vts.getNumEffectChannels() == 2, "X11: two channels remain");
            check(static_cast<int>(vts.getEffectState(0).getProperty(P::id, -1)) == 1
                      && static_cast<int>(vts.getEffectState(1).getProperty(P::id, -1)) == 2,
                  "X11: the surviving ids are dense 1..2");
            check(static_cast<int>(vts.getEffectsState().getProperty(P::count, -1)) == 2,
                  "X11: <Effects count> says two, not the four children the container has");
            check(static_cast<int>(vts.getIOState().getProperty(P::effectChannels, -1)) == 2,
                  "X11: ...and so does Config/IO/effectChannels");

            auto stranger = vts.getEffectsState().getChildWithName(foreign);
            check(stranger.isValid(),
                  "X11: the foreign child is still there - an unknown node is not a licence to delete it");
            check(! stranger.hasProperty(P::id),
                  "X11: ...and the renumber did not invent a channel by stamping an id on it");

            // Leave the container as this phase found it.
            vts.getEffectsState().removeChild(stranger, nullptr);
        }

        // X12: THE SAME DEFECT ONE LEVEL DOWN, where it stops being loud.
        // X11 fixed the CHANNEL index. The band and tap indexes INSIDE a channel
        // were still straight positional reads, on the same reasoning - <FxEq1>
        // holds six <Band>s and <FxDelay> eight <Tap>s "by construction" - which
        // is the same sentence that was wrong about <Effects>, wrong here for the
        // same reason, and reachable through the same file: mergeTreeRecursive
        // lays an unmatched source child down verbatim at EVERY depth, not only
        // at the top of the container.
        //
        // AND IT IS WORSE DOWN HERE, which is why it gets a phase rather than a
        // line in X11. A wrong CHANNEL index returns an INVALID tree: the write
        // is refused and the remote surface is told so. A wrong BAND index
        // returns a VALID tree - the foreign node - so setProperty succeeds, the
        // reply says ok, the value is saved onto that node and read straight back
        // off it on the next load. It round-trips perfectly. The only symptom is
        // an EQ band that does not change the sound, for ever. That is also why
        // the read-back below walks for the id instead of asking the accessor
        // again: a write-then-read through one accessor passes on the broken code.
        {
            static const juce::Identifier eqIntruder("Bar");
            static const juce::Identifier tapIntruder("Baz");

            // Find a node by its id WITHOUT the accessor under test.
            auto childById = [](const juce::ValueTree& parent, const juce::Identifier& type, int wantedId)
            {
                for (int i = 0; i < parent.getNumChildren(); ++i)
                    if (auto c = parent.getChild(i);
                        c.hasType(type) && static_cast<int>(c.getProperty(P::id, -1)) == wantedId)
                        return c;
                return juce::ValueTree();
            };

            vts.setNumEffectChannels(1);
            check(fm.saveEffectsConfig(), "X12: save one channel");

            if (auto doc = juce::XmlDocument::parse(effectsFile()))
            {
                auto* effectsEl = doc->getChildByName(P::Effects.toString());
                auto* first = effectsEl != nullptr ? effectsEl->getChildByName(P::Effect.toString())
                                                   : nullptr;
                auto* eqEl    = first != nullptr ? first->getChildByName(P::FxEq1.toString())   : nullptr;
                auto* delayEl = first != nullptr ? first->getChildByName(P::FxDelay.toString()) : nullptr;
                check(eqEl != nullptr && delayEl != nullptr,
                      "X12: the saved channel carries <FxEq1> and <FxDelay>");

                if (eqEl != nullptr && delayEl != nullptr)
                {
                    // After the FIRST band and the FIRST tap, never at the end. An
                    // intruder at the tail leaves positional indexing accidentally
                    // right for every live band, and this gate would then pass on
                    // the broken code.
                    eqEl->insertChildElement(new juce::XmlElement(eqIntruder.toString()), 1);
                    delayEl->insertChildElement(new juce::XmlElement(tapIntruder.toString()), 1);
                    check(doc->writeTo(effectsFile()),
                          "X12: write it back with a foreign node inside each module");
                }
            }
            else
            {
                check(false, "X12: the saved file parses");
            }

            vts.setNumEffectChannels(0);
            check(fm.loadEffectsConfig(), "X12: load it");

            auto eqNode    = vts.getEffectModuleSection(0, P::FxEq1);
            auto delayNode = vts.getEffectModuleSection(0, P::FxDelay);
            check(eqNode.getChild(1).hasType(eqIntruder),
                  "X12: the foreign node really is inside <FxEq1>, between band 1 and band 2");
            check(delayNode.getChild(1).hasType(tapIntruder),
                  "X12: ...and inside <FxDelay>, between tap 1 and tap 2");

            juce::String wrongBand;
            for (int b = 0; b < D::numEffectEQBands && wrongBand.isEmpty(); ++b)
            {
                auto band = vts.getEffectEQBand(0, 0, b);
                if (! band.isValid())
                    wrongBand = "band " + juce::String(b + 1) + " is unreachable";
                else if (! band.hasType(P::Band))
                    wrongBand = "band " + juce::String(b + 1) + " resolves to a <"
                              + band.getType().toString() + ">";
                else if (static_cast<int>(band.getProperty(P::id, -1)) != b + 1)
                    wrongBand = "band " + juce::String(b + 1) + " resolves to the band with id "
                              + band.getProperty(P::id).toString();
            }
            check(wrongBand.isEmpty(),
                  juce::String("X12: every EQ band resolves to the band it names")
                      + (wrongBand.isEmpty() ? juce::String() : " - " + wrongBand));

            juce::String wrongTap;
            for (int t = 0; t < D::numEffectDelayTaps && wrongTap.isEmpty(); ++t)
            {
                auto tap = vts.getEffectDelayTap(0, t);
                if (! tap.isValid())
                    wrongTap = "tap " + juce::String(t + 1) + " is unreachable";
                else if (! tap.hasType(P::Tap))
                    wrongTap = "tap " + juce::String(t + 1) + " resolves to a <"
                             + tap.getType().toString() + ">";
                else if (static_cast<int>(tap.getProperty(P::id, -1)) != t + 1)
                    wrongTap = "tap " + juce::String(t + 1) + " resolves to the tap with id "
                             + tap.getProperty(P::id).toString();
            }
            check(wrongTap.isEmpty(),
                  juce::String("X12: every delay tap resolves to the tap it names")
                      + (wrongTap.isEmpty() ? juce::String() : " - " + wrongTap));

            // The write, read back BY ID rather than through the accessor. Values
            // no default carries, so a coincidence cannot answer for a hit.
            for (int b = 0; b < D::numEffectEQBands; ++b)
                vts.getEffectEQBand(0, 0, b).setProperty(P::effectEQgain, -1.0 - b, nullptr);
            for (int t = 0; t < D::numEffectDelayTaps; ++t)
                vts.getEffectDelayTap(0, t).setProperty(P::effectDelayTapLevel, -0.5 - t, nullptr);

            juce::String lost;
            for (int b = 0; b < D::numEffectEQBands && lost.isEmpty(); ++b)
            {
                const double got = static_cast<double>(
                    childById(eqNode, P::Band, b + 1).getProperty(P::effectEQgain, 999.0));
                if (std::abs(got - (-1.0 - b)) > 1.0e-6)
                    lost = "<Band id=" + juce::String(b + 1) + "> reads " + juce::String(got)
                         + " where " + juce::String(-1.0 - b) + " was written";
            }
            for (int t = 0; t < D::numEffectDelayTaps && lost.isEmpty(); ++t)
            {
                const double got = static_cast<double>(
                    childById(delayNode, P::Tap, t + 1).getProperty(P::effectDelayTapLevel, 999.0));
                if (std::abs(got - (-0.5 - t)) > 1.0e-6)
                    lost = "<Tap id=" + juce::String(t + 1) + "> reads " + juce::String(got)
                         + " where " + juce::String(-0.5 - t) + " was written";
            }
            check(lost.isEmpty(),
                  juce::String("X12: a write through the accessor lands on the node it named")
                      + (lost.isEmpty() ? juce::String() : " - " + lost));

            check(! eqNode.getChild(1).hasProperty(P::effectEQgain)
                      && ! delayNode.getChild(1).hasProperty(P::effectDelayTapLevel),
                  "X12: and nothing landed on the foreign nodes, where no reader would find it");

            // Durable, and the intruders survive: an unknown node is no more a
            // licence to delete it here than it is in <Effects>.
            check(fm.saveEffectsConfig(), "X12: save the edited channel");
            vts.setNumEffectChannels(0);
            check(fm.loadEffectsConfig(), "X12: reload it");
            eqNode    = vts.getEffectModuleSection(0, P::FxEq1);
            delayNode = vts.getEffectModuleSection(0, P::FxDelay);
            check(eqNode.getChildWithName(eqIntruder).isValid()
                      && delayNode.getChildWithName(tapIntruder).isValid(),
                  "X12: the foreign nodes are still there after a round trip");
            check(std::abs(static_cast<double>(vts.getEffectEQBand(0, 0, 5)
                                                  .getProperty(P::effectEQgain, 999.0)) + 6.0) < 1.0e-6,
                  "X12: ...and band 6 - the one positional indexing pushed off the end - kept its gain");

            // THE SAME RESOLVER, ON THE LIVE FAMILIES. getOutputEQBand,
            // getReverbEQBand and getReverbPostEQBand carried the identical
            // unguarded index, and unlike the effect pair they have callers
            // TODAY: OSC (/wfs/reverb/n/eq/b/...), the MCP band tools and the GUI
            // tabs all resolve a band through them. Driven in memory because this
            // phase owns no reverb CHANNELS and must not move the session's reverb
            // count; <ReverbPostEQ> is a global the container always carries, and
            // output 1 always exists. getReverbEQBand is the same one-line call on
            // the same helper with the same node type as getOutputEQBand - the one
            // of the five this phase does not drive directly.
            {
                auto outEQ = vts.getOutputEQSection(0);
                check(outEQ.isValid(), "X12: output 1 has an <EQ> section");
                outEQ.addChild(juce::ValueTree(eqIntruder), 1, nullptr);

                juce::String wrongOut;
                for (int b = 0; b < D::numEQBands && wrongOut.isEmpty(); ++b)
                {
                    auto band = vts.getOutputEQBand(0, b);
                    if (! band.hasType(P::Band)
                        || static_cast<int>(band.getProperty(P::id, -1)) != b + 1)
                        wrongOut = "output band " + juce::String(b + 1) + " resolves to <"
                                 + band.getType().toString() + " id="
                                 + band.getProperty(P::id).toString() + ">";
                }
                check(wrongOut.isEmpty(),
                      juce::String("X12: an output EQ band resolves by type, not by position")
                          + (wrongOut.isEmpty() ? juce::String() : " - " + wrongOut));

                outEQ.removeChild(outEQ.getChildWithName(eqIntruder), nullptr);
                check(outEQ.getNumChildren() == D::numEQBands,
                      "X12: ...and the output EQ is left exactly as it was found");

                auto postEQ = vts.ensureReverbPostEQSection();
                check(postEQ.isValid(), "X12: the global <ReverbPostEQ> is present");
                postEQ.addChild(juce::ValueTree(eqIntruder), 1, nullptr);

                juce::String wrongPost;
                for (int b = 0; b < D::numReverbPostEQBands && wrongPost.isEmpty(); ++b)
                {
                    auto band = vts.getReverbPostEQBand(b);
                    if (! band.hasType(P::PostEQBand)
                        || static_cast<int>(band.getProperty(P::id, -1)) != b + 1)
                        wrongPost = "post-EQ band " + juce::String(b + 1) + " resolves to <"
                                  + band.getType().toString() + " id="
                                  + band.getProperty(P::id).toString() + ">";
                }
                check(wrongPost.isEmpty(),
                      juce::String("X12: a reverb post-EQ band resolves by its OWN type, <PostEQBand>")
                          + (wrongPost.isEmpty() ? juce::String() : " - " + wrongPost));

                postEQ.removeChild(postEQ.getChildWithName(eqIntruder), nullptr);
                check(postEQ.getNumChildren() == D::numReverbPostEQBands,
                      "X12: ...and the post EQ is left exactly as it was found");
            }
        }

        // X13: THE SEND MATRIX, AND THE MAINTENANCE THAT KEEPS IT POINTED AT THE
        // RIGHT CHANNELS. Five packed rows per channel, and every one of them is
        // a row of columns living in a single string property - which is the
        // shape every bug in this branch has been about. The four defects the
        // phase caught before this one were all the same mistake: code inferring
        // what it may destroy from what it cannot see. These rows ARE that data.
        //
        // Every assertion is made after a save and a reload, like the rest of
        // this phase: a freshly built tree agrees with the builder whatever the
        // file path does, and the columns are exactly what a merge, a backfill or
        // an eviction can quietly move.
        {
            // Read the row TEXT off the node, never through the accessor under
            // test. X12's lesson: a write and a read through one accessor pass
            // each other's mistakes, and a column that is off by one round-trips
            // perfectly.
            auto rowOf = [&](int ch, const juce::Identifier& rowId)
            {
                return juce::StringArray::fromTokens(
                    vts.getEffectSendsSection(ch).getProperty(rowId).toString(), ",", "");
            };
            auto cell = [&](int ch, const juce::Identifier& rowId, int col) -> juce::String
            {
                auto tokens = rowOf(ch, rowId);
                return (col >= 0 && col < tokens.size()) ? tokens[col] : juce::String("<none>");
            };
            auto levelAt = [&](int ch, const juce::Identifier& rowId, int col)
            {
                return cell(ch, rowId, col).getFloatValue();
            };
            auto isAt = [&](int ch, const juce::Identifier& rowId, int col, float wanted)
            {
                return std::abs(levelAt(ch, rowId, col) - wanted) < 1.0e-6f;
            };
            auto onAt = [&](int ch, int col) { return cell(ch, P::effectSendOns, col) == "1"; };

            auto reloadEffects = [&](const char* what)
            {
                check(fm.saveEffectsConfig(), juce::String(what) + ": save the routed session");
                vts.setNumEffectChannels(0);
                check(fm.loadEffectsConfig(), juce::String(what) + ": read it back off disk");
            };

            // How many tokens of a row are NOT at the row's idle value. The
            // strongest form of "the neighbours did not move": one number that
            // catches a write which landed everywhere.
            auto nonIdle = [&](int ch, const juce::Identifier& rowId)
            {
                auto tokens = rowOf(ch, rowId);
                int n = 0;
                for (const auto& t : tokens)
                    if (t.getFloatValue() != 0.0f)
                        ++n;
                return n;
            };

            // ---- X13a: the rows exist, at their widths, at their defaults ----
            // The builder used to return a bare <Sends>, so every channel of
            // every show carried a send matrix that no writer could reach:
            // setEffectParameter only writes where some child already
            // hasProperty(), so an absent row swallows every write for the life
            // of the session.
            // From a CLEAN set: X12 above deliberately leaves a foreign node
            // inside <FxEq1> and proves it survives a round trip, and this phase
            // is about the rows rather than about that fixture. Emptying the
            // family first drops it; the save inside reloadEffects then writes
            // the clean tree over the file X12 left behind.
            vts.setNumEffectChannels(0);
            vts.setNumEffectChannels(3);
            reloadEffects("X13a");
            verifyFamily("X13a (three channels, send matrix stamped)", 3);

            juce::String shapeFault;
            for (int ch = 0; ch < 3 && shapeFault.isEmpty(); ++ch)
            {
                const juce::String who = "effect " + juce::String(ch + 1) + ": ";
                auto sends = vts.getEffectSendsSection(ch);

                if (sends.getNumProperties() != 4)
                    shapeFault = who + "<Sends> carries " + juce::String(sends.getNumProperties())
                               + " properties, expected 4";
                else
                {
                    // THE WIDTHS ARE THE POINT. The input rows are as wide as the
                    // PERMANENT NUMBER space, not as wide as the live channel
                    // list: a number survives a delete, can leave gaps and can be
                    // anything up to the maximum however few channels are live,
                    // so a row fitted to a live count would drop the columns of
                    // channels that still exist.
                    const int widths[] = { rowOf(ch, P::effectSendLevels).size(),
                                           rowOf(ch, P::effectSendOns).size(),
                                           rowOf(ch, P::effectFxSendLevels).size(),
                                           rowOf(ch, P::effectFxSendOns).size() };
                    const int wanted[] = { D::maxInputChannels, D::maxInputChannels,
                                           D::maxEffectChannels, D::maxEffectChannels };
                    const char* names[] = { "effectSendLevels", "effectSendOns",
                                            "effectFxSendLevels", "effectFxSendOns" };
                    for (int r = 0; r < 4 && shapeFault.isEmpty(); ++r)
                        if (widths[r] != wanted[r])
                            shapeFault = who + names[r] + " is " + juce::String(widths[r])
                                       + " columns wide, expected " + juce::String(wanted[r]);

                    if (shapeFault.isEmpty()
                        && (nonIdle(ch, P::effectSendLevels) != 0 || nonIdle(ch, P::effectSendOns) != 0
                            || nonIdle(ch, P::effectFxSendLevels) != 0 || nonIdle(ch, P::effectFxSendOns) != 0))
                        shapeFault = who + "a freshly built row is not all at its default";
                }
            }
            check(shapeFault.isEmpty(),
                  juce::String("X13a: every channel carries four rows at their declared widths")
                      + (shapeFault.isEmpty() ? juce::String() : " - " + shapeFault));

            // ---- X13b: one cell, at its exact value, with quiet neighbours ----
            // The level rows hold dB. Run one through normaliseMuteList - the
            // helper that sits right beside them and looks like it fits - and
            // every send becomes 0 or 1: silence or unity on all 64 columns, with
            // the row still the right width and the right shape.
            check(vts.setEffectSendLevelFromInput(0, 2, -3.0f),  "X13b: a level on the neighbour below");
            check(vts.setEffectSendLevelFromInput(0, 4, -40.0f), "X13b: ...and on the one above");
            check(vts.setEffectSendLevelFromInput(0, 3, -6.5f),  "X13b: the cell between them");
            check(vts.setEffectSendOnFromInput(0, 3, true),      "X13b: ...switched on");
            check(vts.setEffectSendLevelFromInput(0, 17, -12.25f),
                  "X13b: a second cell, far enough up the row to catch a width mistake");
            reloadEffects("X13b");

            check(isAt(0, P::effectSendLevels, 2, -6.5f),
                  "X13b: the level survives the round trip at its EXACT value");
            check(isAt(0, P::effectSendLevels, 1, -3.0f) && isAt(0, P::effectSendLevels, 3, -40.0f),
                  "X13b: ...and both neighbours are exactly where they were left");
            check(isAt(0, P::effectSendLevels, 16, -12.25f),
                  "X13b: ...as is the cell at input 17");
            check(nonIdle(0, P::effectSendLevels) == 4 && nonIdle(0, P::effectSendOns) == 1,
                  "X13b: four levels and one switch moved, and nothing else in either row");
            check(std::abs(vts.getEffectSendLevelFromInput(0, 3) + 6.5f) < 1.0e-6f
                      && vts.getEffectSendOnFromInput(0, 3),
                  "X13b: the accessor reads back what the row text says, keyed by the same number");
            check(std::abs(vts.getEffectSendLevelFromInput(1, 3) - D::effectSendLevelDefault) < 1.0e-6f,
                  "X13b: ...and the other channels were not routed by it");

            // OUT OF RANGE is clamped, not refused and not stored: a level row is
            // the one packed row with a declared range, and the range is what the
            // cell pseudo-identifier's bounds entry promises every later surface.
            check(vts.setEffectSendLevelFromInput(0, 5, -400.0f), "X13b: write a level far below the floor");
            reloadEffects("X13b-clamp");
            check(isAt(0, P::effectSendLevels, 4, D::effectSendLevelMin),
                  "X13b: it lands at the floor, and the row still parses as a row");

            // ---- X13c: a bare number may not eat a row ------------------------
            // Six rows, one hole. inputMutes was guarded after mutes were lost to
            // a QLab cue, an OSC scalar and an MCP enum, each writing a number
            // over the whole list; reverbMutes has been destructible by exactly
            // that route ever since and effectMutes would have inherited it. None
            // of the five has a bounds entry, so the generic numeric clamp never
            // even looks at them.
            {
                const int reverbsBefore = vts.getNumReverbChannels();
                if (reverbsBefore == 0)
                    vts.setNumReverbChannels(1);   // restored below
                check(vts.getNumReverbChannels() > 0, "X13c: a reverb channel to guard");
                check(vts.getNumInputChannels() > 0, "X13c: an input channel to guard");

                const juce::Identifier* rows[] = { &P::inputMutes, &P::reverbMutes, &P::effectMutes,
                                                  &P::effectSendLevels, &P::effectSendOns,
                                                  &P::effectFxSendLevels, &P::effectFxSendOns };

                auto readRow = [&](const juce::Identifier& rowId) -> juce::String
                {
                    if (rowId == P::inputMutes)  return vts.getInputParameter(0, rowId).toString();
                    if (rowId == P::reverbMutes) return vts.getReverbParameter(0, rowId).toString();
                    return vts.getEffectParameter(0, rowId).toString();
                };

                // COLUMN 0 IS ARMED FIRST, and the assertions below are worth
                // nothing without it. A scalar written over a row lands on its
                // FIRST column, and every one of these rows starts idle there, so
                // a guard that refused the write and a guard that took it produce
                // the same row - "0" either way - and the comparison passes for
                // the wrong reason. Armed, the same write has somewhere to show.
                {
                    juce::StringArray armed;
                    for (int i = 0; i < juce::jmax(1, vts.getNumOutputChannels()); ++i)
                        armed.add(i == 0 ? "1" : "0");
                    const juce::String armedRow = armed.joinIntoString(",");

                    check(vts.setInputOutputMute(0, 0, true), "X13c: mute output 1 of input 1");
                    vts.setParameter(P::reverbMutes, armedRow, 0);
                    vts.setParameter(P::effectMutes, armedRow, 0);
                    check(vts.setEffectSendLevelFromInput(0, 1, -2.0f)
                              && vts.setEffectSendOnFromInput(0, 1, true),
                          "X13c: route input 1 into effect 1, so the send rows have a first column too");
                    check(readRow(P::inputMutes).startsWith("1,")
                              && readRow(P::reverbMutes).startsWith("1,")
                              && readRow(P::effectMutes).startsWith("1,")
                              && readRow(P::effectSendOns).startsWith("1,"),
                          "X13c: ...and the first column of all four really is armed");
                }

                juce::StringArray before;
                for (const auto* rowId : rows)
                    before.add(readRow(*rowId));

                juce::String emptyRow;
                for (int r = 0; r < numElementsInArray(rows); ++r)
                    if (before[r].isEmpty())
                        emptyRow = rows[r]->toString();
                check(emptyRow.isEmpty(),
                      juce::String("X13c: all seven rows are on their nodes to begin with")
                          + (emptyRow.isEmpty() ? juce::String() : " - " + emptyRow + " is not"));

                // THE ROUTE THAT DID THE DAMAGE, not a hand-written setProperty:
                // getTreeForParameter resolves the row and writeProperty lands on
                // it, which is where an OSC scalar, an MCP enum and a cue recall
                // all arrive.
                for (int r = 0; r < numElementsInArray(rows); ++r)
                {
                    const int channelIndex = 0;
                    vts.setParameter(*rows[r], 7, channelIndex);      // an int
                    vts.setParameter(*rows[r], 0.5, channelIndex);    // ...and a float
                }

                juce::StringArray eatenRows;
                for (int r = 0; r < numElementsInArray(rows); ++r)
                    if (readRow(*rows[r]) != before[r])
                        eatenRows.add(rows[r]->toString() + " became \"" + readRow(*rows[r]) + "\"");
                const juce::String eaten = eatenRows.joinIntoString("; ");
                check(eaten.isEmpty(),
                      juce::String("X13c: a bare number leaves every one of the seven rows exactly as it was")
                          + (eaten.isEmpty() ? juce::String() : " - " + eaten));

                // ...AND THE SAME SCALAR TYPED AS TEXT, which a clause that tests
                // the TYPE of the write cannot see. reverb_set_mutes is advertised
                // to every MCP client with its value as a STRING ENUM of "unmute"
                // / "MUTE" (Source/Network/MCP/generated_tools.json), and the OSC
                // list form accepts any non-numeric string, so this is the shipped
                // route rather than a hypothesis. One junk token used to tokenise
                // into a full row of DEFAULTS: well-formed, silent, and
                // indistinguishable from a deliberate unmute-all - which is a
                // worse loss than the number that started this guard, not a
                // smaller one.
                const char* notRows[] = { "MUTE", "unmute", "", "   ", "wibble",
                                          "7", "0.5", "1;0;1", "--5",
                                          "MUTE,MUTE", "x,y,z", "1,x,1" };
                for (const auto* text : notRows)
                    for (int r = 0; r < numElementsInArray(rows); ++r)
                        vts.setParameter(*rows[r], juce::String(text), 0);

                juce::StringArray eatenByText;
                for (int r = 0; r < numElementsInArray(rows); ++r)
                    if (readRow(*rows[r]) != before[r])
                        eatenByText.add(rows[r]->toString() + " became \"" + readRow(*rows[r]) + "\"");
                const juce::String eatenText = eatenByText.joinIntoString("; ");
                check(eatenText.isEmpty(),
                      juce::String("X13c: ...and so does a string that is not a row, on all seven")
                          + (eatenText.isEmpty() ? juce::String() : " - " + eatenText));

                // ...and it survives the save too, which is the half that made the
                // original bug permanent: the scalar was written, then saved, and
                // the list was gone from the file as well as from the session.
                check(fm.saveCompleteConfig(), "X13c: save the whole project");
                check(fm.loadCompleteConfig(), "X13c: load it back");
                juce::StringArray lostRows;
                for (int r = 0; r < numElementsInArray(rows); ++r)
                    if (readRow(*rows[r]) != before[r])
                        lostRows.add(rows[r]->toString() + " came back as \"" + readRow(*rows[r]) + "\"");
                const juce::String lost = lostRows.joinIntoString("; ");
                check(lost.isEmpty(),
                      juce::String("X13c: ...and every row comes back off disk unchanged")
                          + (lost.isEmpty() ? juce::String() : " - " + lost));

                // A SHORT ROW NAMES THE COLUMNS IT HAS, and the rest of the row is
                // none of its business. Padded out to the full width with defaults
                // instead, a three-column write clears sixty-one sends nothing
                // asked about - the one-token loss above with three tokens.
                check(vts.setEffectSendLevelFromInput(0, 40, -18.0f)
                          && vts.setEffectSendOnFromInput(0, 40, true),
                      "X13c: route input 40 into effect 1");
                vts.setParameter(P::effectSendLevels, juce::String("-1,-2,-3"), 0);
                check(std::abs(vts.getEffectSendLevelFromInput(0, 40) + 18.0f) < 1.0e-6f,
                      "X13c: a three-column row write leaves column 40 exactly where it was");
                check(std::abs(vts.getEffectSendLevelFromInput(0, 1) + 1.0f) < 1.0e-6f
                          && std::abs(vts.getEffectSendLevelFromInput(0, 3) + 3.0f) < 1.0e-6f,
                      "X13c: ...and takes the three columns it does name as written");

                if (reverbsBefore == 0)
                    vts.setNumReverbChannels(0);
            }

            // ---- X13d: an input delete takes its column with it ---------------
            // removeInputChannel retires a number and leaves a GAP that
            // addInputChannel can hand back out later, and it is followed by a
            // renumber only on a session that has not latched. Both regimes are
            // driven below, because they fail differently: latched, the column is
            // idle only if the delete ZEROED it; unlatched, the compaction must
            // shift the survivors and the zeroing has to happen BEFORE it, or it
            // clears whichever channel moved into the retired number instead.
            const int monoBefore   = vts.getNumInputChannels() - vts.getNumStereoInputChannels();
            const int stereoBefore = vts.getNumStereoInputChannels();
            auto ioLatch = vts.getIOState();
            const bool ownedBefore = static_cast<bool>(ioLatch.getProperty(P::channelNumbersUserOwned, false));
            {
                // Four inputs numbered 1..4, one distinguishable send each. The
                // values matter: a column that moved has to say WHICH channel it
                // belongs to, because a width assertion cannot tell a shift from
                // a rotate and "not the default" cannot either.
                auto armFourInputs = [&](const char* who)
                {
                    vts.setInputChannelCounts(4, 0);
                    reconfig();
                    check(vts.assignInputChannelNumbersBySlot({ 1, 2, 3, 4 }, "self-test X13").wasOk(),
                          juce::String(who) + ": four inputs numbered 1..4");

                    // Cleared through the ROW identifier - the generic path an OSC
                    // or MCP row write takes, and the second way into the
                    // interceptor beside the cell setters.
                    vts.setParameter(P::effectSendLevels,
                                     juce::String::repeatedString("0,", D::maxInputChannels - 1) + "0", 0);
                    vts.setParameter(P::effectSendOns,
                                     juce::String::repeatedString("0,", D::maxInputChannels - 1) + "0", 0);
                    check(nonIdle(0, P::effectSendLevels) == 0 && nonIdle(0, P::effectSendOns) == 0,
                          juce::String(who) + ": a row write through the generic path clears the row it names");

                    for (int number = 1; number <= 4; ++number)
                        check(vts.setEffectSendLevelFromInput(0, number, (float) -number)
                                  && vts.setEffectSendOnFromInput(0, number, true),
                              juce::String(who) + ": route input " + juce::String(number) + " into effect 1");
                };

                // LATCHED: nothing renumbers, so nothing shifts into the hole.
                armFourInputs("X13d");
                check(vts.areChannelNumbersUserOwned(), "X13d: the session is latched");
                check(vts.removeInputChannel(2).wasOk(), "X13d: delete input #2");
                reconfig();
                check(vts.getNumInputChannels() == 3 && vts.getInputChannelNumber(1) == 3,
                      "X13d: three inputs left, still numbered 1,3,4");
                reloadEffects("X13d");

                check(isAt(0, P::effectSendLevels, 1, D::effectSendLevelDefault) && ! onAt(0, 1),
                      "X13d: the retired number's column is idle - the delete zeroed it");
                check(isAt(0, P::effectSendLevels, 0, -1.0f) && isAt(0, P::effectSendLevels, 2, -3.0f)
                          && isAt(0, P::effectSendLevels, 3, -4.0f),
                      "X13d: ...and every survivor kept its own column, by value");
                check(nonIdle(0, P::effectSendLevels) == 3 && nonIdle(0, P::effectSendOns) == 3,
                      "X13d: one column went and no other moved");

                // THE GAP IS REUSABLE, which is what makes the zeroing matter.
                // addInputChannel takes an explicit number precisely so a retired
                // one can be handed back out, and the operator is warned that
                // snapshots and cues addressed to it will reach the new channel.
                // Its sends must not be among them.
                check(vts.addInputChannel(false, 2).wasOk(),
                      "X13d: re-create a channel on the retired number");
                reconfig();
                reloadEffects("X13d-reuse");
                check(isAt(0, P::effectSendLevels, 1, D::effectSendLevelDefault) && ! onAt(0, 1),
                      "X13d: it starts unrouted, instead of inheriting a dead channel's sends");

                // UNLATCHED: the delete is followed by the compaction, so the
                // survivors' numbers move and their columns have to move with
                // them. The latch is lifted for this and put back after - the
                // flag IS the regime, and this session has latched (X7 loaded a
                // project), so there is no other way to reach it here.
                armFourInputs("X13d2");
                ioLatch.setProperty(P::channelNumbersUserOwned, false, nullptr);
                check(vts.removeInputChannel(2).wasOk(), "X13d2: delete input #2 on a fresh session");
                reconfig();
                ioLatch.setProperty(P::channelNumbersUserOwned, true, nullptr);

                check(vts.getNumInputChannels() == 3, "X13d2: three inputs are left");
                check(vts.getInputChannelNumber(0) == 1 && vts.getInputChannelNumber(1) == 2
                          && vts.getInputChannelNumber(2) == 3,
                      "X13d2: ...renumbered 1,2,3 by the compaction");

                reloadEffects("X13d2");

                // BY VALUE, not by width. The surviving columns must still name
                // the channels they were written for: #3 became #2 and #4 became
                // #3, so their levels have to be found at the new numbers.
                check(isAt(0, P::effectSendLevels, 0, -1.0f), "X13d2: input #1 kept its own send");
                check(isAt(0, P::effectSendLevels, 1, -3.0f),
                      "X13d2: the channel that was #3 is now #2 and its send came with it");
                check(isAt(0, P::effectSendLevels, 2, -4.0f),
                      "X13d2: ...and the one that was #4 is now #3");
                check(isAt(0, P::effectSendLevels, 3, D::effectSendLevelDefault) && ! onAt(0, 3),
                      "X13d2: the column the compaction vacated is idle, not a copy of its old occupant");
                check(nonIdle(0, P::effectSendLevels) == 3 && nonIdle(0, P::effectSendOns) == 3,
                      "X13d2: three columns routed, one gone, and no fourth invented");

                // ---- X13e: a relabel that PERMUTES, not one that shifts -------
                // The operator-facing relabel exists so that snapshots, cues and
                // OSC written against the file's numbers still reach the right
                // channel afterwards; a send row keyed by number is one of those
                // references. A SWAP is the case the dense compaction never
                // produces and an incremental remap always loses: moving #1 to #3
                // first overwrites the value #3 still needs, and both channels end
                // up with one of them.
                check(vts.assignInputChannelNumbersBySlot({ 3, 2, 1 }, "self-test X13e").wasOk(),
                      "X13e: swap the numbers of the first and last input");
                reconfig();
                reloadEffects("X13e");

                check(isAt(0, P::effectSendLevels, 0, -4.0f),
                      "X13e: column #1 now holds the send of the channel that took that number");
                check(isAt(0, P::effectSendLevels, 2, -1.0f),
                      "X13e: ...and column #3 holds the other half of the swap");
                check(isAt(0, P::effectSendLevels, 1, -3.0f),
                      "X13e: the channel that kept its number kept its send");
                check(nonIdle(0, P::effectSendLevels) == 3 && nonIdle(0, P::effectSendOns) == 3,
                      "X13e: still three routed columns - a collapsed swap would leave two");
            }

            // ---- X13f: the fx diagonal is off, and stays off ------------------
            // Effect n may not feed itself: that is not a routing choice, it is a
            // unity-gain loop around a delay line. Forced where the row is
            // WRITTEN - the builder, the cell setters, the interceptor and the
            // column maintenance - because a rule enforced only where the row is
            // read is a rule every other reader has to remember.
            {
                juce::String diagonalFault;
                for (int ch = 0; ch < 3 && diagonalFault.isEmpty(); ++ch)
                    if (cell(ch, P::effectFxSendOns, ch) != "0"
                        || ! isAt(ch, P::effectFxSendLevels, ch, D::effectFxSendLevelDefault))
                        diagonalFault = "effect " + juce::String(ch + 1) + " feeds itself out of the builder";
                check(diagonalFault.isEmpty(),
                      juce::String("X13f: the diagonal is off on every freshly built channel")
                          + (diagonalFault.isEmpty() ? juce::String() : " - " + diagonalFault));

                check(! vts.setEffectFxSendOnFromEffect(1, 1, true),
                      "X13f: the cell setter REFUSES the diagonal rather than reporting a write it cannot make");
                check(! vts.setEffectFxSendLevelFromEffect(1, 1, -6.0f),
                      "X13f: ...and so does the level setter");

                // The other door: a whole ROW, every switch on, straight down the
                // generic parameter path an OSC or MCP row write takes.
                vts.setParameter(P::effectFxSendOns,
                                 juce::String::repeatedString("1,", D::maxEffectChannels - 1) + "1", 1);
                reloadEffects("X13f");
                check(cell(1, P::effectFxSendOns, 1) == "0",
                      "X13f: a row write with the diagonal set is stored with it cleared");
                check(cell(1, P::effectFxSendOns, 0) == "1" && cell(1, P::effectFxSendOns, 2) == "1",
                      "X13f: ...and every other column of that row was taken as written");

                // THE LEVEL ROW'S DIAGONAL, forced by the same clause of the same
                // interceptor and asserted here for the first time: a dB sitting
                // in a cell that can never sound is a number no reader may trust,
                // and after a removal shifts the columns it is exactly what would
                // land on the survivor's new diagonal.
                vts.setParameter(P::effectFxSendLevels,
                                 juce::String::repeatedString("-7,", D::maxEffectChannels - 1) + "-7", 1);
                reloadEffects("X13f-levels");
                check(isAt(1, P::effectFxSendLevels, 1, D::effectFxSendLevelDefault),
                      "X13f: a LEVEL row written with the diagonal set is stored with that cell at the default");
                check(isAt(1, P::effectFxSendLevels, 0, -7.0f) && isAt(1, P::effectFxSendLevels, 2, -7.0f),
                      "X13f: ...and every other column of the level row was taken as written");

                // Back to idle before the removal fixture below.
                vts.setParameter(P::effectFxSendOns,
                                 juce::String::repeatedString("0,", D::maxEffectChannels - 1) + "0", 1);
                vts.setParameter(P::effectFxSendLevels,
                                 juce::String::repeatedString("0,", D::maxEffectChannels - 1) + "0", 1);

                // ---- and after a channel removal, at the NEW index ------------
                // The fx rows are keyed by DENSE index, so a delete renumbers
                // their columns exactly as it renumbers the channels. Leave them
                // and every send above the hole re-points one channel down - the
                // quietest kind of wrong, because the matrix still looks full.
                check(vts.setEffectFxSendLevelFromEffect(1, 2, -3.0f) && vts.setEffectFxSendOnFromEffect(1, 2, true),
                      "X13f: effect 2 is fed by effect 3");
                check(vts.setEffectFxSendLevelFromEffect(2, 0, -6.0f) && vts.setEffectFxSendOnFromEffect(2, 0, true),
                      "X13f: effect 3 is fed by effect 1");
                check(vts.setEffectFxSendLevelFromEffect(2, 1, -12.0f) && vts.setEffectFxSendOnFromEffect(2, 1, true),
                      "X13f: ...and by effect 2");

                check(vts.removeEffectChannel(0).wasOk(), "X13f: delete effect 1");
                reloadEffects("X13f-removal");
                verifyFamily("X13f (after an effect channel removal)", 2);

                // Old 2 is index 0 now, old 3 is index 1.
                check(isAt(0, P::effectFxSendLevels, 1, -3.0f) && cell(0, P::effectFxSendOns, 1) == "1",
                      "X13f: the send from old effect 3 followed it down to column 2");
                check(isAt(1, P::effectFxSendLevels, 0, -12.0f) && cell(1, P::effectFxSendOns, 0) == "1",
                      "X13f: the send from old effect 2 followed it down to column 1");
                check(isAt(1, P::effectFxSendLevels, 1, D::effectFxSendLevelDefault)
                          && cell(1, P::effectFxSendOns, 1) == "0",
                      "X13f: the deleted channel's send did not shift onto the survivor's own diagonal");
                check(cell(0, P::effectFxSendOns, 0) == "0",
                      "X13f: ...and the other survivor's diagonal is off at its new index too");
                check(nonIdle(0, P::effectFxSendLevels) == 1 && nonIdle(1, P::effectFxSendLevels) == 1,
                      "X13f: one send each - the deleted column was removed, not blanked in place");
            }

            // ---- X13g: an output count change refits the per-output rows ------
            // effectMutes and reverbMutes are the only two send-matrix rows that
            // FOLLOW a live count, and neither was refitted before: a reverb's row
            // stayed at whatever width it was built at and self-healed only
            // because the reverb tab rewrites it whole, with a hard-coded 16.
            {
                const int outputsBefore = vts.getNumOutputChannels();
                const int reverbsBefore = vts.getNumReverbChannels();
                if (reverbsBefore == 0)
                    vts.setNumReverbChannels(1);

                auto widthOf = [&](juce::ValueTree node, const juce::Identifier& rowId)
                {
                    return juce::StringArray::fromTokens(node.getProperty(rowId).toString(), ",", "").size();
                };
                auto columnOf = [&](juce::ValueTree node, const juce::Identifier& rowId, int col)
                {
                    auto tokens = juce::StringArray::fromTokens(node.getProperty(rowId).toString(), ",", "");
                    return (col >= 0 && col < tokens.size()) ? tokens[col] : juce::String("<none>");
                };
                auto plantRow = [&](juce::ValueTree node, const juce::Identifier& rowId,
                                    int width, int mutedA, int mutedB)
                {
                    juce::StringArray cells;
                    for (int i = 0; i < width; ++i)
                        cells.add((i == mutedA || i == mutedB) ? "1" : "0");
                    node.setProperty(rowId, cells.joinIntoString(","), nullptr);
                };

                // ---- THE PADDING HALF: a narrow row grows to the live count ----
                // Which is the gap this refit was added for: a reverb row left at
                // whatever width it was built at by a tab whose fallback is 16.
                plantRow(vts.getEffectReturnSection(0), P::effectMutes, 4, -1, -1);
                plantRow(vts.getReverbReturnSection(0), P::reverbMutes, 4, -1, -1);
                vts.setNumOutputChannels(outputsBefore);
                check(widthOf(vts.getEffectReturnSection(0), P::effectMutes) == outputsBefore,
                      "X13g: a four-column effectMutes grows to the live output count");
                check(widthOf(vts.getReverbReturnSection(0), P::reverbMutes) == outputsBefore,
                      "X13g: ...and so does reverbMutes, which nothing used to resize");

                // ---- THE HALF THAT DELETES, if it is the same call both ways ---
                // BY VALUE, not by width: a width assertion passes whether the row
                // kept the operator's mutes or was CUT on the way down and padded
                // with "0" on the way back up, which is what fitting a row to the
                // live count in both directions does. Mute a low output and the
                // top one, drop the rig to half its outputs - an interface that
                // disappears, a System Config edit - and bring it back.
                const int lowColumn  = 1;
                const int highColumn = outputsBefore - 1;
                plantRow(vts.getEffectReturnSection(0), P::effectMutes, outputsBefore, lowColumn, highColumn);
                plantRow(vts.getReverbReturnSection(0), P::reverbMutes, outputsBefore, lowColumn, highColumn);
                check(vts.setInputOutputMute(0, highColumn, true),
                      "X13g: ...and the same output muted on input 1");

                const int shrunk = juce::jmax(1, outputsBefore / 2);
                vts.setNumOutputChannels(shrunk);
                handleChannelCountChange();
                check(fm.saveCompleteConfig(), "X13g: save the project on a smaller rig");
                check(fm.loadCompleteConfig(), "X13g: load it back");

                check(columnOf(vts.getEffectReturnSection(0), P::effectMutes, highColumn) == "1",
                      "X13g: the effect's mute on an output the smaller rig has not got is still in the row");
                check(columnOf(vts.getReverbReturnSection(0), P::reverbMutes, highColumn) == "1",
                      "X13g: ...and the reverb's, on a row that has SHIPPED and never had one refit it");
                check(columnOf(vts.getEffectReturnSection(0), P::effectMutes, lowColumn) == "1"
                          && columnOf(vts.getReverbReturnSection(0), P::reverbMutes, lowColumn) == "1",
                      "X13g: ...while the mute on an output the smaller rig does have is untouched");
                check(rowOf(0, P::effectSendLevels).size() == D::maxInputChannels,
                      "X13g: the send rows did NOT follow - their columns are inputs, not outputs");

                vts.setNumOutputChannels(outputsBefore);
                handleChannelCountChange();
                check(fm.saveCompleteConfig(), "X13g: save it back on the original rig");
                check(fm.loadCompleteConfig(), "X13g: load that");
                check(widthOf(vts.getEffectReturnSection(0), P::effectMutes) == outputsBefore,
                      "X13g: effectMutes is at the live width again");
                check(widthOf(vts.getReverbReturnSection(0), P::reverbMutes) == outputsBefore,
                      "X13g: ...and reverbMutes with it");
                check(columnOf(vts.getEffectReturnSection(0), P::effectMutes, highColumn) == "1"
                          && columnOf(vts.getReverbReturnSection(0), P::reverbMutes, highColumn) == "1",
                      "X13g: and the mute at the top of the rig came back with the outputs, not as a 0");

                // inputMutes takes the same trip, on the row the other two were
                // modelled on. The one case this cannot make is a rig of exactly
                // 64 or maxOutputChannels outputs: at those two widths a preserved
                // row cannot be told from the legacy grid list, whose tail the
                // keepTokens window exists to zero. This rig is 16.
                if (outputsBefore != 64 && outputsBefore != D::maxOutputChannels)
                {
                    auto mutesSection = vts.getInputMutesSection(0);
                    check(mutesSection.isValid()
                              && columnOf(mutesSection, P::inputMutes, highColumn) == "1",
                          "X13g: an input's mute on the top output survived the same round trip");
                }

                if (reverbsBefore == 0)
                    vts.setNumReverbChannels(0);
                handleChannelCountChange();
            }

            // ---- X13h: a file's rows are canonicalised ON THE WAY IN ---------
            // Every accessor canonicalises what it READS, so the app was already
            // safe from a row a file carries in the wrong shape - and only the
            // app. The stored text is what the next save writes back, so a
            // self-feed hand-edited into effects.xml stayed in that operator's
            // file indefinitely: a unity-gain loop around a delay line that no
            // load and no save was ever going to take out. A junk level token sat
            // there just as long, reading as 0 dB - UNITY - because "at least one
            // digit and nothing outside the characters a number uses" passes
            // "--5", and getFloatValue() answers 0 for it.
            {
                auto cellsOf = [](const juce::String& row)
                {
                    return juce::StringArray::fromTokens(row, ",", "");
                };
                vts.setNumEffectChannels(2);
                check(fm.saveEffectsConfig(), "X13h: save two channels the app itself wrote");

                if (auto doc = juce::XmlDocument::parse(effectsFile()))
                {
                    auto* effectsEl = doc->getChildByName(P::Effects.toString());
                    auto* first = effectsEl != nullptr ? effectsEl->getChildByName(P::Effect.toString())
                                                       : nullptr;
                    auto* sends = first != nullptr ? first->getChildByName(P::Sends.toString()) : nullptr;
                    check(sends != nullptr, "X13h: the saved channel carries a <Sends> to hand-edit");

                    if (sends != nullptr)
                    {
                        auto fxOns    = cellsOf(sends->getStringAttribute(P::effectFxSendOns.toString()));
                        auto fxLevels = cellsOf(sends->getStringAttribute(P::effectFxSendLevels.toString()));
                        auto levels   = cellsOf(sends->getStringAttribute(P::effectSendLevels.toString()));

                        fxOns.set(0, "1");         // effect 1 feeding ITSELF
                        fxLevels.set(0, "-6");     // ...at a level, on the diagonal
                        fxLevels.set(1, "-500");   // ...and one far below the floor
                        levels.set(0, "--5");      // junk that parses to 0 dB, which is unity
                        levels.set(1, "1e400");    // ...and junk that parses to +infinity

                        sends->setAttribute(P::effectFxSendOns.toString(),    fxOns.joinIntoString(","));
                        sends->setAttribute(P::effectFxSendLevels.toString(), fxLevels.joinIntoString(","));
                        sends->setAttribute(P::effectSendLevels.toString(),   levels.joinIntoString(","));
                        check(doc->writeTo(effectsFile()), "X13h: write the hand-edited routing back");
                    }
                }
                else
                {
                    check(false, "X13h: the saved file parses");
                }

                vts.setNumEffectChannels(0);
                check(fm.loadEffectsConfig(), "X13h: load the hand-edited file");

                check(cell(0, P::effectFxSendOns, 0) == "0",
                      "X13h: the self-feed is off in the TREE, not only in what the accessors answer");
                check(isAt(0, P::effectFxSendLevels, 0, D::effectFxSendLevelDefault),
                      "X13h: ...and the level that sat on the diagonal is back at the default");
                check(isAt(0, P::effectFxSendLevels, 1, D::effectFxSendLevelMin),
                      "X13h: a level far below the floor is clamped to it");
                check(cell(0, P::effectSendLevels, 0) == juce::String(D::effectSendLevelDefault)
                          && cell(0, P::effectSendLevels, 1) == juce::String(D::effectSendLevelDefault),
                      "X13h: and both junk tokens are re-defaulted instead of reading back as unity");

                // AND IT IS IN THE FILE. A repair that lives only in this session
                // leaves the landmine where it was: the next load finds it again,
                // and so does everything else that reads the operator's file.
                check(fm.saveEffectsConfig(), "X13h: save the repaired tree");
                if (auto doc = juce::XmlDocument::parse(effectsFile()))
                {
                    auto* effectsEl = doc->getChildByName(P::Effects.toString());
                    auto* first = effectsEl != nullptr ? effectsEl->getChildByName(P::Effect.toString())
                                                       : nullptr;
                    auto* sends = first != nullptr ? first->getChildByName(P::Sends.toString()) : nullptr;
                    check(sends != nullptr
                              && cellsOf(sends->getStringAttribute(P::effectFxSendOns.toString()))[0] == "0",
                          "X13h: the file the operator keeps no longer carries the self-feed");
                    check(sends != nullptr
                              && ! sends->getStringAttribute(P::effectSendLevels.toString()).contains("--5"),
                          "X13h: ...nor the junk token");
                }
                else
                {
                    check(false, "X13h: the repaired file parses");
                }
            }

            // Leave the input list roughly as this phase found it. The numbers
            // cannot be restored - X13d deleted one - but the counts and the
            // latch can, and nothing below this reads either.
            vts.setInputChannelCounts(juce::jmax(1, monoBefore), stereoBefore);
            reconfig();
            ioLatch.setProperty(P::channelNumbersUserOwned, ownedBefore, nullptr);
        }

        // Leave nothing behind: the folder, and the count this phase raised.
        vts.setNumEffectChannels(0);
        fm.setProjectFolder(previousProject);
        tempProject.deleteRecursively();
    }

    // ---- Y: the calculation engine renders an effect return as a source -----
    // Nothing in the app installs a render-source map with effect returns yet
    // (recomputeRenderSourceCount still builds without effects), so this phase
    // builds one by hand from the live channel types, installs it in the
    // calculation engine and reads the matrices back. Every assertion names a
    // mechanism that has no other caller today: the kind-aware position, the
    // return rows, the feed matrix and its user cells, the diagonal, the two
    // solo masks, the latency rule and the cycle mask.
    {
        namespace P = WFSParameterIDs;
        namespace D = WFSParameterDefaults;
        using Map = spatcore::wfs::RenderSourceMap;
        using Kind = spatcore::wfs::SourceKind;

        auto* calc = calculationEngine.get();
        check(calc != nullptr, "Y0: the calculation engine exists");

        if (calc != nullptr)
        {
            const int inputsBefore = vts.getNumInputChannels();
            const int stereoBefore = vts.getNumStereoInputChannels();
            const bool effectLatchBefore = vts.areEffectPositionsUserOwned();

            vts.setNumEffectChannels(2);

            // The live channel types plus two effect returns: the map the app
            // itself will build once the effects count reaches it
            std::array<uint8_t, Map::kMaxInputChannels> types {};
            const int numTypes = juce::jlimit(0, (int) Map::kMaxInputChannels, inputsBefore);
            for (int i = 0; i < numTypes; ++i)
                if (vts.isInputChannelStereo(i))
                    types[(size_t) i] = Map::Stereo;

            Map map;
            check(Map::build(types.data(), numTypes, 2, map), "Y0: a map with two effect returns builds");
            const int firstFx = map.firstEffectSlot;
            check(firstFx == inputsBefore + 5 * stereoBefore, "Y0: the returns follow the inputs and their slices");
            check(map.count == firstFx + 2, "Y0: the map counts the two returns");
            const int stride = calc->getNumEffects();
            check(stride == D::maxEffectChannels, "Y0: the feed stride is the effects budget");

            // Both effects far upstage (behind every speaker facing the
            // audience, so every return row reaches the array) and hearing
            // everything (angleOn 180 = no feed cone), so the assertions below
            // are about the mechanisms and not about the rig's geometry.
            vts.setEffectParameter(0, P::effectPositionX, 0.0f);
            vts.setEffectParameter(0, P::effectPositionY, 40.0f);
            vts.setEffectParameter(0, P::effectPositionZ, 3.0f);
            vts.setEffectParameter(1, P::effectPositionX, 3.0f);
            vts.setEffectParameter(1, P::effectPositionY, 40.0f);
            vts.setEffectParameter(1, P::effectPositionZ, 3.0f);
            vts.setEffectParameter(0, P::effectAngleOn, 180);
            vts.setEffectParameter(1, P::effectAngleOn, 180);

            calc->setRenderSourceMap(map);
            calc->recalculateAllEffectPositions();
            calc->recalculateMatrix(nullptr);

            const int numOutputs = calc->getNumOutputs();
            const int liveOutputs = vts.getNumOutputChannels();
            auto cellOut  = [&](int slot, int out) { return calc->getLevels()[(size_t) (slot * numOutputs + out)]; };
            auto delayOut = [&](int slot, int out) { return calc->getDelayTimesMs()[(size_t) (slot * numOutputs + out)]; };
            auto cellFx   = [&](int slot, int fx)  { return calc->getInputEffectLevels()[(size_t) (slot * stride + fx)]; };
            auto delayFx  = [&](int slot, int fx)  { return calc->getInputEffectDelayTimesMs()[(size_t) (slot * stride + fx)]; };
            auto rowMax   = [&](int slot)
            {
                float m = 0.0f;
                for (int o = 0; o < liveOutputs; ++o)
                    m = juce::jmax(m, cellOut(slot, o));
                return m;
            };
            auto allOff = [&]
            {
                juce::StringArray row;
                for (int o = 0; o < liveOutputs; ++o)
                    row.add("0");
                return row.joinIntoString(",");
            };

            // Y1: kinds and the position
            check(calc->getSourceKind(firstFx) == Kind::EffectReturn, "Y1: the return slot is an effect return");
            check(calc->getSourceKind(0) == Kind::Input, "Y1: slot 0 is still an input");
            check(calc->getOwningEffectChannel(firstFx) == 0, "Y1: the return slot names effect 0");
            check(calc->getOwningInputChannel(firstFx) == -1, "Y1: the return slot owns no input");
            {
                const auto rp = calc->getRenderSourcePosition(firstFx);
                check(std::abs(rp.x) < 1e-4f && std::abs(rp.y - 40.0f) < 1e-4f && std::abs(rp.z - 3.0f) < 1e-4f,
                      "Y1: the return renders at its position, not at the origin");
            }

            // Y2: the return row of the in x out matrix
            check(rowMax(firstFx) > 0.0f, "Y2: the return row reaches the array");
            {
                bool frZero = true;
                for (int o = 0; o < liveOutputs; ++o)
                    frZero = frZero && calc->getFRLevels()[(size_t) (firstFx * numOutputs + o)] == 0.0f;
                check(frZero, "Y2: a return has no floor reflection");

                int loudOut = -1;
                for (int o = 0; o < liveOutputs && loudOut < 0; ++o)
                    if (cellOut(firstFx, o) > 0.0f)
                        loudOut = o;
                check(loudOut >= 0, "Y2: an output hears the return");

                if (loudOut >= 0)
                {
                    const float before = cellOut(firstFx, loudOut);

                    juce::StringArray mutes;
                    for (int o = 0; o < liveOutputs; ++o)
                        mutes.add(o == loudOut ? "1" : "0");
                    vts.setEffectParameter(0, P::effectMutes, mutes.joinIntoString(","));
                    calc->recalculateMatrix(nullptr);
                    check(cellOut(firstFx, loudOut) == 0.0f, "Y2: effectMutes silences that output");

                    vts.setEffectParameter(0, P::effectMutes, allOff());
                    vts.setEffectParameter(0, P::effectAttenuation, -6.0f);
                    calc->recalculateMatrix(nullptr);
                    check(std::abs(cellOut(firstFx, loudOut) / before - 0.501187f) < 1e-3f,
                          "Y2: effectAttenuation trims the return by 6 dB");
                    vts.setEffectParameter(0, P::effectAttenuation, 0.0f);
                }
            }

            // Y3: the feed matrix, input 0 -> effect 0
            {
                const int in0Number = vts.getInputChannelNumber(0);
                bool allZero = true;
                for (int s = 0; s < map.count; ++s)
                    for (int fx = 0; fx < 2; ++fx)
                        allZero = allZero && cellFx(s, fx) == 0.0f;
                check(allZero, "Y3: every feed cell is 0 while every send is off");
                check(delayFx(0, 0) > 0.0f, "Y3: a closed cell still carries its geometric delay");

                check(vts.setEffectSendOnFromInput(0, in0Number, true), "Y3: the send switch takes");
                check(vts.setEffectSendLevelFromInput(0, in0Number, 0.0f), "Y3: the send level takes");
                calc->recalculateMatrix(nullptr);
                const float openCell = cellFx(0, 0);
                check(openCell > 0.0f && openCell <= 1.0f, "Y3: an open send feeds effect 0 from input 0");

                auto attenSection = vts.getInputAttenuationSection(0);
                auto channelSection = vts.getInputChannelSection(0);
                auto positionSection = vts.getInputPositionSection(0);
                const int law = attenSection.getProperty(P::inputAttenuationLaw, D::inputAttenuationLawDefault);
                const int common = attenSection.getProperty(P::inputCommonAtten, D::inputCommonAttenDefault);
                const int heightPercent = positionSection.getProperty(P::inputHeightFactor, D::inputHeightFactorDefault);
                const float distAtten = attenSection.getProperty(P::inputDistanceAttenuation, D::inputDistanceAttenuationDefault);
                const float trim = channelSection.getProperty(P::inputAttenuation, D::inputAttenuationDefault);
                if (law == 0 && common == 100 && heightPercent == 100)
                {
                    const auto ip = calc->getRenderSourcePosition(0);
                    const auto fp = calc->getEffectFeedPosition(0);
                    const float d = std::sqrt((fp.x - ip.x) * (fp.x - ip.x) + (fp.y - ip.y) * (fp.y - ip.y)
                                              + (fp.z - ip.z) * (fp.z - ip.z));
                    const float expected = std::pow(10.0f, juce::jlimit(-92.0f, 0.0f, trim + distAtten * d) / 20.0f);
                    check(std::abs(openCell - expected) < 1e-3f, "Y3: the open cell is the geometric level");
                }

                vts.setEffectSendLevelFromInput(0, in0Number, -6.0f);
                calc->recalculateMatrix(nullptr);
                check(std::abs(cellFx(0, 0) / openCell - 0.501187f) < 1e-3f, "Y3: the send level scales the cell");
                check(cellFx(0, 1) == 0.0f, "Y3: effect 1's cell stays closed");

                // A stereo channel's derived rows carry the owner's cell at
                // their own geometry; with no slice geometry pushed yet they
                // sit on the anchor at unity, so the rows are bit-equal
                int stereoSlot = -1;
                for (int s = 0; s < inputsBefore && stereoSlot < 0; ++s)
                    if (vts.isInputChannelStereo(s))
                        stereoSlot = s;
                if (stereoSlot >= 0)
                {
                    vts.setEffectSendOnFromInput(0, vts.getInputChannelNumber(stereoSlot), true);
                    calc->recalculateMatrix(nullptr);
                    const int derived = map.firstDerivedSlot[(size_t) stereoSlot];
                    check(derived >= 0 && cellFx(stereoSlot, 0) > 0.0f
                              && cellFx(derived, 0) == cellFx(stereoSlot, 0),
                          "Y3: a derived slice row carries the owner's send cell");
                }
            }

            // Y4: effect -> effect
            {
                check(vts.setEffectFxSendOnFromEffect(1, 0, true), "Y4: the switch 'effect 1 receives effect 0' takes");
                vts.setEffectParameter(0, P::effectMinimalLatency, 0);   // mode 0 keeps the geometric delay visible
                calc->recalculateMatrix(nullptr);
                check(cellFx(firstFx, 1) > 0.0f, "Y4: effect 0's return feeds effect 1");
                check(cellFx(firstFx + 1, 0) == 0.0f, "Y4: effect 1's return does not feed effect 0");
                check(cellFx(firstFx, 0) == 0.0f, "Y4: the diagonal is closed");
                check(delayFx(firstFx, 1) > 0.0f, "Y4: a geometric effect feed carries a delay");

                juce::StringArray ones;
                for (int i = 0; i < D::maxEffectChannels; ++i)
                    ones.add("1");
                vts.setEffectParameter(0, P::effectFxSendOns, ones.joinIntoString(","));
                calc->recalculateMatrix(nullptr);
                check(cellFx(firstFx, 0) == 0.0f, "Y4: a diagonal planted through the row string stays closed");
                check(vts.getEffectFxSendOnFromEffect(0, 1), "Y4: ...while the row's other cells opened");

                vts.setParameter(P::effectsGlobalFxFeedGeometric, 0);
                calc->recalculateMatrix(nullptr);
                check(delayFx(firstFx, 1) == 0.0f, "Y4: a matrix-only effect feed carries no delay");
                check(std::abs(cellFx(firstFx, 1) - 1.0f) < 1e-6f, "Y4: ...and its cell is the user gain (0 dB = 1)");
                vts.setParameter(P::effectsGlobalFxFeedGeometric, 1);
                calc->recalculateMatrix(nullptr);
                check(delayFx(firstFx, 1) > 0.0f, "Y4: geometric again, the delay is back");
            }

            // Y5: the two solo masks
            {
                vts.setEffectParameter(1, P::effectSolo, 1);
                calc->recalculateMatrix(nullptr);
                check(rowMax(firstFx) == 0.0f, "Y5: soloing effect 1 silences effect 0's return row");
                check(rowMax(firstFx + 1) > 0.0f, "Y5: ...and leaves effect 1's");
                vts.setEffectParameter(1, P::effectSolo, 0);

                calc->setSoloEffects(true);
                calc->recalculateMatrix(nullptr);
                check(rowMax(0) == 0.0f, "Y5: solo effects silences input 0's direct row");
                check(rowMax(firstFx) > 0.0f, "Y5: ...and leaves the returns");
                calc->setSoloEffects(false);
                calc->recalculateMatrix(nullptr);
                check(rowMax(0) > 0.0f, "Y5: input 0's direct row is back");
            }

            // Y6: no render-latency reference on a return; the delay trim applies
            {
                vts.setEffectParameter(0, P::effectDelayLatency, 0.0f);
                calc->recalculateMatrix(nullptr);
                std::vector<float> d0((size_t) liveOutputs);
                for (int o = 0; o < liveOutputs; ++o)
                    d0[(size_t) o] = delayOut(firstFx, o);

                // The reference dirties the INPUT rows only, so the return rows
                // must be forced through a recompute here or this assertion
                // passes for the wrong reason (a mutation adding the term to the
                // return rows went unnoticed until the recompute was forced)
                calc->setChannelIntrinsicLatency(0, 3.0f);
                calc->recalculateAllEffectPositions();
                calc->recalculateMatrix(nullptr);
                bool unchanged = true;
                for (int o = 0; o < liveOutputs; ++o)
                    unchanged = unchanged && delayOut(firstFx, o) == d0[(size_t) o];
                check(unchanged, "Y6: the render-latency reference never reaches a return row");
                calc->setChannelIntrinsicLatency(0, 0.0f);

                vts.setEffectParameter(0, P::effectDelayLatency, 10.0f);
                calc->recalculateMatrix(nullptr);
                bool trimmed = true;
                int compared = 0;
                for (int o = 0; o < liveOutputs; ++o)
                {
                    if (cellOut(firstFx, o) <= 0.0f || d0[(size_t) o] < 0.5f)
                        continue;   // silent or clamped cells say nothing about the trim
                    trimmed = trimmed && std::abs(delayOut(firstFx, o) - (d0[(size_t) o] + 10.0f)) < 1e-3f;
                    ++compared;
                }
                check(trimmed && compared > 0, "Y6: effectDelayLatency adds exactly 10 ms to the return row");
                vts.setEffectParameter(0, P::effectDelayLatency, 0.0f);
            }

            // Y7: the cycle mask (Y4 left effect 0 receiving everyone, and
            // effect 1 receiving effect 0: A -> B -> A)
            {
                calc->recalculateMatrix(nullptr);
                check(calc->getEffectCycleMask() == 0x3u, "Y7: A -> B -> A is reported for both effects");
                vts.setEffectFxSendOnFromEffect(1, 0, false);
                calc->recalculateMatrix(nullptr);
                check(calc->getEffectCycleMask() == 0u, "Y7: breaking one leg clears the cycle");
            }

            // Leave nothing behind: the channels, the latch they tripped, the
            // latency reference, and the app's own map
            vts.setNumEffectChannels(0);
            if (! effectLatchBefore)
                vts.getEffectsState().setProperty(P::effectPositionsUserOwned, 0, nullptr);
            calc->setChannelIntrinsicLatency(0, 0.0f);
            recomputeRenderSourceCount();
            calc->recalculateMatrix(nullptr);
        }
    }

    // ---- A: the per-array trim reaches the return rows ---------------------
    // effectArrayAtten1..10 (R5-4) completes the third matrix level, which had
    // a per-output mute and no level at all. The trim is PER EFFECT and per
    // ARRAY: it lives on this channel's <Return> and is applied against each
    // output's array assignment, so a trim on one array must move exactly the
    // outputs of that array, on exactly the effect that carries it, and leave
    // every other cell bit-identical. The hook it fills was a zero-filled local
    // that no test could distinguish from a trim that does nothing.
    {
        namespace P = WFSParameterIDs;
        using Map = spatcore::wfs::RenderSourceMap;

        auto* calc = calculationEngine.get();
        check(calc != nullptr, "A0: the calculation engine exists");

        if (calc != nullptr && vts.getNumOutputChannels() >= 2)
        {
            const int inputsBefore = vts.getNumInputChannels();
            const bool effectLatchBefore = vts.areEffectPositionsUserOwned();
            const int liveOutputs = vts.getNumOutputChannels();

            // Two outputs on two different arrays, so "moved" and "untouched"
            // are both observable in one matrix. Restored at the end.
            const int arrayBefore0 = WFSVar::toInt (vts.getOutputParameter (0, P::outputArray));
            const int arrayBefore1 = WFSVar::toInt (vts.getOutputParameter (1, P::outputArray));
            vts.setOutputParameter (0, P::outputArray, 1);
            vts.setOutputParameter (1, P::outputArray, 2);

            vts.setNumEffectChannels (2);

            std::array<uint8_t, Map::kMaxInputChannels> types {};
            const int numTypes = juce::jlimit (0, (int) Map::kMaxInputChannels, inputsBefore);
            for (int i = 0; i < numTypes; ++i)
                if (vts.isInputChannelStereo (i))
                    types[(size_t) i] = Map::Stereo;

            Map map;
            check (Map::build (types.data(), numTypes, 2, map), "A0: a map with two effect returns builds");
            const int firstFx = map.firstEffectSlot;

            // Upstage of the array and with no feed cone, exactly as phase Y
            // does, so the assertions are about the trim and not the geometry.
            for (int fx = 0; fx < 2; ++fx)
            {
                vts.setEffectParameter (fx, P::effectPositionX, (float) (3 * fx));
                vts.setEffectParameter (fx, P::effectPositionY, 40.0f);
                vts.setEffectParameter (fx, P::effectPositionZ, 3.0f);
                vts.setEffectParameter (fx, P::effectAngleOn, 180);
            }

            calc->setRenderSourceMap (map);
            calc->recalculateAllEffectPositions();
            calc->recalculateMatrix (nullptr);

            const int numOutputs = calc->getNumOutputs();
            auto cellOut = [&] (int slot, int out)
            {
                return calc->getLevels()[(size_t) (slot * numOutputs + out)];
            };

            std::vector<float> before ((size_t) liveOutputs * 2, 0.0f);
            for (int fx = 0; fx < 2; ++fx)
                for (int o = 0; o < liveOutputs; ++o)
                    before[(size_t) (fx * liveOutputs + o)] = cellOut (firstFx + fx, o);

            check (before[0] > 1e-4f && before[1] > 1e-4f,
                   "A1: both compared outputs carry the untrimmed return, well clear of the -92 dB clamp");

            // A2: -6 dB on array 1 of effect 0 only
            vts.setEffectParameter (0, WFSValueTreeState::getEffectArrayAttenId (0), -6.0f);
            calc->recalculateMatrix (nullptr);

            const float expected = std::pow (10.0f, -6.0f / 20.0f);
            int movedInArray1 = 0, wrongInArray1 = 0, movedElsewhere = 0, movedOnEffect1 = 0;

            for (int o = 0; o < liveOutputs; ++o)
            {
                const int arrayNum = WFSVar::toInt (vts.getOutputParameter (o, P::outputArray));
                const float b0 = before[(size_t) o];
                const float a0 = cellOut (firstFx, o);
                const float b1 = before[(size_t) (liveOutputs + o)];
                const float a1 = cellOut (firstFx + 1, o);

                if (arrayNum == 1)
                {
                    if (b0 > 1e-4f)
                    {
                        ++movedInArray1;
                        if (std::abs (a0 / b0 - expected) > 1e-3f)
                            ++wrongInArray1;
                    }
                }
                else if (std::abs (a0 - b0) > 1e-6f)
                {
                    ++movedElsewhere;
                }

                if (std::abs (a1 - b1) > 1e-6f)
                    ++movedOnEffect1;
            }

            check (movedInArray1 > 0 && wrongInArray1 == 0,
                   "A2: a -6 dB trim on array 1 scales exactly the array-1 cells of that return");
            check (movedElsewhere == 0, "A2: outputs outside array 1 are untouched");
            check (movedOnEffect1 == 0, "A2: the trim is per effect - the other return does not move");

            // A3: the trim follows the ARRAY, not the output index. Moving
            // output 1 into array 1 must bring it under the same trim without
            // any write to the effect.
            vts.setOutputParameter (1, P::outputArray, 1);
            calc->recalculateMatrix (nullptr);
            const float b1 = before[1];
            const float a1 = cellOut (firstFx, 1);
            check (b1 > 1e-4f && std::abs (a1 / b1 - expected) < 1e-3f,
                   "A3: an output moved into array 1 picks the trim up from its assignment");

            // A4: back to 0 dB restores the row exactly
            vts.setOutputParameter (1, P::outputArray, 2);
            vts.setEffectParameter (0, WFSValueTreeState::getEffectArrayAttenId (0),
                                    WFSParameterDefaults::effectArrayAttenDefault);
            calc->recalculateMatrix (nullptr);
            int notRestored = 0;
            for (int o = 0; o < liveOutputs; ++o)
                if (std::abs (cellOut (firstFx, o) - before[(size_t) o]) > 1e-6f)
                    ++notRestored;
            check (notRestored == 0, "A4: clearing the trim restores every cell of the row");

            // Leave nothing behind
            vts.setOutputParameter (0, P::outputArray, arrayBefore0);
            vts.setOutputParameter (1, P::outputArray, arrayBefore1);
            vts.setNumEffectChannels (0);
            if (! effectLatchBefore)
                vts.getEffectsState().setProperty (P::effectPositionsUserOwned, 0, nullptr);
            recomputeRenderSourceCount();
            calc->recalculateMatrix (nullptr);
        }
        else
        {
            check (vts.getNumOutputChannels() >= 2, "A0: the session has at least two outputs to compare");
        }
    }

    // ---- L: a link group propagates, and every member can leave it ---------
    // The funnel is the output ARRAY's, not the cluster one (R5-6): membership
    // plus a mode on every member, and the receiver's mode consulted as well as
    // the origin's. Everything below is about that asymmetry and about what a
    // group must never share - mutes above all (R5-1), which propagation cannot
    // express and a group ACTION can (R5-2).
    {
        namespace P = WFSParameterIDs;
        namespace D = WFSParameterDefaults;

        const bool effectLatchBefore = vts.areEffectPositionsUserOwned();

        // Three channels: 1 and 2 in group 1, channel 3 left unlinked as the
        // control that must never move.
        vts.setNumEffectChannels(3);

        auto linkMode = [&](int fx) { return vts.getEffectLinkMode(fx); };
        auto attenOf  = [&](int fx)
        {
            return static_cast<float>(static_cast<double>(vts.getEffectParameter(fx, P::effectAttenuation)));
        };

        check(vts.getNumEffectChannels() == 3, "L0: three effects channels exist");
        check(linkMode(0) == vts.getDefaultEffectLinkMode(),
              "L0: a new channel is stamped with the global link mode, not reading it live");

        for (int fx = 0; fx < 2; ++fx)
        {
            vts.setEffectParameter(fx, P::effectLinkGroup, 1);
            vts.setEffectParameter(fx, P::effectLinkMode, 1);   // ABSOLUTE
        }
        vts.setEffectParameter(2, P::effectLinkGroup, 0);

        for (int fx = 0; fx < 3; ++fx)
            vts.setEffectParameter(fx, P::effectAttenuation, -10.0f);

        // L1: ABSOLUTE on both sides copies the value
        vts.setEffectParameterWithLinkPropagation(0, P::effectAttenuation, -4.0f, true);
        check(std::abs(attenOf(0) + 4.0f) < 1e-4f && std::abs(attenOf(1) + 4.0f) < 1e-4f,
              "L1: absolute on both sides copies the value to the member");
        check(std::abs(attenOf(2) + 10.0f) < 1e-4f, "L1: an unlinked channel never moves");

        // L2: RELATIVE keeps the member's offset
        vts.setEffectParameter(0, P::effectAttenuation, -4.0f);
        vts.setEffectParameter(1, P::effectAttenuation, -20.0f);
        vts.setEffectParameter(1, P::effectLinkMode, 2);        // the RECEIVER asks for relative
        vts.setEffectParameterWithLinkPropagation(0, P::effectAttenuation, -6.0f, true);
        check(std::abs(attenOf(0) + 6.0f) < 1e-4f && std::abs(attenOf(1) + 22.0f) < 1e-4f,
              "L2: relative moves the member by the delta and keeps its offset");

        // L3: the delta clamps to the parameter's own bounds
        vts.setEffectParameter(0, P::effectAttenuation, -4.0f);
        vts.setEffectParameter(1, P::effectAttenuation, -90.0f);
        vts.setEffectParameterWithLinkPropagation(0, P::effectAttenuation, -20.0f, true);
        check(std::abs(attenOf(1) - D::effectAttenuationMin) < 1e-4f,
              "L3: a relative member clamps at the parameter's minimum instead of running past it");

        // L4: the RECEIVER's mode is what detaches it (R5-6)
        vts.setEffectParameter(1, P::effectLinkMode, 0);        // OFF, from the member's side
        vts.setEffectParameter(0, P::effectAttenuation, -4.0f);
        vts.setEffectParameter(1, P::effectAttenuation, -30.0f);
        vts.setEffectParameterWithLinkPropagation(0, P::effectAttenuation, -8.0f, true);
        check(std::abs(attenOf(1) + 30.0f) < 1e-4f,
              "L4: a member set to OFF is skipped although the origin still propagates");

        // L5: and the ORIGIN's mode stops it leaving
        vts.setEffectParameter(1, P::effectLinkMode, 1);
        vts.setEffectParameter(0, P::effectLinkMode, 0);
        vts.setEffectParameter(1, P::effectAttenuation, -30.0f);
        vts.setEffectParameterWithLinkPropagation(0, P::effectAttenuation, -12.0f, true);
        check(std::abs(attenOf(1) + 30.0f) < 1e-4f,
              "L5: a detached origin writes only itself");
        vts.setEffectParameter(0, P::effectLinkMode, 1);

        // L6: leaving the group entirely
        vts.setEffectParameter(1, P::effectLinkGroup, 2);
        vts.setEffectParameter(1, P::effectAttenuation, -30.0f);
        vts.setEffectParameterWithLinkPropagation(0, P::effectAttenuation, -16.0f, true);
        check(std::abs(attenOf(1) + 30.0f) < 1e-4f, "L6: another group does not receive");
        vts.setEffectParameter(1, P::effectLinkGroup, 1);

        // L7: an absolute-only parameter is copied even in relative mode -
        // a toggle has no offset, and a delta would invert matching members
        vts.setEffectParameter(0, P::effectLinkMode, 2);
        vts.setEffectParameter(1, P::effectLinkMode, 2);
        vts.setEffectParameter(0, P::effectMinimalLatency, 0);
        vts.setEffectParameter(1, P::effectMinimalLatency, 0);
        vts.setEffectParameterWithLinkPropagation(0, P::effectMinimalLatency, 1, true);
        check(WFSVar::toInt(vts.getEffectParameter(1, P::effectMinimalLatency)) == 1,
              "L7: a discrete parameter is copied absolutely even when both sides say relative");
        vts.setEffectParameter(0, P::effectLinkMode, 1);
        vts.setEffectParameter(1, P::effectLinkMode, 1);

        // L8: the exclusions. Position is identity, mute is independence.
        vts.setEffectParameter(1, P::effectPositionX, 7.0f);
        vts.setEffectParameterWithLinkPropagation(0, P::effectPositionX, -7.0f, true);
        check(std::abs(static_cast<float>(static_cast<double>(
                  vts.getEffectParameter(1, P::effectPositionX))) - 7.0f) < 1e-4f,
              "L8: position never propagates - it is the channel's identity in the show");

        vts.setEffectParameter(0, P::effectMute, 0);
        vts.setEffectParameter(1, P::effectMute, 0);
        vts.setEffectParameterWithLinkPropagation(0, P::effectMute, 1, true);
        check(WFSVar::toInt(vts.getEffectParameter(1, P::effectMute)) == 0,
              "L8: mute never propagates, so a single channel stays independently mutable (R5-1)");

        // L9: the group mute is an ACTION - it writes every member once and
        // leaves each of them independently editable (R5-2)
        vts.setEffectParameter(0, P::effectMute, 0);
        vts.setEffectParameter(1, P::effectMute, 0);
        vts.setEffectGroupMute(1, true);
        check(WFSVar::toInt(vts.getEffectParameter(0, P::effectMute)) == 1
           && WFSVar::toInt(vts.getEffectParameter(1, P::effectMute)) == 1,
              "L9: the group mute writes every member of the group");
        check(WFSVar::toInt(vts.getEffectParameter(2, P::effectMute)) == 0,
              "L9: and reaches no channel outside it");
        vts.setEffectParameter(1, P::effectMute, 0);
        check(WFSVar::toInt(vts.getEffectParameter(0, P::effectMute)) == 1
           && WFSVar::toInt(vts.getEffectParameter(1, P::effectMute)) == 0,
              "L9: unmuting one member afterwards leaves the other muted - an action, not a coupling");
        vts.setEffectParameter(0, P::effectMute, 0);

        // L9b: the per-output row is not a mute shortcut and is left alone
        {
            auto ret0 = vts.getEffectReturnSection(0);
            const juce::String rowBefore = ret0.getProperty(P::effectMutes).toString();
            vts.setEffectGroupMute(1, true);
            check(ret0.getProperty(P::effectMutes).toString() == rowBefore,
                  "L9: the per-output mute row is spatial routing and the group mute never touches it");
            vts.setEffectGroupMute(1, false);
        }

        // L10: an instanced module - the doubled EQ and dynamics are the whole
        // reason the generic path refuses to resolve them
        {
            auto dynOf = [&](int fx)
            {
                auto s = vts.getEffectModuleSection(fx, P::FxDyn1);
                return static_cast<float>(static_cast<double>(s.getProperty(P::effectDynCompThreshold)));
            };
            auto dyn2Of = [&](int fx)
            {
                auto s = vts.getEffectModuleSection(fx, P::FxDyn2);
                return static_cast<float>(static_cast<double>(s.getProperty(P::effectDynCompThreshold)));
            };
            const float dyn2Before = dyn2Of(1);

            vts.setEffectModuleParameterWithLinkPropagation(0, P::FxDyn1, P::effectDynCompThreshold,
                                                            -33.0f, true);
            check(std::abs(dynOf(0) + 33.0f) < 1e-4f && std::abs(dynOf(1) + 33.0f) < 1e-4f,
                  "L10: a module parameter reaches the same instance on the member");
            check(std::abs(dyn2Of(1) - dyn2Before) < 1e-4f,
                  "L10: and leaves the OTHER instance of that module alone");
        }

        // L11: an EQ band - the same band of the same instance
        {
            auto bandGain = [&](int fx, int inst, int band)
            {
                auto b = vts.getEffectEQBand(fx, inst, band);
                return static_cast<float>(static_cast<double>(b.getProperty(P::effectEQgain)));
            };
            const float otherBandBefore = bandGain(1, 0, 3);
            const float otherInstBefore = bandGain(1, 1, 2);

            vts.setEffectEQBandParameterWithLinkPropagation(0, 0, 2, P::effectEQgain, 5.5f, true);
            check(std::abs(bandGain(0, 0, 2) - 5.5f) < 1e-4f
               && std::abs(bandGain(1, 0, 2) - 5.5f) < 1e-4f,
                  "L11: an EQ band reaches the same band of the same instance on the member");
            check(std::abs(bandGain(1, 0, 3) - otherBandBefore) < 1e-4f
               && std::abs(bandGain(1, 1, 2) - otherInstBefore) < 1e-4f,
                  "L11: and moves no other band and no other instance");
        }

        // L12: a delay tap - the same tap
        {
            auto tapLevel = [&](int fx, int tap)
            {
                auto t = vts.getEffectDelayTap(fx, tap);
                return static_cast<float>(static_cast<double>(t.getProperty(P::effectDelayTapLevel)));
            };
            const float otherTapBefore = tapLevel(1, 4);

            vts.setEffectDelayTapParameterWithLinkPropagation(0, 2, P::effectDelayTapLevel, -9.0f, true);
            check(std::abs(tapLevel(0, 2) + 9.0f) < 1e-4f && std::abs(tapLevel(1, 2) + 9.0f) < 1e-4f,
                  "L12: a delay tap reaches the same tap on the member");
            check(std::abs(tapLevel(1, 4) - otherTapBefore) < 1e-4f,
                  "L12: and moves no other tap");
        }

        // L13: the chain order is a permutation, so it is copied whole
        {
            const juce::String reordered = "eq1,dist,eq2,dyn1,dyn2,mod,phaser,trem,reverb,delay,crush";
            vts.setEffectParameterWithLinkPropagation(0, P::effectChainOrder, reordered, true);
            check(vts.getEffectParameter(1, P::effectChainOrder).toString() == reordered,
                  "L13: the chain order propagates as one string, never as a delta");
            vts.setEffectParameterWithLinkPropagation(0, P::effectChainOrder,
                                                      D::effectChainOrderDefault, true);
        }

        // L14: bypassing propagation writes the origin only
        vts.setEffectParameter(0, P::effectAttenuation, -4.0f);
        vts.setEffectParameter(1, P::effectAttenuation, -4.0f);
        vts.setEffectParameterWithLinkPropagation(0, P::effectAttenuation, -18.0f, false);
        check(std::abs(attenOf(0) + 18.0f) < 1e-4f && std::abs(attenOf(1) + 4.0f) < 1e-4f,
              "L14: a bypassed write reaches the edited channel alone");
        check(linkMode(0) == 1 && linkMode(1) == 1,
              "L14: and leaves both link modes untouched, so the next write propagates again");

        // L15: one undo reverts the origin AND every member it carried
        {
            WFSValueTreeState::ScopedUndoDomain domainScope(vts, UndoDomain::Effects);
            vts.setEffectParameter(0, P::effectAttenuation, -5.0f);
            vts.setEffectParameter(1, P::effectAttenuation, -5.0f);

            vts.beginUndoTransaction("Effects Link Self-Test");
            vts.setEffectParameterWithLinkPropagation(0, P::effectAttenuation, -25.0f, true);
            check(std::abs(attenOf(0) + 25.0f) < 1e-4f && std::abs(attenOf(1) + 25.0f) < 1e-4f,
                  "L15: the gesture moved both channels");

            vts.beginUndoTransaction("Effects Link Self-Test Boundary");
            vts.undo();
            check(std::abs(attenOf(0) + 5.0f) < 1e-4f && std::abs(attenOf(1) + 5.0f) < 1e-4f,
                  "L15: one undo reverts the origin and every member of the gesture");
        }

        // Leave nothing behind
        vts.setNumEffectChannels(0);
        if (! effectLatchBefore)
            vts.getEffectsState().setProperty(P::effectPositionsUserOwned, 0, nullptr);
        recomputeRenderSourceCount();
    }

    // ---- Z: the app's own map carries the effect returns -------------------
    // recomputeRenderSourceCount builds with the live effect count, so a count
    // change through the funnel every structural edit reaches must move
    // numRenderSources, place the returns after the inputs and their slices,
    // install the map in the calculation engine, and hand it the positions of
    // channels that were built as detached subtrees (the listener never saw
    // them). Everything the audio path sizes from numRenderSources follows.
    {
        namespace P = WFSParameterIDs;
        using Kind = spatcore::wfs::SourceKind;

        const int inputsBefore = vts.getNumInputChannels();
        const int stereoBefore = vts.getNumStereoInputChannels();
        const int sourcesBefore = numRenderSources;
        const bool effectLatchBefore = vts.areEffectPositionsUserOwned();

        // Latch first: with the latch off, setNumEffectChannels re-lays the
        // ring on the attached nodes and the engine's listener sees those
        // writes, which would let Z5 pass without the funnel's explicit
        // re-read. With it on, a new channel keeps the placement it was born
        // with on a detached node, and only the re-read can reach the engine.
        vts.markEffectPositionsUserOwned();
        vts.setNumEffectChannels(3);
        reconfig();

        const int firstFx = renderSourceMap.firstEffectSlot;
        check(firstFx == inputsBefore + 5 * stereoBefore, "Z1: the returns follow the inputs and their slices");
        check(renderSourceMap.count == firstFx + 3, "Z2: the map counts the three returns");
        check(numRenderSources == renderSourceMap.count, "Z3: numRenderSources follows the map");
        check(numRenderSources == sourcesBefore + 3, "Z3: ...and grew by exactly the effect count");

        if (calculationEngine != nullptr)
        {
            check(calculationEngine->getSourceKind(firstFx) == Kind::EffectReturn,
                  "Z4: the engine's installed map knows the return");

            const auto rp = calculationEngine->getRenderSourcePosition(firstFx);
            const float ex = static_cast<float>(static_cast<double>(vts.getEffectParameter(0, P::effectPositionX)));
            const float ey = static_cast<float>(static_cast<double>(vts.getEffectParameter(0, P::effectPositionY)));
            const float ez = static_cast<float>(static_cast<double>(vts.getEffectParameter(0, P::effectPositionZ)));
            check(! (std::abs(ex) < 1e-4f && std::abs(ey) < 1e-4f),
                  "Z5: a new channel's ring placement is not the origin");
            check(std::abs(rp.x - ex) < 1e-4f && std::abs(rp.y - ey) < 1e-4f && std::abs(rp.z - ez) < 1e-4f,
                  "Z5: the engine renders the return at that placement");
        }

        vts.setNumEffectChannels(0);
        reconfig();
        check(renderSourceMap.firstEffectSlot == -1, "Z6: no returns, no first slot");
        check(renderSourceMap.count == inputsBefore + 5 * stereoBefore, "Z6: the map is back to the inputs and their slices");
        check(numRenderSources == sourcesBefore, "Z6: numRenderSources is back");

        if (! effectLatchBefore)
            vts.getEffectsState().setProperty(P::effectPositionsUserOwned, 0, nullptr);
    }

    // ---- C: the cook - the tree transcribed into the engine's parameters ---
    // No device and no audio: EffectsHost::cookChannel and buildConfig are
    // pure, and the coalescing is observable through a probe host's own
    // counters, prepared on synthetic rings and never started.
    {
        namespace P = WFSParameterIDs;
        namespace D = WFSParameterDefaults;
        using spatcore::effects::ChainOrder;
        using spatcore::effects::EffectChannelParams;

        const bool effectLatchBefore = vts.areEffectPositionsUserOwned();
        vts.setNumEffectChannels(2);

        ChainOrder order = spatcore::effects::kDefaultOrder;
        bool orderOk = false;

        // C1: a fresh channel cooks to the engine's defaults (which the app's
        // defaults equal, but for the order): every module bypassed, the
        // app's default order, and a spread of fields from the deepest nodes
        {
            const EffectChannelParams fresh {};
            const EffectChannelParams p = EffectsHost::cookChannel(vts, 0, order, orderOk);
            // The original chain, spelled out, so the string cannot drift alone
            const ChainOrder original { 1, 3, 4, 0, 10, 2, 5, 7, 6, 9, 8 };  // eq1 dyn1 dyn2 dist crush eq2 mod trem phaser delay reverb
            check(orderOk, "C1: the default chain order parses");
            check(p.order == original && p.mute == 0 && p.chainBypass == 0, "C1: a fresh channel runs the original chain order, unmuted, chain live");
            check(p.dist.bypass == 1 && p.eq[0].bypass == 1 && p.eq[1].bypass == 1 && p.dyn[0].bypass == 1
                      && p.dyn[1].bypass == 1 && p.mod.bypass == 1 && p.phaser.bypass == 1 && p.trem.bypass == 1
                      && p.reverb.bypass == 1 && p.delay.bypass == 1 && p.crush.bypass == 1,
                  "C1: every module of a fresh channel is bypassed");
            check(p.dist.driveDb == fresh.dist.driveDb && p.eq[1].freqHz[5] == fresh.eq[1].freqHz[5]
                      && p.dyn[1].expScHiCutHz == fresh.dyn[1].expScHiCutHz && p.delay.tapTimeMs[7] == fresh.delay.tapTimeMs[7]
                      && p.crush.ditherDb == fresh.crush.ditherDb && p.reverb.rt60 == fresh.reverb.rt60,
                  "C1: a fresh channel's fields equal the engine's defaults down to the deepest nodes");
        }

        // C2: units are the tree's units - dB stays dB, per cent stays per cent
        {
            vts.setEffectParameter(0, P::effectDistDrive, 3.0f);
            vts.setEffectParameter(0, P::effectTremDepth, 4.0f);
            vts.setEffectParameter(0, P::effectDistMix, 40.0f);
            vts.setEffectParameter(0, P::effectMute, 1);
            vts.setEffectParameter(0, P::effectChainBypass, 1);
            const EffectChannelParams p = EffectsHost::cookChannel(vts, 0, order, orderOk);
            check(p.dist.driveDb == 3.0f, "C2: effectDistDrive lands in dist.driveDb, still in dB");
            check(p.trem.depthDb == 4.0f, "C2: effectTremDepth lands in trem.depthDb, still in dB");
            check(p.dist.mix == 40.0f, "C2: effectDistMix lands in dist.mix, still in per cent");
            check(p.mute == 1 && p.chainBypass == 1, "C2: effectMute and effectChainBypass land in the POD");
        }

        // C3: instances and sub-indices land in their own cell and nowhere else
        {
            vts.getEffectEQBand(0, 1, 2).setProperty(P::effectEQgain, -2.5f, nullptr);
            vts.getEffectEQBand(0, 0, 4).setProperty(P::effectEQshape, 0, nullptr);
            vts.getEffectDynSection(0, 1).setProperty(P::effectDynCompThreshold, -30.0f, nullptr);
            vts.getEffectDelayTap(0, 7).setProperty(P::effectDelayTapTime, 999.0f, nullptr);
            const EffectChannelParams p = EffectsHost::cookChannel(vts, 0, order, orderOk);
            check(p.eq[1].gainDb[2] == -2.5f && p.eq[0].gainDb[2] == 0.0f,
                  "C3: EQ instance 2, band 3 lands in eq[1].gainDb[2] and nowhere else");
            check(p.eq[0].shape[4] == 0 && p.eq[1].shape[4] == 5,
                  "C3: EQ instance 1, band 5 lands in eq[0].shape[4] and nowhere else");
            check(p.dyn[1].compThresholdDb == -30.0f && p.dyn[0].compThresholdDb == -20.0f,
                  "C3: dynamics instance 2 lands in dyn[1] and nowhere else");
            check(p.delay.tapTimeMs[7] == 999.0f && p.delay.tapTimeMs[6] == 2625.0f,
                  "C3: tap 8 lands in delay.tapTimeMs[7] and nowhere else");
        }

        // C4: the chain order is parsed; a bad string keeps the last good one
        {
            vts.setEffectParameter(0, P::effectChainOrder, "crush,delay,reverb,trem,phaser,mod,dyn2,dyn1,eq2,eq1,dist");
            EffectChannelParams p = EffectsHost::cookChannel(vts, 0, order, orderOk);
            const ChainOrder reversed { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
            check(orderOk && p.order == reversed, "C4: a reversed order string cooks to the reversed slot indices");
            vts.getEffectChainSection(0).setProperty(P::effectChainOrder,
                                                     "dist,dist,dist,dist,dist,dist,dist,dist,dist,dist,dist", nullptr);
            p = EffectsHost::cookChannel(vts, 0, order, orderOk);
            check(! orderOk && p.order == reversed, "C4: a bad order string is refused and the last good order stays");
        }

        // C5: the phaser stage snap (the tree bounds a range, the module builds 4/6/8/12)
        {
            vts.setEffectParameter(0, P::effectPhaserStages, 7);
            EffectChannelParams p = EffectsHost::cookChannel(vts, 0, order, orderOk);
            check(p.phaser.stages == 6, "C5: 7 phaser stages snap to 6 (nearest, ties down)");
            vts.setEffectParameter(0, P::effectPhaserStages, 11);
            p = EffectsHost::cookChannel(vts, 0, order, orderOk);
            check(p.phaser.stages == 12, "C5: 11 phaser stages snap to 12");
        }

        // C6: the engine config from the globals and the layout
        {
            auto cfg = EffectsHost::buildConfig(vts, 48000.0, 256, 40, 3, 37);
            check(cfg.matrixStride == D::maxEffectChannels, "C6: the feed stride is the effects budget, not the live count");
            check(cfg.numSources == 40 && cfg.numEffects == 3 && cfg.firstEffectSourceRow == 37,
                  "C6: sources, effects and the first return row are handed over");
            check(cfg.returnCushionBlocks == -1, "C6: cushion 0 in the tree is auto (-1) for the engine");
            vts.setParameter(P::effectsGlobalReturnCushion, 2);
            vts.setParameter(P::effectsGlobalWorkerThreads, 3);
            vts.setParameter(P::effectsGlobalLoopGuardCeiling, 9.0f);
            vts.setParameter(P::effectsGlobalMaxDelaySeconds, 7);
            cfg = EffectsHost::buildConfig(vts, 48000.0, 256, 40, 3, 37);
            check(cfg.returnCushionBlocks == 2 && cfg.workerThreads == 3, "C6: the cushion and the workers follow the globals");
            check(std::abs(cfg.loopGuardCeilingDb - 9.0f) < 1e-6f && std::abs(cfg.maxEffectDelaySeconds - 7.0) < 1e-9,
                  "C6: the loop-guard ceiling and the delay cap follow the globals");
            vts.setParameter(P::effectsGlobalReturnCushion, D::effectsGlobalReturnCushionDefault);
            vts.setParameter(P::effectsGlobalWorkerThreads, D::effectsGlobalWorkerThreadsDefault);
            vts.setParameter(P::effectsGlobalLoopGuardCeiling, D::effectsGlobalLoopGuardCeilingDefault);
            vts.setParameter(P::effectsGlobalMaxDelaySeconds, D::effectsGlobalMaxDelaySecondsDefault);
        }

        // C7: coalescing - many writes on one channel, one publish. A probe
        // host on the same tree, prepared on synthetic rings and never started.
        {
            EffectsHost probe(vts);
            probe.takeDirtyMaskForTest();
            for (int i = 0; i < 20; ++i)
                vts.setEffectParameter(1, P::effectDistDrive, static_cast<float>(i));
            vts.setEffectParameter(0, P::effectTremRate, 2.0f);
            check(probe.takeDirtyMaskForTest() == 0x3u, "C7: twenty writes on channel 2 and one on channel 1 dirty exactly those two bits");
            check(probe.takeDirtyMaskForTest() == 0u, "C7: taking the mask clears it");
            vts.getEffectEQBand(1, 0, 0).setProperty(P::effectEQgain, 1.0f, nullptr);
            check(probe.takeDirtyMaskForTest() == 0x2u, "C7: a band write under channel 2 dirties channel 2 only");

            std::vector<std::unique_ptr<SharedInputRingBuffer>> rings;
            for (int i = 0; i < 4; ++i)
            {
                auto r = std::make_unique<SharedInputRingBuffer>();
                r->setSize(256 * 8);
                rings.push_back(std::move(r));
            }
            check(probe.prepare(48000.0, 256, 4, 2, 2, rings), "C7: a host prepares on synthetic rings");
            probe.takeDirtyMaskForTest();   // prepare marks every channel dirty; start the count clean
            for (int i = 0; i < 20; ++i)
                vts.setEffectParameter(1, P::effectDistDrive, static_cast<float>(i + 1));
            const uint32_t rev0 = probe.getRevision(0);
            const uint32_t rev1 = probe.getRevision(1);
            probe.publishDirty();
            check(probe.getRevision(1) == rev1 + 1, "C7: twenty writes on channel 2, exactly one publish");
            check(probe.getRevision(0) == rev0, "C7: channel 1 untouched, not published");
            probe.publishDirty();
            check(probe.getRevision(1) == rev1 + 1, "C7: nothing dirty, nothing published");
            probe.release();
        }

        // C8: the reverb's model, preset and the six fields its models added
        // land in their own fields - written raw, so no preset expansion moves
        // them, each to a value no default and no neighbour shares
        {
            auto reverb = vts.getEffectModuleSection(0, P::FxReverb);
            reverb.setProperty(P::effectReverbModel, 4, nullptr);
            reverb.setProperty(P::effectReverbType, 11, nullptr);
            reverb.setProperty(P::effectReverbERProfile, 3, nullptr);
            reverb.setProperty(P::effectReverbERLevel, -12.5f, nullptr);
            reverb.setProperty(P::effectReverbModRate, 1.7f, nullptr);
            reverb.setProperty(P::effectReverbModDepth, 63.0f, nullptr);
            reverb.setProperty(P::effectReverbShimmerPitch, 6, nullptr);
            reverb.setProperty(P::effectReverbShimmerAmount, 71.0f, nullptr);
            const EffectChannelParams p = EffectsHost::cookChannel(vts, 0, order, orderOk);
            check(p.reverb.model == 4 && p.reverb.type == 11, "C8: the reverb's model and preset land in reverb.model and reverb.type");
            check(p.reverb.erProfile == 3 && p.reverb.erLevelDb == -12.5f,
                  "C8: the reflection profile and level land in reverb.erProfile and reverb.erLevelDb, still in dB");
            check(p.reverb.modRateHz == 1.7f && p.reverb.modDepth == 63.0f,
                  "C8: the modulation rate and depth land in reverb.modRateHz and reverb.modDepth");
            check(p.reverb.shimmerPitch == 6 && p.reverb.shimmerAmount == 71.0f,
                  "C8: the shimmer interval and amount land in reverb.shimmerPitch and reverb.shimmerAmount");
        }

        vts.setNumEffectChannels(0);
        if (! effectLatchBefore)
            vts.getEffectsState().setProperty(P::effectPositionsUserOwned, 0, nullptr);
    }

    // ---- O: AutomOtion moves an effect return by an offset -----------------
    // The same processor that animates inputs, in offset mode: the movement has
    // to reach what is RENDERED without touching what was AUTHORED, and it has
    // to leave the feed leg alone - an effect that chased its own trigger level
    // as it travelled would ride its own send. No device and no audio: the
    // processor is ticked by hand and the matrix recalculated between ticks.
    {
        namespace P = WFSParameterIDs;
        namespace D = WFSParameterDefaults;
        using Map = spatcore::wfs::RenderSourceMap;

        auto* calc = calculationEngine.get();
        auto* otomo = effectOtomoProcessor.get();

        if (calc == nullptr || otomo == nullptr)
        {
            logLine("SELF-TEST SKIP O: no calculation engine or effect AutomOtion processor");
        }
        else
        {
            const bool effectLatchBefore = vts.areEffectPositionsUserOwned();
            const int inputsBefore = vts.getNumInputChannels();
            vts.setNumEffectChannels(2);

            std::array<uint8_t, Map::kMaxInputChannels> types {};
            const int numTypes = juce::jlimit(0, (int) Map::kMaxInputChannels, inputsBefore);
            for (int i = 0; i < numTypes; ++i)
                if (vts.isInputChannelStereo(i))
                    types[(size_t) i] = Map::Stereo;

            Map map;
            check(Map::build(types.data(), numTypes, 2, map), "O0: a map with two effect returns builds");
            const int firstFx = map.firstEffectSlot;
            const int stride = calc->getNumEffects();
            const int numOutputs = calc->getNumOutputs();
            const int liveOutputs = vts.getNumOutputChannels();

            // Upstage and hearing everything, as in Y: the assertions are about
            // the mechanism, not about this rig's geometry
            vts.setEffectParameter(0, P::effectPositionX, 0.0f);
            vts.setEffectParameter(0, P::effectPositionY, 40.0f);
            vts.setEffectParameter(0, P::effectPositionZ, 3.0f);
            vts.setEffectParameter(0, P::effectAngleOn, 180);
            vts.setEffectParameter(1, P::effectAngleOn, 180);

            // Relative, so the offset the movement publishes IS the dialled
            // destination, and short enough to finish inside the tick loop
            vts.setEffectParameter(0, P::effectOtomoAbsoluteRelative, 1);
            vts.setEffectParameter(0, P::effectOtomoX, 3.0f);
            vts.setEffectParameter(0, P::effectOtomoY, 0.0f);
            vts.setEffectParameter(0, P::effectOtomoZ, 0.0f);
            vts.setEffectParameter(0, P::effectOtomoDuration, 0.2f);
            vts.setEffectParameter(0, P::effectOtomoSpeedProfile, 0);
            vts.setEffectParameter(0, P::effectOtomoCurve, 0);
            vts.setEffectParameter(0, P::effectOtomoCoordinateMode, 0);

            calc->setEffectOtomoOffset(0, 0.0f, 0.0f, 0.0f);
            calc->setRenderSourceMap(map);
            calc->recalculateAllEffectPositions();
            calc->recalculateMatrix(nullptr);

            auto cellFx  = [&](int slot, int fx) { return calc->getInputEffectLevels()[(size_t) (slot * stride + fx)]; };
            auto delayFx = [&](int slot, int fx) { return calc->getInputEffectDelayTimesMs()[(size_t) (slot * stride + fx)]; };
            auto delayOut = [&](int slot, int out) { return calc->getDelayTimesMs()[(size_t) (slot * numOutputs + out)]; };

            // What must not move: the feed leg of every input row, and the
            // authored position
            std::vector<float> feedLevelsBefore, feedDelaysBefore;
            for (int s = 0; s < firstFx; ++s)
            {
                feedLevelsBefore.push_back(cellFx(s, 0));
                feedDelaysBefore.push_back(delayFx(s, 0));
            }
            const float authoredX = static_cast<float>(static_cast<double>(vts.getEffectParameter(0, P::effectPositionX)));
            const float authoredY = static_cast<float>(static_cast<double>(vts.getEffectParameter(0, P::effectPositionY)));
            const float authoredZ = static_cast<float>(static_cast<double>(vts.getEffectParameter(0, P::effectPositionZ)));
            // The whole return row, not one cell of it: a speaker whose cone
            // does not reach 40 m upstage has its cell zeroed and would hold
            // still however far the return travelled.
            auto levelOut = [&](int slot, int out) { return calc->getLevels()[(size_t) (slot * numOutputs + out)]; };
            std::vector<float> returnLevelsBefore;
            for (int o = 0; o < liveOutputs; ++o)
                returnLevelsBefore.push_back(levelOut(firstFx, o));

            // O5: the start is not blocked by guards this family does not have
            check(otomo->startMotion(0), "O5: a movement starts on an effect, which has neither tracking nor sampler");

            float peakOffsetX = 0.0f;
            float minReturnGain = 1.0f;
            float movedPositionX = 0.0f;
            float returnLevelShift = 0.0f;
            bool feedHeld = true;

            // 0.2 s of movement, then the 50 ms fade out, the snap and the
            // 50 ms fade in: 30 ticks of 20 ms covers all of it with room
            for (int tick = 0; tick < 30; ++tick)
            {
                otomo->process(0.02f);
                calc->recalculateMatrix(nullptr);

                const float offX = otomo->getOffsetX(0);
                if (std::abs(offX) > std::abs(peakOffsetX))
                {
                    peakOffsetX = offX;
                    movedPositionX = calc->getRenderSourcePosition(firstFx).x;
                    returnLevelShift = 0.0f;
                    for (int o = 0; o < liveOutputs; ++o)
                        returnLevelShift = juce::jmax(returnLevelShift,
                                                      std::abs(levelOut(firstFx, o) - returnLevelsBefore[(size_t) o]));
                }
                minReturnGain = juce::jmin(minReturnGain, otomo->getReturnGain(0));

                for (int s = 0; s < firstFx; ++s)
                    feedHeld = feedHeld
                            && cellFx(s, 0) == feedLevelsBefore[(size_t) s]
                            && delayFx(s, 0) == feedDelaysBefore[(size_t) s];
            }

            // O1: the movement reaches what is rendered
            check(peakOffsetX > 0.1f, "O1: the movement publishes an offset (peak "
                                      + juce::String(peakOffsetX, 3) + " m)");
            check(std::abs(movedPositionX - (authoredX + peakOffsetX)) < 1.0e-4f,
                  "O1: the return renders at its authored position plus that offset");
            check(liveOutputs == 0 || returnLevelShift > 0.0f,
                  "O1: the moved return re-levels its row of the output matrix (largest shift "
                      + juce::String(returnLevelShift, 5) + ")");

            // O2: and never touches what was authored
            {
                const float nowX = static_cast<float>(static_cast<double>(vts.getEffectParameter(0, P::effectPositionX)));
                const float nowY = static_cast<float>(static_cast<double>(vts.getEffectParameter(0, P::effectPositionY)));
                const float nowZ = static_cast<float>(static_cast<double>(vts.getEffectParameter(0, P::effectPositionZ)));
                check(nowX == authoredX && nowY == authoredY && nowZ == authoredZ,
                      "O2: the authored position is bit-identical after the movement");
            }

            // O3: the feed leg is computed from the base position, so a moving
            // return must not change one cell of it
            check(feedHeld, "O3: not one input's feed cell moved while the return travelled");

            // O4: it comes home by itself, through a fade, with no Stay to read
            check(otomo->getOffsetX(0) == 0.0f && otomo->getOffsetY(0) == 0.0f && otomo->getOffsetZ(0) == 0.0f,
                  "O4: the offset is exactly zero once the movement is over");
            check(! otomo->isActive(0), "O4: the movement ended on its own - this family has no Stay");
            check(minReturnGain < 1.0f, "O4: the snap home is covered by a fade (gain dipped to "
                                        + juce::String(minReturnGain, 3) + ")");
            check(otomo->getReturnGain(0) == 1.0f, "O4: the fade ends back at unity");
            {
                const auto rp = calc->getRenderSourcePosition(firstFx);
                check(std::abs(rp.x - authoredX) < 1.0e-4f && std::abs(rp.y - authoredY) < 1.0e-4f
                          && std::abs(rp.z - authoredZ) < 1.0e-4f,
                      "O4: and the return renders where it was authored again");
            }

            // O6: the delay leg, which minimal latency hides. A return row in
            // mode 1 is measured against its own minimum, and on a rig whose
            // outputs share a listening point a rigid translation of the source
            // shifts every cell by the same amount and cancels exactly - the row
            // is flat at zero and stays there. In mode 0 the geometry is what is
            // published, so the same translation has to re-time the row.
            {
                vts.setEffectParameter(0, P::effectMinimalLatency, 0);
                calc->setEffectOtomoOffset(0, 0.0f, 0.0f, 0.0f);
                calc->recalculateMatrix(nullptr);

                std::vector<float> modeZeroBefore;
                for (int o = 0; o < liveOutputs; ++o)
                    modeZeroBefore.push_back(delayOut(firstFx, o));

                calc->setEffectOtomoOffset(0, 3.0f, 0.0f, 0.0f);
                calc->recalculateMatrix(nullptr);

                float shift = 0.0f;
                for (int o = 0; o < liveOutputs; ++o)
                    shift = juce::jmax(shift, std::abs(delayOut(firstFx, o) - modeZeroBefore[(size_t) o]));

                check(liveOutputs == 0 || shift > 0.0f,
                      "O6: in absolute-latency mode the moved return re-times its row (largest shift "
                          + juce::String(shift, 3) + " ms)");

                vts.setEffectParameter(0, P::effectMinimalLatency, 1);
            }

            // O7: the LFO, the family's second movement, through the same
            // offset path. A sine on X at 2 m over a 1 s period, ticked past
            // the 500 ms fade-in: the return must render at base + LFO with
            // the authored position untouched and the feed leg held; the two
            // offsets must ADD; and once switched off it fades to exactly zero.
            {
                auto* lfo = effectLfoProcessor.get();
                if (lfo == nullptr)
                {
                    logLine("SELF-TEST SKIP O7: no effect LFO processor");
                }
                else
                {
                    calc->setEffectOtomoOffset(0, 0.0f, 0.0f, 0.0f);
                    calc->setEffectLFOOffset(0, 0.0f, 0.0f, 0.0f);
                    calc->recalculateMatrix(nullptr);

                    vts.setEffectParameter(0, P::effectLFOshapeX, 1);       // sine
                    vts.setEffectParameter(0, P::effectLFOamplitudeX, 2.0f);
                    vts.setEffectParameter(0, P::effectLFOrateX, 1.0f);
                    vts.setEffectParameter(0, P::effectLFOperiod, 1.0f);
                    vts.setEffectParameter(0, P::effectLFOphase, 0);
                    vts.setEffectParameter(0, P::effectLFOphaseX, 0);
                    vts.setEffectParameter(0, P::effectLFOactive, 1);

                    float lfoPeak = 0.0f, lfoMovedX = 0.0f;
                    bool lfoFeedHeld = true;
                    for (int tick = 0; tick < 40; ++tick)          // 0.8 s: past both fades
                    {
                        lfo->process(0.02f);
                        calc->setEffectLFOOffset(0, lfo->getOffsetX(0), lfo->getOffsetY(0), lfo->getOffsetZ(0));
                        calc->recalculateMatrix(nullptr);

                        const float offX = lfo->getOffsetX(0);
                        if (std::abs(offX) > std::abs(lfoPeak))
                        {
                            lfoPeak = offX;
                            lfoMovedX = calc->getRenderSourcePosition(firstFx).x;
                        }

                        for (int s = 0; s < firstFx; ++s)
                            lfoFeedHeld = lfoFeedHeld
                                       && cellFx(s, 0) == feedLevelsBefore[(size_t) s]
                                       && delayFx(s, 0) == feedDelaysBefore[(size_t) s];
                    }

                    check(std::abs(lfoPeak) > 0.5f, "O7: the LFO publishes an offset (peak "
                                                    + juce::String(lfoPeak, 3) + " m)");
                    check(std::abs(lfoMovedX - (authoredX + lfoPeak)) < 1.0e-4f,
                          "O7: the return renders at its authored position plus the LFO offset");
                    check(lfoFeedHeld, "O7: not one input's feed cell moved while the LFO ran");
                    {
                        const float nowX = static_cast<float>(static_cast<double>(vts.getEffectParameter(0, P::effectPositionX)));
                        const float nowY = static_cast<float>(static_cast<double>(vts.getEffectParameter(0, P::effectPositionY)));
                        const float nowZ = static_cast<float>(static_cast<double>(vts.getEffectParameter(0, P::effectPositionZ)));
                        check(nowX == authoredX && nowY == authoredY && nowZ == authoredZ,
                              "O7: the authored position is bit-identical after the LFO ran");
                    }

                    // The two movements add: an AutomOtion offset under the running LFO
                    calc->setEffectOtomoOffset(0, 3.0f, 0.0f, 0.0f);
                    calc->recalculateMatrix(nullptr);
                    {
                        const float lfoNow = lfo->getOffsetX(0);
                        const auto rp = calc->getRenderSourcePosition(firstFx);
                        check(std::abs(rp.x - (authoredX + 3.0f + lfoNow)) < 1.0e-4f,
                              "O7: the AutomOtion and LFO offsets add on the rendered return");
                        const auto mv = calc->getEffectMovementOffset(0);
                        check(std::abs(mv.x - (3.0f + lfoNow)) < 1.0e-6f,
                              "O7: the Map reads the sum of both movements");
                    }
                    calc->setEffectOtomoOffset(0, 0.0f, 0.0f, 0.0f);

                    // Off: the 500 ms fade, then exactly zero
                    vts.setEffectParameter(0, P::effectLFOactive, 0);
                    for (int tick = 0; tick < 40; ++tick)
                        lfo->process(0.02f);
                    check(lfo->getOffsetX(0) == 0.0f && lfo->getOffsetY(0) == 0.0f && lfo->getOffsetZ(0) == 0.0f,
                          "O7: the offset is exactly zero once the LFO is off and faded");

                    calc->setEffectLFOOffset(0, 0.0f, 0.0f, 0.0f);
                    vts.setEffectParameter(0, P::effectLFOshapeX, 0);
                    calc->recalculateMatrix(nullptr);
                }
            }

            // Leave nothing behind
            otomo->stopMotion(0);
            calc->setEffectOtomoOffset(0, 0.0f, 0.0f, 0.0f);
            vts.setNumEffectChannels(0);
            if (! effectLatchBefore)
                vts.getEffectsState().setProperty(P::effectPositionsUserOwned, 0, nullptr);
            recomputeRenderSourceCount();
            calc->recalculateMatrix(nullptr);
        }
    }

    // ---- G: the Chain sub-tab's reorder helper ----------------------------
    // Dragging a tile rewrites effectChainOrder through a pure function; if
    // it ever produced anything but a permutation, parseChainOrder would
    // refuse the write and the strip would silently stop reordering.
    {
        namespace fx = spatcore::effects;
        const juce::String base = "dist,eq1,eq2,dyn1,dyn2,mod,phaser,trem,reverb,delay,crush";

        check(EffectsChainPanel::movedOrder(base, 0, 5) == "eq1,eq2,dyn1,dyn2,mod,dist,phaser,trem,reverb,delay,crush",
              "G1: moving the first tile to the sixth position shifts the five between it left");
        check(EffectsChainPanel::movedOrder(base, 10, 0) == "crush,dist,eq1,eq2,dyn1,dyn2,mod,phaser,trem,reverb,delay",
              "G1: moving the last tile to the front shifts everything right");
        check(EffectsChainPanel::movedOrder(base, 4, 4) == base, "G1: a drop on its own position changes nothing");
        check(EffectsChainPanel::movedOrder("not,an,order", 0, 1) == "not,an,order",
              "G1: an unparsable order comes back untouched");

        int perms = 0;
        for (int from = 0; from < fx::kNumModuleSlots; ++from)
            for (int to = 0; to < fx::kNumModuleSlots; ++to)
            {
                fx::ChainOrder o {};
                if (fx::parseChainOrder(EffectsChainPanel::movedOrder(base, from, to).toRawUTF8(), o) && fx::isValidChainOrder(o))
                    ++perms;
            }
        check(perms == fx::kNumModuleSlots * fx::kNumModuleSlots,
              "G2: every (from, to) move yields a valid permutation (" + juce::String(perms) + " of 121)");
    }

    // ---- K: the keyboard acts on the tab that is showing --------------------
    // Injected keys never reach the app from a test shell, so this calls
    // keyPressed directly. The tab numbers are the point: the dispatcher kept
    // its literals when the Effects tab went in at 4, which put F1 on the
    // Effects tab into an INPUT cluster, made Space on Inputs step clusters,
    // and made detaching the Map delete the Clusters tab.
    if (effectsTab != nullptr && inputsTab != nullptr && clustersTab != nullptr && mapTab != nullptr)
    {
        using KP = juce::KeyPress;
        const int effectsBefore = vts.getNumEffectChannels();
        const int tabBefore = tabbedComponent.getCurrentTabIndex();
        vts.setNumEffectChannels(3);
        unfocusAllComponents();

        auto press = [this](int code, bool shift = false)
        {
            return keyPressed(KP(code, shift ? juce::ModifierKeys::shiftModifier : 0, 0));
        };
        auto shownGroup = [&] { return vts.getEffectLinkGroup(effectsTab->getCurrentChannel() - 1); };

        const int inputSlot = vts.getSlotForChannelNumber(inputsTab->getCurrentChannel());
        const int inputClusterBefore = vts.getIntParameter(WFSParameterIDs::inputCluster, inputSlot);

        tabbedComponent.setCurrentTabIndex(TabIndex::Effects);
        effectsTab->selectChannel(2);
        const int group1Before = vts.getEffectLinkGroup(0);
        const int group3Before = vts.getEffectLinkGroup(2);
        press(KP::F3Key);
        check(shownGroup() == 3, "KB1: F3 on the Effects tab puts the shown effect in link group 3");
        check(vts.getIntParameter(WFSParameterIDs::inputCluster, inputSlot) == inputClusterBefore,
              "KB1: and leaves the Inputs tab's input in its cluster");
        check(vts.getEffectLinkGroup(0) == group1Before && vts.getEffectLinkGroup(2) == group3Before,
              "KB1: and no other effect's group changes");
        press(KP::F9Key);
        check(shownGroup() == 3, "KB2: F9 does nothing on the Effects tab (8 link groups)");
        press(KP::F11Key);
        check(shownGroup() == 0, "KB2: F11 takes the effect out of its group");

        effectsTab->selectChannel(3);
        press(KP::spaceKey);
        check(effectsTab->getCurrentChannel() == 1, "KB3: Space on the last effect wraps to the first");
        press(KP::spaceKey, true);
        check(effectsTab->getCurrentChannel() == 3, "KB3: Shift+Space on the first wraps to the last");
        press(KP::spaceKey, true);
        check(effectsTab->getCurrentChannel() == 2, "KB3: Shift+Space steps back one");

        tabbedComponent.setCurrentTabIndex(TabIndex::Inputs);
        const int inputBefore = inputsTab->getCurrentChannel();
        const int clusterSelBefore = clustersTab->getSelectedCluster();
        press(KP::spaceKey);
        check((vts.getNumInputChannels() < 2 || inputsTab->getCurrentChannel() != inputBefore)
                  && clustersTab->getSelectedCluster() == clusterSelBefore,
              "KB4: Space on the Inputs tab steps the input, not the clusters");
        press(KP::spaceKey, true);
        check(inputsTab->getCurrentChannel() == inputBefore, "KB4: Shift+Space steps it back");

        tabbedComponent.setCurrentTabIndex(TabIndex::Clusters);
        press(KP::F2Key);
        check(clustersTab->getSelectedCluster() == 2, "KB5: F2 on the Clusters tab selects cluster 2");
        check(vts.getIntParameter(WFSParameterIDs::inputCluster, inputSlot) == inputClusterBefore,
              "KB5: and assigns no input");
        clustersTab->setSelectedCluster(clusterSelBefore);

        detachMapTab();
        check(tabbedComponent.getNumTabs() == TabIndex::Count
                  && tabbedComponent.getTabContentComponent(TabIndex::Clusters) == clustersTab
                  && tabbedComponent.getTabContentComponent(TabIndex::Map) != mapTab.get(),
              "KB6: detaching the Map replaces the Map tab and keeps the Clusters tab");
        attachMapTab();
        check(tabbedComponent.getNumTabs() == TabIndex::Count
                  && tabbedComponent.getTabContentComponent(TabIndex::Map) == mapTab.get(),
              "KB6: re-attaching puts the Map back in its place");

        tabbedComponent.setCurrentTabIndex(TabIndex::Effects);
        effectsTab->selectChannel(3);
        const auto nameBefore = vts.getEffectParameter(2, WFSParameterIDs::effectName);
        effectsTab->getNameEditorForTest().setText("KB7 name", false);
        effectsTab->pressNameKeyForTest(KP(KP::tabKey, 0, 0));
        check(vts.getEffectParameter(2, WFSParameterIDs::effectName).toString() == "KB7 name",
              "KB7: Tab in the name field keeps the typed name");
        check(effectsTab->getCurrentChannel() == 1, "KB7: and moves on to the next effect, wrapping");
        effectsTab->pressNameKeyForTest(KP(KP::tabKey, juce::ModifierKeys::shiftModifier, 0));
        check(effectsTab->getCurrentChannel() == 3, "KB7: Shift+Tab moves back, wrapping");
        vts.setEffectParameter(2, WFSParameterIDs::effectName, nameBefore);
        unfocusAllComponents();

        vts.setNumEffectChannels(effectsBefore);
        tabbedComponent.setCurrentTabIndex(tabBefore);
    }
    else
    {
        check(false, "KB: the tabs the keyboard drives exist");
    }

    // ---- FE: typed values and Tab sections on the Effects tab ----------------
    // Every value label of the five panels takes a typed number, as the Inputs
    // and Reverb tabs' do. Keys and focus cannot be injected from a test shell,
    // so the label editors are opened, filled and closed directly - the same
    // Label path a click, a typed value and Enter / Esc take.
    if (effectsTab != nullptr)
    {
        namespace P = WFSParameterIDs;
        using Fields = EffectsFieldEditing;
        const int effectsBefore = vts.getNumEffectChannels();
        const int tabBefore = tabbedComponent.getCurrentTabIndex();
        const auto ceilingBefore = parameters.getConfigParam("effectsGlobalLoopGuardCeiling");
        vts.setNumEffectChannels(2);
        tabbedComponent.setCurrentTabIndex(TabIndex::Effects);
        effectsTab->selectChannel(1);

        struct Counter : juce::ValueTree::Listener
        {
            int count = 0;
            void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override { ++count; }
        } counter;
        auto effectsState = vts.getEffectsState();
        auto configState = vts.getConfigState();
        effectsState.addListener(&counter);
        configState.addListener(&counter);

        auto type = [](juce::Label& label, const juce::String& text, bool keep)
        {
            label.showEditor();
            if (auto* ed = label.getCurrentTextEditor())
                ed->setText(text, false);
            label.hideEditor(! keep);
        };
        auto isNear = [](const juce::var& v, float expected, float tol)
        {
            return ! v.isVoid() && std::abs(static_cast<float>(static_cast<double>(v)) - expected) <= tol;
        };

        auto& channel = effectsTab->getChannelPanelForTest();
        auto& movements = effectsTab->getMovementsPanelForTest();
        auto& settings = effectsTab->getSettingsPanelForTest();

        // FE1: every field of every panel, typed at both ends of any range
        auto sweep = [&](Fields& fields, const juce::String& panel)
        {
            juce::StringArray failed;
            int total = 0;
            for (auto* label : fields.getLabelsForTest())
            {
                ++total;
                bool ok = false;
                for (auto* text : { "-100000000", "100000000" })
                {
                    counter.count = 0;
                    type(*label, text, true);
                    if (counter.count > 0 && label->getText() != text)
                    {
                        ok = true;
                        break;
                    }
                }
                if (! ok)
                    failed.add(label->getText());
            }
            check(total > 0 && failed.isEmpty(),
                  "FE1: every value on " + panel + " takes a typed number, writes it and shows it formatted ("
                      + juce::String(total - failed.size()) + "/" + juce::String(total)
                      + (failed.isEmpty() ? juce::String(")") : "; failed: " + failed.joinIntoString(" | ") + ")"));
        };
        sweep(channel.getFieldsForTest(), "Channel Parameters");
        for (int s = 0; s < spatcore::effects::kNumModuleSlots; ++s)
            sweep(effectsTab->getModulePanel(s).getFieldsForTest(), "Chain module " + juce::String(s));
        sweep(movements.getFieldsForTest(), "Movements");
        sweep(settings.getFieldsForTest(), "Settings");

        // FE2: the lenient reading
        auto is = [](std::optional<float> v, float expected) { return v.has_value() && std::abs(*v - expected) < 1.0e-3f; };
        check(is(TypedValue::number("1.20 kHz"), 1200.0f) && is(TypedValue::number("Latency 12,5 ms"), 12.5f)
                  && is(TypedValue::number("-6.0 dB/m"), -6.0f) && is(TypedValue::number("4.0:1"), 4.0f)
                  && is(TypedValue::number("-.5"), -0.5f) && ! TypedValue::number("abc").has_value()
                  && is(TypedValue::number(juce::String::fromUTF8("\xe2\x88\x92" "6 dB")), -6.0f),
              "FE2: numbers are read past units and prefixes, a comma is a decimal point, k means thousands, a Unicode minus is a minus");
        check(is(TypedValue::duration("1m 30s"), 90.0f) && is(TypedValue::duration("5.00 s"), 5.0f)
                  && is(TypedValue::duration("1h"), 3600.0f) && is(TypedValue::duration("500 ms"), 0.5f)
                  && is(TypedValue::duration("2 min"), 120.0f) && ! TypedValue::duration("s").has_value(),
              "FE2: durations are read as the panel shows them (1m 30s, 5.00 s, 1h)");
        check(is(TypedValue::duration("2min"), 120.0f) && is(TypedValue::duration("2mn30"), 150.0f)
                  && is(TypedValue::duration("2m30"), 150.0f) && is(TypedValue::duration("1.5 min"), 90.0f)
                  && is(TypedValue::duration("1h30"), 5400.0f) && is(TypedValue::duration("1 h 30 min"), 5400.0f)
                  && is(TypedValue::duration("2 minutes 5 seconds"), 125.0f) && is(TypedValue::duration("2 Std"), 7200.0f)
                  && is(TypedValue::duration(juce::String::fromUTF8("2" "\xe5\x88\x86" "30" "\xe7\xa7\x92")), 150.0f),
              "FE2: minutes in every usual spelling; a bare number after a unit takes the next unit down (1h30, 2m30)");
        check(is(TypedValue::duration("1:30"), 90.0f) && is(TypedValue::duration("1:02:03"), 3723.0f)
                  && is(TypedValue::duration("0:45.5"), 45.5f) && ! TypedValue::duration(":").has_value(),
              "FE2: the clock form reads m:ss and h:mm:ss");
        check(is(TypedValue::ratio("4.0:1"), 4.0f) && is(TypedValue::ratio("1:2.0"), 2.0f)
                  && is(TypedValue::ratio("3:2"), 1.5f) && is(TypedValue::ratio("3"), 3.0f),
              "FE2: a ratio is read either way round (4.0:1 is 4, 1:2.0 is 2)");

        auto* atten = channel.getFieldsForTest().findLabelForTest("Effect Attenuation");
        auto* latency = channel.getFieldsForTest().findLabelForTest("Effect Delay/Latency");
        if (atten != nullptr && latency != nullptr)
        {
            // FE3: Esc and a text with no number change nothing
            const auto shown = atten->getText();
            counter.count = 0;
            type(*atten, "-30", false);
            check(counter.count == 0 && atten->getText() == shown, "FE3: Esc leaves the value and its text as they were");
            type(*atten, "loud", true);
            check(counter.count == 0 && atten->getText() == shown, "FE3: a text with no number in it changes nothing");

            // FE4: the latency field opens on the signed number and takes one
            type(*latency, "-12", true);
            check(isNear(vts.getEffectParameter(0, P::effectDelayLatency), -12.0f, 0.05f),
                  "FE4: a negative delay typed is stored as that latency (-12 ms)");
            latency->showEditor();
            const bool signedText = latency->getCurrentTextEditor() != nullptr
                                    && latency->getCurrentTextEditor()->getText() == "-12.0";
            latency->hideEditor(true);
            check(signedText, "FE4: and the field opens on the signed number, not the worded label");

            // FE7: Tab keeps to the section, wraps, and skips a hidden member
            auto& cf = channel.getFieldsForTest();
            auto* angleOn = cf.findLabelForTest("Effect Angle On");
            auto* angleOff = cf.findLabelForTest("Effect Angle Off");
            atten->showEditor();
            cf.pressTabForTest(*atten, true);
            const bool wrapped = atten->getCurrentTextEditor() == nullptr && latency->getCurrentTextEditor() != nullptr;
            latency->hideEditor(true);
            check(wrapped, "FE7: Shift+Tab on the first field of a section wraps to its last");

            angleOn->showEditor();
            cf.pressTabForTest(*angleOn, false);
            const bool next = angleOn->getCurrentTextEditor() == nullptr && angleOff->getCurrentTextEditor() != nullptr;
            angleOff->hideEditor(true);
            check(next, "FE7: Tab moves to the next field of the same section");

            vts.setEffectParameter(0, P::effectAttenuationLaw, 0);     // log law: the ratio dial is hidden
            effectsTab->selectChannel(1);
            auto* distAtten = cf.findLabelForTest("Effect Distance Attenuation");
            auto* common = cf.findLabelForTest("Effect Common Attenuation");
            auto* shelf = cf.findLabelForTest("Effect HF Shelf");
            distAtten->showEditor();
            cf.pressTabForTest(*distAtten, false);
            const bool skipped = common->getCurrentTextEditor() != nullptr;
            common->hideEditor(true);
            shelf->showEditor();
            cf.pressTabForTest(*shelf, false);
            const bool stayed = distAtten->getCurrentTextEditor() != nullptr;
            distAtten->hideEditor(true);
            check(skipped && stayed, "FE7: Tab skips the hidden dial and wraps inside the column, never into the array trims");

            // A text box takes Tab through JUCE's own traverser, which asks
            // the box's parents: the panel's sections must answer
            auto& lastPos = channel.getPositionEditorForTest(2);
            auto& lastOff = channel.getOffsetEditorForTest(2);
            auto boxTraverser = lastPos.createKeyboardFocusTraverser();
            check(boxTraverser != nullptr
                      && boxTraverser->getNextComponent(&lastPos) == &channel.getOffsetEditorForTest(0)
                      && boxTraverser->getNextComponent(&lastOff) == &channel.getPositionEditorForTest(0)
                      && boxTraverser->getPreviousComponent(&channel.getPositionEditorForTest(0)) == &lastOff,
                  "FE7: Tab from a position box walks position then offset and wraps, as on the reverb tab");
        }
        else
        {
            check(false, "FE3: the channel panel's attenuation and latency fields exist");
        }

        // FE5: a typed EQ frequency is stored exactly (the slider's own law truncates)
        if (auto* freq = effectsTab->getModulePanel(1).getFieldsForTest().findLabelForTest("Effect EQ Band 1 Freq"))
        {
            type(*freq, "1000", true);
            const int f1 = static_cast<int>(vts.getEffectEQBand(0, 0, 0).getProperty(P::effectEQfreq));
            type(*freq, "1.5 kHz", true);
            const int f2 = static_cast<int>(vts.getEffectEQBand(0, 0, 0).getProperty(P::effectEQfreq));
            check(f1 == 1000 && f2 == 1500, "FE5: a typed EQ frequency is stored as typed (1000, 1.5 kHz = 1500; got "
                                                + juce::String(f1) + ", " + juce::String(f2) + ")");
        }
        else
        {
            check(false, "FE5: the EQ 1 band 1 frequency field exists");
        }

        // FE6: the AutomOtion duration reads minutes and seconds
        if (auto* duration = movements.getFieldsForTest().findLabelForTest("Effect AutomOtion Duration"))
        {
            type(*duration, "1m 30s", true);
            check(isNear(vts.getEffectParameter(0, P::effectOtomoDuration), 90.0f, 0.2f) && duration->getText() == "1m 30s",
                  "FE6: a duration typed as 1m 30s is stored as 90 s and shown as typed");
        }
        else
        {
            check(false, "FE6: the AutomOtion duration field exists");
        }

        // FE8: the position boxes follow the coordinate mode both ways, and a
        // box the operator did not type into is never written
        {
            vts.setEffectParameter(0, P::effectCoordinateMode, 1);
            vts.setEffectParameter(0, P::effectPositionX, 3.0f);
            vts.setEffectParameter(0, P::effectPositionY, 4.0f);
            effectsTab->selectChannel(1);
            auto& radius = channel.getPositionEditorForTest(0);
            check(radius.getText() == "5.00", "FE8: in r theta Z the first box shows the radius (5.00 for 3, 4; got "
                                                  + radius.getText() + ")");

            counter.count = 0;
            channel.closeBoxForTest(radius);
            check(counter.count == 0, "FE8: closing a box nobody typed into writes nothing");

            radius.setText("10", false);
            channel.getFieldsForTest().markEditedForTest(radius);
            channel.closeBoxForTest(radius);
            check(isNear(vts.getEffectParameter(0, P::effectPositionX), 6.0f, 1.0e-3f)
                      && isNear(vts.getEffectParameter(0, P::effectPositionY), 8.0f, 1.0e-3f),
                  "FE8: a typed radius moves the effect along its bearing (10 -> 6, 8)");

            radius.setText("99", false);
            channel.getFieldsForTest().markEditedForTest(radius);
            channel.escapeBoxForTest(radius);
            counter.count = 0;
            channel.closeBoxForTest(radius);
            check(radius.getText() == "10.00" && counter.count == 0, "FE8: Esc puts the stored value back and writes nothing");

            vts.setEffectParameter(0, P::effectCoordinateMode, 0);
        }

        // FE9: a click on an empty patch of a panel closes an open field only
        // if the panel takes the focus (JUCE stops at the field's parents)
        check(channel.getWantsKeyboardFocus() && effectsTab->getChainPanelForTest().getWantsKeyboardFocus()
                  && effectsTab->getModulePanel(0).getWantsKeyboardFocus() && movements.getWantsKeyboardFocus()
                  && settings.getWantsKeyboardFocus(),
              "FE9: every panel that holds fields takes the focus, so a click on it closes an open field");

        effectsState.removeListener(&counter);
        configState.removeListener(&counter);
        parameters.setConfigParam("effectsGlobalLoopGuardCeiling", ceilingBefore);
        vts.setNumEffectChannels(effectsBefore);
        tabbedComponent.setCurrentTabIndex(tabBefore);
    }

    // ---- TV: typed values are read as the other tabs' labels show them -------
    // Those labels kept only the digits of what was typed: "2m 45s" became
    // 245 s, "1:3.0" an expander ratio of 13, "3.0 kHz" 3 Hz, a positive
    // number in the worded latency field a delay, and a word 0 - full level
    // on an attenuation. TypedValue now reads them, the latency field opens on
    // the signed number, and a text with no number puts the label back.
    if (inputsTab != nullptr && reverbTab != nullptr)
    {
        namespace P = WFSParameterIDs;
        const int tabBefore = tabbedComponent.getCurrentTabIndex();
        auto type = [](juce::Label& label, const juce::String& text)
        {
            label.showEditor();
            if (auto* ed = label.getCurrentTextEditor())
                ed->setText(text, false);
            label.hideEditor(false);
        };

        tabbedComponent.setCurrentTabIndex(TabIndex::Inputs);
        const int slot = vts.getSlotForChannelNumber(inputsTab->getCurrentChannel());

        auto& duration = inputsTab->getOtomoDurationLabelForTest();
        const float durationBefore = vts.getFloatParameter(P::inputOtomoDuration, slot);
        type(duration, "2m 45s");
        const float d1 = vts.getFloatParameter(P::inputOtomoDuration, slot);
        type(duration, "1:30");
        const float d2 = vts.getFloatParameter(P::inputOtomoDuration, slot);
        check(std::abs(d1 - 165.0f) < 0.5f && std::abs(d2 - 90.0f) < 0.5f && duration.getText() == "1m 30s",
              "TV1: the Inputs AutomOtion duration reads 2m 45s as 165 s and 1:30 as 90 s (got "
                  + juce::String(d1, 1) + ", " + juce::String(d2, 1) + ")");
        vts.setInputParameter(slot, P::inputOtomoDuration, durationBefore);

        auto& latency = inputsTab->getDelayLatencyLabelForTest();
        const float latencyBefore = vts.getFloatParameter(P::inputDelayLatency, slot);
        type(latency, "-12");
        latency.showEditor();
        const bool signedText = latency.getCurrentTextEditor() != nullptr
                                && latency.getCurrentTextEditor()->getText() == "-12.0";
        latency.hideEditor(true);
        check(std::abs(vts.getFloatParameter(P::inputDelayLatency, slot) + 12.0f) < 0.05f && signedText,
              "TV2: the Inputs Delay/Latency field takes -12 as a 12 ms latency and opens on the signed number");
        vts.setInputParameter(slot, P::inputDelayLatency, latencyBefore);

        auto& atten = inputsTab->getAttenuationLabelForTest();
        const auto attenText = atten.getText();
        const float attenBefore = vts.getFloatParameter(P::inputAttenuation, slot);
        type(atten, "loud");
        check(atten.getText() == attenText
                  && juce::approximatelyEqual(vts.getFloatParameter(P::inputAttenuation, slot), attenBefore),
              "TV3: a word typed into an attenuation changes nothing (it used to set 0 dB)");

        const int reverbsBefore = vts.getNumReverbChannels();
        if (reverbsBefore == 0)
            vts.setNumReverbChannels(1);    // restored below
        if (vts.getNumReverbChannels() > 0)
        {
            tabbedComponent.setCurrentTabIndex(TabIndex::Reverb);
            auto& ratio = reverbTab->getPostExpRatioLabelForTest();
            auto& freq = reverbTab->getEqFreqLabelForTest(0);
            const auto ratioText = ratio.getText();
            const auto freqText = freq.getText();
            type(ratio, "1:3.0");
            type(freq, "3.0 kHz");
            check(ratio.getText() == "1:3.0" && freq.getText() == "3.0 kHz",
                  "TV4: the Reverb expander ratio 1:3.0 is 3 (was 13) and an EQ frequency of 3.0 kHz is 3000 Hz (was 20); shown "
                      + ratio.getText() + ", " + freq.getText());
            type(ratio, ratioText);             // the readers read the old texts back right
            type(freq, freqText);
        }
        else
        {
            check(false, "TV4: a reverb channel to type into");
        }
        if (reverbsBefore == 0)
            vts.setNumReverbChannels(0);

        tabbedComponent.setCurrentTabIndex(tabBefore);
    }
    else
    {
        check(false, "TV: the Inputs and Reverb tabs exist");
    }

    // ---- P: the engine's meters, and the freshness that keeps them honest --
    // A probe host prepared on synthetic rings and driven one batch at a time,
    // so the whole tap - the engine's per-channel peaks, the max-hold, the
    // ballistics and the freshness rule - is exercised with no device and no
    // realtime thread. What must not happen is a meter that holds its last
    // reading after the driver stops: that is the failure that makes a dead
    // engine look like a live one.
    {
        namespace P = WFSParameterIDs;
        using Map = spatcore::wfs::RenderSourceMap;
        using spatcore::rt::SharedInputRingBuffer;

        auto* calc = calculationEngine.get();
        auto* meters = levelMeteringManager.get();

        if (calc == nullptr || meters == nullptr)
        {
            logLine("SELF-TEST SKIP P: no calculation engine or metering manager");
        }
        else
        {
            const bool effectLatchBefore = vts.areEffectPositionsUserOwned();
            const int inputsBefore = vts.getNumInputChannels();
            vts.setNumEffectChannels(2);

            // P1: nothing wired at all
            meters->setEffectsSource(nullptr, 0.0f);
            meters->pollEffectLevels(0.005f);
            check(meters->getEffectLevel(0).peakDb <= -200.0f
                      && meters->getEffectReturnLevel(0).peakDb <= -200.0f,
                  "P1: with no engine wired both taps read silence");

            // A map and an open send, so the published feed matrix actually
            // routes source 0 into effect 0
            std::array<uint8_t, Map::kMaxInputChannels> types {};
            const int numTypes = juce::jlimit(0, (int) Map::kMaxInputChannels, inputsBefore);
            for (int i = 0; i < numTypes; ++i)
                if (vts.isInputChannelStereo(i))
                    types[(size_t) i] = Map::Stereo;

            Map map;
            Map::build(types.data(), numTypes, 2, map);
            vts.setEffectParameter(0, P::effectAngleOn, 180);
            vts.setEffectParameter(1, P::effectAngleOn, 180);
            vts.setEffectSendOnFromInput(0, vts.getInputChannelNumber(0), true);
            vts.setEffectSendLevelFromInput(0, vts.getInputChannelNumber(0), 0.0f);
            calc->setRenderSourceMap(map);
            calc->recalculateAllEffectPositions();
            calc->recalculateMatrix(nullptr);

            // Every row of the map, because the returns sit ABOVE the inputs
            // and their slices: a probe with fewer sources than
            // firstEffectSlot + numEffects is refused, and rightly so.
            const int probeSources = map.count;
            const int probeBlock = 256;

            EffectsHost probe(vts);
            std::vector<std::unique_ptr<SharedInputRingBuffer>> rings;
            for (int i = 0; i < probeSources; ++i)
            {
                auto r = std::make_unique<SharedInputRingBuffer>();
                r->setSize(probeBlock * 8);
                rings.push_back(std::move(r));
            }

            if (! probe.prepare(48000.0, probeBlock, probeSources, map.firstEffectSlot, 2, rings))
            {
                logLine("SELF-TEST SKIP P: the probe host would not prepare");
            }
            else
            {
                probe.setFeedMatrices(*calc, probeSources);
                probe.publishDirty();

                // P2: wired, but the driver has not run. The freshness rule
                // is gated by P4 below, where there IS something to hold on to;
                // this one only says that wiring a source is not itself a
                // reading.
                meters->setEffectsSource(probe.getCore(), 5.0f);
                meters->pollEffectLevels(0.005f);
                check(meters->getEffectLevel(0).peakDb <= -200.0f,
                      "P2: wiring an engine that has not run is not itself a reading");

                // P3: drive it. Half scale into source 0, silence everywhere
                // else. A second of audio, because the send carries the
                // geometric delay from the source to the effect - a channel
                // laid out across the room is a hundred milliseconds of feed
                // history before one sample reaches the chain - and the return
                // adds the chain's latency and the cushion on top. The meter is
                // polled inside the loop, where its max-hold is what catches
                // the arrival whenever it happens.
                auto* core = probe.getMutableCoreForTest();
                std::vector<float> tone(static_cast<size_t> (probeBlock), 0.5f);
                std::vector<float> quiet(static_cast<size_t> (probeBlock), 0.0f);

                for (int b = 0; b < 200; ++b)
                {
                    for (int s = 0; s < probeSources; ++s)
                        rings[(size_t) s]->write(s == 0 ? tone.data() : quiet.data(), probeBlock);
                    core->processBatch();
                    meters->pollEffectLevels(0.005f);
                }

                const float feedDb = meters->getEffectLevel(0).peakDb;
                const float retDb = meters->getEffectReturnLevel(0).peakDb;
                check(feedDb > -60.0f, "P3: the send reaches the engine and the feed tap reads it ("
                                           + juce::String(feedDb, 1) + " dB)");
                check(retDb > -60.0f, "P3: the chain hands it back and the return tap reads it ("
                                          + juce::String(retDb, 1) + " dB)");
                check(meters->getEffectLevel(1).peakDb <= -200.0f,
                      "P3: the effect nothing is sent to stays silent - the taps are per channel");
                check(meters->getEffectsStats().live && meters->getEffectsStats().batchCount > 0,
                      "P3: the driver reports its batches");

                // P4: the driver stops. Within the staleness window the meters
                // have to fall to silence on their own.
                juce::Thread::sleep(300);
                meters->pollEffectLevels(0.005f);
                check(meters->getEffectLevel(0).peakDb <= -200.0f
                          && meters->getEffectReturnLevel(0).peakDb <= -200.0f,
                      "P4: a driver that stopped batching reads as silence, not as its last value");
                check(! meters->getEffectsStats().live, "P4: and the duty stops being reported");

                probe.release();
            }

            // P5: unwiring clears everything
            meters->setEffectsSource(nullptr, 0.0f);
            check(meters->getEffectLevel(0).peakDb <= -200.0f
                      && meters->getEffectsStats().batchCount == 0,
                  "P5: unwiring the source clears the taps and the duty");

            vts.setEffectSendOnFromInput(0, vts.getInputChannelNumber(0), false);
            vts.setNumEffectChannels(0);
            if (! effectLatchBefore)
                vts.getEffectsState().setProperty(P::effectPositionsUserOwned, 0, nullptr);
            recomputeRenderSourceCount();
            calc->recalculateMatrix(nullptr);
        }
    }

    logLine(failures == 0 ? juce::String("SELF-TEST RESULT: ALL PASS")
                          : "SELF-TEST RESULT: " + juce::String(failures) + " FAILURES");
}

void MainComponent::runStereoGeometrySelfTest()
{
    using WFSStereoImage::Axis;
    using WFSStereoImage::AxisFollower;

    // Every geometric assertion below wants the axis the bearing says, not a
    // rate-limited approach to it, so imageAt() is handed a whole second of
    // travel: 360 deg/s over 1 s can reach any target in one step. The limiter
    // gets its own tests (D, D2, D3) where the interval is the point.
    constexpr float kUnlimitedDt = 1.0f;

    int failures = 0;

    auto logLine = [](const juce::String& s) { WFSLogger::getInstance().logInfo(s); };

    auto check = [&](bool ok, const juce::String& what)
    {
        if (! ok) ++failures;
        logLine(juce::String("SELF-TEST ") + (ok ? "PASS " : "FAIL ") + what);
    };

    auto approx = [](float a, float b, float tol) { return std::fabs(a - b) <= tol; };

    // The three calls refreshStereoSliceGeometry() makes per channel, on the
    // azimuths the pass-through backend publishes: slice 0 = 0 (the anchor),
    // slice 1 = -1 (left), slice 2 = +1 (right).
    struct Image { juce::Point<float> centre, left, right; };

    auto imageAt = [kUnlimitedDt](float anchorX, float anchorY, float widthM,
                                 float offsetDeg, AxisFollower& state) -> Image
    {
        const auto axis = WFSStereoImage::rotate(
            WFSStereoImage::followAxis(anchorX, anchorY, state, kUnlimitedDt), offsetDeg);

        Image img;
        float dx = 0.0f, dy = 0.0f;

        WFSStereoImage::sliceOffset(0.0f, widthM, axis, dx, dy);
        img.centre = { anchorX + dx, anchorY + dy };

        WFSStereoImage::sliceOffset(-1.0f, widthM, axis, dx, dy);
        img.left = { anchorX + dx, anchorY + dy };

        WFSStereoImage::sliceOffset(1.0f, widthM, axis, dx, dy);
        img.right = { anchorX + dx, anchorY + dy };

        return img;
    };

    logLine("SELF-TEST begin (stereo image geometry)");

    // A: the dial is the FULL left-to-right distance, so each leg travels half.
    // H rides along: the handedness the whole feature is read against.
    {
        AxisFollower latch;
        const auto img = imageAt(0.0f, 8.0f, 4.0f, 0.0f, latch);

        check(approx(img.left.x, -2.0f, 1.0e-5f) && approx(img.left.y, 8.0f, 1.0e-5f),
              "A: 4 m at (0,8) puts the left leg at (-2,8)");
        check(approx(img.right.x, 2.0f, 1.0e-5f) && approx(img.right.y, 8.0f, 1.0e-5f),
              "A: 4 m at (0,8) puts the right leg at (2,8)");
        check(approx(std::hypot(img.left.x - img.right.x, img.left.y - img.right.y), 4.0f, 1.0e-5f),
              "A: the legs sit exactly the dialled 4 m apart");
        check(img.centre.x == 0.0f && img.centre.y == 8.0f,
              "A: the anchor row stays exactly where the operator put it");
        check(img.right.x > 0.0f,
              "H: azimuth +1 lands audience-right (+X) for an upstage anchor");
    }

    // B: the headline invariant. A metre is a metre wherever the pair stands —
    // the retired fraction-of-the-X-extent reference spread ±R on a circle of
    // radius R at every setting and nothing at all on an array running along Y,
    // so it could not pass this sweep at any tolerance.
    {
        bool allHold = true;
        float worst = 0.0f;

        for (int deg = 0; deg < 360; ++deg)
        {
            const float a = juce::degreesToRadians((float) deg);
            AxisFollower latch;
            const auto img = imageAt(8.0f * std::cos(a), 8.0f * std::sin(a), 4.0f, 0.0f, latch);
            const float span = std::hypot(img.left.x - img.right.x, img.left.y - img.right.y);

            worst = juce::jmax(worst, std::fabs(span - 4.0f));
            allHold = allHold && approx(span, 4.0f, 1.0e-4f);
        }

        check(allHold, "B: 4 m stays 4 m at all 360 bearings on an r=8 circle (worst error "
                       + juce::String(worst, 8) + " m)");
    }

    // C: exactness, not closeness. A width-0 pair must be bit-identical to a
    // point source or the engine's change detection sees a moving source, and
    // the anchor row must not creep at any width the operator can dial.
    {
        bool zeroExact = true;
        bool centreExact = true;
        const float widths[] = { 0.0f, 0.001f, 4.0f, 12.5f, 50.0f };
        const float axisOffsets[] = { 0.0f, 37.0f, -90.0f, 180.0f };

        for (float w : widths)
            for (float off : axisOffsets)
                for (int deg = 0; deg < 360; deg += 15)
                {
                    const float a = juce::degreesToRadians((float) deg);
                    AxisFollower latch;
                    const auto axis = WFSStereoImage::rotate(
                        WFSStereoImage::followAxis(6.0f * std::cos(a), 6.0f * std::sin(a),
                                                   latch, kUnlimitedDt), off);

                    float dx = 1.0f, dy = 1.0f;

                    WFSStereoImage::sliceOffset(0.0f, w, axis, dx, dy);
                    centreExact = centreExact && dx == 0.0f && dy == 0.0f;

                    if (w == 0.0f)
                    {
                        WFSStereoImage::sliceOffset(-1.0f, w, axis, dx, dy);
                        zeroExact = zeroExact && dx == 0.0f && dy == 0.0f;

                        WFSStereoImage::sliceOffset(1.0f, w, axis, dx, dy);
                        zeroExact = zeroExact && dx == 0.0f && dy == 0.0f;
                    }
                }

        check(zeroExact, "C: width 0 gives offsets comparing exactly == 0");
        check(centreExact, "C: slice 0 offset is exactly == 0 at every width and axis");
    }

    // D: the rate limit. A source driving straight through the origin makes the
    // tangential bearing sweep 180° over a few centimetres. The axis must never
    // turn faster than kAxisMaxRateDegPerSec asks for, and it must come out the
    // far side ON the live bearing rather than holding an old one — the hold
    // this replaced did not follow inside a 1 m disc and then snapped through
    // the whole sweep at the exit, which on the default stage (origin at
    // downstage centre) put that snap at the front edge of the stage.
    {
        AxisFollower state;
        constexpr float dt = 0.02f;                 // the 50 Hz tick
        const float maxStepDeg = WFSStereoImage::kAxisMaxRateDegPerSec * dt;

        // Prime well clear of the origin, then walk in 1 cm steps along y = 0.
        WFSStereoImage::followAxis(-5.0f, 0.0f, state, dt);

        Axis previous = state.axis;
        float worstStepDeg = 0.0f;
        bool unitLength = true;

        for (int step = 1; step <= 1000; ++step)
        {
            const float x = -5.0f + (float) step * 0.01f;
            const auto axis = WFSStereoImage::followAxis(x, 0.0f, state, dt);

            const float dot = juce::jlimit(-1.0f, 1.0f, previous.x * axis.x + previous.y * axis.y);
            worstStepDeg = juce::jmax(worstStepDeg, juce::radiansToDegrees(std::acos(dot)));
            unitLength = unitLength && approx(std::hypot(axis.x, axis.y), 1.0f, 1.0e-4f);
            previous = axis;
        }

        // Float slop on acos near dot == 1 is worth a hair of headroom; the
        // failure this guards is a 180° snap, not a hundredth of a degree.
        check(worstStepDeg <= maxStepDeg + 0.01f,
              "D: a traverse through the origin never turns the axis more than the rate allows");
        check(unitLength, "D: the axis stays unit length across a limited traverse");

        // 5 m out on the far side the bearing is trustworthy again and the
        // limiter must have caught up exactly, not left a residue behind.
        const auto settled = WFSStereoImage::followAxis(5.0f, 0.0f, state, 1.0f);
        check(settled.x == 0.0f && settled.y == -1.0f,
              "D: the axis settles bit-exactly on the live bearing once clear");
    }

    // D2: the limiter must be invisible at stage distances. A pair 5 m out moving
    // at a brisk 3 m/s turns its bearing at 34°/s, well inside the cap, so every
    // tick has to land on the live axis as a plain assignment — the width-0 null
    // and A's width invariance both rest on that float being the same float a
    // build with no limiter at all would produce.
    {
        AxisFollower limited, reference;
        constexpr float dt = 0.02f;
        bool identical = true;

        WFSStereoImage::followAxis(-3.0f, 5.0f, limited, dt);

        for (int step = 1; step <= 100; ++step)
        {
            const float x = -3.0f + (float) step * (3.0f * dt);
            const auto a = WFSStereoImage::followAxis(x, 5.0f, limited, dt);

            // What an unlimited build computes: the same expression, freshly primed.
            reference = AxisFollower {};
            const auto b = WFSStereoImage::followAxis(x, 5.0f, reference, dt);

            identical = identical && a.x == b.x && a.y == b.y;
        }

        check(identical, "D2: at stage distances every tick lands bit-exactly on the live axis");
    }

    // D3: the two paths that must snap rather than glide. An unprimed follower
    // has nothing to glide FROM, and an anchor that jumped further than any
    // source travels in a tick did not travel — it was recalled, loaded or
    // flung — so gliding would sweep the image through an orientation the
    // operator never asked for.
    {
        AxisFollower fresh;
        const auto first = WFSStereoImage::followAxis(0.0f, 5.0f, fresh, 0.02f);
        check(first.x == 1.0f && first.y == 0.0f,
              "D3: an unprimed follower snaps to the live axis on its first tick");

        AxisFollower jumped;
        WFSStereoImage::followAxis(0.0f, 5.0f, jumped, 0.02f);          // upstage
        const auto teleported = WFSStereoImage::followAxis(0.0f, -5.0f, jumped, 0.02f);
        check(teleported.x == -1.0f && teleported.y == 0.0f,
              "D3: an anchor jump beyond kAxisTeleportMetres snaps instead of gliding");

        // A step just under the teleport threshold is real travel and must be
        // rate limited, not waved through.
        AxisFollower walked;
        WFSStereoImage::followAxis(0.0f, 0.5f, walked, 0.02f);
        const auto crept = WFSStereoImage::followAxis(0.0f, -0.4f, walked, 0.02f);
        check(! (crept.x == -1.0f && crept.y == 0.0f),
              "D3: a sub-threshold step across the origin is limited, not snapped");
    }

    // D4: the lock. It has to drop the tangential term completely — the same
    // axis for every anchor — because "hold the image still while the source
    // walks" is the whole reason the operator reached for it.
    {
        const float anchors[][2] = { { 0.0f, 8.0f }, { 8.0f, 0.0f }, { -3.0f, -7.0f },
                                     { 0.05f, 0.05f }, { 0.0f, 0.0f } };
        bool constant = true;

        for (const auto& a : anchors)
        {
            AxisFollower state;
            WFSStereoImage::followAxis(0.0f, 8.0f, state, 1.0f);   // prime somewhere else first
            const auto axis = WFSStereoImage::lockedAxis(a[0], a[1], state);
            constant = constant && axis.x == 1.0f && axis.y == 0.0f;
        }

        check(constant, "D4: a locked axis is house left/right at every anchor");

        // Unlocking must glide out of the locked orientation, not out of a
        // follower that went stale while the lock was on.
        AxisFollower state;
        WFSStereoImage::lockedAxis(0.0f, -5.0f, state);
        const auto next = WFSStereoImage::followAxis(0.0f, -5.0f, state, 0.02f);
        check(approx(std::hypot(next.x - 1.0f, next.y), 0.0f, 0.2f),
              "D4: unlocking leaves from the locked axis under the rate limit");
    }

    // E: a pair parked dead centre has no bearing to derive an axis from, so
    // the offset dial is the only thing that can orient it. It must still work,
    // or an in-the-round show loses control of the image exactly where it needs
    // it most.
    {
        AxisFollower latch;
        const auto img = imageAt(0.0f, 0.0f, 4.0f, 0.0f, latch);
        check(approx(img.left.x, -2.0f, 1.0e-5f) && approx(img.left.y, 0.0f, 1.0e-5f)
              && approx(img.right.x, 2.0f, 1.0e-5f) && approx(img.right.y, 0.0f, 1.0e-5f),
              "E: offset 0 at the origin spreads along +/-X");
    }
    {
        AxisFollower latch;
        const auto img = imageAt(0.0f, 0.0f, 4.0f, 90.0f, latch);
        check(approx(img.left.x, 0.0f, 1.0e-5f) && approx(img.left.y, -2.0f, 1.0e-5f)
              && approx(img.right.x, 0.0f, 1.0e-5f) && approx(img.right.y, 2.0f, 1.0e-5f),
              "E: offset 90 at the origin spreads along +/-Y");
    }
    {
        AxisFollower latch;
        const float d = 2.0f * std::sqrt(0.5f);
        const auto img = imageAt(0.0f, 0.0f, 4.0f, 45.0f, latch);
        check(approx(img.left.x, -d, 1.0e-5f) && approx(img.left.y, -d, 1.0e-5f)
              && approx(img.right.x, d, 1.0e-5f) && approx(img.right.y, d, 1.0e-5f),
              "E: offset 45 at the origin spreads along the diagonal");
    }

    // F: the offset is a plain rotation, so two applied in turn equal their sum.
    // The bit-exact early-out at 0 is the thing most likely to break this.
    {
        const float angles[] = { -179.0f, -90.0f, -45.0f, 0.0f, 30.0f, 45.0f, 90.0f, 137.0f, 180.0f };
        const Axis seeds[] = { { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 0.6f, 0.8f }, { -0.28f, -0.96f } };

        bool composes = true;

        for (const auto& seed : seeds)
            for (float t : angles)
                for (float u : angles)
                {
                    const auto twice = WFSStereoImage::rotate(WFSStereoImage::rotate(seed, t), u);
                    const auto once = WFSStereoImage::rotate(seed, t + u);

                    composes = composes && approx(twice.x, once.x, 1.0e-5f)
                                        && approx(twice.y, once.y, 1.0e-5f);
                }

        check(composes, "F: rotate(rotate(a,t),u) == rotate(a,t+u)");
    }

    // G: ±180 is the explicit L/R swap the operator reaches for when a pair is
    // patched or hung backwards.
    {
        AxisFollower plainLatch, flippedLatch;
        const auto plain = imageAt(0.0f, 8.0f, 4.0f, 0.0f, plainLatch);
        const auto flipped = imageAt(0.0f, 8.0f, 4.0f, 180.0f, flippedLatch);

        check(approx(flipped.left.x, plain.right.x, 1.0e-5f)
              && approx(flipped.left.y, plain.right.y, 1.0e-5f)
              && approx(flipped.right.x, plain.left.x, 1.0e-5f)
              && approx(flipped.right.y, plain.left.y, 1.0e-5f),
              "G: offset 180 swaps left and right");
    }

    logLine(failures == 0 ? juce::String("SELF-TEST RESULT: ALL PASS")
                          : "SELF-TEST RESULT: " + juce::String(failures) + " FAILURES");
}

MainComponent::~MainComponent()
{
    WFSLogger::getInstance().logInfo ("Session ending - saving settings");

    // Invalidate in-flight SOFA loader callbacks (they capture this).
    *sofaLoadAlive = false;

    // The Audio Interface window's MIDI selector listens to the trigger and
    // unregisters in its destructor, so it goes first.
    audioInterfaceWindow.reset();

    // Close the MIDI port before UI teardown. The trigger parks notes in
    // atomics and is polled; its only call back into this object is the
    // port-state report, which cannot fire once it is gone.
    midiSnapshotTrigger.reset();

    // Sever NetworkTab's reference to mcpServer + its listener registration
    // on MCPTierEnforcement BEFORE mcpServer is destroyed. Member-destruction
    // order takes mcpServer down before tabbedComponent (which owns
    // NetworkTab), so without this NetworkTab's destructor calls
    // removeListener on freed memory and crashes on shutdown.
    if (networkTab != nullptr)
        networkTab->setMCPServer (nullptr);

    // The Scope window references the parameters, which are destroyed before
    // the session (declared ahead of the tabs so it outlives their rows).
    if (snapshotSession != nullptr)
        snapshotSession->shutdown();

    // Tear down MCP-aware UI before mcpServer destructs. mcpHistoryWindow
    // is declared earlier than mcpServer so it would otherwise outlive
    // the engine + change-record buffer it references.
    mcpHistoryWindow.reset();

    // Stop listening to color scheme changes
    ColorScheme::Manager::getInstance().removeListener(this);

    // Stop and destroy lightpad manager before UI teardown
    if (lightpadManager)
    {
        lightpadManager->stop();
        lightpadManager.reset();
    }

    // Destroy TTSManager singleton before JUCE timer thread shuts down
    TTSManager::shutdown();

    // Clear LocalizationManager resources before JUCE leak detector runs
    LocalizationManager::getInstance().shutdown();

    // Destroy detached map window before tearing down UI
    mapTabWindow.reset();
    mapTabPlaceholder.reset();

    // Remove all child components before destroying LookAndFeel
    // This ensures Windows UI Automation providers are properly released
    removeAllChildren();

    // Reset default LookAndFeel before destroying our custom one
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    wfsLookAndFeel.reset();

    // Stop listening to device manager changes
    deviceManager.removeChangeListener(this);

    // Stop timer
    stopTimer();

    // Save settings before shutdown (while device is still available)
    saveSettings();

    // Also save system config (includes audio patch) to project folder.
    // Auto-save variant: skipped if the folder's config was never loaded this
    // session, so quitting can't clobber a config selected but not yet reloaded.
    auto& fileManager = parameters.getFileManager();
    if (fileManager.hasValidProjectFolder())
        fileManager.autoSaveSystemConfig();

    // Clean up status bar (owned by this component, not TabbedComponent)
    delete statusBar;

    // Stop all processing threads BEFORE shutting down audio device
    // (prevents threads from accessing device state during ASIO teardown)
    if (levelMeteringManager)
    {
        levelMeteringManager->setReverbSources(nullptr, nullptr, 0.0f);
        levelMeteringManager->setEffectsSource(nullptr, 0.0f);
    }
    if (reverbFeedThread)
    {
        reverbFeedThread->stopThread(1000);
        reverbFeedThread.reset();
    }
    // The effects driver reads the shared rings and the calculation engine's
    // matrices through raw pointers: join it before either can go away
    if (effectsHost)
        effectsHost->release();
    inputAlgorithm.releaseResources();
    outputAlgorithm.releaseResources();

    // Unregister our callback first: removeAudioCallback blocks until the audio
    // thread has left it, so nothing can be mid-getNextAudioBlock below.
    deviceManager.removeAudioCallback (&ioCallback);

    // Shutdown audio device (closes it; the base class's AudioSourcePlayer half
    // is inert, we never gave it a source)
    shutdownAudio();

    // Now safe to destroy processor objects
    inputAlgorithm.clear();
    outputAlgorithm.clear();
}

//==============================================================================
void MainComponent::attachAudioCallbacksIfNeeded()
{
    if (audioCallbacksAttached)
        return;

    if (deviceManager.getCurrentAudioDevice() == nullptr)
        return;

    // Registered once and kept: AudioDeviceManager re-arms every registered
    // callback across device changes, so there is nothing to re-attach later.
    //
    // Deliberately NOT setAudioChannels(): that routes through
    // AudioAppComponent's AudioSourcePlayer, whose fixed float* channels[128]
    // caps the callback buffer at 128 channels, and it re-opens the device to
    // do it — re-deriving the channel counts from the current setup and
    // freezing them as AudioDeviceManager's "channels needed", which is how
    // the enable-all masks used to get quietly replaced.
    deviceManager.addAudioCallback (&ioCallback);
    audioCallbacksAttached = true;
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    // Handle device manager changes
    if (source == &deviceManager)
    {
        auto* device = deviceManager.getCurrentAudioDevice();

        if (device != nullptr)
        {
            WFSLogger::getInstance().logInfo ("Audio device changed: " + device->getName()
                                              + " @ " + juce::String (device->getCurrentSampleRate()) + " Hz"
                                              + ", buffer " + juce::String (device->getCurrentBufferSizeSamples()));

            // Update patch matrix hardware channel count from the channels the
            // device actually OPENED, not the ones it merely lists: a channel
            // that was never opened has no slot in the callback buffer, so
            // metering it or sending a test tone to it is silently impossible
            // and showing it as available is a lie.
            parameters.updateHardwareChannelCount (deviceHost.getNumActiveInputs(),
                                                   deviceHost.getNumActiveOutputs());

            // User has successfully selected a device - allow saving from now on
            // This enables saving when user manually selects ASIO after startup failure
            deviceRestoreComplete = true;

            // If audio callbacks weren't attached (e.g., startup with no device),
            // try to attach them now
            if (!audioCallbacksAttached)
            {
                attachAudioCallbacksIfNeeded();
            }
        }
        else
        {
            WFSLogger::getInstance().logWarning ("Audio device changed: no device available");
            DBG("Device changed: no device available");
            audioCallbacksAttached = false;

            // Let the patch trees fall back to the no-device policy
            // (cols = max(64, highest patched channel)).
            parameters.updateHardwareChannelCount (0, 0);
        }
    }
}

void MainComponent::colorSchemeChanged()
{
    // Update tab colors to match new theme
    for (int i = 0; i < tabbedComponent.getNumTabs(); ++i)
    {
        tabbedComponent.setTabBackgroundColour(i, ColorScheme::get().chromeBackground);
    }

    // Notify all components that the LookAndFeel has changed
    // This triggers lookAndFeelChanged() on all child components
    sendLookAndFeelChange();

    // Force repaint of the entire component hierarchy
    repaint();
}

void MainComponent::resizeOutputAttenuation(int numOut, double sampleRate)
{
    numOut = juce::jmax(0, numOut);

    auto newTargets = std::make_unique<std::atomic<float>[]>(static_cast<size_t>(numOut));
    for (int i = 0; i < numOut; ++i)
        newTargets[i].store(1.0f, std::memory_order_relaxed);

    outputAttenuationTargets = std::move(newTargets);
    outputAttenuationTargetsCount = numOut;

    outputAttenuationGains.resize(static_cast<size_t>(numOut));
    for (auto& sv : outputAttenuationGains)
    {
        sv.reset(sampleRate, 0.05);
        sv.setCurrentAndTargetValue(1.0f);
    }

    // Per-output EQ filter bank tracks the same channel count.
    outputEQProcessor.prepare(sampleRate, 0, numOut);
}

void MainComponent::resizeReverbAttenuation(int numReverbs, double sampleRate)
{
    numReverbs = juce::jmax(0, numReverbs);

    auto newTargets = std::make_unique<std::atomic<float>[]>(static_cast<size_t>(numReverbs));
    for (int i = 0; i < numReverbs; ++i)
        newTargets[i].store(1.0f, std::memory_order_relaxed);

    reverbAttenuationTargets = std::move(newTargets);
    reverbAttenuationTargetsCount = numReverbs;

    reverbAttenuationGains.resize(static_cast<size_t>(numReverbs));
    for (auto& sv : reverbAttenuationGains)
    {
        sv.reset(sampleRate, 0.05);
        sv.setCurrentAndTargetValue(1.0f);
    }
}

void MainComponent::recomputeRenderSourceCount()
{
    using Map = spatcore::wfs::RenderSourceMap;

    // Build the slot map from the per-channel type (inputChannelType on each
    // <Input>): mono and stereo channels may interleave freely. Only a
    // structural/type change (stopped-only) can alter the result, so the
    // audio callback may read renderSourceMap unsynchronized. build() fails
    // only if more than kMaxStereoChannels are stamped stereo (hand-edited
    // file) — the UI refuses to create a 9th.
    std::array<uint8_t, Map::kMaxInputChannels> channelTypes {};
    const int numTypes = juce::jlimit (0, (int) Map::kMaxInputChannels, numInputChannels);
    auto& vts = parameters.getValueTreeState();
    for (int i = 0; i < numTypes; ++i)
        if (vts.isInputChannelStereo (i))
            channelTypes[static_cast<size_t> (i)] = Map::Stereo;

    // Effect returns are appended after every input slot and derived slice:
    // the third argument is what makes them render sources of the show, and
    // everything sized from numRenderSources (the routing matrices, the input
    // buffer, the rings, the renderers, binaural) follows from it.
    const int numEffects = juce::jlimit (0, (int) Map::kMaxEffectChannels, vts.getNumEffectChannels());
    if (! Map::build (channelTypes.data(), numTypes, numEffects, renderSourceMap))
    {
        WFSLogger::getInstance().logWarning ("Render-source map build failed (" + juce::String (numTypes) + " inputs, "
                                             + juce::String (numEffects)
                                             + " effects) - treating every channel as mono, with no effect returns");
        const bool ok = Map::buildIdentity (numTypes, renderSourceMap);
        jassert (ok);
        juce::ignoreUnused (ok);
    }

    // The single write of numRenderSources. Every message-thread copy loop that
    // reads a calculation-engine matrix is bounded by this value while the
    // matrices are sized by maxRenderSources, so a count past the budget would
    // be a heap over-read on the 50 Hz path, not an error. The map refuses to
    // build past its own budget and the two budgets are static_asserted equal,
    // so the clamp cannot fire today - it is the defined behaviour for the day
    // it can.
    jassert (renderSourceMap.count <= WFSParameterDefaults::maxRenderSources);
    numRenderSources = juce::jmin (WFSParameterDefaults::maxRenderSources,
                                   renderSourceMap.count > 0 ? renderSourceMap.count : numInputChannels);

    // Both stereo image arrays are keyed by channel SLOT, and a rebuild is
    // exactly the moment a slot can change identity — reorder, delete, type
    // flip, project load. A stale axis follower would hand a pair the previous
    // occupant's orientation and then GLIDE away from it under the rate limit,
    // and a stale leg cache would leave the Map drawing an image bar on a
    // channel that has stopped being stereo. Clearing the follower leaves it
    // unprimed, so the next refresh snaps each surviving pair onto its own
    // axis instead of sweeping there. The next refresh repopulates every
    // channel that still is one.
    stereoAxisState.fill (WFSStereoImage::AxisFollower {});
    for (auto& legs : stereoImageLegs)
    {
        // Discarding a valid pair is a Map change the refresh cannot detect on
        // its own: nothing repopulates a slot that has stopped being stereo, so
        // the erase has to be flagged here or the bar is never painted out.
        stereoImageLegsDirty = stereoImageLegsDirty || legs.valid;
        legs = StereoImageLegs {};
    }

    // The engine renders from the same map (derived rows, slice geometry)
    if (calculationEngine)
        calculationEngine->setRenderSourceMap (renderSourceMap);

    // Budget display: SystemConfigTab shows the derived source total
    if (systemConfigTab)
        systemConfigTab->setRenderSourceTotal (numInputChannels, numRenderSources);

    // Meter aggregation: one meter per visible channel, fed by all of the
    // channel's render sources
    if (levelMeteringManager)
    {
        std::vector<std::vector<int>> aggregation (static_cast<size_t> (numTypes));
        for (int ch = 0; ch < numTypes; ++ch)
        {
            auto& sources = aggregation[static_cast<size_t> (ch)];
            sources.push_back (ch);
            const int firstDerived = renderSourceMap.firstDerivedSlot[static_cast<size_t> (ch)];
            for (int sliceRow = 0; firstDerived >= 0 && sliceRow < Map::kDerivedPerStereo; ++sliceRow)
                sources.push_back (firstDerived + sliceRow);
        }
        levelMeteringManager->setSourceMap (std::move (aggregation));
    }
}

void MainComponent::sanitizeMonoPatchRows()
{
    auto audioPatchTree = parameters.getValueTreeState().getState().getChildWithName(WFSParameterIDs::AudioPatch);
    auto inputPatchTree = audioPatchTree.getChildWithName(WFSParameterIDs::InputPatch);
    if (! inputPatchTree.isValid())
        return;

    juce::String patchDataStr = inputPatchTree.getProperty(WFSParameterIDs::patchData).toString();
    juce::StringArray rows = juce::StringArray::fromTokens(patchDataStr, ";", "");

    bool changed = false;
    for (int row = 0; row < rows.size() && row < numInputChannels; ++row)
    {
        if (parameters.getValueTreeState().isInputChannelStereo (row))
            continue;   // stereo rows keep both columns

        juce::StringArray cols = juce::StringArray::fromTokens(rows[row], ",", "");
        int kept = 0;
        bool rowChanged = false;
        for (int c = 0; c < cols.size(); ++c)
        {
            if (cols[c].getIntValue() == 1 && kept++ > 0)
            {
                cols.set(c, "0");
                rowChanged = true;

                // Logged because this rewrites the operator's patch: on the load
                // path it is how a stereo row saved by a session whose channel
                // list came back all-mono gets its second hardware input taken
                // away, and a silent change to a show's patch is not findable
                // afterwards.
                WFSLogger::getInstance().logWarning(
                    "Patch repair: mono channel "
                    + juce::String(parameters.getValueTreeState().getInputChannelNumber(row))
                    + " held hardware input " + juce::String(c + 1)
                    + " as a second patch point; dropped it");
            }
        }
        if (rowChanged)
        {
            rows.set(row, cols.joinIntoString(","));
            changed = true;
        }
    }

    if (changed)
        inputPatchTree.setProperty(WFSParameterIDs::patchData, rows.joinIntoString(";"), nullptr);
}

void MainComponent::resizeRoutingMatrices()
{
    const int matrixSize = numRenderSources * numOutputChannels;

    delayTimesMs.assign(matrixSize, 0.0f);
    levels.assign(matrixSize, 0.0f);
    hfAttenuation.assign(matrixSize, 0.0f);
    targetDelayTimesMs.assign(matrixSize, 0.0f);
    targetLevels.assign(matrixSize, 0.0f);
    finalTargetDelayTimesMs.assign(matrixSize, 0.0f);
    finalTargetLevels.assign(matrixSize, 0.0f);
    startDelayTimesMs.assign(matrixSize, 0.0f);
    startLevels.assign(matrixSize, 0.0f);

    // Floor Reflection matrices
    frDelayTimesMs.assign(matrixSize, 0.0f);
    frLevels.assign(matrixSize, 0.0f);
    targetFRLevels.assign(matrixSize, 0.0f);
    frHFAttenuation.assign(matrixSize, 0.0f);

    // Initialize with zeros - WFSCalculationEngine will provide real values
    // No more random initialization

    // Resize gradient map evaluators to match input channel count
    {
        size_t newSize = static_cast<size_t> (numInputChannels);
        while (gradientMapEvaluators.size() < newSize)
            gradientMapEvaluators.push_back (std::make_unique<GradientMapEvaluator>());
        if (gradientMapEvaluators.size() > newSize)
            gradientMapEvaluators.resize (newSize);
    }
    updateGradientMapStageBounds();
}

void MainComponent::stopProcessingForConfigurationChange()
{
    if (!audioEngineStarted)
        return;

    // Signal audio callback to stop FIRST — before destroying any processors.
    // The audio thread checks this flag at the top of getNextAudioBlock().
    audioEngineStarted = false;

    processingEnabled = false;
    // processingToggle removed - now managed in System Config tab
    parameters.setConfigParam("ProcessingEnabled", false);

    if (currentAlgorithm == ProcessingAlgorithm::InputBuffer)
    {
        // releaseResources() blocks on stopThread(), giving the audio callback
        // time to see audioEngineStarted=false and exit processBlock() before
        // clear() destroys the processor objects.
        inputAlgorithm.releaseResources();
        inputAlgorithm.clear();
    }
    else if (currentAlgorithm == ProcessingAlgorithm::OutputBuffer)
    {
        outputAlgorithm.releaseResources();
        outputAlgorithm.clear();
    }
#if WFS_GPU_NATIVE
    else if (currentAlgorithm == ProcessingAlgorithm::NativeGpuWfs)
    {
        nativeGpuAlgorithm.releaseResources();
        nativeGpuAlgorithm.clear();
    }
    else if (currentAlgorithm == ProcessingAlgorithm::NativeGpuOutputBuffer)
    {
        nativeGpuOutputAlgorithm.releaseResources();
        nativeGpuOutputAlgorithm.clear();
    }
#endif

    // Clear shared buffer references from consumers before destroying buffers
    if (binauralProcessor)
        binauralProcessor->clearSharedInputBuffers();

    // Stop reverb feed thread and engine for reconfiguration (drop the
    // metering manager's raw feed-thread pointer first; re-wired by the next
    // setupSharedInputFeed). The effects engine is released a few lines below,
    // so its core goes with it.
    if (levelMeteringManager)
    {
        levelMeteringManager->setReverbSources(reverbEngine.get(), nullptr, 0.0f);
        levelMeteringManager->setEffectsSource(nullptr, 0.0f);
    }
    if (reverbFeedThread)
    {
        reverbFeedThread->stopThread(1000);
        reverbFeedThread.reset();
    }
    // The effects driver holds raw pointers into the rings: released BEFORE
    // they are cleared, re-prepared by the next setupSharedInputFeed
    if (effectsHost)
        effectsHost->release();
    sharedInputBuffers.clear();

    if (reverbEngine)
        reverbEngine->stopProcessing();
}

void MainComponent::applySamplerSetPosition (int channelIndex, const juce::ValueTree& samplerNode, int setIndex)
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    int setCount = 0;
    for (int i = 0; i < samplerNode.getNumChildren(); ++i)
    {
        auto child = samplerNode.getChild (i);
        if (child.hasType (SamplerSet))
        {
            if (setCount == setIndex)
            {
                float px = static_cast<float> (child.getProperty (samplerSetPosX, samplerSetPosDefault));
                float py = static_cast<float> (child.getProperty (samplerSetPosY, samplerSetPosDefault));
                float pz = static_cast<float> (child.getProperty (samplerSetPosZ, samplerSetPosDefault));
                parameters.setInputParam (channelIndex, "inputPositionX", px);
                parameters.setInputParam (channelIndex, "inputPositionY", py);
                parameters.setInputParam (channelIndex, "inputPositionZ", pz);
                return;
            }
            ++setCount;
        }
    }
}

void MainComponent::applySamplerControllerMode (int mode)
{
    // Mode 0=Off, 1=Lightpad, 2=Remote
    if (lightpadManager)
    {
        if (mode == 1)
            lightpadManager->start();
        else
            lightpadManager->stop();
    }

    if (inputsTab)
    {
        inputsTab->getSamplerSubTab().setControllerMode (mode);

        if (mode == 2)
        {
            int layout = static_cast<int> (parameters.getConfigParam ("RemotePadGridLayout"));
            int cols = (layout == 1) ? 5 : 3;
            int rows = (layout == 1) ? 3 : 2;
            inputsTab->getSamplerSubTab().setRemotePadGridSize (cols, rows);
        }
    }

    // Send pad config to remote
    if (oscManager)
    {
        if (mode == 2)
        {
            int layout = static_cast<int> (parameters.getConfigParam ("RemotePadGridLayout"));
            int cols = (layout == 1) ? 5 : 3;
            int rows = (layout == 1) ? 3 : 2;
            float sensitivity = static_cast<float> (parameters.getConfigParam ("lightpadSensitivity"));
            if (sensitivity <= 0.0f) sensitivity = 0.05f;
            auto zoneMap = buildZoneToInputMap();
            oscManager->sendRemotePadConfig (true, cols, rows, sensitivity, zoneMap);
        }
        else
        {
            oscManager->sendRemotePadConfig (false, 3, 2, 0.05f, {});
        }
    }
}

void MainComponent::resolveLightpadZoneCollisions()
{
    // A Lightpad zone feeds exactly ONE input. Nothing in the model enforced
    // that — the UI shows existing assignments in the picker and operators
    // simply do not double-book, so the invariant held by convention. A snapshot
    // recall can break it without anyone doing anything wrong: recall channel 3's
    // stored zone 2 while live channel 7 already holds zone 2, and both now claim
    // it. buildZoneToInputMap resolves that with zoneMap[zoneId] = i, so the
    // HIGHEST slot silently wins and the other channel's pad goes dead with no
    // message anywhere.
    //
    // Lowest slot keeps the zone, later claimants are cleared to -1 and logged —
    // the same rule and the same reasoning as dedupeInputPatchColumns: an
    // arbitrary but STABLE choice beats a silent one, and the two agree so a
    // half-repaired session does not contradict itself.
    auto inputs = parameters.getValueTreeState().getInputsState();
    std::map<int, int> firstClaimant;   // zoneId -> slot

    for (int slot = 0; slot < inputs.getNumChildren(); ++slot)
    {
        auto channelSection = inputs.getChild (slot).getChildWithName (WFSParameterIDs::Channel);
        if (! channelSection.isValid())
            continue;

        const int zoneId = static_cast<int> (channelSection.getProperty (WFSParameterIDs::lightpadZoneId, -1));
        if (zoneId < 0)
            continue;

        auto existing = firstClaimant.find (zoneId);
        if (existing == firstClaimant.end())
        {
            firstClaimant[zoneId] = slot;
            continue;
        }

        auto& vts = parameters.getValueTreeState();
        WFSLogger::getInstance().logWarning (
            "Lightpad zone repair: zone " + juce::String (zoneId)
            + " was claimed by input channels " + juce::String (vts.getInputChannelNumber (existing->second))
            + " and " + juce::String (vts.getInputChannelNumber (slot))
            + "; kept it on channel " + juce::String (vts.getInputChannelNumber (existing->second)));

        // Raw setProperty: this is bookkeeping, not an operator edit — it must
        // carry no undo entry and must not mark the project dirty.
        channelSection.setProperty (WFSParameterIDs::lightpadZoneId, -1, nullptr);
    }
}

std::map<int, int> MainComponent::buildZoneToInputMap() const
{
    std::map<int, int> zoneMap;
    auto inputs = parameters.getValueTreeState().getInputsState();
    for (int i = 0; i < inputs.getNumChildren(); ++i)
    {
        auto ch = inputs.getChild (i);
        auto channelSection = ch.getChildWithName (WFSParameterIDs::Channel);
        if (channelSection.isValid())
        {
            int zoneId = static_cast<int> (channelSection.getProperty (
                WFSParameterIDs::lightpadZoneId, -1));
            if (zoneId >= 0)
                zoneMap[zoneId] = i;
        }
    }
    return zoneMap;
}

void MainComponent::resendRemotePadConfig()
{
    if (oscManager == nullptr) return;
    int ctrlMode = static_cast<int> (parameters.getConfigParam ("SamplerControllerMode"));
    if (ctrlMode != 2) return;  // Only for Remote mode

    int layout = static_cast<int> (parameters.getConfigParam ("RemotePadGridLayout"));
    int cols = (layout == 1) ? 5 : 3;
    int rows = (layout == 1) ? 3 : 2;
    float sensitivity = static_cast<float> (parameters.getConfigParam ("lightpadSensitivity"));
    if (sensitivity <= 0.0f) sensitivity = 0.05f;
    auto zoneMap = buildZoneToInputMap();
    oscManager->sendRemotePadConfig (true, cols, rows, sensitivity, zoneMap);
}

void MainComponent::growPatchData(juce::ValueTree& patchTree, int newChannelCount, int numHardwareCols)
{
    if (! patchTree.isValid())
        return;

    juce::String patchDataStr = patchTree.getProperty(WFSParameterIDs::patchData).toString();
    juce::StringArray rows = juce::StringArray::fromTokens(patchDataStr, ";", "");
    int existingRows = rows.size();

    if (newChannelCount <= existingRows)
    {
        // Shrink: truncate rows beyond newChannelCount
        if (newChannelCount < existingRows)
        {
            juce::StringArray trimmed;
            for (int i = 0; i < newChannelCount; ++i)
                trimmed.add(rows[i]);
            patchTree.setProperty(WFSParameterIDs::patchData, trimmed.joinIntoString(";"), nullptr);
            patchTree.setProperty(WFSParameterIDs::rows, newChannelCount, nullptr);
        }
        return;
    }

    // Grow: append new rows with 1:1 diagonal mapping (output patch only —
    // input rows are mirrored per-op by the WFSValueTreeState structural
    // channel ops and normalizeInputPatchRows).
    for (int ch = existingRows; ch < newChannelCount; ++ch)
    {
        juce::StringArray cols;
        for (int c = 0; c < numHardwareCols; ++c)
            cols.add(c == ch ? "1" : "0");
        rows.add(cols.joinIntoString(","));
    }

    patchTree.setProperty(WFSParameterIDs::patchData, rows.joinIntoString(";"), nullptr);
    patchTree.setProperty(WFSParameterIDs::rows, newChannelCount, nullptr);
}

void MainComponent::autoPatchStereoRightColumns()
{
    // Auto-diagonal companion for stereo rows: a pair whose L is patched but
    // whose R is not gets the next hardware column — IF that column is free
    // everywhere (the column side of the invariant is never stolen from).
    // Deliberately a heuristic, not an invariant: a fully unpatched stereo
    // row is left alone, and a claimed next column just leaves R unpatched.
    auto audioPatchTree = parameters.getValueTreeState().getState().getChildWithName(WFSParameterIDs::AudioPatch);
    auto inputPatchTree = audioPatchTree.getChildWithName(WFSParameterIDs::InputPatch);
    if (! inputPatchTree.isValid())
        return;

    juce::String patchDataStr = inputPatchTree.getProperty(WFSParameterIDs::patchData).toString();
    juce::StringArray rows = juce::StringArray::fromTokens(patchDataStr, ";", "");

    // Column claims across ALL rows
    std::vector<bool> columnClaimed(static_cast<size_t>(WFSValueTreeState::maxHardwarePatchChannels), false);
    for (int r = 0; r < rows.size(); ++r)
    {
        juce::StringArray cols = juce::StringArray::fromTokens(rows[r], ",", "");
        for (int c = 0; c < cols.size() && c < (int) columnClaimed.size(); ++c)
            if (cols[c].getIntValue() == 1)
                columnClaimed[static_cast<size_t>(c)] = true;
    }

    bool changed = false;
    for (int r = 0; r < rows.size() && r < numInputChannels; ++r)
    {
        if (! parameters.getValueTreeState().isInputChannelStereo (r))
            continue;

        juce::StringArray cols = juce::StringArray::fromTokens(rows[r], ",", "");
        int patchedCount = 0;
        int leftCol = -1;
        for (int c = 0; c < cols.size(); ++c)
        {
            if (cols[c].getIntValue() == 1)
            {
                if (patchedCount++ == 0)
                    leftCol = c;
            }
        }

        if (patchedCount != 1 || leftCol < 0)
            continue;   // fully unpatched or already a pair

        const int rightCol = leftCol + 1;
        if (rightCol >= WFSValueTreeState::maxHardwarePatchChannels
            || (rightCol < (int) columnClaimed.size() && columnClaimed[static_cast<size_t>(rightCol)]))
            continue;

        while (cols.size() <= rightCol)
            cols.add("0");
        cols.set(rightCol, "1");
        columnClaimed[static_cast<size_t>(rightCol)] = true;
        rows.set(r, cols.joinIntoString(","));
        changed = true;
    }

    if (changed)
        inputPatchTree.setProperty(WFSParameterIDs::patchData, rows.joinIntoString(";"), nullptr);
}

void MainComponent::dedupeInputPatchColumns()
{
    // A hardware input feeds at most ONE channel. That invariant is enforced
    // interactively (commitPatchOperation clears the target column first), but
    // a loaded patchData string is merged in raw, and nothing downstream repairs
    // a violation: loadAudioPatches resolves it by last-writer-wins into
    // inputPatchMap while BOTH rows keep the column in inputPatchPrimaryHw, so
    // one channel silently feeds two.
    //
    // First claimant (lowest row) keeps it — the same "lower wins" rule
    // sanitizeMonoPatchRows uses within a row, so the two agree about which
    // hardware input a half-repaired patch keeps.
    auto audioPatchTree = parameters.getValueTreeState().getState().getChildWithName(WFSParameterIDs::AudioPatch);
    auto inputPatchTree = audioPatchTree.getChildWithName(WFSParameterIDs::InputPatch);
    if (! inputPatchTree.isValid())
        return;

    juce::String patchDataStr = inputPatchTree.getProperty(WFSParameterIDs::patchData).toString();
    juce::StringArray rows = juce::StringArray::fromTokens(patchDataStr, ";", "");

    std::vector<int> claimedBy(static_cast<size_t>(WFSValueTreeState::maxHardwarePatchChannels), -1);
    bool changed = false;

    for (int r = 0; r < rows.size(); ++r)
    {
        juce::StringArray cols = juce::StringArray::fromTokens(rows[r], ",", "");
        bool rowChanged = false;

        for (int c = 0; c < cols.size() && c < (int) claimedBy.size(); ++c)
        {
            if (cols[c].getIntValue() != 1)
                continue;

            auto& owner = claimedBy[static_cast<size_t>(c)];
            if (owner < 0)
            {
                owner = r;
                continue;
            }

            cols.set(c, "0");
            rowChanged = true;

            auto& vts = parameters.getValueTreeState();
            WFSLogger::getInstance().logWarning(
                "Patch repair: hardware input " + juce::String(c + 1)
                + " was claimed by channels " + juce::String(vts.getInputChannelNumber(owner))
                + " and " + juce::String(vts.getInputChannelNumber(r))
                + "; kept it on channel " + juce::String(vts.getInputChannelNumber(owner)));
        }

        if (rowChanged)
        {
            rows.set(r, cols.joinIntoString(","));
            changed = true;
        }
    }

    if (changed)
        inputPatchTree.setProperty(WFSParameterIDs::patchData, rows.joinIntoString(";"), nullptr);
}

void MainComponent::repairInputPatchAfterLoad()
{
    // The one definition of the repair order, so the load path and the
    // channel-count path cannot drift apart.
    //
    // A loaded patch reaches the tree through a bare mergeTreeRecursive: no row
    // count check, no per-row capacity check, no column-uniqueness check. Until
    // this ran on the load path, only a channel COUNT change triggered any of
    // it — so reopening a show whose channel list came back differently (every
    // stereo pair rebuilt as mono, before the channel inventory existed) left
    // two hardware inputs sitting on a mono channel, exactly as saved.
    //
    // Every step is idempotent and a no-op on a well-formed patch, which is what
    // lets both call sites run it without checking whether the other already did.
    parameters.getValueTreeState().normalizeInputPatchRows();  // row count vs channel list
    sanitizeMonoPatchRows();                                   // mono row keeps its lowest column
    dedupeInputPatchColumns();                                 // one owner per hardware input
    autoPatchStereoRightColumns();                             // stereo L with no R gets a free R
}

void MainComponent::loadAudioPatches()
{
    // Load input patch matrix from ValueTree
    auto audioPatchTree = parameters.getValueTreeState().getState().getChildWithName(WFSParameterIDs::AudioPatch);
    auto inputPatchTree = audioPatchTree.getChildWithName(WFSParameterIDs::InputPatch);
    auto outputPatchTree = audioPatchTree.getChildWithName(WFSParameterIDs::OutputPatch);

    // Reset patch maps to "unmapped" (-1)
    inputPatchMap.assign(LevelMeteringManager::MaxHardwareInputs, -1);  // Max hardware inputs
    outputPatchMap.assign(WFSParameterDefaults::maxOutputChannels, -1); // Max WFS outputs
    inputPatchPrimaryHw.assign(WFSParameterDefaults::maxInputChannels, -1);
    inputPatchSecondaryHw.assign(WFSParameterDefaults::maxInputChannels, -1);

    // Load input patches: hardware channel → WFS channel
    if (inputPatchTree.isValid())
    {
        juce::String patchDataStr = inputPatchTree.getProperty(WFSParameterIDs::patchData).toString();
        juce::StringArray rows = juce::StringArray::fromTokens(patchDataStr, ";", "");

        for (int wfsChannel = 0; wfsChannel < rows.size(); ++wfsChannel)
        {
            juce::StringArray cols = juce::StringArray::fromTokens(rows[wfsChannel], ",", "");
            for (int hwChannel = 0; hwChannel < cols.size(); ++hwChannel)
            {
                if (cols[hwChannel].getIntValue() == 1)
                {
                    if (hwChannel < (int) inputPatchMap.size())
                        inputPatchMap[hwChannel] = wfsChannel;

                    // Row-keyed columns, ascending: the LOWER column of a
                    // stereo-pair row is the left channel by convention
                    if (wfsChannel < (int) inputPatchPrimaryHw.size())
                    {
                        if (inputPatchPrimaryHw[wfsChannel] < 0)
                            inputPatchPrimaryHw[wfsChannel] = hwChannel;
                        else if (inputPatchSecondaryHw[wfsChannel] < 0)
                            inputPatchSecondaryHw[wfsChannel] = hwChannel;
                    }
                }
            }
        }
    }

    // Load output patches: WFS channel → hardware channel
    if (outputPatchTree.isValid())
    {
        juce::String patchDataStr = outputPatchTree.getProperty(WFSParameterIDs::patchData).toString();
        juce::StringArray rows = juce::StringArray::fromTokens(patchDataStr, ";", "");

        for (int wfsChannel = 0; wfsChannel < rows.size(); ++wfsChannel)
        {
            juce::StringArray cols = juce::StringArray::fromTokens(rows[wfsChannel], ",", "");
            for (int hwChannel = 0; hwChannel < cols.size(); ++hwChannel)
            {
                if (cols[hwChannel].getIntValue() == 1)
                {
                    if (wfsChannel < (int) outputPatchMap.size())
                        outputPatchMap[wfsChannel] = hwChannel;
                }
            }
        }
    }

    // Apply cols policy using current device counts (0/0 when no device).
    // This keeps cols bounded to the device size or to the highest patched
    // channel (whichever is larger), without needing a device-change event.
    // Opened channels, not listed ones — see changeListenerCallback.
    parameters.updateHardwareChannelCount (deviceHost.getNumActiveInputs(),
                                           deviceHost.getNumActiveOutputs());
}

void MainComponent::applyInputPatch(const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Apply input patching: remap hardware inputs to WFS inputs
    int totalBufferChannels = bufferToFill.buffer->getNumChannels();

    // Prepare patched buffer if needed
    if (patchedInputBuffer.getNumChannels() != numRenderSources ||
        patchedInputBuffer.getNumSamples() < bufferToFill.numSamples)
    {
        patchedInputBuffer.setSize(numRenderSources, bufferToFill.numSamples, false, false, true);
    }

    // Shape-change safety net for the stereo raw buffer (preallocated in
    // prepareToPlay, like patchedInputBuffer above)
    const int stereoRawChannels = 2 * StereoChannelManager::kMaxStereoChannels;
    if (stereoRawBuffer.getNumChannels() != stereoRawChannels ||
        stereoRawBuffer.getNumSamples() < bufferToFill.numSamples)
    {
        stereoRawBuffer.setSize(stereoRawChannels, bufferToFill.numSamples, false, false, true);
    }

    patchedInputBuffer.clear();

    // Copy audio according to input patch map. Stereo-pair rows are skipped:
    // their raw L/R goes into stereoRawBuffer below, and the decomposition
    // stage is the sole writer of their patchedInputBuffer slots (so no
    // read/write aliasing is possible inside the decomposer).
    for (int hwChannel = 0; hwChannel < totalBufferChannels && hwChannel < (int)inputPatchMap.size(); ++hwChannel)
    {
        int wfsChannel = inputPatchMap[hwChannel];
        if (wfsChannel >= 0 && wfsChannel < numInputChannels
            && renderSourceMap.firstDerivedSlot[static_cast<size_t> (wfsChannel)] < 0)
        {
            patchedInputBuffer.copyFrom(wfsChannel, bufferToFill.startSample,
                                        *bufferToFill.buffer, hwChannel,
                                        bufferToFill.startSample, bufferToFill.numSamples);
        }
    }

    // Stereo-pair rows: raw L/R per stereo ordinal (2k = L, 2k+1 = R),
    // ordinals counted in ascending channel order to match RenderSourceMap
    {
        int ordinal = 0;
        for (int ch = 0; ch < numInputChannels
                         && ch < (int) renderSourceMap.firstDerivedSlot.size(); ++ch)
        {
            if (renderSourceMap.firstDerivedSlot[static_cast<size_t> (ch)] < 0)
                continue;

            const int rawL = 2 * ordinal;
            const int rawR = rawL + 1;
            ++ordinal;
            if (rawR >= stereoRawBuffer.getNumChannels())
                break;

            const int hwPair[2] = { ch < (int) inputPatchPrimaryHw.size() ? inputPatchPrimaryHw[ch] : -1,
                                    ch < (int) inputPatchSecondaryHw.size() ? inputPatchSecondaryHw[ch] : -1 };
            const int rawPair[2] = { rawL, rawR };
            for (int side = 0; side < 2; ++side)
            {
                const int hw = hwPair[side];
                if (hw >= 0 && hw < totalBufferChannels)
                    stereoRawBuffer.copyFrom(rawPair[side], bufferToFill.startSample,
                                             *bufferToFill.buffer, hw,
                                             bufferToFill.startSample, bufferToFill.numSamples);
                else
                    stereoRawBuffer.clear(rawPair[side], bufferToFill.startSample,
                                          bufferToFill.numSamples);
            }
        }
    }

    // No copy-back: downstream consumers read directly from patchedInputBuffer
}

int MainComponent::packEffectVisualisationRows (std::vector<float>& delays,
                                               std::vector<float>& levels,
                                               std::vector<float>& hf) const
{
    // The calculation engine publishes the feed matrix at the effects BUDGET's
    // stride, which is what the engine indexes with and never the live count.
    // The tab reads a live-width block, so the rows are re-indexed here exactly
    // as the reverb rows are a few lines above every call site.
    const int numEffects = renderSourceMap.numEffectChannels;
    if (calculationEngine == nullptr || numEffects <= 0)
    {
        delays.clear();
        levels.clear();
        hf.clear();
        return 0;
    }

    const float* calcDelays = calculationEngine->getInputEffectDelayTimesMs();
    const float* calcLevels = calculationEngine->getInputEffectLevels();
    const float* calcHF = calculationEngine->getInputEffectHFAttenuationDb();
    const int calcStride = calculationEngine->getNumEffects();

    delays.assign (static_cast<size_t> (numRenderSources * numEffects), 0.0f);
    levels.assign (static_cast<size_t> (numRenderSources * numEffects), 0.0f);
    hf.assign (static_cast<size_t> (numRenderSources * numEffects), 0.0f);

    for (int inIdx = 0; inIdx < numRenderSources; ++inIdx)
    {
        for (int fx = 0; fx < numEffects; ++fx)
        {
            const size_t srcIdx = static_cast<size_t> (inIdx * calcStride + fx);
            const size_t dstIdx = static_cast<size_t> (inIdx * numEffects + fx);
            delays[dstIdx] = calcDelays[srcIdx];
            levels[dstIdx] = calcLevels[srcIdx];
            hf[dstIdx] = calcHF[srcIdx];
        }
    }

    return numEffects;
}

void MainComponent::meterRenderSourceInputs (int startSample, int numSamples) noexcept
{
    // RT-safe: one pass per render source over patchedInputBuffer, then two
    // relaxed atomic stores. No allocation, no locks, no logging, and no
    // transcendentals in the loop — the decay coefficient is computed once
    // below and the dB conversion happens on the message thread.
    //
    // Ungated on purpose. The obvious optimisation is to skip this when no
    // meter is on screen, but AutomOtion's audio trigger reads the same levels
    // with every meter closed, and the pre-gate hardware feed above already
    // walks up to 512 channels unconditionally — this walks at most as many
    // render sources as the session has, and is strictly cheaper.
    if (levelMeteringManager == nullptr || numSamples <= 0)
        return;

    const double sr = currentDeviceSampleRate.load (std::memory_order_relaxed);
    if (sr <= 0.0)
        return;

    const float decay = LevelMeteringManager::blockDecayCoef (
        numSamples, sr, LevelMeteringManager::kInputMeterTauSeconds);

    const int numSources = juce::jmin (numRenderSources,
                                       patchedInputBuffer.getNumChannels(),
                                       LevelMeteringManager::MaxRenderSources);

    for (int src = 0; src < numSources; ++src)
    {
        // Peak and energy in ONE pass. getMagnitude() + getRMSLevel() read
        // every sample twice for the same two numbers, and getRMSLevel is a
        // scalar double loop ending in a sqrt we would immediately undo.
        const float* data = patchedInputBuffer.getReadPointer (src, startSample);
        float maxAbs = 0.0f;
        float sumSquares = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float v = data[i];
            maxAbs = juce::jmax (maxAbs, std::abs (v));
            sumSquares += v * v;
        }

        levelMeteringManager->pushRenderSourceBlockLevels (
            src, maxAbs, sumSquares / (float) numSamples, decay);
    }

    // Slots retired by a channel-count or mono/stereo change are no longer
    // written, so without this they would hold their last value forever. The
    // per-block stamp below covers "nothing is rendering", not "this particular
    // slot stopped being rendered".
    for (int src = numSources; src < lastMeteredRenderSources; ++src)
        levelMeteringManager->pushRenderSourceBlockLevels (src, 0.0f, 0.0f, 0.0f);
    lastMeteredRenderSources = numSources;

    levelMeteringManager->markRenderSourceMeterBlock();
}

void MainComponent::runStereoDecompositionStage (int startSample, int numSamples) noexcept
{
    // RT-safe: reads only stereoRawBuffer, the retained render-source map
    // (stable while audio runs — type changes are stopped-only) and one
    // RtSnapshot per channel inside processChannel(). No allocation, no locks
    // beyond the snapshot POD copies, no logging.
    if (stereoChannelManager == nullptr || numSamples <= 0)
        return;

    int ordinal = 0;
    const int maxCh = juce::jmin (numInputChannels,
                                  (int) renderSourceMap.firstDerivedSlot.size(),
                                  patchedInputBuffer.getNumChannels());
    for (int ch = 0; ch < maxCh; ++ch)
    {
        const int firstDerived = renderSourceMap.firstDerivedSlot[static_cast<size_t> (ch)];
        if (firstDerived < 0)
            continue;

        const int k = ordinal++;
        const int rawL = 2 * k;
        const int rawR = rawL + 1;
        const int lastSlot = firstDerived + StereoChannelManager::kMaxSlices - 2;

        if (rawR >= stereoRawBuffer.getNumChannels()
            || lastSlot >= patchedInputBuffer.getNumChannels())
            continue;

        float* slicePtrs[StereoChannelManager::kMaxSlices];
        slicePtrs[0] = patchedInputBuffer.getWritePointer (ch, startSample);
        for (int slice = 1; slice < StereoChannelManager::kMaxSlices; ++slice)
            slicePtrs[slice] = patchedInputBuffer.getWritePointer (firstDerived + (slice - 1), startSample);

        stereoChannelManager->processChannel (k,
                                              stereoRawBuffer.getReadPointer (rawL, startSample),
                                              stereoRawBuffer.getReadPointer (rawR, startSample),
                                              slicePtrs, numSamples);
    }
}

bool MainComponent::refreshStereoSliceGeometry()
{
    // Consumed on every pass, including the ones that bail out below: a rebuild
    // that retired the last stereo channel still owes the Map one repaint.
    bool legsChanged = stereoImageLegsDirty;
    stereoImageLegsDirty = false;

    if (calculationEngine == nullptr || stereoChannelManager == nullptr)
        return legsChanged;

    // Nothing to place when no stereo channels exist (the common case)
    if (renderSourceMap.count == renderSourceMap.numInputChannels)
    {
        // A show with no stereo channels never reaches the axis follower, so the
        // clock it rate-limits against must not keep running either: the first
        // pass after a mono-only stretch would otherwise see a huge dt and let
        // the axis turn as far as it likes in one tick.
        lastStereoAxisTickMs = -1.0;
        return legsChanged;
    }

    // Measured tick interval for the axis rate limit. The timer nominally runs
    // at 50 Hz, but hardcoding 20 ms would under-rotate exactly when the message
    // thread has stalled — which is when the anchor has moved furthest. The
    // first pass, and the first after a mono-only stretch, gets 0 so the
    // follower holds rather than sweeping on an interval nobody measured.
    const double nowMs = juce::Time::getMillisecondCounterHiRes();
    const float axisDt = (lastStereoAxisTickMs < 0.0)
                       ? 0.0f
                       : (float) ((nowMs - lastStereoAxisTickMs) * 0.001);
    lastStereoAxisTickMs = nowMs;

    // No speaker position is read anywhere below, and that is the point: the
    // width is an absolute distance, so it means the same thing on a frontal
    // bar, a circle and an array running along Y — the last of which had zero X
    // extent and so silently disabled the width dial entirely under the old
    // fraction-of-the-array reference. It also retires a per-frame
    // numOutputChannels loop from the 50 Hz path. The confidence-collapse curve
    // is Phase 1; Phase 0 slices carry full confidence.

    int ordinal = 0;
    for (int ch = 0; ch < numInputChannels
                     && ch < (int) renderSourceMap.firstDerivedSlot.size(); ++ch)
    {
        if (renderSourceMap.firstDerivedSlot[static_cast<size_t> (ch)] < 0)
            continue;

        const int k = ordinal++;

        auto channelSection = parameters.getValueTreeState().getInputChannelSection (ch);
        const float widthM = juce::jlimit (WFSParameterDefaults::inputStereoWidthMin,
                                           WFSParameterDefaults::inputStereoWidthMax,
            (float) (double) channelSection.getProperty (WFSParameterIDs::inputStereoWidth.toString(),
                                                         WFSParameterDefaults::inputStereoWidthDefault));
        const int axisOffsetDeg = juce::jlimit (WFSParameterDefaults::inputStereoAxisOffsetMin,
                                                WFSParameterDefaults::inputStereoAxisOffsetMax,
            (int) channelSection.getProperty (WFSParameterIDs::inputStereoAxisOffset.toString(),
                                              WFSParameterDefaults::inputStereoAxisOffsetDefault));

        const bool axisLocked = static_cast<int> (channelSection.getProperty (
                                    WFSParameterIDs::inputStereoAxisLock.toString(),
                                    WFSParameterDefaults::inputStereoAxisLockDefault)) != 0;

        // The offset rotates the spread axis in the WORLD frame, so it is NOT
        // mirrored by inputFlipX/Y: the flip already mirrors the anchor, and the
        // automatic axis follows the mirrored bearing on its own. Applying the
        // operator's dialled rotation to the mirror as well would turn it the
        // wrong way on every flipped channel.
        //
        // Locked, the automatic term is gone entirely and the offset becomes an
        // absolute bearing off house left/right; unlocked, followAxis() tracks
        // the tangential bearing under a rate limit so it can never snap.
        const auto anchor = calculationEngine->getCompositeInputPosition (ch);
        auto& axisState = stereoAxisState[static_cast<size_t> (ch)];
        const auto baseAxis = axisLocked
            ? WFSStereoImage::lockedAxis (anchor.x, anchor.y, axisState)
            : WFSStereoImage::followAxis (anchor.x, anchor.y, axisState, axisDt);
        const auto axis = WFSStereoImage::rotate (baseAxis, (float) axisOffsetDeg);

        // Config down (audio side). Width is deliberately absent from the
        // backend config — azimuth is normalized by contract and the metre
        // scaling happens here. Stagger keeps multi-channel STFT work off the
        // same callback (doc §6).
        spatcore::dsp::StereoDecomposerConfig cfg;
        cfg.staggerIndex = k;
        cfg.staggerCount = StereoChannelManager::kMaxStereoChannels;
        stereoChannelManager->publishConfig (k, cfg);

        // Slice state up (audio side) → metre offsets for the engine
        const auto states = stereoChannelManager->getSliceStates (k);

        float offsets[StereoChannelManager::kMaxSlices * 3] = {};
        float gains[StereoChannelManager::kMaxSlices] = {};
        bool active[StereoChannelManager::kMaxSlices] = {};
        for (int slice = 0; slice < StereoChannelManager::kMaxSlices; ++slice)
        {
            const auto& st = states.slices[slice];

            // Z is never written: the image is planar, and a slice must not
            // drift off the height the operator set.
            WFSStereoImage::sliceOffset (juce::jlimit (-1.0f, 1.0f, st.azimuth), widthM, axis,
                                         offsets[slice * 3 + 0], offsets[slice * 3 + 1]);
            gains[slice] = st.gainLinear;
            active[slice] = st.active;
        }

        // Slice 0 is the channel's reference row and renders AT the anchor. A
        // future backend publishing a nonzero azimuth there would silently walk
        // the position the operator placed out from under them.
        jassert (offsets[0] == 0.0f && offsets[1] == 0.0f && offsets[2] == 0.0f);

        calculationEngine->setSliceGeometry (ch, offsets, gains, active);

        // Map read-back: the legs the engine was just handed, in absolute stage
        // metres, so the drawing and the render can never disagree.
        const juce::Point<float> newLeft  { anchor.x + offsets[1 * 3 + 0],
                                            anchor.y + offsets[1 * 3 + 1] };
        const juce::Point<float> newRight { anchor.x + offsets[2 * 3 + 0],
                                            anchor.y + offsets[2 * 3 + 1] };

        auto& legs = stereoImageLegs[static_cast<size_t> (ch)];

        // The threshold is a quarter pixel at the Map's maximum zoom (500 px/m),
        // so a backend whose azimuths never settle to the same float twice
        // cannot pin the Map at a 50 Hz repaint for motion nobody can see. A
        // channel gaining its legs counts on its own — the bar has to appear.
        constexpr float legMoveEpsilonM = 0.0005f;
        if (! legs.valid
            || std::abs (newLeft.x  - legs.left.x)  > legMoveEpsilonM
            || std::abs (newLeft.y  - legs.left.y)  > legMoveEpsilonM
            || std::abs (newRight.x - legs.right.x) > legMoveEpsilonM
            || std::abs (newRight.y - legs.right.y) > legMoveEpsilonM)
            legsChanged = true;

        legs.left  = newLeft;
        legs.right = newRight;
        legs.valid = true;

        // Backend intrinsic latency → the render-latency reference hook
        // (0 for the Phase-0 pass-through, ~21 ms for the Phase-1 STFT)
        calculationEngine->setChannelIntrinsicLatency (ch, stereoChannelManager->getLatencyMs (k));
    }

    return legsChanged;
}

void MainComponent::applyOutputPatch(const juce::AudioSourceChannelInfo& bufferToFill,
                                     const juce::AudioBuffer<float>& wfsOutput)
{
    // Single-pass output remap: WFS output buffer → hardware output buffer
    int numHardwareOutputs = bufferToFill.buffer->getNumChannels();
    int numSamples = bufferToFill.numSamples;
    int startSample = bufferToFill.startSample;

    // Clear hardware output buffer
    bufferToFill.clearActiveBufferRegion();

    // Remap WFS channels to hardware channels using addFrom (supports many-to-one)
    // Iterate up to wfsOutput size to include binaural channels beyond WFS output range
    int patchChannels = juce::jmin(wfsOutput.getNumChannels(), (int)outputPatchMap.size());
    for (int wfsChannel = 0; wfsChannel < patchChannels; ++wfsChannel)
    {
        int hwChannel = outputPatchMap[wfsChannel];
        if (hwChannel >= 0 && hwChannel < numHardwareOutputs && wfsChannel < wfsOutput.getNumChannels())
        {
            bufferToFill.buffer->addFrom(hwChannel, startSample,
                                         wfsOutput, wfsChannel,
                                         startSample, numSamples);
        }
    }
}

void MainComponent::handleProcessingChange(bool enabled)
{
    WFSLogger::getInstance().logInfo (juce::String ("Processing ") + (enabled ? "enabled" : "disabled"));

    // Patching is only editable while stopped, so this is the moment the audio
    // thread needs the edits. Load them BEFORE the flag flips: the callback
    // gates patching on processingEnabled, so nothing is reading the maps yet.
    if (enabled)
        loadAudioPatches();

    processingEnabled = enabled;

    // When starting processing, close the audio interface window and stop test signals
    if (processingEnabled)
    {
        if (audioInterfaceWindow != nullptr && audioInterfaceWindow->isVisible())
            audioInterfaceWindow->setVisible(false);

        if (testSignalGenerator != nullptr)
            testSignalGenerator->reset();
    }

    if (processingEnabled && !audioEngineStarted)
    {
        // Start audio engine on first activation
        startAudioEngine();
    }
    else if (processingEnabled && audioEngineStarted)
    {
        // Re-attach shared input buffers for binaural BEFORE enabling processors
        // (audio callback writes to shared buffers when processingEnabled is true)
        if (binauralProcessor && !sharedInputBuffers.empty())
            binauralProcessor->setSharedInputBuffers(sharedInputBuffers);

        // Restart reverb engine thread (may have been stopped on disable)
        if (reverbEngine)
            reverbEngine->startProcessing();

        // Enable existing processors last (audio callback starts processing)
        if (currentAlgorithm == ProcessingAlgorithm::InputBuffer)
        {
            inputAlgorithm.setProcessingEnabled(true);
        }
        else if (currentAlgorithm == ProcessingAlgorithm::OutputBuffer)
        {
            outputAlgorithm.setProcessingEnabled(true);
        }
#if WFS_GPU_NATIVE
        else if (currentAlgorithm == ProcessingAlgorithm::NativeGpuWfs)
        {
            nativeGpuAlgorithm.setProcessingEnabled(true);
        }
        else if (currentAlgorithm == ProcessingAlgorithm::NativeGpuOutputBuffer)
        {
            nativeGpuOutputAlgorithm.setProcessingEnabled(true);
        }
#endif
    }
    else
    {
        // Disable processing
        if (currentAlgorithm == ProcessingAlgorithm::InputBuffer)
        {
            inputAlgorithm.setProcessingEnabled(processingEnabled);
        }
        else if (currentAlgorithm == ProcessingAlgorithm::OutputBuffer)
        {
            outputAlgorithm.setProcessingEnabled(processingEnabled);
        }
#if WFS_GPU_NATIVE
        else if (currentAlgorithm == ProcessingAlgorithm::NativeGpuWfs)
        {
            nativeGpuAlgorithm.setProcessingEnabled(processingEnabled);
        }
        else if (currentAlgorithm == ProcessingAlgorithm::NativeGpuOutputBuffer)
        {
            nativeGpuOutputAlgorithm.setProcessingEnabled(processingEnabled);
        }
#endif

        // Stop reverb engine thread to save CPU
        if (reverbEngine)
            reverbEngine->stopProcessing();

        // The effects engine stays prepared (the rings persist) but stops being
        // notified; clear it so a tail frozen at the stop does not resume stale
        // whenever processing comes back
        if (effectsHost)
            effectsHost->requestClear();

        // Switch binaural to private ring buffers so binaural-only path can use pushInput
        if (binauralProcessor)
            binauralProcessor->clearSharedInputBuffers();
    }
}

void MainComponent::handleChannelCountChange()
{
    // The four counts come from the tree - the one place every caller used to
    // read them from before passing three of them here. Effects is read and
    // logged beside the others; nothing below consumes it until the engine is
    // wired.
    const int inputs  = parameters.getNumInputChannels();
    const int outputs = parameters.getNumOutputChannels();
    const int reverbs = parameters.getNumReverbChannels();
    const int effects = parameters.getNumEffectChannels();

    WFSLogger::getInstance().logInfo ("Channel count changed: " + juce::String (inputs) + " inputs, "
                                      + juce::String (outputs) + " outputs, "
                                      + juce::String (reverbs) + " reverbs, "
                                      + juce::String (effects) + " effects");
    numInputChannels = inputs;
    numOutputChannels = outputs;

    // Channel inventory to the tablets first, before the reconfiguration pass
    // below spends milliseconds on it. This is the funnel every structural edit
    // reaches — add, remove, move, type flip, config load — so it is the only
    // place that catches a drag-reorder: a reorder changes no count, so the
    // inputChannels ValueTree hook in OSCManager never fires for one, and the
    // tablet would keep drawing the old display order. sendRemoteChannelList
    // skips an unchanged payload, which is what keeps the add/remove loop behind
    // setInputChannelCounts from sending one inventory per step.
    if (oscManager != nullptr)
        oscManager->sendRemoteChannelList();

    recomputeRenderSourceCount();  // keep the renderer dimension in lockstep
    stopProcessingForConfigurationChange();
    resizeRoutingMatrices();

    {
        auto* device = deviceManager.getCurrentAudioDevice();
        double sr = device ? device->getCurrentSampleRate() : 48000.0;
        resizeOutputAttenuation(numOutputChannels, sr);
        resizeReverbAttenuation(reverbs, sr);
    }

    // Patch rows: the structural channel ops already mirror input rows 1:1
    // (row = slot); this normalize only reconciles after a wholesale
    // patchData rewrite (config load). Output rows keep the count-driven
    // grow/truncate. (PatchMatrixComponent may not exist if the Audio
    // Interface window was never opened.)
    {
        auto audioPatchTree = parameters.getValueTreeState().getState().getChildWithName(WFSParameterIDs::AudioPatch);
        auto outputPatchTree = audioPatchTree.getChildWithName(WFSParameterIDs::OutputPatch);
        int hwOutCols = outputPatchTree.isValid() ? (int) outputPatchTree.getProperty(WFSParameterIDs::cols, WFSParameterDefaults::maxOutputChannels) : WFSParameterDefaults::maxOutputChannels;
        parameters.getValueTreeState().normalizeInputPatchRows();
        growPatchData(outputPatchTree, outputs, hwOutCols);
    }

    // Count changes can move the mono/stereo boundary: rows that became mono
    // may hold a leftover second column — drop it (lower column = L is kept);
    // stereo rows with an L but no R get the next free column (auto-diagonal
    // companion); then rebuild the runtime patch maps for the new shape.
    // (normalizeInputPatchRows already ran above; the pass is idempotent.)
    repairInputPatchAfterLoad();
    loadAudioPatches();

    // Update reverb engine node count and resize MainComponent's reverb buffers
    if (reverbEngine)
    {
        reverbEngine->setNumNodes(reverbs);

        // Resize MainComponent's reverb buffers to match the new node count
        auto* device = deviceManager.getCurrentAudioDevice();
        int blockSize = device ? device->getCurrentBufferSizeSamples() : 512;

        if (reverbs > 0)
        {
            reverbFeedBuffer.setSize(reverbs, blockSize);
            reverbReturnBuffer.setSize(reverbs, blockSize);

            if (reverbSRRatio > 1)
            {
                int dsBlockSize = blockSize / reverbSRRatio;
                reverbDownsampleBuf.setSize (reverbs, dsBlockSize);
                reverbUpsampleBuf.setSize (reverbs, dsBlockSize);
            }
        }
        else
        {
            reverbFeedBuffer.setSize(1, blockSize);
            reverbReturnBuffer.setSize(1, blockSize);
        }

        // The return distribution is sized by (nodes x outputs), so both counts
        // move it. prepareToPlay covers a device restart; this covers a channel
        // count edit that does not force one.
        reverbReturnProcessor.prepare (device ? device->getCurrentSampleRate() : 48000.0,
                                       blockSize, reverbs, outputs,
                                       &workgroupCoordinator);
    }

    // Effect channels are built as whole subtrees (a detached node, then
    // appended), so the calculation engine's property listener never saw
    // their positions: re-read them here, as the reverb positions are
    // re-read on a reload. Marks the effects dirty, which the full recalc
    // below consumes.
    if (calculationEngine != nullptr)
        calculationEngine->recalculateAllEffectPositions();

    // Refresh all tabs to update channel selectors
    if (snapshotSession != nullptr)
    {
        snapshotSession->restoreQLabToggles();
        snapshotSession->refreshList();
    }
    if (inputsTab != nullptr)
    {
        inputsTab->refreshFromValueTree();
        inputsTab->configureVisualisation(outputs, reverbs,
                                          parameters.getValueTreeState().getNumEffectChannels());
    }
    if (outputsTab != nullptr)
        outputsTab->refreshFromValueTree();
    if (reverbTab != nullptr)
        reverbTab->refreshFromValueTree();
    if (effectsTab != nullptr)
    {
        effectsTab->refreshFromValueTree();
        // The per-output mute grid and the per-array trims are sized from the
        // output count, which this funnel is what changed.
        effectsTab->refreshOutputDependentControls();
    }

    // Update level meter channel counts
    if (levelMeteringManager != nullptr)
        levelMeteringManager->setChannelCounts(inputs, outputs);
    if (levelMeterWindow != nullptr)
        levelMeterWindow->rebuildMeters();

    // Re-prepare binaural processor with new input channel count
    if (binauralProcessor)
    {
        binauralProcessor->stopProcessing();
        auto* device = deviceManager.getCurrentAudioDevice();
        double sr = device ? device->getCurrentSampleRate() : 48000.0;
        int bs = device ? device->getCurrentBufferSizeSamples() : 512;
        binauralProcessor->prepareToPlay(sr, bs, numRenderSources);
        if (binauralProcessor->isEnabled())
            binauralProcessor->startProcessing();
    }

    // Recalculate WFS matrices so new channel counts have valid delay/level data
    if (calculationEngine != nullptr)
    {
        calculationEngine->recalculateAllListenerPositions();
        calculationEngine->recalculateAllInputPositions();
        calculationEngine->recalculateAllReverbPositions();
        rebuildAllGradientMaps();
        calculationEngine->recalculateMatrix(lsTamerEngine ? lsTamerEngine->getLSGains() : nullptr);

        const float* calcDelays = calculationEngine->getDelayTimesMs();
        const float* calcLevels = calculationEngine->getLevels();
        const float* calcHF = calculationEngine->getHFAttenuationDb();
        const int calcStride = calculationEngine->getNumOutputs();

        for (int inIdx = 0; inIdx < numRenderSources; ++inIdx)
        {
            for (int outIdx = 0; outIdx < numOutputChannels; ++outIdx)
            {
                int srcIdx = inIdx * calcStride + outIdx;
                int dstIdx = inIdx * numOutputChannels + outIdx;
                targetDelayTimesMs[dstIdx] = calcDelays[srcIdx];
                targetLevels[dstIdx] = calcLevels[srcIdx];
                hfAttenuation[dstIdx] = calcHF[srcIdx];
            }
        }

        const float* calcReverbDelays = calculationEngine->getInputReverbDelayTimesMs();
        const float* calcReverbLevels = calculationEngine->getInputReverbLevels();
        const float* calcReverbHF = calculationEngine->getInputReverbHFAttenuationDb();
        const int calcReverbStride = calculationEngine->getNumReverbs();

        std::vector<float> reverbDelays(numRenderSources * reverbs);
        std::vector<float> reverbLevelsVec(numRenderSources * reverbs);
        std::vector<float> reverbHF(numRenderSources * reverbs);

        for (int inIdx = 0; inIdx < numRenderSources; ++inIdx)
        {
            for (int revIdx = 0; revIdx < reverbs; ++revIdx)
            {
                int srcIdx = inIdx * calcReverbStride + revIdx;
                int dstIdx = inIdx * reverbs + revIdx;
                reverbDelays[dstIdx] = calcReverbDelays[srcIdx];
                reverbLevelsVec[dstIdx] = calcReverbLevels[srcIdx];
                reverbHF[dstIdx] = calcReverbHF[srcIdx];
            }
        }

        if (inputsTab != nullptr)
        {
            std::vector<float> effectDelays, effectLevels, effectHF;
            packEffectVisualisationRows (effectDelays, effectLevels, effectHF);

            inputsTab->updateVisualisation(
                targetDelayTimesMs.data(), targetLevels.data(), hfAttenuation.data(),
                reverbDelays.data(), reverbLevelsVec.data(), reverbHF.data(),
                effectDelays.empty() ? nullptr : effectDelays.data(),
                effectLevels.empty() ? nullptr : effectLevels.data(),
                effectHF.empty() ? nullptr : effectHF.data());
        }

        // Mirror the new channel counts and fresh matrix to connected tablets
        // (sendVisualisationToRemotes carries config + selection + rows)
        if (oscManager != nullptr)
            sendVisualisationToRemotes();
    }

    // Reload patch maps from the (now up-to-date) ValueTree
    loadAudioPatches();
}

void MainComponent::sendVisualisationToRemotes(int targetIndex)
{
    if (oscManager == nullptr || calculationEngine == nullptr || !oscManager->hasConnectedRemote())
        return;

    const int numInputs  = parameters.getNumInputChannels();
    const int numOutputs = parameters.getNumOutputChannels();
    const int numReverbs = parameters.getNumReverbChannels();

    // Current desktop selection: the InputsTab channel is always valid; the map
    // contributes either a temporary multi-selection or the members of the
    // selected cluster/barycenter.
    // Wire ids are permanent channel numbers; the tab/map selections are
    // slots, so convert at the boundary.
    auto& vtsVis = parameters.getValueTreeState();
    int primary = inputsTab != nullptr
                      ? vtsVis.getInputChannelNumber(inputsTab->getSelectedInputIndex())
                      : vtsVis.getInputChannelNumber(0);

    // A tab slot with no channel behind it (the selected channel was deleted)
    // resolves to 0, and tablets up to 1.0-beta_12 drop the whole selection for
    // a primary of 0, leaving blank bars. Fall back to the first live channel;
    // 0 then means there is no channel at all, and no selection goes out.
    if (primary < 1)
        primary = vtsVis.getInputChannelNumber(0);

    int clusterId = 0;
    std::vector<int> selection;
    if (mapTab != nullptr)
    {
        clusterId = juce::jmax(0, mapTab->getSelectedBarycenter());
        if (clusterId >= 1)
        {
            for (int i = 0; i < numInputs; ++i)
            {
                juce::var cv = parameters.getValueTreeState()
                                   .getInputParameter(i, WFSParameterIDs::inputCluster);
                if (!cv.isVoid() && static_cast<int>(cv) == clusterId)
                    selection.push_back(vtsVis.getInputChannelNumber(i));
            }
        }
        else
        {
            for (int idx : mapTab->getSelectedInputSet())  // 0-based slots
                selection.push_back(vtsVis.getInputChannelNumber(idx));

            // A single selected input that is a cluster's reference handle
            // (First Input / Shared Position modes, or the tracked member)
            // drags the whole cluster on the map, so mirror all members on
            // the tablet like a barycenter selection does.
            if (selection.size() == 1)
            {
                const int idx0 = vtsVis.getSlotForChannelNumber(selection.front());
                juce::var cv = parameters.getValueTreeState()
                                   .getInputParameter(idx0, WFSParameterIDs::inputCluster);
                const int cluster = cv.isVoid() ? 0 : static_cast<int>(cv);
                if (cluster >= 1 && mapTab->getClusterRef(cluster) == idx0)
                {
                    clusterId = cluster;
                    selection.clear();
                    for (int i = 0; i < numInputs; ++i)
                    {
                        juce::var mv = parameters.getValueTreeState()
                                           .getInputParameter(i, WFSParameterIDs::inputCluster);
                        if (!mv.isVoid() && static_cast<int>(mv) == cluster)
                            selection.push_back(vtsVis.getInputChannelNumber(i));
                    }
                }
            }
        }
    }

    // Config rides every update (≤10 Hz): the one-shot connect-time send can be
    // lost in the state-dump burst on lossy Wi-Fi, leaving the tablet on
    // "waiting for data" forever. Repeats are a no-op on the tablet.
    oscManager->sendRemoteVisConfig(numOutputs, numReverbs, targetIndex);
    if (primary >= 1)
        oscManager->sendRemoteVisSelection(primary, clusterId, selection, targetIndex);

    // Rows straight from the engine matrices (max-channel stride) — independent
    // of windowVisible and of the GUI's re-strided copies.
    const float* delays  = calculationEngine->getDelayTimesMs();
    const float* levels  = calculationEngine->getLevels();
    const int    stride  = calculationEngine->getNumOutputs();
    const float* rDelays = calculationEngine->getInputReverbDelayTimesMs();
    const float* rLevels = calculationEngine->getInputReverbLevels();
    const int    rStride = calculationEngine->getNumReverbs();

    std::set<int> channels(selection.begin(), selection.end());
    channels.insert(primary);

    // Entries are permanent channel numbers; sendRemoteVisRows resolves each
    // to its engine-matrix slot and drops dead numbers itself.
    for (int ch : channels)
        if (ch >= 1)
            oscManager->sendRemoteVisRows(ch, delays, levels, stride, numOutputs,
                                          rDelays, rLevels, rStride, numReverbs, targetIndex);

    // Per-tablet pinned channels not already covered by the selection broadcast
    for (const auto& [pinTarget, pinChannel] : oscManager->getConnectedRemoteVisPins())
    {
        if (targetIndex >= 0 && pinTarget != targetIndex)
            continue;
        if (channels.count(pinChannel) > 0 || pinChannel < 1)
            continue;
        oscManager->sendRemoteVisRows(pinChannel, delays, levels, stride, numOutputs,
                                      rDelays, rLevels, rStride, numReverbs, pinTarget);
    }

    // Every tablet just got the whole state, so the quiet-scene repeat can wait
    // a full interval (selection broadcasts included). A single-target answer
    // restarts nothing: the other tablets were not refreshed.
    if (targetIndex < 0)
        lastVisSendMs = juce::Time::getMillisecondCounter();
}

void MainComponent::handleAlgorithmSelectionChange(int selectedId)
{
    ProcessingAlgorithm newAlgorithm = currentAlgorithm;
    if (selectedId == 1)
        newAlgorithm = ProcessingAlgorithm::InputBuffer;
    else if (selectedId == 2)
        newAlgorithm = ProcessingAlgorithm::OutputBuffer;
#if WFS_GPU_NATIVE
    else if (selectedId == 3)
        newAlgorithm = ProcessingAlgorithm::NativeGpuWfs;
    else if (selectedId == 4)
        newAlgorithm = ProcessingAlgorithm::NativeGpuOutputBuffer;
#endif

    // Read the device too: switching between two GPU devices is the same algoId,
    // so the algorithm alone would mask a device change.
    std::string newDeviceId = parameters.getConfigParam("ProcessingAlgorithmDevice").toString().toStdString();
    if (newDeviceId.empty())
        newDeviceId = "cpu";

    if (newAlgorithm == currentAlgorithm && newDeviceId == currentDeviceId)
        return;

    const bool wasEnabled = processingEnabled;

    // Releases the OUTGOING algorithm's processors (reads currentAlgorithm,
    // so this must run before the switch), gates the audio callback, and
    // stops the reverb feed thread — the same teardown the channel-count
    // change uses.
    stopProcessingForConfigurationChange();

    currentAlgorithm = newAlgorithm;
    currentDeviceId = newDeviceId;
    WFSLogger::getInstance().logInfo ("Processing algorithm changed to id "
                                      + juce::String (selectedId)
                                      + " device " + juce::String (currentDeviceId)
                                      + (wasEnabled ? " - restarting engine" : ""));

    if (wasEnabled)
    {
        parameters.setConfigParam("ProcessingEnabled", true);
        handleProcessingChange(true); // engine not started -> startAudioEngine() with the new algorithm
    }
}

void MainComponent::handleGpuDepthChange(int depthBlocks)
{
#if WFS_GPU_NATIVE
    // The depth is read from the config param at prepare time; a restart
    // applies it. Only meaningful while a GPU algorithm is live.
    if ((currentAlgorithm != ProcessingAlgorithm::NativeGpuWfs
         && currentAlgorithm != ProcessingAlgorithm::NativeGpuOutputBuffer)
        || !audioEngineStarted)
        return;

    const bool wasEnabled = processingEnabled;
    stopProcessingForConfigurationChange();

    WFSLogger::getInstance().logInfo ("GPU pipeline depth changed to "
                                      + juce::String (depthBlocks)
                                      + " blocks" + (wasEnabled ? " - restarting engine" : ""));
    if (wasEnabled)
    {
        parameters.setConfigParam("ProcessingEnabled", true);
        handleProcessingChange(true);
    }
#else
    juce::ignoreUnused (depthBlocks);
#endif
}

void MainComponent::openProjectFromFile (const juce::File& folder)
{
    if (!folder.isDirectory())
    {
        WFSLogger::getInstance().logError ("openProjectFromFile: folder does not exist: " + folder.getFullPathName());
        return;
    }

    WFSLogger::getInstance().logInfo ("Opening project from: " + folder.getFullPathName());

    // The folder switches only once the load is confirmed. A Cancel in the
    // channel-identity dialog below used to leave it on the project that was
    // not opened -- its snapshot list, its MIDI note bindings (armed against
    // the show still loaded, even while the dialog was up) and the folder
    // restored at the next launch.
    const auto systemFile = folder.getChildFile ("system" + juce::String (WFSFileManager::systemConfigExtension));
    const auto inputsFile = folder.getChildFile ("inputs" + juce::String (WFSFileManager::inputConfigExtension));

    // The pair (system.xml vs inputs.xml) is checked before anything is
    // applied. This runs after the message loop is up (Main.cpp defers it
    // through callAsync), so a dialog here is fine.
    ChannelIdentityGate::Context ctx;
    ctx.parent     = this;
    ctx.parameters = &parameters;
    ctx.afterStructuralChange = [this]
    {
        handleChannelCountChange();
    };
    ctx.showStatus = [this] (const juce::String& text)
    {
        WFSLogger::getInstance().logInfo (text);
        if (inputsTab != nullptr) inputsTab->showStatusMessage (text);
    };

    ChannelIdentityGate::confirmThenLoadProject (ctx, systemFile, inputsFile,
        [this, folder]
        {
            auto& fm = parameters.getFileManager();

            // Set folder (also auto-creates .wfs manifest if missing). The
            // clearance the gate granted names folder/system.xml, which is what
            // getSystemConfigFile() resolves to from here on.
            fm.setProjectFolder (folder);
            fm.createProjectFolderStructure();
            AppSettings::setLastFolder ("lastProjectFolder", folder);

            // Suppressed as System Config's Reload Complete Config suppresses the
            // very same load: a project that opens is not an operator edit, and
            // without this every input and effect item it set read as
            // "modified" in the Scope window.
            auto& tracker = parameters.getDirtyTracker();
            tracker.beginSuppression();
            const bool loaded = fm.loadCompleteConfig();
            if (loaded)
                handleConfigReloaded();
            tracker.endSuppressionAndClear();

            if (loaded)
            {
                // Update window title with project name
                if (auto* window = findParentComponentOfClass<juce::DocumentWindow>())
                    window->setName (ProjectInfo::projectName + juce::String (" - ") + folder.getFileName());

                // Diagnostic hook: WFS_TEST_AUTOSTART_PROCESSING starts
                // processing once the project is open, through the same
                // request the Stream Deck's start key makes. It exists for an
                // audio check driven from a shell that has no interactive
                // desktop to long-press the button from; the delay lets the
                // audio device finish opening first. Never set in production.
                if (std::getenv ("WFS_TEST_AUTOSTART_PROCESSING") != nullptr)
                {
                    // A named pointer: in a nested lambda's init-capture MSVC
                    // resolves a bare `this` to the enclosing closure
                    MainComponent* const self = this;
                    juce::Timer::callAfterDelay (1500, [safe = juce::Component::SafePointer<MainComponent> (self)]
                    {
                        if (safe != nullptr && safe->systemConfigTab != nullptr)
                        {
                            WFSLogger::getInstance().logInfo ("WFS_TEST_AUTOSTART_PROCESSING: requesting processing start");
                            safe->systemConfigTab->requestStartProcessing();
                        }
                    });
                }
            }
            else
            {
                WFSLogger::getInstance().logWarning ("openProjectFromFile: no config files found in " + folder.getFullPathName());
            }
        });
}

bool MainComponent::recallSnapshotByName (const juce::String& snapshotName, bool fromMidi, bool fromOsc)
{
    JUCE_ASSERT_MESSAGE_THREAD

    // Not re-entrant: handleConfigReloaded is very heavy, and the dirty
    // tracker's suppression flag has no nesting counter -- an inner
    // endSuppressionAndClear() would un-suppress while the outer load is still
    // writing, marking the outer recall's remaining writes as operator edits.
    if (snapshotRecallInProgress)
        return false;

    auto& fileManager = parameters.getFileManager();

    // A cue that does nothing must say so: the operator is looking at the
    // stage, not at this window, so MIDI failures are logged and announced too.
    auto reportFailure = [this, fromMidi, fromOsc, &snapshotName] (const juce::String& message)
    {
        WFSLogger::getInstance().logWarning ("Snapshot recall of '" + snapshotName + "'"
                                             + (fromMidi ? " (MIDI)" : fromOsc ? " (OSC)" : "")
                                             + " failed: " + message);
        if (inputsTab != nullptr)
            inputsTab->showStatusMessage (message);
        if (fromMidi)
            TTSManager::getInstance().announceImmediate (
                message, juce::AccessibilityHandler::AnnouncementPriority::high);
    };

    if (! fileManager.hasValidProjectFolder())
    {
        reportFailure (LOC("fileManager.errors.noProjectFolder"));
        return false;
    }

    if (! fileManager.getInputSnapshotNames().contains (snapshotName))
    {
        reportFailure (LOC("inputs.messages.snapshotNotFound").replace ("{name}", snapshotName));

        // The file was renamed or removed behind the index's back: rebuild it
        // now, so the next hit of this note already sees what is on disk.
        if (fromMidi)
            refreshMidiSnapshotBindings();
        return false;
    }

    const juce::ScopedValueSetter<bool> reentryGuard (snapshotRecallInProgress, true);

    // Read the scope from the file every time rather than from InputsTab's
    // lazily-populated cache: the cache only covers snapshots the user has
    // touched, and MIDI/OSC can name any of them.
    auto scope = fileManager.getExtendedSnapshotScope (snapshotName);

    // Externally triggered recalls create no undo entry, so a cue-driven show
    // does not bury the operator's own edits under one entry per cue. Scoped
    // tightly around the load so a user Ctrl+Z can never see the suppression.
    const bool external = fromMidi || fromOsc;
    std::optional<WFSValueTreeState::ScopedUndoSuppression> noUndo;
    if (external)
        noUndo.emplace (parameters.getValueTreeState());

    // A cue must never block, so a hardware-input fingerprint mismatch on a
    // cue-driven recall is reported loudly and the cue is applied anyway: the
    // show goes on, and the operator was told at the manual test (the Inputs
    // tab's button goes through a dialog instead - see InputsTab::reloadSnapshot).
    juce::String patchWarning;
    if (external)
    {
        const auto diff = fileManager.preflightSnapshotChannelIdentity (snapshotName);
        if (! diff.patchDiffers.empty())
        {
            patchWarning = LOC("inputs.messages.snapshotPatchMismatchApplied")
                               .replace ("{name}", snapshotName)
                               .replace ("{n}", juce::String ((int) diff.patchDiffers.size()));
            WFSLogger::getInstance().logWarning ("Cue recall of '" + snapshotName
                                                 + "' applied despite a hardware-input fingerprint mismatch ("
                                                 + WFSFileManager::summariseChannelIdentityDiff (diff) + ")");
        }
    }

    parameters.getDirtyTracker().beginSuppression();

    const bool ok = fileManager.loadInputSnapshotWithExtendedScope (snapshotName, scope);

    if (ok)
    {
        // handleConfigReloaded() calls inputsTab->refreshFromValueTree(), which
        // reloads the channel parameters, so the OSC path's old extra
        // refreshFromState() was a duplicate and is deliberately not carried over.
        handleConfigReloaded();

        juce::String statusText = LOC(fromMidi ? "inputs.messages.snapshotLoadedByMidi"
                                               : "inputs.messages.snapshotLoaded")
                                      .replace ("{name}", snapshotName);

        // Entries with no live channel used to vanish silently.
        const auto& skipped = fileManager.getLastRecallSkippedNumbers();
        if (! skipped.empty())
        {
            juce::StringArray nums;
            for (int n : skipped) nums.add ("#" + juce::String (n));
            statusText = LOC("inputs.messages.snapshotEntriesSkipped")
                             .replace ("{name}", snapshotName)
                             .replace ("{n}", juce::String ((int) skipped.size()))
                             .replace ("{numbers}", nums.joinIntoString (", "));
            WFSLogger::getInstance().logInfo (statusText);
        }

        // The same for the effects half: an <Effect> whose id no live channel
        // carries was skipped (and stays in the file).
        const auto& skippedEffects = fileManager.getLastRecallSkippedEffectIds();
        if (! skippedEffects.empty())
        {
            juce::StringArray ids;
            for (int n : skippedEffects) ids.add (juce::String (n));
            const auto effectsText = LOC("inputs.messages.snapshotEffectsSkipped")
                                         .replace ("{name}", snapshotName)
                                         .replace ("{n}", juce::String ((int) skippedEffects.size()))
                                         .replace ("{ids}", ids.joinIntoString (", "));
            WFSLogger::getInstance().logInfo (effectsText);
            statusText = skipped.empty() ? effectsText : statusText + "  " + effectsText;
        }
        if (patchWarning.isNotEmpty())
            statusText = patchWarning;   // the most important line wins the status bar

        if (inputsTab != nullptr)
        {
            // AFTER handleConfigReloaded: that rebuilds the snapshot dropdown, so
            // selecting first would be overwritten by the rebuild. Moving the
            // dropdown cancels a snapshot button held meanwhile, which would
            // otherwise act on the cue's snapshot instead of the one picked.
            if (snapshotSession->selectFromExternalRecall (snapshotName))
                statusText += "  " + LOC("inputs.messages.snapshotActionCancelled");
            inputsTab->showStatusMessage (statusText);
        }

        // A hardware-triggered state change has no visual focus, so announce it.
        // High priority: the TTS rate limit would otherwise drop the second of
        // two cues less than 500 ms apart, and the newest cue is the one that
        // matters.
        if (fromMidi)
            TTSManager::getInstance().announceImmediate (
                patchWarning.isNotEmpty() ? patchWarning
                                          : LOC("inputs.messages.snapshotLoadedByMidi").replace ("{name}", snapshotName),
                juce::AccessibilityHandler::AnnouncementPriority::high);

        // An undo barrier. The cue wrote no undo entry, so the history still
        // holds the operator's earlier edits, recorded against the state the
        // cue has just replaced: Ctrl+Z would set values neither of them chose
        // and, after a Reload Snapshot, remove and insert gradient shapes at
        // stale indices. clearAllUndoHistories() reaches every manager
        // directly, so the suppression still in scope does not hide them.
        if (external)
            parameters.getValueTreeState().clearAllUndoHistories();
    }
    else
    {
        reportFailure (LOC("inputs.messages.snapshotRecallFailed")
                           .replace ("{name}", snapshotName)
                           .replace ("{error}", fileManager.getLastError()));
    }

    parameters.getDirtyTracker().endSuppressionAndClear();
    return ok;
}

void MainComponent::refreshMidiSnapshotBindings()
{
    if (midiSnapshotTrigger == nullptr)
        return;

    std::vector<std::tuple<int, int, juce::String>> rows;

    auto& fileManager = parameters.getFileManager();
    snapshotFolderSignature = fileManager.getInputSnapshotsFolderSignature();

    for (const auto& b : fileManager.scanSnapshotMidiBindings())
        rows.emplace_back (b.channel, b.note, b.snapshotName);

    midiSnapshotTrigger->setBindings (rows);
}

void MainComponent::reportMidiPortState (MidiSnapshotTrigger::PortState previous,
                                         MidiSnapshotTrigger::PortState current)
{
    using PortState = MidiSnapshotTrigger::PortState;

    if (midiSnapshotTrigger == nullptr || current == previous)
        return;

    const auto name = midiSnapshotTrigger->getSelectedName();
    juce::String msg;

    if (current == PortState::refused)
        msg = LOC("inputs.messages.midiPortRefused").replace ("{name}", name);
    else if (current == PortState::absent)
        msg = LOC("inputs.messages.midiPortDisconnected").replace ("{name}", name);
    else if (current == PortState::open && previous != PortState::off)
        msg = LOC("inputs.messages.midiPortConnected").replace ("{name}", name);

    if (msg.isEmpty())
        return;   // choosing a device or Off is its own feedback

    if (current == PortState::open)
        WFSLogger::getInstance().logInfo (msg);
    else
        WFSLogger::getInstance().logWarning (msg);

    if (statusBar != nullptr)
        statusBar->showTemporaryMessage (msg, 5000);
}

void MainComponent::handleConfigReloaded()
{
    WFSLogger::getInstance().logInfo ("Configuration reloaded");

    // Update local channel counts from newly loaded config.
    // Always assign — the cached member must track the ValueTree children count so every
    // per-input loop in timerCallback (speed limiter, LFO offsets, gradient maps, etc.)
    // covers all inputs. Resize matrices only when the counts actually change.
    int newInputChannels = parameters.getNumInputChannels();
    int newOutputChannels = parameters.getNumOutputChannels();
    int newReverbChannels = parameters.getNumReverbChannels();
    bool countsChanged = (newInputChannels != numInputChannels || newOutputChannels != numOutputChannels);
    bool reverbCountChanged = (newReverbChannels != reverbAttenuationTargetsCount);
    numInputChannels = newInputChannels;
    numOutputChannels = newOutputChannels;
    const int previousRenderSources = numRenderSources;
    recomputeRenderSourceCount();  // keep the renderer dimension in lockstep

    // Every routing matrix is numRenderSources x numOutputChannels, and a stereo
    // channel contributes TWO render sources — so a loaded project can change that
    // dimension without changing either count: 8 mono channels replaced by 7 mono
    // plus 1 stereo is still 8 channels and 16 outputs. Without this the matrices
    // keep the previous session's row count while the copy loops below (and every
    // per-render-source loop in timerCallback) walk the new one, writing past the
    // end of the vectors.
    countsChanged = countsChanged || (numRenderSources != previousRenderSources);

    // A load that moves the effects engine's prepared layout - the effect
    // count, or the slot the returns start at - needs rings the engine still
    // reads rebuilt, which cannot happen under a running callback. Stop
    // processing first: the stopped-engine contract every structural edit
    // follows, and the operator restarts as after a count edit.
    if (effectsHost && effectsHost->isPrepared()
        && (parameters.getNumEffectChannels() != effectsHost->getPreparedEffectCount()
            || renderSourceMap.firstEffectSlot != effectsHost->getFirstEffectSlot()))
    {
        WFSLogger::getInstance().logInfo ("Effects layout changed on reload ("
                                          + juce::String (effectsHost->getPreparedEffectCount()) + " -> "
                                          + juce::String (parameters.getNumEffectChannels())
                                          + " effects) - processing stopped for the rebuild");
        stopProcessingForConfigurationChange();
    }

    if (countsChanged)
    {
        resizeRoutingMatrices();

        auto* device = deviceManager.getCurrentAudioDevice();
        double sr = device ? device->getCurrentSampleRate() : 48000.0;
        resizeOutputAttenuation(numOutputChannels, sr);

        // Update level meter channel counts
        if (levelMeteringManager != nullptr)
            levelMeteringManager->setChannelCounts(newInputChannels, newOutputChannels);
        if (levelMeterWindow != nullptr)
            levelMeterWindow->rebuildMeters();
    }
    if (reverbCountChanged)
    {
        auto* device = deviceManager.getCurrentAudioDevice();
        double sr = device ? device->getCurrentSampleRate() : 48000.0;
        resizeReverbAttenuation(newReverbChannels, sr);
    }

    // Reload audio patches from ValueTree (input/output channel routing).
    // Repair FIRST: the patch arrived through a bare mergeTreeRecursive with no
    // validation of row count, per-row capacity or column uniqueness, and
    // loadAudioPatches would bake the damage into the runtime maps rather than
    // report it. This is the funnel every load reaches — project open, snapshot
    // recall, config-reloaded callback — so one call here covers them all.
    repairInputPatchAfterLoad();
    loadAudioPatches();

    // Same reasoning, different resource: a snapshot recall can now restore
    // lightpadZoneId, and a PARTIALLY scoped recall can therefore leave two
    // channels claiming one pad zone. Repair before anything reads the map, then
    // push the corrected assignment to the pads — neither LightpadManager rebuild
    // path is triggered by a recall, so without this the hardware would keep the
    // pre-recall mapping while the tree says otherwise.
    resolveLightpadZoneCollisions();
    if (lightpadManager != nullptr)
    {
        auto inputs = parameters.getValueTreeState().getInputsState();
        for (int i = 0; i < inputs.getNumChildren(); ++i)
        {
            auto channelSection = inputs.getChild (i).getChildWithName (WFSParameterIDs::Channel);
            if (! channelSection.isValid())
                continue;
            const int zoneId = static_cast<int> (channelSection.getProperty (WFSParameterIDs::lightpadZoneId, -1));
            if (zoneId >= 0)
                lightpadManager->assignZoneToInput (zoneId, i);
        }
    }
    resendRemotePadConfig();

    // Drop reposition prompts from the previous session's geometry. Position
    // ownership itself now travels inside the session file (positionsUserOwned).
    if (systemConfigTab != nullptr)
        systemConfigTab->onSessionLoaded();

    // Refresh all tabs to show newly loaded config data
    if (networkTab != nullptr)
        networkTab->refreshFromValueTree();

    // The snapshot row (both tabs): the show's QLab toggles and the folder's list.
    if (snapshotSession != nullptr)
    {
        snapshotSession->restoreQLabToggles();
        snapshotSession->refreshList();
    }

    if (inputsTab != nullptr)
        inputsTab->refreshFromValueTree();

    if (outputsTab != nullptr)
        outputsTab->refreshFromValueTree();

    if (reverbTab != nullptr)
        reverbTab->refreshFromValueTree();
    if (effectsTab != nullptr)
        effectsTab->refreshFromValueTree();

    if (mapTab != nullptr)
        mapTab->repaint();

    if (clustersTab != nullptr)
        clustersTab->refreshFromValueTree();

    // Reconfigure visualization with potentially changed channel counts
    if (inputsTab != nullptr)
    {
        // Ensure the selected channel is still live. currentChannel is a
        // permanent NUMBER: with gaps it can legitimately exceed the channel
        // COUNT, so the count is not a bound. The fallback is the first live
        // channel's number, since number 1 may have been deleted.
        auto& vts = parameters.getValueTreeState();
        int currentChannel = inputsTab->getCurrentChannel();
        if (vts.getSlotForChannelNumber (currentChannel) < 0)
        {
            inputsTab->selectChannel (vts.getInputChannelNumber (0));
        }

        inputsTab->configureVisualisation(parameters.getNumOutputChannels(),
                                          parameters.getNumReverbChannels(),
                                          parameters.getValueTreeState().getNumEffectChannels());

        // Refresh sampler master enable state and controller mode from config
        bool samplerOn = (bool)parameters.getConfigParam("SamplerEnabled");
        inputsTab->setSamplerMasterEnabled(samplerOn);
        int ctrlMode = samplerOn ? static_cast<int> (parameters.getConfigParam ("SamplerControllerMode")) : 0;
        applySamplerControllerMode (ctrlMode);

        // Restart Lightpad and restore zone assignments
        if (lightpadManager && samplerOn)
        {
            lightpadManager->start();

            // Push current topology to SystemConfigTab (won't re-fire if already detected)
            if (systemConfigTab)
                systemConfigTab->updateLightpadLayout (lightpadManager->getPadLayouts());
            auto inputs = parameters.getValueTreeState().getInputsState();
            for (int i = 0; i < inputs.getNumChildren(); ++i)
            {
                auto ch = inputs.getChild (i);
                auto channelSection = ch.getChildWithName (WFSParameterIDs::Channel);
                if (channelSection.isValid())
                {
                    int zoneId = static_cast<int> (channelSection.getProperty (
                        WFSParameterIDs::lightpadZoneId, -1));
                    if (zoneId >= 0)
                        lightpadManager->assignZoneToInput (zoneId, i);
                }
            }
        }

        // Re-activate sampler engines for channels with sampler enabled
        if (samplerManager != nullptr)
        {
            for (int ch = 0; ch < numInputChannels; ++ch)
            {
                auto val = parameters.getInputParam (ch, "inputSamplerActive");
                bool active = ! val.isVoid() && static_cast<int> (val) != 0;
                samplerManager->setChannelActive (ch, active);
                if (active)
                {
                    auto samplerTree = parameters.getValueTreeState().getInputSamplerSection (ch);
                    if (samplerTree.isValid())
                    {
                        auto samplesFolder = parameters.getFileManager().getSamplesFolder();
                        samplerManager->loadChannelCells (ch, samplerTree, samplesFolder);
                        samplerManager->loadChannelSetFromTree (ch, samplerTree, 0);
                        applySamplerSetPosition (ch, samplerTree, 0);
                    }
                }
            }
        }
    }

    // Force full recalculation of DSP matrix after config reload
    // Positions may have changed - recalculate all cached positions
    if (calculationEngine != nullptr)
    {
        calculationEngine->recalculateAllListenerPositions();
        calculationEngine->recalculateAllInputPositions();
        calculationEngine->recalculateAllReverbPositions();
        calculationEngine->recalculateAllEffectPositions();

        // Debug: Print speaker positions after reload
        juce::Logger::writeToLog("=== Speaker Positions After Config Reload ===");
        for (int i = 0; i < juce::jmin(numOutputChannels, 8); ++i)
        {
            auto pos = calculationEngine->getSpeakerPosition(i);
            juce::Logger::writeToLog("Output " + juce::String(i+1) + ": x=" + juce::String(pos.x, 2)
                + " y=" + juce::String(pos.y, 2) + " z=" + juce::String(pos.z, 2));
        }

        // Rebuild all gradient map bitmaps after config reload
        rebuildAllGradientMaps();

        // Force immediate recalculation (don't wait for next timer tick)
        calculationEngine->recalculateMatrix(lsTamerEngine ? lsTamerEngine->getLSGains() : nullptr);

        // Immediately update visualization with recalculated values
        if (inputsTab != nullptr)
        {
            const float* calcDelays = calculationEngine->getDelayTimesMs();
            const float* calcLevels = calculationEngine->getLevels();
            const float* calcHF = calculationEngine->getHFAttenuationDb();
            const int calcStride = calculationEngine->getNumOutputs();

            // Debug: Print calculated levels for input 0
            juce::Logger::writeToLog("=== Calculated Levels for Input 1 ===");
            for (int outIdx = 0; outIdx < juce::jmin(numOutputChannels, 8); ++outIdx)
            {
                int idx = 0 * calcStride + outIdx;
                float levelDb = (calcLevels[idx] > 0.0f) ? 20.0f * std::log10(calcLevels[idx]) : -60.0f;
                juce::Logger::writeToLog("Output " + juce::String(outIdx+1) + ": level=" + juce::String(levelDb, 1)
                    + " dB, delay=" + juce::String(calcDelays[idx], 2) + " ms");
            }

            // Copy to local arrays with correct stride
            for (int inIdx = 0; inIdx < numRenderSources; ++inIdx)
            {
                for (int outIdx = 0; outIdx < numOutputChannels; ++outIdx)
                {
                    int srcIdx = inIdx * calcStride + outIdx;
                    int dstIdx = inIdx * numOutputChannels + outIdx;
                    targetDelayTimesMs[dstIdx] = calcDelays[srcIdx];
                    targetLevels[dstIdx] = calcLevels[srcIdx];
                    hfAttenuation[dstIdx] = calcHF[srcIdx];
                }
            }

            // Create reverb arrays with correct stride
            const float* calcReverbDelays = calculationEngine->getInputReverbDelayTimesMs();
            const float* calcReverbLevels = calculationEngine->getInputReverbLevels();
            const float* calcReverbHF = calculationEngine->getInputReverbHFAttenuationDb();
            const int calcReverbStride = calculationEngine->getNumReverbs();
            int numReverbs = parameters.getNumReverbChannels();

            std::vector<float> reverbDelays(numRenderSources * numReverbs);
            std::vector<float> reverbLevels(numRenderSources * numReverbs);
            std::vector<float> reverbHF(numRenderSources * numReverbs);

            for (int inIdx = 0; inIdx < numRenderSources; ++inIdx)
            {
                for (int revIdx = 0; revIdx < numReverbs; ++revIdx)
                {
                    int srcIdx = inIdx * calcReverbStride + revIdx;
                    int dstIdx = inIdx * numReverbs + revIdx;
                    reverbDelays[dstIdx] = calcReverbDelays[srcIdx];
                    reverbLevels[dstIdx] = calcReverbLevels[srcIdx];
                    reverbHF[dstIdx] = calcReverbHF[srcIdx];
                }
            }

            std::vector<float> effectDelays, effectLevels, effectHF;
            packEffectVisualisationRows (effectDelays, effectLevels, effectHF);

            inputsTab->updateVisualisation(
                targetDelayTimesMs.data(), targetLevels.data(), hfAttenuation.data(),
                reverbDelays.data(), reverbLevels.data(), reverbHF.data(),
                effectDelays.empty() ? nullptr : effectDelays.data(),
                effectLevels.empty() ? nullptr : effectLevels.data(),
                effectHF.empty() ? nullptr : effectHF.data());
        }
    }

    // Re-apply controller device settings from loaded config
    {
        int pcDevice = (int) parameters.getConfigParam ("PositionControlDevice");
        if (controllerManager)
            controllerManager->setEnabled (pcDevice > 0);

        int dbDevice = (int) parameters.getConfigParam ("DialsAndButtonsDevice");
        if (streamDeckManager)
            streamDeckManager->setEnabled (dbDevice == 1);
    }

    // Flush all pending OSC messages immediately after loading
    // This ensures all parameter changes are broadcast to OSC targets
    if (oscManager != nullptr)
    {
        oscManager->flushMessages();

        // Resend full state to connected Remote (Android) targets
        // Ensures input count, positions, and stage config are synchronized after reload
        oscManager->resendStateToRemoteTargets();

        // Mirror the loaded session's visualisation state. The dump above
        // carries the vis config, but selection and delay/level rows only
        // travel through this call — without it a session load left the
        // tablet's Visualisation tab empty until the next selection change
        // (the load recalculates the matrix directly, so the timer's
        // dirty-gated trailing send never fires either). Each dump thread
        // sends the same state again once its dump has gone out, so the rows
        // also land after the dump rather than only racing it.
        sendVisualisationToRemotes();
    }

    // Outside the oscManager block on purpose: the MIDI binding index must be
    // rebuilt on every config reload whether or not OSC was constructed.
    // One directory scan of outer XML elements -- cheap enough to run here.
    refreshMidiSnapshotBindings();
}

void MainComponent::setupPatchWindowStreamDeck (PatchWindowPages::PatchCallbacks& cb,
                                                PatchWindowPages::PatchStateQueries& q)
{
    auto* win = audioInterfaceWindow.get();
    if (win == nullptr)
        return;

    auto* content = win->getContent();
    if (content == nullptr)
        return;

    InputPatchTab*  inTab  = content->getInputPatchTab();
    OutputPatchTab* outTab = content->getOutputPatchTab();

    // --- Sub-tab switching ---
    cb.switchOverrideSubTab = [this] (int subTab)
    {
        juce::MessageManager::callAsync ([this, subTab]()
        {
            if (streamDeckManager)
                streamDeckManager->setOverrideSubTab (subTab);
        });
    };

    cb.switchPatchTab = [content] (int tab)
    {
        // tab: 0=Input, 1=Output, but actual tab indices are 1 and 2 (Device Settings is 0)
        juce::MessageManager::callAsync ([content, tab]()
        {
            content->getTabbedComponent().setCurrentTabIndex (tab + 1);
        });
    };

    // --- Input patch mode ---
    cb.setInputPatchMode = [inTab] (PatchMatrixComponent::Mode mode)
    {
        juce::MessageManager::callAsync ([inTab, mode]()
        {
            if (inTab) inTab->setMode (mode);
        });
    };

    // --- Output patch mode ---
    cb.setOutputPatchMode = [outTab] (PatchMatrixComponent::Mode mode)
    {
        juce::MessageManager::callAsync ([outTab, mode]()
        {
            if (outTab) outTab->setMode (mode);
        });
    };

    // --- Input scroll/select ---
    cb.scrollInputByCell = [inTab] (int dx, int dy)
    {
        juce::MessageManager::callAsync ([inTab, dx, dy]()
        {
            if (inTab && inTab->getPatchMatrix())
                inTab->getPatchMatrix()->scrollByCell (dx, dy);
        });
    };

    cb.moveInputSelectedCell = [inTab] (int dx, int dy)
    {
        juce::MessageManager::callAsync ([inTab, dx, dy]()
        {
            if (inTab && inTab->getPatchMatrix())
            {
                auto* m = inTab->getPatchMatrix();
                auto sel = m->getSelectedCell();
                if (sel.x < 0) sel = { 0, 0 };
                m->setSelectedCell ({ sel.x + dx, sel.y + dy });
            }
        });
    };

    cb.activateInputSelectedCell = [inTab]()
    {
        juce::MessageManager::callAsync ([inTab]()
        {
            if (inTab && inTab->getPatchMatrix())
                inTab->getPatchMatrix()->activateSelectedCell();
        });
    };

    // --- Output scroll/select ---
    cb.scrollOutputByCell = [outTab] (int dx, int dy)
    {
        juce::MessageManager::callAsync ([outTab, dx, dy]()
        {
            if (outTab && outTab->getPatchMatrix())
                outTab->getPatchMatrix()->scrollByCell (dx, dy);
        });
    };

    cb.moveOutputSelectedCell = [outTab] (int dx, int dy)
    {
        juce::MessageManager::callAsync ([outTab, dx, dy]()
        {
            if (outTab && outTab->getPatchMatrix())
            {
                auto* m = outTab->getPatchMatrix();
                auto sel = m->getSelectedCell();
                if (sel.x < 0) sel = { 0, 0 };
                m->setSelectedCell ({ sel.x + dx, sel.y + dy });
            }
        });
    };

    cb.activateOutputSelectedCell = [outTab]()
    {
        juce::MessageManager::callAsync ([outTab]()
        {
            if (outTab && outTab->getPatchMatrix())
                outTab->getPatchMatrix()->activateSelectedCell();
        });
    };

    // --- Test signal controls ---
    cb.toggleHold = [outTab]()
    {
        juce::MessageManager::callAsync ([outTab]()
        {
            if (outTab)
                outTab->setHoldEnabled (! outTab->isHoldEnabled());
        });
    };

    cb.setTestSignalType = [outTab] (int type)
    {
        juce::MessageManager::callAsync ([outTab, type]()
        {
            if (outTab && outTab->getTestSignalGenerator())
            {
                outTab->getTestSignalGenerator()->setSignalType (static_cast<TestSignalGenerator::SignalType> (type));
                outTab->syncTestControlsFromGenerator();
            }
        });
    };

    cb.setTestLevel = [outTab] (float dB)
    {
        juce::MessageManager::callAsync ([outTab, dB]()
        {
            if (outTab && outTab->getTestSignalGenerator())
            {
                outTab->getTestSignalGenerator()->setLevel (dB);
                outTab->syncTestControlsFromGenerator();
            }
        });
    };

    cb.setTestFrequency = [outTab] (float hz)
    {
        juce::MessageManager::callAsync ([outTab, hz]()
        {
            if (outTab && outTab->getTestSignalGenerator())
            {
                outTab->getTestSignalGenerator()->setFrequency (hz);
                outTab->syncTestControlsFromGenerator();
            }
        });
    };

    // --- State queries ---
    q.getCurrentPatchTab = [content]()
    {
        return content->getTabbedComponent().getCurrentTabIndex();
    };

    q.getInputPatchMode = [inTab]()
    {
        if (inTab && inTab->getPatchMatrix())
            return static_cast<int> (inTab->getPatchMatrix()->getMode());
        return 0;
    };

    q.getOutputPatchMode = [outTab]()
    {
        if (outTab && outTab->getPatchMatrix())
            return static_cast<int> (outTab->getPatchMatrix()->getMode());
        return 0;
    };

    // Input matrix state
    q.getInputNumHardwareChannels = [inTab]()
    {
        return (inTab && inTab->getPatchMatrix()) ? inTab->getPatchMatrix()->getNumHardwareChannels() : 0;
    };
    q.getInputNumWFSChannels = [inTab]()
    {
        return (inTab && inTab->getPatchMatrix()) ? inTab->getPatchMatrix()->getNumWFSChannels() : 0;
    };
    q.getInputScrollCol = [inTab]()
    {
        if (inTab && inTab->getPatchMatrix())
        {
            auto* m = inTab->getPatchMatrix();
            return (m->getCellWidth() > 0) ? m->getScrollOffsetX() / m->getCellWidth() : 0;
        }
        return 0;
    };
    q.getInputScrollRow = [inTab]()
    {
        if (inTab && inTab->getPatchMatrix())
        {
            auto* m = inTab->getPatchMatrix();
            return (m->getCellHeight() > 0) ? m->getScrollOffsetY() / m->getCellHeight() : 0;
        }
        return 0;
    };
    q.getInputSelectedCol = [inTab]()
    {
        return (inTab && inTab->getPatchMatrix()) ? juce::jmax (0, inTab->getPatchMatrix()->getSelectedCell().x) : 0;
    };
    q.getInputSelectedRow = [inTab]()
    {
        return (inTab && inTab->getPatchMatrix()) ? juce::jmax (0, inTab->getPatchMatrix()->getSelectedCell().y) : 0;
    };

    // Output matrix state
    q.getOutputNumHardwareChannels = [outTab]()
    {
        return (outTab && outTab->getPatchMatrix()) ? outTab->getPatchMatrix()->getNumHardwareChannels() : 0;
    };
    q.getOutputNumWFSChannels = [outTab]()
    {
        return (outTab && outTab->getPatchMatrix()) ? outTab->getPatchMatrix()->getNumWFSChannels() : 0;
    };
    q.getOutputScrollCol = [outTab]()
    {
        if (outTab && outTab->getPatchMatrix())
        {
            auto* m = outTab->getPatchMatrix();
            return (m->getCellWidth() > 0) ? m->getScrollOffsetX() / m->getCellWidth() : 0;
        }
        return 0;
    };
    q.getOutputScrollRow = [outTab]()
    {
        if (outTab && outTab->getPatchMatrix())
        {
            auto* m = outTab->getPatchMatrix();
            return (m->getCellHeight() > 0) ? m->getScrollOffsetY() / m->getCellHeight() : 0;
        }
        return 0;
    };
    q.getOutputSelectedCol = [outTab]()
    {
        return (outTab && outTab->getPatchMatrix()) ? juce::jmax (0, outTab->getPatchMatrix()->getSelectedCell().x) : 0;
    };
    q.getOutputSelectedRow = [outTab]()
    {
        return (outTab && outTab->getPatchMatrix()) ? juce::jmax (0, outTab->getPatchMatrix()->getSelectedCell().y) : 0;
    };

    // Test signal state
    q.isHoldEnabled = [outTab]()
    {
        return outTab ? outTab->isHoldEnabled() : false;
    };
    q.getTestSignalType = [outTab]()
    {
        if (outTab && outTab->getTestSignalGenerator())
            return static_cast<int> (outTab->getTestSignalGenerator()->getSignalType());
        return 0;
    };
    q.getTestLevel = [outTab]()
    {
        if (outTab && outTab->getTestSignalGenerator())
            return outTab->getTestSignalGenerator()->getLevelDb();
        return -40.0f;
    };
    q.getTestFrequency = [outTab]()
    {
        if (outTab && outTab->getTestSignalGenerator())
            return outTab->getTestSignalGenerator()->getFrequency();
        return 1000.0f;
    };
}

//==============================================================================
// Detachable Map Window
//==============================================================================

void MainComponent::detachMapTab()
{
    if (mapTabWindow != nullptr)
    {
        // Already detached — bring to front
        mapTabWindow->toFront(true);
        return;
    }

    // Remove MapTab from TabbedComponent (ownership=false, so it won't be deleted).
    // By name, not number: the tab before it is owned, and removing that one
    // deletes it.
    tabbedComponent.removeTab(TabIndex::Map);

    // The placeholder goes in the Map's place (it is the last tab)
    mapTabPlaceholder = std::make_unique<MapTabPlaceholder>();
    mapTabPlaceholder->onReattachRequested = [this]() { attachMapTab(); };
    juce::String tabMap = LOC("tabs.map");
    tabbedComponent.addTab(tabMap, ColorScheme::get().chromeBackground, mapTabPlaceholder.get(), false);

    // Create the detached window (displays MapTab without owning it)
    mapTabWindow = std::make_unique<MapTabWindow>(mapTab.get());
    mapTab->setDetached(true);

    // Stream Deck: show map pages when detached window is focused
    mapTabWindow->onWindowFocused = [this]()
    {
        if (streamDeckManager)
            streamDeckManager->setMainTab(MapTabPages::MAP_MAIN_TAB_INDEX);
    };

    mapTabWindow->onWindowUnfocused = [this]()
    {
        if (streamDeckManager)
            streamDeckManager->setMainTab(tabbedComponent.getCurrentTabIndex());
    };

    mapTabWindow->onWindowClosed = [this]()
    {
        // Re-attach on next message loop iteration to avoid destroying
        // the window from within its own callback
        juce::MessageManager::callAsync([this]() { attachMapTab(); });
    };
}

void MainComponent::attachMapTab()
{
    if (mapTabWindow == nullptr)
        return;

    // Destroy the detached window (setContentNonOwned — MapTab is NOT deleted)
    mapTabWindow.reset();
    mapTab->setDetached(false);

    // Remove the placeholder from the Map's place
    tabbedComponent.removeTab(TabIndex::Map);
    mapTabPlaceholder.reset();

    // Re-add MapTab in its place (the last tab)
    juce::String tabMap = LOC("tabs.map");
    tabbedComponent.addTab(tabMap, ColorScheme::get().chromeBackground, mapTab.get(), false);

    // Show the re-attached map tab
    tabbedComponent.setCurrentTabIndex(TabIndex::Map);

    // Restore Stream Deck to current tab
    if (streamDeckManager)
        streamDeckManager->setMainTab(TabIndex::Map);
}

void MainComponent::openAudioInterfaceWindow()
{
    // Patching a live rig is how speakers get destroyed and how a room gets a
    // burst of full-level noise, so the window is stopped-only: processing
    // start closes it (handleProcessingChange), and until processing stops it
    // cannot be reopened.
    if (processingEnabled)
    {
        const auto message = LOC("audioPatch.messages.stopProcessingFirst");

        if (statusBar != nullptr)
            statusBar->showTemporaryMessage (message, 3000);

        TTSManager::getInstance().announceImmediate (message,
            juce::AccessibilityHandler::AnnouncementPriority::medium);
        return;
    }

    // Opening this window no longer spends the latch — merely LOOKING at the
    // default patch must leave a fresh session fresh. The latch moved to the
    // first actual patch edit (AudioPatchTab's onBeforePatchEdit, wired below),
    // which still fires before the write lands, preserving the re-flow's
    // call-graph contract: anything that authors a patch latches first.

    if (audioInterfaceWindow == nullptr)
    {
        audioInterfaceWindow = std::make_unique<AudioInterfaceWindow>(
            deviceManager,
            parameters.getValueTreeState(),
            testSignalGenerator.get(),
            midiSnapshotTrigger.get()
        );

        // Wire Stream Deck+ focus callbacks
        audioInterfaceWindow->onWindowFocused = [this]()
        {
            if (! streamDeckManager)
                return;

            // Build the callbacks & state queries, then set the override factory
            PatchWindowPages::PatchCallbacks patchCB;
            PatchWindowPages::PatchStateQueries patchQ;
            setupPatchWindowStreamDeck (patchCB, patchQ);

            streamDeckManager->setOverridePageFactory ([patchCB, patchQ] (int subTab)
            {
                return PatchWindowPages::createPage (subTab, patchCB, patchQ);
            });
        };

        audioInterfaceWindow->onWindowUnfocused = [this]()
        {
            if (streamDeckManager && streamDeckManager->hasOverride())
                streamDeckManager->clearOverridePageFactory();
        };

        // Bidirectional sync: UI tab changes → StreamDeck+ page changes
        if (auto* content = audioInterfaceWindow->getContent())
        {
            content->setOnTabChanged([this](int tabIndex)
            {
                if (streamDeckManager && streamDeckManager->hasOverride())
                    streamDeckManager->setOverrideSubTab(tabIndex);
            });

            // Bidirectional sync: UI mode changes → StreamDeck+ page refresh
            if (auto* inTab = content->getInputPatchTab())
            {
                inTab->onModeChanged = [this](PatchMatrixComponent::Mode)
                {
                    if (streamDeckManager && streamDeckManager->hasOverride())
                        streamDeckManager->refreshCurrentPage();
                };

                // Signal-presence tint on hardware-input header cells.
                inTab->setHardwareInputPeakProvider([this](int ch) -> float
                {
                    return levelMeteringManager
                               ? levelMeteringManager->getHardwareInputPeakDb(ch)
                               : -200.0f;
                });
            }
            if (auto* outTab = content->getOutputPatchTab())
            {
                outTab->onModeChanged = [this](PatchMatrixComponent::Mode)
                {
                    if (streamDeckManager && streamDeckManager->hasOverride())
                        streamDeckManager->refreshCurrentPage();
                };
            }

            // Auto-save patch to disk when routing changes (debounced 3s)
            auto wirePatchAutoSave = [this](PatchMatrixComponent* matrix)
            {
                if (matrix)
                    matrix->onPatchChanged = [this]() { patchSaveCountdown = 600; };
            };
            if (auto* inTab = content->getInputPatchTab())
                wirePatchAutoSave (inTab->getPatchMatrix());
            if (auto* outTab = content->getOutputPatchTab())
                wirePatchAutoSave (outTab->getPatchMatrix());

            // The INPUT patch is where the fresh-session latch is spent now that
            // opening this window no longer does: the first hand edit is the act
            // that authors the patch, and it must latch BEFORE the write lands so
            // the next structural edit's re-flow cannot eat it (the re-flow
            // contract). User edits only — the matrix's programmatic saves that
            // react to channel-count changes do not fire this. The OUTPUT patch
            // never latches input numbering.
            if (auto* inTab = content->getInputPatchTab())
                if (auto* matrix = inTab->getPatchMatrix())
                    matrix->onBeforeUserPatchEdit = [this]()
                    {
                        parameters.getValueTreeState()
                            .markChannelNumbersUserOwned ("input patch edit");
                    };

            // The output tab forwards the matrix's status messages, but nothing
            // was listening, so every one of them (including "choose a test
            // signal") went nowhere and a rejected test click looked exactly
            // like a working one that made no sound. Only the output patch has
            // a testing mode, so only it produces these.
            if (auto* outTab = content->getOutputPatchTab())
                outTab->onStatusMessage = [this](const juce::String& message)
                {
                    if (statusBar != nullptr)
                        statusBar->showTemporaryMessage (message, 3000);
                };
        }
    }
    else
    {
        audioInterfaceWindow->setVisible(true);
        audioInterfaceWindow->toFront(true);
    }

    // Always trigger the focus callback to ensure StreamDeck+ page switches
    // (activeWindowStatusChanged doesn't fire reliably on first show or reshow)
    if (audioInterfaceWindow->onWindowFocused)
        audioInterfaceWindow->onWindowFocused();

    // If wizard is on the "Open Audio Interface" step, auto-advance
    if (gettingStartedWizard && gettingStartedWizard->isActive()
        && gettingStartedWizard->getCurrentStepIndex() == 6)
    {
        gettingStartedWizard->nextStep();
    }
}

void MainComponent::openNetworkLogWindow()
{
    if (networkLogWindow == nullptr)
    {
        // Get project folder from file manager
        juce::File projectFolder = parameters.getFileManager().getProjectFolder();

        networkLogWindow = std::make_unique<NetworkLogWindow>(
            oscManager->getLogger(),
            *oscManager,
            projectFolder
        );
    }
    else
    {
        networkLogWindow->setVisible(true);
        networkLogWindow->toFront(true);
    }
}

void MainComponent::openMCPHistoryWindow()
{
    if (mcpServer == nullptr)
        return;

    if (mcpHistoryWindow == nullptr)
    {
        mcpHistoryWindow = std::make_unique<MCPHistoryWindow>(
            mcpServer->getUndoEngine(),
            mcpServer->getChangeRecords()
        );
    }
    mcpHistoryWindow->setVisible(true);
    mcpHistoryWindow->toFront(true);
}

void MainComponent::openLevelMeterWindow()
{
    // Guard hoisted out of the creation branch: without a metering manager there
    // is no window to show, and an open that shows nothing must not spend the
    // latch below.
    if (levelMeteringManager == nullptr)
        return;

    // Opening the meters no longer spends the latch: it is a display-only
    // window, and a fresh-session renumber reaches it anyway — every structural
    // edit runs handleChannelCountChange, which rebuilds the meter labels.

    if (levelMeterWindow == nullptr)
    {
        levelMeterWindow = std::make_unique<LevelMeterWindow>(*levelMeteringManager,
                                                                 parameters.getValueTreeState(),
                                                                 calculationEngine.get());
    }
    else
    {
        levelMeterWindow->setVisible(true);
        levelMeterWindow->toFront(true);
        levelMeteringManager->setMeterWindowEnabled(true);
    }
}

//==============================================================================
void MainComponent::openGettingStartedWizard()
{
    // Close any existing wizard first
    gettingStartedWizard.reset();

    gettingStartedWizard = std::make_unique<GettingStartedWizard>(
        *this,
        [this](int tabIndex) { tabbedComponent.setCurrentTabIndex(tabIndex); }
    );

    gettingStartedWizard->onWizardClosed = [this]() {
        // Save current step for resume (reset to 0 if wizard was completed)
        if (gettingStartedWizard)
        {
            int step = gettingStartedWizard->getCurrentStepIndex();
            lastWizardStepIndex = (step >= 11) ? 0 : step; // 11 = last step index (Explore Inputs)
        }
        // Clear window-close hooks so they don't fire after wizard is gone
        if (audioInterfaceWindow)
            audioInterfaceWindow->onWindowClosed = nullptr;
        gettingStartedWizard.reset();
    };

    // Helper: convert bounds from a tab's local coords to MainComponent coords
    auto tabBoundsToMain = [this](juce::Component* tab, juce::Rectangle<int> localBounds) -> juce::Rectangle<int> {
        if (tab == nullptr) return {};
        auto topLeft = tab->localPointToGlobal(localBounds.getTopLeft());
        auto bottomRight = tab->localPointToGlobal(localBounds.getBottomRight());
        return juce::Rectangle<int>(getLocalPoint(nullptr, topLeft),
                                     getLocalPoint(nullptr, bottomRight));
    };

    // Step indices (0-based) for skip targets:
    // 0=ProjectFolder, 1=Inputs, 2=Outputs, 3=Reverbs, 4=Stage, 5=Origin,
    // 6=OpenAudioInterface, 7=ConfigureAudioInterface,
    // 8=OpenWizardOfOutZ, 9=ConfigureOutputPositions,
    // 10=StartProcessing

    // Step 0: Select Project Folder
    gettingStartedWizard->addStep({
        0, // SystemConfig tab
        "wizard.steps.projectFolder.title",
        "wizard.steps.projectFolder.description",
        [this, tabBoundsToMain]() -> juce::Rectangle<int> {
            return tabBoundsToMain(systemConfigTab, systemConfigTab->getProjectFolderButtonBounds());
        },
        [this]() -> bool {
            return parameters.getFileManager().hasValidProjectFolder();
        },
        nullptr, nullptr, -1
    });

    // Step 1: Set Mono and Stereo Inputs (spotlight covers both count rows)
    gettingStartedWizard->addStep({
        0,
        "wizard.steps.inputChannels.title",
        "wizard.steps.inputChannels.description",
        [this, tabBoundsToMain]() -> juce::Rectangle<int> {
            return tabBoundsToMain(systemConfigTab, systemConfigTab->getInputChannelsBounds());
        },
        [this]() -> bool { return parameters.getNumInputChannels() > 0; },
        nullptr, nullptr, -1
    });

    // Step 2: Set Output Channels
    gettingStartedWizard->addStep({
        0,
        "wizard.steps.outputChannels.title",
        "wizard.steps.outputChannels.description",
        [this, tabBoundsToMain]() -> juce::Rectangle<int> {
            return tabBoundsToMain(systemConfigTab, systemConfigTab->getOutputChannelsBounds());
        },
        [this]() -> bool { return parameters.getNumOutputChannels() > 0; },
        nullptr, nullptr, -1
    });

    // Step 3: Set Reverb Channels
    gettingStartedWizard->addStep({
        0,
        "wizard.steps.reverbChannels.title",
        "wizard.steps.reverbChannels.description",
        [this, tabBoundsToMain]() -> juce::Rectangle<int> {
            return tabBoundsToMain(systemConfigTab, systemConfigTab->getReverbChannelsBounds());
        },
        nullptr, nullptr, nullptr, -1
    });

    // Step 4: Set Stage Shape & Size
    gettingStartedWizard->addStep({
        0,
        "wizard.steps.stageConfig.title",
        "wizard.steps.stageConfig.description",
        [this, tabBoundsToMain]() -> juce::Rectangle<int> {
            return tabBoundsToMain(systemConfigTab, systemConfigTab->getStageSectionBounds());
        },
        nullptr, nullptr, nullptr, -1
    });

    // Step 5: Set Origin Point
    gettingStartedWizard->addStep({
        0,
        "wizard.steps.originPoint.title",
        "wizard.steps.originPoint.description",
        [this, tabBoundsToMain]() -> juce::Rectangle<int> {
            return tabBoundsToMain(systemConfigTab, systemConfigTab->getOriginSectionBounds());
        },
        nullptr, nullptr, nullptr, -1
    });

    // Step 6: Open Audio Interface (spotlight on button, Skip to bypass)
    gettingStartedWizard->addStep({
        0,
        "wizard.steps.audioInterface.title",
        "wizard.steps.audioInterface.description",
        [this, tabBoundsToMain]() -> juce::Rectangle<int> {
            return tabBoundsToMain(systemConfigTab, systemConfigTab->getAudioPatchingButtonBounds());
        },
        [this]() -> bool { return deviceManager.getCurrentAudioDevice() != nullptr; },
        nullptr, nullptr, -1
    });

    // Step 7: Configure Audio Interface (card ON external window, no skip)
    {
        WizardStep step;
        step.tabIndex = -1; // stay on current tab
        step.titleKey = "wizard.steps.audioDevice.title";
        step.descriptionKey = "wizard.steps.audioDevice.description";
        step.getSpotlightBounds = []() -> juce::Rectangle<int> { return {}; };
        step.isComplete = [this]() -> bool { return deviceManager.getCurrentAudioDevice() != nullptr; };
        step.onEnter = [this]() {
            openAudioInterfaceWindow();
            if (audioInterfaceWindow)
            {
                audioInterfaceWindow->onWindowClosed = [this]() {
                    if (gettingStartedWizard && gettingStartedWizard->isActive()
                        && gettingStartedWizard->getCurrentStepIndex() == 7)
                    {
                        gettingStartedWizard->nextStep();
                    }
                };
            }
        };
        step.getExternalWindowContent = [this]() -> juce::Component* {
            return audioInterfaceWindow ? audioInterfaceWindow->getContentComp() : nullptr;
        };
        step.onExit = [this]() {
            if (audioInterfaceWindow && audioInterfaceWindow->isVisible())
                audioInterfaceWindow->setVisible(false);
        };
        gettingStartedWizard->addStep(std::move(step));
    }

    // Step 8: Open Wizard of OutZ (spotlight on button, Skip to bypass)
    gettingStartedWizard->addStep({
        2, // Outputs tab
        "wizard.steps.wizardOfOutZ.title",
        "wizard.steps.wizardOfOutZ.description",
        [this, tabBoundsToMain]() -> juce::Rectangle<int> {
            return tabBoundsToMain(outputsTab, outputsTab->getArrayHelperButtonBounds());
        },
        nullptr, nullptr, nullptr, -1
    });

    // Step 9: Configure Output Positions (card ON Wizard of OutZ window)
    {
        WizardStep step;
        step.tabIndex = -1;
        step.titleKey = "wizard.steps.configureOutputs.title";
        step.descriptionKey = "wizard.steps.configureOutputs.description";
        step.getSpotlightBounds = []() -> juce::Rectangle<int> { return {}; };
        step.onEnter = [this]() {
            outputsTab->requestOpenArrayHelper();
            if (auto* window = outputsTab->getArrayHelperWindow())
            {
                window->onWindowClosed = [this]() {
                    if (gettingStartedWizard && gettingStartedWizard->isActive()
                        && gettingStartedWizard->getCurrentStepIndex() == 9)
                    {
                        gettingStartedWizard->nextStep();
                    }
                };
            }
        };
        step.getExternalWindowContent = [this]() -> juce::Component* {
            auto* window = outputsTab->getArrayHelperWindow();
            return window ? window->getContentComp() : nullptr;
        };
        step.onExit = [this]() {
            if (auto* window = outputsTab->getArrayHelperWindow())
                if (window->isVisible())
                    window->setVisible(false);
        };
        gettingStartedWizard->addStep(std::move(step));
    }

    // Step 10: Start Processing — spotlight covers full col3 so the card positions to the left
    gettingStartedWizard->addStep({
        0, // SystemConfig tab
        "wizard.steps.startProcessing.title",
        "wizard.steps.startProcessing.description",
        [this, tabBoundsToMain]() -> juce::Rectangle<int> {
            return tabBoundsToMain(systemConfigTab, systemConfigTab->getColumn3Bounds());
        },
        [this]() -> bool { return processingEnabled; },
        nullptr, nullptr, -1
    });

    // Step 11: Explore Inputs — final step on the Map tab, free interaction with fade-out
    {
        WizardStep exploreStep;
        exploreStep.tabIndex = 6; // Map tab
        exploreStep.titleKey = "wizard.steps.exploreInputs.title";
        exploreStep.descriptionKey = "wizard.steps.exploreInputs.description";
        exploreStep.getSpotlightBounds = nullptr;
        exploreStep.isComplete = nullptr;
        exploreStep.onEnter = nullptr;
        exploreStep.getExternalWindowContent = nullptr;
        exploreStep.skipToStepIndex = -1;
        exploreStep.freeInteraction = true;
        gettingStartedWizard->addStep(std::move(exploreStep));
    }

    gettingStartedWizard->startFromStep(lastWizardStepIndex);
}

void MainComponent::closeGettingStartedWizard()
{
    if (gettingStartedWizard)
        gettingStartedWizard->close();
}

void MainComponent::showUpdateBanner (const juce::String& version, const juce::String& url)
{
    if (updateBanner != nullptr)
    {
        updateBanner->showUpdate (version, url);
        resized();
    }

    // Also surface the update on the System Config tab next to the version label,
    // because the top banner is easy to miss.
    if (systemConfigTab != nullptr)
        systemConfigTab->setUpdateAvailable (version, url);
}

//==============================================================================
void MainComponent::setupSharedInputFeed (int blockSize, double sampleRate)
{
    // Only meaningful once the engine is started; the audio callback gates its
    // use of these buffers on audioEngineStarted/processingEnabled.
    if (! audioEngineStarted)
        return;

    // Idempotent: stop any existing feed thread BEFORE its source buffers are
    // freed (it holds raw pointers into sharedInputBuffers), then rebuild
    // everything for the current block size / sample-rate ratio.
    if (reverbFeedThread)
    {
        reverbFeedThread->stopThread (1000);
        reverbFeedThread.reset();
    }

    // The effects driver reads the same rings through raw pointers: joined
    // and freed before they are cleared, re-prepared below once they exist
    if (effectsHost)
        effectsHost->release();

    // Create shared input buffers (used by the reverb feed thread, binaural and
    // the effects engine). Depth: four blocks, eight when effect channels
    // exist. The effects engine refuses a ring under two blocks
    // (EffectsEngineCore::prepare) and detects a lap at capacity minus one
    // block, so eight blocks tolerate a seven-block driver stall where four
    // tolerate three - headroom paid only by sessions that have effects.
    const int ringBlocks = renderSourceMap.numEffectChannels > 0 ? 8 : 4;
    sharedInputBuffers.clear();
    for (int i = 0; i < numRenderSources; ++i)
    {
        auto buf = std::make_unique<SharedInputRingBuffer>();
        buf->setSize (blockSize * ringBlocks);
        sharedInputBuffers.push_back (std::move (buf));
    }

    // Start reverb feed thread (computes reverb feeds off the audio callback)
    if (reverbEngine && calculationEngine)
    {
        int numReverbs = reverbEngine->getNumNodes();
        if (numReverbs > 0 && ! sharedInputBuffers.empty())
        {
            reverbFeedThread = std::make_unique<ReverbFeedThread>();
            // Coordinator first: prepare() creates the send-path worker pool,
            // whose threads join the same audio workgroup.
            reverbFeedThread->setWorkgroupCoordinator (&workgroupCoordinator);
            // All three send matrices, not just the levels. The engine has
            // always computed the delay and HF ones; the send path applies
            // them now instead of only the GUI drawing them.
            reverbFeedThread->prepare (sharedInputBuffers, reverbEngine.get(),
                                       calculationEngine->getInputReverbLevels(),
                                       calculationEngine->getInputReverbDelayTimesMs(),
                                       calculationEngine->getInputReverbHFAttenuationDb(),
                                       calculationEngine->getNumReverbs(),
                                       numRenderSources, numReverbs,
                                       blockSize, reverbSRRatio, sampleRate);
            reverbFeedThread->startRealtimeThread (juce::Thread::RealtimeOptions{}
                                                       .withApproximateAudioProcessingTime (blockSize, sampleRate));
        }
    }

    // Wire binaural processor to shared input buffers (reads directly, no push needed)
    if (binauralProcessor && ! sharedInputBuffers.empty())
        binauralProcessor->setSharedInputBuffers (sharedInputBuffers);

    // Start the effects engine: the chains run off the audio callback on their
    // own realtime driver, fed from the rings just rebuilt, which is why it is
    // prepared here and nowhere else. Only when effect channels exist - an
    // empty session allocates nothing and runs no thread. prepare() joins any
    // previous driver and does not restart it: the priority is the owner's
    // choice, exactly as for the reverb feed thread above.
    if (effectsHost && calculationEngine)
    {
        const int numEffects = renderSourceMap.numEffectChannels;
        if (numEffects > 0 && renderSourceMap.firstEffectSlot >= 0 && ! sharedInputBuffers.empty())
        {
            if (effectsHost->prepare (sampleRate, blockSize, numRenderSources, renderSourceMap.firstEffectSlot,
                                      numEffects, sharedInputBuffers))
            {
                effectsHost->setFeedMatrices (*calculationEngine, numRenderSources);
                effectsHost->startRealtimeThread (juce::Thread::RealtimeOptions{}
                                                      .withApproximateAudioProcessingTime (blockSize, sampleRate));
            }
        }
    }

    // (Re)wire the GPU pipeline strip's reverb telemetry sources: the feed
    // thread was just rebuilt (or dropped when numReverbs == 0), so refresh
    // the metering manager's raw pointers. Feed budget = one device block.
    if (levelMeteringManager)
        levelMeteringManager->setReverbSources (
            reverbEngine.get(), reverbFeedThread.get(),
            sampleRate > 0.0 ? (float) (1000.0 * blockSize / sampleRate) : 0.0f);

    // The same for the effects driver, whose budget is the same device block.
    // getCore() is null unless the host is prepared, which is exactly when the
    // manager should report silence.
    if (levelMeteringManager)
        levelMeteringManager->setEffectsSource (
            effectsHost != nullptr ? effectsHost->getCore() : nullptr,
            sampleRate > 0.0 ? (float) (1000.0 * blockSize / sampleRate) : 0.0f);
}

void MainComponent::startAudioEngine()
{
    if (audioEngineStarted)
        return;

    attachAudioCallbacksIfNeeded();

    // Get current audio settings
    auto* device = deviceManager.getCurrentAudioDevice();
    if (device == nullptr)
    {
        WFSLogger::getInstance().logWarning ("startAudioEngine: no audio device available");
        DBG("ERROR: No audio device available!");
        return;
    }

    double sampleRate = device->getCurrentSampleRate();
    int blockSize = device->getCurrentBufferSizeSamples();

    // Publish the audio device's realtime workgroup so the DSP worker threads join it
    // (macOS; no-op elsewhere), and make sure the algorithms hand it to their processors.
    workgroupCoordinator.set (device->getWorkgroup());
    inputAlgorithm.setWorkgroupCoordinator (&workgroupCoordinator);
    outputAlgorithm.setWorkgroupCoordinator (&workgroupCoordinator);

    WFSLogger::getInstance().logInfo ("Starting audio engine: " + device->getName()
                                      + " @ " + juce::String (sampleRate) + " Hz"
                                      + ", buffer " + juce::String (blockSize)
                                      + ", " + juce::String (numRenderSources) + " in / "
                                      + juce::String (numOutputChannels) + " out");

    DBG("startAudioEngine: numRenderSources=" + juce::String(numRenderSources) +
        " numOutputChannels=" + juce::String(numOutputChannels) +
        " sampleRate=" + juce::String(sampleRate) + " blockSize=" + juce::String(blockSize));

    // Pick up an algorithm change made while processing was stopped
    {
        int algoId = (int) parameters.getConfigParam("ProcessingAlgorithm");
        if (algoId == 1) currentAlgorithm = ProcessingAlgorithm::InputBuffer;
        else if (algoId == 2) currentAlgorithm = ProcessingAlgorithm::OutputBuffer;
#if WFS_GPU_NATIVE
        else if (algoId == 3) currentAlgorithm = ProcessingAlgorithm::NativeGpuWfs;
        else if (algoId == 4) currentAlgorithm = ProcessingAlgorithm::NativeGpuOutputBuffer;

        currentDeviceId = parameters.getConfigParam("ProcessingAlgorithmDevice").toString().toStdString();
        if (currentDeviceId.empty())
            currentDeviceId = (algoId >= 3) ? GpuDeviceManager::instance().firstGpuId() : std::string("cpu");
#endif
    }

    bool prepared = false;
#if WFS_GPU_NATIVE
    DBG("startAudioEngine: algorithm=" + juce::String(currentAlgorithm == ProcessingAlgorithm::InputBuffer ? "InputBuffer" :
        currentAlgorithm == ProcessingAlgorithm::OutputBuffer ? "OutputBuffer" :
        currentAlgorithm == ProcessingAlgorithm::NativeGpuWfs ? "NativeGpuWfs" : "NativeGpuOutputBuffer"));
#else
    // GPU enum values only exist under WFS_GPU_NATIVE; non-GPU builds (e.g. Linux
    // without CUDA) only ever have InputBuffer / OutputBuffer.
    DBG("startAudioEngine: algorithm=" + juce::String(currentAlgorithm == ProcessingAlgorithm::InputBuffer ? "InputBuffer" : "OutputBuffer"));
#endif

    if (currentAlgorithm == ProcessingAlgorithm::InputBuffer)
    {
        inputAlgorithm.prepare(numRenderSources, numOutputChannels,
                              sampleRate, blockSize,
                              delayTimesMs.data(), levels.data(),
                              processingEnabled,
                              hfAttenuation.data(),
                              frDelayTimesMs.data(),
                              frLevels.data(),
                              frHFAttenuation.data());
        prepared = true;
    }
    else if (currentAlgorithm == ProcessingAlgorithm::OutputBuffer)
    {
        outputAlgorithm.prepare(numRenderSources, numOutputChannels,
                               sampleRate, blockSize,
                               delayTimesMs.data(), levels.data(),
                               processingEnabled,
                               hfAttenuation.data(),
                               frDelayTimesMs.data(),
                               frLevels.data(),
                               frHFAttenuation.data());
        prepared = true;
    }
#if WFS_GPU_NATIVE
    else if (currentAlgorithm == ProcessingAlgorithm::NativeGpuWfs)
    {
        int gpuDepth = (int) parameters.getConfigParam ("GpuPipelineDepth");
        if (gpuDepth < WFSParameterDefaults::gpuPipelineDepthMin
            || gpuDepth > WFSParameterDefaults::gpuPipelineDepthMax)
            gpuDepth = WFSParameterDefaults::gpuPipelineDepthDefault;

        prepared = nativeGpuAlgorithm.prepare(numRenderSources, numOutputChannels,
                                              sampleRate, blockSize,
                                              delayTimesMs.data(), levels.data(),
                                              processingEnabled,
                                              hfAttenuation.data(),
                                              frDelayTimesMs.data(),
                                              frLevels.data(),
                                              frHFAttenuation.data(),
                                              gpuDepth, currentDeviceId);
        if (!prepared)
        {
            // GPU init failed: log, inform, fall back to the CPU InputBuffer.
            auto errorMsg = nativeGpuAlgorithm.getLastError();
            WFSLogger::getInstance().logWarning ("Native GPU init failed: " + errorMsg
                                                 + " - falling back to CPU InputBuffer");
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                "GPU",
                "GPU initialization failed:\n" + errorMsg
                + "\n\nFalling back to the CPU InputBuffer algorithm.");
            nativeGpuAlgorithm.clear();

            currentAlgorithm = ProcessingAlgorithm::InputBuffer;
            parameters.setConfigParam("ProcessingAlgorithm", 1);

            inputAlgorithm.prepare(numRenderSources, numOutputChannels,
                                   sampleRate, blockSize,
                                   delayTimesMs.data(), levels.data(),
                                   processingEnabled,
                                   hfAttenuation.data(),
                                   frDelayTimesMs.data(),
                                   frLevels.data(),
                                   frHFAttenuation.data());
            prepared = true;
        }
        else
        {
            WFSLogger::getInstance().logInfo ("Native GPU WFS active: "
                + juce::String (numRenderSources) + " in x "
                + juce::String (numOutputChannels) + " out on "
                + nativeGpuAlgorithm.getDeviceName()
                + ", async pipeline depth " + juce::String (nativeGpuAlgorithm.getPipelineDepthBlocks())
                + " = +" + juce::String (nativeGpuAlgorithm.getPipelineLatencyMs(), 2)
                + " ms, pre-subtracted from WFS delays");
        }
    }
    else // ProcessingAlgorithm::NativeGpuOutputBuffer
    {
        int gpuDepth = (int) parameters.getConfigParam ("GpuPipelineDepth");
        if (gpuDepth < WFSParameterDefaults::gpuPipelineDepthMin
            || gpuDepth > WFSParameterDefaults::gpuPipelineDepthMax)
            gpuDepth = WFSParameterDefaults::gpuPipelineDepthDefault;

        prepared = nativeGpuOutputAlgorithm.prepare(numRenderSources, numOutputChannels,
                                                    sampleRate, blockSize,
                                                    delayTimesMs.data(), levels.data(),
                                                    processingEnabled,
                                                    hfAttenuation.data(),
                                                    frDelayTimesMs.data(),
                                                    frLevels.data(),
                                                    frHFAttenuation.data(),
                                                    gpuDepth, currentDeviceId);
        if (!prepared)
        {
            // GPU init failed: log, inform, fall back to the CPU OutputBuffer
            // (the CPU twin of this scatter algorithm).
            auto errorMsg = nativeGpuOutputAlgorithm.getLastError();
            WFSLogger::getInstance().logWarning ("Native GPU OutputBuffer init failed: " + errorMsg
                                                 + " - falling back to CPU OutputBuffer");
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                "GPU",
                "GPU initialization failed:\n" + errorMsg
                + "\n\nFalling back to the CPU OutputBuffer algorithm.");
            nativeGpuOutputAlgorithm.clear();

            currentAlgorithm = ProcessingAlgorithm::OutputBuffer;
            parameters.setConfigParam("ProcessingAlgorithm", 2);

            outputAlgorithm.prepare(numRenderSources, numOutputChannels,
                                    sampleRate, blockSize,
                                    delayTimesMs.data(), levels.data(),
                                    processingEnabled,
                                    hfAttenuation.data(),
                                    frDelayTimesMs.data(),
                                    frLevels.data(),
                                    frHFAttenuation.data());
            prepared = true;
        }
        else
        {
            WFSLogger::getInstance().logInfo ("Native GPU OutputBuffer active: "
                + juce::String (numRenderSources) + " in x "
                + juce::String (numOutputChannels) + " out on "
                + nativeGpuOutputAlgorithm.getDeviceName()
                + ", async pipeline depth " + juce::String (nativeGpuOutputAlgorithm.getPipelineDepthBlocks())
                + " = +" + juce::String (nativeGpuOutputAlgorithm.getPipelineLatencyMs(), 2)
                + " ms, pre-subtracted from WFS delays");
        }
    }
#endif

    audioEngineStarted = prepared;
    if (!audioEngineStarted && processingEnabled)
    {
        processingEnabled = false;
        processingToggle.setToggleState(false, juce::dontSendNotification);
    }

    // Build shared input buffers, reverb feed thread, and binaural wiring.
    // Extracted into setupSharedInputFeed() so prepareToPlay() can rebuild them
    // after a device restart (see the call there for the rationale).
    setupSharedInputFeed (blockSize, sampleRate);

    // Start reverb engine thread (may have been stopped by channel count change)
    if (audioEngineStarted && reverbEngine)
        reverbEngine->startProcessing();
}

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    WFSLogger::getInstance().logInfo ("prepareToPlay: sampleRate=" + juce::String (sampleRate)
                                      + " bufferSize=" + juce::String (samplesPerBlockExpected));

    currentDeviceSampleRate.store (sampleRate > 0.0 ? sampleRate : 48000.0,
                                   std::memory_order_relaxed);

    // Preallocate the patched input buffer here, off the audio thread. The
    // conditional setSize in applyInputPatch() remains only as a shape-change
    // safety net (avoidReallocating=true) and should never fire in steady state.
    if (samplesPerBlockExpected > 0)
    {
        patchedInputBuffer.setSize (juce::jmax (1, numRenderSources),
                                    samplesPerBlockExpected, false, false, true);
        stereoRawBuffer.setSize (2 * StereoChannelManager::kMaxStereoChannels,
                                 samplesPerBlockExpected, false, false, true);
        stereoRawBuffer.clear();
    }

    // Stereo decomposition backends: prepared for every ordinal so a channel
    // configured stereo while stopped needs no re-prepare on start (the manager
    // itself is constructed with the component, so the Map's image read-back
    // does not wait on a device)
    if (stereoChannelManager != nullptr && sampleRate > 0.0 && samplesPerBlockExpected > 0)
        stereoChannelManager->prepare (sampleRate, samplesPerBlockExpected);

    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // The device's realtime workgroup can change with the device/sample rate.
    // Republish it so the DSP worker threads rejoin (macOS; no-op elsewhere).
    if (auto* dev = deviceManager.getCurrentAudioDevice())
        workgroupCoordinator.set (dev->getWorkgroup());

    // If audio engine was already started, update processor settings
    if (audioEngineStarted)
    {
        if (currentAlgorithm == ProcessingAlgorithm::InputBuffer)
        {
            inputAlgorithm.reprepare(sampleRate, samplesPerBlockExpected, processingEnabled);
        }
        else if (currentAlgorithm == ProcessingAlgorithm::OutputBuffer)
        {
            outputAlgorithm.reprepare(sampleRate, samplesPerBlockExpected, processingEnabled);
        }
#if WFS_GPU_NATIVE
        else if (currentAlgorithm == ProcessingAlgorithm::NativeGpuWfs)
        {
            // Device/buffer change: full re-prepare (backend is sized per block)
            int gpuDepth = (int) parameters.getConfigParam ("GpuPipelineDepth");
            if (gpuDepth < WFSParameterDefaults::gpuPipelineDepthMin
                || gpuDepth > WFSParameterDefaults::gpuPipelineDepthMax)
                gpuDepth = WFSParameterDefaults::gpuPipelineDepthDefault;

            nativeGpuAlgorithm.prepare(numRenderSources, numOutputChannels,
                                       sampleRate, samplesPerBlockExpected,
                                       delayTimesMs.data(), levels.data(),
                                       processingEnabled,
                                       hfAttenuation.data(),
                                       frDelayTimesMs.data(),
                                       frLevels.data(),
                                       frHFAttenuation.data(),
                                       gpuDepth, currentDeviceId);
        }
        else if (currentAlgorithm == ProcessingAlgorithm::NativeGpuOutputBuffer)
        {
            // Device/buffer change: full re-prepare (backend is sized per block)
            int gpuDepth = (int) parameters.getConfigParam ("GpuPipelineDepth");
            if (gpuDepth < WFSParameterDefaults::gpuPipelineDepthMin
                || gpuDepth > WFSParameterDefaults::gpuPipelineDepthMax)
                gpuDepth = WFSParameterDefaults::gpuPipelineDepthDefault;

            nativeGpuOutputAlgorithm.prepare(numRenderSources, numOutputChannels,
                                             sampleRate, samplesPerBlockExpected,
                                             delayTimesMs.data(), levels.data(),
                                             processingEnabled,
                                             hfAttenuation.data(),
                                             frDelayTimesMs.data(),
                                             frLevels.data(),
                                             frHFAttenuation.data(),
                                             gpuDepth, currentDeviceId);
        }
#endif
    }

    // Prepare test signal generator
    if (testSignalGenerator)
    {
        testSignalGenerator->prepare(sampleRate, samplesPerBlockExpected);
    }

    // Prepare binaural processor. Stop the worker first: on device/sample-rate/
    // buffer changes JUCE calls releaseResources -> prepareToPlay, and the worker
    // must not run while prepareToPlay rebuilds its buffers (see the jassert there).
    if (binauralProcessor)
    {
        binauralProcessor->stopProcessing();
        binauralProcessor->prepareToPlay(sampleRate, samplesPerBlockExpected, numRenderSources);
        binauralProcessor->startProcessing();

        // Sample rate / block size may have changed: the cooked SOFA set is
        // stale, force the 50 Hz poll to reload/re-cook for the new format.
        lastPushedSofaKey.clear();
    }

    // Prepare reverb engine
    if (reverbEngine)
    {
        int numReverbs = parameters.getNumReverbChannels();

        // Run reverb at 48kHz when system SR is an integer multiple
        double reverbSR = sampleRate;
        reverbSRRatio = 1;
        if (sampleRate > 48000.0)
        {
            int ratio = static_cast<int> (sampleRate / 48000.0);
            if (std::abs (sampleRate - 48000.0 * ratio) < 1.0)
            {
                reverbSRRatio = ratio;
                reverbSR = 48000.0;
            }
        }

        int reverbBlockSize = samplesPerBlockExpected / reverbSRRatio;
        reverbEngine->prepareToPlay (reverbSR, reverbBlockSize, numReverbs);

        // Resize reverb feed/return audio buffers
        if (numReverbs > 0)
        {
            reverbFeedBuffer.setSize (numReverbs, samplesPerBlockExpected);
            reverbReturnBuffer.setSize (numReverbs, samplesPerBlockExpected);

            if (reverbSRRatio > 1)
            {
                int dsBlockSize = samplesPerBlockExpected / reverbSRRatio;
                reverbDownsampleBuf.setSize (numReverbs, dsBlockSize);
                reverbUpsampleBuf.setSize (numReverbs, dsBlockSize);
            }
        }

        // Return distribution runs at DEVICE rate, after the SR-ratio upsample,
        // so its delay lines are sized from sampleRate rather than reverbSR.
        // The matrix stride it will be handed is the engine's max reverb count,
        // but the cell arrays only need the live channel count.
        reverbReturnProcessor.prepare (sampleRate, samplesPerBlockExpected,
                                       numReverbs, parameters.getNumOutputChannels(),
                                       &workgroupCoordinator);

        // (Re)build the binaural monitor's reverb-return taps and hand them
        // to the worker (spinlock+generation publish, safe while running).
        sharedReverbReturnBuffers.clear();
        for (int i = 0; i < numReverbs; ++i)
        {
            auto ring = std::make_unique<SharedInputRingBuffer>();
            ring->setSize (samplesPerBlockExpected * 4);
            sharedReverbReturnBuffers.push_back (std::move (ring));
        }
        if (binauralProcessor)
            binauralProcessor->setSharedReverbBuffers (sharedReverbReturnBuffers);

        reverbEngine->startProcessing();

#if REVERB_DIAGNOSTICS
        reverbDiagReporter = std::make_unique<ReverbDiagnosticReporter> (
            reverbEngine->getDiagnostics());
        reverbDiagReporter->startReporting (1000);
#endif
    }

    // Rebuild the reverb feed path after a device (re)start. JUCE calls
    // releaseResources() -> prepareToPlay() on any device / sample-rate / buffer
    // change; releaseResources() destroyed reverbFeedThread + sharedInputBuffers,
    // and startAudioEngine() will not re-run (audioEngineStarted is still true),
    // so without this the restarted reverb engine has no feed and pullNodeOutput()
    // underruns every block -> a permanently stuck "Reverb dropout detected" banner.
    // (Also re-wires the binaural monitor, which reads the same shared buffers.)
    if (audioEngineStarted)
        setupSharedInputFeed (samplesPerBlockExpected, sampleRate);

    // Prepare sampler manager
    if (samplerManager == nullptr)
        samplerManager = std::make_unique<SamplerManager>();
    samplerManager->prepare (sampleRate, samplesPerBlockExpected, numInputChannels);

    // Activate sampler channels that are already enabled and load their data
    for (int ch = 0; ch < numInputChannels; ++ch)
    {
        auto val = parameters.getInputParam (ch, "inputSamplerActive");
        bool active = ! val.isVoid() && static_cast<int> (val) != 0;
        samplerManager->setChannelActive (ch, active);
        if (active)
        {
            auto samplerTree = parameters.getValueTreeState().getInputSamplerSection (ch);
            if (samplerTree.isValid())
            {
                auto samplesFolder = parameters.getFileManager().getSamplesFolder();
                samplerManager->loadChannelCells (ch, samplerTree, samplesFolder);
                samplerManager->loadChannelSetFromTree (ch, samplerTree, 0);
                applySamplerSetPosition (ch, samplerTree, 0);
            }
        }
    }

    // Load audio patch matrices
    loadAudioPatches();

    // Separate time constants: delays need slower smoothing to avoid Doppler artifacts
    // on fast position changes; levels can change faster without audible issues.
    double callbackIntervalSec = samplesPerBlockExpected / sampleRate;
    delaySmoothingFactor = static_cast<float>(1.0 - std::exp(-callbackIntervalSec / 0.100));  // 100ms
    levelSmoothingFactor = static_cast<float>(1.0 - std::exp(-callbackIntervalSec / 0.050));  // 50ms

    // Initialize master level smoother
    masterLevelGain.reset(sampleRate, 0.05);  // 50ms ramp
    masterLevelGain.setCurrentAndTargetValue(masterLevelGainTarget.load(std::memory_order_relaxed));

    // Per-output attenuation smoothers
    resizeOutputAttenuation(numOutputChannels, sampleRate);

    // Per-reverb return attenuation smoothers
    resizeReverbAttenuation(parameters.getNumReverbChannels(), sampleRate);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Xrun detection (lock-free, deferred logging)
    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        int xruns = device->getXRunCount();
        if (xruns > lastXRunCount)
        {
            WFSLogger::getInstance().logFromAudioThread ("xrun detected (count: " + juce::String (xruns) + ")");
            lastXRunCount = xruns;
        }
    }

    // Hardware-input signal-presence meter for the Input Patch header tint.
    // Runs unconditionally (before the DSP gate) so users can verify input
    // wiring even when WFS and the binaural renderer are both stopped.
    if (levelMeteringManager != nullptr && bufferToFill.buffer != nullptr)
    {
        const int hwChannels = bufferToFill.buffer->getNumChannels();
        const int activeInputs = juce::jmin (hwChannels,
                                             LevelMeteringManager::MaxHardwareInputs);
        const double sr = currentDeviceSampleRate.load (std::memory_order_relaxed);
        const float decay = LevelMeteringManager::blockDecayCoef (
            bufferToFill.numSamples, sr, LevelMeteringManager::kInputMeterTauSeconds);

        for (int ch = 0; ch < activeInputs; ++ch)
        {
            const float mag = bufferToFill.buffer->getMagnitude (ch,
                                                                 bufferToFill.startSample,
                                                                 bufferToFill.numSamples);
            levelMeteringManager->pushHardwareInputBlockPeak (ch, mag, decay);
        }
        levelMeteringManager->markHardwareMeterBlock();
    }

    // Process WFS audio if engine is started AND processing is enabled
    if (audioEngineStarted && processingEnabled)
    {
        // Apply input patching: hardware channels → WFS channels (single copy into patchedInputBuffer)
        applyInputPatch(bufferToFill);

        // Overwrite input channels where sampler is active (writes to patchedInputBuffer)
        if (samplerManager != nullptr && samplerManager->hasAnyActiveChannel())
        {
            for (int ch = 0; ch < numInputChannels; ++ch)
            {
                if (samplerManager->isChannelActive (ch)
                    && ch < patchedInputBuffer.getNumChannels())
                {
                    samplerManager->processChannel (ch, patchedInputBuffer,
                                                    bufferToFill.startSample,
                                                    bufferToFill.numSamples);
                }
            }
        }

        // Effect returns: pop every return into its render-source row - after
        // the input patch (which cleared the rows) and before anything reads
        // them: the meters, the shared rings (through which the engine's own
        // effect-to-effect feed sees block n) and the renderers. Silence when
        // the engine is not ready or a return is late; never blocks.
        if (effectsHost != nullptr && renderSourceMap.firstEffectSlot >= 0)
            effectsHost->pullReturns (patchedInputBuffer, bufferToFill.startSample, bufferToFill.numSamples,
                                      renderSourceMap.numEffectChannels);

        // Apply AutomOtion return fade gain (50ms fade out/in during position snap-back).
        // For a stereo-pair row the gain goes onto BOTH raw channels before
        // decomposition — a per-channel linear gain commutes with the
        // decomposition by the reconstruction identity, and the channel's
        // patchedInputBuffer slot has not been written yet at this point.
        if (automOtionProcessor != nullptr)
        {
            int stereoOrdinal = 0;
            for (int ch = 0; ch < numInputChannels && ch < patchedInputBuffer.getNumChannels(); ++ch)
            {
                const bool stereoRow = ch < (int) renderSourceMap.firstDerivedSlot.size()
                                    && renderSourceMap.firstDerivedSlot[static_cast<size_t> (ch)] >= 0;
                float gain = automOtionProcessor->getReturnGain (ch);
                if (stereoRow)
                {
                    const int rawL = 2 * stereoOrdinal;
                    ++stereoOrdinal;
                    if (gain < 1.0f && rawL + 1 < stereoRawBuffer.getNumChannels())
                    {
                        stereoRawBuffer.applyGain (rawL, bufferToFill.startSample,
                                                   bufferToFill.numSamples, gain);
                        stereoRawBuffer.applyGain (rawL + 1, bufferToFill.startSample,
                                                   bufferToFill.numSamples, gain);
                    }
                }
                else if (gain < 1.0f)
                {
                    patchedInputBuffer.applyGain (ch, bufferToFill.startSample,
                                                   bufferToFill.numSamples, gain);
                }
            }
        }

        // The same fade for an effect return that is snapping home. The row
        // was popped a few lines up and nothing has read it yet, so one gain on
        // the render-source row covers the renderers, the meters and the ring
        // the engine's own effect-to-effect feed reads.
        if (effectOtomoProcessor != nullptr && renderSourceMap.firstEffectSlot >= 0)
        {
            for (int fx = 0; fx < renderSourceMap.numEffectChannels; ++fx)
            {
                const int row = renderSourceMap.firstEffectSlot + fx;
                if (row >= patchedInputBuffer.getNumChannels())
                    break;

                const float gain = effectOtomoProcessor->getReturnGain (fx);
                if (gain < 1.0f)
                    patchedInputBuffer.applyGain (row, bufferToFill.startSample,
                                                  bufferToFill.numSamples, gain);
            }
        }

        // Stereo decomposition: raw L/R → the channels' six render-source
        // slots, before anything downstream reads patchedInputBuffer
        runStereoDecompositionStage (bufferToFill.startSample, bufferToFill.numSamples);

        // Input meters, measured on the finished render sources (see the
        // binaural-only branch for the twin call — these are the only two).
        meterRenderSourceInputs (bufferToFill.startSample, bufferToFill.numSamples);

        // Write patched input to shared buffers + notify consumers (only when needed)
        {
            bool needSharedBuffers = (reverbFeedThread != nullptr)
                                  || (binauralProcessor && binauralProcessor->isEnabled())
                                  || (effectsHost != nullptr && effectsHost->isReady());

            if (needSharedBuffers && !sharedInputBuffers.empty())
            {
                int safeInputCount = juce::jmin(numRenderSources, patchedInputBuffer.getNumChannels(), (int)sharedInputBuffers.size());
                for (int ch = 0; ch < safeInputCount; ++ch)
                {
                    if (sharedInputBuffers[ch] == nullptr)
                        continue;
                    auto* inputData = patchedInputBuffer.getReadPointer(ch, bufferToFill.startSample);
                    sharedInputBuffers[ch]->write(inputData, bufferToFill.numSamples);
                }

                if (binauralProcessor && binauralProcessor->isEnabled())
                    binauralProcessor->notifyInputAvailable();

                if (reverbFeedThread)
                {
                    reverbFeedThread->setMuted(muteReverbPre.load(std::memory_order_relaxed));
                    reverbFeedThread->notifyInputAvailable();
                }

                // The engine's batch order: every row of block n is in its
                // ring (the popped returns included) before the driver wakes
                if (effectsHost != nullptr && effectsHost->isReady())
                {
                    effectsHost->setMuted (muteEffectsPre.load (std::memory_order_relaxed));
                    effectsHost->notifyInputAvailable();
                }
            }
        }

        // Parameter smoothing (runs on ASIO thread, immune to message-pump
        // throttling when the window is minimized)
        if (processingEnabled)
        {
            int matrixSize = numRenderSources * numOutputChannels;
            for (int i = 0; i < matrixSize; ++i)
            {
                delayTimesMs[i] += (targetDelayTimesMs[i] - delayTimesMs[i]) * delaySmoothingFactor;
                levels[i] += (targetLevels[i] - levels[i]) * levelSmoothingFactor;
                frLevels[i] += (targetFRLevels[i] - frLevels[i]) * levelSmoothingFactor;
            }
        }

        // Count reverb nodes for return mixing (feed computation now on ReverbFeedThread)
        int numReverbs = 0;
        if (reverbEngine && reverbEngine->isActive() && calculationEngine)
        {
            numReverbs = reverbEngine->getNumNodes();

            int bufferChannels = reverbReturnBuffer.getNumChannels();
            if (reverbSRRatio > 1)
                bufferChannels = juce::jmin(bufferChannels, reverbUpsampleBuf.getNumChannels());
            if (numReverbs > bufferChannels)
                numReverbs = bufferChannels;
        }

        // Prepare WFS output buffer (algorithm writes here, then single remap to HW)
        int numSamples = bufferToFill.numSamples;
        int startSample = bufferToFill.startSample;

        if (wfsOutputBuffer.getNumChannels() < numOutputChannels ||
            wfsOutputBuffer.getNumSamples() < numSamples)
        {
            wfsOutputBuffer.setSize(numOutputChannels, numSamples, false, false, true);
        }

        // Create AudioSourceChannelInfo wrapping wfsOutputBuffer for algorithm use
        juce::AudioSourceChannelInfo wfsOut(&wfsOutputBuffer, startSample, numSamples);

        // Process WFS audio — algorithms read from patchedInputBuffer, write to wfsOutputBuffer
        if (currentAlgorithm == ProcessingAlgorithm::InputBuffer)
        {
            inputAlgorithm.processBlock(wfsOut, patchedInputBuffer, numRenderSources, numOutputChannels);
        }
        else if (currentAlgorithm == ProcessingAlgorithm::OutputBuffer)
        {
            outputAlgorithm.processBlock(wfsOut, patchedInputBuffer, numRenderSources, numOutputChannels);
        }
#if WFS_GPU_NATIVE
        else if (currentAlgorithm == ProcessingAlgorithm::NativeGpuWfs)
        {
            nativeGpuAlgorithm.processBlock(wfsOut, patchedInputBuffer, numRenderSources, numOutputChannels);
        }
        else // ProcessingAlgorithm::NativeGpuOutputBuffer
        {
            nativeGpuOutputAlgorithm.processBlock(wfsOut, patchedInputBuffer, numRenderSources, numOutputChannels);
        }
#endif

        // Mix reverb returns into WFS output (after WFS processing wrote speaker data)
        if (numReverbs > 0 && reverbEngine && calculationEngine)
        {
            // Solo Reverbs: clear direct sound so only reverb returns are heard
            if (soloReverbs.load (std::memory_order_relaxed))
            {
                for (int outIdx = 0; outIdx < numOutputChannels; ++outIdx)
                    wfsOutputBuffer.clear (outIdx, startSample, numSamples);
            }

            // Pull wet reverb output and mix into WFS outputs
            // Index: [reverbIndex * calcOutputStride + outputIndex]
            // All three return matrices: a node return is an ambient source at
            // its return position, so it reaches each speaker with a real
            // propagation delay and real air absorption, not just a level.
            const float* reverbOutputLevelsPtr = calculationEngine->getReverbOutputLevels();
            const float* reverbOutputDelaysPtr = calculationEngine->getReverbOutputDelayTimesMs();
            const float* reverbOutputHFPtr = calculationEngine->getReverbOutputHFAttenuationDb();
            const int calcOutputStride = calculationEngine->getNumOutputs();
            bool isPostMuted = muteReverbPost.load (std::memory_order_relaxed);

            int reverbPullSamples = numSamples / reverbSRRatio;

            for (int revIdx = 0; revIdx < numReverbs; ++revIdx)
            {
                float* returnData = reverbReturnBuffer.getWritePointer(revIdx);

                if (reverbSRRatio > 1)
                {
                    // Pull downsampled output and upsample via linear interpolation
                    float* dsData = reverbUpsampleBuf.getWritePointer (revIdx);
                    reverbEngine->pullNodeOutput (revIdx, dsData, reverbPullSamples);

                    for (int i = 0; i < reverbPullSamples; ++i)
                    {
                        float v0 = dsData[i];
                        float v1 = (i + 1 < reverbPullSamples) ? dsData[i + 1] : v0;
                        for (int j = 0; j < reverbSRRatio; ++j)
                        {
                            float t = static_cast<float> (j) / static_cast<float> (reverbSRRatio);
                            returnData[i * reverbSRRatio + j] = v0 + (v1 - v0) * t;
                        }
                    }
                }
                else
                {
                    reverbEngine->pullNodeOutput (revIdx, returnData, numSamples);
                }

                // Apply per-reverb return attenuation (reverbAttenuation) in-place on the
                // wet signal, so the reverb engine runs at full level but its contribution
                // to the mix is attenuated.
                if (revIdx < reverbAttenuationTargetsCount
                    && revIdx < static_cast<int>(reverbAttenuationGains.size()))
                {
                    auto& sv = reverbAttenuationGains[static_cast<size_t>(revIdx)];
                    sv.setTargetValue(reverbAttenuationTargets[revIdx].load(std::memory_order_relaxed));

                    if (sv.isSmoothing())
                    {
                        for (int s = 0; s < numSamples; ++s)
                            returnData[s] *= sv.getNextValue();
                    }
                    else
                    {
                        float gain = sv.getCurrentValue();
                        if (gain < 1.0f)
                            juce::FloatVectorOperations::multiply(returnData, gain, numSamples);
                    }
                }

                if (! isPostMuted)
                {
                    // Feed the binaural monitor's reverb tap: post node-trim,
                    // pre speaker-matrix, device rate. Multi-consumer ring —
                    // never the SPSC engine rings (those are destructive).
                    if (revIdx < static_cast<int>(sharedReverbReturnBuffers.size())
                        && sharedReverbReturnBuffers[static_cast<size_t>(revIdx)] != nullptr)
                        sharedReverbReturnBuffers[static_cast<size_t>(revIdx)]->write(returnData, numSamples);

                    // Publish into the return delay lines. The speaker mix runs
                    // once, after every node has been pushed (below).
                    reverbReturnProcessor.pushNodeReturn (revIdx, returnData, numSamples);
                }
            }

            // Distribute every node to the speakers: delay tap + air-absorption
            // shelf + return level, parallelised across output channels.
            if (! isPostMuted)
            {
                reverbReturnProcessor.mixToOutputs (wfsOutputBuffer, startSample, numSamples,
                                                    numReverbs, numOutputChannels,
                                                    reverbOutputLevelsPtr, reverbOutputDelaysPtr,
                                                    reverbOutputHFPtr, calcOutputStride);
            }
            else
            {
                // Muted: keep the delay lines running on silence so lifting the
                // mute cannot tap stale wet audio at a non-zero return delay.
                reverbReturnProcessor.skipBlock (numSamples);
            }
        }

        // Per-output parametric EQ (after reverb-return mix, before attenuation/master gain)
        outputEQProcessor.processBlock(wfsOutputBuffer, startSample, numSamples);

        // Apply per-output attenuation (before master gain, after reverb-return mix)
        {
            const int numCh = juce::jmin((int) outputAttenuationGains.size(),
                                         wfsOutputBuffer.getNumChannels(),
                                         outputAttenuationTargetsCount);
            for (int ch = 0; ch < numCh; ++ch)
            {
                auto& sv = outputAttenuationGains[static_cast<size_t>(ch)];
                sv.setTargetValue(outputAttenuationTargets[ch].load(std::memory_order_relaxed));

                if (sv.isSmoothing())
                {
                    float* data = wfsOutputBuffer.getWritePointer(ch) + startSample;
                    for (int s = 0; s < numSamples; ++s)
                        data[s] *= sv.getNextValue();
                }
                else
                {
                    float gain = sv.getCurrentValue();
                    if (gain < 1.0f)
                        wfsOutputBuffer.applyGain(ch, startSample, numSamples, gain);
                }
            }
        }

        // Apply master level gain to WFS output (before hardware remap)
        {
            masterLevelGain.setTargetValue(masterLevelGainTarget.load(std::memory_order_relaxed));

            if (masterLevelGain.isSmoothing())
            {
                int numCh = wfsOutputBuffer.getNumChannels();
                for (int sample = 0; sample < numSamples; ++sample)
                {
                    float gain = masterLevelGain.getNextValue();
                    for (int ch = 0; ch < numCh; ++ch)
                        wfsOutputBuffer.getWritePointer(ch)[startSample + sample] *= gain;
                }
            }
            else
            {
                float gain = masterLevelGain.getCurrentValue();
                if (gain < 1.0f)
                {
                    int numCh = wfsOutputBuffer.getNumChannels();
                    for (int ch = 0; ch < numCh; ++ch)
                        wfsOutputBuffer.applyGain(ch, startSample, numSamples, gain);
                }
            }
        }

        // Single-pass output remap: WFS channels → hardware channels (no intermediate copy-back)
        applyOutputPatch(bufferToFill, wfsOutputBuffer);

        // Pull binaural output directly to hardware buffer (bypasses WFS→HW patch remap)
        if (binauralProcessor && binauralProcessor->isEnabled() && binauralCalcEngine)
        {
            int binauralCh = binauralCalcEngine->getBinauralOutputChannel();
            int hwChannels = bufferToFill.buffer->getNumChannels();
            if (binauralCh >= 0 && binauralCh + 1 < hwChannels)
            {
                float* leftOut = bufferToFill.buffer->getWritePointer(binauralCh, startSample);
                float* rightOut = bufferToFill.buffer->getWritePointer(binauralCh + 1, startSample);
                binauralProcessor->pullOutput(leftOut, rightOut, numSamples);
            }
        }
    }
    else
    {
        // WFS engine not started — but binaural can run independently
        if (binauralProcessor && binauralProcessor->isEnabled() && binauralCalcEngine)
        {
            int binauralCh = binauralCalcEngine->getBinauralOutputChannel();
            if (binauralCh >= 0)
            {
                // Input patching: hardware → WFS channels
                applyInputPatch(bufferToFill);

                // Sampler injection (for preproduction with sampler)
                if (samplerManager != nullptr && samplerManager->hasAnyActiveChannel())
                {
                    for (int ch = 0; ch < numInputChannels; ++ch)
                    {
                        if (samplerManager->isChannelActive (ch)
                            && ch < patchedInputBuffer.getNumChannels())
                        {
                            samplerManager->processChannel (ch, patchedInputBuffer,
                                                            bufferToFill.startSample,
                                                            bufferToFill.numSamples);
                        }
                    }
                }

                // Stereo decomposition (the binaural-only path renders the
                // same render sources the WFS path would)
                runStereoDecompositionStage (bufferToFill.startSample, bufferToFill.numSamples);

                // ...and meters them the same way. This is the whole point of
                // metering here rather than inside a WFS algorithm: no algorithm
                // runs on this path, so anything that asked one for input levels
                // got silence while audio was plainly flowing.
                meterRenderSourceInputs (bufferToFill.startSample, bufferToFill.numSamples);

                // Push input data to binaural processor from patchedInputBuffer
                int safeInputCount = juce::jmin(numRenderSources, patchedInputBuffer.getNumChannels());
                for (int i = 0; i < safeInputCount; ++i)
                {
                    const float* inputData = patchedInputBuffer.getReadPointer(i, bufferToFill.startSample);
                    binauralProcessor->pushInput(i, inputData, bufferToFill.numSamples);
                }

                // Pull binaural directly to hardware buffer (no WFS→HW patch needed)
                int numSamples = bufferToFill.numSamples;
                int hwChannels = bufferToFill.buffer->getNumChannels();
                bufferToFill.clearActiveBufferRegion();

                if (binauralCh + 1 < hwChannels)
                {
                    float* leftOut = bufferToFill.buffer->getWritePointer(binauralCh, bufferToFill.startSample);
                    float* rightOut = bufferToFill.buffer->getWritePointer(binauralCh + 1, bufferToFill.startSample);
                    binauralProcessor->pullOutput(leftOut, rightOut, numSamples);
                }
            }
            else
            {
                bufferToFill.clearActiveBufferRegion();
            }
        }
        else
        {
            bufferToFill.clearActiveBufferRegion();
        }
    }

    // Inject test signals (works independently of DSP processing for interface testing)
    if (testSignalGenerator && testSignalGenerator->isActive())
    {
        testSignalGenerator->renderNextBlock(*bufferToFill.buffer,
                                             bufferToFill.startSample,
                                             bufferToFill.numSamples);
    }
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // Release resources based on current algorithm
    if (currentAlgorithm == ProcessingAlgorithm::InputBuffer)
    {
        inputAlgorithm.releaseResources();
    }
    else if (currentAlgorithm == ProcessingAlgorithm::OutputBuffer)
    {
        outputAlgorithm.releaseResources();
    }
#if WFS_GPU_NATIVE
    else if (currentAlgorithm == ProcessingAlgorithm::NativeGpuWfs)
    {
        nativeGpuAlgorithm.releaseResources();
    }
    else if (currentAlgorithm == ProcessingAlgorithm::NativeGpuOutputBuffer)
    {
        nativeGpuOutputAlgorithm.releaseResources();
    }
#endif

    // Stop reverb feed thread (drop the metering manager's raw pointer first;
    // re-wired by the next setupSharedInputFeed). The effects host is released
    // just below, so the manager must let go of its core here.
    if (levelMeteringManager)
    {
        levelMeteringManager->setReverbSources(reverbEngine.get(), nullptr, 0.0f);
        levelMeteringManager->setEffectsSource(nullptr, 0.0f);
    }
    if (reverbFeedThread)
    {
        reverbFeedThread->stopThread(1000);
        reverbFeedThread.reset();
    }

    // Same rule for the effects driver: join it before the rings it reads die
    if (effectsHost)
        effectsHost->release();

    // Stop the binaural worker and drop its raw pointers into sharedInputBuffers
    // BEFORE destroying the buffers below — the worker must not outlive what it reads.
    if (binauralProcessor)
    {
        binauralProcessor->stopProcessing();
        binauralProcessor->clearSharedInputBuffers();
    }

    sharedInputBuffers.clear();

    // Release reverb engine
#if REVERB_DIAGNOSTICS
    if (reverbDiagReporter)
    {
        reverbDiagReporter->stopReporting();
        reverbDiagReporter.reset();
    }
#endif
    if (reverbEngine)
        reverbEngine->releaseResources();

    // Return distribution: clear the delay lines and filter state so a restart
    // at a different rate or block size cannot tap the previous session.
    reverbReturnProcessor.reset();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // Thread performance is now displayed in the Level Meter window
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    WfsLookAndFeel::uiScale = static_cast<float>(getHeight()) / 1080.0f;
    tabbedComponent.setTabBarDepth(juce::jmax(25, static_cast<int>(35.0f * WfsLookAndFeel::uiScale)));
    auto bounds = getLocalBounds();
    const int statusBarHeight = juce::jmax(20, static_cast<int>(30.0f * WfsLookAndFeel::uiScale));

    // Status bar at bottom, full width
    statusBar->setBounds(bounds.removeFromBottom(statusBarHeight));

    // Update banner at top (if visible)
    if (updateBanner != nullptr && updateBanner->isVisible())
        updateBanner->setBounds (bounds.removeFromTop (juce::jmax (20, static_cast<int> (30.0f * WfsLookAndFeel::uiScale))));

    // Tabbed component takes remaining space
    tabbedComponent.setBounds(bounds);

    // Update wizard overlay if active
    if (gettingStartedWizard && gettingStartedWizard->isActive())
        gettingStartedWizard->updateLayout();

    // Phase 5c: position the MCP undo toast in the top-right of this
    // component. The overlay sizes itself based on row count and hides
    // when empty.
    if (mcpUndoOverlay != nullptr)
        mcpUndoOverlay->positionInParent (getLocalBounds());
}

//==============================================================================
void MainComponent::saveSettings()
{
    // Get application properties
    juce::PropertiesFile::Options options;
    options.applicationName = "WFS-DIY";
    options.filenameSuffix = ".settings";
    options.osxLibrarySubFolder = "Application Support";
    options.folderName = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                            .getChildFile("WFS-DIY").getFullPathName();

    juce::PropertiesFile props(options);

    // Save WFS processing channel counts (independent of sound card I/O)
    props.setValue("numInputChannels", numInputChannels);
    props.setValue("numOutputChannels", numOutputChannels);

    // Save full audio device state as XML for fast restoration on next startup
    // This allows JUCE to restore the exact device configuration without full enumeration
    if (auto xml = deviceManager.createStateXml())
    {
        props.setValue("audioDeviceState", xml->toString());
    }

    // Also save type/name for logging and fallback purposes
    juce::String currentDeviceType = deviceManager.getCurrentAudioDeviceType();
    if (currentDeviceType.isNotEmpty())
        props.setValue("audioDeviceType", currentDeviceType);

    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        juce::String deviceName = device->getName();
        if (deviceName.isNotEmpty())
            props.setValue("audioDeviceName", deviceName);
    }

    props.saveIfNeeded();

}

void MainComponent::timerCallback()
{
    // MIDI-triggered recall. The MIDI thread only parked a packed (ch<<8)|note;
    // the recall itself (XML read + whole-ValueTree write + handleConfigReloaded)
    // runs here, on the message thread. One slot = latest-wins coalescing at
    // 5 ms, which is far shorter than a recall takes anyway.
    if (midiSnapshotTrigger != nullptr)
    {
        const int key = midiSnapshotTrigger->takePendingRecall();
        if (key >= 0)
        {
            const auto pending = midiSnapshotTrigger->resolve (key);
            if (pending.isNotEmpty())
                recallSnapshotByName (pending, /*fromMidi*/ true);
        }

        // Once a second: a snapshot renamed, copied in or restored outside the
        // app changes which note recalls what. The listing is cheap, and the
        // full rescan only runs when it changed.
        if (++snapshotFolderPollTick >= 200)
        {
            snapshotFolderPollTick = 0;
            if (parameters.getFileManager().getInputSnapshotsFolderSignature() != snapshotFolderSignature)
                refreshMidiSnapshotBindings();
        }
    }

    // Once per second, when asked for (WFS_EFFECTS_TRACE): the effects
    // engine's telemetry - batches, duty, per-effect feed and return peaks,
    // underruns, NaN trips, loop-guard state, chain latency. This is what makes
    // an audio check readable from the session log with no GUI.
    // The engine overwrites its per-channel peaks on every batch with no
    // ballistics of their own, so they are sampled on THIS tick rather than the
    // 20 ms metering one, which would step over three batches out of four.
    if (levelMeteringManager != nullptr)
        levelMeteringManager->pollEffectLevels (0.005f);

    if (effectsTraceEnabled && ++effectsTraceTick >= 200) // 5 ms timer
    {
        effectsTraceTick = 0;
        if (effectsHost != nullptr && effectsHost->isPrepared())
        {
            juce::String line = effectsHost->describeTelemetry();

            // What the engine is fed with: the source meters of the first slots
            // and the strongest calc-engine feed cell per live effect, so a
            // silent feed can be told apart from a silent source
            if (levelMeteringManager != nullptr)
            {
                line << "\n  srcPk=";
                const int shown = juce::jmin (numRenderSources, 10);
                for (int s = 0; s < shown; ++s)
                    line << (s > 0 ? " " : "") << "s" << s << ":"
                         << juce::String (levelMeteringManager->getInputLevel (s).peakDb, 1);
            }
            if (calculationEngine != nullptr)
            {
                const float* feedLevels = calculationEngine->getInputEffectLevels();
                const int stride = calculationEngine->getNumEffects();
                line << "\n  feedMax=";
                for (int fx = 0; fx < renderSourceMap.numEffectChannels; ++fx)
                {
                    float best = 0.0f;
                    int bestSlot = -1;
                    for (int s = 0; s < numRenderSources; ++s)
                    {
                        const float v = feedLevels[static_cast<size_t> (s * stride + fx)];
                        if (v > best) { best = v; bestSlot = s; }
                    }
                    line << (fx > 0 ? " " : "") << "fx" << (fx + 1) << ":"
                         << juce::String (best, 4) << "@s" << bestSlot;
                }
            }

            // What the metering manager made of the engine's peaks. The
            // engine's own numbers are above; these have been through the
            // 5 ms poll, the max-hold, the ballistics and the freshness rule,
            // so a live run says whether the tap is wired and tracking rather
            // than only whether the engine is running.
            if (levelMeteringManager != nullptr)
            {
                const auto stats = levelMeteringManager->getEffectsStats();
                line << "\n  meters=" << (stats.live ? "live" : "stale")
                     << " duty=" << juce::String (stats.pct, 1) << "%";
                for (int fx = 0; fx < renderSourceMap.numEffectChannels; ++fx)
                    line << " fx" << (fx + 1) << ":"
                         << juce::String (levelMeteringManager->getEffectLevel (fx).peakDb, 1) << "/"
                         << juce::String (levelMeteringManager->getEffectReturnLevel (fx).peakDb, 1);
            }

            WFSLogger::getInstance().logInfo (line);
        }
    }

#if WFS_GPU_NATIVE
    // Once per second: surface GPU pipeline underruns (silence-filled blocks).
    // They never trip the device xrun counter (the callback doesn't wait on
    // the GPU), so without this log a too-shallow depth would fail silently.
    if (++gpuPipelineStatTick >= 200) // 5 ms timer
    {
        gpuPipelineStatTick = 0;
        // Surface stats from whichever GPU algorithm is live (gather or scatter).
        const bool obActive = (currentAlgorithm == ProcessingAlgorithm::NativeGpuOutputBuffer)
                              && nativeGpuOutputAlgorithm.isReady();
        const uint32_t u = obActive ? nativeGpuOutputAlgorithm.getUnderrunCount()
                         : (nativeGpuAlgorithm.isReady() ? nativeGpuAlgorithm.getUnderrunCount() : 0);
        if (u > gpuUnderrunsLogged)
        {
            const float peakMs = obActive ? nativeGpuOutputAlgorithm.getAndResetPeakGpuExecMs()
                                          : nativeGpuAlgorithm.getAndResetPeakGpuExecMs();
            WFSLogger::getInstance().logInfo ("GPU pipeline underruns: +"
                + juce::String (u - gpuUnderrunsLogged) + " (total " + juce::String (u)
                + "), peak pump " + juce::String (peakMs, 2) + " ms");
            gpuUnderrunsLogged = u;
        }
        else if (u < gpuUnderrunsLogged)
        {
            gpuUnderrunsLogged = u; // pipeline was re-prepared; counter re-baselined
        }

        // Reverb GPU pump runs its OWN pipeline (separate from the direct sound).
        // Log once/sec while a GPU reverb is live AND it's either underrunning or
        // its peak launch is creeping toward the per-block budget (> 60%); stays
        // quiet when comfortably under budget.
        uint32_t ru = 0;
        float rPeak = 0.0f, rBudget = 0.0f;
        juce::String rDevice;
        if (reverbEngine != nullptr && reverbEngine->getReverbGpuPumpStats (ru, rPeak, rBudget, rDevice))
        {
            const bool moreUnderruns = ru > reverbGpuUnderrunsLogged;
            if (moreUnderruns || (rBudget > 0.0f && rPeak > 0.6f * rBudget))
            {
                // Device name included so a reverb overrun can be attributed:
                // the reverb and the direct path may sit on different GPUs.
                WFSLogger::getInstance().logInfo ("Reverb GPU pump: peak "
                    + juce::String (rPeak, 2) + " / " + juce::String (rBudget, 2)
                    + " ms budget, underruns +" + juce::String (moreUnderruns ? ru - reverbGpuUnderrunsLogged : 0)
                    + " (total " + juce::String (ru) + ")"
                    + (rDevice.isNotEmpty() ? " on " + rDevice : juce::String()));
            }
            reverbGpuUnderrunsLogged = ru; // track total (also re-baselines on re-prepare)
        }
    }
#endif
    // Check once whether the window is visible — skip all repaints and
    // visual-only updates when minimized to avoid message-queue congestion
    // that can starve the audio thread's parameter updates.
    const bool windowVisible = isShowing();
    const bool mapVisible = (mapTabWindow != nullptr) ? mapTabWindow->isVisible()
                          : (windowVisible && tabbedComponent.getCurrentTabIndex() == TabIndex::Map);

    // Update master level gain target (message thread → audio thread via atomic)
    {
        float masterLevelDb = (float)parameters.getConfigParam("MasterLevel");
        masterLevelGainTarget.store(
            juce::Decibels::decibelsToGain(masterLevelDb, -92.0f),
            std::memory_order_relaxed);
    }

    // Update per-output attenuation targets (message thread → audio thread via atomics).
    // A muted array's outputs target silence here, so the mute rides the same
    // 50 ms linear ramp as an attenuation change (click-free) and, sitting after
    // the reverb-return mix, silences WFS and reverb alike.
    {
        const auto& arrayMutes = parameters.getValueTreeState().getArrayMutes();
        const bool anyArrayMuted = arrayMutes.anyMuted();
        const int n = juce::jmin(numOutputChannels, outputAttenuationTargetsCount);
        for (int i = 0; i < n; ++i)
        {
            float gain = 0.0f;
            if (! (anyArrayMuted
                   && arrayMutes.isMuted (WFSVar::toInt (parameters.getOutputParam (i, "outputArray")))))
            {
                float dB = (float) parameters.getOutputParam(i, "outputAttenuation");
                gain = juce::Decibels::decibelsToGain(dB, -92.0f);
            }
            outputAttenuationTargets[i].store(gain, std::memory_order_relaxed);
        }
    }

    // Update per-reverb return attenuation targets (message thread → audio thread via atomics)
    {
        const int numReverbs = parameters.getNumReverbChannels();
        const int n = juce::jmin(numReverbs, reverbAttenuationTargetsCount);
        for (int i = 0; i < n; ++i)
        {
            float dB = (float) parameters.getReverbParam(i, "reverbAttenuation");
            reverbAttenuationTargets[i].store(
                juce::Decibels::decibelsToGain(dB, -92.0f),
                std::memory_order_relaxed);
        }
    }

    // Debounced auto-save of audio patch to disk (auto-save variant: won't
    // overwrite a project folder's config that hasn't been loaded this session)
    if (patchSaveCountdown > 0 && --patchSaveCountdown == 0)
    {
        auto& fm = parameters.getFileManager();
        if (fm.hasValidProjectFolder())
            fm.autoSaveSystemConfig();
    }

    // Increment tick counter
    timerTicksSinceLastRandom++;

    // WFS Calculation at ~50Hz (every 4 ticks = 20ms = 50Hz)
    // Recalculate matrix from input/output positions and update target values
    if (calculationEngine != nullptr && (timerTicksSinceLastRandom % 4) == 0)
    {
        // Step OSC-driven parameter ramps (3rd-float "transition time in seconds")
        // BEFORE everything else so the 50 Hz recalculation below sees the latest values.
        if (oscManager != nullptr)
            oscManager->processParameterRamps();

        // Process Input Speed Limiter at 50Hz (BEFORE flip/offset/LFO)
        if (speedLimiter != nullptr)
        {
            auto& vts = parameters.getValueTreeState();

            // Update target positions and speed limits from ValueTree
            for (int i = 0; i < numInputChannels; ++i)
            {
                auto posSection = vts.getInputPositionSection(i);
                float targetX = posSection.getProperty(WFSParameterIDs::inputPositionX, 0.0f);
                float targetY = posSection.getProperty(WFSParameterIDs::inputPositionY, 0.0f);
                float targetZ = posSection.getProperty(WFSParameterIDs::inputPositionZ, 0.0f);

                bool active = static_cast<int>(posSection.getProperty(WFSParameterIDs::inputMaxSpeedActive, 0)) != 0;
                float maxSpeed = posSection.getProperty(WFSParameterIDs::inputMaxSpeed, 1.0f);
                bool pathModeActive = static_cast<int>(posSection.getProperty(WFSParameterIDs::inputPathModeActive, 0)) != 0;

                speedLimiter->setTargetPosition(i, targetX, targetY, targetZ);
                speedLimiter->setSpeedLimit(i, active, maxSpeed);
                speedLimiter->setPathModeEnabled(i, pathModeActive);
            }

            speedLimiter->process(0.02f);  // 20ms delta time (50Hz)

            // Pass speed-limited positions to calculation engine
            for (int i = 0; i < numInputChannels; ++i)
            {
                float x, y, z;
                speedLimiter->getPosition(i, x, y, z);
                calculationEngine->setSpeedLimitedPosition(i, x, y, z);
            }

            // Override positions for channels with active sampler playback
            if (samplerManager != nullptr)
            {
                for (int i = 0; i < numInputChannels; ++i)
                {
                    float sx, sy, sz;
                    if (samplerManager->getPositionOverride (i, sx, sy, sz))
                        calculationEngine->setSamplerCellOffset (i, sx, sy, sz);
                    else
                        calculationEngine->setSamplerCellOffset (i, 0.0f, 0.0f, 0.0f);

                    // Surface audio-thread trigger rejections (empty/invalid cell)
                    if (auto* engine = samplerManager->getEngine (i))
                    {
                        int rejected = engine->fetchAndClearRejectedTriggers();
                        if (rejected > 0)
                            WFSLogger::getInstance().logWarning ("Sampler: input " + juce::String (i + 1)
                                + " trigger rejected - cell has no audio loaded ("
                                + juce::String (rejected) + "x)");
                    }
                }
            }

            // Keep map repainting while speed limiter is catching up
            if (mapVisible && mapTab != nullptr)
            {
                if (speedLimiter->isAnyInputMoving())
                    mapTab->repaint();
            }

            // Auto-stop recording for channels that haven't received remote positions
            juce::int64 now = juce::Time::currentTimeMillis();
            for (auto it = remoteWaypointTimestamps.begin(); it != remoteWaypointTimestamps.end(); )
            {
                int channelIndex = it->first;
                juce::int64 lastTime = it->second;

                if (now - lastTime > remoteWaypointTimeoutMs)
                {
                    // Timeout: stop recording for this channel
                    if (speedLimiter->isRecording(channelIndex))
                    {
                        speedLimiter->stopRecording(channelIndex);
                    }
                    // Remove from tracking map
                    it = remoteWaypointTimestamps.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        // Process LFO at 50Hz (control rate)
        if (lfoProcessor != nullptr)
        {
            lfoProcessor->process(0.02f);  // 20ms delta time (50Hz)
        }

        // Collapse the render-source input meters onto channels. Must run
        // BEFORE the AutomOtion block below, which reads the result, and is
        // deliberately not gated on isMeteringActive() — the audio trigger
        // needs these levels with every meter closed.
        if (levelMeteringManager != nullptr)
            levelMeteringManager->refreshInputLevels();

        // Collect audio levels for AutomOtion triggering
        if (automOtionProcessor != nullptr)
        {
            // getCurrentChannel() is the permanent NUMBER; `i` below is a
            // SLOT, and numbers have gaps and are not in slot order after a
            // reorder. -1 when nothing live is selected, matching no slot.
            const int selSlot = (inputsTab != nullptr)
                ? parameters.getValueTreeState().getSlotForChannelNumber (inputsTab->getCurrentChannel())
                : -1;
            for (int i = 0; i < numInputChannels; ++i)
            {
                // Same input levels the meters show. This used to switch on
                // whichever WFS algorithm was CONFIGURED and read its detector,
                // which meant the audio trigger was dead whenever that algorithm
                // was not running — binaural-only monitoring most of all. The
                // ballistics differ slightly from that detector's (it is the
                // Live Source Tamer's compressor envelope, not a meter's), so
                // triggers may fire a touch differently.
                const auto level = levelMeteringManager != nullptr
                    ? levelMeteringManager->getInputLevel (i)
                    : LevelMeteringManager::LevelData{};
                const float shortPeakDb = level.peakDb;
                const float rmsDb = level.rmsDb;
                automOtionProcessor->setInputLevels(i, shortPeakDb, rmsDb);

                // Update trigger/reset indicators for the currently selected input
                if (inputsTab && i == selSlot)
                    inputsTab->updateOtomoLevelIndicators (shortPeakDb, rmsDb);
            }
        }

        // The same levels for the effect returns, read from the return row's
        // own render-source meter: that row is what the callback popped out of
        // the engine, so it is the return the operator hears. getInputLevel()
        // cannot serve here - it collapses onto channels and stops at the
        // input count.
        if (effectOtomoProcessor != nullptr && levelMeteringManager != nullptr
            && renderSourceMap.firstEffectSlot >= 0)
        {
            for (int fx = 0; fx < renderSourceMap.numEffectChannels; ++fx)
            {
                const auto level = levelMeteringManager->getRenderSourceLevel (
                    renderSourceMap.firstEffectSlot + fx);
                effectOtomoProcessor->setInputLevels (fx, level.peakDb, level.rmsDb);

                // The tab's two trigger indicators, for the channel it shows.
                if (effectsTab != nullptr && fx == effectsTab->getCurrentChannel() - 1)
                    effectsTab->updateOtomoLevelIndicators (level.peakDb, level.rmsDb);
            }
        }

        // Process AutomOtion at 50Hz (control rate)
        if (automOtionProcessor != nullptr)
        {
            automOtionProcessor->process(0.02f);  // 20ms delta time (50Hz)

            // Repaint map while AutomOtion is active (shows moving grey dot)
            if (mapVisible && automOtionProcessor->isAnyActive() && mapTab != nullptr)
                mapTab->repaint();
        }

        // The effect returns move on the same control tick. The offsets it
        // publishes reach the calculation engine through the sink, which
        // dirties the effect rows itself.
        if (effectOtomoProcessor != nullptr)
        {
            effectOtomoProcessor->process (0.02f);

            if (mapVisible && effectOtomoProcessor->isAnyActive() && mapTab != nullptr)
                mapTab->repaint();
        }

        // The effect LFOs, on the same tick. Where an input's LFO offset is
        // summed here with the sampler and gradient offsets before one
        // setLFOOffset, an effect return has exactly one LFO contribution, so
        // it goes straight to its own engine slot and adds to the AutomOtion's
        // there. The tab's progress dial and output bars follow the channel
        // it shows.
        if (effectLfoProcessor != nullptr && calculationEngine != nullptr)
        {
            effectLfoProcessor->process (0.02f);

            bool anyEffectLfoMoving = false;
            const int numFx = juce::jmin (parameters.getNumEffectChannels(),
                                          WFSParameterDefaults::maxEffectChannels);
            for (int fx = 0; fx < numFx; ++fx)
            {
                const float ox = effectLfoProcessor->getOffsetX (fx);
                const float oy = effectLfoProcessor->getOffsetY (fx);
                const float oz = effectLfoProcessor->getOffsetZ (fx);
                calculationEngine->setEffectLFOOffset (fx, ox, oy, oz);
                anyEffectLfoMoving = anyEffectLfoMoving
                                  || std::abs (ox) > 0.001f || std::abs (oy) > 0.001f || std::abs (oz) > 0.001f;

                if (effectsTab != nullptr && fx == effectsTab->getCurrentChannel() - 1)
                    effectsTab->updateLFOIndicators (effectLfoProcessor->getRampProgress (fx),
                                                     effectLfoProcessor->isActive (fx),
                                                     effectLfoProcessor->getNormalizedX (fx),
                                                     effectLfoProcessor->getNormalizedY (fx),
                                                     effectLfoProcessor->getNormalizedZ (fx));
            }

            if (mapVisible && anyEffectLfoMoving && mapTab != nullptr)
                mapTab->repaint();
        }

        // Update level metering at 50Hz (20ms)
        if (levelMeteringManager != nullptr && levelMeteringManager->isMeteringActive())
        {
            LevelMeteringManager::ProcessingAlgorithm meteringAlg
                = LevelMeteringManager::ProcessingAlgorithm::OutputBuffer;
            switch (currentAlgorithm)
            {
                case ProcessingAlgorithm::InputBuffer:
                    meteringAlg = LevelMeteringManager::ProcessingAlgorithm::InputBuffer;
                    break;
                case ProcessingAlgorithm::OutputBuffer:
                    meteringAlg = LevelMeteringManager::ProcessingAlgorithm::OutputBuffer;
                    break;
#if WFS_GPU_NATIVE
                case ProcessingAlgorithm::NativeGpuWfs:
                    meteringAlg = LevelMeteringManager::ProcessingAlgorithm::NativeGpuWfs;
                    break;
                case ProcessingAlgorithm::NativeGpuOutputBuffer:
                    meteringAlg = LevelMeteringManager::ProcessingAlgorithm::NativeGpuOutputBuffer;
                    break;
#endif
            }
            levelMeteringManager->setCurrentAlgorithm(meteringAlg);
            // Outputs and thread performance come from the algorithm, so they
            // need to know whether one is actually running — the algorithm enum
            // only says which is configured.
            levelMeteringManager->setWfsProcessingActive(audioEngineStarted && processingEnabled);
            levelMeteringManager->updateLevels();

            // Repaint map if level overlay is enabled
            if (mapVisible && levelMeteringManager->isMapOverlayEnabled() && mapTab != nullptr)
                mapTab->repaint();
        }

        // The binaural listener glyph's FACING follows the tracker, and tracker
        // attitude never reaches the ValueTree — so MapTab's property-change
        // coalescer, which covers every other part of the glyph, cannot see it
        // move. 12.5 Hz is enough to read a head turn and cheap enough to leave
        // on; everything else about the glyph still repaints on change.
        if (mapVisible && mapTab != nullptr && (timerTicksSinceLastRandom % 16) == 0
            && headTrackerManager != nullptr && headTrackerManager->getActiveSource() != nullptr
            && parameters.getValueTreeState().getBinauralEnabled())
        {
            mapTab->repaint();
        }

        // Pass combined LFO + AutomOtion offsets and gyrophone offsets to calculation engine for DSP
        for (int i = 0; i < numInputChannels; ++i)
        {
            // Combine LFO and AutomOtion offsets
            float totalOffsetX = 0.0f;
            float totalOffsetY = 0.0f;
            float totalOffsetZ = 0.0f;

            if (lfoProcessor != nullptr)
            {
                totalOffsetX += lfoProcessor->getOffsetX(i);
                totalOffsetY += lfoProcessor->getOffsetY(i);
                totalOffsetZ += lfoProcessor->getOffsetZ(i);
            }

            // AutomOtion writes directly to inputPositionX/Y/Z — no offset needed

            // Add cluster LFO offset (offset-based, no ValueTree writes)
            if (clustersTab != nullptr)
            {
                float cx = 0.0f, cy = 0.0f, cz = 0.0f;
                clustersTab->getClusterLFOOffset(i, cx, cy, cz);
                totalOffsetX += cx;
                totalOffsetY += cy;
                totalOffsetZ += cz;
            }

            calculationEngine->setLFOOffset(i, totalOffsetX, totalOffsetY, totalOffsetZ);

            // Gyrophone offset from LFO only
            if (lfoProcessor != nullptr)
            {
                calculationEngine->setGyrophoneOffset(i,
                    lfoProcessor->getGyrophoneOffsetRad(i));
            }
        }

        // Update delay mode ramps (decays compensation offset for smooth mode transitions)
        calculationEngine->updateDelayModeRamps(0.02f);  // 20ms delta time (50Hz)

        // Evaluate gradient maps at composite input positions (O(1) bitmap lookup per input)
        for (int i = 0; i < numInputChannels && i < static_cast<int> (gradientMapEvaluators.size()); ++i)
        {
            if (gradientMapEvaluators[static_cast<size_t> (i)]->hasAnyActiveBitmap())
            {
                auto pos = calculationEngine->getCompositeInputPosition (i);
                auto offsets = gradientMapEvaluators[static_cast<size_t> (i)]->evaluate (pos.x, pos.y);
                calculationEngine->setGradientMapOffsets (i, offsets.attenuationDb,
                                                          offsets.heightMeters, offsets.hfShelfDb);
            }
            else
            {
                calculationEngine->setGradientMapOffsets (i, 0.0f, 0.0f, 0.0f);
            }
        }

        // The editor's input marker sits where the map is sampled: the
        // composite position, after flip, offset, LFO and constraints. Pushed
        // from here so it follows every way of moving the input, including
        // the Inputs-tab joystick, which lives on another sub-tab and so
        // writes while the editor is hidden.
        if (inputsTab != nullptr && inputsTab->getGradientMapEditor().isShowing())
        {
            const int slot = parameters.getValueTreeState().getSlotForChannelNumber (inputsTab->getCurrentChannel());
            if (slot >= 0 && slot < numInputChannels)
            {
                auto pos = calculationEngine->getCompositeInputPosition (slot);
                inputsTab->getGradientMapEditor().setInputPosition (pos.x, pos.y, slot);
            }
        }

        // Process Live Source Tamer at 50Hz
        if (lsTamerEngine != nullptr)
        {
            using namespace WFSParameterIDs;

            std::vector<float> peakGRs(static_cast<size_t>(numInputChannels));
            std::vector<float> slowGRs(static_cast<size_t>(numInputChannels));

            bool anyLSActive = false;

            for (int i = 0; i < numInputChannels; ++i)
            {
                // Get LS section for this input
                auto lsSection = parameters.getValueTreeState().getInputLiveSourceSection(i);

                // Check if LS is enabled
                bool lsActive = static_cast<int>(lsSection.getProperty(inputLSactive, 0)) != 0;
                if (lsActive)
                    anyLSActive = true;

                // Get LS compressor parameters from ValueTree
                float peakThresh = lsSection.getProperty(inputLSpeakThreshold, -20.0f);
                float peakRatio = lsSection.getProperty(inputLSpeakRatio, 2.0f);
                float slowThresh = lsSection.getProperty(inputLSslowThreshold, -20.0f);
                float slowRatio = lsSection.getProperty(inputLSslowRatio, 2.0f);

                // Push LS parameters to / read GR from whichever algorithm is
                // active. The native GPU paths compute GR host-side too; the
                // resulting attenuation is applied via the WFS level matrix (shared
                // by CPU and GPU), so no audio is gained here — we only supply GR.
                switch (currentAlgorithm)
                {
                    case ProcessingAlgorithm::InputBuffer:
                        inputAlgorithm.setLSParameters(static_cast<size_t>(i),
                            peakThresh, peakRatio, slowThresh, slowRatio);
                        peakGRs[i] = inputAlgorithm.getPeakGainReduction(static_cast<size_t>(i));
                        slowGRs[i] = inputAlgorithm.getSlowGainReduction(static_cast<size_t>(i));
                        break;
                    case ProcessingAlgorithm::OutputBuffer:
                        outputAlgorithm.setLSParameters(static_cast<size_t>(i),
                            peakThresh, peakRatio, slowThresh, slowRatio);
                        peakGRs[i] = outputAlgorithm.getPeakGainReduction(static_cast<size_t>(i));
                        slowGRs[i] = outputAlgorithm.getSlowGainReduction(static_cast<size_t>(i));
                        break;
#if WFS_GPU_NATIVE
                    case ProcessingAlgorithm::NativeGpuWfs:
                        nativeGpuAlgorithm.setLSParameters(static_cast<size_t>(i), lsActive,
                            peakThresh, peakRatio, slowThresh, slowRatio);
                        peakGRs[i] = nativeGpuAlgorithm.getPeakGainReduction(static_cast<size_t>(i));
                        slowGRs[i] = nativeGpuAlgorithm.getSlowGainReduction(static_cast<size_t>(i));
                        break;
                    case ProcessingAlgorithm::NativeGpuOutputBuffer:
                        nativeGpuOutputAlgorithm.setLSParameters(static_cast<size_t>(i), lsActive,
                            peakThresh, peakRatio, slowThresh, slowRatio);
                        peakGRs[i] = nativeGpuOutputAlgorithm.getPeakGainReduction(static_cast<size_t>(i));
                        slowGRs[i] = nativeGpuOutputAlgorithm.getSlowGainReduction(static_cast<size_t>(i));
                        break;
#endif
                }
            }

            // Process LS gains
            lsTamerEngine->process(peakGRs, slowGRs);

            // Push LS GR to InputsTab meter display (gated on enable flags)
            if (windowVisible && inputsTab != nullptr)
            {
                // getCurrentChannel() is the permanent NUMBER; the LS tree
                // section and the peakGRs/slowGRs arrays are slot-indexed, and
                // numbers have gaps and are not in slot order after a reorder.
                int ch = parameters.getValueTreeState().getSlotForChannelNumber (inputsTab->getCurrentChannel());
                if (ch >= 0 && ch < numInputChannels)
                {
                    auto lsSection = parameters.getValueTreeState().getInputLiveSourceSection(ch);
                    bool lsActive = static_cast<int>(lsSection.getProperty(inputLSactive, 0)) != 0;
                    bool peakOn = lsActive && static_cast<int>(lsSection.getProperty(inputLSpeakEnable, 0)) != 0;
                    bool slowOn = lsActive && static_cast<int>(lsSection.getProperty(inputLSslowEnable, 0)) != 0;

                    inputsTab->setLSGainReduction(
                        peakOn ? peakGRs[ch] : 1.0f,
                        slowOn ? slowGRs[ch] : 1.0f);
                }
            }

            // Mark only inputs that were active at start of LS processing
            // This is more efficient than marking all inputs, and ensures
            // the final ramp-out tick triggers visualization update
            for (int i = 0; i < numInputChannels; ++i)
            {
                if (lsTamerEngine->inputNeedsRecalculation(i))
                    calculationEngine->markInputDirty(i);
            }
        }

        // Sync binaural processor enabled state from ValueTree.
        // Ordering matters: the worker may already be running (started gated-off in
        // prepareToPlay), so quiesce it, rebuild buffers, publish the RT snapshot,
        // and only then un-gate — the hot loop must never see half-built state.
        if (binauralProcessor)
        {
            bool enabled = parameters.getValueTreeState().getBinauralEnabled();
            bool wasEnabled = binauralProcessor->isEnabled();

            if (enabled && !wasEnabled)
            {
                // No audio device = nothing to prepare against and nowhere for
                // the output to go. Starting the worker unprepared crashed on
                // its null output rings (first field report: enabling binaural
                // with the device closed), so refuse instead — the enable is
                // retried every tick and succeeds once a device is running.
                auto* device = deviceManager.getCurrentAudioDevice();
                if (device == nullptr)
                {
                    if (!binauralNoDeviceWarned)
                    {
                        WFSLogger::getInstance().logWarning(
                            "Binaural: not starting - no audio device is running");
                        binauralNoDeviceWarned = true;
                    }
                }
                else
                {
                    binauralNoDeviceWarned = false;
                    binauralProcessor->stopProcessing();   // quiesce before reconfiguring
                    binauralProcessor->prepareToPlay(device->getCurrentSampleRate(),
                                                     device->getCurrentBufferSizeSamples(),
                                                     numRenderSources);
                    if (binauralCalcEngine != nullptr)
                        binauralCalcEngine->refreshRtSnapshot();  // publish before un-gating
                    binauralProcessor->setEnabled(true);
                    binauralProcessor->startProcessing();
                }
            }
            else if (!enabled && wasEnabled)
            {
                binauralProcessor->setEnabled(false);
                binauralProcessor->stopProcessing();
            }
        }

        // Always republish the binaural RT snapshot (recalculates listener/speaker
        // positions and publishes params/solo state for the realtime thread)
        if (binauralCalcEngine != nullptr)
            binauralCalcEngine->refreshRtSnapshot();

        // SOFA HRTF set management: reload/re-cook when the selection or audio
        // format changes; release sets the render worker retired.
        if (binauralProcessor != nullptr)
            updateBinauralSofaSet();

        // Head-orientation source selection (fast-path tracker vs manual).
        // Unknown/absent device ids resolve to manual; the persisted id is
        // kept so the tracker re-engages when it reappears.
        if (binauralProcessor != nullptr && headTrackerManager != nullptr)
        {
            auto binauralState = parameters.getValueTreeState().getBinauralState();

            // ORTF legacy (renderMode 0) never reads head orientation at all —
            // BinauralProcessor::processBlock returns before the pose is built.
            // Resolve to manual there so an exclusive device is not held for
            // nothing: selecting the webcam and switching to legacy used to
            // leave the camera open, LED on, unavailable to anything else,
            // with zero effect on the audio. The PERSISTED id is untouched, so
            // the tracker re-engages on the way back to an HRTF mode, exactly
            // like the existing absent-device path.
            const bool orientationUsed = binauralState.isValid()
                && juce::jlimit(WFSParameterDefaults::binauralRenderModeMin,
                                WFSParameterDefaults::binauralRenderModeMax,
                                (int) binauralState.getProperty(WFSParameterIDs::binauralRenderMode,
                                                                WFSParameterDefaults::binauralRenderModeDefault)) != 0;

            const juce::String wanted = (binauralState.isValid() && orientationUsed)
                ? binauralState.getProperty(WFSParameterIDs::binauralHeadTrackerSource, "manual").toString()
                : juce::String("manual");
            // Re-resolve on a selection change, and also when a device is
            // wanted but nothing is bound — a persisted "usb:1234" chosen
            // while the dongle was unplugged has to engage when it returns.
            bool rebind = (wanted != lastAppliedHeadTrackerSource);

            if (! rebind && wanted != "manual"
                && headTrackerManager->getActiveSource() == nullptr)
            {
                // This tick runs at 200 Hz (startTimer(5)) and hasSource()
                // compares juce::Strings returned BY VALUE, which allocate.
                // The atomic null-check above is free and covers the normal
                // case; the rate limit covers the waiting-for-a-device case.
                const auto now = juce::Time::getMillisecondCounter();
                if (now - lastHeadTrackerRebindMs > 500)
                {
                    lastHeadTrackerRebindMs = now;
                    rebind = headTrackerManager->hasSource(wanted);
                }
            }

            if (rebind)
            {
                headTrackerManager->setActiveSource(wanted);
                binauralProcessor->setHeadOrientationSource(headTrackerManager->getActiveSource());
                lastAppliedHeadTrackerSource = wanted;
            }
        }

        // Stereo slice geometry: publish config down / slice states up and
        // hand the engine fresh per-slice offsets (marks channels dirty only
        // on an actual change, so this adds no recalc work for mono shows)
        const bool stereoLegsChanged = refreshStereoSliceGeometry();

        // The Map draws its spread bar out of the cache the call above fills,
        // and nothing else asks it to repaint when a width/axis dial or an
        // anchor move changes it — every neighbouring repaint here is gated on
        // something animating, and MapTab drops non-MCP property changes. The
        // frames that DO get painted (a drag, an arrow-key nudge) run before
        // this tick refills the cache, so they draw the previous legs; driving
        // the repaint from the change is what stops the bar lagging a gesture
        // behind the dot. Gated on the change so a static show with a stereo
        // channel does not repaint the Map 50 times a second.
        if (stereoLegsChanged && mapVisible && mapTab != nullptr)
            mapTab->repaint();

        // Only recalculate WFS matrix if input positions have changed (dirty flag set).
        // LS gains are supplied fresh each call (never cached by the engine).
        if (calculationEngine->recalculateMatrixIfDirty(lsTamerEngine ? lsTamerEngine->getLSGains() : nullptr))
        {
            // The effects engine reads the feed triplet through raw pointers
            // handed over at prepare; the six scalars are re-handed whenever a
            // recalc ran, so the live source and effect counts follow the map
            if (effectsHost != nullptr && effectsHost->isPrepared())
                effectsHost->setFeedMatrices (*calculationEngine, numRenderSources);

            // Copy calculated values to target arrays
            // Note: Calculation engine uses maxOutputChannels (128) for stride,
            // but our local arrays use numOutputChannels (user-configured)
            const float* calcDelays = calculationEngine->getDelayTimesMs();
            const float* calcLevels = calculationEngine->getLevels();
            const float* calcHF = calculationEngine->getHFAttenuationDb();
            const int calcStride = calculationEngine->getNumOutputs();  // maxOutputChannels (128)

            // Copy FR matrices from calculation engine
            const float* calcFRDelays = calculationEngine->getFRDelayTimesMs();
            const float* calcFRLevels = calculationEngine->getFRLevels();
            const float* calcFRHF = calculationEngine->getFRHFAttenuationDb();

            for (int inIdx = 0; inIdx < numRenderSources; ++inIdx)
            {
                for (int outIdx = 0; outIdx < numOutputChannels; ++outIdx)
                {
                    int srcIdx = inIdx * calcStride + outIdx;
                    int dstIdx = inIdx * numOutputChannels + outIdx;
                    targetDelayTimesMs[dstIdx] = calcDelays[srcIdx];
                    targetLevels[dstIdx] = calcLevels[srcIdx];
                    hfAttenuation[dstIdx] = calcHF[srcIdx];  // HF doesn't need smoothing - filter handles it

                    // Copy FR matrices. Delays/HF apply directly (delay changes are
                    // smoothed per-processor; HF steps are filter-coefficient updates),
                    // but the LEVEL goes through the same one-pole fade as the direct
                    // level - an instant FR level at engage produced a loud coherent
                    // onset (tap starts at the direct delay before sliding away).
                    frDelayTimesMs[dstIdx] = calcFRDelays[srcIdx];
                    targetFRLevels[dstIdx] = calcFRLevels[srcIdx];
                    frHFAttenuation[dstIdx] = calcFRHF[srcIdx];
                }
            }

            // Update FR filter parameters for each render source. Derived
            // slice slots have no Hackoustics section and floor reflections
            // are N/A for stereo channels — force their filters off (their FR
            // matrix rows are zero anyway).
            for (int i = 0; i < numRenderSources; ++i)
            {
                using namespace WFSParameterIDs;

                bool lowCutActive = false;
                float lowCutFreq = 100.0f;
                bool highShelfActive = false;
                float highShelfFreq = 3000.0f;
                float highShelfGain = -2.0f;
                float highShelfSlope = 0.4f;
                float diffusion = 0.0f;

                if (i < numInputChannels)
                {
                    auto frSection = parameters.getValueTreeState().getInputHackousticsSection(i);

                    lowCutActive = static_cast<int>(frSection.getProperty(inputFRlowCutActive, 0)) != 0;
                    lowCutFreq = frSection.getProperty(inputFRlowCutFreq, 100.0f);
                    highShelfActive = static_cast<int>(frSection.getProperty(inputFRhighShelfActive, 0)) != 0;
                    highShelfFreq = frSection.getProperty(inputFRhighShelfFreq, 3000.0f);
                    highShelfGain = frSection.getProperty(inputFRhighShelfGain, -2.0f);
                    highShelfSlope = frSection.getProperty(inputFRhighShelfSlope, 0.4f);
                    diffusion = frSection.getProperty(inputFRdiffusion, 20.0f);
                }

                if (currentAlgorithm == ProcessingAlgorithm::InputBuffer)
                {
                    inputAlgorithm.setFRFilterParams(static_cast<size_t>(i),
                        lowCutActive, lowCutFreq,
                        highShelfActive, highShelfFreq, highShelfGain, highShelfSlope);
                    inputAlgorithm.setFRDiffusion(static_cast<size_t>(i), diffusion);
                }
                else if (currentAlgorithm == ProcessingAlgorithm::OutputBuffer)
                {
                    outputAlgorithm.setFRFilterParams(static_cast<size_t>(i),
                        lowCutActive, lowCutFreq,
                        highShelfActive, highShelfFreq, highShelfGain, highShelfSlope);
                    outputAlgorithm.setFRDiffusion(static_cast<size_t>(i), diffusion);
                }
#if WFS_GPU_NATIVE
                else if (currentAlgorithm == ProcessingAlgorithm::NativeGpuWfs)
                {
                    nativeGpuAlgorithm.setFRFilterParams(static_cast<size_t>(i),
                        lowCutActive, lowCutFreq,
                        highShelfActive, highShelfFreq, highShelfGain, highShelfSlope);
                    nativeGpuAlgorithm.setFRDiffusion(static_cast<size_t>(i), diffusion);
                }
                else if (currentAlgorithm == ProcessingAlgorithm::NativeGpuOutputBuffer)
                {
                    nativeGpuOutputAlgorithm.setFRFilterParams(static_cast<size_t>(i),
                        lowCutActive, lowCutFreq,
                        highShelfActive, highShelfFreq, highShelfGain, highShelfSlope);
                    nativeGpuOutputAlgorithm.setFRDiffusion(static_cast<size_t>(i), diffusion);
                }
#endif
            }

            // Update visualisation with current DSP matrix values (skip when minimized)
            // Use our correctly-strided local arrays (targetDelayTimesMs has numOutputChannels stride)
            if (windowVisible && inputsTab != nullptr)
            {
                // Create temporary reverb arrays with correct stride for visualization
                // Calculation engine uses maxReverbChannels (32) stride, but user may have fewer
                const float* calcReverbDelays = calculationEngine->getInputReverbDelayTimesMs();
                const float* calcReverbLevels = calculationEngine->getInputReverbLevels();
                const float* calcReverbHF = calculationEngine->getInputReverbHFAttenuationDb();
                const int calcReverbStride = calculationEngine->getNumReverbs();  // maxReverbChannels (32)
                int numReverbs = parameters.getNumReverbChannels();

                // Reindex reverb data with user-configured stride
                std::vector<float> reverbDelays(numRenderSources * numReverbs);
                std::vector<float> reverbLevels(numRenderSources * numReverbs);
                std::vector<float> reverbHF(numRenderSources * numReverbs);

                for (int inIdx = 0; inIdx < numRenderSources; ++inIdx)
                {
                    for (int revIdx = 0; revIdx < numReverbs; ++revIdx)
                    {
                        int srcIdx = inIdx * calcReverbStride + revIdx;
                        int dstIdx = inIdx * numReverbs + revIdx;
                        reverbDelays[dstIdx] = calcReverbDelays[srcIdx];
                        reverbLevels[dstIdx] = calcReverbLevels[srcIdx];
                        reverbHF[dstIdx] = calcReverbHF[srcIdx];
                    }
                }

                std::vector<float> effectDelays, effectLevels, effectHF;
                packEffectVisualisationRows (effectDelays, effectLevels, effectHF);

                inputsTab->updateVisualisation(
                    targetDelayTimesMs.data(), targetLevels.data(), hfAttenuation.data(),
                    reverbDelays.data(), reverbLevels.data(), reverbHF.data(),
                    effectDelays.empty() ? nullptr : effectDelays.data(),
                    effectLevels.empty() ? nullptr : effectLevels.data(),
                    effectHF.empty() ? nullptr : effectHF.data());
            }

            // Tablet mirroring is throttled below and must run even when the
            // window is hidden, so only mark the recalc here.
            visSendPending = true;
        }

        // Drain pending tablet visualisation updates at most every visSendIntervalMs.
        // Trailing-edge: the last recalc of a drag always goes out. With nothing
        // pending, repeat the current state every visKeepaliveIntervalMs: vis only
        // went out on a matrix recalc, so a tablet that lost the connect-time burst
        // (or any later send) kept "Waiting for data..." or blank bars until
        // something moved. A drag drains every 100 ms, so the repeat never adds to
        // one. Unsigned subtraction on the monotonic counter: wrap-safe.
        {
            const juce::uint32 nowMs = juce::Time::getMillisecondCounter();
            const juce::uint32 sinceVisMs = nowMs - lastVisSendMs;
            if ((visSendPending && sinceVisMs >= static_cast<juce::uint32>(visSendIntervalMs))
                || sinceVisMs >= static_cast<juce::uint32>(visKeepaliveIntervalMs))
            {
                if (oscManager != nullptr)
                {
                    // Nothing pending = a pure repeat: keep its selection and rows
                    // out of the Network Log, or it buries everything else there.
                    const WFSNetwork::OSCManager::ScopedQuietRemoteVisLog quietRepeat (*oscManager,
                                                                                       ! visSendPending);
                    sendVisualisationToRemotes();
                }
                visSendPending = false;
                lastVisSendMs = nowMs;
            }
        }

        // Update LFO indicators in InputsTab for the selected input (skip when minimized)
        if (windowVisible && inputsTab != nullptr && lfoProcessor != nullptr)
        {
            int selectedInput = inputsTab->getSelectedInputIndex();
            if (selectedInput >= 0 && selectedInput < numInputChannels)
            {
                inputsTab->updateLFOIndicators(
                    lfoProcessor->getRampProgress(selectedInput),
                    lfoProcessor->isActive(selectedInput),
                    lfoProcessor->getNormalizedX(selectedInput),
                    lfoProcessor->getNormalizedY(selectedInput),
                    lfoProcessor->getNormalizedZ(selectedInput));
            }
        }

        // Effects, every 50 Hz tick: the direct-row solo mask, the loop-guard
        // switch (the one global that applies live), the coalesced parameter
        // publish (one cook per changed channel per tick), and the cycle log
        if (calculationEngine != nullptr)
            calculationEngine->setSoloEffects (soloEffects.load (std::memory_order_relaxed));

        if (effectsHost != nullptr && effectsHost->isPrepared())
        {
            auto globals = parameters.getValueTreeState().getEffectsGlobalSection();
            const bool loopGuard = static_cast<int> (globals.getProperty (WFSParameterIDs::effectsGlobalLoopGuard,
                                                                          WFSParameterDefaults::effectsGlobalLoopGuardDefault)) != 0;
            effectsHost->setLoopGuardEnabled (loopGuard);
            effectsHost->publishDirty();
        }

        if (calculationEngine != nullptr)
        {
            const uint32_t cycleMask = calculationEngine->getEffectCycleMask();
            if (cycleMask != lastLoggedEffectCycleMask)
            {
                if (cycleMask != 0)
                {
                    juce::StringArray members;
                    for (int fx = 0; fx < WFSParameterDefaults::maxEffectChannels; ++fx)
                        if ((cycleMask & (1u << fx)) != 0)
                            members.add (juce::String (fx + 1));
                    WFSLogger::getInstance().logWarning ("Effects: feedback cycle among effect channels "
                                                         + members.joinIntoString (", ")
                                                         + " (the loop guard catches runaway; the routing is yours)");
                }
                else
                {
                    WFSLogger::getInstance().logInfo ("Effects: no feedback cycle remains");
                }
                lastLoggedEffectCycleMask = cycleMask;
            }
        }

        // What the Effects tab's three indicators show. All of it is session
        // state the engine and the calculation engine own, never a property:
        // the loop-guard trip, membership of a feedback cycle, and whether
        // anything feeds this channel at all - which is what makes it the
        // ENTRY POINT of its bunch. The entry role is emergent by decision, so
        // it is derived here from the send row rather than stored.
        if (effectsTab != nullptr && calculationEngine != nullptr)
        {
            const int fx = effectsTab->getCurrentChannel() - 1;

            if (fx >= 0 && fx < parameters.getValueTreeState().getNumEffectChannels())
            {
                const auto* core = effectsHost != nullptr ? effectsHost->getCore() : nullptr;
                const bool tripped = core != nullptr && core->isLoopGuardTripped (fx);
                const bool inCycle = (calculationEngine->getEffectCycleMask() & (1u << fx)) != 0;

                bool fedByAnInput = false;
                for (int in = 1; in <= WFSParameterDefaults::maxInputChannels && ! fedByAnInput; ++in)
                    fedByAnInput = parameters.getValueTreeState().getEffectSendOnFromInput (fx, in);

                // The Chain sub-tab's latency readout and per-tile meters, from
                // the same core pointer. No core (processing stopped) reads as
                // -1 and silent meters.
                std::array<float, EffectsChainPanel::numSlots> slotMetersDb {};
                int chainLatency = -1;
                if (core != nullptr)
                {
                    chainLatency = core->getChainLatencySamples (fx);
                    for (int s = 0; s < EffectsChainPanel::numSlots; ++s)
                        slotMetersDb[static_cast<size_t> (s)] = core->getSlotMeterDb (fx, s);
                }
                else
                {
                    slotMetersDb.fill (-120.0f);
                }
                auto* liveDevice = deviceManager.getCurrentAudioDevice();
                const double liveRate = liveDevice != nullptr ? liveDevice->getCurrentSampleRate() : 48000.0;

                effectsTab->setLiveState (fx, tripped, inCycle, fedByAnInput, chainLatency, liveRate, slotMetersDb);
            }

            effectsTab->setCycleMask (calculationEngine->getEffectCycleMask());
        }

        // Update reverb engine parameters (every timer tick, independent of position changes)
        if (reverbEngine && reverbEngine->isActive())
        {
            using namespace WFSParameterIDs;
            auto& vts = parameters.getValueTreeState();
            auto algoSection = vts.getReverbAlgorithmSection();

            if (algoSection.isValid())
            {
                // Switch algorithm type if changed (0=SDN, 1=FDN, 2=IR)
                int algoType = static_cast<int>(algoSection.getProperty(reverbAlgoType, 0));
                if (algoType != lastLoggedAlgoType)
                {
                    const char* algoNames[] = { "SDN", "FDN", "IR" };
                    const char* algoName = (algoType >= 0 && algoType <= 2) ? algoNames[algoType] : "Unknown";
                    WFSLogger::getInstance().logInfo ("Reverb algorithm changed to " + juce::String (algoName));
                    lastLoggedAlgoType = algoType;
                }
                reverbEngine->setAlgorithmType(algoType);

#if WFS_GPU_NATIVE
                // SDN backend (CPU/GPU toggle); fallback status flows back to the
                // ReverbTab below via getReverbGpuStatus().
                if (algoType == 0)  // SDN
                {
                    std::string sdnDev = algoSection.hasProperty(reverbSDNGpuDevice)
                        ? algoSection.getProperty(reverbSDNGpuDevice).toString().toStdString()
                        : (static_cast<int>(algoSection.getProperty(reverbSDNGpu, 0)) != 0
                               ? GpuDeviceManager::instance().firstGpuId() : std::string("cpu"));
                    reverbEngine->setSDNBackendDevice(sdnDev);
                }

                // FDN convolution backend (CPU/GPU toggle); fallback status flows
                // back to the ReverbTab below via getReverbGpuStatus().
                if (algoType == 1)  // FDN
                {
                    std::string fdnDev = algoSection.hasProperty(reverbFDNGpuDevice)
                        ? algoSection.getProperty(reverbFDNGpuDevice).toString().toStdString()
                        : (static_cast<int>(algoSection.getProperty(reverbFDNGpu, 0)) != 0
                               ? GpuDeviceManager::instance().firstGpuId() : std::string("cpu"));
                    reverbEngine->setFDNBackendDevice(fdnDev);
                }
#endif

                // Push IR-specific parameters
                if (algoType == 2)  // IR
                {
#if WFS_GPU_NATIVE
                    // Convolution backend (CPU/GPU toggle); fallback status flows
                    // back to the ReverbTab below.
                    std::string irDev = algoSection.hasProperty(reverbIRGpuDevice)
                        ? algoSection.getProperty(reverbIRGpuDevice).toString().toStdString()
                        : (static_cast<int>(algoSection.getProperty(reverbIRGpu, 0)) != 0
                               ? GpuDeviceManager::instance().firstGpuId() : std::string("cpu"));
                    reverbEngine->setIRBackendDevice(irDev);
#endif

                    juce::String irFilePath = algoSection.getProperty(reverbIRfile, "").toString();
                    float irTrim = algoSection.getProperty(reverbIRtrim, 0.0f);
                    float irLength = algoSection.getProperty(reverbIRlength, 6.0f);

                    if (irFilePath.isNotEmpty() && irFilePath != lastPushedIRFile)
                    {
                        auto irFile = parameters.getFileManager().getIRFolder().getChildFile(irFilePath);
                        reverbEngine->loadIRFile(irFile);
                        lastPushedIRFile = irFilePath;
                    }

                    if (irTrim != lastPushedIRTrim || irLength != lastPushedIRLength)
                    {
                        reverbEngine->setIRParameters(irTrim, irLength);
                        lastPushedIRTrim = irTrim;
                        lastPushedIRLength = irLength;
                    }
                }
                else
                {
                    lastPushedIRFile.clear();  // Reset when not in IR mode
                }

                AlgorithmParameters algoParams;
                algoParams.rt60         = algoSection.getProperty(reverbRT60, 1.5f);
                algoParams.rt60LowMult  = algoSection.getProperty(reverbRT60LowMult, 1.3f);
                algoParams.rt60HighMult = algoSection.getProperty(reverbRT60HighMult, 0.5f);
                algoParams.crossoverLow  = algoSection.getProperty(reverbCrossoverLow, 200.0f);
                algoParams.crossoverHigh = algoSection.getProperty(reverbCrossoverHigh, 4000.0f);
                algoParams.diffusion    = algoSection.getProperty(reverbDiffusion, 0.5f);
                algoParams.sdnScale     = algoSection.getProperty(reverbSDNscale, 1.0f);
                algoParams.fdnSize      = algoSection.getProperty(reverbFDNsize, 1.0f);

                float wetLevelDb = algoSection.getProperty(reverbWetLevel, 0.0f);
                algoParams.wetLevel = juce::Decibels::decibelsToGain(wetLevelDb);

                reverbEngine->setAlgorithmParameters(algoParams);
            }

            // Update node geometry (for SDN)
            int numReverbs = reverbEngine->getNumNodes();
            if (numReverbs > 0)
            {
                std::vector<NodePosition> positions(static_cast<size_t>(numReverbs));
                for (int i = 0; i < numReverbs; ++i)
                {
                    auto pos = calculationEngine->getReverbFeedPosition(i);
                    positions[static_cast<size_t>(i)] = { pos.x, pos.y, pos.z };
                }
                reverbEngine->updateGeometry(positions);
            }

            // Push pre-processor parameters (per-node EQ + global compressor)
            {
                ReverbPreProcessor::PreProcessorParams preParams;

                // Per-node EQ bands
                for (int n = 0; n < numReverbs; ++n)
                {
                    auto eqSection = vts.getReverbEQSection(n);
                    preParams.eqEnabled[static_cast<size_t>(n)] =
                        eqSection.isValid() && static_cast<int>(eqSection.getProperty(reverbPreEQenable, 1)) != 0;

                    for (int b = 0; b < 4; ++b)
                    {
                        auto band = vts.getReverbEQBand(n, b);
                        if (band.isValid())
                        {
                            auto& bp = preParams.eqBands[static_cast<size_t>(n)][static_cast<size_t>(b)];
                            bp.shape = static_cast<int>(band.getProperty(reverbPreEQshape, 0));
                            bp.freq  = static_cast<float>(static_cast<int>(band.getProperty(reverbPreEQfreq, 1000)));
                            bp.gain  = static_cast<float>(band.getProperty(reverbPreEQgain, 0.0f));
                            bp.q     = static_cast<float>(band.getProperty(reverbPreEQq, 0.7f));
                            bp.slope = static_cast<float>(band.getProperty(reverbPreEQslope, 0.7f));
                        }
                    }
                }

                // Global compressor
                auto compSection = vts.getReverbPreCompSection();
                if (compSection.isValid())
                {
                    preParams.compBypass    = static_cast<int>(compSection.getProperty(reverbPreCompBypass, 1)) != 0;
                    preParams.compThreshold = static_cast<float>(compSection.getProperty(reverbPreCompThreshold, -12.0f));
                    preParams.compRatio     = static_cast<float>(compSection.getProperty(reverbPreCompRatio, 2.0f));
                    preParams.compAttack    = static_cast<float>(compSection.getProperty(reverbPreCompAttack, 10.0f));
                    preParams.compRelease   = static_cast<float>(compSection.getProperty(reverbPreCompRelease, 100.0f));
                }

                reverbEngine->setPreProcessorParams(preParams);
            }

            // Push post-processor parameters (global EQ + sidechain-keyed expander)
            {
                ReverbPostProcessor::PostProcessorParams postParams;

                // Global post-EQ bands
                auto postEQSection = vts.getReverbPostEQSection();
                postParams.eqEnabled = postEQSection.isValid()
                    && static_cast<int>(postEQSection.getProperty(reverbPostEQenable, 1)) != 0;

                for (int b = 0; b < 4; ++b)
                {
                    auto band = vts.getReverbPostEQBand(b);
                    if (band.isValid())
                    {
                        auto& bp = postParams.eqBands[static_cast<size_t>(b)];
                        bp.shape = static_cast<int>(band.getProperty(reverbPostEQshape, 0));
                        bp.freq  = static_cast<float>(static_cast<int>(band.getProperty(reverbPostEQfreq, 1000)));
                        bp.gain  = static_cast<float>(band.getProperty(reverbPostEQgain, 0.0f));
                        bp.q     = static_cast<float>(band.getProperty(reverbPostEQq, 0.7f));
                        bp.slope = static_cast<float>(band.getProperty(reverbPostEQslope, 0.7f));
                    }
                }

                // Global expander
                auto expSection = vts.getReverbPostExpSection();
                if (expSection.isValid())
                {
                    postParams.expBypass    = static_cast<int>(expSection.getProperty(reverbPostExpBypass, 1)) != 0;
                    postParams.expThreshold = static_cast<float>(expSection.getProperty(reverbPostExpThreshold, -40.0f));
                    postParams.expRatio     = static_cast<float>(expSection.getProperty(reverbPostExpRatio, 2.0f));
                    postParams.expAttack    = static_cast<float>(expSection.getProperty(reverbPostExpAttack, 1.0f));
                    postParams.expRelease   = static_cast<float>(expSection.getProperty(reverbPostExpRelease, 200.0f));
                }

                reverbEngine->setPostProcessorParams(postParams);
            }

            // Push reverb GR to ReverbTab meter display
            if (windowVisible && reverbTab != nullptr)
                reverbTab->setGainReduction(
                    reverbEngine->getCompGainReductionDb(),
                    reverbEngine->getExpGainReductionDb());

#if WFS_GPU_NATIVE
            // Push IR convolution backend status (GPU active / CPU fallback)
            if (windowVisible && reverbTab != nullptr)
            {
                auto gpuStatus = reverbEngine->getReverbGpuStatus();
                reverbTab->setReverbGpuStatus(static_cast<int>(gpuStatus.mode),
                                              gpuStatus.device, gpuStatus.error,
                                              gpuStatus.latencyMs);
            }
#endif

            // Check for reverb dropouts every ~1s (200 ticks at 5ms)
            if ((timerTicksSinceLastRandom % 200) == 100)
            {
                uint64_t drops = reverbEngine->getAndResetDropoutCount();
                if (drops > 0 && statusBar != nullptr)
                    statusBar->showTemporaryMessage (
                        "Reverb dropout detected - consider reducing reverb channels, IR length, or switching to SDN/FDN",
                        5000);
            }
        }

        // Push per-output EQ parameters every tick. The biquad short-circuits on
        // no-change, so this is cheap when the user isn't touching the GUI.
        {
            using namespace WFSParameterIDs;
            auto& vts = parameters.getValueTreeState();

            OutputEQProcessor::Params eqParams;
            eqParams.channels.resize(static_cast<size_t>(numOutputChannels));

            for (int c = 0; c < numOutputChannels; ++c)
            {
                auto& cp = eqParams.channels[static_cast<size_t>(c)];

                cp.enabled = static_cast<int>(vts.getOutputParameter(c, outputEQenabled)) != 0;

                for (int b = 0; b < OutputEQProcessor::NUM_EQ_BANDS; ++b)
                {
                    auto band = vts.getOutputEQBand(c, b);
                    auto& bp = cp.bands[static_cast<size_t>(b)];

                    if (band.isValid())
                    {
                        bp.shape = static_cast<int>  (band.getProperty(eqShape,     0));
                        bp.freq  = static_cast<float>(band.getProperty(eqFrequency, 1000.0f));
                        bp.gain  = static_cast<float>(band.getProperty(eqGain,      0.0f));
                        bp.q     = static_cast<float>(band.getProperty(eqQ,         0.7f));
                        bp.slope = static_cast<float>(band.getProperty(eqSlope,     0.7f));
                    }
                }
            }

            outputEQProcessor.setParameters(eqParams);
        }

        // Check if any LFO is producing movement (used for map repaint and composite delta rate)
        bool anyLFOActive = false;
        if (lfoProcessor != nullptr)
        {
            for (int i = 0; i < numInputChannels && !anyLFOActive; ++i)
            {
                if (std::abs(lfoProcessor->getOffsetX(i)) > 0.001f ||
                    std::abs(lfoProcessor->getOffsetY(i)) > 0.001f)
                {
                    anyLFOActive = true;
                }
            }
        }

        // Repaint map if any LFO (per-input or cluster) is active
        bool anyClusterLFOActive = (clustersTab != nullptr && clustersTab->isAnyClusterLFOActive());
        if (mapVisible && mapTab != nullptr && (anyLFOActive || anyClusterLFOActive))
            mapTab->repaint();

        // Send composite delta to Remote targets (delta = composite - target position)
        // Rate-limited: ~50Hz when LFO active (every 4 ticks), ~20Hz otherwise (every 10 ticks)
        // This enables the Android app to show the offset from transformations (flip, offset, LFO, speed limiting)
        static int compositeDeltaTickCounter = 0;
        compositeDeltaTickCounter++;
        int rateLimit = anyLFOActive ? 4 : 10;  // 4 ticks = 20ms = 50Hz, 10 ticks = 50ms = 20Hz
        if (compositeDeltaTickCounter >= rateLimit && oscManager != nullptr && calculationEngine != nullptr)
        {
            compositeDeltaTickCounter = 0;

            constexpr float deltaThreshold = 0.01f;  // 1cm threshold for considering delta significant
            constexpr float changeThreshold = 0.005f;  // 5mm threshold for detecting delta change

            // Sampler playing-state transitions (for the orange ring on the remote).
            if ((int) prevSamplerPlaying.size() < numInputChannels)
                prevSamplerPlaying.resize((size_t) numInputChannels, 0u);
            for (int i = 0; i < numInputChannels; ++i)
            {
                bool playingNow = (samplerManager != nullptr && samplerManager->isChannelPlaying(i));
                bool playingPrev = prevSamplerPlaying[(size_t) i] != 0u;
                if (playingNow != playingPrev)
                {
                    // The wire addresses inputs by permanent channel NUMBER;
                    // `i` is a SLOT, and numbers have gaps and are not in slot
                    // order after a reorder.
                    oscManager->sendInputSamplerPlayingState(
                        parameters.getValueTreeState().getInputChannelNumber (i), playingNow);
                    prevSamplerPlaying[(size_t) i] = playingNow ? 1u : 0u;
                }
            }

            for (int i = 0; i < numInputChannels; ++i)
            {
                // Get target position (raw user-controlled position)
                auto posSection = parameters.getValueTreeState().getInputPositionSection(i);
                float targetX = posSection.getProperty(WFSParameterIDs::inputPositionX, 0.0f);
                float targetY = posSection.getProperty(WFSParameterIDs::inputPositionY, 0.0f);

                // Get composite position (final DSP position after all transformations)
                auto compositePos = calculationEngine->getCompositeInputPosition(i);

                // Compute delta (composite - target)
                float deltaX = compositePos.x - targetX;
                float deltaY = compositePos.y - targetY;

                // Check if this delta is significant (non-zero)
                bool deltaIsSignificant = (std::abs(deltaX) > deltaThreshold || std::abs(deltaY) > deltaThreshold);

                // Get last sent delta (or assume zero if never sent)
                auto lastIt = lastSentCompositeDeltas.find(i);
                float lastDeltaX = 0.0f;
                float lastDeltaY = 0.0f;
                if (lastIt != lastSentCompositeDeltas.end())
                {
                    lastDeltaX = lastIt->second.first;
                    lastDeltaY = lastIt->second.second;
                }

                // Check if delta changed from what we last sent
                bool deltaChanged = (std::abs(deltaX - lastDeltaX) > changeThreshold ||
                                     std::abs(deltaY - lastDeltaY) > changeThreshold);

                // Send if: delta changed AND (it's significant OR it just became zero)
                if (deltaChanged)
                {
                    bool lastWasSignificant = (std::abs(lastDeltaX) > deltaThreshold || std::abs(lastDeltaY) > deltaThreshold);
                    if (deltaIsSignificant || lastWasSignificant)
                    {
                        // The wire addresses inputs by permanent channel
                        // NUMBER; numbers have gaps and are not in slot order
                        // after a reorder. The cache stays keyed by SLOT.
                        int inputId = parameters.getValueTreeState().getInputChannelNumber (i);
                        oscManager->sendCompositeDeltaToRemote(inputId, deltaX, deltaY);
                        lastSentCompositeDeltas[i] = std::make_pair(deltaX, deltaY);
                    }
                }
            }
        }
    }

    // Parameter smoothing has moved to getNextAudioBlock() so it runs on the
    // ASIO thread and is immune to message-pump throttling when minimized.

    // Repaint to update CPU usage display (every 10 ticks = 50ms)
    if (windowVisible && processingEnabled && audioEngineStarted
        && timerTicksSinceLastRandom % 10 == 0)
        repaint();

    // Check for device changes every 200 ticks (once per second) to avoid overhead
    // Only save after device restoration is complete to avoid saving fallback device
    if (deviceRestoreComplete && timerTicksSinceLastRandom % 200 == 0)
    {
        juce::String currentDeviceType = deviceManager.getCurrentAudioDeviceType();
        juce::String currentDeviceName;
        if (auto* device = deviceManager.getCurrentAudioDevice())
            currentDeviceName = device->getName();

        bool typeChanged = (currentDeviceType != lastSavedDeviceType && currentDeviceType.isNotEmpty());
        bool nameChanged = (currentDeviceName != lastSavedDeviceName && currentDeviceName.isNotEmpty());

        if (typeChanged || nameChanged)
        {
            lastSavedDeviceType = currentDeviceType;
            lastSavedDeviceName = currentDeviceName;
            saveSettings();
        }
    }
}

//==============================================================================
// Routing matrix access methods
void MainComponent::setDelay(int inputChannel, int outputChannel, float delayMs)
{
    if (inputChannel >= 0 && inputChannel < numInputChannels &&
        outputChannel >= 0 && outputChannel < numOutputChannels)
    {
        int idx = inputChannel * numOutputChannels + outputChannel;
        delayTimesMs[idx] = delayMs;
    }
}

void MainComponent::setLevel(int inputChannel, int outputChannel, float level)
{
    if (inputChannel >= 0 && inputChannel < numInputChannels &&
        outputChannel >= 0 && outputChannel < numOutputChannels)
    {
        int idx = inputChannel * numOutputChannels + outputChannel;
        levels[idx] = juce::jlimit(0.0f, 1.0f, level);
    }
}

float MainComponent::getDelay(int inputChannel, int outputChannel) const
{
    if (inputChannel >= 0 && inputChannel < numInputChannels &&
        outputChannel >= 0 && outputChannel < numOutputChannels)
    {
        int idx = inputChannel * numOutputChannels + outputChannel;
        return delayTimesMs[idx];
    }
    return 0.0f;
}

float MainComponent::getLevel(int inputChannel, int outputChannel) const
{
    if (inputChannel >= 0 && inputChannel < numInputChannels &&
        outputChannel >= 0 && outputChannel < numOutputChannels)
    {
        int idx = inputChannel * numOutputChannels + outputChannel;
        return levels[idx];
    }
    return 0.0f;
}

//==============================================================================
// Keyboard handling implementation

bool MainComponent::isTextEditorFocused() const
{
    // Check if any TextEditor component has keyboard focus
    auto* focused = juce::Component::getCurrentlyFocusedComponent();
    return dynamic_cast<juce::TextEditor*>(focused) != nullptr;
}

void MainComponent::startChannelSelection(ChannelSelectionMode mode)
{
    channelSelectionMode = mode;
    channelNumberBuffer.clear();
    channelSelectionStartTime = juce::Time::currentTimeMillis();

    // Switch to appropriate tab
    juce::String prompt;
    switch (mode)
    {
        case ChannelSelectionMode::Input:
            tabbedComponent.setCurrentTabIndex(TabIndex::Inputs);
            prompt = "Select Input Channel: ";
            break;
        case ChannelSelectionMode::Output:
            tabbedComponent.setCurrentTabIndex(TabIndex::Outputs);
            prompt = "Select Output Channel: ";
            break;
        case ChannelSelectionMode::Reverb:
            tabbedComponent.setCurrentTabIndex(TabIndex::Reverb);
            prompt = "Select Reverb Channel: ";
            break;
        default:
            break;
    }

    // Grab keyboard focus back so we receive subsequent key presses
    grabKeyboardFocus();

    // Show message after any tab-switching UI updates have completed
    if (statusBar != nullptr)
    {
        juce::MessageManager::callAsync([this, prompt]() {
            if (statusBar != nullptr && channelSelectionMode != ChannelSelectionMode::None)
                statusBar->showTemporaryMessage(prompt, channelSelectionTimeoutMs);
        });
    }
}

void MainComponent::cancelChannelSelection()
{
    channelSelectionMode = ChannelSelectionMode::None;
    channelNumberBuffer.clear();
    if (statusBar != nullptr)
        statusBar->clearText();
}

void MainComponent::confirmChannelSelection()
{
    int channelNum = channelNumberBuffer.getIntValue();
    if (channelNum > 0)
    {
        switch (channelSelectionMode)
        {
            case ChannelSelectionMode::Input:
                // The operator types a permanent channel NUMBER, which may
                // exceed the channel COUNT once the list has gaps — so the
                // number must be tested for liveness, not bounded by the
                // count. Output/reverb ids below ARE dense slot positions.
                if (inputsTab != nullptr
                    && parameters.getValueTreeState().getSlotForChannelNumber (channelNum) >= 0)
                    inputsTab->selectChannel(channelNum);
                break;
            case ChannelSelectionMode::Output:
                if (outputsTab != nullptr && channelNum <= outputsTab->getNumChannels())
                    outputsTab->selectChannel(channelNum);
                break;
            case ChannelSelectionMode::Reverb:
                if (reverbTab != nullptr && channelNum <= reverbTab->getNumChannels())
                    reverbTab->selectChannel(channelNum);
                break;
            default:
                break;
        }
    }
    cancelChannelSelection();
}

void MainComponent::resetHelpCycle()
{
    if (helpCycleIndex >= 0 && helpCycleIndex < (int)helpCycleButtons.size())
        helpCycleButtons[helpCycleIndex]->dismiss();
    helpCycleIndex = -1;
    helpCycleStartIndex = -1;
    helpCycleButtons.clear();
}

void MainComponent::cycleHelpCards()
{
    // Get the active tab's help card provider
    HelpCardProvider* provider = nullptr;
    // By NAMED index: the Effects tab (4) moved Inputs, Clusters and Map up
    // one, and the literal indices here kept cycling the Inputs tab's cards
    // on the Effects tab and gave the Map none.
    switch (tabbedComponent.getCurrentTabIndex())
    {
        case TabIndex::SystemConfig: provider = systemConfigTab; break;
        case TabIndex::Network:      provider = networkTab; break;
        case TabIndex::Outputs:      provider = outputsTab; break;
        case TabIndex::Reverb:       provider = reverbTab; break;
        case TabIndex::Effects:      provider = effectsTab; break;
        case TabIndex::Inputs:       provider = inputsTab; break;
        case TabIndex::Clusters:     provider = clustersTab; break;
        case TabIndex::Map:          provider = dynamic_cast<HelpCardProvider*>(mapTab.get()); break;
    }
    if (provider == nullptr) return;

    auto buttons = provider->getVisibleHelpButtons();
    if (buttons.empty()) return;

    // Sort buttons: column-first (X), then top-down (Y)
    std::sort(buttons.begin(), buttons.end(),
        [](const HelpCardButton* a, const HelpCardButton* b)
        {
            int ax = a->getScreenX(), bx = b->getScreenX();
            if (std::abs(ax - bx) <= 50)
                return a->getScreenY() < b->getScreenY();
            return ax < bx;
        });

    helpCycleButtons = buttons;

    // If any card is already open (manually clicked or previous cycle step), advance from it.
    // The cycle wraps around and closes only after every card has been shown once — we stop
    // when advancing would return to helpCycleStartIndex (the anchor for this cycle).
    int activeIdx = -1;
    for (int i = 0; i < (int)buttons.size(); ++i)
    {
        if (buttons[i]->getIsActive())
        {
            activeIdx = i;
            break;
        }
    }
    if (activeIdx >= 0)
    {
        // If the user manually clicked a card before pressing H, adopt that card as the
        // cycle's anchor so we visit the remaining cards and stop after returning to it.
        if (helpCycleStartIndex < 0)
            helpCycleStartIndex = activeIdx;

        buttons[activeIdx]->dismiss();
        int nextIdx = (activeIdx + 1) % (int)buttons.size();

        if (nextIdx == helpCycleStartIndex)
        {
            // Full cycle completed — close the help panel.
            helpCycleIndex = -1;
            helpCycleStartIndex = -1;
            helpCycleButtons.clear();
            return;
        }

        helpCycleIndex = nextIdx;
        buttons[nextIdx]->activate();
        return;
    }

    // Fresh cycle: start from button closest to mouse
    auto mousePos = juce::Desktop::getInstance().getMainMouseSource().getScreenPosition().toInt();
    int closestIdx = 0;
    int closestDist = std::numeric_limits<int>::max();
    for (int i = 0; i < (int)buttons.size(); ++i)
    {
        auto btnCentre = buttons[i]->getScreenBounds().getCentre();
        int dx = btnCentre.x - mousePos.x;
        int dy = btnCentre.y - mousePos.y;
        int dist = dx * dx + dy * dy;
        if (dist < closestDist)
        {
            closestDist = dist;
            closestIdx = i;
        }
    }

    helpCycleIndex = closestIdx;
    helpCycleStartIndex = closestIdx;
    buttons[closestIdx]->activate();
}

void MainComponent::cycleChannel(int delta)
{
    int currentTabIndex = tabbedComponent.getCurrentTabIndex();

    if (currentTabIndex == TabIndex::Inputs && inputsTab != nullptr)
    {
        inputsTab->cycleChannel(delta);
    }
    else if (currentTabIndex == TabIndex::Outputs && outputsTab != nullptr)
    {
        outputsTab->cycleChannel(delta);
    }
    else if (currentTabIndex == TabIndex::Reverb && reverbTab != nullptr)
    {
        reverbTab->cycleChannel(delta);
    }
    else if (currentTabIndex == TabIndex::Effects && effectsTab != nullptr)
    {
        effectsTab->cycleChannel(delta);
    }
}

void MainComponent::nudgeInputPosition(int axis, float delta, int inputOverride)
{
    auto& state = parameters.getValueTreeState();

    // Everything below indexes by SLOT. The override already is one (callers
    // pass the Map's selected input), but getCurrentChannel() is a permanent
    // NUMBER, and numbers have gaps and are not in slot order after a reorder.
    int channel = (inputOverride >= 0) ? inputOverride
                                       : (inputsTab != nullptr
                                              ? state.getSlotForChannelNumber (inputsTab->getCurrentChannel())
                                              : -1);
    if (channel < 0)
        return;

    // Check if constraint is enabled for this axis
    juce::Identifier constraintId;
    switch (axis)
    {
        case 0: constraintId = WFSParameterIDs::inputConstraintX; break;
        case 1: constraintId = WFSParameterIDs::inputConstraintY; break;
        case 2: constraintId = WFSParameterIDs::inputConstraintZ; break;
        default: return;
    }
    bool constrained = state.getIntParameter(constraintId, channel) != 0;

    // Check if tracking is enabled (globally, protocol enabled, AND on channel)
    bool globalTracking = state.getIntParameter(WFSParameterIDs::trackingEnabled) != 0;
    bool protocolEnabled = state.getIntParameter(WFSParameterIDs::trackingProtocol) != 0;
    bool channelTracking = state.getIntParameter(WFSParameterIDs::inputTrackingActive, channel) != 0;
    bool autoMotionActive = automOtionProcessor && automOtionProcessor->isMotionActive (channel);
    bool useOffset = (globalTracking && protocolEnabled && channelTracking) || autoMotionActive;

    // Invert delta when flip is enabled and modifying position (not offset)
    // Offset is added AFTER flip, so offset nudge direction stays normal
    if (!useOffset)
    {
        juce::Identifier flipId;
        switch (axis)
        {
            case 0: flipId = WFSParameterIDs::inputFlipX; break;
            case 1: flipId = WFSParameterIDs::inputFlipY; break;
            case 2: flipId = WFSParameterIDs::inputFlipZ; break;
            default: break;
        }
        if (state.getIntParameter(flipId, channel) != 0)
            delta = -delta;
    }

    juce::Identifier paramId;
    switch (axis)
    {
        case 0:  // X
            paramId = useOffset ? WFSParameterIDs::inputOffsetX : WFSParameterIDs::inputPositionX;
            break;
        case 1:  // Y
            paramId = useOffset ? WFSParameterIDs::inputOffsetY : WFSParameterIDs::inputPositionY;
            break;
        case 2:  // Z
            paramId = useOffset ? WFSParameterIDs::inputOffsetZ : WFSParameterIDs::inputPositionZ;
            break;
        default:
            return;
    }

    float current = state.getFloatParameter(paramId, channel);
    float newValue = current + delta;

    // Apply constraints if enabled (matching InputsTab's getStageMin/Max methods)
    if (constrained)
    {
        // Check if we should skip rectangular X/Y constraint in favor of distance constraint
        int coordMode = state.getIntParameter(WFSParameterIDs::inputCoordinateMode, channel);
        int constraintDist = state.getIntParameter(WFSParameterIDs::inputConstraintDistance, channel);
        bool useDistanceConstraint = (coordMode == 1 || coordMode == 2) && (constraintDist != 0);

        // In cylindrical mode with distance constraint, skip X/Y rectangular bounds
        // In spherical mode with distance constraint, skip X/Y/Z rectangular bounds
        // (distance constraint below will handle circular/spherical bounds instead)
        bool skipRectangularBounds = useDistanceConstraint &&
            ((coordMode == 1 && (axis == 0 || axis == 1)) ||   // Cylindrical: skip X/Y
             (coordMode == 2));                                  // Spherical: skip X/Y/Z

        if (!skipRectangularBounds)
        {
            float minVal = 0.0f, maxVal = 0.0f;
            int stageShape = static_cast<int>(parameters.getConfigParam("StageShape"));

            switch (axis)
            {
                case 0:  // X - uses half size (center-referenced)
                {
                    float halfSize = (stageShape == 0)
                        ? static_cast<float>(parameters.getConfigParam("StageWidth")) / 2.0f
                        : static_cast<float>(parameters.getConfigParam("StageDiameter")) / 2.0f;
                    float origin = static_cast<float>(parameters.getConfigParam("StageOriginWidth"));
                    minVal = -halfSize - origin;
                    maxVal = halfSize - origin;
                    break;
                }
                case 1:  // Y - uses half size (center-referenced)
                {
                    float halfSize = (stageShape == 0)
                        ? static_cast<float>(parameters.getConfigParam("StageDepth")) / 2.0f
                        : static_cast<float>(parameters.getConfigParam("StageDiameter")) / 2.0f;
                    float origin = static_cast<float>(parameters.getConfigParam("StageOriginDepth"));
                    minVal = -halfSize - origin;
                    maxVal = halfSize - origin;
                    break;
                }
                case 2:  // Z - uses direct size (floor-referenced)
                {
                    float stageSize = static_cast<float>(parameters.getConfigParam("StageHeight"));
                    float origin = static_cast<float>(parameters.getConfigParam("StageOriginHeight"));
                    minVal = -origin;
                    maxVal = stageSize - origin;
                    break;
                }
            }
            newValue = juce::jlimit(minVal, maxVal, newValue);
        }
    }

    // Apply distance constraint for Cylindrical/Spherical modes
    int coordMode = state.getIntParameter(WFSParameterIDs::inputCoordinateMode, channel);
    if (coordMode == 1 || coordMode == 2)
    {
        int constraintDist = state.getIntParameter(WFSParameterIDs::inputConstraintDistance, channel);
        if (constraintDist != 0)
        {
            float minDist = state.getFloatParameter(WFSParameterIDs::inputConstraintDistanceMin, channel);
            float maxDist = state.getFloatParameter(WFSParameterIDs::inputConstraintDistanceMax, channel);

            // Get all position values (use offset if tracking, position otherwise)
            float x, y, z;
            if (useOffset)
            {
                x = state.getFloatParameter(WFSParameterIDs::inputOffsetX, channel);
                y = state.getFloatParameter(WFSParameterIDs::inputOffsetY, channel);
                z = state.getFloatParameter(WFSParameterIDs::inputOffsetZ, channel);
            }
            else
            {
                x = state.getFloatParameter(WFSParameterIDs::inputPositionX, channel);
                y = state.getFloatParameter(WFSParameterIDs::inputPositionY, channel);
                z = state.getFloatParameter(WFSParameterIDs::inputPositionZ, channel);
            }

            // Update with new value being set
            if (axis == 0) x = newValue;
            else if (axis == 1) y = newValue;
            else if (axis == 2) z = newValue;

            // Calculate and apply distance constraint
            float currentDist = (coordMode == 1)
                ? std::sqrt(x * x + y * y)        // Cylindrical: XY plane
                : std::sqrt(x * x + y * y + z * z);  // Spherical: 3D

            if (currentDist < 0.0001f) currentDist = 0.0001f;
            float targetDist = juce::jlimit(minDist, maxDist, currentDist);

            if (!juce::approximatelyEqual(currentDist, targetDist))
            {
                float scale = targetDist / currentDist;
                if (coordMode == 1)  // Cylindrical: scale X, Y
                {
                    x *= scale;
                    y *= scale;
                }
                else  // Spherical: scale X, Y, Z
                {
                    x *= scale;
                    y *= scale;
                    z *= scale;
                }

                // Set all position values
                if (useOffset)
                {
                    state.setInputParameter(channel, WFSParameterIDs::inputOffsetX, x);
                    state.setInputParameter(channel, WFSParameterIDs::inputOffsetY, y);
                    state.setInputParameter(channel, WFSParameterIDs::inputOffsetZ, z);
                }
                else
                {
                    state.setInputParameter(channel, WFSParameterIDs::inputPositionX, x);
                    state.setInputParameter(channel, WFSParameterIDs::inputPositionY, y);
                    state.setInputParameter(channel, WFSParameterIDs::inputPositionZ, z);
                }
                return;  // Already set all values
            }
        }
    }

    state.setInputParameter(channel, paramId, newValue);
}

void MainComponent::nudgeOutputPosition(int axis, float delta)
{
    if (outputsTab == nullptr)
        return;

    int channel = outputsTab->getCurrentChannel() - 1;  // Convert to 0-based
    if (channel < 0)
        return;

    auto& state = parameters.getValueTreeState();

    juce::Identifier paramId;
    switch (axis)
    {
        case 0:  // X
            paramId = WFSParameterIDs::outputPositionX;
            break;
        case 1:  // Y
            paramId = WFSParameterIDs::outputPositionY;
            break;
        case 2:  // Z
            paramId = WFSParameterIDs::outputPositionZ;
            break;
        default:
            return;
    }

    float current = state.getFloatParameter(paramId, channel);
    state.setOutputParameter(channel, paramId, current + delta);
}

void MainComponent::nudgeReverbPosition(int axis, float delta)
{
    if (reverbTab == nullptr)
        return;

    int channel = reverbTab->getCurrentChannel() - 1;  // Convert to 0-based
    if (channel < 0)
        return;

    auto& state = parameters.getValueTreeState();

    juce::Identifier paramId;
    switch (axis)
    {
        case 0:  // X
            paramId = WFSParameterIDs::reverbPositionX;
            break;
        case 1:  // Y
            paramId = WFSParameterIDs::reverbPositionY;
            break;
        case 2:  // Z
            paramId = WFSParameterIDs::reverbPositionZ;
            break;
        default:
            return;
    }

    float current = state.getFloatParameter(paramId, channel);
    state.setReverbParameter(channel, paramId, current + delta);
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    // Phase 5c: AI-undo / AI-redo keyboard shortcuts. Cmd/Ctrl-Alt-Z and
    // Cmd/Ctrl-Alt-Y drive the same engine the toast overlay uses. Works
    // regardless of whether the toast is visible or has been dismissed —
    // the engine operates on the change-record ring buffer directly.
    {
        const auto mods = key.getModifiers();
        if (mods.isCommandDown() && mods.isAltDown() && mcpServer != nullptr)
        {
            const auto reportOutcome = [this] (const WFSNetwork::UndoResult& result, const juce::String& verb)
            {
                if (statusBar == nullptr) return;
                const auto key = result.success ? juce::String("ai.undo.successPrefix")
                                                : juce::String("ai.undo.errorPrefix");
                const auto detail = result.success ? result.operatorDescription : result.errorMessage;
                statusBar->showTemporaryMessage(
                    LocalizationManager::getInstance().get(key,
                        {{"verb", verb},
                         {"description", detail},
                         {"message", detail}}),
                    2500);
            };

            if (key.isKeyCode('Z'))
            {
                WFSNetwork::OriginTagScope originScope { WFSNetwork::OriginTag::MCP };
                reportOutcome(mcpServer->getUndoEngine().undoLast(), LOC("ai.undo.verbUndo"));
                return true;
            }
            if (key.isKeyCode('Y'))
            {
                WFSNetwork::OriginTagScope originScope { WFSNetwork::OriginTag::MCP };
                reportOutcome(mcpServer->getUndoEngine().redoLast(), LOC("ai.undo.verbRedo"));
                return true;
            }
        }
    }

    // Check for channel selection timeout
    if (channelSelectionMode != ChannelSelectionMode::None)
    {
        if (juce::Time::currentTimeMillis() - channelSelectionStartTime > channelSelectionTimeoutMs)
            cancelChannelSelection();
    }

    // If in channel selection mode, handle digit input
    if (channelSelectionMode != ChannelSelectionMode::None)
    {
        if (key.isKeyCode(juce::KeyPress::returnKey))
        {
            confirmChannelSelection();
            return true;
        }
        if (key.isKeyCode(juce::KeyPress::escapeKey))
        {
            cancelChannelSelection();
            return true;
        }
        juce::juce_wchar c = key.getTextCharacter();
        if (c >= '0' && c <= '9')
        {
            channelNumberBuffer += c;
            if (statusBar != nullptr)
            {
                juce::String prompt;
                switch (channelSelectionMode)
                {
                    case ChannelSelectionMode::Input: prompt = "Select Input Channel: "; break;
                    case ChannelSelectionMode::Output: prompt = "Select Output Channel: "; break;
                    case ChannelSelectionMode::Reverb: prompt = "Select Reverb Channel: "; break;
                    default: break;
                }
                statusBar->showTemporaryMessage(prompt + channelNumberBuffer,
                    channelSelectionTimeoutMs - (int)(juce::Time::currentTimeMillis() - channelSelectionStartTime));
            }
            return true;
        }
        // Backspace to delete last digit
        if (key.isKeyCode(juce::KeyPress::backspaceKey) && channelNumberBuffer.isNotEmpty())
        {
            channelNumberBuffer = channelNumberBuffer.dropLastCharacters(1);
            // Update display
            if (statusBar != nullptr)
            {
                juce::String prompt;
                switch (channelSelectionMode)
                {
                    case ChannelSelectionMode::Input: prompt = "Select Input Channel: "; break;
                    case ChannelSelectionMode::Output: prompt = "Select Output Channel: "; break;
                    case ChannelSelectionMode::Reverb: prompt = "Select Reverb Channel: "; break;
                    default: break;
                }
                statusBar->showTemporaryMessage(prompt + channelNumberBuffer,
                    channelSelectionTimeoutMs - (int)(juce::Time::currentTimeMillis() - channelSelectionStartTime));
            }
            return true;
        }
        return false;
    }

    // Help card cycling: H key — works even when a text editor has focus
    // (unfocuses the editor so H isn't typed into it)
    if (key.isKeyCode('H') && !key.getModifiers().isCommandDown())
    {
        if (auto* focused = juce::Component::getCurrentlyFocusedComponent())
            if (dynamic_cast<juce::TextEditor*>(focused) != nullptr)
                focused->giveAwayKeyboardFocus();
        cycleHelpCards();
        return true;
    }

    // Skip shortcuts if text editor has focus (user is typing in a field)
    if (isTextEditorFocused())
        return false;

    // Undo/Redo: Ctrl+Z, Ctrl+Y, Ctrl+Shift+Z
    if (key.getModifiers().isCommandDown())
    {
        auto& state = parameters.getValueTreeState();
        if (key.isKeyCode('Z'))
        {
            if (key.getModifiers().isShiftDown())
            {
                // Ctrl+Shift+Z = Redo
                if (state.canRedo())
                {
                    state.redo();
                    repaintActiveTab();
                    return true;
                }
            }
            else
            {
                // Ctrl+Z = Undo
                if (state.canUndo())
                {
                    state.undo();
                    repaintActiveTab();
                    return true;
                }
            }
        }
        if (key.isKeyCode('Y'))
        {
            // Ctrl+Y = Redo
            if (state.canRedo())
            {
                state.redo();
                repaintActiveTab();
                return true;
            }
        }
    }

    // Tab switching with channel selection: I, O, R
    if (key.isKeyCode('I') && !key.getModifiers().isCommandDown())
    {
        startChannelSelection(ChannelSelectionMode::Input);
        return true;
    }
    if (key.isKeyCode('O') && !key.getModifiers().isCommandDown())
    {
        startChannelSelection(ChannelSelectionMode::Output);
        return true;
    }
    if (key.isKeyCode('R') && !key.getModifiers().isCommandDown())
    {
        startChannelSelection(ChannelSelectionMode::Reverb);
        return true;
    }
    if (key.isKeyCode('M') && !key.getModifiers().isCommandDown())
    {
        tabbedComponent.setCurrentTabIndex(TabIndex::Map);
        return true;
    }
    if (key.isKeyCode('N') && !key.getModifiers().isCommandDown())
    {
        tabbedComponent.setCurrentTabIndex(TabIndex::Network);
        return true;
    }
    if (key.isKeyCode('C') && !key.getModifiers().isCommandDown())
    {
        tabbedComponent.setCurrentTabIndex(TabIndex::Clusters);
        return true;
    }

    // Help card cycling: H key
    // Get current tab index for tab-specific shortcuts
    int currentTabIndex = tabbedComponent.getCurrentTabIndex();

    // Clusters tab: Space cycles clusters, not channels
    if (currentTabIndex == TabIndex::Clusters && clustersTab != nullptr && key.isKeyCode(juce::KeyPress::spaceKey))
    {
        if (key.getModifiers().isShiftDown())
            clustersTab->selectPreviousCluster();  // Shift+Space = previous cluster
        else
            clustersTab->selectNextCluster();      // Space = next cluster
        return true;
    }

    // Channel cycling: Space / Shift+Space (for non-Clusters tabs)
    if (key.isKeyCode(juce::KeyPress::spaceKey))
    {
        cycleChannel(key.getModifiers().isShiftDown() ? -1 : 1);
        return true;
    }

    // Cluster/Array assignment: F1-F10 assign to Cluster/Array 1-10, F11 removes (Single)
    // Inputs tab: F1-F10 = Cluster 1-10, F11 = Single
    if (currentTabIndex == TabIndex::Inputs && inputsTab != nullptr)
    {
        for (int i = 0; i < 10; ++i)
        {
            if (key.isKeyCode(juce::KeyPress::F1Key + i))
            {
                inputsTab->setCluster(i + 1);  // F1 = Cluster 1, F10 = Cluster 10
                return true;
            }
        }
        if (key.isKeyCode(juce::KeyPress::F11Key))
        {
            inputsTab->setCluster(0);  // Single
            return true;
        }
    }

    // Outputs tab: F1-F10 = Array 1-10, F11 = Single
    if (currentTabIndex == TabIndex::Outputs && outputsTab != nullptr)
    {
        for (int i = 0; i < 10; ++i)
        {
            if (key.isKeyCode(juce::KeyPress::F1Key + i))
            {
                outputsTab->setArray(i + 1);  // F1 = Array 1, F10 = Array 10
                return true;
            }
        }
        if (key.isKeyCode(juce::KeyPress::F11Key))
        {
            outputsTab->setArray(0);  // Single
            return true;
        }
    }

    // Effects tab: F1-F8 = link group 1-8, F11 = unlinked. The effects' own
    // groups, not the input clusters.
    if (currentTabIndex == TabIndex::Effects && effectsTab != nullptr)
    {
        for (int i = 0; i < WFSParameterDefaults::effectLinkGroupMax; ++i)
        {
            if (key.isKeyCode(juce::KeyPress::F1Key + i))
            {
                effectsTab->setLinkGroup(i + 1);
                return true;
            }
        }
        if (key.isKeyCode(juce::KeyPress::F11Key))
        {
            effectsTab->setLinkGroup(0);
            return true;
        }
    }

    // Clusters tab: F1-F10 = select Cluster 1-10
    if (currentTabIndex == TabIndex::Clusters && clustersTab != nullptr)
    {
        for (int i = 0; i < 10; ++i)
        {
            if (key.isKeyCode(juce::KeyPress::F1Key + i))
            {
                clustersTab->setSelectedCluster(i + 1);  // F1 = Cluster 1, F10 = Cluster 10
                return true;
            }
        }
    }

    // Map tab: F1-F10 = assign selected inputs to Cluster 1-10
    //          F11 = remove from cluster (inputs) or break up cluster (barycenter)
    if (currentTabIndex == TabIndex::Map && mapTab != nullptr)
    {
        for (int i = 0; i < 10; ++i)
        {
            if (key.isKeyCode(juce::KeyPress::F1Key + i))
            {
                if (mapTab->assignSelectedInputsToCluster(i + 1))
                {
                    if (statusBar != nullptr)
                    {
                        int count = mapTab->getMultiSelectionCount();
                        if (count == 1)
                            statusBar->showTemporaryMessage(
                                LOC("map.messages.assignedCluster")
                                    .replace("{channel}", juce::String(*mapTab->getSelectedInputSet().begin() + 1))
                                    .replace("{cluster}", juce::String(i + 1)), 2000);
                        else if (count > 1)
                            statusBar->showTemporaryMessage(
                                LOC("map.messages.assignedClusterMulti")
                                    .replace("{count}", juce::String(count))
                                    .replace("{cluster}", juce::String(i + 1)), 2000);
                    }
                }
                return true;
            }
        }

        if (key.isKeyCode(juce::KeyPress::F11Key))
        {
            if (mapTab->getSelectedBarycenter() >= 1)
            {
                int cluster = mapTab->getSelectedBarycenter();
                if (mapTab->breakUpSelectedCluster())
                {
                    if (statusBar != nullptr)
                        statusBar->showTemporaryMessage(
                            LOC("map.messages.clusterBrokenUp")
                                .replace("{cluster}", juce::String(cluster)), 2000);
                }
            }
            else if (!mapTab->getSelectedInputSet().empty())
            {
                int count = mapTab->getMultiSelectionCount();
                if (mapTab->removeSelectedInputsFromCluster())
                {
                    if (statusBar != nullptr)
                    {
                        if (count == 1)
                            statusBar->showTemporaryMessage(
                                LOC("map.messages.setSingle")
                                    .replace("{channel}", juce::String(*mapTab->getSelectedInputSet().begin() + 1)), 2000);
                        else
                            statusBar->showTemporaryMessage(
                                LOC("map.messages.setSingleMulti")
                                    .replace("{count}", juce::String(count)), 2000);
                    }
                }
            }
            return true;
        }
    }

    // Position nudging: Arrow keys, Page Up/Down (Inputs, Outputs, Reverb tabs)
    const float nudgeAmount = 0.1f;

    // Inputs tab
    if (currentTabIndex == TabIndex::Inputs)
    {
        if (key.isKeyCode(juce::KeyPress::leftKey))
        {
            nudgeInputPosition(0, -nudgeAmount);  // X-
            return true;
        }
        if (key.isKeyCode(juce::KeyPress::rightKey))
        {
            nudgeInputPosition(0, nudgeAmount);   // X+
            return true;
        }
        if (key.isKeyCode(juce::KeyPress::upKey))
        {
            nudgeInputPosition(1, nudgeAmount);   // Y+ (depth)
            return true;
        }
        if (key.isKeyCode(juce::KeyPress::downKey))
        {
            nudgeInputPosition(1, -nudgeAmount);  // Y- (depth)
            return true;
        }
        if (key.isKeyCode(juce::KeyPress::pageUpKey))
        {
            nudgeInputPosition(2, nudgeAmount);   // Z+ (height)
            return true;
        }
        if (key.isKeyCode(juce::KeyPress::pageDownKey))
        {
            nudgeInputPosition(2, -nudgeAmount);  // Z- (height)
            return true;
        }
    }

    // Outputs tab
    if (currentTabIndex == TabIndex::Outputs)
    {
        if (key.isKeyCode(juce::KeyPress::leftKey))
        {
            nudgeOutputPosition(0, -nudgeAmount);  // X-
            return true;
        }
        if (key.isKeyCode(juce::KeyPress::rightKey))
        {
            nudgeOutputPosition(0, nudgeAmount);   // X+
            return true;
        }
        if (key.isKeyCode(juce::KeyPress::upKey))
        {
            nudgeOutputPosition(1, nudgeAmount);   // Y+ (depth)
            return true;
        }
        if (key.isKeyCode(juce::KeyPress::downKey))
        {
            nudgeOutputPosition(1, -nudgeAmount);  // Y- (depth)
            return true;
        }
        if (key.isKeyCode(juce::KeyPress::pageUpKey))
        {
            nudgeOutputPosition(2, nudgeAmount);   // Z+ (height)
            return true;
        }
        if (key.isKeyCode(juce::KeyPress::pageDownKey))
        {
            nudgeOutputPosition(2, -nudgeAmount);  // Z- (height)
            return true;
        }
    }

    // Reverb tab
    if (currentTabIndex == TabIndex::Reverb)
    {
        if (key.isKeyCode(juce::KeyPress::leftKey))
        {
            nudgeReverbPosition(0, -nudgeAmount);  // X-
            return true;
        }
        if (key.isKeyCode(juce::KeyPress::rightKey))
        {
            nudgeReverbPosition(0, nudgeAmount);   // X+
            return true;
        }
        if (key.isKeyCode(juce::KeyPress::upKey))
        {
            nudgeReverbPosition(1, nudgeAmount);   // Y+ (depth)
            return true;
        }
        if (key.isKeyCode(juce::KeyPress::downKey))
        {
            nudgeReverbPosition(1, -nudgeAmount);  // Y- (depth)
            return true;
        }
        if (key.isKeyCode(juce::KeyPress::pageUpKey))
        {
            nudgeReverbPosition(2, nudgeAmount);   // Z+ (height)
            return true;
        }
        if (key.isKeyCode(juce::KeyPress::pageDownKey))
        {
            nudgeReverbPosition(2, -nudgeAmount);  // Z- (height)
            return true;
        }
    }

    // Map tab - nudge selected input
    if (currentTabIndex == TabIndex::Map && mapTab != nullptr)
    {
        int selectedInput = mapTab->getSelectedInput();
        if (selectedInput >= 0)
        {
            bool nudged = false;
            if (key.isKeyCode(juce::KeyPress::leftKey))
            {
                nudgeInputPosition(0, -nudgeAmount, selectedInput);  // X-
                nudged = true;
            }
            else if (key.isKeyCode(juce::KeyPress::rightKey))
            {
                nudgeInputPosition(0, nudgeAmount, selectedInput);   // X+
                nudged = true;
            }
            else if (key.isKeyCode(juce::KeyPress::upKey))
            {
                nudgeInputPosition(1, nudgeAmount, selectedInput);   // Y+ (depth)
                nudged = true;
            }
            else if (key.isKeyCode(juce::KeyPress::downKey))
            {
                nudgeInputPosition(1, -nudgeAmount, selectedInput);  // Y- (depth)
                nudged = true;
            }
            else if (key.isKeyCode(juce::KeyPress::pageUpKey))
            {
                nudgeInputPosition(2, nudgeAmount, selectedInput);   // Z+ (height)
                nudged = true;
            }
            else if (key.isKeyCode(juce::KeyPress::pageDownKey))
            {
                nudgeInputPosition(2, -nudgeAmount, selectedInput);  // Z- (height)
                nudged = true;
            }

            if (nudged)
            {
                mapTab->repaint();  // Trigger visual update
                return true;
            }
        }
    }

    return false;
}

//==============================================================================
// Gradient Map Support
//==============================================================================

void MainComponent::updateGradientMapStageBounds()
{
    using namespace WFSParameterIDs;
    using namespace WFSParameterDefaults;

    auto stageTree = parameters.getValueTreeState().getStageState();
    if (! stageTree.isValid())
        return;

    int shape = stageTree.getProperty (stageShape, stageShapeDefault);
    float originW = stageTree.getProperty (originWidth, originWidthDefault);
    float originD = stageTree.getProperty (originDepth, originDepthDefault);

    float minX, maxX, minY, maxY;

    if (shape == 0)  // Box
    {
        float w = stageTree.getProperty (stageWidth, stageWidthDefault);
        float d = stageTree.getProperty (stageDepth, stageDepthDefault);
        float halfW = w * 0.5f;
        float halfD = d * 0.5f;
        minX = -halfW - originW;
        maxX =  halfW - originW;
        minY = -halfD - originD;
        maxY =  halfD - originD;
    }
    else  // Cylinder / Dome
    {
        float diam = stageTree.getProperty (stageDiameter, stageDiameterDefault);
        float halfD = diam * 0.5f;
        minX = -halfD - originW;
        maxX =  halfD - originW;
        minY = -halfD - originD;
        maxY =  halfD - originD;
    }

    for (auto& evaluator : gradientMapEvaluators)
        evaluator->setStageBounds (minX, maxX, minY, maxY);
}

void MainComponent::rebuildGradientMapForInput (int channelIndex)
{
    if (channelIndex < 0 || channelIndex >= static_cast<int> (gradientMapEvaluators.size()))
        return;

    auto& vts = parameters.getValueTreeState();
    auto gmTree = vts.getInputGradientMapsSection (channelIndex);
    if (! gmTree.isValid())
        return;

    auto map = GradientMap::InputGradientMap::fromValueTree (gmTree);
    gradientMapEvaluators[static_cast<size_t> (channelIndex)]->rasterizeAll (map);
}

void MainComponent::repaintActiveTab()
{
    int tabIndex = tabbedComponent.getCurrentTabIndex();
    if (auto* tab = tabbedComponent.getTabContentComponent(tabIndex))
        tab->repaint();
}

void MainComponent::rebuildAllGradientMaps()
{
    updateGradientMapStageBounds();
    for (int i = 0; i < static_cast<int> (gradientMapEvaluators.size()); ++i)
        rebuildGradientMapForInput (i);
}

//==============================================================================
// Binaural SOFA (HRTF) set management

juce::File MainComponent::resolveBinauralSofaFile (const juce::String& fileName, double sampleRate) const
{
    // User file: relative name inside <project>/sofa (IR/sampler pattern).
    if (fileName.isNotEmpty())
        return parameters.getFileManager().getSofaFolder().getChildFile (fileName);

    // Built-in SADIE II KU100 set: one 48 kHz file (repacked float64 +
    // shuffle + deflate, ~11 MB — see tools/repack_sofa.py); libmysofa
    // resamples it at load for other device rates. Deployed builds have it
    // beside the exe (post-build copy of assets/SOFA); dev builds fall back
    // to the repo's assets/SOFA next to Resources/.
    juce::ignoreUnused (sampleRate);
    const char* builtin = "D1_48K_24bit_256tap_FIR_SOFA.sofa";

    auto resourceDir = LocalizationManager::getInstance().getResourceDirectory();
    auto sofaDir = resourceDir.getChildFile ("SOFA");
    if (! sofaDir.getChildFile (builtin).existsAsFile())
        sofaDir = resourceDir.getParentDirectory().getChildFile ("assets").getChildFile ("SOFA");
    return sofaDir.getChildFile (builtin);
}

void MainComponent::updateBinauralSofaSet()
{
    auto& engine = binauralProcessor->getHrtfEngine();

    // Give back any HRIR sets the render worker retired (release here, never
    // on the RT thread).
    engine.collectRetiredSofaSets();

    auto binauralState = parameters.getValueTreeState().getBinauralState();
    auto* device = deviceManager.getCurrentAudioDevice();
    const int renderMode = binauralState.isValid()
        ? juce::jlimit (WFSParameterDefaults::binauralRenderModeMin,
                        WFSParameterDefaults::binauralRenderModeMax,
                        (int) binauralState.getProperty (WFSParameterIDs::binauralRenderMode,
                                                         WFSParameterDefaults::binauralRenderModeDefault))
        : 0;

    if (renderMode != 2 || device == nullptr)
    {
        if (lastPushedSofaKey.isNotEmpty())
        {
            engine.publishSofaSet (nullptr);
            lastPushedSofaKey.clear();
        }
        return;
    }

    const double sampleRate = device->getCurrentSampleRate();
    const int blockSize = binauralProcessor->getCurrentBlockSize();
    const juce::String fileName = binauralState.getProperty (WFSParameterIDs::binauralSofaFile, "").toString();
    const juce::File sofaFile = resolveBinauralSofaFile (fileName, sampleRate);
    const juce::String key = sofaFile.getFullPathName() + "|" + juce::String (sampleRate, 0)
                           + "|" + juce::String (blockSize);

    if (key == lastPushedSofaKey || sofaLoadInProgress.load (std::memory_order_acquire))
        return;

    // Key set before the load starts: a failed load logs and keeps the
    // previous set rather than retrying every tick.
    lastPushedSofaKey = key;
    sofaLoadInProgress.store (true, std::memory_order_release);

    auto alive = sofaLoadAlive;
    juce::Thread::launch ([this, alive, sofaFile, sampleRate, blockSize]
    {
        auto loaded = spatcore::binaural::sofa::loadSofaFile (sofaFile, sampleRate);
        std::shared_ptr<const spatcore::binaural::CookedHrirSet> cooked;
        if (loaded.database != nullptr)
            cooked = spatcore::binaural::cookHrirSet (loaded.database, blockSize);

        juce::MessageManager::callAsync ([this, alive, cooked, status = loaded.status]
        {
            if (! *alive)
                return;
            sofaLoadInProgress.store (false, std::memory_order_release);
            if (cooked != nullptr && binauralProcessor != nullptr)
            {
                binauralProcessor->getHrtfEngine().publishSofaSet (cooked);
                WFSLogger::getInstance().logInfo ("Binaural SOFA loaded: " + status);
            }
            else
            {
                // Keep whatever set was active (or the structural fallback).
                WFSLogger::getInstance().logWarning ("Binaural SOFA load failed: " + status);
            }
        });
    });
}

