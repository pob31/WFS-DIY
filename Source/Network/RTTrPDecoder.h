#pragma once

#include <JuceHeader.h>
#include <map>
#include <cstring>

namespace RTTrP
{

// Protocol constants
static constexpr uint16_t SIGNATURE_INT = 0x4154;   // "AT" in big-endian
static constexpr uint16_t SIGNATURE_FLOAT = 0x4334; // "C4" (IEEE 754 float)
static constexpr int DEFAULT_PORT = 24220;
static constexpr int MAX_PACKET_SIZE = 1500;

// Module type IDs from RTTrP spec
enum ModuleType : uint8_t
{
    MOD_TRACKABLE = 0x01,
    MOD_CENTROID = 0x02,
    MOD_QUATERNION = 0x03,
    MOD_EULER = 0x04,
    MOD_CENTROID_ACC_VEL = 0x06,
    MOD_LED = 0x21,
    MOD_LED_ACC_VEL = 0x22
};

struct Position
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct Quaternion
{
    double qx = 0.0;
    double qy = 0.0;
    double qz = 0.0;
    double qw = 1.0;
};

struct EulerAngles
{
    uint16_t order = 0;  // Rotation order
    double r1 = 0.0;
    double r2 = 0.0;
    double r3 = 0.0;
};

struct Trackable
{
    int id = -1;
    juce::String name;
    Position position;
    Quaternion quaternion;
    EulerAngles euler;
    bool hasPosition = false;
    bool hasQuaternion = false;
    bool hasEuler = false;
};

/**
 * RTTrP Decoder
 *
 * Minimal decoder for RTTrP (Real-Time Tracking Protocol) motion packets.
 * Parses position (CentroidMod), quaternion orientation (QuatModule),
 * and Euler orientation (EulerModule) from RTTrPM packets.
 *
 * Cross-platform: Uses only standard C++ and JUCE types.
 * Based on RTTrP v2.4.2.0 specification.
 */
class Decoder
{
public:
    Decoder() = default;
    ~Decoder() = default;

    /**
     * Decode an RTTrP packet.
     * @param data Raw packet data
     * @param size Size of packet in bytes
     * @return true if packet was valid RTTrP and decoded successfully
     */
    bool decode(const char* data, size_t size);

    /**
     * Get decoded trackables from last packet.
     * Map key is trackable ID.
     */
    const std::map<int, Trackable>& getTrackables() const { return trackables; }

    /**
     * Clear decoded trackables.
     */
    void clear() { trackables.clear(); }

private:
    bool parsePacket(const uint8_t* data, size_t size);
    bool parseTrackable(const uint8_t* data, size_t size, size_t& offset);
    bool parseCentroid(const uint8_t* data, size_t size, size_t& offset, Trackable& t);
    bool parseQuaternion(const uint8_t* data, size_t size, size_t& offset, Trackable& t);
    bool parseEuler(const uint8_t* data, size_t size, size_t& offset, Trackable& t);

    // Byte-order aware reading helpers
    double readDouble(const uint8_t* data, bool swap) const;
    uint32_t readUint32(const uint8_t* data, bool swap) const;
    uint16_t readUint16(const uint8_t* data, bool swap) const;

    std::map<int, Trackable> trackables;
    bool needsByteSwap = false;
};

} // namespace RTTrP
