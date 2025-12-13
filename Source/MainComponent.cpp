#include "MainComponent.h"
#include "Parameters/WFSParameterIDs.h"

//==============================================================================
MainComponent::MainComponent()
{
    // Load saved channel counts and device state
    juce::PropertiesFile::Options options;
    options.applicationName = "WFS-DIY";
    options.filenameSuffix = ".settings";
    options.osxLibrarySubFolder = "Application Support";
    options.folderName = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                            .getChildFile("WFS-DIY").getFullPathName();

    juce::PropertiesFile props(options);

    // Load channel counts (persisted), clamp, and fall back to a usable default
    numInputChannels = props.getIntValue("numInputChannels", 0);
    numOutputChannels = props.getIntValue("numOutputChannels", 2);
    numInputChannels = juce::jlimit(0, 64, numInputChannels);
    numOutputChannels = juce::jlimit(2, 64, numOutputChannels);
    if (numInputChannels == 0)
        numInputChannels = 2; // Default to a sensible input count to avoid silent processing

    // Initialize routing matrices with default values
    resizeRoutingMatrices();

    // Load saved device type and name
    juce::String savedDeviceType = props.getValue("audioDeviceType");
    juce::String savedDeviceName = props.getValue("audioDeviceName");

    // Load saved audio device channel selections (as binary strings)
    juce::String savedInputChannelsBits = props.getValue("audioInputChannelsBits");
    juce::String savedOutputChannelsBits = props.getValue("audioOutputChannelsBits");

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

    // Pass status bar to tabs that support it
    systemConfigTab->setStatusBar(statusBar);
    networkTab->setStatusBar(statusBar);
    outputsTab->setStatusBar(statusBar);
    inputsTab->setStatusBar(statusBar);

    // Set up callbacks from System Config tab
    systemConfigTab->setProcessingCallback([this](bool enabled) {
        handleProcessingChange(enabled);
    });

    systemConfigTab->setChannelCountCallback([this](int inputs, int outputs) {
        handleChannelCountChange(inputs, outputs);
    });

    systemConfigTab->setAudioInterfaceCallback([this]() {
        openAudioInterfaceWindow();
    });

    // Add tabs to tabbed component
    tabbedComponent.addTab("System Configuration", juce::Colours::darkgrey, systemConfigTab, true);
    tabbedComponent.addTab("Network", juce::Colours::darkgrey, networkTab, true);
    tabbedComponent.addTab("Outputs", juce::Colours::darkgrey, outputsTab, true);
    tabbedComponent.addTab("Reverb", juce::Colours::darkgrey, reverbTab, true);
    tabbedComponent.addTab("Inputs", juce::Colours::darkgrey, inputsTab, true);
    tabbedComponent.addTab("Clusters", juce::Colours::darkgrey, clustersTab, true);
    tabbedComponent.addTab("Map", juce::Colours::darkgrey, mapTab, true);

    // Initialize OSC Manager for network communication
    oscManager = std::make_unique<WFSNetwork::OSCManager>(parameters.getValueTreeState());

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

    // Restore saved device and settings asynchronously (like DAWs do)
    // This happens after the window is shown
    juce::MessageManager::callAsync([this, savedDeviceType, savedDeviceName,
                                      savedInputChannelsBits, savedOutputChannelsBits]()
    {
        bool deviceRestored = false;

        // Try to restore saved device type (e.g., ASIO)
        if (savedDeviceType.isNotEmpty())
        {
            DBG("Attempting to restore device type: " + savedDeviceType);
            deviceManager.setCurrentAudioDeviceType(savedDeviceType, true);

            // Try to restore specific device name
            if (savedDeviceName.isNotEmpty())
            {
                juce::AudioDeviceManager::AudioDeviceSetup setup;
                deviceManager.getAudioDeviceSetup(setup);
                setup.outputDeviceName = savedDeviceName;
                setup.inputDeviceName = savedDeviceName;

                // Restore saved channel selections if available
                if (savedInputChannelsBits.isNotEmpty() || savedOutputChannelsBits.isNotEmpty())
                {
                    setup.useDefaultInputChannels = false;
                    setup.useDefaultOutputChannels = false;

                    // Restore exact channel bit patterns from saved settings
                    if (savedInputChannelsBits.isNotEmpty())
                        setup.inputChannels.parseString(savedInputChannelsBits, 2); // Parse binary string

                    if (savedOutputChannelsBits.isNotEmpty())
                        setup.outputChannels.parseString(savedOutputChannelsBits, 2); // Parse binary string
                }

                auto error = deviceManager.setAudioDeviceSetup(setup, true);
                deviceRestored = error.isEmpty();

                if (deviceRestored)
                {
                    int restoredInputs = setup.inputChannels.countNumberOfSetBits();
                    int restoredOutputs = setup.outputChannels.countNumberOfSetBits();

                    DBG("Successfully restored: " + savedDeviceName +
                        " | Hardware I/O: " + juce::String(restoredInputs) + " inputs, " +
                        juce::String(restoredOutputs) + " outputs");

                    // Settings restored successfully - update tracking
                    lastSavedDeviceType = savedDeviceType;
                    lastSavedDeviceName = savedDeviceName;
                }
                else
                {
                    DBG("Could not restore saved device: " + savedDeviceName);
                    DBG("Error: " + error);
                    DBG("Please select your audio device from the Audio Settings panel");
                }
            }
        }

        if (!deviceRestored)
        {
            DBG("Using default audio device - please configure in Audio Settings");
        }

        attachAudioCallbacksIfNeeded();
    });

    // Start timer for device monitoring and parameter smoothing
    lastSavedDeviceType = deviceManager.getCurrentAudioDeviceType();
    if (auto* device = deviceManager.getCurrentAudioDevice())
        lastSavedDeviceName = device->getName();
    startTimer(5); // 5ms timer for smooth parameter updates
}

MainComponent::~MainComponent()
{
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
}

void MainComponent::resizeRoutingMatrices()
{
    const int matrixSize = numInputChannels * numOutputChannels;

    delayTimesMs.assign(matrixSize, 0.0f);
    levels.assign(matrixSize, 0.0f);
    targetDelayTimesMs.assign(matrixSize, 0.0f);
    targetLevels.assign(matrixSize, 0.0f);
    finalTargetDelayTimesMs.assign(matrixSize, 0.0f);
    finalTargetLevels.assign(matrixSize, 0.0f);
    startDelayTimesMs.assign(matrixSize, 0.0f);
    startLevels.assign(matrixSize, 0.0f);

    for (int inCh = 0; inCh < numInputChannels; ++inCh)
    {
        for (int outCh = 0; outCh < numOutputChannels; ++outCh)
        {
            const int idx = inCh * numOutputChannels + outCh;
            const float delay = random.nextFloat() * 1000.0f;  // 0-1000ms
            const float level = random.nextFloat();            // 0-1

            delayTimesMs[idx] = delay;
            levels[idx] = level;
            targetDelayTimesMs[idx] = delay;
            targetLevels[idx] = level;
            startDelayTimesMs[idx] = delay;
            startLevels[idx] = level;
            finalTargetDelayTimesMs[idx] = random.nextFloat() * 1000.0f;
            finalTargetLevels[idx] = random.nextFloat();
        }
    }
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

    audioEngineStarted = false;
}

void MainComponent::handleProcessingChange(bool enabled)
{
    processingEnabled = enabled;

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

void MainComponent::handleChannelCountChange(int inputs, int outputs)
{
    numInputChannels = inputs;
    numOutputChannels = outputs;
    stopProcessingForConfigurationChange();
    resizeRoutingMatrices();
}

void MainComponent::openAudioInterfaceWindow()
{
    if (audioInterfaceWindow == nullptr)
    {
        audioInterfaceWindow = std::make_unique<AudioInterfaceWindow>(deviceManager);
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
                              processingEnabled);
        prepared = true;
    }
    else if (currentAlgorithm == ProcessingAlgorithm::OutputBuffer)
    {
        outputAlgorithm.prepare(numInputChannels, numOutputChannels,
                               sampleRate, blockSize,
                               delayTimesMs.data(), levels.data(),
                               processingEnabled);
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
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Safety check: ensure processors are initialized
    if (!audioEngineStarted)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

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
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // Display CPU usage and processing time for processor threads
    if (audioEngineStarted)
    {
        g.setColour(juce::Colours::white);
        g.setFont(12.0f);

        int yPos = getHeight() - 120;

        if (currentAlgorithm == ProcessingAlgorithm::InputBuffer && !inputAlgorithm.isEmpty())
        {
            g.drawText("Thread Performance (InputBuffer):", 10, yPos, 300, 20, juce::Justification::left);

            yPos += 20;
        const auto inputProcessorCount = static_cast<int>(inputAlgorithm.getNumProcessors());
        for (int i = 0; i < inputProcessorCount; ++i)
        {
            float cpuUsage = inputAlgorithm.getCpuUsagePercent(i);
            float procTime = inputAlgorithm.getProcessingTimeMicroseconds(i);

            juce::String text = juce::String::formatted("Input %d: %.1f%% | %.1f us/block",
                                                        (int)i, cpuUsage, procTime);
            g.drawText(text, 10, yPos + (i * 15), 300, 15, juce::Justification::left);
        }
        }
        else if (currentAlgorithm == ProcessingAlgorithm::OutputBuffer && !outputAlgorithm.isEmpty())
        {
            g.drawText("Thread Performance (OutputBuffer):", 10, yPos, 300, 20, juce::Justification::left);

            yPos += 20;
            const auto outputProcessorCount = static_cast<int>(outputAlgorithm.getNumProcessors());
            for (int i = 0; i < outputProcessorCount; ++i)
            {
                float cpuUsage = outputAlgorithm.getCpuUsagePercent(i);
                float procTime = outputAlgorithm.getProcessingTimeMicroseconds(i);

                juce::String text = juce::String::formatted("Output %d: %.1f%% | %.1f us/block",
                                                            (int)i, cpuUsage, procTime);
                g.drawText(text, 10, yPos + (i * 15), 300, 15, juce::Justification::left);
            }
        }
        // else if (currentAlgorithm == ProcessingAlgorithm::GpuInputBuffer && gpuInputAlgorithm.isReady())
        // {
        //     g.drawText("GPU InputBuffer (GPU Audio)", 10, yPos, 320, 20, juce::Justification::left);
        //
        //     // Draw a readable telemetry badge in the lower-right corner
        //     const int boxWidth = juce::jmin(420, getWidth() - 20);
        //     const int boxHeight = 70;
        //     const int boxX = getWidth() - boxWidth - 10;
        //     const int boxY = getHeight() - boxHeight - 10;
        //
        //     g.setColour(juce::Colours::black.withAlpha(0.45f));
        //     g.fillRoundedRectangle((float)boxX, (float)boxY, (float)boxWidth, (float)boxHeight, 6.0f);
        //     g.setColour(juce::Colours::white.withAlpha(0.85f));
        //     g.drawRoundedRectangle((float)boxX, (float)boxY, (float)boxWidth, (float)boxHeight, 6.0f, 1.5f);
        //
        //     juce::String device = gpuInputAlgorithm.getDeviceName();
        //     if (device.isEmpty())
        //         device = "<unknown device>";
        //
        //     const auto execMs = gpuInputAlgorithm.getLastGpuExecMs();
        //     const auto execSamples = gpuInputAlgorithm.getLastGpuLaunchSamples();
        //     const bool execFailed = gpuInputAlgorithm.getLastExecuteFailed();
        //
        //     g.setFont(14.0f);
        //     g.setColour(juce::Colours::white);
        //     g.drawText("GPU InputBuffer (GPU Audio)", boxX + 10, boxY + 6, boxWidth - 20, 20, juce::Justification::left);
        //
        //     g.setFont(13.0f);
        //     g.drawText("Device: " + device, boxX + 10, boxY + 26, boxWidth - 20, 18, juce::Justification::left);
        //
        //     auto statusColour = execFailed ? juce::Colours::red : juce::Colours::greenyellow;
        //     g.setColour(statusColour);
        //     juce::String status = "Last launch: " + juce::String(execSamples) + " samples, " +
        //                           juce::String(execMs, 2) + " ms";
        //     if (execFailed)
        //         status += " (failed)";
        //     g.drawText(status, boxX + 10, boxY + 44, boxWidth - 20, 18, juce::Justification::left);
        // }
    }
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

    // Get actual audio device channel configuration
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);

    // Save audio device type and name
    juce::String currentDeviceType = deviceManager.getCurrentAudioDeviceType();
    if (currentDeviceType.isNotEmpty())
    {
        props.setValue("audioDeviceType", currentDeviceType);
    }

    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        juce::String deviceName = device->getName();
        if (deviceName.isNotEmpty())
            props.setValue("audioDeviceName", deviceName);
    }

    // Save the selected physical I/O channels from the sound card as bit patterns
    // This preserves which specific channels are enabled (e.g., channels 3,4,5,6 out of 64)
    props.setValue("audioInputChannelsBits", setup.inputChannels.toString(2));  // Binary string
    props.setValue("audioOutputChannelsBits", setup.outputChannels.toString(2)); // Binary string

    props.saveIfNeeded();

    DBG("Settings saved - Device: " + currentDeviceType + " / " +
        (deviceManager.getCurrentAudioDevice() ? deviceManager.getCurrentAudioDevice()->getName() : "none") +
        " | WFS: " + juce::String(numInputChannels) + "x" + juce::String(numOutputChannels) +
        " | Hardware I/O: " + juce::String(setup.inputChannels.countNumberOfSetBits()) + " in, " +
        juce::String(setup.outputChannels.countNumberOfSetBits()) + " out");
}

void MainComponent::timerCallback()
{
    // Apply ramping and exponential smoothing to routing parameters (when processing is enabled)
    if (processingEnabled && audioEngineStarted)
    {
        int matrixSize = numInputChannels * numOutputChannels;

        // Calculate ramp progress (0.0 to 1.0 over 1 second)
        float rampProgress = (float)timerTicksSinceLastRandom / (float)rampDurationTicks;
        rampProgress = juce::jlimit(0.0f, 1.0f, rampProgress);

        // Update ramping targets: linearly interpolate from start to final over 1 second
        for (int i = 0; i < matrixSize; ++i)
        {
            targetDelayTimesMs[i] = startDelayTimesMs[i] + (finalTargetDelayTimesMs[i] - startDelayTimesMs[i]) * rampProgress;
            targetLevels[i] = startLevels[i] + (finalTargetLevels[i] - startLevels[i]) * rampProgress;
        }

        // Apply exponential smoothing to actual values: smooth towards ramping targets
        for (int i = 0; i < matrixSize; ++i)
        {
            delayTimesMs[i] += (targetDelayTimesMs[i] - delayTimesMs[i]) * smoothingFactor;
            levels[i] += (targetLevels[i] - levels[i]) * smoothingFactor;
        }

        // Repaint to update CPU usage display (every 10 ticks = 50ms)
        if (timerTicksSinceLastRandom % 10 == 0)
            repaint();
    }

    // Start new ramp every 1 second (200 ticks at 5ms)
    timerTicksSinceLastRandom++;
    if (timerTicksSinceLastRandom >= rampDurationTicks && processingEnabled && audioEngineStarted)
    {
        timerTicksSinceLastRandom = 0;

        int matrixSize = numInputChannels * numOutputChannels;

        // Save current final targets as new start values
        for (int i = 0; i < matrixSize; ++i)
        {
            startDelayTimesMs[i] = finalTargetDelayTimesMs[i];
            startLevels[i] = finalTargetLevels[i];
        }

        // Generate new final targets for next ramp
        for (int i = 0; i < matrixSize; ++i)
        {
            finalTargetDelayTimesMs[i] = random.nextFloat() * 1000.0f;  // 0-1000ms
            finalTargetLevels[i] = random.nextFloat();  // 0-1
        }
    }

    // Check for device changes every 200 ticks (once per second) to avoid overhead
    if (timerTicksSinceLastRandom % 200 == 0)
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
                // ReverbTab not implemented yet - placeholder
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
    // ReverbTab cycling not implemented yet
}

void MainComponent::nudgeInputPosition(int axis, float delta)
{
    if (inputsTab == nullptr)
        return;

    int channel = inputsTab->getCurrentChannel() - 1;  // Convert to 0-based
    if (channel < 0)
        return;

    auto& state = parameters.getValueTreeState();

    // Check if tracking is enabled (globally AND on channel)
    bool globalTracking = state.getIntParameter(WFSParameterIDs::trackingEnabled) != 0;
    bool channelTracking = state.getIntParameter(WFSParameterIDs::inputTrackingActive, channel) != 0;
    bool useOffset = globalTracking && channelTracking;

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
    state.setInputParameter(channel, paramId, current + delta);
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

    // Channel cycling: Space / Shift+Space
    if (key.isKeyCode(juce::KeyPress::spaceKey))
    {
        cycleChannel(key.getModifiers().isShiftDown() ? -1 : 1);
        return true;
    }

    // Position nudging: Arrow keys, Page Up/Down (Inputs and Outputs tabs)
    int currentTabIndex = tabbedComponent.getCurrentTabIndex();
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

    // Reverb tab (index 3) - no position parameters yet

    return false;
}

