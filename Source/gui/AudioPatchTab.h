#pragma once

#include <JuceHeader.h>
#include "PatchMatrixComponent.h"
#include "../WfsParameters.h"
#include "../DSP/TestSignalGenerator.h"

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

    juce::Label signalTypeLabel{"", "Signal:"};
    juce::ComboBox signalTypeCombo;

    juce::Label frequencyLabel{"", "Frequency:"};
    juce::Slider frequencySlider;

    juce::Label levelLabel{"", "Level:"};
    juce::Slider levelSlider;

    juce::ToggleButton holdButton{"Hold"};

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

private:
    WFSValueTreeState& parameters;

    // Mode buttons
    juce::TextButton scrollingButton{"Scrolling"};
    juce::TextButton patchingButton{"Patching"};
    juce::TextButton unpatchAllButton{"Unpatch All"};

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
 * Includes test signal control panel for testing mode.
 */
class OutputPatchTab : public juce::Component
{
public:
    OutputPatchTab(WFSValueTreeState& valueTreeState,
                   TestSignalGenerator* testSignalGen);
    ~OutputPatchTab() override = default;

    void resized() override;
    void paint(juce::Graphics& g) override;

    /** Notify that processing state changed (stops test signals) */
    void setProcessingStateChanged(bool isProcessing);

    /** Reset mode to scrolling (called when leaving tab or window) */
    void resetMode();

private:
    WFSValueTreeState& parameters;
    TestSignalGenerator* testSignalGenerator;

    // Mode buttons
    juce::TextButton scrollingButton{"Scrolling"};
    juce::TextButton patchingButton{"Patching"};
    juce::TextButton testingButton{"Testing"};
    juce::TextButton unpatchAllButton{"Unpatch All"};

    // Patch matrix
    std::unique_ptr<PatchMatrixComponent> patchMatrix;

    // Test signal controls
    std::unique_ptr<TestSignalControlPanel> testControlPanel;

    void setMode(PatchMatrixComponent::Mode mode);
    void handleUnpatchAll();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputPatchTab)
    JUCE_DECLARE_WEAK_REFERENCEABLE(OutputPatchTab)
};
