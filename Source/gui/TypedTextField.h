#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <optional>
#include <set>
#include "../Helpers/TypedValue.h"

/**
    TypedTextField - the rules for a TextEditor that holds a number the
    operator types (re-audit 2026-09-29, F8).

    The fields used to read their text with getFloatValue(), which stops at a
    comma ("-2,5" read as -2) and turns empty text or a stray letter into 0,
    so a cleared box moved a source to the origin. Fields restricted to
    "-0123456789." also dropped the comma as it was typed ("-2,5" became -25).

    Now:
      - the comma is a decimal point (restrictDecimal allows it);
      - the text is read by TypedValue::number, the reader every click-to-type
        label uses, and a field whose text holds no number puts the stored
        value back and writes nothing (read);
      - a field is written only if the operator typed into it (EditTracker),
        so clicking in and out, or Esc, does not round a stored value through
        its display.
*/
namespace TypedTextField
{
    /** Digits, both decimal separators and, when allowed, a sign. */
    inline void restrictDecimal (juce::TextEditor& editor, int maxLength, bool allowNegative)
    {
        editor.setInputRestrictions (maxLength, allowNegative ? "0123456789.,-+" : "0123456789.,");
    }

    /** The number in `text`, or nothing when it holds none or it is not finite. */
    inline std::optional<float> parse (const juce::String& text)
    {
        if (auto value = TypedValue::number (text); value && std::isfinite (*value))
            return value;
        return std::nullopt;
    }

    /** The editor's number, or nothing - and then the editor shows
        `restoreText` again, and the caller writes nothing. */
    inline std::optional<float> read (juce::TextEditor& editor, const juce::String& restoreText)
    {
        if (auto value = parse (editor.getText()))
            return value;

        editor.setText (restoreText, false);
        return std::nullopt;
    }

    /** A whole number in [lo, hi] typed as digits only (ports, counts, patch
        numbers), or nothing for empty text, a non-digit or a value out of range. */
    inline std::optional<int> readInt (const juce::String& text, int lo, int hi)
    {
        const auto trimmed = text.trim();
        if (trimmed.isEmpty() || trimmed.length() > 9 || ! trimmed.containsOnly ("0123456789"))
            return std::nullopt;

        const int value = trimmed.getIntValue();
        return value >= lo && value <= hi ? std::optional<int> (value) : std::nullopt;
    }

    /** "Was this field typed into since it was loaded?" Loads use
        setText (..., false), which does not count. */
    class EditTracker final : private juce::TextEditor::Listener
    {
    public:
        void watch (juce::TextEditor& editor)        { editor.addListener (this); }

        /** True once per edit: the caller writes, the edit is forgotten. */
        bool take (juce::TextEditor& editor)         { return edited.erase (&editor) > 0; }

        void forget (juce::TextEditor& editor)       { edited.erase (&editor); }
        void markEditedForTest (juce::TextEditor& editor) { edited.insert (&editor); }

    private:
        void textEditorTextChanged (juce::TextEditor& editor) override { edited.insert (&editor); }

        std::set<juce::TextEditor*> edited;
    };
}
