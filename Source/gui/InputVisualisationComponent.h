#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
 * A read-only vertical slider for displaying DSP values.
 * Shows a filled bar from bottom (or center for bidirectional) with value at top.
 */
class VisualisationSlider : public juce::Component,
                             public juce::TooltipClient
{
public:
    VisualisationSlider()
    {
        setInterceptsMouseClicks(true, false);
    }

    void setRange(float min, float max, bool centerZero = false)
    {
        minValue = min;
        maxValue = max;
        isCenterZero = centerZero;
        repaint();
    }

    void setValue(float newValue)
    {
        value = juce::jlimit(minValue, maxValue, newValue);
        repaint();
    }

    float getValue() const { return value; }

    void setColour(juce::Colour c)
    {
        fillColour = c;
        repaint();
    }

    void setOutputName(const juce::String& name) { outputName = name; }
    void setValueUnit(const juce::String& unit) { valueUnit = unit; }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Background
        g.setColour(juce::Colour(0xFF1A1A1A));
        g.fillRect(bounds);

        // Calculate fill height
        float sliderArea = bounds.reduced(2.0f).getHeight() - 18.0f;  // Leave space for value text
        float fillHeight = 0.0f;

        if (isCenterZero)
        {
            // Bidirectional: fill from center
            float centerY = bounds.getCentreY();
            float normalizedValue = (value - minValue) / (maxValue - minValue) - 0.5f;
            fillHeight = std::abs(normalizedValue) * sliderArea;

            auto fillBounds = bounds.reduced(2.0f);
            fillBounds.removeFromTop(18.0f);  // Value text area

            g.setColour(fillColour);
            if (value >= 0)
                g.fillRect(fillBounds.getX(), centerY - fillHeight, fillBounds.getWidth(), fillHeight);
            else
                g.fillRect(fillBounds.getX(), centerY, fillBounds.getWidth(), fillHeight);
        }
        else
        {
            // Standard: fill from bottom
            float normalizedValue = (value - minValue) / (maxValue - minValue);
            fillHeight = normalizedValue * sliderArea;

            auto fillBounds = bounds.reduced(2.0f);
            fillBounds.removeFromTop(18.0f);  // Value text area

            g.setColour(fillColour);
            g.fillRect(fillBounds.getX(), fillBounds.getBottom() - fillHeight,
                       fillBounds.getWidth(), fillHeight);
        }

        // Value text at top
        g.setColour(juce::Colours::white);
        g.setFont(10.0f);
        juce::String valueText = juce::String(static_cast<int>(std::round(value)));
        g.drawText(valueText, bounds.removeFromTop(18.0f), juce::Justification::centred, false);
    }

    juce::String getTooltip() override
    {
        return outputName + ": " + juce::String(value, 1) + " " + valueUnit;
    }

private:
    float value = 0.0f;
    float minValue = 0.0f;
    float maxValue = 100.0f;
    bool isCenterZero = false;
    juce::Colour fillColour { 0xFF4A90D9 };
    juce::String outputName;
    juce::String valueUnit;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VisualisationSlider)
};

//==============================================================================
/**
 * Input Visualisation Component
 *
 * Displays DSP matrix values for the currently selected input channel:
 * - Row 1: Delay times (0-350ms) - Yellow
 * - Row 2: HF attenuation (-24 to 0 dB) - Pink
 * - Row 3: Level attenuation (-60 to 0 dB) - Blue
 *
 * Shows one slider per output channel + reverb feed.
 * Read-only display that updates from WFSCalculationEngine.
 */
class InputVisualisationComponent : public juce::Component,
                                     private juce::Timer
{
public:
    InputVisualisationComponent()
    {
        // Create tooltip window for hover tooltips on sliders
        tooltipWindow = std::make_unique<juce::TooltipWindow>(this, 300);

        // Row labels
        addAndMakeVisible(delayLabel);
        delayLabel.setText("delay", juce::dontSendNotification);
        delayLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFD4A017));  // Yellow
        delayLabel.setJustificationType(juce::Justification::centredRight);

        addAndMakeVisible(delayUnitLabel);
        delayUnitLabel.setText("ms", juce::dontSendNotification);
        delayUnitLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFD4A017));
        delayUnitLabel.setJustificationType(juce::Justification::centredRight);

        addAndMakeVisible(hfLabel);
        hfLabel.setText("HF\ndamping", juce::dontSendNotification);
        hfLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFE07878));  // Pink/coral
        hfLabel.setJustificationType(juce::Justification::centredRight);

        addAndMakeVisible(hfUnitLabel);
        hfUnitLabel.setText("dB", juce::dontSendNotification);
        hfUnitLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFE07878));
        hfUnitLabel.setJustificationType(juce::Justification::centredRight);

        addAndMakeVisible(levelLabel);
        levelLabel.setText("level", juce::dontSendNotification);
        levelLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF4A90D9));  // Blue
        levelLabel.setJustificationType(juce::Justification::centredRight);

        addAndMakeVisible(levelUnitLabel);
        levelUnitLabel.setText("dB", juce::dontSendNotification);
        levelUnitLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF4A90D9));
        levelUnitLabel.setJustificationType(juce::Justification::centredRight);
    }

    ~InputVisualisationComponent() override
    {
        stopTimer();
    }

    /**
     * Configure the component with output and reverb count.
     * Creates sliders for each output + reverb feed.
     */
    void configure(int outputCount, int reverbCount)
    {
        numOutputs = outputCount;
        numReverbs = reverbCount;
        int totalSliders = outputCount + reverbCount;

        // Create delay sliders
        delaySliders.clear();
        for (int i = 0; i < totalSliders; ++i)
        {
            auto* slider = delaySliders.add(new VisualisationSlider());
            slider->setRange(0.0f, 350.0f);
            slider->setColour(juce::Colour(0xFFD4A017));  // Yellow
            slider->setValueUnit("ms");
            if (i < numOutputs)
                slider->setOutputName("Output " + juce::String(i + 1));
            else
                slider->setOutputName("Reverb " + juce::String(i - numOutputs + 1));
            addAndMakeVisible(slider);
        }

        // Create HF sliders
        hfSliders.clear();
        for (int i = 0; i < totalSliders; ++i)
        {
            auto* slider = hfSliders.add(new VisualisationSlider());
            slider->setRange(-24.0f, 0.0f);
            slider->setColour(juce::Colour(0xFFE07878));  // Pink/coral
            slider->setValueUnit("dB");
            if (i < numOutputs)
                slider->setOutputName("Output " + juce::String(i + 1));
            else
                slider->setOutputName("Reverb " + juce::String(i - numOutputs + 1));
            addAndMakeVisible(slider);
        }

        // Create level sliders
        levelSliders.clear();
        for (int i = 0; i < totalSliders; ++i)
        {
            auto* slider = levelSliders.add(new VisualisationSlider());
            slider->setRange(-60.0f, 0.0f);
            slider->setColour(juce::Colour(0xFF4A90D9));  // Blue
            slider->setValueUnit("dB");
            if (i < numOutputs)
                slider->setOutputName("Output " + juce::String(i + 1));
            else
                slider->setOutputName("Reverb " + juce::String(i - numOutputs + 1));
            addAndMakeVisible(slider);
        }

        resized();
    }

    /**
     * Set the currently selected input channel (0-based index).
     */
    void setSelectedInput(int inputIndex)
    {
        selectedInput = inputIndex;
    }

    /**
     * Update display with new DSP values.
     * Call this from a timer at ~50Hz.
     *
     * @param delaysMs Array of delay values [inputIndex * numOutputs + outputIndex]
     * @param levels Array of level values (linear 0-1)
     * @param hfDb Array of HF attenuation values (dB, negative)
     * @param reverbDelaysMs Array of reverb delays [inputIndex * numReverbs + reverbIndex]
     * @param reverbLevels Array of reverb levels (linear 0-1)
     * @param reverbHfDb Array of reverb HF attenuation (dB)
     */
    void updateValues(const float* delaysMs, const float* levels, const float* hfDb,
                      const float* reverbDelaysMs, const float* reverbLevels, const float* reverbHfDb)
    {
        if (selectedInput < 0) return;

        // Update output sliders
        for (int i = 0; i < numOutputs && i < delaySliders.size(); ++i)
        {
            int idx = selectedInput * numOutputs + i;

            if (delaysMs != nullptr)
                delaySliders[i]->setValue(delaysMs[idx]);

            if (levels != nullptr)
            {
                // Convert linear to dB
                float linearLevel = levels[idx];
                float dB = (linearLevel > 0.0f) ? 20.0f * std::log10(linearLevel) : -60.0f;
                levelSliders[i]->setValue(juce::jmax(-60.0f, dB));
            }

            if (hfDb != nullptr)
                hfSliders[i]->setValue(hfDb[idx]);
        }

        // Update reverb sliders
        for (int i = 0; i < numReverbs && (numOutputs + i) < delaySliders.size(); ++i)
        {
            int idx = selectedInput * numReverbs + i;
            int sliderIdx = numOutputs + i;

            if (reverbDelaysMs != nullptr)
                delaySliders[sliderIdx]->setValue(reverbDelaysMs[idx]);

            if (reverbLevels != nullptr)
            {
                float linearLevel = reverbLevels[idx];
                float dB = (linearLevel > 0.0f) ? 20.0f * std::log10(linearLevel) : -60.0f;
                levelSliders[sliderIdx]->setValue(juce::jmax(-60.0f, dB));
            }

            if (reverbHfDb != nullptr)
                hfSliders[sliderIdx]->setValue(reverbHfDb[idx]);
        }

        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xFF1E1E1E));

        // Draw row backgrounds with colored borders matching slider colors
        auto bounds = getLocalBounds();
        int rowHeight = (bounds.getHeight() - 6) / 3;

        // Delay row - yellow border
        g.setColour(juce::Colour(0xFFD4A017));
        g.drawRect(bounds.removeFromTop(rowHeight), 1);

        bounds.removeFromTop(2);  // Spacing

        // HF row - pink border
        g.setColour(juce::Colour(0xFFE07878));
        g.drawRect(bounds.removeFromTop(rowHeight), 1);

        bounds.removeFromTop(2);  // Spacing

        // Level row - blue border
        g.setColour(juce::Colour(0xFF4A90D9));
        g.drawRect(bounds, 1);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        const int labelWidth = 80;
        const int spacing = 2;
        const int gapBetweenOutputsAndReverbs = 10;  // Double padding between outputs and reverbs

        int totalSliders = numOutputs + numReverbs;
        if (totalSliders == 0) totalSliders = 1;

        // Calculate slider width accounting for the gap between outputs and reverbs
        int availableWidth = bounds.getWidth() - labelWidth - 10;
        if (numOutputs > 0 && numReverbs > 0)
            availableWidth -= gapBetweenOutputsAndReverbs;
        int sliderWidth = juce::jmax(15, availableWidth / totalSliders);
        int rowHeight = (bounds.getHeight() - 6) / 3;

        // Helper to layout sliders in a row with gap between outputs and reverbs
        auto layoutSlidersInRow = [&](juce::OwnedArray<VisualisationSlider>& sliders,
                                       juce::Rectangle<int>& row) {
            int x = spacing;
            for (int i = 0; i < sliders.size(); ++i)
            {
                // Add gap before reverb sliders
                if (i == numOutputs && numReverbs > 0)
                    x += gapBetweenOutputsAndReverbs;

                sliders[i]->setBounds(x, row.getY() + spacing,
                                      sliderWidth - spacing, row.getHeight() - spacing * 2);
                x += sliderWidth;
            }
        };

        // Delay row
        auto delayRow = bounds.removeFromTop(rowHeight);
        auto delayLabelArea = delayRow.removeFromRight(labelWidth);
        delayUnitLabel.setBounds(delayLabelArea.removeFromTop(20));
        delayLabel.setBounds(delayLabelArea.removeFromTop(20));
        layoutSlidersInRow(delaySliders, delayRow);

        bounds.removeFromTop(spacing);

        // HF row
        auto hfRow = bounds.removeFromTop(rowHeight);
        auto hfLabelArea = hfRow.removeFromRight(labelWidth);
        hfUnitLabel.setBounds(hfLabelArea.removeFromTop(20));
        hfLabel.setBounds(hfLabelArea.removeFromTop(35));
        layoutSlidersInRow(hfSliders, hfRow);

        bounds.removeFromTop(spacing);

        // Level row
        auto levelRow = bounds;
        auto levelLabelArea = levelRow.removeFromRight(labelWidth);
        levelUnitLabel.setBounds(levelLabelArea.removeFromTop(20));
        levelLabel.setBounds(levelLabelArea.removeFromTop(20));
        layoutSlidersInRow(levelSliders, levelRow);
    }

    /**
     * Enable/disable automatic updates via timer.
     * When enabled, call setDataSource() to provide callback for fetching values.
     */
    void setAutoUpdate(bool enabled)
    {
        if (enabled)
            startTimerHz(50);
        else
            stopTimer();
    }

    /**
     * Set callback to fetch DSP values.
     * Signature: void callback(int inputIndex, float* delays, float* levels, float* hf,
     *                          float* reverbDelays, float* reverbLevels, float* reverbHf)
     */
    std::function<void(int, float*, float*, float*, float*, float*, float*)> onFetchValues;

private:
    void timerCallback() override
    {
        if (onFetchValues && selectedInput >= 0)
        {
            std::vector<float> delays(numOutputs);
            std::vector<float> levels(numOutputs);
            std::vector<float> hf(numOutputs);
            std::vector<float> reverbDelays(numReverbs);
            std::vector<float> reverbLevels(numReverbs);
            std::vector<float> reverbHf(numReverbs);

            onFetchValues(selectedInput, delays.data(), levels.data(), hf.data(),
                          reverbDelays.data(), reverbLevels.data(), reverbHf.data());

            updateValues(delays.data(), levels.data(), hf.data(),
                         reverbDelays.data(), reverbLevels.data(), reverbHf.data());
        }
    }

    int numOutputs = 0;
    int numReverbs = 0;
    int selectedInput = 0;

    // Sliders for each output + reverb
    juce::OwnedArray<VisualisationSlider> delaySliders;
    juce::OwnedArray<VisualisationSlider> hfSliders;
    juce::OwnedArray<VisualisationSlider> levelSliders;

    // Labels
    juce::Label delayLabel, delayUnitLabel;
    juce::Label hfLabel, hfUnitLabel;
    juce::Label levelLabel, levelUnitLabel;

    // Tooltip window for hover tooltips on sliders
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InputVisualisationComponent)
};
