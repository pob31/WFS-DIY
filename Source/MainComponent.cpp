#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    // Load saved channel counts only (don't apply audio settings yet)
    juce::PropertiesFile::Options options;
    options.applicationName = "WFS-DIY";
    options.filenameSuffix = ".settings";
    options.osxLibrarySubFolder = "Application Support";
    options.folderName = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                            .getChildFile("WFS-DIY").getFullPathName();

    juce::PropertiesFile props(options);

    // Clear any old audio device settings that might cause issues (only once)
    if (!props.getBoolValue("settingsCleaned", false))
    {
        props.removeValue("audioDeviceType");
        props.removeValue("outputDeviceName");
        props.removeValue("inputDeviceName");
        props.removeValue("sampleRate");
        props.removeValue("bufferSize");
        props.removeValue("inputChannels");
        props.removeValue("outputChannels");
        props.setValue("settingsCleaned", true);
        props.saveIfNeeded();
        DBG("Cleared old audio settings");
    }

    // Load only channel counts
    numInputChannels = props.getIntValue("numInputChannels", 4);
    numOutputChannels = props.getIntValue("numOutputChannels", 4);
    numInputChannels = juce::jlimit(2, 64, numInputChannels);
    numOutputChannels = juce::jlimit(2, 64, numOutputChannels);

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
        DBG("Processing toggle clicked - enabled: " + juce::String(processingEnabled ? "true" : "false"));

        if (processingEnabled && !audioEngineStarted)
        {
            // Start audio engine on first activation
            DBG("Starting audio engine...");
            startAudioEngine();
            DBG("Audio engine started. Processors: " + juce::String(inputProcessors.size()));
        }
        else if (processingEnabled && audioEngineStarted)
        {
            // Just enable existing processors
            DBG("Enabling existing processors");
            for (auto& processor : inputProcessors)
                processor->setProcessingEnabled(processingEnabled);
        }
        else
        {
            // Disable processing
            DBG("Disabling processors");
            for (auto& processor : inputProcessors)
                processor->setProcessingEnabled(processingEnabled);
        }

        // Enable/disable channel count controls
        numInputsSlider.setEnabled(!processingEnabled);
        numOutputsSlider.setEnabled(!processingEnabled);
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

    // Make sure you set the size of the component after
    // you add any child components.
    setSize (800, 600);

    // Initialize audio - this connects AudioAppComponent to receive callbacks
    // We need this to get getNextAudioBlock called
    setAudioChannels (numInputChannels, numOutputChannels);
}

MainComponent::~MainComponent()
{
    // Save settings before shutdown
    saveSettings();

    // Stop all processing threads before shutting down audio
    inputProcessors.clear();

    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::startAudioEngine()
{
    if (audioEngineStarted)
    {
        DBG("Audio engine already started");
        return;
    }

    // Get current audio settings
    auto* device = deviceManager.getCurrentAudioDevice();
    if (device == nullptr)
    {
        DBG("ERROR: No audio device available!");
        return;
    }

    DBG("Audio device: " + device->getName());
    DBG("Audio device is active: " + juce::String(device->isOpen() ? "true" : "false"));
    DBG("Audio device is playing: " + juce::String(device->isPlaying() ? "true" : "false"));

    double sampleRate = device->getCurrentSampleRate();
    int blockSize = device->getCurrentBufferSizeSamples();

    DBG("Starting audio engine with SR: " + juce::String(sampleRate) + " BS: " + juce::String(blockSize));
    DBG("Creating " + juce::String(numInputChannels) + " input processors with " + juce::String(numOutputChannels) + " outputs each");

    // Create and prepare input processors (one thread per input channel)
    for (int i = 0; i < numInputChannels; ++i)
    {
        auto processor = std::make_unique<InputProcessor>(i, numOutputChannels);
        processor->prepare(sampleRate, blockSize);
        inputProcessors.push_back(std::move(processor));
        DBG("Created processor " + juce::String(i));
    }

    audioEngineStarted = true;

    // Start threads AFTER all processors are created and prepared
    for (int i = 0; i < inputProcessors.size(); ++i)
    {
        inputProcessors[i]->setProcessingEnabled(processingEnabled);
        inputProcessors[i]->startThread(juce::Thread::Priority::high);
        DBG("Started thread for processor " + juce::String(i));
    }

    DBG("Audio engine fully initialized");
}

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    DBG("prepareToPlay called - SR: " + juce::String(sampleRate) + " BS: " + juce::String(samplesPerBlockExpected));

    // If audio engine was already started, update processor settings
    if (audioEngineStarted)
    {
        DBG("Audio engine already started, updating settings");
        // Stop threads first
        for (auto& processor : inputProcessors)
        {
            processor->stopThread(1000);
        }

        // Re-prepare and restart
        for (auto& processor : inputProcessors)
        {
            processor->prepare(sampleRate, samplesPerBlockExpected);
            processor->setProcessingEnabled(processingEnabled);
            processor->startThread(juce::Thread::Priority::high);
        }
    }
    else
    {
        DBG("Audio engine not started yet");
    }
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Get buffer info
    auto totalChannels = bufferToFill.buffer->getNumChannels();
    auto numSamples = bufferToFill.numSamples;

    static int callCount = 0;
    if (callCount++ < 5)
    {
        DBG("getNextAudioBlock called - channels: " + juce::String(totalChannels) +
            " samples: " + juce::String(numSamples) +
            " engineStarted: " + (audioEngineStarted ? "true" : "false"));
    }

    // Safety check: ensure processors are initialized
    if (inputProcessors.empty() || !audioEngineStarted)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    // Determine actual available channels (min of what we expect and what we have)
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

        for (int outChannel = 0; outChannel < numChannels; ++outChannel)
        {
            if (outChannel >= totalChannels)
                break;

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

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // Stop all processing threads
    for (auto& processor : inputProcessors)
    {
        processor->stopThread(1000);
        processor->reset();
    }
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    auto bounds = getLocalBounds();

    // Top controls area
    auto controlsArea = bounds.removeFromTop(80).reduced(10);

    // Toggle button at the top
    processingToggle.setBounds(controlsArea.removeFromTop(30));

    controlsArea.removeFromTop(5); // spacing

    // Channel count controls
    auto inputsArea = controlsArea.removeFromLeft(getWidth() / 2);
    numInputsLabel.setBounds(inputsArea.removeFromLeft(120));
    numInputsSlider.setBounds(inputsArea.removeFromLeft(150));

    auto outputsArea = controlsArea;
    numOutputsLabel.setBounds(outputsArea.removeFromLeft(120));
    numOutputsSlider.setBounds(outputsArea.removeFromLeft(150));

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

    // Save only channel counts (not audio device settings)
    props.setValue("numInputChannels", numInputChannels);
    props.setValue("numOutputChannels", numOutputChannels);

    props.saveIfNeeded();
}

