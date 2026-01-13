#pragma once

#include <JuceHeader.h>

namespace WFSNetwork
{

/**
 * OSCSerializer
 *
 * Simple OSC serializer that converts messages and bundles to raw bytes.
 * This is the inverse of OSCParser - used for TCP transmission where we
 * need to manually serialize OSC data (JUCE OSCSender only supports UDP).
 */
namespace OSCSerializer
{
    // Align size to 4-byte boundary
    inline int alignTo4(int size) { return (size + 3) & ~3; }

    // Write a null-terminated OSC string with 4-byte padding
    inline void writeString(juce::MemoryOutputStream& out, const juce::String& str)
    {
        auto utf8 = str.toRawUTF8();
        int len = static_cast<int>(std::strlen(utf8));
        out.write(utf8, static_cast<size_t>(len + 1));  // Include null terminator
        int padded = alignTo4(len + 1);
        for (int i = len + 1; i < padded; ++i)
            out.writeByte(0);
    }

    // Write a 4-byte big-endian int32
    inline void writeInt32(juce::MemoryOutputStream& out, int32_t value)
    {
        out.writeByte(static_cast<char>((value >> 24) & 0xFF));
        out.writeByte(static_cast<char>((value >> 16) & 0xFF));
        out.writeByte(static_cast<char>((value >> 8) & 0xFF));
        out.writeByte(static_cast<char>(value & 0xFF));
    }

    // Write a 4-byte big-endian float
    inline void writeFloat32(juce::MemoryOutputStream& out, float value)
    {
        uint32_t bits;
        std::memcpy(&bits, &value, sizeof(float));
        writeInt32(out, static_cast<int32_t>(bits));
    }

    // Write an 8-byte big-endian int64 (for timetag)
    inline void writeInt64(juce::MemoryOutputStream& out, int64_t value)
    {
        for (int i = 7; i >= 0; --i)
            out.writeByte(static_cast<char>((value >> (i * 8)) & 0xFF));
    }

    // Write an OSC blob (size prefix + data + padding)
    inline void writeBlob(juce::MemoryOutputStream& out, const juce::MemoryBlock& blob)
    {
        writeInt32(out, static_cast<int32_t>(blob.getSize()));
        out.write(blob.getData(), blob.getSize());
        int padded = alignTo4(static_cast<int>(blob.getSize()));
        for (int i = static_cast<int>(blob.getSize()); i < padded; ++i)
            out.writeByte(0);
    }

    // Serialize an OSC message to bytes
    inline juce::MemoryBlock serializeMessage(const juce::OSCMessage& message)
    {
        juce::MemoryOutputStream out;

        // Write address pattern
        writeString(out, message.getAddressPattern().toString());

        // Build type tag string
        juce::String typeTag = ",";
        for (const auto& arg : message)
        {
            if (arg.isInt32())        typeTag += "i";
            else if (arg.isFloat32()) typeTag += "f";
            else if (arg.isString())  typeTag += "s";
            else if (arg.isBlob())    typeTag += "b";
            // Add more types as needed (T, F, etc.)
        }
        writeString(out, typeTag);

        // Write arguments
        for (const auto& arg : message)
        {
            if (arg.isInt32())
                writeInt32(out, arg.getInt32());
            else if (arg.isFloat32())
                writeFloat32(out, arg.getFloat32());
            else if (arg.isString())
                writeString(out, arg.getString());
            else if (arg.isBlob())
                writeBlob(out, arg.getBlob());
        }

        return out.getMemoryBlock();
    }

    // Forward declaration
    inline juce::MemoryBlock serializeBundle(const juce::OSCBundle& bundle);

    // Serialize an OSC bundle to bytes
    inline juce::MemoryBlock serializeBundle(const juce::OSCBundle& bundle)
    {
        juce::MemoryOutputStream out;

        // Write bundle header "#bundle\0"
        out.write("#bundle", 8);

        // Write timetag (8 bytes, immediate = 1)
        writeInt64(out, 1);

        // Write each element
        for (const auto& element : bundle)
        {
            juce::MemoryBlock elementData;
            if (element.isMessage())
                elementData = serializeMessage(element.getMessage());
            else if (element.isBundle())
                elementData = serializeBundle(element.getBundle());

            // Write element size prefix
            writeInt32(out, static_cast<int32_t>(elementData.getSize()));
            out.write(elementData.getData(), elementData.getSize());
        }

        return out.getMemoryBlock();
    }

} // namespace OSCSerializer

} // namespace WFSNetwork
