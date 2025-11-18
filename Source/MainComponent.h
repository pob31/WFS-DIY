#pragma once

#include <JuceHeader.h>
#include "InputProcessor.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent,
                         private juce::Timer
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

    void startAudioEngine();
    void saveSettings();
    void timerCallback() override;

    // Routing matrix access
    void setDelay(int inputChannel, int outputChannel, float delayMs);
    void setLevel(int inputChannel, int outputChannel, float level);
    float getDelay(int inputChannel, int outputChannel) const;
    float getLevel(int inputChannel, int outputChannel) const;

private:
    //==============================================================================
    // Your private member variables go here...
    std::unique_ptr<juce::AudioDeviceSelectorComponent> audioSetupComp;
    juce::ToggleButton processingToggle;

    juce::Label numInputsLabel;
    juce::Label numOutputsLabel;
    juce::Slider numInputsSlider;
    juce::Slider numOutputsSlider;

    // Threaded processing architecture
    int numInputChannels = 4;
    int numOutputChannels = 4;
    std::vector<std::unique_ptr<InputProcessor>> inputProcessors;
    bool processingEnabled = false;
    bool audioEngineStarted = false;

    // Routing matrix: delays[inputChannel * numOutputChannels + outputChannel]
    std::vector<float> delayTimesMs;
    // Gain matrix: levels[inputChannel * numOutputChannels + outputChannel]
    std::vector<float> levels;

    // Random generator with exponential smoothing (temporary for testing)
    std::vector<float> targetDelayTimesMs;
    std::vector<float> targetLevels;
    float smoothingFactor = 0.22f;  // ~20ms settling time with 5ms updates
    int timerTicksSinceLastRandom = 0;
    juce::Random random;

    // Track device type and device name changes
    juce::String lastSavedDeviceType;
    juce::String lastSavedDeviceName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
