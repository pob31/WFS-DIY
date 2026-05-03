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
#include "DSP/ReverbEngine.h"
#include "DSP/ReverbFeedThread.h"
#include "DSP/SharedInputRingBuffer.h"
// #include "DSP/GpuInputBufferAlgorithm.h"  // Commented out - GPU Audio SDK not configured
#include "WfsParameters.h"
#include "gui/HelpCard.h"
#include "Accessibility/TTSManager.h"
#include "gui/StatusBar.h"
#include "gui/UpdateBanner.h"
#include "gui/SystemConfigTab.h"
#include "gui/NetworkTab.h"
#include "gui/OutputsTab.h"
#include "gui/InputsTab.h"
#include "gui/ClustersTab.h"
#include "gui/ReverbTab.h"
#include "gui/MapTab.h"
#include "gui/AudioInterfaceWindow.h"
#include "gui/MapTabWindow.h"
#include "gui/MapTabPlaceholder.h"
#include "gui/NetworkLogWindow.h"
#include "gui/MCPUndoOverlay.h"
#include "gui/MCPHistoryWindow.h"
#include "gui/LevelMeterWindow.h"
#include "gui/ColorScheme.h"
#include "DSP/LevelMeteringManager.h"
#include "gui/WfsLookAndFeel.h"
#include "gui/GettingStartedWizard.h"
#include "Network/OSCManager.h"
#include "Network/MCP/MCPServer.h"
#include "Controllers/DialsAndButtons/StreamDeckManager.h"
#include "Controllers/DialsAndButtons/QuickKeysManager.h"
#include "Controllers/DialsAndButtons/pages/PatchWindowPages.h"
#include "Controllers/PositionControl/ControllerManager.h"
#include "Controllers/Sampler/LightpadManager.h"
#include "Controllers/Touch/TouchManager.h"
#if defined (__linux__)
  #include "gui/LinuxTouchscreenWindow.h"
#endif
#include "GradientMap/GradientMapEvaluator.h"
#include "GradientMap/GradientMapData.h"
#include "Sampler/SamplerManager.h"

//==============================================================================
/**
 * Custom TabbedComponent that announces tab changes for screen reader accessibility
 */
class AccessibleTabbedComponent : public juce::TabbedComponent
{
public:
    AccessibleTabbedComponent(juce::TabbedButtonBar::Orientation orientation)
        : juce::TabbedComponent(orientation) {}

    /** Optional callback fired when the active tab changes */
    std::function<void(int)> onTabChanged;

    void currentTabChanged(int newCurrentTabIndex, const juce::String& newCurrentTabName) override
    {
        juce::TabbedComponent::currentTabChanged(newCurrentTabIndex, newCurrentTabName);

        if (onTabChanged)
            onTabChanged(newCurrentTabIndex);

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

    // Audio device access (for crash handler)
    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }

    // Processing state query (for quit confirmation)
    bool isProcessingActive() const { return processingEnabled && audioEngineStarted; }

    // Open a project from a .wfs manifest file (auto-loads config)
    void openProjectFromFile (const juce::File& projectFolder);

    // Audio Interface Window
    void openAudioInterfaceWindow();
    void setupPatchWindowStreamDeck (PatchWindowPages::PatchCallbacks& cb,
                                     PatchWindowPages::PatchStateQueries& q);

    // Detachable Map Window
    void detachMapTab();
    void attachMapTab();

    // Network Log Window
    void openNetworkLogWindow();

    // AI History Window (Phase 5d) — persistent navigator over the MCP
    // change-record + redo rings; complements the transient toast overlay.
    void openMCPHistoryWindow();

    // Level Meter Window
    void openLevelMeterWindow();

    // Getting Started Wizard
    void openGettingStartedWizard();
    void closeGettingStartedWizard();

    // Update notification
    void showUpdateBanner (const juce::String& version, const juce::String& url);

    // Gradient Map Support
    void updateGradientMapStageBounds();
    void rebuildGradientMapForInput (int channelIndex);
    void rebuildAllGradientMaps();

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
    std::unique_ptr<MapTab> mapTab;                          // Owned here, not by TabbedComponent
    std::unique_ptr<MapTabWindow> mapTabWindow;              // Non-null when map is detached
    std::unique_ptr<MapTabPlaceholder> mapTabPlaceholder;    // Shown in tab 6 when map is detached

    std::unique_ptr<AudioInterfaceWindow> audioInterfaceWindow;
    std::unique_ptr<NetworkLogWindow> networkLogWindow;
    std::unique_ptr<MCPHistoryWindow> mcpHistoryWindow;
    std::unique_ptr<LevelMeterWindow> levelMeterWindow;
    std::unique_ptr<LevelMeteringManager> levelMeteringManager;
    std::unique_ptr<GettingStartedWizard> gettingStartedWizard;
    std::unique_ptr<UpdateBanner> updateBanner;
    int lastWizardStepIndex = 0;

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
    std::atomic<bool> audioEngineStarted { false };
    std::atomic<double> currentDeviceSampleRate { 48000.0 };

    // Tracks previous sampler-playing state per input for transition detection
    // in the 50Hz remote-sender timer (message thread only).
    std::vector<uint8_t> prevSamplerPlaying;
    std::atomic<bool> soloReverbs { false };
    std::atomic<bool> muteReverbPre  { false };
    std::atomic<bool> muteReverbPost { false };

    // Parameter management system
    WfsParameters parameters;

    // Network OSC management
    std::unique_ptr<WFSNetwork::OSCManager> oscManager;

    // MCP server (AI control surface — Phase 1 Block 3 skeleton).
    // Started after the audio engine + parameter system are ready; the
    // NetworkTab UI surfaces start/stop/port in Phase 1 Block 6.
    std::unique_ptr<WFSNetwork::MCPServer> mcpServer;

    // Phase 5c: growing-toast overlay rendering AI-undo records. Owned
    // here so it can sit on top of the tab interface; positioned via
    // resized().
    std::unique_ptr<MCPUndoOverlay> mcpUndoOverlay;

    // Stream Deck+ physical controller
    std::unique_ptr<StreamDeckManager> streamDeckManager;
    std::unique_ptr<QuickKeysManager> quickKeysManager;

    // Input controllers (SpaceMouse, joystick, gamepad)
    std::unique_ptr<ControllerManager> controllerManager;

    // ROLI Lightpad Block controllers
    std::unique_ptr<LightpadManager> lightpadManager;

    // Linux multitouch (no-op stub on macOS/Windows)
    std::unique_ptr<WFSTouch::EvdevTouchManager> touchManager;

   #if defined (__linux__)
    // Settings dialog for mapping touchscreens to displays. Lazily created.
    std::unique_ptr<LinuxTouchscreenWindow> touchscreenWindow;
   #endif

    // WFS calculation engine (computes delays, levels, HF attenuation)
    std::unique_ptr<WFSCalculationEngine> calculationEngine;

    // Binaural solo monitoring
    std::unique_ptr<BinauralCalculationEngine> binauralCalcEngine;
    std::unique_ptr<BinauralProcessor> binauralProcessor;

    // Shared input ring buffers (written by audio callback, read by algorithm + reverb thread)
    std::vector<std::unique_ptr<SharedInputRingBuffer>> sharedInputBuffers;

    // Reverb feed thread (computes reverb feeds off the audio callback)
    std::unique_ptr<ReverbFeedThread> reverbFeedThread;

    // Reverb engine (thread-based DSP processing)
    std::unique_ptr<ReverbEngine> reverbEngine;
    juce::AudioBuffer<float> reverbFeedBuffer;    // numReverbs channels, accumulates per-node feed sums
    juce::AudioBuffer<float> reverbReturnBuffer;  // numReverbs channels, receives wet reverb output
    std::vector<float> reverbFeedTemp;            // Temporary per-sample feed accumulation
    int reverbSRRatio = 1;                       // systemSR / reverbSR (integer, 1 = no conversion)
    juce::AudioBuffer<float> reverbDownsampleBuf; // downsampled feed buffer
    juce::AudioBuffer<float> reverbUpsampleBuf;   // downsampled return buffer
#if REVERB_DIAGNOSTICS
    std::unique_ptr<ReverbDiagnosticReporter> reverbDiagReporter;
#endif

    // LFO processor for input position modulation
    std::unique_ptr<LFOProcessor> lfoProcessor;

    // AutomOtion processor for programmed input position movement
    std::unique_ptr<AutomOtionProcessor> automOtionProcessor;

    // Input speed limiter for smooth position movement
    std::unique_ptr<InputSpeedLimiter> speedLimiter;

    // Gradient map evaluators (one per input channel, bitmap-based O(1) lookup)
    std::vector<std::unique_ptr<GradientMapEvaluator>> gradientMapEvaluators;

    // Per-channel sampler engine manager
    std::unique_ptr<SamplerManager> samplerManager;

    // Track last remote position timestamp per channel for auto-stop recording
    // Key: channelIndex, Value: timestamp in milliseconds
    std::map<int, juce::int64> remoteWaypointTimestamps;
    static constexpr int remoteWaypointTimeoutMs = 500;  // Auto-stop after 500ms of no positions

    // Track last sent composite deltas to Remote targets
    // Key: inputIndex (0-based), Value: (deltaX, deltaY)
    std::map<int, std::pair<float, float>> lastSentCompositeDeltas;

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
    juce::AudioBuffer<float> wfsOutputBuffer;  // Algorithm writes here, then single remap to HW outputs

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
    float delaySmoothingFactor = 0.03f;          // ~100ms time constant (slow for Doppler-free delay changes)
    float levelSmoothingFactor = 0.05f;          // ~50ms time constant (faster for levels, less artifact-prone)
    int timerTicksSinceLastRandom = 0;
    const int rampDurationTicks = 200;          // 1 second at 5ms per tick
    int patchSaveCountdown = 0;                 // Debounce timer for auto-saving patch (0 = idle)
    juce::Random random;

    // Master level gain (smoothed for click-free operation)
    std::atomic<float> masterLevelGainTarget { 1.0f };
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> masterLevelGain;

    // Per-output attenuation (smoothed, click-free). Targets live in an atomic[]
    // because std::atomic is not movable and can't go in a std::vector. The
    // SmoothedValue array is audio-thread-only.
    std::unique_ptr<std::atomic<float>[]> outputAttenuationTargets;
    int outputAttenuationTargetsCount = 0;
    std::vector<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative>> outputAttenuationGains;

    // Per-reverb return attenuation (smoothed). Applied to each reverb's wet output
    // signal before mixing into WFS outputs, so the reverb engine runs at full level
    // internally but contributes less to the mix.
    std::unique_ptr<std::atomic<float>[]> reverbAttenuationTargets;
    int reverbAttenuationTargetsCount = 0;
    std::vector<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative>> reverbAttenuationGains;

    // Track device type and device name changes
    juce::String lastSavedDeviceType;
    juce::String lastSavedDeviceName;

    // IR parameter change tracking (avoid redundant pushes every timer tick)
    juce::String lastPushedIRFile;
    float lastPushedIRTrim = -1.0f;
    float lastPushedIRLength = -1.0f;

    // Algorithm type change tracking for session logging
    int lastLoggedAlgoType = -1;

    // Xrun detection for session logging
    int lastXRunCount = 0;
    bool deviceRestoreComplete = false;  // Prevents saving fallback device during startup

    void attachAudioCallbacksIfNeeded();
    void resizeRoutingMatrices();
    void resizeOutputAttenuation(int numOut, double sampleRate);
    void resizeReverbAttenuation(int numReverbs, double sampleRate);
    void stopProcessingForConfigurationChange();
    void loadAudioPatches();  // Load input/output patch matrices from ValueTree
    void applyInputPatch(const juce::AudioSourceChannelInfo& bufferToFill);  // Apply input patching
    void applyOutputPatch(const juce::AudioSourceChannelInfo& bufferToFill,
                          const juce::AudioBuffer<float>& wfsOutput); // Single-pass WFS→HW remap

    // Handlers for callbacks from System Config tab
    void handleProcessingChange(bool enabled);
    void handleChannelCountChange(int inputs, int outputs, int reverbs);
    void handleConfigReloaded();
    void applySamplerSetPosition (int channelIndex, const juce::ValueTree& samplerNode, int setIndex);
    void applySamplerControllerMode (int mode);
    std::map<int, int> buildZoneToInputMap() const;
    void resendRemotePadConfig();
    void growPatchData(juce::ValueTree& patchTree, int newChannelCount, int numHardwareCols);
    void repaintActiveTab();

    // Keyboard handling helpers
    enum class ChannelSelectionMode { None, Input, Output, Reverb };
    ChannelSelectionMode channelSelectionMode = ChannelSelectionMode::None;
    juce::String channelNumberBuffer;
    juce::int64 channelSelectionStartTime = 0;
    static constexpr int channelSelectionTimeoutMs = 5000;

    // Help card cycling (H key)
    int helpCycleIndex = -1;
    int helpCycleStartIndex = -1;  // Anchor where the current cycle began — cycle closes when advancing would return here
    std::vector<HelpCardButton*> helpCycleButtons;
    void resetHelpCycle();
    void cycleHelpCards();

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
