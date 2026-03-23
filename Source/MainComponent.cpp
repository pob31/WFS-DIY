#include "MainComponent.h"
#include "WFSLogger.h"
#include "Parameters/WFSParameterIDs.h"
#include "Localization/LocalizationManager.h"
#include "Accessibility/TTSManager.h"
#include "Network/QLabCueBuilder.h"
#include "Controllers/DialsAndButtons/pages/InputsTabPages.h"
#include "Controllers/DialsAndButtons/pages/GradientMapPages.h"
#include "Controllers/DialsAndButtons/pages/NetworkTabPages.h"
#include "Controllers/DialsAndButtons/pages/OutputsTabPages.h"
#include "Controllers/DialsAndButtons/pages/SystemConfigTabPages.h"
#include "Controllers/DialsAndButtons/pages/MapTabPages.h"
#include "Controllers/DialsAndButtons/pages/ReverbTabPages.h"
#include "Controllers/DialsAndButtons/pages/ClustersTabPages.h"
#include "Controllers/DialsAndButtons/pages/PatchWindowPages.h"
#include "Controllers/PositionControl/SpaceMouseDevice.h"
#include "Controllers/Sampler/LightpadManager.h"

//==============================================================================
MainComponent::MainComponent()
{
    // Initialize localization - try to load language file from Resources/lang/
    auto& locMgr = LocalizationManager::getInstance();
    auto exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();

    // Try multiple locations for Resources folder:
    // 1. Next to executable (production deployment)
    // 2. macOS bundle: Contents/Resources (standard bundle structure)
    // 3. Project root (development from Visual Studio - 5 levels up from exe)
    // 4. Project root (development from macOS - 6 levels up from Contents/MacOS/)
    juce::File resourceDir = exeDir.getChildFile("Resources");

    if (!resourceDir.getChildFile("lang/en.json").existsAsFile())
    {
        // macOS bundle path: exe is at Contents/MacOS/, resources at Contents/Resources/
        resourceDir = exeDir.getParentDirectory().getChildFile("Resources");
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
    if (!locMgr.loadLanguage(savedLanguage))
    {
        // Fall back to English if saved language fails to load
        if (savedLanguage != "en")
            locMgr.loadLanguage("en");
        DBG("LocalizationManager: Could not load " + savedLanguage + ".json - using fallback");
    }

    // Load channel counts (persisted), clamp, and fall back to a usable default
    numInputChannels = props.getIntValue("numInputChannels", 0);
    numOutputChannels = props.getIntValue("numOutputChannels", 2);
    numInputChannels = juce::jlimit(0, 64, numInputChannels);
    numOutputChannels = juce::jlimit(2, 64, numOutputChannels);
    if (numInputChannels == 0)
        numInputChannels = 2; // Default to a sensible input count to avoid silent processing

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
    // else if (algorithmId == 3)
    //     currentAlgorithm = ProcessingAlgorithm::GpuInputBuffer;

    /* Algorithm change handler - no longer needed as UI is in SystemConfigTab
    auto algorithmChangeHandler = [this]() {
        int selectedId = (int)parameters.getConfigParam("ProcessingAlgorithm");
        ProcessingAlgorithm newAlgorithm = currentAlgorithm;
        if (selectedId == 1)
            newAlgorithm = ProcessingAlgorithm::InputBuffer;
        else if (selectedId == 2)
            newAlgorithm = ProcessingAlgorithm::OutputBuffer;
        // else if (selectedId == 3)
        //     newAlgorithm = ProcessingAlgorithm::GpuInputBuffer;

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
                // else  // Commented out - GPU Audio SDK not configured
                // {
                //     gpuInputAlgorithm.releaseResources();
                //     gpuInputAlgorithm.clear();
                // }

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
    inputsTab = new InputsTab(parameters);
    clustersTab = new ClustersTab(parameters);
    reverbTab = new ReverbTab(parameters);
    mapTab = new MapTab(parameters);

    // Set accessible names for screen readers (prevents "Custom" announcement)
    systemConfigTab->setName("System Configuration");
    networkTab->setName("Network");
    outputsTab->setName("Outputs");
    inputsTab->setName("Inputs");
    clustersTab->setName("Clusters");
    reverbTab->setName("Reverb");
    mapTab->setName("Map");

    // Pass status bar to tabs that support it
    systemConfigTab->setStatusBar(statusBar);
    networkTab->setStatusBar(statusBar);
    outputsTab->setStatusBar(statusBar);
    inputsTab->setStatusBar(statusBar);
    reverbTab->setStatusBar(statusBar);
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

    systemConfigTab->setChannelCountCallback([this](int inputs, int outputs, int reverbs) {
        handleChannelCountChange(inputs, outputs, reverbs);
    });

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
        // 0=Off, 1=Stream Deck+, 2=XenceLabs Quick Keys

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
        if (quickKeysManager)
        {
            quickKeysManager->setEnabled (deviceIndex == 2);
            if (deviceIndex == 2)
                quickKeysManager->setActivePage (tabbedComponent.getCurrentTabIndex(), 0);
        }
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

    // Set up callbacks for individual tab config reloads
    outputsTab->onConfigReloaded = [this]() {
        handleConfigReloaded();
    };

    inputsTab->onConfigReloaded = [this]() {
        handleConfigReloaded();
    };

    inputsTab->isQLabAvailable = [this]() {
        return oscManager && oscManager->hasQLabTarget();
    };

    // QLab export callback for InputsTab
    inputsTab->onQLabExportRequested = [this](const juce::String& snapshotName,
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

        if (!inputsData.isValid())
        {
            if (inputsTab != nullptr)
                inputsTab->showStatusMessage ("QLab export: no input data in snapshot");
            return;
        }

        int numChannels = parameters.getNumInputChannels();
        int patchNumber = oscManager->getQLabPatchNumber();
        int cueCount = WFSNetwork::QLabCueBuilder::countCues (inputsData, scope, numChannels);

        if (cueCount == 0)
        {
            if (inputsTab != nullptr)
                inputsTab->showStatusMessage ("QLab export: no parameters in scope");
            return;
        }

        auto sequence = WFSNetwork::QLabCueBuilder::buildSnapshotCues (
            snapshotName, inputsData, scope, numChannels, patchNumber);

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
    inputsTab->onQLabSnapshotLoadCueRequested = [this](const juce::String& snapshotName) {
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
        int ch = inputsTab->getCurrentChannel() - 1;  // 1-based → 0-based
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
        int ch = inputsTab->getCurrentChannel() - 1;
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

    // Create global tooltip window for hover tooltips
    tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 500);

    // Add tabs to tabbed component (using localized names)
    // Store names in local variables to ensure proper String lifetime
    juce::String tabSystemConfig = LOC("tabs.systemConfig");
    juce::String tabNetwork = LOC("tabs.network");
    juce::String tabOutputs = LOC("tabs.outputs");
    juce::String tabReverb = LOC("tabs.reverb");
    juce::String tabInputs = LOC("tabs.inputs");
    juce::String tabClusters = LOC("tabs.clusters");
    juce::String tabMap = LOC("tabs.map");

    tabbedComponent.addTab(tabSystemConfig, ColorScheme::get().chromeBackground, systemConfigTab, true);
    tabbedComponent.addTab(tabNetwork, ColorScheme::get().chromeBackground, networkTab, true);
    tabbedComponent.addTab(tabOutputs, ColorScheme::get().chromeBackground, outputsTab, true);
    tabbedComponent.addTab(tabReverb, ColorScheme::get().chromeBackground, reverbTab, true);
    tabbedComponent.addTab(tabInputs, ColorScheme::get().chromeBackground, inputsTab, true);
    tabbedComponent.addTab(tabClusters, ColorScheme::get().chromeBackground, clustersTab, true);
    tabbedComponent.addTab(tabMap, ColorScheme::get().chromeBackground, mapTab, true);

    // Wire per-tab undo domain: Ctrl+Z only affects the currently focused tab
    tabbedComponent.onTabChanged = [this](int tabIndex) {
        static const UndoDomain domainForTab[] = {
            UndoDomain::Config,   // 0: SystemConfig
            UndoDomain::Config,   // 1: Network
            UndoDomain::Output,   // 2: Outputs
            UndoDomain::Reverb,   // 3: Reverb
            UndoDomain::Input,    // 4: Inputs
            UndoDomain::Clusters, // 5: Clusters
            UndoDomain::Map       // 6: Map
        };
        if (tabIndex >= 0 && tabIndex < 7)
            parameters.getValueTreeState().setActiveDomain (domainForTab[tabIndex]);
        if (controllerManager)
            controllerManager->activeTab = tabIndex;
        if (streamDeckManager)
        {
            // Sync subtab + channel state atomically before page render
            if (tabIndex == 3 && reverbTab != nullptr)
                streamDeckManager->syncNavigation (tabIndex, reverbTab->getCurrentSubTab(), reverbTab->getCurrentChannel());
            else if (tabIndex == 2 && outputsTab != nullptr)
                streamDeckManager->syncNavigation (tabIndex, 0, outputsTab->getCurrentChannel());
            else if (tabIndex == 4 && inputsTab != nullptr)
                streamDeckManager->syncNavigation (tabIndex, 0, inputsTab->getCurrentChannel());
            else
                streamDeckManager->setMainTab (tabIndex);
        }
        if (quickKeysManager)
            quickKeysManager->setActivePage (tabIndex, 0);
    };

    // Load saved color scheme from parameters and apply it
    // This will trigger WfsLookAndFeel::colorSchemeChanged() to update widget colors
    int colorSchemeId = (int)parameters.getConfigParam("ColorScheme");
    ColorScheme::Manager::getInstance().setTheme(colorSchemeId);

    // Subscribe to color scheme changes for component repaints
    ColorScheme::Manager::getInstance().addListener(this);

    // Force initial color refresh: setTheme() was called before addListener(this), so
    // colorSchemeChanged() was never triggered at startup. This ensures TextEditor cached
    // colors match the active theme from the first frame.
    colorSchemeChanged();

    // Set up navigation callback from Map tab to other tabs via long-press gesture
    // Parameters: (tabType, index) where tabType is: 0=Input, 1=Cluster, 2=Output, 3=Reverb
    mapTab->setNavigateToItemCallback([this](int tabType, int index) {
        switch (tabType)
        {
            case 0:  // Input
                tabbedComponent.setCurrentTabIndex(4);  // Inputs tab
                inputsTab->selectChannel(index + 1);    // Convert 0-based to 1-based
                break;
            case 1:  // Cluster
                tabbedComponent.setCurrentTabIndex(5);  // Clusters tab
                clustersTab->setSelectedCluster(index);
                break;
            case 2:  // Output
                tabbedComponent.setCurrentTabIndex(2);  // Outputs tab
                outputsTab->selectChannel(index + 1);   // Convert 0-based to 1-based
                break;
            case 3:  // Reverb
                tabbedComponent.setCurrentTabIndex(3);  // Reverb tab
                reverbTab->selectChannel(index + 1);    // Convert 0-based to 1-based
                break;
        }
    });

    // Initialize OSC Manager for network communication
    oscManager = std::make_unique<WFSNetwork::OSCManager>(parameters.getValueTreeState());

    // Initialize Stream Deck+ physical controller
    streamDeckManager = std::make_unique<StreamDeckManager>();

    // Initialize Xencelabs Quick Keys controller
    quickKeysManager = std::make_unique<QuickKeysManager>();

    // Apply initial Dials & Buttons device selection (default Off)
    {
        int dbDevice = static_cast<int> (parameters.getConfigParam ("DialsAndButtonsDevice"));
        streamDeckManager->setEnabled (dbDevice == 1);
        quickKeysManager->setEnabled (dbDevice == 2);
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
        auto lfoSubModeState  = std::make_shared<int> (0);
        auto outputEqBandState = std::make_shared<int> (0);

        InputsTabPages::MovementCallbacks movCB;
        movCB.startMotion  = [this](int ch) { if (automOtionProcessor) automOtionProcessor->startClusterMotion (ch); };
        movCB.stopMotion   = [this](int ch) { if (automOtionProcessor) automOtionProcessor->stopClusterMotion (ch); };
        movCB.pauseMotion  = [this](int ch) { if (automOtionProcessor) automOtionProcessor->pauseClusterMotion (ch); };
        movCB.resumeMotion = [this](int ch) { if (automOtionProcessor) automOtionProcessor->resumeClusterMotion (ch); };
        movCB.stopAll      = [this]()       { if (automOtionProcessor) automOtionProcessor->stopAllMotion(); };

        for (int subTab = 0; subTab < 5; ++subTab)
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
                    InputsTabPages::createPage (subTab, vts, 0, flipModeState, lfoSubModeState, movCB));
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
                OutputsTabPages::createPage (subTab, vts, 0, outputEqBandState, onEqBandSelectedGui));
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

        // Register Quick Keys System Config page (binaural traversal)
        {
            using namespace WFSParameterIDs;
            using namespace WFSParameterDefaults;

            QuickKeysPage qkSysPage;
            qkSysPage.pageName = "System Config";
            qkSysPage.tabName = "Binaural";
            qkSysPage.sectionName = "Renderer";

            const auto sliderOrange = juce::Colour (0xFFFF5722);
            const auto dialGrey     = juce::Colour (0xFF808080);

            // Distance (short name for 8-char OLED)
            QuickKeysBinding distBinding;
            distBinding.dial = SystemConfigTabPages::makeBinauralFloatDial (
                "Distance", LOC ("units.meters"),
                binauralListenerDistanceMin, binauralListenerDistanceMax,
                0.1f, 0.01f, 2, vts, binauralListenerDistance);
            distBinding.ledColour = sliderOrange;
            qkSysPage.bindings.push_back (std::move (distBinding));

            // Angle
            QuickKeysBinding angleBinding;
            angleBinding.dial = SystemConfigTabPages::makeBinauralIntDial (
                "Angle", LOC ("units.degrees"),
                binauralListenerAngleMin, binauralListenerAngleMax,
                5, 1, vts, binauralListenerAngle);
            angleBinding.ledColour = dialGrey;
            qkSysPage.bindings.push_back (std::move (angleBinding));

            // Level
            QuickKeysBinding levelBinding;
            levelBinding.dial = SystemConfigTabPages::makeBinauralFloatDial (
                "Level", LOC ("units.decibels"),
                binauralAttenuationMin, binauralAttenuationMax,
                0.5f, 0.1f, 1, vts, binauralAttenuation);
            levelBinding.ledColour = sliderOrange;
            qkSysPage.bindings.push_back (std::move (levelBinding));

            // Delay
            QuickKeysBinding delayBinding;
            delayBinding.dial = SystemConfigTabPages::makeBinauralFloatDial (
                "Delay", LOC ("units.milliseconds"),
                binauralDelayMin, binauralDelayMax,
                1.0f, 0.1f, 1, vts, binauralDelay);
            delayBinding.ledColour = sliderOrange;
            qkSysPage.bindings.push_back (std::move (delayBinding));

            quickKeysManager->registerPage (
                SystemConfigTabPages::SYSCONFIG_MAIN_TAB_INDEX, 0,
                std::move (qkSysPage));
        }

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

        // Wire map selection changes to rebuild Stream Deck page
        mapTab->setMapSelectionChangedCallback ([this]()
        {
            if (streamDeckManager && streamDeckManager->getCurrentMainTab() == MapTabPages::MAP_MAIN_TAB_INDEX)
                streamDeckManager->refreshCurrentPage();
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

        // Set page rebuild callback for channel changes and binding swaps
        streamDeckManager->onPageNeedsRebuild = [this, flipModeState, lfoSubModeState, movCB, outputEqBandState, onEqBandSelectedGui, netCB, sysCB, mapCB, mapQ, mapPosOffsetMode, reverbPreEqBandState, reverbPreDynMode, reverbPostEqBandState, reverbPostDynMode, reverbSoloState, reverbMutePreState, reverbMutePostState, reverbEditOnMapState, reverbAlgoSubMode, reverbIRDuration, onSoloReverbSD, onMutePreSD, onMutePostSD, onEditOnMapSD, clusterLfoSubMode, presetCol, presetRow, clusterCB](int mainTab, int subTab, int channel)
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
                    streamDeckManager->registerPage (mainTab, subTab,
                        InputsTabPages::createPage (subTab, vts, channel - 1, flipModeState, lfoSubModeState, movCB));
                }
            }
            else if (mainTab == OutputsTabPages::OUTPUTS_MAIN_TAB_INDEX)
            {
                auto& vts = parameters.getValueTreeState();
                streamDeckManager->registerPage (mainTab, subTab,
                    OutputsTabPages::createPage (subTab, vts, channel - 1, outputEqBandState, onEqBandSelectedGui));
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
                int channel = itemIndex + 1;  // Convert 0-based to 1-based
                switch (tabIndex)
                {
                    case 4:  if (inputsTab)   inputsTab->selectChannel (channel);   break;
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

        controllerManager->callbacks.rotateSelected = [this] (float deltaDeg)
        {
            juce::MessageManager::callAsync ([this, deltaDeg]()
            {
                // Determine which inputs to rotate
                std::set<int> targets;
                if (mapTab)
                    targets = mapTab->getSelectedInputSet();

                // On Inputs tab with no map selection, rotate the current channel
                if (targets.empty() && inputsTab)
                {
                    int ch = inputsTab->getSelectedInputIndex();
                    if (ch >= 0 && ch < parameters.getNumInputChannels())
                        targets.insert (ch);
                }

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

                // Also sync InputsTab channel selector
                if (inputsTab)
                    inputsTab->selectChannel (next + 1);  // 1-based
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
        lightpadManager = std::make_unique<LightpadManager> (parameters);

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

    // Initialize Binaural Solo Monitoring
    binauralCalcEngine = std::make_unique<BinauralCalculationEngine>(
        parameters.getValueTreeState(), *calculationEngine);
    binauralProcessor = std::make_unique<BinauralProcessor>(*binauralCalcEngine);

    // Initialize Reverb Engine
    reverbEngine = std::make_unique<ReverbEngine>();

    // Initialize LFO Processor for input position modulation
    lfoProcessor = std::make_unique<LFOProcessor>(parameters.getValueTreeState(), 64);

    // Initialize AutomOtion Processor for programmed input position movement
    automOtionProcessor = std::make_unique<AutomOtionProcessor>(parameters.getValueTreeState(), 64);

    // Initialize Input Speed Limiter for smooth position movement
    speedLimiter = std::make_unique<InputSpeedLimiter>();
    speedLimiter->resize(WFSParameterDefaults::maxInputChannels);

    // Pass AutomOtionProcessor to InputsTab for UI control
    if (inputsTab != nullptr)
        inputsTab->setAutoMotionProcessor(automOtionProcessor.get());

    // Initialize Live Source Tamer engine for per-speaker gain reduction
    // Uses max channel counts to match calculationEngine matrix dimensions
    lsTamerEngine = std::make_unique<LiveSourceTamerEngine>(
        parameters.getValueTreeState(),
        *calculationEngine,
        WFSParameterDefaults::maxInputChannels,
        WFSParameterDefaults::maxOutputChannels);

    // Initialize Test Signal Generator for audio interface testing
    testSignalGenerator = std::make_unique<TestSignalGenerator>();
    calculationEngine->setLSGainsPtr(lsTamerEngine->getLSGains());

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

        // Set up speed-limited position callback for MapTab visualization
        mapTab->setSpeedLimitedPositionCallback([this](int inputIndex, float& x, float& y, float& z) {
            if (speedLimiter != nullptr)
            {
                speedLimiter->getPosition(inputIndex, x, y, z);
            }
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

    // Configure OSC Manager with initial network settings from parameters
    WFSNetwork::GlobalConfig oscGlobalConfig;
    oscGlobalConfig.udpReceivePort = (int)parameters.getConfigParam("NetworkRxUDPport");
    oscGlobalConfig.tcpReceivePort = (int)parameters.getConfigParam("NetworkRxTCPport");
    oscManager->applyGlobalConfig(oscGlobalConfig);

    // Pass OSCManager to NetworkTab for UI integration
    networkTab->setOSCManager(oscManager.get());

    // Set up NetworkLogWindow callback
    networkTab->setNetworkLogWindowCallback([this]() {
        openNetworkLogWindow();
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
    };

    // Activate/deactivate sampler channel in the audio engine
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

    // Sync StreamDeck to InputsTab's initial channel (1-indexed)
    if (streamDeckManager)
        streamDeckManager->setChannel (1);

    // Connect OutputsTab channel and subtab selection to StreamDeck
    outputsTab->onChannelSelected = [this](int channelId)
    {
        if (streamDeckManager && tabbedComponent.getCurrentTabIndex() == 2)
            streamDeckManager->setChannel (channelId);
    };

    outputsTab->onSubTabChanged = [this](int subTabIndex)
    {
        if (streamDeckManager && tabbedComponent.getCurrentTabIndex() == 2)
            streamDeckManager->setSubTab (subTabIndex);
    };

    // Connect ReverbTab channel and subtab selection to StreamDeck
    reverbTab->onChannelSelected = [this](int channelId)
    {
        if (streamDeckManager && tabbedComponent.getCurrentTabIndex() == 3)
            streamDeckManager->setChannel (channelId);
    };

    reverbTab->onSubTabChanged = [this](int subTabIndex)
    {
        if (streamDeckManager && tabbedComponent.getCurrentTabIndex() == 3)
            streamDeckManager->setSubTab (subTabIndex);
    };

    // Snapshot OSC command callbacks
    oscManager->onSnapshotLoadRequested = [this](const juce::String& snapshotName) {
        auto& fileManager = parameters.getFileManager();
        if (!fileManager.hasValidProjectFolder())
        {
            DBG ("OSC snapshot/load: no project folder configured");
            return;
        }

        auto names = fileManager.getInputSnapshotNames();
        if (!names.contains (snapshotName))
        {
            DBG ("OSC snapshot/load: snapshot not found: " << snapshotName);
            if (inputsTab != nullptr)
                inputsTab->showStatusMessage ("Snapshot not found: " + snapshotName);
            return;
        }

        auto scope = fileManager.getExtendedSnapshotScope (snapshotName);

        parameters.getDirtyTracker().beginSuppression();

        if (fileManager.loadInputSnapshotWithExtendedScope (snapshotName, scope))
        {
            if (inputsTab != nullptr)
            {
                inputsTab->refreshFromState();
                inputsTab->showStatusMessage (
                    LOC("inputs.messages.snapshotLoaded").replace ("{name}", snapshotName));
            }
            handleConfigReloaded();
        }
        else
        {
            DBG ("OSC snapshot/load: failed to load: " << fileManager.getLastError());
        }

        parameters.getDirtyTracker().endSuppressionAndClear();
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
                inputsTab->refreshSnapshotSelector();
                inputsTab->showStatusMessage (
                    LOC("inputs.messages.snapshotUpdated").replace ("{name}", snapshotName));
            }
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

        bool maxSpeedActive = maxSpeedActiveVar.isInt() ? (static_cast<int>(maxSpeedActiveVar) != 0) : false;
        bool pathModeActive = pathModeActiveVar.isInt() ? (static_cast<int>(pathModeActiveVar) != 0) : false;

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
        // Check controller mode is Remote
        int ctrlMode = static_cast<int> (parameters.getConfigParam ("SamplerControllerMode"));
        if (ctrlMode != 2) return;

        // Look up zone → input
        auto zoneMap = buildZoneToInputMap();
        auto it = zoneMap.find (zoneId);
        if (it == zoneMap.end()) return;
        int inputIdx = it->second;

        if (samplerManager == nullptr || ! samplerManager->isChannelActive (inputIdx))
            return;

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

    // Send composite deltas for all inputs when a Remote client connects and initial data has been sent
    oscManager->onRemoteConnectionReady = [this](int /*targetIndex*/)
    {
        if (calculationEngine == nullptr || oscManager == nullptr)
            return;

        // Get number of input channels
        int numInputChannels = parameters.getNumInputChannels();

        constexpr float deltaThreshold = 0.01f;  // 1cm threshold for considering delta significant

        // Send composite delta for each input
        for (int i = 0; i < numInputChannels; ++i)
        {
            int channelId = i + 1;  // 1-based for OSC messages

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
                                      parameters.getNumReverbChannels());

    // Make sure you set the size of the component after
    // you add any child components.
    // Set initial size to 90% of the screen, with minimum bounds
    auto displayArea = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
    int windowWidth = juce::jmax(1280, (int)(displayArea.getWidth() * 0.9f));
    int windowHeight = juce::jmax(720, (int)(displayArea.getHeight() * 0.9f));
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
                // Use initialise() with saved state - this is faster than manual setup
                // Pass selectDefaultDeviceOnFailure=false to avoid scanning if saved device unavailable
                auto error = deviceManager.initialise(256, 256, savedStateXml.get(), false);

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
            deviceManager.setCurrentAudioDeviceType(savedDeviceType, true);

            juce::AudioDeviceManager::AudioDeviceSetup setup;
            deviceManager.getAudioDeviceSetup(setup);
            setup.outputDeviceName = savedDeviceName;
            setup.inputDeviceName = savedDeviceName;
            setup.useDefaultInputChannels = false;
            setup.useDefaultOutputChannels = false;
            setup.inputChannels.setRange(0, 256, true);
            setup.outputChannels.setRange(0, 256, true);

            auto error = deviceManager.setAudioDeviceSetup(setup, true);
            deviceRestored = error.isEmpty();

            if (deviceRestored)
            {
                // Enable all available channels from the device
                auto* device = deviceManager.getCurrentAudioDevice();
                if (device != nullptr)
                {
                    auto inputNames = device->getInputChannelNames();
                    auto outputNames = device->getOutputChannelNames();

                    deviceManager.getAudioDeviceSetup(setup);
                    setup.inputChannels.clear();
                    setup.inputChannels.setRange(0, inputNames.size(), true);
                    setup.outputChannels.clear();
                    setup.outputChannels.setRange(0, outputNames.size(), true);
                    deviceManager.setAudioDeviceSetup(setup, true);

                    lastSavedDeviceType = savedDeviceType;
                    lastSavedDeviceName = savedDeviceName;
                }
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

    // Start timer for device monitoring and parameter smoothing
    startTimer(5); // 5ms timer for smooth parameter updates

    // Listen for device manager changes to re-attach audio callbacks when device changes
    deviceManager.addChangeListener(this);
}

MainComponent::~MainComponent()
{
    WFSLogger::getInstance().logInfo ("Session ending - saving settings");

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

    // Also save system config (includes audio patch) to project folder
    auto& fileManager = parameters.getFileManager();
    if (fileManager.hasValidProjectFolder())
        fileManager.saveSystemConfig();

    // Clean up status bar (owned by this component, not TabbedComponent)
    delete statusBar;

    // Stop all processing threads BEFORE shutting down audio device
    // (prevents threads from accessing device state during ASIO teardown)
    if (reverbFeedThread)
    {
        reverbFeedThread->stopThread(1000);
        reverbFeedThread.reset();
    }
    inputAlgorithm.releaseResources();
    outputAlgorithm.releaseResources();

    // Shutdown audio device (stops audio callbacks)
    shutdownAudio();

    // Now safe to destroy processor objects
    inputAlgorithm.clear();
    outputAlgorithm.clear();
    // gpuInputAlgorithm.clear();  // Commented out - GPU Audio SDK not configured
}

//==============================================================================
void MainComponent::attachAudioCallbacksIfNeeded()
{
    if (audioCallbacksAttached)
        return;

    if (deviceManager.getCurrentAudioDevice() == nullptr)
        return;

    auto setup = deviceManager.getAudioDeviceSetup();
    const int numInputs = setup.inputChannels.countNumberOfSetBits();
    const int numOutputs = setup.outputChannels.countNumberOfSetBits();

    // Preserve the user's current device and channel selection when wiring callbacks
    std::unique_ptr<juce::XmlElement> state(deviceManager.createStateXml());
    setAudioChannels(numInputs, numOutputs, state.get());
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

            // Update patch matrix hardware channel count from actual device
            auto hwInputs = device->getInputChannelNames().size();
            auto hwOutputs = device->getOutputChannelNames().size();
            parameters.updateHardwareChannelCount (hwInputs, hwOutputs);

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

void MainComponent::resizeRoutingMatrices()
{
    const int matrixSize = numInputChannels * numOutputChannels;

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
    // else  // Commented out - GPU Audio SDK not configured
    // {
    //     gpuInputAlgorithm.releaseResources();
    //     gpuInputAlgorithm.clear();
    // }

    // Clear shared buffer references from consumers before destroying buffers
    if (binauralProcessor)
        binauralProcessor->clearSharedInputBuffers();

    // Stop reverb feed thread and engine for reconfiguration
    if (reverbFeedThread)
    {
        reverbFeedThread->stopThread(1000);
        reverbFeedThread.reset();
    }
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

    // Grow: append new rows with 1:1 diagonal mapping
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

void MainComponent::loadAudioPatches()
{
    // Load input patch matrix from ValueTree
    auto audioPatchTree = parameters.getValueTreeState().getState().getChildWithName(WFSParameterIDs::AudioPatch);
    auto inputPatchTree = audioPatchTree.getChildWithName(WFSParameterIDs::InputPatch);
    auto outputPatchTree = audioPatchTree.getChildWithName(WFSParameterIDs::OutputPatch);

    // Reset patch maps to "unmapped" (-1)
    inputPatchMap.assign(64, -1);  // Max hardware inputs
    outputPatchMap.assign(64, -1); // Max WFS outputs

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

}

void MainComponent::applyInputPatch(const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Apply input patching: remap hardware inputs to WFS inputs
    int totalBufferChannels = bufferToFill.buffer->getNumChannels();

    // Prepare patched buffer if needed
    if (patchedInputBuffer.getNumChannels() != numInputChannels ||
        patchedInputBuffer.getNumSamples() < bufferToFill.numSamples)
    {
        patchedInputBuffer.setSize(numInputChannels, bufferToFill.numSamples, false, false, true);
    }

    patchedInputBuffer.clear();

    // Copy audio according to input patch map
    for (int hwChannel = 0; hwChannel < totalBufferChannels && hwChannel < (int)inputPatchMap.size(); ++hwChannel)
    {
        int wfsChannel = inputPatchMap[hwChannel];
        if (wfsChannel >= 0 && wfsChannel < numInputChannels)
        {
            patchedInputBuffer.copyFrom(wfsChannel, bufferToFill.startSample,
                                        *bufferToFill.buffer, hwChannel,
                                        bufferToFill.startSample, bufferToFill.numSamples);
        }
    }

    // No copy-back: downstream consumers read directly from patchedInputBuffer
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

        // Stop reverb engine thread to save CPU
        if (reverbEngine)
            reverbEngine->stopProcessing();

        // Switch binaural to private ring buffers so binaural-only path can use pushInput
        if (binauralProcessor)
            binauralProcessor->clearSharedInputBuffers();
    }
}

void MainComponent::handleChannelCountChange(int inputs, int outputs, int reverbs)
{
    WFSLogger::getInstance().logInfo ("Channel count changed: " + juce::String (inputs) + " inputs, "
                                      + juce::String (outputs) + " outputs, "
                                      + juce::String (reverbs) + " reverbs");
    numInputChannels = inputs;
    numOutputChannels = outputs;
    stopProcessingForConfigurationChange();
    resizeRoutingMatrices();

    // Grow/shrink patch data in ValueTree so new channels get 1:1 mapping
    // (PatchMatrixComponent may not exist if Audio Interface window was never opened)
    {
        auto audioPatchTree = parameters.getValueTreeState().getState().getChildWithName(WFSParameterIDs::AudioPatch);
        auto inputPatchTree = audioPatchTree.getChildWithName(WFSParameterIDs::InputPatch);
        auto outputPatchTree = audioPatchTree.getChildWithName(WFSParameterIDs::OutputPatch);
        int hwInCols = inputPatchTree.isValid() ? (int) inputPatchTree.getProperty(WFSParameterIDs::cols, 64) : 64;
        int hwOutCols = outputPatchTree.isValid() ? (int) outputPatchTree.getProperty(WFSParameterIDs::cols, 64) : 64;
        growPatchData(inputPatchTree, inputs, hwInCols);
        growPatchData(outputPatchTree, outputs, hwOutCols);
    }

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
    }

    // Refresh all tabs to update channel selectors
    if (inputsTab != nullptr)
    {
        inputsTab->refreshFromValueTree();
        inputsTab->configureVisualisation(outputs, reverbs);
    }
    if (outputsTab != nullptr)
        outputsTab->refreshFromValueTree();
    if (reverbTab != nullptr)
        reverbTab->refreshFromValueTree();

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
        binauralProcessor->prepareToPlay(sr, bs, numInputChannels);
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
        calculationEngine->recalculateMatrix();

        const float* calcDelays = calculationEngine->getDelayTimesMs();
        const float* calcLevels = calculationEngine->getLevels();
        const float* calcHF = calculationEngine->getHFAttenuationDb();
        const int calcStride = calculationEngine->getNumOutputs();

        for (int inIdx = 0; inIdx < numInputChannels; ++inIdx)
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

        std::vector<float> reverbDelays(numInputChannels * reverbs);
        std::vector<float> reverbLevelsVec(numInputChannels * reverbs);
        std::vector<float> reverbHF(numInputChannels * reverbs);

        for (int inIdx = 0; inIdx < numInputChannels; ++inIdx)
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
            inputsTab->updateVisualisation(
                targetDelayTimesMs.data(), targetLevels.data(), hfAttenuation.data(),
                reverbDelays.data(), reverbLevelsVec.data(), reverbHF.data());
        }
    }

    // Reload patch maps from the (now up-to-date) ValueTree
    loadAudioPatches();
}

void MainComponent::handleConfigReloaded()
{
    WFSLogger::getInstance().logInfo ("Configuration reloaded");

    // Update local channel counts from newly loaded config
    int newInputChannels = parameters.getNumInputChannels();
    int newOutputChannels = parameters.getNumOutputChannels();
    if (newInputChannels != numInputChannels || newOutputChannels != numOutputChannels)
    {
        numInputChannels = newInputChannels;
        numOutputChannels = newOutputChannels;
        resizeRoutingMatrices();

        // Update level meter channel counts
        if (levelMeteringManager != nullptr)
            levelMeteringManager->setChannelCounts(newInputChannels, newOutputChannels);
        if (levelMeterWindow != nullptr)
            levelMeterWindow->rebuildMeters();
    }

    // Reload audio patches from ValueTree (input/output channel routing)
    loadAudioPatches();

    // Refresh all tabs to show newly loaded config data
    if (networkTab != nullptr)
        networkTab->refreshFromValueTree();

    if (inputsTab != nullptr)
        inputsTab->refreshFromValueTree();

    if (outputsTab != nullptr)
        outputsTab->refreshFromValueTree();

    if (reverbTab != nullptr)
        reverbTab->refreshFromValueTree();

    if (mapTab != nullptr)
        mapTab->repaint();

    if (clustersTab != nullptr)
        clustersTab->repaint();

    // Reconfigure visualization with potentially changed channel counts
    if (inputsTab != nullptr)
    {
        // Ensure the selected channel is valid for the new input count
        int currentChannel = inputsTab->getCurrentChannel();
        if (currentChannel < 1 || currentChannel > numInputChannels)
        {
            inputsTab->selectChannel(1);  // Reset to channel 1 if out of range
        }

        inputsTab->configureVisualisation(parameters.getNumOutputChannels(),
                                          parameters.getNumReverbChannels());

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
        calculationEngine->recalculateMatrix();

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
            for (int inIdx = 0; inIdx < numInputChannels; ++inIdx)
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

            std::vector<float> reverbDelays(numInputChannels * numReverbs);
            std::vector<float> reverbLevels(numInputChannels * numReverbs);
            std::vector<float> reverbHF(numInputChannels * numReverbs);

            for (int inIdx = 0; inIdx < numInputChannels; ++inIdx)
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

            inputsTab->updateVisualisation(
                targetDelayTimesMs.data(), targetLevels.data(), hfAttenuation.data(),
                reverbDelays.data(), reverbLevels.data(), reverbHF.data());
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
        if (quickKeysManager)
        {
            quickKeysManager->setEnabled (dbDevice == 2);
            if (dbDevice == 2)
                quickKeysManager->setActivePage (tabbedComponent.getCurrentTabIndex(), 0);
        }
    }

    // Flush all pending OSC messages immediately after loading
    // This ensures all parameter changes are broadcast to OSC targets
    if (oscManager != nullptr)
    {
        oscManager->flushMessages();

        // Resend full state to connected Remote (Android) targets
        // Ensures input count, positions, and stage config are synchronized after reload
        oscManager->resendStateToRemoteTargets();
    }
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

void MainComponent::openAudioInterfaceWindow()
{
    if (audioInterfaceWindow == nullptr)
    {
        audioInterfaceWindow = std::make_unique<AudioInterfaceWindow>(
            deviceManager,
            parameters.getValueTreeState(),
            testSignalGenerator.get()
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

void MainComponent::openLevelMeterWindow()
{
    if (levelMeterWindow == nullptr)
    {
        if (levelMeteringManager == nullptr)
            return;

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

    // Step 1: Set Input Channels
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

//==============================================================================
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

    WFSLogger::getInstance().logInfo ("Starting audio engine: " + device->getName()
                                      + " @ " + juce::String (sampleRate) + " Hz"
                                      + ", buffer " + juce::String (blockSize)
                                      + ", " + juce::String (numInputChannels) + " in / "
                                      + juce::String (numOutputChannels) + " out");

    DBG("startAudioEngine: numInputChannels=" + juce::String(numInputChannels) +
        " numOutputChannels=" + juce::String(numOutputChannels) +
        " sampleRate=" + juce::String(sampleRate) + " blockSize=" + juce::String(blockSize));

    bool prepared = false;
    DBG("startAudioEngine: algorithm=" + juce::String(currentAlgorithm == ProcessingAlgorithm::InputBuffer ? "InputBuffer" :
        currentAlgorithm == ProcessingAlgorithm::OutputBuffer ? "OutputBuffer" : "GpuInputBuffer"));

    if (currentAlgorithm == ProcessingAlgorithm::InputBuffer)
    {
        inputAlgorithm.prepare(numInputChannels, numOutputChannels,
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
        outputAlgorithm.prepare(numInputChannels, numOutputChannels,
                               sampleRate, blockSize,
                               delayTimesMs.data(), levels.data(),
                               processingEnabled,
                               hfAttenuation.data(),
                               frDelayTimesMs.data(),
                               frLevels.data(),
                               frHFAttenuation.data());
        prepared = true;
    }
    // else // ProcessingAlgorithm::GpuInputBuffer
    // {
    //     prepared = gpuInputAlgorithm.prepare(numInputChannels, numOutputChannels,
    //                                          sampleRate, blockSize,
    //                                          delayTimesMs.data(), levels.data(),
    //                                          processingEnabled);
    // }

    audioEngineStarted = prepared;
    if (!audioEngineStarted && processingEnabled)
    {
        processingEnabled = false;
        processingToggle.setToggleState(false, juce::dontSendNotification);
    }

    // Create shared input buffers (used by reverb feed thread)
    if (audioEngineStarted)
    {
        sharedInputBuffers.clear();
        for (int i = 0; i < numInputChannels; ++i)
        {
            auto buf = std::make_unique<SharedInputRingBuffer>();
            buf->setSize(blockSize * 4);
            sharedInputBuffers.push_back(std::move(buf));
        }
    }

    // Start reverb feed thread (computes reverb feeds off the audio callback)
    if (audioEngineStarted && reverbEngine && calculationEngine)
    {
        int numReverbs = reverbEngine->getNumNodes();
        if (numReverbs > 0 && !sharedInputBuffers.empty())
        {
            reverbFeedThread = std::make_unique<ReverbFeedThread>();
            reverbFeedThread->prepare(sharedInputBuffers, reverbEngine.get(),
                                       calculationEngine->getInputReverbLevels(),
                                       calculationEngine->getNumReverbs(),
                                       numInputChannels, numReverbs,
                                       blockSize, reverbSRRatio);
            reverbFeedThread->startThread(juce::Thread::Priority::high);
        }
    }

    // Wire binaural processor to shared input buffers (reads directly, no push needed)
    if (audioEngineStarted && binauralProcessor && !sharedInputBuffers.empty())
        binauralProcessor->setSharedInputBuffers(sharedInputBuffers);

    // Start reverb engine thread (may have been stopped by channel count change)
    if (audioEngineStarted && reverbEngine)
        reverbEngine->startProcessing();
}

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    WFSLogger::getInstance().logInfo ("prepareToPlay: sampleRate=" + juce::String (sampleRate)
                                      + " bufferSize=" + juce::String (samplesPerBlockExpected));

    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

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
        // else // GPU InputBuffer
        // {
        //     // Safely tear down GPU processing on device/sample-rate changes.
        //     // User can re-enable processing after the device change completes.
        //     gpuInputAlgorithm.releaseResources();
        //     gpuInputAlgorithm.clear();
        //     audioEngineStarted = false;
        //     processingEnabled = false;
        //     processingToggle.setToggleState(false, juce::dontSendNotification);
        //     DBG("GPU Audio: Disabled GPU path due to device/sample-rate change. Re-enable processing to reinit.");
        // }
    }

    // Prepare test signal generator
    if (testSignalGenerator)
    {
        testSignalGenerator->prepare(sampleRate, samplesPerBlockExpected);
    }

    // Prepare binaural processor
    if (binauralProcessor)
    {
        binauralProcessor->prepareToPlay(sampleRate, samplesPerBlockExpected, numInputChannels);
        binauralProcessor->startProcessing();
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

        reverbEngine->startProcessing();

#if REVERB_DIAGNOSTICS
        reverbDiagReporter = std::make_unique<ReverbDiagnosticReporter> (
            reverbEngine->getDiagnostics());
        reverbDiagReporter->startReporting (1000);
#endif
    }

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

        // Apply AutomOtion return fade gain (50ms fade out/in during position snap-back)
        if (automOtionProcessor != nullptr)
        {
            for (int ch = 0; ch < numInputChannels && ch < patchedInputBuffer.getNumChannels(); ++ch)
            {
                float gain = automOtionProcessor->getReturnGain (ch);
                if (gain < 1.0f)
                    patchedInputBuffer.applyGain (ch, bufferToFill.startSample,
                                                   bufferToFill.numSamples, gain);
            }
        }

        // Write patched input to shared buffers + notify consumers (only when needed)
        {
            bool needSharedBuffers = (reverbFeedThread != nullptr)
                                  || (binauralProcessor && binauralProcessor->isEnabled());

            if (needSharedBuffers && !sharedInputBuffers.empty())
            {
                int safeInputCount = juce::jmin(numInputChannels, patchedInputBuffer.getNumChannels(), (int)sharedInputBuffers.size());
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
            }
        }

        // Parameter smoothing (runs on ASIO thread, immune to message-pump
        // throttling when the window is minimized)
        if (processingEnabled)
        {
            int matrixSize = numInputChannels * numOutputChannels;
            for (int i = 0; i < matrixSize; ++i)
            {
                delayTimesMs[i] += (targetDelayTimesMs[i] - delayTimesMs[i]) * delaySmoothingFactor;
                levels[i] += (targetLevels[i] - levels[i]) * levelSmoothingFactor;
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
            inputAlgorithm.processBlock(wfsOut, patchedInputBuffer, numInputChannels, numOutputChannels);
        }
        else if (currentAlgorithm == ProcessingAlgorithm::OutputBuffer)
        {
            outputAlgorithm.processBlock(wfsOut, patchedInputBuffer, numInputChannels, numOutputChannels);
        }
        // else // ProcessingAlgorithm::GpuInputBuffer
        // {
        //     gpuInputAlgorithm.processBlock(wfsOut, patchedInputBuffer, numInputChannels, numOutputChannels);
        // }

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
            const float* reverbOutputLevelsPtr = calculationEngine->getReverbOutputLevels();
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

                if (! isPostMuted)
                {
                    // Mix reverb return into WFS outputs using return level matrix
                    for (int outIdx = 0; outIdx < numOutputChannels; ++outIdx)
                    {
                        float returnLevel = reverbOutputLevelsPtr[revIdx * calcOutputStride + outIdx];

                        if (returnLevel > 0.0001f)
                        {
                            float* outputData = wfsOutputBuffer.getWritePointer(outIdx, startSample);
                            juce::FloatVectorOperations::addWithMultiply(outputData, returnData, returnLevel, numSamples);
                        }
                    }
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

                // Push input data to binaural processor from patchedInputBuffer
                int safeInputCount = juce::jmin(numInputChannels, patchedInputBuffer.getNumChannels());
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
    // else  // Commented out - GPU Audio SDK not configured
    // {
    //     gpuInputAlgorithm.releaseResources();
    // }

    // Stop reverb feed thread
    if (reverbFeedThread)
    {
        reverbFeedThread->stopThread(1000);
        reverbFeedThread.reset();
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

    // Tabbed component takes remaining space
    tabbedComponent.setBounds(bounds);

    // Update wizard overlay if active
    if (gettingStartedWizard && gettingStartedWizard->isActive())
        gettingStartedWizard->updateLayout();
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
    // Check once whether the window is visible — skip all repaints and
    // visual-only updates when minimized to avoid message-queue congestion
    // that can starve the audio thread's parameter updates.
    const bool windowVisible = isShowing();

    // Debounced auto-save of audio patch to disk
    if (patchSaveCountdown > 0 && --patchSaveCountdown == 0)
    {
        auto& fm = parameters.getFileManager();
        if (fm.hasValidProjectFolder())
            fm.saveSystemConfig();
    }

    // Increment tick counter
    timerTicksSinceLastRandom++;

    // WFS Calculation at ~50Hz (every 4 ticks = 20ms = 50Hz)
    // Recalculate matrix from input/output positions and update target values
    if (calculationEngine != nullptr && (timerTicksSinceLastRandom % 4) == 0)
    {
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
                }
            }

            // Keep map repainting while speed limiter is catching up
            if (windowVisible && mapTab != nullptr)
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

        // Collect audio levels for AutomOtion triggering
        if (automOtionProcessor != nullptr)
        {
            for (int i = 0; i < numInputChannels; ++i)
            {
                float shortPeakDb, rmsDb;
                if (currentAlgorithm == ProcessingAlgorithm::InputBuffer)
                {
                    shortPeakDb = inputAlgorithm.getShortPeakLevelDb(static_cast<size_t>(i));
                    rmsDb = inputAlgorithm.getRmsLevelDb(static_cast<size_t>(i));
                }
                else
                {
                    shortPeakDb = outputAlgorithm.getShortPeakLevelDb(static_cast<size_t>(i));
                    rmsDb = outputAlgorithm.getRmsLevelDb(static_cast<size_t>(i));
                }
                automOtionProcessor->setInputLevels(i, shortPeakDb, rmsDb);
            }
        }

        // Process AutomOtion at 50Hz (control rate)
        if (automOtionProcessor != nullptr)
        {
            automOtionProcessor->process(0.02f);  // 20ms delta time (50Hz)

            // Repaint map while AutomOtion is active (shows moving grey dot)
            if (windowVisible && automOtionProcessor->isAnyActive() && mapTab != nullptr)
                mapTab->repaint();
        }

        // Update level metering at 50Hz (20ms)
        if (levelMeteringManager != nullptr && levelMeteringManager->isMeteringActive())
        {
            levelMeteringManager->setCurrentAlgorithm(
                currentAlgorithm == ProcessingAlgorithm::InputBuffer
                    ? LevelMeteringManager::ProcessingAlgorithm::InputBuffer
                    : LevelMeteringManager::ProcessingAlgorithm::OutputBuffer);
            levelMeteringManager->updateLevels();

            // Repaint map if level overlay is enabled
            if (windowVisible && levelMeteringManager->isMapOverlayEnabled() && mapTab != nullptr)
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

                // Pass parameters to detector based on current algorithm
                if (currentAlgorithm == ProcessingAlgorithm::InputBuffer)
                {
                    inputAlgorithm.setLSParameters(static_cast<size_t>(i),
                        peakThresh, peakRatio, slowThresh, slowRatio);
                    peakGRs[i] = inputAlgorithm.getPeakGainReduction(static_cast<size_t>(i));
                    slowGRs[i] = inputAlgorithm.getSlowGainReduction(static_cast<size_t>(i));
                }
                else  // OutputBuffer algorithm
                {
                    outputAlgorithm.setLSParameters(static_cast<size_t>(i),
                        peakThresh, peakRatio, slowThresh, slowRatio);
                    peakGRs[i] = outputAlgorithm.getPeakGainReduction(static_cast<size_t>(i));
                    slowGRs[i] = outputAlgorithm.getSlowGainReduction(static_cast<size_t>(i));
                }
            }

            // Process LS gains
            lsTamerEngine->process(peakGRs, slowGRs);

            // Push LS GR to InputsTab meter display (gated on enable flags)
            if (windowVisible && inputsTab != nullptr)
            {
                int ch = inputsTab->getCurrentChannel() - 1;  // Convert 1-based to 0-based
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

        // Sync binaural processor enabled state from ValueTree
        if (binauralProcessor)
        {
            bool enabled = parameters.getValueTreeState().getBinauralEnabled();
            bool wasEnabled = binauralProcessor->isEnabled();
            binauralProcessor->setEnabled(enabled);

            if (enabled && !wasEnabled)
            {
                // Ensure processor is prepared and thread is running
                auto* device = deviceManager.getCurrentAudioDevice();
                if (device)
                {
                    binauralProcessor->prepareToPlay(device->getCurrentSampleRate(),
                                                     device->getCurrentBufferSizeSamples(),
                                                     numInputChannels);
                }
                binauralProcessor->startProcessing();
            }
            else if (!enabled && wasEnabled)
            {
                binauralProcessor->stopProcessing();
            }
        }

        // Always recalculate binaural positions (listener params may have changed independently)
        if (binauralCalcEngine != nullptr)
            binauralCalcEngine->recalculatePositions();

        // Only recalculate WFS matrix if input positions have changed (dirty flag set)
        if (calculationEngine->recalculateMatrixIfDirty())
        {

            // Copy calculated values to target arrays
            // Note: Calculation engine uses maxOutputChannels (64) for stride,
            // but our local arrays use numOutputChannels (user-configured)
            const float* calcDelays = calculationEngine->getDelayTimesMs();
            const float* calcLevels = calculationEngine->getLevels();
            const float* calcHF = calculationEngine->getHFAttenuationDb();
            const int calcStride = calculationEngine->getNumOutputs();  // maxOutputChannels (64)

            // Copy FR matrices from calculation engine
            const float* calcFRDelays = calculationEngine->getFRDelayTimesMs();
            const float* calcFRLevels = calculationEngine->getFRLevels();
            const float* calcFRHF = calculationEngine->getFRHFAttenuationDb();

            for (int inIdx = 0; inIdx < numInputChannels; ++inIdx)
            {
                for (int outIdx = 0; outIdx < numOutputChannels; ++outIdx)
                {
                    int srcIdx = inIdx * calcStride + outIdx;
                    int dstIdx = inIdx * numOutputChannels + outIdx;
                    targetDelayTimesMs[dstIdx] = calcDelays[srcIdx];
                    targetLevels[dstIdx] = calcLevels[srcIdx];
                    hfAttenuation[dstIdx] = calcHF[srcIdx];  // HF doesn't need smoothing - filter handles it

                    // Copy FR matrices (direct copy, no smoothing needed)
                    frDelayTimesMs[dstIdx] = calcFRDelays[srcIdx];
                    frLevels[dstIdx] = calcFRLevels[srcIdx];
                    frHFAttenuation[dstIdx] = calcFRHF[srcIdx];
                }
            }

            // Update FR filter parameters for each input
            for (int i = 0; i < numInputChannels; ++i)
            {
                using namespace WFSParameterIDs;
                auto frSection = parameters.getValueTreeState().getInputHackousticsSection(i);

                bool lowCutActive = static_cast<int>(frSection.getProperty(inputFRlowCutActive, 0)) != 0;
                float lowCutFreq = frSection.getProperty(inputFRlowCutFreq, 100.0f);
                bool highShelfActive = static_cast<int>(frSection.getProperty(inputFRhighShelfActive, 0)) != 0;
                float highShelfFreq = frSection.getProperty(inputFRhighShelfFreq, 3000.0f);
                float highShelfGain = frSection.getProperty(inputFRhighShelfGain, -2.0f);
                float highShelfSlope = frSection.getProperty(inputFRhighShelfSlope, 0.4f);
                float diffusion = frSection.getProperty(inputFRdiffusion, 20.0f);

                if (currentAlgorithm == ProcessingAlgorithm::InputBuffer)
                {
                    inputAlgorithm.setFRFilterParams(static_cast<size_t>(i),
                        lowCutActive, lowCutFreq,
                        highShelfActive, highShelfFreq, highShelfGain, highShelfSlope);
                    inputAlgorithm.setFRDiffusion(static_cast<size_t>(i), diffusion);
                }
                else  // OutputBuffer
                {
                    outputAlgorithm.setFRFilterParams(static_cast<size_t>(i),
                        lowCutActive, lowCutFreq,
                        highShelfActive, highShelfFreq, highShelfGain, highShelfSlope);
                    outputAlgorithm.setFRDiffusion(static_cast<size_t>(i), diffusion);
                }
            }

            // Update visualisation with current DSP matrix values (skip when minimized)
            // Use our correctly-strided local arrays (targetDelayTimesMs has numOutputChannels stride)
            if (windowVisible && inputsTab != nullptr)
            {
                // Create temporary reverb arrays with correct stride for visualization
                // Calculation engine uses maxReverbChannels (16) stride, but user may have fewer
                const float* calcReverbDelays = calculationEngine->getInputReverbDelayTimesMs();
                const float* calcReverbLevels = calculationEngine->getInputReverbLevels();
                const float* calcReverbHF = calculationEngine->getInputReverbHFAttenuationDb();
                const int calcReverbStride = calculationEngine->getNumReverbs();  // maxReverbChannels (16)
                int numReverbs = parameters.getNumReverbChannels();

                // Reindex reverb data with user-configured stride
                std::vector<float> reverbDelays(numInputChannels * numReverbs);
                std::vector<float> reverbLevels(numInputChannels * numReverbs);
                std::vector<float> reverbHF(numInputChannels * numReverbs);

                for (int inIdx = 0; inIdx < numInputChannels; ++inIdx)
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

                inputsTab->updateVisualisation(
                    targetDelayTimesMs.data(), targetLevels.data(), hfAttenuation.data(),
                    reverbDelays.data(), reverbLevels.data(), reverbHF.data());
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

                // Push IR-specific parameters
                if (algoType == 2)  // IR
                {
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
        if (windowVisible && mapTab != nullptr && (anyLFOActive || anyClusterLFOActive))
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
                        int inputId = i + 1;  // 1-based for OSC messages
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
            tabbedComponent.setCurrentTabIndex(4);  // Inputs tab index
            prompt = "Select Input Channel: ";
            break;
        case ChannelSelectionMode::Output:
            tabbedComponent.setCurrentTabIndex(2);  // Outputs tab index
            prompt = "Select Output Channel: ";
            break;
        case ChannelSelectionMode::Reverb:
            tabbedComponent.setCurrentTabIndex(3);  // Reverb tab index
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
                if (inputsTab != nullptr && channelNum <= inputsTab->getNumChannels())
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

void MainComponent::cycleChannel(int delta)
{
    int currentTabIndex = tabbedComponent.getCurrentTabIndex();

    if (currentTabIndex == 4 && inputsTab != nullptr)  // Inputs tab
    {
        inputsTab->cycleChannel(delta);
    }
    else if (currentTabIndex == 2 && outputsTab != nullptr)  // Outputs tab
    {
        outputsTab->cycleChannel(delta);
    }
    else if (currentTabIndex == 3 && reverbTab != nullptr)  // Reverb tab
    {
        reverbTab->cycleChannel(delta);
    }
}

void MainComponent::nudgeInputPosition(int axis, float delta, int inputOverride)
{
    // Use override if provided, otherwise get from InputsTab
    int channel = (inputOverride >= 0) ? inputOverride : (inputsTab != nullptr ? inputsTab->getCurrentChannel() - 1 : -1);
    if (channel < 0)
        return;

    auto& state = parameters.getValueTreeState();

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
        tabbedComponent.setCurrentTabIndex(6);  // Map tab
        return true;
    }

    // Get current tab index for tab-specific shortcuts
    int currentTabIndex = tabbedComponent.getCurrentTabIndex();

    // Clusters tab (index 5): Space cycles clusters, not channels
    if (currentTabIndex == 5 && clustersTab != nullptr && key.isKeyCode(juce::KeyPress::spaceKey))
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
    // Inputs tab (index 4): F1-F10 = Cluster 1-10, F11 = Single
    if (currentTabIndex == 4 && inputsTab != nullptr)
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

    // Outputs tab (index 2): F1-F10 = Array 1-10, F11 = Single
    if (currentTabIndex == 2 && outputsTab != nullptr)
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

    // Clusters tab (index 5): F1-F10 = select Cluster 1-10
    if (currentTabIndex == 5 && clustersTab != nullptr)
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

    // Map tab (index 6): F1-F10 = assign selected inputs to Cluster 1-10
    //                     F11 = remove from cluster (inputs) or break up cluster (barycenter)
    if (currentTabIndex == 6 && mapTab != nullptr)
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

    // Inputs tab (index 4)
    if (currentTabIndex == 4)
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

    // Outputs tab (index 2)
    if (currentTabIndex == 2)
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

    // Reverb tab (index 3)
    if (currentTabIndex == 3)
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

    // Map tab (index 6) - nudge selected input
    if (currentTabIndex == 6 && mapTab != nullptr)
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

