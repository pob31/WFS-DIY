#include "RTTrPDecoder.h"

namespace RTTrP
{

bool Decoder::decode(const char* data, size_t size)
{
    // Minimum header size: 18 bytes
    // (2 intSig + 2 fltSig + 2 version + 4 packetId + 1 format + 2 packetSize + 4 context + 1 numModules)
    if (data == nullptr || size < 18)
        return false;

    trackables.clear();
    return parsePacket(reinterpret_cast<const uint8_t*>(data), size);
}

bool Decoder::parsePacket(const uint8_t* data, size_t size)
{
    // Read signatures (always big-endian in packet)
    uint16_t intSig = static_cast<uint16_t>((data[0] << 8) | data[1]);
    uint16_t fltSig = static_cast<uint16_t>((data[2] << 8) | data[3]);

    // Determine if byte swapping is needed based on float signature
    // 0x4334 indicates IEEE 754 float in network (big-endian) order
    needsByteSwap = (fltSig == SIGNATURE_FLOAT);

    // Verify valid RTTrP packet signature
    if (intSig != SIGNATURE_INT)
        return false;

    size_t offset = 4;

    // Version (2 bytes) - we don't validate version, just skip
    // uint16_t version = readUint16(data + offset, needsByteSwap);
    offset += 2;

    // Packet ID (4 bytes)
    // uint32_t packetId = readUint32(data + offset, needsByteSwap);
    offset += 4;

    // Packet format (1 byte) - 0x00 = Raw, 0x01 = Protobuf, 0x02 = Thrift
    // We only support Raw format
    uint8_t format = data[offset++];
    if (format != 0x00)
        return false;  // Only raw format supported

    // Packet size (2 bytes) - total size of content after header
    // uint16_t packetSize = readUint16(data + offset, needsByteSwap);
    offset += 2;

    // Context (4 bytes) - application-defined context
    // uint32_t context = readUint32(data + offset, needsByteSwap);
    offset += 4;

    // Number of modules/trackables (1 byte)
    uint8_t numModules = data[offset++];

    // Parse each trackable module
    for (uint8_t i = 0; i < numModules && offset < size; ++i)
    {
        if (!parseTrackable(data, size, offset))
            return false;
    }

    return true;
}

bool Decoder::parseTrackable(const uint8_t* data, size_t size, size_t& offset)
{
    // Need at least: size(2) + nameLen(1)
    if (offset + 3 > size)
        return false;

    Trackable trackable;

    // Module size (2 bytes) - total size of trackable module content
    uint16_t modSize = readUint16(data + offset, needsByteSwap);
    offset += 2;

    size_t moduleEnd = offset + modSize - 2;  // -2 because size field already read

    // Name length (1 byte)
    uint8_t nameLen = data[offset++];

    // Name string
    if (offset + nameLen > size)
        return false;
    trackable.name = juce::String(reinterpret_cast<const char*>(data + offset), nameLen);
    offset += nameLen;

    // Extract trackable ID from name
    // RTTrP uses name to identify trackables - typically numeric IDs or "Tracker1", "Tracker2", etc.
    // Try to extract integer from start of name
    trackable.id = trackable.name.getIntValue();
    if (trackable.id == 0 && !trackable.name.startsWithChar('0'))
    {
        // Name doesn't start with a number, try to extract number from anywhere in name
        juce::String numPart;
        for (int i = 0; i < trackable.name.length(); ++i)
        {
            if (trackable.name[i] >= '0' && trackable.name[i] <= '9')
                numPart += trackable.name[i];
        }
        if (numPart.isNotEmpty())
            trackable.id = numPart.getIntValue();
    }

    // Number of sub-modules (1 byte)
    if (offset >= size)
        return false;
    uint8_t numSubMods = data[offset++];

    // Timestamp (4 bytes) - microseconds
    if (offset + 4 > size)
        return false;
    // uint32_t timestamp = readUint32(data + offset, needsByteSwap);
    offset += 4;

    // Parse sub-modules (position, orientation, etc.)
    for (uint8_t j = 0; j < numSubMods && offset < size && offset < moduleEnd; ++j)
    {
        if (offset >= size)
            return false;

        uint8_t modType = data[offset++];

        switch (modType)
        {
            case MOD_CENTROID:
                if (!parseCentroid(data, size, offset, trackable))
                    return false;
                break;

            case MOD_QUATERNION:
                if (!parseQuaternion(data, size, offset, trackable))
                    return false;
                break;

            case MOD_EULER:
                if (!parseEuler(data, size, offset, trackable))
                    return false;
                break;

            default:
                // Skip unknown modules - read size and skip content
                if (offset + 2 > size)
                    return false;
                uint16_t skipSize = readUint16(data + offset, needsByteSwap);
                offset += 2;
                if (offset + skipSize > size)
                    return false;
                offset += skipSize - 2;  // -2 because size itself counts
                break;
        }
    }

    trackables[trackable.id] = trackable;
    return true;
}

bool Decoder::parseCentroid(const uint8_t* data, size_t size, size_t& offset, Trackable& t)
{
    // CentroidMod structure:
    // - Size (2 bytes) - includes size field itself
    // - Latency (2 bytes)
    // - X, Y, Z (3 x 8 bytes double)
    // Total: 28 bytes

    if (offset + 28 > size)
        return false;

    // Skip size (2 bytes)
    offset += 2;

    // Skip latency (2 bytes)
    offset += 2;

    // Read position coordinates
    t.position.x = readDouble(data + offset, needsByteSwap);
    offset += 8;

    t.position.y = readDouble(data + offset, needsByteSwap);
    offset += 8;

    t.position.z = readDouble(data + offset, needsByteSwap);
    offset += 8;

    t.hasPosition = true;
    return true;
}

bool Decoder::parseQuaternion(const uint8_t* data, size_t size, size_t& offset, Trackable& t)
{
    // QuatModule structure:
    // - Size (2 bytes)
    // - Latency (2 bytes)
    // - Qx, Qy, Qz, Qw (4 x 8 bytes double)
    // Total: 36 bytes

    if (offset + 36 > size)
        return false;

    // Skip size (2 bytes)
    offset += 2;

    // Skip latency (2 bytes)
    offset += 2;

    // Read quaternion components
    t.quaternion.qx = readDouble(data + offset, needsByteSwap);
    offset += 8;

    t.quaternion.qy = readDouble(data + offset, needsByteSwap);
    offset += 8;

    t.quaternion.qz = readDouble(data + offset, needsByteSwap);
    offset += 8;

    t.quaternion.qw = readDouble(data + offset, needsByteSwap);
    offset += 8;

    t.hasQuaternion = true;
    return true;
}

bool Decoder::parseEuler(const uint8_t* data, size_t size, size_t& offset, Trackable& t)
{
    // EulerModule structure:
    // - Size (2 bytes)
    // - Latency (2 bytes)
    // - Order (2 bytes) - rotation order (e.g., XYZ, ZYX, etc.)
    // - R1, R2, R3 (3 x 8 bytes double) - rotation angles
    // Total: 30 bytes

    if (offset + 30 > size)
        return false;

    // Skip size (2 bytes)
    offset += 2;

    // Skip latency (2 bytes)
    offset += 2;

    // Read rotation order
    t.euler.order = readUint16(data + offset, needsByteSwap);
    offset += 2;

    // Read rotation angles
    t.euler.r1 = readDouble(data + offset, needsByteSwap);
    offset += 8;

    t.euler.r2 = readDouble(data + offset, needsByteSwap);
    offset += 8;

    t.euler.r3 = readDouble(data + offset, needsByteSwap);
    offset += 8;

    t.hasEuler = true;
    return true;
}

double Decoder::readDouble(const uint8_t* data, bool swap) const
{
    double val;

    if (swap)
    {
        // Reverse byte order for big-endian to little-endian conversion
        uint8_t tmp[8];
        for (int i = 0; i < 8; ++i)
            tmp[i] = data[7 - i];
        std::memcpy(&val, tmp, 8);
    }
    else
    {
        std::memcpy(&val, data, 8);
    }

    return val;
}

uint32_t Decoder::readUint32(const uint8_t* data, bool swap) const
{
    if (swap)
    {
        // Big-endian
        return (static_cast<uint32_t>(data[0]) << 24) |
               (static_cast<uint32_t>(data[1]) << 16) |
               (static_cast<uint32_t>(data[2]) << 8) |
               static_cast<uint32_t>(data[3]);
    }
    else
    {
        // Little-endian
        return (static_cast<uint32_t>(data[3]) << 24) |
               (static_cast<uint32_t>(data[2]) << 16) |
               (static_cast<uint32_t>(data[1]) << 8) |
               static_cast<uint32_t>(data[0]);
    }
}

uint16_t Decoder::readUint16(const uint8_t* data, bool swap) const
{
    if (swap)
    {
        // Big-endian
        return static_cast<uint16_t>((data[0] << 8) | data[1]);
    }
    else
    {
        // Little-endian
        return static_cast<uint16_t>((data[1] << 8) | data[0]);
    }
}

} // namespace RTTrP
