#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 4 : 0, 4); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (4, 4);
    }

    // Create audio device selector
    audioSetupComp.reset(new juce::AudioDeviceSelectorComponent(
        deviceManager,
        4, 4,  // min/max input channels
        4, 4,  // min/max output channels
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
    };
    addAndMakeVisible(processingToggle);

    // Make sure you set the size of the component after
    // you add any child components.
    setSize (800, 600);
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    currentSampleRate = sampleRate;

    // Allocate 1 second delay buffer for 4 channels
    delayBufferLength = (int)(sampleRate * 1.0);
    delayBuffer.setSize(4, delayBufferLength);
    delayBuffer.clear();

    writePosition = 0;
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto numChannels = juce::jmin(4, bufferToFill.buffer->getNumChannels());
    auto numSamples = bufferToFill.numSamples;

    if (!processingEnabled)
    {
        // No processing - output silence
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    // Each input goes to all outputs with rotating delays:
    // Input 1: Out1(200ms), Out2(400ms), Out3(600ms), Out4(800ms)
    // Input 2: Out2(200ms), Out3(400ms), Out4(600ms), Out1(800ms)
    // Input 3: Out3(200ms), Out4(400ms), Out1(600ms), Out2(800ms)
    // Input 4: Out4(200ms), Out1(400ms), Out2(600ms), Out3(800ms)

    // Base delay increment (200ms)
    int delayIncrement = (int)(currentSampleRate * 0.2);

    // Create temp buffer to store input samples
    juce::AudioBuffer<float> inputBuffer(numChannels, numSamples);

    // Copy inputs to temp buffer and write to delay buffers
    for (int inChannel = 0; inChannel < numChannels; ++inChannel)
    {
        auto* inputData = bufferToFill.buffer->getReadPointer(inChannel, bufferToFill.startSample);
        auto* tempData = inputBuffer.getWritePointer(inChannel);
        auto* delayData = delayBuffer.getWritePointer(inChannel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            tempData[sample] = inputData[sample];
            int pos = (writePosition + sample) % delayBufferLength;
            delayData[pos] = inputData[sample];
        }
    }

    // Clear output buffer
    bufferToFill.clearActiveBufferRegion();

    // Mix each input to all outputs with appropriate delays
    for (int inChannel = 0; inChannel < numChannels; ++inChannel)
    {
        auto* delayData = delayBuffer.getReadPointer(inChannel);

        for (int outChannel = 0; outChannel < numChannels; ++outChannel)
        {
            auto* outputData = bufferToFill.buffer->getWritePointer(outChannel, bufferToFill.startSample);

            // Calculate delay: shortest for matching channel, increasing by 200ms for each step
            int delaySteps = (outChannel - inChannel + numChannels) % numChannels;
            int delaySamples = delayIncrement * (delaySteps + 1);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                int readPos = (writePosition + sample - delaySamples + delayBufferLength) % delayBufferLength;
                outputData[sample] += delayData[readPos];
            }
        }
    }

    // Advance write position for next block
    writePosition = (writePosition + numSamples) % delayBufferLength;
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    delayBuffer.setSize(0, 0);
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

    // Toggle button at the top
    processingToggle.setBounds(bounds.removeFromTop(40).reduced(10));

    // Audio setup component takes the rest
    if (audioSetupComp != nullptr)
        audioSetupComp->setBounds(bounds);
}
