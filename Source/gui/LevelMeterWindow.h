#pragma once

#include <JuceHeader.h>
#include "../DSP/LevelMeteringManager.h"
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

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().reduced(2);

        // Background
        g.setColour(ColorScheme::get().background.darker(0.3f));
        g.fillRoundedRectangle(bounds.toFloat(), 3.0f);

        // Calculate meter height (0 dB at top, -60 dB at bottom)
        auto meterBounds = bounds.reduced(2);
        float meterHeight = static_cast<float>(meterBounds.getHeight());

        // RMS level (wider bar)
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

    float currentPeakDb = -200.0f;
    float currentRmsDb = -200.0f;
    float peakHoldDb = -200.0f;
    int64_t peakHoldTime = 0;
    bool isSoloHighlighted = false;
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
    LevelMeterWindowContent(LevelMeteringManager& manager, WFSValueTreeState& vts)
        : levelManager(manager), valueTreeState(vts)
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

        // Visual Solo selector (for map highlighting)
        addAndMakeVisible(soloLabel);
        soloLabel.setText(LOC("levelMeter.solo"), juce::dontSendNotification);

        addAndMakeVisible(soloSelector);
        // Note: soloSelector items are populated in rebuildMeters()
        soloSelector.onChange = [this]() {
            int selected = soloSelector.getSelectedId();
            levelManager.setVisualSoloInput(selected > 1 ? selected - 2 : -1);
        };

        // Clear Solo button (for binaural solo)
        addAndMakeVisible(clearSoloButton);
        clearSoloButton.setButtonText(LOC("levelMeter.buttons.clearSolo"));
        clearSoloButton.onClick = [this]() {
            valueTreeState.clearAllSoloStates();
            updateSoloButtonStates();
        };

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

        // Bottom controls (solo selector and clear solo button)
        auto controlsArea = bounds.removeFromBottom(controlHeight);
        soloLabel.setBounds(controlsArea.removeFromLeft(80));
        soloSelector.setBounds(controlsArea.removeFromLeft(150));
        controlsArea.removeFromLeft(20);  // Spacing
        clearSoloButton.setBounds(controlsArea.removeFromLeft(100));

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
            soloBtn->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFFFFD700));  // Yellow when on
            soloBtn->onClick = [this, i]() {
                bool newState = inputSoloButtons[i]->getToggleState();
                valueTreeState.setInputSoloed(i, newState);

                // In Single mode, also update Visual Solo
                if (valueTreeState.getBinauralSoloMode() == 0 && newState)
                {
                    levelManager.setVisualSoloInput(i);
                    soloSelector.setSelectedId(i + 2, juce::dontSendNotification);
                }
                else if (!newState && levelManager.getVisualSoloInput() == i)
                {
                    // Clearing solo also clears visual solo if it was this input
                    levelManager.setVisualSoloInput(-1);
                    soloSelector.setSelectedId(1, juce::dontSendNotification);
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

        // Rebuild solo selector
        soloSelector.clear();
        soloSelector.addItem(LOC("levelMeter.soloNone"), 1);
        for (int i = 0; i < numInputs; ++i)
            soloSelector.addItem("Input " + juce::String(i + 1), i + 2);
        soloSelector.setSelectedId(1);

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
        // Solo highlighting only in Single mode
        int soloInput = levelManager.getVisualSoloInput();
        bool isSingleMode = (valueTreeState.getBinauralSoloMode() == 0);
        for (int i = 0; i < outputMeters.size(); ++i)
        {
            auto level = levelManager.getOutputLevel(i);
            outputMeters[i]->setLevel(level.peakDb, level.rmsDb);

            // Solo highlighting - only in Single mode
            bool highlight = false;
            if (isSingleMode && soloInput >= 0)
            {
                highlight = level.peakDb > -60.0f;
            }
            outputMeters[i]->setSoloHighlight(highlight);
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
    }

    void layoutInputMeters(juce::Rectangle<int>& area, bool showPerfBars)
    {
        if (inputMeters.isEmpty())
            return;

        int numMeters = inputMeters.size();
        int meterWidth = juce::jmin(30, (area.getWidth() - 20) / numMeters);
        int spacing = juce::jmax(2, (area.getWidth() - numMeters * meterWidth) / (numMeters + 1));

        int x = spacing;
        int labelHeight = 15;
        int soloButtonHeight = 18;
        int perfBarHeight = showPerfBars ? 10 : 0;
        int meterHeight = area.getHeight() - labelHeight - soloButtonHeight - perfBarHeight - 8;

        for (int i = 0; i < numMeters; ++i)
        {
            int y = 0;
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

        int x = spacing;
        int labelHeight = 15;
        int perfBarHeight = showPerfBars ? 10 : 0;
        int meterHeight = area.getHeight() - labelHeight - perfBarHeight - 5;

        for (int i = 0; i < numMeters; ++i)
        {
            meters[i]->setBounds(x, 0, meterWidth, meterHeight);
            labels[i]->setBounds(x, meterHeight + 2, meterWidth, labelHeight);

            if (showPerfBars && i < perfBars.size())
            {
                perfBars[i]->setBounds(x, meterHeight + labelHeight + 3, meterWidth, perfBarHeight);
            }

            x += meterWidth + spacing;
        }
    }

    void updateSoloButtonStates()
    {
        for (int i = 0; i < inputSoloButtons.size(); ++i)
        {
            bool isSoloed = valueTreeState.isInputSoloed(i);
            inputSoloButtons[i]->setToggleState(isSoloed, juce::dontSendNotification);
        }
    }

    LevelMeteringManager& levelManager;
    WFSValueTreeState& valueTreeState;

    juce::Label inputsLabel;
    juce::Label outputsLabel;
    juce::Label soloLabel;
    juce::ComboBox soloSelector;

    juce::OwnedArray<LevelMeterBar> inputMeters;
    juce::OwnedArray<juce::Label> inputLabels;
    juce::OwnedArray<ThreadPerformanceBar> inputPerfBars;
    juce::OwnedArray<juce::TextButton> inputSoloButtons;

    juce::OwnedArray<LevelMeterBar> outputMeters;
    juce::OwnedArray<juce::Label> outputLabels;
    juce::OwnedArray<ThreadPerformanceBar> outputPerfBars;

    juce::TextButton clearSoloButton;

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
    LevelMeterWindow(LevelMeteringManager& manager, WFSValueTreeState& vts)
        : DocumentWindow(LOC("levelMeter.windowTitle"),
                         ColorScheme::get().background,
                         DocumentWindow::allButtons),
          levelManager(manager), valueTreeState(vts)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);

        content = std::make_unique<LevelMeterWindowContent>(manager, vts);
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
    std::unique_ptr<LevelMeterWindowContent> content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeterWindow)
};
