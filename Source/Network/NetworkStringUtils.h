#pragma once

#include <JuceHeader.h>
#include <cstring>

namespace WFSNetwork
{

/**
 * Safe juce::String constructors for bytes that came in over the wire.
 *
 * Why this exists:
 *   juce::String(const char* t, size_t maxChars) calls
 *     jassert(t == nullptr || CharPointer_ASCII::isValidString(t, (int)maxChars))
 *   in juce_String.cpp. That assert fires for ANY byte >= 128 — not just
 *   invalid UTF-8 sequences, but every legitimate UTF-8 multibyte char and
 *   every random-garbage byte. Network parsers must therefore validate the
 *   payload as UTF-8 themselves and use the UTF-8 constructor.
 *
 *   In debug builds the assert is __debugbreak(); in release the same
 *   constructor silently produces a malformed juce::String that propagates
 *   into the ValueTree, GUI, and DSP. Both behaviours are wrong for input
 *   we do not control.
 */

/** Construct a juce::String from a known-length byte buffer. Returns an
 *  empty string if the bytes are not valid UTF-8 (or len <= 0). Never
 *  reads past data + len. */
inline juce::String safeStringFromBytes (const char* data, int len)
{
    if (data == nullptr || len <= 0)
        return {};

    if (! juce::CharPointer_UTF8::isValidString (data, len))
        return {};

    return juce::String::fromUTF8 (data, len);
}

/** Construct a juce::String from a buffer that is *expected* to contain a
 *  null-terminated C string within the first maxBytes. Scans for the null
 *  inside the bounded range — never reads past data + maxBytes — then
 *  hands off to safeStringFromBytes for UTF-8 validation. */
inline juce::String safeStringFromBoundedCString (const char* data, int maxBytes)
{
    if (data == nullptr || maxBytes <= 0)
        return {};

    int len = 0;
    while (len < maxBytes && data[len] != '\0')
        ++len;

    return safeStringFromBytes (data, len);
}

} // namespace WFSNetwork
