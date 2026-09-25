#pragma once

#include <JuceHeader.h>
#include <optional>

/**
    Reading a number someone typed into a value field.

    Every click-to-type label in the app shows its value WITH its unit, and
    the operator edits that text, so the reader has to understand what the
    display says; the digits alone are not enough. "2.5 kHz" is 2500 Hz, not
    2.5. "2m 30s" is 150 s, not 230. "1:2.0" (an expander ratio) is 2, not 12.
    These readers are the app's one way to do it: a field may show its value
    in whatever form reads best, as long as the matching reader reads that
    form back.

    Each returns nothing when the text holds no number at all, so a stray
    letter never becomes a value of 0 (0 dB - full level - on an attenuation
    field). The caller then puts the label back as it was.

    A COPY of spatcore/ui/TypedValue.h (namespace spatcore::ui::typed, spatcore
    3b11648 on feature/typed-value), where XOA and Go.dot get it. The app keeps
    its own until its spatcore pin - on the dial-acceleration branch - includes
    that commit; then include the spatcore header and delete this one. Until
    then any change here goes to both.
*/
namespace TypedValue
{
    namespace detail
    {
        inline bool isDigit (juce::juce_wchar c) { return c >= '0' && c <= '9'; }

        /** Commas as decimal points, the Unicode minus as a hyphen. */
        inline juce::String normalise (const juce::String& text)
        {
            return text.replaceCharacter (',', '.').replaceCharacter (juce::juce_wchar (0x2212), '-');
        }
    }

    /** The first number in the text. Units, words and spaces around it are
        ignored, a comma is a decimal point, and a "k" right after the number
        (with or without a space) means thousands: "1.2 kHz", "1,2k" -> 1200. */
    inline std::optional<float> number (const juce::String& text)
    {
        using detail::isDigit;
        const auto t = detail::normalise (text);
        const int n = t.length();

        for (int i = 0; i < n; ++i)
        {
            const bool startsNumber = isDigit (t[i])
                                   || (t[i] == '.' && i + 1 < n && isDigit (t[i + 1]))
                                   || (t[i] == '-' && i + 1 < n && (isDigit (t[i + 1]) || t[i + 1] == '.'));
            if (! startsNumber)
                continue;

            int end = i + 1;
            while (end < n && (isDigit (t[end]) || t[end] == '.'))
                ++end;

            float value = t.substring (i, end).getFloatValue();

            int next = end;
            while (next < n && t[next] == ' ')
                ++next;
            if (next < n && (t[next] == 'k' || t[next] == 'K'))
                value *= 1000.0f;

            return value;
        }

        return std::nullopt;
    }

    /** A duration in seconds, read the way people write one:
          "90", "5.00 s", "500 ms"
          "2m 30s", "2 min", "2min", "2 mn 30", "1.5 min"
          "1h", "1 h 30 min", "1h30" - a bare number after hours is minutes,
                 after minutes it is seconds
          "1:30" (m:ss), "1:02:03" (h:mm:ss), "1:30.5"
        The unit is read from its first letter, so words work too (sec,
        second, minute, hr, hour), as do the Japanese/Chinese/Korean hour,
        minute and second signs and the German "Std". */
    inline std::optional<float> duration (const juce::String& text)
    {
        using detail::isDigit;
        const auto t = detail::normalise (text).toLowerCase().trim();

        // Clock form: each field is sixty of the next
        if (t.containsChar (':'))
        {
            juce::StringArray fields;
            fields.addTokens (t, ":", "");
            if (fields.size() > 3)
                return std::nullopt;

            float total = 0.0f;
            bool any = false;
            for (const auto& field : fields)
            {
                const auto digits = field.retainCharacters ("0123456789.");
                any = any || digits.isNotEmpty();
                total = total * 60.0f + digits.getFloatValue();
            }
            return any ? std::optional<float> (total) : std::nullopt;
        }

        const int n = t.length();
        float total = 0.0f;
        float lastScale = 0.0f;
        bool any = false;

        for (int i = 0; i < n;)
        {
            if (! isDigit (t[i]) && ! (t[i] == '.' && i + 1 < n && isDigit (t[i + 1])))
            {
                ++i;
                continue;
            }

            int end = i;
            while (end < n && (isDigit (t[end]) || t[end] == '.'))
                ++end;
            const float value = t.substring (i, end).getFloatValue();

            int u = end;
            while (u < n && t[u] == ' ')
                ++u;
            const auto c  = u < n     ? t[u]     : juce::juce_wchar (0);
            const auto c2 = u + 1 < n ? t[u + 1] : juce::juce_wchar (0);

            float scale;
            if (c == 'h' || (c == 's' && c2 == 't')                            // h, hr, hour; Std
                || c == 0x6642 || c == 0x65F6 || c == 0xC2DC)                   // U+6642 U+65F6 U+C2DC (hour signs)
                scale = 3600.0f;
            else if (c == 'm' && c2 == 's')                                     // ms, msec
                scale = 0.001f;
            else if (c == 'm' || c == 0x5206 || c == 0xBD84)                    // m, min, mn; U+5206 U+BD84 (minute signs)
                scale = 60.0f;
            else if (c == 's' || c == 0x79D2 || c == 0xCD08)                    // s, sec; U+79D2 U+CD08 (second signs)
                scale = 1.0f;
            else
                scale = lastScale >= 3600.0f ? 60.0f : 1.0f;                    // bare: the next unit down

            total += value * scale;
            lastScale = scale;
            any = true;
            i = end;
        }

        return any ? std::optional<float> (total) : std::nullopt;
    }

    /** A ratio as the compressor and expander labels show it: "4.0:1" -> 4,
        "1:2.0" -> 2 (the side that is not 1), "3:2" -> 1.5, or a bare "3". */
    inline std::optional<float> ratio (const juce::String& text)
    {
        if (! text.containsChar (':'))
            return number (text);

        const auto left  = number (text.upToFirstOccurrenceOf (":", false, false));
        const auto right = number (text.fromFirstOccurrenceOf (":", false, false));

        if (left.has_value() && right.has_value())
        {
            if (std::abs (*right - 1.0f) < 1.0e-6f) return left;
            if (std::abs (*left - 1.0f) < 1.0e-6f)  return right;
            if (*right != 0.0f)                     return *left / *right;
            return std::nullopt;
        }

        return left.has_value() ? left : right;
    }
}
