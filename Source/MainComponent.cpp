#include "MainComponent.h"
#include "Parameters/WFSParameterIDs.h"
#include "Localization/LocalizationManager.h"
#include "Accessibility/TTSManager.h"

//==============================================================================
MainComponent::MainComponent()
{
    // Initialize localization - try to load language file from Resources/lang/
    auto& locMgr = LocalizationManager::getInstance();
    auto exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();

    // Try multiple locations for Resources folder:
    // 1. Next to executable (production deployment)
    // 2. Project root (development from Visual Studio)
    juce::File resourceDir = exeDir.getChildFile("Resources");

    if (!resourceDir.getChildFile("lang/en.json").existsAsFile())
    {
        // Try development path: go up from Builds/VisualStudio2022/x64/Debug/App to project root
        auto projectRoot = exeDir.getParentDirectory()  // x64/Debug
                                 .getParentDirectory()  // x64
                                 .getParentDirectory()  // VisualStudio2022
                                 .getParentDirectory()  // Builds
                                 .getParentDirectory(); // Project root
        resourceDir = projectRoot.getChildFile("Resources");
        DBG("LocalizationManager: Trying development path: " + resourceDir.getFullPathName());
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

    DBG("Settings file location: " + props.getFile().getFullPathName());

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
    // Set up tabbed interface
    addAndMakeVisible(tabbedComponent);
    tabbedComponent.setTabBarDepth(35);
    tabbedComponent.setOutline(0);

    // Create status bar
    statusBar = new StatusBar();
    addAndMakeVisible(statusBar);

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
    mapTab->setStatusBar(statusBar);

    // Note: AutomOtionProcessor is set after it's created (later in constructor)

    // Set up callbacks from System Config tab
    systemConfigTab->setProcessingCallback([this](bool enabled) {
        handleProcessingChange(enabled);
    });

    systemConfigTab->setChannelCountCallback([this](int inputs, int outputs, int reverbs) {
        handleChannelCountChange(inputs, outputs, reverbs);
    });

    systemConfigTab->setAudioInterfaceCallback([this]() {
        openAudioInterfaceWindow();
    });

    systemConfigTab->setConfigReloadedCallback([this]() {
        handleConfigReloaded();
    });

    // Set up callbacks for individual tab config reloads
    outputsTab->onConfigReloaded = [this]() {
        handleConfigReloaded();
    };

    inputsTab->onConfigReloaded = [this]() {
        handleConfigReloaded();
    };

    // Level Meter window callbacks for InputsTab and OutputsTab
    inputsTab->onLevelMeterWindowRequested = [this]() {
        openLevelMeterWindow();
    };

    outputsTab->onLevelMeterWindowRequested = [this]() {
        openLevelMeterWindow();
    };

    reverbTab->onConfigReloaded = [this]() {
        handleConfigReloaded();
    };

    // Create and apply custom LookAndFeel for centralized widget theming
    wfsLookAndFeel = std::make_unique<WfsLookAndFeel>();
    juce::LookAndFeel::setDefaultLookAndFeel(wfsLookAndFeel.get());

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

    // Load saved color scheme from parameters and apply it
    // This will trigger WfsLookAndFeel::colorSchemeChanged() to update widget colors
    int colorSchemeId = (int)parameters.getConfigParam("ColorScheme");
    ColorScheme::Manager::getInstance().setTheme(colorSchemeId);

    // Subscribe to color scheme changes for component repaints
    ColorScheme::Manager::getInstance().addListener(this);

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

    // Connect InputsTab channel selection to OSCManager for REMOTE protocol
    inputsTab->onChannelSelected = [this](int channelId)
    {
        if (oscManager)
            oscManager->setRemoteSelectedChannel(channelId);
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

    // Send composite deltas for all inputs when a Remote client connects and initial data has been sent
    oscManager->onRemoteConnectionReady = [this](int targetIndex)
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

        DBG("MainComponent: Sent composite deltas for " << numInputChannels << " inputs to target " << targetIndex);
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

    DBG("Startup: Loaded saved device settings - Type: " + savedDeviceType + ", Name: " + savedDeviceName);

    // Restore saved device asynchronously (like DAWs do)
    // This happens after the window is shown for faster perceived startup
    juce::MessageManager::callAsync([this, savedDeviceStateXml, savedDeviceType, savedDeviceName]()
    {
        bool deviceRestored = false;

        // FAST PATH: Try to restore from saved XML state (skips device enumeration)
        if (savedDeviceStateXml.isNotEmpty())
        {
            DBG("Attempting fast restore from saved device state XML...");

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
                    DBG("Fast restore successful: " + device->getName() +
                        " | " + juce::String(device->getInputChannelNames().size()) + " inputs, " +
                        juce::String(device->getOutputChannelNames().size()) + " outputs");

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
            DBG("Trying fallback restore with type/name: " + savedDeviceType + "/" + savedDeviceName);

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

                    DBG("Fallback restore successful: " + savedDeviceName);
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

    // Start timer for device monitoring and parameter smoothing
    startTimer(5); // 5ms timer for smooth parameter updates

    // Listen for device manager changes to re-attach audio callbacks when device changes
    deviceManager.addChangeListener(this);
}

MainComponent::~MainComponent()
{
    // Stop listening to color scheme changes
    ColorScheme::Manager::getInstance().removeListener(this);

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

    // Clean up status bar (owned by this component, not TabbedComponent)
    delete statusBar;

    // Shutdown audio device first (this stops the audio callbacks)
    shutdownAudio();

    // Now safe to clear processing threads (audio callbacks no longer running)
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

    DBG("Audio callbacks attached - " + juce::String(numInputs) + " inputs, " + juce::String(numOutputs) + " outputs");
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    // Handle device manager changes
    if (source == &deviceManager)
    {
        auto* device = deviceManager.getCurrentAudioDevice();

        if (device != nullptr)
        {
            DBG("Device changed to: " + device->getName());

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
}

void MainComponent::stopProcessingForConfigurationChange()
{
    if (!audioEngineStarted)
        return;

    processingEnabled = false;
    // processingToggle removed - now managed in System Config tab
    parameters.setConfigParam("ProcessingEnabled", false);

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

    // Stop reverb engine for reconfiguration
    if (reverbEngine)
        reverbEngine->stopProcessing();

    audioEngineStarted = false;
}

void MainComponent::loadAudioPatches()
{
    // Load input patch matrix from ValueTree
    auto audioPatchTree = parameters.getValueTreeState().getState().getChildWithName(WFSParameterIDs::AudioPatch);
    auto inputPatchTree = audioPatchTree.getChildWithName(WFSParameterIDs::InputPatch);
    auto outputPatchTree = audioPatchTree.getChildWithName(WFSParameterIDs::OutputPatch);

    // Initialize patch maps to "unmapped" (-1)
    inputPatchMap.resize(64, -1);  // Max hardware inputs
    outputPatchMap.resize(64, -1); // Max WFS outputs

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
                    // Patch found: hardware channel hwChannel → WFS channel wfsChannel
                    if (hwChannel < inputPatchMap.size())
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
                    // Patch found: WFS channel wfsChannel → hardware channel hwChannel
                    if (wfsChannel < outputPatchMap.size())
                        outputPatchMap[wfsChannel] = hwChannel;
                }
            }
        }
    }
}

void MainComponent::applyInputPatch(const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Apply input patching: remap hardware inputs to WFS inputs
    int numHardwareInputs = bufferToFill.buffer->getNumChannels();

    // Prepare patched buffer if needed
    if (patchedInputBuffer.getNumChannels() != numInputChannels ||
        patchedInputBuffer.getNumSamples() < bufferToFill.numSamples)
    {
        patchedInputBuffer.setSize(numInputChannels, bufferToFill.numSamples, false, false, true);
    }

    patchedInputBuffer.clear();

    // Copy audio according to input patch map
    for (int hwChannel = 0; hwChannel < numHardwareInputs && hwChannel < inputPatchMap.size(); ++hwChannel)
    {
        int wfsChannel = inputPatchMap[hwChannel];
        if (wfsChannel >= 0 && wfsChannel < numInputChannels)
        {
            patchedInputBuffer.copyFrom(wfsChannel, bufferToFill.startSample,
                                        *bufferToFill.buffer, hwChannel,
                                        bufferToFill.startSample, bufferToFill.numSamples);
        }
    }

    // Replace buffer with patched version
    for (int ch = 0; ch < juce::jmin(numInputChannels, numHardwareInputs); ++ch)
    {
        bufferToFill.buffer->copyFrom(ch, bufferToFill.startSample,
                                      patchedInputBuffer, ch,
                                      bufferToFill.startSample, bufferToFill.numSamples);
    }
}

void MainComponent::applyOutputPatch(const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Apply output patching: remap WFS outputs to hardware outputs
    int numHardwareOutputs = bufferToFill.buffer->getNumChannels();

    // Prepare patched buffer if needed
    if (patchedOutputBuffer.getNumChannels() != numHardwareOutputs ||
        patchedOutputBuffer.getNumSamples() < bufferToFill.numSamples)
    {
        patchedOutputBuffer.setSize(numHardwareOutputs, bufferToFill.numSamples, false, false, true);
    }

    patchedOutputBuffer.clear();

    // Copy audio according to output patch map
    for (int wfsChannel = 0; wfsChannel < numOutputChannels && wfsChannel < outputPatchMap.size(); ++wfsChannel)
    {
        int hwChannel = outputPatchMap[wfsChannel];
        if (hwChannel >= 0 && hwChannel < numHardwareOutputs)
        {
            patchedOutputBuffer.addFrom(hwChannel, bufferToFill.startSample,
                                        *bufferToFill.buffer, wfsChannel,
                                        bufferToFill.startSample, bufferToFill.numSamples);
        }
    }

    // Replace buffer with patched version
    for (int ch = 0; ch < numHardwareOutputs; ++ch)
    {
        bufferToFill.buffer->copyFrom(ch, bufferToFill.startSample,
                                      patchedOutputBuffer, ch,
                                      bufferToFill.startSample, bufferToFill.numSamples);
    }
}

void MainComponent::handleProcessingChange(bool enabled)
{
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
        // Just enable existing processors
        if (currentAlgorithm == ProcessingAlgorithm::InputBuffer)
        {
            inputAlgorithm.setProcessingEnabled(processingEnabled);
        }
        else if (currentAlgorithm == ProcessingAlgorithm::OutputBuffer)
        {
            outputAlgorithm.setProcessingEnabled(processingEnabled);
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
    }
}

void MainComponent::handleChannelCountChange(int inputs, int outputs, int reverbs)
{
    numInputChannels = inputs;
    numOutputChannels = outputs;
    stopProcessingForConfigurationChange();
    resizeRoutingMatrices();

    // Update reverb engine node count
    if (reverbEngine)
        reverbEngine->setNumNodes(reverbs);

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
}

void MainComponent::handleConfigReloaded()
{
    // Update local channel counts from newly loaded config
    int newInputChannels = parameters.getNumInputChannels();
    int newOutputChannels = parameters.getNumOutputChannels();
    if (newInputChannels != numInputChannels || newOutputChannels != numOutputChannels)
    {
        numInputChannels = newInputChannels;
        numOutputChannels = newOutputChannels;
        resizeRoutingMatrices();
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

void MainComponent::openAudioInterfaceWindow()
{
    if (audioInterfaceWindow == nullptr)
    {
        audioInterfaceWindow = std::make_unique<AudioInterfaceWindow>(
            deviceManager,
            parameters.getValueTreeState(),
            testSignalGenerator.get()
        );
    }
    else
    {
        audioInterfaceWindow->setVisible(true);
        audioInterfaceWindow->toFront(true);
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
void MainComponent::startAudioEngine()
{
    if (audioEngineStarted)
        return;

    attachAudioCallbacksIfNeeded();

    // Get current audio settings
    auto* device = deviceManager.getCurrentAudioDevice();
    if (device == nullptr)
    {
        DBG("ERROR: No audio device available!");
        return;
    }

    double sampleRate = device->getCurrentSampleRate();
    int blockSize = device->getCurrentBufferSizeSamples();

    bool prepared = false;
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
}

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
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
        reverbEngine->prepareToPlay(sampleRate, samplesPerBlockExpected, numReverbs);

        // Resize reverb feed/return audio buffers
        if (numReverbs > 0)
        {
            reverbFeedBuffer.setSize(numReverbs, samplesPerBlockExpected);
            reverbReturnBuffer.setSize(numReverbs, samplesPerBlockExpected);
        }

        reverbEngine->startProcessing();
    }

    // Load audio patch matrices
    loadAudioPatches();
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Process WFS audio if engine is started
    if (audioEngineStarted)
    {
        // Apply input patching: hardware channels → WFS channels
        applyInputPatch(bufferToFill);

        // Process WFS audio
        if (currentAlgorithm == ProcessingAlgorithm::InputBuffer)
        {
            inputAlgorithm.processBlock(bufferToFill, numInputChannels, numOutputChannels);
        }
        else if (currentAlgorithm == ProcessingAlgorithm::OutputBuffer)
        {
            outputAlgorithm.processBlock(bufferToFill, numInputChannels, numOutputChannels);
        }
        // else // ProcessingAlgorithm::GpuInputBuffer
        // {
        //     gpuInputAlgorithm.processBlock(bufferToFill, numInputChannels, numOutputChannels);
        // }

        // Process reverb: compute per-node feed sums, push to engine, pull wet returns, mix into outputs
        if (reverbEngine && reverbEngine->isActive() && calculationEngine)
        {
            int numReverbs = reverbEngine->getNumNodes();

            if (numReverbs > 0)
            {
                int numSamples = bufferToFill.numSamples;
                int startSample = bufferToFill.startSample;

                // Get input→reverb level matrix from calculation engine
                // Index: [inputIndex * calcReverbStride + reverbIndex]
                const float* reverbLevelsPtr = calculationEngine->getInputReverbLevels();
                const int calcReverbStride = calculationEngine->getNumReverbs();

                // Compute per-node feed sums: for each reverb node, sum input contributions
                reverbFeedBuffer.clear();
                for (int revIdx = 0; revIdx < numReverbs; ++revIdx)
                {
                    float* feedData = reverbFeedBuffer.getWritePointer(revIdx);

                    for (int inIdx = 0; inIdx < numInputChannels; ++inIdx)
                    {
                        float feedLevel = reverbLevelsPtr[inIdx * calcReverbStride + revIdx];

                        if (feedLevel > 0.0001f)
                        {
                            const float* inputData = bufferToFill.buffer->getReadPointer(inIdx, startSample);
                            juce::FloatVectorOperations::addWithMultiply(feedData, inputData, feedLevel, numSamples);
                        }
                    }

                    // Push feed to reverb engine
                    reverbEngine->pushNodeInput(revIdx, feedData, numSamples);
                }

                // Pull wet reverb output and mix into WFS outputs
                // Index: [reverbIndex * calcOutputStride + outputIndex]
                const float* reverbOutputLevelsPtr = calculationEngine->getReverbOutputLevels();
                const int calcOutputStride = calculationEngine->getNumOutputs();

                for (int revIdx = 0; revIdx < numReverbs; ++revIdx)
                {
                    float* returnData = reverbReturnBuffer.getWritePointer(revIdx);
                    reverbEngine->pullNodeOutput(revIdx, returnData, numSamples);

                    // Mix reverb return into WFS outputs using return level matrix
                    for (int outIdx = 0; outIdx < numOutputChannels; ++outIdx)
                    {
                        float returnLevel = reverbOutputLevelsPtr[revIdx * calcOutputStride + outIdx];

                        if (returnLevel > 0.0001f)
                        {
                            float* outputData = bufferToFill.buffer->getWritePointer(outIdx, startSample);
                            juce::FloatVectorOperations::addWithMultiply(outputData, returnData, returnLevel, numSamples);
                        }
                    }
                }
            }
        }

        // Process binaural output (all inputs, or only soloed inputs if any are soloed)
        if (binauralProcessor && binauralProcessor->isEnabled() && binauralCalcEngine)
        {
            int binauralCh = binauralCalcEngine->getBinauralOutputChannel();
            if (binauralCh >= 0 && binauralCh + 1 < numOutputChannels)
            {
                // Push input data to binaural processor (thread will process)
                for (int i = 0; i < numInputChannels; ++i)
                {
                    const float* inputData = bufferToFill.buffer->getReadPointer(i, bufferToFill.startSample);
                    binauralProcessor->pushInput(i, inputData, bufferToFill.numSamples);
                }

                // Pull processed binaural output
                float* leftOut = bufferToFill.buffer->getWritePointer(binauralCh, bufferToFill.startSample);
                float* rightOut = bufferToFill.buffer->getWritePointer(binauralCh + 1, bufferToFill.startSample);
                binauralProcessor->pullOutput(leftOut, rightOut, bufferToFill.numSamples);
            }
        }

        // Apply output patching: WFS channels → hardware channels
        applyOutputPatch(bufferToFill);
    }
    else
    {
        // Clear buffer if no processing
        bufferToFill.clearActiveBufferRegion();
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

    // Release reverb engine
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
    auto bounds = getLocalBounds();
    const int statusBarHeight = 30;

    // Status bar at bottom, full width
    statusBar->setBounds(bounds.removeFromBottom(statusBarHeight));

    // Tabbed component takes remaining space
    tabbedComponent.setBounds(bounds);
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
        DBG("Saved audio device state XML");
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

    // Log saved settings
    auto setup = deviceManager.getAudioDeviceSetup();
    DBG("Settings saved - Device: " + currentDeviceType + " / " +
        (deviceManager.getCurrentAudioDevice() ? deviceManager.getCurrentAudioDevice()->getName() : "none") +
        " | WFS: " + juce::String(numInputChannels) + "x" + juce::String(numOutputChannels) +
        " | Hardware I/O: " + juce::String(setup.inputChannels.countNumberOfSetBits()) + " in, " +
        juce::String(setup.outputChannels.countNumberOfSetBits()) + " out");
}

void MainComponent::timerCallback()
{
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

            // Keep map repainting while speed limiter is catching up
            if (speedLimiter->isAnyInputMoving() && mapTab != nullptr)
                mapTab->repaint();

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
            if (levelMeteringManager->isMapOverlayEnabled() && mapTab != nullptr)
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

            if (automOtionProcessor != nullptr)
            {
                totalOffsetX += automOtionProcessor->getOffsetX(i);
                totalOffsetY += automOtionProcessor->getOffsetY(i);
                totalOffsetZ += automOtionProcessor->getOffsetZ(i);
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
            binauralProcessor->setEnabled(enabled);
        }

        // Only recalculate if positions have changed (dirty flag set)
        if (calculationEngine->recalculateMatrixIfDirty())
        {
            // Update binaural virtual speaker positions (depends on listener params)
            if (binauralCalcEngine != nullptr)
                binauralCalcEngine->recalculatePositions();

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

            // Update reverb engine parameters at 50Hz
            if (reverbEngine && reverbEngine->isActive())
            {
                using namespace WFSParameterIDs;
                auto& vts = parameters.getValueTreeState();
                auto algoSection = vts.getReverbAlgorithmSection();

                if (algoSection.isValid())
                {
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
            }

            // Update visualisation with current DSP matrix values
            // Use our correctly-strided local arrays (targetDelayTimesMs has numOutputChannels stride)
            if (inputsTab != nullptr)
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

        // Update LFO indicators in InputsTab for the selected input
        if (inputsTab != nullptr && lfoProcessor != nullptr)
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

        // Repaint map if any LFO is active
        if (mapTab != nullptr && anyLFOActive)
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

    // Apply exponential smoothing to delay/level parameters (every tick for smooth movement)
    if (processingEnabled && audioEngineStarted)
    {
        int matrixSize = numInputChannels * numOutputChannels;

        // Apply exponential smoothing to actual values: smooth towards targets
        for (int i = 0; i < matrixSize; ++i)
        {
            delayTimesMs[i] += (targetDelayTimesMs[i] - delayTimesMs[i]) * smoothingFactor;
            levels[i] += (targetLevels[i] - levels[i]) * smoothingFactor;
        }

        // Repaint to update CPU usage display (every 10 ticks = 50ms)
        if (timerTicksSinceLastRandom % 10 == 0)
            repaint();
    }

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
    bool useOffset = globalTracking && protocolEnabled && channelTracking;

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
                    return true;
                }
            }
            else
            {
                // Ctrl+Z = Undo
                if (state.canUndo())
                {
                    state.undo();
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

