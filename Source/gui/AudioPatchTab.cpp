#include "AudioPatchTab.h"
#include "ColorScheme.h"

//==============================================================================
// TestSignalControlPanel Implementation
//==============================================================================

TestSignalControlPanel::TestSignalControlPanel(TestSignalGenerator* testSignalGen)
    : testSignalGenerator(testSignalGen)
{
    // Signal type selector
    addAndMakeVisible(signalTypeLabel);
    signalTypeLabel.setJustificationType(juce::Justification::centredRight);
    signalTypeLabel.setFont(juce::FontOptions(14.0f));

    addAndMakeVisible(signalTypeCombo);
    signalTypeCombo.addItem("Off", 1);
    signalTypeCombo.addItem("Pink Noise", 2);
    signalTypeCombo.addItem("Tone", 3);
    signalTypeCombo.addItem("Sweep", 4);
    signalTypeCombo.addItem("Dirac Pulse", 5);
    signalTypeCombo.setSelectedId(1, juce::dontSendNotification);  // Default: Off
    signalTypeCombo.onChange = [this]() {
        updateFrequencyVisibility();
        // Auto-boost level from -inf to audible when selecting a signal type
        if (signalTypeCombo.getSelectedId() > 1 && levelSlider.getValue() < -80.0)
        {
            levelSlider.setValue(-40.0, juce::sendNotification);  // Set to -40 dB
        }
        applySettings();
    };

    // Frequency slider (for Tone mode)
    addAndMakeVisible(frequencyLabel);
    frequencyLabel.setJustificationType(juce::Justification::centredRight);
    frequencyLabel.setFont(juce::FontOptions(14.0f));

    addAndMakeVisible(frequencySlider);
    frequencySlider.setRange(20, 20000, 1);
    frequencySlider.setValue(1000);
    frequencySlider.setSkewFactorFromMidPoint(1000);
    frequencySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20);
    frequencySlider.setTextValueSuffix(" Hz");
    frequencySlider.onValueChange = [this]() { applySettings(); };

    // Level slider
    addAndMakeVisible(levelLabel);
    levelLabel.setJustificationType(juce::Justification::centredRight);
    levelLabel.setFont(juce::FontOptions(14.0f));

    addAndMakeVisible(levelSlider);
    levelSlider.setRange(-92, 0, 0.1);
    levelSlider.setValue(-40);  // Default: -40 dB
    levelSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    levelSlider.setTextValueSuffix(" dB");
    levelSlider.onValueChange = [this]() { applySettings(); };

    // Hold button
    addAndMakeVisible(holdButton);
    holdButton.setClickingTogglesState(true);
    holdButton.onClick = [this]()
    {
        applySettings();

        // When hold is disabled, stop the test signal and clear highlighting
        if (!holdButton.getToggleState() && onHoldDisabled)
        {
            onHoldDisabled();
        }
    };

    // Initial state
    updateFrequencyVisibility();
    applySettings();
}

void TestSignalControlPanel::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    const int labelWidth = 80;
    const int spacing = 10;
    const int rowHeight = 30;

    // Row 1: Signal type and hold button
    auto row1 = bounds.removeFromTop(rowHeight);
    signalTypeLabel.setBounds(row1.removeFromLeft(labelWidth));
    row1.removeFromLeft(spacing);
    signalTypeCombo.setBounds(row1.removeFromLeft(150));
    row1.removeFromLeft(spacing * 2);
    holdButton.setBounds(row1.removeFromLeft(80));

    bounds.removeFromTop(spacing);

    // Row 2: Frequency (only visible for Tone mode)
    auto row2 = bounds.removeFromTop(rowHeight);
    frequencyLabel.setBounds(row2.removeFromLeft(labelWidth));
    row2.removeFromLeft(spacing);
    frequencySlider.setBounds(row2.removeFromLeft(250));

    bounds.removeFromTop(spacing);

    // Row 3: Level
    auto row3 = bounds.removeFromTop(rowHeight);
    levelLabel.setBounds(row3.removeFromLeft(labelWidth));
    row3.removeFromLeft(spacing);
    levelSlider.setBounds(row3.removeFromLeft(250));
}

void TestSignalControlPanel::paint(juce::Graphics& g)
{
    g.fillAll(ColorScheme::get().backgroundAlt);

    // Draw border
    g.setColour(ColorScheme::get().chromeDivider);
    g.drawRect(getLocalBounds(), 1);
}

void TestSignalControlPanel::setEnabled(bool shouldBeEnabled)
{
    signalTypeCombo.setEnabled(shouldBeEnabled);
    frequencySlider.setEnabled(shouldBeEnabled);
    levelSlider.setEnabled(shouldBeEnabled);
    holdButton.setEnabled(shouldBeEnabled);
}

void TestSignalControlPanel::updateFrequencyVisibility()
{
    // Only show frequency for Tone mode (ID 3)
    bool isToneMode = signalTypeCombo.getSelectedId() == 3;

    frequencyLabel.setVisible(isToneMode);
    frequencySlider.setVisible(isToneMode);
}

void TestSignalControlPanel::applySettings()
{
    if (!testSignalGenerator)
        return;

    // Set signal type
    int selectedId = signalTypeCombo.getSelectedId();
    TestSignalGenerator::SignalType signalType = TestSignalGenerator::SignalType::Off;

    switch (selectedId)
    {
        case 1: signalType = TestSignalGenerator::SignalType::Off; break;
        case 2: signalType = TestSignalGenerator::SignalType::PinkNoise; break;
        case 3: signalType = TestSignalGenerator::SignalType::Tone; break;
        case 4: signalType = TestSignalGenerator::SignalType::Sweep; break;
        case 5: signalType = TestSignalGenerator::SignalType::DiracPulse; break;
    }

    testSignalGenerator->setSignalType(signalType);

    // Set frequency (for Tone mode)
    testSignalGenerator->setFrequency(static_cast<float>(frequencySlider.getValue()));

    // Set level
    testSignalGenerator->setLevel(static_cast<float>(levelSlider.getValue()));

    // Set hold mode
    testSignalGenerator->setHoldEnabled(holdButton.getToggleState());
}

void TestSignalControlPanel::syncFromGenerator()
{
    if (!testSignalGenerator)
        return;

    // Sync signal type combo
    int comboId = 1;  // Default: Off
    switch (testSignalGenerator->getSignalType())
    {
        case TestSignalGenerator::SignalType::Off:        comboId = 1; break;
        case TestSignalGenerator::SignalType::PinkNoise:  comboId = 2; break;
        case TestSignalGenerator::SignalType::Tone:       comboId = 3; break;
        case TestSignalGenerator::SignalType::Sweep:      comboId = 4; break;
        case TestSignalGenerator::SignalType::DiracPulse: comboId = 5; break;
    }
    signalTypeCombo.setSelectedId(comboId, juce::dontSendNotification);

    // Sync level slider
    float levelDb = testSignalGenerator->getLevelDb();
    levelSlider.setValue(levelDb, juce::dontSendNotification);

    // Update frequency visibility based on current signal type
    updateFrequencyVisibility();
}

//==============================================================================
// InputPatchTab Implementation
//==============================================================================

InputPatchTab::InputPatchTab(WFSValueTreeState& valueTreeState)
    : parameters(valueTreeState)
{
    // Create mode buttons
    addAndMakeVisible(scrollingButton);
    scrollingButton.setToggleState(true, juce::dontSendNotification);
    scrollingButton.setRadioGroupId(1);
    scrollingButton.setClickingTogglesState(true);
    scrollingButton.onClick = [this]() {
        if (scrollingButton.getToggleState())
            setMode(PatchMatrixComponent::Mode::Scrolling);
    };

    addAndMakeVisible(patchingButton);
    patchingButton.setRadioGroupId(1);
    patchingButton.setClickingTogglesState(true);
    patchingButton.onClick = [this]() {
        if (patchingButton.getToggleState())
            setMode(PatchMatrixComponent::Mode::Patching);
    };

    // Unpatch All button
    addAndMakeVisible(unpatchAllButton);
    unpatchAllButton.onClick = [this]() { handleUnpatchAll(); };

    // Create patch matrix
    patchMatrix = std::make_unique<PatchMatrixComponent>(parameters, true, nullptr);
    addAndMakeVisible(patchMatrix.get());
}

void InputPatchTab::resized()
{
    auto bounds = getLocalBounds();

    // Mode buttons at top
    auto buttonBar = bounds.removeFromTop(40);
    buttonBar.reduce(10, 5);

    const int buttonWidth = 100;
    const int buttonSpacing = 10;

    scrollingButton.setBounds(buttonBar.removeFromLeft(buttonWidth));
    buttonBar.removeFromLeft(buttonSpacing);
    patchingButton.setBounds(buttonBar.removeFromLeft(buttonWidth));

    // Unpatch All button on the right
    unpatchAllButton.setBounds(buttonBar.removeFromRight(buttonWidth));

    // Patch matrix fills remaining space
    patchMatrix->setBounds(bounds);
}

void InputPatchTab::paint(juce::Graphics& g)
{
    g.fillAll(ColorScheme::get().background);
}

void InputPatchTab::setMode(PatchMatrixComponent::Mode mode)
{
    if (patchMatrix)
    {
        patchMatrix->setMode(mode);
    }
}

void InputPatchTab::resetMode()
{
    scrollingButton.setToggleState(true, juce::dontSendNotification);
    setMode(PatchMatrixComponent::Mode::Scrolling);
}

void InputPatchTab::handleUnpatchAll()
{
    // Use weak reference to avoid dangling pointer if tab is destroyed during dialog
    juce::WeakReference<InputPatchTab> weakThis(this);

    juce::AlertWindow::showOkCancelBox(
        juce::MessageBoxIconType::WarningIcon,
        "Unpatch All Inputs",
        "Are you sure you want to remove all input patches?",
        "Unpatch All",
        "Cancel",
        this,
        juce::ModalCallbackFunction::create([weakThis](int result) {
            if (weakThis != nullptr && result == 1)  // 1 = OK button clicked
            {
                if (weakThis->patchMatrix)
                    weakThis->patchMatrix->clearAllPatches();
            }
        }));
}

//==============================================================================
// OutputPatchTab Implementation
//==============================================================================

OutputPatchTab::OutputPatchTab(WFSValueTreeState& valueTreeState,
                               TestSignalGenerator* testSignalGen)
    : parameters(valueTreeState),
      testSignalGenerator(testSignalGen)
{
    // Create mode buttons
    addAndMakeVisible(scrollingButton);
    scrollingButton.setToggleState(true, juce::dontSendNotification);
    scrollingButton.setRadioGroupId(2);
    scrollingButton.setClickingTogglesState(true);
    scrollingButton.onClick = [this]() {
        if (scrollingButton.getToggleState())
            setMode(PatchMatrixComponent::Mode::Scrolling);
    };

    addAndMakeVisible(patchingButton);
    patchingButton.setRadioGroupId(2);
    patchingButton.setClickingTogglesState(true);
    patchingButton.onClick = [this]() {
        if (patchingButton.getToggleState())
            setMode(PatchMatrixComponent::Mode::Patching);
    };

    addAndMakeVisible(testingButton);
    testingButton.setRadioGroupId(2);
    testingButton.setClickingTogglesState(true);
    testingButton.onClick = [this]() {
        if (testingButton.getToggleState())
            setMode(PatchMatrixComponent::Mode::Testing);
    };

    // Unpatch All button
    addAndMakeVisible(unpatchAllButton);
    unpatchAllButton.onClick = [this]() { handleUnpatchAll(); };

    // Create patch matrix
    patchMatrix = std::make_unique<PatchMatrixComponent>(parameters, false, testSignalGen);
    addAndMakeVisible(patchMatrix.get());

    // Create test signal control panel
    testControlPanel = std::make_unique<TestSignalControlPanel>(testSignalGen);
    addAndMakeVisible(testControlPanel.get());
    testControlPanel->setVisible(false);  // Hidden until testing mode

    // Wire up callback to sync control panel when test signal is auto-configured
    patchMatrix->onTestSignalConfigured = [this]()
    {
        if (testControlPanel)
            testControlPanel->syncFromGenerator();
    };

    // Wire up callback to clear active test channel when hold is disabled
    testControlPanel->onHoldDisabled = [this]()
    {
        if (patchMatrix)
            patchMatrix->clearActiveTestChannel();
    };
}

void OutputPatchTab::resized()
{
    auto bounds = getLocalBounds();

    // Mode buttons at top
    auto buttonBar = bounds.removeFromTop(40);
    buttonBar.reduce(10, 5);

    const int buttonWidth = 100;
    const int buttonSpacing = 10;

    scrollingButton.setBounds(buttonBar.removeFromLeft(buttonWidth));
    buttonBar.removeFromLeft(buttonSpacing);
    patchingButton.setBounds(buttonBar.removeFromLeft(buttonWidth));
    buttonBar.removeFromLeft(buttonSpacing);
    testingButton.setBounds(buttonBar.removeFromLeft(buttonWidth));

    // Unpatch All button on the right
    unpatchAllButton.setBounds(buttonBar.removeFromRight(buttonWidth));

    // Test control panel (if visible)
    if (testControlPanel->isVisible())
    {
        auto controlPanel = bounds.removeFromTop(120);
        testControlPanel->setBounds(controlPanel);
    }

    // Patch matrix fills remaining space
    patchMatrix->setBounds(bounds);
}

void OutputPatchTab::paint(juce::Graphics& g)
{
    g.fillAll(ColorScheme::get().background);
}

void OutputPatchTab::setMode(PatchMatrixComponent::Mode mode)
{
    if (patchMatrix)
    {
        patchMatrix->setMode(mode);
    }

    // Show/hide test control panel
    if (testControlPanel)
    {
        testControlPanel->setVisible(mode == PatchMatrixComponent::Mode::Testing);

        // Sync control panel UI when entering testing mode
        if (mode == PatchMatrixComponent::Mode::Testing)
        {
            testControlPanel->syncFromGenerator();
        }

        resized();  // Update layout
    }

    // Reset test signals when leaving testing mode
    if (mode != PatchMatrixComponent::Mode::Testing && testSignalGenerator)
    {
        testSignalGenerator->reset();
    }
}

void OutputPatchTab::setProcessingStateChanged(bool isProcessing)
{
    if (patchMatrix)
    {
        patchMatrix->setProcessingStateChanged(isProcessing);
    }
}

void OutputPatchTab::resetMode()
{
    scrollingButton.setToggleState(true, juce::dontSendNotification);
    setMode(PatchMatrixComponent::Mode::Scrolling);
}

void OutputPatchTab::handleUnpatchAll()
{
    // Use weak reference to avoid dangling pointer if tab is destroyed during dialog
    juce::WeakReference<OutputPatchTab> weakThis(this);

    juce::AlertWindow::showOkCancelBox(
        juce::MessageBoxIconType::WarningIcon,
        "Unpatch All Outputs",
        "Are you sure you want to remove all output patches?",
        "Unpatch All",
        "Cancel",
        this,
        juce::ModalCallbackFunction::create([weakThis](int result) {
            if (weakThis != nullptr && result == 1)  // 1 = OK button clicked
            {
                if (weakThis->patchMatrix)
                    weakThis->patchMatrix->clearAllPatches();
            }
        }));
}
