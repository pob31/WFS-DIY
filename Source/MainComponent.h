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

    // Track device type and device name changes
    juce::String lastSavedDeviceType;
    juce::String lastSavedDeviceName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
