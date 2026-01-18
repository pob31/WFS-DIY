#pragma once

#include <JuceHeader.h>
#include <map>
#include "DSP/InputBufferAlgorithm.h"
#include "DSP/OutputBufferAlgorithm.h"
#include "DSP/WFSCalculationEngine.h"
#include "DSP/LFOProcessor.h"
#include "DSP/AutomOtionProcessor.h"
#include "DSP/InputSpeedLimiter.h"
#include "DSP/LiveSourceTamerEngine.h"
#include "DSP/TestSignalGenerator.h"
#include "DSP/BinauralCalculationEngine.h"
#include "DSP/BinauralProcessor.h"
// #include "DSP/GpuInputBufferAlgorithm.h"  // Commented out - GPU Audio SDK not configured
#include "WfsParameters.h"
#include "Accessibility/TTSManager.h"
#include "gui/StatusBar.h"
#include "gui/SystemConfigTab.h"
#include "gui/NetworkTab.h"
#include "gui/OutputsTab.h"
#include "gui/InputsTab.h"
#include "gui/ClustersTab.h"
#include "gui/ReverbTab.h"
#include "gui/MapTab.h"
#include "gui/AudioInterfaceWindow.h"
#include "gui/NetworkLogWindow.h"
#include "gui/LevelMeterWindow.h"
#include "gui/ColorScheme.h"
#include "DSP/LevelMeteringManager.h"
#include "gui/WfsLookAndFeel.h"
#include "Network/OSCManager.h"

//==============================================================================
/**
 * Custom TabbedComponent that announces tab changes for screen reader accessibility
 */
class AccessibleTabbedComponent : public juce::TabbedComponent
{
public:
    AccessibleTabbedComponent(juce::TabbedButtonBar::Orientation orientation)
        : juce::TabbedComponent(orientation) {}

    void currentTabChanged(int newCurrentTabIndex, const juce::String& newCurrentTabName) override
    {
        juce::TabbedComponent::currentTabChanged(newCurrentTabIndex, newCurrentTabName);

        // Announce tab change for screen readers (only if name is valid)
        if (newCurrentTabName.isNotEmpty())
        {
            juce::String announcement = newCurrentTabName + " tab";
            TTSManager::getInstance().announceImmediate(
                announcement,
                juce::AccessibilityHandler::AnnouncementPriority::medium);
        }
    }
};

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent,
                         private juce::Timer,
                         private juce::ChangeListener,
                         private ColorScheme::Manager::Listener
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

    // Keyboard handling
    bool keyPressed(const juce::KeyPress& key) override;

    void startAudioEngine();
    void saveSettings();
    void timerCallback() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void colorSchemeChanged() override;

    // Routing matrix access
    void setDelay(int inputChannel, int outputChannel, float delayMs);
    void setLevel(int inputChannel, int outputChannel, float level);
    float getDelay(int inputChannel, int outputChannel) const;
    float getLevel(int inputChannel, int outputChannel) const;

    // Audio Interface Window
    void openAudioInterfaceWindow();

    // Network Log Window
    void openNetworkLogWindow();

    // Level Meter Window
    void openLevelMeterWindow();

private:
    //==============================================================================
    // Your private member variables go here...
    // audioSetupComp moved to AudioInterfaceWindow
    juce::ToggleButton processingToggle;

    juce::Label numInputsLabel;
    juce::Label numOutputsLabel;
    juce::Slider numInputsSlider;
    juce::Slider numOutputsSlider;

    juce::Label algorithmLabel;
    juce::ComboBox algorithmSelector;

    // Main tabbed interface with status bar
    AccessibleTabbedComponent tabbedComponent { juce::TabbedButtonBar::TabsAtTop };
    StatusBar* statusBar = nullptr;  // Owned by container component
    SystemConfigTab* systemConfigTab = nullptr;  // Owned by TabbedComponent
    NetworkTab* networkTab = nullptr;
    OutputsTab* outputsTab = nullptr;
    InputsTab* inputsTab = nullptr;
    ClustersTab* clustersTab = nullptr;
    ReverbTab* reverbTab = nullptr;
    MapTab* mapTab = nullptr;

    std::unique_ptr<AudioInterfaceWindow> audioInterfaceWindow;
    std::unique_ptr<NetworkLogWindow> networkLogWindow;
    std::unique_ptr<LevelMeterWindow> levelMeterWindow;
    std::unique_ptr<LevelMeteringManager> levelMeteringManager;

    // Custom LookAndFeel for centralized widget theming
    std::unique_ptr<WfsLookAndFeel> wfsLookAndFeel;

    // Global tooltip window for hover tooltips
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;

    // Threaded processing architecture
    enum class ProcessingAlgorithm
    {
        InputBuffer,   // Read-time delays (current/original approach)
        OutputBuffer   // Write-time delays (alternative approach)
        // GpuInputBuffer // GPU Audio-backed input-buffer variant (commented out - GPU Audio SDK not configured)
    };

    ProcessingAlgorithm currentAlgorithm = ProcessingAlgorithm::InputBuffer;
    int numInputChannels = 4;
    int numOutputChannels = 4;
    InputBufferAlgorithm inputAlgorithm;
    OutputBufferAlgorithm outputAlgorithm;
    // GpuInputBufferAlgorithm gpuInputAlgorithm;  // Commented out - GPU Audio SDK not configured
    bool audioCallbacksAttached = false;
    bool processingEnabled = false;
    bool audioEngineStarted = false;

    // Parameter management system
    WfsParameters parameters;

    // Network OSC management
    std::unique_ptr<WFSNetwork::OSCManager> oscManager;

    // WFS calculation engine (computes delays, levels, HF attenuation)
    std::unique_ptr<WFSCalculationEngine> calculationEngine;

    // Binaural solo monitoring
    std::unique_ptr<BinauralCalculationEngine> binauralCalcEngine;
    std::unique_ptr<BinauralProcessor> binauralProcessor;

    // LFO processor for input position modulation
    std::unique_ptr<LFOProcessor> lfoProcessor;

    // AutomOtion processor for programmed input position movement
    std::unique_ptr<AutomOtionProcessor> automOtionProcessor;

    // Input speed limiter for smooth position movement
    std::unique_ptr<InputSpeedLimiter> speedLimiter;

    // Track last remote position timestamp per channel for auto-stop recording
    // Key: channelIndex, Value: timestamp in milliseconds
    std::map<int, juce::int64> remoteWaypointTimestamps;
    static constexpr int remoteWaypointTimeoutMs = 500;  // Auto-stop after 500ms of no positions

    // Live Source Tamer engine for per-speaker gain reduction
    std::unique_ptr<LiveSourceTamerEngine> lsTamerEngine;

    // Test signal generator for audio interface testing
    std::unique_ptr<TestSignalGenerator> testSignalGenerator;

    // Audio patch matrices: hardware channel → WFS channel mappings
    // inputPatchMap[hardwareChannel] = wfsChannel (-1 if unmapped)
    std::vector<int> inputPatchMap;
    // outputPatchMap[wfsChannel] = hardwareChannel (-1 if unmapped)
    std::vector<int> outputPatchMap;
    // Temporary buffers for patch application
    juce::AudioBuffer<float> patchedInputBuffer;
    juce::AudioBuffer<float> patchedOutputBuffer;

    // Routing matrix: delays[inputChannel * numOutputChannels + outputChannel]
    std::vector<float> delayTimesMs;
    // Gain matrix: levels[inputChannel * numOutputChannels + outputChannel]
    std::vector<float> levels;
    // HF attenuation matrix (dB): hfAttenuation[inputChannel * numOutputChannels + outputChannel]
    std::vector<float> hfAttenuation;

    // Floor Reflection matrices: [inputChannel * numOutputChannels + outputChannel]
    std::vector<float> frDelayTimesMs;   // Extra delay for reflected path (ms)
    std::vector<float> frLevels;         // Linear gain for reflected signal
    std::vector<float> frHFAttenuation;  // HF attenuation for reflected path (dB)

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
    bool deviceRestoreComplete = false;  // Prevents saving fallback device during startup

    void attachAudioCallbacksIfNeeded();
    void resizeRoutingMatrices();
    void stopProcessingForConfigurationChange();
    void loadAudioPatches();  // Load input/output patch matrices from ValueTree
    void applyInputPatch(const juce::AudioSourceChannelInfo& bufferToFill);  // Apply input patching
    void applyOutputPatch(const juce::AudioSourceChannelInfo& bufferToFill); // Apply output patching

    // Handlers for callbacks from System Config tab
    void handleProcessingChange(bool enabled);
    void handleChannelCountChange(int inputs, int outputs, int reverbs);
    void handleConfigReloaded();

    // Keyboard handling helpers
    enum class ChannelSelectionMode { None, Input, Output, Reverb };
    ChannelSelectionMode channelSelectionMode = ChannelSelectionMode::None;
    juce::String channelNumberBuffer;
    juce::int64 channelSelectionStartTime = 0;
    static constexpr int channelSelectionTimeoutMs = 5000;

    bool isTextEditorFocused() const;
    void startChannelSelection(ChannelSelectionMode mode);
    void cancelChannelSelection();
    void confirmChannelSelection();
    void cycleChannel(int delta);
    void nudgeInputPosition(int axis, float delta, int inputOverride = -1);  // axis: 0=X, 1=Y, 2=Z
    void nudgeOutputPosition(int axis, float delta);  // axis: 0=X, 1=Y, 2=Z
    void nudgeReverbPosition(int axis, float delta);  // axis: 0=X, 1=Y, 2=Z

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
