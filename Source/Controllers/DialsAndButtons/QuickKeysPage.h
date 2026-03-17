#pragma once

/**
 * QuickKeysPage — Data structures for Xencelabs Quick Keys page bindings.
 *
 * Unlike the Stream Deck+ which has sections/buttons/dials, the Quick Keys
 * has a single wheel + wheel button. Parameters are organized as a flat list
 * that the user traverses with the wheel, then presses to adjust.
 *
 * Reuses DialBinding from StreamDeckPage.h for parameter definitions.
 */

#include "StreamDeckPage.h"

//==============================================================================
/** A single traversable parameter binding for the Quick Keys wheel. */
struct QuickKeysBinding
{
    /** The parameter definition (reuses DialBinding for getValue/setValue/step/etc). */
    DialBinding dial;

    /** LED ring color when this parameter is focused (matches the GUI slider active track). */
    juce::Colour ledColour = juce::Colour (0xFFFF5722);  // Default: deep orange (WfsStandardSlider)
};

//==============================================================================
/** A page represents the complete Quick Keys layout for a specific tab+subtab.
    Contains a flat list of traversable parameters. */
struct QuickKeysPage
{
    /** Human-readable page name (for debug). */
    juce::String pageName;

    /** OLED chunk 1: tab name (max 8 chars, e.g., "Binaural"). */
    juce::String tabName;

    /** OLED chunk 2: section/subtab name (max 8 chars, e.g., "Renderer"). */
    juce::String sectionName;

    /** The ordered list of traversable parameters. */
    std::vector<QuickKeysBinding> bindings;

    /** Index of the currently focused parameter. */
    int focusedIndex = 0;

    /** Move focus to the next parameter. Returns true if focus changed. */
    bool focusNext()
    {
        if (bindings.empty())
            return false;
        int next = focusedIndex + 1;
        if (next >= (int) bindings.size())
            next = 0;  // Wrap around
        if (next == focusedIndex)
            return false;
        focusedIndex = next;
        return true;
    }

    /** Move focus to the previous parameter. Returns true if focus changed. */
    bool focusPrev()
    {
        if (bindings.empty())
            return false;
        int prev = focusedIndex - 1;
        if (prev < 0)
            prev = (int) bindings.size() - 1;  // Wrap around
        if (prev == focusedIndex)
            return false;
        focusedIndex = prev;
        return true;
    }

    /** Get the currently focused binding (or nullptr if empty). */
    QuickKeysBinding* getFocused()
    {
        if (bindings.empty())
            return nullptr;
        focusedIndex = juce::jlimit (0, (int) bindings.size() - 1, focusedIndex);
        return &bindings[(size_t) focusedIndex];
    }

    const QuickKeysBinding* getFocused() const
    {
        if (bindings.empty())
            return nullptr;
        int idx = juce::jlimit (0, (int) bindings.size() - 1, focusedIndex);
        return &bindings[(size_t) idx];
    }
};
