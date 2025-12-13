#pragma once

#include <JuceHeader.h>

namespace WFSNetwork
{

/**
 * OSCParser
 *
 * Simple OSC parser that handles messages and bundles.
 * We need our own parser because juce::OSCInputStream is a private class.
 */
namespace OSCParser
{
    // Align position to 4-byte boundary
    inline int alignTo4(int pos) { return (pos + 3) & ~3; }

    // Read a null-terminated OSC string with 4-byte alignment
    inline juce::String readString(const char* data, int dataSize, int& pos)
    {
        if (pos >= dataSize)
            return {};

        const char* start = data + pos;
        const char* end = start;

        while ((end - data) < dataSize && *end != '\0')
            ++end;

        juce::String result(start, static_cast<size_t>(end - start));
        pos = alignTo4(static_cast<int>(end - data + 1));  // Skip null and align
        return result;
    }

    // Read a 4-byte big-endian int32
    inline int32_t readInt32(const char* data, int dataSize, int& pos)
    {
        if (pos + 4 > dataSize)
            return 0;

        uint32_t value = (static_cast<uint8_t>(data[pos]) << 24) |
                         (static_cast<uint8_t>(data[pos + 1]) << 16) |
                         (static_cast<uint8_t>(data[pos + 2]) << 8) |
                         static_cast<uint8_t>(data[pos + 3]);
        pos += 4;
        return static_cast<int32_t>(value);
    }

    // Read a 4-byte big-endian float
    inline float readFloat32(const char* data, int dataSize, int& pos)
    {
        if (pos + 4 > dataSize)
            return 0.0f;

        uint32_t bits = (static_cast<uint8_t>(data[pos]) << 24) |
                        (static_cast<uint8_t>(data[pos + 1]) << 16) |
                        (static_cast<uint8_t>(data[pos + 2]) << 8) |
                        static_cast<uint8_t>(data[pos + 3]);
        pos += 4;

        float result;
        std::memcpy(&result, &bits, sizeof(float));
        return result;
    }

    // Read a 8-byte big-endian int64 (for timetag)
    inline int64_t readInt64(const char* data, int dataSize, int& pos)
    {
        if (pos + 8 > dataSize)
            return 0;

        uint64_t value = 0;
        for (int i = 0; i < 8; ++i)
            value = (value << 8) | static_cast<uint8_t>(data[pos + i]);
        pos += 8;
        return static_cast<int64_t>(value);
    }

    // Parse a single OSC message
    inline juce::OSCMessage parseMessage(const char* data, int dataSize, int& pos)
    {
        // Read address pattern
        juce::String address = readString(data, dataSize, pos);
        if (address.isEmpty() || !address.startsWith("/"))
            throw juce::OSCFormatError("Invalid OSC address pattern");

        juce::OSCMessage message(address);

        // Read type tag string
        juce::String typeTag = readString(data, dataSize, pos);
        if (typeTag.isEmpty() || !typeTag.startsWith(","))
            return message;  // No arguments

        // Parse arguments based on type tags
        for (int i = 1; i < typeTag.length(); ++i)
        {
            char type = static_cast<char>(typeTag[i]);
            switch (type)
            {
                case 'i':  // int32
                    message.addInt32(readInt32(data, dataSize, pos));
                    break;
                case 'f':  // float32
                    message.addFloat32(readFloat32(data, dataSize, pos));
                    break;
                case 's':  // string
                    message.addString(readString(data, dataSize, pos));
                    break;
                case 'b':  // blob
                {
                    int blobSize = readInt32(data, dataSize, pos);
                    if (pos + blobSize <= dataSize)
                    {
                        message.addBlob(juce::MemoryBlock(data + pos, static_cast<size_t>(blobSize)));
                        pos = alignTo4(pos + blobSize);
                    }
                    break;
                }
                case 'T':  // True
                    message.addArgument(juce::OSCArgument(true));
                    break;
                case 'F':  // False
                    message.addArgument(juce::OSCArgument(false));
                    break;
                // Other types (h, t, d, c, r, m, N, I) can be added as needed
                default:
                    // Unknown type - skip
                    break;
            }
        }

        return message;
    }

    // Forward declaration for parseBundle
    inline juce::OSCBundle parseBundle(const char* data, int dataSize, int& pos);

    // Parse an OSC bundle
    inline juce::OSCBundle parseBundle(const char* data, int dataSize, int& pos)
    {
        // Skip "#bundle" identifier (should already be confirmed)
        pos += 8;

        // Read timetag (8 bytes)
        readInt64(data, dataSize, pos);  // We don't use the timetag currently

        juce::OSCBundle bundle;

        // Read bundle elements
        while (pos < dataSize)
        {
            // Read element size (4 bytes)
            int elementSize = readInt32(data, dataSize, pos);
            if (elementSize <= 0 || pos + elementSize > dataSize)
                break;

            int elementEnd = pos + elementSize;

            // Check if element is a bundle or message
            if (elementSize >= 8 && std::memcmp(data + pos, "#bundle", 7) == 0)
            {
                // Nested bundle
                auto nestedBundle = parseBundle(data, dataSize, pos);
                bundle.addElement(nestedBundle);
            }
            else
            {
                // Message
                auto message = parseMessage(data, dataSize, pos);
                bundle.addElement(message);
            }

            pos = elementEnd;  // Ensure we're at the correct position
        }

        return bundle;
    }

} // namespace OSCParser

} // namespace WFSNetwork
