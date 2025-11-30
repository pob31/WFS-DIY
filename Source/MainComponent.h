#pragma once

#include <JuceHeader.h>
#include "InputBufferAlgorithm.h"
#include "OutputBufferAlgorithm.h"
#include "GpuInputBufferAlgorithm.h"
#include "gui/GuiPreviewWindow.h"
#include "gui/DialsPreviewWindow.h"

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

    juce::Label algorithmLabel;
    juce::ComboBox algorithmSelector;
    juce::TextButton uiPreviewButton;
    juce::TextButton dialsPreviewButton;

    std::unique_ptr<GuiPreviewWindow> previewWindow;
    std::unique_ptr<DialsPreviewWindow> dialsPreviewWindow;

    // Threaded processing architecture
    enum class ProcessingAlgorithm
    {
        InputBuffer,   // Read-time delays (current/original approach)
        OutputBuffer,  // Write-time delays (alternative approach)
        GpuInputBuffer // GPU Audio-backed input-buffer variant
    };

    ProcessingAlgorithm currentAlgorithm = ProcessingAlgorithm::InputBuffer;
    int numInputChannels = 4;
    int numOutputChannels = 4;
    InputBufferAlgorithm inputAlgorithm;
    OutputBufferAlgorithm outputAlgorithm;
    GpuInputBufferAlgorithm gpuInputAlgorithm;
    bool audioCallbacksAttached = false;
    bool processingEnabled = false;
    bool audioEngineStarted = false;

    // Routing matrix: delays[inputChannel * numOutputChannels + outputChannel]
    std::vector<float> delayTimesMs;
    // Gain matrix: levels[inputChannel * numOutputChannels + outputChannel]
    std::vector<float> levels;

    // Random generator with ramping and exponential smoothing (temporary for testing)
    std::vector<float> targetDelayTimesMs;      // Current ramp targets (updated every tick)
    std::vector<float> targetLevels;            // Current ramp targets (updated every tick)
    std::vector<float> finalTargetDelayTimesMs; // Final destination for 1-second ramp
    std::vector<float> finalTargetLevels;       // Final destination for 1-second ramp
    std::vector<float> startDelayTimesMs;       // Starting values for 1-second ramp
    std::vector<float> startLevels;             // Starting values for 1-second ramp
    float smoothingFactor = 0.22f;              // ~20ms settling time with 5ms updates
    int timerTicksSinceLastRandom = 0;
    const int rampDurationTicks = 200;          // 1 second at 5ms per tick
    juce::Random random;

    // Track device type and device name changes
    juce::String lastSavedDeviceType;
    juce::String lastSavedDeviceName;

    void attachAudioCallbacksIfNeeded();
    void resizeRoutingMatrices();
    void stopProcessingForConfigurationChange();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
