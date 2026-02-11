#pragma once

#include <JuceHeader.h>
#include "PatchMatrixComponent.h"
#include "sliders/WfsStandardSlider.h"
#include "../WfsParameters.h"
#include "../DSP/TestSignalGenerator.h"
#include "../Localization/LocalizationManager.h"

/**
 * TestSignalControlPanel
 *
 * UI controls for test signal generation in output patch testing mode.
 * Includes signal type selection, frequency, level, and hold button.
 */
class TestSignalControlPanel : public juce::Component
{
public:
    TestSignalControlPanel(TestSignalGenerator* testSignalGen);
    ~TestSignalControlPanel() override = default;

    void resized() override;
    void paint(juce::Graphics& g) override;

    /** Enable/disable all controls */
    void setEnabled(bool shouldBeEnabled);

    /** Sync UI controls with current generator state (called after external changes) */
    void syncFromGenerator();

    /** Callback when hold is disabled (to stop test signal and clear highlighting) */
    std::function<void()> onHoldDisabled;

private:
    TestSignalGenerator* testSignalGenerator;

    juce::Label signalTypeLabel;
    juce::ComboBox signalTypeCombo;

    juce::Label frequencyLabel;
    juce::Slider frequencySlider;

    juce::Label levelLabel;
    juce::Slider levelSlider;

    juce::ToggleButton holdButton;

    void updateFrequencyVisibility();
    void applySettings();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestSignalControlPanel)
};

/**
 * InputPatchTab
 *
 * Tab for input patch matrix with Scrolling and Patching modes.
 */
class InputPatchTab : public juce::Component
{
public:
    InputPatchTab(WFSValueTreeState& valueTreeState);
    ~InputPatchTab() override = default;

    void resized() override;
    void paint(juce::Graphics& g) override;

    /** Reset mode to scrolling (called when leaving tab or window) */
    void resetMode();

    /** Give keyboard focus to the patch matrix for arrow key navigation */
    void grabPatchMatrixFocus();

private:
    WFSValueTreeState& parameters;

    // Mode buttons
    juce::TextButton scrollingButton;
    juce::TextButton patchingButton;
    juce::TextButton unpatchAllButton;

    // Patch matrix
    std::unique_ptr<PatchMatrixComponent> patchMatrix;

    void setMode(PatchMatrixComponent::Mode mode);
    void handleUnpatchAll();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InputPatchTab)
    JUCE_DECLARE_WEAK_REFERENCEABLE(InputPatchTab)
};

/**
 * OutputPatchTab
 *
 * Tab for output patch matrix with Scrolling, Patching, and Testing modes.
 * Test signal controls appear inline in the button bar when in Testing mode.
 */
class OutputPatchTab : public juce::Component,
                       public juce::Label::Listener,
                       private juce::Timer
{
public:
    OutputPatchTab(WFSValueTreeState& valueTreeState,
                   TestSignalGenerator* testSignalGen);
    ~OutputPatchTab() override { stopTimer(); }

    void resized() override;
    void paint(juce::Graphics& g) override;

    /** Notify that processing state changed (stops test signals) */
    void setProcessingStateChanged(bool isProcessing);

    /** Reset mode to scrolling (called when leaving tab or window) */
    void resetMode();

    /** Give keyboard focus to the patch matrix for arrow key navigation */
    void grabPatchMatrixFocus();

    /** Stop test audio without resetting settings (for tab switching) */
    void stopTestAudio();

    /** Callback for status bar messages */
    std::function<void(const juce::String&)> onStatusMessage;

private:
    WFSValueTreeState& parameters;
    TestSignalGenerator* testSignalGenerator;

    // Mode buttons
    juce::TextButton scrollingButton;
    juce::TextButton patchingButton;
    juce::TextButton testingButton;
    juce::TextButton unpatchAllButton;

    // Patch matrix
    std::unique_ptr<PatchMatrixComponent> patchMatrix;

    // Inline test signal controls (shown when in Testing mode)
    juce::ComboBox signalTypeCombo;
    juce::TextButton holdButton;
    WfsStandardSlider levelSlider;
    juce::Label levelValueLabel;
    WfsStandardSlider frequencySlider;  // For Tone mode (after level)
    juce::Label frequencyValueLabel;

    // Status message label (shown briefly when action blocked)
    juce::Label statusMessageLabel;
    void showTemporaryMessage(const juce::String& msg);
    void timerCallback() override;

    void setMode(PatchMatrixComponent::Mode mode);
    void handleUnpatchAll();
    void applyTestSettings();
    void updateTestControlsVisibility(bool visible);
    void updateTestControlsEnabledState();
    void updateFrequencyVisibility();
    void updateFrequencySliderColor();
    void labelTextChanged(juce::Label* label) override;
    float sliderValueToDb(float sliderValue) const;
    float dbToSliderValue(float dB) const;
    float sliderValueToFrequency(float sliderValue) const;
    float frequencyToSliderValue(float freq) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputPatchTab)
    JUCE_DECLARE_WEAK_REFERENCEABLE(OutputPatchTab)
};
