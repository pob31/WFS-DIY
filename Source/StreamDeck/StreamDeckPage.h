#pragma once

/**
 * StreamDeckPage — Data structures for the page/section/binding system.
 *
 * Hierarchy:
 *   StreamDeckPage  (one per tab+subtab combination)
 *     └── StreamDeckSection[4]  (selected by top-row buttons 0-3)
 *           ├── ButtonBinding[4]  (bottom-row buttons 4-7)
 *           └── DialBinding[4]    (rotary dials 0-3, displayed on LCD strip)
 *
 * Each binding type uses std::function callbacks for getValue/setValue,
 * making bindings independent of any specific parameter system.
 */

#include <JuceHeader.h>

//==============================================================================
/** Describes how a rotary dial is bound to a parameter. */
struct DialBinding
{
    /** Display name shown on LCD strip (should be localized). */
    juce::String paramName;

    /** Unit label shown after value (e.g., "dB", "ms", "%", "Hz"). */
    juce::String paramUnit;

    /** Display range — the human-readable min/max values. */
    float minValue = 0.0f;
    float maxValue = 1.0f;

    /** Increment per dial detent click. */
    float step = 0.01f;

    /** If true, use exponential mapping: value = min * pow(max/min, normalized).
        Good for frequency, RT60, and other perceptually-scaled parameters. */
    bool isExponential = false;

    /** Number of decimal places for display formatting. */
    int decimalPlaces = 2;

    /** The type of control this dial represents. */
    enum Type
    {
        Float,      // Continuous value — rotation changes by step
        Int,        // Integer value — rotation changes by 1 (or step)
        ComboBox    // Discrete selection — press to open, rotate to browse, press to confirm
    };

    Type type = Float;

    /** For ComboBox type: the list of option labels. */
    juce::StringArray comboOptions;

    /** Get the current value (called to display on LCD). */
    std::function<float()> getValue;

    /** Set a new value (called when dial is rotated). */
    std::function<void (float)> setValue;

    /** Returns true if this binding is configured (has valid callbacks). */
    bool isValid() const { return getValue != nullptr && setValue != nullptr; }

    /** Format the current value as a display string. */
    juce::String formatValue() const
    {
        if (! isValid())
            return "--";

        float v = getValue();

        if (type == ComboBox)
        {
            int index = juce::roundToInt (v);
            if (index >= 0 && index < comboOptions.size())
                return comboOptions[index];
            return juce::String (index);
        }

        if (type == Int)
            return juce::String (juce::roundToInt (v));

        return juce::String (v, decimalPlaces);
    }

    /** Format value with unit for LCD display. */
    juce::String formatValueWithUnit() const
    {
        juce::String val = formatValue();
        if (paramUnit.isNotEmpty() && type != ComboBox)
            return val + " " + paramUnit;
        return val;
    }

    /** Apply one step of rotation (direction: +1 or -1). Returns new value. */
    float applyStep (int direction) const
    {
        if (! isValid())
            return 0.0f;

        float current = getValue();

        if (type == ComboBox)
        {
            int index = juce::roundToInt (current) + direction;
            index = juce::jlimit (0, comboOptions.size() - 1, index);
            return static_cast<float> (index);
        }

        if (isExponential && minValue > 0.0f && maxValue > minValue)
        {
            // Convert to normalized 0-1, step in linear space, convert back
            float normalized = std::log (current / minValue) / std::log (maxValue / minValue);
            normalized = juce::jlimit (0.0f, 1.0f, normalized + step * direction);
            return minValue * std::pow (maxValue / minValue, normalized);
        }

        float newVal = current + step * direction;
        return juce::jlimit (minValue, maxValue, newVal);
    }
};

//==============================================================================
/** Describes how a bottom-row button is bound to a function. */
struct ButtonBinding
{
    /** Short label displayed on the button (max ~8 characters). */
    juce::String label;

    /** Background color when the button is in "off" state. */
    juce::Colour colour = juce::Colours::darkgrey;

    /** Background color when the button is in "on" state (for toggles). */
    juce::Colour activeColour = juce::Colours::dodgerblue;

    /** The type of button behavior. */
    enum Type
    {
        Toggle,     // Click toggles on/off state
        Momentary,  // Active while held down
        Action      // Single-fire on press (no state)
    };

    Type type = Toggle;

    /** Get the current toggle state (for Toggle type). */
    std::function<bool()> getState;

    /** Called when the button is pressed. */
    std::function<void()> onPress;

    /** Called when the button is released (for Momentary type). */
    std::function<void()> onRelease;

    /** Returns true if this binding is configured. */
    bool isValid() const { return onPress != nullptr; }
};

//==============================================================================
/** A section groups 4 bottom-row buttons + 4 dial bindings.
    Selected by one of the 4 top-row buttons. */
struct StreamDeckSection
{
    /** Name shown on the top-row section selector button. */
    juce::String sectionName;

    /** Color for the section selector button. */
    juce::Colour sectionColour = juce::Colours::grey;

    /** The 4 bottom-row button bindings (indices map to button IDs 4-7). */
    ButtonBinding buttons[4];

    /** The 4 rotary dial bindings. */
    DialBinding dials[4];
};

//==============================================================================
/** A page represents the complete Stream Deck layout for a specific tab+subtab.
    Contains up to 4 sections, one of which is active at a time. */
class StreamDeckPage
{
public:
    StreamDeckPage() = default;

    explicit StreamDeckPage (const juce::String& name)
        : pageName (name) {}

    /** Human-readable page name (e.g., "Inputs > Parameters"). */
    juce::String pageName;

    /** Up to 4 sections (selected by top-row buttons). */
    StreamDeckSection sections[4];

    /** Number of active sections on this page (1-4). */
    int numSections = 0;

    /** Index of the currently active section. */
    int activeSectionIndex = 0;

    /** Get the currently active section. */
    StreamDeckSection& getActiveSection()
    {
        return sections[juce::jlimit (0, juce::jmax (0, numSections - 1), activeSectionIndex)];
    }

    const StreamDeckSection& getActiveSection() const
    {
        return sections[juce::jlimit (0, juce::jmax (0, numSections - 1), activeSectionIndex)];
    }

    /** Select a section by index (0-3). Returns true if the section changed. */
    bool setActiveSection (int index)
    {
        int clamped = juce::jlimit (0, juce::jmax (0, numSections - 1), index);
        if (clamped == activeSectionIndex)
            return false;
        activeSectionIndex = clamped;
        return true;
    }
};
