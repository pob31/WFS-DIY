#include "AudioInterfaceWindow.h"
#include "WindowUtils.h"

//==============================================================================
// DeviceInfoBar Implementation
//==============================================================================

DeviceInfoBar::DeviceInfoBar(juce::AudioDeviceManager& devManager)
    : deviceManager(devManager)
{
    updateDeviceInfo();
    startTimer(1000);  // Update every second
}

DeviceInfoBar::~DeviceInfoBar()
{
    stopTimer();
}

void DeviceInfoBar::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF252525));

    // Draw border
    g.setColour(juce::Colour(0xFF404040));
    g.drawRect(getLocalBounds(), 1);

    // Draw device info
    auto bounds = getLocalBounds().reduced(10);

    g.setColour(juce::Colours::white);
    g.setFont(14.0f);

    // Line 1: Device type and name
    juce::String line1 = deviceType + ": " + deviceName;
    g.drawText(line1, bounds.removeFromTop(20), juce::Justification::centredLeft);

    bounds.removeFromTop(5);

    // Line 2: Sample rate and buffer size
    juce::String line2 = juce::String(sampleRate, 0) + " Hz, " +
                        juce::String(bufferSize) + " samples";
    g.setFont(12.0f);
    g.setColour(juce::Colours::lightgrey);
    g.drawText(line2, bounds.removeFromTop(16), juce::Justification::centredLeft);
}

void DeviceInfoBar::resized()
{
    // Fixed height component
}

void DeviceInfoBar::timerCallback()
{
    updateDeviceInfo();
}

void DeviceInfoBar::updateDeviceInfo()
{
    auto* device = deviceManager.getCurrentAudioDevice();

    if (device != nullptr)
    {
        deviceType = device->getTypeName();
        deviceName = device->getName();
        sampleRate = device->getCurrentSampleRate();
        bufferSize = device->getCurrentBufferSizeSamples();
    }
    else
    {
        deviceType = "No Device";
        deviceName = "Not configured";
        sampleRate = 0.0;
        bufferSize = 0;
    }

    repaint();
}

//==============================================================================
// DeviceSettingsPanel Implementation
//==============================================================================

DeviceSettingsPanel::DeviceSettingsPanel(juce::AudioDeviceManager& devManager)
    : deviceManager(devManager)
{
    // Setup labels
    addAndMakeVisible(deviceTypeLabel);
    deviceTypeLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(deviceLabel);
    deviceLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(sampleRateLabel);
    sampleRateLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(bufferSizeLabel);
    bufferSizeLabel.setJustificationType(juce::Justification::centredRight);

    // Setup combo boxes
    addAndMakeVisible(deviceTypeCombo);
    deviceTypeCombo.onChange = [this]() { deviceTypeChanged(); };

    addAndMakeVisible(deviceCombo);
    deviceCombo.onChange = [this]() { deviceChanged(); };

    addAndMakeVisible(sampleRateCombo);
    sampleRateCombo.onChange = [this]() { sampleRateChanged(); };

    addAndMakeVisible(bufferSizeCombo);
    bufferSizeCombo.onChange = [this]() { bufferSizeChanged(); };

    // Setup buttons
    addAndMakeVisible(controlPanelButton);
    controlPanelButton.onClick = [this]()
    {
        if (auto* device = deviceManager.getCurrentAudioDevice())
        {
            device->showControlPanel();
        }
    };

    addAndMakeVisible(resetDeviceButton);
    resetDeviceButton.onClick = [this]()
    {
        deviceManager.restartLastAudioDevice();
    };

    // Listen for device manager changes
    deviceManager.addChangeListener(this);

    // Initialize all controls
    updateAllControls();
}

DeviceSettingsPanel::~DeviceSettingsPanel()
{
    deviceManager.removeChangeListener(this);
}

void DeviceSettingsPanel::resized()
{
    auto bounds = getLocalBounds().reduced(20);

    const int labelWidth = 150;
    const int rowHeight = 30;
    const int spacing = 10;

    // Device type row
    auto row = bounds.removeFromTop(rowHeight);
    deviceTypeLabel.setBounds(row.removeFromLeft(labelWidth));
    row.removeFromLeft(spacing);
    deviceTypeCombo.setBounds(row);

    bounds.removeFromTop(spacing);

    // Device row
    row = bounds.removeFromTop(rowHeight);
    deviceLabel.setBounds(row.removeFromLeft(labelWidth));
    row.removeFromLeft(spacing);
    deviceCombo.setBounds(row);

    bounds.removeFromTop(spacing);

    // Sample rate row
    row = bounds.removeFromTop(rowHeight);
    sampleRateLabel.setBounds(row.removeFromLeft(labelWidth));
    row.removeFromLeft(spacing);
    sampleRateCombo.setBounds(row);

    bounds.removeFromTop(spacing);

    // Buffer size row
    row = bounds.removeFromTop(rowHeight);
    bufferSizeLabel.setBounds(row.removeFromLeft(labelWidth));
    row.removeFromLeft(spacing);
    bufferSizeCombo.setBounds(row);

    bounds.removeFromTop(spacing * 2);

    // Buttons row
    row = bounds.removeFromTop(rowHeight);
    row.removeFromLeft(labelWidth + spacing);  // Align with combos
    controlPanelButton.setBounds(row.removeFromLeft(120));
    row.removeFromLeft(spacing);
    resetDeviceButton.setBounds(row.removeFromLeft(120));
}

void DeviceSettingsPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1E1E1E));
}

void DeviceSettingsPanel::setEnabled(bool shouldBeEnabled)
{
    deviceTypeCombo.setEnabled(shouldBeEnabled);
    deviceCombo.setEnabled(shouldBeEnabled);
    sampleRateCombo.setEnabled(shouldBeEnabled);
    bufferSizeCombo.setEnabled(shouldBeEnabled);
    controlPanelButton.setEnabled(shouldBeEnabled);
    resetDeviceButton.setEnabled(shouldBeEnabled);
}

void DeviceSettingsPanel::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &deviceManager)
    {
        updateAllControls();
    }
}

void DeviceSettingsPanel::updateDeviceTypes()
{
    if (isUpdating)
        return;

    isUpdating = true;

    deviceTypeCombo.clear(juce::dontSendNotification);

    const auto& types = deviceManager.getAvailableDeviceTypes();
    juce::String currentType = deviceManager.getCurrentAudioDeviceType();

    int selectedId = 0;
    int id = 1;

    for (auto* type : types)
    {
        deviceTypeCombo.addItem(type->getTypeName(), id);
        if (type->getTypeName() == currentType)
            selectedId = id;
        ++id;
    }

    if (selectedId > 0)
        deviceTypeCombo.setSelectedId(selectedId, juce::dontSendNotification);

    isUpdating = false;
}

void DeviceSettingsPanel::updateDevices()
{
    if (isUpdating)
        return;

    isUpdating = true;

    deviceCombo.clear(juce::dontSendNotification);

    auto* currentType = deviceManager.getCurrentDeviceTypeObject();
    if (currentType == nullptr)
    {
        isUpdating = false;
        return;
    }

    auto deviceNames = currentType->getDeviceNames();
    juce::String currentDevice = deviceManager.getCurrentAudioDevice()
                                    ? deviceManager.getCurrentAudioDevice()->getName()
                                    : "";

    int selectedId = 0;
    int id = 1;

    for (const auto& name : deviceNames)
    {
        deviceCombo.addItem(name, id);
        if (name == currentDevice)
            selectedId = id;
        ++id;
    }

    if (selectedId > 0)
        deviceCombo.setSelectedId(selectedId, juce::dontSendNotification);

    isUpdating = false;
}

void DeviceSettingsPanel::updateSampleRates()
{
    if (isUpdating)
        return;

    isUpdating = true;

    sampleRateCombo.clear(juce::dontSendNotification);

    auto* device = deviceManager.getCurrentAudioDevice();
    if (device == nullptr)
    {
        isUpdating = false;
        return;
    }

    auto rates = device->getAvailableSampleRates();
    double currentRate = device->getCurrentSampleRate();

    int selectedId = 0;
    int id = 1;

    for (double rate : rates)
    {
        sampleRateCombo.addItem(juce::String(static_cast<int>(rate)) + " Hz", id);
        if (std::abs(rate - currentRate) < 1.0)
            selectedId = id;
        ++id;
    }

    if (selectedId > 0)
        sampleRateCombo.setSelectedId(selectedId, juce::dontSendNotification);

    isUpdating = false;
}

void DeviceSettingsPanel::updateBufferSizes()
{
    if (isUpdating)
        return;

    isUpdating = true;

    bufferSizeCombo.clear(juce::dontSendNotification);

    auto* device = deviceManager.getCurrentAudioDevice();
    if (device == nullptr)
    {
        isUpdating = false;
        return;
    }

    auto sizes = device->getAvailableBufferSizes();
    int currentSize = device->getCurrentBufferSizeSamples();
    double sampleRate = device->getCurrentSampleRate();

    int selectedId = 0;
    int id = 1;

    for (int size : sizes)
    {
        double latencyMs = (sampleRate > 0) ? (size * 1000.0 / sampleRate) : 0.0;
        juce::String label = juce::String(size) + " samples (" +
                            juce::String(latencyMs, 1) + " ms)";
        bufferSizeCombo.addItem(label, id);
        if (size == currentSize)
            selectedId = id;
        ++id;
    }

    if (selectedId > 0)
        bufferSizeCombo.setSelectedId(selectedId, juce::dontSendNotification);

    isUpdating = false;
}

void DeviceSettingsPanel::updateAllControls()
{
    updateDeviceTypes();
    updateDevices();
    updateSampleRates();
    updateBufferSizes();

    // Show/hide control panel button based on device type (ASIO has control panel)
    bool hasControlPanel = false;
    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        hasControlPanel = device->hasControlPanel();
    }
    controlPanelButton.setVisible(hasControlPanel);
}

void DeviceSettingsPanel::deviceTypeChanged()
{
    if (isUpdating)
        return;

    int selectedId = deviceTypeCombo.getSelectedId();
    if (selectedId <= 0)
        return;

    const auto& types = deviceManager.getAvailableDeviceTypes();
    int index = selectedId - 1;

    if (index >= 0 && index < static_cast<int>(types.size()))
    {
        deviceManager.setCurrentAudioDeviceType(types[index]->getTypeName(), true);
        // After changing type, enable all channels on the new default device
        enableAllChannels();
    }
}

void DeviceSettingsPanel::deviceChanged()
{
    if (isUpdating)
        return;

    int selectedId = deviceCombo.getSelectedId();
    if (selectedId <= 0)
        return;

    auto* currentType = deviceManager.getCurrentDeviceTypeObject();
    if (currentType == nullptr)
        return;

    auto deviceNames = currentType->getDeviceNames();
    int index = selectedId - 1;

    if (index >= 0 && index < deviceNames.size())
    {
        // Set up the device with all channels enabled
        juce::AudioDeviceManager::AudioDeviceSetup setup;
        deviceManager.getAudioDeviceSetup(setup);

        setup.inputDeviceName = deviceNames[index];
        setup.outputDeviceName = deviceNames[index];

        // Enable all available channels
        setup.inputChannels.setRange(0, 256, true);
        setup.outputChannels.setRange(0, 256, true);

        // Clear to use default sample rate and buffer size
        setup.sampleRate = 0;
        setup.bufferSize = 0;

        auto error = deviceManager.setAudioDeviceSetup(setup, true);

        if (error.isEmpty())
        {
            // After device is set up, ensure all channels are enabled
            enableAllChannels();
        }
        else
        {
            DBG("Device setup error: " + error);
        }
    }
}

void DeviceSettingsPanel::sampleRateChanged()
{
    if (isUpdating)
        return;

    int selectedId = sampleRateCombo.getSelectedId();
    if (selectedId <= 0)
        return;

    auto* device = deviceManager.getCurrentAudioDevice();
    if (device == nullptr)
        return;

    auto rates = device->getAvailableSampleRates();
    int index = selectedId - 1;

    if (index >= 0 && index < static_cast<int>(rates.size()))
    {
        juce::AudioDeviceManager::AudioDeviceSetup setup;
        deviceManager.getAudioDeviceSetup(setup);
        setup.sampleRate = rates[static_cast<size_t>(index)];

        auto error = deviceManager.setAudioDeviceSetup(setup, true);
        if (!error.isEmpty())
        {
            DBG("Sample rate change error: " + error);
        }
    }
}

void DeviceSettingsPanel::bufferSizeChanged()
{
    if (isUpdating)
        return;

    int selectedId = bufferSizeCombo.getSelectedId();
    if (selectedId <= 0)
        return;

    auto* device = deviceManager.getCurrentAudioDevice();
    if (device == nullptr)
        return;

    auto sizes = device->getAvailableBufferSizes();
    int index = selectedId - 1;

    if (index >= 0 && index < static_cast<int>(sizes.size()))
    {
        juce::AudioDeviceManager::AudioDeviceSetup setup;
        deviceManager.getAudioDeviceSetup(setup);
        setup.bufferSize = sizes[static_cast<size_t>(index)];

        auto error = deviceManager.setAudioDeviceSetup(setup, true);
        if (!error.isEmpty())
        {
            DBG("Buffer size change error: " + error);
        }
    }
}

void DeviceSettingsPanel::enableAllChannels()
{
    auto* device = deviceManager.getCurrentAudioDevice();
    if (device == nullptr)
        return;

    // Get current setup
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);

    // Get actual available channel counts from device
    auto inputChannelNames = device->getInputChannelNames();
    auto outputChannelNames = device->getOutputChannelNames();

    int numInputs = inputChannelNames.size();
    int numOutputs = outputChannelNames.size();

    // Enable all available input channels
    setup.inputChannels.clear();
    setup.inputChannels.setRange(0, numInputs, true);

    // Enable all available output channels
    setup.outputChannels.clear();
    setup.outputChannels.setRange(0, numOutputs, true);

    // Apply the setup
    auto error = deviceManager.setAudioDeviceSetup(setup, true);

    if (error.isEmpty())
    {
        DBG("Enabled all channels: " + juce::String(numInputs) + " inputs, " +
            juce::String(numOutputs) + " outputs");
    }
    else
    {
        DBG("Error enabling all channels: " + error);
    }
}

//==============================================================================
// AudioInterfaceContent Implementation
//==============================================================================

AudioInterfaceContent::AudioInterfaceContent(juce::AudioDeviceManager& devManager,
                                             WFSValueTreeState& valueTreeState,
                                             TestSignalGenerator* testSignalGen)
    : deviceManager(devManager),
      parameters(valueTreeState),
      testSignalGenerator(testSignalGen)
{
    // Create device info bar
    deviceInfoBar = std::make_unique<DeviceInfoBar>(deviceManager);
    addAndMakeVisible(deviceInfoBar.get());

    // Create tabbed component
    addAndMakeVisible(tabbedComponent);
    tabbedComponent.setTabBarDepth(35);
    tabbedComponent.setOutline(0);

    // Create custom device settings panel (replaces AudioDeviceSelectorComponent)
    deviceSettingsPanel = std::make_unique<DeviceSettingsPanel>(deviceManager);

    // Create patch tabs
    inputPatchTab = new InputPatchTab(parameters);
    outputPatchTab = new OutputPatchTab(parameters, testSignalGen);

    // Add tabs to tabbed component
    tabbedComponent.addTab("Device Settings", juce::Colours::darkgrey, deviceSettingsPanel.get(), false);
    tabbedComponent.addTab("Input Patch", juce::Colours::darkgrey, inputPatchTab, true);
    tabbedComponent.addTab("Output Patch", juce::Colours::darkgrey, outputPatchTab, true);
}

AudioInterfaceContent::~AudioInterfaceContent()
{
}

void AudioInterfaceContent::resized()
{
    auto bounds = getLocalBounds();

    // Device info bar at top
    deviceInfoBar->setBounds(bounds.removeFromTop(60));

    // Tabbed component fills remaining space
    tabbedComponent.setBounds(bounds);
}

void AudioInterfaceContent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1E1E1E));
}

void AudioInterfaceContent::setProcessingStateChanged(bool isProcessing)
{
    if (outputPatchTab != nullptr)
    {
        outputPatchTab->setProcessingStateChanged(isProcessing);
    }

    // Disable device settings panel when processing is active
    if (deviceSettingsPanel != nullptr)
    {
        deviceSettingsPanel->setEnabled(!isProcessing);
    }

    // Also disable test signals when processing starts
    if (isProcessing && testSignalGenerator)
    {
        testSignalGenerator->reset();
    }
}

void AudioInterfaceContent::resetAllModes()
{
    if (inputPatchTab != nullptr)
        inputPatchTab->resetMode();

    if (outputPatchTab != nullptr)
        outputPatchTab->resetMode();
}

//==============================================================================
// AudioInterfaceWindow Implementation
//==============================================================================

AudioInterfaceWindow::AudioInterfaceWindow(juce::AudioDeviceManager& deviceManager,
                                           WFSValueTreeState& valueTreeState,
                                           TestSignalGenerator* testSignalGen)
    : DocumentWindow("Audio Interface and Patching",
                     juce::Colour(0xFF1E1E1E),
                     DocumentWindow::allButtons),
      testSignalGenerator(testSignalGen)
{
    setUsingNativeTitleBar(true);
    setResizable(true, true);

    // Create content
    auto* newContent = new AudioInterfaceContent(deviceManager, valueTreeState, testSignalGen);
    setContentOwned(newContent, false);
    content = newContent;

    // Window sizing (same pattern as NetworkLogWindow)
    const int preferredWidth = 900;
    const int preferredHeight = 700;

    // Get display bounds
    auto& displays = juce::Desktop::getInstance().getDisplays();
    const auto* displayPtr = displays.getPrimaryDisplay();
    juce::Rectangle<int> userArea = (displayPtr != nullptr && !displayPtr->userArea.isEmpty())
        ? displayPtr->userArea
        : displays.getTotalBounds(true);

    const int margin = 40;
    const int windowWidth = juce::jmin(preferredWidth, userArea.getWidth() - margin);
    const int windowHeight = juce::jmin(preferredHeight, userArea.getHeight() - margin);

    setResizeLimits(600, 500, userArea.getWidth(), userArea.getHeight());

    centreWithSize(windowWidth, windowHeight);
    setVisible(true);
    WindowUtils::enableDarkTitleBar(this);
}

void AudioInterfaceWindow::closeButtonPressed()
{
    // Reset all modes to scrolling (safety measure)
    if (content != nullptr)
    {
        content->resetAllModes();
    }

    // Disable test signals when closing
    if (testSignalGenerator)
    {
        testSignalGenerator->reset();
    }

    setVisible(false);
}

void AudioInterfaceWindow::setProcessingStateChanged(bool isProcessing)
{
    if (content != nullptr)
    {
        content->setProcessingStateChanged(isProcessing);
    }
}
