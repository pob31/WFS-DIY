#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent
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

private:
    //==============================================================================
    // Your private member variables go here...
    std::unique_ptr<juce::AudioDeviceSelectorComponent> audioSetupComp;
    juce::ToggleButton processingToggle;

    // Delay buffers - 1 second circular buffers for each channel
    juce::AudioBuffer<float> delayBuffer;
    int delayBufferLength = 0;
    int writePosition = 0;
    double currentSampleRate = 0.0;
    bool processingEnabled = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
