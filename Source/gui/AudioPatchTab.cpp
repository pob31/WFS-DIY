#include "AudioPatchTab.h"
#include "ColorScheme.h"
#include <cmath>

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
        {
            setMode(PatchMatrixComponent::Mode::Scrolling);
            if (patchMatrix)
                patchMatrix->grabKeyboardFocus();
        }
    };

    addAndMakeVisible(patchingButton);
    patchingButton.setRadioGroupId(1);
    patchingButton.setClickingTogglesState(true);
    patchingButton.onClick = [this]() {
        if (patchingButton.getToggleState())
        {
            setMode(PatchMatrixComponent::Mode::Patching);
            if (patchMatrix)
                patchMatrix->grabKeyboardFocus();
        }
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

void InputPatchTab::grabPatchMatrixFocus()
{
    if (patchMatrix)
        patchMatrix->grabKeyboardFocus();
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
        {
            setMode(PatchMatrixComponent::Mode::Scrolling);
            if (patchMatrix)
                patchMatrix->grabKeyboardFocus();
        }
    };

    addAndMakeVisible(patchingButton);
    patchingButton.setRadioGroupId(2);
    patchingButton.setClickingTogglesState(true);
    patchingButton.onClick = [this]() {
        if (patchingButton.getToggleState())
        {
            setMode(PatchMatrixComponent::Mode::Patching);
            if (patchMatrix)
                patchMatrix->grabKeyboardFocus();
        }
    };

    addAndMakeVisible(testingButton);
    testingButton.setRadioGroupId(2);
    testingButton.setClickingTogglesState(true);
    testingButton.onClick = [this]() {
        if (testingButton.getToggleState())
        {
            setMode(PatchMatrixComponent::Mode::Testing);
            if (patchMatrix)
                patchMatrix->grabKeyboardFocus();
        }
    };

    // Unpatch All button
    addAndMakeVisible(unpatchAllButton);
    unpatchAllButton.onClick = [this]() { handleUnpatchAll(); };

    // Inline test signal controls (initially hidden)
    addChildComponent(signalTypeCombo);
    signalTypeCombo.addItem("Off", 1);
    signalTypeCombo.addItem("Pink Noise", 2);
    signalTypeCombo.addItem("Tone", 3);
    signalTypeCombo.addItem("Sweep", 4);
    signalTypeCombo.addItem("Pulse", 5);
    signalTypeCombo.setSelectedId(1, juce::dontSendNotification);
    signalTypeCombo.onChange = [this]() {
        bool isOff = signalTypeCombo.getSelectedId() == 1;

        // When selecting "Off", stop any playing test signal and clear highlighting
        if (isOff)
        {
            if (testSignalGenerator)
                testSignalGenerator->setOutputChannel(-1);
            if (patchMatrix)
                patchMatrix->clearActiveTestChannel();
            // Turn off hold when selecting Off
            holdButton.setToggleState(false, juce::dontSendNotification);
        }
        else
        {
            // Auto-boost level when selecting a signal type
            if (sliderValueToDb(levelSlider.getValue()) < -80.0f)
                levelSlider.setValue(dbToSliderValue(-40.0f));
        }

        updateTestControlsEnabledState();
        updateFrequencyVisibility();
        applyTestSettings();
    };

    addChildComponent(holdButton);
    holdButton.setClickingTogglesState(true);
    holdButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::green);
    holdButton.onClick = [this]() {
        applyTestSettings();
        // When hold is disabled, stop test signal and clear highlighting
        if (!holdButton.getToggleState() && patchMatrix)
            patchMatrix->clearActiveTestChannel();
    };

    addChildComponent(levelSlider);
    levelSlider.setTrackColours(juce::Colour(0xFF2D2D2D), juce::Colour(0xFF4CAF50));  // Green track
    levelSlider.setValue(dbToSliderValue(-40.0f));  // Default: -40 dB
    levelSlider.onValueChanged = [this](float v) {
        float dB = sliderValueToDb(v);
        levelValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
        applyTestSettings();
    };

    addChildComponent(levelValueLabel);
    levelValueLabel.setText("-40.0 dB", juce::dontSendNotification);
    levelValueLabel.setJustificationType(juce::Justification::centredLeft);
    levelValueLabel.setFont(juce::FontOptions(14.0f));

    // Frequency slider (for Tone mode only) - uses logarithmic mapping 20-20000 Hz
    addChildComponent(frequencySlider);
    frequencySlider.setValue(frequencyToSliderValue(1000.0f));  // Default: 1000 Hz
    frequencySlider.onValueChanged = [this](float v) {
        float freq = sliderValueToFrequency(v);
        if (freq >= 1000.0f)
            frequencyValueLabel.setText(juce::String(freq / 1000.0f, 1) + " kHz", juce::dontSendNotification);
        else
            frequencyValueLabel.setText(juce::String(static_cast<int>(freq)) + " Hz", juce::dontSendNotification);
        updateFrequencySliderColor();
        applyTestSettings();
        if (patchMatrix)
            patchMatrix->repaint();  // Update cell color based on frequency
    };

    addChildComponent(frequencyValueLabel);
    frequencyValueLabel.setText("1.0 kHz", juce::dontSendNotification);
    frequencyValueLabel.setJustificationType(juce::Justification::centredLeft);
    frequencyValueLabel.setFont(juce::FontOptions(14.0f));

    // Initialize frequency slider color
    updateFrequencySliderColor();

    // Status message label (hidden by default)
    addChildComponent(statusMessageLabel);
    statusMessageLabel.setJustificationType(juce::Justification::centred);
    statusMessageLabel.setFont(juce::FontOptions(13.0f));
    statusMessageLabel.setColour(juce::Label::textColourId, juce::Colours::orange);

    // Create patch matrix
    patchMatrix = std::make_unique<PatchMatrixComponent>(parameters, false, testSignalGen);
    addAndMakeVisible(patchMatrix.get());

    // Wire up callback for status bar messages
    patchMatrix->onStatusMessage = [this](const juce::String& msg) {
        showTemporaryMessage(msg);
        if (onStatusMessage)
            onStatusMessage(msg);
    };

    // Wire up callback to sync controls when test signal is auto-configured
    patchMatrix->onTestSignalConfigured = [this]()
    {
        if (testSignalGenerator)
        {
            // Sync signal type
            int comboId = 1;
            switch (testSignalGenerator->getSignalType())
            {
                case TestSignalGenerator::SignalType::Off:        comboId = 1; break;
                case TestSignalGenerator::SignalType::PinkNoise:  comboId = 2; break;
                case TestSignalGenerator::SignalType::Tone:       comboId = 3; break;
                case TestSignalGenerator::SignalType::Sweep:      comboId = 4; break;
                case TestSignalGenerator::SignalType::DiracPulse: comboId = 5; break;
            }
            signalTypeCombo.setSelectedId(comboId, juce::dontSendNotification);
            float freq = testSignalGenerator->getFrequency();
            frequencySlider.setValue(frequencyToSliderValue(freq));
            if (freq >= 1000.0f)
                frequencyValueLabel.setText(juce::String(freq / 1000.0f, 1) + " kHz", juce::dontSendNotification);
            else
                frequencyValueLabel.setText(juce::String(static_cast<int>(freq)) + " Hz", juce::dontSendNotification);
            updateFrequencySliderColor();
            float dB = testSignalGenerator->getLevelDb();
            levelSlider.setValue(dbToSliderValue(dB));
            levelValueLabel.setText(juce::String(dB, 1) + " dB", juce::dontSendNotification);
            updateTestControlsEnabledState();
            updateFrequencyVisibility();
        }
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

    // Inline test controls (between Testing button and Unpatch All)
    if (signalTypeCombo.isVisible())
    {
        buttonBar.removeFromLeft(buttonSpacing * 2);  // Extra spacing before controls
        signalTypeCombo.setBounds(buttonBar.removeFromLeft(100));

        buttonBar.removeFromLeft(buttonSpacing);
        holdButton.setBounds(buttonBar.removeFromLeft(60));
        buttonBar.removeFromLeft(buttonSpacing);
        levelSlider.setBounds(buttonBar.removeFromLeft(120));
        buttonBar.removeFromLeft(4);
        levelValueLabel.setBounds(buttonBar.removeFromLeft(65));

        // Frequency slider (only for Tone mode) - after level
        if (frequencySlider.isVisible())
        {
            buttonBar.removeFromLeft(buttonSpacing);
            frequencySlider.setBounds(buttonBar.removeFromLeft(120));
            buttonBar.removeFromLeft(4);
            frequencyValueLabel.setBounds(buttonBar.removeFromLeft(55));
        }

        // Status message in remaining space (right-aligned before Unpatch All)
        buttonBar.removeFromLeft(buttonSpacing);
        statusMessageLabel.setBounds(buttonBar);
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

    // Show/hide inline test controls
    updateTestControlsVisibility(mode == PatchMatrixComponent::Mode::Testing);

    // Stop test audio when leaving testing mode (but keep settings for quick re-testing)
    if (mode != PatchMatrixComponent::Mode::Testing && testSignalGenerator)
    {
        testSignalGenerator->setOutputChannel(-1);
        if (patchMatrix)
            patchMatrix->clearActiveTestChannel();
    }

    resized();  // Update layout
}

void OutputPatchTab::applyTestSettings()
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
    testSignalGenerator->setFrequency(sliderValueToFrequency(frequencySlider.getValue()));
    testSignalGenerator->setLevel(sliderValueToDb(levelSlider.getValue()));
    testSignalGenerator->setHoldEnabled(holdButton.getToggleState());
}

void OutputPatchTab::updateTestControlsVisibility(bool visible)
{
    signalTypeCombo.setVisible(visible);
    holdButton.setVisible(visible);
    levelSlider.setVisible(visible);
    levelValueLabel.setVisible(visible);

    if (visible)
        updateTestControlsEnabledState();
}

void OutputPatchTab::updateTestControlsEnabledState()
{
    // When signal is "Off" (ID 1), dim the Hold button and level slider
    bool signalActive = signalTypeCombo.getSelectedId() > 1;
    holdButton.setEnabled(signalActive);
    levelSlider.setEnabled(signalActive);

    // Dim the slider alpha when disabled
    if (!signalActive)
    {
        holdButton.setAlpha(0.5f);
        levelSlider.setAlpha(0.5f);
        levelValueLabel.setAlpha(0.5f);
    }
    else
    {
        holdButton.setAlpha(1.0f);
        levelSlider.setAlpha(1.0f);
        levelValueLabel.setAlpha(1.0f);
    }
}

void OutputPatchTab::updateFrequencyVisibility()
{
    // Only show frequency slider for Tone mode (ID 3)
    bool isToneMode = signalTypeCombo.getSelectedId() == 3;
    bool visible = isToneMode && signalTypeCombo.isVisible();
    frequencySlider.setVisible(visible);
    frequencyValueLabel.setVisible(visible);
    resized();  // Re-layout to accommodate slider
}

void OutputPatchTab::updateFrequencySliderColor()
{
    // Map frequency (20-20000 Hz) to hue (log scale)
    // Purple (0.8) for low frequencies, red (0) for high frequencies
    float freq = sliderValueToFrequency(frequencySlider.getValue());
    float logFreq = std::log10(juce::jlimit(20.0f, 20000.0f, freq));
    float minLog = std::log10(20.0f);
    float maxLog = std::log10(20000.0f);
    float t = (logFreq - minLog) / (maxLog - minLog);  // 0 = low freq, 1 = high freq
    float hue = 0.8f * (1.0f - t);  // Purple (0.8) at low, red (0) at high
    juce::Colour freqColor = juce::Colour::fromHSV(hue, 0.8f, 0.9f, 1.0f);
    frequencySlider.setTrackColours(juce::Colour(0xFF2D2D2D), freqColor);
}

float OutputPatchTab::sliderValueToDb(float sliderValue) const
{
    // Same logarithmic mapping as output attenuation: 0-1 slider to -92 to 0 dB
    // Formula: dB = 20 * log10(10^(-92/20) + ((1 - 10^(-92/20)) * v^2))
    float minLinear = std::pow(10.0f, -92.0f / 20.0f);
    float dB = 20.0f * std::log10(minLinear + ((1.0f - minLinear) * sliderValue * sliderValue));
    return juce::jlimit(-92.0f, 0.0f, dB);
}

float OutputPatchTab::dbToSliderValue(float dB) const
{
    // Inverse of sliderValueToDb
    dB = juce::jlimit(-92.0f, 0.0f, dB);
    float minLinear = std::pow(10.0f, -92.0f / 20.0f);
    float targetLinear = std::pow(10.0f, dB / 20.0f);
    float v = std::sqrt((targetLinear - minLinear) / (1.0f - minLinear));
    return juce::jlimit(0.0f, 1.0f, v);
}

float OutputPatchTab::sliderValueToFrequency(float sliderValue) const
{
    // Logarithmic mapping: 0-1 slider to 20-20000 Hz
    // Formula: freq = 20 * pow(10, 3*x)
    return 20.0f * std::pow(10.0f, 3.0f * sliderValue);
}

float OutputPatchTab::frequencyToSliderValue(float freq) const
{
    // Inverse: x = log10(freq/20) / 3
    freq = juce::jlimit(20.0f, 20000.0f, freq);
    return std::log10(freq / 20.0f) / 3.0f;
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

    // Reset test signal controls to default state
    signalTypeCombo.setSelectedItemIndex(0, juce::dontSendNotification);  // "Off"
    holdButton.setToggleState(false, juce::dontSendNotification);

    // Stop any active test signal
    if (testSignalGenerator)
    {
        testSignalGenerator->setSignalType(TestSignalGenerator::SignalType::Off);
        testSignalGenerator->setOutputChannel(-1);
    }

    // Clear test channel highlighting
    if (patchMatrix)
        patchMatrix->clearActiveTestChannel();

    // Update UI state
    updateTestControlsEnabledState();
    updateFrequencyVisibility();
}

void OutputPatchTab::grabPatchMatrixFocus()
{
    if (patchMatrix)
        patchMatrix->grabKeyboardFocus();
}

void OutputPatchTab::stopTestAudio()
{
    // Stop audio output but keep all settings (signal type, hold, level, frequency)
    if (testSignalGenerator)
        testSignalGenerator->setOutputChannel(-1);

    if (patchMatrix)
        patchMatrix->clearActiveTestChannel();
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

void OutputPatchTab::showTemporaryMessage(const juce::String& msg)
{
    statusMessageLabel.setText(msg, juce::dontSendNotification);
    statusMessageLabel.setVisible(true);
    startTimer(3000);  // Hide after 3 seconds
}

void OutputPatchTab::timerCallback()
{
    stopTimer();
    statusMessageLabel.setVisible(false);
}
