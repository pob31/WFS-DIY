#pragma once

#include <JuceHeader.h>
#include "../DSP/LevelMeteringManager.h"
#include "../DSP/WFSCalculationEngine.h"
#include "../Parameters/WFSValueTreeState.h"
#include "ColorScheme.h"
#include "WindowUtils.h"
#include "../Localization/LocalizationManager.h"

/**
 * LevelMeterBar
 * A vertical meter bar showing peak and RMS levels with peak hold.
 */
class LevelMeterBar : public juce::Component
{
public:
    LevelMeterBar()
    {
        peakHoldTime = juce::Time::currentTimeMillis();
    }

    void setLevel(float peakDb, float rmsDb)
    {
        currentPeakDb = peakDb;
        currentRmsDb = rmsDb;

        // Update peak hold
        if (peakDb > peakHoldDb || (juce::Time::currentTimeMillis() - peakHoldTime) > 1500)
        {
            peakHoldDb = peakDb;
            peakHoldTime = juce::Time::currentTimeMillis();
        }

        repaint();
    }

    void setSoloHighlight(bool highlighted)
    {
        isSoloHighlighted = highlighted;
        repaint();
    }

    /** Enable contribution mode - shows calculated level from soloed input */
    void setContributionMode(bool enabled)
    {
        isContributionMode = enabled;
        if (!enabled)
            contributionDb = -200.0f;
        repaint();
    }

    /** Set the contribution level (input level + attenuation) */
    void setContributionLevel(float db)
    {
        contributionDb = db;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().reduced(2);

        // Background - darker purple tint when in contribution mode
        if (isContributionMode)
            g.setColour(juce::Colour(0xFF1A1A2E));  // Dark purple-ish background
        else
            g.setColour(ColorScheme::get().background.darker(0.3f));
        g.fillRoundedRectangle(bounds.toFloat(), 3.0f);

        // Calculate meter height (0 dB at top, -60 dB at bottom)
        auto meterBounds = bounds.reduced(2);
        float meterHeight = static_cast<float>(meterBounds.getHeight());

        if (isContributionMode)
        {
            // Contribution mode: show calculated level with cyan/magenta color scheme
            float contribNormalized = juce::jlimit(0.0f, 1.0f, (contributionDb + 60.0f) / 60.0f);
            float contribHeight = contribNormalized * meterHeight;
            if (contribHeight > 1.0f)
            {
                auto contribRect = meterBounds.removeFromBottom(static_cast<int>(contribHeight));
                g.setColour(getContributionColor(contributionDb));
                g.fillRoundedRectangle(contribRect.toFloat(), 2.0f);
            }

            // Contribution level line at top of bar
            if (contribNormalized > 0.01f)
            {
                int contribY = bounds.getY() + 2 + static_cast<int>((1.0f - contribNormalized) * meterHeight);
                g.setColour(juce::Colours::white);
                g.fillRect(bounds.getX() + 2, contribY, bounds.getWidth() - 4, 2);
            }

            // Contribution mode border - cyan
            g.setColour(juce::Colour(0xFF00BFFF));  // Deep sky blue
            g.drawRoundedRectangle(getLocalBounds().toFloat(), 3.0f, 2.0f);
        }
        else
        {
            // Normal mode: RMS level (wider bar)
            float rmsNormalized = juce::jlimit(0.0f, 1.0f, (currentRmsDb + 60.0f) / 60.0f);
            float rmsHeight = rmsNormalized * meterHeight;
            if (rmsHeight > 1.0f)
            {
                auto rmsRect = meterBounds.removeFromBottom(static_cast<int>(rmsHeight));
                g.setColour(getLevelColor(currentRmsDb).withAlpha(0.7f));
                g.fillRoundedRectangle(rmsRect.toFloat(), 2.0f);
            }

            // Peak level (thin line)
            float peakNormalized = juce::jlimit(0.0f, 1.0f, (currentPeakDb + 60.0f) / 60.0f);
            int peakY = meterBounds.getY() + static_cast<int>((1.0f - peakNormalized) * meterHeight);
            if (peakNormalized > 0.01f)
            {
                g.setColour(getLevelColor(currentPeakDb));
                g.fillRect(bounds.getX() + 2, peakY, bounds.getWidth() - 4, 3);
            }

            // Peak hold line
            float holdNormalized = juce::jlimit(0.0f, 1.0f, (peakHoldDb + 60.0f) / 60.0f);
            int holdY = bounds.getY() + 2 + static_cast<int>((1.0f - holdNormalized) * meterHeight);
            if (holdNormalized > 0.01f)
            {
                g.setColour(juce::Colours::white);
                g.fillRect(bounds.getX() + 2, holdY, bounds.getWidth() - 4, 2);
            }

            // Solo highlight border
            if (isSoloHighlighted)
            {
                g.setColour(juce::Colours::yellow);
                g.drawRoundedRectangle(getLocalBounds().toFloat(), 3.0f, 2.0f);
            }

            // Clip indicator
            if (currentPeakDb > -0.5f)
            {
                g.setColour(juce::Colours::red);
                g.fillRoundedRectangle(bounds.toFloat().removeFromTop(6), 2.0f);
            }
        }
    }

private:
    static juce::Colour getLevelColor(float db)
    {
        if (db < -12.0f)
            return juce::Colours::green;
        else if (db < -6.0f)
            return juce::Colours::yellow;
        else
            return juce::Colours::red;
    }

    /** Contribution mode color: cyan → blue → magenta gradient */
    static juce::Colour getContributionColor(float db)
    {
        // Map -60 to 0 dB to a cyan→magenta gradient
        float normalized = juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);

        // Cyan (0xFF00FFFF) at low levels → Magenta (0xFFFF00FF) at high levels
        // Through blue (0xFF0080FF) in the middle
        if (normalized < 0.5f)
        {
            // Cyan to Blue: increase red channel slightly, keep full blue
            float t = normalized * 2.0f;
            return juce::Colour::fromFloatRGBA(t * 0.5f, 1.0f - t * 0.5f, 1.0f, 0.85f);
        }
        else
        {
            // Blue to Magenta: increase red, decrease green
            float t = (normalized - 0.5f) * 2.0f;
            return juce::Colour::fromFloatRGBA(0.5f + t * 0.5f, 0.5f - t * 0.5f, 1.0f, 0.85f);
        }
    }

    float currentPeakDb = -200.0f;
    float currentRmsDb = -200.0f;
    float peakHoldDb = -200.0f;
    int64_t peakHoldTime = 0;
    bool isSoloHighlighted = false;

    // Contribution mode state
    bool isContributionMode = false;
    float contributionDb = -200.0f;
};

/**
 * ThreadPerformanceBar
 * A small horizontal bar showing CPU usage percentage.
 */
class ThreadPerformanceBar : public juce::Component,
                             public juce::SettableTooltipClient
{
public:
    void setPerformance(float cpuPercent, float microseconds)
    {
        currentCpuPercent = cpuPercent;
        currentMicroseconds = microseconds;
        setTooltip(juce::String::formatted("%.1f%% | %.0f us", cpuPercent, microseconds));
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().reduced(1);

        // Background
        g.setColour(ColorScheme::get().background.darker(0.3f));
        g.fillRoundedRectangle(bounds.toFloat(), 2.0f);

        // CPU bar
        float normalized = juce::jlimit(0.0f, 1.0f, currentCpuPercent / 100.0f);
        int barWidth = static_cast<int>(normalized * bounds.getWidth());
        if (barWidth > 0)
        {
            auto barRect = bounds.removeFromLeft(barWidth);
            g.setColour(getCpuColor(currentCpuPercent));
            g.fillRoundedRectangle(barRect.toFloat(), 2.0f);
        }
    }

private:
    static juce::Colour getCpuColor(float percent)
    {
        if (percent < 50.0f)
            return juce::Colours::green;
        else if (percent < 80.0f)
            return juce::Colours::yellow;
        else
            return juce::Colours::red;
    }

    float currentCpuPercent = 0.0f;
    float currentMicroseconds = 0.0f;
};

/**
 * LevelMeterWindowContent
 * Main content showing input/output meters with thread performance.
 */
class LevelMeterWindowContent : public juce::Component,
                                 private juce::Timer
{
public:
    LevelMeterWindowContent(LevelMeteringManager& manager, WFSValueTreeState& vts,
                            WFSCalculationEngine* calcEngine = nullptr)
        : levelManager(manager), valueTreeState(vts), calculationEngine(calcEngine)
    {
        // Input section label
        addAndMakeVisible(inputsLabel);
        inputsLabel.setText(LOC("levelMeter.inputs"), juce::dontSendNotification);
        inputsLabel.setFont(juce::FontOptions().withHeight(14.0f).withStyle("Bold"));

        // Output section label
        addAndMakeVisible(outputsLabel);
        outputsLabel.setText(LOC("levelMeter.outputs"), juce::dontSendNotification);
        outputsLabel.setFont(juce::FontOptions().withHeight(14.0f).withStyle("Bold"));

        // Create meters
        rebuildMeters();

        // Clear Solo button (for binaural solo)
        addAndMakeVisible(clearSoloButton);
        clearSoloButton.setButtonText(LOC("levelMeter.buttons.clearSolo"));
        clearSoloButton.setTooltip(LOC("levelMeter.tooltips.clearSolo"));
        clearSoloButton.onClick = [this]() {
            valueTreeState.clearAllSoloStates();
            updateSoloButtonStates();
        };

        // Solo mode toggle button (Single/Multi)
        addAndMakeVisible(soloModeButton);
        updateSoloModeButtonText();
        soloModeButton.setTooltip(LOC("levelMeter.tooltips.soloMode"));
        soloModeButton.onClick = [this]() {
            toggleSoloMode();
        };

        // Initialize button states
        updateSoloButtonStates();
        updateSoloButtonColors();

        startTimerHz(20);  // 20 Hz update rate
    }

    ~LevelMeterWindowContent() override
    {
        stopTimer();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(ColorScheme::get().background);

        // Draw section separators
        auto bounds = getLocalBounds();
        int sectionY = bounds.getHeight() / 2;

        g.setColour(ColorScheme::get().textSecondary.withAlpha(0.3f));
        g.drawLine(10.0f, static_cast<float>(sectionY),
                   static_cast<float>(bounds.getWidth() - 10), static_cast<float>(sectionY), 1.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);
        int controlHeight = 30;

        // Bottom controls (clear solo button, solo mode toggle)
        auto controlsArea = bounds.removeFromBottom(controlHeight);
        clearSoloButton.setBounds(controlsArea.removeFromLeft(100));
        controlsArea.removeFromLeft(10);  // Spacing
        soloModeButton.setBounds(controlsArea.removeFromLeft(100));

        bounds.removeFromBottom(10);  // Spacing

        // Split remaining area for inputs and outputs
        int halfHeight = bounds.getHeight() / 2;
        auto inputsArea = bounds.removeFromTop(halfHeight);
        bounds.removeFromTop(5);  // Spacing
        auto outputsArea = bounds;

        // Input section
        inputsLabel.setBounds(inputsArea.removeFromTop(20));
        layoutInputMeters(inputsArea,
                         levelManager.getCurrentAlgorithm() == LevelMeteringManager::ProcessingAlgorithm::InputBuffer);

        // Output section
        outputsLabel.setBounds(outputsArea.removeFromTop(20));
        layoutMeters(outputsArea, outputMeters, outputLabels, outputPerfBars,
                    levelManager.getCurrentAlgorithm() == LevelMeteringManager::ProcessingAlgorithm::OutputBuffer);
    }

    void rebuildMeters()
    {
        inputMeters.clear();
        inputLabels.clear();
        inputPerfBars.clear();
        inputSoloButtons.clear();
        outputMeters.clear();
        outputLabels.clear();
        outputPerfBars.clear();

        int numInputs = levelManager.getNumInputChannels();
        int numOutputs = levelManager.getNumOutputChannels();

        for (int i = 0; i < numInputs; ++i)
        {
            auto* meter = inputMeters.add(new LevelMeterBar());
            addAndMakeVisible(meter);

            auto* label = inputLabels.add(new juce::Label());
            label->setText(juce::String(i + 1), juce::dontSendNotification);
            label->setJustificationType(juce::Justification::centred);
            label->setFont(juce::FontOptions().withHeight(10.0f));
            addAndMakeVisible(label);

            auto* perfBar = inputPerfBars.add(new ThreadPerformanceBar());
            addAndMakeVisible(perfBar);

            auto* soloBtn = inputSoloButtons.add(new juce::TextButton("S"));
            soloBtn->setClickingTogglesState(true);
            soloBtn->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFFFFD700));  // Yellow background when on
            soloBtn->setColour(juce::TextButton::textColourOnId, juce::Colours::black);        // Black text when on
            soloBtn->setTooltip(LOC("levelMeter.tooltips.solo"));
            soloBtn->onClick = [this, i]() {
                bool newState = inputSoloButtons[i]->getToggleState();
                valueTreeState.setInputSoloed(i, newState);

                // In Single mode, also update Visual Solo (for map highlighting)
                if (valueTreeState.getBinauralSoloMode() == 0 && newState)
                {
                    levelManager.setVisualSoloInput(i);
                }
                else if (!newState && levelManager.getVisualSoloInput() == i)
                {
                    // Clearing solo also clears visual solo if it was this input
                    levelManager.setVisualSoloInput(-1);
                }

                updateSoloButtonStates();
            };
            addAndMakeVisible(soloBtn);
        }

        for (int i = 0; i < numOutputs; ++i)
        {
            auto* meter = outputMeters.add(new LevelMeterBar());
            addAndMakeVisible(meter);

            auto* label = outputLabels.add(new juce::Label());
            label->setText(juce::String(i + 1), juce::dontSendNotification);
            label->setJustificationType(juce::Justification::centred);
            label->setFont(juce::FontOptions().withHeight(10.0f));
            addAndMakeVisible(label);

            auto* perfBar = outputPerfBars.add(new ThreadPerformanceBar());
            addAndMakeVisible(perfBar);
        }

        resized();
    }

private:
    void timerCallback() override
    {
        // Update input meters
        for (int i = 0; i < inputMeters.size(); ++i)
        {
            auto level = levelManager.getInputLevel(i);
            inputMeters[i]->setLevel(level.peakDb, level.rmsDb);
        }

        // Update output meters
        // Solo highlighting and contribution mode
        int soloInput = levelManager.getVisualSoloInput();
        bool isSingleMode = (valueTreeState.getBinauralSoloMode() == 0);
        int numSoloed = valueTreeState.getNumSoloedInputs();

        // Contribution mode: single mode with solo OR exactly one input soloed
        bool showContribution = calculationEngine != nullptr &&
                                ((isSingleMode && soloInput >= 0) || numSoloed == 1);

        // If exactly one soloed but not in single mode, find which one
        int contributionInput = soloInput;
        if (showContribution && contributionInput < 0 && numSoloed == 1)
        {
            for (int ch = 0; ch < valueTreeState.getNumInputChannels(); ++ch)
            {
                if (valueTreeState.isInputSoloed(ch))
                {
                    contributionInput = ch;
                    break;
                }
            }
        }

        for (int i = 0; i < outputMeters.size(); ++i)
        {
            auto level = levelManager.getOutputLevel(i);
            outputMeters[i]->setLevel(level.peakDb, level.rmsDb);

            if (showContribution && contributionInput >= 0)
            {
                // Get routing level from calculation engine
                float routingLevel = calculationEngine->getLevel(contributionInput, i);

                // Calculate contribution: input level + routing attenuation
                float contributionDb = levelManager.getInputContributionToOutput(
                    contributionInput, i, routingLevel);

                outputMeters[i]->setContributionMode(true);
                outputMeters[i]->setContributionLevel(contributionDb);
                outputMeters[i]->setSoloHighlight(false);  // Don't show yellow border in contribution mode
            }
            else
            {
                outputMeters[i]->setContributionMode(false);

                // Solo highlighting - only in Single mode (when not in contribution mode)
                bool highlight = false;
                if (isSingleMode && soloInput >= 0)
                {
                    highlight = level.peakDb > -60.0f;
                }
                outputMeters[i]->setSoloHighlight(highlight);
            }
        }

        // Update thread performance bars
        bool isInputBuffer = (levelManager.getCurrentAlgorithm() ==
                              LevelMeteringManager::ProcessingAlgorithm::InputBuffer);

        if (isInputBuffer)
        {
            for (int i = 0; i < inputPerfBars.size(); ++i)
            {
                auto perf = levelManager.getThreadPerformance(i);
                inputPerfBars[i]->setPerformance(perf.cpuPercent, perf.microsecondsPerBlock);
                inputPerfBars[i]->setVisible(true);
            }
            for (auto* bar : outputPerfBars)
                bar->setVisible(false);
        }
        else
        {
            for (int i = 0; i < outputPerfBars.size(); ++i)
            {
                auto perf = levelManager.getThreadPerformance(i);
                outputPerfBars[i]->setPerformance(perf.cpuPercent, perf.microsecondsPerBlock);
                outputPerfBars[i]->setVisible(true);
            }
            for (auto* bar : inputPerfBars)
                bar->setVisible(false);
        }

        // Update solo button states and colors
        updateSoloButtonStates();
        updateSoloButtonColors();
        updateSoloModeButtonText();  // Keep in sync with changes from other tabs
    }

    void layoutInputMeters(juce::Rectangle<int>& area, bool showPerfBars)
    {
        if (inputMeters.isEmpty())
            return;

        int numMeters = inputMeters.size();
        int meterWidth = juce::jmin(30, (area.getWidth() - 20) / numMeters);
        int spacing = juce::jmax(2, (area.getWidth() - numMeters * meterWidth) / (numMeters + 1));

        int x = area.getX() + spacing;
        int baseY = area.getY();
        int labelHeight = 15;
        int soloButtonHeight = 18;
        int perfBarHeight = showPerfBars ? 10 : 0;
        int meterHeight = area.getHeight() - labelHeight - soloButtonHeight - perfBarHeight - 8;

        for (int i = 0; i < numMeters; ++i)
        {
            int y = baseY;
            inputMeters[i]->setBounds(x, y, meterWidth, meterHeight);
            y += meterHeight + 2;
            inputLabels[i]->setBounds(x, y, meterWidth, labelHeight);
            y += labelHeight + 2;
            inputSoloButtons[i]->setBounds(x, y, meterWidth, soloButtonHeight);
            y += soloButtonHeight + 2;

            if (showPerfBars && i < inputPerfBars.size())
            {
                inputPerfBars[i]->setBounds(x, y, meterWidth, perfBarHeight);
            }

            x += meterWidth + spacing;
        }
    }

    void layoutMeters(juce::Rectangle<int>& area,
                      juce::OwnedArray<LevelMeterBar>& meters,
                      juce::OwnedArray<juce::Label>& labels,
                      juce::OwnedArray<ThreadPerformanceBar>& perfBars,
                      bool showPerfBars)
    {
        if (meters.isEmpty())
            return;

        int numMeters = meters.size();
        int meterWidth = juce::jmin(30, (area.getWidth() - 20) / numMeters);
        int spacing = juce::jmax(2, (area.getWidth() - numMeters * meterWidth) / (numMeters + 1));

        int x = area.getX() + spacing;
        int baseY = area.getY();
        int labelHeight = 15;
        int perfBarHeight = showPerfBars ? 10 : 0;
        int meterHeight = area.getHeight() - labelHeight - perfBarHeight - 5;

        for (int i = 0; i < numMeters; ++i)
        {
            meters[i]->setBounds(x, baseY, meterWidth, meterHeight);
            labels[i]->setBounds(x, baseY + meterHeight + 2, meterWidth, labelHeight);

            if (showPerfBars && i < perfBars.size())
            {
                perfBars[i]->setBounds(x, baseY + meterHeight + labelHeight + 3, meterWidth, perfBarHeight);
            }

            x += meterWidth + spacing;
        }
    }

    void updateSoloButtonStates()
    {
        bool anySoloed = false;
        for (int i = 0; i < inputSoloButtons.size(); ++i)
        {
            bool isSoloed = valueTreeState.isInputSoloed(i);
            inputSoloButtons[i]->setToggleState(isSoloed, juce::dontSendNotification);
            if (isSoloed) anySoloed = true;
        }

        // Dim Clear Solo button when no solos are engaged
        auto disabledColour = ColorScheme::get().textDisabled;
        auto enabledColour = ColorScheme::get().textPrimary;
        clearSoloButton.setColour(juce::TextButton::textColourOffId, anySoloed ? enabledColour : disabledColour);
        clearSoloButton.setColour(juce::TextButton::textColourOnId, anySoloed ? enabledColour : disabledColour);
    }

    void updateSoloButtonColors()
    {
        // Yellow in Single mode, Orange in Multi mode
        bool isMultiMode = (valueTreeState.getBinauralSoloMode() == 1);
        juce::Colour buttonOnColour = isMultiMode ? juce::Colour(0xFFFF8C00) : juce::Colour(0xFFFFD700);  // Orange vs Yellow

        for (auto* btn : inputSoloButtons)
        {
            btn->setColour(juce::TextButton::buttonOnColourId, buttonOnColour);
            btn->setColour(juce::TextButton::textColourOnId, juce::Colours::black);
        }
    }

    void toggleSoloMode()
    {
        int currentMode = valueTreeState.getBinauralSoloMode();
        int newMode = (currentMode == 0) ? 1 : 0;  // Toggle between Single (0) and Multi (1)
        valueTreeState.setBinauralSoloMode(newMode);
        updateSoloModeButtonText();
        updateSoloButtonColors();
    }

    void updateSoloModeButtonText()
    {
        int mode = valueTreeState.getBinauralSoloMode();
        if (mode == 0)
            soloModeButton.setButtonText(LOC("levelMeter.buttons.soloModeSingle"));
        else
            soloModeButton.setButtonText(LOC("levelMeter.buttons.soloModeMulti"));
    }

    LevelMeteringManager& levelManager;
    WFSValueTreeState& valueTreeState;
    WFSCalculationEngine* calculationEngine = nullptr;

    juce::Label inputsLabel;
    juce::Label outputsLabel;

    juce::OwnedArray<LevelMeterBar> inputMeters;
    juce::OwnedArray<juce::Label> inputLabels;
    juce::OwnedArray<ThreadPerformanceBar> inputPerfBars;
    juce::OwnedArray<juce::TextButton> inputSoloButtons;

    juce::OwnedArray<LevelMeterBar> outputMeters;
    juce::OwnedArray<juce::Label> outputLabels;
    juce::OwnedArray<ThreadPerformanceBar> outputPerfBars;

    juce::TextButton clearSoloButton;
    juce::TextButton soloModeButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeterWindowContent)
};

/**
 * LevelMeterWindow
 * Floating window for monitoring audio levels and thread performance.
 */
class LevelMeterWindow : public juce::DocumentWindow,
                         public ColorScheme::Manager::Listener
{
public:
    LevelMeterWindow(LevelMeteringManager& manager, WFSValueTreeState& vts,
                     WFSCalculationEngine* calcEngine = nullptr)
        : DocumentWindow(LOC("levelMeter.windowTitle"),
                         ColorScheme::get().background,
                         DocumentWindow::allButtons),
          levelManager(manager), valueTreeState(vts), calculationEngine(calcEngine)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);

        content = std::make_unique<LevelMeterWindowContent>(manager, vts, calcEngine);
        content->setName(LOC("levelMeter.windowTitle"));
        setContentOwned(content.get(), false);

        // Window size
        const int preferredWidth = 800;
        const int preferredHeight = 500;

        // Get display bounds
        auto& displays = juce::Desktop::getInstance().getDisplays();
        const auto* displayPtr = displays.getPrimaryDisplay();
        juce::Rectangle<int> userArea = (displayPtr != nullptr && !displayPtr->userArea.isEmpty())
            ? displayPtr->userArea
            : displays.getTotalBounds(true);

        const int margin = 40;
        const int windowWidth = juce::jmin(preferredWidth, userArea.getWidth() - margin);
        const int windowHeight = juce::jmin(preferredHeight, userArea.getHeight() - margin);

        setResizeLimits(400, 300, userArea.getWidth(), userArea.getHeight());

        centreWithSize(windowWidth, windowHeight);
        setVisible(true);
        WindowUtils::enableDarkTitleBar(this);

        ColorScheme::Manager::getInstance().addListener(this);

        // Enable metering when window opens
        levelManager.setMeterWindowEnabled(true);
    }

    ~LevelMeterWindow() override
    {
        ColorScheme::Manager::getInstance().removeListener(this);
    }

    void closeButtonPressed() override
    {
        levelManager.setMeterWindowEnabled(false);
        setVisible(false);
    }

    void colorSchemeChanged() override
    {
        setBackgroundColour(ColorScheme::get().background);
        repaint();
    }

    void rebuildMeters()
    {
        if (content)
            content->rebuildMeters();
    }

private:
    LevelMeteringManager& levelManager;
    WFSValueTreeState& valueTreeState;
    WFSCalculationEngine* calculationEngine = nullptr;
    std::unique_ptr<LevelMeterWindowContent> content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeterWindow)
};
