#pragma once

#include <JuceHeader.h>
#include <functional>

/**
 * TTSComponentInfo - Metadata for TTS-enabled components
 *
 * Stores all information needed for accessibility announcements:
 * - Parameter name for immediate announcement
 * - Help text for delayed announcement (after static stay)
 * - Optional function to get current formatted value
 *
 * Usage:
 *   ttsInfoMap[&posXEditor] = TTSComponentInfo(
 *       "X Position",
 *       "Object position in Width. Nudge with Left and Right Arrow Keys.",
 *       [this]() { return posXEditor.getText() + " m"; }
 *   );
 */
struct TTSComponentInfo
{
    /** User-readable parameter name (e.g., "X Position", "Master Level") */
    juce::String parameterName;

    /** Full help text description for delayed announcement */
    juce::String helpText;

    /**
     * Optional function that returns the current formatted value.
     * Should include units where applicable (e.g., "2.5 m", "-3.0 dB").
     * If nullptr, only the parameter name will be announced.
     */
    std::function<juce::String()> getValueFunc;

    //==========================================================================
    // Constructors
    //==========================================================================

    /** Default constructor */
    TTSComponentInfo() = default;

    /** Full constructor with value getter */
    TTSComponentInfo(const juce::String& name,
                     const juce::String& help,
                     std::function<juce::String()> getValue = nullptr)
        : parameterName(name)
        , helpText(help)
        , getValueFunc(std::move(getValue))
    {
    }

    /** Constructor without value getter (for buttons, labels, etc.) */
    TTSComponentInfo(const juce::String& name,
                     const juce::String& help)
        : parameterName(name)
        , helpText(help)
        , getValueFunc(nullptr)
    {
    }

    //==========================================================================
    // Helpers
    //==========================================================================

    /** Get the current value string, or empty if no getter is set */
    juce::String getCurrentValue() const
    {
        if (getValueFunc)
            return getValueFunc();
        return {};
    }

    /** Check if this info has a value getter */
    bool hasValueGetter() const
    {
        return getValueFunc != nullptr;
    }
};
