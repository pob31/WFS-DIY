#include "MainComponent.h"

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

    // Load channel counts
    numInputChannels = props.getIntValue("numInputChannels", 4);
    numOutputChannels = props.getIntValue("numOutputChannels", 4);
    numInputChannels = juce::jlimit(2, 64, numInputChannels);
    numOutputChannels = juce::jlimit(2, 64, numOutputChannels);

    // Initialize routing matrices with default values
    int matrixSize = numInputChannels * numOutputChannels;
    delayTimesMs.resize(matrixSize);
    levels.resize(matrixSize);
    targetDelayTimesMs.resize(matrixSize);
    targetLevels.resize(matrixSize);
    finalTargetDelayTimesMs.resize(matrixSize);
    finalTargetLevels.resize(matrixSize);
    startDelayTimesMs.resize(matrixSize);
    startLevels.resize(matrixSize);

    for (int inCh = 0; inCh < numInputChannels; ++inCh)
    {
        for (int outCh = 0; outCh < numOutputChannels; ++outCh)
        {
            int idx = inCh * numOutputChannels + outCh;
            // Initialize with random values
            delayTimesMs[idx] = random.nextFloat() * 1000.0f;  // 0-1000ms
            levels[idx] = random.nextFloat();  // 0-1
            // Initialize all target/ramp values
            targetDelayTimesMs[idx] = delayTimesMs[idx];
            targetLevels[idx] = levels[idx];
            startDelayTimesMs[idx] = delayTimesMs[idx];
            startLevels[idx] = levels[idx];
            // Generate initial final targets for first ramp
            finalTargetDelayTimesMs[idx] = random.nextFloat() * 1000.0f;
            finalTargetLevels[idx] = random.nextFloat();
        }
    }

    // Load saved device type and name
    juce::String savedDeviceType = props.getValue("audioDeviceType");
    juce::String savedDeviceName = props.getValue("audioDeviceName");

    // Create audio device selector
    audioSetupComp.reset(new juce::AudioDeviceSelectorComponent(
        deviceManager,
        2, 64,  // min/max input channels
        2, 64,  // min/max output channels
        false, // show MIDI input
        false, // show MIDI output
        false, // show channels as stereo pairs
        false  // hide advanced options
    ));
    addAndMakeVisible(audioSetupComp.get());

    // Create processing toggle button
    processingToggle.setButtonText("Processing ON/OFF");
    processingToggle.setToggleState(false, juce::dontSendNotification);
    processingToggle.onClick = [this]() {
        processingEnabled = processingToggle.getToggleState();

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
                for (auto& processor : inputProcessors)
                    processor->setProcessingEnabled(processingEnabled);
            }
            else
            {
                for (auto& processor : outputProcessors)
                    processor->setProcessingEnabled(processingEnabled);
            }
        }
        else
        {
            // Disable processing
            if (currentAlgorithm == ProcessingAlgorithm::InputBuffer)
            {
                for (auto& processor : inputProcessors)
                    processor->setProcessingEnabled(processingEnabled);
            }
            else
            {
                for (auto& processor : outputProcessors)
                    processor->setProcessingEnabled(processingEnabled);
            }
        }

        // Enable/disable channel count controls and algorithm selector
        numInputsSlider.setEnabled(!processingEnabled);
        numOutputsSlider.setEnabled(!processingEnabled);
        algorithmSelector.setEnabled(!processingEnabled);
    };
    addAndMakeVisible(processingToggle);

    // Create input channel count controls
    numInputsLabel.setText("Input Channels:", juce::dontSendNotification);
    numInputsLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(numInputsLabel);

    numInputsSlider.setSliderStyle(juce::Slider::IncDecButtons);
    numInputsSlider.setRange(2, 64, 1);
    numInputsSlider.setValue(numInputChannels, juce::dontSendNotification);
    numInputsSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 20);
    numInputsSlider.onValueChange = [this]() {
        numInputChannels = (int)numInputsSlider.getValue();
        saveSettings();
    };
    addAndMakeVisible(numInputsSlider);

    // Create output channel count controls
    numOutputsLabel.setText("Output Channels:", juce::dontSendNotification);
    numOutputsLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(numOutputsLabel);

    numOutputsSlider.setSliderStyle(juce::Slider::IncDecButtons);
    numOutputsSlider.setRange(2, 64, 1);
    numOutputsSlider.setValue(numOutputChannels, juce::dontSendNotification);
    numOutputsSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 20);
    numOutputsSlider.onValueChange = [this]() {
        numOutputChannels = (int)numOutputsSlider.getValue();
        saveSettings();
    };
    addAndMakeVisible(numOutputsSlider);

    // Create algorithm selector
    algorithmLabel.setText("Algorithm:", juce::dontSendNotification);
    algorithmLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(algorithmLabel);

    algorithmSelector.addItem("InputBuffer (read-time delays)", 1);
    algorithmSelector.addItem("OutputBuffer (write-time delays)", 2);
    algorithmSelector.setSelectedId(1, juce::dontSendNotification);
    algorithmSelector.onChange = [this]() {
        int selectedId = algorithmSelector.getSelectedId();
        ProcessingAlgorithm newAlgorithm = (selectedId == 1) ? ProcessingAlgorithm::InputBuffer
                                                              : ProcessingAlgorithm::OutputBuffer;

        // Only act if algorithm actually changed
        if (newAlgorithm != currentAlgorithm)
        {
            // If audio engine is running, clean up old processors
            if (audioEngineStarted)
            {
                // Remember if processing was enabled
                bool wasEnabled = processingEnabled;
                processingEnabled = false;

                // Clear old processors based on CURRENT algorithm
                if (currentAlgorithm == ProcessingAlgorithm::InputBuffer)
                {
                    for (auto& processor : inputProcessors)
                        processor->stopThread(1000);
                    inputProcessors.clear();
                }
                else
                {
                    for (auto& processor : outputProcessors)
                        processor->stopThread(1000);
                    outputProcessors.clear();
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
    };
    addAndMakeVisible(algorithmSelector);

    // Make sure you set the size of the component after
    // you add any child components.
    setSize (800, 600);

    // Always initialize audio with default device first
    setAudioChannels(numInputChannels, numOutputChannels);

    // Then restore saved device asynchronously
    if (savedDeviceType.isNotEmpty())
    {
        juce::MessageManager::callAsync([this, savedDeviceType, savedDeviceName]()
        {
            deviceManager.setCurrentAudioDeviceType(savedDeviceType, true);

            if (savedDeviceName.isNotEmpty())
            {
                juce::AudioDeviceManager::AudioDeviceSetup setup;
                deviceManager.getAudioDeviceSetup(setup);
                setup.outputDeviceName = savedDeviceName;
                setup.inputDeviceName = savedDeviceName;

                juce::String error = deviceManager.setAudioDeviceSetup(setup, true);
                if (error.isEmpty())
                {
                    // Update the last saved values to match the restored device
                    lastSavedDeviceType = savedDeviceType;
                    lastSavedDeviceName = savedDeviceName;
                }
                else
                {
                    DBG("Failed to restore ASIO device: " + error);
                    DBG("The device may be locked from a previous session or in use by another application");
                    DBG("Falling back to Windows Audio");

                    // Fall back to Windows Audio if ASIO fails
                    deviceManager.setCurrentAudioDeviceType("Windows Audio", true);

                    // Reinitialize with the fallback device
                    shutdownAudio();
                    setAudioChannels(numInputChannels, numOutputChannels);

                    // Update tracking variables to prevent saving the failed state
                    lastSavedDeviceType = deviceManager.getCurrentAudioDeviceType();
                    if (auto* device = deviceManager.getCurrentAudioDevice())
                        lastSavedDeviceName = device->getName();
                }
            }
        });
    }

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

    // Shutdown audio device first (this stops the audio callbacks)
    shutdownAudio();

    // Now safe to clear processing threads (audio callbacks no longer running)
    inputProcessors.clear();
    outputProcessors.clear();
}

//==============================================================================
void MainComponent::startAudioEngine()
{
    if (audioEngineStarted)
        return;

    // Get current audio settings
    auto* device = deviceManager.getCurrentAudioDevice();
    if (device == nullptr)
    {
        DBG("ERROR: No audio device available!");
        return;
    }

    double sampleRate = device->getCurrentSampleRate();
    int blockSize = device->getCurrentBufferSizeSamples();

    if (currentAlgorithm == ProcessingAlgorithm::InputBuffer)
    {
        // Create input-based processors (one thread per input channel)
        // Read-time delays: delay calculation happens when generating outputs
        for (int i = 0; i < numInputChannels; ++i)
        {
            auto processor = std::make_unique<InputBufferProcessor>(i, numOutputChannels,
                                                                     delayTimesMs.data(),
                                                                     levels.data());
            processor->prepare(sampleRate, blockSize);
            inputProcessors.push_back(std::move(processor));
        }

        // Start threads AFTER all processors are created and prepared
        for (auto& processor : inputProcessors)
        {
            processor->setProcessingEnabled(processingEnabled);
            processor->startThread(juce::Thread::Priority::high);
        }
    }
    else // ProcessingAlgorithm::OutputBuffer
    {
        // Create output-based processors (one thread per output channel)
        // Write-time delays: delay calculation happens when input arrives
        for (int i = 0; i < numOutputChannels; ++i)
        {
            auto processor = std::make_unique<OutputBufferProcessor>(i, numInputChannels, numOutputChannels,
                                                                      delayTimesMs.data(),
                                                                      levels.data());
            processor->prepare(sampleRate, blockSize);
            outputProcessors.push_back(std::move(processor));
        }

        // Start threads AFTER all processors are created and prepared
        for (auto& processor : outputProcessors)
        {
            processor->setProcessingEnabled(processingEnabled);
            processor->startThread(juce::Thread::Priority::high);
        }
    }

    audioEngineStarted = true;
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
            // Stop threads first
            for (auto& processor : inputProcessors)
                processor->stopThread(1000);

            // Re-prepare and restart
            for (auto& processor : inputProcessors)
            {
                processor->prepare(sampleRate, samplesPerBlockExpected);
                processor->setProcessingEnabled(processingEnabled);
                processor->startThread(juce::Thread::Priority::high);
            }
        }
        else // OutputBuffer
        {
            // Stop threads first
            for (auto& processor : outputProcessors)
                processor->stopThread(1000);

            // Re-prepare and restart
            for (auto& processor : outputProcessors)
            {
                processor->prepare(sampleRate, samplesPerBlockExpected);
                processor->setProcessingEnabled(processingEnabled);
                processor->startThread(juce::Thread::Priority::high);
            }
        }
    }
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Get buffer info
    auto totalChannels = bufferToFill.buffer->getNumChannels();
    auto numSamples = bufferToFill.numSamples;

    // Safety check: ensure processors are initialized
    if (!audioEngineStarted)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    if (currentAlgorithm == ProcessingAlgorithm::InputBuffer)
    {
        // InputBuffer algorithm: one processor per input
        if (inputProcessors.empty())
        {
            bufferToFill.clearActiveBufferRegion();
            return;
        }

        // Determine actual available channels
        auto numChannels = juce::jmin(numInputChannels, totalChannels, (int)inputProcessors.size());

        // Step 1: Distribute input data to each input processor thread
        for (int inChannel = 0; inChannel < numChannels; ++inChannel)
        {
            if (inputProcessors[inChannel] != nullptr && inChannel < totalChannels)
            {
                auto* inputData = bufferToFill.buffer->getReadPointer(inChannel, bufferToFill.startSample);
                inputProcessors[inChannel]->pushInput(inputData, numSamples);
            }
        }

        // Step 2: Clear output buffer
        bufferToFill.clearActiveBufferRegion();

        // Step 3: Sum outputs from all input processors to output channels
        juce::AudioBuffer<float> tempBuffer(1, numSamples);

        for (int inChannel = 0; inChannel < numChannels; ++inChannel)
        {
            if (inputProcessors[inChannel] == nullptr)
                continue;

            // Loop over all output channels (not limited by input count)
            int numOutputs = juce::jmin(numOutputChannels, totalChannels);
            for (int outChannel = 0; outChannel < numOutputs; ++outChannel)
            {
                auto* outputData = bufferToFill.buffer->getWritePointer(outChannel, bufferToFill.startSample);
                auto* tempData = tempBuffer.getWritePointer(0);

                // Pull processed data from this input processor for this output channel
                int samplesRead = inputProcessors[inChannel]->pullOutput(outChannel, tempData, numSamples);

                // Sum into output channel
                for (int i = 0; i < samplesRead; ++i)
                {
                    outputData[i] += tempData[i];
                }
            }
        }
    }
    else // ProcessingAlgorithm::OutputBuffer
    {
        // OutputBuffer algorithm: one processor per output
        if (outputProcessors.empty())
        {
            bufferToFill.clearActiveBufferRegion();
            return;
        }

        // Determine actual available channels
        auto numChannels = juce::jmin(numInputChannels, totalChannels);

        // Step 1: Distribute input data to all output processors
        for (int inChannel = 0; inChannel < numChannels; ++inChannel)
        {
            auto* inputData = bufferToFill.buffer->getReadPointer(inChannel, bufferToFill.startSample);

            // Send this input to all output processors
            for (auto& processor : outputProcessors)
            {
                processor->pushInput(inChannel, inputData, numSamples);
            }
        }

        // Step 2: Clear output buffer
        bufferToFill.clearActiveBufferRegion();

        // Step 3: Pull processed outputs from each output processor
        int numOutputs = juce::jmin(numOutputChannels, totalChannels, (int)outputProcessors.size());
        for (int outChannel = 0; outChannel < numOutputs; ++outChannel)
        {
            if (outputProcessors[outChannel] == nullptr)
                continue;

            auto* outputData = bufferToFill.buffer->getWritePointer(outChannel, bufferToFill.startSample);

            // Pull processed data from this output processor
            outputProcessors[outChannel]->pullOutput(outputData, numSamples);
        }
    }
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // Stop all processing threads
    if (currentAlgorithm == ProcessingAlgorithm::InputBuffer)
    {
        for (auto& processor : inputProcessors)
        {
            processor->stopThread(1000);
            processor->reset();
        }
    }
    else // OutputBuffer
    {
        for (auto& processor : outputProcessors)
        {
            processor->stopThread(1000);
            processor->reset();
        }
    }
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

        if (currentAlgorithm == ProcessingAlgorithm::InputBuffer && !inputProcessors.empty())
        {
            g.drawText("Thread Performance (InputBuffer):", 10, yPos, 300, 20, juce::Justification::left);

            yPos += 20;
            for (int i = 0; i < inputProcessors.size(); ++i)
            {
                float cpuUsage = inputProcessors[i]->getCpuUsagePercent();
                float procTime = inputProcessors[i]->getProcessingTimeMicroseconds();

                juce::String text = juce::String::formatted("Input %d: %.1f%% | %.1f us/block",
                                                            i, cpuUsage, procTime);
                g.drawText(text, 10, yPos + (i * 15), 300, 15, juce::Justification::left);
            }
        }
        else if (currentAlgorithm == ProcessingAlgorithm::OutputBuffer && !outputProcessors.empty())
        {
            g.drawText("Thread Performance (OutputBuffer):", 10, yPos, 300, 20, juce::Justification::left);

            yPos += 20;
            for (int i = 0; i < outputProcessors.size(); ++i)
            {
                float cpuUsage = outputProcessors[i]->getCpuUsagePercent();
                float procTime = outputProcessors[i]->getProcessingTimeMicroseconds();

                juce::String text = juce::String::formatted("Output %d: %.1f%% | %.1f us/block",
                                                            i, cpuUsage, procTime);
                g.drawText(text, 10, yPos + (i * 15), 300, 15, juce::Justification::left);
            }
        }
    }
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    auto bounds = getLocalBounds();

    // Top controls area - made taller to accommodate 3 rows
    auto controlsArea = bounds.removeFromTop(110).reduced(10);

    // Row 1: Toggle button at the top
    processingToggle.setBounds(controlsArea.removeFromTop(30));

    controlsArea.removeFromTop(5); // spacing

    // Row 2: Channel count controls
    auto channelRow = controlsArea.removeFromTop(25);

    auto inputsArea = channelRow.removeFromLeft(getWidth() / 2);
    numInputsLabel.setBounds(inputsArea.removeFromLeft(120));
    numInputsSlider.setBounds(inputsArea.removeFromLeft(150));

    auto outputsArea = channelRow.removeFromLeft(getWidth() / 2);
    numOutputsLabel.setBounds(outputsArea.removeFromLeft(120));
    numOutputsSlider.setBounds(outputsArea.removeFromLeft(150));

    controlsArea.removeFromTop(5); // spacing

    // Row 3: Algorithm selector
    auto algorithmRow = controlsArea.removeFromTop(25);
    algorithmLabel.setBounds(algorithmRow.removeFromLeft(120));
    algorithmSelector.setBounds(algorithmRow.removeFromLeft(300));

    // Audio setup component takes the rest
    if (audioSetupComp != nullptr)
        audioSetupComp->setBounds(bounds);
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

    // Save channel counts
    props.setValue("numInputChannels", numInputChannels);
    props.setValue("numOutputChannels", numOutputChannels);

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

    props.saveIfNeeded();
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

